// Babushka — the pedals' parameters, chain order and factory pedal presets (shared by the processor and the editor).
#pragma once

#include <array>
#include <string_view>
#include <vector>

namespace babushka
{
constexpr int kNumPedals = 4;

struct PedalKnobInfo { const char* id; const char* name; };

struct PedalInfo
{
    const char* name;                         // on the pedal
    const char* prefix;                       // of its parameter ids
    const char* onId, *modeId;
    std::vector<PedalKnobInfo> knobs;         // in the order the menu shows them; the first two are on the pedal
    const char* cvJackLabel;                  // what its CV jack moves
};

inline const std::array<PedalInfo, kNumPedals>& pedalInfo()
{
    static const std::array<PedalInfo, kNumPedals> info { {
        { "SATURATOR", "sat", "satOn", "satMode",
          { { "satDrive", "DRIVE" }, { "satMix", "MIX" }, { "satBias", "BIAS" }, { "satBody", "BODY" }, { "satTone", "TONE" }, { "satOutput", "OUTPUT" } }, "DRIVE" },
        { "PHASER", "pha", "phaOn", "phaMode",
          { { "phaRate", "RATE" }, { "phaDepth", "DEPTH" }, { "phaFeedback", "COLOR" }, { "phaCenter", "CENTER" }, { "phaStereo", "STEREO" }, { "phaMix", "MIX" } }, "RATE" },
        { "BBD CHORUS", "cho", "choOn", "choMode",
          { { "choRate", "RATE" }, { "choDepth", "DEPTH" }, { "choMix", "MIX" }, { "choWidth", "WIDTH" }, { "choNoise", "NOISE" }, { "choAge", "AGE" } }, "RATE" },
        { "TAPE ECHO", "tap", "tapOn", "tapHeads",
          { { "tapTime", "TIME" }, { "tapFeedback", "FEEDBACK" }, { "tapMix", "MIX" }, { "tapWow", "WOW" }, { "tapFlutter", "FLUTTER" }, { "tapAge", "AGE" },
            { "tapBass", "BASS" }, { "tapTreble", "TREBLE" } }, "TIME" },
    } };
    return info;
}

// The pedal a parameter belongs to (−1: none). "fxOrder" belongs to the board as a whole (returns kNumPedals).
inline int pedalOfParam (std::string_view id)
{
    if (id == "fxOrder") return kNumPedals;
    for (int i = 0; i < kNumPedals; ++i)
        if (id.substr (0, 3) == pedalInfo()[(size_t) i].prefix && id.size() > 3 && id[3] >= 'A' && id[3] <= 'Z') return i;
    return -1;
}

// Chain order ↔ the "fxOrder" parameter (0 … 23: the permutations of the four pedals in lexicographic order).
inline std::array<int, kNumPedals> pedalOrderFromIndex (int index)
{
    std::array<int, kNumPedals> order {};
    std::vector<int> left { 0, 1, 2, 3 };
    int fact[] = { 6, 2, 1, 1 };
    index = index < 0 || index > 23 ? 0 : index;
    for (int k = 0; k < kNumPedals; ++k)
    {
        const int pick = index / fact[k];
        index %= fact[k];
        order[(size_t) k] = left[(size_t) pick];
        left.erase (left.begin() + pick);
    }
    return order;
}
inline int pedalOrderIndex (const std::array<int, kNumPedals>& order)
{
    for (int i = 0; i < 24; ++i) if (pedalOrderFromIndex (i) == order) return i;
    return 0;
}

// ---- factory presets of the pedals (values in real units: knob 0 … 1, choice index) -------------------------------------
struct PedalPreset { const char* name; std::vector<std::pair<const char*, float>> values; const char* about; };

inline const std::vector<PedalPreset>& factoryPedalPresets (int pedal)
{
    static const std::array<std::vector<PedalPreset>, kNumPedals> all { {
        {   // SATURATOR
            { "Default", { { "satMode", 0 }, { "satDrive", 0.4f }, { "satBias", 0.35f }, { "satBody", 0.35f }, { "satTone", 0.5f }, { "satMix", 1.0f }, { "satOutput", 0.5f } },
              "The pedal as it comes: the triode at a moderate drive, every other knob in the middle. A start for your own setting." },
            { "Warm Triode", { { "satMode", 0 }, { "satDrive", 0.55f }, { "satBias", 0.35f }, { "satBody", 0.4f }, { "satTone", 0.5f }, { "satMix", 1.0f }, { "satOutput", 0.5f } },
              "A 12AX7 stage pushed gently: rounder, a touch of 2nd harmonic, peaks sag a little." },
            { "Hot Preamp", { { "satMode", 0 }, { "satDrive", 0.85f }, { "satBias", 0.55f }, { "satBody", 0.3f }, { "satTone", 0.62f }, { "satMix", 1.0f }, { "satOutput", 0.45f } },
              "The triode driven hard and biased off-centre: thick, singing overdrive." },
            { "Subtle Glow", { { "satMode", 0 }, { "satDrive", 0.25f }, { "satBias", 0.25f }, { "satBody", 0.3f }, { "satTone", 0.5f }, { "satMix", 0.7f }, { "satOutput", 0.5f } },
              "Barely there: a little warmth and weight without audible distortion." },
            { "Tape Glue", { { "satMode", 1 }, { "satDrive", 0.5f }, { "satBias", 0.0f }, { "satBody", 0.45f }, { "satTone", 0.42f }, { "satMix", 1.0f }, { "satOutput", 0.5f } },
              "Tape saturation: soft compression, a head bump in the lows, the highs rounded off." },
            { "Hot Tape", { { "satMode", 1 }, { "satDrive", 0.8f }, { "satBias", 0.1f }, { "satBody", 0.5f }, { "satTone", 0.38f }, { "satMix", 1.0f }, { "satOutput", 0.45f } },
              "Tape hit hard: dense, darker, the transients squashed." },
            { "Iron Bass", { { "satMode", 2 }, { "satDrive", 0.6f }, { "satBias", 0.1f }, { "satBody", 0.75f }, { "satTone", 0.4f }, { "satMix", 1.0f }, { "satOutput", 0.5f } },
              "An output transformer saturating on the lows: big, solid bass, clean mids." },
            { "Diode Crunch", { { "satMode", 3 }, { "satDrive", 0.75f }, { "satBias", 0.2f }, { "satBody", 0.35f }, { "satTone", 0.6f }, { "satMix", 0.85f }, { "satOutput", 0.45f } },
              "Silicon diodes in the feedback loop: brighter, edgier overdrive." },
        },
        {   // PHASER
            { "Default", { { "phaMode", 0 }, { "phaRate", 0.3f }, { "phaDepth", 0.7f }, { "phaFeedback", 0.25f }, { "phaCenter", 0.5f }, { "phaStereo", 0.4f }, { "phaMix", 0.5f } },
              "The pedal as it comes: four stages, a medium sweep, a little colour. A start for your own setting." },
            { "Vintage Swirl", { { "phaMode", 0 }, { "phaRate", 0.3f }, { "phaDepth", 0.8f }, { "phaFeedback", 0.2f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } },
              "Four JFET stages, a slow round swirl with two notches." },
            { "Orange Slow", { { "phaMode", 0 }, { "phaRate", 0.12f }, { "phaDepth", 0.9f }, { "phaFeedback", 0.1f }, { "phaCenter", 0.5f }, { "phaStereo", 0.2f }, { "phaMix", 0.5f } },
              "The classic slow sweep of the little four-stage phasers." },
            { "Color Jet", { { "phaMode", 2 }, { "phaRate", 0.22f }, { "phaDepth", 1.0f }, { "phaFeedback", 0.75f }, { "phaCenter", 0.45f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } },
              "OTA stages with feedback: resonant peaks, the jet-plane sweep." },
            { "Deep Eight", { { "phaMode", 3 }, { "phaRate", 0.25f }, { "phaDepth", 0.85f }, { "phaFeedback", 0.5f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } },
              "Eight stages, four notches: thick phasing for pads." },
            { "Fast Warble", { { "phaMode", 1 }, { "phaRate", 0.7f }, { "phaDepth", 0.5f }, { "phaFeedback", 0.3f }, { "phaCenter", 0.55f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } },
              "Six stages at speed: a watery, rotary-like warble." },
            { "Stereo Spin", { { "phaMode", 1 }, { "phaRate", 0.33f }, { "phaDepth", 0.8f }, { "phaFeedback", 0.35f }, { "phaCenter", 0.5f }, { "phaStereo", 1.0f }, { "phaMix", 0.5f } },
              "Left and right sweep in opposite directions: the sound spins round the head." },
            { "Slow Wash", { { "phaMode", 3 }, { "phaRate", 0.08f }, { "phaDepth", 0.7f }, { "phaFeedback", 0.3f }, { "phaCenter", 0.4f }, { "phaStereo", 0.6f }, { "phaMix", 0.6f } },
              "A very slow, wide movement for drones and pads." },
        },
        {   // BBD CHORUS
            { "Default", { { "choMode", 0 }, { "choRate", 0.45f }, { "choDepth", 0.5f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.2f }, { "choAge", 0.25f } },
              "The pedal as it comes: mode I, medium rate and depth, full width. A start for your own setting." },
            { "Mode I", { { "choMode", 0 }, { "choRate", 0.4236f }, { "choDepth", 0.6f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f } },
              "In the manner of the classic polysynth chorus, mode I: ≈0.5 Hz, wide and soft." },
            { "Mode II", { { "choMode", 1 }, { "choRate", 0.519f }, { "choDepth", 0.75f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f } },
              "Mode II: ≈0.9 Hz and deeper, more obviously moving." },
            { "I + II", { { "choMode", 2 }, { "choRate", 0.962f }, { "choDepth", 0.25f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f } },
              "Both buttons: a fast, shallow shimmer." },
            { "Slow Ensemble", { { "choMode", 4 }, { "choRate", 0.234f }, { "choDepth", 0.7f }, { "choMix", 0.55f }, { "choWidth", 1.0f }, { "choNoise", 0.2f }, { "choAge", 0.25f } },
              "Three lines per side, string-machine style: thick without an obvious wobble." },
            { "Vibrato", { { "choMode", 3 }, { "choRate", 0.858f }, { "choDepth", 0.55f }, { "choMix", 1.0f }, { "choWidth", 1.0f }, { "choNoise", 0.2f }, { "choAge", 0.3f } },
              "Wet only: pitch vibrato from the bucket brigade." },
            { "Wide Pad", { { "choMode", 1 }, { "choRate", 0.355f }, { "choDepth", 0.85f }, { "choMix", 0.6f }, { "choWidth", 1.0f }, { "choNoise", 0.15f }, { "choAge", 0.2f } },
              "Slow and deep, as wide as it gets." },
            { "Dirty BBD", { { "choMode", 0 }, { "choRate", 0.453f }, { "choDepth", 0.65f }, { "choMix", 0.5f }, { "choWidth", 0.8f }, { "choNoise", 0.8f }, { "choAge", 0.85f } },
              "A tired unit: dark, hissy, the compander breathing." },
        },
        {   // TAPE ECHO
            { "Default", { { "tapHeads", 0 }, { "tapSync", 0 }, { "tapTime", 0.45f }, { "tapFeedback", 0.35f }, { "tapMix", 0.35f }, { "tapWow", 0.2f }, { "tapFlutter", 0.2f }, { "tapAge", 0.3f }, { "tapBass", 0.5f }, { "tapTreble", 0.5f } },
              "The pedal as it comes: head 1, a few repeats, a little wow and flutter. A start for your own setting." },
            { "Slapback", { { "tapHeads", 0 }, { "tapSync", 0 }, { "tapTime", 0.3736f }, { "tapFeedback", 0.12f }, { "tapMix", 0.45f }, { "tapWow", 0.15f }, { "tapFlutter", 0.2f }, { "tapAge", 0.3f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } },
              "One short repeat, rockabilly style: head 1, almost no feedback." },
            { "Dub Echo", { { "tapHeads", 5 }, { "tapSync", 0 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.5f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f } },
              "Heads 1 + 3, long tail, every repeat darker and dirtier than the last." },
            { "Three Heads", { { "tapHeads", 6 }, { "tapSync", 0 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.45f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapBass", 0.5f }, { "tapTreble", 0.5f } },
              "All three heads at once: a rhythmic pattern of repeats." },
            { "Runaway", { { "tapHeads", 4 }, { "tapSync", 0 }, { "tapTime", 0.435f }, { "tapFeedback", 0.97f }, { "tapMix", 0.5f }, { "tapWow", 0.35f }, { "tapFlutter", 0.3f }, { "tapAge", 0.5f }, { "tapBass", 0.55f }, { "tapTreble", 0.45f } },
              "On the edge of self-oscillation: the echo builds and howls, held by the tape's saturation." },
            { "Wobbly Cassette", { { "tapHeads", 1 }, { "tapSync", 0 }, { "tapTime", 0.406f }, { "tapFeedback", 0.4f }, { "tapMix", 0.45f }, { "tapWow", 0.85f }, { "tapFlutter", 0.6f }, { "tapAge", 0.85f }, { "tapBass", 0.45f }, { "tapTreble", 0.35f } },
              "Stretched old tape: strong wow and flutter, dull highs, hiss." },
            { "Dotted Eighth", { { "tapHeads", 0 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.4f }, { "tapMix", 0.4f }, { "tapWow", 0.15f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } },
              "Locked to the DAW's tempo (dotted 1/8), still sounding like tape." },
            { "Ambient Wash", { { "tapHeads", 6 }, { "tapSync", 0 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.55f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f } },
              "Long repeats from every head plus the spring: an endless cloud." },
        },
    } };
    return all[(size_t) (pedal < 0 || pedal >= kNumPedals ? 0 : pedal)];
}
} // namespace babushka
