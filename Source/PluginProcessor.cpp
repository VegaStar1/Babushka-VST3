#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

namespace ids
{
    // Oscillators
    static constexpr const char* osc1Octave = "osc1Octave";
    static constexpr const char* osc1Wave   = "osc1Wave";
    static constexpr const char* osc2Octave = "osc2Octave";
    static constexpr const char* osc2Wave   = "osc2Wave";
    static constexpr const char* osc2Freq   = "osc2Freq";
    static constexpr const char* osc2Sync   = "osc2Sync";
    // Mixer
    static constexpr const char* mixOsc1  = "mixOsc1";
    static constexpr const char* mixOsc2  = "mixOsc2";
    static constexpr const char* mixNoise = "mixNoise";
    // Modulation
    static constexpr const char* lfoRate   = "lfoRate";
    static constexpr const char* lfoWave   = "lfoWave";
    static constexpr const char* lfoPitch  = "lfoPitchAmt";
    static constexpr const char* lfoCutoff = "lfoCutoffAmt";
    static constexpr const char* lfoPw     = "lfoPwAmt";
    static constexpr const char* modWheel  = "modWheel";
    // Filter
    static constexpr const char* cutoff    = "cutoff";
    static constexpr const char* resonance = "resonance";
    static constexpr const char* kbdTrack  = "kbdTrack";
    static constexpr const char* envAmt    = "envAmt";
    // Envelope
    static constexpr const char* attack  = "attack";
    static constexpr const char* decay   = "decay";
    static constexpr const char* sustain = "sustain";
    static constexpr const char* release = "release";
    // Output
    static constexpr const char* vcaMode   = "vcaMode";
    static constexpr const char* reverbMix = "reverbMix";
    static constexpr const char* reverbDecay = "reverbDecay";
    static constexpr const char* volume    = "volume";
    // Left-hand controller / global
    static constexpr const char* glide        = "glide";
    static constexpr const char* glideType    = "glideType";
    static constexpr const char* legatoGlide  = "legatoGlide";
    static constexpr const char* bendRange    = "bendRange";
    static constexpr const char* notePriority = "notePriority";
    static constexpr const char* fineTune     = "fineTune";
    static constexpr const char* drift        = "drift";
    static constexpr const char* oversampling = "oversampling";
    // External input
    static constexpr const char* extLevel = "extLevel";
    // Arpeggiator / sequencer
    static constexpr const char* arpPlay     = "arpPlay";
    static constexpr const char* arpHold     = "arpHold";
    static constexpr const char* arpMode     = "arpMode";
    static constexpr const char* arpDir      = "arpDirection";
    static constexpr const char* arpOctSeq   = "arpOctSeq";
    static constexpr const char* arpRate     = "arpRate";
    static constexpr const char* arpSync     = "arpSync";
    static constexpr const char* arpFreeStep = "arpFreeStep";
    // Utilities (patchable)
    static constexpr const char* hpCutoff   = "hpCutoff";
    static constexpr const char* attenuator = "attenuator";
    // Global clock settings (CLOCK IN / CLOCK OUT jacks)
    static constexpr const char* extClockMode = "extClockMode";
    static constexpr const char* extClockPpqn = "extClockPpqn";
    static constexpr const char* clockOutPpqn = "clockOutPpqn";
    // Global settings (manual p. 38)
    static constexpr const char* localOn    = "localOn";
    static constexpr const char* kbOutRange = "kbOutRange";
}

APVTS::ParameterLayout BabushkaProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto twoDecimals = [] (float v, int) { return String (v, 2); };
    auto knob = [&] (const char* id, const char* name, float def) {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def,
                                                            AudioParameterFloatAttributes().withStringFromValueFunction (twoDecimals))); };
    auto choice = [&] (const char* id, const char* name, StringArray items, int def) {
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };

    // Oscillator 1 / 2
    choice (ids::osc1Octave, "Osc 1 Octave", { "32'", "16'", "8'", "4'" }, 1);
    choice (ids::osc1Wave, "Osc 1 Wave", { "Triangle", "Sawtooth", "Square", "Narrow Pulse" }, 1);
    choice (ids::osc2Octave, "Osc 2 Octave", { "16'", "8'", "4'", "2'" }, 0);
    choice (ids::osc2Wave, "Osc 2 Wave", { "Triangle", "Sawtooth", "Square", "Narrow Pulse" }, 1);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::osc2Freq, 1 }, "Osc 2 Frequency",
                                                        NormalisableRange<float> (-7.0f, 7.0f), 0.0f,
                                                        AudioParameterFloatAttributes().withLabel ("st").withStringFromValueFunction (twoDecimals)));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::osc2Sync, 1 }, "Osc 2 Sync", false));

    // Mixer
    knob (ids::mixOsc1, "Mixer Osc 1", 0.6f);
    knob (ids::mixOsc2, "Mixer Osc 2", 0.0f);
    knob (ids::mixNoise, "Mixer Noise", 0.0f);

    // Modulation
    knob (ids::lfoRate, "Mod Rate", 0.45f);
    choice (ids::lfoWave, "Mod Wave", { "Sine", "Sawtooth", "Ramp", "Square" }, 0);
    knob (ids::lfoPitch, "Mod Pitch Amt", 0.0f);
    knob (ids::lfoCutoff, "Mod Cutoff Amt", 0.0f);
    knob (ids::lfoPw, "Mod Pulse Width Amt", 0.0f);
    knob (ids::modWheel, "Mod Wheel", 0.0f);

    // Filter
    knob (ids::cutoff, "Cutoff", 0.6f);
    knob (ids::resonance, "Resonance", 0.0f);
    choice (ids::kbdTrack, "Kbd Track", { "Off", "1:2", "1:1" }, 0);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::envAmt, 1 }, "Envelope Amt",
                                                        NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
                                                        AudioParameterFloatAttributes().withStringFromValueFunction (twoDecimals)));
    // Envelope
    knob (ids::attack, "Attack", 0.0f);
    knob (ids::decay, "Decay", 0.35f);
    knob (ids::sustain, "Sustain", 1.0f);
    knob (ids::release, "Release", 0.25f);

    // Output
    choice (ids::vcaMode, "VCA Mode", { "ENV", "KB RLS", "DRONE" }, 0);
    knob (ids::reverbMix, "Spring Reverb Mix", 0.0f);
    {   // plugin-only: length of the spring tail, 1 = the hardware tank
        NormalisableRange<float> range (0.3f, 2.5f);
        range.setSkewForCentre (1.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::reverbDecay, 1 }, "Spring Reverb Decay", range, 1.0f,
                                                            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
                                                                return "x" + String (v, 2) + (std::abs (v - 1.0f) < 0.005f ? " (stock)" : ""); })));
    }
    knob (ids::volume, "Volume", 0.8f);

    // Left-hand controller / global settings
    knob (ids::glide, "Glide", 0.0f);
    choice (ids::glideType, "Glide Type", { "LCR (constant rate)", "LCT (constant time)", "Exponential" }, 2);
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::legatoGlide, 1 }, "Legato Glide", false));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { ids::bendRange, 1 }, "Pitch Bend Range", 1, 12, 2));
    choice (ids::notePriority, "Note Priority", { "Last", "Low", "High" }, 0);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::fineTune, 1 }, "Fine Tune",
                                                        NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
                                                        AudioParameterFloatAttributes().withLabel ("st").withStringFromValueFunction (twoDecimals)));
    knob (ids::drift, "Analog Drift", 0.25f);
    choice (ids::oversampling, "Quality", { "Eco (1x)", "Normal (2x)", "High (4x)" }, 1);

    // Utilities
    knob (ids::hpCutoff, "High Pass", 0.0f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::attenuator, 1 }, "Attenuator",
                                                        NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
                                                        AudioParameterFloatAttributes().withStringFromValueFunction (twoDecimals)));

    // External input (rear INSTRUMENT IN → mixer). The level knob is plugin-only.
    knob (ids::extLevel, "Ext In Level", 0.5f);

    // Arpeggiator / sequencer
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::arpPlay, 1 }, "Arp/Seq Play", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::arpHold, 1 }, "Arp/Seq Hold", false));
    choice (ids::arpMode, "Arp/Seq Mode", { "ARP", "SEQ", "REC" }, 0);
    choice (ids::arpDir, "Arp/Seq Direction", { "ORDR", "FWD/BKWD", "RNDM" }, 0);
    choice (ids::arpOctSeq, "Oct / Seq", { "1", "2", "3" }, 0);
    knob (ids::arpRate, "Arp/Seq Rate", babushka::dsp::ArpSeq::knobForBpm (120.0f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::arpSync, 1 }, "Arp/Seq Host Sync", false));
    choice (ids::arpFreeStep, "Free Tempo Step", { "1/4", "1/8", "1/16" }, 1);
    choice (ids::extClockMode, "Clock In Mode", { "Clock", "Step-Advance" }, 0);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { ids::extClockPpqn, 1 }, "Clock In PPQN", 1, 48, 2));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { ids::clockOutPpqn, 1 }, "Clock Out PPQN", 1, 48, 2));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::localOn, 1 }, "Local On", true));
    choice (ids::kbOutRange, "KB Out Range", { "-5 to +5 V", "0 to +10 V" }, 0);

    return { p.begin(), p.end() };
}

BabushkaProcessor::BabushkaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Eurorack Out", juce::AudioChannelSet::mono(), false)
                                       .withOutput ("Reverb Out", juce::AudioChannelSet::mono(), false)
                                       .withInput ("Ext In", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "BABUSHKA", createLayout())
{
}

bool BabushkaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();
    bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    for (int b = 1; b < layouts.outputBuses.size(); ++b)    // Eurorack Out / Reverb Out: off, mono or stereo
    {
        const auto cs = layouts.getChannelSet (false, b);
        outOk = outOk && (cs.isDisabled() || cs == juce::AudioChannelSet::mono() || cs == juce::AudioChannelSet::stereo());
    }
    const bool inOk  = in.isDisabled() || in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    return outOk && inOk;
}

static int oversamplingFromChoice (int idx) { return idx == 0 ? 1 : (idx == 2 ? 4 : 2); }

void BabushkaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentOversampling = oversamplingFromChoice ((int) apvts.getRawParameterValue (ids::oversampling)->load());
    engine.prepare (sampleRate, currentOversampling);
    engine.setParams (readParams());
    patchDirty.store (true);          // re-send the cables to the freshly prepared engine
    mono.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    euro.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    reverbOut.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    ext.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    {
        const juce::SpinLock::ScopedLockType lock (seqLock);
        engine.arpSeq().sequences() = pendingSequences;   // sequences survive re-prepare
    }
}

babushka::dsp::Params BabushkaProcessor::readParams() const
{
    auto f = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto i = [&] (const char* id) { return (int) std::lround (f (id)); };
    babushka::dsp::Params p;
    p.osc1Octave = i (ids::osc1Octave); p.osc1Wave = i (ids::osc1Wave);
    p.osc2Octave = i (ids::osc2Octave); p.osc2Wave = i (ids::osc2Wave);
    p.osc2Freq = f (ids::osc2Freq); p.osc2Sync = f (ids::osc2Sync) > 0.5f;
    p.mixOsc1 = f (ids::mixOsc1); p.mixOsc2 = f (ids::mixOsc2); p.mixNoise = f (ids::mixNoise);
    p.lfoRate = f (ids::lfoRate); p.lfoWave = i (ids::lfoWave);
    p.lfoPitchAmt = f (ids::lfoPitch); p.lfoCutoffAmt = f (ids::lfoCutoff); p.lfoPwAmt = f (ids::lfoPw);
    p.modWheel = f (ids::modWheel);
    p.cutoff = f (ids::cutoff); p.resonance = f (ids::resonance); p.kbdTrack = i (ids::kbdTrack); p.envAmt = f (ids::envAmt);
    p.attack = f (ids::attack); p.decay = f (ids::decay); p.sustain = f (ids::sustain); p.release = f (ids::release);
    p.vcaMode = i (ids::vcaMode); p.reverbMix = f (ids::reverbMix); p.volume = f (ids::volume);
    p.reverbDecay = f (ids::reverbDecay);
    p.glide = f (ids::glide); p.glideType = i (ids::glideType); p.legatoGlide = f (ids::legatoGlide) > 0.5f;
    p.bendRange = i (ids::bendRange); p.notePriority = i (ids::notePriority);
    p.fineTune = f (ids::fineTune); p.drift = f (ids::drift);
    p.extLevel = f (ids::extLevel);
    p.hpCutoff = f (ids::hpCutoff); p.attenuator = f (ids::attenuator);
    p.localOn = f (ids::localOn) > 0.5f; p.kbOutRange = i (ids::kbOutRange);
    return p;
}

babushka::dsp::ArpSeqParams BabushkaProcessor::readArpParams() const
{
    auto f = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    babushka::dsp::ArpSeqParams a;
    a.play = f (ids::arpPlay) > 0.5f; a.hold = f (ids::arpHold) > 0.5f; a.sync = f (ids::arpSync) > 0.5f;
    a.mode = (int) std::lround (f (ids::arpMode)); a.direction = (int) std::lround (f (ids::arpDir));
    a.octSeq = (int) std::lround (f (ids::arpOctSeq)); a.rate = f (ids::arpRate);
    a.freeStep = (int) std::lround (f (ids::arpFreeStep));
    a.extStepAdvance = (int) std::lround (f (ids::extClockMode)) == 1;
    a.extPpqn = (int) std::lround (f (ids::extClockPpqn)); a.clockOutPpqn = (int) std::lround (f (ids::clockOutPpqn));
    return a;
}

babushka::dsp::PatchState BabushkaProcessor::getPatchCopy() const
{
    const juce::SpinLock::ScopedLockType lock (patchLock);
    return patchState;
}

bool BabushkaProcessor::addCable (babushka::dsp::Jack a, babushka::dsp::Jack b)
{
    bool ok;
    { const juce::SpinLock::ScopedLockType lock (patchLock); ok = patchState.add (a, b); }
    if (ok) { patchDirty.store (true); ++patchVersion; }
    return ok;
}

void BabushkaProcessor::removeCable (int index)
{
    { const juce::SpinLock::ScopedLockType lock (patchLock); patchState.remove (index); }
    patchDirty.store (true);
    ++patchVersion;
}

void BabushkaProcessor::clearCables()
{
    { const juce::SpinLock::ScopedLockType lock (patchLock); patchState.clear(); }
    patchDirty.store (true);
    ++patchVersion;
}

void BabushkaProcessor::uiNoteOn (int note, int velocity)
{
    int s1, n1, s2, n2;
    uiFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 + n2 < 1) return;
    uiNotes[(size_t) (n1 > 0 ? s1 : s2)] = { juce::jlimit (0, 127, note), juce::jlimit (1, 127, velocity) };
    uiFifo.finishedWrite (1);
}

void BabushkaProcessor::uiNoteOff (int note)
{
    int s1, n1, s2, n2;
    uiFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 + n2 < 1) return;
    uiNotes[(size_t) (n1 > 0 ? s1 : s2)] = { juce::jlimit (0, 127, note), 0 };
    uiFifo.finishedWrite (1);
}

void BabushkaProcessor::setUiPitchBend (float normalised)
{
    uiBend.store (juce::jlimit (-1.0f, 1.0f, normalised));
    uiBendDirty.store (true);
}

// TAP: at least three taps in a row set the free tempo (as on the hardware).
void BabushkaProcessor::tapTempo()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (! tapTimes.isEmpty() && now - tapTimes.getLast() > 2.0) tapTimes.clear();
    tapTimes.add (now);
    if (tapTimes.size() > 4) tapTimes.remove (0);
    if (tapTimes.size() >= 3)
    {
        const double avg = (tapTimes.getLast() - tapTimes.getFirst()) / (tapTimes.size() - 1);
        if (avg > 0.0) setArpTempoFromTap (60.0 / avg);
    }
}

juce::String BabushkaProcessor::getArpRateText() const
{
    const auto a = readArpParams();
    using babushka::dsp::ArpSeq;
    if (a.sync) return juce::String (ArpSeq::divisionName (ArpSeq::divisionIndex (a.rate))) + " (host)";
    static const char* steps[] = { "1/4", "1/8", "1/16" };
    return juce::String (ArpSeq::freeBpm (a.rate), 1) + " BPM, " + steps[juce::jlimit (0, 2, a.freeStep)];
}

void BabushkaProcessor::setArpTempoFromTap (double bpm)
{
    if (auto* prm = apvts.getParameter (ids::arpRate))
        prm->setValueNotifyingHost (babushka::dsp::ArpSeq::knobForBpm ((float) bpm));
}

// Sequences are stored as text: one token per step — note number, or R (rest); suffix T = tie, A = accent.
juce::String BabushkaProcessor::sequenceToString (const babushka::dsp::Sequence& s)
{
    juce::StringArray tokens;
    for (int i = 0; i < s.length; ++i)
    {
        const auto& st = s.steps[(size_t) i];
        juce::String t = st.rest ? "R" : juce::String ((int) st.note);
        if (st.tie) t << "T";
        if (st.accent) t << "A";
        tokens.add (t);
    }
    return tokens.joinIntoString (" ");
}

babushka::dsp::Sequence BabushkaProcessor::sequenceFromString (const juce::String& text)
{
    babushka::dsp::Sequence s;
    for (auto t : juce::StringArray::fromTokens (text, " ", ""))
    {
        if (t.isEmpty() || s.length >= (int) s.steps.size()) continue;
        babushka::dsp::SeqStep st;
        st.tie = t.containsChar ('T'); st.accent = t.containsChar ('A');
        st.rest = t.startsWithChar ('R');
        if (! st.rest) st.note = (uint8_t) juce::jlimit (0, 127, t.retainCharacters ("0123456789").getIntValue());
        s.steps[(size_t) s.length++] = st;
    }
    return s;
}

void BabushkaProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn()) { engine.noteOn (m.getNoteNumber(), m.getVelocity()); setHeld (m.getNoteNumber(), true); }
    else if (m.isNoteOff()) { engine.noteOff (m.getNoteNumber()); setHeld (m.getNoteNumber(), false); }
    else if (m.isPitchWheel()) engine.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (cc == 1) engine.setModWheelCC ((float) v / 127.0f);
        else if (cc == 64) engine.setSustain (v >= 64);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) { engine.allNotesOff(); heldBits[0] = 0; heldBits[1] = 0; }
}

void BabushkaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    const int wantedOs = oversamplingFromChoice ((int) apvts.getRawParameterValue (ids::oversampling)->load());
    if (wantedOs != currentOversampling)
    {
        // Quality change: re-prepare (rare, user-initiated; allocates only the small oversampling buffer).
        currentOversampling = wantedOs;
        engine.prepare (currentSampleRate, currentOversampling);
        patchDirty.store (true);
    }
    if ((int) mono.size() < numSamples) mono.resize ((size_t) numSamples);
    if ((int) euro.size() < numSamples) euro.resize ((size_t) numSamples);
    if ((int) reverbOut.size() < numSamples) reverbOut.resize ((size_t) numSamples);
    auto busEnabled = [this] (int index) { auto* b = getBus (false, index); return b != nullptr && b->isEnabled(); };
    const bool euroOn = busEnabled (1), reverbOn = busEnabled (2);

    if ((int) ext.size() < numSamples) ext.resize ((size_t) numSamples);

    // External input (sidechain bus) → mono, before the buffer is overwritten by the output.
    const float* extPtr = nullptr;
    {
        auto inBus = getBusBuffer (buffer, true, 0);
        const int inCh = getBus (true, 0) != nullptr && getBus (true, 0)->isEnabled() ? inBus.getNumChannels() : 0;
        if (inCh > 0)
        {
            juce::FloatVectorOperations::copy (ext.data(), inBus.getReadPointer (0), numSamples);
            for (int c = 1; c < inCh; ++c) juce::FloatVectorOperations::add (ext.data(), inBus.getReadPointer (c), numSamples);
            if (inCh > 1) juce::FloatVectorOperations::multiply (ext.data(), 1.0f / (float) inCh, numSamples);
            extPtr = ext.data();
        }
    }

    engine.setParams (readParams());

    // Cables edited in the UI: pick them up without ever blocking the audio thread.
    if (patchDirty.exchange (false))
    {
        const juce::SpinLock::ScopedTryLockType tryLock (patchLock);
        if (tryLock.isLocked()) engine.setPatch (patchState); else patchDirty.store (true);
    }

    // Host transport for synced ARP/SEQ.
    bool playing = false; double ppq = 0.0, bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }

    const juce::SpinLock::ScopedLockType lock (seqLock);
    engine.setArpParams (readArpParams());
    engine.setTransport (playing, ppq, bpm);
    auto& arp = engine.arpSeq();
    if (recRestRequests.exchange (0) > 0)   arp.recRest();
    if (recTieRequests.exchange (0) > 0)    arp.recTie();
    if (recAccentRequests.exchange (0) > 0) arp.recAccent();

    // Notes and pitch wheel from the on-screen controls.
    {
        int s1, n1, s2, n2;
        uiFifo.prepareToRead (uiFifo.getNumReady(), s1, n1, s2, n2);
        auto apply = [this] (int start, int count) {
            for (int i = start; i < start + count; ++i)
            {
                const auto& n = uiNotes[(size_t) i];
                if (n.velocity > 0) { engine.noteOn (n.note, n.velocity); setHeld (n.note, true); }
                else                { engine.noteOff (n.note); setHeld (n.note, false); }
            } };
        apply (s1, n1); apply (s2, n2);
        uiFifo.finishedRead (n1 + n2);
        if (uiBendDirty.exchange (false)) engine.setPitchBend (uiBend.load());
    }

    // Sample-accurate MIDI: render up to each event, then apply it.
    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, numSamples, meta.samplePosition);
        if (at > pos)
        {
            engine.process (mono.data() + pos, extPtr ? extPtr + pos : nullptr, at - pos,
                            euroOn ? euro.data() + pos : nullptr, reverbOn ? reverbOut.data() + pos : nullptr);
            pos = at;
        }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples)
        engine.process (mono.data() + pos, extPtr ? extPtr + pos : nullptr, numSamples - pos,
                        euroOn ? euro.data() + pos : nullptr, reverbOn ? reverbOut.data() + pos : nullptr);

    currentSeqLength.store (arp.currentLength());
    pendingSequences = arp.sequences();

    {   // main output (mono voice on every channel of bus 0) and the two optional mono outputs
        auto mainOut = getBusBuffer (buffer, false, 0);
        for (int ch = 0; ch < mainOut.getNumChannels(); ++ch) mainOut.copyFrom (ch, 0, mono.data(), numSamples);
        if (euroOn)
        {
            auto b = getBusBuffer (buffer, false, 1);
            for (int ch = 0; ch < b.getNumChannels(); ++ch) b.copyFrom (ch, 0, euro.data(), numSamples);
        }
        if (reverbOn)
        {
            auto b = getBusBuffer (buffer, false, 2);
            for (int ch = 0; ch < b.getNumChannels(); ++ch) b.copyFrom (ch, 0, reverbOut.data(), numSamples);
        }
    }
}

juce::AudioProcessorEditor* BabushkaProcessor::createEditor() { return new BabushkaEditor (*this); }

std::unique_ptr<juce::XmlElement> BabushkaProcessor::createStateXml()
{
    auto state = apvts.copyState();
    std::array<babushka::dsp::Sequence, 3> copy;
    {
        const juce::SpinLock::ScopedLockType lock (seqLock);   // short: plain copy only, no allocation
        copy = pendingSequences;
    }
    for (int i = 0; i < 3; ++i)
        state.setProperty ("seq" + juce::String (i + 1), sequenceToString (copy[(size_t) i]), nullptr);
    state.setProperty ("patch", juce::String (getPatchCopy().toString()), nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);
    return state.createXml();
}

void BabushkaProcessor::restoreFromXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (xml);
    std::array<babushka::dsp::Sequence, 3> loaded;
    for (int i = 0; i < 3; ++i)
        loaded[(size_t) i] = sequenceFromString (tree.getProperty ("seq" + juce::String (i + 1)).toString());
    setSequences (loaded);
    {
        const juce::SpinLock::ScopedLockType lock (patchLock);
        patchState = babushka::dsp::PatchState::fromString (tree.getProperty ("patch").toString().toStdString());
    }
    patchDirty.store (true);
    ++patchVersion;
    currentPresetName = tree.hasProperty ("presetName") ? tree.getProperty ("presetName").toString() : juce::String ("Init");
    apvts.replaceState (tree);
}

void BabushkaProcessor::setSequences (const std::array<babushka::dsp::Sequence, 3>& loaded)
{
    const juce::SpinLock::ScopedLockType lock (seqLock);
    pendingSequences = loaded;
    engine.arpSeq().sequences() = loaded;
}

void BabushkaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = createStateXml()) copyXmlToBinary (*xml, destData);
}

void BabushkaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes)) restoreFromXml (*xml);
}

// ---- presets ---------------------------------------------------------------------------------------------
void BabushkaProcessor::applyFactoryPreset (int index)
{
    const auto& all = babushka::factoryPresets();
    if (index < 0 || index >= (int) all.size()) return;
    const auto& pr = all[(size_t) index];

    // Back to defaults (except the global settings), then the preset's own values.
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! babushka::isGlobalParam (rp->paramID.toRawUTF8()))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, value] : pr.values)
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
        else jassertfalse;   // unknown parameter id in a factory preset

    {
        const juce::SpinLock::ScopedLockType lock (patchLock);
        patchState = babushka::dsp::PatchState::fromString (pr.patch);
    }
    patchDirty.store (true);
    ++patchVersion;

    if (pr.seq1[0] != 0 || pr.seq2[0] != 0 || pr.seq3[0] != 0)
    {
        std::array<babushka::dsp::Sequence, 3> seqs;
        { const juce::SpinLock::ScopedLockType lock (seqLock); seqs = pendingSequences; }
        const char* texts[3] = { pr.seq1, pr.seq2, pr.seq3 };
        for (int i = 0; i < 3; ++i) if (texts[i][0] != 0) seqs[(size_t) i] = sequenceFromString (texts[i]);
        setSequences (seqs);
    }
    currentPresetName = pr.name;
}

juce::File BabushkaProcessor::getUserPresetDirectory() const
{
    if (userPresetDir != juce::File())
        return userPresetDir;
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Babushka").getChildFile ("Presets");
}

juce::Array<juce::File> BabushkaProcessor::getUserPresetFiles() const
{
    auto files = getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.babushka");
    files.sort();
    return files;
}

bool BabushkaProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;
    auto dir = getUserPresetDirectory();
    if (! dir.createDirectory().wasOk()) return false;
    currentPresetName = name;
    auto xml = createStateXml();
    return xml != nullptr && xml->writeTo (dir.getChildFile (name + ".babushka"));
}

bool BabushkaProcessor::loadUserPreset (const juce::File& file)
{
    if (auto xml = juce::parseXML (file))
    {
        if (! xml->hasTagName (apvts.state.getType())) return false;
        restoreFromXml (*xml);
        currentPresetName = file.getFileNameWithoutExtension();
        return true;
    }
    return false;
}

std::vector<BabushkaProcessor::PresetRef> BabushkaProcessor::listPresets() const
{
    std::vector<PresetRef> list;
    const auto& all = babushka::factoryPresets();
    for (int i = 0; i < (int) all.size(); ++i) list.push_back ({ true, i, {}, all[(size_t) i].name, all[(size_t) i].category });
    for (auto& f : getUserPresetFiles()) list.push_back ({ false, -1, f, f.getFileNameWithoutExtension(), "User" });
    return list;
}

void BabushkaProcessor::loadPreset (const PresetRef& ref)
{
    if (ref.factory) applyFactoryPreset (ref.factoryIndex); else loadUserPreset (ref.file);
}

void BabushkaProcessor::stepPreset (int delta)
{
    const auto list = listPresets();
    if (list.empty()) return;
    int cur = -1;
    for (int i = 0; i < (int) list.size(); ++i) if (list[(size_t) i].name == currentPresetName) { cur = i; break; }
    const int n = (int) list.size();
    const int next = cur < 0 ? (delta > 0 ? 0 : n - 1) : ((cur + delta) % n + n) % n;
    loadPreset (list[(size_t) next]);
}

bool BabushkaProcessor::deleteUserPreset (const juce::File& file) { return file.getParentDirectory() == getUserPresetDirectory() && file.deleteFile(); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BabushkaProcessor(); }
