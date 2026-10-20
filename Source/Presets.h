// Babushka — factory presets (test set).
//
// A preset lists only the parameters that differ from the defaults, in real units (choice index, semitones,
// 0…1 knob value). Everything else — except the "global" settings listed in kGlobalParams — is reset to its
// default when a preset is loaded, so presets do not leak into each other. `patch` is a cable list in the
// Patch.h text format ("jackId:jackId ..."), `seq1…3` are sequences ("60 63T R 67A", see PluginProcessor.cpp).
//
// Choice indices: Osc 1 octave 32' 16' 8' 4' = 0…3; Osc 2 octave 16' 8' 4' 2' = 0…3; waves Triangle, Saw,
// Square, Narrow Pulse = 0…3; LFO wave Sine, Saw, Ramp, Square = 0…3; VCA mode ENV, KB RLS, DRONE = 0…2;
// Kbd track Off, 1:2, 1:1 = 0…2; Arp mode ARP, SEQ, REC = 0…2; direction ORDR, FWD/BKWD, RNDM = 0…2;
// free step 1/4, 1/8, 1/16 = 0…2; Arp/Seq rate is a knob (0.703 ≈ 128 BPM).
#pragma once

#include <string_view>
#include <utility>
#include <vector>

namespace babushka
{
struct FactoryPreset
{
    const char* category;
    const char* name;
    const char* description;
    std::vector<std::pair<const char*, float>> values;
    const char* patch = "";
    const char* seq1 = "";
    const char* seq2 = "";
    const char* seq3 = "";
};

// Parameters a preset never touches: user's tuning, quality and clock-jack setup.
inline bool isGlobalParam (const char* id)
{
    static const char* kGlobalParams[] = { "oversampling", "bendRange", "notePriority", "fineTune", "drift", "extLevel",
                                           "extClockMode", "extClockPpqn", "clockOutPpqn", "localOn", "kbOutRange",
                                           "reverbDecay" };
    for (auto* g : kGlobalParams) if (std::string_view (g) == id) return true;
    return false;
}

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        // ---------------------------------------------------------------- Basic
        { "Basic", "Init", "All knobs at their defaults, no cables.", {} },

        { "Basic", "Classic Bass", "Two detuned saws at 16', closed filter with an envelope sweep. Play low notes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f },
            { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.32f }, { "resonance", 0.35f }, { "envAmt", 0.45f },
            { "attack", 0.0f }, { "decay", 0.35f }, { "sustain", 0.45f }, { "release", 0.2f }, { "kbdTrack", 1 } } },

        { "Basic", "Sync Lead", "Hard-synced oscillator 2 with glide; the mod wheel adds vibrato (try CC1) and OSC 2 Frequency sweeps the sync tone.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Sync", 1 }, { "osc2Freq", 5.0f },
            { "mixOsc1", 0.25f }, { "mixOsc2", 0.75f }, { "cutoff", 0.72f }, { "resonance", 0.25f }, { "envAmt", 0.2f },
            { "decay", 0.5f }, { "sustain", 0.8f }, { "release", 0.3f }, { "glide", 0.35f }, { "glideType", 2 },
            { "lfoRate", 0.42f }, { "lfoPitchAmt", 0.18f }, { "reverbMix", 0.22f } } },

        { "Basic", "Sine Oscillator", "The filter self-oscillates and is played from the keyboard (Kbd Track 1:1): a pure sine.",
          { { "mixOsc1", 0.0f }, { "resonance", 1.0f }, { "kbdTrack", 2 }, { "cutoff", 0.66f }, { "sustain", 1.0f }, { "release", 0.3f } } },

        { "Basic", "Spring Pluck", "Narrow pulse pluck into the spring reverb; 50 % wet.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "mixOsc1", 0.6f }, { "cutoff", 0.55f }, { "envAmt", 0.5f }, { "resonance", 0.2f },
            { "decay", 0.3f }, { "sustain", 0.0f }, { "release", 0.3f }, { "reverbMix", 0.5f }, { "kbdTrack", 1 } } },

        { "Basic", "LFO Wobble Bass", "Square + saw, noise, cutoff wobbled by the mod LFO (mod wheel fully up), KB RLS VCA.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", -0.1f },
            { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "mixNoise", 0.15f }, { "cutoff", 0.42f }, { "resonance", 0.55f },
            { "lfoRate", 0.38f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.45f }, { "modWheel", 1.0f },
            { "vcaMode", 1 }, { "sustain", 0.9f }, { "release", 0.35f }, { "reverbMix", 0.15f } } },

        { "Basic", "Warm Pad", "Two square waves with slow pulse-width modulation, long envelope, reverb.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.12f },
            { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.45f }, { "resonance", 0.15f }, { "envAmt", 0.25f },
            { "attack", 0.55f }, { "decay", 0.6f }, { "sustain", 0.7f }, { "release", 0.6f },
            { "lfoRate", 0.2f }, { "lfoWave", 0 }, { "lfoPwAmt", 0.4f }, { "modWheel", 0.8f }, { "reverbMix", 0.35f } } },

        { "Basic", "Brass Swell", "Detuned saws with a slow filter swell and singing resonance; reverb up.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f },
            { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.3f }, { "resonance", 0.55f }, { "envAmt", 0.6f },
            { "attack", 0.35f }, { "decay", 0.5f }, { "sustain", 0.55f }, { "release", 0.6f },
            { "lfoRate", 0.3f }, { "lfoPitchAmt", 0.08f }, { "modWheel", 0.25f }, { "reverbMix", 0.5f } } },

        { "Basic", "Noise Hit", "Filtered noise burst for percussion; play any key.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 0.9f }, { "cutoff", 0.5f }, { "resonance", 0.3f }, { "envAmt", 0.6f },
            { "decay", 0.22f }, { "sustain", 0.0f }, { "release", 0.15f }, { "reverbMix", 0.3f } } },

        // ---------------------------------------------------------------- Patched (cables)
        { "Patched", "Gated Drone", "DRONE VCA chopped by the mod LFO (square) through VCA AMT IN. Cable: MOD WAVE OUT -> VCA AMT IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f },
            { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.55f }, { "vcaMode", 2 },
            { "lfoRate", 0.485f }, { "lfoWave", 3 }, { "reverbMix", 0.25f } },
          "modWaveOut:vcaAmtIn" },

        { "Patched", "Velocity Lead", "Play harder for a brighter sound. Cable: KB VEL OUT -> CUTOFF IN (through the attenuator).",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f },
            { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "cutoff", 0.28f }, { "resonance", 0.3f }, { "envAmt", 0.25f },
            { "decay", 0.4f }, { "sustain", 0.7f }, { "release", 0.3f }, { "attenuator", 0.35f }, { "reverbMix", 0.15f } },
          "kbVelOut:attIn attOut:filterCutoffIn" },

        { "Patched", "Keytracked Wobble", "The LFO rate follows the keyboard (1 V/oct): higher notes wobble faster. Cable: KB OUT -> RATE IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f },
            { "cutoff", 0.35f }, { "resonance", 0.5f }, { "lfoRate", 0.05f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.5f }, { "modWheel", 1.0f },
            { "vcaMode", 1 }, { "sustain", 1.0f }, { "release", 0.25f } },
          "kbOut:modRateIn" },

        { "Patched", "Split Bass and Thin", "Oscillator 1 stays full, oscillator 2 goes through the high-pass filter first. Cables: OSC 2 WAVE OUT -> HIGH PASS IN -> MIXER OSC 2 IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f },
            { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.62f }, { "resonance", 0.2f }, { "hpCutoff", 0.62f },
            { "decay", 0.4f }, { "sustain", 0.8f }, { "release", 0.25f } },
          "osc2WaveOut:hpfIn hpfOut:mixOsc2In" },

        { "Patched", "Random Notes", "Every key press picks a new random pitch: S/H sampled by GATE OUT -> SYNC IN, attenuated and sent to both oscillators through the mult.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.4f },
            { "cutoff", 0.55f }, { "resonance", 0.35f }, { "envAmt", 0.3f }, { "lfoRate", 0.0f }, { "attenuator", 0.2f },
            { "decay", 0.3f }, { "sustain", 0.3f }, { "release", 0.2f }, { "reverbMix", 0.2f } },
          "gateOut:modSyncIn modShOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "Patched", "FM Bell", "Oscillator 1 frequency-modulates oscillator 2 (linear FM); only oscillator 2 is heard. Cable: OSC 1 WAVE OUT -> OSC 2 LIN FM IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 0 }, { "osc2Freq", 7.0f },
            { "mixOsc1", 0.0f }, { "mixOsc2", 0.6f }, { "cutoff", 0.8f }, { "envAmt", 0.3f },
            { "decay", 0.45f }, { "sustain", 0.0f }, { "release", 0.5f }, { "reverbMix", 0.4f } },
          "osc1WaveOut:osc2LinFmIn" },

        // ---------------------------------------------------------------- Arp / Seq
        { "Arp / Seq", "Arp Minor Pluck", "Hold a chord: forward-backward over two octaves, 1/16 at 128 BPM.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f },
            { "mixOsc1", 0.5f }, { "mixOsc2", 0.35f }, { "cutoff", 0.45f }, { "resonance", 0.4f }, { "envAmt", 0.55f },
            { "decay", 0.22f }, { "sustain", 0.0f }, { "release", 0.2f }, { "reverbMix", 0.25f }, { "kbdTrack", 1 },
            { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 1 }, { "arpOctSeq", 1 }, { "arpRate", 0.703f }, { "arpFreeStep", 2 } } },

        { "Arp / Seq", "Seq Acid Line", "Plays by itself. Press a key to transpose. Accents open the filter through KB VEL OUT -> attenuator -> CUTOFF IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.65f }, { "cutoff", 0.3f }, { "resonance", 0.65f }, { "envAmt", 0.6f },
            { "decay", 0.3f }, { "sustain", 0.1f }, { "release", 0.1f }, { "glide", 0.25f }, { "legatoGlide", 1 }, { "attenuator", 0.3f },
            { "reverbMix", 0.12f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.703f }, { "arpFreeStep", 2 } },
          "kbVelOut:attIn attOut:filterCutoffIn",
          "36 36 48 36A 39 R 43 36 36T 48 36A 46 R 41 43 36",
          "48 52 55 60 55 52 48 55",
          "36 R 36 R 43 R 41 R" },
    };
    return presets;
}
} // namespace babushka
