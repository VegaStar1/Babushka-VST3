// Babushka — arpeggiator and step sequencer (Grandmother-style ARP / SEQ / REC).
//
// ARP:  plays held keys one at a time in the order they were pressed (ORDR), forward-then-back
//       (FWD/BKWD) or random (RNDM), over 1–3 octaves. HOLD latches: keys pressed while others are
//       held are added; a key pressed after all keys were released starts a new pattern.
// SEQ:  three sequences of up to 256 steps (note / rest, with tie and accent flags). Playing a key
//       transposes the sequence relative to its first note.
// REC:  step entry from the keyboard; TIE / REST / ACCENT buttons; holding a key while pressing
//       the next one ties them (legato). The first note entered erases the selected sequence.
// Clock: free tempo 20–280 BPM, or host-synced note divisions (same table as the hardware's CC 90).
//
// ASSUMPTIONS (not in the manual): gate length 50 % of a step; free-tempo step = 1/8 note
// (selectable). ACCENT is an envelope on KB VEL OUT (Engine). Rear-panel jacks: CLOCK IN (clock mode:
// edges advance the clock by 1/PPQN of a beat; step-advance mode: one step per edge), ON/OFF IN (> 2.5 V
// plays), RESET IN (> 2.5 V → start of the pattern), CLOCK OUT (PPQN pulses per quarter note).
#pragma once

#include "Common.h"
#include <array>

namespace babushka::dsp
{
struct SeqStep
{
    uint8_t note = 60;
    bool rest = false;
    bool tie = false;     // this step continues into the next one without a gap
    bool accent = false;
};

struct Sequence
{
    std::array<SeqStep, 256> steps {};
    int length = 0;
};

struct ArpSeqParams
{
    bool play = false, hold = false, sync = false;
    int mode = 0;          // 0 = ARP, 1 = SEQ, 2 = REC
    int direction = 0;     // 0 = ORDR, 1 = FWD/BKWD, 2 = RNDM
    int octSeq = 0;        // ARP: 1/2/3 octaves; SEQ/REC: sequence 1/2/3
    float rate = 0.5f;     // knob 0..1
    int freeStep = 1;      // free tempo step: 0 = 1/4, 1 = 1/8, 2 = 1/16
    // Global settings of the hardware (manual p. 38):
    bool extStepAdvance = false;   // CLOCK IN mode: false = clock, true = step-advance
    int  extPpqn = 2;              // pulses per quarter note expected on CLOCK IN
    int  clockOutPpqn = 2;         // pulses per quarter note sent on CLOCK OUT
};

class ArpSeq
{
public:
    struct Event { int type; int note; };   // type: 1 = note on, 2 = note off
    static constexpr int kNumDivisions = 24;

    // Sync divisions, slowest → fastest, in quarter-note beats (matches CC 90 order).
    static float divisionBeats (int i) noexcept
    {
        static constexpr float d[kNumDivisions] = {
            16.0f, 12.0f, 8.0f, 6.0f, 4.0f, 3.0f, 8.0f / 3.0f, 2.0f, 1.5f, 4.0f / 3.0f, 1.0f, 0.75f,
            2.0f / 3.0f, 0.5f, 0.375f, 1.0f / 3.0f, 0.25f, 0.1875f, 1.0f / 6.0f, 0.125f, 0.09375f, 1.0f / 12.0f, 0.0625f, 1.0f / 24.0f };
        return d[std::min (std::max (i, 0), kNumDivisions - 1)];
    }
    static const char* divisionName (int i) noexcept
    {
        static const char* n[kNumDivisions] = {
            "4 whole", "3 whole", "2 whole", "dotted whole", "whole", "dotted 1/2", "1/1 triplet", "1/2", "dotted 1/4",
            "1/2 triplet", "1/4", "dotted 1/8", "1/4 triplet", "1/8", "dotted 1/16", "1/8 triplet", "1/16",
            "dotted 1/32", "1/16 triplet", "1/32", "dotted 1/64", "1/32 triplet", "1/64", "1/64 triplet" };
        return n[std::min (std::max (i, 0), kNumDivisions - 1)];
    }
    static int divisionIndex (float knob) noexcept { return (int) std::lround (clampf (knob, 0.0f, 1.0f) * (kNumDivisions - 1)); }
    static float freeBpm (float knob) noexcept { return 20.0f * std::pow (14.0f, clampf (knob, 0.0f, 1.0f)); }   // 20–280 BPM
    static float knobForBpm (float bpm) noexcept { return std::log (clampf (bpm, 20.0f, 280.0f) / 20.0f) / std::log (14.0f); }

    void prepare (float sampleRate) { fs = sampleRate; rng.seed (0xA4B5C6u); stopAll(); }

    void setParams (const ArpSeqParams& p) noexcept
    {
        const bool wasRunning = running();
        const int oldMode = params.mode;
        params = p;
        if (oldMode != p.mode) recFirst = true;
        if (! p.hold && latched) unlatch();
        if (wasRunning != running() || (oldMode != p.mode && running()))
        {
            stopRequested = true;           // release the sounding note, restart cleanly
            phase = 1.0;                    // first step on the next tick
            stepCounter = 0; pingDir = 1;
            lastStepIndex = -1;
        }
    }

    // Host transport at the start of the next process() call.
    void setTransport (bool playing, double ppq, double bpm) noexcept { hostPlaying = playing; hostPpq = ppq; hostBpm = bpm > 0 ? bpm : 120.0; }

    bool running() const noexcept { return (onOffPatched ? onOffHigh : params.play) && (params.mode == 0 || params.mode == 1); }
    bool sequencerRunning() const noexcept { return running() && params.mode == 1; }

    // ---- rear-panel patch jacks (called by the engine once per host sample) --------------------
    void externalClockEdge() noexcept { if (clockPatched) ++pendingEdges; }
    void resetPattern() noexcept { stepCounter = 0; pingDir = 1; lastStepIndex = -1; }
    void setExternalControl (bool clockIn, bool onOffIn, bool onOffHighNow) noexcept
    {
        const bool wasRunning = running();
        clockPatched = clockIn; onOffPatched = onOffIn; onOffHigh = onOffHighNow;
        if (! clockIn) pendingEdges = 0;
        if (wasRunning != running()) { stopRequested = true; phase = 1.0; stepCounter = 0; pingDir = 1; lastStepIndex = -1; stepElapsed = 0.0; }
    }
    bool clockOutHigh() const noexcept { return clockOutPhase < 0.5; }
    bool capturesKeys() const noexcept { return running(); }   // keys go to ARP/SEQ instead of the voice

    // ---- keyboard ---------------------------------------------------------------------------
    void keyOn (int note)
    {
        physicalAdd (note);
        if (params.mode == 0)        // ARP pattern
        {
            if (params.hold && physicalCount == 1 && latched) { patternCount = 0; }   // new pattern
            patternAdd (note);
            latched = params.hold;
        }
        else if (params.mode == 1)   // SEQ: transpose
        {
            const auto& s = seqs[(size_t) seqIndex()];
            transpose = s.length > 0 ? note - firstNote (s) : 0;
        }
        else if (params.mode == 2)   // REC
        {
            auto& s = seqs[(size_t) seqIndex()];
            if (recFirst) { s.length = 0; recFirst = false; }
            if (physicalCount > 1 && s.length > 0) s.steps[(size_t) s.length - 1].tie = true;   // legato entry
            appendStep (s, { (uint8_t) clampNote (note), false, false, pendingAccent });
            pendingAccent = false;
        }
    }

    void keyOff (int note)
    {
        physicalRemove (note);
        if (params.mode == 0 && ! params.hold) patternRemove (note);
    }

    // REC buttons (called on the audio thread).
    void recRest()  { if (params.mode != 2) return; auto& s = seqs[(size_t) seqIndex()]; if (recFirst) { s.length = 0; recFirst = false; } appendStep (s, { 60, true, false, false }); }
    void recTie()
    {
        if (params.mode != 2) return;
        auto& s = seqs[(size_t) seqIndex()];
        if (recFirst || s.length == 0) return;
        auto last = s.steps[(size_t) s.length - 1];
        if (last.rest) return;
        s.steps[(size_t) s.length - 1].tie = true;
        last.tie = false; last.accent = false;
        appendStep (s, last);
    }
    void recAccent() { if (params.mode == 2) pendingAccent = true; }

    // ---- clock ------------------------------------------------------------------------------
    // Advance one host-rate sample; writes up to 3 events. Returns the event count.
    int tick (Event* out) noexcept
    {
        int n = 0;
        if (stopRequested)
        {
            stopRequested = false;
            if (sounding >= 0) { out[n++] = { 2, sounding }; sounding = -1; }
            gateOpen = false;
        }
        {   // CLOCK OUT: PPQN pulses per quarter note at the current tempo (50 % duty), always running.
            const double bpm = params.sync ? hostBpm : (double) freeBpm (params.rate);
            clockOutPhase += bpm / 60.0 / fs * (double) std::max (params.clockOutPpqn, 1);
            clockOutPhase -= std::floor (clockOutPhase);
        }
        if (! running()) { pendingEdges = 0; advanceHost(); return n; }

        const double beatsPerSample = (params.sync ? hostBpm : 0.0) / 60.0 / fs;
        bool newStep = false;
        double fracInStep = 0.0;

        if (clockPatched)
        {
            // External clock: every edge is 1/PPQN of a beat (clock mode) or one step (step-advance mode).
            stepElapsed += 1.0;
            const double stepBeats = params.freeStep == 0 ? 1.0 : (params.freeStep == 1 ? 0.5 : 0.25);
            while (pendingEdges > 0)
            {
                --pendingEdges;
                if (params.extStepAdvance) newStep = true;
                else
                {
                    phase += (1.0 / (double) std::max (params.extPpqn, 1)) / stepBeats;
                    if (phase >= 1.0) { phase -= std::floor (phase); newStep = true; }
                }
            }
            if (newStep) { if (stepElapsed > 8.0) stepLenEstimate = stepElapsed; stepElapsed = 0.0; }
            fracInStep = stepLenEstimate > 0.0 ? stepElapsed / stepLenEstimate : 0.0;   // gate length from the measured step time
            lastStepIndex = -1;
        }
        else if (params.sync && hostPlaying)
        {
            const double stepBeats = divisionBeats (divisionIndex (params.rate));
            const double pos = hostPpq / stepBeats;
            const long long idx = (long long) std::floor (pos);
            fracInStep = pos - (double) idx;
            if (idx != lastStepIndex) { newStep = true; lastStepIndex = idx; }
        }
        else
        {
            const double stepBeats = params.sync ? divisionBeats (divisionIndex (params.rate))
                                                 : (params.freeStep == 0 ? 1.0 : (params.freeStep == 1 ? 0.5 : 0.25));
            const double bpm = params.sync ? hostBpm : freeBpm (params.rate);
            phase += bpm / 60.0 / fs / stepBeats;
            if (phase >= 1.0) { phase -= std::floor (phase); newStep = true; }
            fracInStep = phase;
            lastStepIndex = -1;
        }
        (void) beatsPerSample;
        advanceHost();

        if (newStep) n += startStep (out + n);
        else if (gateOpen && ! currentTie && fracInStep >= kGate)
        {
            if (sounding >= 0) out[n++] = { 2, sounding };
            sounding = -1;
            gateOpen = false;
        }
        return n;
    }

    // ---- sequence storage (guarded by the owner) -------------------------------------------
    std::array<Sequence, 3>& sequences() noexcept { return seqs; }
    int currentLength() const noexcept { return seqs[(size_t) seqIndex()].length; }
    bool lastStepAccented() const noexcept { return lastAccent; }

    void stopAll() noexcept { stopRequested = true; physicalCount = 0; patternCount = 0; latched = false; }

private:
    static constexpr double kGate = 0.5;
    float fs = 48000.0f;
    ArpSeqParams params;
    std::array<Sequence, 3> seqs {};
    Rng rng;

    std::array<int, 32> physical {}; int physicalCount = 0;
    std::array<int, 32> pattern {};  int patternCount = 0;
    bool latched = false, recFirst = true, pendingAccent = false;

    double phase = 1.0;
    long long lastStepIndex = -1;
    bool hostPlaying = false; double hostPpq = 0.0, hostBpm = 120.0;
    int stepCounter = 0, pingDir = 1, transpose = 0;
    bool clockPatched = false, onOffPatched = false, onOffHigh = false;
    int pendingEdges = 0;
    double stepElapsed = 0.0, stepLenEstimate = 12000.0, clockOutPhase = 0.0;
    int sounding = -1; bool gateOpen = false, currentTie = false, stopRequested = false, lastAccent = false;

    int seqIndex() const noexcept { return std::min (std::max (params.octSeq, 0), 2); }
    static int clampNote (int n) noexcept { return std::min (std::max (n, 0), 127); }
    static int firstNote (const Sequence& s) noexcept
    {
        for (int i = 0; i < s.length; ++i) if (! s.steps[(size_t) i].rest) return s.steps[(size_t) i].note;
        return 60;
    }
    static void appendStep (Sequence& s, SeqStep st) noexcept { if (s.length < (int) s.steps.size()) s.steps[(size_t) s.length++] = st; }

    void advanceHost() noexcept { if (hostPlaying) hostPpq += hostBpm / 60.0 / fs; }

    void physicalAdd (int note) { physicalRemove (note); if (physicalCount < 32) physical[(size_t) physicalCount++] = note; }
    void physicalRemove (int note)
    {
        for (int i = 0; i < physicalCount; ++i)
            if (physical[(size_t) i] == note) { for (int j = i; j < physicalCount - 1; ++j) physical[(size_t) j] = physical[(size_t) j + 1]; --physicalCount; return; }
    }
    void patternAdd (int note)
    {
        for (int i = 0; i < patternCount; ++i) if (pattern[(size_t) i] == note) return;
        if (patternCount < 32) pattern[(size_t) patternCount++] = note;
    }
    void patternRemove (int note)
    {
        for (int i = 0; i < patternCount; ++i)
            if (pattern[(size_t) i] == note) { for (int j = i; j < patternCount - 1; ++j) pattern[(size_t) j] = pattern[(size_t) j + 1]; --patternCount; return; }
    }
    void unlatch()
    {
        latched = false;
        patternCount = 0;
        for (int i = 0; i < physicalCount; ++i) patternAdd (physical[(size_t) i]);
    }

    // Index in [0, len) for the current direction; advances the counter.
    int nextIndex (int len) noexcept
    {
        if (len <= 0) return -1;
        if (params.direction == 2) return (int) (rng.unipolar() * (float) len) % len;
        if (params.direction == 1 && len > 1)
        {
            const int cycle = 2 * len - 2;
            const int c = stepCounter++ % cycle;
            return c < len ? c : cycle - c;
        }
        return stepCounter++ % len;
    }

    int startStep (Event* out) noexcept
    {
        int n = 0;
        int note = -1; bool tie = false, rest = false, accent = false;

        if (params.mode == 0)
        {
            const int octaves = std::min (std::max (params.octSeq, 0), 2) + 1;
            const int len = patternCount * octaves;
            const int idx = nextIndex (len);
            if (idx < 0) rest = true;
            else note = pattern[(size_t) (idx % patternCount)] + 12 * (idx / patternCount);
        }
        else
        {
            const auto& s = seqs[(size_t) seqIndex()];
            const int idx = nextIndex (s.length);
            if (idx < 0) rest = true;
            else
            {
                const auto& st = s.steps[(size_t) idx];
                rest = st.rest; tie = st.tie; accent = st.accent;
                note = st.note + transpose;
            }
        }

        const bool wasTied = currentTie && sounding >= 0;
        if (rest || note < 0)
        {
            if (sounding >= 0) { out[n++] = { 2, sounding }; sounding = -1; }
            gateOpen = false; currentTie = false;
            return n;
        }
        note = clampNote (note);
        if (wasTied && sounding == note) { currentTie = tie; gateOpen = true; lastAccent = accent; return n; }

        out[n++] = { 1, note };                                     // new note first → legato when tied
        if (sounding >= 0 && sounding != note) out[n++] = { 2, sounding };
        sounding = note; gateOpen = true; currentTie = tie; lastAccent = accent;
        return n;
    }
};
} // namespace babushka::dsp
