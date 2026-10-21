// Babushka — a pedal's own menu (opened from its display): pedal presets, the mode switch, every knob, a picture of
// what the pedal does right now, ON, LOCK and (tape echo) HOLD.
#pragma once

#include "Overlays.h"
#include <complex>

namespace babushka::ui
{
// A key that is on only while it is held down (the tape echo's HOLD).
class HoldKey : public juce::Component
{
public:
    explicit HoldKey (const juce::String& t) : text (t) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void (bool)> onHold;
    void setLit (bool on) { if (on != lit) { lit = on; repaint(); } }
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
    void mouseDown (const juce::MouseEvent&) override { if (onHold) onHold (true); }
    void mouseUp (const juce::MouseEvent&) override { if (onHold) onHold (false); }
private:
    juce::String text; bool lit = false;
};

// What the pedal does, drawn live: the saturator's transfer curve, the phaser's notches, the chorus' two delay
// sweeps, the echo's tape loop with its heads and the train of repeats.
class PedalPicture : public juce::Component
{
public:
    PedalPicture (BabushkaProcessor& p, int pedalIndex) : processor (p), pedal (pedalIndex) { setInterceptsMouseClicks (false, false); }
    void advance (float seconds)
    {
        if (pedal == 1) phase += babushka::dsp::pedallaw::phaserHz (pedalValue (processor, "phaRate")) * seconds;
        if (pedal == 2) phase += babushka::dsp::pedallaw::chorusHz (pedalValue (processor, "choRate")) * seconds;
        phase -= std::floor (phase);
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0c0905));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        const auto area = r.reduced (14.0f, 12.0f).withTrimmedBottom (18.0f);
        const auto lcd = pedalLook (pedal).lcd;
        const auto grid = lcd.withAlpha (0.13f);
        juce::String caption;
        switch (pedal)
        {
            case 0: caption = saturator (g, area, lcd, grid); break;
            case 1: caption = phaser (g, area, lcd, grid); break;
            case 2: caption = chorus (g, area, lcd, grid); break;
            default: caption = echo (g, area, lcd, grid); break;
        }
        label (g, caption, r.withTop (r.getBottom() - 24.0f).reduced (10.0f, 0.0f), lcd.withAlpha (0.7f), 12.5f, Weight::NarrowSemi, juce::Justification::centred, 0.03f);
    }

private:
    BabushkaProcessor& processor;
    int pedal;
    float phase = 0.0f;

    float v (const char* id) const { return pedalValue (processor, id); }
    static float tri (float x) { x -= std::floor (x); return x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x; }

    juce::String saturator (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c, juce::Colour grid)
    {
        const int mode = (int) v ("satMode");
        const float drive = v ("satDrive"), bias = v ("satBias"), gain = std::pow (10.0f, 36.0f * drive / 20.0f) * 1.2f;
        auto shape = [&] (float x) {
            const float u = 0.5f * x * gain;                     // ±0.5: about the loudest the voice gets into the pedal
            switch (mode)
            {
                case 0: { const float b = 0.6f * bias;
                          auto t = [] (float w) { return w >= 0.0f ? std::tanh (w) : (std::exp (w) - 1.0f) * 0.85f + std::tanh (w) * 0.15f; };
                          return t (u + b) - t (b); }
                case 1: { const float w = 0.06f + 0.18f * bias; return u / std::sqrt (1.0f + u * u) * (1.0f - 0.5f * w) + w * 0.5f * std::tanh (u); }
                case 2: return std::tanh (0.8f * std::tanh (1.6f * u) / 1.6f) / 0.8f;
                default: { const float k = u >= 0.0f ? 2.5f : 2.5f - 1.2f * bias; return u / std::pow (1.0f + std::pow (std::abs (u), k), 1.0f / k); }
            } };
        const float peak = juce::jmax (std::abs (shape (1.0f)), std::abs (shape (-1.0f)), 0.2f);
        const auto sq = a.withSizeKeepingCentre (juce::jmin (a.getWidth(), a.getHeight()), juce::jmin (a.getWidth(), a.getHeight()));
        g.setColour (grid);
        g.drawLine (sq.getX(), sq.getCentreY(), sq.getRight(), sq.getCentreY());
        g.drawLine (sq.getCentreX(), sq.getY(), sq.getCentreX(), sq.getBottom());
        const float dash[] = { 4.0f, 4.0f };
        g.drawDashedLine ({ sq.getBottomLeft(), sq.getTopRight() }, dash, 2, 1.0f);
        juce::Path p;
        for (int i = 0; i <= 120; ++i)
        {
            const float x = -1.0f + 2.0f * (float) i / 120.0f;
            const float y = shape (x) / peak;
            const juce::Point<float> pt (sq.getCentreX() + x * sq.getWidth() * 0.5f, sq.getCentreY() - y * sq.getHeight() * 0.5f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (c.withAlpha (0.25f));
        g.strokePath (p, juce::PathStrokeType (6.0f));
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (2.2f));
        return "input -> output";
    }

    juce::String phaser (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c, juce::Colour grid)
    {
        const int mode = (int) v ("phaMode");
        const int stages = mode == 1 ? 6 : (mode == 3 ? 8 : 4);
        const float depth = v ("phaDepth"), mix = v ("phaMix");
        const float fb = (mode == 2 ? 0.92f : 0.6f) * v ("phaFeedback");
        const float center = 120.0f * std::pow (2.0f, 3.6f * v ("phaCenter"));
        const float t = tri (phase), lfo = t * (1.5f - 0.5f * t * t);
        const float f0 = center * std::exp2 (2.2f * depth * lfo);
        auto xOf = [&a] (float f) { return a.getX() + a.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); };
        auto yOf = [&a] (float db) { return a.getY() + a.getHeight() * (12.0f - db) / 36.0f; };
        g.setColour (grid);
        for (float f : { 100.0f, 1000.0f, 10000.0f }) g.drawVerticalLine ((int) xOf (f), a.getY(), a.getBottom());
        g.drawHorizontalLine ((int) yOf (0.0f), a.getX(), a.getRight());
        juce::Path p;
        for (int i = 0; i <= 160; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, (float) i / 160.0f);
            std::complex<float> ap (1.0f, 0.0f);
            static const float spread[8] = { 1.0f, 1.07f, 0.94f, 1.03f, 0.97f, 1.09f, 0.92f, 1.05f };
            for (int s = 0; s < stages; ++s) ap *= std::polar (1.0f, -2.0f * std::atan (f / (f0 * spread[s])));
            const auto wet = ap / (1.0f - fb * ap);
            const float mag = std::abs ((1.0f - mix) + mix * wet) * (1.0f + 0.4f * mix);
            const float db = juce::jlimit (-24.0f, 12.0f, 20.0f * std::log10 (juce::jmax (mag, 1.0e-4f)));
            const juce::Point<float> pt (xOf (f), yOf (db));
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (c.withAlpha (0.25f));
        g.strokePath (p, juce::PathStrokeType (6.0f));
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (2.2f));
        return juce::String (stages) + " stages, the notches sweep at " + pedalText (processor, "phaRate");
    }

    juce::String chorus (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c, juce::Colour grid)
    {
        const int mode = (int) v ("choMode");
        const float depth = v ("choDepth"), width = v ("choWidth");
        g.setColour (grid);
        g.drawHorizontalLine ((int) a.getCentreY(), a.getX(), a.getRight());
        // two seconds of the delay time of the left and the right line, newest on the right
        const float hz = babushka::dsp::pedallaw::chorusHz (v ("choRate"));
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::Path p;
            for (int i = 0; i <= 160; ++i)
            {
                const float back = 2.0f * (1.0f - (float) i / 160.0f);
                float l = tri (phase - hz * back);
                if (ch == 1) l *= (1.0f - 2.0f * width);
                const float y = a.getCentreY() - l * depth * a.getHeight() * 0.42f;
                const juce::Point<float> pt (a.getX() + a.getWidth() * (float) i / 160.0f, y);
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            g.setColour ((ch == 0 ? c : c.withRotatedHue (0.12f)).withAlpha (ch == 0 ? 1.0f : 0.75f));
            g.strokePath (p, juce::PathStrokeType (2.0f));
        }
        label (g, "L", a.removeFromLeft (14.0f).withHeight (16.0f), c, 12.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.0f);
        static const char* what[] = { "one line per side, gentle", "deeper and quicker", "fast and shallow", "pitch vibrato, no dry signal", "three lines per side" };
        return juce::String ("delay time L / R: ") + what[juce::jlimit (0, 4, mode)];
    }

    juce::String echo (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c, juce::Colour grid)
    {
        static const int heads[][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 1, 0 }, { 0, 1, 1 }, { 1, 0, 1 }, { 1, 1, 1 } };
        const int mode = juce::jlimit (0, 6, (int) v ("tapHeads"));
        const int sync = (int) v ("tapSync");
        const float fbk = v ("tapFeedback");
        // the tape loop with the record head and the three playback heads
        const auto loop = a.removeFromTop (a.getHeight() * 0.48f).reduced (10.0f, 6.0f);
        g.setColour (grid.withAlpha (0.4f));
        g.drawRoundedRectangle (loop, loop.getHeight() * 0.5f, 3.0f);
        auto head = [&] (float x, bool lit, const juce::String& name) {
            const auto h = juce::Rectangle<float> (18.0f, 14.0f).withCentre ({ x, loop.getBottom() });
            g.setColour (lit ? c : juce::Colour (0xff2a3a2a));
            g.fillRoundedRectangle (h, 3.0f);
            label (g, name, h.translated (0.0f, -16.0f), lit ? c : c.withAlpha (0.35f), 11.0f, Weight::NarrowBold, juce::Justification::centred, 0.0f);
        };
        head (loop.getX() + loop.getWidth() * 0.2f, true, "REC");
        for (int i = 0; i < 3; ++i) head (loop.getX() + loop.getWidth() * (0.42f + 0.18f * (float) i), heads[mode][i] != 0, juce::String (i + 1));
        // the repeats: one bar per echo over two seconds, each lower than the one before
        const float h1 = sync > 0 ? 0.0f : babushka::dsp::pedallaw::echoSeconds (v ("tapTime"));
        const auto bars = a.reduced (6.0f, 4.0f);
        g.setColour (grid);
        g.drawHorizontalLine ((int) bars.getBottom(), bars.getX(), bars.getRight());
        if (h1 > 0.0f)
        {
            int longest = 1;
            for (int i = 0; i < 3; ++i) if (heads[mode][i]) longest = i + 1;
            for (int i = 0; i < 3; ++i)
            {
                if (! heads[mode][i]) continue;
                float level = 0.9f;
                for (float t = h1 * (float) (i + 1); t < 2.0f; t += h1 * (float) longest)
                {
                    const float x = bars.getX() + bars.getWidth() * t / 2.0f;
                    g.setColour (c.withAlpha (0.35f + 0.65f * level));
                    g.fillRect (x - 1.5f, bars.getBottom() - bars.getHeight() * level, 3.0f, bars.getHeight() * level);
                    level *= fbk;
                    if (level < 0.03f) break;
                }
            }
            juce::StringArray taps;
            for (int i = 0; i < 3; ++i) if (heads[mode][i]) taps.add (juce::String (juce::roundToInt (1000.0f * h1 * (float) (i + 1))) + " ms");
            return "repeats: " + taps.joinIntoString ("  ");
        }
        return "head 1 locked to the tempo: " + pedalText (processor, "tapSync");
    }
};

class PedalOverlay : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    PedalOverlay (BabushkaProcessor& p, int pedalIndex)
        : processor (p), pedal (pedalIndex), info (babushka::pedalInfo()[(size_t) pedalIndex]), look (pedalLook (pedalIndex)),
          saveButton ("SAVE"), deleteButton ("DELETE"), closeButton ("CLOSE"), picture (p, pedalIndex)
    {
        setLookAndFeel (&lookAndFeel);
        list.setModel (this);
        list.setRowHeight (32);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0b0a09));
        list.setColour (juce::ListBox::outlineColourId, col::outline.withAlpha (0.3f));
        list.setOutlineThickness (1);
        addAndMakeVisible (list);
        styleField (nameField, "name of your setting");
        addAndMakeVisible (nameField);
        for (auto* b : { &saveButton, &deleteButton, &closeButton }) addAndMakeVisible (*b);
        saveButton.onClick = [this] { save(); };
        nameField.onReturnKey = [this] { save(); };
        deleteButton.onClick = [this] { removeSelected(); };
        closeButton.onClick = [this] { if (onClose) onClose(); };

        addAndMakeVisible (onChip);
        onChip.onClick = [this] { setPedalParam (processor, info.onId, pedalValue (processor, info.onId) > 0.5f ? 0.0f : 1.0f); refresh(); };
        addAndMakeVisible (lockChip);
        lockChip.onClick = [this] { processor.setFxLocks (processor.getFxLocks() ^ (1 << pedal)); refresh(); };
        if (pedal == 3)
        {
            addAndMakeVisible (holdKey);
            holdKey.onHold = [this] (bool held) { setPedalParam (processor, "tapHold", held ? 1.0f : 0.0f, false); refresh(); };
        }
        auto addChips = [this] (const juce::String& id, std::vector<std::unique_ptr<ChipButton>>& chips) {
            if (auto* rp = pedalParam (processor, id))
            {
                const auto items = rp->getAllValueStrings();
                for (int i = 0; i < items.size(); ++i)
                {
                    auto chip = std::make_unique<ChipButton> (items[i] == "1/8 D" ? juce::String ("1/8 DOT") : items[i]);
                    chip->onClick = [this, id, i] { setPedalParam (processor, id, (float) i); refresh(); };
                    addAndMakeVisible (*chip);
                    chips.push_back (std::move (chip));
                }
            } };
        addChips (info.modeId, modeChips);
        if (pedal == 3) addChips ("tapSync", syncChips);
        for (auto& k : info.knobs)
        {
            auto knob = std::make_unique<Knob> (p.apvts, k.id, 46.0f);
            addAndMakeVisible (*knob);
            knobs.push_back (std::move (knob));
        }
        addAndMakeVisible (picture);
        setSize (1000, 620);
        reload();
        startTimerHz (30);
    }
    ~PedalOverlay() override { setLookAndFeel (nullptr); }

    std::function<void()> onClose;
    int pedalIndex() const { return pedal; }

    void paint (juce::Graphics& g) override
    {
        const auto frame = getLocalBounds().toFloat().reduced (4.0f);
        drawDialogFrame (g, frame, info.name);
        // a stripe in the pedal's colour under the title
        g.setColour (look.top);
        g.fillRoundedRectangle (juce::Rectangle<float> (120.0f, 4.0f).withCentre ({ frame.getCentreX(), 58.0f }), 2.0f);
        static const char* tagline[] = { "warm drive: tube, tape, transformer, diodes", "the swirl of the classic JFET and OTA phasers",
                                         "bucket-brigade chorus with its compander and hiss", "three playback heads on a loop of tape" };
        label (g, tagline[pedal], { 0.0f, 64.0f, (float) getWidth(), 22.0f }, col::ink.withAlpha (0.7f), 15.0f, Weight::NarrowSemi, juce::Justification::centred, 0.02f);

        auto heading = [&g] (const juce::String& t, float x, float y, float w) {
            label (g, t, { x, y, w, 20.0f }, col::amber.brighter (0.2f), 13.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.16f);
        };
        heading ("PEDAL PRESETS", 34.0f, 98.0f, 260.0f);
        heading (pedal == 3 ? "HEADS" : pedal == 0 ? "CHARACTER" : "MODE", 340.0f, 98.0f, 200.0f);
        if (pedal == 3) heading ("SYNC", 340.0f, 150.0f, 200.0f);
        // knob names and values
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const auto b = knobs[i]->getBounds().toFloat();
            label (g, info.knobs[i].name, { b.getCentreX() - 50.0f, b.getBottom() + 1.0f, 100.0f, 18.0f }, col::ink, 13.5f, Weight::NarrowBold,
                   juce::Justification::centred, 0.08f);
            label (g, pedalText (processor, info.knobs[i].id), { b.getCentreX() - 50.0f, b.getBottom() + 18.0f, 100.0f, 18.0f }, col::ledAmber, 13.5f,
                   Weight::NarrowSemi, juce::Justification::centred, 0.02f);
        }
        // what the selected preset is about, or what the controls do
        label (g, about, { 340.0f, 482.0f, (float) getWidth() - 374.0f, 44.0f }, col::ink.withAlpha (0.8f), 15.0f, Weight::Narrow, juce::Justification::topLeft, 0.01f, 2);
        juce::String foot = juce::String (info.knobs[0].name) + " and " + info.knobs[1].name + " are also on the pedal. LOCK keeps this pedal when presets change.";
        if (processor.getFxLocks() & BabushkaProcessor::kLockAllFx) foot = "FX lock is on in the preset block: every pedal stays as it is when presets change.";
        label (g, foot, { 34.0f, (float) getHeight() - 58.0f, (float) getWidth() - 220.0f, 30.0f }, col::ink.withAlpha (0.6f), 14.0f, Weight::Narrow,
               juce::Justification::centredLeft, 0.01f);
    }

    void resized() override
    {
        list.setBounds (34, 120, 270, 32 * 10 + 2);
        nameField.setBounds (34, 452, 270, 34);
        saveButton.setBounds (34, 494, 130, 32);
        deleteButton.setBounds (174, 494, 130, 32);
        closeButton.setBounds (getWidth() - 150, getHeight() - 62, 116, 36);
        onChip.setBounds (getWidth() - 270, 22, 70, 28);
        lockChip.setBounds (getWidth() - 190, 22, 76, 28);
        if (pedal == 3) holdKey.setBounds (getWidth() - 106, 22, 72, 28);
        auto place = [] (std::vector<std::unique_ptr<ChipButton>>& chips, int x, int y) {
            for (auto& c : chips)
            {
                const int w = juce::jmax (52, (int) juce::GlyphArrangement::getStringWidth (font (Weight::NarrowBold, 13.0f, 0.08f), c->getText()) + 28);
                c->setBounds (x, y, w, 30);
                x += w + 6;
            }
        };
        place (modeChips, 340, 120);
        place (syncChips, 340, 172);
        const int top = pedal == 3 ? 218 : 166;
        picture.setBounds (340, top, 280, pedal == 3 ? 250 : 302);
        // knobs: two rows on the right of the picture
        const int cols = (int) (knobs.size() + 1) / 2;
        const float x0 = 640.0f, x1 = (float) getWidth() - 34.0f;
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const int row = (int) i / cols, col = (int) i % cols;
            const float cx = x0 + (x1 - x0) * ((float) col + 0.5f) / (float) cols;
            const float cy = (float) top + 52.0f + (float) row * 140.0f;
            knobs[i]->setCentrePosition ((int) cx, (int) cy);
        }
    }

private:
    BabushkaProcessor& processor;
    int pedal;
    const babushka::PedalInfo& info;
    const PedalLook& look;
    DialogLookAndFeel lookAndFeel;
    juce::ListBox list;
    juce::TextEditor nameField;
    PlateButton saveButton, deleteButton, closeButton;
    ChipButton onChip { "ON" }, lockChip { "LOCK" };
    HoldKey holdKey { "HOLD" };
    std::vector<std::unique_ptr<ChipButton>> modeChips, syncChips;
    std::vector<std::unique_ptr<Knob>> knobs;
    PedalPicture picture;
    std::vector<PedalPresetEntry> presets;
    juce::String about, matched, valuesSeen;

    void reload()
    {
        presets = PedalPresets::list (processor, pedal);
        list.updateContent();
        refresh();
    }

    void refresh()
    {
        matched = PedalPresets::match (processor, pedal);
        for (int i = 0; i < (int) presets.size(); ++i)
            if (presets[(size_t) i].name == matched) { about = presets[(size_t) i].about; break; }
        if (matched.isEmpty()) about = "Your own setting - SAVE keeps it under a name.";
        onChip.setLit (pedalValue (processor, info.onId) > 0.5f);
        lockChip.setLit (processor.isPedalLocked (pedal));
        holdKey.setLit (pedal == 3 && pedalValue (processor, "tapHold") > 0.5f);
        const int mode = (int) pedalValue (processor, info.modeId);
        for (int i = 0; i < (int) modeChips.size(); ++i) modeChips[(size_t) i]->setLit (i == mode);
        if (pedal == 3)
        {
            const int sync = (int) pedalValue (processor, "tapSync");
            for (int i = 0; i < (int) syncChips.size(); ++i) syncChips[(size_t) i]->setLit (i == sync);
        }
        int selected = -1;
        for (int i = 0; i < (int) presets.size(); ++i) if (presets[(size_t) i].name == matched) selected = i;
        if (selected >= 0) list.selectRow (selected, true, true); else list.deselectAllRows();
        deleteButton.setEnabled (selected >= 0 && ! presets[(size_t) selected].factory);
        list.repaint();
        repaint();
    }

    void timerCallback() override
    {
        picture.advance (1.0f / 30.0f);
        // knobs moved here, on the pedal, by MIDI or by the host: update the read-outs and the selection
        juce::String seen;
        for (auto& [id, val] : PedalPresets::current (processor, pedal)) seen << id << juce::String (val, 4);
        seen << (int) (pedalValue (processor, info.onId) > 0.5f) << processor.getFxLocks() << (int) (pedalValue (processor, "tapHold") > 0.5f);
        if (seen != valuesSeen) { valuesSeen = seen; refresh(); }
    }

    void save()
    {
        const auto name = nameField.getText().trim();
        if (name.isEmpty()) { nameField.grabKeyboardFocus(); return; }
        if (PedalPresets::save (processor, pedal, name).existsAsFile()) { nameField.clear(); reload(); }
    }
    void removeSelected()
    {
        const int row = list.getSelectedRow();
        if (row < 0 || row >= (int) presets.size() || presets[(size_t) row].factory) return;
        PedalPresets::remove (processor, pedal, presets[(size_t) row]);
        reload();
    }

    int getNumRows() override { return (int) presets.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (row < 0 || row >= (int) presets.size()) return;
        const auto& e = presets[(size_t) row];
        if (selected)
        {
            g.setColour (look.lcd.withAlpha (0.16f));
            g.fillRect (0, 0, width, height);
            g.setColour (look.lcd);
            g.fillRect (0, 0, 3, height);
        }
        label (g, e.name, { 14.0f, 0.0f, (float) width - 70.0f, (float) height }, selected ? look.lcd : col::ink, 16.0f, Weight::NarrowSemi,
               juce::Justification::centredLeft, 0.01f);
        if (! e.factory)
            label (g, "USER", { (float) width - 56.0f, 0.0f, 46.0f, (float) height }, col::ink.withAlpha (0.45f), 11.0f, Weight::NarrowBold,
                   juce::Justification::centredRight, 0.12f);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        if (row < 0 || row >= (int) presets.size()) return;
        if (pedalValue (processor, info.onId) < 0.5f) setPedalParam (processor, info.onId, 1.0f, false);   // a preset you pick is one you want to hear
        PedalPresets::apply (processor, pedal, presets[(size_t) row]);
        refresh();
    }
};
} // namespace babushka::ui
