// Babushka — the rear panel that slides out from under the top edge, its pull handle, and the output meters.
//
// The rear panel holds what sits on the back of the hardware (AUDIO and ARP/SEQ CV jacks). Pulling the handle at the
// top of the instrument slides the whole front panel down; the rear panel comes out above it and the keyboard
// section gets shorter, so the window keeps its size and every control stays in view.
#pragma once

#include "PedalUi.h"
#include <functional>

namespace babushka::ui
{
// ---- pull handle: a tab with three ribs on the top edge of the chassis ------------------------------------------
class GripHandle : public juce::Component
{
public:
    GripHandle() { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); setSize (164, 24); }
    std::function<void()> onClick, onHover, onLeave;   // onLeave: the pointer has gone (hides the hint)
    std::function<void (float)> onDrag;               // vertical distance from where the drag started (logical px)
    std::function<void()> onDragEnd;
    void setOpen (bool o) { if (o != open) { open = o; repaint(); } }

    // A finger notch pressed into the chassis lip, with three engraved ribs, and an engraved chevron on each side that
    // shows which way it goes (down: pull the rear panel out, up: push it back). The chassis' own finish; the ribs and
    // chevrons warm to amber under the pointer.
    void paint (juce::Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        const auto r = juce::Rectangle<float> (104.0f, 22.0f).withCentre (all.getCentre());
        const auto c = skin().chassis;
        const bool lit = hover || dragging;
        juce::ColourGradient body (c.darker (0.6f), r.getX(), r.getY(), c.brighter (0.45f), r.getX(), r.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (r, 7.0f);
        grain (g, r, 1.0f);
        juce::ColourGradient inner (juce::Colours::black.withAlpha (0.6f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 8.0f, false);
        g.setGradientFill (inner);                                       // shadow of the upper edge of the notch
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.22f : 0.14f));   // light catching the lower lip
        g.drawLine (r.getX() + 7.0f, r.getBottom() + 0.5f, r.getRight() - 7.0f, r.getBottom() + 0.5f, 1.0f);
        const auto edge = lit ? col::ledAmber.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.30f);
        for (int i = 0; i < 3; ++i)                                      // engraved ribs: a groove and its lit lower edge
        {
            const float y = r.getY() + 5.5f + 4.0f * (float) i;
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.fillRoundedRectangle (r.getCentreX() - 26.0f, y, 52.0f, 1.7f, 0.8f);
            g.setColour (edge);
            g.fillRoundedRectangle (r.getCentreX() - 26.0f, y + 1.7f, 52.0f, 1.0f, 0.5f);
        }
        for (float x : { all.getX() + 14.0f, all.getRight() - 14.0f })   // chevrons
        {
            const float cy = all.getCentreY(), d = open ? -1.0f : 1.0f;
            juce::Path v;
            v.startNewSubPath (x - 6.0f, cy - 3.0f * d);
            v.lineTo (x, cy + 3.0f * d);
            v.lineTo (x + 6.0f, cy - 3.0f * d);
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.strokePath (v, juce::PathStrokeType (2.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
            g.setColour (lit ? col::ledAmber : col::ink.withAlpha (0.55f));
            g.strokePath (v, juce::PathStrokeType (1.3f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded),
                          juce::AffineTransform::translation (0.0f, 0.8f));
        }
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); if (onLeave) onLeave(); }
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
    std::function<void()> onClick, onHover, onLeave;   // onLeave: the pointer has gone (hides the hint)
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
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); if (onLeave) onLeave(); }
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
        for (int i = 0; i < babushka::kNumPedals; ++i)
        {
            auto& u = pedals[(size_t) i];
            u = std::make_unique<PedalUnit> (p, i);
            addAndMakeVisible (*u);
            u->onDragMove = [this] (int pedal, float x) { dragPedal (pedal, x); };
            u->onDragEnd = [this] (int pedal) { dropPedal (pedal); };
            u->leftArrow.onClick = [this, i] { movePedal (i, -1); };
            u->rightArrow.onClick = [this, i] { movePedal (i, +1); };
            u->lcd.onClick = [this, i] { if (onOpenPedal) onOpenPedal (i); };
        }
        syncOrder (true);
        startTimerHz (30);
    }
    MidiSocket midiSocket;                       // onClick: the MIDI menu
    std::function<void (int)> onOpenPedal;       // a pedal's display was clicked
    std::function<void()> onJacksMoved;          // the pedals (and their jacks) changed places

    void setHints (HintSink* h) { for (auto& u : pedals) u->setHints (h); }
    std::vector<Knob*> pedalKnobs() const
    {
        std::vector<Knob*> all;
        for (auto& u : pedals) for (auto* k : u->knobControls()) all.push_back (k);
        return all;
    }

    // The pedal board: four places of 192 px, 8 px apart, between the MIDI and AUDIO plates. The plates on the right:
    // AUDIO 944 … 1084, ARP/SEQ CV 1094 … 1332.
    static constexpr float kBoardX = 143.0f, kSlot = 200.0f, kPedalY = 8.0f;
    static constexpr float kAudioX = 944.0f, kAudioW = 140.0f, kCvX = 1094.0f, kCvW = 238.0f;

    // Jack centres in the panel's own coordinates (the full-height artwork). The pedals' jacks go where the pedals are.
    static std::vector<std::pair<Jack, juce::Point<float>>> fixedJacks()
    {
        std::vector<std::pair<Jack, juce::Point<float>>> j;
        j.push_back ({ Jack::EuroOut, { kAudioX + kAudioW * 0.26f, 70.0f } });
        j.push_back ({ Jack::ReverbOut, { kAudioX + kAudioW * 0.74f, 70.0f } });
        const Jack cv[] = { Jack::ClockIn, Jack::OnOffIn, Jack::ResetIn, Jack::ClockOut };
        for (int i = 0; i < 4; ++i) j.push_back ({ cv[i], { kCvX + kCvW * ((float) i + 0.5f) / 4.0f, 70.0f } });
        return j;
    }
    std::vector<std::pair<Jack, juce::Point<float>>> jackPositions() const
    {
        auto j = fixedJacks();
        const Jack pj[][3] = { { Jack::SatIn, Jack::SatCv, Jack::SatOut }, { Jack::PhaIn, Jack::PhaCv, Jack::PhaOut },
                               { Jack::ChoIn, Jack::ChoCv, Jack::ChoOut }, { Jack::TapIn, Jack::TapCv, Jack::TapOut } };
        for (int p = 0; p < babushka::kNumPedals; ++p)
            for (int k = 0; k < 3; ++k)
                j.push_back ({ pj[p][k], { pedalX[(size_t) p] + PedalUnit::kJackX[k], kPedalY + PedalUnit::kJackY } });
        return j;
    }

    void resized() override
    {
        const int y = getHeight() - kHeight;    // the artwork slides: controls sit at their place on it
        content.setTopLeftPosition (0, y);
        midiSocket.setCentrePosition (71, y + 58);
        midiLamp.setCentrePosition (24, y + 104);
        midiLcd.setBounds (34, y + 95, 90, 18);
        for (int i = 0; i < babushka::kNumPedals; ++i)
            pedals[(size_t) i]->setTopLeftPosition ((int) std::lround (pedalX[(size_t) i]), y + (int) kPedalY);
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
    std::array<std::unique_ptr<PedalUnit>, babushka::kNumPedals> pedals;
    std::array<int, babushka::kNumPedals> order { 0, 1, 2, 3 };     // chain order: pedal indices, first to last
    std::array<float, babushka::kNumPedals> pedalX {};               // left edge of each pedal (by pedal index)
    int orderIndex = -1, refreshTick = 0;

    // ---- order of the chain --------------------------------------------------------------------------------------
    int slotOf (int pedal) const { for (int k = 0; k < babushka::kNumPedals; ++k) if (order[(size_t) k] == pedal) return k; return 0; }

    // Picks up the order from the parameter (a preset, undo, the host) unless a pedal is being dragged.
    void syncOrder (bool force = false)
    {
        for (auto& u : pedals) if (u->isDragging()) return;
        const int idx = (int) std::lround (pedalValue (processor, "fxOrder"));
        if (idx == orderIndex && ! force) return;
        orderIndex = idx;
        order = babushka::pedalOrderFromIndex (idx);
        for (int k = 0; k < babushka::kNumPedals; ++k) pedalX[(size_t) order[(size_t) k]] = kBoardX + kSlot * (float) k;
        resized();
        if (onJacksMoved) onJacksMoved();
    }
    void setOrder (const std::array<int, babushka::kNumPedals>& o)
    {
        setPedalParam (processor, "fxOrder", (float) babushka::pedalOrderIndex (o));
        syncOrder (true);
    }
    void movePedal (int pedal, int delta)
    {
        const int k = slotOf (pedal), j = k + delta;
        if (j < 0 || j >= babushka::kNumPedals) return;
        auto o = order;
        std::swap (o[(size_t) k], o[(size_t) j]);
        setOrder (o);
    }
    // While dragging, the pedal follows the pointer and the others make room for it.
    std::array<int, babushka::kNumPedals> orderWithDragged (int pedal, float x) const
    {
        std::array<int, babushka::kNumPedals> o {};
        const int target = juce::jlimit (0, babushka::kNumPedals - 1, (int) std::lround ((x - kBoardX) / kSlot));
        int n = 0;
        for (int k = 0; k < babushka::kNumPedals; ++k)
        {
            if (k == target) o[(size_t) k] = pedal;
            else { while (order[(size_t) n] == pedal) ++n; o[(size_t) k] = order[(size_t) n++]; }
        }
        return o;
    }
    void dragPedal (int pedal, float x)
    {
        x = juce::jlimit (kBoardX, kBoardX + kSlot * (float) (babushka::kNumPedals - 1), x);
        const auto o = orderWithDragged (pedal, x);
        for (int k = 0; k < babushka::kNumPedals; ++k)
            if (o[(size_t) k] != pedal) pedalX[(size_t) o[(size_t) k]] = kBoardX + kSlot * (float) k;
        pedalX[(size_t) pedal] = x;
        resized();
        if (onJacksMoved) onJacksMoved();
    }
    void dropPedal (int pedal)
    {
        const auto o = orderWithDragged (pedal, pedalX[(size_t) pedal]);
        if (o != order) setOrder (o); else syncOrder (true);
    }

    void timerCallback() override
    {
        syncOrder();
        if (++refreshTick % 3 == 0) for (auto& u : pedals) u->refresh();
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
            {   // the pedal board between the MIDI and AUDIO plates: a ribbed rubber mat the pedals sit on
                const juce::Rectangle<float> board (139.0f, 6.0f, 800.0f, (float) kHeight - 12.0f);
                g.setColour (juce::Colour (0xff0a0a0a));
                g.fillRoundedRectangle (board, 4.0f);
                for (float yy = board.getY() + 4.0f; yy < board.getBottom() - 2.0f; yy += 4.0f)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.035f));
                    g.fillRect (board.getX() + 3.0f, yy, board.getWidth() - 6.0f, 1.0f);
                }
                g.setColour (juce::Colours::black.withAlpha (0.8f));
                g.drawRoundedRectangle (board, 4.0f, 1.0f);
            }

            auto plate = [&g] (juce::Rectangle<float> r, juce::Colour fill, const juce::String& title) {
                drawPlate (g, r, fill, true);
                label (g, title, juce::Rectangle<float> (r.getWidth() - 12.0f, 19.5f).withCentre ({ r.getCentreX(), r.getY() + 15.0f }), inkOn (fill), type::title,
                       Weight::NarrowBold, juce::Justification::centred, 0.12f);
            };
            // the name under the jack, on two lines when it is long (EURORACK / OUT)
            auto jack = [&g] (juce::Point<float> c, const juce::String& line1, const juce::String& line2, juce::Colour textColour) {
                drawJack (g, c);
                label (g, line1, juce::Rectangle<float> (54.0f, 14.0f).withCentre ({ c.x, c.y + 20.0f }), textColour, type::jack, Weight::NarrowBold,
                       juce::Justification::centred, 0.03f);
                if (line2.isNotEmpty())
                    label (g, line2, juce::Rectangle<float> (54.0f, 14.0f).withCentre ({ c.x, c.y + 32.0f }), textColour, type::jack, Weight::NarrowBold,
                           juce::Justification::centred, 0.03f);
            };
            plate ({ 8.0f, 8.0f, 126.0f, 134.0f }, S.panel, "MIDI");
            label (g, juce::String::fromUTF8 ("MIDI IN · MAP"), juce::Rectangle<float> (110.0f, 14.0f).withCentre ({ 71.0f, 128.0f }), ink, type::jack, Weight::NarrowBold,
                   juce::Justification::centred, 0.05f);
            plate ({ kAudioX, 8.0f, kAudioW, 134.0f }, S.panel, "AUDIO");
            plate ({ kCvX, 8.0f, kCvW, 134.0f }, S.rearCv, "ARP/SEQ CV");
            static const char* names[][2] = { { "EURORACK", "OUT" }, { "REVERB", "OUT" }, { "CLOCK", "IN" }, { "ON / OFF", "IN" }, { "RESET", "IN" }, { "CLOCK", "OUT" } };
            int i = 0;
            for (auto& [id, pos] : fixedJacks()) { jack (pos, names[i][0], names[i][1], i < 2 ? ink : cvInk); ++i; }
        }
    } content;
};

// ---- output meters: a needle VU meter, an oscilloscope and a clip lamp -------------------------------------------------
// Both read the ring of recent output samples the processor keeps. The VU meter follows the classic standard
// (0 VU = −12 dBFS RMS here, ≈300 ms to reach the reading, a slight overshoot); the scope triggers on rising zero
// crossings and shows about two and a half periods of the note.
//
// The backlight follows the needle: dim at rest, glowing up as the level rises (with a bulb's lag).
// Cheap to run: the face (lamps, scale, print) is drawn only when the window is scaled, as two images — lamps low and
// lamps full — and each frame blends them; only the needle area, the scope and the lamp are repainted, and only when
// they change.
class OutputMeters : public juce::Component, private juce::Timer
{
public:
    explicit OutputMeters (BabushkaProcessor& p) : processor (p)
    {
        setInterceptsMouseClicks (true, false);           // only for the hints
        lastPos = processor.scopeWritePosition();
        lastClips = processor.getClipCount();
        startTimerHz (60);
    }

    HintSink* hints = nullptr;
    // What the meters show, when the pointer rests on them.
    void mouseMove (const juce::MouseEvent& e) override
    {
        if (hints == nullptr) return;
        const auto p = e.position;
        const int zone = p.getDistanceFrom (lampCentre()) < 22.0f ? 1 : vuArea().contains (p) ? 2 : 0;
        if (zone == hintZone) return;
        hintZone = zone;
        if (zone == 1) hints->showHint (*this, "CLIP: a sample peak reached 0 dBFS (it would clip; the output limiter catches it)");
        else if (zone == 2) hints->showHint (*this, "VU: the average level (0 VU = -12 dBFS); the peaks light CLIP");
        else hints->hideHint();
    }
    void mouseExit (const juce::MouseEvent&) override { hintZone = 0; if (hints) hints->hideHint(); }

    void paint (juce::Graphics& g) override
    {
        drawVu (g, vuArea());
        drawScope (g, scopeArea());
        const auto c = lampCentre();
        if (clipHold > 0.0f) drawGlow (g, c, 14.0f, col::ledRed, juce::jmin (1.0f, clipHold * 2.0f));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - 5.5f, c.y - 5.0f, 11.0f, 11.0f);
        g.setColour (clipHold > 0.0f ? col::ledRed.brighter (0.3f) : col::ledRed.withMultipliedBrightness (0.25f));
        g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9.0f, 9.0f);
        label (g, "CLIP", juce::Rectangle<float> (40.0f, 14.0f).withCentre ({ c.x, c.y + 16.0f }), col::ink, type::jack, Weight::NarrowBold, juce::Justification::centred, 0.08f);
    }

private:
    BabushkaProcessor& processor;
    int lastPos = 0;
    uint32_t lastClips = 0;
    float needle = 0.0f, needleVel = 0.0f, drawnNeedle = -1.0f, clipHold = 0.0f, scopeGain = 1.0f, lastRms = 0.0f, sinceAudio = 1.0f;
    float backlight = 0.0f;             // 0 … 1, follows the needle
    bool traceDirty = true, lampWasOn = false;
    std::vector<float> trace;          // the scope's current sweep
    juce::Image faceDim, faceLit;      // the face with the lamps low and at full
    float cachedScale = 0.0f;

    juce::Rectangle<float> vuArea() const { return { 6.0f, 4.0f, 206.0f, (float) getHeight() - 8.0f }; }
    juce::Rectangle<float> scopeArea() const { return { 222.0f, 4.0f, 206.0f, (float) getHeight() - 8.0f }; }
    int hintZone = 0;
    juce::Point<float> lampCentre() const { return { (float) getWidth() - 24.0f, (float) getHeight() * 0.5f - 10.0f }; }

    // VU scale: the needle moves in proportion to the signal voltage, from −20 VU to +3 VU.
    static float needleFor (float vu) { const float v = std::pow (10.0f, vu / 20.0f); return juce::jlimit (-0.02f, 1.06f, (v - 0.1f) / (std::pow (10.0f, 0.15f) - 0.1f)); }

    // Needle geometry, shared by the face (scale) and the needle.
    struct Dial
    {
        juce::Point<float> pivot; float radius, sweep;
        explicit Dial (juce::Rectangle<float> face)
            : pivot (face.getCentreX(), face.getBottom() + 88.0f), radius (face.getHeight() + 70.0f), sweep (juce::degreesToRadians (36.0f)) {}
        juce::Point<float> at (float pos, float rad) const
        {
            const float a = -sweep + 2.0f * sweep * pos;
            return { pivot.x + rad * std::sin (a), pivot.y - rad * std::cos (a) };
        }
    };

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
        const float vu = 20.0f * std::log10 (juce::jmax (lastRms, 1.0e-6f)) + 12.0f;
        // needle: a damped spring (ζ ≈ 0.8, ≈300 ms to the reading)
        constexpr float w = juce::MathConstants<float>::twoPi * 2.1f, zeta = 0.8f;
        needleVel += (w * w * (needleFor (vu) - needle) - 2.0f * zeta * w * needleVel) * dt;
        needle = juce::jlimit (-0.03f, 1.08f, needle + needleVel * dt);
        // CLIP: samples at 0 dBFS before the output limiter (or at the output itself)
        const uint32_t clips = processor.getClipCount();
        if (clips != lastClips || peak >= 0.999f) clipHold = 1.2f; else clipHold = juce::jmax (0.0f, clipHold - dt);
        lastClips = clips;
        // backlight: follows the needle, warming up quicker than it cools down, like a filament
        const float wantedLight = juce::jlimit (0.0f, 1.0f, 0.3f + needle * 1.1f);
        const float newLight = backlight + (wantedLight - backlight) * (wantedLight > backlight ? 0.25f : 0.07f);
        const bool lightChanged = std::abs (newLight - backlight) > 0.002f;
        backlight = newLight;
        if (fresh > 0) { updateTrace (pos); traceDirty = true; }
        else if (sinceAudio > 0.15f && ! trace.empty())
        {
            float m = 0.0f;
            for (auto& v : trace) { v *= 0.85f; m = juce::jmax (m, std::abs (v)); }   // the host stopped: the trace fades
            if (m < 0.002f) trace.clear();
            traceDirty = true;
        }
        // repaint only what changed
        if (lightChanged) repaint (vuArea().toNearestInt().expanded (2));
        else if (std::abs (needle - drawnNeedle) > 0.0008f) repaint (vuArea().toNearestInt());       // the needle and its shadow
        if (traceDirty) { repaint (scopeArea().toNearestInt()); traceDirty = false; }
        if ((clipHold > 0.0f) != lampWasOn || clipHold > 0.0f) { repaint (juce::Rectangle<float> (40.0f, 46.0f).withCentre (lampCentre()).toNearestInt()); lampWasOn = clipHold > 0.0f; }
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

    // The face as an image, redrawn only when the light or the scale has changed.
    void drawVu (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 5.0f);
        juce::ColourGradient bezel (juce::Colour (0xff3a3632), r.getX(), r.getY(), juce::Colour (0xff141210), r.getX(), r.getBottom(), false);
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (r, 5.0f);
        const float light = juce::jmax (0.0f, backlight);
        // lamp light catching the inner edge of the bezel, round the window
        g.setColour (juce::Colour (0xffffb347).withAlpha (0.10f + 0.35f * light));
        g.drawRoundedRectangle (r.reduced (2.0f), 4.0f, 1.2f);

        const auto face = r.reduced (3.0f);
        const float scale = juce::jmax (1.0f, (float) g.getInternalContext().getPhysicalPixelScaleFactor());
        if (! faceDim.isValid() || std::abs (cachedScale - scale) > 0.01f)
        {
            const int w = (int) std::ceil (face.getWidth() * scale), h = (int) std::ceil (face.getHeight() * scale);
            for (auto* im : { &faceDim, &faceLit })
            {
                *im = juce::Image (juce::Image::ARGB, w, h, true);
                juce::Graphics ig (*im);
                ig.addTransform (juce::AffineTransform::scale (scale));
                drawFace (ig, face.withPosition (0.0f, 0.0f), im == &faceLit ? 1.0f : 0.88f);   // at rest: well lit, just less than with signal
            }
            cachedScale = scale;
        }
        g.drawImage (faceDim, face);
        {
            juce::Graphics::ScopedSaveState fade (g);
            g.setOpacity (light);
            g.drawImage (faceLit, face);
        }

        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (face.toNearestInt());
        const Dial dial (face);
        // the needle: a thin blade, wider at the pivot and tapering to the tip, standing a little off the scale, so it
        // throws a soft shadow onto the paper (offset down-right, from the lamps' light)
        const auto tip = dial.at (needle, dial.radius + 3.0f), base = dial.at (needle, 52.0f);
        const auto dir = (tip - base) / juce::jmax (1.0f, tip.getDistanceFrom (base));
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path blade;
        blade.startNewSubPath (base + n * 1.8f);
        blade.lineTo (tip + n * 0.55f);
        blade.lineTo (tip - n * 0.55f);
        blade.lineTo (base - n * 1.8f);
        blade.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillPath (blade, juce::AffineTransform::translation (4.0f, 5.0f));
        g.setColour (juce::Colours::black.withAlpha (0.20f));
        g.fillPath (blade, juce::AffineTransform::translation (2.4f, 3.0f));
        g.setColour (juce::Colour (0xff111111));
        g.fillPath (blade);
        g.setColour (juce::Colours::white.withAlpha (0.18f));                           // a glint along one edge
        g.drawLine (juce::Line<float> (base + n * 1.2f, tip + n * 0.35f), 0.5f);
        drawnNeedle = needle;
        // shadow of the bezel on the face, then the glass
        juce::ColourGradient inner (juce::Colours::black.withAlpha (0.45f), 0.0f, face.getY(), juce::Colours::transparentBlack, 0.0f, face.getY() + 9.0f, false);
        g.setGradientFill (inner);
        g.fillRect (face.withHeight (9.0f));
        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.36f).reduced (2.0f, 1.5f), 2.5f);
    }

    // Backlit like the meters of old consoles: warm incandescent lamps behind the scale paper. light = 0 … 1: with the
    // lamps low the paper is a dim brown-amber, at full the middle glows almost white.
    static void drawFace (juce::Graphics& g, juce::Rectangle<float> face, float light)
    {
        const float k = 0.35f + 0.65f * light;
        juce::ColourGradient lamp (juce::Colour (0xffffefc0).withMultipliedBrightness (k), face.getCentreX(), face.getBottom() + 6.0f,
                                   juce::Colour (0xffb0702a).withMultipliedBrightness (0.45f + 0.55f * light), face.getX() - 10.0f, face.getY() - 4.0f, true);
        lamp.addColour (0.42, juce::Colour (0xfff9cf78).withMultipliedBrightness (k));
        lamp.addColour (0.78, juce::Colour (0xffe3a548).withMultipliedBrightness (0.4f + 0.6f * light));
        g.setGradientFill (lamp);
        g.fillRoundedRectangle (face, 3.0f);
        for (float x : { face.getX() + face.getWidth() * 0.28f, face.getRight() - face.getWidth() * 0.28f })   // the lamps' hot spots
        {
            juce::ColourGradient spot (juce::Colour (0xfffff6dc).withAlpha (0.55f * light), x, face.getBottom() + 2.0f,
                                       juce::Colour (0x00ffffff), x + face.getHeight() * 0.9f, face.getBottom() + 2.0f, true);
            g.setGradientFill (spot);
            g.fillEllipse (x - face.getHeight() * 0.9f, face.getBottom() - face.getHeight() * 0.9f + 2.0f, face.getHeight() * 1.8f, face.getHeight() * 1.8f);
        }
        const Dial dial (face);
        const juce::Colour ink (0xff2b241a), red (0xffc0392b);
        juce::Path arc, redArc;
        for (int i = 0; i <= 40; ++i)
        {
            const auto q = dial.at ((float) i / 40.0f, dial.radius);
            if (i == 0) arc.startNewSubPath (q); else arc.lineTo (q);
        }
        for (int i = 0; i <= 20; ++i)
        {
            const auto q = dial.at (needleFor (0.0f) + (needleFor (3.0f) - needleFor (0.0f)) * (float) i / 20.0f, dial.radius + 2.5f);
            if (i == 0) redArc.startNewSubPath (q); else redArc.lineTo (q);
        }
        g.setColour (ink); g.strokePath (arc, juce::PathStrokeType (1.0f));
        g.setColour (red); g.strokePath (redArc, juce::PathStrokeType (4.0f));
        for (int db : { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 })
        {
            const float p = needleFor ((float) db);
            g.setColour (db >= 0 ? red : ink);
            g.drawLine (juce::Line<float> (dial.at (p, dial.radius), dial.at (p, dial.radius - 5.0f)), 1.0f);
            if (db != -2 && db != 2)                                                     // every mark numbered but ±2
                label (g, db > 0 ? "+" + juce::String (db) : juce::String (db), juce::Rectangle<float> (22.0f, 12.0f).withCentre (dial.at (p, dial.radius - 12.0f)),
                       db >= 0 ? red : ink, 9.0f, Weight::NarrowBold, juce::Justification::centred, 0.0f);
        }
        label (g, "VU", juce::Rectangle<float> (30.0f, 14.0f).withCentre ({ face.getCentreX(), face.getBottom() - 11.0f }), ink, 12.0f, Weight::Bold, juce::Justification::centred, 0.1f);
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
            const int stride = juce::jmax (1, n / 400);                                 // at most ~400 points
            for (int i = 0; i < n; i += stride)
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
