#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg      { 0xff1d1b1a };
    const juce::Colour kPanel   { 0xff2b2725 };
    const juce::Colour kAccent  { 0xffd9772b };
    const juce::Colour kText    { 0xffece3d8 };
    constexpr int kCell = 86, kCellH = 96, kHeader = 26, kPad = 8;
}

// ---------------------------------------------------------------------------------------------------------
PatchPanelComponent::PatchPanelComponent (BabushkaProcessor& p) : processor (p)
{
    using namespace babushka::dsp;
    fillBox (fromBox, true);
    fillBox (toBox, false);
    fromBox.setTextWhenNothingSelected ("from (output / mult)");
    toBox.setTextWhenNothingSelected ("to (input / mult)");
    for (auto* b : { &patchButton, &removeButton, &clearButton })
    {
        b->setColour (juce::TextButton::buttonColourId, kBg);
        b->setColour (juce::TextButton::textColourOffId, kText);
        addAndMakeVisible (*b);
    }
    patchButton.onClick = [this] {
        if (fromBox.getSelectedId() == 0 || toBox.getSelectedId() == 0) return;
        processor.addCable ((babushka::dsp::Jack) (fromBox.getSelectedId() - 1), (babushka::dsp::Jack) (toBox.getSelectedId() - 1));
        refresh(); };
    removeButton.onClick = [this] { const int r = list.getSelectedRow(); if (r >= 0) { processor.removeCable (r); refresh(); } };
    clearButton.onClick = [this] { processor.clearCables(); refresh(); };
    addAndMakeVisible (fromBox);
    addAndMakeVisible (toBox);
    list.setModel (this);
    list.setRowHeight (20);
    list.setColour (juce::ListBox::backgroundColourId, kBg);
    addAndMakeVisible (list);
    hint.setFont (juce::FontOptions (11.5f));
    hint.setColour (juce::Label::textColourId, kText.withAlpha (0.7f));
    hint.setText ("A cable in an input replaces its normalled source. Mult jacks are one node. Delete key removes the selected cable.", juce::dontSendNotification);
    addAndMakeVisible (hint);
    refresh();
}

void PatchPanelComponent::fillBox (juce::ComboBox& box, bool outputs)
{
    using namespace babushka::dsp;
    juce::String lastSection;
    for (int i = 0; i < kNumJacks; ++i)
    {
        const auto& info = jackInfo ((Jack) i);
        const bool wanted = info.kind == JackKind::Mult || (outputs ? info.kind == JackKind::Output : info.kind == JackKind::Input);
        if (! wanted) continue;
        if (juce::String (info.section) != lastSection) { lastSection = info.section; box.addSectionHeading (lastSection); }
        box.addItem (juce::String (info.label), i + 1);
    }
}

void PatchPanelComponent::refresh()
{
    cached = processor.getPatchCopy();
    list.updateContent();
    list.repaint();
}

void PatchPanelComponent::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    using namespace babushka::dsp;
    if (row < 0 || row >= cached.count) return;
    if (selected) g.fillAll (kAccent.withAlpha (0.35f));
    const auto& c = cached.cables[(size_t) row];
    g.setColour (kText);
    g.setFont (juce::FontOptions (12.5f));
    g.drawText (juce::String (jackInfo (c.a).section) + " · " + jackInfo (c.a).label + "   <->   "
                    + jackInfo (c.b).section + " · " + jackInfo (c.b).label, 8, 0, width - 12, height, juce::Justification::centredLeft);
}

void PatchPanelComponent::paint (juce::Graphics& g)
{
    g.setColour (kPanel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText ("PATCH PANEL  (41 points: 21 inputs, 16 outputs, 4 mult)", 10, 4, getWidth() - 20, 18, juce::Justification::centredLeft);
}

void PatchPanelComponent::resized()
{
    auto r = getLocalBounds().reduced (8, 0);
    r.removeFromTop (26);
    auto row = r.removeFromTop (26);
    fromBox.setBounds (row.removeFromLeft (230));
    row.removeFromLeft (6);
    toBox.setBounds (row.removeFromLeft (230));
    row.removeFromLeft (8);
    patchButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (6);
    removeButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (6);
    clearButton.setBounds (row.removeFromLeft (90));
    r.removeFromTop (4);
    hint.setBounds (r.removeFromBottom (18));
    list.setBounds (r.reduced (0, 2));
}

// ---------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------
PresetBrowser::PresetBrowser (BabushkaProcessor& p) : processor (p)
{
    list.setModel (this);
    list.setRowHeight (24);
    list.setColour (juce::ListBox::backgroundColourId, kBg);
    addAndMakeVisible (list);
    nameEditor.setTextToShowWhenEmpty ("name for a new preset", kText.withAlpha (0.5f));
    nameEditor.setColour (juce::TextEditor::backgroundColourId, kBg);
    nameEditor.setColour (juce::TextEditor::textColourId, kText);
    nameEditor.setColour (juce::TextEditor::outlineColourId, kAccent.withAlpha (0.5f));
    addAndMakeVisible (nameEditor);
    for (auto* b : { &saveButton, &deleteButton, &folderButton })
    {
        b->setColour (juce::TextButton::buttonColourId, kBg);
        b->setColour (juce::TextButton::textColourOffId, kText);
        addAndMakeVisible (*b);
    }
    saveButton.onClick = [this] {
        const auto name = nameEditor.getText().trim();
        if (name.isEmpty()) { status.setText ("Type a name first.", juce::dontSendNotification); return; }
        if (processor.saveUserPreset (name)) { status.setText ("Saved '" + name + "'.", juce::dontSendNotification); refresh(); if (onLoaded) onLoaded(); }
        else status.setText ("Could not save to " + processor.getUserPresetDirectory().getFullPathName(), juce::dontSendNotification); };
    deleteButton.onClick = [this] { deleteKeyPressed (list.getSelectedRow()); };
    folderButton.onClick = [this] { processor.getUserPresetDirectory().createDirectory(); processor.getUserPresetDirectory().revealToUser(); };
    for (auto* l : { &description, &status })
    {
        l->setFont (juce::FontOptions (12.5f));
        l->setColour (juce::Label::textColourId, kText.withAlpha (0.85f));
        l->setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (*l);
    }
    refresh();
}

void PresetBrowser::refresh()
{
    rows.clear();
    juce::String lastCategory;
    const auto& all = babushka::factoryPresets();
    for (int i = 0; i < (int) all.size(); ++i)
    {
        if (juce::String (all[(size_t) i].category) != lastCategory)
        {
            lastCategory = all[(size_t) i].category;
            rows.push_back ({ Row::Header, lastCategory.toUpperCase(), -1, {} });
        }
        rows.push_back ({ Row::Factory, all[(size_t) i].name, i, {} });
    }
    const auto files = processor.getUserPresetFiles();
    if (! files.isEmpty()) rows.push_back ({ Row::Header, "USER", -1, {} });
    for (auto& f : files) rows.push_back ({ Row::User, f.getFileNameWithoutExtension(), -1, f });
    list.updateContent();
    selectCurrentByName();
}

void PresetBrowser::selectCurrentByName()
{
    suppress = true;
    list.deselectAllRows();
    const auto cur = processor.getCurrentPresetName();
    for (int i = 0; i < (int) rows.size(); ++i)
        if (rows[(size_t) i].kind != Row::Header && rows[(size_t) i].text == cur)
        {
            list.selectRow (i, false, true);
            if (rows[(size_t) i].kind == Row::Factory)
                description.setText (juce::String (babushka::factoryPresets()[(size_t) rows[(size_t) i].factoryIndex].description), juce::dontSendNotification);
            nameEditor.setText (cur, false);
            break;
        }
    suppress = false;
    list.repaint();
}

void PresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= (int) rows.size()) return;
    const auto& r = rows[(size_t) row];
    if (r.kind == Row::Header)
    {
        g.setColour (kPanel);
        g.fillRect (0, 0, width, height);
        g.setColour (kAccent);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (r.text, 8, 0, width - 12, height, juce::Justification::centredLeft);
        return;
    }
    if (selected) g.fillAll (kAccent.withAlpha (0.35f));
    g.setColour (kText);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (r.text, 16, 0, width - 20, height, juce::Justification::centredLeft);
}

void PresetBrowser::loadRow (int row)
{
    if (row < 0 || row >= (int) rows.size()) return;
    const auto& r = rows[(size_t) row];
    if (r.kind == Row::Factory)
    {
        processor.applyFactoryPreset (r.factoryIndex);
        description.setText (juce::String (babushka::factoryPresets()[(size_t) r.factoryIndex].description), juce::dontSendNotification);
    }
    else if (r.kind == Row::User)
    {
        if (! processor.loadUserPreset (r.file)) { status.setText ("Could not read " + r.file.getFileName(), juce::dontSendNotification); return; }
        description.setText ("User preset (" + r.file.getFullPathName() + ")", juce::dontSendNotification);
    }
    else return;
    status.setText ({}, juce::dontSendNotification);
    nameEditor.setText (r.text, false);
    if (onLoaded) onLoaded();
}

void PresetBrowser::selectedRowsChanged (int lastRowSelected)
{
    if (! suppress) loadRow (lastRowSelected);
}

void PresetBrowser::deleteKeyPressed (int row)
{
    if (row < 0 || row >= (int) rows.size() || rows[(size_t) row].kind != Row::User) { status.setText ("Only user presets can be deleted.", juce::dontSendNotification); return; }
    const auto name = rows[(size_t) row].text;
    if (processor.deleteUserPreset (rows[(size_t) row].file)) { status.setText ("Deleted '" + name + "'.", juce::dontSendNotification); refresh(); }
}

void PresetBrowser::loadRelative (int delta)
{
    int cur = -1;
    for (int i = 0; i < (int) rows.size(); ++i)
        if (rows[(size_t) i].kind != Row::Header && rows[(size_t) i].text == processor.getCurrentPresetName()) { cur = i; break; }
    int i = cur;
    for (int step = 0; step < (int) rows.size(); ++step)
    {
        i = ((i + delta) % (int) rows.size() + (int) rows.size()) % (int) rows.size();
        if (cur < 0 && delta < 0) i = (int) rows.size() - 1 - step;   // nothing selected: start from the end / the start
        if (rows[(size_t) i].kind != Row::Header) break;
    }
    suppress = true; list.selectRow (i, true, true); suppress = false;
    loadRow (i);
    list.repaint();
}

void PresetBrowser::paint (juce::Graphics& g)
{
    g.setColour (kPanel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText ("PRESETS  (click to load; factory presets can be edited and saved as user presets)", 10, 4, getWidth() - 20, 18, juce::Justification::centredLeft);
}

void PresetBrowser::resized()
{
    auto r = getLocalBounds().reduced (8, 0);
    r.removeFromTop (26);
    auto bottom = r.removeFromBottom (120);
    list.setBounds (r.reduced (0, 2));
    auto row = bottom.removeFromTop (28).reduced (0, 2);
    nameEditor.setBounds (row.removeFromLeft (260));
    row.removeFromLeft (6);
    saveButton.setBounds (row.removeFromLeft (70));
    row.removeFromLeft (6);
    deleteButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (6);
    folderButton.setBounds (row.removeFromLeft (120));
    description.setBounds (bottom.removeFromTop (60).reduced (2, 4));
    status.setBounds (bottom.reduced (2, 0));
}

// ---------------------------------------------------------------------------------------------------------
BabushkaEditor::BabushkaEditor (BabushkaProcessor& p) : AudioProcessorEditor (&p), processor (p), patchPanel (p), presetBrowser (p)
{
    lnf.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    lnf.setColour (juce::Slider::thumbColourId, kAccent);
    lnf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lnf.setColour (juce::ComboBox::backgroundColourId, kBg);
    lnf.setColour (juce::ComboBox::outlineColourId, kAccent.withAlpha (0.5f));
    lnf.setColour (juce::Label::textColourId, kText);
    lnf.setColour (juce::ToggleButton::tickColourId, kAccent);
    setLookAndFeel (&lnf);

    addSection ("MODULATION",  { "lfoRate", "lfoWave", "lfoPitchAmt", "lfoCutoffAmt", "lfoPwAmt", "modWheel" });
    addSection ("OSCILLATOR 1", { "osc1Octave", "osc1Wave" });
    addSection ("OSCILLATOR 2", { "osc2Octave", "osc2Wave", "osc2Freq", "osc2Sync" });
    addSection ("MIXER",       { "mixOsc1", "mixOsc2", "mixNoise", "extLevel" });
    addSection ("UTILITIES",   { "hpCutoff", "attenuator" });
    addSection ("FILTER",      { "cutoff", "resonance", "kbdTrack", "envAmt" });
    addSection ("ENVELOPE",    { "attack", "decay", "sustain", "release" });
    addSection ("OUTPUT",      { "vcaMode", "reverbMix", "volume" });
    addSection ("CONTROLLER / GLOBAL", { "glide", "glideType", "legatoGlide", "bendRange", "notePriority", "fineTune", "drift", "oversampling" });
    addSection ("ARP / SEQ",   { "arpPlay", "arpHold", "arpMode", "arpDirection", "arpOctSeq", "arpRate", "arpSync", "arpFreeStep" });
    addSection ("JACKS / GLOBAL", { "extClockMode", "extClockPpqn", "clockOutPpqn", "kbOutRange", "localOn" });

    addAndMakeVisible (patchPanel);
    addAndMakeVisible (presetBrowser);
    presetBrowser.setVisible (false);
    for (auto* b : { &synthTab, &presetsTab, &prevButton, &nextButton })
    {
        b->setColour (juce::TextButton::buttonColourId, kBg);
        b->setColour (juce::TextButton::buttonOnColourId, kAccent);
        b->setColour (juce::TextButton::textColourOffId, kText);
        b->setColour (juce::TextButton::textColourOnId, kBg);
        addAndMakeVisible (*b);
    }
    synthTab.setClickingTogglesState (false);
    synthTab.onClick = [this] { showPresetsTab (false); };
    presetsTab.onClick = [this] { showPresetsTab (true); };
    prevButton.onClick = [this] { presetBrowser.loadRelative (-1); };
    nextButton.onClick = [this] { presetBrowser.loadRelative (+1); };
    presetLabel.setJustificationType (juce::Justification::centred);
    presetLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    presetLabel.setColour (juce::Label::textColourId, kText);
    addAndMakeVisible (presetLabel);
    presetBrowser.onLoaded = [this] { timerCallback(); };
    synthTab.setToggleState (true, juce::dontSendNotification);
    synthTab.setClickingTogglesState (false);
    presetsTab.setClickingTogglesState (false);

    for (auto* b : { &tapButton, &tieButton, &restButton, &accentButton })
    {
        b->setColour (juce::TextButton::buttonColourId, kBg);
        b->setColour (juce::TextButton::textColourOffId, kText);
        addAndMakeVisible (*b);
    }
    tapButton.onClick    = [this] { handleTap(); };
    tieButton.onClick    = [this] { processor.requestRecTie(); };
    restButton.onClick   = [this] { processor.requestRecRest(); };
    accentButton.onClick = [this] { processor.requestRecAccent(); };
    tieButton.setTooltip ("REC mode: extend the last note by one step");
    restButton.setTooltip ("REC mode: add a rest");
    accentButton.setTooltip ("REC mode: accent the next note");
    arpInfo.setFont (juce::FontOptions (12.0f));
    arpInfo.setColour (juce::Label::textColourId, kText.withAlpha (0.8f));
    addAndMakeVisible (arpInfo);

    auto& arpSection = sections[sections.size() - 2];   // ARP / SEQ (JACKS / GLOBAL follows it)
    arpSection.extraHeight = 58;
    arpSection.layoutExtra = [this] (juce::Rectangle<int> r) {
        auto row = r.removeFromTop (26);
        const int w = row.getWidth() / 4;
        for (auto* b : { &tapButton, &tieButton, &restButton, &accentButton })
            b->setBounds (row.removeFromLeft (w).reduced (3, 0));
        arpInfo.setBounds (r.reduced (3, 2));
    };

    setResizable (true, true);
    setResizeLimits (720, 480, 1800, 1600);
    setSize (1040, 1180);
    startTimerHz (10);
    timerCallback();
}

BabushkaEditor::~BabushkaEditor() { stopTimer(); setLookAndFeel (nullptr); }

void BabushkaEditor::showPresetsTab (bool show)
{
    presetsVisible = show;
    for (auto& s : sections)
        for (auto& c : s.controls) { c->widget->setVisible (! show); c->label->setVisible (! show); }
    for (auto* b : { &tapButton, &tieButton, &restButton, &accentButton }) b->setVisible (! show);
    arpInfo.setVisible (! show);
    patchPanel.setVisible (! show);
    presetBrowser.setVisible (show);
    synthTab.setToggleState (! show, juce::dontSendNotification);
    presetsTab.setToggleState (show, juce::dontSendNotification);
    if (show) presetBrowser.refresh();
    timerCallback();
    repaint();
}

void BabushkaEditor::timerCallback()
{
    presetLabel.setText (processor.getCurrentPresetName(), juce::dontSendNotification);
    patchPanel.refresh();
    const int mode = (int) processor.apvts.getRawParameterValue ("arpMode")->load();
    juce::String t = processor.getArpRateText();
    if (mode != 0)
        t << "   |   seq " << (int) processor.apvts.getRawParameterValue ("arpOctSeq")->load() + 1
          << ": " << processor.getCurrentSequenceLength() << " / 256 steps";
    if (mode == 2) t << "   |   REC: play keys to enter notes";
    arpInfo.setText (t, juce::dontSendNotification);
}

// TAP: at least three taps in a row set the free tempo (as on the hardware).
void BabushkaEditor::handleTap()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (! tapTimes.isEmpty() && now - tapTimes.getLast() > 2.0) tapTimes.clear();
    tapTimes.add (now);
    if (tapTimes.size() > 4) tapTimes.remove (0);
    if (tapTimes.size() >= 3)
    {
        const double avg = (tapTimes.getLast() - tapTimes.getFirst()) / (tapTimes.size() - 1);
        if (avg > 0.0) processor.setArpTempoFromTap (60.0 / avg);
    }
}

void BabushkaEditor::addSection (const juce::String& title, std::initializer_list<const char*> paramIds)
{
    Section s; s.title = title;
    for (auto* id : paramIds)
    {
        auto* param = processor.apvts.getParameter (id);
        jassert (param != nullptr);
        auto c = std::make_unique<Control>();
        c->paramId = id;
        c->label = std::make_unique<juce::Label> (juce::String(), param->getName (32));
        c->label->setJustificationType (juce::Justification::centred);
        c->label->setFont (juce::FontOptions (12.0f));

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            auto box = std::make_unique<juce::ComboBox>();
            box->addItemList (choice->choices, 1);
            c->comboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, id, *box);
            c->widget = std::move (box);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            auto b = std::make_unique<juce::ToggleButton> ("on");
            c->buttonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, id, *b);
            c->widget = std::move (b);
        }
        else
        {
            auto sl = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
            sl->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
            sl->setNumDecimalPlacesToDisplay (2);
            c->sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, *sl);
            sl->setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
            c->widget = std::move (sl);
        }
        addAndMakeVisible (*c->widget);
        addAndMakeVisible (*c->label);
        s.controls.push_back (std::move (c));
    }
    sections.push_back (std::move (s));
}

void BabushkaEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("BABUSHKA", 14, 6, 300, 30, juce::Justification::centredLeft);
    g.setColour (kText.withAlpha (0.55f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("semi-modular analog voice", 170, 6, 220, 30, juce::Justification::centredLeft);

    if (presetsVisible) return;
    for (auto& s : sections)
    {
        g.setColour (kPanel);
        g.fillRoundedRectangle (s.bounds.toFloat(), 6.0f);
        g.setColour (kAccent);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (s.title, s.bounds.getX() + 10, s.bounds.getY() + 4, s.bounds.getWidth() - 20, 18, juce::Justification::centredLeft);
    }
}

void BabushkaEditor::resized()
{
    // Flow layout: sections wrap across rows; each section is a grid of cells.
    const int top = 42, width = getWidth() - 2 * kPad;
    int x = kPad, y = top, rowH = 0;
    for (auto& s : sections)
    {
        const int n = (int) s.controls.size();
        const int cols = std::min (n, 4);
        const int rows = (n + cols - 1) / cols;
        const int w = cols * kCell + 12, h = kHeader + rows * kCellH + 6 + s.extraHeight;
        if (x + w > kPad + width) { x = kPad; y += rowH + kPad; rowH = 0; }
        s.bounds = { x, y, w, h };
        for (int i = 0; i < n; ++i)
        {
            auto& c = *s.controls[(size_t) i];
            const int cx = x + 6 + (i % cols) * kCell, cy = y + kHeader + (i / cols) * kCellH;
            c.label->setBounds (cx, cy, kCell, 16);
            const bool isKnob = dynamic_cast<juce::Slider*> (c.widget.get()) != nullptr;
            if (isKnob) c.widget->setBounds (cx + 4, cy + 16, kCell - 8, kCellH - 20);
            else        c.widget->setBounds (cx + 4, cy + 34, kCell - 8, 24);
        }
        if (s.layoutExtra)
            s.layoutExtra ({ x + 6, y + kHeader + rows * kCellH, w - 12, s.extraHeight });
        x += w + kPad;
        rowH = std::max (rowH, h);
    }
    patchPanel.setBounds (kPad, y + rowH + kPad, width, std::max (170, getHeight() - (y + rowH + 2 * kPad)));

    // Header: preset selector in the middle, page tabs on the right; the preset page fills the area below.
    auto head = getLocalBounds().withHeight (36).reduced (kPad, 5);
    auto tabs = head.removeFromRight (180);
    synthTab.setBounds (tabs.removeFromLeft (86));
    tabs.removeFromLeft (6);
    presetsTab.setBounds (tabs.removeFromLeft (88));
    auto sel = head.withTrimmedLeft (400).withTrimmedRight (12);
    prevButton.setBounds (sel.removeFromLeft (30));
    nextButton.setBounds (sel.removeFromRight (30));
    presetLabel.setBounds (sel);
    presetBrowser.setBounds (kPad, 42, width, getHeight() - 42 - kPad);
}
