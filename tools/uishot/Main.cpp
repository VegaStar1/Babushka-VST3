// Renders the editor to PNG files so the look of the interface can be checked without a host.
//   babushka_uishot <outdir>
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
        // the other skin, with a few cables, then a small window (legibility of the print)
        if (tick == 160) { skinBefore = babushka::ui::skinIndex(); ed.setSkin (1); load ("Random Notes");
                           for (auto* id : { "osc2Sync", "arpHold" }) proc.apvts.getParameter (id)->setValueNotifyingHost (1.0f); }
        if (tick == 190) save (ed, out.getChildFile ("ui_black.png"));
        if (tick == 191) ed.setSize (900, 651);
        if (tick == 194) save (ed, out.getChildFile ("ui_small_black.png"));
        if (tick == 195) { ed.setSkin (0); }
        if (tick == 198) { save (ed, out.getChildFile ("ui_small.png")); ed.setSize (1500, 1085); ed.setSkin (skinBefore); }
        if (tick == 200) { std::printf ("ok\n"); juce::MessageManager::getInstance()->stopDispatchLoop(); }
    }
    int before = 0, skinBefore = 0;
};
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
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
