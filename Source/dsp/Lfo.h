// Babushka — Modulation oscillator (0.07 Hz – 1.3 kHz) with Sample & Hold of noise.
#pragma once

#include "Common.h"

namespace babushka::dsp
{
class Lfo
{
public:
    enum class Wave { Sine = 0, Saw, Ramp, Square };

    void prepare (float sampleRate) noexcept { fs = sampleRate; }
    void reset() noexcept { phase = 0.0f; held = 0.0f; syncPending = false; }
    void setWave (Wave w) noexcept { wave = w; }
    // Rising edge on SYNC IN: the wave restarts, which is a zero-crossing, so S/H samples the noise again.
    void sync() noexcept { phase = 0.0f; syncPending = true; }

    // Returns the selected wave in [-1, 1]; noiseSample is captured at each cycle start (S/H).
    float process (float rateHz, float noiseSample) noexcept
    {
        phase += rateHz / fs;
        if (phase >= 1.0f) { phase -= std::floor (phase); held = noiseSample; }
        if (syncPending) { held = noiseSample; syncPending = false; }
        switch (wave)
        {
            case Wave::Sine:   return std::sin (kTwoPi * phase);
            case Wave::Saw:    return 1.0f - 2.0f * phase;   // falling
            case Wave::Ramp:   return 2.0f * phase - 1.0f;   // rising
            case Wave::Square: return phase < 0.5f ? 1.0f : -1.0f;
        }
        return 0.0f;
    }

    float sampleAndHold() const noexcept { return held; }

private:
    float fs = 48000.0f, phase = 0.0f, held = 0.0f;
    bool syncPending = false;
    Wave wave = Wave::Sine;
};
} // namespace babushka::dsp
