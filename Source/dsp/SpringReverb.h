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
    }

    // Returns 100%-wet spring output.
    float process (float x) noexcept
    {
        // Driver transducer: band-limited, soft saturation when hit hard.
        float d = inLP.process (inHP.process (x));
        d = std::tanh (1.4f * d) * (1.0f / 1.4f);

        float wet = 0.0f;
        for (auto& sp : springs)
        {
            const float v = d + sp.g * sp.fb;
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
        return outLP2.process (outLP.process (outHP.process (wet))) * wetGain;
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
    };

    float fs = 48000.0f;
    Spring springs[2];
    OnePoleHP inHP, outHP;
    OnePoleLP inLP, outLP, outLP2;
    float wetGain = 0.70f;   // 100 % wet ≈ dry level on a held note (engine level test, 48 kHz)

    void setupSpring (Spring& sp, float transitSeconds, int stages, int K, float a, float t60, uint32_t seed)
    {
        sp.transit = transitSeconds * fs;
        sp.delay.assign ((size_t) (sp.transit + 64.0f), 0.0f);
        sp.chain.prepare (stages, K, a);
        sp.loopLP.setCutoff (7000.0f, fs);
        sp.loopLP2.setCutoff (8000.0f, fs);
        sp.loopHP.setCutoff (7.0f, fs);
        sp.loopShelf.set (330.0f, 340.0f, fs);
        sp.ripple.prepare (fs, seed, 2.0f, 0.25f);
        // Loop delay ≈ transit + mean dispersion delay (≈ stages·K samples).
        const float loopSeconds = (sp.transit + (float) (stages * K)) / fs;
        sp.g = std::pow (10.0f, -3.0f * loopSeconds / t60);
    }
};
} // namespace babushka::dsp
