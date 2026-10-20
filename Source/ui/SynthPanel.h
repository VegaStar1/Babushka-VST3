// Babushka — the instrument: module plates, knobs, switches, jacks, left-hand controller, keyboard, rear strip.
//
// The layout follows the Grandmother front panel (modules left to right: ARP/SEQ, MODULATION, OSCILLATORS, MIXER,
// UTILITIES, FILTER, ENVELOPE, OUTPUT + SPRING REVERB; title strip; left-hand controller and keyboard below) in a
// fixed logical coordinate system. Static artwork is a list of drawing operations recorded while the layout is
// built, so every coordinate is written down once.
#pragma once

#include "KeyboardWidget.h"
#include "Widgets.h"
#include "CableLayer.h"
#include "../dsp/ArpSeq.h"
#include <functional>

namespace babushka::ui
{
// ---- small plate button (PRESETS, SETTINGS, arrows) ------------------------------------------------------------
class PlateButton : public juce::Component
{
public:
    PlateButton (const juce::String& t, float textHeight = 11.0f) : text (t), height (textHeight) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick;
    void setText (const juce::String& t) { text = t; repaint(); }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 4.0f);
        juce::ColourGradient grad (juce::Colour (down ? 0xff1c1b1a : 0xff3a3836), r.getX(), r.getY(), juce::Colour (down ? 0xff2d2b29 : 0xff161514), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (col::outline.withAlpha (0.45f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        label (g, text, r.translated (0.0f, down ? 1.0f : 0.0f), col::ink, height, Weight::SemiBold, juce::Justification::centred, 0.08f);
    }
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { down = false; repaint(); if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
private:
    juce::String text; float height; bool down = false;
};

namespace fmt
{
    inline juce::String hz (double v) { return v >= 1000.0 ? juce::String (v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz" : juce::String (v, v >= 100.0 ? 0 : 1) + " Hz"; }
    inline juce::String seconds (double v) { return v < 1.0 ? juce::String (v * 1000.0, v < 0.01 ? 1 : 0) + " ms" : juce::String (v, v < 10.0 ? 2 : 1) + " s"; }
    inline juce::String percent (double v) { return juce::String (v * 100.0, 0) + " %"; }
    inline juce::String bipolar (double v) { return (v >= 0 ? "+" : "") + juce::String (v * 100.0, 0) + " %"; }
    inline juce::String cutoff (double k) { return hz (10.0 * std::pow (1100.0, juce::jlimit (0.0, 1.0, k))); }
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
        : processor (p), apvts (p.apvts), hints (hintSink), keyboard (p)
    {
        buildLayout();
        presetPrev.onClick = std::move (prev);
        presetNext.onClick = std::move (next);
        presetsButton.onClick = std::move (openPresets);
        settingsButton.onClick = std::move (openSettings);
        for (auto* b : { &presetPrev, &presetNext, &presetsButton, &settingsButton }) addAndMakeVisible (*b);
        presetPrev.setBounds (780, 957, 36, 30);   presetNext.setBounds (1100, 957, 36, 30);
        presetsButton.setBounds (1170, 957, 110, 30);  settingsButton.setBounds (1170, 996, 110, 30);
        setSize (kW, kH);
        startTimerHz (30);
    }

    std::vector<JackSite> jackSites() const { return jacks; }
    juce::String presetName;                       // shown on the display; set by the editor

    void paint (juce::Graphics& g) override
    {
        for (auto& op : drawOps) op (g);
        drawPresetDisplay (g);
    }

private:
    struct Placed { juce::Component* c; };

    BabushkaProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    HintSink& hints;
    std::vector<std::function<void (juce::Graphics&)>> drawOps;
    std::vector<JackSite> jacks;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<LeverSwitch>> levers;
    std::vector<std::unique_ptr<Pad>> pads;
    std::vector<std::unique_ptr<Lamp>> lamps;
    std::unique_ptr<Fader> sustain;
    std::unique_ptr<Wheel> pitchWheel, modWheel;
    std::unique_ptr<ParamLink> modLink;
    KeyboardWidget keyboard;
    PlateButton presetPrev { "<", 15.0f }, presetNext { ">", 15.0f }, presetsButton { "PRESETS" }, settingsButton { "SETTINGS" };
    Pad* syncPad = nullptr; Pad* playPad = nullptr; Pad* holdPad = nullptr; Pad* tapPad = nullptr;
    std::unique_ptr<ParamLink> syncLink;
    Lamp* modLamp = nullptr; Lamp* arpLamp = nullptr;
    bool shiftHeld = false, shiftUsed = false;
    int flash[3] { 0, 0, 0 };
    float modPhase = 0.0f, arpPhase = 0.0f;

    // ---- recording of static artwork -------------------------------------------------------------------------------------
    void op (std::function<void (juce::Graphics&)> f) { drawOps.push_back (std::move (f)); }

    void textAt (const juce::String& t, float x, float y, juce::Colour c, float size = 8.5f, Weight w = Weight::SemiBold, float width = 120.0f, float kern = 0.06f)
    {
        op ([=] (juce::Graphics& g) { label (g, t, juce::Rectangle<float> (width, size * 1.6f).withCentre ({ x, y }), c, size, w, juce::Justification::centred, kern); });
    }
    void pill (const juce::String& t, float x, float y, float w, juce::Colour fill, juce::Colour text, bool outlineOnly = false)
    {
        op ([=] (juce::Graphics& g) {
            const auto r = juce::Rectangle<float> (w, 15.0f).withCentre ({ x, y });
            if (outlineOnly) { g.setColour (col::ink.withAlpha (0.8f)); g.drawRoundedRectangle (r, 3.0f, 1.0f); }
            else { g.setColour (fill); g.fillRoundedRectangle (r, 3.0f); }
            label (g, t, r, text, 7.8f, Weight::Bold, juce::Justification::centred, 0.07f); });
    }
    void module (juce::Rectangle<float> r, juce::Colour fill, const juce::String& title, juce::Colour titleColour, bool outlined = true)
    {
        op ([=] (juce::Graphics& g) { drawPlate (g, r, fill, outlined); });
        if (title.isNotEmpty()) textAt (title, r.getCentreX(), r.getY() + 14.0f, titleColour, 10.5f, Weight::Bold, r.getWidth(), 0.08f);
    }
    void jack (Jack id, float x, float y, const juce::String& text, juce::Colour textColour)
    {
        jacks.push_back ({ id, { x, y } });
        op ([=] (juce::Graphics& g) { drawJack (g, { x, y }); });
        if (text.isNotEmpty()) textAt (text, x, y + 19.0f, textColour, 7.4f, Weight::SemiBold, 76.0f, 0.04f);
    }
    Knob* knob (const char* id, float cx, float cy, float diameter, std::function<juce::String (double)> formatter = nullptr, bool selector = false)
    {
        auto k = std::make_unique<Knob> (apvts, id, diameter, std::move (formatter), selector);
        k->hints = &hints;
        k->setCentrePosition ((int) std::lround (cx), (int) std::lround (cy));
        addAndMakeVisible (*k);
        knobs.push_back (std::move (k));
        return knobs.back().get();
    }
    LeverSwitch* lever (const char* id, float cx, float cy, std::vector<int> map, std::function<juce::String (int)> names = nullptr)
    {
        auto l = std::make_unique<LeverSwitch> (apvts, id, std::move (map), std::move (names));
        l->hints = &hints;
        l->setCentrePosition ((int) std::lround (cx), (int) std::lround (cy));
        addAndMakeVisible (*l);
        levers.push_back (std::move (l));
        return levers.back().get();
    }
    void ticks (float cx, float cy, float d, int count, juce::Colour c, float length = 0.1f, int majorEvery = 0)
    {
        op ([=] (juce::Graphics& g) { drawKnobTicks (g, { cx, cy }, d, count, c, length, -135.0f, 270.0f, majorEvery); });
    }
    void bipolarMarks (float cx, float cy, float d, juce::Colour c)
    {
        ticks (cx, cy, d, 11, c, 0.1f, 5);
        textAt ("0", cx, cy - d * 0.78f, c, 8.0f, Weight::Bold, 20.0f);
        textAt ("-", cx - d * 0.74f, cy - d * 0.33f, c, 10.0f, Weight::Bold, 14.0f);
        textAt ("+", cx + d * 0.74f, cy - d * 0.33f, c, 10.0f, Weight::Bold, 14.0f);
    }
    void selectorLabels (float cx, float cy, float d, const std::vector<juce::String>& labels, juce::Colour c, float size = 8.0f)
    {
        const int n = (int) labels.size();
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::degreesToRadians (-135.0f + 270.0f * (float) i / (float) (n - 1));
            const float r = d * 0.5f + 12.0f;
            textAt (labels[(size_t) i], cx + std::sin (a) * r, cy - std::cos (a) * r, c, size, Weight::Bold, 26.0f, 0.02f);
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
            op ([=] (juce::Graphics& g) { drawGlyph (g, gl, p, c, 5.0f); });
        }
        ticks (cx, cy, d, n, c, 0.06f);
    }

    // Logarithmic frequency scale round the CUTOFF knob: 10 Hz … 11 kHz over 270°, a tick at m × 10^n Hz.
    void logCutoffScale (float cx, float cy, float d, juce::Colour c)
    {
        auto angleFor = [] (double hz) { return -135.0 + 270.0 * std::log (hz / 10.0) / std::log (1100.0); };
        op ([=] (juce::Graphics& g) {
            for (double decade = 10.0; decade <= 10000.0; decade *= 10.0)
                for (int m = 1; m <= 9; ++m)
                {
                    const double hz = decade * m;
                    if (hz > 11000.0) break;
                    const float a = juce::degreesToRadians ((float) angleFor (hz) - 90.0f);
                    const bool major = (m == 2) || (m == 1 && decade == 10.0);
                    const float r0 = d * 0.54f, r1 = d * (major ? 0.70f : 0.62f);
                    g.setColour (c);
                    g.drawLine (cx + r0 * std::cos (a), cy + r0 * std::sin (a), cx + r1 * std::cos (a), cy + r1 * std::sin (a), major ? 1.6f : 0.9f);
                }
            const float aEnd = juce::degreesToRadians (135.0f - 90.0f);
            g.drawLine (cx + d * 0.54f * std::cos (aEnd), cy + d * 0.54f * std::sin (aEnd), cx + d * 0.70f * std::cos (aEnd), cy + d * 0.70f * std::sin (aEnd), 1.6f); });
        auto at = [&] (double hz, const juce::String& t, float extra) {
            const float a = juce::degreesToRadians ((float) angleFor (hz));
            const float r = d * 0.5f + 25.0f + extra;
            textAt (t, cx + std::sin (a) * r, cy - std::cos (a) * r, c, 7.0f, Weight::Bold, 40.0f);
        };
        at (20.0, "20Hz", 0.0f); at (200.0, "200Hz", -2.0f); at (2000.0, "2kHz", 0.0f); at (11000.0, "11kHz", 0.0f);
    }

    // ---- the layout ----------------------------------------------------------------------------------------------------------
    void buildLayout()
    {
        using namespace babushka::dsp;
        const auto ink = col::ink, dark = col::inkDark;

        // chassis
        op ([] (juce::Graphics& g) {
            const juce::Rectangle<float> r (30.0f, 26.0f, 1440.0f, 1040.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (r.translated (0.0f, 6.0f).expanded (4.0f), 22.0f);
            juce::ColourGradient grad (juce::Colour (0xff1e1e1e), r.getX(), r.getY(), juce::Colour (0xff0a0a0a), r.getX(), r.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r, 20.0f);
            grain (g, r, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawRoundedRectangle (r.reduced (0.7f), 20.0f, 1.2f); });

        // ---------------- ARP / SEQ ----------------
        module ({ 78, 56, 107, 400 }, col::amber, "ARP/SEQ", dark);
        jack (Jack::GateOut, 128, 97, "GATE OUT", dark); jack (Jack::KbOut, 101, 137, "KB OUT", dark); jack (Jack::KbVelOut, 157, 137, "KB VEL OUT", dark);
        ticks (127, 208, 52, 11, dark);
        knob ("arpRate", 127, 208, 52, [] (double v) { return fmt::arpRate (v); });
        textAt ("RATE", 127, 246, dark, 8.5f, Weight::Bold, 60.0f);
        arpLamp = addLamp (col::ledRed, 160, 246);
        lever ("arpMode", 131, 284, { 0, 1, 2 }, [] (int p) { static const char* n[] = { "Arpeggiator", "Sequencer", "Record" }; return juce::String (n[p]); });
        textAt ("ARP", 107, 304, dark, 7.5f, Weight::Bold, 30.0f); textAt ("SEQ", 131, 304, dark, 7.5f, Weight::Bold, 30.0f);
        pill ("REC", 157, 304, 24, {}, dark, true);
        textAt ("MODE", 131, 318, dark, 8.0f, Weight::Bold, 60.0f);
        lever ("arpDirection", 131, 346, { 0, 1, 2 }, [] (int p) { static const char* n[] = { "Order", "Forward / backward", "Random" }; return juce::String (n[p]); });
        textAt ("ORDR", 100, 366, dark, 6.0f, Weight::Bold, 24.0f, 0.02f); textAt ("FWD/BKWD", 131, 366, dark, 5.6f, Weight::Bold, 38.0f, 0.0f); textAt ("RNDM", 164, 366, dark, 6.0f, Weight::Bold, 26.0f, 0.02f);
        textAt ("DIRECTION", 131, 379, dark, 8.0f, Weight::Bold, 80.0f);
        lever ("arpOctSeq", 131, 405, { 0, 1, 2 }, [] (int p) { return juce::String (p + 1); });
        textAt ("1", 110, 425, dark, 7.5f, Weight::Bold, 14.0f); textAt ("2", 131, 425, dark, 7.5f, Weight::Bold, 14.0f); textAt ("3", 152, 425, dark, 7.5f, Weight::Bold, 14.0f);
        textAt ("OCT / SEQ", 131, 439, dark, 8.0f, Weight::Bold, 80.0f);

        // ---------------- MODULATION ----------------
        module ({ 193, 56, 212, 400 }, col::panel, "MODULATION", ink);
        jack (Jack::ModRateIn, 217, 97, "RATE IN", ink); jack (Jack::ModWaveOut, 381, 97, "WAVE OUT", ink);
        jack (Jack::ModSyncIn, 272, 137, "SYNC IN", ink); jack (Jack::ModShOut, 326, 137, "S/H OUT", ink);
        ticks (299, 208, 52, 11, ink);
        knob ("lfoRate", 299, 208, 52, [] (double v) { return fmt::lfoRate (v); });
        textAt ("RATE", 299, 247, ink, 8.5f, Weight::Bold, 60.0f);
        modLamp = addLamp (col::ledRed, 299, 260);
        ticks (243, 307, 46, 11, ink); knob ("lfoPitchAmt", 243, 307, 46, [] (double v) { return fmt::percent (v); });
        pill ("PITCH AMT", 243, 342, 70, col::teal, dark);
        ticks (348, 307, 46, 11, ink); knob ("lfoCutoffAmt", 348, 307, 46, [] (double v) { return fmt::percent (v); });
        pill ("CUTOFF AMT", 348, 342, 74, col::olive, dark);
        ticks (252, 405, 46, 11, ink); knob ("lfoPwAmt", 252, 405, 46, [] (double v) { return fmt::percent (v); });
        pill ("PULSE WIDTH AMT", 252, 440, 104, {}, ink, true);
        knob ("lfoWave", 350, 402, 46, fmt::names ({ "Sine", "Sawtooth", "Ramp", "Square" }), true);
        selectorGlyphs (350, 402, 46, { Glyph::Sine, Glyph::Saw, Glyph::Ramp, Glyph::Square }, ink);
        textAt ("WAVEFORM", 350, 440, ink, 8.0f, Weight::Bold, 80.0f);

        // ---------------- OSCILLATORS ----------------
        module ({ 415, 56, 212, 400 }, col::teal, "OSCILLATORS", dark);
        op ([=] (juce::Graphics& g) { g.setColour (dark.withAlpha (0.8f)); g.drawLine (521.0f, 82.0f, 521.0f, 444.0f, 1.0f); });
        textAt ("1", 427, 90, dark, 11.0f, Weight::Bold, 16.0f); textAt ("2", 616, 90, dark, 11.0f, Weight::Bold, 16.0f);
        jack (Jack::Osc1WaveOut, 469, 97, "WAVE OUT", dark); jack (Jack::Osc1PitchIn, 441, 137, "PITCH IN", dark); jack (Jack::Osc1PwmIn, 497, 137, "PWM IN", dark);
        jack (Jack::Osc2WaveOut, 574, 97, "WAVE OUT", dark); jack (Jack::Osc2PitchIn, 546, 137, "PITCH IN", dark); jack (Jack::Osc2LinFmIn, 601, 137, "LIN FM IN", dark);
        knob ("osc1Octave", 469, 208, 50, fmt::names ({ "32'", "16'", "8'", "4'" }), true);
        selectorLabels (469, 208, 50, { "32'", "16'", "8'", "4'" }, dark);
        textAt ("OCTAVE", 469, 250, dark, 8.0f, Weight::Bold, 60.0f);
        knob ("osc2Octave", 574, 208, 50, fmt::names ({ "16'", "8'", "4'", "2'" }), true);
        selectorLabels (574, 208, 50, { "16'", "8'", "4'", "2'" }, dark);
        textAt ("OCTAVE", 574, 250, dark, 8.0f, Weight::Bold, 60.0f);
        op ([=] (juce::Graphics& g) { g.setColour (dark.withAlpha (0.85f)); g.drawLine (494.0f, 306.0f, 546.0f, 306.0f, 1.2f); });
        syncLink = std::make_unique<ParamLink> (apvts, "osc2Sync", [this] (float v) { if (syncPad) syncPad->setLit (v > 0.5f); });
        syncPad = addPad (col::brick.brighter (0.25f), 469, 306, 44);
        syncPad->onPress = [this] { syncLink->setComplete (syncLink->get() > 0.5f ? 0.0f : 1.0f); };
        syncPad->onHover = [this] { hints.showHint (*syncPad, "Osc 2 Sync   " + juce::String (syncLink->get() > 0.5f ? "on" : "off")); };
        syncLink->setComplete (syncLink->get());
        textAt ("SYNC", 469, 342, dark, 8.0f, Weight::Bold, 60.0f);
        bipolarMarks (574, 306, 50, dark);
        knob ("osc2Freq", 574, 306, 50, [] (double v) { return fmt::semitones (v); });
        textAt ("FREQUENCY", 574, 342, dark, 8.0f, Weight::Bold, 80.0f);
        knob ("osc1Wave", 469, 403, 46, fmt::names ({ "Triangle", "Sawtooth", "Square", "Narrow pulse" }), true);
        selectorGlyphs (469, 403, 46, { Glyph::Triangle, Glyph::Saw, Glyph::Square, Glyph::Pulse }, dark);
        textAt ("WAVEFORM", 469, 441, dark, 8.0f, Weight::Bold, 80.0f);
        knob ("osc2Wave", 574, 403, 46, fmt::names ({ "Triangle", "Sawtooth", "Square", "Narrow pulse" }), true);
        selectorGlyphs (574, 403, 46, { Glyph::Triangle, Glyph::Saw, Glyph::Square, Glyph::Pulse }, dark);
        textAt ("WAVEFORM", 574, 441, dark, 8.0f, Weight::Bold, 80.0f);

        // ---------------- MIXER ----------------
        module ({ 637, 56, 106, 400 }, col::panel, "MIXER", ink);
        jack (Jack::MixOsc1In, 661, 97, "OSC 1 IN", ink); jack (Jack::MixOsc2In, 719, 97, "OSC 2 IN", ink);
        jack (Jack::MixNoiseIn, 661, 137, "NOISE IN", ink); jack (Jack::MixerOut, 719, 137, "OUTPUT", ink);
        ticks (690, 208, 48, 11, ink); knob ("mixOsc1", 690, 208, 48, [] (double v) { return fmt::percent (v); });
        pill ("OSCILLATOR 1", 690, 244, 84, col::teal, dark);
        ticks (690, 306, 48, 11, ink); knob ("mixOsc2", 690, 306, 48, [] (double v) { return fmt::percent (v); });
        pill ("OSCILLATOR 2", 690, 342, 84, col::teal, dark);
        ticks (690, 403, 48, 11, ink); knob ("mixNoise", 690, 403, 48, [] (double v) { return fmt::percent (v); });
        textAt ("NOISE", 690, 440, ink, 8.5f, Weight::Bold, 60.0f);

        // ---------------- UTILITIES ----------------
        module ({ 753, 56, 107, 400 }, col::panel, "UTILITIES", ink);
        jack (Jack::Mult1, 780, 97, "", ink); jack (Jack::Mult2, 836, 97, "", ink); jack (Jack::Mult3, 780, 137, "", ink); jack (Jack::Mult4, 836, 137, "", ink);
        op ([=] (juce::Graphics& g) { g.setColour (ink.withAlpha (0.8f)); g.drawRect (juce::Rectangle<float> (780.0f, 97.0f, 56.0f, 40.0f), 1.2f); });
        textAt ("MULT", 808, 160, ink, 8.0f, Weight::Bold, 50.0f);
        ticks (806, 208, 48, 11, ink); knob ("hpCutoff", 806, 208, 48, [] (double v) { return fmt::highPass (v); });
        textAt ("HIGH PASS", 806, 244, ink, 8.5f, Weight::Bold, 90.0f);
        jack (Jack::HpfIn, 779, 272, "INPUT", ink); jack (Jack::HpfOut, 835, 272, "OUTPUT", ink);
        bipolarMarks (806, 355, 48, ink);
        knob ("attenuator", 806, 355, 48, [] (double v) { return fmt::bipolar (v); });
        textAt ("ATTENUATOR", 806, 392, ink, 8.5f, Weight::Bold, 100.0f);
        jack (Jack::AttenuatorIn, 779, 424, "INPUT", ink); jack (Jack::AttenuatorOut, 835, 424, "OUTPUT", ink);

        // ---------------- FILTER ----------------
        module ({ 870, 56, 212, 400 }, col::olive, "FILTER", dark);
        jack (Jack::FilterIn, 896, 97, "INPUT", dark); jack (Jack::FilterOut, 1055, 97, "OUTPUT", dark);
        jack (Jack::FilterEnvAmtIn, 940, 130, "ENV AMT IN", dark); jack (Jack::FilterCutoffIn, 1012, 130, "CUTOFF IN", dark);
        knob ("cutoff", 976, 226, 80, [] (double v) { return fmt::cutoff (v); });
        logCutoffScale (976.0f, 226.0f, 80.0f, dark);
        textAt ("CUTOFF", 976, 277, dark, 8.5f, Weight::Bold, 70.0f);
        lever ("kbdTrack", 976, 304, { 1, 0, 2 }, [] (int p) { static const char* n[] = { "1:2", "off", "1:1" }; return juce::String (n[p]); });
        textAt ("1:2", 947, 326, dark, 7.0f, Weight::Bold, 24.0f); textAt ("OFF", 976, 326, dark, 7.0f, Weight::Bold, 24.0f); textAt ("1:1", 1005, 326, dark, 7.0f, Weight::Bold, 24.0f);
        textAt ("KBD TRACK", 976, 339, dark, 8.0f, Weight::Bold, 80.0f);
        bipolarMarks (922, 404, 50, dark);
        knob ("envAmt", 922, 404, 50, [] (double v) { return fmt::bipolar (v); });
        textAt ("ENVELOPE AMT", 922, 440, dark, 8.0f, Weight::Bold, 100.0f);
        ticks (1027, 404, 50, 11, dark); knob ("resonance", 1027, 404, 50, [] (double v) { return fmt::percent (v); });
        textAt ("RESONANCE", 1027, 440, dark, 8.0f, Weight::Bold, 90.0f);

        // ---------------- ENVELOPE ----------------
        module ({ 1090, 56, 212, 400 }, col::panel, "ENVELOPE", ink);
        jack (Jack::EnvTriggerIn, 1196, 97, "TRIGGER IN", ink);
        jack (Jack::EnvPosOut, 1145, 137, "+ ENV OUT", ink); jack (Jack::EnvNegOut, 1248, 137, "- ENV OUT", ink);
        ticks (1144, 208, 50, 11, ink); knob ("attack", 1144, 208, 50, [] (double v) { return fmt::envTime (v, 0.001); });
        textAt ("ATTACK", 1144, 245, ink, 8.5f, Weight::Bold, 70.0f);
        ticks (1144, 306, 50, 11, ink); knob ("decay", 1144, 306, 50, [] (double v) { return fmt::envTime (v, 0.002); });
        textAt ("DECAY", 1144, 343, ink, 8.5f, Weight::Bold, 70.0f);
        ticks (1144, 403, 50, 11, ink); knob ("release", 1144, 403, 50, [] (double v) { return fmt::envTime (v, 0.002); });
        textAt ("RELEASE", 1144, 440, ink, 8.5f, Weight::Bold, 70.0f);
        sustain = std::make_unique<Fader> (apvts, "sustain", 232.0f);
        sustain->hints = &hints;
        sustain->setCentrePosition (1246, 318);
        addAndMakeVisible (*sustain);
        textAt ("SUSTAIN", 1246, 440, ink, 8.5f, Weight::Bold, 70.0f);

        // ---------------- OUTPUT + SPRING REVERB ----------------
        module ({ 1312, 56, 106, 285 }, col::panel, "OUTPUT", ink);
        jack (Jack::VcaAmtIn, 1365, 98, "VCA AMT IN", ink); jack (Jack::VcaIn, 1337, 138, "VCA IN", ink); jack (Jack::ReverbIn, 1393, 138, "REVERB IN", ink);
        ticks (1365, 213, 50, 11, ink); knob ("volume", 1365, 213, 50, [] (double v) { return fmt::percent (v * v); });
        textAt ("VOLUME", 1365, 250, ink, 8.5f, Weight::Bold, 70.0f);
        lever ("vcaMode", 1365, 292, { 0, 1, 2 }, [] (int p) { static const char* n[] = { "Envelope", "Keyboard release", "Drone" }; return juce::String (n[p]); });
        textAt ("ENV", 1339, 312, ink, 6.5f, Weight::Bold, 28.0f); textAt ("KB RLS", 1365, 312, ink, 6.0f, Weight::Bold, 34.0f); textAt ("DRONE", 1392, 312, ink, 6.5f, Weight::Bold, 32.0f);
        textAt ("VCA MODE", 1365, 326, ink, 7.5f, Weight::Bold, 80.0f);
        module ({ 1312, 348, 106, 108 }, col::brick, "SPRING REVERB", dark);
        ticks (1365, 411, 48, 11, dark); knob ("reverbMix", 1365, 411, 48, [] (double v) { return fmt::percent (v); });
        textAt ("MIX", 1365, 445, dark, 8.5f, Weight::Bold, 50.0f);

        // ---------------- title strip ----------------
        op ([] (juce::Graphics& g) {
            const juce::Rectangle<float> r (78.0f, 466.0f, 1340.0f, 62.0f);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 3.0f);
            juce::ColourGradient grad (col::cream.brighter (0.08f), r.getX(), r.getY(), col::cream.darker (0.14f), r.getX(), r.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r, 3.0f);
            grain (g, r, 0.8f);
            auto f1 = font (Weight::Bold, 46.0f, 0.0f), f2 = font (Weight::Regular, 46.0f, 0.0f);
            g.setColour (col::inkDark);
            g.setFont (f1); g.drawText ("BABUSH", juce::Rectangle<float> (96.0f, 470.0f, 300.0f, 54.0f), juce::Justification::centredLeft, false);
            const float w1 = juce::GlyphArrangement::getStringWidth (f1, "BABUSH");
            g.setFont (f2); g.drawText ("KA", juce::Rectangle<float> (96.0f + w1, 470.0f, 160.0f, 54.0f), juce::Justification::centredLeft, false);
            label (g, "SEMI-MODULAR", juce::Rectangle<float> (440.0f, 478.0f, 320.0f, 22.0f), col::inkDark, 17.0f, Weight::Bold, juce::Justification::centredLeft, 0.05f);
            label (g, "ANALOG SYNTHESIZER", juce::Rectangle<float> (440.0f, 497.0f, 340.0f, 22.0f), col::inkDark, 17.0f, Weight::Bold, juce::Justification::centredLeft, 0.05f);
            // maker's mark: NUWMUN
            g.setColour (col::inkDark);
            g.setFont (font (Weight::Bold, 34.0f, 0.18f));
            g.drawText ("NUWMUN", juce::Rectangle<float> (1120.0f, 474.0f, 280.0f, 46.0f), juce::Justification::centredRight, false);
            g.drawLine (1130.0f, 522.0f, 1396.0f, 522.0f, 2.0f);
        });

        // ---------------- left-hand controller ----------------
        op ([] (juce::Graphics& g) {
            const juce::Rectangle<float> r (66.0f, 556.0f, 232.0f, 372.0f);
            g.setColour (juce::Colour (0xff0c0c0c));
            g.fillRoundedRectangle (r, 6.0f);
            grain (g, r, 0.8f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f); });
        ticks (101, 608, 40, 11, ink);
        knob ("glide", 101, 608, 40, [] (double v) { return fmt::glide (v); });
        textAt ("GLIDE", 101, 640, ink, 8.0f, Weight::Bold, 60.0f);
        const float padY = 610;
        playPad = addPad (col::ledGreen, 162, padY, 40);
        holdPad = addPad (juce::Colour (0xff62c9d8), 207, padY, 40);
        tapPad  = addPad (col::ledAmber, 252, padY, 40);
        textAt ("[< KB]", 162, 572, ink, 7.0f, Weight::Bold, 50.0f); textAt ("[SHIFT]", 207, 572, ink, 7.0f, Weight::Bold, 50.0f); textAt ("[KB >]", 252, 572, ink, 7.0f, Weight::Bold, 50.0f);
        pill ("PLAY", 162, 646, 40, col::ledAmber.withAlpha (0.95f), dark); pill ("HOLD", 207, 646, 40, col::ledAmber.withAlpha (0.95f), dark); pill ("TAP", 252, 646, 40, col::ledAmber.withAlpha (0.95f), dark);
        textAt ("(TIE)", 162, 661, ink, 6.5f, Weight::SemiBold, 40.0f); textAt ("(REST)", 207, 661, ink, 6.5f, Weight::SemiBold, 40.0f); textAt ("(ACCENT)", 252, 661, ink, 6.5f, Weight::SemiBold, 46.0f);
        setupPads();
        pitchWheel = std::make_unique<Wheel> (true, 190.0f);
        pitchWheel->title = "Pitch";
        pitchWheel->hints = &hints;
        pitchWheel->onChange = [this] (float v) { processor.setUiPitchBend ((v - 0.5f) * 2.0f); };
        pitchWheel->setCentrePosition (134, 770);
        addAndMakeVisible (*pitchWheel);
        modWheel = std::make_unique<Wheel> (false, 190.0f);
        modWheel->title = "Mod wheel";
        modWheel->hints = &hints;
        modLink = std::make_unique<ParamLink> (apvts, "modWheel", [this] (float v) { if (modWheel) modWheel->setValueFromParameter (v); });
        modWheel->onChange = [this] (float v) { modLink->setComplete (v); };
        modWheel->setCentrePosition (226, 770);
        addAndMakeVisible (*modWheel);
        pill ("PITCH", 134, 890, 48, juce::Colour (0xff62c9d8), dark); pill ("MOD", 226, 890, 40, juce::Colour (0xff62c9d8), dark);

        // ---------------- keyboard ----------------
        op ([] (juce::Graphics& g) {
            const juce::Rectangle<float> r (298.0f, 552.0f, 1124.0f, 376.0f);
            g.setColour (juce::Colours::black);
            g.fillRoundedRectangle (r, 3.0f); });
        keyboard.setBounds (302, 556, 1116, 366);
        addAndMakeVisible (keyboard);

        // ---------------- rear strip ----------------
        module ({ 78, 944, 120, 100 }, col::panel, "", ink);
        ticks (138, 986, 36, 11, ink); knob ("fineTune", 138, 986, 36, [] (double v) { return fmt::semitones (v * 100.0).replace (" st", " ct"); });
        textAt ("FINE TUNE", 138, 1022, ink, 8.0f, Weight::Bold, 80.0f);
        module ({ 214, 944, 190, 100 }, col::panel, "AUDIO", ink);
        jack (Jack::EuroOut, 285, 988, "EURORACK OUT", ink); jack (Jack::ReverbOut, 355, 988, "REVERB OUT", ink);
        module ({ 420, 944, 290, 100 }, col::amber, "ARP/SEQ CV", dark);
        jack (Jack::ClockIn, 466, 988, "CLOCK IN", dark); jack (Jack::OnOffIn, 540, 988, "ON / OFF IN", dark);
        jack (Jack::ResetIn, 614, 988, "RESET IN", dark); jack (Jack::ClockOut, 678, 988, "CLOCK OUT", dark);
    }

    Lamp* addLamp (juce::Colour c, float x, float y)
    {
        auto l = std::make_unique<Lamp> (c, 7.0f);
        l->setCentrePosition ((int) x, (int) y);
        addAndMakeVisible (*l);
        lamps.push_back (std::move (l));
        return lamps.back().get();
    }
    Pad* addPad (juce::Colour c, float x, float y, float size)
    {
        auto p = std::make_unique<Pad> (c, size);
        p->setCentrePosition ((int) x, (int) y);
        addAndMakeVisible (*p);
        pads.push_back (std::move (p));
        return pads.back().get();
    }

    bool recordMode() const { return (int) std::lround (apvts.getRawParameterValue ("arpMode")->load()) == 2; }
    void toggleParam (const char* id)
    {
        if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
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
        const auto name = processor.getCurrentPresetName();
        if (name != presetName) { presetName = name; repaint (770, 950, 380, 80); }
    }

    void drawPresetDisplay (juce::Graphics& g)
    {
        const juce::Rectangle<float> r (824.0f, 950.0f, 268.0f, 82.0f);
        label (g, "PRESET", juce::Rectangle<float> (824.0f, 934.0f, 268.0f, 14.0f), col::ink.withAlpha (0.8f), 8.5f, Weight::Bold, juce::Justification::centred, 0.14f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 6.0f);
        juce::ColourGradient bezel (juce::Colour (0xff3a3836), r.getX(), r.getY(), juce::Colour (0xff121110), r.getX(), r.getBottom(), false);
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (r, 6.0f);
        const auto lcd = r.reduced (6.0f);
        juce::ColourGradient glass (juce::Colour (0xff17110a), lcd.getX(), lcd.getY(), juce::Colour (0xff0a0704), lcd.getX(), lcd.getBottom(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (lcd, 4.0f);
        g.setColour (col::ledAmber.withAlpha (0.10f));
        g.fillRoundedRectangle (lcd.reduced (2.0f), 3.0f);
        auto f = font (Weight::Bold, 25.0f, 0.06f);
        g.setFont (f);
        g.setColour (col::ledAmber.withAlpha (0.28f));                                             // glow
        g.drawText (presetName.toUpperCase(), lcd.translated (0.0f, 1.0f), juce::Justification::centred, true);
        g.setColour (col::ledAmber);
        g.drawText (presetName.toUpperCase(), lcd, juce::Justification::centred, true);
        g.setColour (juce::Colours::white.withAlpha (0.07f));                                      // glass reflection
        g.fillRoundedRectangle (lcd.withHeight (lcd.getHeight() * 0.36f).reduced (3.0f, 2.0f), 3.0f);
    }
};
} // namespace babushka::ui
