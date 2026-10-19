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

    return { p.begin(), p.end() };
}

BabushkaProcessor::BabushkaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BABUSHKA", createLayout())
{
}

bool BabushkaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

static int oversamplingFromChoice (int idx) { return idx == 0 ? 1 : (idx == 2 ? 4 : 2); }

void BabushkaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentOversampling = oversamplingFromChoice ((int) apvts.getRawParameterValue (ids::oversampling)->load());
    engine.prepare (sampleRate, currentOversampling);
    engine.setParams (readParams());
    mono.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
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
    p.glide = f (ids::glide); p.glideType = i (ids::glideType); p.legatoGlide = f (ids::legatoGlide) > 0.5f;
    p.bendRange = i (ids::bendRange); p.notePriority = i (ids::notePriority);
    p.fineTune = f (ids::fineTune); p.drift = f (ids::drift);
    return p;
}

void BabushkaProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn()) engine.noteOn (m.getNoteNumber());
    else if (m.isNoteOff()) engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel()) engine.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (cc == 1) engine.setModWheelCC ((float) v / 127.0f);
        else if (cc == 64) engine.setSustain (v >= 64);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
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
    }
    if ((int) mono.size() < numSamples) mono.resize ((size_t) numSamples);

    engine.setParams (readParams());

    // Sample-accurate MIDI: render up to each event, then apply it.
    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, numSamples, meta.samplePosition);
        if (at > pos) { engine.process (mono.data() + pos, at - pos); pos = at; }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples) engine.process (mono.data() + pos, numSamples - pos);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, mono.data(), numSamples);
}

juce::AudioProcessorEditor* BabushkaProcessor::createEditor() { return new BabushkaEditor (*this); }

void BabushkaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void BabushkaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BabushkaProcessor(); }
