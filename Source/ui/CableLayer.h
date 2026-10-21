// Babushka — patch cables with rope physics.
//
// Each cable is a chain of points integrated with Verlet steps under gravity and held together by distance
// constraints (position-based dynamics). Constraints between every second point resist sharp bends, so the cable
// behaves like a springy rubber-jacketed lead rather than a string: it hangs in a wide curve, follows a dragged
// plug and settles quickly. The two plugs are pinned to their jacks. A plugged cable is a little longer than the
// span between its jacks; a cable in the hand pays out or takes in length as the plug moves away or comes back.
//
// Interaction: press an empty jack and drag to pull a new plug out, release over another jack to connect.
// Press a jack that already has a cable to pull that plug out again; release away from any jack and the cable
// is removed. Right-click a plugged jack to remove its cable. The layer only catches the mouse near jacks and
// while a plug is held, so the knobs underneath keep working.
#pragma once

#include "Theme.h"
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace babushka::ui
{
struct JackSite { babushka::dsp::Jack id; juce::Point<float> pos; };

class CableLayer : public juce::Component, private juce::Timer
{
public:
    explicit CableLayer (BabushkaProcessor& p) : processor (p)
    {
        setInterceptsMouseClicks (true, false);
        startTimerHz (60);
    }
    ~CableLayer() override { stopTimer(); }

    HintSink* hints = nullptr;

    void setJacks (std::vector<JackSite> sites) { jacks = std::move (sites); }

    // Mouse only reaches this layer near a jack (or while a plug is in hand).
    bool hitTest (int x, int y) override
    {
        if (held >= 0) return true;
        return jackAt ({ (float) x, (float) y }, kGrab) >= 0;
    }

    void paint (juce::Graphics& g) override
    {
        for (size_t i = 0; i < ropes.size(); ++i) paintRope (g, ropes[i], (int) i == held);
        if (hoverJack >= 0 && ! isHolding())
            paintRing (g, jacks[(size_t) hoverJack].pos, juce::Colours::white.withAlpha (0.55f));
        if (isHolding() && targetJack >= 0)
            paintRing (g, jacks[(size_t) targetJack].pos, col::ledGreen.withAlpha (0.9f));
    }

    void mouseMove (const juce::MouseEvent& e) override { updateHover (e.position); }
    void mouseExit (const juce::MouseEvent&) override { hoverJack = -1; if (hints) hints->hideHint(); repaint(); }
    void mouseDown (const juce::MouseEvent& e) override { pressAt (e.position, e.mods.isPopupMenu() || e.mods.isRightButtonDown()); }
    void mouseDrag (const juce::MouseEvent& e) override { dragTo (e.position); }
    void mouseUp (const juce::MouseEvent& e) override { releaseAt (e.position); }

    // The pointer logic, callable without a mouse (used by the UI tests).
    void pressAt (juce::Point<float> pos, bool removeOnly)
    {
        const int j = jackAt (pos, kGrab);
        if (j < 0) return;
        const auto id = jacks[(size_t) j].id;
        const int existing = topRopeAt (id);
        if (removeOnly)
        {
            if (existing >= 0)
            {
                removeModelCable (ropes[(size_t) existing]);
                ropes.erase (ropes.begin() + existing);
                if (held > existing) --held;
                repaint();
            }
            return;
        }
        if (existing >= 0)
        {
            // pull the plug out: the cable stays attached at its other end
            Rope r = ropes[(size_t) existing];
            ropes.erase (ropes.begin() + existing);
            removeModelCable (r);
            if (r.a == id) { std::swap (r.a, r.b); std::reverse (r.p.begin(), r.p.end()); std::reverse (r.prev.begin(), r.prev.end()); }
            r.pending = true;                                  // anchor = r.a, loose end = r.b
            r.dragPos = pos;
            ropes.push_back (std::move (r));
        }
        else
        {
            Rope r = makeRope (id, id, jackPos (id), jackPos (id), kMinLength);
            r.pending = true;
            r.dragPos = pos;
            ropes.push_back (std::move (r));
        }
        held = (int) ropes.size() - 1;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        dragTo (pos);
    }

    void dragTo (juce::Point<float> pos)
    {
        if (held < 0) return;
        Rope& r = ropes[(size_t) held];
        const auto anchor = jackPos (r.a);
        auto d = pos;
        const float dist = anchor.getDistanceFrom (d);
        if (dist > kMaxLength * 0.97f) d = anchor + (d - anchor) * (kMaxLength * 0.97f / dist);   // the longest cable in the bag
        r.dragPos = d;
        r.targetLength = slackFor (anchor.getDistanceFrom (d)) + 20.0f;
        targetJack = jackAt (pos, kDrop);
        if (targetJack >= 0 && jacks[(size_t) targetJack].id == r.a) targetJack = -1;
        if (hints && targetJack >= 0) hints->showHint (*this, jackName (jacks[(size_t) targetJack].id));
        else if (hints) hints->hideHint();
    }

    void releaseAt (juce::Point<float> pos)
    {
        if (held < 0) return;
        Rope r = ropes[(size_t) held];
        ropes.erase (ropes.begin() + held);
        held = -1;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        const int t = jackAt (pos, kDrop);
        if (t >= 0 && jacks[(size_t) t].id != r.a)
        {
            const auto target = jacks[(size_t) t].id;
            if (processor.addCable (r.a, target))
            {
                r.b = target; r.pending = false;
                r.dragPos = jackPos (target);
                r.targetLength = slackFor (jackPos (r.a).getDistanceFrom (jackPos (target)));
                ropes.push_back (std::move (r));
                knownVersion = processor.getPatchVersion();
            }
        }
        targetJack = -1;
        if (hints) hints->hideHint();
        updateHover (pos);
    }

    juce::Point<float> jackCentre (babushka::dsp::Jack id) const { return jackPos (id); }
    int ropeCount() const { return (int) ropes.size(); }
    bool isHoldingPlug() const { return held >= 0; }

private:
    static constexpr float kGrab = 15.0f, kDrop = 19.0f;
    static constexpr float kMinLength = 120.0f, kMaxLength = 1100.0f;
    static constexpr int kPoints = 34;

    // Length of a plugged cable: some slack over the straight span, so it hangs in a curve without dragging on the panel.
    static float slackFor (float span) { return juce::jlimit (kMinLength, kMaxLength, span * 1.07f + 40.0f); }

    struct Rope
    {
        babushka::dsp::Jack a {}, b {};
        bool pending = false;                       // one end is in the user's hand
        float length = 200.0f, targetLength = 200.0f;
        std::vector<juce::Point<float>> p, prev;
        juce::Point<float> dragPos;
        juce::Colour colour;
    };

    BabushkaProcessor& processor;
    std::vector<JackSite> jacks;
    std::vector<Rope> ropes;
    int held = -1, hoverJack = -1, targetJack = -1;
    int knownVersion = -1;

    bool isHolding() const { return held >= 0; }

    juce::Point<float> jackPos (babushka::dsp::Jack id) const
    {
        for (auto& j : jacks) if (j.id == id) return j.pos;
        return {};
    }
    static juce::String jackName (babushka::dsp::Jack id)
    {
        const auto& i = babushka::dsp::jackInfo (id);
        return juce::String (i.section) + " · " + i.label;
    }
    int jackAt (juce::Point<float> p, float radius) const
    {
        int best = -1; float bestD = radius;
        for (size_t i = 0; i < jacks.size(); ++i)
        {
            const float d = jacks[i].pos.getDistanceFrom (p);
            if (d <= bestD) { bestD = d; best = (int) i; }
        }
        return best;
    }
    int topRopeAt (babushka::dsp::Jack id) const
    {
        for (int i = (int) ropes.size() - 1; i >= 0; --i)
            if (! ropes[(size_t) i].pending && (ropes[(size_t) i].a == id || ropes[(size_t) i].b == id)) return i;
        return -1;
    }
    void updateHover (juce::Point<float> p)
    {
        const int j = jackAt (p, kGrab);
        if (j != hoverJack)
        {
            hoverJack = j;
            if (hints) { if (j >= 0) hints->showHint (*this, jackName (jacks[(size_t) j].id)); else hints->hideHint(); }
            repaint();
        }
    }
    void removeModelCable (const Rope& r)
    {
        const auto cab = processor.getPatchCopy();
        for (int i = 0; i < cab.count; ++i)
        {
            const auto& c = cab.cables[(size_t) i];
            if ((c.a == r.a && c.b == r.b) || (c.a == r.b && c.b == r.a)) { processor.removeCable (i); break; }
        }
        knownVersion = processor.getPatchVersion();
    }

    // Each new cable takes the next colour of the set, like a bag of coloured cables.
    juce::Colour nextCableColour()
    {
        static const juce::uint32 palette[] = { 0xffc83a2f, 0xffe3b23c, 0xff3f78bd, 0xff4f9d57, 0xffe0782c, 0xffe6e1d4, 0xff8a58a8, 0xff2fa3a0, 0xff2c2c2e };
        return juce::Colour (palette[(colourCounter++) % 9]);
    }
    int colourCounter = 0;

    Rope makeRope (babushka::dsp::Jack a, babushka::dsp::Jack b, juce::Point<float> pa, juce::Point<float> pb, float length)
    {
        Rope r; r.a = a; r.b = b; r.length = r.targetLength = length; r.colour = nextCableColour();
        const int n = kPoints;
        const float chord = pa.getDistanceFrom (pb);
        const float sag = std::sqrt (juce::jmax (0.0f, 0.375f * juce::jmax (chord, 40.0f) * juce::jmax (0.0f, length - chord)));
        r.p.resize ((size_t) n); r.prev.resize ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) (n - 1);
            juce::Point<float> q = pa + (pb - pa) * t;
            q.y += sag * 4.0f * t * (1.0f - t);
            r.p[(size_t) i] = r.prev[(size_t) i] = q;
        }
        r.dragPos = pb;
        return r;
    }

    // Keeps the rope list in step with the patch stored in the processor (preset loads, undo, other editors).
    // Repaints whenever the list changed: a removed rope must disappear even when nothing else moves.
    void syncWithModel()
    {
        const int v = processor.getPatchVersion();
        if (v == knownVersion) return;
        knownVersion = v;
        repaint();
        const auto cab = processor.getPatchCopy();
        // drop ropes that are no longer in the patch
        for (int i = (int) ropes.size() - 1; i >= 0; --i)
        {
            if (ropes[(size_t) i].pending) continue;
            bool found = false;
            for (int k = 0; k < cab.count && ! found; ++k)
            {
                const auto& c = cab.cables[(size_t) k];
                found = (c.a == ropes[(size_t) i].a && c.b == ropes[(size_t) i].b) || (c.a == ropes[(size_t) i].b && c.b == ropes[(size_t) i].a);
            }
            if (! found) { ropes.erase (ropes.begin() + i); if (held > i) --held; }
        }
        // add ropes for new cables (loaded presets, other editors)
        for (int k = 0; k < cab.count; ++k)
        {
            const auto& c = cab.cables[(size_t) k];
            bool have = false;
            for (auto& r : ropes) if (! r.pending && ((r.a == c.a && r.b == c.b) || (r.a == c.b && r.b == c.a))) { have = true; break; }
            if (have) continue;
            const auto pa = jackPos (c.a), pb = jackPos (c.b);
            ropes.insert (ropes.begin() + (held >= 0 ? held : (int) ropes.size()), makeRope (c.a, c.b, pa, pb, slackFor (pa.getDistanceFrom (pb))));
            if (held >= 0) ++held;
        }
    }

    void timerCallback() override
    {
        syncWithModel();
        if (ropes.empty()) return;
        constexpr float dt = 1.0f / 60.0f, gravity = 1900.0f, damping = 0.93f, bendStiffness = 0.14f;
        bool moving = false;
        for (auto& r : ropes)
        {
            const auto pa = jackPos (r.a);
            const auto pb = r.pending ? r.dragPos : jackPos (r.b);
            const size_t n = r.p.size();
            r.length += (r.targetLength - r.length) * 0.25f;
            for (size_t i = 1; i + 1 < n; ++i)
            {
                const auto v = (r.p[i] - r.prev[i]) * damping;
                r.prev[i] = r.p[i];
                r.p[i] += v + juce::Point<float> (0.0f, gravity * dt * dt);
            }
            r.p[0] = pa; r.p[n - 1] = pb;
            const float seg = r.length / (float) (n - 1);
            for (int it = 0; it < 16; ++it)
            {
                for (size_t i = 0; i + 1 < n; ++i)
                {
                    auto d = r.p[i + 1] - r.p[i];
                    const float len = std::max (0.0001f, std::sqrt (d.x * d.x + d.y * d.y));
                    const float diff = (len - seg) / len;
                    const bool fa = (i == 0), fb = (i + 1 == n - 1);
                    if (fa && fb) continue;
                    if (fa)       r.p[i + 1] -= d * diff;
                    else if (fb)  r.p[i] += d * diff;
                    else        { r.p[i] += d * (0.5f * diff); r.p[i + 1] -= d * (0.5f * diff); }
                }
                // bending: points two apart are pushed towards a straight line, so the jacket resists kinks
                for (size_t i = 0; i + 2 < n; ++i)
                {
                    auto d = r.p[i + 2] - r.p[i];
                    const float len = std::max (0.0001f, std::sqrt (d.x * d.x + d.y * d.y));
                    const float minLen = 1.97f * seg;
                    if (len >= minLen) continue;
                    const auto corr = d * (bendStiffness * 0.5f * (len - minLen) / len);
                    if (i != 0)         r.p[i] += corr;
                    if (i + 2 != n - 1) r.p[i + 2] -= corr;
                }
                r.p[0] = pa; r.p[n - 1] = pb;
            }
            for (size_t i = 1; i + 1 < n; ++i) if (r.p[i].getDistanceFrom (r.prev[i]) > 0.02f) { moving = true; break; }
            if (std::abs (r.targetLength - r.length) > 0.5f) moving = true;
        }
        if (moving || held >= 0) repaint();
    }

    // ---- drawing ----------------------------------------------------------------------------------------------------------
    static juce::Path smoothPath (const std::vector<juce::Point<float>>& p)
    {
        juce::Path path;
        if (p.size() < 2) return path;
        path.startNewSubPath (p.front());
        for (size_t i = 1; i + 1 < p.size(); ++i)
        {
            const auto mid = (p[i] + p[i + 1]) * 0.5f;
            path.quadraticTo (p[i], mid);
        }
        path.lineTo (p.back());
        return path;
    }

    void paintRope (juce::Graphics& g, const Rope& r, bool inHand)
    {
        const auto path = smoothPath (r.p);
        const float lift = inHand ? 1.5f : 1.0f;                 // a plug in the hand is lifted off the panel
        for (int i = 0; i < 3; ++i)                              // soft shadow
        {
            g.setColour (juce::Colours::black.withAlpha (0.10f));
            g.strokePath (path, juce::PathStrokeType (7.0f + (float) i * 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                          juce::AffineTransform::translation (3.0f * lift, 5.0f * lift));
        }
        g.setColour (r.colour.darker (0.75f));
        g.strokePath (path, juce::PathStrokeType (7.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (r.colour.darker (0.15f));
        g.strokePath (path, juce::PathStrokeType (5.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (r.colour);
        g.strokePath (path, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), juce::AffineTransform::translation (-0.4f, -0.6f));
        g.setColour (r.colour.brighter (0.7f).withAlpha (0.6f));
        g.strokePath (path, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), juce::AffineTransform::translation (-1.0f, -1.4f));
        paintPlug (g, r.p[0], r.colour, false);
        paintPlug (g, r.p.back(), r.colour, inHand);
    }

    static void paintPlug (juce::Graphics& g, juce::Point<float> c, juce::Colour colour, bool lifted)
    {
        const float s = lifted ? 1.22f : 1.0f;
        const float ro = 11.5f * s, ri = 8.2f * s;
        g.setColour (juce::Colours::black.withAlpha (lifted ? 0.30f : 0.38f));
        g.fillEllipse (c.x - ro + 2.5f * s, c.y - ro + 4.0f * s, ro * 2, ro * 2);
        juce::ColourGradient ring (juce::Colour (0xfff3f4f5), c.x - ro * 0.7f, c.y - ro * 0.8f, juce::Colour (0xff56595e), c.x + ro, c.y + ro, false);
        ring.addColour (0.5, juce::Colour (0xffb7babe));
        g.setGradientFill (ring);
        g.fillEllipse (c.x - ro, c.y - ro, ro * 2, ro * 2);
        juce::ColourGradient boot (colour.brighter (0.5f), c.x - ri * 0.6f, c.y - ri * 0.7f, colour.darker (0.55f), c.x + ri, c.y + ri, true);
        g.setGradientFill (boot);
        g.fillEllipse (c.x - ri, c.y - ri, ri * 2, ri * 2);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.fillEllipse (c.x - ri * 0.55f, c.y - ri * 0.75f, ri * 0.8f, ri * 0.5f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (c.x - ro, c.y - ro, ro * 2, ro * 2, 0.8f);
    }

    static void paintRing (juce::Graphics& g, juce::Point<float> c, juce::Colour colour)
    {
        g.setColour (colour);
        g.drawEllipse (c.x - 15.0f, c.y - 15.0f, 30.0f, 30.0f, 1.8f);
    }
};
} // namespace babushka::ui
