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

    // Arpeggiator through the hosted VST3: hold a chord with PLAY on → separate note bursts.
    {
        juce::AudioProcessorParameter* play = nullptr;
        for (auto* prm : plugin->getParameters()) if (prm->getName (64) == "Arp/Seq Play") play = prm;
        if (play == nullptr) return fail ("no Arp/Seq Play parameter");
        play->setValueNotifyingHost (1.0f);
        cutoff->setValueNotifyingHost (0.6f);   // the state test above left the filter almost closed
        juce::Array<float> env;
        for (int b = 0; b < (int) (3.0 * 48000 / 512); ++b)
        {
            juce::MidiBuffer m;
            if (b == 0) { m.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);
                          m.addEvent (juce::MidiMessage::noteOn (1, 52, (juce::uint8) 100), 1);
                          m.addEvent (juce::MidiMessage::noteOn (1, 55, (juce::uint8) 100), 2); }
            buf.clear(); plugin->processBlock (buf, m);
            env.add (buf.getRMSLevel (0, 0, 512));
        }
        float mx = 0; for (auto v : env) mx = std::max (mx, v);
        std::printf ("  play param value seen by host: %.2f; rms per 100 ms:", play->getValue());
        for (int i = 0; i < env.size(); i += 9) std::printf (" %.2f", env[i]);
        std::printf ("\n");
        int bursts = 0; bool above = false;
        for (auto v : env) { if (! above && v > 0.5f * mx) { ++bursts; above = true; } else if (above && v < 0.2f * mx) above = false; }
        std::printf ("Arpeggiator (hosted): %d note bursts in 3 s (expect ~12 at 120 BPM 1/8)\n", bursts);
        if (bursts < 6) return fail ("arpeggiator did not step");
        play->setValueNotifyingHost (0.0f);
        juce::MidiBuffer off; for (int n : { 48, 52, 55 }) off.addEvent (juce::MidiMessage::noteOff (1, n), 0);
        buf.clear(); plugin->processBlock (buf, off);
    }

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

    // In-process: sequence recording survives save/load; external input reaches the output.
    {
        BabushkaProcessor rec;
        rec.setPlayConfigDetails (0, 2, 48000.0, 512);
        rec.prepareToPlay (48000.0, 512);
        rec.apvts.getParameter ("arpMode")->setValueNotifyingHost (1.0f);   // REC
        juce::AudioBuffer<float> b2 (2, 512);
        int t = 0;
        for (int n : { 60, 63, 67 })
        {
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            m.addEvent (juce::MidiMessage::noteOff (1, n), 200); b2.clear(); rec.processBlock (b2, m); ++t;
        }
        rec.requestRecRest(); { juce::MidiBuffer m; b2.clear(); rec.processBlock (b2, m); }
        std::printf ("Recorded sequence length: %d (expect 4)\n", rec.getCurrentSequenceLength());
        if (rec.getCurrentSequenceLength() != 4) return fail ("sequence recording");
        juce::MemoryBlock st; rec.getStateInformation (st);

        BabushkaProcessor loaded;
        loaded.setPlayConfigDetails (0, 2, 48000.0, 512);
        loaded.prepareToPlay (48000.0, 512);
        loaded.setStateInformation (st.getData(), (int) st.getSize());
        { juce::MidiBuffer m; b2.clear(); loaded.processBlock (b2, m); }
        std::printf ("Sequence after state reload: %d steps\n", loaded.getCurrentSequenceLength());
        if (loaded.getCurrentSequenceLength() != 4) return fail ("sequence state");

        // External input: enable the sidechain bus, silence the oscillators, VCA in DRONE.
        BabushkaProcessor ex;
        auto layout = ex.getBusesLayout();
        layout.inputBuses.getReference (0) = juce::AudioChannelSet::stereo();
        if (! ex.setBusesLayout (layout)) return fail ("cannot enable Ext In bus");
        ex.prepareToPlay (48000.0, 512);
        ex.apvts.getParameter ("mixOsc1")->setValueNotifyingHost (0.0f);
        ex.apvts.getParameter ("vcaMode")->setValueNotifyingHost (1.0f);   // DRONE
        ex.apvts.getParameter ("cutoff")->setValueNotifyingHost (0.9f);
        juce::AudioBuffer<float> io (2, 512);
        double peakOut = 0;
        for (int blk = 0; blk < 40; ++blk)
        {
            for (int i = 0; i < 512; ++i) { const float v = 0.5f * std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * (blk * 512 + i) / 48000.0);
                                            io.setSample (0, i, v); io.setSample (1, i, v); }
            juce::MidiBuffer m; ex.processBlock (io, m);
            if (blk > 10) peakOut = std::max (peakOut, (double) io.getMagnitude (0, 0, 512));
        }
        std::printf ("External input in DRONE mode: output peak %.3f\n", peakOut);
        if (peakOut < 0.05) return fail ("external input not audible");
    }

    // Patch panel: a cable changes the sound, and cables survive save / load.
    {
        using namespace babushka::dsp;
        BabushkaProcessor pp;
        pp.setPlayConfigDetails (0, 2, 48000.0, 512);
        pp.prepareToPlay (48000.0, 512);
        pp.apvts.getParameter ("vcaMode")->setValueNotifyingHost (1.0f);   // DRONE: sounds without a key
        juce::AudioBuffer<float> b3 (2, 512);
        auto level = [&] (BabushkaProcessor& proc) {
            double pk = 0;
            for (int blk = 0; blk < 40; ++blk) { juce::MidiBuffer m; b3.clear(); proc.processBlock (b3, m); if (blk > 20) pk = std::max (pk, (double) b3.getMagnitude (0, 0, 512)); }
            return pk; };
        const double open = level (pp);
        pp.addCable (Jack::AttenuatorOut, Jack::VcaAmtIn);                 // attenuator at centre = 0 V → VCA closed
        const double shut = level (pp);
        std::printf ("Patch panel: DRONE level %.3f, with ATTENUATOR OUT -> VCA AMT IN (0 V) %.4f\n", open, shut);
        if (open < 0.05 || shut > 0.01) return fail ("patch cable did not change the sound");
        juce::MemoryBlock st; pp.getStateInformation (st);

        BabushkaProcessor loaded;
        loaded.setPlayConfigDetails (0, 2, 48000.0, 512);
        loaded.prepareToPlay (48000.0, 512);
        loaded.setStateInformation (st.getData(), (int) st.getSize());
        const double shut2 = level (loaded);
        std::printf ("Patch panel after state reload: %d cable(s), level %.4f\n", loaded.getPatchCopy().count, shut2);
        if (loaded.getPatchCopy().count != 1 || shut2 > 0.01) return fail ("cables not restored from state");
        loaded.clearCables();
        const double reopened = level (loaded);
        std::printf ("Patch panel after CLEAR ALL: level %.3f\n", reopened);
        if (reopened < 0.05) return fail ("clearing cables did not restore the normalled path");
    }

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
