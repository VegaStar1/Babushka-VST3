// Babushka — panel controls drawn from code: knobs, rotary selectors, lever switches, lit pads, fader, wheels, lamps.
#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <map>

namespace babushka::ui
{
// Called by controls while they are touched, so the editor can show "Cutoff  1.2 kHz" near the pointer.
struct HintSink { virtual ~HintSink() = default; virtual void showHint (juce::Component& from, const juce::String& text) = 0; virtual void hideHint() = 0; };

// ---- parameter binding ------------------------------------------------------------------------------------------
// Thin wrapper over juce::ParameterAttachment working in real (not normalised) units.
class ParamLink
{
public:
    ParamLink (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, std::function<void (float)> onChange)
        : param (*apvts.getParameter (id)), ranged (dynamic_cast<juce::RangedAudioParameter&> (param)),
          attachment (ranged, std::move (onChange), nullptr)
    { attachment.sendInitialUpdate(); }

    void begin() { attachment.beginGesture(); }
    void set (float realValue) { attachment.setValueAsPartOfGesture (realValue); }
    void end() { attachment.endGesture(); }
    void setComplete (float realValue) { attachment.setValueAsCompleteGesture (realValue); }
    float get() const { return ranged.convertFrom0to1 (ranged.getValue()); }
    float getDefault() const { return ranged.convertFrom0to1 (ranged.getDefaultValue()); }
    float proportion() const { return ranged.getValue(); }
    juce::String text (float realValue) const { return ranged.getText (ranged.convertTo0to1 (realValue), 32); }
    juce::String name() const { return ranged.getName (32); }
    const juce::NormalisableRange<float>& range() const { return ranged.getNormalisableRange(); }
    float snapped (float v) const { return ranged.convertFrom0to1 (ranged.convertTo0to1 (v)); }

private:
    juce::AudioProcessorParameter& param;
    juce::RangedAudioParameter& ranged;
    juce::ParameterAttachment attachment;
};

// ---- knob -------------------------------------------------------------------------------------------------------------
namespace detail
{
    // Draws the knob body (shadow, knurled skirt, brushed cap) into a unit square 1.3 x 1.3 with the knob centred.
    inline void drawKnobBody (juce::Graphics& g)
    {
        const float c = 0.65f;
        for (int i = 0; i < 9; ++i)       // soft contact shadow
        {
            g.setColour (juce::Colours::black.withAlpha (0.075f));
            const float r = 0.5f + 0.014f * (float) i;
            g.fillEllipse (c - r + 0.012f, c - r + 0.06f, 2 * r, 2 * r);
        }
        {   // rubber skirt
            juce::ColourGradient grad (juce::Colour (0xff45423f), c - 0.18f, c - 0.22f, juce::Colour (0xff070707), c + 0.42f, c + 0.45f, true);
            g.setGradientFill (grad);
            g.fillEllipse (c - 0.5f, c - 0.5f, 1.0f, 1.0f);
        }
        for (int i = 0; i < 64; ++i)      // knurling
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 64.0f;
            const float r0 = 0.452f, r1 = 0.496f;
            g.setColour ((i & 1) ? juce::Colours::white.withAlpha (0.10f) : juce::Colours::black.withAlpha (0.38f));
            g.drawLine (c + r0 * std::cos (a), c + r0 * std::sin (a), c + r1 * std::cos (a), c + r1 * std::sin (a), 0.016f);
        }
        {   // rim light on the skirt
            juce::ColourGradient grad (juce::Colours::white.withAlpha (0.42f), c - 0.34f, c - 0.34f, juce::Colours::white.withAlpha (0.0f), c + 0.1f, c + 0.1f, false);
            g.setGradientFill (grad);
            g.drawEllipse (c - 0.494f, c - 0.494f, 0.988f, 0.988f, 0.012f);
        }
        g.setColour (juce::Colours::black.withAlpha (0.85f));                     // gap between skirt and cap
        g.drawEllipse (c - 0.428f, c - 0.428f, 0.856f, 0.856f, 0.034f);
        drawBrushedDisc (g, { c, c }, 0.416f, 40.0f, 0.62f, 0.30f);
        for (int i = 0; i < 20; ++i)      // lathe-turned rings
        {
            const float r = 0.10f + 0.0155f * (float) i;
            g.setColour ((i & 1) ? juce::Colours::white.withAlpha (0.07f) : juce::Colours::black.withAlpha (0.09f));
            g.drawEllipse (c - r, c - r, 2 * r, 2 * r, 0.004f);
        }
        {   // overall shading of the cap
            juce::ColourGradient grad (juce::Colours::white.withAlpha (0.16f), c - 0.14f, c - 0.16f, juce::Colours::black.withAlpha (0.30f), c + 0.34f, c + 0.38f, true);
            g.setGradientFill (grad);
            g.fillEllipse (c - 0.416f, c - 0.416f, 0.832f, 0.832f);
        }
        {   // centre dimple
            juce::ColourGradient grad (juce::Colour (0x66101214), c - 0.09f, c - 0.1f, juce::Colour (0x88ffffff), c + 0.1f, c + 0.1f, false);
            g.setGradientFill (grad);
            g.fillEllipse (c - 0.115f, c - 0.115f, 0.23f, 0.23f);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawEllipse (c - 0.115f, c - 0.115f, 0.23f, 0.23f, 0.006f);
        }
        {   // bright edge of the cap
            juce::ColourGradient grad (juce::Colours::white.withAlpha (0.75f), c - 0.3f, c - 0.3f, juce::Colours::black.withAlpha (0.5f), c + 0.3f, c + 0.3f, false);
            g.setGradientFill (grad);
            g.drawEllipse (c - 0.416f, c - 0.416f, 0.832f, 0.832f, 0.011f);
        }
    }

    inline const juce::Image& knobImage (int pixels)
    {
        static std::map<int, juce::Image> cache;
        auto it = cache.find (pixels);
        if (it != cache.end()) return it->second;
        juce::Image im (juce::Image::ARGB, pixels, pixels, true);
        {
            juce::Graphics g (im);
            g.addTransform (juce::AffineTransform::scale ((float) pixels / 1.3f));
            drawKnobBody (g);
        }
        return cache.emplace (pixels, im).first->second;
    }

    inline void drawPointer (juce::Graphics& g, juce::Point<float> c, float diameter, float angleRad, bool wedge)
    {
        const float D = diameter;
        juce::Graphics::ScopedSaveState s (g);
        g.addTransform (juce::AffineTransform::rotation (angleRad, c.x, c.y));
        juce::Path p;
        if (wedge)  { p.startNewSubPath (c.x, c.y - 0.405f * D); p.lineTo (c.x + 0.045f * D, c.y - 0.15f * D); p.lineTo (c.x - 0.045f * D, c.y - 0.15f * D); p.closeSubPath(); }
        else        p.addRoundedRectangle (c.x - 0.026f * D, c.y - 0.40f * D, 0.052f * D, 0.27f * D, 0.02f * D);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillPath (p, juce::AffineTransform::translation (0.012f * D, 0.016f * D));
        g.setColour (juce::Colour (0xfff3eddc));
        g.fillPath (p);
        g.setColour (juce::Colour (0xfff3eddc));                                                          // mark on the skirt
        g.fillRoundedRectangle (c.x - 0.03f * D, c.y - 0.497f * D, 0.06f * D, 0.075f * D, 0.012f * D);
    }
}

class Knob : public juce::Slider
{
public:
    // diameter: size of the knob body in logical pixels. selector: discrete positions drawn with a wedge pointer.
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, float diameter, std::function<juce::String (double)> formatter = nullptr,
          bool selector = false)
        : diameterPx (diameter), isSelector (selector), format (std::move (formatter))
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        setMouseDragSensitivity (230);
        setScrollWheelEnabled (true);
        setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, *this);
        if (auto* p = apvts.getParameter (paramId))
            setDoubleClickReturnValue (true, dynamic_cast<juce::RangedAudioParameter*> (p)->convertFrom0to1 (p->getDefaultValue()));
        name = apvts.getParameter (paramId)->getName (32);
        setSize ((int) std::ceil (diameter * 1.3f), (int) std::ceil (diameter * 1.3f));
        setWantsKeyboardFocus (false);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    HintSink* hints = nullptr;

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float scale = pixelScale (g);
        const int px = juce::jmax (8, (int) std::ceil (b.getWidth() * scale));
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (detail::knobImage (px), b);
        const float prop = (float) valueToProportionOfLength (getValue());
        const float angle = juce::degreesToRadians (-135.0f + 270.0f * prop);
        detail::drawPointer (g, b.getCentre(), diameterPx, angle, isSelector);
    }

    juce::String getTextFromValue (double v) override { return format ? format (v) : juce::Slider::getTextFromValue (v); }

    void mouseDown (const juce::MouseEvent& e) override { juce::Slider::mouseDown (e); hint(); }
    void mouseDrag (const juce::MouseEvent& e) override { juce::Slider::mouseDrag (e); hint(); }
    void mouseUp (const juce::MouseEvent& e) override { juce::Slider::mouseUp (e); if (hints) hints->hideHint(); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { juce::Slider::mouseWheelMove (e, w); hint(); }
    void mouseDoubleClick (const juce::MouseEvent& e) override { juce::Slider::mouseDoubleClick (e); hint(); }
    void mouseEnter (const juce::MouseEvent&) override { hint(); }
    void mouseExit (const juce::MouseEvent&) override { if (hints) hints->hideHint(); }

    float diameter() const { return diameterPx; }

private:
    float diameterPx;
    bool isSelector;
    std::function<juce::String (double)> format;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String name;

    void hint() { if (hints) hints->showHint (*this, name + "   " + getTextFromValue (getValue())); }
};

// Tick marks around a knob, drawn into the panel background.
inline void drawKnobTicks (juce::Graphics& g, juce::Point<float> c, float knobDiameter, int count, juce::Colour colour, float length = 0.1f,
                           float startDeg = -135.0f, float sweepDeg = 270.0f, int majorEvery = 0)
{
    for (int i = 0; i < count; ++i)
    {
        const float a = juce::degreesToRadians (startDeg + sweepDeg * (float) i / (float) juce::jmax (1, count - 1) - 90.0f);
        const bool major = majorEvery > 0 && (i % majorEvery == 0);
        const float r0 = knobDiameter * 0.54f, r1 = knobDiameter * (0.54f + (major ? length * 1.5f : length));
        g.setColour (colour);
        g.drawLine (c.x + r0 * std::cos (a), c.y + r0 * std::sin (a), c.x + r1 * std::cos (a), c.y + r1 * std::sin (a), major ? 1.5f : 1.0f);
    }
}

// ---- three-way lever switch ---------------------------------------------------------------------------------------
class LeverSwitch : public juce::Component
{
public:
    // positions: panel order left→right; map[i] is the parameter's choice index for panel position i.
    LeverSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, std::vector<int> map, std::function<juce::String (int)> positionName = nullptr)
        : mapping (std::move (map)), nameOf (std::move (positionName)),
          link (apvts, paramId, [this] (float v) { const int choice = (int) std::lround (v);
                                                     for (size_t i = 0; i < mapping.size(); ++i) if (mapping[i] == choice) position = (int) i;
                                                     repaint(); })
    {
        setSize (58, 44);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        title = link.name();
    }

    HintSink* hints = nullptr;

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const auto c = juce::Point<float> (b.getCentreX(), b.getCentreY() + 5.0f);
        // plate with a rectangular slot
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (b.reduced (4.0f, 8.0f).translated (0.0f, 1.5f), 5.0f);
        juce::ColourGradient plate (juce::Colour (0xff3a3a3a), b.getX(), b.getY(), juce::Colour (0xff101010), b.getX(), b.getBottom(), false);
        g.setGradientFill (plate);
        g.fillRoundedRectangle (b.reduced (4.0f, 8.0f), 5.0f);
        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (b.reduced (8.0f, 12.0f), 3.5f);
        // lever: base nut and tilted bat handle
        const float tilt = juce::degreesToRadians ((position - 1) * 34.0f);
        const float len = 22.0f;
        const juce::Point<float> tip (c.x + std::sin (tilt) * len, c.y - std::cos (tilt) * len);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawLine (c.x + 2.0f, c.y + 2.0f, tip.x + 3.0f, tip.y + 3.5f, 5.5f);
        {   // hex nut
            juce::Path nut; nut.addPolygon (c, 6, 9.5f, juce::MathConstants<float>::pi / 6.0f);
            juce::ColourGradient grad (col::metalLight, c.x - 6.0f, c.y - 6.0f, col::metalDark, c.x + 8.0f, c.y + 9.0f, false);
            g.setGradientFill (grad);
            g.fillPath (nut);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.strokePath (nut, juce::PathStrokeType (0.8f));
        }
        {   // handle: tapered, shaded across its width
            juce::Path h;
            const juce::Point<float> n (std::cos (tilt), std::sin (tilt));
            h.startNewSubPath (c.x - n.x * 3.4f, c.y - n.y * 3.4f);
            h.lineTo (tip.x - n.x * 2.3f, tip.y - n.y * 2.3f);
            h.lineTo (tip.x + n.x * 2.3f, tip.y + n.y * 2.3f);
            h.lineTo (c.x + n.x * 3.4f, c.y + n.y * 3.4f);
            h.closeSubPath();
            juce::ColourGradient grad (juce::Colour (0xfff2f3f4), c.x - n.x * 3.4f, c.y - n.y * 3.4f, juce::Colour (0xff62666b), c.x + n.x * 3.4f, c.y + n.y * 3.4f, false);
            grad.addColour (0.45, juce::Colour (0xffcfd2d5));
            g.setGradientFill (grad);
            g.fillPath (h);
            juce::ColourGradient ball (juce::Colours::white, tip.x - 1.6f, tip.y - 1.8f, juce::Colour (0xff6b6f74), tip.x + 2.5f, tip.y + 3.0f, true);
            g.setGradientFill (ball);
            g.fillEllipse (tip.x - 3.6f, tip.y - 3.6f, 7.2f, 7.2f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override { link.begin(); choose (e); }
    void mouseDrag (const juce::MouseEvent& e) override { choose (e); }
    void mouseUp (const juce::MouseEvent&) override { link.end(); if (hints) hints->hideHint(); }
    void mouseEnter (const juce::MouseEvent&) override { hint(); }
    void mouseExit (const juce::MouseEvent&) override { if (hints) hints->hideHint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const int next = juce::jlimit (0, (int) mapping.size() - 1, position + (w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0)));
        if (next != position) { link.setComplete ((float) mapping[(size_t) next]); hint(); }
    }

private:
    std::vector<int> mapping;
    std::function<juce::String (int)> nameOf;
    ParamLink link;
    int position = 0;
    juce::String title;

    void choose (const juce::MouseEvent& e)
    {
        const int n = (int) mapping.size();
        const int p = juce::jlimit (0, n - 1, (int) std::floor ((float) e.x / (float) getWidth() * (float) n));
        if (p != position) { position = p; link.set ((float) mapping[(size_t) p]); repaint(); }
        hint();
    }
    void hint() { if (hints) hints->showHint (*this, title + "   " + (nameOf ? nameOf (position) : link.text ((float) mapping[(size_t) position]))); }
};

// ---- lit pad (SYNC, PLAY / HOLD / TAP) ----------------------------------------------------------------------------------
class Pad : public juce::Component
{
public:
    Pad (juce::Colour colour, float size) : lampColour (colour)
    {
        setSize ((int) (size + 20.0f), (int) (size + 20.0f));
        padSize = size;
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    std::function<void()> onPress, onRelease;
    std::function<void()> onHover;
    void setLit (bool on) { if (lit != on) { lit = on; repaint(); } }
    bool isLit() const { return lit; }

    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const auto face = juce::Rectangle<float> (padSize - 8.0f, padSize - 8.0f).withCentre (c).translated (0.0f, down ? 1.0f : 0.0f);
        const auto bezel = juce::Rectangle<float> (padSize, padSize).withCentre (c);
        drawGlow (g, c, padSize * 0.95f, lampColour, lit ? 1.0f : 0.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (bezel.translated (0.0f, 1.6f), 6.5f);
        juce::ColourGradient bz (juce::Colour (0xff3d3b39), bezel.getX(), bezel.getY(), juce::Colour (0xff0b0b0b), bezel.getX(), bezel.getBottom(), false);
        g.setGradientFill (bz);
        g.fillRoundedRectangle (bezel, 6.5f);
        const auto base = lit ? lampColour : lampColour.withMultipliedBrightness (0.30f).withMultipliedSaturation (0.75f);
        juce::ColourGradient fg (base.brighter (lit ? 0.45f : 0.12f), face.getX(), face.getY(), base.darker (0.35f), face.getX(), face.getBottom(), false);
        g.setGradientFill (fg);
        g.fillRoundedRectangle (face, 4.5f);
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.38f : 0.10f));                           // soft top reflection
        g.fillRoundedRectangle (face.reduced (3.0f, 2.0f).withHeight (face.getHeight() * 0.38f), 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (face, 4.5f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); if (onPress) onPress(); }
    void mouseUp (const juce::MouseEvent&) override { down = false; repaint(); if (onRelease) onRelease(); }
    void mouseEnter (const juce::MouseEvent&) override { if (onHover) onHover(); }

private:
    juce::Colour lampColour;
    float padSize = 40.0f;
    bool lit = false, down = false;
};

// ---- vertical fader (SUSTAIN) -------------------------------------------------------------------------------------------
class Fader : public juce::Component
{
public:
    Fader (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float height)
        : link (apvts, id, [this] (float v) { value = link.range().convertTo0to1 (v); repaint(); })
    {
        setSize (70, (int) height);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        title = link.name();
    }
    HintSink* hints = nullptr;

    juce::Rectangle<float> travel() const { return { 0.0f, 16.0f, (float) getWidth(), (float) getHeight() - 32.0f }; }

    void paint (juce::Graphics& g) override
    {
        const auto t = travel();
        const float cx = (float) getWidth() * 0.5f;
        // ladder of scale lines on both sides, as on the hardware
        for (int i = 0; i <= 40; ++i)
        {
            const float y = t.getY() + t.getHeight() * (float) i / 40.0f;
            const bool major = (i % 4) == 0;
            g.setColour (col::ink.withAlpha (major ? 0.85f : 0.45f));
            const float len = major ? 17.0f : 11.0f;
            g.drawLine (cx - 5.0f - len, y, cx - 5.0f, y, major ? 1.4f : 0.9f);
            g.drawLine (cx + 5.0f, y, cx + 5.0f + len, y, major ? 1.4f : 0.9f);
        }
        g.setColour (juce::Colour (0xff030303));                                          // slot
        g.fillRoundedRectangle (cx - 4.0f, t.getY() - 4.0f, 8.0f, t.getHeight() + 8.0f, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (cx - 4.0f, t.getY() - 4.0f, 8.0f, t.getHeight() + 8.0f, 4.0f, 0.8f);
        // cap
        const float y = t.getBottom() - value * t.getHeight();
        const juce::Rectangle<float> cap (cx - 25.0f, y - 12.0f, 50.0f, 24.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (cap.translated (1.5f, 3.0f), 3.0f);
        juce::ColourGradient grad (juce::Colour (0xff4a4a4c), cap.getX(), cap.getY(), juce::Colour (0xff0d0d0e), cap.getX(), cap.getBottom(), false);
        grad.addColour (0.5, juce::Colour (0xff262628));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (cap, 3.0f);
        for (int i = 0; i < 7; ++i)                                                          // ribs
        {
            const float ry = cap.getY() + 4.0f + (float) i * 2.7f;
            g.setColour ((i & 1) ? juce::Colours::white.withAlpha (0.06f) : juce::Colours::black.withAlpha (0.3f));
            g.drawLine (cap.getX() + 3.0f, ry, cap.getRight() - 3.0f, ry, 1.0f);
        }
        g.setColour (juce::Colour (0xfff1ecdd));                                              // index line
        g.fillRect (cap.getX() + 3.0f, cap.getCentreY() - 1.0f, cap.getWidth() - 6.0f, 2.2f);
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.drawRoundedRectangle (cap.reduced (0.5f), 3.0f, 0.8f);
    }

    void mouseDown (const juce::MouseEvent& e) override { link.begin(); lastY = e.position.y; dragStartValue = value; jumpTo (e); }
    void mouseDrag (const juce::MouseEvent& e) override { jumpTo (e); }
    void mouseUp (const juce::MouseEvent&) override { link.end(); if (hints) hints->hideHint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { link.setComplete (link.getDefault()); }
    void mouseEnter (const juce::MouseEvent&) override { hint(); }
    void mouseExit (const juce::MouseEvent&) override { if (hints) hints->hideHint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        link.setComplete (link.range().convertFrom0to1 (juce::jlimit (0.0f, 1.0f, value + w.deltaY * 0.15f))); hint();
    }

private:
    ParamLink link;
    float value = 0.0f, lastY = 0.0f, dragStartValue = 0.0f;
    juce::String title;
    void jumpTo (const juce::MouseEvent& e)
    {
        const auto t = travel();
        const float v = juce::jlimit (0.0f, 1.0f, (t.getBottom() - e.position.y) / t.getHeight());
        link.set (link.range().convertFrom0to1 (v));
        hint();
    }
    void hint() { if (hints) hints->showHint (*this, title + "   " + juce::String (value, 2)); }
};

// ---- wheel (PITCH springs back, MOD stays) ------------------------------------------------------------------------------
class Wheel : public juce::Component, private juce::Timer
{
public:
    // value01: 0 … 1 (0.5 = centre for the pitch wheel).
    Wheel (bool springBack, float height) : spring (springBack)
    {
        setSize (62, (int) height);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        if (springBack) value = 0.5f;
    }
    std::function<void (float)> onChange;           // user moved the wheel
    HintSink* hints = nullptr;
    juce::String title;

    void setValueFromParameter (float v01) { if (! dragging) { value = v01; repaint(); } }
    float getValue() const { return value; }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced (6.0f, 2.0f);
        g.setColour (juce::Colour (0xff020202));                                    // slot in the plate
        g.fillRoundedRectangle (b, 7.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
        const auto w = b.reduced (6.0f, 6.0f);
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip; clip.addRoundedRectangle (w, 5.0f);
        g.reduceClipRegion (clip);
        // cylinder shading across the width
        juce::ColourGradient cyl (juce::Colour (0xff1b1b1c), w.getX(), 0, juce::Colour (0xff1b1b1c), w.getRight(), 0, false);
        cyl.addColour (0.18, juce::Colour (0xff77797c));
        cyl.addColour (0.42, juce::Colour (0xffd9dbdd));
        cyl.addColour (0.62, juce::Colour (0xff8c8e91));
        cyl.addColour (0.88, juce::Colour (0xff333435));
        g.setGradientFill (cyl);
        g.fillRect (w);
        // ridges that roll with the value
        const float pitch = 7.0f;
        const float offset = std::fmod (value * 520.0f, pitch);
        for (float y = w.getY() - pitch + offset; y < w.getBottom() + pitch; y += pitch)
        {
            g.setColour (juce::Colours::black.withAlpha (0.38f));
            g.fillRect (w.getX(), y, w.getWidth(), 2.2f);
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.fillRect (w.getX(), y + 2.2f, w.getWidth(), 1.0f);
        }
        // light falls off at the top and bottom as the wheel turns away
        juce::ColourGradient fall (juce::Colours::black.withAlpha (0.78f), 0, w.getY(), juce::Colours::black.withAlpha (0.0f), 0, w.getY() + w.getHeight() * 0.28f, false);
        g.setGradientFill (fall); g.fillRect (w);
        juce::ColourGradient fall2 (juce::Colours::black.withAlpha (0.78f), 0, w.getBottom(), juce::Colours::black.withAlpha (0.0f), 0, w.getBottom() - w.getHeight() * 0.28f, false);
        g.setGradientFill (fall2); g.fillRect (w);
        if (spring)                                                                   // centre notch of the pitch wheel
        {
            g.setColour (juce::Colour (0xfff0eadb));
            g.fillRect (w.getX() + 6.0f, w.getCentreY() - 1.2f - (value - 0.5f) * 60.0f, w.getWidth() - 12.0f, 2.4f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override { dragging = true; stopTimer(); lastY = e.position.y; hint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float dy = lastY - e.position.y; lastY = e.position.y;
        setValue (value + dy / ((float) getHeight() * 0.62f));
        hint();
    }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; if (hints) hints->hideHint(); if (spring) startTimerHz (60); }
    void mouseDoubleClick (const juce::MouseEvent&) override { setValue (spring ? 0.5f : 0.0f); }
    void mouseEnter (const juce::MouseEvent&) override { hint(); }
    void mouseExit (const juce::MouseEvent&) override { if (hints) hints->hideHint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { setValue (value + w.deltaY * 0.12f); hint(); }

private:
    bool spring, dragging = false;
    float value = 0.0f, lastY = 0.0f;

    void setValue (float v)
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (v != value) { value = v; repaint(); if (onChange) onChange (value); }
    }
    void timerCallback() override
    {
        value += (0.5f - value) * 0.35f;                                              // spring
        if (std::abs (value - 0.5f) < 0.002f) { value = 0.5f; stopTimer(); }
        repaint();
        if (onChange) onChange (value);
    }
    void hint() { if (hints) hints->showHint (*this, title + "   " + (spring ? juce::String ((value - 0.5f) * 2.0f, 2) : juce::String (value, 2))); }
};

// ---- lamp -----------------------------------------------------------------------------------------------------------------
class Lamp : public juce::Component
{
public:
    explicit Lamp (juce::Colour c, float diameter = 7.0f) : colour (c), d (diameter) { setSize ((int) (diameter * 4), (int) (diameter * 4)); setInterceptsMouseClicks (false, false); }
    void setLevel (float v) { v = juce::jlimit (0.0f, 1.0f, v); if (std::abs (v - level) > 0.02f) { level = v; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        drawGlow (g, c, d * 1.9f, colour, level);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - d * 0.5f - 1.0f, c.y - d * 0.5f - 0.5f, d + 2.0f, d + 2.0f);
        juce::ColourGradient grad (colour.withMultipliedBrightness (0.25f + 0.95f * level).brighter (0.2f * level), c.x - d * 0.2f, c.y - d * 0.25f,
                                   colour.withMultipliedBrightness (0.12f + 0.55f * level), c.x + d * 0.4f, c.y + d * 0.5f, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - d * 0.5f, c.y - d * 0.5f, d, d);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillEllipse (c.x - d * 0.25f, c.y - d * 0.36f, d * 0.32f, d * 0.22f);
    }
private:
    juce::Colour colour; float d; float level = 0.0f;
};

// ---- jack socket (static drawing; the cable layer adds hover states and plugs) ---------------------------------------------
inline void drawJack (juce::Graphics& g, juce::Point<float> c, float r = 10.5f)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - r - 0.5f, c.y - r + 1.2f, 2 * r + 1.0f, 2 * r + 1.0f);
    juce::Path nut; nut.addPolygon (c, 6, r, juce::MathConstants<float>::pi / 6.0f);
    juce::ColourGradient grad (juce::Colour (0xfff0f1f2), c.x - r * 0.7f, c.y - r * 0.8f, juce::Colour (0xff595c61), c.x + r * 0.9f, c.y + r, false);
    grad.addColour (0.5, juce::Colour (0xffb9bcc0));
    g.setGradientFill (grad);
    g.fillPath (nut);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (nut, juce::PathStrokeType (0.8f));
    const float rr = r * 0.66f;                                                                  // collar
    juce::ColourGradient collar (juce::Colour (0xff2a2b2d), c.x - rr, c.y - rr, juce::Colour (0xffe4e6e8), c.x + rr, c.y + rr, false);
    g.setGradientFill (collar);
    g.fillEllipse (c.x - rr, c.y - rr, 2 * rr, 2 * rr);
    const float hr = r * 0.47f;                                                                  // hole
    juce::ColourGradient hole (juce::Colour (0xff000000), c.x - hr * 0.3f, c.y - hr * 0.5f, juce::Colour (0xff1a1a1c), c.x + hr, c.y + hr, true);
    g.setGradientFill (hole);
    g.fillEllipse (c.x - hr, c.y - hr, 2 * hr, 2 * hr);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawEllipse (c.x - hr, c.y - hr, 2 * hr, 2 * hr, 0.8f);
}
} // namespace babushka::ui
