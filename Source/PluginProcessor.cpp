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

    // Pedals on the rear panel (plugin-only, see dsp/Pedals.h). Off by default: a preset without them sounds as before.
    // Knobs read 0 … 10 like the pedals' own scales; rates and the echo time in real units.
    // Each pedal starts at its first factory preset (PedalInfo.h): WARM TRIODE, VINTAGE SWIRL, MODE I, SLAPBACK.
    auto fxKnob = [&] (const char* id, const char* name, float def, std::function<String (float)> text) {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def,
                                                            AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); }))); };
    auto fx = [&] (const char* id, const char* name, float def) { fxKnob (id, name, def, [] (float v) { return String (v * 10.0f, 1); }); };
    auto hz = [] (float f) { return String (f, f < 1.0f ? 2 : 1) + " Hz"; };
    auto pct = [&] (const char* id, const char* name, float def) { fxKnob (id, name, def, [] (float v) { return String (juce::roundToInt (v * 100.0f)) + " %"; }); };
    auto db = [&] (const char* id, const char* name) {           // ±12 dB around the middle
        fxKnob (id, name, 0.5f, [] (float v) { const int d = juce::roundToInt ((v - 0.5f) * 24.0f); return (d > 0 ? "+" : "") + String (d) + " dB"; }); };
    auto onOff = [&] (const char* id, const char* name) { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, false)); };
    onOff ("satOn", "Saturator On");
    choice ("satMode", "Saturator Character", { "TRIODE", "TAPE", "TRANSFORMER", "DIODE" }, 0);
    fx ("satDrive", "Saturator Drive", 0.55f);   fx ("satBias", "Saturator Bias", 0.35f);
    fx ("satBody", "Saturator Body", 0.4f);    fx ("satTone", "Saturator Tone", 0.5f);
    pct ("satMix", "Saturator Mix", 1.0f);       db ("satOutput", "Saturator Output");
    onOff ("phaOn", "Phaser On");
    choice ("phaMode", "Phaser Mode", { "4-STAGE", "6-STAGE", "4 + COLOR", "8-STAGE" }, 0);
    fxKnob ("phaRate", "Phaser Rate", 0.3f, [hz] (float v) { return hz (babushka::dsp::pedallaw::phaserHz (v)); });
    fx ("phaDepth", "Phaser Depth", 0.8f);
    fx ("phaFeedback", "Phaser Color", 0.2f);  fx ("phaCenter", "Phaser Center", 0.5f);
    fx ("phaStereo", "Phaser Stereo", 0.3f);    pct ("phaMix", "Phaser Mix", 0.5f);
    onOff ("choOn", "Chorus On");
    choice ("choMode", "Chorus Mode", { "I", "II", "I + II", "VIBRATO", "ENSEMBLE" }, 0);
    fxKnob ("choRate", "Chorus Rate", 0.4236f, [hz] (float v) { return hz (babushka::dsp::pedallaw::chorusHz (v)); });
    fx ("choDepth", "Chorus Depth", 0.6f);
    pct ("choMix", "Chorus Mix", 0.5f);          fx ("choWidth", "Chorus Width", 1.0f);
    fx ("choNoise", "Chorus Noise", 0.25f);     fx ("choAge", "Chorus Age", 0.3f);
    onOff ("tapOn", "Tape Echo On");
    choice ("tapHeads", "Tape Echo Heads", { "1", "2", "3", "1+2", "2+3", "1+3", "1+2+3" }, 0);
    choice ("tapSync", "Tape Echo Sync", { "FREE", "1/4", "1/8", "1/8 D", "1/16" }, 0);
    fxKnob ("tapTime", "Tape Echo Time", 0.3736f, [] (float v) { return String (juce::roundToInt (1000.0f * babushka::dsp::pedallaw::echoSeconds (v))) + " ms"; });
    fx ("tapFeedback", "Tape Echo Feedback", 0.12f);
    pct ("tapMix", "Tape Echo Mix", 0.45f);      fx ("tapWow", "Tape Echo Wow", 0.15f);
    fx ("tapFlutter", "Tape Echo Flutter", 0.2f); fx ("tapAge", "Tape Echo Age", 0.3f);
    db ("tapBass", "Tape Echo Bass");     fxKnob ("tapTreble", "Tape Echo Treble", 0.55f, [] (float v) { const int d = juce::roundToInt ((v - 0.5f) * 24.0f); return (d > 0 ? "+" : "") + String (d) + " dB"; });
    onOff ("tapHold", "Tape Echo Hold");
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "fxOrder", 1 }, "Pedal Order", 0, 23, 0));

    return { p.begin(), p.end() };
}

BabushkaProcessor::BabushkaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Eurorack Out", juce::AudioChannelSet::mono(), false)
                                       .withOutput ("Reverb Out", juce::AudioChannelSet::mono(), false)
                                       .withInput ("Ext In", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "BABUSHKA", createLayout())
{
    addListener (this);
    resetHistory ("Init");
    midiMap.initialise();
}

BabushkaProcessor::~BabushkaProcessor() { removeListener (this); }

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
    return paramsFromState ([this] (const char* id) { return apvts.getRawParameterValue (id)->load(); });
}

babushka::dsp::ArpSeqParams BabushkaProcessor::readArpParams() const
{
    return arpParamsFromState ([this] (const char* id) { return apvts.getRawParameterValue (id)->load(); });
}

// The engine's parameters from parameter values in real units (live parameters, or a stored state for the randomizer).
babushka::dsp::Params BabushkaProcessor::paramsFromState (const std::function<float (const char*)>& f)
{
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
    auto& q = p.pedals;
    q.order = babushka::pedalOrderFromIndex (i ("fxOrder"));
    q.on = { f ("satOn") > 0.5f, f ("phaOn") > 0.5f, f ("choOn") > 0.5f, f ("tapOn") > 0.5f };
    q.satMode = i ("satMode"); q.satDrive = f ("satDrive"); q.satBias = f ("satBias"); q.satBody = f ("satBody");
    q.satTone = f ("satTone"); q.satMix = f ("satMix"); q.satOutput = f ("satOutput");
    q.phaMode = i ("phaMode"); q.phaRate = f ("phaRate"); q.phaDepth = f ("phaDepth"); q.phaFeedback = f ("phaFeedback");
    q.phaCenter = f ("phaCenter"); q.phaStereo = f ("phaStereo"); q.phaMix = f ("phaMix");
    q.choMode = i ("choMode"); q.choRate = f ("choRate"); q.choDepth = f ("choDepth"); q.choMix = f ("choMix");
    q.choWidth = f ("choWidth"); q.choNoise = f ("choNoise"); q.choAge = f ("choAge");
    q.tapHeads = i ("tapHeads"); q.tapSync = i ("tapSync"); q.tapTime = f ("tapTime"); q.tapFeedback = f ("tapFeedback");
    q.tapMix = f ("tapMix"); q.tapWow = f ("tapWow"); q.tapFlutter = f ("tapFlutter"); q.tapAge = f ("tapAge");
    q.tapBass = f ("tapBass"); q.tapTreble = f ("tapTreble"); q.tapHold = f ("tapHold") > 0.5f;
    return p;
}

babushka::dsp::ArpSeqParams BabushkaProcessor::arpParamsFromState (const std::function<float (const char*)>& f)
{
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
    if (ok) { patchDirty.store (true); ++patchVersion; soundChanged ("Cable " + juce::String (babushka::dsp::jackInfo (a).label) + " - " + babushka::dsp::jackInfo (b).label); }
    return ok;
}

void BabushkaProcessor::removeCable (int index)
{
    { const juce::SpinLock::ScopedLockType lock (patchLock); patchState.remove (index); }
    patchDirty.store (true);
    ++patchVersion;
    soundChanged ("Cable removed");
}

void BabushkaProcessor::clearCables()
{
    { const juce::SpinLock::ScopedLockType lock (patchLock); patchState.clear(); }
    patchDirty.store (true);
    ++patchVersion;
    soundChanged ("All cables removed");
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
    bendPosition.store (juce::jlimit (-1.0f, 1.0f, normalised));
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

// Recent samples of the main output, for the VU meter and the oscilloscope on the panel.
void BabushkaProcessor::meterBlock (const float* x, int n) noexcept
{
    int pos = scopePos.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i, ++pos)
        scope[(size_t) (pos & (kScopeSize - 1))].store (x[i], std::memory_order_relaxed);
    scopePos.store (pos & 0x3fffffff, std::memory_order_release);
}

void BabushkaProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn()) { engine.noteOn (m.getNoteNumber(), m.getVelocity()); setHeld (m.getNoteNumber(), true); }
    else if (m.isNoteOff()) { engine.noteOff (m.getNoteNumber()); setHeld (m.getNoteNumber(), false); }
    else if (m.isPitchWheel())
    {
        const float bend = juce::jlimit (-1.0f, 1.0f, (float) (m.getPitchWheelValue() - 8192) / 8192.0f);
        engine.setPitchBend (bend);
        bendPosition.store (bend);
    }
    // All Notes Off / All Sound Off are controllers (CC 123 / 120), so they must be tested before the generic CC branch.
    else if (m.isAllNotesOff() || m.isAllSoundOff()) { engine.allNotesOff(); heldBits[0] = 0; heldBits[1] = 0; }
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (midiMap.handleController (m.getChannel(), cc, v)) return;     // the user's knob mapping comes first
        if (cc == 1) { engine.setModWheelCC ((float) v / 127.0f); modWheelCC.store ((float) v / 127.0f); }
        else if (cc == 64) engine.setSustain (v >= 64);
    }
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
    if ((int) right.size() < numSamples) right.resize ((size_t) numSamples);
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
    if (echoClearRequests.exchange (0) > 0) engine.clearEcho();

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
                            euroOn ? euro.data() + pos : nullptr, reverbOn ? reverbOut.data() + pos : nullptr, right.data() + pos);
            pos = at;
        }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples)
        engine.process (mono.data() + pos, extPtr ? extPtr + pos : nullptr, numSamples - pos,
                        euroOn ? euro.data() + pos : nullptr, reverbOn ? reverbOut.data() + pos : nullptr, right.data() + pos);

    currentSeqLength.store (arp.currentLength());
    pendingSequences = arp.sequences();

    {   // main output (stereo after the pedals; a mono bus gets the sum) and the two optional mono outputs
        auto mainOut = getBusBuffer (buffer, false, 0);
        float* left = mono.data();
        if (mainOut.getNumChannels() >= 2)
        {
            mainOut.copyFrom (0, 0, left, numSamples);
            mainOut.copyFrom (1, 0, right.data(), numSamples);
            for (int ch = 2; ch < mainOut.getNumChannels(); ++ch) mainOut.copyFrom (ch, 0, left, numSamples);
        }
        juce::FloatVectorOperations::add (left, right.data(), numSamples);      // → the mid, for a mono bus and the meters
        juce::FloatVectorOperations::multiply (left, 0.5f, numSamples);
        if (mainOut.getNumChannels() == 1) mainOut.copyFrom (0, 0, left, numSamples);
        meterBlock (left, numSamples);
        if (const int overs = engine.takeOvers(); overs > 0) clipCount.fetch_add ((uint32_t) overs);
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

juce::AudioProcessorEditor* BabushkaProcessor::createEditor() { editorCreated = true; return new BabushkaEditor (*this); }

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
    state.setProperty ("presetModified", soundModified, nullptr);
    state.setProperty ("presetRandom", randomSound, nullptr);
    state.setProperty ("presetFile", currentPresetFile.getFullPathName(), nullptr);
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
    soundModified = (bool) tree.getProperty ("presetModified", false);
    randomSound = (bool) tree.getProperty ("presetRandom", false);
    currentPresetFile = tree.getProperty ("presetFile").toString().isNotEmpty() && juce::File::isAbsolutePath (tree.getProperty ("presetFile").toString())
                            ? juce::File (tree.getProperty ("presetFile").toString()) : juce::File();
    for (auto* key : { "tags", "category", "description", "auditionNote", "fxLocks" })   // preset-file metadata, not live state
        tree.removeProperty (key, nullptr);
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
    if (auto xml = createStateXml())
    {
        xml->setAttribute ("fxLocks", fxLocks.load());   // the project keeps the locks; presets and undo steps don't
        copyXmlToBinary (*xml, destData);
    }
}

void BabushkaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    setFxLocks (xml->getIntAttribute ("fxLocks", 0));

    // The standalone app saves the state when it quits and loads it back at start, before its window (and our editor)
    // exists. That is not a project: a new start of the app begins with Init, keeping only the global settings.
    // "Load a saved state..." from the app's menu comes later, with the window open, and restores the sound.
    if (wrapperType == wrapperType_Standalone && ! editorCreated)
    {
        for (auto* child : xml->getChildIterator())
        {
            const auto id = child->getStringAttribute ("id");
            if (id.isNotEmpty() && id != "fineTune" && babushka::isGlobalParam (id.toRawUTF8()))   // the app always starts in tune
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
                    rp->setValueNotifyingHost (rp->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
        }
        resetHistory ("Init");
        return;
    }

    // A project reopened in the DAW: the sound exactly as it was saved (globals included), with a fresh history.
    restoreFromXml (*xml);
    resetHistory ("Project: " + currentPresetName);
}

// ---- history ------------------------------------------------------------------------------------------------------
void BabushkaProcessor::resetHistory (const juce::String& label)
{
    history.clear();
    historyPos = -1;
    addHistoryStep (label);
}

void BabushkaProcessor::addHistoryStep (const juce::String& label)
{
    if (restoringHistory) return;
    auto xml = createStateXml();
    if (xml == nullptr) return;
    auto text = xml->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader());
    if (historyPos >= 0 && history[(size_t) historyPos].second == text) return;   // nothing changed
    history.resize ((size_t) (historyPos + 1));                                   // a new step drops the redo branch
    history.emplace_back (label, std::move (text));
    while ((int) history.size() > kMaxHistory) history.erase (history.begin());
    historyPos = (int) history.size() - 1;
}

void BabushkaProcessor::jumpToHistory (int index)
{
    if (index < 0 || index >= (int) history.size() || index == historyPos) return;
    if (auto xml = juce::parseXML (history[(size_t) index].second))
    {
        const juce::ScopedValueSetter<bool> svs (restoringHistory, true);
        restoreFromXml (*xml);
        historyPos = index;
    }
}

juce::StringArray BabushkaProcessor::getHistoryLabels() const
{
    juce::StringArray labels;
    for (auto& h : history) labels.add (h.first);
    return labels;
}

void BabushkaProcessor::soundChanged (const juce::String& label)
{
    if (restoringHistory) return;
    if (historyPos >= 0)
        if (auto xml = createStateXml())       // a control that only re-sent its value (e.g. when the editor opens) is not an edit
            if (xml->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader()) == history[(size_t) historyPos].second) return;
    soundModified = true;
    addHistoryStep (label);
}

void BabushkaProcessor::audioProcessorParameterChangeGestureEnd (juce::AudioProcessor*, int parameterIndex)
{
    if (! juce::MessageManager::existsAndIsCurrentThread()) return;
    auto* prm = dynamic_cast<juce::RangedAudioParameter*> (getParameters()[parameterIndex]);
    if (prm == nullptr) return;
    const auto label = prm->getName (40) + " " + prm->getCurrentValueAsText();
    if (babushka::isGlobalParam (prm->paramID.toRawUTF8())) addHistoryStep (label);   // settings: undoable, not an edit of the sound
    else soundChanged (label);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BabushkaProcessor(); }
