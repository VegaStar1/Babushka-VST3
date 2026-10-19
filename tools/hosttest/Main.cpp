// Headless host test: loads the built Babushka.vst3 like a DAW, plays MIDI, checks output,
// saves/restores state, opens the editor and writes a screenshot.
//   babushka_hosttest <path/to/Babushka.vst3> <screenshot.png>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Source/PluginProcessor.h"

static int fail (const juce::String& msg) { std::printf ("FAIL: %s\n", msg.toRawUTF8()); return 1; }

static int runTest (const juce::String& pluginPath, const juce::String& shotPath)
{
    juce::AudioPluginFormatManager fm;
    fm.addFormat (new juce::VST3PluginFormat());

    juce::OwnedArray<juce::PluginDescription> found;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (found, pluginPath);
    if (found.isEmpty()) return fail ("no plugin found in " + pluginPath);
    const auto& desc = *found[0];
    std::printf ("Found: %s by %s, instrument=%d, category=%s\n", desc.name.toRawUTF8(),
                 desc.manufacturerName.toRawUTF8(), (int) desc.isInstrument, desc.category.toRawUTF8());

    juce::String err;
    auto plugin = fm.createPluginInstance (desc, 48000.0, 512, err);
    if (plugin == nullptr) return fail ("instantiate: " + err);

    plugin->setPlayConfigDetails (0, 2, 48000.0, 512);
    plugin->prepareToPlay (48000.0, 512);
    std::printf ("Parameters: %d\n", plugin->getParameters().size());

    juce::AudioBuffer<float> buf (2, 512);
    auto renderSeconds = [&] (double seconds, std::function<void (juce::MidiBuffer&, int)> midiFn) {
        double peak = 0, sumsq = 0; int n = 0; bool bad = false;
        for (int b = 0; b < (int) (seconds * 48000 / 512); ++b)
        {
            juce::MidiBuffer midi; if (midiFn) midiFn (midi, b);
            buf.clear();
            plugin->processBlock (buf, midi);
            for (int i = 0; i < 512; ++i)
            {
                const float v = buf.getSample (0, i);
                if (! std::isfinite (v)) bad = true;
                peak = std::max (peak, (double) std::abs (v)); sumsq += v * v; ++n;
            }
        }
        return std::make_tuple (peak, std::sqrt (sumsq / std::max (1, n)), bad);
    };

    auto [p0, r0, b0] = renderSeconds (0.5, nullptr);
    std::printf ("Silence before notes: peak %.6f\n", p0);
    auto [p1, r1, b1] = renderSeconds (2.0, [] (juce::MidiBuffer& m, int b) {
        if (b == 0)  m.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 10);
        if (b == 40) m.addEvent (juce::MidiMessage::noteOn (1, 55, (juce::uint8) 100), 100);
        if (b == 80) { m.addEvent (juce::MidiMessage::noteOff (1, 48), 0); m.addEvent (juce::MidiMessage::noteOff (1, 55), 0); }
        if (b == 20) m.addEvent (juce::MidiMessage::pitchWheel (1, 12000), 0);
        if (b == 30) m.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), 0); });
    std::printf ("Playing notes: peak %.3f rms %.3f %s\n", p1, r1, b1 ? "NaN!" : "finite");
    if (b1 || p1 < 0.01) return fail ("no or invalid audio while playing");

    // Parameter change + state round-trip
    auto* cutoff = plugin->getParameters()[0];
    for (auto* prm : plugin->getParameters())
        if (prm->getName (64) == "Cutoff") cutoff = prm;
    cutoff->setValueNotifyingHost (0.123f);
    juce::MemoryBlock state; plugin->getStateInformation (state);
    cutoff->setValueNotifyingHost (0.9f);
    plugin->setStateInformation (state.getData(), (int) state.getSize());
    std::printf ("State round-trip: Cutoff restored to %.3f (expected 0.123), state %d bytes\n", cutoff->getValue(), (int) state.getSize());
    if (std::abs (cutoff->getValue() - 0.123f) > 1e-3f) return fail ("state restore");

    // Editor
    if (plugin->hasEditor())
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (plugin->createEditorIfNeeded());
        if (ed == nullptr) return fail ("editor");
        ed->setVisible (true);
        std::printf ("Hosted editor opened: %dx%d\n", ed->getWidth(), ed->getHeight());
        plugin->editorBeingDeleted (ed.get());
    }

    plugin->releaseResources();

    // In-process editor screenshot (a hosted VST3 view is a native child window and cannot be snapshotted).
    {
        BabushkaProcessor proc;
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out (shotPath); out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("Editor screenshot written (%dx%d)\n", ed->getWidth(), ed->getHeight());
    }
    std::printf ("ALL HOST TESTS PASSED\n");
    return 0;
}

int main (int argc, char** argv)
{
    if (argc < 3) { std::printf ("usage: babushka_hosttest <plugin.vst3> <shot.png>\n"); return 2; }
    juce::ScopedJuceInitialiser_GUI gui;
    const int rc = runTest (argv[1], argv[2]);
    return rc;
}
