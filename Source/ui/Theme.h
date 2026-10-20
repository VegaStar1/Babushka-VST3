// Babushka — look of the interface: palette, embedded fonts, textures and shared drawing helpers.
//
// Everything is drawn from code (vector paths, gradients, a little seeded noise), so it stays sharp at any
// window scale. The editor works in a fixed logical coordinate system (kW x kH) that is scaled to the window.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>
#include <cmath>

namespace babushka::ui
{
constexpr int kW = 1500, kH = 1085;      // logical size of the whole instrument (front panel, keyboard, rear strip)

namespace col
{
    inline const juce::Colour chassis   { 0xff0e0e0e };
    inline const juce::Colour panel     { 0xff1d1c1b };      // black module plates
    inline const juce::Colour panelLow  { 0xff151413 };
    inline const juce::Colour outline   { 0xffd8d2c4 };      // thin line round a module
    inline const juce::Colour ink       { 0xffe9e4d6 };      // text on dark plates
    inline const juce::Colour inkDark   { 0xff161514 };      // text on coloured plates
    inline const juce::Colour amber     { 0xffd9a73c };      // ARP / SEQ
    inline const juce::Colour teal      { 0xff6a9aa6 };      // OSCILLATORS and the title strip
    inline const juce::Colour olive     { 0xff88a552 };      // FILTER
    inline const juce::Colour brick     { 0xffb94a3d };      // SPRING REVERB
    inline const juce::Colour cream     { 0xffd9cfb4 };      // title strip
    inline const juce::Colour ledRed    { 0xffff4a3a };
    inline const juce::Colour ledAmber  { 0xffffb540 };
    inline const juce::Colour ledGreen  { 0xff7be07a };
    inline const juce::Colour metalLight{ 0xffe4e6e8 };
    inline const juce::Colour metalDark { 0xff5b5e62 };
}

// ---- fonts (Montserrat, SIL Open Font License 1.1; see docs/licenses) -----------------------------------------
enum class Weight { Regular, SemiBold, Bold };

inline juce::Typeface::Ptr typeface (Weight w)
{
    static const juce::Typeface::Ptr regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratRegular_ttf,  (size_t) BinaryData::MontserratRegular_ttfSize);
    static const juce::Typeface::Ptr semibold = juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, (size_t) BinaryData::MontserratSemiBold_ttfSize);
    static const juce::Typeface::Ptr bold     = juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf,     (size_t) BinaryData::MontserratBold_ttfSize);
    return w == Weight::Regular ? regular : (w == Weight::SemiBold ? semibold : bold);
}

inline juce::Font font (Weight w, float height, float kerning = 0.0f)
{
    return juce::Font (juce::FontOptions (typeface (w)).withHeight (height).withKerningFactor (kerning));
}

// Engraved / printed panel text. Small caps look: uppercase, a little letter spacing.
inline void label (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Colour colour,
                   float height = 9.0f, Weight w = Weight::SemiBold, juce::Justification j = juce::Justification::centred, float kerning = 0.04f)
{
    g.setColour (colour);
    g.setFont (font (w, height, kerning));
    g.drawText (text, area, j, false);
}

// ---- noise (printed-circuit-board-ish grain on plates) ----------------------------------------------------------
inline const juce::Image& noiseTile()
{
    static const juce::Image tile = [] {
        juce::Image im (juce::Image::ARGB, 160, 160, true);
        juce::Random r (20240607);
        juce::Image::BitmapData d (im, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < d.height; ++y)
            for (int x = 0; x < d.width; ++x)
            {
                const float v = r.nextFloat();
                const bool light = v > 0.5f;
                const juce::uint8 a = (juce::uint8) (r.nextInt (26));
                d.setPixelColour (x, y, (light ? juce::Colours::white : juce::Colours::black).withAlpha ((juce::uint8) a));
            }
        return im;
    }();
    return tile;
}

inline void grain (juce::Graphics& g, juce::Rectangle<float> area, float opacity = 0.5f, juce::Rectangle<float>* clipPath = nullptr)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (area.getSmallestIntegerContainer());
    (void) clipPath;
    g.setTiledImageFill (noiseTile(), 0, 0, opacity);
    g.fillRect (area);
}

// ---- module plate: dark or coloured panel with a thin outline just inside the edge ------------------------------
inline void drawPlate (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fill, bool outlined = true)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.expanded (1.5f).translated (0.0f, 1.5f), 3.5f);                    // contact shadow
    juce::ColourGradient grad (fill.brighter (0.06f), r.getX(), r.getY(), fill.darker (0.12f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 2.5f);
    grain (g, r, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (r.reduced (0.5f), 2.5f, 1.0f);                                         // bevel highlight
    if (outlined)
    {
        g.setColour (col::outline.withAlpha (0.78f));
        g.drawRoundedRectangle (r.reduced (4.5f), 2.0f, 1.0f);
    }
}

inline void drawScrew (juce::Graphics& g, juce::Point<float> c, float radius, float angleRadians)
{
    juce::ColourGradient grad (juce::Colour (0xffdcdcdc), c.x - radius * 0.5f, c.y - radius * 0.5f, juce::Colour (0xff6c6c6c), c.x + radius, c.y + radius, true);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - radius - 0.6f, c.y - radius + 0.9f, radius * 2 + 1.2f, radius * 2 + 1.2f);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - radius, c.y - radius, radius * 2, radius * 2);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    juce::Path slot; slot.addRoundedRectangle (-radius * 0.78f, -radius * 0.13f, radius * 1.56f, radius * 0.26f, radius * 0.1f);
    g.fillPath (slot, juce::AffineTransform::rotation (angleRadians).translated (c.x, c.y));
}

// Brushed metal disc: anisotropic highlights drawn as thin radial wedges.
inline void drawBrushedDisc (juce::Graphics& g, juce::Point<float> c, float radius, float lightAngleDeg = 40.0f, float base = 0.60f, float contrast = 0.30f,
                             juce::Colour tint = juce::Colour (0xff9aa0a8))
{
    juce::Random rnd (static_cast<juce::int64> (radius * 100.0f) + 7);
    const int steps = 360;
    for (int i = 0; i < steps; ++i)
    {
        const float a0 = juce::degreesToRadians ((float) i - 0.4f - 90.0f), a1 = juce::degreesToRadians ((float) i + 1.4f - 90.0f);
        const float t = juce::degreesToRadians ((float) i - lightAngleDeg);
        float b = base + contrast * (0.65f * std::cos (2.0f * t) + 0.35f * std::cos (4.0f * t)) + (rnd.nextFloat() - 0.5f) * 0.05f;
        b = juce::jlimit (0.05f, 1.0f, b);
        juce::Path w;
        w.startNewSubPath (c);
        w.lineTo (c.x + radius * std::cos (a0), c.y + radius * std::sin (a0));
        w.lineTo (c.x + radius * std::cos (a1), c.y + radius * std::sin (a1));
        w.closeSubPath();
        g.setColour (tint.withMultipliedBrightness (b * 1.7f).withAlpha (1.0f));
        g.fillPath (w);
    }
}

// Soft coloured glow for lamps and lit buttons.
inline void drawGlow (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour colour, float strength)
{
    if (strength <= 0.001f) return;
    juce::ColourGradient grad (colour.withAlpha (0.55f * strength), c.x, c.y, colour.withAlpha (0.0f), c.x + radius, c.y, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - radius, c.y - radius, radius * 2, radius * 2);
}

// Pixel scale of the current drawing target (window scale x display scale), used to size cached images.
inline float pixelScale (juce::Graphics& g) { return juce::jmax (1.0f, (float) g.getInternalContext().getPhysicalPixelScaleFactor()); }
} // namespace babushka::ui
