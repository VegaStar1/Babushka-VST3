// Babushka — monophonic voice + output section with the 41-point patch panel.
//
// Signal path (hard-wired "normalled" path of the hardware):
//   VCO1 + VCO2 (+sync) + Noise -> Mixer (overdrives) -> Ladder LPF -> VCA -> Spring reverb mix -> Volume
// Plugin-only: the rear panel's pedal board (Pedals.h) sits between the VCA and the reverb, as a stereo pair.
// Modulation: Mod Osc (LFO) -> pitch / cutoff / pulse width, depths scaled by the MOD wheel.
// The voice runs oversampled (2x/4x); reverb and output run at the host rate.
//
// Patching (see Patch.h): every module publishes its outputs in volts into outV[]; every input reads the
// sum of the outputs that are cabled to it, or its normalled internal source when nothing is plugged in.
// Modules run in a fixed order each oversampled sample, so a cable from a later module to an earlier one
// is delayed by one sample (like a real feedback patch, and stable). With no cables the signal path is
// exactly the hard-wired one above.
//
// Values marked "ASSUMPTION" are not specified in the manual and must be calibrated against
// recordings of the hardware.
#pragma once

#include "ArpSeq.h"
#include "Common.h"
#include "Decimator.h"
#include "Envelope.h"
#include "LadderFilter.h"
#include "Lfo.h"
#include "Oscillator.h"
#include "Patch.h"
#include "Pedals.h"
#include "SpringReverb.h"

#include <array>
#include <vector>

namespace babushka::dsp
{
// CUTOFF law, as printed on the panel: 20 Hz at the start, 200 Hz / 2 kHz at a third / two thirds, 20 kHz at the end.
// (Before: 10 Hz … 11 kHz. States saved with that law are converted when they load, see cutoffKnobFromLegacy.)
inline float cutoffKnobToHz (float knob) noexcept { return 20.0f * std::pow (1000.0f, knob); }
// The knob position that gives the frequency the old law (10 Hz · 1100^k) gave; linear, clamped at 20 Hz.
inline float cutoffKnobFromLegacy (float k) noexcept
{
    return clampf ((std::log (0.5f) + k * std::log (1100.0f)) / std::log (1000.0f), 0.0f, 1.0f);
}
constexpr float kCutoffDefault = 0.5079f;   // 663 Hz, the old default (0.6)

// Panel state. Knobs are normalised 0..1 unless noted; mapping to physical units lives in the engine.
struct Params
{
    // Oscillator 1: octave index 0..3 = 32',16',8',4'
    int   osc1Octave = 1;
    int   osc1Wave   = 1;            // Oscillator::Wave
    // Oscillator 2: octave index 0..3 = 16',8',4',2'
    int   osc2Octave = 0;
    int   osc2Wave   = 1;
    float osc2Freq   = 0.0f;         // semitones, ±7 (±24 extended)
    bool  osc2Sync   = false;

    // Mixer levels
    float mixOsc1 = 0.6f, mixOsc2 = 0.0f, mixNoise = 0.0f;
    float extLevel = 0.5f;          // plugin-only gain for the INSTRUMENT IN (hardware has none)

    // Modulation
    float lfoRate = 0.45f;           // knob → 0.07 Hz … 1.3 kHz
    int   lfoWave = 0;               // Lfo::Wave
    float lfoPitchAmt = 0.0f, lfoCutoffAmt = 0.0f, lfoPwAmt = 0.0f;
    float modWheel = 0.0f;

    // Filter
    float cutoff = kCutoffDefault;   // knob → 20 Hz … 20 kHz, a decade per third of the travel (CV can push it beyond)
    float resonance = 0.0f;
    int   kbdTrack = 0;              // 0 = off, 1 = 1:2, 2 = 1:1
    float envAmt = 0.0f;             // −1 … +1

    // Envelope
    float attack = 0.0f, decay = 0.35f, sustain = 1.0f, release = 0.25f;

    // Utilities (patchable)
    float hpCutoff = 0.0f;           // HIGH PASS knob: 16 Hz … 8 kHz (ASSUMPTION: range not in the manual)
    float attenuator = 0.0f;         // ATTENUATOR knob −1 … +1 (centre = fully attenuated; −1 = inverted)

    // Output
    int   vcaMode = 0;               // 0 = ENV, 1 = KB RLS, 2 = DRONE
    float reverbMix = 0.0f;
    float reverbDecay = 1.0f;        // extra: tail length relative to the hardware tank
    float volume = 0.8f;

    // Left-hand controller / globals
    float glide = 0.0f;
    int   glideType = 2;             // 0 = LCR, 1 = LCT, 2 = EXP
    bool  legatoGlide = false;
    int   bendRange = 2;             // semitones
    int   notePriority = 0;          // 0 = last, 1 = low, 2 = high
    float fineTune = 0.0f;           // semitones, ±1
    float drift = 0.25f;             // extra: analogue drift amount (0 = stable)
    // Global settings of the hardware (manual p. 38)
    bool  localOn = true;            // off: keyboard, pitch wheel and ARP/SEQ only drive KB OUT / GATE OUT / KB VEL OUT
    int   kbOutRange = 0;            // KB OUT voltage range: 0 = −5…+5 V, 1 = 0…+10 V
    bool  noiseFloor = false;        // plugin-only, host parameter without a control: the hardware's hiss in the pauses
    // Plugin-only: the pedals on the rear panel
    PedalParams pedals;
};

class Engine
{
public:
    void prepare (double hostSampleRate, int oversampling)
    {
        fsBase = (float) hostSampleRate;
        os = oversampling == 4 ? 4 : (oversampling == 1 ? 1 : 2);
        fsOs = fsBase * (float) os;

        decimator.prepare (os);
        filter.prepare (fsOs);
        env.prepare (fsOs);
        kbRls.prepare (fsOs);
        lfo.prepare (fsOs);
        reverb.prepare (fsBase);
        board.prepare (fsBase);
        arp.prepare (fsBase);
        // AC coupling: measured on a recorded square wave (flat tops sag ≈17 % over a half-period at 66 Hz)
        // the whole hardware chain behaves like one high-pass at ≈5.5 Hz; the two blockers add up to that.
        filterDc.setCutoff (1.5f, fsOs);
        outDc.setCutoff (4.0f, fsBase); outDcR.setCutoff (4.0f, fsBase);
        // Patch outputs marked "AC coupled" in the manual (oscillator, filter, high-pass and reverb outputs).
        for (auto& b : acOut) b.setCutoff (2.0f, fsOs);
        cutoffFeedHp.setCutoff (kCutoffFeedHz, fsOs);
        revOutDc.setCutoff (2.0f, fsBase);
        accentDecay = std::exp (-1.0f / (0.06f * fsOs));    // ASSUMPTION: accent envelope release ≈ 60 ms
        drift1.prepare (fsOs, 0xD1F7u); drift2.prepare (fsOs, 0xD2F7u);
        spreadRng.seed (0x5EEDu);
        noise.seed (0xBAB05CAu);
        floorNoise.seed (0xF100Du);

        for (auto* s : { &sCutoff, &sRes, &sMix1, &sMix2, &sMixN, &sEnvAmt, &sOsc2Freq,
                         &sLfoPitch, &sLfoCut, &sLfoPw, &sWheel, &sRevMix, &sVolume, &sLfoRate })
            s->setTime (0.012f, fsBase);
        firstBlock = true;

        osc1.reset (0.13f);
        osc2.reset (0.61f);
        osBuf.assign ((size_t) os, 0.0f);
        reset();
    }

    void reset()
    {
        filter.reset(); env.reset(); kbRls.reset(); lfo.reset(); reverb.reset(); decimator.reset();
        filterDc.reset(); outDc.reset(); outDcR.reset(); revOutDc.reset(); board.reset();
        for (auto& b : acOut) b.reset();
        hpf.reset(); cutoffFeedHp.reset();
        heldCount = 0; sustainPedal = false; gate = false; extPrev = 0.0f; spread = {};
        for (auto& h : held) h = { -1, false };
        std::fill (std::begin (outV), std::end (outV), 0.0f);
        trigHigh = syncHigh = clockHigh = resetHigh = false;
        velHeld = 0.0f; accentLevel = 0.0f; revAcc = 0.0f; revIn = 0.0f; clockOutHigh = false;
    }

    void setParams (const Params& p) noexcept { params = p; }

    // Compiled cables; cheap enough to call from the audio thread at the start of a block.
    void setPatch (const PatchState& p) noexcept { routing = Routing::compile (p); }
    const Routing& currentRouting() const noexcept { return routing; }
    // Samples since the last call at or over full scale, 0 dBFS (the CLIP lamp).
    int takeOvers() noexcept { const int o = overs; overs = 0; return o; }
    // Last voltage published on an output jack (for meters / tests).
    float outputVolts (Jack j) const noexcept { const int i = (int) j - kFirstOutput; return i >= 0 && i < kNumOutputs ? outV[i] : 0.0f; }

    void setArpParams (const ArpSeqParams& p)
    {
        const bool wasCapturing = arp.capturesKeys();
        arp.setParams (p);
        if (! wasCapturing && arp.capturesKeys()) releaseVoiceKeys();   // ARP/SEQ takes over the keyboard
    }
    void setTransport (bool playing, double ppq, double bpm) noexcept { arp.setTransport (playing, ppq, bpm); hostBpm = bpm; }
    ArpSeq& arpSeq() noexcept { return arp; }

    // ---- MIDI / keyboard --------------------------------------------------------------------
    // Keys go to the ARP/SEQ when it is running, otherwise straight to the voice (REC also records).
    void noteOn (int note, int velocity = 100)
    {
        noteVel[(size_t) (note & 127)] = (uint8_t) std::min (std::max (velocity, 1), 127);
        const bool captured = arp.capturesKeys();
        arp.keyOn (note);
        if (! captured) voiceOn (note);
    }

    void noteOff (int note)
    {
        const bool captured = arp.capturesKeys();
        arp.keyOff (note);
        if (! captured) voiceOff (note);
    }

    void voiceOn (int note)
    {
        drawNoteSpread();
        velHeld = 5.0f * (float) noteVel[(size_t) (note & 127)] / 127.0f;   // KB VEL OUT: 0 … +5 V
        removeHeld (note);
        if (heldCount < (int) held.size()) held[(size_t) heldCount++] = { note, false };
        updateKey (true);
    }

    void voiceOff (int note, bool pedalApplies = true)
    {
        for (int i = 0; i < heldCount; ++i)
            if (held[(size_t) i].note == note)
            {
                if (sustainPedal && pedalApplies) held[(size_t) i].sustained = true;
                else removeHeld (note);
                break;
            }
        updateKey (false);
    }

    void setSustain (bool down)
    {
        sustainPedal = down;
        if (! down)
        {
            for (int i = heldCount - 1; i >= 0; --i)
                if (held[(size_t) i].sustained) removeHeld (held[(size_t) i].note);
            updateKey (false);
        }
    }

    void setPitchBend (float normalised) noexcept { bend = clampf (normalised, -1.0f, 1.0f); }   // −1..1
    void setModWheelCC (float v) noexcept { ccWheel = clampf (v, 0.0f, 1.0f); }
    void allNotesOff() { heldCount = 0; sustainPedal = false; updateKey (false); arp.stopAll(); }
    void releaseVoiceKeys() { heldCount = 0; updateKey (false); }
    void clearEcho() noexcept { board.clearEcho(); }        // the tape echo's CLEAR

    // ---- Audio ------------------------------------------------------------------------------
    void process (float* out, int numSamples) noexcept { process (out, nullptr, numSamples); }

    // ext: optional INSTRUMENT IN signal (host rate), may be nullptr.
    // euroOut / reverbOut: optional extra outputs (host rate): EURORACK OUT (after the reverb MIX, before
    // VOLUME; ±1 ≈ ±5 V) and REVERB OUT (spring tank, always 100 % wet).
    // outR: the right channel; without it `out` gets the mono sum (the same as before the pedals with them all off).
    void process (float* out, const float* ext, int numSamples, float* euroOut = nullptr, float* reverbOut = nullptr,
                  float* outR = nullptr) noexcept
    {
        updateBlockConstants();
        ArpSeq::Event ev[4];
        for (int n = 0; n < numSamples; ++n)
        {
            servicePatchedControl();
            const int ne = arp.tick (ev);
            for (int e = 0; e < ne; ++e)
            {
                if (ev[e].type == 1)
                {
                    voiceOn (ev[e].note);
                    if (arp.sequencerRunning() && arp.lastStepAccented()) accentLevel = 1.0f;   // accent → KB VEL OUT
                }
                else voiceOff (ev[e].note, false);
            }

            tickSmoothers();
            // INSTRUMENT IN: preamp → mixer. Linear interpolation up to the oversampled rate.
            const float extNow = ext != nullptr ? ext[n] * extGain (params.extLevel) : 0.0f;
            revAcc = 0.0f;
            for (int i = 0; i < os; ++i)
            {
                const float t = (float) (i + 1) / (float) os;
                osBuf[(size_t) i] = tickVoice (extPrev + (extNow - extPrev) * t);
            }
            extPrev = extNow;
            const float voice = decimator.process (osBuf.data());

            // Output section: VCA out -> pedals -> reverb (normalled REVERB IN) -> dry/wet MIX -> VOLUME.
            // A cable in REVERB IN replaces the VCA output as the reverb source (averaged down to the host rate).
            float l = voice, r = voice;
            if (board.anyActive() || anyPedalJack())
            {
                const Jack inJacks[] = { Jack::SatIn, Jack::PhaIn, Jack::ChoIn, Jack::TapIn };
                for (int i = 0; i < kNumPedals; ++i)
                {
                    pedalJacks.inPatched[(size_t) i] = routing.inputPatched[(int) inJacks[i]];
                    if (pedalJacks.inPatched[(size_t) i]) pedalJacks.in[(size_t) i] = readIn (inJacks[i]) * 0.2f;
                }
                board.process (l, r, pedalJacks);
                const Jack outJacks[] = { Jack::SatOut, Jack::PhaOut, Jack::ChoOut, Jack::TapOut };
                for (int i = 0; i < kNumPedals; ++i) if (isUsed (outJacks[i])) setOut (outJacks[i], pedalJacks.out[(size_t) i] * 5.0f);
            }
            revIn = revAcc * (1.0f / (float) os);
            const float wet = reverb.process (routing.inputPatched[(int) Jack::ReverbIn] ? revIn : 0.5f * (l + r));
            const float mix = sRevMix.v;
            float y = outDc.process (l * (1.0f - mix) + wet * mix);
            float yR = outDcR.process (r * (1.0f - mix) + wet * mix);
            if (reverbOut != nullptr || isUsed (Jack::ReverbOut))
            {
                const float rv = revOutDc.process (wet);                                               // always 100 % wet, AC coupled
                if (reverbOut != nullptr) reverbOut[n] = rv;
                if (isUsed (Jack::ReverbOut)) setOut (Jack::ReverbOut, rv * 5.0f);
            }
            const float mono = 0.5f * (y + yR);
            if (euroOut != nullptr) euroOut[n] = mono;                                                 // after MIX, before VOLUME
            if (isUsed (Jack::EuroOut)) setOut (Jack::EuroOut, mono * 5.0f);
            const float gain = volumeGain (sVolume.v) * 1.6f;
            y *= gain; yR *= gain;
            if (params.noiseFloor) { const float h = kNoiseFloorRms * floorNoise.gaussianish(); y += h; yR += h; }
            if (std::abs (y) >= 1.0f || std::abs (yR) >= 1.0f) ++overs;     // 0 dBFS reached: CLIP
            if (outR != nullptr) { out[n] = outputGuard (y); outR[n] = outputGuard (yR); }
            else out[n] = outputGuard (0.5f * (y + yR));
        }
    }

private:
    struct HeldNote { int note; bool sustained; };

    Params params;
    float fsBase = 48000.0f, fsOs = 96000.0f;
    int os = 2;
    bool firstBlock = true;

    Oscillator osc1, osc2;
    LadderFilter filter;
    Envelope env, kbRls;
    Lfo lfo;
    SpringReverb reverb;
    ArpSeq arp;
    Decimator decimator;
    float extPrev = 0.0f;
    int overs = 0;
    DcBlocker filterDc, outDc, outDcR, revOutDc;
    PedalBoard board;
    PedalBoard::Jacks pedalJacks;
    double hostBpm = 120.0;
    DcBlocker acOut[3];                 // AC-coupled patch outputs: Osc 1, Osc 2, Filter
    OnePoleHP hpf;                      // Utilities: static 6 dB/oct high-pass
    OnePoleHP cutoffFeedHp;             // CV feedthrough of the ladder (envelope sweep leaking into the audio)
    Drift drift1, drift2;
    // Note-to-note spread: drawn on every note, scaled by Analog Drift (0 = every note identical).
    struct NoteSpread { float pitch1 = 0, pitch2 = 0, cutoff = 0, pw = 0, attack = 0, decay = 0, release = 0; };
    NoteSpread spread;
    Rng spreadRng;
    float spreadScale = 0.0f;
    Rng noise, floorNoise;
    std::vector<float> osBuf;

    Smoother sCutoff, sRes, sMix1, sMix2, sMixN, sEnvAmt, sOsc2Freq, sLfoPitch, sLfoCut, sLfoPw,
             sWheel, sRevMix, sVolume, sLfoRate;

    // Patch panel
    Routing routing;
    float outV[kNumOutputs] {};         // last voltage on every output jack
    bool trigHigh = false, syncHigh = false, clockHigh = false, resetHigh = false;
    bool clockOutHigh = false;
    float revAcc = 0.0f, revIn = 0.0f;
    std::array<uint8_t, 128> noteVel = [] { std::array<uint8_t, 128> a {}; a.fill (100); return a; }();
    float velHeld = 0.0f, accentLevel = 0.0f, accentDecay = 0.999f;

    // Keyboard
    std::array<HeldNote, 32> held {};
    int heldCount = 0;
    bool sustainPedal = false, gate = false;
    float targetPitch = 60.0f, currentPitch = 60.0f, glideRate = 0.0f;
    bool glideActive = false;
    float bend = 0.0f, ccWheel = 0.0f;

    // Block constants
    float glideCoeff = 1.0f, glideSeconds = 0.0f;

    void removeHeld (int note)
    {
        for (int i = 0; i < heldCount; ++i)
            if (held[(size_t) i].note == note)
            {
                for (int j = i; j < heldCount - 1; ++j) held[(size_t) j] = held[(size_t) j + 1];
                --heldCount;
                return;
            }
    }

    int priorityNote() const
    {
        if (heldCount == 0) return -1;
        int chosen = held[(size_t) heldCount - 1].note;
        if (params.notePriority == 1) { for (int i = 0; i < heldCount; ++i) chosen = std::min (chosen, held[(size_t) i].note); }
        else if (params.notePriority == 2) { for (int i = 0; i < heldCount; ++i) chosen = std::max (chosen, held[(size_t) i].note); }
        return chosen;
    }

    // Single triggering (as the hardware): the envelope only restarts when no key was held.
    void updateKey (bool fromNoteOn)
    {
        const int note = priorityNote();
        if (note < 0)
        {
            if (gate) { gate = false; if (! routing.inputPatched[(int) Jack::EnvTriggerIn]) env.gateOff(); kbRls.gateOff(); }
            return;
        }
        const bool legato = gate;
        const float newPitch = (float) note;
        const bool doGlide = glideSeconds > 0.0f && (! params.legatoGlide || legato);
        if (doGlide)
        {
            targetPitch = newPitch;
            glideActive = true;
            // LCT: constant time regardless of interval; LCR: constant rate (time per octave).
            glideRate = params.glideType == 1 ? std::abs (targetPitch - currentPitch) / std::max (1.0e-4f, glideSeconds * fsOs)
                                              : 12.0f / std::max (1.0e-4f, glideSeconds * fsOs);
        }
        else
        {
            targetPitch = currentPitch = newPitch;
            glideActive = false;
        }

        if (! gate && fromNoteOn)
        {
            gate = true;
            if (params.localOn)                                                     // Local Off: no hard-wired gate
            {
                if (! routing.inputPatched[(int) Jack::EnvTriggerIn]) env.gateOn();   // TRIGGER IN replaces the keyboard gate
                kbRls.gateOn();
            }
        }
    }

    // Each component (the two VCO cores, the ladder, the envelope's timing capacitors) lands a little differently on
    // every note: the panel and the keyboard CV are the same, the analogue parts are not. Ranges at the default
    // Analog Drift (0.25); a full knob gives about three times as much, 0 gives identical notes.
    // ASSUMPTION: the ranges are not measured on hardware (the recordings only bound the slow pitch drift).
    static constexpr float kSpreadCents = 1.5f;           // each VCO, ± cents
    // Ladder cutoff, ± octaves. Kept small: with the filter self-oscillating (KBD TRACK 1:1) this is a pitch error,
    // ±0.05 oct would put a whistle lead ±60 cents off the key. ±0.005 oct = ±6 cents at the default drift.
    static constexpr float kSpreadCutoffOct = 0.005f;
    static constexpr float kSpreadPw = 0.01f;             // pulse width, ±
    static constexpr float kSpreadEnvTime = 0.03f;        // attack / decay / release times, ± relative
    static float noteSpreadScale (float drift) noexcept   // 0 → 0, 0.25 → 1, 1 → 3
    {
        return drift <= 0.0f ? 0.0f : 3.0f * std::pow (drift, 0.7925f);
    }
    void drawNoteSpread() noexcept
    {
        spread = { spreadRng.bipolar(), spreadRng.bipolar(), spreadRng.bipolar(), spreadRng.bipolar(),
                   spreadRng.bipolar(), spreadRng.bipolar(), spreadRng.bipolar() };
    }

    // ---- Mappings (ASSUMPTION where noted) ---------------------------------------------------
    static float lfoRateHz (float knob) noexcept { return 0.07f * std::pow (1300.0f / 0.07f, clampf (knob, 0.0f, 1.0f)); }
    static float envTime (float knob, float minS) noexcept { return minS * std::pow (10.0f / minS, clampf (knob, 0.0f, 1.0f)); } // ASSUMPTION: min..10 s
    static float glideTime (float knob) noexcept { return knob <= 0.001f ? 0.0f : 0.002f * std::pow (2500.0f, knob); }         // ASSUMPTION: 2 ms..5 s
    static float mixerGain (float knob) noexcept { return 1.3f * std::pow (clampf (knob, 0.0f, 1.0f), 1.3f); }
    static float volumeGain (float knob) noexcept { return knob * knob; }
    // Noise floor of the recordings of the hardware: ≈ −65 … −70 dBFS. ASSUMPTION: white, after VOLUME, −68 dBFS rms.
    static constexpr float kNoiseFloorRms = 3.98e-4f;
    // Plugin-only: 0.5 ≈ a full-scale host signal hits the mixer like an oscillator at ~2 o'clock;
    // 1.0 drives the mixer into overdrive (the hardware preamp brings guitars to 10 Vpp).
    static float extGain (float knob) noexcept { return 2.0f * knob * knob * 2.0f; }

    // The output is not limited: like any instrument plugin it may go over 0 dBFS (the host's float mix does not clip;
    // the channel fader takes it down). Only a fault is caught: NaN / Inf become silence, and a runaway (a patch
    // feeding back on itself) is held at +12 dBFS so it cannot blast the speakers.
    static float outputGuard (float x) noexcept
    {
        if (! std::isfinite (x)) return 0.0f;
        return clampf (x, -4.0f, 4.0f);
    }

    // ---- patch helpers -----------------------------------------------------------------------
    bool isUsed (Jack out) const noexcept { return routing.outputUsed[(int) out - kFirstOutput]; }
    bool anyPedalJack() const noexcept
    {
        for (Jack j : { Jack::SatIn, Jack::PhaIn, Jack::ChoIn, Jack::TapIn }) if (routing.inputPatched[(int) j]) return true;
        for (Jack j : { Jack::SatOut, Jack::PhaOut, Jack::ChoOut, Jack::TapOut }) if (isUsed (j)) return true;
        return false;
    }
    void setOut (Jack out, float volts) noexcept { outV[(int) out - kFirstOutput] = std::isfinite (volts) ? clampf (volts, -16.0f, 16.0f) : 0.0f; }
    // Sum of everything cabled to an input (0 when nothing drives it).
    float readIn (Jack in) const noexcept
    {
        const int i = (int) in; float sum = 0.0f;
        for (int k = 0; k < routing.sourceCount[i]; ++k) sum += outV[routing.source[i][k]];
        return sum;
    }

    // ARP/SEQ rear-panel jacks, once per host sample.
    void servicePatchedControl() noexcept
    {
        const bool clockP = routing.inputPatched[(int) Jack::ClockIn], onOffP = routing.inputPatched[(int) Jack::OnOffIn],
                   resetP = routing.inputPatched[(int) Jack::ResetIn];
        if (clockP) { const bool hi = readIn (Jack::ClockIn) > 1.2f; if (hi && ! clockHigh) arp.externalClockEdge(); clockHigh = hi; }
        else clockHigh = false;
        if (resetP) { const bool hi = readIn (Jack::ResetIn) > 2.5f; if (hi && ! resetHigh) arp.resetPattern(); resetHigh = hi; }
        else resetHigh = false;
        const bool wasCapturing = arp.capturesKeys();
        arp.setExternalControl (clockP, onOffP, onOffP && readIn (Jack::OnOffIn) > 2.5f);
        if (! wasCapturing && arp.capturesKeys()) releaseVoiceKeys();
        if (isUsed (Jack::ClockOut)) setOut (Jack::ClockOut, arp.clockOutHigh() ? 5.0f : 0.0f);
    }

    void updateBlockConstants() noexcept
    {
        const auto& p = params;
        hpf.setCutoff (16.0f * std::pow (500.0f, clampf (p.hpCutoff, 0.0f, 1.0f)), fsOs);
        spreadScale = noteSpreadScale (p.drift);
        const float sp = spreadScale * kSpreadEnvTime;
        const float relS = envTime (p.release, 0.002f) * (1.0f + sp * spread.release);
        env.setParameters (envTime (p.attack, 0.001f) * (1.0f + sp * spread.attack),
                           envTime (p.decay, 0.002f) * (1.0f + sp * spread.decay), p.sustain, relS);
        // KB RLS: instant (1 ms anti-click) attack, full level while a key is held whatever SUSTAIN is, then the
        // envelope's release time (hardware: the VCA is open while the key is down, then follows RELEASE).
        kbRls.setParameters (0.001f, 0.001f, 1.0f, relS);
        lfo.setWave ((Lfo::Wave) p.lfoWave);
        osc1.setWave ((Oscillator::Wave) p.osc1Wave);
        osc2.setWave ((Oscillator::Wave) p.osc2Wave);
        reverb.setDecayScale (p.reverbDecay);
        {   // pedals: CV jacks move each pedal's main knob (±5 V = the whole knob), read once per block
            std::array<float, kNumPedals> cv {};
            const Jack cvJacks[] = { Jack::SatCv, Jack::PhaCv, Jack::ChoCv, Jack::TapCv };
            for (int i = 0; i < kNumPedals; ++i)
                if (routing.inputPatched[(int) cvJacks[i]]) cv[(size_t) i] = clampf (readIn (cvJacks[i]) * 0.2f, -1.0f, 1.0f);
            board.set (p.pedals, hostBpm, cv);
        }
        glideSeconds = glideTime (p.glide);
        glideCoeff = glideSeconds > 0.0f ? 1.0f - std::exp (-1.0f / (glideSeconds * 0.35f * fsOs)) : 1.0f;

        if (firstBlock)
        {
            sCutoff.snap (p.cutoff); sRes.snap (p.resonance); sMix1.snap (p.mixOsc1); sMix2.snap (p.mixOsc2);
            sMixN.snap (p.mixNoise); sEnvAmt.snap (p.envAmt); sOsc2Freq.snap (p.osc2Freq);
            sLfoPitch.snap (p.lfoPitchAmt); sLfoCut.snap (p.lfoCutoffAmt); sLfoPw.snap (p.lfoPwAmt);
            sWheel.snap (std::max (p.modWheel, ccWheel)); sRevMix.snap (p.reverbMix); sVolume.snap (p.volume);
            sLfoRate.snap (p.lfoRate);
            firstBlock = false;
        }
    }

    void tickSmoothers() noexcept
    {
        const auto& p = params;
        sCutoff.process (p.cutoff); sRes.process (p.resonance);
        sMix1.process (p.mixOsc1); sMix2.process (p.mixOsc2); sMixN.process (p.mixNoise);
        sEnvAmt.process (p.envAmt); sOsc2Freq.process (p.osc2Freq);
        sLfoPitch.process (p.lfoPitchAmt); sLfoCut.process (p.lfoCutoffAmt); sLfoPw.process (p.lfoPwAmt);
        sWheel.process (std::max (p.modWheel, ccWheel));
        sRevMix.process (p.reverbMix); sVolume.process (p.volume); sLfoRate.process (p.lfoRate);
    }

    // ASSUMPTION: linear FM depth is not given in the manual; 100 Hz per volt (±5 V audio → ±500 Hz).
    static constexpr float kLinFmHzPerVolt = 100.0f;
    // ASSUMPTION: ladder input level for a full-scale mixer (was 0.6). At 2.0 the fundamental of a 110 Hz saw at
    // mixer max through a 400 Hz cutoff is compressed ≈1.8 dB more than at 0.6 (≈0.5 dB at 12 o'clock); to calibrate
    // with a recording of one oscillator at mixer 25/50/75/100 %. The self-oscillation level does not depend on the
    // input, so against notes it is ≈10 dB quieter than at 0.6 (the self-oscillating presets were re-levelled).
    static constexpr float kLadderDrive = 2.0f;
    // CV feedthrough ("thump"): the hardware is not perfectly balanced, so its control signals leak into the audio.
    // Ladder: a fast envelope sweep of the cutoff unbalances the ladder's bias currents; modelled as the envelope's
    // share of the cutoff (in octaves) through a high-pass, added to the filter output. VCA: the control current
    // leaves a small offset at the output, proportional to the gain. Both pass the AC coupling after them, so a fast
    // attack gives a short low "thump" and the end of a note a slow return to zero with an undershoot (as seen in the
    // recordings of the hardware after the gate closes). Slow envelopes leak only a slow, inaudible DC shift.
    // ASSUMPTION: levels chosen for a thump ≈ −34 dB below a full note (no recording with the mixer at zero yet).
    static constexpr float kCutoffFeedPerOct = 0.0006f;    // ladder output per octave of envelope sweep
    static constexpr float kCutoffFeedHz = 20.0f;         // ASSUMPTION: feedthrough coupling corner
    static constexpr float kVcaOffset = 0.004f;           // VCA output offset at full gain
    // ASSUMPTION: PWM IN 0 … +5 V moves the duty cycle by up to +0.45 (matches the internal LFO path).
    static constexpr float kPwmPerVolt = 0.09f;

    float tickVoice (float extIn) noexcept
    {
        const auto& p = params;
        const auto& rt = routing;
        auto patched = [&rt] (Jack j) noexcept { return rt.inputPatched[(int) j]; };

        // Glide
        if (glideActive)
        {
            if (p.glideType == 2) currentPitch += glideCoeff * (targetPitch - currentPitch);
            else
            {
                const float d = targetPitch - currentPitch;
                currentPitch += clampf (d, -glideRate, glideRate);
            }
            if (std::abs (targetPitch - currentPitch) < 1.0e-4f) { currentPitch = targetPitch; glideActive = false; }
        }

        // Keyboard / ARP / SEQ outputs. KB OUT carries the glided pitch, 1 V/oct, 0 V = middle C (ASSUMPTION).
        accentLevel *= accentDecay;
        if (isUsed (Jack::GateOut)) setOut (Jack::GateOut, gate ? 8.0f : 0.0f);
        if (isUsed (Jack::KbOut))
        {
            const float v = (currentPitch - 60.0f) * (1.0f / 12.0f);
            setOut (Jack::KbOut, p.kbOutRange == 1 ? clampf (v + 5.0f, 0.0f, 10.0f) : clampf (v, -5.0f, 5.0f));
        }
        if (isUsed (Jack::KbVelOut)) setOut (Jack::KbVelOut, arp.sequencerRunning() ? 5.0f * accentLevel : velHeld);

        // Attenuator (bipolar; its input is normalled to +8 V, so unpatched it is a DC source).
        if (patched (Jack::AttenuatorIn) || isUsed (Jack::AttenuatorOut))
            setOut (Jack::AttenuatorOut, p.attenuator * (patched (Jack::AttenuatorIn) ? readIn (Jack::AttenuatorIn) : 8.0f));

        // Envelope trigger: any signal above +1.2 V is a trigger; it replaces the keyboard gate.
        if (patched (Jack::EnvTriggerIn))
        {
            const bool hi = readIn (Jack::EnvTriggerIn) > 1.2f;
            if (hi && ! trigHigh) env.gateOn(); else if (! hi && trigHigh) env.gateOff();
            trigHigh = hi;
        }
        else if (trigHigh) { trigHigh = false; if (! gate) env.gateOff(); }

        // Noise and Mod Osc
        const float whiteNoise = noise.gaussianish() * 0.45f;
        float lfoHz = lfoRateHz (sLfoRate.v);
        if (patched (Jack::ModRateIn)) lfoHz = std::min (lfoHz * std::exp2 (clampf (readIn (Jack::ModRateIn), -10.0f, 10.0f)), 0.45f * fsOs);   // 1 V/oct
        if (patched (Jack::ModSyncIn))
        {
            const bool hi = readIn (Jack::ModSyncIn) > 1.2f;
            if (hi && ! syncHigh) lfo.sync();
            syncHigh = hi;
        }
        else syncHigh = false;
        const float lfoOut = lfo.process (lfoHz, whiteNoise);
        if (isUsed (Jack::ModWaveOut)) setOut (Jack::ModWaveOut, lfoOut * 5.0f);
        if (isUsed (Jack::ModShOut))   setOut (Jack::ModShOut, clampf (lfo.sampleAndHold() * (2.5f / 0.45f), -5.0f, 5.0f));
        const float wheel = sWheel.v;
        const float pitchMod = lfoOut * sLfoPitch.v * sLfoPitch.v * wheel * 24.0f;  // ASSUMPTION: max ±2 oct
        const float cutMod   = lfoOut * sLfoCut.v * wheel * 5.0f;                    // ASSUMPTION: max ±5 oct
        const float pwMod    = lfoOut * sLfoPw.v * wheel * 0.45f;

        // Pitch: 8' = keyboard pitch. 32' … 4' / 16' … 2'. PITCH IN is summed with the keyboard CV (1 V/oct).
        // Local Off: the internal voice sees neither the keyboard CV nor the pitch wheel (0 V = middle C).
        const float kbCv = p.localOn ? currentPitch : 60.0f;
        const float basePitch = kbCv + (p.localOn ? bend * (float) p.bendRange : 0.0f) + p.fineTune + pitchMod;
        const float d = p.drift * 0.03f;  // up to ±3 cents at drift = 1
        const float spreadSemis = spreadScale * kSpreadCents * 0.01f;
        float pitch1 = basePitch + (float) (p.osc1Octave - 2) * 12.0f + d * drift1.process() + spreadSemis * spread.pitch1;
        float pitch2 = basePitch + (float) (p.osc2Octave - 1) * 12.0f + sOsc2Freq.v + d * drift2.process() + spreadSemis * spread.pitch2;
        if (patched (Jack::Osc1PitchIn)) pitch1 += 12.0f * readIn (Jack::Osc1PitchIn);
        if (patched (Jack::Osc2PitchIn)) pitch2 += 12.0f * readIn (Jack::Osc2PitchIn);

        // Narrow pulse widths measured on hardware ≈16 % (osc 1) and ≈18 % (osc 2) (SOS review).
        const float pwSpread = spreadScale * kSpreadPw * spread.pw;
        float pw1 = (p.osc1Wave == 3 ? 0.16f : Oscillator::kSquareDuty1) + pwMod + pwSpread;
        if (patched (Jack::Osc1PwmIn)) pw1 += kPwmPerVolt * readIn (Jack::Osc1PwmIn);
        osc1.setPulseWidth (pw1);
        osc2.setPulseWidth ((p.osc2Wave == 3 ? 0.18f : Oscillator::kSquareDuty2) + pwMod + pwSpread);

        bool wrapped = false; float wrapFrac = 0.0f;
        const float o1 = osc1.process (midiToHz (pitch1) / fsOs, wrapped, wrapFrac);
        float dt2 = midiToHz (pitch2) / fsOs;
        if (patched (Jack::Osc2LinFmIn)) dt2 += kLinFmHzPerVolt * readIn (Jack::Osc2LinFmIn) / fsOs;   // linear FM
        bool w2; float f2;
        const float o2 = (p.osc2Sync && wrapped) ? osc2.processSynced (dt2, wrapFrac) : osc2.process (dt2, w2, f2);
        if (isUsed (Jack::Osc1WaveOut)) setOut (Jack::Osc1WaveOut, acOut[0].process (o1) * 5.0f);
        if (isUsed (Jack::Osc2WaveOut)) setOut (Jack::Osc2WaveOut, acOut[1].process (o2) * 5.0f);

        // Mixer (CP3-style summing amp, limited headroom): gentle drive above ~1 o'clock,
        // overdrive near max; a loud input compresses the others. Slight asymmetry adds 2nd harmonic.
        // A cable in an input replaces that source (the knob then sets the level of the new signal).
        const float m1 = patched (Jack::MixOsc1In)  ? readIn (Jack::MixOsc1In) * 0.2f  : o1;
        const float m2 = patched (Jack::MixOsc2In)  ? readIn (Jack::MixOsc2In) * 0.2f  : o2;
        const float mn = patched (Jack::MixNoiseIn) ? readIn (Jack::MixNoiseIn) * 0.2f : whiteNoise;
        const float sum = m1 * mixerGain (sMix1.v) + m2 * mixerGain (sMix2.v) + mn * mixerGain (sMixN.v) + extIn;
        constexpr float drive = 1.2f, bias = 0.12f;
        const float mixed = (std::tanh (sum * drive + bias) - std::tanh (bias)) * (1.0f / drive);
        if (isUsed (Jack::MixerOut)) setOut (Jack::MixerOut, mixed * 5.0f);

        // Utilities: static 6 dB/oct high-pass (patch-only: nothing is normalled to its input).
        if (patched (Jack::HpfIn) || isUsed (Jack::HpfOut))
            setOut (Jack::HpfOut, hpf.process (patched (Jack::HpfIn) ? readIn (Jack::HpfIn) * 0.2f : 0.0f) * 5.0f);

        // Filter cutoff: knob 20 Hz … 20 kHz (the panel print); envelope amount bipolar ±6 oct (ASSUMPTION);
        // key tracking 0 / ½ / 1 oct per oct around middle C. CUTOFF IN: 1 V/oct; ENV AMT IN: ±5 V = ±1 on the knob.
        const float e = env.process();
        const float kb = kbRls.process();
        if (isUsed (Jack::EnvPosOut)) setOut (Jack::EnvPosOut, e * 8.0f);
        if (isUsed (Jack::EnvNegOut)) setOut (Jack::EnvNegOut, -e * 8.0f);
        const float track = p.kbdTrack == 0 ? 0.0f : (p.kbdTrack == 1 ? 0.5f : 1.0f);
        float envAmt = sEnvAmt.v;
        if (patched (Jack::FilterEnvAmtIn)) envAmt += 0.2f * readIn (Jack::FilterEnvAmtIn);
        const float envOctaves = envAmt * 6.0f * e;
        float octaves = sCutoff.v * std::log2 (1000.0f)
                        + envOctaves
                        + track * (kbCv - 60.0f) * (1.0f / 12.0f)
                        + cutMod
                        + spreadScale * kSpreadCutoffOct * spread.cutoff;
        if (patched (Jack::FilterCutoffIn)) octaves += readIn (Jack::FilterCutoffIn);
        const float cutoffHz = std::min (20.0f * std::exp2 (octaves), 24000.0f);
        // Resonance: self-oscillation starts near 89 % of the knob (k = 4).
        const float k = 4.5f * sRes.v;
        const float fcTuned = std::min (cutoffHz * LadderFilter::tuningCompensation (k), 24000.0f);

        // Drive scaling: mixer level ±1 ≈ kLadderDrive in ladder units (2·Vt). Hot enough that a mixer above
        // 12 o'clock also drives the ladder itself (the harmonics near and above the cutoff are compressed);
        // the output is scaled back, so the small-signal gain is unchanged.
        const float fIn = patched (Jack::FilterIn) ? readIn (Jack::FilterIn) * 0.2f : mixed;
        float y = filter.process (fIn * kLadderDrive, fcTuned, k) * (1.0f / kLadderDrive);
        y += kCutoffFeedPerOct * cutoffFeedHp.process (envOctaves);
        y = filterDc.process (y);
        if (isUsed (Jack::FilterOut)) setOut (Jack::FilterOut, acOut[2].process (y) * 5.0f);

        // VCA: gain from ENV / KB RLS / DRONE, mild output saturation and a small bleed when closed
        // (−66 dB; recordings of the hardware: −60 … −77 dB below the sounding note).
        // VCA AMT IN: in DRONE it alone sets the level (0 … +8 V); in ENV / KB RLS it multiplies the envelope.
        float gain = p.vcaMode == 0 ? e : (p.vcaMode == 1 ? kb : 1.0f);
        if (patched (Jack::VcaAmtIn))
        {
            const float c = readIn (Jack::VcaAmtIn) * (1.0f / 8.0f);
            gain = p.vcaMode == 2 ? std::max (c, 0.0f) : gain * c;
        }
        const float offset = kVcaOffset * gain;
        gain = gain + 5.0e-4f;
        const float vIn = patched (Jack::VcaIn) ? readIn (Jack::VcaIn) * 0.2f : y;
        const float v = vIn * gain * 1.25f + offset;
        if (patched (Jack::ReverbIn)) revAcc += readIn (Jack::ReverbIn) * 0.2f;
        return std::tanh (v * 0.8f) * (1.0f / 0.8f) * 0.8f;
    }
};
} // namespace babushka::dsp
