#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Engine.h"
#include "Presets.h"
#include "PresetTags.h"

class BabushkaProcessor : public juce::AudioProcessor, private juce::AudioProcessorListener
{
public:
    BabushkaProcessor();
    ~BabushkaProcessor() override;

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

    // On-screen keyboard and wheels (message thread -> audio thread through a lock-free queue).
    void uiNoteOn (int note, int velocity);
    void uiNoteOff (int note);
    void setUiPitchBend (float normalised);                 // −1 … +1
    bool isNoteHeld (int note) const noexcept { return note >= 0 && note < 128 && ((heldBits[note >> 6].load() >> (note & 63)) & 1ull) != 0; }
    int getPatchVersion() const noexcept { return patchVersion.load(); }   // changes whenever the cables change
    void tapTempo();                                       // TAP button: three taps in a row set the free tempo

    // ---- Presets (message thread; PresetLibrary.cpp) -------------------------------------------------------
    // Factory presets are compiled in (Presets.h). The preset library is a folder (Documents/Babushka/Presets):
    //   Factory/<category>/<name>.babushka   copies of the factory presets, written once, for sharing and editing
    //   User/[<folder>/]<name>.babushka      the user's own presets, optionally in sub-folders
    // A .babushka file is the plugin state as XML plus the preset's tags. Loading a preset keeps the global settings.
    void applyFactoryPreset (int index);
    std::unique_ptr<juce::XmlElement> factoryPresetXml (int index) const;   // the state a factory preset sets
    juce::File getPresetRoot() const;
    juce::File getUserPresetDirectory() const { return getPresetRoot().getChildFile ("User"); }
    juce::File getFactoryPresetDirectory() const { return getPresetRoot().getChildFile ("Factory"); }
    void setPresetRoot (const juce::File& dir) { presetRoot = dir; }      // tests / custom location
    void ensurePresetLibrary();                      // creates the folders, writes the factory copies if missing
    // Writes Factory/<category>/<name>.babushka files into dir; existing files are kept unless overwrite is set.
    int writeFactoryPresetFiles (const juce::File& dir, bool overwrite = false) const;   // returns the number written

    juce::File userPresetFile (const juce::String& name, const juce::String& folder) const;
    juce::File saveUserPreset (const juce::String& name, const juce::String& folder = {}, const juce::StringArray& tags = {});
    bool loadUserPreset (const juce::File& file);
    bool deleteUserPreset (const juce::File& file);
    bool createUserFolder (const juce::String& folder);
    juce::StringArray getUserFolders() const;        // sub-folders of User, as relative paths ("Bass", "Track X/Leads")
    juce::Array<juce::File> getUserPresetFiles() const;
    int importPresets (const juce::Array<juce::File>& files, const juce::String& folder = {});   // copies into User
    bool exportCurrentSound (const juce::File& destination);
    juce::File duplicateCurrentSound();              // saves the current sound as "<name> copy" next to it (User)
    juce::String getCurrentPresetName() const { return currentPresetName; }
    juce::File getCurrentPresetFile() const { return currentPresetFile; }   // the user preset file, or none
    bool isSoundModified() const { return soundModified; }              // knobs or cables changed since the preset loaded
    bool isRandomSound() const { return randomSound; }                  // made by RANDOMIZE and not saved

    struct PresetRef
    {
        bool factory = true; int factoryIndex = -1; juce::File file;
        juce::String name, category, folder, description;   // category: factory category or "User"; folder: under User
        juce::StringArray tags;
    };
    // Factory presets (categories in alphabetical order, table order inside), then the user presets (by folder, name).
    std::vector<PresetRef> listPresets() const;
    void loadPreset (const PresetRef& ref);
    void stepPreset (int delta);                     // previous / next in that list (wraps around)

    // ---- History (message thread) ---------------------------------------------------------------------------
    // The last kMaxHistory states of the sound. A step is a finished knob / switch gesture, a cable, a preset load
    // or a RANDOMIZE; undo / redo restore the whole sound (knobs, cables, sequences, preset name).
    static constexpr int kMaxHistory = 20;
    bool canUndo() const { return historyPos > 0; }
    bool canRedo() const { return historyPos + 1 < (int) history.size(); }
    void undo() { jumpToHistory (historyPos - 1); }
    void redo() { jumpToHistory (historyPos + 1); }
    void jumpToHistory (int index);
    juce::StringArray getHistoryLabels() const;      // oldest first
    int getHistoryPosition() const { return historyPos; }
    void addHistoryStep (const juce::String& label);
    void resetHistory (const juce::String& label);

    // ---- Randomizer (message thread; Randomizer.cpp) --------------------------------------------------------
    enum class RandomKind { Any, Bass, Lead, Pad, Pluck, Keys, ArpSeq, Fx };
    static juce::StringArray randomKindNames();      // "ANY", "BASS", ... in enum order
    void setRandomKind (RandomKind k) { randomKind = k; }
    RandomKind getRandomKind() const { return randomKind; }
    // Makes a new sound, checks it with a test render and loads it (not saved). Returns false only if no attempt
    // passed the checks (the sound is then left as it was). seed 0 = from the clock.
    bool randomize (uint32_t seed = 0);
    struct Audition { double peak = 0, loud = 0, held = 0, zcr = 0; bool finite = true; };
    // Renders a state (1.5 s, a held note or chord) offline and measures it; public for the tests.
    static Audition auditionState (const juce::XmlElement& state, double sampleRate = 48000.0);
    int getCurrentSequenceLength() const { return currentSeqLength.load(); }
    juce::String getArpRateText() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    babushka::dsp::Engine engine;
    std::vector<float> mono, ext, euro, reverbOut;
    juce::SpinLock seqLock;
    struct UiNote { int note = 0; int velocity = 0; };      // velocity 0 = note off
    juce::AbstractFifo uiFifo { 256 };
    std::array<UiNote, 256> uiNotes {};
    std::atomic<float> uiBend { 0.0f };
    std::atomic<bool> uiBendDirty { false };
    std::atomic<uint64_t> heldBits[2] { 0, 0 };
    std::atomic<int> patchVersion { 0 };
    juce::Array<double> tapTimes;
    void setHeld (int note, bool on) noexcept
    {
        if (note < 0 || note > 127) return;
        if (on) heldBits[note >> 6].fetch_or (1ull << (note & 63)); else heldBits[note >> 6].fetch_and (~(1ull << (note & 63)));
    }
    juce::File presetRoot;
    juce::File currentPresetFile;                    // the user preset file the sound came from, if any
    struct TagCacheEntry { juce::Time modified; juce::StringArray tags; };
    mutable std::map<juce::String, TagCacheEntry> tagCache;
    juce::StringArray readPresetTags (const juce::File& file) const;
    juce::String currentPresetName { "Init" };
    bool soundModified = false, randomSound = false;
    RandomKind randomKind = RandomKind::Any;
    int randomCount = 0;
    std::vector<std::pair<juce::String, juce::String>> history;   // label, state XML
    int historyPos = -1;
    bool restoringHistory = false;
    juce::String gestureLabel;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override {}
    void audioProcessorChanged (juce::AudioProcessor*, const juce::AudioProcessorListener::ChangeDetails&) override {}
    void audioProcessorParameterChangeGestureEnd (juce::AudioProcessor*, int parameterIndex) override;
    void restoreKeepingGlobals (const juce::XmlElement&);
    void soundChanged (const juce::String& historyLabel);           // after a knob / cable change by the user
    mutable juce::SpinLock patchLock;
    babushka::dsp::PatchState patchState;
    std::atomic<bool> patchDirty { true };
    std::array<babushka::dsp::Sequence, 3> pendingSequences {};   // last known sequences (for save / re-prepare)
    std::atomic<int> recRestRequests { 0 }, recTieRequests { 0 }, recAccentRequests { 0 }, currentSeqLength { 0 };
    int currentOversampling = 2;
    double currentSampleRate = 48000.0;

    std::unique_ptr<juce::XmlElement> createStateXml();
    void restoreFromXml (const juce::XmlElement&);
    static babushka::dsp::Params paramsFromState (const std::function<float (const char*)>& value);
    static babushka::dsp::ArpSeqParams arpParamsFromState (const std::function<float (const char*)>& value);
    void setSequences (const std::array<babushka::dsp::Sequence, 3>&);
    void handleMidi (const juce::MidiMessage& m);
    babushka::dsp::Params readParams() const;
    babushka::dsp::ArpSeqParams readArpParams() const;
    static juce::String sequenceToString (const babushka::dsp::Sequence&);
    static babushka::dsp::Sequence sequenceFromString (const juce::String&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabushkaProcessor)
};
