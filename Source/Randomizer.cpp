// Babushka — RANDOMIZE: new sounds from musical recipes, checked by a test render before they are loaded.
//
// A random knob setting on a synth is mostly silence, mud or a whistle. So a sound is built from a recipe for its kind
// (bass, lead, pad, ...): every value is drawn from a range that suits the kind, a few rules keep the pieces together
// (a closed filter needs envelope, PWM needs a pulse wave, an arpeggio needs a short envelope), the arpeggiator,
// sequencer and cables come only from known-good combinations. Then the sound is rendered offline with the same DSP
// engine: silent, clipping, non-finite, hiss-only or (for held sounds) dying notes are rejected and drawn again, and
// the volume of the one that passes is set so it plays as loud as the factory presets.
#include "PluginProcessor.h"

namespace
{
using Kind = BabushkaProcessor::RandomKind;

struct Range { float lo, hi; };

// One kind of sound: ranges in real parameter units.
struct Recipe
{
    std::vector<int> osc1Oct, osc2Oct;          // octave choice indices
    Range cutoff, resonance, envAmt, attack, decay, sustain, release, reverb, glide;
    std::vector<int> kbdTrack;
    float noiseChance, lfoChance, arpChance, seqChance, cableChance, droneChance;
    int note;                                    // audition note (single key)
};

const Recipe& recipe (Kind k)
{
    static const Recipe bass  { { 0, 1 }, { 0, 1 }, { 0.25f, 0.45f }, { 0.05f, 0.6f }, { 0.25f, 0.6f }, { 0.0f, 0.05f }, { 0.3f, 0.6f },
                                { 0.15f, 0.6f }, { 0.15f, 0.35f }, { 0.0f, 0.12f }, { 0.2f, 0.35f }, { 1, 2 }, 0.15f, 0.2f, 0.12f, 0.08f, 0.3f, 0.0f, 48 };
    static const Recipe lead  { { 2, 2, 3 }, { 1, 2 }, { 0.45f, 0.7f }, { 0.1f, 0.5f }, { 0.15f, 0.45f }, { 0.0f, 0.15f }, { 0.45f, 0.7f },
                                { 0.5f, 0.9f }, { 0.3f, 0.5f }, { 0.1f, 0.3f }, { 0.2f, 0.4f }, { 1, 2 }, 0.1f, 0.6f, 0.08f, 0.0f, 0.3f, 0.0f, 60 };
    static const Recipe pad   { { 1, 2 }, { 1, 2 }, { 0.32f, 0.55f }, { 0.05f, 0.35f }, { 0.1f, 0.4f }, { 0.4f, 0.75f }, { 0.6f, 0.85f },
                                { 0.6f, 0.9f }, { 0.55f, 0.8f }, { 0.25f, 0.45f }, { 0.0f, 0.2f }, { 1 }, 0.25f, 0.8f, 0.0f, 0.0f, 0.35f, 0.15f, 53 };
    static const Recipe pluck { { 1, 2, 3 }, { 1, 2 }, { 0.3f, 0.5f }, { 0.1f, 0.5f }, { 0.4f, 0.7f }, { 0.0f, 0.02f }, { 0.3f, 0.55f },
                                { 0.0f, 0.1f }, { 0.3f, 0.5f }, { 0.15f, 0.45f }, { 0.0f, 0.1f }, { 2 }, 0.1f, 0.25f, 0.2f, 0.1f, 0.35f, 0.0f, 60 };
    static const Recipe keys  { { 2 }, { 1, 2 }, { 0.4f, 0.6f }, { 0.05f, 0.3f }, { 0.2f, 0.45f }, { 0.0f, 0.03f }, { 0.45f, 0.65f },
                                { 0.3f, 0.7f }, { 0.35f, 0.55f }, { 0.1f, 0.3f }, { 0.0f, 0.0f }, { 2 }, 0.05f, 0.3f, 0.08f, 0.0f, 0.3f, 0.0f, 60 };
    static const Recipe arp   { { 1, 2 }, { 1, 2 }, { 0.3f, 0.55f }, { 0.15f, 0.6f }, { 0.3f, 0.65f }, { 0.0f, 0.02f }, { 0.3f, 0.5f },
                                { 0.0f, 0.3f }, { 0.25f, 0.45f }, { 0.1f, 0.4f }, { 0.0f, 0.15f }, { 2 }, 0.1f, 0.4f, 1.0f, 0.45f, 0.3f, 0.0f, 48 };
    static const Recipe fx    { { 0, 1, 2, 3 }, { 0, 1, 2, 3 }, { 0.3f, 0.75f }, { 0.3f, 0.85f }, { -0.6f, 0.7f }, { 0.0f, 0.6f }, { 0.3f, 0.8f },
                                { 0.0f, 0.8f }, { 0.3f, 0.8f }, { 0.2f, 0.6f }, { 0.0f, 0.6f }, { 0, 1, 2 }, 0.45f, 0.85f, 0.12f, 0.05f, 0.6f, 0.25f, 60 };
    switch (k)
    {
        case Kind::Bass: return bass;   case Kind::Lead: return lead;   case Kind::Pad: return pad;
        case Kind::Pluck: return pluck; case Kind::Keys: return keys;   case Kind::ArpSeq: return arp;
        case Kind::Any: case Kind::Fx: break;
    }
    return fx;
}

// Cables the randomizer may add, with the knob settings that make each one musical. Patch.h text format.
struct SafeCable { const char* patch; std::vector<std::pair<const char*, float>> values; bool needsLfo, onlyFx; };
const std::vector<SafeCable>& safeCables()
{
    static const std::vector<SafeCable> list = {
        { "modWaveOut:osc1PwmIn", { { "osc1Wave", 2.0f } }, true, false },                                    // LFO PWM
        { "modShOut:attIn attOut:filterCutoffIn", { { "attenuator", 0.25f } }, true, false },                // S/H filter steps
        { "kbVelOut:attIn attOut:filterCutoffIn", { { "attenuator", 0.4f } }, false, false },                // velocity brightness
        { "envPosOut:attIn attOut:osc2PitchIn", { { "attenuator", 0.12f } }, false, false },                 // pitch blip on osc 2
        { "envPosOut:attIn attOut:osc1PwmIn", { { "attenuator", 0.5f }, { "osc1Wave", 2.0f } }, false, false }, // envelope PWM
        { "osc1WaveOut:osc2LinFmIn", {}, false, false },                                                    // FM bell / growl
        { "mixerOut:hpfIn hpfOut:filterIn", { { "hpCutoff", 0.35f } }, false, false },                        // thin, band-passed
        { "kbOut:modRateIn", {}, true, false },                                                              // LFO follows the keys
        { "modShOut:attIn attOut:osc1PitchIn", { { "attenuator", 0.3f } }, true, true },                      // random pitches
        { "modWaveOut:envTriggerIn", { { "sustain", 0.0f } }, true, true },                                  // rhythmic re-trigger
    };
    return list;
}

const char* kindName (Kind k)
{
    static const char* names[] = { "ANY", "BASS", "LEAD", "PAD", "PLUCK", "KEYS", "ARP/SEQ", "FX" };
    return names[(int) k];
}

// A 8 or 16 step sequence in a minor / pentatonic scale around a root, with rests, ties and accents.
juce::String randomSequence (juce::Random& r, int root)
{
    static const int minor[] = { 0, 2, 3, 5, 7, 8, 10 }, penta[] = { 0, 3, 5, 7, 10 };
    const bool usePenta = r.nextBool();
    const int len = r.nextBool() ? 16 : 8;
    juce::StringArray steps;
    for (int i = 0; i < len; ++i)
    {
        if (i > 0 && r.nextFloat() < 0.15f) { steps.add ("R"); continue; }
        const int deg = usePenta ? penta[r.nextInt (5)] : minor[r.nextInt (7)];
        int note = root + deg + (r.nextFloat() < 0.2f ? 12 : 0);
        if (i % 4 == 0 && r.nextFloat() < 0.5f) note = root;          // keep coming home to the root
        juce::String t (note);
        if (r.nextFloat() < 0.1f) t << "T";
        if (r.nextFloat() < 0.25f) t << "A";
        steps.add (t);
    }
    return steps.joinIntoString (" ");
}
}

juce::StringArray BabushkaProcessor::randomKindNames()
{
    juce::StringArray s;
    for (int i = 0; i <= (int) RandomKind::Fx; ++i) s.add (kindName ((RandomKind) i));
    return s;
}

// ---- test render ----------------------------------------------------------------------------------------------------
BabushkaProcessor::Audition BabushkaProcessor::auditionState (const juce::XmlElement& state, double sampleRate)
{
    std::map<juce::String, float> values;
    for (auto* c : state.getChildIterator())
        if (c->hasAttribute ("id")) values[c->getStringAttribute ("id")] = (float) c->getDoubleAttribute ("value");
    auto get = [&values] (const char* id) { auto it = values.find (id); return it != values.end() ? it->second : 0.0f; };

    auto engine = std::make_unique<babushka::dsp::Engine>();
    engine->prepare (sampleRate, 1);
    auto p = paramsFromState (get);
    p.drift = 0.0f;
    engine->setParams (p);
    engine->setPatch (babushka::dsp::PatchState::fromString (state.getStringAttribute ("patch").toStdString()));
    for (int i = 0; i < 3; ++i)
        engine->arpSeq().sequences()[(size_t) i] = sequenceFromString (state.getStringAttribute ("seq" + juce::String (i + 1)));
    auto a = arpParamsFromState (get);
    a.sync = false;
    engine->setArpParams (a);
    engine->setTransport (false, 0.0, 120.0);

    const bool arp = a.play && a.mode == 0, seq = a.play && a.mode == 1;
    const int note = state.getIntAttribute ("auditionNote", 53);
    std::vector<int> keys;
    if (arp) keys = { note, note + 3, note + 7 };
    else if (! seq) keys = { note };

    const int fs = (int) sampleRate, block = 64;
    const int holdEnd = (int) ((arp || seq ? 2.0 : 1.2) * fs), total = holdEnd + (int) (0.4 * fs);
    std::vector<float> y ((size_t) total, 0.0f);
    for (int k : keys) engine->noteOn (k, 100);
    for (int pos = 0; pos < total; pos += block)
    {
        if (pos >= holdEnd && pos < holdEnd + block) for (int k : keys) engine->noteOff (k);
        engine->process (y.data() + pos, std::min (block, total - pos));
    }

    Audition r;
    double heldSum = 0; int heldN = 0, crossings = 0;
    for (int i = 0; i < total; ++i)
    {
        const float v = y[(size_t) i];
        if (! std::isfinite (v)) { r.finite = false; return r; }
        r.peak = std::max (r.peak, (double) std::abs (v));
        if (i > (int) (0.6 * fs) && i < holdEnd) { heldSum += (double) v * v; ++heldN; if (i > 0 && (v >= 0) != (y[(size_t) i - 1] >= 0)) ++crossings; }
    }
    for (int from = 0; from + 2400 <= total; from += 1200)            // loudest 50 ms, as babushka_presetaudit
    {
        double t = 0;
        for (int i = from; i < from + 2400; ++i) t += (double) y[(size_t) i] * y[(size_t) i];
        r.loud = std::max (r.loud, std::sqrt (t / 2400.0));
    }
    r.held = std::sqrt (heldSum / std::max (1, heldN));
    r.zcr = 0.5 * crossings / (std::max (1, heldN) / sampleRate);
    return r;
}

// ---- RANDOMIZE ------------------------------------------------------------------------------------------------------
bool BabushkaProcessor::randomize (uint32_t seed)
{
    juce::Random r (seed != 0 ? (juce::int64) seed : juce::Time::currentTimeMillis() ^ ((juce::int64) randomCount * 2654435761LL));
    auto pickI = [&r] (const std::vector<int>& v) { return v[(size_t) r.nextInt ((int) v.size())]; };
    auto in = [&r] (Range g) { return g.lo + r.nextFloat() * (g.hi - g.lo); };
    auto chance = [&r] (float p) { return r.nextFloat() < p; };

    // Every parameter of the sound at its default; the global settings stay as the user has them.
    auto base = apvts.copyState();
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! keptOnLoad (rp->paramID))
            {
                auto child = base.getChildWithProperty ("id", rp->paramID);
                if (child.isValid()) child.setProperty ("value", rp->convertFrom0to1 (rp->getDefaultValue()), nullptr);
            }

    constexpr double kTargetLoud = 0.2;     // loudest 50 ms rms, about the median of the factory presets
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        const Kind kind = randomKind == Kind::Any ? (Kind) (1 + r.nextInt ((int) Kind::Fx)) : randomKind;
        const Recipe& rc = recipe (kind);
        auto state = base.createCopy();
        std::map<juce::String, float> v;

        // Oscillators: two waves, an interval that sounds good (unison detune, octave, fifth) or hard sync.
        v["osc1Octave"] = (float) pickI (rc.osc1Oct);
        v["osc2Octave"] = (float) pickI (rc.osc2Oct);
        v["osc1Wave"] = (float) r.nextInt (4);
        v["osc2Wave"] = (float) r.nextInt (4);
        const bool sync = (kind == Kind::Lead || kind == Kind::Fx) && chance (0.3f);
        if (sync) { v["osc2Sync"] = 1.0f; v["osc2Freq"] = 2.0f + r.nextFloat() * 5.0f; }
        else
        {
            const float roll = r.nextFloat();
            v["osc2Freq"] = roll < 0.6f ? (chance (0.5f) ? 1.0f : -1.0f) * (0.03f + r.nextFloat() * 0.12f)
                          : roll < 0.8f ? 7.0f + (r.nextFloat() - 0.5f) * 0.06f
                          : roll < 0.9f ? -5.0f : 0.0f;
        }
        v["mixOsc1"] = 0.55f + r.nextFloat() * 0.35f;
        v["mixOsc2"] = chance (0.2f) && ! sync ? 0.0f : 0.3f + r.nextFloat() * 0.5f;
        if (sync) v["mixOsc2"] = std::max (v["mixOsc2"], 0.6f);
        v["mixNoise"] = chance (rc.noiseChance) ? 0.05f + r.nextFloat() * (kind == Kind::Fx ? 0.6f : 0.15f) : 0.0f;

        // Filter and envelope; a dark filter always gets enough envelope to open.
        v["cutoff"] = in (rc.cutoff);
        v["resonance"] = in (rc.resonance);
        v["kbdTrack"] = (float) pickI (rc.kbdTrack);
        v["envAmt"] = in (rc.envAmt);
        if (v["cutoff"] < 0.38f) v["envAmt"] = std::max (v["envAmt"], 0.3f);
        v["attack"] = in (rc.attack); v["decay"] = in (rc.decay); v["sustain"] = in (rc.sustain); v["release"] = in (rc.release);
        v["reverbMix"] = in (rc.reverb);
        v["vcaMode"] = chance (rc.droneChance) ? 2.0f : 0.0f;
        if (chance (0.3f)) { v["glide"] = in (rc.glide); v["legatoGlide"] = chance (0.5f) ? 1.0f : 0.0f; }

        // Mod oscillator: vibrato for leads, slow sweeps for pads, anything for FX.
        const bool lfo = chance (rc.lfoChance);
        if (lfo)
        {
            v["lfoWave"] = (float) r.nextInt (4);
            v["modWheel"] = 1.0f;
            if (kind == Kind::Lead) { v["lfoWave"] = 0.0f; v["lfoRate"] = 0.42f + r.nextFloat() * 0.05f; v["lfoPitchAmt"] = 0.02f + r.nextFloat() * 0.04f; v["modWheel"] = 0.3f + r.nextFloat() * 0.4f; }
            else if (kind == Kind::Pad) { v["lfoRate"] = r.nextFloat() * 0.25f; if (chance (0.5f)) v["lfoCutoffAmt"] = 0.1f + r.nextFloat() * 0.2f; else { v["lfoPwAmt"] = 0.2f + r.nextFloat() * 0.25f; v["osc1Wave"] = 2.0f; } }
            else if (kind == Kind::Fx) { v["lfoRate"] = r.nextFloat() * 0.7f; v["lfoCutoffAmt"] = r.nextFloat() * 0.4f; if (chance (0.4f)) v["lfoPitchAmt"] = r.nextFloat() * 0.3f; }
            else { v["lfoRate"] = 0.2f + r.nextFloat() * 0.35f; v["lfoCutoffAmt"] = 0.08f + r.nextFloat() * 0.2f; }
        }

        // Arpeggiator / sequencer: needs a short envelope so the steps are heard.
        int auditionNote = rc.note;
        if (chance (rc.arpChance))
        {
            v["arpPlay"] = 1.0f;
            v["arpRate"] = babushka::dsp::ArpSeq::knobForBpm (90.0f + r.nextFloat() * 50.0f);
            v["arpFreeStep"] = chance (0.7f) ? 2.0f : 1.0f;
            v["sustain"] = std::min (v["sustain"], 0.3f);
            v["decay"] = std::min (v["decay"], 0.55f);
            v["attack"] = std::min (v["attack"], 0.05f);
            v["vcaMode"] = 0.0f;
            if (chance (rc.seqChance))
            {
                v["arpMode"] = 1.0f;
                state.setProperty ("seq1", randomSequence (r, kind == Kind::Bass ? 36 : 48), nullptr);
            }
            else
            {
                v["arpMode"] = 0.0f;
                v["arpDirection"] = (float) r.nextInt (3);
                v["arpOctSeq"] = (float) r.nextInt (kind == Kind::Bass ? 1 : 3);
                auditionNote = kind == Kind::Bass ? 36 : 48;
            }
        }

        // Cables: at most two, from the known-good list.
        juce::StringArray patch;
        if (chance (rc.cableChance))
        {
            const int n = chance (0.3f) ? 2 : 1;
            for (int tries = 0; tries < 8 && patch.size() < n; ++tries)
            {
                const auto& c = safeCables()[(size_t) r.nextInt ((int) safeCables().size())];
                if ((c.onlyFx && kind != Kind::Fx) || patch.contains (c.patch)) continue;
                const juce::String p (c.patch);
                // one cable per attenuator
                if (p.contains ("attIn") && patch.joinIntoString (" ").contains ("attIn")) continue;
                patch.add (p);
                for (auto& [id, val] : c.values) v[id] = val;
                if (c.needsLfo && ! lfo) { v["lfoRate"] = 0.3f + r.nextFloat() * 0.25f; }
            }
        }
        state.setProperty ("patch", patch.joinIntoString (" "), nullptr);

        v["volume"] = 0.5f;
        for (auto& [id, val] : v)
        {
            auto child = state.getChildWithProperty ("id", id);
            if (child.isValid()) child.setProperty ("value", val, nullptr);
            else jassertfalse;
        }
        state.setProperty ("auditionNote", auditionNote, nullptr);
        auto xml = state.createXml();
        if (xml == nullptr) continue;

        // Test render at volume 0.5, then the volume that brings it to the target loudness (gain = volume²).
        const auto au = auditionState (*xml, 48000.0);
        if (! au.finite || au.loud < 1.0e-4) continue;
        const double scale = std::min (kTargetLoud / au.loud, 4.0);  // gain factor; volume 1 is 4x volume 0.5
        const double volume = 0.5 * std::sqrt (scale);
        if (au.loud * scale < 0.08) continue;                          // too quiet even at full volume
        if (au.peak * scale > 0.9) continue;                           // would clip
        const bool heldSound = ! (v["arpPlay"] > 0.5f) && kind != Kind::Pluck && kind != Kind::Fx && v["sustain"] > 0.15f;
        if (heldSound && au.held < 0.2 * au.loud) continue;           // a held note must keep sounding
        if (kind != Kind::Fx && au.zcr > 3500.0) continue;            // hiss or whistle, not a note

        for (auto* c : xml->getChildIterator())
            if (c->getStringAttribute ("id") == "volume") c->setAttribute ("value", volume);
        xml->removeAttribute ("auditionNote");
        ++randomCount;
        xml->setAttribute ("presetName", "Random " + juce::String (kindName (kind)) + " " + juce::String (randomCount).paddedLeft ('0', 2));
        xml->setAttribute ("presetModified", 0);
        xml->setAttribute ("presetRandom", 1);
        restoreKeepingGlobals (*xml);
        currentPresetFile = juce::File();
        addHistoryStep ("Randomize " + juce::String (kindName (kind)));
        return true;
    }
    return false;
}
