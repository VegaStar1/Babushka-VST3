// Renders the editor to PNG files so the look of the interface can be checked without a host.
//   babushka_uishot <outdir>
//   babushka_uishot --icons <dir>     renders the Standalone app icons (app_icon_big.png 1024 px, app_icon_small.png
//                                     512 px with thicker strokes for 16-32 px use) from Source/ui/art/nuwmun_logo.svg
#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Source/PluginProcessor.h"
#include "../../Source/PluginEditor.h"
#include "../../Source/ui/Overlays.h"

namespace
{
void save (juce::Component& c, const juce::File& f)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

// Steps through a few UI states on the message thread (timers and the cable physics need a running loop).
struct Runner : private juce::Timer
{
    Runner (BabushkaProcessor& p, BabushkaEditor& e, juce::File o) : proc (p), ed (e), out (o) { startTimer (100); }
    BabushkaProcessor& proc; BabushkaEditor& ed; juce::File out; int tick = 0;

    void midiWheels (int bend, int mod)
    {
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::pitchWheel (1, bend), 0);
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, mod), 0);
        proc.processBlock (buf, midi);
    }
    // ~100 ms of audio with a held note, so the VU meter and the scope have something to show
    void play (int note)
    {
        juce::AudioBuffer<float> buf (2, 480);
        for (int i = 0; i < 10; ++i)
        {
            juce::MidiBuffer midi;
            if (i == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            proc.processBlock (buf, midi);
        }
    }
    void load (const char* name) { for (auto& r : proc.listPresets()) if (r.name == name) proc.loadPreset (r); }

    void timerCallback() override
    {
        using J = babushka::dsp::Jack;
        auto& cl = ed.cableLayer();
        ++tick;
        if (tick == 8)  save (ed, out.getChildFile ("ui_default.png"));
        if (tick == 9)  load ("Random Notes");
        if (tick == 40) save (ed, out.getChildFile ("ui_patched.png"));
        // interaction: pull a cable out of OSC 1 WAVE OUT, let it swing, drop it into MIXER OSC 1 IN
        if (tick == 41) { load ("Init"); }
        if (tick == 50) { before = proc.getPatchCopy().count; cl.pressAt (cl.jackCentre (J::Osc1WaveOut), false); cl.dragTo ({ 620.0f, 590.0f }); }
        if (tick == 51) cl.dragTo ({ 560.0f, 600.0f });
        if (tick == 53) cl.dragTo ({ 700.0f, 520.0f });
        if (tick == 58) save (ed, out.getChildFile ("ui_dragging.png"));
        if (tick == 59) cl.dragTo (cl.jackCentre (J::MixOsc1In));
        if (tick == 60) cl.releaseAt (cl.jackCentre (J::MixOsc1In));
        if (tick == 90) { save (ed, out.getChildFile ("ui_connected.png")); std::printf ("cables: %d -> %d (expect +1)\n", before, proc.getPatchCopy().count); }
        // add a second cable, then pull the first one out and drop it on empty space
        if (tick == 91) { cl.pressAt (cl.jackCentre (J::ModWaveOut), false); cl.dragTo (cl.jackCentre (J::FilterCutoffIn)); cl.releaseAt (cl.jackCentre (J::FilterCutoffIn)); }
        if (tick == 120) { std::printf ("cables: %d (expect 2)\n", proc.getPatchCopy().count); cl.pressAt (cl.jackCentre (J::MixOsc1In), false); cl.dragTo ({ 400.0f, 800.0f }); }
        if (tick == 135) { cl.releaseAt ({ 400.0f, 800.0f }); std::printf ("cables after pulling one out: %d (expect 1)\n", proc.getPatchCopy().count); }
        if (tick == 136) { cl.pressAt (cl.jackCentre (J::FilterCutoffIn), true); std::printf ("cables after right-click removal: %d (expect 0)\n", proc.getPatchCopy().count); }
        // preset browser: all presets, a user folder (two user presets with tags), the SAVE AS dialog
        if (tick == 138)
        {
            load ("Classic Bass");
            proc.saveUserPreset ("My Acid Bass", "My Basses", { "RESONANT", "AGGRESSIVE" });
            load ("Warm Pad");
            proc.saveUserPreset ("Night Pad", "Track X", { "DARK", "EVOLVING" });
            proc.saveUserPreset ("Quick Sketch");
            load ("Init");
        }
        if (tick == 140) ed.showPresets (true);
        if (tick == 144) { save (ed, out.getChildFile ("ui_presets.png")); ed.presetBrowser()->selectBranch ("User"); }
        if (tick == 146) { save (ed, out.getChildFile ("ui_presets_user.png")); ed.presetBrowser()->openSaveDialog(); }
        if (tick == 148) { save (ed, out.getChildFile ("ui_presets_save.png")); ed.showPresets (false); ed.showSettings (true); }
        if (tick == 150) { save (ed, out.getChildFile ("ui_settings.png")); ed.showSettings (false); }
        // RANDOMIZE: the display shows the new, unsaved sound
        if (tick == 152) { const bool ok = proc.randomize (777); std::printf ("randomize: %s -> '%s'\n", ok ? "ok" : "FAILED", proc.getCurrentPresetName().toRawUTF8()); }
        if (tick == 156) save (ed, out.getChildFile ("ui_random.png"));
        // rear panel: half way (no animation), then fully out with a cable into a rear jack
        if (tick == 157) ed.synthPanel().setRearOpen (false, false);
        if (tick == 158) { ed.synthPanel().setRearOpen (true, true); }
        if (tick == 159) { save (ed, out.getChildFile ("ui_rear_sliding.png")); }
        // the other skin, with a few cables, then a small window (legibility of the print)
        if (tick == 166) { std::printf ("rear open: %s\n", ed.synthPanel().isRearOpen() ? "yes" : "no");
                           cl.pressAt (cl.jackCentre (J::ClockOut), false); cl.dragTo (cl.jackCentre (J::EnvTriggerIn)); cl.releaseAt (cl.jackCentre (J::EnvTriggerIn));
                           std::printf ("cable into CLOCK OUT: %d (expect 1)\n", proc.getPatchCopy().count); }
        if (tick == 170) load ("Two Saws");
        if (tick > 170 && tick <= 180) play (48);
        if (tick == 181) save (ed, out.getChildFile ("ui_rear_open.png"));
        if (tick == 182) { proc.clearCables(); ed.synthPanel().setRearOpen (false, false); }
        if (tick == 183) { skinBefore = babushka::ui::skinIndex(); ed.setSkin (1); load ("Random Notes");
                           for (auto* id : { "osc2Sync", "arpHold" }) proc.apvts.getParameter (id)->setValueNotifyingHost (1.0f); }
        if (tick == 186) midiWheels (8191 + 6000, 100);      // a MIDI keyboard's wheels: the panel's wheels must follow
        if (tick == 192) { save (ed, out.getChildFile ("ui_black.png")); midiWheels (8192, 0); }
        if (tick == 193) ed.setSize (900, 651);
        if (tick == 195) save (ed, out.getChildFile ("ui_small_black.png"));
        if (tick == 196) { ed.setSkin (0); }
        if (tick == 198) { save (ed, out.getChildFile ("ui_small.png")); ed.setSize (1500, 1085); ed.setSkin (skinBefore); }
        if (tick == 200) { std::printf ("ok\n"); juce::MessageManager::getInstance()->stopDispatchLoop(); }
    }
    int before = 0, skinBefore = 0;
};
}

// App icon: the NUWMUN logo in amber-yellow with a soft glow on a dark tile, as wide as most of the tile.
juce::Image renderIcon (int size, float thicken)
{
    const juce::String svg (BinaryData::nuwmun_logo_svg, (size_t) BinaryData::nuwmun_logo_svgSize);
    auto logo = juce::Drawable::parseSVGPath (svg.fromFirstOccurrenceOf (" d=\"", false, false).upToFirstOccurrenceOf ("\"", false, false));
    logo.setUsingNonZeroWinding (false);
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Graphics g (img);
    const float s = (float) size;
    juce::ColourGradient bg (juce::Colour (0xff292826), s * 0.5f, s * 0.42f, juce::Colour (0xff0d0d0c), 0.0f, 0.0f, true);
    g.setGradientFill (bg);
    g.fillAll();
    juce::Random rnd (7);
    for (int i = 0; i < size * size / 40; ++i)               // faint grain, as on the panel
    {
        g.setColour (juce::Colours::white.withAlpha (0.012f + 0.02f * rnd.nextFloat()));
        g.fillRect ((float) rnd.nextInt (size), (float) rnd.nextInt (size), 1.0f, 1.0f);
    }
    const auto lb = logo.getBounds();
    const float w = s * 0.84f, h = w * lb.getHeight() / lb.getWidth();
    logo.applyTransform (juce::RectanglePlacement (juce::RectanglePlacement::centred).getTransformToFit (lb, { (s - w) * 0.5f, (s - h) * 0.5f, w, h }));
    const juce::Colour yellow (0xffffe614);
    for (int i = 12; i >= 1; --i)                             // glow: wide, faint strokes under the letters
    {
        g.setColour (yellow.withAlpha (0.018f));
        g.strokePath (logo, juce::PathStrokeType (s * 0.0035f * (float) i, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (yellow);
    g.fillPath (logo);
    if (thicken > 0.0f) g.strokePath (logo, juce::PathStrokeType (s * thicken, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    return img;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "--icons")
    {
        const juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
        for (auto [name, size, thicken] : { std::make_tuple ("app_icon_big.png", 1024, 0.0f), std::make_tuple ("app_icon_small.png", 512, 0.012f) })
        {
            const auto f = dir.getChildFile (name);
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat().writeImageToStream (renderIcon (size, thicken), os);
            std::printf ("wrote %s\n", f.getFullPathName().toRawUTF8());
        }
        return 0;
    }
    const juce::File out (argc > 1 ? argv[1] : ".");
    out.createDirectory();
    BabushkaProcessor proc;
    const auto presetRoot = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("babushka_uishot_presets");
    presetRoot.deleteRecursively();
    proc.setPresetRoot (presetRoot);      // never touch the real preset folder
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (1500, 1085);
    auto* be = dynamic_cast<BabushkaEditor*> (ed.get());
    Runner runner (proc, *be, out);
    juce::MessageManager::getInstance()->runDispatchLoop();
    ed.reset();
    presetRoot.deleteRecursively();
    return 0;
}
