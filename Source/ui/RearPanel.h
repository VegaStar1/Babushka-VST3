// Babushka — the rear panel that slides out from under the top edge, its pull handle, and the output meters.
//
// The rear panel holds what sits on the back of the hardware (AUDIO and ARP/SEQ CV jacks). Pulling the handle at the
// top of the instrument slides the whole front panel down; the rear panel comes out above it and the keyboard
// section gets shorter, so the window keeps its size and every control stays in view.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"
#include <functional>

namespace babushka::ui
{
// ---- pull handle: a tab with three ribs on the top edge of the chassis ------------------------------------------
class GripHandle : public juce::Component
{
public:
    GripHandle() { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); setSize (96, 22); }
    std::function<void()> onClick, onHover;
    std::function<void (float)> onDrag;               // vertical distance from where the drag started (logical px)
    std::function<void()> onDragEnd;
    void setOpen (bool o) { if (o != open) { open = o; repaint(); } }

    // A finger notch pressed into the chassis lip: the chassis' own finish, darker inside, with three ribs engraved
    // across it. It only warms up a little under the pointer, so it does not stand out from the case.
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
        const auto c = skin().chassis;
        juce::ColourGradient body (c.darker (0.45f), r.getX(), r.getY(), c.brighter (0.35f), r.getX(), r.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (r, 6.0f);
        grain (g, r, 1.0f);
        juce::ColourGradient inner (juce::Colours::black.withAlpha (0.55f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 7.0f, false);
        g.setGradientFill (inner);                                       // shadow of the upper edge of the notch
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.09f));                // light catching the lower lip
        g.drawLine (r.getX() + 6.0f, r.getBottom() + 0.5f, r.getRight() - 6.0f, r.getBottom() + 0.5f, 1.0f);
        const bool lit = hover || dragging;
        for (int i = 0; i < 3; ++i)                                      // engraved ribs: a groove and its lit lower edge
        {
            const float y = r.getY() + 5.5f + 4.0f * (float) i;
            g.setColour (juce::Colours::black.withAlpha (0.75f));
            g.fillRoundedRectangle (r.getCentreX() - 22.0f, y, 44.0f, 1.6f, 0.8f);
            g.setColour (lit ? col::ledAmber.withAlpha (0.45f) : juce::Colours::white.withAlpha (0.13f));
            g.fillRoundedRectangle (r.getCentreX() - 22.0f, y + 1.6f, 44.0f, 1.0f, 0.5f);
        }
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging && std::abs (e.getDistanceFromDragStartY()) < 4) return;
        dragging = true; repaint();
        if (onDrag) onDrag ((float) e.getDistanceFromDragStartY());
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging) { dragging = false; repaint(); if (onDragEnd) onDragEnd(); }
        else if (onClick) onClick();
    }

private:
    bool hover = false, dragging = false, open = false;
};

// ---- MIDI socket: a 5-pin DIN socket that opens the MIDI menu --------------------------------------------------------
class MidiSocket : public juce::Component
{
public:
    MidiSocket() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setSize (58, 58); }
    std::function<void()> onClick, onHover;
    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const float r = 26.0f;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - r, c.y - r + 1.5f, 2 * r, 2 * r);
        juce::ColourGradient ring (juce::Colour (0xfff0f1f2), c.x - r * 0.7f, c.y - r * 0.8f, juce::Colour (0xff55585d), c.x + r, c.y + r, false);
        ring.addColour (0.5, juce::Colour (0xffb9bcc0));
        g.setGradientFill (ring);
        g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
        const float h = r * 0.72f;
        juce::ColourGradient hole (juce::Colour (0xff1c1c1e), c.x - h * 0.3f, c.y - h * 0.4f, juce::Colour (0xff030303), c.x + h, c.y + h, true);
        g.setGradientFill (hole);
        g.fillEllipse (c.x - h, c.y - h, 2 * h, 2 * h);
        g.setColour (hover ? col::ledAmber.withAlpha (0.9f) : juce::Colours::white.withAlpha (0.15f));
        g.drawEllipse (c.x - h, c.y - h, 2 * h, 2 * h, hover ? 1.6f : 0.8f);
        // five pins on a half circle and the key notch
        for (int i = 0; i < 5; ++i)                                            // 180° … 360°: the upper half
        {
            const float a = juce::degreesToRadians (180.0f + 45.0f * (float) i);
            const juce::Point<float> p (c.x + std::cos (a) * h * 0.58f, c.y + std::sin (a) * h * 0.58f + 2.0f);
            g.setColour (juce::Colour (0xffa7a9ac));
            g.fillEllipse (p.x - 2.4f, p.y - 2.4f, 4.8f, 4.8f);
        }
        g.setColour (juce::Colour (0xff6d6f73));
        g.fillRoundedRectangle (c.x - 4.0f, c.y + h * 0.62f, 8.0f, 4.0f, 1.5f);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
private:
    bool hover = false;
};

// ---- a small amber read-out (the MIDI controller's name) --------------------------------------------------------------
class SmallLcd : public juce::Component
{
public:
    SmallLcd() { setInterceptsMouseClicks (false, false); }
    void setText (const juce::String& t) { if (t != text) { text = t; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0b0704));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 0.8f);
        label (g, text, r.reduced (3.0f, 0.0f), col::ledAmber, 10.5f, Weight::NarrowBold, juce::Justification::centred, 0.05f);
    }
private:
    juce::String text;
};

// ---- rear panel ---------------------------------------------------------------------------------------------------
// The panel's artwork is a fixed-size image (kHeight tall) that slides with the opening: the visible strip is its
// bottom part, as if it came out from under the chassis lip.
class RearPanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int kX = 78, kY = 56, kWidth = 1340, kHeight = 150, kGap = 10;     // kGap: to the front panel when open
    using Jack = babushka::dsp::Jack;

    explicit RearPanel (BabushkaProcessor& p) : processor (p)
    {
        setInterceptsMouseClicks (false, true);
        content.setInterceptsMouseClicks (false, false);
        content.setBufferedToImage (true);
        content.setSize (kWidth, kHeight);
        addAndMakeVisible (content);
        addAndMakeVisible (midiSocket);
        addAndMakeVisible (midiLamp);
        addAndMakeVisible (midiLcd);
        startTimerHz (30);
    }
    MidiSocket midiSocket;                       // onClick: the MIDI menu

    // Jack centres in the panel's own coordinates (the full-height artwork).
    static std::vector<std::pair<Jack, juce::Point<float>>> jackPositions()
    {
        std::vector<std::pair<Jack, juce::Point<float>>> j;
        j.push_back ({ Jack::EuroOut, { 902.0f, 70.0f } });
        j.push_back ({ Jack::ReverbOut, { 976.0f, 70.0f } });
        const Jack cv[] = { Jack::ClockIn, Jack::OnOffIn, Jack::ResetIn, Jack::ClockOut };
        for (int i = 0; i < 4; ++i) j.push_back ({ cv[i], { 1024.0f + 308.0f * ((float) i + 0.5f) / 4.0f, 70.0f } });
        return j;
    }

    void resized() override
    {
        const int y = getHeight() - kHeight;    // the artwork slides: controls sit at their place on it
        content.setTopLeftPosition (0, y);
        midiSocket.setCentrePosition (71, y + 58);
        midiLamp.setCentrePosition (24, y + 104);
        midiLcd.setBounds (34, y + 95, 90, 18);
    }

    void paint (juce::Graphics& g) override
    {
        // the recess the panel slides out of
        const auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (r, 5.0f);
    }
    void paintOverChildren (juce::Graphics& g) override
    {
        // shadows: under the chassis lip at the top, and of the front panel at the bottom
        const auto r = getLocalBounds().toFloat();
        juce::ColourGradient top (juce::Colours::black.withAlpha (0.75f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 14.0f, false);
        g.setGradientFill (top); g.fillRect (r.withHeight (14.0f));
        juce::ColourGradient bottom (juce::Colours::black.withAlpha (0.7f), 0.0f, r.getBottom(), juce::Colours::transparentBlack, 0.0f, r.getBottom() - 10.0f, false);
        g.setGradientFill (bottom); g.fillRect (r.withTrimmedTop (juce::jmax (0.0f, r.getHeight() - 10.0f)));
    }

private:
    BabushkaProcessor& processor;
    Lamp midiLamp { col::ledGreen, 7.0f };
    SmallLcd midiLcd;
    uint32_t lastActivity = 0;
    float lampLevel = 0.0f;

    void timerCallback() override
    {
        const auto a = processor.midiMap.lastActivity();
        if (a.counter != lastActivity) { lastActivity = a.counter; lampLevel = 1.0f; }
        else lampLevel = juce::jmax (0.0f, lampLevel - 0.12f);
        midiLamp.setLevel (lampLevel);
        midiLcd.setText (processor.midiMap.getShortName());
    }

    struct Content : public juce::Component
    {
        void paint (juce::Graphics& g) override
        {
            const auto& S = skin();
            const auto ink = inkOn (S.panel), cvInk = inkOn (S.rearCv);
            // back plate of the chassis with its screws
            const juce::Rectangle<float> back (4.0f, 4.0f, (float) kWidth - 8.0f, (float) kHeight - 8.0f);
            juce::ColourGradient metal (juce::Colour (0xff1b1b1b), 0.0f, back.getY(), juce::Colour (0xff0d0d0d), 0.0f, back.getBottom(), false);
            g.setGradientFill (metal);
            g.fillRoundedRectangle (back, 4.0f);
            grain (g, back, 0.7f);
            for (auto p : { juce::Point<float> (148.0f, 18.0f), juce::Point<float> (852.0f, 18.0f),           // between the plates
                            juce::Point<float> (148.0f, (float) kHeight - 18.0f), juce::Point<float> (852.0f, (float) kHeight - 18.0f) })
                drawScrew (g, p, 4.5f, 0.6f);

            auto plate = [&g] (juce::Rectangle<float> r, juce::Colour fill, const juce::String& title) {
                drawPlate (g, r, fill, true);
                label (g, title, juce::Rectangle<float> (r.getWidth() - 12.0f, 19.5f).withCentre ({ r.getCentreX(), r.getY() + 15.0f }), inkOn (fill), 13.0f,
                       Weight::NarrowBold, juce::Justification::centred, 0.12f);
            };
            // the name under the jack, on two lines when it is long (EURORACK / OUT)
            auto jack = [&g] (juce::Point<float> c, const juce::String& line1, const juce::String& line2, juce::Colour textColour) {
                drawJack (g, c);
                label (g, line1, juce::Rectangle<float> (70.0f, 14.0f).withCentre ({ c.x, c.y + 20.0f }), textColour, 10.0f, Weight::NarrowBold,
                       juce::Justification::centred, 0.03f);
                if (line2.isNotEmpty())
                    label (g, line2, juce::Rectangle<float> (70.0f, 14.0f).withCentre ({ c.x, c.y + 32.0f }), textColour, 10.0f, Weight::NarrowBold,
                           juce::Justification::centred, 0.03f);
            };
            plate ({ 8.0f, 8.0f, 126.0f, 134.0f }, S.panel, "MIDI");
            label (g, juce::String::fromUTF8 ("MIDI IN · MAP"), juce::Rectangle<float> (110.0f, 14.0f).withCentre ({ 71.0f, 128.0f }), ink, 10.0f, Weight::NarrowBold,
                   juce::Justification::centred, 0.05f);
            plate ({ 864.0f, 8.0f, 150.0f, 134.0f }, S.panel, "AUDIO");
            plate ({ 1024.0f, 8.0f, 308.0f, 134.0f }, S.rearCv, "ARP/SEQ CV");
            static const char* names[][2] = { { "EURORACK", "OUT" }, { "REVERB", "OUT" }, { "CLOCK", "IN" }, { "ON / OFF", "IN" }, { "RESET", "IN" }, { "CLOCK", "OUT" } };
            int i = 0;
            for (auto& [id, pos] : jackPositions()) { jack (pos, names[i][0], names[i][1], i < 2 ? ink : cvInk); ++i; }
        }
    } content;
};

// ---- output meters: a needle VU meter, an oscilloscope and a clip lamp -------------------------------------------------
// Both read the ring of recent output samples the processor keeps. The VU meter follows the classic standard
// (0 VU = −12 dBFS RMS here, ≈300 ms to reach the reading, a slight overshoot); the scope triggers on rising zero
// crossings and shows about two and a half periods of the note.
class OutputMeters : public juce::Component, private juce::Timer
{
public:
    explicit OutputMeters (BabushkaProcessor& p) : processor (p)
    {
        setInterceptsMouseClicks (false, false);
        lastPos = processor.scopeWritePosition();
        lastClips = processor.getClipCount();
        startTimerHz (60);
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        drawVu (g, { 6.0f, 4.0f, 206.0f, r.getHeight() - 8.0f });
        drawScope (g, { 222.0f, 4.0f, 206.0f, r.getHeight() - 8.0f });
        // clip lamp
        const juce::Point<float> c (r.getRight() - 24.0f, r.getCentreY() - 10.0f);
        if (clipHold > 0.0f) drawGlow (g, c, 18.0f, col::ledRed, juce::jmin (1.0f, clipHold * 2.0f));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - 5.5f, c.y - 5.0f, 11.0f, 11.0f);
        g.setColour (clipHold > 0.0f ? col::ledRed.brighter (0.3f) : col::ledRed.withMultipliedBrightness (0.25f));
        g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9.0f, 9.0f);
        label (g, "CLIP", juce::Rectangle<float> (40.0f, 14.0f).withCentre ({ c.x, c.y + 16.0f }), col::ink, 10.0f, Weight::NarrowBold, juce::Justification::centred, 0.08f);
    }

private:
    BabushkaProcessor& processor;
    int lastPos = 0;
    uint32_t lastClips = 0;
    float needle = 0.0f, needleVel = 0.0f, clipHold = 0.0f, scopeGain = 1.0f, lastRms = 0.0f, sinceAudio = 1.0f;
    float lampGlow = 1.0f, lampPhase = 0.0f;          // the lamps' brightness; it breathes a little, as old bulbs on a mains supply do
    std::vector<float> trace;          // the scope's current sweep

    // VU scale: the needle moves in proportion to the signal voltage, from −20 VU to +3 VU.
    static float needleFor (float vu) { const float v = std::pow (10.0f, vu / 20.0f); return juce::jlimit (-0.02f, 1.06f, (v - 0.1f) / (std::pow (10.0f, 0.15f) - 0.1f)); }

    void timerCallback() override
    {
        const int pos = processor.scopeWritePosition();
        const int fresh = juce::jlimit (0, BabushkaProcessor::kScopeSize / 2, (pos - lastPos) & 0x3fffffff);
        lastPos = pos;
        float sum = 0.0f, peak = 0.0f;
        for (int i = pos - fresh; i < pos; ++i) { const float x = processor.scopeSample (i); sum += x * x; peak = juce::jmax (peak, std::abs (x)); }
        // the host delivers audio in blocks, often less often than this timer runs: keep the last reading for a while
        constexpr float dt = 1.0f / 60.0f;
        if (fresh > 0) { lastRms = std::sqrt (sum / (float) fresh); sinceAudio = 0.0f; }
        else if ((sinceAudio += dt) > 0.15f) lastRms = 0.0f;
        const float rms = lastRms;
        const float vu = 20.0f * std::log10 (juce::jmax (rms, 1.0e-6f)) + 12.0f;
        // needle: a damped spring (ζ ≈ 0.8, ≈300 ms to the reading)
        constexpr float w = juce::MathConstants<float>::twoPi * 2.1f, zeta = 0.8f;
        const float target = needleFor (vu);
        needleVel += (w * w * (target - needle) - 2.0f * zeta * w * needleVel) * dt;
        needle = juce::jlimit (-0.03f, 1.08f, needle + needleVel * dt);
        // CLIP: the output limiter caught samples over full scale (or the output itself reached it)
        const uint32_t clips = processor.getClipCount();
        if (clips != lastClips || peak >= 0.99f) clipHold = 1.2f; else clipHold = juce::jmax (0.0f, clipHold - dt);
        lastClips = clips;
        lampPhase += dt;
        lampGlow = 0.96f + 0.04f * std::sin (lampPhase * 1.3f) * std::sin (lampPhase * 0.37f);
        if (fresh > 0) updateTrace (pos);
        else if (sinceAudio > 0.15f) for (auto& v : trace) v *= 0.85f;          // the host stopped: the trace fades
        repaint();
    }

    void updateTrace (int pos)
    {
        constexpr int span = BabushkaProcessor::kScopeSize / 2;
        const int start = pos - span;
        auto at = [&] (int i) { return processor.scopeSample (start + i); };
        // rising zero crossings (with a little hysteresis), newest last
        std::vector<int> ups;
        bool armed = false;
        for (int i = 1; i < span; ++i)
        {
            const float x = at (i);
            if (x < -0.02f) armed = true;
            else if (armed && x >= 0.0f && at (i - 1) < 0.0f) { ups.push_back (i); armed = false; }
        }
        int window = 1024;
        if (ups.size() >= 2) window = juce::jlimit (160, span / 2, (int) std::lround (2.5 * (ups.back() - ups[ups.size() - 2])));
        int from = span - window;
        for (int k = (int) ups.size() - 1; k >= 0; --k) if (ups[(size_t) k] + window <= span) { from = ups[(size_t) k]; break; }
        float pk = 0.0f;
        trace.assign ((size_t) window, 0.0f);
        for (int i = 0; i < window; ++i) { trace[(size_t) i] = at (from + i); pk = juce::jmax (pk, std::abs (trace[(size_t) i])); }
        const float wanted = 0.9f / juce::jmax (pk, 0.05f);               // slow auto-gain, so quiet notes are visible too
        scopeGain += (wanted - scopeGain) * 0.15f;
    }

    void drawVu (juce::Graphics& g, juce::Rectangle<float> r) const
    {
        // Backlit like the meters of old consoles: a warm incandescent lamp behind the scale. Its light spills out
        // round the bezel onto the panel, and the face is brightest low in the middle, where the lamps sit.
        const juce::Colour lampLight (0xffffb347);
        for (int i = 7; i >= 1; --i)
        {
            g.setColour (lampLight.withAlpha (0.07f * lampGlow));
            g.fillRoundedRectangle (r.expanded ((float) i * 1.15f), 6.0f + (float) i);
        }
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 5.0f);
        juce::ColourGradient bezel (juce::Colour (0xff3a3632), r.getX(), r.getY(), juce::Colour (0xff141210), r.getX(), r.getBottom(), false);
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (r, 5.0f);
        const auto face = r.reduced (3.0f);
        juce::ColourGradient lamp (juce::Colour (0xffffefc0).withMultipliedBrightness (0.9f + 0.1f * lampGlow), face.getCentreX(), face.getBottom() + 6.0f,
                                   juce::Colour (0xffb0702a), face.getX() - 10.0f, face.getY() - 4.0f, true);
        lamp.addColour (0.42, juce::Colour (0xfff9cf78));
        lamp.addColour (0.78, juce::Colour (0xffe3a548));
        g.setGradientFill (lamp);
        g.fillRoundedRectangle (face, 3.0f);
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (face.toNearestInt());
        // the two lamps under the scale, seen as hot spots through the paper
        for (float x : { face.getX() + face.getWidth() * 0.28f, face.getRight() - face.getWidth() * 0.28f })
        {
            juce::ColourGradient spot (juce::Colour (0xfffff6dc).withAlpha (0.5f * lampGlow), x, face.getBottom() + 2.0f,
                                       juce::Colour (0x00ffffff), x + face.getHeight() * 0.9f, face.getBottom() + 2.0f, true);
            g.setGradientFill (spot);
            g.fillEllipse (x - face.getHeight() * 0.9f, face.getBottom() - face.getHeight() * 0.9f + 2.0f, face.getHeight() * 1.8f, face.getHeight() * 1.8f);
        }
        // a long needle on a pivot below the window, so the scale spans most of its width
        const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + 88.0f);
        const float radius = face.getHeight() + 70.0f, sweep = juce::degreesToRadians (36.0f);
        auto pointAt = [&] (float pos, float rad) { const float a = -sweep + 2.0f * sweep * pos; return juce::Point<float> (pivot.x + rad * std::sin (a), pivot.y - rad * std::cos (a)); };
        const juce::Colour ink (0xff2b241a), red (0xffc0392b);
        juce::Path arc, redArc;
        for (int i = 0; i <= 40; ++i)
        {
            const float p = (float) i / 40.0f;
            const auto q = pointAt (p, radius);
            if (i == 0) arc.startNewSubPath (q); else arc.lineTo (q);
        }
        for (int i = 0; i <= 20; ++i)
        {
            const float p = needleFor (0.0f) + (needleFor (3.0f) - needleFor (0.0f)) * (float) i / 20.0f;
            const auto q = pointAt (p, radius + 2.5f);
            if (i == 0) redArc.startNewSubPath (q); else redArc.lineTo (q);
        }
        g.setColour (ink); g.strokePath (arc, juce::PathStrokeType (1.0f));
        g.setColour (red); g.strokePath (redArc, juce::PathStrokeType (4.0f));
        for (int db : { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 })
        {
            const float p = needleFor ((float) db);
            g.setColour (db >= 0 ? red : ink);
            g.drawLine (juce::Line<float> (pointAt (p, radius), pointAt (p, radius - 5.0f)), 1.0f);
            if (db != -2 && db != 2)                                                     // every mark numbered but ±2
                label (g, db > 0 ? "+" + juce::String (db) : juce::String (db), juce::Rectangle<float> (22.0f, 12.0f).withCentre (pointAt (p, radius - 12.0f)),
                       db >= 0 ? red : ink, 9.0f, Weight::NarrowBold, juce::Justification::centred, 0.0f);
        }
        label (g, "VU", juce::Rectangle<float> (30.0f, 14.0f).withCentre ({ face.getCentreX(), face.getBottom() - 11.0f }), ink, 12.0f, Weight::Bold, juce::Justification::centred, 0.1f);
        // needle and its shadow
        const auto tip = pointAt (needle, radius + 2.0f), base = pointAt (needle, 60.0f);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.drawLine (juce::Line<float> (base.translated (2.0f, 2.0f), tip.translated (2.0f, 2.0f)), 1.6f);
        g.setColour (juce::Colour (0xff141414));
        g.drawLine (juce::Line<float> (base, tip), 1.4f);
        // shadow of the bezel on the face, then the glass
        juce::ColourGradient inner (juce::Colours::black.withAlpha (0.45f), 0.0f, face.getY(), juce::Colours::transparentBlack, 0.0f, face.getY() + 9.0f, false);
        g.setGradientFill (inner);
        g.fillRect (face.withHeight (9.0f));
        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.36f).reduced (2.0f, 1.5f), 2.5f);
    }

    void drawScope (juce::Graphics& g, juce::Rectangle<float> r) const
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 5.0f);
        g.setColour (juce::Colour (0xff2a2724));
        g.fillRoundedRectangle (r, 5.0f);
        const auto face = r.reduced (3.0f);
        g.setColour (juce::Colour (0xff031008));
        g.fillRoundedRectangle (face, 3.0f);
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (face.toNearestInt());
        g.setColour (juce::Colour (0xff0f3a1f));                                        // graticule
        for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (face.getX() + face.getWidth() * (float) i / 8.0f), face.getY(), face.getBottom());
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (face.getY() + face.getHeight() * (float) i / 4.0f), face.getX(), face.getRight());
        if (trace.size() > 1)
        {
            juce::Path p;
            const float mid = face.getCentreY(), amp = face.getHeight() * 0.46f;
            const int n = (int) trace.size();
            for (int i = 0; i < n; ++i)
            {
                const float x = face.getX() + face.getWidth() * (float) i / (float) (n - 1);
                const float y = mid - juce::jlimit (-1.0f, 1.0f, trace[(size_t) i] * scopeGain) * amp;
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour (juce::Colour (0xff5dff8f).withAlpha (0.18f));                  // phosphor glow
            g.strokePath (p, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (juce::Colour (0xff7dffa6));
            g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            g.setColour (juce::Colour (0xff7dffa6).withAlpha (0.7f));
            g.drawHorizontalLine ((int) face.getCentreY(), face.getX(), face.getRight());
        }
        label (g, "SCOPE", juce::Rectangle<float> (50.0f, 12.0f).withPosition (face.getX() + 4.0f, face.getY() + 3.0f), juce::Colour (0xff5dff8f).withAlpha (0.7f),
               9.0f, Weight::NarrowBold, juce::Justification::centredLeft, 0.12f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.38f).reduced (2.0f, 1.5f), 2.5f);
    }
};
} // namespace babushka::ui
