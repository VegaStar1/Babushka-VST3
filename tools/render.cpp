// Babushka — offline renderer and test harness for the DSP core (no JUCE needed).
//   render <outdir>
// Writes demo WAVs, raw test signals for analysis, and prints stability/CPU results.
#include "../Source/dsp/Engine.h"

#include <chrono>
#include <cstdio>
#include <functional>
#include <string>
#include <tuple>
#include <algorithm>
#include <vector>

using namespace babushka::dsp;

static void writeWav (const std::string& path, const std::vector<float>& x, int fs)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) { std::printf ("cannot write %s\n", path.c_str()); return; }
    const uint32_t dataBytes = (uint32_t) (x.size() * 4), riffSize = 36 + dataBytes;
    const uint16_t fmt = 3, ch = 1, bits = 32, align = 4;
    const uint32_t rate = (uint32_t) fs, byteRate = rate * 4, fmtSize = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riffSize, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtSize, 4, 1, f); std::fwrite (&fmt, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&rate, 4, 1, f); std::fwrite (&byteRate, 4, 1, f); std::fwrite (&align, 2, 1, f);
    std::fwrite (&bits, 2, 1, f); std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    std::fwrite (x.data(), 4, x.size(), f);
    std::fclose (f);
}

struct Ev { double t; int type; int note; float v; };   // type 0 = on, 1 = off, 2 = param-callback index

static std::vector<float> renderSeq (Params p, const std::vector<Ev>& evs, double seconds, int fs = 48000, int osf = 2,
                                     std::function<void (Params&, double)> automation = nullptr)
{
    Engine e; e.prepare (fs, osf); e.setParams (p);
    const int total = (int) (seconds * fs), block = 64;
    std::vector<float> out ((size_t) total, 0.0f);
    size_t ei = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        const double t = (double) pos / fs;
        while (ei < evs.size() && evs[ei].t <= t)
        {
            if (evs[ei].type == 0) e.noteOn (evs[ei].note); else e.noteOff (evs[ei].note);
            ++ei;
        }
        if (automation) { automation (p, t); e.setParams (p); }
        e.process (out.data() + pos, std::min (block, total - pos));
    }
    return out;
}

static std::vector<Ev> notes (std::initializer_list<std::tuple<double, double, int>> list)
{
    std::vector<Ev> v;
    for (auto& [on, len, n] : list) { v.push_back ({ on, 0, n, 0 }); v.push_back ({ on + len, 1, n, 0 }); }
    std::sort (v.begin(), v.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
    return v;
}

static void normalisePeak (std::vector<float>& x, float peak = 0.89f)
{
    float m = 0; for (float v : x) m = std::max (m, std::abs (v));
    if (m > 0) for (float& v : x) v *= peak / m;
}

int main (int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    const int fs = 48000;

    // ---------------- 1. Filter small-signal impulse responses (for frequency response) -------
    for (float res : { 0.0f, 0.5f, 0.9f })
    {
        LadderFilter lf; lf.prepare (96000.0f); lf.setThermalNoise (0.0f);
        std::vector<float> ir (16384);
        for (size_t i = 0; i < ir.size(); ++i) ir[i] = lf.process (i == 0 ? 0.001f : 0.0f, 1000.0f, 4.2f * res) * 1000.0f;
        char name[64]; std::snprintf (name, sizeof name, "/test_filter_ir_res%02d.wav", (int) (res * 100));
        writeWav (dir + name, ir, 96000);
    }

    // ---------------- 2. Self-oscillation pitch vs cutoff ---------------------------------------
    std::printf ("Self-oscillation (k = 4.2, fs = 96 kHz):\n");
    for (float fc : { 110.0f, 440.0f, 1760.0f, 5000.0f })
    {
        LadderFilter lf; lf.prepare (96000.0f);
        const int n = 96000 * 2;
        std::vector<float> y ((size_t) n);
        for (int i = 0; i < n; ++i) y[(size_t) i] = lf.process (0.0f, fc, 4.2f);
        // count rising zero crossings in last second with sub-sample interpolation
        int first = -1, count = 0; double tFirst = 0, tLast = 0; float amp = 0;
        for (int i = n / 2 + 1; i < n; ++i)
        {
            amp = std::max (amp, std::abs (y[(size_t) i]));
            if (y[(size_t) i - 1] < 0 && y[(size_t) i] >= 0)
            {
                const double t = i - 1 + y[(size_t) i - 1] / (y[(size_t) i - 1] - y[(size_t) i]);
                if (first < 0) { first = i; tFirst = t; } tLast = t; ++count;
            }
        }
        const double f = count > 1 ? (count - 1) / ((tLast - tFirst) / 96000.0) : 0.0;
        std::printf ("  cutoff %7.1f Hz -> oscillates at %8.2f Hz (%+6.1f cents), peak %.3f\n",
                     fc, f, f > 0 ? 1200.0 * std::log2 (f / fc) : 0.0, amp);
    }

    // ---------------- 3. Spring reverb impulse response ---------------------------------------
    {
        SpringReverb rv; rv.prepare ((float) fs);
        std::vector<float> ir ((size_t) fs * 4);
        for (size_t i = 0; i < ir.size(); ++i) ir[i] = rv.process (i == 0 ? 0.5f : 0.0f);
        writeWav (dir + "/test_spring_ir.wav", ir, fs);
    }

    // ---------------- 4. Musical demos ----------------------------------------------------------
    {   // Classic bass: two saws 16', slight detune, low cutoff, env sweep.
        Params p; p.osc1Octave = 1; p.osc1Wave = 1; p.osc2Octave = 0; p.osc2Wave = 1; p.osc2Freq = 0.08f;
        p.mixOsc1 = 0.6f; p.mixOsc2 = 0.6f; p.cutoff = 0.32f; p.resonance = 0.35f; p.envAmt = 0.45f;
        p.attack = 0.0f; p.decay = 0.35f; p.sustain = 0.45f; p.release = 0.2f; p.kbdTrack = 1;
        auto seq = notes ({ {0.0,0.2,36},{0.25,0.2,36},{0.5,0.2,48},{0.75,0.2,36},{1.0,0.2,39},{1.25,0.2,41},{1.5,0.4,43},
                            {2.0,0.2,36},{2.25,0.2,36},{2.5,0.2,48},{2.75,0.2,46},{3.0,0.6,43} });
        auto x = renderSeq (p, seq, 4.2); normalisePeak (x); writeWav (dir + "/demo_1_bass.wav", x, fs);
    }
    {   // Sync lead with glide, cutoff swept by hand, vibrato from the mod wheel, some reverb.
        Params p; p.osc1Octave = 2; p.osc1Wave = 1; p.osc2Octave = 1; p.osc2Wave = 1; p.osc2Sync = true; p.osc2Freq = 5.0f;
        p.mixOsc1 = 0.25f; p.mixOsc2 = 0.75f; p.cutoff = 0.72f; p.resonance = 0.25f; p.envAmt = 0.2f;
        p.decay = 0.5f; p.sustain = 0.8f; p.release = 0.3f; p.glide = 0.35f; p.glideType = 2;
        p.lfoRate = 0.42f; p.lfoPitchAmt = 0.18f; p.modWheel = 0.0f; p.reverbMix = 0.22f;
        auto seq = notes ({ {0.0,0.45,60},{0.5,0.45,63},{1.0,0.45,67},{1.5,0.9,72},{2.5,0.45,70},{3.0,1.2,67} });
        auto x = renderSeq (p, seq, 5.0, fs, 2, [] (Params& q, double t) {
            q.osc2Freq = 3.0f + 4.0f * (float) std::sin (t * 1.7);
            q.modWheel = t > 1.5 ? (float) std::min (1.0, (t - 1.5)) : 0.0f; });
        normalisePeak (x); writeWav (dir + "/demo_2_sync_lead.wav", x, fs);
    }
    {   // Self-oscillating filter played as a sine (key tracking 1:1), plus resonance sweep on a saw.
        Params p; p.mixOsc1 = 0.0f; p.resonance = 1.0f; p.kbdTrack = 2; p.cutoff = 0.66f; p.sustain = 1.0f; p.release = 0.3f;
        auto seq = notes ({ {0.0,0.4,48},{0.5,0.4,55},{1.0,0.4,60},{1.5,0.8,67} });
        auto x = renderSeq (p, seq, 2.6); normalisePeak (x); writeWav (dir + "/demo_3_self_osc.wav", x, fs);

        Params q; q.osc1Octave = 1; q.mixOsc1 = 0.6f; q.sustain = 1.0f; q.release = 0.2f;
        auto seq2 = notes ({ {0.0,5.5,36} });
        auto y = renderSeq (q, seq2, 6.0, fs, 2, [] (Params& r, double t) {
            r.cutoff = 0.25f + 0.5f * (float) (0.5 - 0.5 * std::cos (t * 1.2));
            r.resonance = (float) std::min (1.0, t / 5.0); });
        normalisePeak (y); writeWav (dir + "/demo_4_resonance_sweep.wav", y, fs);
    }
    {   // Mixer drive: same patch, mixer levels from clean to overdriven.
        Params p; p.osc1Octave = 1; p.osc2Octave = 0; p.osc2Freq = 7.0f; p.osc1Wave = 2; p.osc2Wave = 1;
        p.cutoff = 0.62f; p.resonance = 0.3f; p.sustain = 1.0f;
        auto seq = notes ({ {0.0,1.4,40},{1.5,1.4,40},{3.0,1.4,40} });
        auto x = renderSeq (p, seq, 4.6, fs, 2, [] (Params& q, double t) {
            const float lvl = t < 1.5 ? 0.4f : (t < 3.0 ? 0.7f : 1.0f); q.mixOsc1 = lvl; q.mixOsc2 = lvl; });
        normalisePeak (x); writeWav (dir + "/demo_5_mixer_drive.wav", x, fs);
    }
    {   // Spring reverb on plucks, 50 % mix.
        Params p; p.osc1Octave = 2; p.osc1Wave = 3; p.mixOsc1 = 0.6f; p.cutoff = 0.55f; p.envAmt = 0.5f; p.resonance = 0.2f;
        p.decay = 0.3f; p.sustain = 0.0f; p.release = 0.3f; p.reverbMix = 0.5f; p.kbdTrack = 1;
        auto seq = notes ({ {0.0,0.1,60},{0.6,0.1,67},{1.2,0.1,64},{1.8,0.1,72},{2.4,0.1,48} });
        auto x = renderSeq (p, seq, 5.5); normalisePeak (x); writeWav (dir + "/demo_6_spring.wav", x, fs);
    }
    {   // S/H-ish wobble: LFO square on cutoff with noise mixed in, KB RLS VCA mode.
        Params p; p.osc1Octave = 1; p.osc1Wave = 2; p.osc2Octave = 0; p.osc2Freq = -0.1f; p.osc2Wave = 1;
        p.mixOsc1 = 0.55f; p.mixOsc2 = 0.55f; p.mixNoise = 0.15f; p.cutoff = 0.42f; p.resonance = 0.55f;
        p.lfoRate = 0.38f; p.lfoWave = 0; p.lfoCutoffAmt = 0.45f; p.modWheel = 1.0f; p.vcaMode = 1; p.sustain = 0.9f;
        p.release = 0.35f; p.reverbMix = 0.15f;
        auto seq = notes ({ {0.0,1.8,36},{2.0,1.8,31} });
        auto x = renderSeq (p, seq, 4.4); normalisePeak (x); writeWav (dir + "/demo_7_lfo_wobble.wav", x, fs);
    }

    // ---------------- 5. Oscillator aliasing check: saw sweep, filter wide open ---------------
    {
        Params p; p.osc1Octave = 3; p.mixOsc1 = 0.3f; p.cutoff = 1.0f; p.envAmt = 0.4f; p.sustain = 1.0f; p.drift = 0.0f;
        p.glide = 0.95f; p.glideType = 1;
        auto seq = notes ({ {0.0,0.05,48},{0.1,3.9,96} });
        auto x = renderSeq (p, seq, 4.0); writeWav (dir + "/test_saw_sweep.wav", x, fs);
    }

    // ---------------- 5b. Engine-level: self-osc tracking 1:1 and reverb wet/dry level -------
    {
        auto pitchOf = [] (const std::vector<float>& y, int fsr) {
            int c = 0; double t0 = 0, t1 = 0;
            for (size_t i = y.size() / 2 + 1; i < y.size(); ++i)
                if (y[i - 1] < 0 && y[i] >= 0) { const double t = i - 1 + y[i - 1] / (y[i - 1] - y[i]); if (! c) t0 = t; t1 = t; ++c; }
            return c > 1 ? (c - 1) / ((t1 - t0) / fsr) : 0.0; };
        std::printf ("Self-oscillating filter, KBD TRACK 1:1, resonance max (engine):\n");
        double ref = 0;
        for (int note : { 48, 60, 72, 84 })
        {
            Params p; p.mixOsc1 = 0; p.resonance = 1.0f; p.kbdTrack = 2; p.cutoff = 0.6f; p.drift = 0;
            auto y = renderSeq (p, notes ({ { 0.0, 2.0, note } }), 2.0);
            const double f = pitchOf (y, fs); if (note == 48) ref = f;
            float pk = 0; for (size_t i = y.size() / 2; i < y.size(); ++i) pk = std::max (pk, std::abs (y[i]));
            std::printf ("  note %d: %.2f Hz, interval from note 48 = %+.1f cents vs ideal %d00, level %.3f\n",
                         note, f, 1200.0 * std::log2 (f / ref), note - 48, pk);
        }
        Params p; p.mixOsc1 = 0.6f; p.cutoff = 0.7f; p.sustain = 1.0f;
        auto rms = [] (const std::vector<float>& y) { double a = 0; for (size_t i = y.size() / 4; i < y.size(); ++i) a += y[i] * y[i]; return std::sqrt (a / (y.size() * 0.75)); };
        auto dry = renderSeq (p, notes ({ { 0.0, 3.0, 48 } }), 3.0);
        p.reverbMix = 1.0f;
        auto wet = renderSeq (p, notes ({ { 0.0, 3.0, 48 } }), 3.0);
        p.reverbMix = 0.0f; p.mixOsc1 = 1.0f; p.mixOsc2 = 1.0f; p.mixNoise = 1.0f; p.resonance = 0.8f; p.volume = 1.0f;
        auto hot = renderSeq (p, notes ({ { 0.0, 3.0, 36 } }), 3.0);
        float hp = 0; for (float v : hot) hp = std::max (hp, std::abs (v));
        std::printf ("Levels: dry rms %.3f, 100%% wet rms %.3f (ratio %.1f dB); hottest patch peak %.3f\n",
                     rms (dry), rms (wet), 20 * std::log10 (rms (wet) / rms (dry)), hp);
    }

    // ---------------- 5c. Arpeggiator / sequencer logic -------------------------------------
    {
        auto runArp = [&] (ArpSeq& a, int samples, std::vector<std::pair<int, ArpSeq::Event>>& log) {
            ArpSeq::Event ev[4];
            for (int i = 0; i < samples; ++i) { const int n = a.tick (ev); for (int k = 0; k < n; ++k) log.push_back ({ i, ev[k] }); }
        };
        auto onsList = [] (const std::vector<std::pair<int, ArpSeq::Event>>& log, int maxN) {
            std::string r; int c = 0;
            for (auto& [t, e] : log) if (e.type == 1 && c++ < maxN) r += std::to_string (e.note) + "@" + std::to_string (t) + " ";
            return r; };
        int failures = 0;
        auto expectNotes = [&] (const char* name, const std::vector<std::pair<int, ArpSeq::Event>>& log, std::vector<int> want) {
            std::vector<int> got; for (auto& [t, e] : log) if (e.type == 1) got.push_back (e.note);
            bool ok = got.size() >= want.size();
            for (size_t i = 0; ok && i < want.size(); ++i) ok = got[i] == want[i];
            std::printf ("  %-34s %s  %s\n", name, ok ? "OK  " : "FAIL", onsList (log, (int) want.size()).c_str());
            if (! ok) ++failures; };

        std::printf ("Arpeggiator / sequencer:\n");
        {   // ORDR, 2 octaves, 120 BPM, 1/8 → 12000 samples per step at 48 kHz (0.25 s)
            ArpSeq a; a.prepare (48000.0f);
            ArpSeqParams p; p.play = true; p.mode = 0; p.octSeq = 1; p.rate = ArpSeq::knobForBpm (120.0f); p.freeStep = 1;
            a.setParams (p); a.keyOn (60); a.keyOn (64); a.keyOn (67);
            std::vector<std::pair<int, ArpSeq::Event>> log; runArp (a, 48000 * 2, log);
            expectNotes ("ARP ORDR 2 oct", log, { 60, 64, 67, 72, 76, 79, 60, 64 });
            int t1 = -1, t2 = -1, off1 = -1, c = 0;
            for (auto& [t, e] : log) { if (e.type == 1) { if (c == 1) t1 = t; if (c == 2) t2 = t; ++c; } if (e.type == 2 && off1 < 0) off1 = t; }
            std::printf ("  step length %d samples (expect 12000), first gate off after %d (expect 6000)\n", t2 - t1, off1);
            if (std::abs ((t2 - t1) - 12000) > 1 || std::abs (off1 - 6000) > 1) ++failures;
        }
        {   // FWD/BKWD in pressed order 67, 60, 64
            ArpSeq a; a.prepare (48000.0f);
            ArpSeqParams p; p.play = true; p.mode = 0; p.direction = 1; p.rate = 0.9f; a.setParams (p);
            a.keyOn (67); a.keyOn (60); a.keyOn (64);
            std::vector<std::pair<int, ArpSeq::Event>> log; runArp (a, 48000 * 3, log);
            expectNotes ("ARP FWD/BKWD (pressed order)", log, { 67, 60, 64, 60, 67, 60, 64 });
        }
        {   // HOLD latch, then a new chord after releasing all keys starts a new pattern
            ArpSeq a; a.prepare (48000.0f);
            ArpSeqParams p; p.play = true; p.mode = 0; p.hold = true; p.rate = 0.9f; a.setParams (p);
            a.keyOn (48); a.keyOn (55); a.keyOff (48); a.keyOff (55);
            std::vector<std::pair<int, ArpSeq::Event>> log; runArp (a, 20000, log);
            a.keyOn (50); a.keyOff (50);
            runArp (a, 20000, log);
            expectNotes ("ARP HOLD + new pattern", log, { 48, 55, 48 });
            bool sawOld = false; int c = 0; for (auto& [t, e] : log) if (e.type == 1 && c++ >= 4 && e.note != 50) sawOld = true;
            std::printf ("  after new key only 50 plays: %s\n", sawOld ? "FAIL" : "OK");
            if (sawOld) ++failures;
        }
        {   // REC: 60, REST, 63 (+TIE), 67 accent; then SEQ transposed by +2
            ArpSeq a; a.prepare (48000.0f);
            ArpSeqParams p; p.mode = 2; p.octSeq = 0; a.setParams (p);
            a.keyOn (60); a.keyOff (60); a.recRest(); a.keyOn (63); a.keyOff (63); a.recTie(); a.recAccent(); a.keyOn (67); a.keyOff (67);
            std::printf ("  recorded length %d (expect 5)\n", a.currentLength());
            if (a.currentLength() != 5) ++failures;
            p.mode = 1; p.play = true; p.rate = ArpSeq::knobForBpm (120.0f); a.setParams (p);
            a.keyOn (62); a.keyOff (62);
            std::vector<std::pair<int, ArpSeq::Event>> log; runArp (a, 12000 * 10, log);
            expectNotes ("SEQ transposed +2", log, { 62, 65, 69, 62, 65, 69 });
            // The tie: 65 must sound across two steps (no note-off at the half-step of step 3).
            int on65 = -1, off65 = -1;
            for (auto& [t, e] : log) { if (e.type == 1 && e.note == 65 && on65 < 0) on65 = t; if (e.type == 2 && e.note == 65 && on65 >= 0 && off65 < 0) off65 = t; }
            std::printf ("  tied note length %d samples (expect 18000 = 1.5 steps)\n", off65 - on65);
            if (std::abs ((off65 - on65) - 18000) > 2) ++failures;
        }
        {   // Host sync: 1/16 at 120 BPM, transport running from bar start → 6000 samples per step
            ArpSeq a; a.prepare (48000.0f);
            ArpSeqParams p; p.play = true; p.mode = 0; p.sync = true; p.rate = 16.0f / 23.0f; a.setParams (p);
            a.setTransport (true, 0.0, 120.0);
            a.keyOn (60); a.keyOn (62);
            std::vector<std::pair<int, ArpSeq::Event>> log; runArp (a, 48000, log);
            std::vector<int> times; for (auto& [t, e] : log) if (e.type == 1) times.push_back (t);
            std::printf ("  host sync %s: onsets", ArpSeq::divisionName (ArpSeq::divisionIndex (p.rate)));
            bool ok = times.size() >= 4;
            for (size_t i = 0; i < std::min<size_t> (times.size(), 5); ++i) { std::printf (" %d", times[i]); if (i > 0 && std::abs (times[i] - (int) i * 6000) > 1) ok = false; }
            std::printf ("  %s\n", ok ? "OK" : "FAIL");
            if (! ok) ++failures;
        }
        std::printf ("  arp/seq tests: %s\n", failures == 0 ? "ALL PASSED" : "FAILURES");

        // Audio demo: arpeggio through the full engine
        Engine e; e.prepare (fs, 2);
        Params pp; pp.osc1Octave = 2; pp.osc1Wave = 1; pp.osc2Octave = 1; pp.osc2Freq = 7.02f; pp.mixOsc1 = 0.6f; pp.mixOsc2 = 0.4f;
        pp.cutoff = 0.5f; pp.resonance = 0.55f; pp.envAmt = 0.45f; pp.decay = 0.25f; pp.sustain = 0.2f; pp.release = 0.2f;
        pp.reverbMix = 0.3f; pp.kbdTrack = 1;
        e.setParams (pp);
        ArpSeqParams ap; ap.play = true; ap.mode = 0; ap.octSeq = 2; ap.direction = 1; ap.rate = ArpSeq::knobForBpm (132.0f); ap.freeStep = 2;
        e.setArpParams (ap);
        std::vector<float> x ((size_t) fs * 6, 0.0f);
        e.noteOn (48); e.noteOn (51); e.noteOn (55);
        for (int pos = 0; pos < (int) x.size(); pos += 64)
        {
            if (pos == fs * 3) { e.noteOff (51); e.noteOn (53); }
            if (pos == fs * 5) { e.noteOff (48); e.noteOff (53); e.noteOff (55); }
            pp.cutoff = 0.42f + 0.2f * (float) std::sin (pos / (double) fs * 1.3); e.setParams (pp);
            e.process (x.data() + pos, std::min (64, (int) x.size() - pos));
        }
        normalisePeak (x); writeWav (dir + "/demo_8_arpeggio.wav", x, fs);
    }

    // ---------------- 6. Fuzz / stability: random parameter jumps -----------------------------
    {
        Engine e; e.prepare (fs, 2); Rng r; r.seed (1234);
        std::vector<float> buf (256);
        double maxAbs = 0; bool bad = false;
        for (int blk = 0; blk < fs * 60 / 256; ++blk)
        {
            if (blk % 20 == 0)
            {
                Params p;
                p.osc1Octave = (int) (r.unipolar() * 4); p.osc2Octave = (int) (r.unipolar() * 4);
                p.osc1Wave = (int) (r.unipolar() * 4); p.osc2Wave = (int) (r.unipolar() * 4);
                p.osc2Freq = r.bipolar() * 7; p.osc2Sync = r.unipolar() > 0.5f;
                p.mixOsc1 = r.unipolar(); p.mixOsc2 = r.unipolar(); p.mixNoise = r.unipolar();
                p.lfoRate = r.unipolar(); p.lfoWave = (int) (r.unipolar() * 4);
                p.lfoPitchAmt = r.unipolar(); p.lfoCutoffAmt = r.unipolar(); p.lfoPwAmt = r.unipolar(); p.modWheel = r.unipolar();
                p.cutoff = r.unipolar(); p.resonance = r.unipolar(); p.kbdTrack = (int) (r.unipolar() * 3); p.envAmt = r.bipolar();
                p.attack = r.unipolar(); p.decay = r.unipolar(); p.sustain = r.unipolar(); p.release = r.unipolar();
                p.vcaMode = (int) (r.unipolar() * 3); p.reverbMix = r.unipolar(); p.volume = 1.0f;
                p.glide = r.unipolar() * 0.5f; p.glideType = (int) (r.unipolar() * 3);
                e.setParams (p);
                if (r.unipolar() > 0.5f) e.noteOn (24 + (int) (r.unipolar() * 84)); else e.allNotesOff();
                e.setPitchBend (r.bipolar());
            }
            e.process (buf.data(), (int) buf.size());
            for (float v : buf) { if (! std::isfinite (v)) bad = true; maxAbs = std::max (maxAbs, (double) std::abs (v)); }
        }
        std::printf ("Fuzz 60 s random patches: %s, max |out| = %.3f\n", bad ? "NaN/Inf FOUND" : "no NaN/Inf", maxAbs);
    }

    // ---------------- 7. CPU ------------------------------------------------------------------
    for (int osf : { 2, 4 })
    {
        Params p; p.mixOsc1 = 0.7f; p.mixOsc2 = 0.7f; p.osc2Sync = true; p.mixNoise = 0.2f; p.resonance = 0.8f; p.reverbMix = 0.4f;
        Engine e; e.prepare (fs, osf); e.setParams (p); e.noteOn (48);
        std::vector<float> buf (512);
        const int blocks = fs * 20 / 512;
        auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < blocks; ++b) e.process (buf.data(), 512);
        const double el = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("CPU %dx oversampling: 20 s audio in %.3f s  (%.2f%% of one core at 48 kHz)\n", osf, el, 100.0 * el / 20.0);
    }
    return 0;
}
