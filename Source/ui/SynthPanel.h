// Babushka — the instrument: module plates, knobs, switches, jacks, left-hand controller, keyboard, rear strip.
//
// The layout follows the signal flow (modules left to right: ARP/SEQ, MODULATION, OSCILLATORS, MIXER,
// UTILITIES, FILTER, ENVELOPE, OUTPUT + SPRING REVERB; title strip; left-hand controller and keyboard below) in a
// fixed logical coordinate system. Static artwork is a list of drawing operations recorded while the layout is
// built, so every coordinate is written down once.
#pragma once

#include "KeyboardWidget.h"
#include "Widgets.h"
#include "CableLayer.h"
#include "RearPanel.h"
#include "../dsp/ArpSeq.h"
#include <functional>

namespace babushka::ui
{
// ---- small plate button (SETTINGS, RANDOMIZE, UNDO / REDO, arrows) -------------------------------------------------
class PlateButton : public juce::Component
{
public:
    PlateButton (const juce::String& t, float textHeight = type::title) : text (t), height (textHeight) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick, onRightClick;          // right click (or ctrl-click on a Mac) opens a menu where there is one
    void setText (const juce::String& t) { text = t; repaint(); }
    bool primary = false;                                  // the main button of a dialog: lit amber
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (3.0f);
        const bool lit = primary && isEnabled();
        drawKey (g, r, lit, down);
        label (g, text, r.translated (0.0f, down ? 1.0f : 0.0f), lit ? juce::Colour (0xff2a1700) : col::ink.withAlpha (isEnabled() ? 1.0f : 0.35f), height,
               Weight::NarrowBold, juce::Justification::centred, 0.12f);
    }
    void enablementChanged() override { repaint(); }
    void mouseDown (const juce::MouseEvent& e) override { if (e.mods.isPopupMenu()) return; down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
        down = false; repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
    }
private:
    juce::String text; float height; bool down = false;
};

// ---- preset display: amber LCD with the preset name; a click opens the preset browser ------------------------------
class PresetDisplay : public juce::Component
{
public:
    PresetDisplay() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick;
    void setName (const juce::String& n) { if (n != name) { name = n; repaint(); } }
    void setBadge (const juce::String& b) { if (b != badge) { badge = b; repaint(); } }      // "NOT SAVED", "EDITED" or empty

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 6.0f);
        // chrome bezel round the glass, as on the panel's other screens
        juce::ColourGradient bezel (juce::Colour (0xffd7d9dc), 0.0f, r.getY(), juce::Colour (0xffacb0b5), 0.0f, r.getBottom(), false);
        bezel.addColour (0.5, juce::Colour (0xff62666b));
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (r, 6.0f);
        const auto lcd = r.reduced (3.5f);
        juce::ColourGradient glass (juce::Colour (0xff1b130a), lcd.getX(), lcd.getY(), juce::Colour (0xff0a0704), lcd.getX(), lcd.getBottom(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (lcd, 4.0f);
        const auto amber = col::ledAmber.brighter (hover ? 0.25f : 0.1f);
        juce::ColourGradient backlight (amber.withAlpha (hover ? 0.22f : 0.15f), lcd.getCentreX(), lcd.getCentreY(), amber.withAlpha (0.02f), lcd.getX(), lcd.getY(), true);
        g.setGradientFill (backlight);
        g.fillRoundedRectangle (lcd.reduced (2.0f), 3.0f);
        auto text = lcd.withTrimmedLeft (34.0f).withTrimmedRight (34.0f);
        if (badge.isNotEmpty())
        {
            const auto bf = font (Weight::NarrowBold, 11.0f, 0.12f);
            const float bw = juce::GlyphArrangement::getStringWidth (bf, badge) + 12.0f;
            const juce::Rectangle<float> b (text.getCentreX() - bw * 0.5f, lcd.getY() + 4.0f, bw, 14.0f);
            g.setColour (amber.withAlpha (0.85f));
            g.fillRoundedRectangle (b, 3.0f);
            label (g, badge, b, juce::Colour (0xff1a1306), 11.0f, Weight::NarrowBold, juce::Justification::centred, 0.12f);
            text = text.withTrimmedTop (14.0f);
        }
        float size = 27.0f;                                                                               // long names shrink
        while (size > 17.0f && juce::GlyphArrangement::getStringWidth (font (Weight::Bold, size, 0.06f), name.toUpperCase()) > text.getWidth()) size -= 1.0f;
        g.setFont (font (Weight::Bold, size, 0.06f));
        for (int i = 3; i >= 1; --i)                                                                  // glow of the segments
        {
            g.setColour (amber.withAlpha (0.10f));
            for (auto d : { juce::Point<float> (-0.8f * (float) i, 0.0f), juce::Point<float> (0.8f * (float) i, 0.0f), juce::Point<float> (0.0f, 0.8f * (float) i) })
                g.drawText (name.toUpperCase(), text.translated (d.x, d.y), juce::Justification::centred, true);
        }
        g.setColour (amber);
        g.drawText (name.toUpperCase(), text, juce::Justification::centred, true);
        // list icon: this display opens the browser
        g.setColour (amber.withAlpha (hover ? 0.95f : 0.55f));
        for (int i = 0; i < 3; ++i)
            g.fillRoundedRectangle (lcd.getRight() - 26.0f, lcd.getCentreY() - 7.0f + 6.0f * (float) i, 14.0f, 2.2f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));                                          // glass reflection
        g.fillRoundedRectangle (lcd.withHeight (lcd.getHeight() * 0.36f).reduced (3.0f, 2.0f), 3.0f);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }

private:
    juce::String name, badge;
    bool hover = false;
};

namespace fmt
{
    inline juce::String hz (double v) { return v >= 1000.0 ? juce::String (v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz" : juce::String (v, v >= 100.0 ? 0 : 1) + " Hz"; }
    inline juce::String seconds (double v) { return v < 1.0 ? juce::String (v * 1000.0, v < 0.01 ? 1 : 0) + " ms" : juce::String (v, v < 10.0 ? 2 : 1) + " s"; }
    inline juce::String percent (double v) { return juce::String (v * 100.0, 0) + " %"; }
    inline juce::String bipolar (double v) { return (v >= 0 ? "+" : "") + juce::String (v * 100.0, 0) + " %"; }
    inline juce::String cutoff (double k) { return hz (babushka::dsp::cutoffKnobToHz ((float) juce::jlimit (0.0, 1.0, k))); }
    inline juce::String lfoRate (double k) { return hz (0.07 * std::pow (1300.0 / 0.07, juce::jlimit (0.0, 1.0, k))); }
    inline juce::String envTime (double k, double minS) { return seconds (minS * std::pow (10.0 / minS, juce::jlimit (0.0, 1.0, k))); }
    inline juce::String highPass (double k) { return hz (16.0 * std::pow (500.0, juce::jlimit (0.0, 1.0, k))); }
    inline juce::String glide (double k) { return k <= 0.001 ? juce::String ("off") : seconds (0.002 * std::pow (2500.0, k)); }
    inline juce::String semitones (double v) { return (v >= 0 ? "+" : "") + juce::String (v, 2) + " st"; }
    inline juce::String arpRate (double k) { return juce::String (babushka::dsp::ArpSeq::freeBpm ((float) k), 0) + " BPM"; }
    inline std::function<juce::String (double)> names (std::vector<juce::String> n)
    {
        return [n = std::move (n)] (double v) { const int i = (int) std::lround (v); return i >= 0 && i < (int) n.size() ? n[(size_t) i] : juce::String(); };
    }
}

enum class Glyph { Sine, Triangle, Saw, Ramp, Square, Pulse };

inline void drawGlyph (juce::Graphics& g, Glyph kind, juce::Point<float> c, juce::Colour colour, float s = 5.6f)
{
    const float h = s * 0.8f;
    juce::Path p;
    switch (kind)
    {
        case Glyph::Triangle: p.startNewSubPath (c.x - s, c.y + h); p.lineTo (c.x, c.y - h); p.lineTo (c.x + s, c.y + h); break;
        case Glyph::Saw:      p.startNewSubPath (c.x - s, c.y + h); p.lineTo (c.x + s, c.y - h); p.lineTo (c.x + s, c.y + h); break;
        case Glyph::Ramp:     p.startNewSubPath (c.x - s, c.y + h); p.lineTo (c.x - s, c.y - h); p.lineTo (c.x + s, c.y + h); break;
        case Glyph::Square:   p.startNewSubPath (c.x - s, c.y + h); p.lineTo (c.x - s, c.y - h); p.lineTo (c.x, c.y - h); p.lineTo (c.x, c.y + h); p.lineTo (c.x + s, c.y + h); break;
        case Glyph::Pulse:    p.startNewSubPath (c.x - s, c.y + h); p.lineTo (c.x - s, c.y - h); p.lineTo (c.x - s * 0.25f, c.y - h); p.lineTo (c.x - s * 0.25f, c.y + h); p.lineTo (c.x + s, c.y + h); break;
        case Glyph::Sine:
            p.startNewSubPath (c.x - s, c.y);
            for (int i = 1; i <= 16; ++i) { const float t = (float) i / 16.0f; p.lineTo (c.x - s + 2 * s * t, c.y - h * std::sin (t * juce::MathConstants<float>::twoPi)); }
            break;
    }
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
}

class SynthPanel : public juce::Component, private juce::Timer
{
public:
    using Jack = babushka::dsp::Jack;

    SynthPanel (BabushkaProcessor& p, HintSink& hintSink, std::function<void()> prev, std::function<void()> next,
                std::function<void()> openPresets, std::function<void()> openSettings)
        : processor (p), apvts (p.apvts), hints (hintSink), rear (p), keyboard (p)
    {
        addAndMakeVisible (frontArt);          // behind every control
        addChildComponent (rear);
        buildLayout();
        presetPrev.onClick = std::move (prev);
        presetNext.onClick = std::move (next);
        presetDisplay.onClick = std::move (openPresets);
        settingsButton.onClick = std::move (openSettings);
        randomButton.onClick = [this] {
            if (! processor.randomize()) hints.showHint (randomButton, "No sound passed the checks - try again");
            else hints.showHint (randomButton, processor.getCurrentPresetName() + "  (not saved)"); };
        randomButton.onRightClick = [this] { showRandomMenu(); };
        undoButton.onClick = [this] { processor.undo(); };
        undoButton.onRightClick = [this] { showHistoryMenu(); };
        redoButton.onClick = [this] { processor.redo(); };
        redoButton.onRightClick = [this] { showHistoryMenu(); };
        for (juce::Component* b : { (juce::Component*) &presetPrev, (juce::Component*) &presetNext, (juce::Component*) &settingsButton,
                                    (juce::Component*) &randomButton, (juce::Component*) &undoButton, (juce::Component*) &redoButton,
                                    (juce::Component*) &presetDisplay, (juce::Component*) &fxLock }) addAndMakeVisible (*b);
        // rear strip, right: [UNDO / REDO] [FX lock] [<] display [>]  [SETTINGS / RANDOMIZE], 20 px margins inside the
        // plate (726 … 1418), 14 px between the groups, everything centred on y = 1000. The preset browser opens from the display.
        undoButton.setBounds (746, 963, 70, 32);   redoButton.setBounds (746, 1003, 70, 32);
        fxLock.setBounds (830, 968, 38, 58);
        presetPrev.setBounds (882, 984, 30, 32);  presetDisplay.setBounds (920, 967, 316, 66);  presetNext.setBounds (1244, 984, 30, 32);   // display centre 1078: PRESET is printed over it
        settingsButton.setBounds (1288, 963, 110, 32);  randomButton.setBounds (1288, 1003, 110, 32);
        fxLock.onClick = [this] {
            processor.setFxLocks (processor.getFxLocks() ^ BabushkaProcessor::kLockAllFx);
            const bool on = (processor.getFxLocks() & BabushkaProcessor::kLockAllFx) != 0;
            fxLock.setLocked (on);
            hints.showHint (fxLock, on ? "FX locked: the pedals stay as they are when you change presets" : "FX unlocked: presets set the pedals"); };
        fxLock.onHover = [this] { hints.showHint (fxLock, "FX lock: keep the pedals when you change presets"); };
        fxLock.onLeave = [this] { hints.hideHint(); };
        setupGrip();
        setSize (kW, kH);
        layoutForRear();
        startTimerHz (30);
    }

    // Jack positions for the cable layer; they move while the rear panel slides. onJacksMoved reports every change.
    std::vector<JackSite> jackSites() const
    {
        std::vector<JackSite> all;
        const float r = std::round (rearPx), shift = (float) frontShift();
        for (auto j : frontJacks) { j.pos.y += shift; all.push_back (j); }
        const bool reachable = rearPx >= (float) RearPanel::kHeight - 0.5f;
        for (auto& [id, pos] : rear.jackPositions())
            all.push_back ({ id, { (float) RearPanel::kX + pos.x, (float) RearPanel::kY + r - (float) RearPanel::kHeight + pos.y }, reachable });
        return all;
    }
    std::function<void (std::vector<JackSite>)> onJacksMoved;
    std::function<void (bool)> onRearToggled;         // the user opened or closed the rear panel
    std::function<void()> onOpenMidi;                 // the MIDI socket on the rear panel was clicked
    std::function<void (int)> onOpenPedal;            // a pedal's display was clicked (pedal index)

    // Controls that can be bound to a MIDI knob (MIDI learn on the panel): the control and its parameter.
    struct LearnSite { juce::Component* control; juce::String paramId; };
    const std::vector<LearnSite>& learnSites() const { return learnables; }

    OutputMeters& outputMeters() { return *meters; }

    bool isRearOpen() const { return rearTarget > 0.5f; }
    // Opens or closes the rear panel, with the slide animation or at once.
    void setRearOpen (bool open, bool animate = true)
    {
        rearTarget = open ? (float) RearPanel::kHeight : 0.0f;
        grip.setOpen (open);
        if (! animate) { anim.stopTimer(); rearPx = rearTarget; layoutForRear(); return; }
        animFrom = rearPx; animStart = juce::Time::getMillisecondCounterHiRes();
        anim.startTimerHz (60);
    }

    void paint (juce::Graphics& g) override
    {
        for (auto& op : fixedOps) op (g);
        paintKeysSection (g);
    }

private:
    struct Placed { juce::Component* c; };

    BabushkaProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    HintSink& hints;
    // Static artwork is recorded once. Ops of the front panel (modules and title strip) go into a layer that slides
    // down with the rear panel and is cached as an image; the rest (chassis, bottom strip) stays put.
    enum class Zone { Fixed, Front, Keys };
    Zone zone = Zone::Fixed;
    struct ArtLayer : public juce::Component
    {
        ArtLayer() { setInterceptsMouseClicks (false, false); setBufferedToImage (true); }
        std::vector<std::function<void (juce::Graphics&)>> ops;
        void paint (juce::Graphics& g) override { g.addTransform (juce::AffineTransform::translation (0.0f, -(float) kFrontTop)); for (auto& o : ops) o (g); }
    } frontArt;
    static constexpr int kFrontTop = 56, kFrontBottom = 532;          // the front panel's rows, closed position
    std::vector<std::function<void (juce::Graphics&)>> fixedOps;
    std::vector<JackSite> frontJacks;
    std::vector<std::pair<juce::Component*, juce::Rectangle<int>>> frontComps;   // controls of the front panel, closed position
    RearPanel rear;
    std::vector<LearnSite> learnables;
    GripHandle grip;
    std::unique_ptr<OutputMeters> meters;
    Knob* glideKnob = nullptr;
    float rearPx = 0.0f, rearTarget = 0.0f, animFrom = 0.0f;
    double animStart = 0.0;
    float gripDragStart = 0.0f;
    int knownPatchVersion = -1;
    struct AnimTimer : public juce::Timer { std::function<void()> tick; void timerCallback() override { tick(); } } anim;
    // Left-hand controller and keyboard: positions for the current opening of the rear panel.
    // Left-hand controller on the same edges as the module rows (78 … 294), the keyboard bed 298 … 1418.
    static constexpr float kGlideX = 108.0f, kPadX[3] = { 166.0f, 210.0f, 254.0f }, kWheelX[2] = { 140.0f, 232.0f };
    struct KeysLayout { float top = 552, height = 376, labelsY = 573, padY = 610, glideY = 606, glideLabelY = 640, pillY = 645, subY = 663,
                        wheelTop = 680, wheelBottom = 876, wheelPillY = 901; } kl;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<LeverSwitch>> levers;
    std::vector<std::unique_ptr<Pad>> pads;
    std::vector<std::unique_ptr<Lamp>> lamps;
    std::unique_ptr<Fader> sustain;
    std::unique_ptr<Wheel> pitchWheel, modWheel;
    std::unique_ptr<ParamLink> modLink;
    KeyboardWidget keyboard;
    PlateButton presetPrev { "<", 17.0f }, presetNext { ">", 17.0f }, settingsButton { "SETTINGS" }, randomButton { "RANDOMIZE" },
                undoButton { "UNDO" }, redoButton { "REDO" };
    PresetDisplay presetDisplay;
    FxLockButton fxLock;
    Pad* syncPad = nullptr; Pad* playPad = nullptr; Pad* holdPad = nullptr; Pad* tapPad = nullptr;
    std::unique_ptr<ParamLink> syncLink;
    Lamp* modLamp = nullptr; Lamp* arpLamp = nullptr;
    bool shiftHeld = false, shiftUsed = false;
    int flash[3] { 0, 0, 0 };
    float modPhase = 0.0f, arpPhase = 0.0f;

    // ---- recording of static artwork -------------------------------------------------------------------------------------
    void op (std::function<void (juce::Graphics&)> f) { (zone == Zone::Front ? frontArt.ops : fixedOps).push_back (std::move (f)); }
    // A control of the front panel moves with it when the rear panel opens.
    void placed (juce::Component& c) { if (zone == Zone::Front) frontComps.push_back ({ &c, c.getBounds() }); }

    static void drawText (juce::Graphics& g, const juce::String& t, float x, float y, juce::Colour c, float size = type::control, Weight w = Weight::NarrowBold,
                          float width = 120.0f, float kern = 0.06f)
    {
        label (g, t, juce::Rectangle<float> (width, size * 1.5f).withCentre ({ x, y }), c, size, w, juce::Justification::centred, kern);
    }
    // Label in a box: filled with the skin's colour, or just outlined in the plate's ink when the skin has no fill.
    static void drawPill (juce::Graphics& g, const juce::String& t, float x, float y, float w, juce::Colour fill, juce::Colour plateInk)
    {
        const auto r = juce::Rectangle<float> (w, 17.0f).withCentre ({ x, y });
        const bool outlined = fill.getAlpha() == 0;
        if (outlined) { g.setColour (plateInk.withAlpha (0.9f)); g.drawRoundedRectangle (r, 2.5f, 1.1f); }
        else { g.setColour (fill); g.fillRoundedRectangle (r, 2.5f); }
        label (g, t, r.reduced (6.0f, 0.0f), outlined ? plateInk : inkOn (fill), type::control, Weight::NarrowBold, juce::Justification::centred, 0.08f);
    }
    // A label printed on a module never runs past its frame: centred where it is, it is at most as wide as the room to
    // the nearer edge allows (the text is then squeezed; on small windows the print is enlarged, see label()).
    float fitOnPlate (float x, float y, float width) const
    {
        if (! currentPlate.contains (x, y)) return width;
        const auto inner = currentPlate.reduced (kPlateMargin, 0.0f);
        return juce::jmax (10.0f, juce::jmin (width, 2.0f * juce::jmin (x - inner.getX(), inner.getRight() - x)));
    }
    static constexpr float kPlateMargin = 6.0f;
    void textAt (const juce::String& t, float x, float y, juce::Colour c, float size = type::control, Weight w = Weight::NarrowBold, float width = 120.0f, float kern = 0.06f)
    {
        width = fitOnPlate (x, y, width);
        op ([=] (juce::Graphics& g) { drawText (g, t, x, y, c, size, w, width, kern); });
    }
    void pill (const juce::String& t, float x, float y, float w, juce::Colour fill, juce::Colour plateInk)
    {
        w = fitOnPlate (x, y, w);
        op ([=] (juce::Graphics& g) { drawPill (g, t, x, y, w, fill, plateInk); });
    }
    void module (juce::Rectangle<float> r, juce::Colour fill, const juce::String& title, bool outlined = true)
    {
        currentPlate = r;
        op ([=] (juce::Graphics& g) { drawPlate (g, r, fill, outlined); });
        if (title.isNotEmpty()) textAt (title, r.getCentreX(), r.getY() + 15.0f, inkOn (fill), type::title, Weight::NarrowBold, r.getWidth() - 12.0f, 0.12f);
    }
    void jack (Jack id, float x, float y, const juce::String& text, juce::Colour textColour)
    {
        frontJacks.push_back ({ id, { x, y } });
        op ([=] (juce::Graphics& g) { drawJack (g, { x, y }); });
        if (text.isNotEmpty()) jackLabels.push_back ({ text, x, y + 20.0f, textColour, currentPlate });
    }
    Knob* knob (const char* id, float cx, float cy, float diameter, std::function<juce::String (double)> formatter = nullptr, bool selector = false)
    {
        auto k = std::make_unique<Knob> (apvts, id, diameter, std::move (formatter), selector);
        k->hints = &hints;
        k->setCentrePosition ((int) std::lround (cx), (int) std::lround (cy));
        addAndMakeVisible (*k);
        placed (*k);
        learnables.push_back ({ k.get(), id });
        knobs.push_back (std::move (k));
        return knobs.back().get();
    }
    LeverSwitch* lever (const char* id, float cx, float cy, std::vector<int> map, std::function<juce::String (int)> names = nullptr)
    {
        auto l = std::make_unique<LeverSwitch> (apvts, id, std::move (map), std::move (names));
        l->hints = &hints;
        l->setCentrePosition ((int) std::lround (cx), (int) std::lround (cy));
        addAndMakeVisible (*l);
        placed (*l);
        learnables.push_back ({ l.get(), id });
        levers.push_back (std::move (l));
        return levers.back().get();
    }
    // Names of the three lever positions, under the lever.
    // (each name keeps a few pixels from its neighbours, so the words never run together)
    void leverLabels (float cx, float y, const juce::String& a, const juce::String& b, const juce::String& c, juce::Colour ink, float spread = 28.0f)
    {
        // boxes with clear gaps between them: a name too long for its box is squeezed sideways, never run into the next
        textAt (a, cx - spread, y, ink, type::mark, Weight::NarrowBold, spread - 8.0f, 0.0f);
        textAt (b, cx, y, ink, type::mark, Weight::NarrowBold, spread + 2.0f, 0.0f);     // the middle name may be the longest
        textAt (c, cx + spread, y, ink, type::mark, Weight::NarrowBold, spread - 8.0f, 0.0f);
    }

    // Jack names. Each gets the room between its plate's frame and halfway to the next jack of its row; a name near
    // the frame moves a little inwards. All jack names are one size (in a small window they grow with the rest of the
    // print); a name longer than its room is squeezed sideways, so names never cross the frame or each other.
    struct JackLabel { juce::String text; float x, y; juce::Colour colour; juce::Rectangle<float> plate; };
    std::vector<JackLabel> jackLabels;
    juce::Rectangle<float> currentPlate;

    void placeJackLabels()
    {
        constexpr float natural = 58.0f, gap = 6.0f;
        for (auto& l : jackLabels)
        {
            float lo = l.plate.getX() + 7.0f, hi = l.plate.getRight() - 7.0f;           // inside the printed frame
            for (auto& o : jackLabels)
                if (&o != &l && o.plate == l.plate && std::abs (o.y - l.y) < 12.0f)
                {
                    const float mid = 0.5f * (o.x + l.x);
                    if (o.x < l.x) lo = juce::jmax (lo, mid + gap * 0.5f);
                    else           hi = juce::jmin (hi, mid - gap * 0.5f);
                }
            const float left = juce::jmax (lo, l.x - natural * 0.5f), right = juce::jmin (hi, l.x + natural * 0.5f);
            const float x = 0.5f * (left + right), w = right - left;
            const auto t = l.text; const auto c = l.colour; const float y = l.y;
            op ([=] (juce::Graphics& g) { drawJackName (g, t, { x, y }, w, c, type::jack, isOutputName (t)); });
        }
        jackLabels.clear();
    }
    void ticks (float cx, float cy, float d, int count, juce::Colour c, float length = 0.1f, int majorEvery = 0)
    {
        op ([=] (juce::Graphics& g) { drawKnobTicks (g, { cx, cy }, d, count, c, length, -135.0f, 270.0f, majorEvery); });
    }
    void bipolarMarks (float cx, float cy, float d, juce::Colour c)
    {
        ticks (cx, cy, d, 11, c, 0.1f, 5);
        textAt ("0", cx, cy - d * 0.80f, c, type::mark, Weight::NarrowBold, 20.0f);
        textAt ("-", cx - d * 0.76f, cy - d * 0.34f, c, type::title, Weight::NarrowBold, 14.0f);
        textAt ("+", cx + d * 0.76f, cy - d * 0.34f, c, type::title, Weight::NarrowBold, 14.0f);
    }
    void selectorLabels (float cx, float cy, float d, const std::vector<juce::String>& labels, juce::Colour c)
    {
        const int n = (int) labels.size();
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::degreesToRadians (-135.0f + 270.0f * (float) i / (float) (n - 1));
            const float r = d * 0.5f + 15.0f;
            textAt (labels[(size_t) i], cx + std::sin (a) * r, cy - std::cos (a) * r, c, type::mark, Weight::NarrowBold, 26.0f, 0.02f);
        }
        ticks (cx, cy, d, n, c, 0.08f);
    }
    void selectorGlyphs (float cx, float cy, float d, const std::vector<Glyph>& glyphs, juce::Colour c)
    {
        const int n = (int) glyphs.size();
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::degreesToRadians (-135.0f + 270.0f * (float) i / (float) (n - 1));
            const float r = d * 0.5f + 13.0f;
            const juce::Point<float> p (cx + std::sin (a) * r, cy - std::cos (a) * r);
            const auto gl = glyphs[(size_t) i];
            op ([=] (juce::Graphics& g) { drawGlyph (g, gl, p, c, 5.4f); });
        }
        ticks (cx, cy, d, n, c, 0.06f);
    }

    // Logarithmic frequency scale round the CUTOFF knob: 10 Hz … 11 kHz over 270°, a tick at m × 10^n Hz.
    // The CUTOFF print of the hardware: evenly spaced ticks over the knob's travel, a long one every half decade,
    // 20Hz / 200Hz / 2kHz / 20kHz at the start, a third, two thirds and the end. Like the hardware's, the print is the
    // nominal 20 Hz ... 20 kHz range; the knob itself reaches about 11 kHz (CV goes further), its value shows on hover.
    void cutoffScale (float cx, float cy, float d, juce::Colour c)
    {
        ticks (cx, cy, d, 31, c, 0.09f, 5);
        auto at = [&] (float deg, const juce::String& t) {
            const float a = juce::degreesToRadians (deg), r = d * 0.5f + 25.0f;
            textAt (t, cx + std::sin (a) * r, cy - std::cos (a) * r, c, type::mark, Weight::NarrowBold, 40.0f, 0.02f);
        };
        at (-135.0f, "20Hz"); at (-45.0f, "200Hz"); at (45.0f, "2kHz"); at (135.0f, "20kHz");
    }

    // ---- the layout ----------------------------------------------------------------------------------------------------------
    void buildLayout()
    {
        using namespace babushka::dsp;
        const auto& S = skin();
        const auto ink = inkOn (S.panel);
        const auto arpInk = inkOn (S.arp), oscInk = inkOn (S.osc), filInk = inkOn (S.filter), revInk = inkOn (S.reverb);

        // chassis
        op ([c = S.chassis] (juce::Graphics& g) {
            const juce::Rectangle<float> r (30.0f, 26.0f, 1440.0f, 1040.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (r.translated (0.0f, 6.0f).expanded (4.0f), 22.0f);
            // black powder coat: a rough relief, lit from the top, a little worn
            juce::ColourGradient grad (juce::Colour (0xff3a3a3a), r.getX(), r.getY(), c.darker (0.2f), r.getX(), r.getBottom(), false);
            grad.addColour (0.06, juce::Colour (0xff262626)); grad.addColour (0.5, juce::Colour (0xff1c1c1c));
            g.setGradientFill (grad);
            juce::Path body; body.addRoundedRectangle (r, 20.0f);
            g.fillPath (body);
            fillTiled (g, Texture::Powder, body, 1.0f);
            drawWear (g, r.reduced (4.0f), false, 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawRoundedRectangle (r.reduced (0.7f), 20.0f, 1.2f); });

        // ---------------- ARP / SEQ ----------------
        zone = Zone::Front;
        module ({ 78, 56, 107, 400 }, S.arp, "ARP/SEQ");
        jack (Jack::GateOut, 128, 97, "GATE OUT", arpInk); jack (Jack::KbOut, 101, 137, "KB OUT", arpInk); jack (Jack::KbVelOut, 157, 137, "KB VEL OUT", arpInk);
        ticks (127, 210, 52, 11, arpInk);
        knob ("arpRate", 127, 210, 52, [] (double v) { return fmt::arpRate (v); });
        textAt ("RATE", 127, 249, arpInk, type::control);
        arpLamp = addLamp (col::ledRed, 162, 249);
        lever ("arpMode", 131, 284, { 0, 1, 2 }, [] (int p) { static const char* n[] = { "Arpeggiator", "Sequencer", "Record" }; return juce::String (n[p]); });
        textAt ("ARP", 103, 305, arpInk, type::mark, Weight::NarrowBold, 30.0f); textAt ("SEQ", 131, 305, arpInk, type::mark, Weight::NarrowBold, 30.0f);
        op ([=] (juce::Graphics& g) { const auto r = juce::Rectangle<float> (27.0f, 14.0f).withCentre ({ 159.0f, 305.0f });
                                      g.setColour (arpInk); g.drawRoundedRectangle (r, 2.5f, 1.0f);
                                      label (g, "REC", r, arpInk, type::mark, Weight::NarrowBold, juce::Justification::centred, 0.02f); });
        textAt ("MODE", 131, 320, arpInk, type::control);
        lever ("arpDirection", 131, 346, { 0, 1, 2 }, [] (int p) { static const char* n[] = { "Order", "Forward / backward", "Random" }; return juce::String (n[p]); });
        leverLabels (131, 367, "ORDR", "FWD/BKWD", "RNDM", arpInk, 33.0f);
        textAt ("DIRECTION", 131, 381, arpInk, type::control, Weight::NarrowBold, 90.0f);
        lever ("arpOctSeq", 131, 405, { 0, 1, 2 }, [] (int p) { return juce::String (p + 1); });
        leverLabels (131, 426, "1", "2", "3", arpInk, 22.0f);
        textAt ("OCT / SEQ", 131, 440, arpInk, type::control, Weight::NarrowBold, 90.0f);

        // ---------------- MODULATION ----------------
        module ({ 193, 56, 212, 400 }, S.panel, "MODULATION");
        jack (Jack::ModRateIn, 217, 97, "RATE IN", ink); jack (Jack::ModWaveOut, 381, 97, "WAVE OUT", ink);
        jack (Jack::ModSyncIn, 272, 137, "SYNC IN", ink); jack (Jack::ModShOut, 326, 137, "S/H OUT", ink);
        ticks (299, 210, 52, 11, ink);
        knob ("lfoRate", 299, 210, 52, [] (double v) { return fmt::lfoRate (v); });
        textAt ("RATE", 299, 249, ink, type::control);
        modLamp = addLamp (col::ledRed, 299, 264);
        ticks (243, 309, 46, 11, ink); knob ("lfoPitchAmt", 243, 309, 46, [] (double v) { return fmt::percent (v); });
        pill ("PITCH AMT", 243, 344, 78, S.pillOsc, ink);
        ticks (348, 309, 46, 11, ink); knob ("lfoCutoffAmt", 348, 309, 46, [] (double v) { return fmt::percent (v); });
        pill ("CUTOFF AMT", 348, 344, 84, S.pillFilter, ink);
        ticks (252, 405, 46, 11, ink); knob ("lfoPwAmt", 252, 405, 46, [] (double v) { return fmt::percent (v); });
        pill ("PULSE WIDTH AMT", 255, 440, 106, juce::Colours::transparentBlack, ink);
        knob ("lfoWave", 350, 402, 46, fmt::names ({ "Sine", "Sawtooth", "Ramp", "Square" }), true);
        selectorGlyphs (350, 402, 46, { Glyph::Sine, Glyph::Saw, Glyph::Ramp, Glyph::Square }, ink);
        textAt ("WAVEFORM", 352, 440, ink, type::control, Weight::NarrowBold, 72.0f);

        // ---------------- OSCILLATORS ----------------
        module ({ 415, 56, 212, 400 }, S.osc, "OSCILLATORS");
        op ([=] (juce::Graphics& g) { g.setColour (oscInk.withAlpha (0.8f)); g.drawLine (521.0f, 82.0f, 521.0f, 444.0f, 1.0f); });
        textAt ("1", 428, 72, oscInk, type::title, Weight::NarrowBold, 16.0f); textAt ("2", 614, 72, oscInk, type::title, Weight::NarrowBold, 16.0f);
        jack (Jack::Osc1WaveOut, 469, 97, "WAVE OUT", oscInk); jack (Jack::Osc1PitchIn, 441, 137, "PITCH IN", oscInk); jack (Jack::Osc1PwmIn, 497, 137, "PWM IN", oscInk);
        jack (Jack::Osc2WaveOut, 574, 97, "WAVE OUT", oscInk); jack (Jack::Osc2PitchIn, 546, 137, "PITCH IN", oscInk); jack (Jack::Osc2LinFmIn, 601, 137, "LIN FM IN", oscInk);
        knob ("osc1Octave", 469, 210, 50, fmt::names ({ "32'", "16'", "8'", "4'" }), true);
        selectorLabels (469, 210, 50, { "32'", "16'", "8'", "4'" }, oscInk);
        textAt ("OCTAVE", 469, 252, oscInk, type::control);
        knob ("osc2Octave", 574, 210, 50, fmt::names ({ "16'", "8'", "4'", "2'" }), true);
        selectorLabels (574, 210, 50, { "16'", "8'", "4'", "2'" }, oscInk);
        textAt ("OCTAVE", 574, 252, oscInk, type::control);
        op ([=] (juce::Graphics& g) { g.setColour (oscInk.withAlpha (0.85f)); g.drawLine (494.0f, 306.0f, 546.0f, 306.0f, 1.2f); });
        syncLink = std::make_unique<ParamLink> (apvts, "osc2Sync", [this] (float v) { if (syncPad) syncPad->setLit (v > 0.5f); });
        syncPad = addPad (S.sync, 469, 306, 44);
        learnables.push_back ({ syncPad, "osc2Sync" });
        syncPad->onPress = [this] { syncLink->setComplete (syncLink->get() > 0.5f ? 0.0f : 1.0f); };
        syncPad->onHover = [this] { hints.showHint (*syncPad, "Osc 2 Sync   " + juce::String (syncLink->get() > 0.5f ? "on" : "off")); };
        syncLink->setComplete (syncLink->get());
        syncPad->onLeave = [this] { hints.hideHint(); };
        textAt ("SYNC", 469, 344, oscInk, type::control);
        bipolarMarks (574, 306, 50, oscInk);
        knob ("osc2Freq", 574, 306, 50, [] (double v) { return fmt::semitones (v); });
        textAt ("FREQUENCY", 574, 344, oscInk, type::control, Weight::NarrowBold, 90.0f);
        knob ("osc1Wave", 469, 403, 46, fmt::names ({ "Triangle", "Sawtooth", "Square", "Narrow pulse" }), true);
        selectorGlyphs (469, 403, 46, { Glyph::Triangle, Glyph::Saw, Glyph::Square, Glyph::Pulse }, oscInk);
        textAt ("WAVEFORM", 469, 441, oscInk, type::control, Weight::NarrowBold, 80.0f);
        knob ("osc2Wave", 574, 403, 46, fmt::names ({ "Triangle", "Sawtooth", "Square", "Narrow pulse" }), true);
        selectorGlyphs (574, 403, 46, { Glyph::Triangle, Glyph::Saw, Glyph::Square, Glyph::Pulse }, oscInk);
        textAt ("WAVEFORM", 574, 441, oscInk, type::control, Weight::NarrowBold, 80.0f);

        // ---------------- MIXER ----------------
        module ({ 637, 56, 106, 400 }, S.panel, "MIXER");
        jack (Jack::MixOsc1In, 661, 97, "OSC 1 IN", ink); jack (Jack::MixOsc2In, 719, 97, "OSC 2 IN", ink);
        jack (Jack::MixNoiseIn, 661, 137, "NOISE IN", ink); jack (Jack::MixerOut, 719, 137, "OUTPUT", ink);
        ticks (690, 210, 48, 11, ink); knob ("mixOsc1", 690, 210, 48, [] (double v) { return fmt::percent (v); });
        pill ("OSCILLATOR 1", 690, 246, 92, S.pillOsc, ink);
        ticks (690, 306, 48, 11, ink); knob ("mixOsc2", 690, 306, 48, [] (double v) { return fmt::percent (v); });
        pill ("OSCILLATOR 2", 690, 342, 92, S.pillOsc, ink);
        ticks (690, 403, 48, 11, ink); knob ("mixNoise", 690, 403, 48, [] (double v) { return fmt::percent (v); });
        textAt ("NOISE", 690, 440, ink, type::control);

        // ---------------- UTILITIES ----------------
        module ({ 753, 56, 107, 400 }, S.panel, "UTILITIES");
        // MULT: the four jacks are bussed together; the printed frame runs under them, corner to corner
        op ([=] (juce::Graphics& g) {
            const juce::Rectangle<float> f (780.0f, 97.0f, 56.0f, 40.0f);
            g.setColour (ink.withAlpha (0.9f));
            g.drawRoundedRectangle (f, 3.0f, 1.6f); });
        jack (Jack::Mult1, 780, 97, "", ink); jack (Jack::Mult2, 836, 97, "", ink); jack (Jack::Mult3, 780, 137, "", ink); jack (Jack::Mult4, 836, 137, "", ink);
        textAt ("MULT", 808, 161, ink, type::control, Weight::NarrowBold, 50.0f);
        ticks (806, 210, 48, 11, ink); knob ("hpCutoff", 806, 210, 48, [] (double v) { return fmt::highPass (v); });
        textAt ("HIGH PASS", 806, 246, ink, type::control, Weight::NarrowBold, 90.0f);
        jack (Jack::HpfIn, 779, 274, "INPUT", ink); jack (Jack::HpfOut, 835, 274, "OUTPUT", ink);
        bipolarMarks (806, 357, 48, ink);
        knob ("attenuator", 806, 357, 48, [] (double v) { return fmt::bipolar (v); });
        textAt ("ATTENUATOR", 806, 393, ink, type::control, Weight::NarrowBold, 96.0f);
        jack (Jack::AttenuatorIn, 779, 422, "INPUT", ink); jack (Jack::AttenuatorOut, 835, 422, "OUTPUT", ink);

        // ---------------- FILTER ----------------
        module ({ 870, 56, 212, 400 }, S.filter, "FILTER");
        jack (Jack::FilterIn, 896, 97, "INPUT", filInk); jack (Jack::FilterOut, 1055, 97, "OUTPUT", filInk);
        jack (Jack::FilterEnvAmtIn, 940, 128, "ENV AMT IN", filInk); jack (Jack::FilterCutoffIn, 1012, 128, "CUTOFF IN", filInk);
        knob ("cutoff", 976, 228, 80, [] (double v) { return fmt::cutoff (v); });
        cutoffScale (976.0f, 228.0f, 80.0f, filInk);
        textAt ("CUTOFF", 976, 280, filInk, type::control, Weight::NarrowBold, 70.0f);
        lever ("kbdTrack", 976, 306, { 1, 0, 2 }, [] (int p) { static const char* n[] = { "1:2", "off", "1:1" }; return juce::String (n[p]); });
        leverLabels (976, 328, "1:2", "OFF", "1:1", filInk, 29.0f);
        textAt ("KBD TRACK", 976, 342, filInk, type::control, Weight::NarrowBold, 90.0f);
        bipolarMarks (922, 404, 50, filInk);
        knob ("envAmt", 922, 404, 50, [] (double v) { return fmt::bipolar (v); });
        textAt ("ENVELOPE AMT", 924, 441, filInk, type::control, Weight::NarrowBold, 100.0f);
        ticks (1027, 404, 50, 11, filInk); knob ("resonance", 1027, 404, 50, [] (double v) { return fmt::percent (v); });
        textAt ("RESONANCE", 1027, 441, filInk, type::control, Weight::NarrowBold, 90.0f);

        // ---------------- ENVELOPE ----------------
        module ({ 1090, 56, 212, 400 }, S.panel, "ENVELOPE");
        jack (Jack::EnvTriggerIn, 1196, 97, "TRIGGER IN", ink);
        jack (Jack::EnvPosOut, 1145, 137, "+ ENV OUT", ink); jack (Jack::EnvNegOut, 1248, 137, "- ENV OUT", ink);
        ticks (1144, 210, 50, 11, ink); knob ("attack", 1144, 210, 50, [] (double v) { return fmt::envTime (v, 0.001); });
        textAt ("ATTACK", 1144, 247, ink, type::control, Weight::NarrowBold, 70.0f);
        ticks (1144, 307, 50, 11, ink); knob ("decay", 1144, 307, 50, [] (double v) { return fmt::envTime (v, 0.002); });
        textAt ("DECAY", 1144, 344, ink, type::control, Weight::NarrowBold, 70.0f);
        ticks (1144, 404, 50, 11, ink); knob ("release", 1144, 404, 50, [] (double v) { return fmt::envTime (v, 0.002); });
        textAt ("RELEASE", 1144, 441, ink, type::control, Weight::NarrowBold, 70.0f);
        sustain = std::make_unique<Fader> (apvts, "sustain", 222.0f);
        sustain->hints = &hints;
        sustain->setCentrePosition (1246, 313);
        addAndMakeVisible (*sustain);
        placed (*sustain);
        learnables.push_back ({ sustain.get(), "sustain" });
        textAt ("SUSTAIN", 1246, 441, ink, type::control, Weight::NarrowBold, 70.0f);

        // ---------------- OUTPUT + SPRING REVERB ----------------
        module ({ 1312, 56, 106, 285 }, S.panel, "OUTPUT");
        jack (Jack::VcaAmtIn, 1365, 98, "VCA AMT IN", ink); jack (Jack::VcaIn, 1337, 138, "VCA IN", ink); jack (Jack::ReverbIn, 1393, 138, "REVERB IN", ink);
        ticks (1365, 214, 50, 11, ink); knob ("volume", 1365, 214, 50, [] (double v) { return fmt::percent (v * v); });
        textAt ("VOLUME", 1365, 252, ink, type::control, Weight::NarrowBold, 70.0f);
        lever ("vcaMode", 1365, 290, { 0, 1, 2 }, [] (int p) { static const char* n[] = { "Envelope", "Keyboard release", "Drone" }; return juce::String (n[p]); });
        leverLabels (1365, 311, "ENV", "KB RLS", "DRONE", ink, 31.0f);
        textAt ("VCA MODE", 1365, 325, ink, type::control, Weight::NarrowBold, 80.0f);
        module ({ 1312, 348, 106, 108 }, S.reverb, "SPRING REVERB");
        ticks (1365, 410, 46, 11, revInk); knob ("reverbMix", 1365, 410, 46, [] (double v) { return fmt::percent (v); });
        textAt ("MIX", 1365, 445, revInk, type::control, Weight::NarrowBold, 50.0f);

        // ---------------- title strip ----------------
        op ([strip = S.strip] (juce::Graphics& g) { drawTitleStrip (g, juce::Rectangle<float> (78.0f, 466.0f, 1340.0f, 62.0f), strip); });

        // ---------------- left-hand controller and keyboard (laid out by layoutKeys, drawn by paintKeysSection) ----------
        placeJackLabels();                  // still in the front zone
        zone = Zone::Keys;
        glideKnob = knob ("glide", 101, 606, 40, [] (double v) { return fmt::glide (v); });
        playPad = addPad (S.play, 162, 610, 34);
        holdPad = addPad (S.hold, 207, 610, 34);
        tapPad  = addPad (S.tap, 252, 610, 34);
        setupPads();
        pitchWheel = std::make_unique<Wheel> (true, 196.0f, S.wheel);
        pitchWheel->title = "Pitch";
        pitchWheel->hints = &hints;
        pitchWheel->onChange = [this] (float v) { processor.setUiPitchBend ((v - 0.5f) * 2.0f); };
        addAndMakeVisible (*pitchWheel);
        modWheel = std::make_unique<Wheel> (false, 196.0f, S.wheel);
        modWheel->title = "Mod wheel";
        modWheel->hints = &hints;
        modLink = std::make_unique<ParamLink> (apvts, "modWheel", [this] (float v) { if (modWheel) modWheel->setValueFromParameter (v); });
        modWheel->onChange = [this] (float v) { modLink->setComplete (v); };
        addAndMakeVisible (*modWheel);
        learnables.push_back ({ modWheel.get(), "modWheel" });
        addAndMakeVisible (keyboard);

        // ---------------- bottom strip: fine tune, output meters, presets ----------------
        zone = Zone::Fixed;
        module ({ 78, 944, 120, 100 }, S.panel, "");
        ticks (138, 984, 36, 11, ink); knob ("fineTune", 138, 984, 36, [] (double v) { return fmt::semitones (v * 100.0).replace (" st", " ct"); });
        textAt ("FINE TUNE", 138, 1022, ink, type::control, Weight::NarrowBold, 90.0f);
        module ({ 214, 944, 496, 100 }, S.panel, "");
        meters = std::make_unique<OutputMeters> (processor);
        meters->setBounds (222, 952, 480, 84);
        meters->hints = &hints;
        addAndMakeVisible (*meters);
        module ({ 726, 944, 692, 100 }, S.panel, "");
        textAt ("PRESET", 1078, 958, ink, type::control, Weight::NarrowBold, 120.0f, 0.2f);
    }

    // The left-hand controller and the keyboard bed for the current layout (they change height with the rear panel).
    void paintKeysSection (juce::Graphics& g) const
    {
        const auto& S = skin();
        const auto lhInk = col::ink;
        {
            const juce::Rectangle<float> r (78.0f, kl.top + 4.0f, 216.0f, kl.height - 4.0f);
            drawPlate (g, r, S.panel, true);                                // a black plate like the modules above
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        }
        drawKnobTicks (g, { kGlideX, kl.glideY }, 40.0f, 11, lhInk, 0.1f);
        drawText (g, "GLIDE", kGlideX, kl.glideLabelY, lhInk, type::control, Weight::NarrowBold, 60.0f);
        drawText (g, "[< KB]", kPadX[0], kl.labelsY, lhInk, type::mark, Weight::NarrowBold, 44.0f, 0.02f);
        drawText (g, "[SHIFT]", kPadX[1], kl.labelsY, lhInk, type::mark, Weight::NarrowBold, 44.0f, 0.02f);
        drawText (g, "[KB >]", kPadX[2], kl.labelsY, lhInk, type::mark, Weight::NarrowBold, 44.0f, 0.02f);
        drawPill (g, "PLAY", kPadX[0], kl.pillY, 39, S.pillArp, lhInk); drawPill (g, "HOLD", kPadX[1], kl.pillY, 39, S.pillArp, lhInk);
        drawPill (g, "TAP", kPadX[2], kl.pillY, 39, S.pillArp, lhInk);
        drawText (g, "(TIE)", kPadX[0], kl.subY, lhInk, type::mark, Weight::NarrowSemi, 44.0f, 0.02f);
        drawText (g, "(REST)", kPadX[1], kl.subY, lhInk, type::mark, Weight::NarrowSemi, 44.0f, 0.02f);
        drawText (g, "(ACCENT)", kPadX[2], kl.subY, lhInk, type::mark, Weight::NarrowSemi, 46.0f, 0.02f);
        drawPill (g, "PITCH", kWheelX[0], kl.wheelPillY, 52, S.pillWheel, lhInk); drawPill (g, "MOD", kWheelX[1], kl.wheelPillY, 52, S.pillWheel, lhInk);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (juce::Rectangle<float> (298.0f, kl.top, 1120.0f, kl.height), 3.0f);
    }

    // ---- rear panel: positions of everything for the current opening ---------------------------------------------------
    // rearPx = 0 (closed) … RearPanel::kHeight (open). The front panel moves down by rearPx; the keyboard section keeps
    // its bottom edge and gets shorter, its left-hand controller packs tighter (closed and open layouts, blended).
    // How far the front panel is pushed down: the rear panel's opening plus a gap that opens with it.
    int frontShift() const
    {
        return (int) std::lround (rearPx * (float) (RearPanel::kHeight + RearPanel::kGap) / (float) RearPanel::kHeight);
    }

    void layoutForRear()
    {
        const int r = (int) std::lround (rearPx), shift = frontShift();
        const float t = juce::jlimit (0.0f, 1.0f, (float) r / (float) RearPanel::kHeight);
        rear.setBounds (RearPanel::kX, RearPanel::kY, RearPanel::kWidth, r);
        rear.setVisible (r > 0);
        frontArt.setBounds (0, kFrontTop + shift, kW, kFrontBottom - kFrontTop);
        for (auto& [c, b] : frontComps) c->setBounds (b.translated (0, shift));

        auto mix = [t] (float closed, float open) { return closed + (open - closed) * t; };
        kl.top = 552.0f + (float) shift;
        kl.height = 376.0f - (float) shift;
        kl.labelsY = kl.top + mix (21, 14);    kl.padY = kl.top + mix (58, 44);
        kl.glideY = kl.top + mix (54, 42);     kl.glideLabelY = kl.top + mix (88, 74);
        kl.pillY = kl.top + mix (93, 76);      kl.subY = kl.top + mix (111, 91);
        kl.wheelTop = kl.top + mix (128, 104); kl.wheelBottom = kl.top + kl.height - mix (52, 30);
        kl.wheelPillY = kl.top + kl.height - mix (27, 14);
        glideKnob->setCentrePosition ((int) kGlideX, (int) std::lround (kl.glideY));
        playPad->setCentrePosition ((int) kPadX[0], (int) std::lround (kl.padY));
        holdPad->setCentrePosition ((int) kPadX[1], (int) std::lround (kl.padY));
        tapPad->setCentrePosition ((int) kPadX[2], (int) std::lround (kl.padY));
        const int wheelH = (int) std::lround (kl.wheelBottom - kl.wheelTop);
        pitchWheel->setBounds ((int) kWheelX[0] - 31, (int) std::lround (kl.wheelTop), 62, wheelH);
        modWheel->setBounds ((int) kWheelX[1] - 31, (int) std::lround (kl.wheelTop), 62, wheelH);
        keyboard.setBounds (302, (int) std::lround (kl.top) + 4, 1112, (int) std::lround (kl.height) - 10);
        repaint();
        if (onJacksMoved) onJacksMoved (jackSites());
    }

    bool rearCablesPlugged() const
    {
        const auto patch = processor.getPatchCopy();
        for (int i = 0; i < patch.count; ++i)
            for (auto& [id, pos] : rear.jackPositions())
                if (patch.cables[(size_t) i].a == id || patch.cables[(size_t) i].b == id) return true;
        return false;
    }

    void toggleRear()
    {
        if (isRearOpen() && rearCablesPlugged()) { hints.showHint (grip, "Unplug the cables from the rear jacks first"); return; }
        setRearOpen (! isRearOpen());
        if (onRearToggled) onRearToggled (isRearOpen());
    }

    void setupGrip()
    {
        rear.midiSocket.onClick = [this] { if (onOpenMidi) onOpenMidi(); };
        rear.setHints (&hints);
        for (auto* k : rear.pedalKnobs()) learnables.push_back ({ k, k->paramId });
        rear.onOpenPedal = [this] (int pedal) { if (onOpenPedal) onOpenPedal (pedal); };
        rear.onJacksMoved = [this] { if (onJacksMoved) onJacksMoved (jackSites()); };
        rear.midiSocket.onHover = [this] { hints.showHint (rear.midiSocket, "MIDI controller: knob mapping"); };
        rear.midiSocket.onLeave = [this] { hints.hideHint(); };
        grip.setTopLeftPosition (kW / 2 - grip.getWidth() / 2, 30);
        addAndMakeVisible (grip);
        grip.onClick = [this] { toggleRear(); };
        grip.onHover = [this] { hints.showHint (grip, isRearOpen() ? "Push the rear panel back" : "Pull out the rear panel"); };
        grip.onLeave = [this] { hints.hideHint(); };
        grip.onDrag = [this] (float dy) {
            if (gripDragStart < 0.0f) gripDragStart = rearPx;
            anim.stopTimer();
            const bool locked = rearCablesPlugged() && isRearOpen();
            rearPx = juce::jlimit (locked ? (float) RearPanel::kHeight : 0.0f, (float) RearPanel::kHeight, gripDragStart + dy);
            layoutForRear(); };
        grip.onDragEnd = [this] {
            gripDragStart = -1.0f;
            const bool wasOpen = isRearOpen();
            setRearOpen (rearPx > (float) RearPanel::kHeight * 0.5f);
            if (wasOpen != isRearOpen() && onRearToggled) onRearToggled (isRearOpen()); };
        gripDragStart = -1.0f;
        anim.tick = [this] {
            const double elapsed = (juce::Time::getMillisecondCounterHiRes() - animStart) / 280.0;      // 0.28 s
            const float e = (float) juce::jlimit (0.0, 1.0, elapsed);
            const float eased = 1.0f - std::pow (1.0f - e, 3.0f);                                     // ease-out
            rearPx = animFrom + (rearTarget - animFrom) * eased;
            if (e >= 1.0f) { rearPx = rearTarget; anim.stopTimer(); }
            layoutForRear(); };
    }

    // Logotype: the name in a wide heavy face (Unbounded Black), then SEMI-MODULAR / SYNTHESIZER on two lines; the maker's mark on the right. The three share one band: the
    // capitals of the name, the two lines of print and the mark all run from its top to its bottom, the band is centred
    // in the strip and the margins left of the name and right of the mark are equal. Sizes come from the outlines of
    // the letters, not from font metrics, so the edges line up exactly.
    static void drawTitleStrip (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fill)
    {
        const auto print = inkOn (fill);
        const bool dark = fill.getPerceivedBrightness() < 0.42f;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 3.0f);
        juce::ColourGradient grad (fill.brighter (dark ? 0.15f : 0.08f), r.getX(), r.getY(), fill.darker (0.14f), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        juce::Path face; face.addRoundedRectangle (r, 3.0f);
        g.fillPath (face);
        enamel (g, face, r);
        drawWear (g, r, ! dark, 1.2f);
        if (dark) { g.setColour (col::outline.withAlpha (0.78f)); g.drawRoundedRectangle (r.reduced (4.5f), 2.0f, 1.0f); }

        const auto band = r.reduced (0.0f, 13.0f);          // 36 px tall, centred
        const float margin = 26.0f;

        // Outline of a line of text at the origin (baseline y = 0).
        auto textPath = [] (const juce::String& text, Weight w, float size, float kerning) {
            juce::GlyphArrangement ga;
            ga.addLineOfText (font (w, size, kerning), text, 0.0f, 0.0f);
            juce::Path p; ga.createPath (p);
            return p;
        };
        // Bounds of the drawn outline (Path::getBounds includes the curves' control points, which stick out).
        auto tight = [] (const juce::Path& p) {
            float x0 = 1.0e9f, y0 = 1.0e9f, x1 = -1.0e9f, y1 = -1.0e9f;
            for (juce::PathFlatteningIterator it (p, {}, 0.05f); it.next();)
            {
                x0 = juce::jmin (x0, it.x1, it.x2); x1 = juce::jmax (x1, it.x1, it.x2);
                y0 = juce::jmin (y0, it.y1, it.y2); y1 = juce::jmax (y1, it.y1, it.y2);
            }
            return juce::Rectangle<float>::leftTopRightBottom (x0, y0, x1, y1);
        };
        // Scales and moves a path so its outline spans [top, top + height] and starts at x.
        auto place = [] (juce::Path& p, const juce::Rectangle<float>& bounds, float x, float top, float height, float stretch) {
            const float k = height / bounds.getHeight();
            p.applyTransform (juce::AffineTransform::translation (-bounds.getX(), -bounds.getY()).scaled (k * stretch, k).translated (x, top));
            return bounds.getWidth() * k * stretch;
        };

        // BABUSHKA, solid, in its own face.
        static const auto wide = juce::Typeface::createSystemTypefaceFor (BinaryData::UnboundedBlack_ttf, (size_t) BinaryData::UnboundedBlack_ttfSize);
        juce::Path name;
        {
            juce::GlyphArrangement ga;
            ga.addLineOfText (juce::Font (juce::FontOptions (wide).withHeight (47.0f).withKerningFactor (0.01f)), "BABUSHKA", 0.0f, 0.0f);
            ga.createPath (name);
        }
        const auto nameBounds = tight (name);
        const float nameW = place (name, nameBounds, band.getX() + margin, band.getY(), band.getHeight(), 1.0f);
        g.setColour (print);
        g.fillPath (name);

        // Two lines of print: the first starts at the top of the band, the second ends at its bottom.
        const float capH = band.getHeight() / 2.6f, textX = band.getX() + margin + nameW + 24.0f;
        auto line1 = textPath ("SEMI-MODULAR", Weight::Bold, 17.0f, 0.04f), line2 = textPath ("SYNTHESIZER", Weight::Bold, 17.0f, 0.04f);
        // capitals of the font (a flat-topped letter) set the scale for both lines; round letters may overshoot a little
        const auto capRef = tight (textPath ("H", Weight::Bold, 17.0f, 0.04f));
        const float k = capH / capRef.getHeight();
        line1.applyTransform (juce::AffineTransform::scale (k).translated (textX, band.getY() - capRef.getY() * k));
        line2.applyTransform (juce::AffineTransform::scale (k).translated (textX, band.getBottom() - capH - capRef.getY() * k));
        g.fillPath (line1);
        g.fillPath (line2);

        // NUWMUN mark (vector, from the logo artwork), as tall as the band, the same margin from the right edge.
        const auto& logo = nuwmunLogo();
        static const auto lb = tight (logo);
        const float w = band.getHeight() * lb.getWidth() / lb.getHeight();
        const juce::Rectangle<float> dest (band.getRight() - margin - w, band.getY(), w, band.getHeight());
        g.fillPath (logo, juce::RectanglePlacement (juce::RectanglePlacement::stretchToFit).getTransformToFit (lb, dest));
    }

    static const juce::Path& nuwmunLogo()
    {
        static const juce::Path path = [] {
            const juce::String svg (BinaryData::nuwmun_logo_svg, (size_t) BinaryData::nuwmun_logo_svgSize);
            auto p = juce::Drawable::parseSVGPath (svg.fromFirstOccurrenceOf (" d=\"", false, false).upToFirstOccurrenceOf ("\"", false, false));
            p.setUsingNonZeroWinding (false);
            return p;
        }();
        return path;
    }

    Lamp* addLamp (juce::Colour c, float x, float y)
    {
        auto l = std::make_unique<Lamp> (c, 7.0f);
        l->setCentrePosition ((int) x, (int) y);
        addAndMakeVisible (*l);
        placed (*l);
        lamps.push_back (std::move (l));
        return lamps.back().get();
    }
    Pad* addPad (juce::Colour c, float x, float y, float size)
    {
        auto p = std::make_unique<Pad> (c, size);
        p->setCentrePosition ((int) x, (int) y);
        addAndMakeVisible (*p);
        placed (*p);
        pads.push_back (std::move (p));
        return pads.back().get();
    }

    bool recordMode() const { return (int) std::lround (apvts.getRawParameterValue ("arpMode")->load()) == 2; }
    void toggleParam (const char* id)
    {
        if (auto* prm = apvts.getParameter (id))
        {   // a complete gesture, so the press is a step of the history
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
            prm->endChangeGesture();
        }
    }

    // PLAY / HOLD / TAP double as [< KB] / [SHIFT] / [KB >]: hold HOLD and press PLAY or TAP to move the keyboard by an octave.
    void setupPads()
    {
        playPad->onPress = [this] {
            if (shiftHeld) { keyboard.shiftOctave (-1); shiftUsed = true; hints.showHint (*playPad, "Keyboard down an octave"); return; }
            if (recordMode()) { processor.requestRecTie(); flash[0] = 8; return; }
            toggleParam ("arpPlay"); };
        holdPad->onPress = [this] { shiftHeld = true; shiftUsed = false; };
        holdPad->onRelease = [this] {
            shiftHeld = false;
            if (shiftUsed) return;
            if (recordMode()) { processor.requestRecRest(); flash[1] = 8; return; }
            toggleParam ("arpHold"); };
        tapPad->onPress = [this] {
            if (shiftHeld) { keyboard.shiftOctave (+1); shiftUsed = true; hints.showHint (*tapPad, "Keyboard up an octave"); return; }
            if (recordMode()) { processor.requestRecAccent(); flash[2] = 8; return; }
            processor.tapTempo(); flash[2] = 6; };
        playPad->onHover = [this] { hints.showHint (*playPad, recordMode() ? "Tie (REC)" : "Play arpeggiator / sequencer"); };
        holdPad->onHover = [this] { hints.showHint (*holdPad, recordMode() ? "Rest (REC)    hold = SHIFT" : "Hold    (SHIFT with PLAY / TAP)"); };
        tapPad->onHover = [this] { hints.showHint (*tapPad, recordMode() ? "Accent (REC)" : "Tap tempo"); };
        for (auto* pad : { playPad, holdPad, tapPad }) pad->onLeave = [this] { hints.hideHint(); };
    }

    void timerCallback() override
    {
        const auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
        playPad->setLit (raw ("arpPlay") > 0.5f || flash[0] > 0);
        holdPad->setLit (raw ("arpHold") > 0.5f || flash[1] > 0 || shiftHeld);
        tapPad->setLit (flash[2] > 0);
        for (auto& f : flash) if (f > 0) --f;
        // lamps blink at the LFO rate / tempo (clamped so they stay visible)
        const float lfoHz = 0.07f * std::pow (1300.0f / 0.07f, raw ("lfoRate"));
        modPhase += juce::jmin (lfoHz, 7.5f) / 30.0f; modPhase -= std::floor (modPhase);
        modLamp->setLevel (modPhase < 0.5f ? 1.0f : 0.12f);
        const float bpm = babushka::dsp::ArpSeq::freeBpm (raw ("arpRate"));
        arpPhase += juce::jmin (bpm / 60.0f, 7.5f) / 30.0f; arpPhase -= std::floor (arpPhase);
        arpLamp->setLevel (arpPhase < 0.22f ? 1.0f : 0.12f);
        presetDisplay.setName (processor.getCurrentPresetName());
        presetDisplay.setBadge (processor.isRandomSound() ? "NOT SAVED" : processor.isSoundModified() ? "EDITED" : "");
        fxLock.setLocked ((processor.getFxLocks() & BabushkaProcessor::kLockAllFx) != 0);
        undoButton.setEnabled (processor.canUndo());
        redoButton.setEnabled (processor.canRedo());
        // the wheels show where pitch bend and modulation are, also when they come from a MIDI keyboard
        if (! pitchWheel->isBusy()) pitchWheel->setValueFromParameter (0.5f + 0.5f * processor.getPitchBendPosition());
        if (! modWheel->isBusy()) modWheel->setValueFromParameter (juce::jmax (modLink->get(), processor.getModWheelCC()));
        // a preset with cables in the rear jacks pulls the rear panel out
        if (const int v = processor.getPatchVersion(); v != knownPatchVersion)
        {
            knownPatchVersion = v;
            if (! isRearOpen() && rearCablesPlugged()) { setRearOpen (true); if (onRearToggled) onRearToggled (true); }
        }
    }

    // Right click on RANDOMIZE: which kind of sound it makes (remembered while the plugin is open).
    void showRandomMenu()
    {
        juce::PopupMenu m;
        m.addSectionHeader ("RANDOMIZE TYPE");
        const auto names = BabushkaProcessor::randomKindNames();
        for (int i = 0; i < names.size(); ++i)
            m.addItem (i + 1, i == 0 ? "ANY - any kind of sound" : names[i], true, (int) processor.getRandomKind() == i);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&randomButton), [this] (int id) {
            if (id <= 0) return;
            processor.setRandomKind ((BabushkaProcessor::RandomKind) (id - 1));
            hints.showHint (randomButton, "RANDOMIZE makes: " + BabushkaProcessor::randomKindNames()[id - 1]); });
    }

    // Right click on UNDO / REDO: the history, newest first; a click goes back (or forward) to that step.
    void showHistoryMenu()
    {
        juce::PopupMenu m;
        m.addSectionHeader ("HISTORY");
        const auto labels = processor.getHistoryLabels();
        for (int i = labels.size() - 1; i >= 0; --i) m.addItem (i + 1, labels[i], true, i == processor.getHistoryPosition());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&undoButton), [this] (int id) {
            if (id > 0) processor.jumpToHistory (id - 1); });
    }
};
} // namespace babushka::ui
