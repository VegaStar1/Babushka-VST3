// Babushka — the pedal board on the rear panel: SATURATOR, PHASER, BBD CHORUS and TAPE ECHO (plugin-only, not on the
// hardware). They sit between the VCA and the spring reverb, in the order the user sets; from the first pedal on the
// signal is a stereo pair (the voice is mono, the phaser, chorus and echo spread it).
//
// Each pedal is modelled on the circuits that gave the classic sound, kept small enough to run comfortably:
//
//   SATURATOR  four characters, 2× oversampled:
//              TRIODE  a 12AX7-style stage: asymmetric transfer (the grid conducts on positive swings and clips
//                      harder than the cut-off side), a movable operating point (BIAS → more 2nd harmonic) and
//                      power-supply sag (the gain dips a little on loud passages);
//              TAPE    magnetic saturation with a little hysteresis (the curve depends on the direction the signal
//                      moves; BIAS = how under-biased the tape is), head bump, high-frequency loss growing with drive;
//              TRANSFORMER  the core saturates on low frequencies first: fat, dense bass, clean mids;
//              DIODE   a pair of silicon diodes in the feedback loop of an op-amp (sharper knee, odd harmonics).
//              BODY = a low shelf before the drive, TONE = a tilt after it, OUTPUT = level.
//   PHASER     4 / 6 / 8 first-order all-pass stages swept by a triangle LFO on an exponential (JFET/OTA-like) law,
//              each stage slightly off its neighbours as real parts are; COLOR = feedback through a soft-saturating
//              OTA. STEREO offsets the right channel's LFO.
//   BBD CHORUS a bucket-brigade delay per channel with its anti-alias and reconstruction filters, the compander's
//              breathing hiss and soft overload, modulated by a triangle LFO in opposite phase left / right
//              (MODE I ≈ 0.5 Hz, II ≈ 0.9 Hz, I+II a fast shallow shimmer, as on the classic polysynth choruses);
//              VIBRATO = wet only; ENSEMBLE = three sine-modulated lines per side. AGE lowers the BBD's bandwidth.
//   TAPE ECHO  a record head and three playback heads (spaced 1 : 2 : 3) on a tape loop; MODE picks the heads.
//              The tape speed follows TIME with the inertia of a motor, so turning it bends the pitch of the
//              repeats; wow and flutter wobble the speed; the feedback path saturates like tape and loses highs,
//              so repeats grow darker and can run away without clipping digitally. HOLD loops what is on the tape.
//              Switching the pedal off stops recording but lets the repeats ring out.
#pragma once

#include "Common.h"
#include <array>
#include <vector>

namespace babushka::dsp
{
enum class Pedal : int { Saturator = 0, Phaser = 1, Chorus = 2, TapeEcho = 3 };
constexpr int kNumPedals = 4;

struct PedalParams
{
    std::array<int, kNumPedals> order { 0, 1, 2, 3 };       // chain order (pedal indices, first to last)
    std::array<bool, kNumPedals> on { false, false, false, false };
    // SATURATOR
    int   satMode = 0;                                        // 0 triode, 1 tape, 2 transformer, 3 diode
    float satDrive = 0.4f, satBias = 0.35f, satBody = 0.35f, satTone = 0.5f, satMix = 1.0f, satOutput = 0.5f;
    // PHASER
    int   phaMode = 0;                                        // 0 4-stage, 1 6-stage, 2 4-stage + colour, 3 8-stage
    float phaRate = 0.3f, phaDepth = 0.7f, phaFeedback = 0.25f, phaCenter = 0.5f, phaStereo = 0.4f, phaMix = 0.5f;
    // BBD CHORUS
    int   choMode = 0;                                        // 0 I, 1 II, 2 I+II, 3 vibrato, 4 ensemble
    float choRate = 0.45f, choDepth = 0.5f, choMix = 0.5f, choWidth = 1.0f, choNoise = 0.2f, choAge = 0.25f;
    // TAPE ECHO
    int   tapHeads = 0;                                       // 0 1, 1 2, 2 3, 3 1+2, 4 2+3, 5 1+3, 6 1+2+3
    int   tapSync = 0;                                        // 0 free, 1 1/4, 2 1/8, 3 dotted 1/8, 4 1/16
    float tapTime = 0.45f, tapFeedback = 0.35f, tapMix = 0.35f, tapWow = 0.2f, tapFlutter = 0.2f, tapAge = 0.3f,
          tapBass = 0.5f, tapTreble = 0.5f;
    bool  tapHold = false;
};

// Knob → physical value laws shared by the engine and the editor's read-outs.
namespace pedallaw
{
    inline float phaserHz (float k) noexcept   { return 0.04f * std::pow (10.0f / 0.04f, clampf (k, 0.0f, 1.0f)); }      // 0.04 … 10 Hz
    inline float chorusHz (float k) noexcept   { return 0.05f * std::pow (12.0f / 0.05f, clampf (k, 0.0f, 1.0f)); }      // 0.05 … 12 Hz
    inline float echoSeconds (float k) noexcept { return 0.04f * std::pow (0.6f / 0.04f, clampf (k, 0.0f, 1.0f)); }     // head 1: 40 … 600 ms
    inline float echoDivisionBeats (int sync) noexcept
    {
        static const float beats[] = { 0.0f, 1.0f, 0.5f, 0.75f, 0.25f };
        return beats[sync < 0 || sync > 4 ? 0 : sync];
    }
}

// ---- building blocks ---------------------------------------------------------------------------------------------
// 2× up / down sampling with a Kaiser-windowed half-band FIR (for the saturator's curves).
class Oversampler2x
{
public:
    Oversampler2x()
    {
        const int n = kTaps;
        const double mid = (n - 1) * 0.5, beta = 8.0;
        auto i0 = [] (double x) { double s = 1.0, t = 1.0; for (int k = 1; k < 40; ++k) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; } return s; };
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double t = i - mid;
            const double sinc = std::abs (t) < 1.0e-9 ? 0.5 : std::sin (3.14159265358979 * 0.5 * t) / (3.14159265358979 * t);
            const double r = t / mid;
            h[(size_t) i] = (float) (sinc * i0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / i0 (beta));
            sum += h[(size_t) i];
        }
        for (auto& v : h) v = (float) (v / sum);
    }
    void reset() { up.fill (0.0f); down.fill (0.0f); upPos = downPos = 0; }
    // one input sample → two at the doubled rate
    void upsample (float x, float& a, float& b) noexcept
    {
        push (up, upPos, x * 2.0f); a = convolve (up, upPos);
        push (up, upPos, 0.0f);     b = convolve (up, upPos);
    }
    // two samples at the doubled rate → one
    float downsample (float a, float b) noexcept
    {
        push (down, downPos, a);
        push (down, downPos, b);
        return convolve (down, downPos);
    }
private:
    static constexpr int kTaps = 31;
    std::array<float, kTaps> h {};
    std::array<float, 2 * kTaps> up {}, down {};
    int upPos = 0, downPos = 0;
    static void push (std::array<float, 2 * kTaps>& buf, int& pos, float x) noexcept
    {
        buf[(size_t) pos] = x; buf[(size_t) (pos + kTaps)] = x;
        if (++pos >= kTaps) pos = 0;
    }
    float convolve (const std::array<float, 2 * kTaps>& buf, int pos) const noexcept
    {
        const float* x = buf.data() + pos;          // oldest … newest
        float acc = 0.0f;
        for (int i = 0; i < kTaps; ++i) acc += h[(size_t) i] * x[i];
        return acc;
    }
};

// RBJ biquad (peaking / shelving / low-pass), direct form II transposed.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() noexcept { z1 = z2 = 0.0f; }
    float process (float x) noexcept { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void set (float nb0, float nb1, float nb2, float na0, float na1, float na2) noexcept
    { b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0; a1 = na1 / na0; a2 = na2 / na0; }
    void lowPass (float fc, float q, float fs) noexcept
    {
        const float w = kTwoPi * clampf (fc, 10.0f, 0.45f * fs) / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        set ((1 - c) * 0.5f, 1 - c, (1 - c) * 0.5f, 1 + al, -2 * c, 1 - al);
    }
    void peak (float fc, float q, float dB, float fs) noexcept
    {
        const float A = std::pow (10.0f, dB / 40.0f), w = kTwoPi * clampf (fc, 10.0f, 0.45f * fs) / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
};

// Fractional read of a circular buffer (4-point Hermite).
inline float readHermite (const std::vector<float>& buf, int writePos, float delaySamples) noexcept
{
    const int size = (int) buf.size();
    const float d = clampf (delaySamples, 1.0f, (float) size - 4.0f);
    const int di = (int) d;
    const float f = d - (float) di;
    auto at = [&] (int k) { int i = writePos - k; while (i < 0) i += size; return buf[(size_t) (i % size)]; };
    const float xm1 = at (di - 1), x0 = at (di), x1 = at (di + 1), x2 = at (di + 2);
    const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

inline float triangle (float phase) noexcept { const float p = phase - std::floor (phase); return 4.0f * std::abs (p - 0.5f) - 1.0f; }   // −1 … 1

// ---- SATURATOR -----------------------------------------------------------------------------------------------------
class SaturatorPedal
{
public:
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        for (int c = 0; c < 2; ++c)
        {
            ch[(size_t) c].bodySplit.setCutoff (120.0f, fs);
            ch[(size_t) c].toneSplit.setCutoff (900.0f, fs);
            ch[(size_t) c].ironSplit.setCutoff (200.0f, 2.0f * fs);
            ch[(size_t) c].dc.setCutoff (8.0f, fs);
            ch[(size_t) c].bump.peak (90.0f, 0.9f, 2.0f, fs);
        }
        reset();
    }
    void reset() { for (auto& c : ch) { c.os.reset(); c.bodySplit.reset(); c.toneSplit.reset(); c.ironSplit.reset(); c.dc.reset(); c.hf.reset(); c.bump.reset(); c.env = 0; c.prevV = 0; } }

    void set (const PedalParams& p) noexcept
    {
        mode = p.satMode; drive = p.satDrive; bias = p.satBias; body = p.satBody; tone = p.satTone; mix = p.satMix; output = p.satOutput;
        for (auto& c : ch) c.hf.lowPass (std::min (18000.0f - 10000.0f * drive, 0.45f * fs), 0.6f, fs);
    }

    float process (float x, int c) noexcept
    {
        auto& s = ch[(size_t) c];
        const float low = s.bodySplit.process (x);
        float v = x + low * (1.8f * body);
        const float g = std::pow (10.0f, (36.0f * drive) / 20.0f) * 1.2f;
        // supply sag (triode): loud passages pull the gain down a little
        s.env += (std::abs (v) - s.env) * (std::abs (v) > s.env ? 0.01f : 0.0008f);
        const float sag = mode == 0 ? 1.0f / (1.0f + 0.35f * drive * s.env * g) : 1.0f;
        float a, b;
        s.os.upsample (v * g * sag, a, b);
        a = shape (a, s); b = shape (b, s);
        float y = s.os.downsample (a, b);
        if (mode == 1) y = s.hf.process (s.bump.process (y));       // tape: head bump, highs lost with drive
        y = s.dc.process (y);
        const float tLow = s.toneSplit.process (y), t = (tone - 0.5f) * 2.0f;
        y = tLow * (1.0f - 0.55f * t) + (y - tLow) * (1.0f + 0.55f * t);
        const float makeup = 1.0f / std::pow (g, 0.32f) * std::pow (10.0f, ((output - 0.5f) * 24.0f) / 20.0f) * 0.8f;
        return x + (y * makeup - x) * mix;
    }

private:
    struct Channel
    {
        Oversampler2x os;
        OnePoleLP bodySplit, toneSplit, ironSplit;
        DcBlocker dc;
        Biquad hf, bump;
        float env = 0.0f, prevV = 0.0f;
    };
    float fs = 48000.0f;
    int mode = 0;
    float drive = 0.5f, bias = 0.35f, body = 0.35f, tone = 0.5f, mix = 1.0f, output = 0.5f;
    std::array<Channel, 2> ch;

    float shape (float v, Channel& s) const noexcept
    {
        switch (mode)
        {
            case 0:
            {   // triode: grid conduction clips the positive side harder; BIAS moves the operating point
                const float b = 0.6f * bias;
                auto t = [] (float u) { return u >= 0.0f ? std::tanh (u) : (std::exp (u) - 1.0f) * 0.85f + std::tanh (u) * 0.15f; };
                return t (v + b) - t (b);
            }
            case 1:
            {   // tape: the curve leans with the direction of travel (a touch of hysteresis); BIAS = under-bias
                const float dir = std::tanh ((v - s.prevV) * 40.0f);
                s.prevV = v;
                const float w = 0.06f + 0.18f * bias;
                return v / std::sqrt (1.0f + v * v) * (1.0f - 0.5f * w) + w * 0.5f * std::tanh (v - dir * w);
            }
            case 2:
            {   // transformer: the lows saturate first
                const float lo = s.ironSplit.process (v), hi = v - lo;
                const float loS = std::tanh (1.6f * lo + 0.2f * bias * lo * lo) / 1.6f;
                return std::tanh (0.8f * (loS + hi)) / 0.8f;
            }
            default:
            {   // diode pair: a sharper knee; BIAS makes the two diodes differ
                const float k = v >= 0.0f ? 2.5f : 2.5f - 1.2f * bias;
                return v / std::pow (1.0f + std::pow (std::abs (v), k), 1.0f / k);
            }
        }
    }
};

// ---- PHASER --------------------------------------------------------------------------------------------------------
class PhaserPedal
{
public:
    void prepare (float sampleRate) { fs = sampleRate; reset(); }
    void reset() { for (auto& c : ch) { c.z.fill (0.0f); c.fb = 0.0f; } phase = 0.0f; }
    void set (const PedalParams& p) noexcept
    {
        mode = p.phaMode; rate = pedallaw::phaserHz (p.phaRate); depth = p.phaDepth; feedback = p.phaFeedback;
        center = 120.0f * std::pow (2.0f, 3.6f * p.phaCenter); stereo = p.phaStereo; mix = p.phaMix;
    }
    void tick() noexcept { phase += rate / fs; phase -= std::floor (phase); }

    float process (float x, int c) noexcept
    {
        auto& s = ch[(size_t) c];
        const int stages = mode == 1 ? 6 : (mode == 3 ? 8 : 4);
        // smoothed triangle (the LFO's capacitor rounds the corners), right channel offset by STEREO
        const float tri = triangle (phase + (c == 1 ? 0.5f * stereo : 0.0f));
        const float lfo = tri * (1.5f - 0.5f * tri * tri);
        const float f0 = clampf (center * std::exp2 (2.2f * depth * lfo), 30.0f, 0.42f * fs);
        const float fbAmt = (mode == 2 ? 0.92f : 0.6f) * feedback;
        float v = std::tanh (x * 0.9f + fbAmt * s.fb) / 0.9f;           // input stage / OTA in the feedback loop
        for (int i = 0; i < stages; ++i)
        {
            static const float spread[8] = { 1.0f, 1.07f, 0.94f, 1.03f, 0.97f, 1.09f, 0.92f, 1.05f };   // part tolerances
            const float t = std::tan (kPi * clampf (f0 * spread[i], 20.0f, 0.45f * fs) / fs);
            const float a = (t - 1.0f) / (t + 1.0f);
            const float y = a * v + s.z[(size_t) i];
            s.z[(size_t) i] = v - a * y;
            v = y;
        }
        s.fb = v;
        return (x * (1.0f - mix) + v * mix) * (1.0f + 0.4f * mix);      // the notches take some level: made up
    }

private:
    struct Channel { std::array<float, 8> z {}; float fb = 0.0f; };
    float fs = 48000.0f, rate = 0.3f, depth = 0.8f, feedback = 0.25f, center = 600.0f, stereo = 0.4f, mix = 0.5f, phase = 0.0f;
    int mode = 0;
    std::array<Channel, 2> ch;
};

// ---- BBD CHORUS ----------------------------------------------------------------------------------------------------
class ChorusPedal
{
public:
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        for (auto& c : ch) c.buf.assign ((size_t) (0.05f * fs) + 8, 0.0f);
        rng.seed (0xC40C05u);
        reset();
    }
    void reset() { for (auto& c : ch) { std::fill (c.buf.begin(), c.buf.end(), 0.0f); c.pos = 0; c.pre.reset(); c.post.reset(); c.env = 0; } phase = 0.0f; }
    void set (const PedalParams& p) noexcept
    {
        mode = p.choMode; rate = pedallaw::chorusHz (p.choRate); depth = p.choDepth; mix = mode == 3 ? 1.0f : p.choMix;
        width = p.choWidth; noise = p.choNoise; age = p.choAge;
        const float bw = 10000.0f * (1.0f - 0.55f * age);                 // the BBD's usable bandwidth
        for (auto& c : ch) { c.pre.lowPass (bw, 0.7f, fs); c.post.lowPass (bw, 0.7f, fs); }
    }
    void tick() noexcept { phase += rate / fs; phase -= std::floor (phase); }

    float process (float x, int c) noexcept
    {
        auto& s = ch[(size_t) c];
        // input: anti-alias filter, compander and a soft overload of the bucket chain
        const float in = std::tanh (s.pre.process (x) * 1.1f) / 1.1f;
        s.buf[(size_t) s.pos] = in;
        float wet = 0.0f;
        if (mode == 4)
        {   // ensemble: three lines per side, sine LFOs 120° apart (the right side between the left's)
            for (int k = 0; k < 3; ++k)
            {
                const float ph = phase + (float) k / 3.0f + (c == 1 ? width / 6.0f : 0.0f);
                const float d = (7.0f + 4.0f * depth * std::sin (kTwoPi * ph)) * 0.001f * fs;
                wet += readHermite (s.buf, s.pos, d) * (1.0f / 3.0f);
            }
        }
        else
        {
            // left and right in opposite phase at full WIDTH, in phase at none (vibrato: the same on both)
            const float tri = triangle (phase);
            const float lfo = c == 0 || mode == 3 ? tri : tri * (1.0f - 2.0f * width);
            const float base = mode == 2 ? 3.2f : 3.5f, swing = (mode == 2 ? 0.35f : 1.85f) * (0.3f + depth);
            const float d = (base + swing * (mode == 3 ? std::sin (kTwoPi * phase) : lfo)) * 0.001f * fs;
            wet = readHermite (s.buf, s.pos, d);
        }
        if (++s.pos >= (int) s.buf.size()) s.pos = 0;
        wet = s.post.process (wet);
        // compander hiss: breathes with the signal
        s.env += (std::abs (x) - s.env) * 0.002f;
        wet += rng.bipolar() * noise * 0.0025f * (0.25f + 3.0f * s.env);
        return mode == 3 ? wet : x * (1.0f - 0.15f * mix) + wet * mix * 1.25f;     // dry and the BBD side by side
    }

private:
    struct Channel { std::vector<float> buf; int pos = 0; Biquad pre, post; float env = 0.0f; };
    float fs = 48000.0f, rate = 0.5f, depth = 0.6f, mix = 0.5f, width = 1.0f, noise = 0.25f, age = 0.3f, phase = 0.0f;
    int mode = 0;
    std::array<Channel, 2> ch;
    Rng rng;
};

// ---- TAPE ECHO -----------------------------------------------------------------------------------------------------
class TapeEchoPedal
{
public:
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        tape.assign ((size_t) (3.0f * 0.62f * fs * 1.06f) + 64, 0.0f);
        fbLow.setCutoff (90.0f, fs);
        bassSplit.setCutoff (250.0f, fs); trebleSplit.setCutoff (3000.0f, fs);
        rng.seed (0x7A9Eu);
        speedSmooth.setTime (0.35f, fs);
        reset();
    }
    void reset() { std::fill (tape.begin(), tape.end(), 0.0f); pos = 0; lp.reset(); fbLow.reset(); bassSplit.reset(); trebleSplit.reset(); inGain = 0.0f; wowPhase = flutPhase = 0.0f; drift = 0.0f; first = true; }
    void set (const PedalParams& p, double bpm) noexcept
    {
        heads = p.tapHeads; feedback = p.tapFeedback; mix = p.tapMix; wow = p.tapWow; flutter = p.tapFlutter; age = p.tapAge;
        bass = p.tapBass; treble = p.tapTreble; hold = p.tapHold;
        const float beats = pedallaw::echoDivisionBeats (p.tapSync);
        head1 = beats > 0.0f ? clampf ((float) (60.0 / std::max (bpm, 20.0)) * beats, 0.03f, 0.62f) : pedallaw::echoSeconds (p.tapTime);
        lp.lowPass (7500.0f - 4500.0f * age, 0.6f, fs);
        if (first) { speedSmooth.snap (head1); first = false; }
    }
    bool isRinging() const noexcept { return ringing > 0; }
    // CLEAR: the tape is erased (after a 15 ms fade, so it does not click) and the repeats stop at once.
    void requestClear() noexcept { clearLeft = clearLength = std::max (1, (int) (0.015f * fs)); }

    // in: the mono sum; on: recording. Returns the echo (mono) to add to the dry signal.
    float process (float in, bool on) noexcept
    {
        const float wantIn = on && ! hold ? 1.0f : 0.0f;
        inGain += (wantIn - inGain) * 0.002f;
        // tape speed: motor inertia (pitch bends while TIME turns), wow (slow, irregular) and flutter (fast)
        const float t1 = speedSmooth.process (head1);
        wowPhase += 0.7f / fs; wowPhase -= std::floor (wowPhase);
        flutPhase += 7.3f / fs; flutPhase -= std::floor (flutPhase);
        drift += (rng.bipolar() - drift) * (2.0f / fs);
        const float mod = 1.0f + wow * 0.006f * (std::sin (kTwoPi * wowPhase) + 0.6f * drift * 6.0f)
                               + flutter * 0.0012f * (std::sin (kTwoPi * flutPhase) + 0.4f * std::sin (kTwoPi * flutPhase * 1.71f));
        static const int mask[] = { 1, 2, 4, 3, 6, 5, 7 };
        const int m = mask[heads < 0 || heads > 6 ? 0 : heads];
        float sum = 0.0f; int count = 0;
        for (int h = 0; h < 3; ++h)
            if (m & (1 << h)) { sum += readHermite (tape, pos, (float) (h + 1) * t1 * mod * fs); ++count; }
        const float echo = sum / std::sqrt ((float) std::max (count, 1));
        // feedback: the playback is re-recorded through the tape's saturation and losses. Switched off, the loop no
        // longer sustains itself: whatever the FEEDBACK knob says, the repeats die away within a few seconds.
        const float fbGain = hold ? 1.0f : std::min (feedback * 1.12f, on ? 2.0f : 0.6f);
        float fb = lp.process (echo);
        fb = fb - fbLow.process (fb);
        fb = std::tanh (fb * fbGain * 1.3f) / 1.3f;
        const float hiss = rng.bipolar() * age * 0.0009f * inGain;          // tape hiss, while recording
        float duck = 1.0f;
        if (clearLeft > 0) duck = (float) --clearLeft / (float) clearLength;
        tape[(size_t) pos] = (in * inGain + fb + hiss) * duck;
        if (++pos >= (int) tape.size()) pos = 0;
        if (clearLeft == 0 && clearLength > 0)
        {
            std::fill (tape.begin(), tape.end(), 0.0f);
            lp.reset(); fbLow.reset();
            clearLength = 0;
        }
        // echo level and the two-band tone of the playback amplifier
        const float lo = bassSplit.process (echo), hiSplit = trebleSplit.process (echo);
        const float mid = hiSplit - lo, hi = echo - hiSplit;
        const float out = lo * dbToGain ((bass - 0.5f) * 24.0f) + mid + hi * dbToGain ((treble - 0.5f) * 24.0f);
        ringing = std::abs (out) > 1.0e-5f || inGain > 1.0e-4f ? (int) (fs * 0.1f) : std::max (0, ringing - 1);
        return out * duck * mix * 1.25f;
    }

private:
    std::vector<float> tape;
    int pos = 0, heads = 0, ringing = 0, clearLeft = 0, clearLength = 0;
    float fs = 48000.0f, feedback = 0.4f, mix = 0.35f, wow = 0.2f, flutter = 0.2f, age = 0.3f, bass = 0.5f, treble = 0.5f, head1 = 0.2f;
    float inGain = 0.0f, wowPhase = 0.0f, flutPhase = 0.0f, drift = 0.0f;
    bool hold = false, first = true;
    Biquad lp;
    OnePoleLP fbLow, bassSplit, trebleSplit;
    Smoother speedSmooth;
    Rng rng;
    static float dbToGain (float db) noexcept { return std::pow (10.0f, db / 20.0f); }
};

// ---- the board -----------------------------------------------------------------------------------------------------
// Runs the pedals in order on a stereo pair, one host-rate sample at a time. Each pedal fades in and out over ~10 ms
// when switched; an echo switched off keeps ringing until its repeats have died away. The patch jacks of the pedals (IN replaces the chain's signal
// into a pedal, CV moves its main knob, OUT carries what it puts out) are passed in per sample.
class PedalBoard
{
public:
    struct Jacks
    {
        std::array<bool, kNumPedals> inPatched {};
        std::array<float, kNumPedals> in {}, cv {};          // IN: audio (±1), CV: −1 … +1 added to the main knob
        std::array<float, kNumPedals> out {};                // what each pedal puts out (mono), for its OUT jack
    };

    void prepare (float sampleRate)
    {
        fs = sampleRate;
        sat.prepare (fs); pha.prepare (fs); cho.prepare (fs); tap.prepare (fs);
        for (auto& a : active) a = 0.0f;
        fade = 1.0f - std::exp (-1.0f / (0.01f * fs));
    }
    void reset() { sat.reset(); pha.reset(); cho.reset(); tap.reset(); }
    void clearEcho() noexcept { tap.requestClear(); }

    // Parameters for this block; cv shifts each pedal's main knob (drive, rate, rate, time).
    void set (const PedalParams& p, double bpm, const std::array<float, kNumPedals>& cv = {}) noexcept
    {
        params = p;
        PedalParams q = p;
        q.satDrive = clampf (p.satDrive + cv[0], 0.0f, 1.0f);
        q.phaRate = clampf (p.phaRate + cv[1], 0.0f, 1.0f);
        q.choRate = clampf (p.choRate + cv[2], 0.0f, 1.0f);
        q.tapTime = clampf (p.tapTime + cv[3], 0.0f, 1.0f);
        sat.set (q); pha.set (q); cho.set (q); tap.set (q, bpm);
    }

    bool anyActive() const noexcept
    {
        for (int i = 0; i < kNumPedals; ++i) if (params.on[(size_t) i] || active[(size_t) i] > 1.0e-4f) return true;
        return tap.isRinging();
    }

    void process (float& l, float& r, Jacks& jacks) noexcept
    {
        pha.tick(); cho.tick();
        for (int k = 0; k < kNumPedals; ++k)
        {
            const int i = params.order[(size_t) k];
            if (jacks.inPatched[(size_t) i]) l = r = jacks.in[(size_t) i];
            auto& a = active[(size_t) i];
            a += ((params.on[(size_t) i] ? 1.0f : 0.0f) - a) * fade;
            if (i == (int) Pedal::TapeEcho)
            {
                if (params.on[(size_t) i] || tap.isRinging())
                {
                    const float e = tap.process (0.5f * (l + r), params.on[(size_t) i]);
                    l += e; r += e;
                }
            }
            else if (a > 1.0e-4f)
            {
                float yl, yr;
                if (i == (int) Pedal::Saturator)    { yl = sat.process (l, 0); yr = sat.process (r, 1); }
                else if (i == (int) Pedal::Phaser)  { yl = pha.process (l, 0); yr = pha.process (r, 1); }
                else                                { yl = cho.process (l, 0); yr = cho.process (r, 1); }
                l += (yl - l) * a; r += (yr - r) * a;
            }
            jacks.out[(size_t) i] = 0.5f * (l + r);
        }
    }

private:
    float fs = 48000.0f, fade = 0.002f;
    PedalParams params;
    std::array<float, kNumPedals> active {};
    SaturatorPedal sat;
    PhaserPedal pha;
    ChorusPedal cho;
    TapeEchoPedal tap;
};
} // namespace babushka::dsp
