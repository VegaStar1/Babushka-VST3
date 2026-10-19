#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Temporary functional panel: controls grouped by hardware module.
// The final skinned UI (own design, not a copy of the hardware panel) comes in a later phase.
class BabushkaEditor : public juce::AudioProcessorEditor
{
public:
    explicit BabushkaEditor (BabushkaProcessor&);
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
    };

    BabushkaProcessor& processor;
    std::vector<Section> sections;
    juce::LookAndFeel_V4 lnf;

    void addSection (const juce::String& title, std::initializer_list<const char*> paramIds);
};
