// Babushka — the four pedals on the rear panel: enclosures with their jacks, display, two knobs, LED and footswitch;
// the pedal presets (factory and the user's own), and the square FX lock button of the preset block.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"
#include <functional>
#include <map>

namespace babushka::ui
{
// TAPE ECHO TIME while the echo is locked to the tempo: the knob points at the time of the division at the host's tempo
// (at its end when that is longer than its 600 ms) and says so; taking the knob by hand goes back to FREE, from where
// it points.
inline void bindEchoTime (Knob& k, BabushkaProcessor& p)
{
    if (k.paramId != "tapTime") return;
    namespace law = babushka::dsp::pedallaw;
    auto seconds = [&p] {
        const int sync = (int) std::lround (p.apvts.getRawParameterValue ("tapSync")->load());
        return sync == 0 ? -1.0f : (float) (60.0 / juce::jmax (20.0, p.getHostBpm())) * law::echoDivisionBeats (sync); };
    k.shown = [seconds] { const float s = seconds(); return s < 0.0f ? -1.0f : law::echoKnobFor (s); };
    k.shownText = [seconds, &p] {
        const float s = seconds();
        if (s < 0.0f) return juce::String();
        auto* sync = p.apvts.getParameter ("tapSync");
        return sync->getCurrentValueAsText() + "  " + juce::String (juce::roundToInt (1000.0f * juce::jmin (s, law::kEchoMaxSyncedSeconds))) + " ms"; };
    k.onTouch = [seconds, &p] {
        const float s = seconds();
        if (s < 0.0f) return;
        if (auto* t = p.apvts.getParameter ("tapTime")) t->setValueNotifyingHost (law::echoKnobFor (s));
        if (auto* sync = p.apvts.getParameter ("tapSync")) sync->setValueNotifyingHost (0.0f); };
    k.followShown();
}
}

namespace babushka::ui
{
// ---- looks -------------------------------------------------------------------------------------------------------
struct PedalLook { juce::Colour top, bottom, ink, lcd; const char* folder; };
inline const PedalLook& pedalLook (int pedal)
{
    static const PedalLook looks[] = {
        // enamel colours of the die-cast boxes; the pedals keep them on every skin
        { juce::Colour (0xffc8362f), juce::Colour (0xff9e2822), juce::Colour (0xffece6d6), juce::Colour (0xffffb540), "Saturator" },
        { juce::Colour (0xffe57b25), juce::Colour (0xffbb5f16), juce::Colour (0xff1f1206), juce::Colour (0xffffd28a), "Phaser" },
        { juce::Colour (0xff3f7fc0), juce::Colour (0xff2c6099), juce::Colour (0xffece6d6), juce::Colour (0xff8fe6ff), "BBD Chorus" },
        { juce::Colour (0xffe9e1c8), juce::Colour (0xffc4bb9f), juce::Colour (0xff1a1810), juce::Colour (0xff9dffa0), "Tape Echo" },
    };
    return looks[juce::jlimit (0, babushka::kNumPedals - 1, pedal)];
}

inline juce::RangedAudioParameter* pedalParam (BabushkaProcessor& p, const juce::String& id)
{
    return dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id));
}
inline float pedalValue (BabushkaProcessor& p, const juce::String& id)
{
    if (auto* rp = pedalParam (p, id)) return rp->convertFrom0to1 (rp->getValue());
    return 0.0f;
}
inline juce::String pedalText (BabushkaProcessor& p, const juce::String& id)
{
    if (auto* rp = pedalParam (p, id)) return rp->getCurrentValueAsText();
    return {};
}
// One parameter change as the user's own edit (a step of the undo history).
inline void setPedalParam (BabushkaProcessor& p, const juce::String& id, float realValue, bool gesture = true)
{
    if (auto* rp = pedalParam (p, id))
    {
        if (gesture) rp->beginChangeGesture();
        rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
        if (gesture) rp->endChangeGesture();
    }
}

// What the LCD of a pedal shows on its second line: its main knob in real units (the echo: the synced division).
inline juce::String pedalReadout (BabushkaProcessor& p, int pedal)
{
    const auto& info = babushka::pedalInfo()[(size_t) pedal];
    const auto& knob = info.knobs.front();
    if (pedal == 3 && (int) pedalValue (p, "tapSync") > 0) return "TIME " + pedalText (p, "tapSync");
    return juce::String (knob.name) + " " + pedalText (p, knob.id);
}
inline juce::String pedalModeText (BabushkaProcessor& p, int pedal)
{
    const auto t = pedalText (p, babushka::pedalInfo()[(size_t) pedal].modeId);
    switch (pedal)
    {
        case 2:  return t == "VIBRATO" || t == "ENSEMBLE" ? t : "MODE " + t;
        case 3:  return (t.length() == 1 ? "HEAD " : "HEADS ") + t;
        default: return t;
    }
}

// ---- pedal presets -------------------------------------------------------------------------------------------------
// Factory presets are compiled in (PedalInfo.h); the user's own are small XML files in
// Documents/Babushka/Pedal Presets/<pedal>/<name>.xml, next to the synth presets.
struct PedalPresetEntry
{
    juce::String name, about;
    std::vector<std::pair<juce::String, float>> values;
    bool factory = true;
    juce::File file;
};

class PedalPresets
{
public:
    // The parameters a pedal preset holds: the mode switch, the echo's sync and every knob (not ON, not HOLD).
    static juce::StringArray valueIds (int pedal)
    {
        const auto& info = babushka::pedalInfo()[(size_t) pedal];
        juce::StringArray ids { info.modeId };
        if (pedal == 3) ids.add ("tapSync");
        for (auto& k : info.knobs) ids.add (k.id);
        return ids;
    }

    static juce::File folder (const BabushkaProcessor& p, int pedal)
    {
        return p.getPresetRoot().getSiblingFile ("Pedal Presets").getChildFile (pedalLook (pedal).folder);
    }

    static std::vector<PedalPresetEntry> list (const BabushkaProcessor& p, int pedal)
    {
        std::vector<PedalPresetEntry> all;
        for (auto& f : babushka::factoryPedalPresets (pedal))
        {
            PedalPresetEntry e { f.name, f.about, {}, true, {} };
            for (auto& [id, v] : f.values) e.values.push_back ({ id, v });
            all.push_back (std::move (e));
        }
        for (auto& e : userPresets (p, pedal)) all.push_back (e);
        return all;
    }

    static std::vector<std::pair<juce::String, float>> current (BabushkaProcessor& p, int pedal)
    {
        std::vector<std::pair<juce::String, float>> v;
        for (auto& id : valueIds (pedal)) v.push_back ({ id, pedalValue (p, id) });
        return v;
    }

    // Sets the pedal to a preset as one step of the undo history.
    static void apply (BabushkaProcessor& p, int pedal, const PedalPresetEntry& e)
    {
        for (auto& [id, v] : e.values) setPedalParam (p, id, v, false);
        p.commitSoundEdit (juce::String (babushka::pedalInfo()[(size_t) pedal].name) + ": " + e.name);
    }

    // The name of the preset the pedal is set to (empty when its knobs have been moved since).
    static juce::String match (BabushkaProcessor& p, int pedal)
    {
        const auto now = current (p, pedal);
        auto same = [&now] (const PedalPresetEntry& e) {
            for (auto& [id, v] : e.values)
            {
                bool found = false;
                for (auto& [nid, nv] : now) if (nid == id) { found = true; if (std::abs (nv - v) > 0.004f) return false; }
                if (! found) return false;
            }
            return ! e.values.empty(); };
        for (auto& f : babushka::factoryPedalPresets (pedal))
        {
            PedalPresetEntry e;
            for (auto& [id, v] : f.values) e.values.push_back ({ id, v });
            if (same (e)) return f.name;
        }
        for (auto& e : userPresets (p, pedal)) if (same (e)) return e.name;
        return {};
    }

    static juce::File save (BabushkaProcessor& p, int pedal, const juce::String& rawName)
    {
        const auto name = juce::File::createLegalFileName (rawName.trim());
        if (name.isEmpty()) return {};
        const auto dir = folder (p, pedal);
        if (! dir.createDirectory().wasOk()) return {};
        juce::XmlElement xml ("BABUSHKA_PEDAL");
        xml.setAttribute ("pedal", babushka::pedalInfo()[(size_t) pedal].name);
        xml.setAttribute ("name", name);
        for (auto& [id, v] : current (p, pedal))
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", id);
            e->setAttribute ("value", (double) v);
        }
        const auto file = dir.getChildFile (name + ".xml");
        if (! xml.writeTo (file)) return {};
        cache().erase (dir.getFullPathName());
        return file;
    }

    static bool remove (const BabushkaProcessor& p, int pedal, const PedalPresetEntry& e)
    {
        if (e.factory || ! e.file.existsAsFile()) return false;
        const bool ok = e.file.deleteFile();
        cache().erase (folder (p, pedal).getFullPathName());
        return ok;
    }

private:
    static std::map<juce::String, std::vector<PedalPresetEntry>>& cache() { static std::map<juce::String, std::vector<PedalPresetEntry>> c; return c; }

    static const std::vector<PedalPresetEntry>& userPresets (const BabushkaProcessor& p, int pedal)
    {
        const auto dir = folder (p, pedal);
        auto& c = cache();
        auto it = c.find (dir.getFullPathName());
        if (it != c.end()) return it->second;
        std::vector<PedalPresetEntry> found;
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.xml");
        files.sort();
        for (auto& f : files)
            if (auto xml = juce::parseXML (f); xml != nullptr && xml->hasTagName ("BABUSHKA_PEDAL"))
            {
                PedalPresetEntry e { f.getFileNameWithoutExtension(), "Your own setting.", {}, false, f };
                for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
                    e.values.push_back ({ child->getStringAttribute ("id"), (float) child->getDoubleAttribute ("value") });
                found.push_back (std::move (e));
            }
        return c.emplace (dir.getFullPathName(), std::move (found)).first->second;
    }
};

// ---- small parts ---------------------------------------------------------------------------------------------------
inline void drawLockIcon (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, bool closed, float stroke = 1.3f)
{
    const float w = r.getWidth(), h = r.getHeight();
    const auto body = juce::Rectangle<float> (r.getX(), r.getY() + h * 0.42f, w, h * 0.58f);
    g.setColour (c);
    g.fillRoundedRectangle (body, w * 0.12f);
    juce::Path shackle;
    const float sx0 = r.getX() + w * 0.22f, sx1 = r.getRight() - w * 0.22f, top = r.getY() + stroke * 0.5f, base = body.getY();
    const float rad = (sx1 - sx0) * 0.5f;
    shackle.startNewSubPath (sx0, base);
    shackle.lineTo (sx0, top + rad);
    shackle.addCentredArc (sx0 + rad, top + rad, rad, rad, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi);
    shackle.lineTo (sx1, closed ? base : top + rad + h * 0.08f);
    if (! closed) shackle.applyTransform (juce::AffineTransform::translation (0.0f, -h * 0.12f));
    g.strokePath (shackle, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// A tiny arrow key on the pedal's top edge: moves the pedal one place left or right in the chain.
class PedalArrow : public juce::Component
{
public:
    explicit PedalArrow (bool leftward) : left (leftward) { setMouseCursor (juce::MouseCursor::PointingHandCursor); setSize (18, 18); }
    std::function<void()> onClick, onHover, onLeave;   // onLeave: the pointer has gone (hides the hint)
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colours::black.withAlpha (hover ? 0.6f : 0.32f));
        g.fillRoundedRectangle (r, 3.0f);
        const auto c = r.getCentre();
        const float d = left ? -1.0f : 1.0f;
        juce::Path p;
        p.startNewSubPath (c.x - 2.5f * d, c.y - 4.5f);
        p.lineTo (c.x + 2.5f * d, c.y);
        p.lineTo (c.x - 2.5f * d, c.y + 4.5f);
        g.setColour (hover ? col::ledAmber : juce::Colour (0xffe9e4d6));
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); if (onLeave) onLeave(); }
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
private:
    bool left, hover = false;
};

// The pedal's display: the preset (or the mode) and the main knob's value. A click opens the pedal's menu.
class PedalLcd : public juce::Component
{
public:
    explicit PedalLcd (juce::Colour glow) : colour (glow) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick, onHover, onLeave;   // onLeave: the pointer has gone (hides the hint)
    void set (const juce::String& l1, const juce::String& l2, bool lockShown, bool on)
    {
        if (l1 == line1 && l2 == line2 && lockShown == locked && on == lit) return;
        line1 = l1; line2 = l2; locked = lockShown; lit = on;
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff2a2724));
        g.fillRoundedRectangle (r, 4.0f);
        const auto glass = r.reduced (2.0f);
        g.setColour (juce::Colour (0xff0c0905));
        g.fillRoundedRectangle (glass, 3.0f);
        const auto ink = colour.withMultipliedAlpha (lit ? 1.0f : 0.55f);
        if (lit)
        {
            juce::ColourGradient haze (colour.withAlpha (0.10f), glass.getCentreX(), glass.getCentreY(), colour.withAlpha (0.0f), glass.getX(), glass.getY(), true);
            g.setGradientFill (haze);
            g.fillRoundedRectangle (glass, 3.0f);
        }
        // two lines as one block, centred in the glass; with the padlock, padlock and name are centred together
        const float cy = glass.getCentreY();
        const auto top = juce::Rectangle<float> (glass.getX() + 5.0f, cy - 12.5f, glass.getWidth() - 10.0f, 13.0f);
        const auto bottom = juce::Rectangle<float> (glass.getX() + 5.0f, cy + 0.5f, glass.getWidth() - 10.0f, 11.0f);
        const float lockW = locked ? 12.0f : 0.0f;
        const float textW = juce::jmin (juce::GlyphArrangement::getStringWidth (font (Weight::NarrowBold, kLine1, 0.05f), line1), top.getWidth() - lockW);
        const float x = top.getCentreX() - (textW + lockW) * 0.5f;
        if (locked) drawLockIcon (g, { x, top.getCentreY() - 5.0f, 8.0f, 9.5f }, ink, true, 1.2f);
        label (g, line1, { x + lockW, top.getY(), textW + 1.0f, top.getHeight() }, ink, kLine1, Weight::NarrowBold, juce::Justification::centred, 0.05f);
        label (g, line2, bottom, ink.withMultipliedAlpha (0.8f), kLine2, Weight::NarrowSemi, juce::Justification::centred, 0.04f);
        g.setColour (hover ? col::ledAmber : juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (r.reduced (0.75f), 4.0f, hover ? 1.5f : 1.0f);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); if (onLeave) onLeave(); }
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
private:
    static constexpr float kLine1 = 12.0f, kLine2 = 10.0f;
    juce::Colour colour;
    juce::String line1, line2;
    bool locked = false, lit = false, hover = false;
};

// A chrome stomp switch. Click: on / off. On the tape echo a long press is HOLD: the repeats loop while it is held down.
class Footswitch : public juce::Component, private juce::Timer
{
public:
    Footswitch() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setSize (30, 30); }
    std::function<void()> onToggle, onHover, onLeave;   // onLeave: the pointer has gone (hides the hint)
    std::function<void (bool)> onHold;             // long press began (true) / ended (false); none: no long press
    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre().translated (0.0f, down ? 0.8f : 0.0f);
        const float r = 13.0f;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - r, c.y - r + 1.6f, 2 * r, 2 * r);
        juce::Path nut; nut.addPolygon (c, 6, r, juce::MathConstants<float>::pi / 6.0f);       // the mounting nut
        juce::ColourGradient metal (juce::Colour (0xffe6e8ea), c.x - r, c.y - r, juce::Colour (0xff53565b), c.x + r, c.y + r, false);
        g.setGradientFill (metal);
        g.fillPath (nut);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.strokePath (nut, juce::PathStrokeType (0.8f));
        const float b = r * (down ? 0.66f : 0.70f);                                                // the button cap
        juce::ColourGradient cap (juce::Colour (0xffffffff), c.x - b * 0.5f, c.y - b * 0.6f, juce::Colour (0xff6b6e73), c.x + b, c.y + b, true);
        cap.addColour (0.45, juce::Colour (0xffc4c7cb));
        g.setGradientFill (cap);
        g.fillEllipse (c.x - b, c.y - b, 2 * b, 2 * b);
        g.setColour ((hover ? col::ledAmber : juce::Colours::black).withAlpha (hover ? 0.8f : 0.4f));
        g.drawEllipse (c.x - b, c.y - b, 2 * b, 2 * b, hover ? 1.4f : 0.8f);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); if (onLeave) onLeave(); }
    void mouseDown (const juce::MouseEvent&) override
    {
        down = true; holding = false; repaint();
        if (onHold) startTimer (450);
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        stopTimer();
        down = false; repaint();
        if (holding) { holding = false; if (onHold) onHold (false); return; }
        if (getLocalBounds().contains (e.getPosition()) && onToggle) onToggle();
    }
    bool isHolding() const { return holding; }
private:
    bool down = false, hover = false, holding = false;
    void timerCallback() override { stopTimer(); if (down) { holding = true; if (onHold) onHold (true); } }
};

// A small black push button sunk in a chrome bezel (the tape echo's CLEAR).
class PushButton : public juce::Component
{
public:
    PushButton() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setSize (24, 24); }
    std::function<void()> onClick, onHover, onLeave;   // onLeave: the pointer has gone (hides the hint)
    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const float r = 9.0f, b = down ? 6.0f : 6.6f;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - r, c.y - r + 1.4f, 2 * r, 2 * r);
        juce::ColourGradient bezel (juce::Colour (0xffe6e8ea), c.x - r, c.y - r, juce::Colour (0xff53565b), c.x + r, c.y + r, false);
        g.setGradientFill (bezel);
        g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
        juce::ColourGradient cap (juce::Colour (0xff3a3836), c.x - b * 0.5f, c.y - b * 0.6f, juce::Colour (0xff050505), c.x + b, c.y + b, true);
        g.setGradientFill (cap);
        g.fillEllipse (c.x - b, c.y - b + (down ? 0.6f : 0.0f), 2 * b, 2 * b);
        g.setColour (hover ? col::ledAmber : juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (c.x - b, c.y - b + (down ? 0.6f : 0.0f), 2 * b, 2 * b, hover ? 1.3f : 0.8f);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); if (onLeave) onLeave(); }
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { down = false; repaint(); if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
private:
    bool hover = false, down = false;
};

// ---- a pedal ---------------------------------------------------------------------------------------------------------
// 192 × 134: arrows and the IN / CV / OUT jacks on the top edge (the jacks themselves are drawn here; the cable layer
// plugs into them), the display, two knobs, the LED and the footswitch, the name at the bottom. Dragging the
// enclosure moves the pedal along the chain.
class PedalUnit : public juce::Component
{
public:
    static constexpr int kW = 192, kH = 134;
    static constexpr float kJackY = 20.0f, kJackX[3] = { 58.0f, 96.0f, 134.0f };
    static constexpr float kKnobX[2] = { 30.0f, 76.0f }, kRowY = 105.0f, kNamesY = 84.0f, kFootX = 162.0f, kClearX = 127.0f;

    PedalUnit (BabushkaProcessor& p, int pedalIndex)
        : lcd (pedalLook (pedalIndex).lcd), processor (p), pedal (pedalIndex), look (pedalLook (pedalIndex)), info (babushka::pedalInfo()[(size_t) pedalIndex])
    {
        setSize (kW, kH);
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        for (int i = 0; i < 2; ++i)
        {
            knobs[(size_t) i] = std::make_unique<Knob> (p.apvts, info.knobs[(size_t) i].id, 24.0f);
            knobs[(size_t) i]->setCentrePosition ((int) kKnobX[i], (int) kRowY);
            bindEchoTime (*knobs[(size_t) i], p);
            addAndMakeVisible (*knobs[(size_t) i]);
        }
        led.setCentrePosition ((int) kFootX, (int) kNamesY);
        addAndMakeVisible (led);
        foot.setCentrePosition ((int) kFootX, (int) kRowY);
        addAndMakeVisible (foot);
        lcd.setBounds (10, 45, kW - 20, 30);
        addAndMakeVisible (lcd);
        leftArrow.setTopLeftPosition (6, 11);
        rightArrow.setTopLeftPosition (kW - 24, 11);
        addAndMakeVisible (leftArrow);
        addAndMakeVisible (rightArrow);
        foot.onToggle = [this] { setPedalParam (processor, info.onId, pedalValue (processor, info.onId) > 0.5f ? 0.0f : 1.0f); refresh(); };
        if (pedal == 3)
        {
            foot.onHold = [this] (bool held) { setPedalParam (processor, "tapHold", held ? 1.0f : 0.0f, false); refresh(); };
            clear.setCentrePosition ((int) kClearX, (int) kRowY);
            addAndMakeVisible (clear);
            clear.onClick = [this] { processor.clearEcho(); };
        }
    }

    void setHints (HintSink* h)
    {
        hints = h;
        for (auto& k : knobs) k->hints = h;
        foot.onHover = [this] { if (hints) hints->showHint (foot, pedal == 3 ? "Footswitch: on / off  -  hold it down: HOLD (the repeats loop)" : "Footswitch: on / off"); };
        lcd.onHover = [this] { if (hints) hints->showHint (lcd, juce::String (info.name) + ": presets and every setting"); };
        leftArrow.onHover = [this] { if (hints) hints->showHint (leftArrow, "Earlier in the chain (or drag the pedal)"); };
        rightArrow.onHover = [this] { if (hints) hints->showHint (rightArrow, "Later in the chain (or drag the pedal)"); };
        clear.onHover = [this] { if (hints) hints->showHint (clear, "CLEAR: erase the tape - stops the repeats at once"); };
        auto hide = [this] { if (hints) hints->hideHint(); };
        foot.onLeave = hide; lcd.onLeave = hide; leftArrow.onLeave = hide; rightArrow.onLeave = hide; clear.onLeave = hide;
    }

    PedalArrow leftArrow { true }, rightArrow { false };
    PedalLcd lcd;
    std::function<void (int, float)> onDragMove;   // pedal, x of its left edge in the parent
    std::function<void (int)> onDragEnd;

    // Called by the rear panel's timer: the display, the LED.
    void refresh()
    {
        const bool on = pedalValue (processor, info.onId) > 0.5f;
        const bool hold = pedal == 3 && pedalValue (processor, "tapHold") > 0.5f;
        blink = (blink + 1) % 16;
        led.setLevel (on ? (hold && blink >= 8 ? 0.35f : 1.0f) : 0.0f);
        auto name = PedalPresets::match (processor, pedal).toUpperCase();
        if (name.isEmpty()) name = pedalModeText (processor, pedal);
        lcd.set (name, hold ? juce::String ("HOLD") : on ? pedalReadout (processor, pedal) : juce::String ("BYPASS"),
                 processor.isPedalLocked (pedal), on);
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f, 0.0f).withTrimmedBottom (2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));                                            // shadow on the board
        g.fillRoundedRectangle (r.translated (0.0f, 2.5f), 9.0f);
        // die-cast box in smooth enamel: a gloss from the top-left light, the face a few pixels inside the rounded edge,
        // worn corners, four screws
        juce::ColourGradient body (look.top.brighter (0.08f), 0.0f, r.getY(), look.bottom, 0.0f, r.getBottom(), false);
        g.setGradientFill (body);
        juce::Path box; box.addRoundedRectangle (r, 9.0f);
        g.fillPath (box);
        enamel (g, box, r);
        drawWear (g, r, true, 1.4f);
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.45f), 0.0f, r.getY(), juce::Colours::black.withAlpha (0.35f), 0.0f, r.getBottom(), false);
        g.setGradientFill (edge);
        g.drawRoundedRectangle (r.reduced (0.8f), 8.5f, 1.6f);
        g.setColour (juce::Colours::black.withAlpha (0.16f));
        g.drawRoundedRectangle (r.reduced (4.0f), 6.0f, 1.0f);                                           // the face inside the cast edge
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawRoundedRectangle (r.reduced (4.0f).translated (0.0f, 1.0f), 6.0f, 0.8f);
        for (auto c : { juce::Point<float> (r.getX() + 6.0f, r.getY() + 6.0f), juce::Point<float> (r.getRight() - 6.0f, r.getY() + 6.0f),
                        juce::Point<float> (r.getX() + 6.0f, r.getBottom() - 6.0f), juce::Point<float> (r.getRight() - 6.0f, r.getBottom() - 6.0f) })
            drawScrew (g, c, 2.8f, 0.6f + c.x * 0.37f);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawRoundedRectangle (r.reduced (0.3f), 9.0f, 0.8f);
        if (dragging)
        {
            g.setColour (col::ledAmber.withAlpha (0.7f));
            g.drawRoundedRectangle (r.reduced (1.0f), 9.0f, 1.6f);
        }
        const juce::String names[] = { "IN", info.cvJackLabel, "OUT" };
        for (int i = 0; i < 3; ++i)
        {
            drawJack (g, { kJackX[i], kJackY }, 9.5f);
            drawJackName (g, names[i], { kJackX[i], 38.0f }, 36.0f, i == 2 ? col::inkDark : look.ink, type::jack, i == 2);
        }
        for (int i = 0; i < 2; ++i)
            label (g, info.knobs[(size_t) i].name, juce::Rectangle<float> (48.0f, 14.0f).withCentre ({ kKnobX[i], kNamesY }), look.ink, type::control,
                   Weight::NarrowBold, juce::Justification::centred, 0.03f);
        if (pedal == 3)
            label (g, "CLEAR", juce::Rectangle<float> (40.0f, 14.0f).withCentre ({ kClearX, kNamesY }), look.ink, type::control, Weight::NarrowBold,
                   juce::Justification::centred, 0.03f);
        label (g, info.name, juce::Rectangle<float> (r.getWidth() - 16.0f, 16.0f).withCentre ({ r.getCentreX(), 124.0f }), look.ink, type::title,
               Weight::NarrowBold, juce::Justification::centred, 0.12f);
    }

    void mouseDown (const juce::MouseEvent& e) override { grabX = e.getMouseDownX(); dragging = false; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging && std::abs (e.getDistanceFromDragStartX()) < 5) return;
        if (! dragging) { dragging = true; toFront (false); repaint(); }
        if (onDragMove) onDragMove (pedal, (float) (e.getEventRelativeTo (getParentComponent()).x - grabX));
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (! dragging) return;
        dragging = false; repaint();
        if (onDragEnd) onDragEnd (pedal);
    }
    int index() const { return pedal; }
    std::vector<Knob*> knobControls() const { return { knobs[0].get(), knobs[1].get() }; }
    bool isDragging() const { return dragging; }

private:
    BabushkaProcessor& processor;
    int pedal;
    const PedalLook& look;
    const babushka::PedalInfo& info;
    HintSink* hints = nullptr;
    std::array<std::unique_ptr<Knob>, 2> knobs;
    Lamp led { juce::Colour (0xffff3b2a), 7.0f };
    Footswitch foot;
    PushButton clear;                              // tape echo only
    int grabX = 0, blink = 0;
    bool dragging = false;
};

// ---- the FX lock of the preset block: a square key with a padlock, "FX" under it ---------------------------------------
class FxLockButton : public juce::Component
{
public:
    FxLockButton() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick, onHover, onLeave;   // onLeave: the pointer has gone (hides the hint)
    void setLocked (bool l) { if (l != locked) { locked = l; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        const auto key = all.withHeight (all.getWidth()).reduced (1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (key.translated (0.0f, 1.5f), 5.0f);
        if (locked)
        {
            drawGlow (g, key.getCentre(), key.getWidth() * 0.9f, col::ledAmber, 0.35f);
            juce::ColourGradient lit (juce::Colour (0xffffc766), key.getX(), key.getY(), juce::Colour (0xffd98f1f), key.getX(), key.getBottom(), false);
            g.setGradientFill (lit);
        }
        else
        {
            juce::ColourGradient grad (juce::Colour (down ? 0xff1c1b1a : 0xff3a3836), key.getX(), key.getY(), juce::Colour (down ? 0xff2d2b29 : 0xff161514),
                                       key.getX(), key.getBottom(), false);
            g.setGradientFill (grad);
        }
        g.fillRoundedRectangle (key, 5.0f);
        g.setColour (locked ? juce::Colours::black.withAlpha (0.35f) : col::outline.withAlpha (hover ? 0.9f : 0.45f));
        g.drawRoundedRectangle (key, 5.0f, 1.0f);
        const auto icon = juce::Rectangle<float> (14.0f, 16.0f).withCentre (key.getCentre().translated (0.0f, down ? 1.0f : 0.0f));
        drawLockIcon (g, icon, locked ? juce::Colour (0xff161514) : col::ink.withAlpha (0.85f), locked, 2.0f);
        label (g, "FX", all.withTop (key.getBottom() + 3.0f), locked ? col::ledAmber : col::ink, type::control, Weight::NarrowBold,
               juce::Justification::centredTop, 0.14f);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); if (onHover) onHover(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); if (onLeave) onLeave(); }
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        down = false; repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
    }
private:
    bool locked = false, hover = false, down = false;
};
} // namespace babushka::ui
