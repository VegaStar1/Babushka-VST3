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

    // ---------------- 3b. Calibration against recordings of the hardware ----------------------
    // Targets come from analysis of recorded Grandmother material (see docs/analysis/hardware-recordings.md).
    {
        // Spring tank: T60 per octave band, estimated like the recordings: band-pass, smoothed energy,
        // linear fit of the dB decay from 0.3 s after the peak down to 30 dB below it.
        SpringReverb rv; rv.prepare ((float) fs);
        Rng nr; nr.seed (1234);
        std::vector<float> wet ((size_t) fs * 14);
        for (size_t i = 0; i < wet.size(); ++i) wet[i] = rv.process (i < (size_t) fs / 20 ? 0.5f * nr.bipolar() : 0.0f);

        struct Band { float lo, hi, target, tolLo, tolHi; };
        const Band bands[] = { {  60,  120, 8.4f, 0.7f, 1.4f }, { 120,  240, 10.5f, 0.7f, 1.4f }, { 240,  480, 6.8f, 0.7f, 1.4f },
                               { 480,  960, 4.4f, 0.7f, 1.4f }, { 960, 1920,  3.6f, 0.7f, 1.4f }, { 1920, 3840, 2.5f, 0.7f, 1.4f },
                               { 3840, 7680, 1.1f, 0.7f, 1.8f } };
        std::printf ("Spring tank T60 per octave band (hardware recordings -> model):\n");
        int springFail = 0;
        for (const Band& b : bands)
        {
            // Two cascaded RBJ band-pass biquads (constant 0 dB peak), centre = geometric mean, Q = 1.4.
            const double f0 = std::sqrt ((double) b.lo * b.hi), w0 = 6.283185307179586 * f0 / fs, q = 1.4;
            const double al = std::sin (w0) / (2 * q), a0 = 1 + al;
            const double bb0 = al / a0, bb2 = -al / a0, aa1 = -2 * std::cos (w0) / a0, aa2 = (1 - al) / a0;
            std::vector<double> e (wet.size());
            double z1[2] = { 0, 0 }, z2[2] = { 0, 0 };
            for (size_t i = 0; i < wet.size(); ++i)
            {
                double v = wet[i];
                for (int s = 0; s < 2; ++s) { const double o = bb0 * v + z1[s]; z1[s] = -aa1 * o + z2[s]; z2[s] = bb2 * v - aa2 * o; v = o; }
                e[i] = v * v;
            }
            const double a = 1.0 - std::exp (-6.283185307179586 * 15.0 / fs);
            double s1 = 0, s2 = 0;
            for (double& x : e) { s1 += a * (x - s1); s2 += a * (s1 - s2); x = 10.0 * std::log10 (s2 + 1e-14); }
            size_t pk = 0; for (size_t i = 0; i < (size_t) (1.2 * fs); ++i) if (e[i] > e[pk]) pk = i;
            double sx = 0, sy = 0, sxx = 0, sxy = 0; int n = 0;
            for (size_t i = pk + (size_t) (0.3 * fs); i < std::min (e.size(), pk + (size_t) (4.0 * fs)); ++i)
                if (e[i] > e[pk] - 30.0) { const double t = (double) i / fs; sx += t; sy += e[i]; sxx += t * t; sxy += t * e[i]; ++n; }
            const double slope = n > 100 ? (n * sxy - sx * sy) / (n * sxx - sx * sx) : 0.0;
            const double t60 = slope < 0 ? -60.0 / slope : 99.0;
            const bool ok = t60 >= b.target * b.tolLo && t60 <= b.target * b.tolHi;
            std::printf ("  %5.0f-%5.0f Hz: recorded %5.1f s, model %5.1f s  %s\n", b.lo, b.hi, b.target, t60, ok ? "OK" : "FAIL");
            if (! ok) ++springFail;
        }
        std::printf ("  spring T60 calibration: %s\n", springFail == 0 ? "ALL PASSED" : "FAILURES");
    }
    {
        // AC coupling: a recorded 66 Hz square wave sags ≈17 % over a half-period (≈5.5 Hz single-pole equivalent).
        Params p; p.osc1Octave = 2; p.osc1Wave = 2; p.mixOsc1 = 0.35f; p.mixOsc2 = 0.0f; p.cutoff = 1.0f; p.resonance = 0.0f;
        p.vcaMode = 2; p.volume = 1.0f; p.drift = 0.0f; p.reverbMix = 0.0f;
        auto y = renderSeq (p, notes ({ { 0.0, 3.0, 36 } }), 3.0);
        double hi0 = 0, hi1 = 0; int nc = 0;
        std::vector<int> ed;
        for (size_t i = (size_t) fs * 2 + 1; i < y.size(); ++i) if (y[i - 1] < 0 && y[i] >= 0) ed.push_back ((int) i);
        for (size_t k = 0; k + 1 < ed.size(); ++k)
        {
            const int a = ed[k]; const double per = ed[k + 1] - a;
            hi0 += y[(size_t) (a + (int) (0.06 * per))]; hi1 += y[(size_t) (a + (int) (0.44 * per))]; ++nc;
        }
        const double ratio = nc > 0 ? hi1 / hi0 : 0.0, period = 1.0 / 65.406;
        const double corner = -std::log (ratio) / (0.38 * period) / 6.283185307179586;
        const bool ok = corner > 4.5 && corner < 6.5;
        std::printf ("Output coupling: square-wave sag %.3f over 0.38 period -> %.2f Hz equivalent (recorded ≈5.5 Hz)  %s\n",
                     ratio, corner, ok ? "OK" : "FAIL");
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

    // ---------------- 5d. Patch panel -------------------------------------------------------------
    {
        int failures = 0;
        auto check = [&] (const char* name, bool ok, const char* detail) {
            std::printf ("  %-44s %s  %s\n", name, ok ? "OK  " : "FAIL", detail);
            if (! ok) ++failures; };
        auto cables = [] (std::initializer_list<std::pair<Jack, Jack>> list) {
            PatchState ps; for (auto& [a, b] : list) ps.add (a, b); return ps; };
        auto run = [&] (const Params& p, const PatchState& ps, std::initializer_list<std::pair<int, int>> keys, double seconds,
                        std::vector<float>* probe = nullptr, Jack probeJack = Jack::KbOut, int vel = 100) {
            Engine e; e.prepare (fs, 2); e.setParams (p); e.setPatch (ps);
            const int total = (int) (seconds * fs);
            std::vector<float> out ((size_t) total);
            for (auto [note, v] : keys) e.noteOn (note, v > 0 ? v : vel);
            const int block = probe ? 1 : 64;
            for (int pos = 0; pos < total; pos += block)
            {
                e.process (out.data() + pos, std::min (block, total - pos));
                if (probe) probe->push_back (e.outputVolts (probeJack));
            }
            return out; };
        auto rmsOf = [] (const std::vector<float>& x, size_t from) {
            double a = 0; for (size_t i = from; i < x.size(); ++i) a += (double) x[i] * x[i]; return std::sqrt (a / (double) (x.size() - from)); };
        auto brightness = [&] (const std::vector<float>& x, size_t from) {      // rms of the first difference / rms
            double a = 0, b = 0; for (size_t i = from + 1; i < x.size(); ++i) { const double d = x[i] - x[i - 1]; a += d * d; b += (double) x[i] * x[i]; }
            return std::sqrt (a / std::max (b, 1.0e-12)); };
        auto pitchHz = [&] (const std::vector<float>& y) {
            int c = 0; double t0 = 0, t1 = 0;
            for (size_t i = y.size() / 2 + 1; i < y.size(); ++i)
                if (y[i - 1] < 0 && y[i] >= 0) { const double t = (double) i - 1 + y[i - 1] / (y[i - 1] - y[i]); if (! c) t0 = t; t1 = t; ++c; }
            return c > 1 ? (c - 1) / ((t1 - t0) / fs) : 0.0; };
        auto maxDiff = [] (const std::vector<float>& a, const std::vector<float>& b) {
            double m = 0; for (size_t i = 0; i < a.size(); ++i) m = std::max (m, (double) std::abs (a[i] - b[i])); return m; };
        char buf[160];

        std::printf ("Patch panel (41 points: %d inputs, %d outputs, 4 mult):\n", kNumInputs, kNumOutputs);
        check ("jack counts", kNumJacks == 41 && kNumInputs == 21 && kNumOutputs == 16, "21 + 16 + 4 mult");
        {   // serialisation
            PatchState a = cables ({ { Jack::Osc2WaveOut, Jack::HpfIn }, { Jack::HpfOut, Jack::MixOsc2In }, { Jack::KbOut, Jack::Mult1 }, { Jack::Mult2, Jack::ModRateIn } });
            const auto b = PatchState::fromString (a.toString());
            check ("save / load of cables", a.toString() == b.toString() && b.count == 4, a.toString().c_str());
        }

        Params base; base.osc1Octave = 2; base.osc1Wave = 1; base.mixOsc1 = 0.5f; base.cutoff = 0.7f; base.drift = 0.0f; base.sustain = 1.0f;
        const auto ref = run (base, PatchState(), { { 60, 0 } }, 1.0);

        // normalled connections re-made with cables must (almost) reproduce the hard-wired path
        {
            auto y = run (base, cables ({ { Jack::MixerOut, Jack::FilterIn } }), { { 60, 0 } }, 1.0);
            const double d = maxDiff (ref, y);
            std::snprintf (buf, sizeof buf, "max difference %.2e", d);
            check ("MIXER OUT -> FILTER IN equals normalled", d < 1.0e-4, buf);
        }
        {
            auto y = run (base, cables ({ { Jack::Osc1WaveOut, Jack::MixOsc1In } }), { { 60, 0 } }, 1.0);
            const double d = maxDiff (ref, y) / rmsOf (ref, 0);
            std::snprintf (buf, sizeof buf, "max difference %.2f of rms (AC coupling + 1 sample delay)", d);
            check ("OSC 1 WAVE OUT -> MIXER OSC 1 IN", d < 0.08, buf);
        }
        {
            auto y = run (base, cables ({ { Jack::GateOut, Jack::EnvTriggerIn } }), { { 60, 0 } }, 1.0);
            const double d = maxDiff (ref, y) / rmsOf (ref, 0);
            std::snprintf (buf, sizeof buf, "max difference %.3f of rms", d);
            check ("GATE OUT -> TRIGGER IN equals normalled", d < 0.03, buf);
        }
        {   // 1 V/oct: KB OUT (note 72 = +1 V) into OSC 1 PITCH IN through the mult doubles the pitch
            Params q = base; q.cutoff = 1.0f; q.mixOsc1 = 0.3f; q.osc1Wave = 0;
            const double f0 = pitchHz (run (q, PatchState(), { { 72, 0 } }, 1.5));
            const double f1 = pitchHz (run (q, cables ({ { Jack::KbOut, Jack::Mult1 }, { Jack::Mult2, Jack::Osc1PitchIn } }), { { 72, 0 } }, 1.5));
            std::snprintf (buf, sizeof buf, "%.1f Hz -> %.1f Hz (x%.3f)", f0, f1, f1 / f0);
            check ("KB OUT -> mult -> OSC 1 PITCH IN (1 V/oct)", std::abs (f1 / f0 - 2.0) < 0.01, buf);
        }
        {   // High pass patched into the mixer removes bass
            Params q = base; q.mixOsc1 = 0.0f; q.mixOsc2 = 0.5f; q.osc2Octave = 0; q.cutoff = 1.0f;
            const auto cab = cables ({ { Jack::Osc2WaveOut, Jack::HpfIn }, { Jack::HpfOut, Jack::MixOsc2In } });
            q.hpCutoff = 0.0f; const double lo = rmsOf (run (q, cab, { { 36, 0 } }, 1.0), fs / 2);
            q.hpCutoff = 0.8f; const double hi = rmsOf (run (q, cab, { { 36, 0 } }, 1.0), fs / 2);
            std::snprintf (buf, sizeof buf, "rms %.3f -> %.3f with the knob up", lo, hi);
            check ("HIGH PASS in the oscillator 2 path", hi < 0.5 * lo, buf);
        }
        {   // LFO into VCA AMT IN: tremolo in DRONE mode
            Params q = base; q.vcaMode = 2; q.lfoRate = 0.45f; q.lfoWave = 0;
            auto plain = run (q, PatchState(), { { 48, 0 } }, 2.0);
            auto trem = run (q, cables ({ { Jack::ModWaveOut, Jack::VcaAmtIn } }), { { 48, 0 } }, 2.0);
            auto spread = [&] (const std::vector<float>& x) {
                double lo = 1e9, hi = 0; const size_t w = (size_t) fs / 50;
                for (size_t i = (size_t) fs / 2; i + w < x.size(); i += w) { double a = 0; for (size_t k = 0; k < w; ++k) a += (double) x[i + k] * x[i + k]; a = std::sqrt (a / (double) w); lo = std::min (lo, a); hi = std::max (hi, a); }
                return hi / std::max (lo, 1.0e-9); };
            std::snprintf (buf, sizeof buf, "level ripple x%.1f (plain x%.2f)", spread (trem), spread (plain));
            check ("MOD WAVE OUT -> VCA AMT IN (tremolo)", spread (trem) > 4.0 && spread (plain) < 1.3, buf);
        }
        {   // Velocity -> cutoff
            Params q = base; q.cutoff = 0.35f; q.mixOsc1 = 0.5f;
            const auto cab = cables ({ { Jack::KbVelOut, Jack::FilterCutoffIn } });
            const double soft = brightness (run (q, cab, { { 48, 20 } }, 1.0), fs / 2), hard = brightness (run (q, cab, { { 48, 127 } }, 1.0), fs / 2);
            const double none = brightness (run (q, PatchState(), { { 48, 20 } }, 1.0), fs / 2);
            std::snprintf (buf, sizeof buf, "brightness: no cable %.3f, soft key %.3f, hard key %.3f", none, soft, hard);
            check ("KB VEL OUT -> CUTOFF IN", hard > 1.5 * soft, buf);
        }
        {   // Attenuator: centre = 0 V, +1 = +8 V, -1 = -8 V (input normalled to +8 V)
            std::vector<float> pr; Params q = base;
            q.attenuator = 1.0f; run (q, cables ({ { Jack::AttenuatorOut, Jack::FilterCutoffIn } }), { { 48, 0 } }, 0.05, &pr, Jack::AttenuatorOut);
            const float up = pr.back(); pr.clear();
            q.attenuator = -1.0f; run (q, cables ({ { Jack::AttenuatorOut, Jack::FilterCutoffIn } }), { { 48, 0 } }, 0.05, &pr, Jack::AttenuatorOut);
            const float dn = pr.back(); pr.clear();
            q.attenuator = 0.0f; run (q, cables ({ { Jack::AttenuatorOut, Jack::FilterCutoffIn } }), { { 48, 0 } }, 0.05, &pr, Jack::AttenuatorOut);
            std::snprintf (buf, sizeof buf, "%+.2f V / %+.2f V / %+.2f V at +1 / -1 / 0", up, dn, pr.back());
            check ("ATTENUATOR (bipolar, normalled to +8 V)", std::abs (up - 8.0f) < 0.01f && std::abs (dn + 8.0f) < 0.01f && std::abs (pr.back()) < 0.01f, buf);
        }
        {   // Keyboard outputs
            std::vector<float> g, v, k; Params q = base;
            run (q, cables ({ { Jack::GateOut, Jack::AttenuatorIn } }), { { 60, 127 } }, 0.05, &g, Jack::GateOut);
            run (q, cables ({ { Jack::KbVelOut, Jack::AttenuatorIn } }), { { 60, 64 } }, 0.05, &v, Jack::KbVelOut);
            run (q, cables ({ { Jack::KbOut, Jack::AttenuatorIn } }), { { 72, 0 } }, 0.05, &k, Jack::KbOut);
            std::snprintf (buf, sizeof buf, "gate %.1f V, velocity 64 -> %.2f V, note 72 -> %.2f V", g.back(), v.back(), k.back());
            check ("GATE / KB VEL / KB OUT levels", std::abs (g.back() - 8.0f) < 0.01f && std::abs (v.back() - 2.52f) < 0.02f && std::abs (k.back() - 1.0f) < 0.01f, buf);
        }
        {   // LFO rate follows the keyboard (1 V/oct) when KB OUT is patched into RATE IN
            std::vector<float> w0, w1; Params q = base; q.lfoRate = 0.55f;
            const auto cab = cables ({ { Jack::ModWaveOut, Jack::AttenuatorIn } });
            run (q, cab, {}, 3.0, &w0, Jack::ModWaveOut);
            const auto cab2 = cables ({ { Jack::ModWaveOut, Jack::AttenuatorIn }, { Jack::KbOut, Jack::ModRateIn } });
            std::vector<float> a, b2;
            run (q, cab2, { { 60, 0 } }, 3.0, &a, Jack::ModWaveOut);
            run (q, cab2, { { 72, 0 } }, 3.0, &b2, Jack::ModWaveOut);
            auto freq = [&] (const std::vector<float>& x) { int c = 0; for (size_t i = 1; i < x.size(); ++i) if (x[i - 1] < 0 && x[i] >= 0) ++c; return c / 3.0; };
            std::snprintf (buf, sizeof buf, "%.2f Hz (C4) -> %.2f Hz (C5), free-running %.2f Hz", freq (a), freq (b2), freq (w0));
            check ("KB OUT -> RATE IN (LFO tracks keyboard)", std::abs (freq (b2) / freq (a) - 2.0) < 0.05, buf);
        }
        {   // Clock out: 120 BPM, PPQN 2 -> 4 pulses per second
            Engine e; e.prepare (fs, 2); Params q = base; e.setParams (q); e.setPatch (cables ({ { Jack::ClockOut, Jack::AttenuatorIn } }));
            ArpSeqParams ap; ap.rate = ArpSeq::knobForBpm (120.0f); e.setArpParams (ap);
            std::vector<float> o (fs * 3); int edges = 0; bool high = false;
            for (int pos = 0; pos < (int) o.size(); ++pos) { e.process (o.data() + pos, 1); const bool h = e.outputVolts (Jack::ClockOut) > 2.5f; if (h && ! high) ++edges; high = h; }
            std::snprintf (buf, sizeof buf, "%d pulses in 3 s (expect 12)", edges);
            check ("CLOCK OUT (120 BPM, 2 PPQN)", edges >= 11 && edges <= 13, buf);
        }
        {   // Clock in / on-off in / reset in drive the arpeggiator
            ArpSeq a; a.prepare (48000.0f);
            ArpSeqParams ap; ap.play = false; ap.mode = 0; ap.freeStep = 1; ap.extPpqn = 2; a.setParams (ap);
            a.keyOn (60); a.keyOn (64); a.keyOn (67);
            a.setExternalControl (true, true, true);                // clock patched, ON/OFF high
            std::vector<int> notes, at; ArpSeq::Event ev[4];
            for (int i = 0; i < 48000; ++i)
            {
                if (i % 12000 == 100) a.externalClockEdge();         // 4 edges per second = 120 BPM at 2 PPQN
                const int n = a.tick (ev);
                for (int k = 0; k < n; ++k) if (ev[k].type == 1) { notes.push_back (ev[k].note); at.push_back (i); }
            }
            std::snprintf (buf, sizeof buf, "notes %d %d %d %d at %d %d %d %d", notes.size() > 0 ? notes[0] : 0, notes.size() > 1 ? notes[1] : 0,
                           notes.size() > 2 ? notes[2] : 0, notes.size() > 3 ? notes[3] : 0, at.size() > 0 ? at[0] : 0, at.size() > 1 ? at[1] : 0,
                           at.size() > 2 ? at[2] : 0, at.size() > 3 ? at[3] : 0);
            check ("CLOCK IN + ON/OFF IN start the arpeggiator", notes.size() == 4 && notes[0] == 60 && notes[1] == 64 && notes[2] == 67 && notes[3] == 60 && at[1] - at[0] == 12000, buf);

            ArpSeq b; b.prepare (48000.0f);
            ap.extStepAdvance = true; b.setParams (ap);
            b.keyOn (60); b.keyOn (64); b.keyOn (67);
            b.setExternalControl (true, true, true);
            std::vector<int> n2;
            for (int i = 0; i < 20000; ++i)
            {
                if (i == 100 || i == 5000 || i == 5100 || i == 9000) b.externalClockEdge();   // irregular edges
                if (i == 9500) b.resetPattern();
                if (i == 12000) b.externalClockEdge();
                const int n = b.tick (ev);
                for (int k = 0; k < n; ++k) if (ev[k].type == 1) n2.push_back (ev[k].note);
            }
            std::snprintf (buf, sizeof buf, "%zu steps: %d %d %d %d %d", n2.size(), n2.size() > 0 ? n2[0] : 0, n2.size() > 1 ? n2[1] : 0,
                           n2.size() > 2 ? n2[2] : 0, n2.size() > 3 ? n2[3] : 0, n2.size() > 4 ? n2[4] : 0);
            // 4 edges walk 60 64 67 60; RESET IN sends the pattern back to its first note, so the next edge plays 60 again
            check ("STEP-ADVANCE + RESET IN", n2.size() == 5 && n2[0] == 60 && n2[1] == 64 && n2[2] == 67 && n2[3] == 60 && n2[4] == 60, buf);
        }
        {   // Fuzz: random cables and parameters, 60 s
            Engine e; e.prepare (fs, 2); Rng r; r.seed (777);
            std::vector<float> o (256); double maxAbs = 0; bool bad = false;
            for (int blk = 0; blk < fs * 60 / 256; ++blk)
            {
                if (blk % 40 == 0)
                {
                    PatchState ps; const int n = (int) (r.unipolar() * 12);
                    for (int i = 0; i < n; ++i) ps.add ((Jack) (int) (r.unipolar() * kNumJacks), (Jack) (int) (r.unipolar() * kNumJacks));
                    e.setPatch (ps);
                    Params q; q.osc1Octave = (int) (r.unipolar() * 4); q.osc2Octave = (int) (r.unipolar() * 4);
                    q.osc1Wave = (int) (r.unipolar() * 4); q.osc2Wave = (int) (r.unipolar() * 4); q.osc2Sync = r.unipolar() > 0.5f;
                    q.mixOsc1 = r.unipolar(); q.mixOsc2 = r.unipolar(); q.mixNoise = r.unipolar();
                    q.lfoRate = r.unipolar(); q.lfoWave = (int) (r.unipolar() * 4); q.modWheel = r.unipolar();
                    q.lfoPitchAmt = r.unipolar(); q.lfoCutoffAmt = r.unipolar(); q.lfoPwAmt = r.unipolar();
                    q.cutoff = r.unipolar(); q.resonance = r.unipolar(); q.envAmt = r.bipolar(); q.kbdTrack = (int) (r.unipolar() * 3);
                    q.attack = r.unipolar(); q.decay = r.unipolar(); q.sustain = r.unipolar(); q.release = r.unipolar();
                    q.vcaMode = (int) (r.unipolar() * 3); q.reverbMix = r.unipolar(); q.volume = 1.0f;
                    q.hpCutoff = r.unipolar(); q.attenuator = r.bipolar();
                    e.setParams (q);
                    ArpSeqParams ap; ap.play = r.unipolar() > 0.5f; ap.mode = (int) (r.unipolar() * 2); ap.rate = r.unipolar();
                    e.setArpParams (ap);
                    if (r.unipolar() > 0.4f) e.noteOn (24 + (int) (r.unipolar() * 84), 1 + (int) (r.unipolar() * 126)); else e.allNotesOff();
                }
                e.process (o.data(), (int) o.size());
                for (float v : o) { if (! std::isfinite (v)) bad = true; maxAbs = std::max (maxAbs, (double) std::abs (v)); }
            }
            std::snprintf (buf, sizeof buf, "max |out| %.3f", maxAbs);
            check ("fuzz: 60 s of random cables and parameters", ! bad && maxAbs <= 1.21, buf);
        }
        std::printf ("  patch panel tests: %s\n", failures == 0 ? "ALL PASSED" : "FAILURES");
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
