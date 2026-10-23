// Babushka — on-screen keyboard: 32 keys (F to C, as the hardware), velocity from where the key is pressed.
#pragma once

#include "Theme.h"
#include "../PluginProcessor.h"

namespace babushka::ui
{
class KeyboardWidget : public juce::Component, private juce::Timer
{
public:
    explicit KeyboardWidget (BabushkaProcessor& p) : processor (p) { startTimerHz (30); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    ~KeyboardWidget() override { stopTimer(); releaseAll(); }

    static constexpr int kNumKeys = 32;
    int baseNote() const noexcept { return base; }
    void setBaseNote (int note) { releaseAll(); base = juce::jlimit (0, 127 - kNumKeys + 1, note); repaint(); }
    void shiftOctave (int octaves) { setBaseNote (base + 12 * octaves); }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        // key bed
        g.setColour (juce::Colour (0xff050505));
        g.fillRect (r);
        const float ww = r.getWidth() / (float) numWhite();
        // white keys
        int wi = 0;
        for (int n = base; n < base + kNumKeys; ++n)
        {
            if (isBlack (n)) continue;
            const auto key = juce::Rectangle<float> (r.getX() + (float) wi * ww, r.getY(), ww, r.getHeight());
            const bool on = held (n);
            const float press = on ? 3.0f : 0.0f;
            const auto face = key.reduced (1.2f, 0.0f).withTrimmedBottom (4.0f).translated (0.0f, press * 0.5f);
            juce::ColourGradient grad (juce::Colour (0xfff4f0e6), face.getX(), face.getY(), juce::Colour (0xffd5cfbe), face.getX(), face.getBottom(), false);
            grad.addColour (0.82, juce::Colour (0xffe9e4d6));
            g.setGradientFill (on ? juce::ColourGradient (juce::Colour (0xffd9d3c2), face.getX(), face.getY(), juce::Colour (0xffb9b29f), face.getX(), face.getBottom(), false) : grad);
            g.fillRoundedRectangle (face, 2.5f);
            g.setColour (juce::Colours::black.withAlpha (0.28f));                                 // front lip and side shading
            g.fillRect (face.getX(), face.getBottom() - 6.0f + press * 0.3f, face.getWidth(), 6.0f);
            juce::ColourGradient side (juce::Colours::black.withAlpha (0.20f), face.getX(), 0, juce::Colours::black.withAlpha (0.0f), face.getX() + 7.0f, 0, false);
            g.setGradientFill (side); g.fillRect (face.getX(), face.getY(), 7.0f, face.getHeight() - 4.0f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawRoundedRectangle (face, 2.5f, 0.9f);
            if (n % 12 == 0)                                                                       // C: engraved mark
            {
                g.setColour (juce::Colour (0xff8a8574));
                g.setFont (font (Weight::SemiBold, 9.0f));
                g.drawText ("C" + juce::String (n / 12 - 1), face.withTrimmedBottom (10.0f).removeFromBottom (14.0f), juce::Justification::centred, false);
            }
            ++wi;
        }
        // black keys
        wi = 0;
        for (int n = base; n < base + kNumKeys; ++n)
        {
            if (! isBlack (n)) { ++wi; continue; }
            const float cx = r.getX() + (float) wi * ww;                                           // boundary between two white keys
            const auto key = juce::Rectangle<float> (cx - ww * 0.31f, r.getY(), ww * 0.62f, r.getHeight() * 0.62f);
            const bool on = held (n);
            const auto face = key.translated (0.0f, on ? 2.0f : 0.0f);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (face.translated (3.0f, 5.0f), 3.0f);
            // the sides: dark at the edges, a soft sheen down the middle
            juce::ColourGradient grad (juce::Colour (0xff030303), face.getX(), 0.0f, juce::Colour (0xff030303), face.getRight(), 0.0f, false);
            grad.addColour (0.2, juce::Colour (0xff1d1d1d)); grad.addColour (0.5, juce::Colour (0xff262626)); grad.addColour (0.8, juce::Colour (0xff151515));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (face, 3.0f);
            // the top of the key: lighter towards its front edge, then the lit front lip
            const float lip = juce::jmax (8.0f, face.getHeight() * 0.06f);
            const auto top = face.reduced (ww * 0.06f, 0.0f).withTrimmedBottom (lip);
            juce::ColourGradient top2 (juce::Colour (0xff0d0d0d), 0.0f, top.getY(), juce::Colour (on ? 0xff2c2c2c : 0xff3d3d3d), 0.0f, top.getBottom(), false);
            top2.addColour (0.6, juce::Colour (on ? 0xff1f1f1f : 0xff262626)); top2.addColour (0.92, juce::Colour (on ? 0xff2c2c2c : 0xff3d3d3d));
            g.setGradientFill (top2);
            g.fillRoundedRectangle (top, 2.5f);
            g.setColour (juce::Colour (on ? 0xff555555 : 0xff777777));
            g.drawLine (top.getX() + 2.0f, top.getBottom() - 0.5f, top.getRight() - 2.0f, top.getBottom() - 0.5f, 1.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override { pressAt (e.position); }
    void mouseDrag (const juce::MouseEvent& e) override { pressAt (e.position); }
    void mouseUp (const juce::MouseEvent&) override { releaseAll(); }

private:
    BabushkaProcessor& processor;
    int base = 41;                          // F2 on the left; the SHIFT + KB buttons move it by octaves
    int playing = -1;
    uint64_t lastHeld[2] { ~0ull, ~0ull };

    static bool isBlack (int note) noexcept { const int n = note % 12; return n == 1 || n == 3 || n == 6 || n == 8 || n == 10; }
    int numWhite() const noexcept { int c = 0; for (int n = base; n < base + kNumKeys; ++n) if (! isBlack (n)) ++c; return c; }
    bool held (int n) const noexcept { return processor.isNoteHeld (n); }

    int noteAt (juce::Point<float> p) const
    {
        const float ww = (float) getWidth() / (float) numWhite();
        int wi = 0;
        // black keys first (they sit on top)
        for (int n = base; n < base + kNumKeys; ++n)
        {
            if (! isBlack (n)) { ++wi; continue; }
            const float cx = (float) wi * ww;
            if (p.y < (float) getHeight() * 0.62f && std::abs (p.x - cx) < ww * 0.31f) return n;
        }
        wi = 0;
        for (int n = base; n < base + kNumKeys; ++n)
        {
            if (isBlack (n)) continue;
            if (p.x >= (float) wi * ww && p.x < (float) (wi + 1) * ww) return n;
            ++wi;
        }
        return -1;
    }

    void pressAt (juce::Point<float> p)
    {
        const int n = noteAt (p);
        if (n == playing) return;
        if (playing >= 0) processor.uiNoteOff (playing);
        playing = n;
        if (n >= 0)
        {
            const float v = juce::jlimit (0.0f, 1.0f, p.y / (float) getHeight());              // near the front = harder
            processor.uiNoteOn (n, 28 + (int) (99.0f * (0.25f + 0.75f * v)));
        }
    }
    void releaseAll() { if (playing >= 0) { processor.uiNoteOff (playing); playing = -1; } }

    void timerCallback() override
    {
        uint64_t now[2] { 0, 0 };
        for (int i = 0; i < 128; ++i) if (processor.isNoteHeld (i)) now[i >> 6] |= 1ull << (i & 63);
        if (now[0] != lastHeld[0] || now[1] != lastHeld[1]) { lastHeld[0] = now[0]; lastHeld[1] = now[1]; repaint(); }
    }
};
} // namespace babushka::ui
