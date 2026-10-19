// Babushka — DSP core. Original implementation; no third-party DSP code.
// Common utilities shared by all modules.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace babushka::dsp
{
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

inline float clampf (float x, float lo, float hi) noexcept { return std::min (std::max (x, lo), hi); }
inline float midiToHz (float note) noexcept { return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f)); }

// tanh(v)/v, used by the linearised (implicit) nonlinear solvers. Equals 1 at v = 0.
inline float tanhRatio (float v) noexcept
{
    const float a = std::abs (v);
    if (a < 1.0e-4f) return 1.0f - v * v * (1.0f / 3.0f);
    return std::tanh (v) / v;
}

// xorshift32 — fast, deterministic per-instance noise.
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    void seed (uint32_t v) noexcept { s = v != 0 ? v : 0x9E3779B9u; }
    uint32_t next() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float bipolar() noexcept { return (float) (int32_t) next() * (1.0f / 2147483648.0f); } // [-1, 1)
    float unipolar() noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); }    // [0, 1)
    // Approximately Gaussian white noise (sum of 3 uniforms), unit-ish variance.
    float gaussianish() noexcept { return (bipolar() + bipolar() + bipolar()) * 1.0f; }
};

struct OnePoleLP
{
    float a = 1.0f, z = 0.0f;
    void setCutoff (float fc, float fs) noexcept { a = 1.0f - std::exp (-kTwoPi * fc / fs); }
    float process (float x) noexcept { z += a * (x - z); return z; }
    void reset (float v = 0.0f) noexcept { z = v; }
};

struct OnePoleHP
{
    OnePoleLP lp;
    void setCutoff (float fc, float fs) noexcept { lp.setCutoff (fc, fs); }
    float process (float x) noexcept { return x - lp.process (x); }
    void reset() noexcept { lp.reset(); }
};

// AC coupling (the hardware's audio outputs are AC-coupled).
struct DcBlocker
{
    float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
    void setCutoff (float fc, float fs) noexcept { r = std::exp (-kTwoPi * fc / fs); }
    float process (float x) noexcept { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
    void reset() noexcept { x1 = y1 = 0.0f; }
};

// Parameter de-zippering.
struct Smoother
{
    float a = 1.0f, v = 0.0f;
    void setTime (float seconds, float fs) noexcept { a = seconds <= 0.0f ? 1.0f : 1.0f - std::exp (-1.0f / (seconds * fs)); }
    void snap (float x) noexcept { v = x; }
    float process (float target) noexcept { v += a * (target - v); return v; }
};

// Slow random wander in [-1, 1] — models component drift / thermal tuning instability.
struct Drift
{
    Rng rng;
    float target = 0.0f, value = 0.0f, coeff = 0.0f;
    int counter = 0, period = 1;

    void prepare (float fs, uint32_t seed, float newTargetsPerSecond = 3.0f, float smoothSeconds = 0.6f)
    {
        rng.seed (seed);
        period = std::max (1, (int) (fs / newTargetsPerSecond));
        coeff  = 1.0f - std::exp (-1.0f / (smoothSeconds * fs));
        target = rng.bipolar();
        value  = target;
        counter = (int) (rng.unipolar() * (float) period);
    }

    float process() noexcept
    {
        if (--counter <= 0) { counter = period; target = rng.bipolar(); }
        value += coeff * (target - value);
        return value;
    }
};
} // namespace babushka::dsp
