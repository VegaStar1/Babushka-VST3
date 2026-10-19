// Babushka — band-limited VCO.
//
// Naive waveforms corrected with 2-point polynomial BLEP (steps) and BLAMP (slope changes).
// Every discontinuity, including hard-sync resets, is placed at its exact fractional time.
// Output is delayed by one sample so corrections can be applied on both sides of an event.
//
// Waveforms follow the Grandmother panel: Triangle, Sawtooth, Square, Narrow Pulse.
#pragma once

#include "Common.h"

namespace babushka::dsp
{
class Oscillator
{
public:
    enum class Wave { Triangle = 0, Saw, Square, Pulse };

    void reset (float startPhase = 0.0f) noexcept { phase = startPhase; delayed = naive (phase); }
    void setWave (Wave w) noexcept { wave = w; }
    // Pulse width for Square (nominally 0.5) and Pulse (narrow) — includes PWM.
    void setPulseWidth (float pw) noexcept { pulseWidth = clampf (pw, 0.03f, 0.97f); }

    // Advance one sample. dt = frequency / sampleRate (cycles per sample, < 0.5).
    // Returns true if the oscillator wrapped this sample; wrapFrac = time (in samples) from the
    // wrap to the end of the sample. Used to hard-sync another oscillator.
    float process (float dt, bool& wrapped, float& wrapFrac) noexcept
    {
        dt = clampf (dt, 1.0e-7f, 0.45f);
        current = 0.0f;
        wrapped = false;
        float p = phase;
        scanSegment (p, p + dt, 0.0f, dt, &wrapped, &wrapFrac);
        p += dt;
        if (p >= 1.0f) p -= 1.0f;
        phase = p;
        return finish();
    }

    // As process(), but with a hard-sync reset happening syncFrac samples before the end of this sample.
    float processSynced (float dt, float syncFrac) noexcept
    {
        dt = clampf (dt, 1.0e-7f, 0.45f);
        current = 0.0f;
        bool w; float wf;
        const float before = 1.0f - syncFrac;              // time from start of sample to the reset
        float pA = phase + dt * before;
        scanSegment (phase, pA, syncFrac, dt, &w, &wf);
        if (pA >= 1.0f) pA -= 1.0f;

        // Value step at the reset.
        const float h = naive (0.0f) - naive (pA);
        if (std::abs (h) > 1.0e-9f) addStep (h, syncFrac);
        if (wave == Wave::Triangle)
        {
            // Slope at reset jumps to the rising slope.
            const float slopeBefore = pA < 0.5f ? 4.0f : -4.0f;
            addRamp ((4.0f - slopeBefore) * dt, syncFrac);
        }

        const float pB = dt * syncFrac;
        scanSegment (0.0f, pB, 0.0f, dt, &w, &wf);
        phase = pB;
        return finish();
    }

private:
    Wave wave = Wave::Saw;
    float phase = 0.0f, pulseWidth = 0.5f;
    float delayed = 0.0f, current = 0.0f;   // correction accumulators

    float naive (float p) const noexcept
    {
        switch (wave)
        {
            case Wave::Triangle: return p < 0.5f ? (-1.0f + 4.0f * p) : (3.0f - 4.0f * p);
            case Wave::Saw:      return 2.0f * p - 1.0f;
            case Wave::Square:
            case Wave::Pulse:    return p < pulseWidth ? 1.0f : -1.0f;
        }
        return 0.0f;
    }

    // Step of height h, occurring f samples before the end of this sample.
    void addStep (float h, float f) noexcept
    {
        const float half = 0.5f * h, g = 1.0f - f;
        delayed += half * f * f;
        current -= half * g * g;
    }

    // Slope change of m (units per sample), occurring f samples before the end of this sample.
    void addRamp (float m, float f) noexcept
    {
        const float g = 1.0f - f;
        delayed += m * f * f * f * (1.0f / 6.0f);
        current += m * g * g * g * (1.0f / 6.0f);
    }

    // Emit events for breakpoints b with pStart < b <= pEnd. tEnd = time from pEnd to end of sample.
    void scanSegment (float pStart, float pEnd, float tEnd, float dt, bool* wrapped, float* wrapFrac) noexcept
    {
        auto timeOf = [&] (float b) { return tEnd + (pEnd - b) / dt; };
        auto crosses = [&] (float b) { return pStart < b && b <= pEnd; };

        const bool wrap = crosses (1.0f);
        if (wrap) { *wrapped = true; *wrapFrac = timeOf (1.0f); }

        switch (wave)
        {
            case Wave::Saw:
                if (wrap) addStep (-2.0f, timeOf (1.0f));
                break;
            case Wave::Square:
            case Wave::Pulse:
                if (crosses (pulseWidth))        addStep (-2.0f, timeOf (pulseWidth));
                if (wrap)                        addStep ( 2.0f, timeOf (1.0f));
                if (crosses (pulseWidth + 1.0f)) addStep (-2.0f, timeOf (pulseWidth + 1.0f));
                break;
            case Wave::Triangle:
                if (crosses (0.5f)) addRamp (-8.0f * dt, timeOf (0.5f));
                if (wrap)           addRamp ( 8.0f * dt, timeOf (1.0f));
                if (crosses (1.5f)) addRamp (-8.0f * dt, timeOf (1.5f));
                break;
        }
    }

    float finish() noexcept
    {
        const float out = delayed;
        delayed = naive (phase) + current;
        return out;
    }
};
} // namespace babushka::dsp
