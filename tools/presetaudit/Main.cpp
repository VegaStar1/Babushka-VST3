// Plays every factory preset the way a person trying it would, and reports its level and character.
//   babushka_presetaudit [--wav <dir>] [--tsv <file>]
//   babushka_presetaudit --export <dir>        writes the factory presets as <dir>/<category>/<name>.babushka
//   babushka_presetaudit --check-files <dir>   checks those files against Presets.h (and that every preset has tags)
//   babushka_presetaudit --random <n>          n RANDOMIZE rolls per kind: each must load, be finite, not clip, be audible
// Sustained presets: one held note (bass presets low, leads high), then the release tail. Arpeggiator presets: a held
// minor chord. Sequencer presets: run on their own. A preset fails when it is silent, clips, produces NaN/Inf or is
// far louder or quieter than the rest (loudness = the loudest 50 ms window). --wav writes every render as a WAV file,
// --tsv writes name / peak / loudness as tab-separated lines (used to balance the preset volumes).
#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Source/PluginProcessor.h"

namespace
{
struct Result { double peak = 0, rms = 0, loud = 0, tail = 0, zcr = 0; bool bad = false; };

int testNote (const babushka::FactoryPreset& p)
{
    const juce::String c (p.category);
    if (c.containsIgnoreCase ("bass")) return 48;      // C3, low on the 32-key keyboard
    if (c.containsIgnoreCase ("lead")) return 60;
    if (c.containsIgnoreCase ("drum") || c.containsIgnoreCase ("perc")) return 48;
    return 53;
}

float value (const babushka::FactoryPreset& p, const char* id, float def)
{
    for (auto& [k, v] : p.values) if (juce::String (k) == id) return v;
    return def;
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File wavDir, tsvFile, exportDir, checkDir;
    int randomRolls = 0;
    for (int a = 1; a + 1 < argc; a += 2)
    {
        const auto f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[a + 1]);
        if (juce::String (argv[a]) == "--wav") wavDir = f;
        if (juce::String (argv[a]) == "--tsv") tsvFile = f;
        if (juce::String (argv[a]) == "--export") exportDir = f;
        if (juce::String (argv[a]) == "--check-files") checkDir = f;
        if (juce::String (argv[a]) == "--random") randomRolls = juce::String (argv[a + 1]).getIntValue();
    }
    if (exportDir != juce::File())
    {
        BabushkaProcessor proc;
        exportDir.deleteRecursively();
        const int n = proc.writeFactoryPresetFiles (exportDir, true);
        std::printf ("wrote %d factory preset files to %s\n", n, exportDir.getFullPathName().toRawUTF8());
        return n == (int) babushka::factoryPresets().size() ? 0 : 1;
    }
    if (checkDir != juce::File())
    {
        BabushkaProcessor proc;
        const auto& all = babushka::factoryPresets();
        int bad = 0;
        for (int i = 0; i < (int) all.size(); ++i)
        {
            const auto& pr = all[(size_t) i];
            if (juce::String (babushka::factoryPresetTags (pr.name)).isEmpty()) { std::printf ("no tags: %s\n", pr.name); ++bad; }
            const auto file = checkDir.getChildFile (juce::File::createLegalFileName (pr.category)).getChildFile (juce::File::createLegalFileName (pr.name) + ".babushka");
            auto onDisk = juce::parseXML (file);
            auto expected = proc.factoryPresetXml (i);
            if (onDisk == nullptr || ! onDisk->isEquivalentTo (expected.get(), false)) { std::printf ("out of date: %s\n", file.getFullPathName().toRawUTF8()); ++bad; }
        }
        const int files = checkDir.findChildFiles (juce::File::findFiles, true, "*.babushka").size();
        if (files != (int) all.size()) { std::printf ("%d files in the folder, %d presets\n", files, (int) all.size()); ++bad; }
        std::printf ("factory preset files: %s\n", bad == 0 ? "up to date" : "FAIL (run babushka_presetaudit --export Presets/Factory)");
        return bad == 0 ? 0 : 1;
    }
    if (randomRolls > 0)
    {
        BabushkaProcessor proc;
        proc.setPlayConfigDetails (0, 2, 48000.0, 256);
        proc.prepareToPlay (48000.0, 256);
        const auto kinds = BabushkaProcessor::randomKindNames();
        int failures = 0;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < kinds.size(); ++k)
        {
            proc.setRandomKind ((BabushkaProcessor::RandomKind) k);
            double minLoud = 1e9, maxLoud = 0, maxPeak = 0; int notLoaded = 0, arps = 0, cabled = 0;
            for (int i = 0; i < randomRolls; ++i)
            {
                if (! proc.randomize ((uint32_t) (1 + k * 100000 + i))) { ++notLoaded; continue; }
                // Play the loaded sound through the real processor, the way the presets are audited.
                const bool arp = proc.apvts.getRawParameterValue ("arpPlay")->load() > 0.5f;
                const bool seq = arp && (int) proc.apvts.getRawParameterValue ("arpMode")->load() == 1;
                arps += arp ? 1 : 0; cabled += proc.getPatchCopy().count > 0 ? 1 : 0;
                proc.releaseResources(); proc.prepareToPlay (48000.0, 256);
                juce::AudioBuffer<float> buf (2, 256);
                double peak = 0, loud = 0; bool finite = true;
                std::vector<float> y;
                for (int b = 0; b < 300; ++b)
                {
                    juce::MidiBuffer midi;
                    if (b == 2 && ! seq) { if (arp) for (int n : { 48, 51, 55 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                                           else midi.addEvent (juce::MidiMessage::noteOn (1, 53, (juce::uint8) 100), 0); }
                    if (b == 230) midi.addEvent (juce::MidiMessage::allNotesOff (1), 0);
                    buf.clear(); proc.processBlock (buf, midi);
                    for (int s = 0; s < 256; ++s) { const float v = buf.getSample (0, s); finite = finite && std::isfinite (v); peak = std::max (peak, (double) std::abs (v)); y.push_back (v); }
                }
                for (size_t from = 0; from + 2400 <= y.size(); from += 1200)
                { double t = 0; for (size_t s = from; s < from + 2400; ++s) t += y[s] * y[s]; loud = std::max (loud, std::sqrt (t / 2400.0)); }
                minLoud = std::min (minLoud, loud); maxLoud = std::max (maxLoud, loud); maxPeak = std::max (maxPeak, peak);
                if (! finite || peak > 0.97 || loud < 0.05 || loud > 0.5)
                { ++failures; std::printf ("  FAIL %s roll %d: peak %.3f loud %.3f%s\n", kinds[k].toRawUTF8(), i, peak, loud, finite ? "" : " NaN"); }
            }
            std::printf ("%-8s %d rolls: loud %.3f..%.3f, peak max %.3f, arp/seq %d, with cables %d, not loaded %d\n",
                         kinds[k].toRawUTF8(), randomRolls, minLoud, maxLoud, maxPeak, arps, cabled, notLoaded);
            failures += notLoaded;
        }
        std::printf ("randomize: %.1f ms per roll, %d failing\n", (juce::Time::getMillisecondCounterHiRes() - t0) / (randomRolls * kinds.size()), failures);
        return failures == 0 ? 0 : 1;
    }
    if (wavDir != juce::File()) wavDir.createDirectory();
    juce::String tsv;
    constexpr double fs = 48000.0;
    constexpr int block = 256;
    const auto& all = babushka::factoryPresets();
    int failures = 0;
    std::vector<double> levels;
    std::printf ("%-12s %-28s %7s %7s %7s %7s %7s  %s\n", "category", "preset", "peak", "loud", "rms", "tail", "zcr Hz", "notes");
    for (int i = 0; i < (int) all.size(); ++i)
    {
        const auto& pr = all[(size_t) i];
        BabushkaProcessor proc;
        proc.setPlayConfigDetails (0, 2, fs, block);
        proc.prepareToPlay (fs, block);
        proc.applyFactoryPreset (i);
        const bool arp = value (pr, "arpPlay", 0) > 0.5f;
        const bool seq = arp && (int) value (pr, "arpMode", 0) == 1;
        const bool drone = (int) value (pr, "vcaMode", 0) == 2;
        const int note = testNote (pr);
        const double holdS = arp ? 4.0 : 2.0, tailS = 2.0;
        const int holdBlocks = (int) (holdS * fs / block), total = (int) ((holdS + tailS) * fs / block);
        juce::AudioBuffer<float> buf (2, block), all2 (1, total * block);
        Result r; double sum = 0; int n = 0, crossings = 0; float prev = 0;
        for (int b = 0; b < total; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 2 && ! seq)
            {
                if (arp) for (int k : { 48, 51, 55 }) midi.addEvent (juce::MidiMessage::noteOn (1, k, (juce::uint8) 100), 0);
                else midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            }
            if (b == holdBlocks)
            {
                if (arp) { for (int k : { 48, 51, 55 }) midi.addEvent (juce::MidiMessage::noteOff (1, k), 0);
                           proc.apvts.getParameter ("arpPlay")->setValueNotifyingHost (0.0f); }
                else midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
            }
            buf.clear();
            proc.processBlock (buf, midi);
            all2.copyFrom (0, b * block, buf, 0, 0, block);
            for (int s = 0; s < block; ++s)
            {
                const float v = buf.getSample (0, s);
                if (! std::isfinite (v)) r.bad = true;
                r.peak = std::max (r.peak, (double) std::abs (v));
                if (b > 4 && b < holdBlocks) { sum += v * v; ++n; if ((v >= 0) != (prev >= 0)) ++crossings; }
                prev = v;
            }
        }
        r.rms = std::sqrt (sum / std::max (1, n));
        for (int from = 0; from + 2400 <= all2.getNumSamples(); from += 1200)        // loudest 50 ms
        { double t = 0; for (int s = from; s < from + 2400; ++s) t += all2.getSample (0, s) * all2.getSample (0, s);
          r.loud = std::max (r.loud, std::sqrt (t / 2400.0)); }
        r.zcr = 0.5 * crossings / (std::max (1, n) / fs);
        { double t = 0; const int from = (total - (int) (0.3 * fs / block)) * block;
          for (int s = from; s < total * block; ++s) t += all2.getSample (0, s) * all2.getSample (0, s);
          r.tail = std::sqrt (t / (total * block - from)); }
        juce::String notes;
        if (r.bad) notes << "NaN/Inf ";
        const bool hits = juce::String (pr.category).containsIgnoreCase ("drum") || juce::String (pr.category) == "FX";
        if (r.loud < (hits ? 0.04 : 0.06)) notes << "TOO QUIET ";          // short hits and clicks measure lower
        if (r.peak > 0.97) notes << "CLIPS ";
        if (r.loud > 0.50) notes << "TOO LOUD ";
        if (! drone && r.tail > 0.05) notes << "(long tail) ";
        if (notes.containsAnyOf ("NTC")) ++failures;
        levels.push_back (r.loud);
        std::printf ("%-12s %-28s %7.3f %7.3f %7.3f %7.3f %7.0f  %s\n", pr.category, pr.name, r.peak, r.loud, r.rms, r.tail, r.zcr, notes.toRawUTF8());
        tsv << pr.name << "\t" << juce::String (r.peak, 4) << "\t" << juce::String (r.loud, 4) << "\n";
        if (wavDir != juce::File())
        {
            auto f = wavDir.getChildFile (juce::String (i).paddedLeft ('0', 3) + " " + juce::File::createLegalFileName (pr.name) + ".wav");
            f.deleteFile();
            std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
            juce::WavAudioFormat wav;
            if (auto* w = wav.createWriterFor (os.get(), fs, 1, 24, {}, 0)) { os.release(); std::unique_ptr<juce::AudioFormatWriter> wr (w); wr->writeFromAudioSampleBuffer (all2, 0, all2.getNumSamples()); }
        }
    }
    if (tsvFile != juce::File()) tsvFile.replaceWithText (tsv);
    std::sort (levels.begin(), levels.end());
    std::printf ("\n%d presets, median loudness (max 50 ms rms) %.3f, %d failing\n", (int) all.size(), levels[levels.size() / 2], failures);
    return failures == 0 ? 0 : 1;
}
