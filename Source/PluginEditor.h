#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/SynthPanel.h"

namespace babushka::ui { class SettingsOverlay; class PresetOverlay; class HintBubble; class Backdrop; }

// The instrument: a skeuomorphic front panel drawn in a fixed logical size and scaled to the window.
class BabushkaEditor : public juce::AudioProcessorEditor, private babushka::ui::HintSink
{
public:
    explicit BabushkaEditor (BabushkaProcessor&);
    ~BabushkaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void showPresets (bool show);
    void showSettings (bool show);
    void setSkin (int index);                                       // index into babushka::ui::skins(); remembered per user
    babushka::ui::CableLayer& cableLayer() { return *cables; }     // for tests
    babushka::ui::SynthPanel& synthPanel() { return *panel; }

private:
    void showHint (juce::Component& from, const juce::String& text) override;
    void hideHint() override;
    void updateBackdrop();
    void buildPanel();
    static juce::PropertiesFile::Options uiSettingsOptions();

    BabushkaProcessor& processor;
    struct Content : public juce::Component { } content;           // logical size kW x kH, scaled into the window
    std::unique_ptr<babushka::ui::SynthPanel> panel;
    std::unique_ptr<babushka::ui::CableLayer> cables;
    std::unique_ptr<babushka::ui::HintBubble> bubble;
    std::unique_ptr<babushka::ui::Backdrop> backdrop;
    std::unique_ptr<babushka::ui::PresetOverlay> presets;
    std::unique_ptr<babushka::ui::SettingsOverlay> settings;
    juce::PropertiesFile uiSettings { uiSettingsOptions() };      // per-user look of the editor (skin)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabushkaEditor)
};
