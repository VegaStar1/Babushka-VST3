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
        for (int i = 0; i < 96; ++i)      // knurling
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 96.0f;
            const float r0 = 0.40f, r1 = 0.496f;
            g.setColour ((i & 1) ? juce::Colours::white.withAlpha (0.10f) : juce::Colours::black.withAlpha (0.38f));
            g.drawLine (c + r0 * std::cos (a), c + r0 * std::sin (a), c + r1 * std::cos (a), c + r1 * std::sin (a), 0.016f);
        }
        {   // rim light on the skirt
            juce::ColourGradient grad (juce::Colours::white.withAlpha (0.42f), c - 0.34f, c - 0.34f, juce::Colours::white.withAlpha (0.0f), c + 0.1f, c + 0.1f, false);
            g.setGradientFill (grad);
            g.drawEllipse (c - 0.494f, c - 0.494f, 0.988f, 0.988f, 0.012f);
        }
        g.setColour (juce::Colours::black.withAlpha (0.85f));                     // gap between skirt and cap
        g.drawEllipse (c - 0.382f, c - 0.382f, 0.764f, 0.764f, 0.03f);
        drawBrushedDisc (g, { c, c }, 0.37f, 40.0f, 0.72f, 0.26f);
        for (int i = 0; i < 20; ++i)      // lathe-turned rings
        {
            const float r = 0.08f + 0.0145f * (float) i;
            g.setColour ((i & 1) ? juce::Colours::white.withAlpha (0.07f) : juce::Colours::black.withAlpha (0.09f));
            g.drawEllipse (c - r, c - r, 2 * r, 2 * r, 0.004f);
        }
        {   // overall shading of the cap
            juce::ColourGradient grad (juce::Colours::white.withAlpha (0.16f), c - 0.14f, c - 0.16f, juce::Colours::black.withAlpha (0.16f), c + 0.34f, c + 0.38f, true);
            g.setGradientFill (grad);
            g.fillEllipse (c - 0.37f, c - 0.37f, 0.74f, 0.74f);
        }
        {   // bright edge of the cap
            juce::ColourGradient grad (juce::Colours::white.withAlpha (0.75f), c - 0.3f, c - 0.3f, juce::Colours::black.withAlpha (0.5f), c + 0.3f, c + 0.3f, false);
            g.setGradientFill (grad);
            g.drawEllipse (c - 0.37f, c - 0.37f, 0.74f, 0.74f, 0.011f);
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

    // ---- selector knob (OCTAVE, WAVEFORM): a lever over a round glossy base, as on the hardware ------------------
    // The lever's outline comes from a mask (beak pointing up, centred on the cap); in the mask's square the base disc
    // has a radius of 0.45 of the side.
    inline const juce::Image& selectorMask (bool slit)
    {
        static const juce::Image lever = juce::ImageFileFormat::loadFrom (BinaryData::selector_lever_png, BinaryData::selector_lever_pngSize);
        static const juce::Image line = juce::ImageFileFormat::loadFrom (BinaryData::selector_slit_png, BinaryData::selector_slit_pngSize);
        return slit ? line : lever;
    }

    // The mask at `pixels`, its alpha from the mask, filled by `paint` (drawn into the square first).
    inline juce::Image maskedImage (bool slit, int pixels, std::function<void (juce::Graphics&, float)> paint)
    {
        juce::Image out (juce::Image::ARGB, pixels, pixels, true);
        { juce::Graphics g (out); paint (g, (float) pixels); }
        const auto m = selectorMask (slit).rescaled (pixels, pixels, juce::Graphics::highResamplingQuality);
        juce::Image::BitmapData d (out, juce::Image::BitmapData::readWrite);
        const juce::Image::BitmapData md (m, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < pixels; ++y)
            for (int x = 0; x < pixels; ++x)
            {
                const auto c = d.getPixelColour (x, y);
                d.setPixelColour (x, y, c.withAlpha (c.getFloatAlpha() * md.getPixelColour (x, y).getBrightness()));
            }
        return out;
    }

    struct SelectorImages { juce::Image shadow, rim, lever, slit; };
    inline const SelectorImages& selectorImages (int pixels)
    {
        static std::map<int, SelectorImages> cache;
        auto it = cache.find (pixels);
        if (it != cache.end()) return it->second;
        SelectorImages s;
        s.shadow = maskedImage (false, pixels, [] (juce::Graphics& g, float) { g.fillAll (juce::Colours::black); });
        s.rim = maskedImage (false, pixels, [] (juce::Graphics& g, float) { g.fillAll (juce::Colour (0xffc8c8c8)); });
        s.lever = maskedImage (false, pixels, [] (juce::Graphics& g, float n) {
            juce::ColourGradient gloss (juce::Colour (0xff7a7a7a), n * 0.38f, n * 0.28f, juce::Colour (0xff0a0a0a), n * 0.38f + n * 0.6f, n * 0.28f + n * 0.5f, true);
            gloss.addColour (0.38, juce::Colour (0xff2e2e2e));
            g.setGradientFill (gloss); g.fillAll(); });
        s.slit = maskedImage (true, pixels, [] (juce::Graphics& g, float) { g.fillAll (juce::Colour (0xfff6f2e8)); });
        return cache.emplace (pixels, std::move (s)).first->second;
    }

    // Static part of a selector into the 1.3 x 1.3 unit square: shadow and the glossy flared base.
    inline void drawSelectorBase (juce::Graphics& g)
    {
        const float c = 0.65f;
        for (int i = 0; i < 9; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.075f));
            const float r = 0.5f + 0.014f * (float) i;
            g.fillEllipse (c - r + 0.012f, c - r + 0.06f, 2 * r, 2 * r);
        }
        g.setColour (juce::Colours::black);
        g.fillEllipse (c - 0.5f, c - 0.5f, 1.0f, 1.0f);
        drawBrushedDisc (g, { c, c }, 0.49f, 30.0f, 0.10f, 0.12f, juce::Colour (0xff8a8a8a));       // glossy, highlights sweeping round it
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.32f), c - 0.3f, c - 0.36f, juce::Colours::transparentWhite, c + 0.2f, c + 0.2f, false);
        g.setGradientFill (edge);
        g.drawEllipse (c - 0.47f, c - 0.47f, 0.94f, 0.94f, 0.02f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillEllipse (c - 0.25f, c - 0.25f, 0.5f, 0.5f);
    }

    // Small spun-aluminium cap in the middle of a selector (radius as a fraction of the knob diameter).
    inline void drawSelectorCap (juce::Graphics& g, juce::Point<float> c, float radius)
    {
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillEllipse (c.x - radius - 1.0f, c.y - radius, 2 * radius + 2.0f, 2 * radius + 2.0f);
        drawBrushedDisc (g, c, radius, 40.0f, 0.62f, 0.30f);
        juce::ColourGradient grad (juce::Colours::white.withAlpha (0.6f), c.x - radius * 0.7f, c.y - radius * 0.7f, juce::Colours::black.withAlpha (0.45f), c.x + radius * 0.7f, c.y + radius * 0.7f, false);
        g.setGradientFill (grad);
        g.drawEllipse (c.x - radius, c.y - radius, 2 * radius, 2 * radius, 0.9f);
    }

    inline const juce::Image& selectorBaseImage (int pixels)
    {
        static std::map<int, juce::Image> cache;
        auto it = cache.find (pixels);
        if (it != cache.end()) return it->second;
        juce::Image im (juce::Image::ARGB, pixels, pixels, true);
        { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale ((float) pixels / 1.3f)); drawSelectorBase (g); }
        return cache.emplace (pixels, im).first->second;
    }

    inline void drawPointer (juce::Graphics& g, juce::Point<float> c, float diameter, float angleRad, bool wedge)
    {
        const float D = diameter;
        juce::Graphics::ScopedSaveState s (g);
        g.addTransform (juce::AffineTransform::rotation (angleRad, c.x, c.y));
        juce::Path p;
        if (wedge)  { p.startNewSubPath (c.x, c.y - 0.405f * D); p.lineTo (c.x + 0.045f * D, c.y - 0.15f * D); p.lineTo (c.x - 0.045f * D, c.y - 0.15f * D); p.closeSubPath(); }
        else        p.addRoundedRectangle (c.x - 0.026f * D, c.y - 0.41f * D, 0.052f * D, 0.36f * D, 0.02f * D);
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
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId_, float diameter, std::function<juce::String (double)> formatter = nullptr,
          bool selector = false)
        : paramId (paramId_), diameterPx (diameter), isSelector (selector), format (std::move (formatter))
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        setMouseDragSensitivity (230);
        setVelocityModeParameters (0.2, 1, 0.0, true, juce::ModifierKeys::shiftModifier);   // Shift + drag: fine adjustment
        setScrollWheelEnabled (true);
        setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId_, *this);
        if (auto* p = apvts.getParameter (paramId_))
            setDoubleClickReturnValue (true, dynamic_cast<juce::RangedAudioParameter*> (p)->convertFrom0to1 (p->getDefaultValue()));
        name = apvts.getParameter (paramId_)->getName (32);
        setSize ((int) std::ceil (diameter * 1.3f), (int) std::ceil (diameter * 1.3f));
        setWantsKeyboardFocus (false);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    HintSink* hints = nullptr;
    const juce::String paramId;
    // Optional: where the knob points when something else sets the value (< 0: its own value), what its hint says
    // then, and a call when the hand takes the knob (before the drag starts). A knob that has `shown` looks again
    // ten times a second.
    std::function<float()> shown;
    std::function<juce::String()> shownText;
    std::function<void()> onTouch;
    void followShown() { shownTimer.knob = this; shownTimer.startTimerHz (10); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float scale = pixelScale (g);
        const int px = juce::jmax (8, (int) std::ceil (b.getWidth() * scale));
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        const float other = shown ? shown() : -1.0f;
        drawnShown = other;
        const float prop = other >= 0.0f ? other : (float) valueToProportionOfLength (getValue());
        const float angle = juce::degreesToRadians (-135.0f + 270.0f * prop);
        if (isSelector)
        {
            g.drawImage (detail::selectorBaseImage (px), b);
            // the lever: its square is diameter / 0.9 (base disc radius 0.45 of it), turned about the knob's centre
            const float side = diameterPx / 0.9f;
            const int lpx = juce::jmax (8, (int) std::ceil (side * scale));
            const auto& im = detail::selectorImages (lpx);
            const auto c = b.getCentre();
            auto place = [&] (float grow, float dx, float dy) {
                return juce::AffineTransform::scale (side * grow / (float) lpx).translated (c.x - side * grow * 0.5f + dx, c.y - side * grow * 0.5f + dy)
                           .rotated (angle, c.x + dx, c.y + dy); };
            g.setOpacity (0.75f); g.drawImageTransformed (im.shadow, place (1.0f, 2.0f, 4.0f));
            g.setOpacity (1.0f);  g.drawImageTransformed (im.shadow, place (1.06f, 0.0f, 0.0f));
            g.setOpacity (0.8f);  g.drawImageTransformed (im.rim, place (1.0f, -0.7f, -0.9f));
            g.setOpacity (1.0f);  g.drawImageTransformed (im.lever, place (1.0f, 0.0f, 0.0f));
            g.drawImageTransformed (im.slit, place (1.0f, 0.0f, 0.0f));
            detail::drawSelectorCap (g, c, diameterPx * 0.5f * 0.433f);
            return;
        }
        g.drawImage (detail::knobImage (px), b);
        detail::drawPointer (g, b.getCentre(), diameterPx, angle, false);
    }

    juce::String getTextFromValue (double v) override { return format ? format (v) : juce::Slider::getTextFromValue (v); }

    void mouseDown (const juce::MouseEvent& e) override { if (onTouch) onTouch(); juce::Slider::mouseDown (e); hint(); }
    void mouseDrag (const juce::MouseEvent& e) override { juce::Slider::mouseDrag (e); hint(); }
    void mouseUp (const juce::MouseEvent& e) override { juce::Slider::mouseUp (e); if (hints) hints->hideHint(); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { if (onTouch) onTouch(); juce::Slider::mouseWheelMove (e, w); hint(); }
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

    struct ShownTimer : public juce::Timer
    {
        Knob* knob = nullptr;
        void timerCallback() override { if (knob->shown && std::abs (knob->shown() - knob->drawnShown) > 0.0005f) knob->repaint(); }
    } shownTimer;
    float drawnShown = -1.0f;

    void hint()
    {
        if (! hints) return;
        const auto t = shownText ? shownText() : juce::String();
        hints->showHint (*this, name + "   " + (t.isNotEmpty() ? t : getTextFromValue (getValue())));
    }
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
    LeverSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId_, std::vector<int> map, std::function<juce::String (int)> positionName = nullptr)
        : paramId (paramId_), mapping (std::move (map)), nameOf (std::move (positionName)),
          link (apvts, paramId_, [this] (float v) { const int choice = (int) std::lround (v);
                                                     for (size_t i = 0; i < mapping.size(); ++i) if (mapping[i] == choice) position = (int) i;
                                                     repaint(); })
    {
        setSize (58, 44);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        title = link.name();
    }

    HintSink* hints = nullptr;
    const juce::String paramId;

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
        {   // round chrome nut
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillEllipse (c.x - 7.5f, c.y - 6.0f, 15.0f, 15.0f);
            juce::ColourGradient grad (juce::Colour (0xfff2f3f4), c.x - 5.0f, c.y - 6.0f, juce::Colour (0xff5d6166), c.x + 6.0f, c.y + 7.0f, false);
            grad.addColour (0.45, juce::Colour (0xffe5e7e9)); grad.addColour (0.6, juce::Colour (0xff7c8186));
            g.setGradientFill (grad);
            g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f, 0.7f);
        }
        {   // handle: tapered, shaded across its width
            juce::Path h;
            const juce::Point<float> n (std::cos (tilt), std::sin (tilt));
            h.startNewSubPath (c.x - n.x * 3.4f, c.y - n.y * 3.4f);
            h.lineTo (tip.x - n.x * 2.3f, tip.y - n.y * 2.3f);
            h.lineTo (tip.x + n.x * 2.3f, tip.y + n.y * 2.3f);
            h.lineTo (c.x + n.x * 3.4f, c.y + n.y * 3.4f);
            h.closeSubPath();
            // white plastic bat, rounded at the end
            juce::ColourGradient grad (juce::Colour (0xff9e9c96), c.x - n.x * 3.4f, c.y - n.y * 3.4f, juce::Colour (0xff8f8d87), c.x + n.x * 3.4f, c.y + n.y * 3.4f, false);
            grad.addColour (0.38, juce::Colours::white); grad.addColour (0.66, juce::Colour (0xffefede8));
            g.setGradientFill (grad);
            g.fillPath (h);
            g.fillEllipse (tip.x - 2.6f, tip.y - 2.6f, 5.2f, 5.2f);
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.strokePath (h, juce::PathStrokeType (0.6f));
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
// The component is larger than the pad so the glow can spill onto the panel; only the pad itself takes clicks.
class Pad : public juce::Component
{
public:
    Pad (juce::Colour colour, float size) : lampColour (colour), padSize (size)
    {
        setSize ((int) (size * 2.6f), (int) (size * 2.6f));
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    std::function<void()> onPress, onRelease;
    std::function<void()> onHover, onLeave;          // onLeave: the pointer has gone (hides the hint)
    void setLit (bool on) { if (lit != on) { lit = on; repaint(); } }
    bool isLit() const { return lit; }

    bool hitTest (int x, int y) override { return bezel().contains ((float) x, (float) y); }

    // Hard plastic key in a black frame with an LED under it: dim tinted plastic when off; when on, a hot white middle,
    // saturated colour towards the edges, and light that spills over the frame onto the panel.
    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const auto bz = bezel();
        const auto face = bz.reduced (3.5f).translated (0.0f, down ? 1.0f : 0.0f);
        const auto col = lampColour;
        if (lit) drawGlow (g, c, padSize * 1.45f, col, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (bz.translated (0.0f, 2.0f), 6.0f);
        g.setColour (juce::Colour (0xff303030));
        g.fillRoundedRectangle (bz.expanded (1.0f), 6.5f);
        g.setColour (juce::Colour (0xff060606));
        g.fillRoundedRectangle (bz, 6.0f);
        if (lit)
        {
            juce::ColourGradient fg (juce::Colours::white, face.getCentreX(), face.getCentreY(), col.darker (0.25f), face.getX(), face.getY(), true);
            fg.addColour (0.3, col.interpolatedWith (juce::Colours::white, 0.6f));
            fg.addColour (0.68, col);
            g.setGradientFill (fg);
            g.fillRoundedRectangle (face, 4.5f);
        }
        else
        {
            const auto top = col.interpolatedWith (juce::Colour (0xff3a3a3a), 0.45f), bottom = col.interpolatedWith (juce::Colour (0xff050505), 0.70f);
            juce::ColourGradient fg (top, face.getX(), face.getY(), bottom, face.getX(), face.getBottom(), false);
            fg.addColour (0.55, col.interpolatedWith (juce::Colour (0xff121212), 0.58f));
            g.setGradientFill (fg);
            g.fillRoundedRectangle (face, 4.5f);
        }
        // the moulded top face catches the light
        juce::ColourGradient refl (juce::Colours::white.withAlpha (lit ? 0.55f : 0.25f), 0.0f, face.getY() + 1.0f, juce::Colours::transparentWhite, 0.0f, face.getY() + face.getHeight() * 0.45f, false);
        g.setGradientFill (refl);
        g.fillRoundedRectangle (face.reduced (2.5f, 1.5f).withHeight (face.getHeight() * 0.45f), 3.5f);
        g.setColour (juce::Colours::black.withAlpha (lit ? 0.25f : 0.5f));
        g.drawRoundedRectangle (face, 4.5f, 1.0f);
        if (lit)
        {
            // light through the plastic falls over the frame too
            for (int i = 0; i < 3; ++i)
            {
                g.setColour (col.withAlpha (0.42f - 0.12f * (float) i));
                g.drawRoundedRectangle (bz.expanded (0.5f + 1.6f * (float) i), 6.5f + 1.6f * (float) i, 1.8f);
            }
            g.setColour (col.interpolatedWith (juce::Colours::white, 0.5f).withAlpha (0.8f));
            g.drawRoundedRectangle (face.expanded (1.0f), 5.0f, 1.3f);
        }
    }

    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); if (onPress) onPress(); }
    void mouseUp (const juce::MouseEvent&) override { down = false; repaint(); if (onRelease) onRelease(); }
    void mouseEnter (const juce::MouseEvent&) override { if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { if (onLeave) onLeave(); }

private:
    juce::Colour lampColour;
    float padSize = 40.0f;
    bool lit = false, down = false;

    juce::Rectangle<float> bezel() const { return juce::Rectangle<float> (padSize, padSize).withCentre (getLocalBounds().toFloat().getCentre()); }
};

// ---- vertical fader (SUSTAIN) -------------------------------------------------------------------------------------------
class Fader : public juce::Component
{
public:
    Fader (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float height)
        : paramId (id), link (apvts, id, [this] (float v) { value = link.range().convertTo0to1 (v); repaint(); })
    {
        setSize (100, (int) height);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        title = link.name();
    }
    HintSink* hints = nullptr;
    const juce::String paramId;

    juce::Rectangle<float> travel() const { return { 0.0f, 16.0f, (float) getWidth(), (float) getHeight() - 32.0f }; }

    void paint (juce::Graphics& g) override
    {
        const auto t = travel();
        const float cx = (float) getWidth() * 0.5f;
        // a wide ladder of scale lines on both sides, as on the hardware: long every fifth, short between, a gap round the slot
        for (int i = 0; i <= 30; ++i)
        {
            const float y = t.getY() + t.getHeight() * (float) i / 30.0f;
            const bool major = (i % 5) == 0;
            g.setColour (col::ink.withAlpha (major ? 0.9f : 0.7f));
            const float len = major ? 40.0f : 31.0f;
            g.drawLine (cx - 7.0f - len, y, cx - 7.0f, y, major ? 1.4f : 1.1f);
            g.drawLine (cx + 7.0f, y, cx + 7.0f + len, y, major ? 1.4f : 1.1f);
        }
        g.setColour (juce::Colour (0xff030303));                                          // slot
        g.fillRoundedRectangle (cx - 4.5f, t.getY() - 4.0f, 9.0f, t.getHeight() + 8.0f, 4.5f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (cx - 4.5f, t.getY() - 4.0f, 9.0f, t.getHeight() + 8.0f, 4.5f, 0.8f);
        // cap
        const float y = t.getBottom() - value * t.getHeight();
        const juce::Rectangle<float> cap (cx - 33.0f, y - 14.0f, 66.0f, 28.0f);
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
// Seen from above: a ribbed cylinder standing out of a slot. Ribs sit at fixed angles on the cylinder, so they crowd
// together towards the top and bottom edges and roll with the value; the light comes from the top of the panel.
class Wheel : public juce::Component, private juce::Timer
{
public:
    // value01: 0 … 1 (0.5 = centre for the pitch wheel).
    Wheel (bool springBack, float height, juce::Colour plastic) : spring (springBack), tone (plastic)
    {
        setSize (62, (int) height);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        if (springBack) value = 0.5f;
    }
    std::function<void (float)> onChange;           // user moved the wheel
    HintSink* hints = nullptr;
    juce::String title;

    void setValueFromParameter (float v01) { if (! dragging && std::abs (v01 - value) > 1.0e-4f) { value = v01; repaint(); } }
    float getValue() const { return value; }
    bool isBusy() const { return dragging || isTimerRunning(); }      // in the hand, or springing back

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced (3.0f, 1.0f);
        // housing: a recessed slot with a bevelled lip
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillRoundedRectangle (b.translated (0.0f, 1.5f), 8.0f);
        juce::ColourGradient lip (juce::Colour (0xff2c2c2d), b.getX(), b.getY(), juce::Colour (0xff070707), b.getX(), b.getBottom(), false);
        g.setGradientFill (lip);
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);
        const auto slot = b.reduced (5.0f, 6.0f);
        g.setColour (juce::Colour (0xff010101));
        g.fillRoundedRectangle (slot, 5.0f);

        const auto w = slot.reduced (4.0f, 3.0f);
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip; clip.addRoundedRectangle (w, 3.0f);
        g.reduceClipRegion (clip);

        const float R = w.getHeight() * 0.5f / std::sin (kVisible), cy = w.getCentreY();
        // turning the wheel up (a larger value) rolls the tread upwards: angles grow downwards on screen, so negative
        const float turn = -(value - (spring ? 0.5f : 0.0f)) * kTravel;
        auto yAt = [&] (float theta) { return cy + R * std::sin (theta); };
        // tread: lit from the top, falling off as the surface turns away
        const int bands = 48;
        for (int i = 0; i < bands; ++i)
        {
            const float t0 = -kVisible + 2.0f * kVisible * (float) i / (float) bands, t1 = t0 + 2.0f * kVisible / (float) bands;
            const float tm = 0.5f * (t0 + t1);
            const float light = juce::jlimit (0.0f, 1.0f, 0.08f + 0.95f * std::cos (tm + 0.38f) * std::cos (tm));
            g.setColour (tone.withMultipliedBrightness (0.12f + 0.92f * std::pow (light, 1.4f)));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (w.getX(), yAt (t0), w.getRight(), yAt (t1) + 0.6f));
        }
        // ribs moulded into the tread
        const float step = juce::degreesToRadians (5.0f);
        const float first = std::fmod (turn, step) - std::ceil (kVisible / step) * step;
        for (float th = first; th < kVisible; th += step)
        {
            if (th < -kVisible) continue;
            const float c = std::cos (th);
            const float y = yAt (th), hgt = juce::jmax (0.3f, 1.4f * c);
            g.setColour (juce::Colours::black.withAlpha (0.35f * c));
            g.fillRect (w.getX(), y - hgt * 0.5f, w.getWidth(), hgt);
            const bool facingLight = th < 0.25f;
            g.setColour (juce::Colours::white.withAlpha ((facingLight ? 0.22f : 0.08f) * c));
            g.fillRect (w.getX(), y + hgt * 0.5f, w.getWidth(), juce::jmax (0.4f, 1.1f * c));
        }
        // centre mark of the pitch wheel, or the end stop of the mod wheel, moulded as a broad flat rib
        {
            const float th = spring ? turn : turn + kTravel * 0.5f;      // mod wheel at 0: the stop low in the slot
            if (std::abs (th) < kVisible)
            {
                const float c = std::cos (th);
                const float y = yAt (th);
                const auto mark = tone.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff1a1a1a) : juce::Colour (0xfff2ecdc);
                g.setColour (spring ? mark.withAlpha (0.9f * c) : juce::Colours::black.withAlpha (0.6f * c));
                g.fillRect (w.getX() + 3.0f, y - 2.0f * c, w.getWidth() - 6.0f, 4.0f * c);
            }
        }
        // rounded edges of the tread and the shadow of the slot
        juce::ColourGradient sides (juce::Colours::black.withAlpha (0.55f), w.getX(), 0.0f, juce::Colours::black.withAlpha (0.55f), w.getRight(), 0.0f, false);
        sides.addColour (0.2, juce::Colours::transparentBlack); sides.addColour (0.8, juce::Colours::transparentBlack);
        g.setGradientFill (sides); g.fillRect (w);
        juce::ColourGradient top (juce::Colours::black.withAlpha (0.8f), 0.0f, w.getY(), juce::Colours::transparentBlack, 0.0f, w.getY() + 10.0f, false);
        g.setGradientFill (top); g.fillRect (w);
        juce::ColourGradient bottom (juce::Colours::black.withAlpha (0.6f), 0.0f, w.getBottom(), juce::Colours::transparentBlack, 0.0f, w.getBottom() - 8.0f, false);
        g.setGradientFill (bottom); g.fillRect (w);
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
    static constexpr float kVisible = 1.15f;     // half of the arc that shows through the slot (radians)
    static constexpr float kTravel = 1.2f;       // rotation over the full range of the value (radians)
    bool spring, dragging = false;
    juce::Colour tone;
    float value = 0.0f, lastY = 0.0f;

    void setValue (float v)
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (std::abs (v - value) > 1.0e-6f) { value = v; repaint(); if (onChange) onChange (value); }
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
    explicit Lamp (juce::Colour c, float diameter = 7.0f) : colour (c), d (diameter) { setSize ((int) (diameter * 7), (int) (diameter * 7)); setInterceptsMouseClicks (false, false); }
    void setLevel (float v) { v = juce::jlimit (0.0f, 1.0f, v); if (std::abs (v - level) > 0.02f) { level = v; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        drawGlow (g, c, d * 3.2f, colour, level);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - d * 0.5f - 1.0f, c.y - d * 0.5f - 0.5f, d + 2.0f, d + 2.0f);
        juce::ColourGradient grad (colour.withMultipliedBrightness (0.25f + 0.95f * level).brighter (0.7f * level), c.x - d * 0.1f, c.y - d * 0.15f,
                                   colour.withMultipliedBrightness (0.12f + 0.75f * level), c.x + d * 0.5f, c.y + d * 0.5f, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - d * 0.5f, c.y - d * 0.5f, d, d);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillEllipse (c.x - d * 0.25f, c.y - d * 0.36f, d * 0.32f, d * 0.22f);
    }
private:
    juce::Colour colour; float d; float level = 0.0f;
};

// ---- jack socket (static drawing; the cable layer adds hover states and plugs) ---------------------------------------------
// As on the hardware: a hexagon nut with a chamfer, a chrome washer, the dark socket with its contact ring.
inline void drawJack (juce::Graphics& g, juce::Point<float> c, float r = 10.5f)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - r - 0.5f + 1.2f, c.y - r + 2.2f, 2 * r + 1.0f, 2 * r + 1.0f);           // shadow, down and right
    juce::Path nut; nut.addPolygon (c, 6, r * 1.06f, juce::MathConstants<float>::pi / 2.0f);   // flats top and bottom, points left and right
    juce::ColourGradient grad (juce::Colour (0xffe9ebed), c.x - r * 0.6f, c.y - r, juce::Colour (0xff4d5155), c.x + r * 0.6f, c.y + r, false);
    grad.addColour (0.5, juce::Colour (0xff8c9095));
    g.setGradientFill (grad);
    g.fillPath (nut);
    juce::Path face; face.addPolygon (c, 6, r * 0.9f, juce::MathConstants<float>::pi / 2.0f);   // the face inside the chamfer
    juce::ColourGradient fg (juce::Colour (0xfff7f8f9), c.x - r * 0.5f, c.y - r * 0.9f, juce::Colour (0xff80858a), c.x + r * 0.5f, c.y + r * 0.9f, false);
    fg.addColour (0.3, juce::Colour (0xffc4c8cc)); fg.addColour (0.55, juce::Colour (0xffa7abb0)); fg.addColour (0.72, juce::Colour (0xffd9dcdf));
    g.setGradientFill (fg);
    g.fillPath (face);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (nut, juce::PathStrokeType (0.7f));
    const float wr = r * 0.6f;                                                                   // washer
    juce::ColourGradient washer (juce::Colours::white, c.x - wr * 0.6f, c.y - wr * 0.7f, juce::Colour (0xff6f747a), c.x + wr * 0.7f, c.y + wr * 0.8f, false);
    washer.addColour (0.45, juce::Colour (0xffd2d5d9));
    g.setGradientFill (washer);
    g.fillEllipse (c.x - wr, c.y - wr, 2 * wr, 2 * wr);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (c.x - wr, c.y - wr, 2 * wr, 2 * wr, 0.7f);
    const float hr = r * 0.38f;                                                                  // socket
    juce::ColourGradient hole (juce::Colour (0xff000000), c.x, c.y + hr * 0.1f, juce::Colour (0xff5d6165), c.x + hr * 1.05f, c.y, true);
    hole.addColour (0.62, juce::Colour (0xff1a1b1c));
    g.setGradientFill (hole);
    g.fillEllipse (c.x - hr, c.y - hr, 2 * hr, 2 * hr);
    g.setColour (juce::Colours::black);
    g.fillEllipse (c.x - hr * 0.55f, c.y - hr * 0.5f, hr * 1.1f, hr * 1.1f);
}

// Outputs are named on a white plaque (as on the hardware), inputs in plain print.
inline bool isOutputName (const juce::String& t) { return t == "OUT" || t == "OUTPUT" || t.endsWith (" OUT") || t.contains (" OUT ") || t.endsWith ("OUTPUT"); }

inline void drawJackName (juce::Graphics& g, const juce::String& text, juce::Point<float> centre, float maxWidth, juce::Colour ink, float size = type::jack,
                          bool plaque = false)
{
    if (! plaque) { label (g, text, juce::Rectangle<float> (maxWidth, size * 1.5f).withCentre (centre), ink, size, Weight::NarrowBold, juce::Justification::centred, 0.03f); return; }
    const float w = juce::jmin (maxWidth, juce::GlyphArrangement::getStringWidth (font (Weight::NarrowBold, size, 0.03f), text) + 8.0f);
    const auto box = juce::Rectangle<float> (w, size + 3.5f).withCentre (centre);
    g.setColour (juce::Colour (0xfff1ede2));
    g.fillRoundedRectangle (box, 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.drawRoundedRectangle (box, 1.5f, 0.5f);
    label (g, text, box.reduced (3.0f, 0.0f), juce::Colour (0xff151414), size, Weight::NarrowBold, juce::Justification::centred, 0.03f);
}
// ---- plastic key of the dialogs and the bottom strip --------------------------------------------------------------
// A black key in a black frame; `lit`: lit amber from inside (a chosen tag, the main button of a dialog).
inline void drawKey (juce::Graphics& g, juce::Rectangle<float> r, bool lit, bool down, float radius = 4.5f)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.5f), radius + 1.0f);
    g.setColour (juce::Colour (0xff2c2c2c));
    g.fillRoundedRectangle (r.expanded (1.0f), radius + 1.5f);
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillRoundedRectangle (r, radius + 0.5f);
    const auto face = r.reduced (2.5f).translated (0.0f, down ? 1.0f : 0.0f);
    if (lit)
    {
        drawGlow (g, r.getCentre(), juce::jmax (r.getWidth(), r.getHeight()) * 0.75f, col::accent, 0.55f);
        juce::ColourGradient fg (juce::Colour (0xfffff3d6), face.getCentreX(), face.getCentreY() - face.getHeight() * 0.1f, juce::Colour (0xffb46e10),
                                 face.getX(), face.getY(), true);
        fg.addColour (0.45, juce::Colour (0xffffcf6e)); fg.addColour (0.8, juce::Colour (0xfff0a22e));
        g.setGradientFill (fg);
        g.fillRoundedRectangle (face, radius);
        g.setColour (col::accent.withAlpha (0.55f));
        g.drawRoundedRectangle (r.expanded (1.5f), radius + 2.0f, 1.4f);
    }
    else
    {
        juce::ColourGradient fg (juce::Colour (down ? 0xff1c1c1c : 0xff444444), 0.0f, face.getY(), juce::Colour (0xff151515), 0.0f, face.getBottom(), false);
        fg.addColour (0.48, juce::Colour (0xff262626)); fg.addColour (0.55, juce::Colour (0xff151515));
        g.setGradientFill (fg);
        g.fillRoundedRectangle (face, radius);
    }
    g.setColour (juce::Colours::white.withAlpha (lit ? 0.7f : 0.22f));
    g.drawLine (face.getX() + radius, face.getY() + 0.7f, face.getRight() - radius, face.getY() + 0.7f, 0.9f);
}

// A screen of a dialog (a list, a text field, a menu): a chrome bezel round dark glass.
inline void drawScreenBezel (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.expanded (3.0f).translated (0.0f, 2.0f), 6.0f);
    juce::ColourGradient chrome (juce::Colour (0xffd9dbde), 0.0f, r.getY() - 3.0f, juce::Colour (0xffcfd2d5), 0.0f, r.getBottom() + 3.0f, false);
    chrome.addColour (0.22, juce::Colour (0xff6e7277)); chrome.addColour (0.55, juce::Colour (0xff3c3f42)); chrome.addColour (0.85, juce::Colour (0xff8d9196));
    g.setGradientFill (chrome);
    g.fillRoundedRectangle (r.expanded (3.0f), 6.0f);
    g.setColour (juce::Colour (0xff0e0a05));
    g.fillRoundedRectangle (r, 4.0f);
}

} // namespace babushka::ui
