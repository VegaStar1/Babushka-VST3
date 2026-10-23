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
                if (line2 == "OUT")
                {
                    // both lines in one small box, tight round the longer name: black with white print on the coloured
                    // plate, soft cream with dark print on the black one
                    const bool onColour = textColour.getPerceivedBrightness() < 0.5f;
                    const auto f = font (Weight::NarrowBold, type::jack, 0.03f);
                    const float w = juce::jmax (juce::GlyphArrangement::getStringWidth (f, line1), juce::GlyphArrangement::getStringWidth (f, line2)) + 6.0f;
                    const auto box = juce::Rectangle<float> (w, 24.0f).withCentre ({ c.x, c.y + 26.0f });
                    g.setColour (onColour ? juce::Colour (0xff161514).withAlpha (0.88f) : juce::Colour (0xffe6e1d3).withAlpha (0.82f));
                    g.fillRoundedRectangle (box, 1.2f);
                    const auto print = onColour ? juce::Colour (0xffece6d6) : juce::Colour (0xff1a1918);
                    for (auto [t, y] : { std::pair<juce::String, float> { line1, c.y + 20.0f }, { line2, c.y + 32.0f } })
                        label (g, t, juce::Rectangle<float> (w - 2.0f, 12.0f).withCentre ({ c.x, y }), print, type::jack, Weight::NarrowBold,
                               juce::Justification::centred, 0.03f);
                    return;
                }
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

// ---- output meters -------------------------------------------------------------------------------------------------
// One window under glass with a drum behind it. Front (the default): a digital L-R meter, two rows of LED segments.
// The button on the right turns the drum (a short flip about its horizontal axis): on the back, a needle VU meter and
// an oscilloscope, and the CLIP lamp comes up beside the window. All of it reads the rings of recent output samples
// the processor keeps.
//
// L-R: 0 on the scale is -12 dBFS RMS, like the VU meter; the bars rise at once, fall 20 dB/s, the peak segment holds
// for a second. VU: 0 VU = -12 dBFS RMS, ~300 ms to the reading with a slight overshoot; its backlight follows the
// needle. The scope triggers on rising zero crossings and shows about two and a half periods of the note.
class OutputMeters : public juce::Component, private juce::Timer
{
public:
    explicit OutputMeters (BabushkaProcessor& p) : processor (p)
    {
        lastPos = processor.scopeWritePosition();
        lastClips = processor.getClipCount();
        startTimerHz (60);
    }

    HintSink* hints = nullptr;
    std::function<void (bool)> onModeChanged;          // the user switched the meters (true: VU + scope)

    bool showsVu() const { return vu; }
    void setShowsVu (bool v, bool animate = true)
    {
        vu = v;
        if (! animate) { flip = vu ? 1.0f : 0.0f; flipVel = 0.0f; }
        repaint();
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        if (hints == nullptr) return;
        const auto p = e.position;
        const int zone = buttonArea().contains (p) ? 3 : (vu && p.getDistanceFrom (lampCentre()) < 14.0f) ? 1 : window().contains (p) ? 2 : 0;
        if (zone == hintZone) return;
        hintZone = zone;
        if (zone == 3) hints->showHint (*this, vu ? "Show the L-R meter" : "Show the VU meter and the oscilloscope");
        else if (zone == 1) hints->showHint (*this, "CLIP: the output reached 0 dBFS - turn VOLUME (or the channel fader) down");
        else if (zone == 2) hints->showHint (*this, vu ? "VU: the average level (0 VU = -12 dBFS); the peaks light CLIP"
                                                       : "L-R: the level of each channel (0 = -12 dBFS); the red segments are the headroom up to 0 dBFS");
        else hints->hideHint();
        setMouseCursor (zone == 3 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }
    void mouseExit (const juce::MouseEvent&) override { hintZone = 0; if (hints) hints->hideHint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! buttonArea().contains (e.position)) return;
        pressed = true; repaint();
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! pressed) return;
        pressed = false;
        if (buttonArea().contains (e.position)) { vu = ! vu; if (onModeChanged) onModeChanged (vu); hintZone = 0; mouseMove (e); }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto win = window();
        // chrome bezel and the dark recess the drum turns in
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (win.expanded (6.0f, 2.0f).translated (0.0f, 2.5f), 7.0f);
        juce::ColourGradient chrome (juce::Colour (0xffeef0f2), 0.0f, win.getY() - 2.0f, juce::Colour (0xffe0e2e5), 0.0f, win.getBottom() + 2.0f, false);
        chrome.addColour (0.16, juce::Colour (0xff8e9297)); chrome.addColour (0.5, juce::Colour (0xff474a4e)); chrome.addColour (0.84, juce::Colour (0xff9fa3a8));
        g.setGradientFill (chrome);
        g.fillRoundedRectangle (win.expanded (6.0f, 2.0f), 7.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (win.expanded (3.0f, 0.0f), 5.0f);
        {
            // the drum: the face that is turned towards the glass, squeezed by the angle and shaded
            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (win.toNearestInt());
            const float angle = flip * juce::MathConstants<float>::pi;
            const float squeeze = juce::jmax (0.02f, std::abs (std::cos (angle)));
            g.addTransform (juce::AffineTransform::scale (1.0f, squeeze, win.getCentreX(), win.getCentreY()));
            if (flip < 0.5f) drawLr (g, win); else { drawVu (g, vuArea()); drawScope (g, scopeArea()); }
            const float shade = std::abs (std::sin (angle)) * 0.75f;
            if (shade > 0.01f)
            {
                juce::ColourGradient sh (juce::Colours::black.withAlpha (shade), 0.0f, win.getY(), juce::Colours::black.withAlpha (shade), 0.0f, win.getBottom(), false);
                sh.addColour (0.5, juce::Colours::black.withAlpha (shade * 0.3f));
                g.setGradientFill (sh);
                g.fillRect (win);
            }
        }
        // the glass: its reflection stays put while the drum turns behind it
        juce::ColourGradient glass (juce::Colours::white.withAlpha (0.22f), win.getX(), win.getY(), juce::Colours::transparentWhite, win.getX() + 90.0f, win.getY() + 60.0f, false);
        glass.addColour (0.5, juce::Colours::white.withAlpha (0.06f));
        g.setGradientFill (glass);
        g.fillRoundedRectangle (win.expanded (3.0f, 0.0f), 5.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawLine (win.getX() + 4.0f, win.getY() + 0.6f, win.getRight() - 4.0f, win.getY() + 0.6f, 0.8f);

        // the bay on the right: CLIP (with the VU meter) over the button
        const float lampUp = juce::jlimit (0.0f, 1.0f, (flip - 0.55f) / 0.35f);
        const auto c = lampCentre();
        if (lampUp > 0.0f)
        {
            juce::Graphics::ScopedSaveState s (g);
            g.setOpacity (lampUp);
            if (clipHold > 0.0f) drawGlow (g, c, 16.0f, col::ledRed, juce::jmin (1.0f, clipHold * 2.0f));
            juce::ColourGradient ring (juce::Colour (0xfff6f7f8), c.x - 8.0f, c.y - 8.0f, juce::Colour (0xff62666b), c.x + 8.0f, c.y + 8.0f, false);
            g.setGradientFill (ring); g.fillEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f);
            const bool on = clipHold > 0.0f;
            juce::ColourGradient lens (on ? juce::Colours::white : juce::Colour (0xffa0483e), c.x - 2.0f, c.y - 2.5f, on ? juce::Colour (0xff8a0e06) : juce::Colour (0xff2a0503), c.x + 7.0f, c.y + 7.0f, true);
            lens.addColour (0.35, on ? col::ledRed.brighter (0.2f) : juce::Colour (0xff5a120c));
            g.setGradientFill (lens); g.fillEllipse (c.x - 7.5f, c.y - 7.5f, 15.0f, 15.0f);
            g.setColour (juce::Colours::white.withAlpha (0.6f)); g.fillEllipse (c.x - 3.5f, c.y - 5.0f, 4.5f, 3.0f);
            label (g, "CLIP", juce::Rectangle<float> (40.0f, 12.0f).withCentre ({ c.x, c.y + 18.0f }), col::ink, type::jack, Weight::NarrowBold, juce::Justification::centred, 0.1f);
        }
        if (lampUp < 1.0f)
        {
            juce::Graphics::ScopedSaveState s (g);
            g.setOpacity (1.0f - lampUp);
            label (g, juce::String::fromUTF8 ("L\xc2\xb7R"), juce::Rectangle<float> (46.0f, 12.0f).withCentre ({ c.x, c.y - 4.0f }), col::ink, type::jack, Weight::NarrowBold, juce::Justification::centred, 0.06f);
            g.setColour (col::ink.withAlpha (0.5f)); g.drawLine (c.x - 15.0f, c.y + 3.0f, c.x + 15.0f, c.y + 3.0f, 0.8f);
            label (g, "VU", juce::Rectangle<float> (46.0f, 12.0f).withCentre ({ c.x, c.y + 10.0f }), col::ink, type::jack, Weight::NarrowBold, juce::Justification::centred, 0.06f);
        }
        {   // the button: a black cap in a chrome ring, its LED amber for L-R, green for VU
            const auto b = buttonArea();
            const auto bc = b.getCentre();
            g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (b.translated (0.0f, 2.5f));
            juce::ColourGradient ring (juce::Colour (0xfff3f4f5), b.getX(), b.getY(), juce::Colour (0xff6f7378), b.getRight(), b.getBottom(), false);
            ring.addColour (0.5, juce::Colour (0xffc9ccd0));
            g.setGradientFill (ring); g.fillEllipse (b);
            const auto cap = b.reduced (5.0f).translated (0.0f, pressed ? 1.0f : 0.0f);
            juce::ColourGradient body (juce::Colour (0xff4a4a4a), cap.getX() + cap.getWidth() * 0.3f, cap.getY(), juce::Colour (0xff050505), cap.getRight(), cap.getBottom(), true);
            g.setGradientFill (body); g.fillEllipse (cap);
            g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawEllipse (cap.reduced (0.5f), 0.8f);
            const auto led = vu ? col::ledGreen : col::ledAmber;
            drawGlow (g, bc, 8.0f, led, 0.9f);
            g.setColour (led.brighter (0.3f)); g.fillEllipse (bc.x - 3.0f, bc.y - 3.0f + (pressed ? 1.0f : 0.0f), 6.0f, 6.0f);
        }
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
    // L-R meter
    bool vu = false, pressed = false;
    float flip = 0.0f, flipVel = 0.0f;               // the drum: 0 = L-R facing the glass, 1 = VU + scope
    float bar[2] { -60.0f, -60.0f }, peakDb[2] { -60.0f, -60.0f }, peakAge[2] { 0.0f, 0.0f }, lastChRms[2] { 0.0f, 0.0f };
    static constexpr float kThresholds[13] = { -20.0f, -17.5f, -15.0f, -12.5f, -10.0f, -8.0f, -6.0f, -4.5f, -3.0f, 0.0f, 1.0f, 2.0f, 4.0f };

    juce::Rectangle<float> window() const { return { 6.0f, 2.0f, 404.0f, 80.0f }; }
    juce::Rectangle<float> vuArea() const { return { 6.0f, 2.0f, 194.0f, 80.0f }; }
    juce::Rectangle<float> scopeArea() const { return { 206.0f, 2.0f, 204.0f, 80.0f }; }
    juce::Rectangle<float> buttonArea() const { return juce::Rectangle<float> (30.0f, 30.0f).withCentre ({ 448.0f, 60.0f }); }
    int hintZone = 0;
    juce::Point<float> lampCentre() const { return { 448.0f, 18.0f }; }

    // The digital L-R meter: two rows of segments (green to -3, red from 0 up), the scale printed between them.
    void drawLr (juce::Graphics& g, juce::Rectangle<float> r) const
    {
        juce::ColourGradient bg (juce::Colour (0xff0b0e12), r.getCentreX(), r.getY() + r.getHeight() * 0.4f, juce::Colour (0xff030405), r.getX(), r.getY(), true);
        g.setGradientFill (bg);
        g.fillRect (r);
        const juce::Colour green (0xffa6ec3c), red (0xffff3a22);
        const float sx = r.getX() + 30.0f, sw = 364.0f, gap = 5.0f, segW = (sw - gap * 12.0f) / 13.0f;
        auto segX = [&] (int i) { return sx + (float) i * (segW + gap); };
        for (int ch = 0; ch < 2; ++ch)
        {
            const float y = r.getY() + (ch == 0 ? 10.0f : 55.0f);
            for (int i = 0; i < 13; ++i)
            {
                const float d = kThresholds[i], next = i + 1 < 13 ? kThresholds[i + 1] : 99.0f;
                const bool on = bar[ch] >= d || (peakDb[ch] >= d && peakDb[ch] < next);
                const bool hot = d >= 0.0f;
                const juce::Rectangle<float> seg (segX (i), y, segW, 15.0f);
                if (on)
                {
                    const auto c = hot ? red : green;
                    g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (seg.expanded (2.5f), 3.0f);
                    g.setColour (c); g.fillRoundedRectangle (seg, 1.5f);
                    juce::ColourGradient hi (juce::Colours::white.withAlpha (0.3f), 0.0f, seg.getY(), juce::Colours::transparentWhite, 0.0f, seg.getCentreY(), false);
                    g.setGradientFill (hi); g.fillRoundedRectangle (seg, 1.5f);
                }
                else
                {
                    g.setColour (hot ? juce::Colour (0xff2c0c08) : juce::Colour (0xff141f0b));
                    g.fillRoundedRectangle (seg, 1.5f);
                }
            }
            label (g, ch == 0 ? "L" : "R", juce::Rectangle<float> (18.0f, 15.0f).withPosition (r.getX() + 6.0f, y), green, 16.0f, Weight::Narrow, juce::Justification::centred, 0.0f);
        }
        label (g, "dB", juce::Rectangle<float> (22.0f, 12.0f).withPosition (r.getX() + 5.0f, r.getY() + 34.0f), green, 12.0f, Weight::Narrow, juce::Justification::centred, 0.0f);
        const std::pair<int, const char*> marks[] = { { 0, "-20" }, { 2, "-15" }, { 4, "-10" }, { 6, "-6" }, { 8, "-3" }, { 9, "0" }, { 11, "+2" }, { 12, "+4" } };
        for (auto [i, t] : marks)
            label (g, t, juce::Rectangle<float> (30.0f, 12.0f).withCentre ({ segX (i) + segW * 0.5f, r.getY() + 40.0f }), i > 9 ? red : green, 13.5f, Weight::Narrow,
                   juce::Justification::centred, 0.0f);
    }

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
        const float vuDb = 20.0f * std::log10 (juce::jmax (lastRms, 1.0e-6f)) + 12.0f;
        // needle: a damped spring (ζ ≈ 0.8, ≈300 ms to the reading)
        constexpr float w = juce::MathConstants<float>::twoPi * 2.1f, zeta = 0.8f;
        needleVel += (w * w * (needleFor (vuDb) - needle) - 2.0f * zeta * w * needleVel) * dt;
        needle = juce::jlimit (-0.03f, 1.08f, needle + needleVel * dt);
        // CLIP: output samples at 0 dBFS
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
        // L-R: RMS of each channel over the fresh samples (0 = -12 dBFS), up at once, down 20 dB/s, peak held a second
        bool lrChanged = false;
        for (int ch = 0; ch < 2; ++ch)
        {
            float s2 = 0.0f;
            for (int i = pos - fresh; i < pos; ++i) { const float x = processor.channelSample (ch, i); s2 += x * x; }
            if (fresh > 0) lastChRms[ch] = std::sqrt (s2 / (float) fresh);
            else if (sinceAudio > 0.15f) lastChRms[ch] = 0.0f;
            const float level = 20.0f * std::log10 (juce::jmax (lastChRms[ch], 1.0e-6f)) + 12.0f;
            const float before = bar[ch], beforePeak = peakDb[ch];
            bar[ch] = juce::jmax (level, bar[ch] - 20.0f * dt);
            if (bar[ch] >= peakDb[ch] || (peakAge[ch] += dt) > 1.0f) { peakDb[ch] = bar[ch]; peakAge[ch] = 0.0f; }
            lrChanged |= std::abs (bar[ch] - before) > 0.05f || std::abs (peakDb[ch] - beforePeak) > 0.05f;
        }
        // the drum: a quick spring to its side with a slight overshoot
        const float target = vu ? 1.0f : 0.0f;
        const bool turning = std::abs (flip - target) > 0.0005f || std::abs (flipVel) > 0.0005f;
        if (turning)
        {
            constexpr float fw = juce::MathConstants<float>::twoPi * 2.4f, fz = 0.72f;
            flipVel += (fw * fw * (target - flip) - 2.0f * fz * fw * flipVel) * dt;
            flip = juce::jlimit (-0.04f, 1.04f, flip + flipVel * dt);
            if (std::abs (flip - target) < 0.0005f && std::abs (flipVel) < 0.01f) { flip = target; flipVel = 0.0f; }
        }
        const bool showsBack = flip >= 0.5f;
        const bool lampChanged = (clipHold > 0.0f) != lampWasOn || clipHold > 0.0f;
        lampWasOn = clipHold > 0.0f;
        if (turning || pressed) repaint();
        else if (showsBack)
        {
            if (lightChanged || std::abs (needle - drawnNeedle) > 0.0008f) repaint (vuArea().toNearestInt().expanded (2));
            if (traceDirty) repaint (scopeArea().toNearestInt());
            if (lampChanged) repaint (juce::Rectangle<float> (40.0f, 40.0f).withCentre (lampCentre()).toNearestInt());
        }
        else if (lrChanged) repaint (window().toNearestInt());
        traceDirty = false;
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
