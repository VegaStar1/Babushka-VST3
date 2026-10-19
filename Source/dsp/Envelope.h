// Babushka — ADSR (911-style RC contour generator).
//
// Analogue ADSRs charge and discharge a capacitor through resistors, so every segment is an
// exponential approach. The attack aims past full scale (overshoot target) and switches to decay
// at 1.0, which gives the familiar slightly-convex attack of the hardware.
// Time constants are assumptions (the manual gives no figures) and are the main calibration target.
#pragma once

#include "Common.h"

namespace babushka::dsp
{
class Envelope
{
public:
    enum class Stage { Idle, Attack, Decay, Release };

    void prepare (float sampleRate) noexcept { fs = sampleRate; }
    void reset() noexcept { value = 0.0f; stage = Stage::Idle; }

    // Times in seconds; sustain 0..1.
    void setParameters (float attackS, float decayS, float sustainLevel, float releaseS) noexcept
    {
        // Attack: from 0 towards overshoot target; reaches 1.0 after attackS.
        attackCoeff  = coeffFor (attackS / std::log (kOvershoot / (kOvershoot - 1.0f)));
        // Decay/release: knob time ≈ time to fall by 40 dB.
        decayCoeff   = coeffFor (decayS / 4.6f);
        releaseCoeff = coeffFor (releaseS / 4.6f);
        sustain = clampf (sustainLevel, 0.0f, 1.0f);
    }

    void gateOn() noexcept  { stage = Stage::Attack; }        // retrigger from current level
    void gateOff() noexcept { if (stage != Stage::Idle) stage = Stage::Release; }
    bool isActive() const noexcept { return stage != Stage::Idle; }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Attack:
                value += attackCoeff * (kOvershoot - value);
                if (value >= 1.0f) { value = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                value += decayCoeff * (sustain - value);
                break;
            case Stage::Release:
                value += releaseCoeff * (0.0f - value);
                if (value < 1.0e-5f) { value = 0.0f; stage = Stage::Idle; }
                break;
            case Stage::Idle: break;
        }
        return value;
    }

private:
    static constexpr float kOvershoot = 1.3f;
    float fs = 48000.0f, value = 0.0f, sustain = 1.0f;
    float attackCoeff = 1.0f, decayCoeff = 1.0f, releaseCoeff = 1.0f;
    Stage stage = Stage::Idle;

    float coeffFor (float tau) const noexcept { return tau <= 0.0f ? 1.0f : 1.0f - std::exp (-1.0f / (tau * fs)); }
};
} // namespace babushka::dsp
