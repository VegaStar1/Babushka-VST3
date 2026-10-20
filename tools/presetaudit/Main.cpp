// Plays every factory preset the way a person trying it would, and reports its level and character.
//   babushka_presetaudit [--wav <dir>] [--tsv <file>]
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
    juce::File wavDir, tsvFile;
    for (int a = 1; a + 1 < argc; a += 2)
    {
        const auto f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[a + 1]);
        if (juce::String (argv[a]) == "--wav") wavDir = f;
        if (juce::String (argv[a]) == "--tsv") tsvFile = f;
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
