// Babushka — FIR decimator (Kaiser-windowed sinc) for the oversampled voice.
#pragma once

#include "Common.h"
#include <vector>

namespace babushka::dsp
{
constexpr double kPiD = 3.14159265358979323846;

class Decimator
{
public:
    // factor = oversampling ratio (1, 2, 4). Pass-band edge ~0.45·fsBase.
    void prepare (int oversamplingFactor)
    {
        factor = std::max (1, oversamplingFactor);
        if (factor == 1) { taps.assign (1, 1.0f); history.assign (2, 0.0f); pos = 0; return; }

        const int n = 64 * factor + 1;
        const double fcNorm = 0.46 / factor;  // cutoff / fsOversampled
        const double beta = 9.0, mid = (n - 1) * 0.5;
        taps.resize ((size_t) n);
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double t = i - mid;
            const double sinc = std::abs (t) < 1.0e-12 ? 2.0 * fcNorm : std::sin (2.0 * kPiD * fcNorm * t) / (kPiD * t);
            const double r = t / mid;
            const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / besselI0 (beta);
            taps[(size_t) i] = (float) (sinc * w);
            sum += sinc * w;
        }
        for (auto& v : taps) v = (float) (v / sum);
        history.assign ((size_t) n * 2, 0.0f);
        pos = 0;
    }

    void reset() { std::fill (history.begin(), history.end(), 0.0f); pos = 0; }

    // Push `factor` oversampled samples, get one base-rate sample.
    float process (const float* in) noexcept
    {
        const int n = (int) taps.size();
        if (factor == 1) return in[0];
        for (int i = 0; i < factor; ++i)
        {
            history[(size_t) pos] = in[i];
            history[(size_t) (pos + n)] = in[i];   // mirrored for contiguous reads
            if (++pos >= n) pos = 0;
        }
        const float* h = history.data() + pos;    // oldest .. newest
        float acc = 0.0f;
        for (int i = 0; i < n; ++i) acc += taps[(size_t) i] * h[i];
        return acc;
    }

    int getFactor() const noexcept { return factor; }

private:
    std::vector<float> taps, history;
    int factor = 1, pos = 0;

    static double besselI0 (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 50; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; if (term < 1e-12 * sum) break; }
        return sum;
    }
};
} // namespace babushka::dsp
