// Babushka — look of the interface: palette, embedded fonts, textures and shared drawing helpers.
//
// Everything is drawn from code (vector paths, gradients, a little seeded noise), so it stays sharp at any
// window scale. The editor works in a fixed logical coordinate system (kW x kH) that is scaled to the window.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>
#include <cmath>
#include <map>
#include <vector>

namespace babushka::ui
{
constexpr int kW = 1500, kH = 1085;      // logical size of the whole instrument (front panel, keyboard, rear strip)
// The part of it the window shows: the chassis and its shadow, without the empty margin round it.
constexpr int kViewX = 25, kViewY = 22, kViewW = 1450, kViewH = 1052;

namespace col
{
    inline const juce::Colour chassis   { 0xff141414 };
    inline const juce::Colour panel     { 0xff19191a };      // black module plates
    inline const juce::Colour panelLow  { 0xff151413 };
    inline const juce::Colour outline   { 0xffd8d2c4 };      // thin line round a module
    inline const juce::Colour ink       { 0xffe9e4d6 };      // text on dark plates
    inline const juce::Colour inkDark   { 0xff161514 };      // text on coloured plates
    inline const juce::Colour arp       { 0xfff28c28 };      // ARP / SEQ: tangerine
    inline const juce::Colour osc       { 0xff23b5b0 };      // OSCILLATORS: turquoise
    inline const juce::Colour filter    { 0xffd2407e };      // FILTER: berry
    inline const juce::Colour reverb    { 0xff6a4fc4 };      // SPRING REVERB: violet
    inline const juce::Colour strip     { 0xff161617 };      // title strip: black
    inline const juce::Colour cream     { 0xffd9cfb4 };
    inline const juce::Colour accent    { 0xffffbe52 };      // amber of the displays and of what is selected in the menus
    inline const juce::Colour ledRed    { 0xffff4a3a };
    inline const juce::Colour ledAmber  { 0xffffb540 };
    inline const juce::Colour ledGreen  { 0xff7be07a };
    inline const juce::Colour metalLight{ 0xffe4e6e8 };
    inline const juce::Colour metalDark { 0xff5b5e62 };
}

// ---- skins --------------------------------------------------------------------------------------------------------
// The layout never changes; a skin only recolours plates, printed labels, lit pads and wheels.
struct Skin
{
    const char* name;
    juce::Colour chassis;
    juce::Colour panel;                                      // dark module plates
    juce::Colour arp, osc, filter, reverb, rearCv;           // plates that are coloured on the Babushka skin
    juce::Colour pillOsc, pillFilter, pillWheel, pillArp;    // filled label boxes; transparent = outlined box
    juce::Colour strip;                                      // title strip
    juce::Colour sync, play, hold, tap;                      // lit pads
    juce::Colour wheel;                                      // plastic of the pitch / mod wheels
};

inline const std::vector<Skin>& skins()
{
    // Babushka: tangerine ARP/SEQ, turquoise OSCILLATORS, berry FILTER, violet SPRING REVERB, black title strip.
    // Pastel: the same plates in peach, lavender, mint and coral. Hard plastic keys light in their own colour.
    static const std::vector<Skin> all {
        { "Babushka", juce::Colour (0xff0e0e0e), col::panel, col::arp, col::osc, col::filter, col::reverb, col::arp,
          col::osc, col::filter, col::osc, col::arp, col::strip,
          juce::Colour (0xfff25a2a), juce::Colour (0xff2fd0c8), juce::Colour (0xffe8508e), juce::Colour (0xfff5a030), juce::Colour (0xffece8de) },
        { "Pastel", juce::Colour (0xff0e0e0e), col::panel, juce::Colour (0xfff6b38a), juce::Colour (0xffa79ceb), juce::Colour (0xff78d6b0),
          juce::Colour (0xfff2707a), juce::Colour (0xfff6b38a),
          juce::Colour (0xffa79ceb), juce::Colour (0xff78d6b0), juce::Colour (0xffa79ceb), juce::Colour (0xfff6b38a), col::strip,
          juce::Colour (0xfff2707a), juce::Colour (0xff8fe3c0), juce::Colour (0xffb9aef5), juce::Colour (0xfff8c39e), juce::Colour (0xffece8de) },
    };
    return all;
}
inline int& skinIndex() { static int index = 0; return index; }
inline const Skin& skin() { return skins()[(size_t) juce::jlimit (0, (int) skins().size() - 1, skinIndex())]; }

// Print colour that reads on a plate: dark ink on light plates, cream ink on dark ones.
// The panel's type scale: every printed name on every plate uses one of these four sizes.
namespace type
{
    constexpr float title   = 13.0f;   // plate titles, pedal names, plate buttons
    constexpr float control = 11.5f;   // names of knobs, switches, pads and the pills under knobs
    constexpr float jack    = 10.0f;   // names of jacks, and the small print by lamps and sockets
    constexpr float mark    = 9.5f;    // scales and positions round knobs and levers, the pads' key names
}

inline juce::Colour inkOn (juce::Colour plate) { return plate.getPerceivedBrightness() > 0.5f ? col::inkDark : col::ink; }

// ---- fonts (SIL Open Font License 1.1; see docs/licenses) ----------------------------------------------------------
// Montserrat for the logotype, display and dialogs; Barlow Semi Condensed for the small panel print, which needs
// height more than width.
enum class Weight { Regular, SemiBold, Bold, Narrow, NarrowSemi, NarrowBold };

inline juce::Typeface::Ptr typeface (Weight w)
{
    auto make = [] (const char* data, int size) { return juce::Typeface::createSystemTypefaceFor (data, (size_t) size); };
    static const juce::Typeface::Ptr faces[] = {
        make (BinaryData::MontserratRegular_ttf, BinaryData::MontserratRegular_ttfSize),
        make (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize),
        make (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize),
        make (BinaryData::BarlowSemiCondensedMedium_ttf, BinaryData::BarlowSemiCondensedMedium_ttfSize),
        make (BinaryData::BarlowSemiCondensedSemiBold_ttf, BinaryData::BarlowSemiCondensedSemiBold_ttfSize),
        make (BinaryData::BarlowSemiCondensedBold_ttf, BinaryData::BarlowSemiCondensedBold_ttfSize) };
    return faces[(int) w];
}

// Montserrat sizes are line heights; Barlow sizes are em (point) heights, because Barlow's line is tall for its
// capitals and the panel print is sized by its capitals.
inline juce::Font font (Weight w, float height, float kerning = 0.0f)
{
    const auto o = juce::FontOptions (typeface (w)).withKerningFactor (kerning);
    return juce::Font ((int) w >= (int) Weight::Narrow ? o.withPointHeight (height) : o.withHeight (height));
}

// Printed text. It is squeezed sideways rather than cut when the box is narrow. In a small window all of the panel's
// print (type scale sizes, up to the plate titles) grows by the same factor, up to 25 %, so it stays legible and every
// name keeps its place in the scale: a jack name is the same size as every other jack name, a control name as every
// other control name.
inline void label (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Colour colour,
                   float height = 9.0f, Weight w = Weight::SemiBold, juce::Justification j = juce::Justification::centred, float kerning = 0.04f,
                   int maxLines = 1)
{
    const float scale = juce::jmax (0.1f, (float) g.getInternalContext().getPhysicalPixelScaleFactor());
    const float h = height <= type::title + 0.01f ? height * juce::jlimit (1.0f, 1.25f, 1.0f / scale) : height;
    if (maxLines == 1 && area.getHeight() < h * 1.3f) area = area.withSizeKeepingCentre (area.getWidth(), h * 1.3f);
    juce::GlyphArrangement ga;
    ga.addFittedText (font (w, h, kerning), text, area.getX(), area.getY(), area.getWidth(), area.getHeight(), j, maxLines, 0.62f);
    g.setColour (colour);
    ga.draw (g);
}

// ---- noise (printed-circuit-board-ish grain on plates) ----------------------------------------------------------
inline const juce::Image& noiseTile (int pixelScale = 1)
{
    static std::map<int, juce::Image> cache;
    pixelScale = juce::jlimit (1, 4, pixelScale);
    if (auto it = cache.find (pixelScale); it != cache.end()) return it->second;
    const juce::Image tile = [pixelScale] {
        juce::Image im (juce::Image::ARGB, 160 * pixelScale, 160 * pixelScale, true);
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
    return cache.emplace (pixelScale, tile).first->second;
}

// Physical pixels per logical pixel of the drawing target, as a whole number for the texture caches (1 … 4).
inline int textureScale (juce::Graphics& g) { return juce::jlimit (1, 4, (int) std::ceil ((float) g.getInternalContext().getPhysicalPixelScaleFactor() - 0.01f)); }

inline void grain (juce::Graphics& g, juce::Rectangle<float> area, float opacity = 0.5f, juce::Rectangle<float>* clipPath = nullptr)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (area.getSmallestIntegerContainer());
    (void) clipPath;
    const int ts = textureScale (g);
    juce::FillType fill (noiseTile (ts), juce::AffineTransform::scale (1.0f / (float) ts));
    fill.setOpacity (opacity);
    g.setFillType (fill);
    g.fillRect (area);
}

// ---- surface textures ------------------------------------------------------------------------------------------
// Tiles made once from seeded, tileable value noise. A tile is a height field lit from the top left and stored as
// white (lit) and black (shaded) at low alpha, so it shades whatever colour is under it.
namespace detail
{
    // Tileable value noise: `cells` lattice cells across the tile, smooth interpolation, `octaves` octaves.
    inline std::vector<float> noiseField (int size, int cells, int octaves, juce::int64 seed)
    {
        std::vector<float> h ((size_t) (size * size), 0.0f);
        float amp = 1.0f, total = 0.0f;
        for (int o = 0; o < octaves; ++o, cells *= 2, amp *= 0.5f)
        {
            juce::Random r (seed + o * 7919);
            std::vector<float> lattice ((size_t) (cells * cells));
            for (auto& v : lattice) v = r.nextFloat();
            auto at = [&] (int x, int y) { return lattice[(size_t) (((y % cells + cells) % cells) * cells + ((x % cells + cells) % cells))]; };
            const float step = (float) cells / (float) size;
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                {
                    const float fx = (float) x * step, fy = (float) y * step;
                    const int ix = (int) fx, iy = (int) fy;
                    float tx = fx - (float) ix, ty = fy - (float) iy;
                    tx = tx * tx * (3.0f - 2.0f * tx); ty = ty * ty * (3.0f - 2.0f * ty);
                    const float v = juce::jmap (ty, juce::jmap (tx, at (ix, iy), at (ix + 1, iy)), juce::jmap (tx, at (ix, iy + 1), at (ix + 1, iy + 1)));
                    h[(size_t) (y * size + x)] += amp * v;
                }
            total += amp;
        }
        for (auto& v : h) v /= total;
        return h;
    }

    // Shading tile from a height field: lit = white, shaded = black; `strength` is the alpha at full slope.
    // `pixelScale`: the tile has that many pixels per logical pixel (same pattern, finer pixels).
    inline juce::Image litTile (int size, int cells, int octaves, juce::int64 seed, float relief, float strength, int pixelScale = 1)
    {
        size *= pixelScale;
        relief *= (float) pixelScale;
        const auto h = noiseField (size, cells, octaves, seed);
        juce::Image im (juce::Image::ARGB, size, size, true);
        juce::Image::BitmapData d (im, juce::Image::BitmapData::writeOnly);
        auto H = [&] (int x, int y) { return h[(size_t) (((y + size) % size) * size + ((x + size) % size))]; };
        const float lx = -0.62f, ly = -0.62f;                    // light from the top left
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const float dx = (H (x + 1, y) - H (x - 1, y)) * relief, dy = (H (x, y + 1) - H (x, y - 1)) * relief;
                const float shade = juce::jlimit (-1.0f, 1.0f, -(dx * lx + dy * ly));
                const auto a = (juce::uint8) juce::jlimit (0, 255, (int) (std::abs (shade) * strength * 255.0f));
                d.setPixelColour (x, y, (shade > 0 ? juce::Colours::white : juce::Colours::black).withAlpha (a));
            }
        return im;
    }

    // Broad blotches of slightly lighter and darker paint.
    inline juce::Image mottleTile()
    {
        const int size = 512;
        const auto h = noiseField (size, 5, 3, 11);
        juce::Image im (juce::Image::ARGB, size, size, true);
        juce::Image::BitmapData d (im, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const float v = (h[(size_t) (y * size + x)] - 0.5f) * 2.0f;
                d.setPixelColour (x, y, (v > 0 ? juce::Colours::white : juce::Colours::black).withAlpha ((juce::uint8) juce::jlimit (0, 255, (int) (std::abs (v) * 12.0f))));
            }
        return im;
    }
}

// Enamel: a fine "orange peel" relief; the chassis: rougher powder coat. Tiles per pixel scale, so they stay sharp on
// retina and 4K screens.
enum class Texture { Enamel, Powder, Mottle };
inline const juce::Image& texture (Texture t, int pixelScale)
{
    static std::map<int, juce::Image> cache;
    pixelScale = juce::jlimit (1, 4, t == Texture::Mottle ? 1 : pixelScale);       // the blotches are soft anyway
    const int key = (int) t * 10 + pixelScale;
    if (auto it = cache.find (key); it != cache.end()) return it->second;
    juce::Image im = t == Texture::Enamel ? detail::litTile (256, 160, 2, 3, 1.2f, 0.07f, pixelScale)
                   : t == Texture::Powder ? detail::litTile (256, 256, 1, 5, 0.9f, 0.12f, pixelScale)
                                          : detail::mottleTile();
    return cache.emplace (key, im).first->second;
}

inline void fillTiled (juce::Graphics& g, Texture t, const juce::Path& area, float opacity)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (area);
    const int ts = t == Texture::Mottle ? 1 : textureScale (g);
    juce::FillType fill (texture (t, ts), juce::AffineTransform::scale (1.0f / (float) ts));
    fill.setOpacity (opacity);
    g.setFillType (fill);
    g.fillPath (area);
}

// Light use: hairline scratches, a few chips on the corners and edges (bare aluminium under the paint), dust and
// rubbed edges. Seeded by the plate, so every plate has its own marks and they are the same on every redraw.
inline void drawWear (juce::Graphics& g, juce::Rectangle<float> r, bool coloured, float amount = 1.0f)
{
    juce::Random rnd ((juce::int64) (r.getX() * 131.0f + r.getY() * 977.0f + r.getWidth() * 7.0f + r.getHeight()));
    const float area = r.getWidth() * r.getHeight();
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (r.toNearestInt());
    // scratches: short, slightly curved, a lighter line over a hint of shadow
    const int scratches = (int) ((3.0f + area / 30000.0f) * amount);
    for (int i = 0; i < scratches; ++i)
    {
        const float x = r.getX() + 8.0f + rnd.nextFloat() * (r.getWidth() - 16.0f), y = r.getY() + 8.0f + rnd.nextFloat() * (r.getHeight() - 16.0f);
        const float len = 5.0f + rnd.nextFloat() * 15.0f;
        const float a = (rnd.nextFloat() - 0.5f) * 1.2f + (rnd.nextFloat() < 0.3f ? juce::MathConstants<float>::halfPi : 0.0f);
        juce::Path p;
        p.startNewSubPath (x, y);
        p.quadraticTo (x + 0.5f * len * std::cos (a) + rnd.nextFloat() - 0.5f, y + 0.5f * len * std::sin (a) + rnd.nextFloat() - 0.5f, x + len * std::cos (a), y + len * std::sin (a));
        g.setColour (juce::Colours::black.withAlpha (0.20f));
        g.strokePath (p, juce::PathStrokeType (0.6f), juce::AffineTransform::translation (0.5f, 0.6f));
        g.setColour (juce::Colours::white.withAlpha (0.14f + 0.14f * rnd.nextFloat()));
        g.strokePath (p, juce::PathStrokeType (0.4f));
    }
    // chips: mostly at the corners
    const juce::Point<float> corners[] = { r.getTopLeft(), r.getTopRight(), r.getBottomLeft(), r.getBottomRight() };
    const int chips = (int) ((coloured ? 3 : 2) * amount);
    for (int i = 0; i < chips; ++i)
    {
        juce::Point<float> c = corners[rnd.nextInt (4)];
        if (i >= 2 && rnd.nextFloat() > 0.75f)
            c = rnd.nextBool() ? juce::Point<float> (r.getX() + rnd.nextFloat() * r.getWidth(), rnd.nextBool() ? r.getY() : r.getBottom())
                               : juce::Point<float> (rnd.nextBool() ? r.getX() : r.getRight(), r.getY() + rnd.nextFloat() * r.getHeight());
        const float size = 1.4f + rnd.nextFloat() * 1.8f;
        juce::Path chip;
        for (int k = 0; k < 7; ++k)
        {
            const float a = (float) k / 7.0f * juce::MathConstants<float>::twoPi + (rnd.nextFloat() - 0.5f) * 0.6f;
            const float rr = size * (0.45f + 0.55f * rnd.nextFloat());
            if (k == 0) chip.startNewSubPath (c.x + rr * std::cos (a), c.y + rr * std::sin (a)); else chip.lineTo (c.x + rr * std::cos (a), c.y + rr * std::sin (a));
        }
        chip.closeSubPath();
        g.setColour (coloured ? juce::Colour (0xff8d9195) : juce::Colour (0xff4a4c4e));
        g.fillPath (chip);
        juce::ColourGradient sh (juce::Colours::white.withAlpha (0.5f), c.x - size, c.y - size, juce::Colours::black.withAlpha (0.35f), c.x + size, c.y + size, false);
        g.setGradientFill (sh); g.fillPath (chip);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (chip, juce::PathStrokeType (0.6f));
    }
    // dust
    const int specks = (int) (area / 900.0f * amount);
    for (int i = 0; i < specks; ++i)
    {
        const float d = 0.5f + rnd.nextFloat();
        g.setColour (juce::Colours::white.withAlpha (0.06f + 0.14f * rnd.nextFloat()));
        g.fillEllipse (r.getX() + rnd.nextFloat() * r.getWidth(), r.getY() + rnd.nextFloat() * r.getHeight(), d, d);
    }
    // edges rubbed thin in a few places
    juce::Path edge; edge.addRoundedRectangle (r.reduced (0.5f), 3.0f);
    juce::Path dashed;
    const float dashes[] = { 14.0f + 26.0f * rnd.nextFloat(), 20.0f + 50.0f * rnd.nextFloat(), 5.0f + 13.0f * rnd.nextFloat(), 30.0f + 60.0f * rnd.nextFloat() };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, edge, dashes, 4);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillPath (dashed);
}

// Enamel finish over a filled area: orange-peel relief, blotches, a broad gloss from the top-left light.
inline void enamel (juce::Graphics& g, const juce::Path& area, juce::Rectangle<float> r)
{
    fillTiled (g, Texture::Enamel, area, 1.0f);
    fillTiled (g, Texture::Mottle, area, 0.9f);
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (area);
    juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.18f), r.getX() + r.getWidth() * 0.22f, r.getY() - r.getHeight() * 0.08f,
                                juce::Colours::white.withAlpha (0.0f), r.getX() + r.getWidth() * 0.22f + juce::jmax (r.getWidth(), r.getHeight()) * 0.7f, r.getY() + r.getHeight() * 0.5f, true);
    g.setGradientFill (gloss);
    g.fillRect (r);
}

// ---- module plate: enamelled panel with a thin outline just inside the edge, lightly worn -------------------------
inline void drawPlate (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fill, bool outlined = true)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.expanded (1.5f).translated (0.0f, 1.5f), 3.5f);                    // contact shadow
    juce::ColourGradient grad (fill.brighter (0.08f), r.getX(), r.getY(), fill.darker (0.14f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    juce::Path face; face.addRoundedRectangle (r, 2.5f);
    g.fillPath (face);
    enamel (g, face, r);
    drawWear (g, r, fill.getPerceivedBrightness() > 0.3f);
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawRoundedRectangle (r.reduced (0.5f), 2.5f, 1.0f);                                         // bevel highlight
    if (outlined)
    {
        g.setColour ((fill.getPerceivedBrightness() > 0.42f ? col::inkDark : col::outline).withAlpha (0.78f));
        g.drawRoundedRectangle (r.reduced (4.5f), 2.0f, 1.1f);
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

// Coloured glow for lamps and lit buttons: a wide soft halo plus a tighter, hotter core.
inline void drawGlow (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour colour, float strength)
{
    if (strength <= 0.001f) return;
    juce::ColourGradient halo (colour.withAlpha (0.50f * strength), c.x, c.y, colour.withAlpha (0.0f), c.x + radius, c.y, true);
    halo.addColour (0.35, colour.withAlpha (0.26f * strength));
    g.setGradientFill (halo);
    g.fillEllipse (c.x - radius, c.y - radius, radius * 2, radius * 2);
    const float r2 = radius * 0.55f;
    juce::ColourGradient core (colour.brighter (0.4f).withAlpha (0.65f * strength), c.x, c.y, colour.withAlpha (0.0f), c.x + r2, c.y, true);
    g.setGradientFill (core);
    g.fillEllipse (c.x - r2, c.y - r2, r2 * 2, r2 * 2);
}

// Pixel scale of the current drawing target (window scale x display scale), used to size cached images.
inline float pixelScale (juce::Graphics& g) { return juce::jmax (1.0f, (float) g.getInternalContext().getPhysicalPixelScaleFactor()); }
} // namespace babushka::ui
