// Babushka — 6" spring tank model (905-style).
//
// Parametric (non-physical) spring model in the spirit of Välimäki/Parker/Abel (2010) and
// Parker (EURASIP JASP 2011): each spring is a feedback loop containing a transit delay and a
// long cascade of "stretched" first-order allpass filters  H(z) = (a + z^-K)/(1 + a·z^-K).
// The cascade makes group delay rise with frequency below fs/(2K), which produces the
// characteristic chirped echoes ("drip"/"boing"). Loop low-pass sets the ~4–5 kHz spring band,
// a slowly modulated delay adds the irregular "ripple" of a real tank. Two springs with slightly
// different lengths are summed, as in a typical two-spring tank. Driver saturation and band-limiting
// model the transducers. Implementation is original.
//
// Decay calibrated against recordings of the hardware tank (3 files, consistent within ±15 %):
// T60 per octave band  85-170 Hz ≈ 9 s, 340 Hz ≈ 7 s, 680 Hz ≈ 4.5 s, 1.4 kHz ≈ 3.6 s, 2.7 kHz ≈ 2.5 s,
// 5.4 kHz ≈ 1.1 s — i.e. a long, dark tail dominated by the low mids. The loop filter below is a fit to
// those numbers (fitted with the same band-T60 estimator, all bands within ≈ ±25 % except the top one,
// which stays long at ≈1.7 s): flat loss, a small step in loss around 335 Hz, two one-pole low-passes
// at 7 and 8 kHz, and a very low loop high-pass. The `t60` argument of setupSpring() is a design value
// for the flat loss; the effective low-frequency T60 is lower because of the shelf, filters and chain.
//
// Held notes. With a 9 s low-frequency T60 each loop is a comb with peaks ≈37 dB above its dips, so a steady tone
// that lands on a peak rings 15–20 dB louder than one between peaks, and at MIX 50 % the coherent dry + wet sum
// nearly cancels or doubles the fundamental. Two measures, neither of which touches a free decay:
//  - Resonance limiter: while the input is a steady tone (level within ±6 dB for ≈0.3 s) and the loops ring
//    louder than the drive, the loop gain is pulled down until they don't. Plucks, decaying notes and every
//    tail after a release run with the calibrated loop gain.
//  - The wet signal is frequency-shifted by 0.5 Hz, so it does not stay locked in phase with the dry signal; the
//    dry/wet sum of a held note becomes a slow swirl instead of a fixed cancellation or doubling.
#pragma once

#include "Common.h"
#include <vector>

namespace babushka::dsp
{
class StretchedAllpassChain
{
public:
    void prepare (int numStages, int stretch, float coeff)
    {
        stages = numStages; K = stretch; a = coeff;
        buf.assign ((size_t) (stages * K), 0.0f);
        idx = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }

    // w[n] = x[n] − a·w[n−K];  y[n] = a·w[n] + w[n−K]
    float process (float x) noexcept
    {
        float* b = buf.data();
        for (int s = 0; s < stages; ++s)
        {
            float& slot = b[s * K + idx];
            const float wOld = slot;
            const float w = x - a * wOld;
            slot = w;
            x = a * w + wOld;
        }
        if (++idx >= K) idx = 0;
        return x;
    }

private:
    std::vector<float> buf;
    int stages = 0, K = 1, idx = 0;
    float a = 0.0f;
};

// First-order "loss shelf": unity gain at DC, fp/fz (< 1) at high frequencies, corners fp < fz.
// |H| = sqrt((1 + (f/fz)^2) / (1 + (f/fp)^2)). Models frequency-dependent damping inside the spring loop.
struct LossShelf
{
    float b0 = 1.0f, b1 = 0.0f, a1 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    void set (float fp, float fz, float fs) noexcept
    {
        const float tp = std::tan (kPi * fp / fs), tz = std::tan (kPi * fz / fs);
        const float k = tp / tz;
        b0 = k * (tz + 1.0f) / (tp + 1.0f);
        b1 = k * (tz - 1.0f) / (tp + 1.0f);
        a1 = (tp - 1.0f) / (tp + 1.0f);
    }
    float process (float x) noexcept { const float y = b0 * x + b1 * x1 - a1 * y1; x1 = x; y1 = y; return y; }
    void reset() noexcept { x1 = y1 = 0.0f; }
};

// Analytic-signal pair: two allpass chains whose outputs differ by 90° (±0.5 % magnitude error from 20 Hz up to
// fs/2 at 48 kHz). Polyphase IIR design after Olli Niemitalo.
class HilbertPair
{
public:
    HilbertPair()
    {
        static constexpr float ca[4] = { 0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f };
        static constexpr float cb[4] = { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };
        for (int i = 0; i < 4; ++i) { a[i].a2 = ca[i] * ca[i]; b[i].a2 = cb[i] * cb[i]; }
    }
    void reset() noexcept { for (auto& s : a) s.reset(); for (auto& s : b) s.reset(); aDelayed = 0.0f; }
    void process (float x, float& re, float& im) noexcept
    {
        float u = x, v = x;
        for (auto& s : a) u = s.process (u);
        for (auto& s : b) v = s.process (v);
        re = aDelayed; im = v; aDelayed = u;
    }

private:
    // y[n] = a²·(x[n] + y[n−2]) − x[n−2]
    struct Section
    {
        float a2 = 0.0f, x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
        float process (float x) noexcept { const float y = a2 * (x + y2) - x2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; }
        void reset() noexcept { x1 = x2 = y1 = y2 = 0.0f; }
    };
    Section a[4], b[4];
    float aDelayed = 0.0f;
};

class SpringReverb
{
public:
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        const int K = std::max (1, (int) std::lround (fs / (2.0f * 4300.0f)));

        // Two springs: transit times ~28/34 ms, slightly different dispersion.
        setupSpring (springs[0], 0.0280f, 72, K, 0.60f, 13.7f, 0x5EED01u);
        setupSpring (springs[1], 0.0341f, 80, K, 0.63f, 14.9f, 0x5EED02u);

        inHP.setCutoff (90.0f, fs);  inLP.setCutoff (5200.0f, fs);
        outHP.setCutoff (110.0f, fs); outLP.setCutoff (4200.0f, fs); outLP2.setCutoff (4200.0f, fs);

        auto coeff = [this] (float seconds) { return 1.0f - std::exp (-1.0f / (seconds * fs)); };
        envFastA = coeff (0.05f); envSlowA = coeff (0.5f); steadyUpA = coeff (0.3f); steadyDownA = coeff (0.02f);
        limiterA = coeff (0.05f);
        peakRelease = std::exp (-1.0f / (3.0f * fs));
        shiftStep = { std::cos (kTwoPi * 0.5f / fs), std::sin (kTwoPi * 0.5f / fs) };
        reset();
    }

    void reset()
    {
        for (auto& sp : springs)
        {
            std::fill (sp.delay.begin(), sp.delay.end(), 0.0f);
            sp.chain.reset(); sp.loopLP.reset(); sp.loopLP2.reset(); sp.loopHP.reset(); sp.loopShelf.reset();
            sp.fb = 0.0f; sp.write = 0;
        }
        inHP.reset(); inLP.reset(); outHP.reset(); outLP.reset(); outLP2.reset();
        inFast = inSlow = inPeak = steady = loopEnv = 0.0f; limit = 1.0f;
        shifter.reset(); shift = { 1.0f, 0.0f };
    }

    // Returns 100%-wet spring output.
    float process (float x) noexcept
    {
        // Driver transducer: band-limited, soft saturation when hit hard.
        float d = inLP.process (inHP.process (x));
        d = std::tanh (1.4f * d) * (1.0f / 1.4f);

        // Resonance limiter (see the header). Steady tone: input level within ±6 dB of its 0.5 s average, not far below
        // its recent peak (a fading release is not steady), held for ≈0.3 s. Loop level = the springs' summed output.
        inFast += envFastA * (d * d - inFast);
        inSlow += envSlowA * (d * d - inSlow);
        inPeak = std::max (inSlow, inPeak * peakRelease);
        const bool isSteady = inSlow > 0.03f * inPeak && inFast < 4.0f * inSlow && inSlow < 4.0f * inFast;
        steady += (isSteady ? steadyUpA : steadyDownA) * ((isSteady ? 1.0f : 0.0f) - steady);
        const float loopSum = springs[0].fb + springs[1].fb;
        loopEnv += envFastA * (0.5f * loopSum * loopSum - loopEnv);
        const float weight = std::clamp (2.0f * steady - 1.0f, 0.0f, 1.0f);
        const float excess = std::min (1.0f, std::sqrt (inSlow / (loopEnv + 1.0e-12f)));
        limit += limiterA * (1.0f - weight * (1.0f - excess) - limit);

        float wet = 0.0f;
        for (auto& sp : springs)
        {
            const float v = d + limit * sp.g * sp.fb;
            sp.delay[(size_t) sp.write] = v;

            // Ripple: slowly wandering fractional read position.
            const float rd = sp.transit + 1.5f * sp.ripple.process();
            float rp = (float) sp.write - rd;
            const int size = (int) sp.delay.size();
            while (rp < 0.0f) rp += (float) size;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            const int i1 = (i0 + 1) % size;
            const float t = sp.delay[(size_t) i0] + fr * (sp.delay[(size_t) i1] - sp.delay[(size_t) i0]);
            if (++sp.write >= size) sp.write = 0;

            float c = sp.chain.process (t);
            c = sp.loopLP2.process (sp.loopLP.process (sp.loopShelf.process (sp.loopHP.process (c))));
            sp.fb = c;
            wet += c;
        }

        // Pickup + recovery amp: the tank band ends around 4–5 kHz (2-pole roll-off).
        const float out = outLP2.process (outLP.process (outHP.process (wet))) * wetGain;

        // 0.5 Hz frequency shift: Re{analytic(out) · e^{jωt}}, the phasor advanced by complex multiplication.
        float re, im;
        shifter.process (out, re, im);
        shift = { shift.re * shiftStep.re - shift.im * shiftStep.im, shift.re * shiftStep.im + shift.im * shiftStep.re };
        const float norm = 1.5f - 0.5f * (shift.re * shift.re + shift.im * shift.im);   // keeps |phasor| = 1
        shift.re *= norm; shift.im *= norm;
        return re * shift.re - im * shift.im;
    }

private:
    struct Spring
    {
        std::vector<float> delay;
        int write = 0;
        float transit = 0.0f, g = 0.9f, fb = 0.0f;
        StretchedAllpassChain chain;
        OnePoleLP loopLP, loopLP2;
        OnePoleHP loopHP;
        LossShelf loopShelf;
        Drift ripple;
        float loopSeconds = 0.03f, t60 = 14.0f;
    };

    float loopGain (const Spring& sp) const noexcept { return std::pow (10.0f, -3.0f * sp.loopSeconds / (sp.t60 * lossScale)); }

public:
    // Length of the tail relative to the calibrated tank (1 = the hardware), 0.3 … 2.5; safe to call every block.
    // Only the flat loop loss can be changed (the loop filters keep theirs), so the setting goes through a table
    // measured on this model: the factor on the flat loss's T60 that makes the broadband tail (time to −30 dB of the
    // energy after an impulse, 48 kHz) as many times longer as asked, within ≈ 5 %.
    void setDecayScale (float scale) noexcept
    {
        scale = std::clamp (scale, 0.3f, 2.5f);
        if (std::abs (scale - decayScale) < 1.0e-6f) return;
        decayScale = scale;
        static constexpr float table[][2] = { { 0.3f, 0.207f }, { 0.5f, 0.407f }, { 0.75f, 0.670f }, { 1.0f, 1.0f },
                                              { 1.5f, 2.253f }, { 2.0f, 4.048f }, { 2.5f, 8.835f } };
        int i = 0;
        while (i < 5 && scale > table[i + 1][0]) ++i;
        const float t = std::log (scale / table[i][0]) / std::log (table[i + 1][0] / table[i][0]);
        lossScale = table[i][1] * std::pow (table[i + 1][1] / table[i][1], t);
        for (auto& sp : springs) sp.g = loopGain (sp);
    }

private:
    float fs = 48000.0f, decayScale = 1.0f, lossScale = 1.0f;
    Spring springs[2];
    OnePoleHP inHP, outHP;
    OnePoleLP inLP, outLP, outLP2;
    float wetGain = 0.84f;   // 100 % wet ≈ dry level on held notes (engine level test, 48 kHz; with the resonance limiter)

    // Resonance limiter and frequency shifter state.
    float envFastA = 0.0f, envSlowA = 0.0f, steadyUpA = 0.0f, steadyDownA = 0.0f, limiterA = 0.0f, peakRelease = 1.0f;
    float inFast = 0.0f, inSlow = 0.0f, inPeak = 0.0f, steady = 0.0f, loopEnv = 0.0f, limit = 1.0f;
    struct Phasor { float re, im; };
    HilbertPair shifter;
    Phasor shift { 1.0f, 0.0f }, shiftStep { 1.0f, 0.0f };

    void setupSpring (Spring& sp, float transitSeconds, int stages, int K, float a, float t60, uint32_t seed)
    {
        // The dispersion chain adds a group delay of K·(1−a)/(1+a) samples per stage at low frequencies; K is rounded
        // from the sample rate, so the plain transit would give a slightly different loop time (and different
        // comb-filter resonances) at 44.1, 48 and 96 kHz. Compensating makes the loop time rate-independent.
        const float chainDelay = (float) stages * (float) K * (1.0f - a) / (1.0f + a);
        sp.transit = std::max (transitSeconds * fs - chainDelay, 8.0f);
        sp.delay.assign ((size_t) (sp.transit + 64.0f), 0.0f);
        sp.chain.prepare (stages, K, a);
        sp.loopLP.setCutoff (7000.0f, fs);
        sp.loopLP2.setCutoff (8000.0f, fs);
        sp.loopHP.setCutoff (7.0f, fs);
        sp.loopShelf.set (330.0f, 340.0f, fs);
        sp.ripple.prepare (fs, seed, 2.0f, 0.25f);
        // Loop delay ≈ transit + mean dispersion delay (stages / 8.6 kHz, independent of the sample rate).
        sp.loopSeconds = transitSeconds + (float) stages / 8600.0f;
        sp.t60 = t60;
        sp.g = loopGain (sp);
    }
};
} // namespace babushka::dsp
