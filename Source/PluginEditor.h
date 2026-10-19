#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Temporary patch bay: pick two jacks, press PATCH to plug a cable in; select a cable and press REMOVE to pull it out.
class PatchPanelComponent : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PatchPanelComponent (BabushkaProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();

private:
    BabushkaProcessor& processor;
    juce::ComboBox fromBox, toBox;
    juce::TextButton patchButton { "PATCH" }, removeButton { "REMOVE" }, clearButton { "CLEAR ALL" };
    juce::ListBox list;
    juce::Label hint;
    babushka::dsp::PatchState cached;

    int getNumRows() override { return cached.count; }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void deleteKeyPressed (int row) override { if (row >= 0) { processor.removeCable (row); refresh(); } }
    void fillBox (juce::ComboBox&, bool outputs);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatchPanelComponent)
};

// Preset browser: factory presets by category, then the user presets found in the preset folder.
// A click loads the preset (quick auditioning); SAVE stores the current sound under the typed name.
class PresetBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetBrowser (BabushkaProcessor&);
    void refresh();                              // rescan the user folder
    void loadRelative (int delta);               // previous / next preset (skips category headers)
    std::function<void()> onLoaded;
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    struct Row { enum Kind { Header, Factory, User } kind = Header; juce::String text; int factoryIndex = -1; juce::File file; };
    BabushkaProcessor& processor;
    std::vector<Row> rows;
    juce::ListBox list;
    juce::TextEditor nameEditor;
    juce::TextButton saveButton { "SAVE" }, deleteButton { "DELETE" }, folderButton { "OPEN FOLDER" };
    juce::Label description, status;
    bool suppress = false;

    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void deleteKeyPressed (int row) override;
    void loadRow (int row);
    void selectCurrentByName();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};

// Temporary functional panel: controls grouped by hardware module.
// The final skinned UI (own design, not a copy of the hardware panel) comes in a later phase.
class BabushkaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BabushkaEditor (BabushkaProcessor&);
    void showPresetsTab (bool show);
    ~BabushkaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Control
    {
        juce::String paramId;
        std::unique_ptr<juce::Component> widget;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAtt;
    };
    struct Section
    {
        juce::String title;
        std::vector<std::unique_ptr<Control>> controls;
        juce::Rectangle<int> bounds;
        int extraHeight = 0;                                       // room for non-parameter widgets
        std::function<void (juce::Rectangle<int>)> layoutExtra;
    };

    BabushkaProcessor& processor;
    std::vector<Section> sections;
    juce::LookAndFeel_V4 lnf;
    PatchPanelComponent patchPanel;
    PresetBrowser presetBrowser;
    juce::TextButton synthTab { "SYNTH" }, presetsTab { "PRESETS" }, prevButton { "<" }, nextButton { ">" };
    juce::Label presetLabel;
    bool presetsVisible = false;

    // ARP/SEQ step-entry buttons, tap tempo and status line.
    juce::TextButton tapButton { "TAP" }, tieButton { "TIE" }, restButton { "REST" }, accentButton { "ACCENT" };
    juce::Label arpInfo;
    juce::Array<double> tapTimes;

    void addSection (const juce::String& title, std::initializer_list<const char*> paramIds);
    void timerCallback() override;
    void handleTap();
};
