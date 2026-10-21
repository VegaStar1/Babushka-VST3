// Babushka — patch panel model: 53 patch points (29 inputs, 20 outputs, 4 parallel-wired Mult jacks),
// cables, and compilation of a patch into per-input routing tables. 41 of them are the hardware's; the IN / CV / OUT
// jacks of the four pedals on the rear panel are plugin-only.
//
// The jack list follows the Grandmother manual (Patch Point Index, signal-flow diagram). Every input has a
// "normalled" internal source; plugging a cable into an input breaks that connection (even if the cable
// carries nothing). Cables are not directional: the net a cable belongs to is driven by the output jacks
// in it, and read by the input jacks in it. The four Mult jacks are one electrical node.
//
// Signal scales used by the engine (volts): audio ±5 V = ±1.0 inside the voice, control voltages 1 V/oct,
// gate +8 V, envelope 0…+8 V, keyboard CV ±5 V, velocity 0…+5 V, clock 0…+5 V.
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace babushka::dsp
{
enum class Jack : uint8_t
{
    // ---- inputs (21) ----
    ModRateIn, ModSyncIn,
    Osc1PitchIn, Osc1PwmIn, Osc2PitchIn, Osc2LinFmIn,
    MixOsc1In, MixOsc2In, MixNoiseIn,
    HpfIn, AttenuatorIn,
    FilterIn, FilterEnvAmtIn, FilterCutoffIn,
    EnvTriggerIn,
    VcaAmtIn, VcaIn, ReverbIn,
    ClockIn, OnOffIn, ResetIn,
    SatIn, SatCv, PhaIn, PhaCv, ChoIn, ChoCv, TapIn, TapCv,        // pedals (plugin-only)
    // ---- outputs (20) ----
    GateOut, KbOut, KbVelOut,
    ModWaveOut, ModShOut,
    Osc1WaveOut, Osc2WaveOut,
    MixerOut,
    HpfOut, AttenuatorOut,
    FilterOut,
    EnvPosOut, EnvNegOut,
    ClockOut, EuroOut, ReverbOut,
    SatOut, PhaOut, ChoOut, TapOut,                                  // pedals (plugin-only)
    // ---- parallel-wired mult (4) ----
    Mult1, Mult2, Mult3, Mult4,
    Count
};

constexpr int kNumJacks   = (int) Jack::Count;          // 53
constexpr int kNumInputs  = (int) Jack::GateOut;        // 29
constexpr int kNumOutputs = (int) Jack::Mult1 - kNumInputs;   // 20
constexpr int kFirstOutput = kNumInputs;
constexpr int kFirstMult   = (int) Jack::Mult1;

enum class JackKind : uint8_t { Input, Output, Mult };

struct JackInfo { const char* id; const char* section; const char* label; JackKind kind; };

inline const JackInfo& jackInfo (Jack j) noexcept
{
    static const JackInfo table[kNumJacks] = {
        { "modRateIn",      "Modulation", "Rate In",         JackKind::Input },
        { "modSyncIn",      "Modulation", "Sync In",         JackKind::Input },
        { "osc1PitchIn",    "Oscillators", "Osc 1 Pitch In", JackKind::Input },
        { "osc1PwmIn",      "Oscillators", "Osc 1 PWM In",   JackKind::Input },
        { "osc2PitchIn",    "Oscillators", "Osc 2 Pitch In", JackKind::Input },
        { "osc2LinFmIn",    "Oscillators", "Osc 2 Lin FM In", JackKind::Input },
        { "mixOsc1In",      "Mixer",      "Osc 1 In",        JackKind::Input },
        { "mixOsc2In",      "Mixer",      "Osc 2 In",        JackKind::Input },
        { "mixNoiseIn",     "Mixer",      "Noise In",        JackKind::Input },
        { "hpfIn",          "Utilities",  "High Pass In",    JackKind::Input },
        { "attIn",          "Utilities",  "Attenuator In",   JackKind::Input },
        { "filterIn",       "Filter",     "Filter In",       JackKind::Input },
        { "filterEnvAmtIn", "Filter",     "Env Amt In",      JackKind::Input },
        { "filterCutoffIn", "Filter",     "Cutoff In",       JackKind::Input },
        { "envTriggerIn",   "Envelope",   "Trigger In",      JackKind::Input },
        { "vcaAmtIn",       "Output",     "VCA Amt In",      JackKind::Input },
        { "vcaIn",          "Output",     "VCA In",          JackKind::Input },
        { "reverbIn",       "Output",     "Reverb In",       JackKind::Input },
        { "clockIn",        "Arp/Seq (rear)", "Clock In",    JackKind::Input },
        { "onOffIn",        "Arp/Seq (rear)", "On/Off In",   JackKind::Input },
        { "resetIn",        "Arp/Seq (rear)", "Reset In",    JackKind::Input },
        { "satIn",          "Saturator",  "In",              JackKind::Input },
        { "satCv",          "Saturator",  "Drive CV",        JackKind::Input },
        { "phaIn",          "Phaser",     "In",              JackKind::Input },
        { "phaCv",          "Phaser",     "Rate CV",         JackKind::Input },
        { "choIn",          "BBD Chorus", "In",              JackKind::Input },
        { "choCv",          "BBD Chorus", "Rate CV",         JackKind::Input },
        { "tapIn",          "Tape Echo",  "In",              JackKind::Input },
        { "tapCv",          "Tape Echo",  "Time CV",         JackKind::Input },

        { "gateOut",        "Arp/Seq",    "Gate Out",        JackKind::Output },
        { "kbOut",          "Arp/Seq",    "KB Out",          JackKind::Output },
        { "kbVelOut",       "Arp/Seq",    "KB Vel Out",      JackKind::Output },
        { "modWaveOut",     "Modulation", "Wave Out",        JackKind::Output },
        { "modShOut",       "Modulation", "S/H Out",         JackKind::Output },
        { "osc1WaveOut",    "Oscillators", "Osc 1 Wave Out", JackKind::Output },
        { "osc2WaveOut",    "Oscillators", "Osc 2 Wave Out", JackKind::Output },
        { "mixerOut",       "Mixer",      "Mixer Out",       JackKind::Output },
        { "hpfOut",         "Utilities",  "High Pass Out",   JackKind::Output },
        { "attOut",         "Utilities",  "Attenuator Out",  JackKind::Output },
        { "filterOut",      "Filter",     "Filter Out",      JackKind::Output },
        { "envPosOut",      "Envelope",   "+ Env Out",       JackKind::Output },
        { "envNegOut",      "Envelope",   "- Env Out",       JackKind::Output },
        { "clockOut",       "Arp/Seq (rear)", "Clock Out",   JackKind::Output },
        { "euroOut",        "Audio (rear)", "Eurorack Out",  JackKind::Output },
        { "reverbOut",      "Audio (rear)", "Reverb Out",    JackKind::Output },
        { "satOut",         "Saturator",  "Out",             JackKind::Output },
        { "phaOut",         "Phaser",     "Out",             JackKind::Output },
        { "choOut",         "BBD Chorus", "Out",             JackKind::Output },
        { "tapOut",         "Tape Echo",  "Out",             JackKind::Output },

        { "mult1",          "Utilities",  "Mult 1",          JackKind::Mult },
        { "mult2",          "Utilities",  "Mult 2",          JackKind::Mult },
        { "mult3",          "Utilities",  "Mult 3",          JackKind::Mult },
        { "mult4",          "Utilities",  "Mult 4",          JackKind::Mult },
    };
    return table[(int) j];
}

inline bool jackFromId (const std::string& id, Jack& out) noexcept
{
    for (int i = 0; i < kNumJacks; ++i)
        if (id == jackInfo ((Jack) i).id) { out = (Jack) i; return true; }
    return false;
}

struct Cable { Jack a = Jack::ModRateIn, b = Jack::ModRateIn; };

constexpr int kMaxCables = 32;      // the hardware ships with a handful of patch cables; 32 is plenty
constexpr int kMaxSources = 4;      // outputs merged into one input (Mult, audio only in the hardware)

// What the user has plugged in.
struct PatchState
{
    std::array<Cable, kMaxCables> cables {};
    int count = 0;

    bool add (Jack a, Jack b) noexcept
    {
        if (a == b || count >= kMaxCables) return false;
        for (int i = 0; i < count; ++i)
            if ((cables[(size_t) i].a == a && cables[(size_t) i].b == b) || (cables[(size_t) i].a == b && cables[(size_t) i].b == a)) return false;
        cables[(size_t) count++] = { a, b };
        return true;
    }
    void remove (int index) noexcept
    {
        if (index < 0 || index >= count) return;
        for (int i = index; i < count - 1; ++i) cables[(size_t) i] = cables[(size_t) i + 1];
        --count;
    }
    void clear() noexcept { count = 0; }

    // "a:b a:b ..." using jack ids; unknown ids are skipped.
    std::string toString() const
    {
        std::string s;
        for (int i = 0; i < count; ++i)
        {
            if (! s.empty()) s += ' ';
            s += jackInfo (cables[(size_t) i].a).id; s += ':'; s += jackInfo (cables[(size_t) i].b).id;
        }
        return s;
    }
    static PatchState fromString (const std::string& text)
    {
        PatchState p;
        size_t pos = 0;
        while (pos < text.size())
        {
            size_t end = text.find (' ', pos); if (end == std::string::npos) end = text.size();
            const std::string tok = text.substr (pos, end - pos);
            const size_t colon = tok.find (':');
            Jack a, b;
            if (colon != std::string::npos && jackFromId (tok.substr (0, colon), a) && jackFromId (tok.substr (colon + 1), b)) p.add (a, b);
            pos = end + 1;
        }
        return p;
    }
};

// Compiled form used by the audio thread: no allocation, no searching.
struct Routing
{
    bool inputPatched[kNumInputs] {};                       // a cable is plugged in → normalled source is cut
    int  sourceCount[kNumInputs] {};
    uint8_t source[kNumInputs][kMaxSources] {};             // indices into the output array (0 … kNumOutputs-1)
    bool outputUsed[kNumOutputs] {};                        // some input reads this output → compute it

    static Routing compile (const PatchState& p) noexcept
    {
        Routing r;
        int parent[kNumJacks];
        for (int i = 0; i < kNumJacks; ++i) parent[i] = i;
        auto find = [&] (int x) { while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; } return x; };
        auto join = [&] (int a, int b) { parent[find (a)] = find (b); };

        for (int m = 1; m < 4; ++m) join (kFirstMult, kFirstMult + m);     // the mult jacks are wired in parallel
        bool cabled[kNumJacks] {};
        for (int i = 0; i < p.count; ++i)
        {
            const int a = (int) p.cables[(size_t) i].a, b = (int) p.cables[(size_t) i].b;
            join (a, b); cabled[a] = cabled[b] = true;
        }
        // The mult node counts as cabled for every jack in it as soon as any mult jack carries a cable.
        for (int m = 0; m < 4; ++m) if (cabled[kFirstMult + m]) for (int k = 0; k < 4; ++k) cabled[kFirstMult + k] = true;

        for (int in = 0; in < kNumInputs; ++in)
        {
            if (! cabled[in]) continue;
            r.inputPatched[in] = true;
            const int net = find (in);
            for (int o = 0; o < kNumOutputs; ++o)
            {
                if (! cabled[kFirstOutput + o] || find (kFirstOutput + o) != net) continue;
                if (r.sourceCount[in] < kMaxSources) { r.source[in][r.sourceCount[in]++] = (uint8_t) o; r.outputUsed[o] = true; }
            }
        }
        return r;
    }
};
} // namespace babushka::dsp
