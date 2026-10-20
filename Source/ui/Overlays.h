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
// A pill-shaped toggle (CHARACTER tags) in the style of the plate buttons.
class ChipButton : public juce::Component
{
public:
    explicit ChipButton (const juce::String& t) : text (t) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick;
    bool lit = false;
    void setLit (bool on) { if (on != lit) { lit = on; repaint(); } }
    const juce::String& getText() const { return text; }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        const float rad = r.getHeight() * 0.5f;
        g.setColour (lit ? col::ledAmber.withAlpha (0.92f) : juce::Colour (0xff1c1b19));
        g.fillRoundedRectangle (r, rad);
        g.setColour (lit ? col::ledAmber : col::outline.withAlpha (0.4f));
        g.drawRoundedRectangle (r, rad, 1.0f);
        label (g, text, r, lit ? col::inkDark : col::ink, 13.0f, Weight::NarrowBold, juce::Justification::centred, 0.08f);
    }
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
private:
    juce::String text;
};

// Text fields of the dialogs: amber text on black glass.
inline void styleField (juce::TextEditor& e, const juce::String& placeholder, juce::Colour textColour = col::ledAmber)
{
    e.setTextToShowWhenEmpty (placeholder, col::ink.withAlpha (0.45f));
    e.setFont (font (Weight::NarrowSemi, 17.0f));
    e.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0a09));
    e.setColour (juce::TextEditor::textColourId, textColour);
    e.setColour (juce::TextEditor::outlineColourId, col::outline.withAlpha (0.4f));
    e.setColour (juce::TextEditor::focusedOutlineColourId, col::ledAmber);
    e.setColour (juce::CaretComponent::caretColourId, col::ledAmber);
    e.setIndents (10, 9);
}

// SAVE AS: name, folder under User presets (or a new one), optional CHARACTER tags.
class SavePresetDialog : public juce::Component
{
public:
    SavePresetDialog() : cancelButton ("CANCEL"), saveButton ("SAVE")
    {
        styleField (nameEditor, "preset name");
        styleField (newFolderEditor, "+ new folder", col::teal.brighter (0.4f));
        for (auto* c : { (juce::Component*) &nameEditor, (juce::Component*) &folderBox, (juce::Component*) &newFolderEditor,
                         (juce::Component*) &cancelButton, (juce::Component*) &saveButton }) addAndMakeVisible (*c);
        for (auto* t : babushka::kCharacterTags)
        {
            auto chip = std::make_unique<ChipButton> (t);
            auto* raw = chip.get();
            chip->onClick = [raw] { raw->setLit (! raw->lit); };
            addAndMakeVisible (*chip);
            chips.push_back (std::move (chip));
        }
        cancelButton.onClick = [this] { if (onCancel) onCancel(); };
        saveButton.onClick = [this] { if (onSave) onSave(); };
        nameEditor.onReturnKey = [this] { if (onSave) onSave(); };
        nameEditor.onTextChange = [this] { confirmReplace = false; status = {}; repaint(); };
        setSize (640, 340);
    }

    std::function<void()> onSave, onCancel;
    bool confirmReplace = false;
    juce::String status;

    void open (const juce::String& name, const juce::StringArray& folders, const juce::String& folder)
    {
        nameEditor.setText (name, false);
        folderBox.clear (juce::dontSendNotification);
        folderBox.addItem ("User presets", 1);
        for (int i = 0; i < folders.size(); ++i) folderBox.addItem ("User presets / " + folders[i], i + 2);
        folderBox.setSelectedId (juce::jmax (1, folders.indexOf (folder) + 2), juce::dontSendNotification);
        folderList = folders;
        newFolderEditor.clear();
        for (auto& c : chips) c->setLit (false);
        confirmReplace = false;
        status = {};
        nameEditor.grabKeyboardFocus();
        nameEditor.selectAll();
    }
    juce::String name() const { return nameEditor.getText().trim(); }
    juce::String folder() const
    {
        const auto typed = newFolderEditor.getText().trim();
        if (typed.isNotEmpty()) return typed;
        const int id = folderBox.getSelectedId();
        return id >= 2 ? folderList[id - 2] : juce::String();
    }
    juce::StringArray tags() const { juce::StringArray t; for (auto& c : chips) if (c->lit) t.add (c->getText()); return t; }

    void paint (juce::Graphics& g) override
    {
        drawDialogFrame (g, getLocalBounds().toFloat().reduced (4.0f), "SAVE PRESET");
        auto caption = [&g] (const char* t, int y) { label (g, t, juce::Rectangle<float> (30.0f, (float) y, 400.0f, 18.0f), col::ink.withAlpha (0.7f), 13.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.14f); };
        caption ("NAME", 62);
        caption ("FOLDER (USER PRESETS)", 132);
        caption ("CHARACTER TAGS - OPTIONAL", 202);
        label (g, status, juce::Rectangle<float> (30.0f, (float) getHeight() - 52.0f, 330.0f, 22.0f), col::ledAmber, 15.0f, Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
    }
    void resized() override
    {
        nameEditor.setBounds (28, 82, getWidth() - 56, 40);
        folderBox.setBounds (28, 152, getWidth() - 56 - 200, 40);
        newFolderEditor.setBounds (getWidth() - 28 - 190, 152, 190, 40);
        int x = 28, y = 222;
        for (auto& c : chips)
        {
            const int w = (int) juce::GlyphArrangement::getStringWidth (font (Weight::NarrowBold, 13.0f, 0.08f), c->getText()) + 30;
            if (x + w > getWidth() - 28) { x = 28; y += 38; }
            c->setBounds (x, y, w, 30);
            x += w + 6;
        }
        saveButton.setBounds (getWidth() - 28 - 110, getHeight() - 60, 110, 36);
        cancelButton.setBounds (getWidth() - 28 - 110 - 10 - 110, getHeight() - 60, 110, 36);
    }

private:
    juce::TextEditor nameEditor, newFolderEditor;
    juce::ComboBox folderBox;
    juce::StringArray folderList;
    std::vector<std::unique_ptr<ChipButton>> chips;
    PlateButton cancelButton, saveButton;
};

// Branches on the left (All, the factory categories, User presets and its folders), search and CHARACTER tags on
// top, the presets of the branch in the middle. A click on a preset loads it.
class PresetOverlay : public juce::Component
{
public:
    explicit PresetOverlay (BabushkaProcessor& p)
        : processor (p), saveButton ("SAVE AS..."), duplicateButton ("DUPLICATE"), deleteButton ("DELETE"), importButton ("IMPORT"),
          exportButton ("EXPORT"), folderButton ("FOLDER"), closeButton ("CLOSE"), clearSearchButton ("X", 13.0f), clearTagsButton ("CLEAR", 12.0f)
    {
        setLookAndFeel (&lookAndFeel);
        styleField (searchBox, "Search names and descriptions in the selected branch");
        searchBox.onTextChange = [this] { rebuildRows(); };
        searchBox.onEscapeKey = [this] { searchBox.clear(); rebuildRows(); };
        addAndMakeVisible (searchBox);
        clearSearchButton.onClick = [this] { searchBox.clear(); rebuildRows(); };
        addChildComponent (clearSearchButton);
        for (auto* t : babushka::kCharacterTags)
        {
            auto chip = std::make_unique<ChipButton> (t);
            auto* raw = chip.get();
            chip->onClick = [this, raw] { raw->setLit (! raw->lit); rebuildRows(); };
            addAndMakeVisible (*chip);
            tagChips.push_back (std::move (chip));
        }
        clearTagsButton.onClick = [this] { for (auto& c : tagChips) c->setLit (false); rebuildRows(); };
        addChildComponent (clearTagsButton);

        for (auto* lb : { &branchList, &presetList })
        {
            lb->setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0b0a09));
            lb->setColour (juce::ListBox::outlineColourId, col::outline.withAlpha (0.3f));
            lb->setOutlineThickness (1);
            addAndMakeVisible (*lb);
        }
        branchList.setModel (&branchModel);
        branchList.setRowHeight (27);
        presetList.setModel (&presetModel);
        presetList.setRowHeight (32);

        for (auto* b : { &saveButton, &duplicateButton, &deleteButton, &importButton, &exportButton, &folderButton, &closeButton }) addAndMakeVisible (*b);
        closeButton.onClick = [this] { if (onClose) onClose(); };
        saveButton.onClick = [this] { openSaveDialog(); };
        duplicateButton.onClick = [this] {
            const auto f = processor.duplicateCurrentSound();
            setStatus (f.existsAsFile() ? "Copy saved as '" + f.getFileNameWithoutExtension() + "' in User presets." : "Could not save the copy.");
            refresh(); };
        deleteButton.onClick = [this] { deleteCurrent(); };
        importButton.onClick = [this] { importFiles(); };
        exportButton.onClick = [this] { exportFile(); };
        folderButton.onClick = [this] { processor.ensurePresetLibrary(); processor.getPresetRoot().revealToUser(); };

        saveDialog.onCancel = [this] { saveDialog.setVisible (false); };
        saveDialog.onSave = [this] { confirmSave(); };
        addChildComponent (saveDialog);
        setSize (1100, 892);
    }
    ~PresetOverlay() override { setLookAndFeel (nullptr); }

    std::function<void()> onClose;

    void visibilityChanged() override
    {
        if (! isVisible()) return;
        processor.ensurePresetLibrary();
        refresh();
        showCurrentInList();
    }

    // Reads the list of presets and folders again (after a save / import / delete, or when the browser opens).
    void refresh()
    {
        presets = processor.listPresets();
        folders = processor.getUserFolders();
        branches.clear();
        branches.push_back ({ "All", "All", false, false });
        juce::StringArray cats;
        for (auto& r : presets) if (r.factory) cats.addIfNotAlreadyThere (r.category);
        for (auto& c : cats) branches.push_back ({ c, c, false, false });
        branches.push_back ({ "User", "User presets", true, false });
        for (auto& f : folders) branches.push_back ({ "User/" + f, f, true, true });
        bool found = false;
        for (auto& b : branches) found = found || b.key == branch;
        if (! found) branch = "All";
        branchList.updateContent();
        rebuildRows();
    }

    // Shows one branch: "All", a factory category, "User" or "User/<folder>".
    void selectBranch (const juce::String& key)
    {
        for (int i = 0; i < (int) branches.size(); ++i) if (branches[(size_t) i].key == key) { selectBranch (i); return; }
    }

    void openSaveDialog()
    {
        const auto cur = processor.getCurrentPresetName();
        juce::String folder;
        if (branch.startsWith ("User/")) folder = branch.substring (5);
        else if (auto* r = currentRef()) if (! r->factory) folder = r->folder;
        saveDialog.open (cur, folders, folder);
        saveDialog.setVisible (true);
        saveDialog.toFront (true);
        resized();
    }

    void paint (juce::Graphics& g) override
    {
        drawDialogFrame (g, getLocalBounds().toFloat().reduced (4.0f), "PRESETS");
        label (g, countText, searchBox.getBounds().toFloat().withX ((float) searchBox.getRight() - 150.0f).withWidth (110.0f), col::ink.withAlpha (0.65f), 14.0f,
               Weight::Narrow, juce::Justification::centredRight, 0.01f);
        label (g, "CHARACTER", juce::Rectangle<float> (28.0f, (float) kTagsY, 110.0f, 30.0f), col::ink.withAlpha (0.7f), 13.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.16f);
        const auto descArea = juce::Rectangle<float> (28.0f, (float) kDescY, (float) getWidth() - 56.0f, 48.0f);
        label (g, description(), descArea, col::ink.withAlpha (0.92f), 15.5f, Weight::Narrow, juce::Justification::topLeft, 0.01f, 2);
        label (g, status, juce::Rectangle<float> (28.0f, (float) getHeight() - 44.0f, (float) getWidth() - 56.0f, 22.0f), col::ledAmber, 15.0f, Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
    }

    void resized() override
    {
        searchBox.setBounds (28, 62, getWidth() - 56, 40);
        clearSearchButton.setBounds (searchBox.getRight() - 36, searchBox.getY() + 6, 28, 28);
        int x = 140;
        for (auto& c : tagChips)
        {
            const int w = (int) juce::GlyphArrangement::getStringWidth (font (Weight::NarrowBold, 13.0f, 0.08f), c->getText()) + 28;
            c->setBounds (x, kTagsY, w, 30);
            x += w + 6;
        }
        clearTagsButton.setBounds (x + 4, kTagsY, 70, 30);
        branchList.setBounds (28, kBodyY, 250, kBodyH);
        presetList.setBounds (292, kBodyY, getWidth() - 292 - 28, kBodyH);
        auto row = juce::Rectangle<int> (28, kButtonsY, getWidth() - 56, 36);
        saveButton.setBounds (row.removeFromLeft (120)); row.removeFromLeft (8);
        duplicateButton.setBounds (row.removeFromLeft (110)); row.removeFromLeft (8);
        deleteButton.setBounds (row.removeFromLeft (90));
        closeButton.setBounds (row.removeFromRight (90)); row.removeFromRight (8);
        folderButton.setBounds (row.removeFromRight (90)); row.removeFromRight (8);
        exportButton.setBounds (row.removeFromRight (90)); row.removeFromRight (8);
        importButton.setBounds (row.removeFromRight (90));
        saveDialog.setCentrePosition (getWidth() / 2, getHeight() / 2);
    }

private:
    static constexpr int kTagsY = 112, kBodyY = 156, kBodyH = 568, kDescY = 734, kButtonsY = 790;

    struct Branch { juce::String key, label; bool user, folder; };
    struct Row { bool header; int preset; juce::String text; };

    // List models forward to the overlay.
    struct BranchModel : public juce::ListBoxModel
    {
        PresetOverlay& o;
        explicit BranchModel (PresetOverlay& owner) : o (owner) {}
        int getNumRows() override { return (int) o.branches.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override { o.paintBranch (row, g, w, h); }
        void listBoxItemClicked (int row, const juce::MouseEvent&) override { o.selectBranch (row); }
    };
    struct PresetModel : public juce::ListBoxModel
    {
        PresetOverlay& o;
        explicit PresetModel (PresetOverlay& owner) : o (owner) {}
        int getNumRows() override { return (int) o.rows.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override { o.paintPreset (row, g, w, h); }
        void listBoxItemClicked (int row, const juce::MouseEvent&) override { o.loadRow (row); }
        void deleteKeyPressed (int) override { o.deleteCurrent(); }
    };

    DialogLookAndFeel lookAndFeel;
    BabushkaProcessor& processor;
    std::vector<BabushkaProcessor::PresetRef> presets;
    juce::StringArray folders;
    std::vector<Branch> branches;
    std::vector<Row> rows;
    juce::String branch { "All" }, countText, status, pendingDelete;
    BranchModel branchModel { *this };
    PresetModel presetModel { *this };
    juce::TextEditor searchBox;
    juce::ListBox branchList, presetList;
    std::vector<std::unique_ptr<ChipButton>> tagChips;
    PlateButton saveButton, duplicateButton, deleteButton, importButton, exportButton, folderButton, closeButton, clearSearchButton, clearTagsButton;
    SavePresetDialog saveDialog;
    std::unique_ptr<juce::FileChooser> chooser;

    void setStatus (const juce::String& s) { status = s; pendingDelete = {}; repaint(); }

    bool inBranch (const BabushkaProcessor::PresetRef& r) const
    {
        if (branch == "All") return true;
        if (branch == "User") return ! r.factory;
        if (branch.startsWith ("User/")) return ! r.factory && (r.folder == branch.substring (5) || r.folder.startsWith (branch.substring (5) + "/"));
        return r.factory && r.category == branch;
    }

    void rebuildRows()
    {
        const auto query = searchBox.getText().trim();
        juce::StringArray wanted;
        for (auto& c : tagChips) if (c->lit) wanted.add (c->getText());
        rows.clear();
        int shown = 0, inB = 0;
        juce::String lastGroup;
        for (int i = 0; i < (int) presets.size(); ++i)
        {
            const auto& r = presets[(size_t) i];
            if (! inBranch (r)) continue;
            ++inB;
            if (query.isNotEmpty() && ! r.name.containsIgnoreCase (query) && ! r.description.containsIgnoreCase (query)
                && ! r.folder.containsIgnoreCase (query)) continue;
            bool tagsOk = true;
            for (auto& t : wanted) tagsOk = tagsOk && r.tags.contains (t);
            if (! tagsOk) continue;
            const juce::String group = r.factory ? r.category : juce::String ("User presets");
            if (branch == "All" && group != lastGroup) { lastGroup = group; rows.push_back ({ true, -1, group }); }
            rows.push_back ({ false, i, r.name });
            ++shown;
        }
        countText = (query.isNotEmpty() || ! wanted.isEmpty()) ? juce::String (shown) + " of " + juce::String (inB) : juce::String (inB) + " presets";
        clearSearchButton.setVisible (query.isNotEmpty());
        clearTagsButton.setVisible (! wanted.isEmpty());
        presetList.updateContent();
        presetList.repaint();
        branchList.repaint();
        repaint();
    }

    void showCurrentInList()
    {
        for (int i = 0; i < (int) rows.size(); ++i)
            if (! rows[(size_t) i].header && isCurrentRow (i)) { presetList.scrollToEnsureRowIsOnscreen (i); break; }
    }
    bool isCurrentRow (int row) const
    {
        const auto& r = presets[(size_t) rows[(size_t) row].preset];
        if (processor.isRandomSound()) return false;
        const auto file = processor.getCurrentPresetFile();
        return r.factory ? (file == juce::File() && r.name == processor.getCurrentPresetName()) : r.file == file;
    }

    void paintBranch (int row, juce::Graphics& g, int w, int h)
    {
        if (row < 0 || row >= (int) branches.size()) return;
        const auto& b = branches[(size_t) row];
        const bool on = b.key == branch;
        if (b.key == "User") { g.setColour (col::outline.withAlpha (0.25f)); g.fillRect (12, 0, w - 24, 1); }
        if (on) { g.setColour (col::amber.withAlpha (0.22f)); g.fillRect (0, 0, w, h); g.setColour (col::amber); g.fillRect (0, 0, 3, h); }
        float x = b.folder ? 36.0f : 14.0f;
        if (b.user)
        {   // folder icon
            juce::Path pth;
            const float fy = (float) h * 0.5f - 6.0f;
            pth.startNewSubPath (x, fy + 1.0f); pth.lineTo (x + 6.0f, fy + 1.0f); pth.lineTo (x + 8.0f, fy + 3.0f); pth.lineTo (x + 15.0f, fy + 3.0f);
            pth.lineTo (x + 15.0f, fy + 12.0f); pth.lineTo (x, fy + 12.0f); pth.closeSubPath();
            g.setColour (col::teal.brighter (0.4f));
            g.strokePath (pth, juce::PathStrokeType (1.4f));
            x += 22.0f;
        }
        int count = 0;
        for (auto& r : presets)
        {
            if (b.key == "All") ++count;
            else if (b.key == "User") count += r.factory ? 0 : 1;
            else if (b.folder) count += (! r.factory && (r.folder == b.label || r.folder.startsWith (b.label + "/"))) ? 1 : 0;
            else count += (r.factory && r.category == b.key) ? 1 : 0;
        }
        label (g, b.label, juce::Rectangle<float> (x, 0.0f, (float) w - x - 40.0f, (float) h), on ? col::ledAmber.brighter (0.2f) : col::ink, 16.0f,
               on ? Weight::NarrowBold : Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
        label (g, juce::String (count), juce::Rectangle<float> ((float) w - 44.0f, 0.0f, 32.0f, (float) h), col::ink.withAlpha (0.55f), 14.0f,
               Weight::Narrow, juce::Justification::centredRight, 0.0f);
    }

    void paintPreset (int row, juce::Graphics& g, int w, int h)
    {
        if (row < 0 || row >= (int) rows.size()) return;
        const auto& e = rows[(size_t) row];
        if (e.header)
        {
            g.setColour (juce::Colour (0xff1d1c1a)); g.fillRect (0, 0, w, h);
            label (g, e.text.toUpperCase(), juce::Rectangle<float> (14.0f, 0.0f, (float) w - 20.0f, (float) h), col::amber.brighter (0.2f), 14.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.18f);
            return;
        }
        const auto& r = presets[(size_t) e.preset];
        const bool sel = isCurrentRow (row);
        if (sel) { g.setColour (col::amber.withAlpha (0.22f)); g.fillRect (0, 0, w, h); g.setColour (col::amber); g.fillRect (0, 0, 3, h); }
        float right = (float) w - 12.0f;
        if (! r.factory)
        {
            const juce::Rectangle<float> tag (right - 44.0f, (float) h * 0.5f - 9.0f, 44.0f, 18.0f);
            g.setColour (col::teal.brighter (0.35f));
            g.fillRoundedRectangle (tag, 3.0f);
            label (g, "USER", tag, col::inkDark, 11.0f, Weight::NarrowBold, juce::Justification::centred, 0.1f);
            right -= 52.0f;
            if (r.folder.isNotEmpty() && branch != "User/" + r.folder)
            {
                label (g, r.folder, juce::Rectangle<float> (right - 160.0f, 0.0f, 160.0f, (float) h), col::ink.withAlpha (0.5f), 14.0f, Weight::Narrow, juce::Justification::centredRight, 0.0f);
                right -= 168.0f;
            }
        }
        const auto tagText = r.tags.joinIntoString ("  ");
        label (g, tagText, juce::Rectangle<float> (right - 230.0f, 0.0f, 230.0f, (float) h), col::ink.withAlpha (0.38f), 12.0f, Weight::NarrowSemi, juce::Justification::centredRight, 0.06f);
        label (g, r.name, juce::Rectangle<float> (24.0f, 0.0f, right - 260.0f, (float) h), sel ? col::ledAmber.brighter (0.2f) : col::ink, 17.0f, Weight::NarrowSemi, juce::Justification::centredLeft, 0.01f);
    }

    void selectBranch (int row)
    {
        if (row < 0 || row >= (int) branches.size()) return;
        branch = branches[(size_t) row].key;
        rebuildRows();
        presetList.scrollToEnsureRowIsOnscreen (0);
    }

    void loadRow (int row)
    {
        if (row < 0 || row >= (int) rows.size() || rows[(size_t) row].header) return;
        const auto& r = presets[(size_t) rows[(size_t) row].preset];
        processor.loadPreset (r);
        setStatus ({});
        presetList.repaint();
    }

    const BabushkaProcessor::PresetRef* currentRef() const
    {
        if (processor.isRandomSound()) return nullptr;
        const auto file = processor.getCurrentPresetFile();
        for (auto& r : presets)
            if (r.factory ? (file == juce::File() && r.name == processor.getCurrentPresetName()) : r.file == file) return &r;
        return nullptr;
    }

    juce::String description() const
    {
        const auto cur = processor.getCurrentPresetName();
        if (processor.isRandomSound()) return cur + " - made by RANDOMIZE, not saved. SAVE AS... keeps it.";
        if (auto* r = currentRef())
        {
            juce::String d = r->factory ? r->description : "User preset" + (r->folder.isNotEmpty() ? " in " + r->folder : juce::String()) + ".";
            if (! r->tags.isEmpty()) d << "   [" << r->tags.joinIntoString (", ") << "]";
            if (processor.isSoundModified()) d = "(edited) " + d;
            return d;
        }
        return cur;
    }

    void confirmSave()
    {
        const auto name = saveDialog.name(), folder = saveDialog.folder();
        if (name.isEmpty()) { saveDialog.status = "Type a name first."; saveDialog.repaint(); return; }
        const auto file = processor.userPresetFile (name, folder);
        if (file.existsAsFile() && ! saveDialog.confirmReplace)
        {
            saveDialog.confirmReplace = true;
            saveDialog.status = "'" + name + "' exists - SAVE again to replace it.";
            saveDialog.repaint();
            return;
        }
        const auto saved = processor.saveUserPreset (name, folder, saveDialog.tags());
        if (! saved.existsAsFile()) { saveDialog.status = "Could not save to " + file.getParentDirectory().getFullPathName(); saveDialog.repaint(); return; }
        saveDialog.setVisible (false);
        refresh();
        const auto rel = saved.getParentDirectory() == processor.getUserPresetDirectory() ? juce::String() : saved.getParentDirectory().getRelativePathFrom (processor.getUserPresetDirectory()).replaceCharacter ('\\', '/');
        branch = rel.isEmpty() ? "User" : "User/" + rel;
        rebuildRows();
        showCurrentInList();
        setStatus ("Saved '" + saved.getFileNameWithoutExtension() + "' in User presets" + (rel.isNotEmpty() ? " / " + rel : juce::String()) + ".");
    }

    void deleteCurrent()
    {
        auto* r = currentRef();
        if (r == nullptr || r->factory) { setStatus ("Only your own presets (USER) can be deleted."); return; }
        if (pendingDelete != r->file.getFullPathName())
        {
            status = "Press DELETE again to delete '" + r->name + "'.";
            pendingDelete = r->file.getFullPathName();
            repaint();
            return;
        }
        const auto name = r->name;
        const bool ok = processor.deleteUserPreset (r->file);
        refresh();
        setStatus (ok ? "Deleted '" + name + "'." : "Could not delete the file.");
    }

    void importFiles()
    {
        chooser = std::make_unique<juce::FileChooser> ("Import Babushka presets", juce::File::getSpecialLocation (juce::File::userHomeDirectory), "*.babushka");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
                              [this] (const juce::FileChooser& fc) {
                                  const auto files = fc.getResults();
                                  if (files.isEmpty()) return;
                                  const auto folder = branch.startsWith ("User/") ? branch.substring (5) : juce::String();
                                  const int n = processor.importPresets (files, folder);
                                  refresh();
                                  setStatus (juce::String (n) + " of " + juce::String (files.size()) + " presets imported into User presets"
                                             + (folder.isNotEmpty() ? " / " + folder : juce::String()) + "."); });
    }

    void exportFile()
    {
        const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (juce::File::createLegalFileName (processor.getCurrentPresetName()) + ".babushka");
        chooser = std::make_unique<juce::FileChooser> ("Export the current sound", start, "*.babushka");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc) {
                                  const auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  setStatus (processor.exportCurrentSound (f) ? "Exported to " + f.withFileExtension ("babushka").getFullPathName() : "Could not write " + f.getFullPathName()); });
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
