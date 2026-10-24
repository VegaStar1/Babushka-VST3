// Babushka — nonlinear transistor-ladder low-pass, zero-delay-feedback.
//
// Model (per stage, normalised units of 2·Vt):
//     dy1/dt = wc · ( tanh(x − k·y4) − tanh(y1) )
//     dyi/dt = wc · ( tanh(y(i−1))   − tanh(yi) ),   i = 2..4
// This is the classic differential-pair ladder description (Huovilainen, DAFx-04).
//
// Discretisation: trapezoidal (TPT) integrators with pre-warped gain g = tan(pi·fc/fs).
// The implicit system is solved without unit delays: every tanh(v) is replaced by v·T(v̂),
// T = tanh(v)/v evaluated at an estimate v̂, which makes the system linear and exactly solvable.
// A second pass refines the estimates. At small signal the filter is an exact linear TPT ladder,
// so cutoff and self-oscillation pitch track correctly (playable when self-oscillating).
//
// Feedback is taken from the 4th stage to the input stage, so pass-band (bass) gain falls as
// 1/(1+k) when resonance rises — this matches the hardware's well-known bass loss.
//
// The input pair is slightly unbalanced (offset kInputBias before its tanh, the resulting DC removed after),
// so a hot input is clipped a little harder on one side: even harmonics, as from a real transistor pair.
#pragma once

#include "Common.h"

namespace babushka::dsp
{
class LadderFilter
{
public:
    void prepare (float sampleRate) noexcept { fs = sampleRate; reset(); noise.seed (0xA11CE5u); }
    void reset() noexcept { for (auto& v : s) v = 0.0f; for (auto& v : y) v = 0.0f; }
    void setThermalNoise (float amplitude) noexcept { thermalNoise = amplitude; }

    // The tanh stages lower the self-oscillation pitch as feedback rises above 4 (measured:
    // −5 c @ k=4.05, −20 c @ 4.2, −43 c @ 4.5, −70 c @ 5.0, independent of cutoff).
    // Returns the cutoff multiplier that keeps self-oscillation in tune (hardware is trimmed for this).
    static float tuningCompensation (float k) noexcept
    {
        static constexpr float ks[] = { 4.0f, 4.05f, 4.2f, 4.5f, 5.0f, 5.5f };
        static constexpr float cents[] = { 0.0f, -5.3f, -19.6f, -42.8f, -70.1f, -88.5f };
        if (k <= ks[0]) return 1.0f;
        float c = cents[5];
        for (int i = 0; i < 5; ++i)
            if (k < ks[i + 1]) { c = cents[i] + (cents[i + 1] - cents[i]) * (k - ks[i]) / (ks[i + 1] - ks[i]); break; }
        return std::exp2 (-c / 1200.0f);
    }

    // cutoffHz: stage cutoff; k: feedback, ~4 = self-oscillation threshold.
    float process (float x, float cutoffHz, float k) noexcept
    {
        const float fc = clampf (cutoffHz, 5.0f, 0.42f * fs);
        const float g  = std::tan (kPi * fc / fs);

        // Tiny thermal noise so self-oscillation starts from silence, like the hardware.
        x += thermalNoise * noise.bipolar();

        float yEst[4] = { y[0], y[1], y[2], y[3] };
        float inEst   = x - k * y[3];

        for (int pass = 0; pass < 2; ++pass)
        {
            const float tIn = biasedRatio (inEst);
            const float t0 = tanhRatio (yEst[0]), t1 = tanhRatio (yEst[1]),
                        t2 = tanhRatio (yEst[2]), t3 = tanhRatio (yEst[3]);

            // Stage 1:  y1 = A1 − B1·y4
            const float d0 = 1.0f / (1.0f + g * t0);
            const float A1 = (s[0] + g * tIn * x) * d0;
            const float B1 = g * tIn * k * d0;
            // Stages 2..4:  yi = alpha_i·y(i−1) + beta_i
            const float d1 = 1.0f / (1.0f + g * t1), d2 = 1.0f / (1.0f + g * t2), d3 = 1.0f / (1.0f + g * t3);
            const float a2 = g * t0 * d1, b2 = s[1] * d1;
            const float a3 = g * t1 * d2, b3 = s[2] * d2;
            const float a4 = g * t2 * d3, b4 = s[3] * d3;
            // y4 = G·y1 + S
            const float G = a2 * a3 * a4;
            const float S = b4 + a4 * (b3 + a3 * b2);

            const float y4 = (G * A1 + S) / (1.0f + G * B1);
            const float y1 = A1 - B1 * y4;
            const float y2 = a2 * y1 + b2;
            const float y3 = a3 * y2 + b3;

            yEst[0] = y1; yEst[1] = y2; yEst[2] = y3; yEst[3] = y4;
            inEst = x - k * y4;

            if (pass == 1)
            {
                // Integrator state update (TPT): s_new = 2·y − s.
                for (int i = 0; i < 4; ++i) { s[i] = 2.0f * yEst[i] - s[i]; y[i] = yEst[i]; }
                // Guard against numerical blow-up (should not happen; protects the host).
                if (! std::isfinite (y[3])) { reset(); return 0.0f; }
            }
        }
        return y[3];
    }

private:
    // ASSUMPTION: offset of the input pair ≈0.03 · 2·Vt (≈1.5 mV of transistor mismatch); not measured.
    static constexpr float kInputBias = 0.03f;
    static constexpr float kTanhBias = 0.0299910f;        // tanh (kInputBias)

    // (tanh(v + b) − tanh(b)) / v: the input stage as a ratio for the linearised solver.
    static float biasedRatio (float v) noexcept
    {
        constexpr float tb = kTanhBias, slope0 = 1.0f - tb * tb;
        if (std::abs (v) < 1.0e-4f) return slope0 - v * tb * slope0;
        return (std::tanh (v + kInputBias) - tb) / v;
    }

    float fs = 96000.0f;
    float s[4] {}, y[4] {};
    Rng noise;
    float thermalNoise = 1.0e-5f;
};
} // namespace babushka::dsp
