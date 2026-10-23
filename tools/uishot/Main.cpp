// Renders the editor to PNG files so the look of the interface can be checked without a host.
//   babushka_uishot <outdir>
//   babushka_uishot --icons <dir>     renders the Standalone app icons (app_icon_big.png 1024 px, app_icon_small.png
//                                     512 px) from Source/ui/art/app_icon.svg
#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Source/PluginProcessor.h"
#include "../../Source/PluginEditor.h"
#include "../../Source/ui/MidiUi.h"

namespace
{
void save (juce::Component& c, const juce::File& f, float scale = 1.0f)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

// Steps through a few UI states on the message thread (timers and the cable physics need a running loop).
struct Runner : private juce::Timer
{
    Runner (BabushkaProcessor& p, BabushkaEditor& e, juce::File o) : proc (p), ed (e), out (o) { startTimer (100); }
    BabushkaProcessor& proc; BabushkaEditor& ed; juce::File out; int tick = 0;

    void cc (int channel, int number, int value)
    {
        juce::AudioBuffer<float> buf (2, 64);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (channel, number, value), 0);
        proc.processBlock (buf, midi);
    }
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
        // preset browser by keyboard: opens on the folders at "All" (Init playing); down = next folder, right = into the
        // list (loads its first preset), down = next preset loaded; hearts and the FAVORITES folder; INIT from the top row
        if (tick == 151)
        {
            using K = juce::KeyPress;
            auto* pb = ed.presetBrowser();
            for (auto& r : proc.listPresets()) if (r.name == "Classic Bass" || r.name == "Warm Pad" || r.name == "Bass Arp") proc.setFavorite (r, true);
            ed.showPresets (true);
            pb->keyPressed (K (K::downKey)); pb->keyPressed (K (K::rightKey));
            const auto first = proc.getCurrentPresetName();
            pb->keyPressed (K (K::downKey)); pb->keyPressed (K (K::downKey));
            std::printf ("keys: right -> '%s', down down -> '%s' (expect the 1st and 3rd Arpeggios preset)\n", first.toRawUTF8(), proc.getCurrentPresetName().toRawUTF8());
            save (ed, out.getChildFile ("ui_presets_keys.png"));
            pb->keyPressed (K (K::leftKey)); pb->keyPressed (K (K::upKey)); pb->keyPressed (K (K::upKey));
            pb->keyPressed (K (K::rightKey));
            std::printf ("favorites: '%s' loaded from FAVORITES (expect 'Bass Arp', the first of them)\n", proc.getCurrentPresetName().toRawUTF8());
            save (ed, out.getChildFile ("ui_presets_favorites.png"));
            pb->keyPressed (K (K::leftKey)); pb->keyPressed (K (K::upKey)); pb->keyPressed (K (K::returnKey));
            std::printf ("INIT row: '%s' (expect 'Init')\n", proc.getCurrentPresetName().toRawUTF8());
            ed.showPresets (false);
            load ("Warm Pad");
            ed.showPresets (true);
            save (ed, out.getChildFile ("ui_presets_reopen.png"));
            ed.showPresets (false);
            for (auto& r : proc.listPresets()) proc.setFavorite (r, false);
        }
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
        if (tick == 182)
        {   // a loud sound: the CLIP lamp must see the output go over 0 dBFS
            const auto before = proc.getClipCount();
            proc.apvts.getParameter ("volume")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("mixOsc2")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("mixOsc1")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("cutoff")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("resonance")->setValueNotifyingHost (0.0f);
            play (36);
            std::printf ("clip: %u samples over full scale caught (expect > 0)\n", proc.getClipCount() - before);
        }
        if (tick == 183) { save (ed, out.getChildFile ("ui_clip.png")); proc.clearCables(); ed.synthPanel().setRearOpen (false, false); }
        if (tick == 184) { skinBefore = babushka::ui::skinIndex(); ed.setSkin (1); load ("Random Notes");
                           for (auto* id : { "osc2Sync", "arpHold" }) proc.apvts.getParameter (id)->setValueNotifyingHost (1.0f); }
        if (tick == 186) midiWheels (8191 + 6000, 100);      // a MIDI keyboard's wheels: the panel's wheels must follow
        if (tick == 192) { save (ed, out.getChildFile ("ui_black.png")); midiWheels (8192, 0); }
        if (tick == 193) ed.setSize (900, 653);
        if (tick == 195) save (ed, out.getChildFile ("ui_small_black.png"));
        if (tick == 196) { ed.setSkin (0); }
        if (tick == 198) { save (ed, out.getChildFile ("ui_small.png")); ed.setSize (1450, 1052); ed.setSkin (skinBefore); }
        // MIDI: the menu with the Oxygen MK V profile, MIDI learn on the panel, soft takeover
        if (tick == 199) { proc.midiMap.applyProfile (1); ed.synthPanel().setRearOpen (true, false); ed.showMidi (true); }
        if (tick == 202) { save (ed, out.getChildFile ("ui_midi.png")); ed.startMidiLearn(); }
        if (tick == 203) { proc.midiMap.startLearn ("hpCutoff"); cc (1, 22, 64); }
        if (tick == 204) proc.midiMap.startLearn ("lfoPitchAmt");
        if (tick == 206) { save (ed, out.getChildFile ("ui_midi_learn.png"));
                           std::printf ("learn: High Pass on CC %d (expect 22)\n", proc.midiMap.ccFor ("hpCutoff")); }
        if (tick == 207)
        {
            proc.midiMap.stopLearn();
            proc.midiMap.applyProfile (1);                            // Oxygen MK V: CC 44 = Cutoff
            auto* cut = proc.apvts.getParameter ("cutoff");
            cut->setValueNotifyingHost (0.6f);
            cc (1, 44, 10);  const float a = cut->getValue();       // far from 0.6: soft takeover ignores it
            cc (1, 44, 77);  const float b = cut->getValue();       // reaches the value: picked up
            cc (1, 44, 20);  const float c = cut->getValue();       // now it follows
            std::printf ("soft takeover: %.3f (expect 0.600), %.3f (expect 0.606), %.3f (expect 0.157)\n", a, b, c);
        }
        if (tick == 208) { proc.midiMap.stopLearn(); ed.synthPanel().setRearOpen (false, false);
                           if (auto* l = ed.midiLearn()) if (l->onDone) l->onDone(); }
        // pedals: three on, the echo first in the chain, a cable from the chorus OUT, the tape echo locked
        auto set = [this] (const char* id, float real) {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (id));
            rp->setValueNotifyingHost (rp->convertTo0to1 (real)); };
        if (tick == 209)
        {
            ed.showMidi (false);
            load ("Two Saws");
            ed.synthPanel().setRearOpen (true, false);
            for (auto* id : { "satOn", "choOn", "tapOn" }) set (id, 1.0f);
            set ("fxOrder", (float) babushka::pedalOrderIndex ({ 3, 0, 1, 2 }));
            proc.setFxLocks (1 << 3);
        }
        if (tick == 211) { cl.pressAt (cl.jackCentre (J::ChoOut), false); cl.dragTo (cl.jackCentre (J::FilterIn)); cl.releaseAt (cl.jackCentre (J::FilterIn));
                           std::printf ("cable from CHORUS OUT: %d (expect 1)\n", proc.getPatchCopy().count); }
        if (tick > 211 && tick <= 220) play (48);
        if (tick == 221) save (ed, out.getChildFile ("ui_pedals.png"));
        if (tick == 222)
        {   // FX lock: a preset load keeps the locked echo, resets the rest
            proc.clearCables();
            set ("tapTime", 0.9f);
            load ("Two Saws");
            std::printf ("locks: tape echo kept %s (expect yes), saturator reset %s (expect yes)\n",
                         proc.apvts.getRawParameterValue ("tapOn")->load() > 0.5f && std::abs (proc.apvts.getRawParameterValue ("tapTime")->load() - 0.9f) < 0.001f ? "yes" : "no",
                         proc.apvts.getRawParameterValue ("satOn")->load() < 0.5f ? "yes" : "no");
            proc.setFxLocks (BabushkaProcessor::kLockAllFx);
            set ("choOn", 1.0f);
            set ("fxOrder", 5.0f);
            load ("Two Saws");
            std::printf ("FX lock: chorus kept %s, order kept %s (expect yes yes)\n", proc.apvts.getRawParameterValue ("choOn")->load() > 0.5f ? "yes" : "no",
                         std::lround (proc.apvts.getRawParameterValue ("fxOrder")->load()) == 5 ? "yes" : "no");
            proc.setFxLocks (0);
            set ("fxOrder", 0.0f);
        }
        for (int p = 0; p < babushka::kNumPedals; ++p)
        {
            if (tick == 224 + 3 * p) ed.showPedal (p);
            if (tick == 226 + 3 * p)
            {
                auto name = juce::String (babushka::pedalInfo()[(size_t) p].name).toLowerCase().replaceCharacter (' ', '_');
                save (ed, out.getChildFile ("ui_pedal_" + name + ".png"));
                ed.showPedal (-1);
            }
        }
        if (tick == 237)
        {   // a pedal preset: applied, then recognised on the display; a user preset saved and found again
            const auto& list = babushka::ui::PedalPresets::list (proc, 3);
            babushka::ui::PedalPresets::apply (proc, 3, list[0]);
            std::printf ("pedal default: '%s' (expect 'Default')\n", babushka::ui::PedalPresets::match (proc, 3).toRawUTF8());
            babushka::ui::PedalPresets::apply (proc, 3, list[2]);
            std::printf ("pedal preset: '%s' (expect 'Dub Echo')\n", babushka::ui::PedalPresets::match (proc, 3).toRawUTF8());
            set ("tapFeedback", 0.5f);
            babushka::ui::PedalPresets::save (proc, 3, "My Echo");
            std::printf ("user pedal preset: '%s' (expect 'My Echo')\n", babushka::ui::PedalPresets::match (proc, 3).toRawUTF8());
            for (auto& e : babushka::ui::PedalPresets::list (proc, 3)) if (! e.factory) babushka::ui::PedalPresets::remove (proc, 3, e);
            ed.synthPanel().setRearOpen (false, false);
        }
        if (tick == 238)
        {   // a runaway echo: switched off its repeats die away; CLEAR stops them at once
            auto level = [this] (int blocks) {
                juce::AudioBuffer<float> buf (2, 480); float peak = 0.0f;
                for (int i = 0; i < blocks; ++i) { juce::MidiBuffer m; proc.processBlock (buf, m); peak = juce::jmax (peak, buf.getMagnitude (0, 480)); }
                return peak; };
            auto release = [this] { juce::AudioBuffer<float> buf (2, 64); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::allNotesOff (1), 0); proc.processBlock (buf, m); };
            load ("Init");
            play (60); release(); level (300);
            std::printf ("echo: baseline without the echo %.4f\n", level (10));
            set ("tapOn", 1.0f); set ("tapFeedback", 1.0f); set ("tapMix", 0.6f);
            play (60); release(); level (300);
            const float before = level (10);
            set ("tapOn", 0.0f); level (600);                          // 6 s
            const float after = level (10);
            set ("tapOn", 1.0f); play (60); release(); level (300);
            proc.clearEcho(); level (2);
            const float cleared = level (10);
            std::printf ("echo: runaway %.3f, 6 s after OFF %.4f (expect < 0.01), after CLEAR %.4f (expect < 0.01)\n", before, after, cleared);
            set ("tapOn", 0.0f);
        }
        // the window most people use (about 78 %), rear panel out: the print in its four sizes
        if (tick == 239) { ed.setSize (1160, 842); load ("Two Saws"); ed.synthPanel().setRearOpen (true, false); }
        if (tick == 241) { save (ed, out.getChildFile ("ui_mid_rear.png")); ed.setSize (1450, 1052); ed.synthPanel().setRearOpen (false, false); }
        // the output meters: L-R with a sound, then the drum turning to VU + scope, and back
        if (tick == 242) load ("Two Saws");
        if (tick > 242 && tick <= 250) play (48);
        if (tick == 250) { save (ed, out.getChildFile ("ui_meter_lr.png")); ed.synthPanel().outputMeters().setShowsVu (true); }
        if (tick == 251) save (ed, out.getChildFile ("ui_meter_turning.png"));
        if (tick > 251 && tick <= 256) play (48);
        if (tick == 256) { save (ed, out.getChildFile ("ui_meter_vu.png")); ed.synthPanel().outputMeters().setShowsVu (false, false); }
        // TAPE ECHO locked to the tempo at 1/2: the TIME knobs point at the synced time
        if (tick == 257) { ed.synthPanel().setRearOpen (true, false); ed.showPedal (3);
                           auto* sync = proc.apvts.getParameter ("tapSync"); sync->setValueNotifyingHost (sync->convertTo0to1 (5.0f)); }
        if (tick == 260) { save (ed, out.getChildFile ("ui_pedal_echo_sync.png"));
                           std::printf ("echo sync: %s\n", proc.apvts.getParameter ("tapSync")->getCurrentValueAsText().toRawUTF8()); }
        // retina and 4K: the same panel drawn at 2x and 3x
        if (tick == 262) { ed.showPedal (-1); ed.synthPanel().setRearOpen (false, false); load ("Two Saws"); }
        if (tick > 262 && tick <= 266) play (48);
        if (tick == 266) { save (ed, out.getChildFile ("ui_2x.png"), 2.0f); save (ed, out.getChildFile ("ui_3x.png"), 3.0f); ed.synthPanel().setRearOpen (true, false); }
        if (tick == 268) { save (ed, out.getChildFile ("ui_rear_2x.png"), 2.0f); ed.setSkin (1); }
        if (tick == 270) { save (ed, out.getChildFile ("ui_black_rear_2x.png"), 2.0f); ed.setSkin (0); ed.synthPanel().setRearOpen (false, false); }
        if (tick == 272) { std::printf ("ok\n"); juce::MessageManager::getInstance()->stopDispatchLoop(); }
    }
    int before = 0, skinBefore = 0;
};
}

// App icon: the NUWMUN logo in amber-yellow with a soft glow on a dark tile, as wide as most of the tile.
// The app icon is the NUWMUN bat on a light disc (Source/ui/art/app_icon.svg), drawn as it is; transparent around the disc.
juce::Image renderIcon (int size)
{
    juce::Image img (juce::Image::ARGB, size, size, true);
    if (auto xml = juce::XmlDocument::parse (juce::String (BinaryData::app_icon_svg, (size_t) BinaryData::app_icon_svgSize)))
        if (auto d = juce::Drawable::createFromSVG (*xml))
        {
            juce::Graphics g (img);
            d->drawWithin (g, { 0.0f, 0.0f, (float) size, (float) size }, juce::RectanglePlacement::centred, 1.0f);
        }
    return img;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "--icons")
    {
        const juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
        for (auto [name, size] : { std::make_pair ("app_icon_big.png", 1024), std::make_pair ("app_icon_small.png", 512) })
        {
            const auto f = dir.getChildFile (name);
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat().writeImageToStream (renderIcon (size), os);
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
    const auto midiMapFile = presetRoot.getSiblingFile ("babushka_uishot_midimap.xml");
    proc.midiMap.setFile (midiMapFile);   // nor the user's MIDI map
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (1450, 1052);
    auto* be = dynamic_cast<BabushkaEditor*> (ed.get());
    Runner runner (proc, *be, out);
    juce::MessageManager::getInstance()->runDispatchLoop();
    ed.reset();
    presetRoot.deleteRecursively();
    return 0;
}
