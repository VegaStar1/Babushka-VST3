#include "PluginEditor.h"
#include "ui/Overlays.h"

using namespace babushka::ui;

BabushkaEditor::BabushkaEditor (BabushkaProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    setOpaque (true);
    skinIndex() = juce::jlimit (0, (int) skins().size() - 1, uiSettings.getIntValue ("skin", 0));
    bubble = std::make_unique<HintBubble>();
    cables = std::make_unique<CableLayer> (p);
    cables->hints = this;
    cables->setBounds (0, 0, kW, kH);

    addAndMakeVisible (content);
    content.setBounds (0, 0, kW, kH);
    buildPanel();
    content.addAndMakeVisible (*cables);
    backdrop = std::make_unique<Backdrop>();
    backdrop->onClick = [this] { showPresets (false); showSettings (false); };
    backdrop->setBounds (0, 0, kW, kH);
    content.addChildComponent (*backdrop);
    content.addChildComponent (*bubble);

    setResizable (true, true);
    setResizeLimits (kW * 3 / 5, kH * 3 / 5, kW * 5 / 4, kH * 5 / 4);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize ((int) (kW * 0.8), (int) (kH * 0.8));
}

BabushkaEditor::~BabushkaEditor() = default;

juce::PropertiesFile::Options BabushkaEditor::uiSettingsOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Babushka";
    o.filenameSuffix = ".settings";
    o.folderName = "Babushka";
    o.osxLibrarySubFolder = "Application Support";
    o.commonToAllUsers = false;
    return o;
}

// The panel holds all of the printed artwork, so a change of skin simply builds a new one (cables keep their state).
void BabushkaEditor::buildPanel()
{
    if (panel != nullptr) content.removeChildComponent (panel.get());
    babushka::ui::HintSink& sink = *this;
    panel = std::make_unique<SynthPanel> (processor, sink,
                                          [this] { processor.stepPreset (-1); },
                                          [this] { processor.stepPreset (+1); },
                                          [this] { showPresets (true); },
                                          [this] { showSettings (true); });
    content.addAndMakeVisible (*panel, 0);
    cables->setJacks (panel->jackSites());
}

void BabushkaEditor::setSkin (int index)
{
    index = juce::jlimit (0, (int) skins().size() - 1, index);
    if (index == skinIndex()) return;
    skinIndex() = index;
    uiSettings.setValue ("skin", index);
    uiSettings.saveIfNeeded();
    hideHint();
    buildPanel();
    content.repaint();
}

void BabushkaEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff070707)); }



void BabushkaEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, kW, kH);
}

// Dialogs sit over the instrument on a dimmed backdrop.
void BabushkaEditor::showPresets (bool show)
{
    if (show && presets == nullptr)
    {
        presets = std::make_unique<PresetOverlay> (processor);
        presets->onClose = [this] { showPresets (false); };
        content.addChildComponent (*presets);
    }
    if (settings != nullptr && show) showSettings (false);
    if (presets != nullptr)
    {
        presets->setCentrePosition (kW / 2, kH / 2);
        presets->setVisible (show);
        if (show) presets->toFront (true);
    }
    updateBackdrop();
}

void BabushkaEditor::updateBackdrop()
{
    const bool on = (presets != nullptr && presets->isVisible()) || (settings != nullptr && settings->isVisible());
    backdrop->setVisible (on);
    if (on) backdrop->toFront (false);
    if (presets != nullptr && presets->isVisible()) presets->toFront (false);
    if (settings != nullptr && settings->isVisible()) settings->toFront (false);
    bubble->toFront (false);
}

void BabushkaEditor::showSettings (bool show)
{
    if (show && settings == nullptr)
    {
        settings = std::make_unique<SettingsOverlay> (processor);
        settings->onClose = [this] { showSettings (false); };
        settings->onSkinChange = [this] (int index) { setSkin (index); };
        content.addChildComponent (*settings);
    }
    if (presets != nullptr && show) showPresets (false);
    if (settings != nullptr)
    {
        settings->setCentrePosition (kW / 2, kH / 2);
        settings->setVisible (show);
        if (show) settings->toFront (true);
    }
    updateBackdrop();
}

void BabushkaEditor::showHint (juce::Component&, const juce::String& text)
{
    const auto mouse = content.getLocalPoint (nullptr, juce::Desktop::getInstance().getMainMouseSource().getScreenPosition());
    bubble->show (text, mouse);
}

void BabushkaEditor::hideHint() { bubble->hide(); }
