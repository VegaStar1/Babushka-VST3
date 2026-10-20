// Babushka — hint bubble, preset browser and settings overlay, in the style of the panel.
#pragma once

#include "SynthPanel.h"

namespace babushka::ui
{
// A small dark label that follows the pointer while a control is touched: "Cutoff   1.2 kHz".
class HintBubble : public juce::Component, private juce::Timer
{
public:
    HintBubble() { setInterceptsMouseClicks (false, false); }
    void show (const juce::String& t, juce::Point<float> at)
    {
        text = t;
        const float w = juce::GlyphArrangement::getStringWidth (font (Weight::NarrowSemi, 16.0f, 0.02f), t) + 26.0f;
        const juce::Rectangle<float> r (w, 28.0f);
        setBounds (r.withCentre ({ at.x, at.y - 30.0f }).toNearestInt().constrainedWithin (juce::Rectangle<int> (0, 0, kW, kH)));
        setVisible (true);
        toFront (false);
        repaint();
        startTimer (3500);
    }
    void hide() { stopTimer(); setVisible (false); }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 6.0f);
        g.setColour (juce::Colour (0xf0121110));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (col::ledAmber.withAlpha (0.65f));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        label (g, text, r, col::ink, 16.0f, Weight::NarrowSemi, juce::Justification::centred, 0.02f);
    }
private:
    juce::String text;
    void timerCallback() override { hide(); }
};

// Dims the instrument behind a dialog and swallows clicks; a click on it closes the dialog.
class Backdrop : public juce::Component
{
public:
    std::function<void()> onClick;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black.withAlpha (0.62f)); }
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
};

// ---- shared look for the dialogs ---------------------------------------------------------------------------------
// Combo boxes, pop-up menus and slider read-outs in the panel's fonts and colours, at a size that stays legible.
class DialogLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DialogLookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff151413));
        setColour (juce::PopupMenu::textColourId, col::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::amber.withAlpha (0.35f));
        setColour (juce::PopupMenu::highlightedTextColourId, col::ledAmber.brighter (0.3f));
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0b0a09));
        setColour (juce::ComboBox::textColourId, col::ledAmber);
        setColour (juce::ComboBox::outlineColourId, col::outline.withAlpha (0.45f));
        setColour (juce::ComboBox::arrowColourId, col::ledAmber);
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0a09));
        setColour (juce::TextEditor::textColourId, col::ledAmber);
        setColour (juce::Label::textColourId, col::ledAmber);
        setColour (juce::Slider::trackColourId, col::amber);
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xff0b0a09));
        setColour (juce::Slider::thumbColourId, col::metalLight);
        setColour (juce::Slider::textBoxTextColourId, col::ledAmber);
        setColour (juce::Slider::textBoxOutlineColourId, col::outline.withAlpha (0.35f));
        setColour (juce::ToggleButton::tickColourId, col::ledAmber);
        setColour (juce::ToggleButton::tickDisabledColourId, col::ink.withAlpha (0.6f));
        setColour (juce::ScrollBar::thumbColourId, col::amber.withAlpha (0.7f));
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (Weight::NarrowSemi, 17.0f, 0.02f); }
    juce::Font getPopupMenuFont() override { return font (Weight::NarrowSemi, 17.0f, 0.02f); }
    juce::Font getLabelFont (juce::Label&) override { return font (Weight::NarrowSemi, 16.0f, 0.02f); }
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override { return typeface (Weight::NarrowSemi); }
};

inline void drawDialogFrame (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (r.translated (0.0f, 8.0f).expanded (6.0f), 14.0f);
    juce::ColourGradient grad (juce::Colour (0xff252321), r.getX(), r.getY(), juce::Colour (0xff131211), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 10.0f);
    grain (g, r, 0.9f);
    g.setColour (col::outline.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (6.0f), 7.0f, 1.0f);
    label (g, title, r.withHeight (46.0f).translated (0.0f, 8.0f), col::ink, 17.0f, Weight::Bold, juce::Justification::centred, 0.18f);
}

// ---- preset browser -----------------------------------------------------------------------------------------------------
class PresetOverlay : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetOverlay (BabushkaProcessor& p) : processor (p), closeButton ("CLOSE"), saveButton ("SAVE"), deleteButton ("DELETE"), folderButton ("OPEN FOLDER")
    {
        setLookAndFeel (&lookAndFeel);
        list.setModel (this);
        list.setRowHeight (34);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0b0a09));
        list.setColour (juce::ListBox::outlineColourId, col::outline.withAlpha (0.3f));
        list.setOutlineThickness (1);
        addAndMakeVisible (list);
        nameEditor.setTextToShowWhenEmpty ("name for a new preset", col::ink.withAlpha (0.4f));
        nameEditor.setFont (font (Weight::NarrowSemi, 17.0f));
        nameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0a09));
        nameEditor.setColour (juce::TextEditor::textColourId, col::ledAmber);
        nameEditor.setColour (juce::TextEditor::outlineColourId, col::outline.withAlpha (0.4f));
        nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, col::ledAmber);
        nameEditor.setColour (juce::CaretComponent::caretColourId, col::ledAmber);
        addAndMakeVisible (nameEditor);
        for (auto* b : { &closeButton, &saveButton, &deleteButton, &folderButton }) addAndMakeVisible (*b);
        closeButton.onClick = [this] { if (onClose) onClose(); };
        saveButton.onClick = [this] {
            const auto name = nameEditor.getText().trim();
            if (name.isEmpty()) { status = "Type a name first."; repaint(); return; }
            status = processor.saveUserPreset (name) ? "Saved '" + name + "'." : "Could not save to " + processor.getUserPresetDirectory().getFullPathName();
            refresh(); };
        deleteButton.onClick = [this] { deleteSelected(); };
        folderButton.onClick = [this] { processor.getUserPresetDirectory().createDirectory(); processor.getUserPresetDirectory().revealToUser(); };
        setSize (760, 840);
    }
    ~PresetOverlay() override { setLookAndFeel (nullptr); }

    std::function<void()> onClose;

    void visibilityChanged() override { if (isVisible()) refresh(); }

    void refresh()
    {
        entries.clear();
        juce::String lastCat;
        for (auto& r : processor.listPresets())
        {
            if (r.category != lastCat) { lastCat = r.category; entries.push_back ({ true, {}, r.category }); }
            entries.push_back ({ false, r, r.name });
        }
        list.updateContent();
        suppress = true;
        list.deselectAllRows();
        for (int i = 0; i < (int) entries.size(); ++i)
            if (! entries[(size_t) i].header && entries[(size_t) i].text == processor.getCurrentPresetName()) { list.selectRow (i, true, true); break; }
        suppress = false;
        if (nameEditor.getText().isEmpty()) nameEditor.setText (processor.getCurrentPresetName(), false);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        drawDialogFrame (g, getLocalBounds().toFloat().reduced (4.0f), "PRESETS");
        label (g, description(), juce::Rectangle<float> (28.0f, (float) getHeight() - 94.0f, (float) getWidth() - 56.0f, 48.0f), col::ink.withAlpha (0.92f), 15.5f,
               Weight::Narrow, juce::Justification::topLeft, 0.01f, 2);
        label (g, status, juce::Rectangle<float> (28.0f, (float) getHeight() - 42.0f, (float) getWidth() - 56.0f, 20.0f), col::ledAmber, 15.0f, Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (28, 0);
        r.removeFromTop (60);
        auto bottom = r.removeFromBottom (136);
        list.setBounds (r);
        auto row = bottom.removeFromTop (34).reduced (0, 2);
        closeButton.setBounds (row.removeFromRight (90));
        row.removeFromRight (8);
        folderButton.setBounds (row.removeFromRight (130));
        row.removeFromRight (8);
        deleteButton.setBounds (row.removeFromRight (90));
        row.removeFromRight (8);
        saveButton.setBounds (row.removeFromRight (80));
        row.removeFromRight (8);
        nameEditor.setBounds (row);
    }

private:
    struct Entry { bool header; BabushkaProcessor::PresetRef ref; juce::String text; };
    DialogLookAndFeel lookAndFeel;
    BabushkaProcessor& processor;
    std::vector<Entry> entries;
    juce::ListBox list;
    juce::TextEditor nameEditor;
    PlateButton closeButton, saveButton, deleteButton, folderButton;
    juce::String status;
    bool suppress = false;

    juce::String description() const
    {
        const auto cur = processor.getCurrentPresetName();
        for (auto& f : babushka::factoryPresets()) if (cur == f.name) return f.description;
        return "User preset.";
    }

    int getNumRows() override { return (int) entries.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (row < 0 || row >= (int) entries.size()) return;
        const auto& e = entries[(size_t) row];
        if (e.header)
        {
            g.setColour (juce::Colour (0xff1d1c1a)); g.fillRect (0, 0, width, height);
            label (g, e.text.toUpperCase(), juce::Rectangle<float> (14.0f, 0.0f, (float) width - 20.0f, (float) height), col::amber.brighter (0.2f), 14.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.18f);
            return;
        }
        if (selected) { g.setColour (col::amber.withAlpha (0.22f)); g.fillRect (0, 0, width, height); g.setColour (col::amber); g.fillRect (0, 0, 3, height); }
        label (g, e.text, juce::Rectangle<float> (24.0f, 0.0f, (float) width - 30.0f, (float) height), selected ? col::ledAmber.brighter (0.2f) : col::ink, 17.0f, Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
    }
    void selectedRowsChanged (int last) override
    {
        if (suppress || last < 0 || last >= (int) entries.size() || entries[(size_t) last].header) return;
        processor.loadPreset (entries[(size_t) last].ref);
        nameEditor.setText (entries[(size_t) last].text, false);
        status = {};
        repaint();
    }
    void deleteKeyPressed (int) override { deleteSelected(); }
    void deleteSelected()
    {
        const int row = list.getSelectedRow();
        if (row < 0 || row >= (int) entries.size() || entries[(size_t) row].header || entries[(size_t) row].ref.factory) { status = "Only user presets can be deleted."; repaint(); return; }
        const auto name = entries[(size_t) row].text;
        status = processor.deleteUserPreset (entries[(size_t) row].ref.file) ? "Deleted '" + name + "'." : "Could not delete the file.";
        refresh();
    }
};

// ---- settings --------------------------------------------------------------------------------------------------------------
// Everything that is not on the hardware front panel: tuning of the model, glide behaviour, clock jacks, quality, look.
class SettingsOverlay : public juce::Component
{
public:
    explicit SettingsOverlay (BabushkaProcessor& p) : closeButton ("CLOSE")
    {
        setLookAndFeel (&lookAndFeel);
        auto& apvts = p.apvts;
        addSkinRow();
        addRow (apvts, "Spring reverb decay", "Length of the spring tail. x1.00 is the hardware tank.", "reverbDecay", Row::Knob);
        addRow (apvts, "Analog drift", "Slow random wander of the oscillator tuning.", "drift", Row::Knob);
        addRow (apvts, "Instrument In level", "Gain of the sidechain input (no control on the hardware).", "extLevel", Row::Knob);
        addRow (apvts, "Quality", "Oversampling of the voice: Eco 1x, Normal 2x, High 4x.", "oversampling", Row::Choice);
        addRow (apvts, "Glide type", "Rate or time law of the glide.", "glideType", Row::Choice);
        addRow (apvts, "Legato glide", "Glide only between overlapping notes.", "legatoGlide", Row::Toggle);
        addRow (apvts, "Pitch bend range", "Semitones, 1 to 12.", "bendRange", Row::Knob);
        addRow (apvts, "Note priority", "Which key sounds when several are held.", "notePriority", Row::Choice);
        addRow (apvts, "Arp/Seq host sync", "Follow the DAW tempo in note divisions.", "arpSync", Row::Toggle);
        addRow (apvts, "Free tempo step", "Step length of the free-running arpeggiator.", "arpFreeStep", Row::Choice);
        addRow (apvts, "Clock In mode", "Clock: follows the pulses. Step-Advance: one step per pulse.", "extClockMode", Row::Choice);
        addRow (apvts, "Clock In PPQN", "Pulses per quarter note expected on CLOCK IN.", "extClockPpqn", Row::Knob);
        addRow (apvts, "Clock Out PPQN", "Pulses per quarter note sent on CLOCK OUT.", "clockOutPpqn", Row::Knob);
        addRow (apvts, "KB Out range", "Voltage range of the KB OUT jack.", "kbOutRange", Row::Choice);
        addRow (apvts, "Local on", "Off: keys and wheels drive only the KB / GATE / VEL jacks.", "localOn", Row::Toggle);
        addAndMakeVisible (closeButton);
        closeButton.onClick = [this] { if (onClose) onClose(); };
        setSize (1040, kTop + (int) rows.size() * kRow + 74);
    }
    ~SettingsOverlay() override { setLookAndFeel (nullptr); }

    std::function<void()> onClose;
    std::function<void (int)> onSkinChange;          // index into skins()

    void paint (juce::Graphics& g) override
    {
        drawDialogFrame (g, getLocalBounds().toFloat().reduced (4.0f), "SETTINGS");
        for (size_t i = 0; i < rows.size(); ++i)
        {
            const float y = (float) (kTop + (int) i * kRow);
            if (i % 2 == 1) { g.setColour (juce::Colours::white.withAlpha (0.025f)); g.fillRect (juce::Rectangle<float> (22.0f, y, (float) getWidth() - 44.0f, (float) kRow)); }
            label (g, rows[i].title, juce::Rectangle<float> (36.0f, y, 250.0f, (float) kRow), col::ink, 17.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.02f);
            label (g, rows[i].hint, juce::Rectangle<float> (290.0f, y + 2.0f, (float) getWidth() - 290.0f - 300.0f, (float) kRow - 4.0f), col::ink.withAlpha (0.8f), 15.0f,
                   Weight::Narrow, juce::Justification::centredLeft, 0.01f, 2);
        }
    }
    void resized() override
    {
        for (size_t i = 0; i < rows.size(); ++i)
            rows[i].widget->setBounds (getWidth() - 286, kTop + (int) i * kRow + 8, 250, kRow - 16);
        closeButton.setBounds (getWidth() - 36 - 110, getHeight() - 58, 110, 34);
    }

private:
    static constexpr int kTop = 66, kRow = 46;
    struct Row
    {
        enum Kind { Knob, Choice, Toggle, Skin } kind;
        juce::String title, hint;
        std::unique_ptr<juce::Component> widget;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
    };
    DialogLookAndFeel lookAndFeel;
    std::vector<Row> rows;
    PlateButton closeButton;

    void addSkinRow()
    {
        Row r; r.kind = Row::Skin; r.title = "Skin"; r.hint = "Colours of the panel. Babushka, or all-black like the classic hardware.";
        auto c = std::make_unique<juce::ComboBox>();
        for (int i = 0; i < (int) skins().size(); ++i) c->addItem (skins()[(size_t) i].name, i + 1);
        c->setSelectedId (skinIndex() + 1, juce::dontSendNotification);
        auto* box = c.get();
        c->onChange = [this, box] { if (onSkinChange) onSkinChange (box->getSelectedId() - 1); };
        r.widget = std::move (c);
        addAndMakeVisible (*r.widget);
        rows.push_back (std::move (r));
    }

    void addRow (juce::AudioProcessorValueTreeState& apvts, const char* title, const char* hint, const char* id, Row::Kind kind)
    {
        Row r; r.kind = kind; r.title = title; r.hint = hint;
        auto* param = apvts.getParameter (id);
        if (kind == Row::Knob)
        {
            auto s = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
            s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 92, 28);
            r.sa = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, *s);
            s->setDoubleClickReturnValue (true, dynamic_cast<juce::RangedAudioParameter*> (param)->convertFrom0to1 (param->getDefaultValue()));
            r.widget = std::move (s);
        }
        else if (kind == Row::Choice)
        {
            auto c = std::make_unique<juce::ComboBox>();
            c->addItemList (dynamic_cast<juce::AudioParameterChoice*> (param)->choices, 1);
            r.ca = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, *c);
            r.widget = std::move (c);
        }
        else
        {
            auto b = std::make_unique<juce::ToggleButton> ("");
            r.ba = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, *b);
            r.widget = std::move (b);
        }
        addAndMakeVisible (*r.widget);
        rows.push_back (std::move (r));
    }
};
} // namespace babushka::ui
