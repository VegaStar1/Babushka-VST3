// Babushka — MIDI controller mapping: the MIDI menu and MIDI learn on the panel.
#pragma once

#include "Overlays.h"

namespace babushka::ui
{
// ---- MIDI menu ------------------------------------------------------------------------------------------------------
// The connected keyboard and its profile, the list of bindings, MIDI learn, Quick assign and soft takeover.
class MidiOverlay : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    explicit MidiOverlay (BabushkaProcessor& p)
        : processor (p), map (p.midiMap), rescanButton ("FIND AGAIN"), applyButton ("APPLY"), learnButton ("LEARN ON PANEL"),
          quickButton ("QUICK ASSIGN"), clearButton ("CLEAR ALL"), closeButton ("CLOSE")
    {
        setLookAndFeel (&lookAndFeel);
        for (int i = 0; i < (int) controllerProfiles().size(); ++i) profileBox.addItem (controllerProfiles()[(size_t) i].name, i + 1);
        addAndMakeVisible (profileBox);
        softTakeover.setButtonText ({});
        softTakeover.setToggleState (map.getSoftTakeover(), juce::dontSendNotification);
        softTakeover.onClick = [this] { map.setSoftTakeover (softTakeover.getToggleState()); };
        addAndMakeVisible (softTakeover);
        list.setModel (this);
        list.setRowHeight (34);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0b0a09));
        list.setColour (juce::ListBox::outlineColourId, col::outline.withAlpha (0.3f));
        list.setOutlineThickness (1);
        addAndMakeVisible (list);
        for (auto* b : { &rescanButton, &applyButton, &learnButton, &quickButton, &clearButton, &closeButton }) addAndMakeVisible (*b);
        rescanButton.onClick = [this] { refreshDevices(); };
        applyButton.onClick = [this] {
            const int id = profileBox.getSelectedId();
            if (id > 0) { map.applyProfile (id - 1); status = "Profile applied: " + juce::String (controllerProfiles()[(size_t) id - 1].name); refresh(); } };
        learnButton.onClick = [this] { map.endQuickAssign(); if (onLearnOnPanel) onLearnOnPanel(); };
        quickButton.onClick = [this] {
            if (map.isQuickAssign()) { map.endQuickAssign(); status = "Quick assign stopped."; }
            else { map.startQuickAssign(); status = {}; }
            refresh(); };
        clearButton.onClick = [this] { map.endQuickAssign(); map.clear(); status = "All bindings removed."; refresh(); };
        closeButton.onClick = [this] { if (onClose) onClose(); };
        closeButton.primary = true;
        setSize (900, 760);
        refreshDevices();
        startTimerHz (20);
    }
    ~MidiOverlay() override { map.endQuickAssign(); setLookAndFeel (nullptr); }

    std::function<void()> onClose, onLearnOnPanel;

    void visibilityChanged() override { if (isVisible()) { refreshDevices(); refresh(); } else map.endQuickAssign(); }

    void refresh()
    {
        rows = map.getBindings();
        std::sort (rows.begin(), rows.end(), [this] (const MidiBinding& a, const MidiBinding& b) { return order (a.paramId) < order (b.paramId); });
        list.updateContent();
        list.repaint();
        int profile = 0;
        for (int i = 0; i < (int) controllerProfiles().size(); ++i) if (map.getProfileName() == controllerProfiles()[(size_t) i].name) profile = i + 1;
        if (profile > 0) profileBox.setSelectedId (profile, juce::dontSendNotification);
        quickButton.setText (map.isQuickAssign() ? "STOP" : "QUICK ASSIGN");
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        drawDialogFrame (g, getLocalBounds().toFloat().reduced (4.0f), "MIDI CONTROLLER");
        drawScreens (g, *this);
        const float x = 36.0f;
        auto row = [&] (float y, const juce::String& title, const juce::String& text, juce::Colour c) {
            label (g, title, { x, y, 160.0f, 34.0f }, col::ink, 17.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.02f);
            if (text.isNotEmpty()) label (g, text, { x + 170.0f, y, (float) getWidth() - x - 170.0f - 190.0f, 34.0f }, c, 16.0f, Weight::NarrowSemi,
                                          juce::Justification::centredLeft, 0.01f);
        };
        row (66.0f, "Keyboard", deviceText, col::ledAmber);
        row (108.0f, "Profile", {}, col::ink);
        const auto a = map.lastActivity();
        row (150.0f, "Last knob", a.cc < 0 ? juce::String ("move a knob on the keyboard...") :
                                  "CC " + juce::String (a.cc) + "   channel " + juce::String (a.channel) + "   value " + juce::String (a.value), col::ink.withAlpha (0.85f));
        row (192.0f, "Soft takeover", {}, col::ink);
        label (g, "A knob that stands elsewhere does not make the parameter jump: it takes over once it reaches the value.",
               { x + 210.0f, 192.0f, (float) getWidth() - x - 250.0f, 34.0f }, col::ink.withAlpha (0.7f), 14.5f, Weight::Narrow, juce::Justification::centredLeft, 0.01f, 2);
        // list header
        const auto lr = list.getBounds().toFloat();
        g.setColour (juce::Colour (0xff1d1c1a));
        g.fillRect (lr.getX(), lr.getY() - 30.0f, lr.getWidth(), 30.0f);
        for (auto [t, cx] : { std::pair<const char*, float> { "PARAMETER", 14.0f }, { "CC", 340.0f }, { "CHANNEL", 420.0f }, { "VALUE", 520.0f } })
            label (g, t, { lr.getX() + cx, lr.getY() - 30.0f, 120.0f, 30.0f }, col::accent.brighter (0.2f), 13.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.16f);
        if (rows.empty())
            label (g, "No bindings. LEARN ON PANEL: click a control, move a knob. QUICK ASSIGN: move the keyboard's knobs one after another.",
                   lr.reduced (30.0f, 0.0f), col::ink.withAlpha (0.6f), 16.0f, Weight::NarrowSemi, juce::Justification::centred, 0.01f, 3);
        // status line
        juce::String line = status;
        if (map.isQuickAssign())
            line = "QUICK ASSIGN: move a knob for " + map.parameterName (map.getLearnTarget()).toUpperCase() + "   (" + juce::String (map.quickAssignCount())
                   + " done - press STOP when you have no more knobs)";
        label (g, line, { x, (float) getHeight() - 104.0f, (float) getWidth() - 2.0f * x, 22.0f }, col::ledAmber, 15.0f, Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
    }

    void resized() override
    {
        const int x = 36, right = getWidth() - 36;
        rescanButton.setBounds (right - 150, 68, 150, 30);
        profileBox.setBounds (x + 170, 110, right - 170 - (x + 170) - 10, 30);
        applyButton.setBounds (right - 150, 110, 150, 30);
        softTakeover.setBounds (x + 166, 194, 34, 30);
        list.setBounds (x, 270, right - x, getHeight() - 270 - 120);
        auto bottom = juce::Rectangle<int> (x, getHeight() - 70, right - x, 36);
        learnButton.setBounds (bottom.removeFromLeft (190)); bottom.removeFromLeft (10);
        quickButton.setBounds (bottom.removeFromLeft (170)); bottom.removeFromLeft (10);
        clearButton.setBounds (bottom.removeFromLeft (130));
        closeButton.setBounds (bottom.removeFromRight (110));
    }

private:
    DialogLookAndFeel lookAndFeel;
    BabushkaProcessor& processor;
    MidiMapper& map;
    juce::ComboBox profileBox;
    juce::ToggleButton softTakeover;
    juce::ListBox list;
    PlateButton rescanButton, applyButton, learnButton, quickButton, clearButton, closeButton;
    std::vector<MidiBinding> rows;
    juce::String deviceText, status;
    uint32_t lastActivity = 0;
    size_t lastCount = 0;
    bool quickShown = false;

    static int order (const juce::String& id)
    {
        const auto& m = mainParameters();
        for (size_t i = 0; i < m.size(); ++i) if (id == m[i]) return (int) i;
        return 1000;
    }

    void refreshDevices()
    {
        const auto inputs = MidiMapper::connectedInputs();
        juce::String found;
        const int idx = MidiMapper::profileForDevices (inputs, &found);
        if (idx >= 0) deviceText = found + "  -  recognised (" + controllerProfiles()[(size_t) idx].name + ")";
        else if (inputs.isEmpty()) deviceText = "no MIDI inputs found (the DAW may hold them - mapping still works)";
        else deviceText = inputs.joinIntoString (", ") + "  -  not in the list, use Quick assign or Learn";
        if (idx >= 0 && map.isAutoMode() && map.getProfileName() != controllerProfiles()[(size_t) idx].name) map.detectAndApply (false);
        repaint();
    }

    void timerCallback() override
    {
        if (map.isQuickAssign() || map.isLearning()) if (map.pollLearn()) refresh();
        if (map.isQuickAssign() != quickShown) { quickShown = map.isQuickAssign(); refresh(); }
        const auto a = map.lastActivity();
        if (a.counter != lastActivity || map.getBindings().size() != lastCount)
        {
            lastActivity = a.counter;
            lastCount = map.getBindings().size();
            if (map.getBindings().size() != rows.size()) refresh();
            list.repaint();
            repaint (0, 140, getWidth(), 50);
        }
    }

    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        const auto& b = rows[(size_t) row];
        if (selected) { g.setColour (col::accent.withAlpha (0.15f)); g.fillRect (0, 0, width, height); }
        const float h = (float) height;
        label (g, map.parameterName (b.paramId), { 14.0f, 0.0f, 320.0f, h }, col::ink, 16.5f, Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
        label (g, "CC " + juce::String (b.cc), { 340.0f, 0.0f, 80.0f, h }, col::ledAmber, 16.5f, Weight::NarrowBold, juce::Justification::centredLeft, 0.01f);
        label (g, b.channel == 0 ? juce::String ("any") : juce::String (b.channel), { 420.0f, 0.0f, 90.0f, h }, col::ink.withAlpha (0.8f), 16.0f, Weight::NarrowSemi,
               juce::Justification::centredLeft, 0.01f);
        float v = 0.0f;
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (processor.apvts.getParameter (b.paramId))) v = p->getValue();
        const juce::Rectangle<float> bar (520.0f, h * 0.5f - 3.0f, 150.0f, 6.0f);
        g.setColour (juce::Colour (0xff2a2826)); g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (col::accent); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * v), 3.0f);
        label (g, "x", { (float) width - 40.0f, 0.0f, 30.0f, h }, col::ink.withAlpha (0.6f), 17.0f, Weight::NarrowBold, juce::Justification::centred, 0.0f);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        if (row < 0 || row >= (int) rows.size() || e.x < list.getWidth() - 50) return;
        map.unbindParameter (rows[(size_t) row].paramId);           // the x at the end of the row
        status = "Binding removed.";
        refresh();
    }
};

// ---- MIDI learn on the panel ------------------------------------------------------------------------------------------
// Lies over the whole instrument: every control that can take a MIDI knob gets a dashed ring. Click one, move a knob
// on the keyboard and it is bound (a green ring with the CC number); go on with other controls; SAVE keeps the
// bindings, CANCEL goes back to how the map was.
class LearnOverlay : public juce::Component, private juce::Timer
{
public:
    LearnOverlay (BabushkaProcessor& p, const std::vector<SynthPanel::LearnSite>& s)
        : map (p.midiMap), sites (s), before (p.midiMap.snapshot()), saveButton ("SAVE"), cancelButton ("CANCEL")
    {
        for (auto* b : { &saveButton, &cancelButton }) addAndMakeVisible (*b);
        saveButton.onClick = [this] { map.stopLearn(); if (onDone) onDone(); };
        saveButton.primary = true;
        cancelButton.onClick = [this] { map.stopLearn(); map.restore (before); if (onDone) onDone(); };
        setSize (kW, kH);
        saveButton.setBounds (banner().getRight() - 250, banner().getCentreY() - 17, 110, 34);
        cancelButton.setBounds (banner().getRight() - 130, banner().getCentreY() - 17, 110, 34);
        startTimerHz (30);
    }
    ~LearnOverlay() override { map.stopLearn(); }

    std::function<void()> onDone;

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.30f));
        const float pulse = 0.5f + 0.5f * std::sin (phase);
        for (auto& s : sites)
        {
            if (! s.control->isShowing()) continue;          // the rear panel's pedals only when it is out
            const auto area = ring (s);
            const int cc = map.ccFor (s.paramId);
            const bool selected = map.isLearning() && map.getLearnTarget() == s.paramId;
            juce::Path p; p.addRoundedRectangle (area, juce::jmin (area.getWidth(), area.getHeight()) * (isRound (s) ? 0.5f : 0.18f));
            if (selected)
            {
                drawGlow (g, area.getCentre(), area.getWidth() * 0.7f, col::ledAmber, 0.4f + 0.6f * pulse);
                g.setColour (col::ledAmber);
                g.strokePath (p, juce::PathStrokeType (3.0f));
            }
            else if (cc >= 0)
            {
                g.setColour (col::ledGreen.withAlpha (0.95f));
                g.strokePath (p, juce::PathStrokeType (2.5f));
            }
            else
            {
                juce::Path dashed;
                const float dashes[] = { 5.0f, 4.0f };
                juce::PathStrokeType (1.6f).createDashedStroke (dashed, p, dashes, 2);
                g.setColour (col::ledAmber.withAlpha (0.75f));
                g.fillPath (dashed);
            }
            if (selected || cc >= 0)
            {
                const juce::String t = selected ? juce::String ("MOVE A KNOB") : "CC " + juce::String (cc);
                const float w = juce::GlyphArrangement::getStringWidth (font (Weight::NarrowBold, 11.0f, 0.06f), t) + 12.0f;
                const juce::Rectangle<float> tag (area.getCentreX() - w * 0.5f, area.getBottom() - 8.0f, w, 16.0f);
                g.setColour (selected ? col::ledAmber : juce::Colour (0xff173a17));
                g.fillRoundedRectangle (tag, 3.0f);
                label (g, t, tag, selected ? col::inkDark : col::ledGreen.brighter (0.3f), 11.0f, Weight::NarrowBold, juce::Justification::centred, 0.06f);
            }
        }
        // banner over the bottom strip
        const auto b = banner();
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (b.translated (0.0f, 4.0f), 10.0f);
        juce::ColourGradient grad (juce::Colour (0xff2a2216), b.getX(), b.getY(), juce::Colour (0xff17120b), b.getX(), b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (col::ledAmber.withAlpha (0.6f));
        g.drawRoundedRectangle (b.reduced (1.0f), 10.0f, 1.2f);
        drawGlow (g, { b.getX() + 30.0f, b.getY() + 30.0f }, 14.0f, col::ledAmber, 0.5f + 0.5f * pulse);
        label (g, "MIDI LEARN", { b.getX() + 46.0f, b.getY() + 14.0f, 300.0f, 30.0f }, col::ledAmber, 20.0f, Weight::Bold, juce::Justification::centredLeft, 0.16f);
        juce::String text;
        if (map.isLearning()) text = "Now move a knob on the MIDI keyboard for " + map.parameterName (map.getLearnTarget()).toUpperCase() + ".";
        else text = "Click a control with a ring, then move a knob on the keyboard. Do as many as you like, then SAVE.";
        label (g, text, { b.getX() + 46.0f, b.getY() + 46.0f, b.getWidth() - 46.0f - 270.0f, 40.0f }, col::ink, 16.0f, Weight::NarrowSemi,
               juce::Justification::topLeft, 0.01f, 2);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (auto& s : sites)
            if (s.control->isShowing() && ring (s).contains (e.position)) { map.startLearn (s.paramId); repaint(); return; }
    }

private:
    MidiMapper& map;
    std::vector<SynthPanel::LearnSite> sites;
    MidiMapper::Snapshot before;
    PlateButton saveButton, cancelButton;
    float phase = 0.0f;

    static juce::Rectangle<float> banner() { return { 214.0f, 940.0f, 1204.0f, 108.0f }; }
    static bool isRound (const SynthPanel::LearnSite& s) { return dynamic_cast<Knob*> (s.control) != nullptr; }
    juce::Rectangle<float> ring (const SynthPanel::LearnSite& s) const
    {
        const auto b = getLocalArea (s.control, s.control->getLocalBounds()).toFloat();
        if (auto* k = dynamic_cast<Knob*> (s.control)) { const float d = k->diameter() + 16.0f; return juce::Rectangle<float> (d, d).withCentre (b.getCentre()); }
        if (dynamic_cast<Pad*> (s.control) != nullptr) return juce::Rectangle<float> (60.0f, 60.0f).withCentre (b.getCentre());
        if (dynamic_cast<LeverSwitch*> (s.control) != nullptr) return b.reduced (0.0f, 4.0f).expanded (4.0f, 0.0f);
        return b.expanded (4.0f);
    }
    void timerCallback() override
    {
        phase += 0.25f;
        map.pollLearn();
        repaint();
    }
};
} // namespace babushka::ui
