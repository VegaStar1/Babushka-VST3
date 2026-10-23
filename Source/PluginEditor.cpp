#include "PluginEditor.h"
#include "ui/MidiUi.h"
#include "ui/PedalMenu.h"

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
    backdrop->onClick = [this] { showPresets (false); showSettings (false); showMidi (false); showPedal (-1); };
    backdrop->setBounds (0, 0, kW, kH);
    content.addChildComponent (*backdrop);
    content.addChildComponent (*bubble);

    setWantsKeyboardFocus (true);      // undo / redo shortcuts; unused keys go back to the host
    setResizable (true, true);
    setResizeLimits (kViewW * 3 / 5, kViewH * 3 / 5, kViewW * 5 / 4, kViewH * 5 / 4);
    getConstrainer()->setFixedAspectRatio ((double) kViewW / (double) kViewH);
    setSize ((int) (kViewW * 0.8), (int) (kViewH * 0.8));
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
    // the rear panel stays as the user left it (closed the very first time)
    panel->setRearOpen (uiSettings.getBoolValue ("rearOpen", false), false);
    panel->onRearToggled = [this] (bool open) { uiSettings.setValue ("rearOpen", open); uiSettings.saveIfNeeded(); };
    // the output meters too: L-R (the default) or VU + scope
    panel->outputMeters().setShowsVu (uiSettings.getBoolValue ("meterVu", false), false);
    panel->outputMeters().onModeChanged = [this] (bool vu) { uiSettings.setValue ("meterVu", vu); uiSettings.saveIfNeeded(); };
    panel->onJacksMoved = [this] (std::vector<JackSite> sites) { cables->setJacks (std::move (sites)); };
    panel->onOpenMidi = [this] { showMidi (true); };
    panel->onOpenPedal = [this] (int index) { showPedal (index); };
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

bool BabushkaEditor::keyPressed (const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    if (! mods.isCommandDown()) return false;
    const int code = key.getKeyCode();
    if ((code == 'Z' || code == 'z') && ! mods.isShiftDown()) { processor.undo(); return true; }
    if (((code == 'Z' || code == 'z') && mods.isShiftDown()) || code == 'Y' || code == 'y') { processor.redo(); return true; }
    return false;
}

void BabushkaEditor::resized()
{
    const float s = (float) getWidth() / (float) kViewW;
    content.setTransform (juce::AffineTransform::translation ((float) -kViewX, (float) -kViewY).scaled (s));
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
    if (show) showPedal (-1);
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
    const bool on = (presets != nullptr && presets->isVisible()) || (settings != nullptr && settings->isVisible())
                    || (midi != nullptr && midi->isVisible()) || pedalOverlay != nullptr;
    backdrop->setVisible (on);
    if (on) backdrop->toFront (false);
    if (presets != nullptr && presets->isVisible()) presets->toFront (false);
    if (settings != nullptr && settings->isVisible()) settings->toFront (false);
    if (midi != nullptr && midi->isVisible()) midi->toFront (false);
    if (pedalOverlay != nullptr) pedalOverlay->toFront (false);
    bubble->toFront (false);
}

// A pedal's menu is built for that pedal each time it opens (and dropped when it closes).
void BabushkaEditor::showPedal (int index)
{
    if (index >= 0) { showPresets (false); showSettings (false); showMidi (false); }
    if (pedalOverlay != nullptr && (index < 0 || pedalOverlay->pedalIndex() != index))
    {
        auto old = std::move (pedalOverlay);          // closed from its own button: deleted once that click has returned
        old->setVisible (false);
        juce::MessageManager::callAsync ([o = std::shared_ptr<babushka::ui::PedalOverlay> (old.release())] {});
    }
    if (index >= 0 && pedalOverlay == nullptr)
    {
        pedalOverlay = std::make_unique<babushka::ui::PedalOverlay> (processor, index);
        pedalOverlay->onClose = [this] { showPedal (-1); };
        content.addAndMakeVisible (*pedalOverlay);
        pedalOverlay->setCentrePosition (kW / 2, kH / 2);
    }
    updateBackdrop();
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
    if (show) showPedal (-1);
    if (settings != nullptr)
    {
        settings->setCentrePosition (kW / 2, kH / 2);
        settings->setVisible (show);
        if (show) settings->toFront (true);
    }
    updateBackdrop();
}

void BabushkaEditor::showMidi (bool show)
{
    if (show && midi == nullptr)
    {
        midi = std::make_unique<MidiOverlay> (processor);
        midi->onClose = [this] { showMidi (false); };
        midi->onLearnOnPanel = [this] { startMidiLearn(); };
        content.addChildComponent (*midi);
    }
    if (show) { showPresets (false); showSettings (false); showPedal (-1); }
    if (midi != nullptr)
    {
        midi->setCentrePosition (kW / 2, kH / 2);
        midi->setVisible (show);
        if (show) midi->toFront (true);
    }
    updateBackdrop();
}

// The MIDI menu steps aside; the panel gets a ring round every control that can take a knob.
void BabushkaEditor::startMidiLearn()
{
    showMidi (false);
    learn = std::make_unique<LearnOverlay> (processor, panel->learnSites());
    juce::Component::SafePointer<BabushkaEditor> safe (this);
    learn->onDone = [safe] {
        juce::MessageManager::callAsync ([safe] {               // the overlay is deleted after its button's click returns
            if (safe == nullptr) return;
            safe->learn.reset();
            safe->showMidi (true); }); };
    content.addAndMakeVisible (*learn);
    learn->toFront (false);
    bubble->toFront (false);
}

void BabushkaEditor::showHint (juce::Component&, const juce::String& text)
{
    const auto mouse = content.getLocalPoint (nullptr, juce::Desktop::getInstance().getMainMouseSource().getScreenPosition());
    bubble->show (text, mouse);
}

void BabushkaEditor::hideHint() { bubble->hide(); }
