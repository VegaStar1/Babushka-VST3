#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Engine.h"
#include "Presets.h"

class BabushkaProcessor : public juce::AudioProcessor
{
public:
    BabushkaProcessor();
    ~BabushkaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Babushka"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Called from the editor (message thread).
    void requestRecRest()   { ++recRestRequests; }
    void requestRecTie()    { ++recTieRequests; }
    void requestRecAccent() { ++recAccentRequests; }
    void setArpTempoFromTap (double bpm);

    // Patch cables (message thread). The audio thread picks up changes at the start of the next block.
    babushka::dsp::PatchState getPatchCopy() const;
    bool addCable (babushka::dsp::Jack a, babushka::dsp::Jack b);
    void removeCable (int index);
    void clearCables();

    // Presets (message thread). Factory presets are compiled in; user presets are XML files in a folder.
    void applyFactoryPreset (int index);
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    bool deleteUserPreset (const juce::File& file);
    juce::Array<juce::File> getUserPresetFiles() const;
    juce::File getUserPresetDirectory() const;
    void setUserPresetDirectory (const juce::File& dir) { userPresetDir = dir; }   // tests / custom location
    juce::String getCurrentPresetName() const { return currentPresetName; }
    int getCurrentSequenceLength() const { return currentSeqLength.load(); }
    juce::String getArpRateText() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    babushka::dsp::Engine engine;
    std::vector<float> mono, ext;
    juce::SpinLock seqLock;
    juce::File userPresetDir;
    juce::String currentPresetName { "Init" };
    mutable juce::SpinLock patchLock;
    babushka::dsp::PatchState patchState;
    std::atomic<bool> patchDirty { true };
    std::array<babushka::dsp::Sequence, 3> pendingSequences {};   // last known sequences (for save / re-prepare)
    std::atomic<int> recRestRequests { 0 }, recTieRequests { 0 }, recAccentRequests { 0 }, currentSeqLength { 0 };
    int currentOversampling = 2;
    double currentSampleRate = 48000.0;

    std::unique_ptr<juce::XmlElement> createStateXml();
    void restoreFromXml (const juce::XmlElement&);
    void setSequences (const std::array<babushka::dsp::Sequence, 3>&);
    void handleMidi (const juce::MidiMessage& m);
    babushka::dsp::Params readParams() const;
    babushka::dsp::ArpSeqParams readArpParams() const;
    static juce::String sequenceToString (const babushka::dsp::Sequence&);
    static babushka::dsp::Sequence sequenceFromString (const juce::String&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabushkaProcessor)
};
