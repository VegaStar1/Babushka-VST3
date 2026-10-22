// Babushka — factory presets (245).
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
                                           "reverbDecay", "noiseFloor" };
    for (auto* g : kGlobalParams) if (std::string_view (g) == id) return true;
    return false;
}

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        // ---------------------------------------------------------------- Basic
        { "Basic", "Init", "All knobs at their defaults, no cables.",
          {  } },

        { "Basic", "Two Saws", "The starting point of most classic sounds: two saws a few cents apart, filter half open, a touch of envelope.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.684f }, { "resonance", 0.15f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.8f }, { "release", 0.567f }, { "volume", 0.729f } } },

        { "Basic", "Sine Oscillator", "The filter self-oscillates and is played from the keyboard (Kbd Track 1:1): a pure sine. Oscillators are off.",
          { { "mixOsc1", 0.0f }, { "cutoff", 0.466f }, { "resonance", 0.98f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.588f }, { "volume", 1.0f } } },

        { "Basic", "Mixer Overdrive", "Both oscillators and noise pushed hard into the mixer: the summing amp saturates before the ladder filter, as on the hardware.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.95f }, { "mixNoise", 0.25f }, { "cutoff", 0.643f }, { "resonance", 0.3f }, { "envAmt", 0.25f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.7f }, { "release", 0.481f }, { "volume", 0.647f } } },

        { "Basic", "Ladder Sweep", "Hold a note and sweep CUTOFF and RESONANCE: a single saw through the 4-pole ladder, envelope off, ready for knob riding.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.7f }, { "cutoff", 0.527f }, { "resonance", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "volume", 0.918f } } },

        { "Basic", "Spring Tank", "Narrow pulse pluck into the spring reverb at 60 % wet: the drip and boing of the tank.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "mixOsc1", 0.7f }, { "cutoff", 0.559f }, { "resonance", 0.2f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.0f }, { "release", 0.606f }, { "reverbMix", 0.6f }, { "volume", 1.0f } } },

        { "Basic", "Two Saws Chorus", "Two Saws through the BBD chorus in mode II: the wide, moving shimmer of the classic polysynth chorus on a mono voice.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.684f }, { "resonance", 0.15f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.8f }, { "release", 0.567f }, { "volume", 0.729f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.519f }, { "choDepth", 0.75f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f } } },

        { "Basic", "Tube Saws", "Two Saws pushed through the triode pedal and a short tape slap: thick, singing, a little rough at the edges.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.684f }, { "resonance", 0.15f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.8f }, { "release", 0.567f }, { "volume", 0.858f }, { "satOn", 1 }, { "satDrive", 0.85f }, { "satBias", 0.55f }, { "satBody", 0.3f }, { "satTone", 0.62f }, { "satOutput", 0.45f }, { "satMode", 0 }, { "satMix", 1.0f }, { "tapOn", 1 }, { "tapMix", 0.3f }, { "tapHeads", 0 }, { "tapSync", 0 }, { "tapTime", 0.3736f }, { "tapFeedback", 0.12f }, { "tapWow", 0.15f }, { "tapFlutter", 0.2f }, { "tapAge", 0.3f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } } },

        { "Basic", "Phased Ladder", "Ladder Sweep into the slow four-stage phaser: the notches drift against the resonance while you sweep.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.7f }, { "cutoff", 0.527f }, { "resonance", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "volume", 0.88f }, { "phaOn", 1 }, { "phaRate", 0.12f }, { "phaDepth", 0.9f }, { "phaFeedback", 0.1f }, { "phaStereo", 0.2f }, { "phaMode", 0 }, { "phaCenter", 0.5f }, { "phaMix", 0.5f } } },

        { "Basic", "Echo Tank", "The spring pluck through all three tape heads: a rhythmic train of repeats washing into the tank.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "mixOsc1", 0.7f }, { "cutoff", 0.559f }, { "resonance", 0.2f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.0f }, { "release", 0.606f }, { "reverbMix", 0.6f }, { "volume", 1.0f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.35f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } } },

        // ---------------------------------------------------------------- Legends
        { "Legends", "Munich '77", "In the style of Giorgio Moroder's bass on Donna Summer's 'I Feel Love' (1977, Moog modular): 16' saws a hair apart, a tight envelope; the double-note pulse that a tape delay gave the original is written into the sequence. Plays by itself; a key transposes; OCT/SEQ picks the chord.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.534f }, { "resonance", 0.25f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.0f }, { "release", 0.417f }, { "reverbMix", 0.15f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.697f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "36 36 48 48 43 43 46 46 36 36 48 48 43 43 46 46",
          "41 41 53 53 48 48 51 51 41 41 53 53 48 48 51 51",
          "43 43 55 55 50 50 53 53 43 43 55 55 50 50 53 53" },

        { "Legends", "Midnight Chase", "In the style of Moroder's 'Chase' (Midnight Express, 1978): relentless 16th octave bass with the filter breathing on a slow LFO. Plays by itself.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.4f }, { "cutoff", 0.486f }, { "resonance", 0.4f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.459f }, { "sustain", 0.0f }, { "release", 0.399f }, { "reverbMix", 0.2f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.2f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.721f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "38 50 38 50 38 50 38 50 36 48 36 48 41 53 41 53",
          "38 50 38 50 38 50 38 50",
          "38 38 50 38 38 50 38 50" },

        { "Legends", "Dark Pursuit", "In the style of Gesaffelstein's dark techno basses ('Pursuit', 2013; his rigs feature Roland SH-101 / MC-202 class monosynths): two saws slammed into the mixer, low resonant filter, short hard envelope, a little glide. Play low, staccato.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", -0.15f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.2f }, { "cutoff", 0.377f }, { "resonance", 0.45f }, { "envAmt", 0.42f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.514f }, { "sustain", 0.25f }, { "release", 0.399f }, { "glide", 0.346f }, { "legatoGlide", 1 }, { "volume", 0.98f } } },

        { "Legends", "Black Glory Lead", "In the style of Gesaffelstein's cold distorted leads: hard-synced oscillator 2 through an overdriven mixer, filter opened by the envelope; bend for menace.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 4.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.6f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.1f }, { "cutoff", 0.626f }, { "resonance", 0.35f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.6f }, { "release", 0.507f }, { "reverbMix", 0.2f }, { "glide", 0.294f }, { "volume", 0.877f } } },

        { "Legends", "Upside Down Arp", "In the style of Kyle Dixon & Michael Stein's 'Stranger Things' main title (2016; Prophet-5 and Roland SH-2 on the original): a C major 7 arpeggio at 84 BPM whose filter slowly opens, long spring. Plays by itself; a key transposes.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "cutoff", 0.559f }, { "resonance", 0.35f }, { "envAmt", 0.3f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.25f }, { "release", 0.541f }, { "reverbMix", 0.45f }, { "lfoRate", 0.026f }, { "lfoCutoffAmt", 0.28f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.544f }, { "arpFreeStep", 2 }, { "volume", 0.928f } },
          "",
          "48 52 55 59 60 59 55 52",
          "45 48 52 55 57 55 52 48",
          "41 45 48 52 53 52 48 45" },

        { "Legends", "Hawkins Pulse", "In the style of the pulsing low synth beds of 80s-revival scores ('Stranger Things', 'It Follows'): 8th-note octave bass that slowly opens. Plays by itself.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.413f }, { "resonance", 0.3f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.1f }, { "release", 0.459f }, { "reverbMix", 0.3f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.32f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.594f }, { "arpFreeStep", 1 }, { "volume", 1.0f } },
          "",
          "33 45 33 45 33 45 33 45 29 41 29 41 31 43 31 43",
          "33 33 45 33 33 45 33 45",
          "33 R 33 R 33 R 36 R" },

        { "Legends", "Lucky Lead", "In the style of Keith Emerson's portamento solo on ELP's 'Lucky Man' (1970, played on his Moog modular): near-unison squares, a resonant filter, a long logarithmic glide; walk slowly up the keys for the famous climbing sweep.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.742f }, { "resonance", 0.45f }, { "kbdTrack", 2 }, { "attack", 0.325f }, { "decay", 0.703f }, { "sustain", 0.9f }, { "release", 0.648f }, { "reverbMix", 0.25f }, { "glide", 0.64f }, { "glideType", 2 }, { "lfoRate", 0.444f }, { "lfoPitchAmt", 0.112f }, { "modWheel", 0.2f }, { "volume", 0.534f } } },

        { "Legends", "Funky Worm", "In the style of the Ohio Players' 'Funky Worm' (1972): a thin, high, whining lead with portamento and quick vibrato. Play an octave or two above middle C.",
          { { "osc1Octave", 3 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 0.0f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.2f }, { "cutoff", 0.706f }, { "resonance", 0.55f }, { "kbdTrack", 2 }, { "attack", 0.325f }, { "decay", 0.648f }, { "sustain", 0.9f }, { "release", 0.541f }, { "reverbMix", 0.15f }, { "glide", 0.523f }, { "lfoRate", 0.461f }, { "lfoPitchAmt", 0.129f }, { "modWheel", 0.5f }, { "volume", 0.828f } } },

        { "Legends", "G-Funk Whine", "In the style of the West Coast G-funk lead of Dr. Dre-era records (1992-93): soft sine-like tone, slow portamento, vibrato. Play high, legato.",
          { { "osc1Octave", 3 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.3f }, { "cutoff", 0.643f }, { "resonance", 0.2f }, { "kbdTrack", 2 }, { "attack", 0.369f }, { "decay", 0.67f }, { "sustain", 0.95f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "glide", 0.56f }, { "glideType", 2 }, { "lfoRate", 0.449f }, { "lfoPitchAmt", 0.112f }, { "modWheel", 0.6f }, { "volume", 0.566f } } },

        { "Legends", "Flash Light Bass", "In the style of Bernie Worrell's bass on Parliament's 'Flash Light' (1977): fat stacked oscillators, punchy envelope, bend down into the notes with the wheel.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.75f }, { "mixOsc2", 0.7f }, { "cutoff", 0.405f }, { "resonance", 0.4f }, { "envAmt", 0.48f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.35f }, { "release", 0.433f }, { "glide", 0.383f }, { "volume", 0.805f } } },

        { "Legends", "Chameleon Bass", "In the style of Herbie Hancock's 'Chameleon' (1973): a rubbery saw bass with a resonant 'wah' envelope on each note. Play the riff legato.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.55f }, { "cutoff", 0.355f }, { "resonance", 0.62f }, { "envAmt", 0.52f }, { "kbdTrack", 1 }, { "attack", 0.175f }, { "decay", 0.588f }, { "sustain", 0.3f }, { "release", 0.433f }, { "volume", 1.0f } } },

        { "Legends", "French Touch Lead", "In the style of the distorted lead of Daft Punk's 'Da Funk' (1995), described as a band-passed synth through a distortion pedal: here the high-pass sits before the ladder (cables: MIXER OUT -> HIGH PASS IN -> FILTER IN) and the mixer is driven hard.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.8f }, { "mixNoise", 0.05f }, { "cutoff", 0.671f }, { "resonance", 0.5f }, { "envAmt", 0.25f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.6f }, { "release", 0.459f }, { "glide", 0.294f }, { "legatoGlide", 1 }, { "hpCutoff", 0.496f }, { "volume", 1.0f } },
          "mixerOut:hpfIn hpfOut:filterIn" },

        { "Legends", "Grid Runner", "In the style of Daft Punk's 'Tron: Legacy' score ('Derezzed', 2010): driving square/saw 16ths with a bright snappy envelope. Plays by itself; a key transposes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.559f }, { "resonance", 0.35f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.459f }, { "sustain", 0.0f }, { "release", 0.378f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.737f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "36 48 36 46 36 48 36 43 36 48 36 46 39 41 43 46",
          "36 36 48 36 36 48 36 48",
          "36 48 43 48 41 48 39 48" },

        { "Legends", "On The Run", "In the style of the hypnotic 8-step EMS sequence on Pink Floyd's 'On the Run' (1973): fast 16ths, filter swept by the LFO, a hiss of noise. Plays by itself.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "mixNoise", 0.12f }, { "cutoff", 0.607f }, { "resonance", 0.55f }, { "envAmt", 0.25f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.459f }, { "sustain", 0.1f }, { "release", 0.399f }, { "reverbMix", 0.2f }, { "lfoRate", 0.164f }, { "lfoCutoffAmt", 0.32f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.8f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "52 55 57 55 62 60 62 64",
          "52 57 55 62 57 64 60 62",
          "52 52 55 57 52 52 59 57" },

        { "Legends", "Oxygen Lead", "In the style of Jean-Michel Jarre's 'Oxygène' leads (1976): a hollow square with slow vibrato and a soft attack, swimming in the spring.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "cutoff", 0.742f }, { "resonance", 0.25f }, { "kbdTrack", 2 }, { "attack", 0.401f }, { "decay", 0.703f }, { "sustain", 0.85f }, { "release", 0.67f }, { "reverbMix", 0.35f }, { "glide", 0.346f }, { "lfoRate", 0.424f }, { "lfoPitchAmt", 0.102f }, { "lfoPwAmt", 0.25f }, { "modWheel", 0.5f }, { "volume", 0.544f } } },

        { "Legends", "Switched-On", "In the style of Wendy Carlos' 'Switched-On Bach' (1968): bright, precise pulse with a crisp attack and full key tracking, for counterpoint.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "osc2Freq", 0.02f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.3f }, { "cutoff", 0.643f }, { "resonance", 0.3f }, { "envAmt", 0.35f }, { "kbdTrack", 2 }, { "attack", 0.119f }, { "decay", 0.567f }, { "sustain", 0.6f }, { "release", 0.459f }, { "reverbMix", 0.15f }, { "volume", 0.985f } } },

        { "Legends", "Nightdrive", "In the style of Kavinsky's 'Nightcall' (2010) and the 'Drive' soundtrack: dark detuned octave bass pumping in 8ths. Plays by itself; a key transposes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.15f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.5f }, { "cutoff", 0.465f }, { "resonance", 0.3f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.175f }, { "decay", 0.567f }, { "sustain", 0.3f }, { "release", 0.459f }, { "reverbMix", 0.3f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.574f }, { "arpFreeStep", 1 }, { "volume", 0.964f } },
          "",
          "34 46 34 46 34 46 34 46 30 42 30 42 32 44 32 44",
          "34 46 34 46 37 49 37 49",
          "34 34 46 34 34 46 34 46" },

        { "Legends", "BFG Bass", "In the spirit of Mick Gordon's 'DOOM' (2016) synth bass, which he built by forcing simple waves through chains of distortion: here overdriven saws and noise, sub-heavy, with a hard-edged envelope. Play low and fast.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", -0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.45f }, { "cutoff", 0.441f }, { "resonance", 0.5f }, { "envAmt", 0.45f }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.45f }, { "release", 0.433f }, { "volume", 0.985f } } },

        { "Legends", "Carpenter Pulse", "In the style of John Carpenter's self-scored thrillers ('Assault on Precinct 13', 'Escape from New York'): an insistent minor pulse with a slowly opening filter. Plays by itself.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.428f }, { "resonance", 0.4f }, { "envAmt", 0.3f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.1f }, { "release", 0.459f }, { "reverbMix", 0.35f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.24f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.653f }, { "arpFreeStep", 1 }, { "volume", 1.0f } },
          "",
          "37 37 37 37 37 37 37 37 35 35 35 35 33 33 35 35",
          "37 49 37 49 37 49 37 49",
          "37 R 37 37 R 37 R 37" },

        { "Legends", "Blade Brass", "In the style of Vangelis' Yamaha CS-80 brass in 'Blade Runner' (1982), done mono: saws with a slow filter swell, wheel vibrato and a big spring tail.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.14f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.465f }, { "resonance", 0.35f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.544f }, { "decay", 0.751f }, { "sustain", 0.65f }, { "release", 0.751f }, { "reverbMix", 0.35f }, { "glide", 0.383f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.087f }, { "modWheel", 0.3f }, { "volume", 0.662f } } },

        { "Legends", "Sandworm Drone", "In the style of the huge low drones of Hans Zimmer's 'Dune' (2021): sub saw and pulse in DRONE mode, a throaty resonant vowel moving very slowly, noise breath.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 3 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.5f }, { "mixNoise", 0.15f }, { "cutoff", 0.396f }, { "resonance", 0.6f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.22f }, { "modWheel", 1.0f }, { "volume", 0.63f } } },

        { "Legends", "Braam", "In the style of the trailer 'BRAAM' made famous by 'Inception' (2010): a low overdriven brass blast that swells open. Hold low notes.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.95f }, { "mixNoise", 0.15f }, { "cutoff", 0.314f }, { "resonance", 0.3f }, { "envAmt", 0.6f }, { "attack", 0.476f }, { "decay", 0.785f }, { "sustain", 0.7f }, { "release", 0.73f }, { "reverbMix", 0.35f }, { "volume", 0.622f } } },

        { "Legends", "Thesis On Sound", "A nod to Adrian Younge, who featured the Grandmother in Moog's 'Thesis on Sound' film: a warm, slightly gritty soul lead with slow vibrato and a wet spring, for retro cinematic themes.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 0 }, { "osc2Freq", 0.09f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.5f }, { "cutoff", 0.658f }, { "resonance", 0.3f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.325f }, { "decay", 0.688f }, { "sustain", 0.8f }, { "release", 0.648f }, { "reverbMix", 0.35f }, { "glide", 0.383f }, { "lfoRate", 0.43f }, { "lfoPitchAmt", 0.091f }, { "modWheel", 0.35f }, { "volume", 0.564f } } },

        { "Legends", "Chicago Acid", "In the style of Phuture's 'Acid Tracks' (1987) and early Chicago acid: resonant saw sequence with accents and slides. Plays by itself; turn CUTOFF and RESONANCE.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.7f }, { "cutoff", 0.396f }, { "resonance", 0.8f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.05f }, { "release", 0.399f }, { "glide", 0.411f }, { "legatoGlide", 1 }, { "attenuator", 0.35f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.679f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "kbVelOut:attIn attOut:filterCutoffIn",
          "36 36A 48 36 R 36 39T 41 36 48A 36 R 46 36 43A 36",
          "36 R 36 36A 48 R 36 R 34 R 34A 46 R 39 41 R",
          "36 48 36 48A 36 48 39 41" },

        { "Legends", "Tape Memory", "In the style of Boards of Canada's hazy, detuned leads: soft triangle and pulse, slow wandering pitch like worn tape, darkened top end.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.18f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.643f }, { "resonance", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.425f }, { "decay", 0.703f }, { "sustain", 0.8f }, { "release", 0.688f }, { "reverbMix", 0.35f }, { "lfoRate", 0.219f }, { "lfoPitchAmt", 0.079f }, { "modWheel", 1.0f }, { "volume", 0.45f } } },

        { "Legends", "Tape Memory Echo", "Tape Memory with a tired BBD chorus and a stretched cassette echo: hazier, more worn, more wandering.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.18f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.643f }, { "resonance", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.425f }, { "decay", 0.703f }, { "sustain", 0.8f }, { "release", 0.688f }, { "reverbMix", 0.35f }, { "lfoRate", 0.219f }, { "lfoPitchAmt", 0.079f }, { "modWheel", 1.0f }, { "volume", 0.552f }, { "choOn", 1 }, { "choRate", 0.453f }, { "choDepth", 0.65f }, { "choWidth", 0.8f }, { "choNoise", 0.8f }, { "choAge", 0.85f }, { "choMode", 0 }, { "choMix", 0.5f }, { "tapOn", 1 }, { "tapHeads", 1 }, { "tapTime", 0.406f }, { "tapFeedback", 0.4f }, { "tapMix", 0.35f }, { "tapWow", 0.85f }, { "tapFlutter", 0.6f }, { "tapAge", 0.85f }, { "tapBass", 0.45f }, { "tapTreble", 0.35f }, { "tapSync", 0 } } },

        { "Legends", "Nightdrive Chorus", "Nightdrive opened up: a deep, wide chorus and a dotted-eighth tape echo locked to the tempo, for night-drive synthwave.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.15f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.5f }, { "cutoff", 0.465f }, { "resonance", 0.3f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.175f }, { "decay", 0.567f }, { "sustain", 0.3f }, { "release", 0.459f }, { "reverbMix", 0.3f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.574f }, { "arpFreeStep", 1 }, { "volume", 0.872f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.355f }, { "choDepth", 0.85f }, { "choMix", 0.6f }, { "choNoise", 0.15f }, { "choAge", 0.2f }, { "choWidth", 1.0f }, { "tapOn", 1 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.4f }, { "tapMix", 0.3f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapHeads", 0 }, { "tapWow", 0.15f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } },
          "",
          "34 46 34 46 34 46 34 46 30 42 30 42 32 44 32 44",
          "34 46 34 46 37 49 37 49",
          "34 34 46 34 34 46 34 46" },

        { "Legends", "Oxygen Phase", "Oxygen Lead through a slow four-stage phaser and the tape heads, the way late-70s leads swirled and echoed.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "cutoff", 0.742f }, { "resonance", 0.25f }, { "kbdTrack", 2 }, { "attack", 0.401f }, { "decay", 0.703f }, { "sustain", 0.85f }, { "release", 0.67f }, { "reverbMix", 0.35f }, { "glide", 0.346f }, { "lfoRate", 0.424f }, { "lfoPitchAmt", 0.102f }, { "lfoPwAmt", 0.25f }, { "modWheel", 0.5f }, { "volume", 0.7f }, { "phaOn", 1 }, { "phaRate", 0.15f }, { "phaStereo", 0.5f }, { "phaMode", 0 }, { "phaDepth", 0.8f }, { "phaFeedback", 0.2f }, { "phaCenter", 0.5f }, { "phaMix", 0.5f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.55f }, { "tapMix", 0.3f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } } },

        { "Legends", "Munich Echo", "Munich '77 with a tempo-locked tape echo answering every note: the pulsing bass gets its ghost.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.534f }, { "resonance", 0.25f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.0f }, { "release", 0.417f }, { "reverbMix", 0.15f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.697f }, { "arpFreeStep", 2 }, { "volume", 0.963f }, { "tapOn", 1 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.3f }, { "tapMix", 0.3f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapHeads", 0 }, { "tapWow", 0.15f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } },
          "",
          "36 36 48 48 43 43 46 46 36 36 48 48 43 43 46 46",
          "41 41 53 53 48 48 51 51 41 41 53 53 48 48 51 51",
          "43 43 55 55 50 50 53 53 43 43 55 55 50 50 53 53" },

        { "Legends", "Black Glory Drive", "Black Glory Lead hit harder: diode clipping on the synced lead and a short slap behind it.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 4.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.6f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.1f }, { "cutoff", 0.626f }, { "resonance", 0.35f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.6f }, { "release", 0.507f }, { "reverbMix", 0.2f }, { "glide", 0.294f }, { "volume", 0.784f }, { "satOn", 1 }, { "satMode", 3 }, { "satDrive", 0.75f }, { "satBias", 0.2f }, { "satBody", 0.35f }, { "satTone", 0.6f }, { "satMix", 0.85f }, { "satOutput", 0.45f }, { "tapOn", 1 }, { "tapMix", 0.3f }, { "tapHeads", 0 }, { "tapSync", 0 }, { "tapTime", 0.3736f }, { "tapFeedback", 0.12f }, { "tapWow", 0.15f }, { "tapFlutter", 0.2f }, { "tapAge", 0.3f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } } },

        { "Legends", "Lucky Lead Echo", "The portamento solo warmed by a triode stage and set in a tape echo, as if played through the stage rig.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.742f }, { "resonance", 0.45f }, { "kbdTrack", 2 }, { "attack", 0.325f }, { "decay", 0.703f }, { "sustain", 0.9f }, { "release", 0.648f }, { "reverbMix", 0.25f }, { "glide", 0.64f }, { "glideType", 2 }, { "lfoRate", 0.444f }, { "lfoPitchAmt", 0.112f }, { "modWheel", 0.2f }, { "volume", 0.439f }, { "satOn", 1 }, { "satDrive", 0.4f }, { "satMode", 0 }, { "satBias", 0.35f }, { "satBody", 0.4f }, { "satTone", 0.5f }, { "satMix", 1.0f }, { "satOutput", 0.5f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.3f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } } },

        // ---------------------------------------------------------------- Cinematic
        { "Cinematic", "Braam Titan", "The trailer BRAAM at full force: 32' and 16' saws a hair apart, mixer in overdrive, the filter tears open in 120 ms and stays wide. Hold low notes.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.18f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.2f }, { "cutoff", 0.342f }, { "resonance", 0.35f }, { "envAmt", 0.65f }, { "kbdTrack", 1 }, { "attack", 0.52f }, { "decay", 0.837f }, { "sustain", 0.75f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "volume", 0.612f } } },

        { "Cinematic", "Braam Distorted", "Gritty, torn BRAAM: square + saw slammed into the mixer with noise, resonant ladder, aggressive envelope.",
          { { "osc1Octave", 0 }, { "osc1Wave", 2 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", -0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.45f }, { "cutoff", 0.366f }, { "resonance", 0.55f }, { "envAmt", 0.6f }, { "attack", 0.445f }, { "decay", 0.799f }, { "sustain", 0.65f }, { "release", 0.751f }, { "reverbMix", 0.3f }, { "volume", 0.94f } } },

        { "Cinematic", "Braam Slow Swell", "A BRAAM that swells for two seconds before it peaks: for the build before the hit.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.15f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.95f }, { "mixNoise", 0.15f }, { "cutoff", 0.278f }, { "resonance", 0.4f }, { "envAmt", 0.75f }, { "kbdTrack", 1 }, { "attack", 0.825f }, { "decay", 0.859f }, { "sustain", 0.85f }, { "release", 0.811f }, { "reverbMix", 0.35f }, { "volume", 0.648f } } },

        { "Cinematic", "Braam Hit", "One short, huge BRAAM stab with the spring crashing behind it. Hit and let go.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.3f }, { "cutoff", 0.355f }, { "resonance", 0.4f }, { "envAmt", 0.65f }, { "attack", 0.425f }, { "decay", 0.717f }, { "sustain", 0.0f }, { "release", 0.751f }, { "reverbMix", 0.65f }, { "volume", 0.709f } } },

        { "Cinematic", "Fifth Braam", "Open fifths (oscillator 2 seven semitones up) for a heroic, hollow BRAAM.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 7.03f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.95f }, { "mixNoise", 0.15f }, { "cutoff", 0.329f }, { "resonance", 0.4f }, { "envAmt", 0.65f }, { "kbdTrack", 1 }, { "attack", 0.544f }, { "decay", 0.837f }, { "sustain", 0.8f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "volume", 0.868f } } },

        { "Cinematic", "Dissonant Braam", "Oscillator 2 one semitone up: a grinding, menacing cluster for horror and tension.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 1.0f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.2f }, { "cutoff", 0.329f }, { "resonance", 0.45f }, { "envAmt", 0.62f }, { "attack", 0.575f }, { "decay", 0.837f }, { "sustain", 0.8f }, { "release", 0.799f }, { "reverbMix", 0.35f }, { "volume", 0.864f } } },

        { "Cinematic", "Horn of Doom", "A massive low horn: slow attack, a resonant 'throat' in the filter, slight pitch sag from the LFO. Hold long notes at C1-C2.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 3 }, { "osc2Freq", 0.1f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.9f }, { "mixNoise", 0.2f }, { "cutoff", 0.314f }, { "resonance", 0.6f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.636f }, { "decay", 0.859f }, { "sustain", 0.8f }, { "release", 0.811f }, { "reverbMix", 0.35f }, { "lfoRate", 0.13f }, { "lfoPitchAmt", 0.065f }, { "modWheel", 1.0f }, { "volume", 0.649f } } },

        { "Cinematic", "War Horn", "Battle horn: square and saw an octave apart, strong attack bite and a held, overdriven body.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.85f }, { "mixNoise", 0.1f }, { "cutoff", 0.413f }, { "resonance", 0.35f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.476f }, { "decay", 0.73f }, { "sustain", 0.75f }, { "release", 0.703f }, { "reverbMix", 0.35f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.058f }, { "modWheel", 0.3f }, { "volume", 0.808f } } },

        { "Cinematic", "Titan Brass", "Epic brass section done mono: wide detuned saws, filter attack slower than the amp so the note blooms, vibrato on the wheel.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "cutoff", 0.428f }, { "resonance", 0.3f }, { "envAmt", 0.6f }, { "kbdTrack", 1 }, { "attack", 0.5f }, { "decay", 0.769f }, { "sustain", 0.7f }, { "release", 0.717f }, { "reverbMix", 0.35f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.071f }, { "modWheel", 0.2f }, { "volume", 0.551f } } },

        { "Cinematic", "Epic Brass Stab", "Short, punchy brass hit for action cues: big filter blip, quick fall, spring tail.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.12f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.9f }, { "mixNoise", 0.1f }, { "cutoff", 0.441f }, { "resonance", 0.3f }, { "envAmt", 0.6f }, { "attack", 0.325f }, { "decay", 0.606f }, { "sustain", 0.2f }, { "release", 0.622f }, { "reverbMix", 0.5f }, { "volume", 0.695f } } },

        { "Cinematic", "Dark Brass Swell", "Low brass crescendo that opens over a second and a half: hold under a theme.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 3 }, { "osc2Freq", 0.15f }, { "mixOsc1", 0.95f }, { "mixOsc2", 0.9f }, { "cutoff", 0.355f }, { "resonance", 0.4f }, { "envAmt", 0.7f }, { "kbdTrack", 1 }, { "attack", 0.794f }, { "decay", 0.837f }, { "sustain", 0.8f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "volume", 0.552f } } },

        { "Cinematic", "Trailer Rise", "Brass that keeps rising in pitch and brightness while held: the last seconds before the title card. Cables: + ENV OUT -> attenuator -> both PITCH IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.25f }, { "cutoff", 0.297f }, { "resonance", 0.45f }, { "envAmt", 0.8f }, { "attack", 0.925f }, { "decay", 0.811f }, { "sustain", 1.0f }, { "release", 0.703f }, { "reverbMix", 0.35f }, { "attenuator", 0.15f }, { "volume", 0.904f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "Cinematic", "Low String Ostinato", "Driving cinematic string-bass ostinato, 8ths at 120 BPM, accents pushing the filter. Plays by itself; a key transposes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.15f }, { "mixOsc1", 0.95f }, { "mixOsc2", 0.9f }, { "cutoff", 0.441f }, { "resonance", 0.3f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.25f }, { "decay", 0.567f }, { "sustain", 0.3f }, { "release", 0.481f }, { "reverbMix", 0.3f }, { "attenuator", 0.3f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.679f }, { "arpFreeStep", 2 }, { "volume", 0.832f } },
          "kbVelOut:attIn attOut:filterCutoffIn",
          "38A 38 38 38 38A 38 41 38 38A 38 38 38 36A 36 34 36",
          "38A 38 38A 38 41A 41 41A 41",
          "38 R 38 38 R 38 38 R" },

        { "Cinematic", "Chase Pulse", "Relentless 16th-note low pulse with the filter slowly breathing, for chase scenes. Plays by itself.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.95f }, { "mixOsc2", 0.7f }, { "cutoff", 0.413f }, { "resonance", 0.4f }, { "envAmt", 0.35f }, { "attack", 0.0f }, { "decay", 0.459f }, { "sustain", 0.1f }, { "release", 0.399f }, { "reverbMix", 0.25f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.3f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.715f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "36 36 36 36 36 36 36 36 34 34 34 34 31 31 33 33",
          "36 36 48 36 36 48 36 48",
          "36 R 36 36 R 36 36 R" },

        { "Cinematic", "Braam Iron Titan", "Braam Titan through a saturating output transformer, with a long tape wash behind it: heavier and wider.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.18f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.2f }, { "cutoff", 0.342f }, { "resonance", 0.35f }, { "envAmt", 0.65f }, { "kbdTrack", 1 }, { "attack", 0.52f }, { "decay", 0.837f }, { "sustain", 0.75f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "volume", 0.435f }, { "satOn", 1 }, { "satMode", 2 }, { "satDrive", 0.6f }, { "satBias", 0.1f }, { "satBody", 0.75f }, { "satTone", 0.4f }, { "satMix", 1.0f }, { "satOutput", 0.5f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.25f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f }, { "tapSync", 0 } } },

        { "Cinematic", "Horn of Doom Echo", "The doom horn glued by tape saturation and thrown into a dark dub echo: each repeat lower and murkier.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 3 }, { "osc2Freq", 0.1f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.9f }, { "mixNoise", 0.2f }, { "cutoff", 0.314f }, { "resonance", 0.6f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.636f }, { "decay", 0.859f }, { "sustain", 0.8f }, { "release", 0.811f }, { "reverbMix", 0.35f }, { "lfoRate", 0.13f }, { "lfoPitchAmt", 0.065f }, { "modWheel", 1.0f }, { "volume", 0.435f }, { "satOn", 1 }, { "satMode", 1 }, { "satDrive", 0.5f }, { "satBias", 0.0f }, { "satBody", 0.45f }, { "satTone", 0.42f }, { "satMix", 1.0f }, { "satOutput", 0.5f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.3f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f }, { "tapSync", 0 } } },

        { "Cinematic", "Titan Brass Ensemble", "Titan Brass spread by the three-line ensemble chorus: a whole section instead of one player.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "cutoff", 0.428f }, { "resonance", 0.3f }, { "envAmt", 0.6f }, { "kbdTrack", 1 }, { "attack", 0.5f }, { "decay", 0.769f }, { "sustain", 0.7f }, { "release", 0.717f }, { "reverbMix", 0.35f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.071f }, { "modWheel", 0.2f }, { "volume", 0.579f }, { "choOn", 1 }, { "choMode", 4 }, { "choRate", 0.234f }, { "choDepth", 0.7f }, { "choMix", 0.55f }, { "choNoise", 0.2f }, { "choAge", 0.25f }, { "choWidth", 1.0f } } },

        { "Cinematic", "Chase Pulse Echo", "Chase Pulse with dotted-eighth repeats and a slow phaser: the ostinato turns into a rolling, restless bed.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.95f }, { "mixOsc2", 0.7f }, { "cutoff", 0.413f }, { "resonance", 0.4f }, { "envAmt", 0.35f }, { "attack", 0.0f }, { "decay", 0.459f }, { "sustain", 0.1f }, { "release", 0.399f }, { "reverbMix", 0.25f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.3f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.715f }, { "arpFreeStep", 2 }, { "volume", 1.0f }, { "tapOn", 1 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.4f }, { "tapMix", 0.3f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapHeads", 0 }, { "tapWow", 0.15f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f }, { "phaOn", 1 }, { "phaMode", 3 }, { "phaRate", 0.08f }, { "phaDepth", 0.7f }, { "phaFeedback", 0.3f }, { "phaCenter", 0.4f }, { "phaStereo", 0.6f }, { "phaMix", 0.4f } },
          "",
          "36 36 36 36 36 36 36 36 34 34 34 34 31 31 33 33",
          "36 36 48 36 36 48 36 48",
          "36 R 36 36 R 36 36 R" },

        { "Cinematic", "Trailer Rise Phased", "Trailer Rise through hot tape and an eight-stage phaser: it rises, swirls and tears at the same time.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.2f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "mixNoise", 0.25f }, { "cutoff", 0.297f }, { "resonance", 0.45f }, { "envAmt", 0.8f }, { "attack", 0.925f }, { "decay", 0.811f }, { "sustain", 1.0f }, { "release", 0.703f }, { "reverbMix", 0.35f }, { "attenuator", 0.15f }, { "volume", 0.669f }, { "satOn", 1 }, { "satMode", 1 }, { "satDrive", 0.8f }, { "satBias", 0.1f }, { "satBody", 0.5f }, { "satTone", 0.38f }, { "satOutput", 0.45f }, { "satMix", 1.0f }, { "phaOn", 1 }, { "phaMode", 3 }, { "phaRate", 0.25f }, { "phaDepth", 0.85f }, { "phaFeedback", 0.5f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        // ---------------------------------------------------------------- Bass
        { "Bass", "Classic Bass", "Two saws at 16', closed filter opened by a short envelope. The default analog bass.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.387f }, { "resonance", 0.35f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.45f }, { "release", 0.481f }, { "volume", 1.0f } } },

        { "Bass", "Sub Square", "Square at 16' with a square an octave up, filter low and dark: club sub bass with body.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.3f }, { "cutoff", 0.465f }, { "resonance", 0.1f }, { "envAmt", 0.15f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.9f }, { "release", 0.459f }, { "volume", 0.585f } } },

        { "Bass", "Round Moog Bass", "The warm round bass of countless records: saw + square, resonance a little up, decay to a medium sustain.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", -0.06f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.55f }, { "cutoff", 0.413f }, { "resonance", 0.3f }, { "envAmt", 0.38f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.58f }, { "sustain", 0.55f }, { "release", 0.459f }, { "volume", 0.749f } } },

        { "Bass", "Pluck Bass", "Short and percussive; the filter envelope does all the work. Good for fast lines.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 3 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.342f }, { "resonance", 0.4f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.0f }, { "release", 0.481f }, { "volume", 1.0f } } },

        { "Bass", "Funk Rubber", "Bouncy resonant bass for slap-style lines; a little glide between legato notes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.396f }, { "resonance", 0.55f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.552f }, { "sustain", 0.25f }, { "release", 0.433f }, { "glide", 0.383f }, { "legatoGlide", 1 }, { "volume", 0.937f } } },

        { "Bass", "Overdriven Saw Bass", "Mixer driven into saturation, filter half open: raw and aggressive.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", -0.12f }, { "mixOsc1", 1.0f }, { "mixOsc2", 1.0f }, { "cutoff", 0.508f }, { "resonance", 0.25f }, { "envAmt", 0.35f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.6f }, { "release", 0.459f }, { "volume", 0.771f } } },

        { "Bass", "Reese", "Two saws detuned against each other for the slow phasing drum-and-bass growl. Hold long notes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.28f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.7f }, { "cutoff", 0.534f }, { "resonance", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 1.0f }, { "release", 0.541f }, { "volume", 0.643f } } },

        { "Bass", "Hollow Pulse Bass", "Narrow pulses an octave apart: nasal, hollow, cuts through a mix without much low end.",
          { { "osc1Octave", 1 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "cutoff", 0.585f }, { "resonance", 0.2f }, { "envAmt", 0.3f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.567f }, { "sustain", 0.5f }, { "release", 0.459f }, { "volume", 0.823f } } },

        { "Bass", "Wobble", "Cutoff wobbled by the square-ish sine LFO (mod wheel up). Change MOD RATE for the speed.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", -0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "mixNoise", 0.1f }, { "cutoff", 0.428f }, { "resonance", 0.55f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.9f }, { "release", 0.507f }, { "vcaMode", 1 }, { "lfoRate", 0.382f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.52f }, { "modWheel", 1.0f }, { "volume", 0.727f } } },

        { "Bass", "Acid Squelch", "One saw, high resonance and a hard envelope: play legato for the slides.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.7f }, { "cutoff", 0.413f }, { "resonance", 0.78f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.567f }, { "sustain", 0.1f }, { "release", 0.433f }, { "glide", 0.454f }, { "legatoGlide", 1 }, { "volume", 1.0f } } },

        { "Bass", "Deep Triangle", "Triangle at 32' and 16': almost a sine, clean low end for minimal styles.",
          { { "osc1Octave", 0 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 0 }, { "mixOsc1", 0.8f }, { "mixOsc2", 0.5f }, { "cutoff", 0.626f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.85f }, { "release", 0.459f }, { "volume", 0.555f } } },

        { "Bass", "Synced Growl", "Oscillator 2 hard-synced to oscillator 1; the mod LFO sweeps its pitch for a vocal growl.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 3.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.3f }, { "mixOsc2", 0.75f }, { "cutoff", 0.607f }, { "resonance", 0.3f }, { "envAmt", 0.2f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.8f }, { "release", 0.459f }, { "lfoRate", 0.318f }, { "lfoWave", 0 }, { "lfoPitchAmt", 0.289f }, { "modWheel", 0.0f }, { "volume", 0.852f } } },

        { "Bass", "Mono Synth Pop", "Bright, slightly buzzy 80s pop bass: pulse + saw, quick envelope, half key tracking.",
          { { "osc1Octave", 1 }, { "osc1Wave", 3 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.486f }, { "resonance", 0.25f }, { "envAmt", 0.42f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.35f }, { "release", 0.433f }, { "volume", 0.888f } } },

        { "Bass", "Fat Taurus", "Pedal-style low bass: two saws an octave apart at 32' and 16', slow filter envelope blooms on each note.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.75f }, { "mixOsc2", 0.7f }, { "cutoff", 0.314f }, { "resonance", 0.25f }, { "envAmt", 0.42f }, { "attack", 0.151f }, { "decay", 0.717f }, { "sustain", 0.55f }, { "release", 0.588f }, { "volume", 0.79f } } },

        { "Bass", "Noise Bass", "Saw bass with noise in the mixer and a resonant filter: dirty and gritty.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "mixNoise", 0.4f }, { "cutoff", 0.428f }, { "resonance", 0.45f }, { "envAmt", 0.45f }, { "attack", 0.0f }, { "decay", 0.567f }, { "sustain", 0.3f }, { "release", 0.459f }, { "volume", 0.965f } } },

        { "Bass", "Bouncy Octaves", "Oscillator 2 an octave up and a touch of glide: each new note scoops.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.4f }, { "cutoff", 0.46f }, { "resonance", 0.35f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.567f }, { "sustain", 0.4f }, { "release", 0.459f }, { "glide", 0.411f }, { "volume", 0.953f } } },

        { "Bass", "Soft Sub Pad Bass", "Slow attack and a dark filter: a swell for long notes under a pad.",
          { { "osc1Octave", 0 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.35f }, { "cutoff", 0.465f }, { "resonance", 0.15f }, { "attack", 0.52f }, { "decay", 0.73f }, { "sustain", 0.9f }, { "release", 0.67f }, { "reverbMix", 0.15f }, { "volume", 0.76f } } },

        { "Bass", "Resonant Thump", "Short resonant ping on top of a saw: the click of the attack defines the note.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.8f }, { "cutoff", 0.355f }, { "resonance", 0.62f }, { "envAmt", 0.6f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.499f }, { "sustain", 0.0f }, { "release", 0.433f }, { "volume", 1.0f } } },

        { "Bass", "Stab Bass", "House organ-ish stab: square + saw a fifth up, very short.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", -5.0f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.534f }, { "resonance", 0.3f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.499f }, { "sustain", 0.0f }, { "release", 0.459f }, { "volume", 0.992f } } },

        { "Bass", "Velocity Bass", "Play harder for a brighter note. Cable: KB VEL OUT -> attenuator -> CUTOFF IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.366f }, { "resonance", 0.35f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.4f }, { "release", 0.459f }, { "attenuator", 0.45f }, { "volume", 1.0f } },
          "kbVelOut:attIn attOut:filterCutoffIn" },

        { "Bass", "Tube Bass", "Classic Bass through the triode pedal: rounder, warmer, more 2nd harmonic, peaks that sag a little.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.387f }, { "resonance", 0.35f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.45f }, { "release", 0.481f }, { "volume", 0.678f }, { "satOn", 1 }, { "satMode", 0 }, { "satDrive", 0.55f }, { "satBias", 0.35f }, { "satBody", 0.4f }, { "satTone", 0.5f }, { "satMix", 1.0f }, { "satOutput", 0.5f } } },

        { "Bass", "Tape Sub", "Sub Square glued by tape saturation: the head bump fattens the lows, the top is softened.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.3f }, { "cutoff", 0.465f }, { "resonance", 0.1f }, { "envAmt", 0.15f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.9f }, { "release", 0.459f }, { "volume", 0.511f }, { "satOn", 1 }, { "satMode", 1 }, { "satDrive", 0.5f }, { "satBias", 0.0f }, { "satBody", 0.45f }, { "satTone", 0.42f }, { "satMix", 1.0f }, { "satOutput", 0.5f } } },

        { "Bass", "Iron Reese", "The Reese growl through a saturating transformer and a slow phaser: heavier lows, more movement up top.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.28f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.7f }, { "cutoff", 0.534f }, { "resonance", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 1.0f }, { "release", 0.541f }, { "volume", 0.577f }, { "satOn", 1 }, { "satMode", 2 }, { "satDrive", 0.6f }, { "satBias", 0.1f }, { "satBody", 0.75f }, { "satTone", 0.4f }, { "satMix", 1.0f }, { "satOutput", 0.5f }, { "phaOn", 1 }, { "phaMode", 3 }, { "phaRate", 0.08f }, { "phaDepth", 0.7f }, { "phaFeedback", 0.3f }, { "phaCenter", 0.4f }, { "phaStereo", 0.6f }, { "phaMix", 0.3f } } },

        { "Bass", "Acid Drive", "Acid Squelch into diode distortion and a dotted-eighth echo: the classic acid line, louder and nastier.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.7f }, { "cutoff", 0.413f }, { "resonance", 0.78f }, { "envAmt", 0.55f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.567f }, { "sustain", 0.1f }, { "release", 0.433f }, { "glide", 0.454f }, { "legatoGlide", 1 }, { "volume", 0.712f }, { "satOn", 1 }, { "satMode", 3 }, { "satDrive", 0.75f }, { "satBias", 0.2f }, { "satBody", 0.35f }, { "satTone", 0.6f }, { "satMix", 0.85f }, { "satOutput", 0.45f }, { "tapOn", 1 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.3f }, { "tapMix", 0.22f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapHeads", 0 }, { "tapWow", 0.15f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } } },

        { "Bass", "Phased Funk", "Funk Rubber through a little four-stage phaser: the swirl of a 70s funk bass rig.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.396f }, { "resonance", 0.55f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.552f }, { "sustain", 0.25f }, { "release", 0.433f }, { "glide", 0.383f }, { "legatoGlide", 1 }, { "volume", 0.937f }, { "phaOn", 1 }, { "phaRate", 0.2f }, { "phaDepth", 0.9f }, { "phaFeedback", 0.1f }, { "phaStereo", 0.2f }, { "phaMode", 0 }, { "phaCenter", 0.5f }, { "phaMix", 0.5f } } },

        { "Bass", "Dub Bass", "Round Moog Bass with a dark dub echo answering the line: play sparse notes and let the tape talk.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", -0.06f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.55f }, { "cutoff", 0.413f }, { "resonance", 0.3f }, { "envAmt", 0.38f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.58f }, { "sustain", 0.55f }, { "release", 0.459f }, { "volume", 0.749f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.22f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.4f }, { "tapTreble", 0.35f }, { "tapSync", 0 } } },

        // ---------------------------------------------------------------- Lead
        { "Lead", "Sync Lead", "Hard-synced oscillator 2 with glide; the mod wheel adds vibrato and OSC 2 FREQUENCY sweeps the sync tone.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 5.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.25f }, { "mixOsc2", 0.75f }, { "cutoff", 0.757f }, { "resonance", 0.25f }, { "envAmt", 0.2f }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.8f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "glide", 0.471f }, { "lfoRate", 0.444f }, { "lfoPitchAmt", 0.102f }, { "volume", 0.87f } } },

        { "Lead", "Fat Mono Lead", "Two detuned saws, filter fairly open, a little glide and spring: the all-round mono lead.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.742f }, { "resonance", 0.2f }, { "envAmt", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.85f }, { "release", 0.567f }, { "reverbMix", 0.18f }, { "glide", 0.411f }, { "lfoRate", 0.444f }, { "lfoPitchAmt", 0.091f }, { "volume", 0.671f } } },

        { "Lead", "Square Lead", "Two squares an octave apart: woody and hollow, for melodies in the middle register.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.715f }, { "resonance", 0.15f }, { "envAmt", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.8f }, { "release", 0.567f }, { "reverbMix", 0.15f }, { "glide", 0.383f }, { "lfoRate", 0.438f }, { "lfoPitchAmt", 0.087f }, { "volume", 0.703f } } },

        { "Lead", "Whistle", "Self-oscillating filter played from the keyboard, with vibrato and glide: a whistling sine lead.",
          { { "mixOsc1", 0.0f }, { "cutoff", 0.466f }, { "resonance", 0.95f }, { "kbdTrack", 2 }, { "attack", 0.369f }, { "decay", 0.648f }, { "sustain", 1.0f }, { "release", 0.588f }, { "reverbMix", 0.25f }, { "glide", 0.435f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.102f }, { "modWheel", 0.6f }, { "volume", 0.97f } } },

        { "Lead", "Singing Resonance", "Saw lead with high resonance that sings over the envelope sweep.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "cutoff", 0.585f }, { "resonance", 0.72f }, { "envAmt", 0.35f }, { "kbdTrack", 2 }, { "attack", 0.25f }, { "decay", 0.688f }, { "sustain", 0.5f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "glide", 0.411f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.091f }, { "volume", 1.0f } } },

        { "Lead", "Screaming Sync", "Sync lead with the filter envelope also sweeping: rock solo territory. Bend up with the pitch wheel.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 4.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.2f }, { "mixOsc2", 0.9f }, { "cutoff", 0.715f }, { "resonance", 0.35f }, { "envAmt", 0.35f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.7f }, { "release", 0.541f }, { "reverbMix", 0.15f }, { "lfoRate", 0.449f }, { "lfoPitchAmt", 0.112f }, { "modWheel", 0.2f }, { "volume", 0.897f } } },

        { "Lead", "Soft Flute", "Triangle with breath noise and a slow attack; vibrato from the wheel.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 0 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.4f }, { "mixNoise", 0.12f }, { "cutoff", 0.706f }, { "kbdTrack", 2 }, { "attack", 0.445f }, { "decay", 0.622f }, { "sustain", 0.9f }, { "release", 0.567f }, { "reverbMix", 0.3f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.091f }, { "modWheel", 0.5f }, { "volume", 0.707f } } },

        { "Lead", "Nasal Pulse Lead", "Narrow pulse with a bit of resonance: reedy, oboe-like.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.695f }, { "resonance", 0.3f }, { "envAmt", 0.15f }, { "kbdTrack", 2 }, { "attack", 0.325f }, { "decay", 0.648f }, { "sustain", 0.8f }, { "release", 0.541f }, { "reverbMix", 0.2f }, { "lfoRate", 0.444f }, { "lfoPitchAmt", 0.079f }, { "modWheel", 0.4f }, { "volume", 0.913f } } },

        { "Lead", "PWM Lead", "Pulse-width modulation by the LFO for a chorus-like, moving square.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.77f }, { "resonance", 0.1f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.85f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "glide", 0.346f }, { "lfoRate", 0.26f }, { "lfoWave", 0 }, { "lfoPwAmt", 0.7f }, { "modWheel", 1.0f }, { "volume", 0.567f } } },

        { "Lead", "Fifths Lead", "Oscillator 2 a fifth above: big, organ-like intervals for rock riffs.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 7.0f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.725f }, { "resonance", 0.2f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.8f }, { "release", 0.541f }, { "reverbMix", 0.15f }, { "glide", 0.346f }, { "volume", 0.769f } } },

        { "Lead", "Glide Solo", "Long exponential portamento between every note: the 70s solo.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.715f }, { "resonance", 0.3f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.8f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "glide", 0.617f }, { "glideType", 2 }, { "lfoRate", 0.442f }, { "lfoPitchAmt", 0.091f }, { "modWheel", 0.3f }, { "volume", 0.638f } } },

        { "Lead", "Brassy Lead", "Filter attack slower than the amp: the note blooms like a horn.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.559f }, { "resonance", 0.2f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.325f }, { "decay", 0.648f }, { "sustain", 0.7f }, { "release", 0.567f }, { "reverbMix", 0.2f }, { "lfoRate", 0.438f }, { "lfoPitchAmt", 0.091f }, { "volume", 0.58f } } },

        { "Lead", "Thin Sync Whine", "High sync whine with vibrato; sits on top of a busy track.",
          { { "osc1Octave", 3 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 6.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.1f }, { "mixOsc2", 0.8f }, { "cutoff", 0.814f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.8f }, { "release", 0.541f }, { "reverbMix", 0.25f }, { "glide", 0.383f }, { "lfoRate", 0.453f }, { "lfoPitchAmt", 0.112f }, { "modWheel", 0.4f }, { "volume", 0.845f } } },

        { "Lead", "Chiptune Square", "Square at 4' with a fast arpeggio feel from the square LFO on pitch (wheel up = an octave trill).",
          { { "osc1Octave", 3 }, { "osc1Wave", 2 }, { "mixOsc1", 0.6f }, { "cutoff", 0.913f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.9f }, { "release", 0.378f }, { "lfoRate", 0.539f }, { "lfoWave", 3 }, { "lfoPitchAmt", 0.707f }, { "modWheel", 0.0f }, { "volume", 0.526f } } },

        { "Lead", "Dirty Lead", "Overdriven mixer and resonant filter: distorted edge without any effect.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", -0.12f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.9f }, { "mixNoise", 0.15f }, { "cutoff", 0.671f }, { "resonance", 0.45f }, { "envAmt", 0.25f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.75f }, { "release", 0.541f }, { "glide", 0.346f }, { "volume", 0.742f } } },

        { "Lead", "Wah Lead", "The LFO opens and closes the filter like a wah pedal; the mod wheel sets the depth.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.559f }, { "resonance", 0.6f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.9f }, { "release", 0.567f }, { "reverbMix", 0.15f }, { "lfoRate", 0.389f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.44f }, { "modWheel", 0.7f }, { "volume", 0.841f } } },

        { "Lead", "Expressive Velocity Lead", "Play harder for a brighter note. Cable: KB VEL OUT -> attenuator -> CUTOFF IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "cutoff", 0.559f }, { "resonance", 0.3f }, { "envAmt", 0.25f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.7f }, { "release", 0.588f }, { "reverbMix", 0.15f }, { "attenuator", 0.35f }, { "volume", 0.999f } },
          "kbVelOut:attIn attOut:filterCutoffIn" },

        { "Lead", "Tube Sync Lead", "Screaming Sync through a hot triode preamp and a short tape slap: the rock solo amp sound.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 4.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.2f }, { "mixOsc2", 0.9f }, { "cutoff", 0.715f }, { "resonance", 0.35f }, { "envAmt", 0.35f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.7f }, { "release", 0.541f }, { "reverbMix", 0.15f }, { "lfoRate", 0.449f }, { "lfoPitchAmt", 0.112f }, { "modWheel", 0.2f }, { "volume", 0.897f }, { "satOn", 1 }, { "satDrive", 0.85f }, { "satBias", 0.55f }, { "satBody", 0.3f }, { "satTone", 0.62f }, { "satOutput", 0.45f }, { "satMode", 0 }, { "satMix", 1.0f }, { "tapOn", 1 }, { "tapMix", 0.35f }, { "tapHeads", 0 }, { "tapSync", 0 }, { "tapTime", 0.3736f }, { "tapFeedback", 0.12f }, { "tapWow", 0.15f }, { "tapFlutter", 0.2f }, { "tapAge", 0.3f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } } },

        { "Lead", "Chorus Mono Lead", "Fat Mono Lead widened by the BBD chorus and set in a light three-head tape echo.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.742f }, { "resonance", 0.2f }, { "envAmt", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.85f }, { "release", 0.567f }, { "reverbMix", 0.18f }, { "glide", 0.411f }, { "lfoRate", 0.444f }, { "lfoPitchAmt", 0.091f }, { "volume", 0.607f }, { "choOn", 1 }, { "choMode", 0 }, { "choRate", 0.4236f }, { "choDepth", 0.6f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.28f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } } },

        { "Lead", "Space Whistle", "The whistling filter sine in an endless tape cloud: sci-fi theremin territory.",
          { { "mixOsc1", 0.0f }, { "cutoff", 0.466f }, { "resonance", 0.95f }, { "kbdTrack", 2 }, { "attack", 0.369f }, { "decay", 0.648f }, { "sustain", 1.0f }, { "release", 0.588f }, { "reverbMix", 0.25f }, { "glide", 0.435f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.102f }, { "modWheel", 0.6f }, { "volume", 0.93f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.4f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f }, { "tapSync", 0 } } },

        { "Lead", "Phased Square", "Square Lead through the classic four-stage phaser: woody and hollow, now slowly turning.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.715f }, { "resonance", 0.15f }, { "envAmt", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.8f }, { "release", 0.567f }, { "reverbMix", 0.15f }, { "glide", 0.383f }, { "lfoRate", 0.438f }, { "lfoPitchAmt", 0.087f }, { "volume", 0.824f }, { "phaOn", 1 }, { "phaMode", 0 }, { "phaRate", 0.3f }, { "phaDepth", 0.8f }, { "phaFeedback", 0.2f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } } },

        { "Lead", "Dirty Echo Lead", "Dirty Lead through diode distortion into a tape echo on the edge of feedback: hold a note and let it build.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", -0.12f }, { "mixOsc1", 1.0f }, { "mixOsc2", 0.9f }, { "mixNoise", 0.15f }, { "cutoff", 0.671f }, { "resonance", 0.45f }, { "envAmt", 0.25f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.75f }, { "release", 0.541f }, { "glide", 0.346f }, { "volume", 0.63f }, { "satOn", 1 }, { "satMode", 3 }, { "satDrive", 0.75f }, { "satBias", 0.2f }, { "satBody", 0.35f }, { "satTone", 0.6f }, { "satMix", 0.85f }, { "satOutput", 0.45f }, { "tapOn", 1 }, { "tapHeads", 4 }, { "tapTime", 0.435f }, { "tapFeedback", 0.97f }, { "tapMix", 0.25f }, { "tapWow", 0.35f }, { "tapFlutter", 0.3f }, { "tapAge", 0.5f }, { "tapBass", 0.55f }, { "tapTreble", 0.45f }, { "tapSync", 0 } } },

        { "Lead", "Glide Solo Delay", "Glide Solo with chorus and a long tape echo: the portamento trails off in darkening repeats.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.715f }, { "resonance", 0.3f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.8f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "glide", 0.617f }, { "glideType", 2 }, { "lfoRate", 0.442f }, { "lfoPitchAmt", 0.091f }, { "modWheel", 0.3f }, { "volume", 0.638f }, { "choOn", 1 }, { "choMode", 0 }, { "choRate", 0.4236f }, { "choDepth", 0.6f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.28f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f }, { "tapSync", 0 } } },

        // ---------------------------------------------------------------- Keys & Plucks
        { "Keys & Plucks", "Spring Pluck", "Narrow pulse pluck into the spring reverb, 50 % wet.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "mixOsc1", 0.7f }, { "cutoff", 0.544f }, { "resonance", 0.2f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.0f }, { "release", 0.606f }, { "reverbMix", 0.5f }, { "volume", 1.0f } } },

        { "Keys & Plucks", "Analog Clav", "Funky clavinet-like pulse with a fast resonant envelope. Play staccato.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 2 }, { "osc2Wave", 3 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.3f }, { "cutoff", 0.559f }, { "resonance", 0.45f }, { "envAmt", 0.45f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.15f }, { "release", 0.378f }, { "volume", 1.0f } } },

        { "Keys & Plucks", "Mallet", "Triangle and square with a quick decay: marimba / vibraphone feel, with spring.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.2f }, { "cutoff", 0.684f }, { "resonance", 0.3f }, { "envAmt", 0.3f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.636f }, { "sustain", 0.0f }, { "release", 0.636f }, { "reverbMix", 0.35f }, { "volume", 1.0f } } },

        { "Keys & Plucks", "Harpsichord", "Bright saw and narrow pulse an octave apart with a plucked envelope.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 3 }, { "osc2Freq", 0.03f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.45f }, { "cutoff", 0.626f }, { "resonance", 0.25f }, { "envAmt", 0.55f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.0f }, { "release", 0.567f }, { "reverbMix", 0.2f }, { "volume", 1.0f } } },

        { "Keys & Plucks", "Kalimba", "Self-oscillating filter pinged by a very short envelope: thumb-piano tines.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "mixOsc1", 0.6f }, { "cutoff", 0.486f }, { "resonance", 0.86f }, { "envAmt", 0.35f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.0f }, { "release", 0.588f }, { "reverbMix", 0.35f }, { "volume", 1.0f } } },

        { "Keys & Plucks", "Plucked String", "Saw + pulse through a fast envelope with key tracking: a guitar-like pluck.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.4f }, { "cutoff", 0.508f }, { "resonance", 0.2f }, { "envAmt", 0.55f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.0f }, { "release", 0.648f }, { "reverbMix", 0.25f }, { "volume", 0.993f } } },

        { "Keys & Plucks", "Organ Drawbar", "Square + square an octave up, filter open, KB RLS VCA: an organ with no envelope at all.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "cutoff", 0.814f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.352f }, { "vcaMode", 1 }, { "reverbMix", 0.25f }, { "lfoRate", 0.456f }, { "lfoPitchAmt", 0.058f }, { "modWheel", 0.0f }, { "volume", 0.582f } } },

        { "Keys & Plucks", "Glass Bell FM", "Oscillator 1 frequency-modulates oscillator 2: inharmonic bell. Cable: OSC 1 WAVE OUT -> OSC 2 LIN FM IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 0 }, { "osc2Freq", 7.0f }, { "mixOsc1", 0.0f }, { "mixOsc2", 0.7f }, { "cutoff", 0.856f }, { "attack", 0.0f }, { "decay", 0.769f }, { "sustain", 0.0f }, { "release", 0.751f }, { "reverbMix", 0.4f }, { "volume", 0.665f } },
          "osc1WaveOut:osc2LinFmIn" },

        { "Keys & Plucks", "Dub Chord", "Minor-seventh-ish stab (osc 2 +3 semitones) drowned in spring reverb.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 3.0f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.5f }, { "cutoff", 0.572f }, { "resonance", 0.35f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.0f }, { "release", 0.588f }, { "reverbMix", 0.65f }, { "volume", 1.0f } } },

        { "Keys & Plucks", "Music Box", "High triangle with a short pling and a long reverb.",
          { { "osc1Octave", 3 }, { "osc1Wave", 0 }, { "osc2Octave", 3 }, { "osc2Wave", 0 }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.25f }, { "cutoff", 0.887f }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.0f }, { "release", 0.703f }, { "reverbMix", 0.45f }, { "volume", 0.686f } } },

        { "Keys & Plucks", "Phased Clav", "Analog Clav through a warm preamp and a four-stage phaser: the funk clavinet rig.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 2 }, { "osc2Wave", 3 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.3f }, { "cutoff", 0.559f }, { "resonance", 0.45f }, { "envAmt", 0.45f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.15f }, { "release", 0.378f }, { "volume", 0.834f }, { "satOn", 1 }, { "satDrive", 0.45f }, { "satMode", 0 }, { "satBias", 0.35f }, { "satBody", 0.4f }, { "satTone", 0.5f }, { "satMix", 1.0f }, { "satOutput", 0.5f }, { "phaOn", 1 }, { "phaRate", 0.38f }, { "phaMode", 0 }, { "phaDepth", 0.8f }, { "phaFeedback", 0.2f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } } },

        { "Keys & Plucks", "Dub Chord Echo", "Dub Chord thrown into the tape echo: heads 1 + 3, every repeat darker, the classic dub stab.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 3.0f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.5f }, { "cutoff", 0.572f }, { "resonance", 0.35f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.0f }, { "release", 0.588f }, { "reverbMix", 0.65f }, { "volume", 0.897f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.5f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f }, { "tapSync", 0 } } },

        { "Keys & Plucks", "Ensemble Harpsichord", "Harpsichord spread by the BBD chorus: a 70s string-machine take on the baroque pluck.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 3 }, { "osc2Freq", 0.03f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.45f }, { "cutoff", 0.626f }, { "resonance", 0.25f }, { "envAmt", 0.55f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.0f }, { "release", 0.567f }, { "reverbMix", 0.2f }, { "volume", 0.925f }, { "choOn", 1 }, { "choMode", 0 }, { "choRate", 0.4236f }, { "choDepth", 0.6f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f } } },

        { "Keys & Plucks", "Music Box Tape", "Music Box played back from an old cassette: wow, flutter, hiss and a worn chorus.",
          { { "osc1Octave", 3 }, { "osc1Wave", 0 }, { "osc2Octave", 3 }, { "osc2Wave", 0 }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.25f }, { "cutoff", 0.887f }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.0f }, { "release", 0.703f }, { "reverbMix", 0.45f }, { "volume", 0.655f }, { "choOn", 1 }, { "choRate", 0.453f }, { "choDepth", 0.65f }, { "choWidth", 0.8f }, { "choNoise", 0.8f }, { "choAge", 0.85f }, { "choMode", 0 }, { "choMix", 0.5f }, { "tapOn", 1 }, { "tapHeads", 1 }, { "tapTime", 0.406f }, { "tapFeedback", 0.4f }, { "tapWow", 0.85f }, { "tapFlutter", 0.6f }, { "tapAge", 0.85f }, { "tapBass", 0.45f }, { "tapTreble", 0.35f }, { "tapSync", 0 }, { "tapMix", 0.45f } } },

        { "Keys & Plucks", "Mallet Echoes", "Mallet through all three tape heads: marimba patterns grow out of single notes.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.2f }, { "cutoff", 0.684f }, { "resonance", 0.3f }, { "envAmt", 0.3f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.636f }, { "sustain", 0.0f }, { "release", 0.636f }, { "reverbMix", 0.35f }, { "volume", 1.0f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.35f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } } },

        // ---------------------------------------------------------------- Pads & Drones
        { "Pads & Drones", "Warm Pad", "Two squares with slow pulse-width modulation, long envelope and spring.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.544f }, { "resonance", 0.15f }, { "envAmt", 0.25f }, { "attack", 0.695f }, { "decay", 0.777f }, { "sustain", 0.75f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "lfoRate", 0.177f }, { "lfoPwAmt", 0.4f }, { "modWheel", 1.0f }, { "volume", 0.419f } } },

        { "Pads & Drones", "Slow Strings", "Detuned saws with a slow attack and a gentle vibrato: a mono string ensemble.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.15f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.706f }, { "resonance", 0.1f }, { "kbdTrack", 1 }, { "attack", 0.663f }, { "decay", 0.73f }, { "sustain", 0.9f }, { "release", 0.751f }, { "reverbMix", 0.35f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.071f }, { "modWheel", 0.5f }, { "volume", 0.503f } } },

        { "Pads & Drones", "Dark Drone", "DRONE VCA: plays without a key. Two saws, a fifth apart, very slow filter LFO. Use the keys to change the pitch.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 7.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.486f }, { "resonance", 0.4f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.014f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.32f }, { "modWheel", 1.0f }, { "volume", 0.906f } } },

        { "Pads & Drones", "Breathing Pad", "Slow attack and release; the filter LFO makes the pad breathe.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "mixNoise", 0.05f }, { "cutoff", 0.559f }, { "resonance", 0.3f }, { "attack", 0.75f }, { "decay", 0.811f }, { "sustain", 0.85f }, { "release", 0.811f }, { "reverbMix", 0.35f }, { "lfoRate", 0.096f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.24f }, { "modWheel", 1.0f }, { "volume", 0.817f } } },

        { "Pads & Drones", "Choir Vowel", "Pulse + noise through a resonant filter wobbled slowly: an \"aah\" vowel.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "mixNoise", 0.12f }, { "cutoff", 0.626f }, { "resonance", 0.6f }, { "kbdTrack", 1 }, { "attack", 0.636f }, { "decay", 0.73f }, { "sustain", 0.9f }, { "release", 0.73f }, { "reverbMix", 0.35f }, { "lfoRate", 0.148f }, { "lfoCutoffAmt", 0.12f }, { "modWheel", 1.0f }, { "volume", 0.683f } } },

        { "Pads & Drones", "Sub Drone", "Low triangle drone with a pulsing square LFO on the filter, very slow. DRONE VCA.",
          { { "osc1Octave", 0 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.03f }, { "mixOsc1", 0.8f }, { "mixOsc2", 0.3f }, { "cutoff", 0.413f }, { "resonance", 0.3f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.2f }, { "lfoRate", 0.13f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.16f }, { "modWheel", 1.0f }, { "volume", 0.813f } } },

        { "Pads & Drones", "Shimmer Swell", "High saws swelling into the spring with a slow vibrato: cinematic air.",
          { { "osc1Octave", 3 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.788f }, { "attack", 0.794f }, { "decay", 0.811f }, { "sustain", 0.9f }, { "release", 0.837f }, { "reverbMix", 0.35f }, { "lfoRate", 0.424f }, { "lfoPitchAmt", 0.065f }, { "modWheel", 0.6f }, { "volume", 0.409f } } },

        { "Pads & Drones", "Ocean Pad", "Noise swept by a slow filter LFO under a soft saw: waves under a chord.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "mixOsc1", 0.45f }, { "mixNoise", 0.85f }, { "cutoff", 0.643f }, { "resonance", 0.3f }, { "attack", 0.77f }, { "decay", 0.811f }, { "sustain", 0.9f }, { "release", 0.837f }, { "reverbMix", 0.35f }, { "lfoRate", 0.055f }, { "lfoCutoffAmt", 0.36f }, { "modWheel", 1.0f }, { "volume", 1.0f } } },

        { "Pads & Drones", "Organ Drone", "Square fifths in DRONE mode, slow PWM: a held harmonium.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 7.0f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.45f }, { "cutoff", 0.684f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.2f }, { "lfoPwAmt", 0.5f }, { "modWheel", 1.0f }, { "volume", 0.587f } } },

        { "Pads & Drones", "Tension Rise", "Slow attack and a filter that keeps opening: a riser for a film cue. Hold the note.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.2f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "mixNoise", 0.1f }, { "cutoff", 0.297f }, { "resonance", 0.5f }, { "envAmt", 0.85f }, { "attack", 0.945f }, { "decay", 0.94f }, { "sustain", 1.0f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "volume", 1.0f } } },

        { "Pads & Drones", "Cosmic Drift", "Two triangles with a wandering pitch from the S/H LFO: lo-fi tape drift.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.3f }, { "cutoff", 0.725f }, { "attack", 0.619f }, { "decay", 0.73f }, { "sustain", 0.9f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "lfoRate", 0.26f }, { "lfoPitchAmt", 0.091f }, { "modWheel", 1.0f }, { "attenuator", 0.012f }, { "volume", 0.387f } },
          "modShOut:attIn attOut:osc1PitchIn" },

        { "Pads & Drones", "Polysynth Pad", "Warm Pad in the BBD chorus, mode II: the lush wide pad of the classic polysynths, from one voice.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.544f }, { "resonance", 0.15f }, { "envAmt", 0.25f }, { "attack", 0.695f }, { "decay", 0.777f }, { "sustain", 0.75f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "lfoRate", 0.177f }, { "lfoPwAmt", 0.4f }, { "modWheel", 1.0f }, { "volume", 0.466f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.519f }, { "choDepth", 0.75f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f } } },

        { "Pads & Drones", "Ensemble Strings", "Slow Strings through the three-line ensemble chorus: a string machine.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.15f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "cutoff", 0.706f }, { "resonance", 0.1f }, { "kbdTrack", 1 }, { "attack", 0.663f }, { "decay", 0.73f }, { "sustain", 0.9f }, { "release", 0.751f }, { "reverbMix", 0.35f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.071f }, { "modWheel", 0.5f }, { "volume", 0.539f }, { "choOn", 1 }, { "choMode", 4 }, { "choRate", 0.234f }, { "choDepth", 0.7f }, { "choMix", 0.55f }, { "choNoise", 0.2f }, { "choAge", 0.25f }, { "choWidth", 1.0f } } },

        { "Pads & Drones", "Phased Drone", "Dark Drone with a very slow eight-stage phaser: the notches drift through the fifths.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 7.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.486f }, { "resonance", 0.4f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.014f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.32f }, { "modWheel", 1.0f }, { "volume", 1.0f }, { "phaOn", 1 }, { "phaMode", 3 }, { "phaRate", 0.08f }, { "phaDepth", 0.7f }, { "phaFeedback", 0.3f }, { "phaCenter", 0.6f }, { "phaStereo", 0.6f }, { "phaMix", 0.45f } } },

        { "Pads & Drones", "Tape Drift Pad", "Cosmic Drift in a worn chorus and an endless tape wash: lo-fi ambient.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.3f }, { "cutoff", 0.725f }, { "attack", 0.619f }, { "decay", 0.73f }, { "sustain", 0.9f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "lfoRate", 0.26f }, { "lfoPitchAmt", 0.091f }, { "modWheel", 1.0f }, { "attenuator", 0.012f }, { "volume", 0.412f }, { "choOn", 1 }, { "choRate", 0.453f }, { "choDepth", 0.65f }, { "choWidth", 0.8f }, { "choNoise", 0.8f }, { "choAge", 0.85f }, { "choMode", 0 }, { "choMix", 0.5f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.4f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f }, { "tapSync", 0 } },
          "modShOut:attIn attOut:osc1PitchIn" },

        { "Pads & Drones", "Shimmer Echo", "Shimmer Swell spread by chorus and set in a long three-head echo: cinematic air, wider and longer.",
          { { "osc1Octave", 3 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.788f }, { "attack", 0.794f }, { "decay", 0.811f }, { "sustain", 0.9f }, { "release", 0.837f }, { "reverbMix", 0.35f }, { "lfoRate", 0.424f }, { "lfoPitchAmt", 0.065f }, { "modWheel", 0.6f }, { "volume", 0.35f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.355f }, { "choDepth", 0.85f }, { "choMix", 0.6f }, { "choNoise", 0.15f }, { "choAge", 0.2f }, { "choWidth", 1.0f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.35f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f }, { "tapSync", 0 } } },

        { "Pads & Drones", "Rotary Organ Drone", "Organ Drone through a tube preamp and a fast phaser warble: a harmonium in front of a rotating speaker.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 7.0f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.45f }, { "cutoff", 0.684f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.2f }, { "lfoPwAmt", 0.5f }, { "modWheel", 1.0f }, { "volume", 0.616f }, { "satOn", 1 }, { "satDrive", 0.45f }, { "satMode", 0 }, { "satBias", 0.35f }, { "satBody", 0.4f }, { "satTone", 0.5f }, { "satMix", 1.0f }, { "satOutput", 0.5f }, { "phaOn", 1 }, { "phaMode", 1 }, { "phaRate", 0.7f }, { "phaDepth", 0.5f }, { "phaFeedback", 0.3f }, { "phaCenter", 0.55f }, { "phaMix", 0.4f }, { "phaStereo", 0.3f } } },

        // ---------------------------------------------------------------- Brass & Strings
        { "Brass & Strings", "Brass Swell", "Detuned saws with a slow filter swell and singing resonance.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.46f }, { "resonance", 0.5f }, { "envAmt", 0.6f }, { "attack", 0.575f }, { "decay", 0.703f }, { "sustain", 0.55f }, { "release", 0.67f }, { "reverbMix", 0.35f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.058f }, { "modWheel", 0.25f }, { "volume", 0.784f } } },

        { "Brass & Strings", "Brass Stab", "Fast filter blip and a short decay: funk horn section hits.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.527f }, { "resonance", 0.2f }, { "envAmt", 0.5f }, { "attack", 0.226f }, { "decay", 0.567f }, { "sustain", 0.3f }, { "release", 0.507f }, { "reverbMix", 0.2f }, { "volume", 0.921f } } },

        { "Brass & Strings", "French Horn", "Mellow: triangle + saw, low cutoff, slow filter rise and gentle vibrato.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "cutoff", 0.508f }, { "resonance", 0.1f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.476f }, { "decay", 0.67f }, { "sustain", 0.8f }, { "release", 0.622f }, { "reverbMix", 0.35f }, { "lfoRate", 0.43f }, { "lfoPitchAmt", 0.065f }, { "modWheel", 0.4f }, { "volume", 0.658f } } },

        { "Brass & Strings", "Solo Cello", "Saw with a slow bow attack, half key tracking and delayed vibrato from the wheel.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 3 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.643f }, { "resonance", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.544f }, { "decay", 0.703f }, { "sustain", 0.9f }, { "release", 0.622f }, { "reverbMix", 0.35f }, { "glide", 0.346f }, { "lfoRate", 0.438f }, { "lfoPitchAmt", 0.079f }, { "modWheel", 0.5f }, { "volume", 0.783f } } },

        { "Brass & Strings", "Trumpet", "Saw with a bright filter attack and a fall back: bold lead brass.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.35f }, { "cutoff", 0.585f }, { "resonance", 0.15f }, { "envAmt", 0.4f }, { "kbdTrack", 2 }, { "attack", 0.369f }, { "decay", 0.588f }, { "sustain", 0.6f }, { "release", 0.541f }, { "reverbMix", 0.25f }, { "lfoRate", 0.444f }, { "lfoPitchAmt", 0.071f }, { "modWheel", 0.3f }, { "volume", 0.831f } } },

        { "Brass & Strings", "Ensemble Brass", "Brass Swell spread by the ensemble chorus: a section of horns instead of one.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.46f }, { "resonance", 0.5f }, { "envAmt", 0.6f }, { "attack", 0.575f }, { "decay", 0.703f }, { "sustain", 0.55f }, { "release", 0.67f }, { "reverbMix", 0.35f }, { "lfoRate", 0.434f }, { "lfoPitchAmt", 0.058f }, { "modWheel", 0.25f }, { "volume", 0.84f }, { "choOn", 1 }, { "choMode", 4 }, { "choRate", 0.234f }, { "choDepth", 0.7f }, { "choMix", 0.55f }, { "choNoise", 0.2f }, { "choAge", 0.25f }, { "choWidth", 1.0f } } },

        { "Brass & Strings", "Tube Brass Stab", "Brass Stab hit through a hot preamp with a short slap: funk horns through a cranked amp.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.527f }, { "resonance", 0.2f }, { "envAmt", 0.5f }, { "attack", 0.226f }, { "decay", 0.567f }, { "sustain", 0.3f }, { "release", 0.507f }, { "reverbMix", 0.2f }, { "volume", 0.679f }, { "satOn", 1 }, { "satDrive", 0.7f }, { "satBias", 0.55f }, { "satBody", 0.3f }, { "satTone", 0.62f }, { "satOutput", 0.45f }, { "satMode", 0 }, { "satMix", 1.0f }, { "tapOn", 1 }, { "tapMix", 0.3f }, { "tapHeads", 0 }, { "tapSync", 0 }, { "tapTime", 0.3736f }, { "tapFeedback", 0.12f }, { "tapWow", 0.15f }, { "tapFlutter", 0.2f }, { "tapAge", 0.3f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } } },

        { "Brass & Strings", "Cello Hall", "Solo Cello with a little chorus and a soft, long tape tail: a cello in a big hall.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 3 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.643f }, { "resonance", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.544f }, { "decay", 0.703f }, { "sustain", 0.9f }, { "release", 0.622f }, { "reverbMix", 0.35f }, { "glide", 0.346f }, { "lfoRate", 0.438f }, { "lfoPitchAmt", 0.079f }, { "modWheel", 0.5f }, { "volume", 0.826f }, { "choOn", 1 }, { "choMix", 0.4f }, { "choMode", 0 }, { "choRate", 0.4236f }, { "choDepth", 0.6f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.22f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f }, { "tapSync", 0 } } },

        { "Brass & Strings", "Horn Echo", "French Horn with a light three-head tape echo: a horn call answered from the hills.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "cutoff", 0.508f }, { "resonance", 0.1f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.476f }, { "decay", 0.67f }, { "sustain", 0.8f }, { "release", 0.622f }, { "reverbMix", 0.35f }, { "lfoRate", 0.43f }, { "lfoPitchAmt", 0.065f }, { "modWheel", 0.4f }, { "volume", 0.658f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.28f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } } },

        // ---------------------------------------------------------------- Arpeggios
        { "Arpeggios", "Arp Minor Pluck", "Hold a chord: up and down over two octaves, 1/16 at 128 BPM.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.4f }, { "cutoff", 0.508f }, { "resonance", 0.4f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.0f }, { "release", 0.507f }, { "reverbMix", 0.25f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 1 }, { "arpOctSeq", 1 }, { "arpRate", 0.703f }, { "arpFreeStep", 2 }, { "volume", 1.0f } } },

        { "Arpeggios", "Trance Gate Arp", "Hold a chord: bright saw arpeggio over three octaves, 1/16 at 138 BPM, plenty of spring.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.643f }, { "resonance", 0.35f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.514f }, { "sustain", 0.2f }, { "release", 0.459f }, { "reverbMix", 0.35f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 2 }, { "arpRate", 0.732f }, { "arpFreeStep", 2 }, { "volume", 1.0f } } },

        { "Arpeggios", "Random Bleeps", "Hold a chord: notes in random order over three octaves, square bleeps, 1/16 at 120 BPM.",
          { { "osc1Octave", 3 }, { "osc1Wave", 2 }, { "mixOsc1", 0.6f }, { "cutoff", 0.788f }, { "resonance", 0.2f }, { "envAmt", 0.2f }, { "attack", 0.0f }, { "decay", 0.433f }, { "sustain", 0.0f }, { "release", 0.399f }, { "reverbMix", 0.3f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 2 }, { "arpOctSeq", 2 }, { "arpRate", 0.679f }, { "arpFreeStep", 2 }, { "volume", 1.0f } } },

        { "Arpeggios", "Slow Eighths", "Hold a chord: warm 1/8 arpeggio at 90 BPM, one octave; for verses.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.3f }, { "cutoff", 0.671f }, { "resonance", 0.2f }, { "envAmt", 0.25f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.2f }, { "release", 0.567f }, { "reverbMix", 0.3f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 0 }, { "arpRate", 0.57f }, { "arpFreeStep", 1 }, { "volume", 0.955f } } },

        { "Arpeggios", "Synthwave Arp", "Hold a chord: saw octaves 1/16 at 100 BPM over two octaves with a long spring. Retro night drive.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.607f }, { "resonance", 0.3f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.1f }, { "release", 0.507f }, { "reverbMix", 0.45f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 1 }, { "arpRate", 0.61f }, { "arpFreeStep", 2 }, { "volume", 1.0f } } },

        { "Arpeggios", "Resonant Arp", "Hold a chord: acid-flavoured arpeggio with high resonance, up/down two octaves at 125 BPM.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.7f }, { "cutoff", 0.428f }, { "resonance", 0.75f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.05f }, { "release", 0.433f }, { "reverbMix", 0.15f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 1 }, { "arpOctSeq", 1 }, { "arpRate", 0.694f }, { "arpFreeStep", 2 }, { "volume", 1.0f } } },

        { "Arpeggios", "Bass Arp", "Hold a chord in the low register: driving 1/16 bass arpeggio, one octave, 124 BPM.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.5f }, { "cutoff", 0.396f }, { "resonance", 0.35f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.1f }, { "release", 0.433f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 0 }, { "arpRate", 0.691f }, { "arpFreeStep", 2 }, { "volume", 0.902f } } },

        { "Arpeggios", "Pluck Cascade", "Hold a chord: fast 1/16 plucks at 150 BPM up three octaves into the spring.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.3f }, { "cutoff", 0.585f }, { "resonance", 0.3f }, { "envAmt", 0.45f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.0f }, { "release", 0.541f }, { "reverbMix", 0.4f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 2 }, { "arpRate", 0.763f }, { "arpFreeStep", 2 }, { "volume", 1.0f } } },

        { "Arpeggios", "Sync Arp", "Hold a chord: synced oscillator 2 whose pitch is swept slowly by the LFO, 1/16 at 118 BPM.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 3.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.2f }, { "mixOsc2", 0.8f }, { "cutoff", 0.788f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.2f }, { "release", 0.433f }, { "reverbMix", 0.25f }, { "lfoRate", 0.078f }, { "lfoWave", 0 }, { "attenuator", 0.15f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 1 }, { "arpOctSeq", 1 }, { "arpRate", 0.673f }, { "arpFreeStep", 2 }, { "volume", 0.9f } },
          "modWaveOut:attIn attOut:osc2PitchIn" },

        { "Arpeggios", "Quarter Pulse", "Hold a chord: slow 1/4 arpeggio at 80 BPM with long release: ambient steps.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.643f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.25f }, { "decay", 0.703f }, { "sustain", 0.3f }, { "release", 0.73f }, { "reverbMix", 0.55f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 1 }, { "arpRate", 0.525f }, { "arpFreeStep", 0 }, { "volume", 0.67f } } },

        { "Arpeggios", "Chip Arp", "Hold a chord: very fast square arpeggio (1/16 at 210 BPM) for 8-bit chords.",
          { { "osc1Octave", 3 }, { "osc1Wave", 2 }, { "mixOsc1", 0.6f }, { "cutoff", 0.935f }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.8f }, { "release", 0.318f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 0 }, { "arpRate", 0.891f }, { "arpFreeStep", 2 }, { "volume", 0.555f } } },

        { "Arpeggios", "Synthwave Arp Echo", "Synthwave Arp with chorus and a dotted-eighth echo locked to the tempo: the retro night drive.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.607f }, { "resonance", 0.3f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.1f }, { "release", 0.507f }, { "reverbMix", 0.45f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 1 }, { "arpRate", 0.61f }, { "arpFreeStep", 2 }, { "volume", 0.907f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.519f }, { "choDepth", 0.75f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f }, { "tapOn", 1 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.4f }, { "tapMix", 0.3f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapHeads", 0 }, { "tapWow", 0.15f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } } },

        { "Arpeggios", "Jet Minor Pluck", "Arp Minor Pluck through the resonant OTA phaser: the jet sweep runs across the arpeggio.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.4f }, { "cutoff", 0.508f }, { "resonance", 0.4f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.0f }, { "release", 0.507f }, { "reverbMix", 0.25f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 1 }, { "arpOctSeq", 1 }, { "arpRate", 0.703f }, { "arpFreeStep", 2 }, { "volume", 0.781f }, { "phaOn", 1 }, { "phaMode", 2 }, { "phaRate", 0.22f }, { "phaDepth", 1.0f }, { "phaFeedback", 0.75f }, { "phaCenter", 0.45f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } } },

        { "Arpeggios", "Dub Arp", "Slow Eighths in a dub echo: every step trails off in darkening repeats.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.3f }, { "cutoff", 0.671f }, { "resonance", 0.2f }, { "envAmt", 0.25f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.2f }, { "release", 0.567f }, { "reverbMix", 0.3f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 0 }, { "arpRate", 0.57f }, { "arpFreeStep", 1 }, { "volume", 0.892f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.35f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f }, { "tapSync", 0 } } },

        { "Arpeggios", "Acid Arp Drive", "Resonant Arp into diode distortion: the acid arpeggio with teeth.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.7f }, { "cutoff", 0.428f }, { "resonance", 0.75f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.05f }, { "release", 0.433f }, { "reverbMix", 0.15f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 1 }, { "arpOctSeq", 1 }, { "arpRate", 0.694f }, { "arpFreeStep", 2 }, { "volume", 0.675f }, { "satOn", 1 }, { "satMode", 3 }, { "satDrive", 0.75f }, { "satBias", 0.2f }, { "satBody", 0.35f }, { "satTone", 0.6f }, { "satMix", 0.85f }, { "satOutput", 0.45f } } },

        { "Arpeggios", "Cassette Arp", "Quarter Pulse from a worn cassette: wow, flutter and a tired chorus on slow ambient steps.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.643f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.25f }, { "decay", 0.703f }, { "sustain", 0.3f }, { "release", 0.73f }, { "reverbMix", 0.55f }, { "arpPlay", 1 }, { "arpMode", 0 }, { "arpDirection", 0 }, { "arpOctSeq", 1 }, { "arpRate", 0.525f }, { "arpFreeStep", 0 }, { "volume", 0.711f }, { "choOn", 1 }, { "choRate", 0.453f }, { "choDepth", 0.65f }, { "choWidth", 0.8f }, { "choNoise", 0.8f }, { "choAge", 0.85f }, { "choMode", 0 }, { "choMix", 0.5f }, { "tapOn", 1 }, { "tapHeads", 1 }, { "tapTime", 0.406f }, { "tapFeedback", 0.4f }, { "tapWow", 0.85f }, { "tapFlutter", 0.6f }, { "tapAge", 0.85f }, { "tapBass", 0.45f }, { "tapTreble", 0.35f }, { "tapSync", 0 }, { "tapMix", 0.45f } } },

        // ---------------------------------------------------------------- Sequences
        { "Sequences", "Seq Acid Line", "Plays by itself; a key transposes. Accents open the filter through KB VEL OUT -> attenuator -> CUTOFF IN. OCT/SEQ selects one of three lines.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.65f }, { "cutoff", 0.366f }, { "resonance", 0.7f }, { "envAmt", 0.6f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.1f }, { "release", 0.433f }, { "reverbMix", 0.12f }, { "glide", 0.435f }, { "legatoGlide", 1 }, { "attenuator", 0.3f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.703f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "kbVelOut:attIn attOut:filterCutoffIn",
          "36 36 48 36A 39 R 43 36 36T 48 36A 46 R 41 43 36",
          "48 52 55 60 55 52 48 55",
          "36 R 36 R 43 R 41 R" },

        { "Sequences", "Berlin School", "Plays by itself: a 16-step minor sequence, the LFO slowly sweeps the filter. Three variations on OCT/SEQ.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "cutoff", 0.486f }, { "resonance", 0.55f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.05f }, { "release", 0.459f }, { "reverbMix", 0.35f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.24f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.673f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "40 47 52 47 55 47 52 47 40 47 52 54 55 54 52 47",
          "40 52 47 52 43 52 47 52",
          "40 40 52 40 50 40 47 40" },

        { "Sequences", "Octave Disco", "Plays by itself: octave-bouncing disco bass at 124 BPM. A key transposes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.45f }, { "cutoff", 0.476f }, { "resonance", 0.3f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.499f }, { "sustain", 0.0f }, { "release", 0.433f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.691f }, { "arpFreeStep", 2 }, { "volume", 0.947f } },
          "",
          "36 48 36 48 39 51 39 51 41 53 41 53 43 55 43 55",
          "36 48 36 48 36 48 36 48",
          "36 48 43 48 46 48 43 48" },

        { "Sequences", "Techno Rumble", "Plays by itself: dark off-beat rolling bass for techno, 132 BPM.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", -0.1f }, { "mixOsc1", 0.8f }, { "mixOsc2", 0.5f }, { "mixNoise", 0.08f }, { "cutoff", 0.342f }, { "resonance", 0.45f }, { "envAmt", 0.4f }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.0f }, { "release", 0.399f }, { "reverbMix", 0.2f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.715f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "R 36 36 36 R 36 36 36 R 36 36 36 R 36 39 36",
          "R 36A 36 R R 36A 36 R R 36A 36 R R 36A 34 R",
          "36 R R 36 R R 36 R" },

        { "Sequences", "Electro Funk", "Plays by itself: syncopated electro bass with rests and accents, 112 BPM.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.04f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.428f }, { "resonance", 0.5f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.0f }, { "release", 0.399f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.653f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "36 R R 36 R R 48 R 36 R 39 R 41 R 43T 43",
          "36 36 R 46 R 48 R 36 R 36 R 43 R 41 R 39",
          "36 R 36 R 36 R 36 R" },

        { "Sequences", "Ambient Steps", "Plays by itself: slow major-pentatonic steps with long release into the spring.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.3f }, { "cutoff", 0.725f }, { "kbdTrack", 1 }, { "attack", 0.25f }, { "decay", 0.717f }, { "sustain", 0.2f }, { "release", 0.751f }, { "reverbMix", 0.6f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.485f }, { "arpFreeStep", 1 }, { "volume", 0.409f } },
          "",
          "60 62 67 69 72 69 67 64",
          "60 R 67 R 64 R 72 R",
          "60 64 67 71 72 71 67 64" },

        { "Sequences", "Minimal Pulse", "Plays by itself: one-note minimal pulse with ties and accents, 126 BPM; the filter LFO evolves it.",
          { { "osc1Octave", 1 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.465f }, { "resonance", 0.6f }, { "envAmt", 0.35f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.459f }, { "sustain", 0.0f }, { "release", 0.399f }, { "reverbMix", 0.25f }, { "lfoRate", 0.014f }, { "lfoCutoffAmt", 0.28f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.697f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "",
          "48 48A 48 48T 48 R 48A 48",
          "48 R 48 R 51 R 48 R",
          "48 55 48 58 48 55 48 60" },

        { "Sequences", "Dark Pulse Seq", "Plays by itself: minor eighth-note ostinato for thriller scores, 96 BPM.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.508f }, { "resonance", 0.4f }, { "envAmt", 0.3f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.541f }, { "sustain", 0.1f }, { "release", 0.481f }, { "reverbMix", 0.35f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.594f }, { "arpFreeStep", 1 }, { "volume", 1.0f } },
          "",
          "38 41 45 41 38 41 46 41",
          "38 38 38 38 36 36 36 36",
          "38 45 50 45 38 46 50 46" },

        { "Sequences", "Gallop", "Plays by itself: galloping eighth + two sixteenths rhythm, 140 BPM; heavy and driving.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.75f }, { "mixOsc2", 0.7f }, { "cutoff", 0.454f }, { "resonance", 0.3f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.459f }, { "sustain", 0.0f }, { "release", 0.378f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.737f }, { "arpFreeStep", 2 }, { "volume", 0.989f } },
          "",
          "36T 36 36 36 36T 36 36 36 36T 36 36 36 39T 39 39 39",
          "36 R 36 36 36 R 36 36",
          "36 36 36 36 34 34 34 34" },

        { "Sequences", "Bleep Melody", "Plays by itself: square bleep melody at 130 BPM, chiptune flavour.",
          { { "osc1Octave", 3 }, { "osc1Wave", 2 }, { "mixOsc1", 0.6f }, { "cutoff", 0.856f }, { "attack", 0.0f }, { "decay", 0.399f }, { "sustain", 0.0f }, { "release", 0.378f }, { "reverbMix", 0.2f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.709f }, { "arpFreeStep", 2 }, { "volume", 0.98f } },
          "",
          "72 R 76 79 R 76 72 R 74 R 77 81 R 77 74 R",
          "72 79 76 79 72 79 76 79",
          "72 R 72 R 84 R 72 R" },

        { "Sequences", "Funk Seq", "Plays by itself: syncopated funk bass with glide on the tied notes, 104 BPM.",
          { { "osc1Octave", 1 }, { "osc1Wave", 2 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.441f }, { "resonance", 0.45f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.2f }, { "release", 0.399f }, { "glide", 0.411f }, { "legatoGlide", 1 }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.625f }, { "arpFreeStep", 2 }, { "volume", 0.919f } },
          "",
          "36 R 36 48 R 46 R 43 R 41 43T 46 R 36 R R",
          "36 R R 36 R R 43 R 36 R R 36 R 46 48 R",
          "36 36 R 48 R 36 R 46" },

        { "Sequences", "Flying Chirps", "Plays by itself: the oscillators are off, the self-oscillating filter sings a high chirping line (Kbd Track 1:1). GATE OUT restarts the LFO saw, which flicks the cutoff through the attenuator. Three variations on OCT/SEQ.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 1 }, { "osc2Wave", 0 }, { "mixOsc1", 0.15f }, { "lfoRate", 0.364f }, { "lfoWave", 1 }, { "cutoff", 0.4669f }, { "resonance", 1.0f }, { "envAmt", 0.02f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.56f }, { "sustain", 0.0f }, { "release", 0.56f }, { "reverbMix", 0.06f }, { "attenuator", 0.08f }, { "arpPlay", 1 }, { "arpHold", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.6944f }, { "arpFreeStep", 2 }, { "volume", 1.0f } },
          "gateOut:modSyncIn modWaveOut:attIn attOut:filterCutoffIn",
          "R 92 R R 97 R R 88 R R 100 R 95 R 104 R",
          "R 92 R 97 R R 100 R R 88 R R 104 R 97 R",
          "92 R 97 R 88 104 R 92 R 100 R 97 R 109 95 R" },

        { "Sequences", "Flying Bass", "Plays by itself: a bouncing 16-step bass in C# with octave jumps, saw and square a few cents apart, a short filter envelope. Three variations on OCT/SEQ.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", -0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "lfoRate", 0.46f }, { "lfoWave", 3 }, { "cutoff", 0.4f }, { "resonance", 0.45f }, { "envAmt", 0.42f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.47f }, { "sustain", 0.0f }, { "release", 0.4f }, { "reverbMix", 0.12f }, { "attenuator", 0.5f }, { "arpPlay", 1 }, { "arpHold", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.6944f }, { "arpFreeStep", 2 }, { "volume", 0.85f } },
          "",
          "R 37 37 R 37 R 49 37 R 37 40 R 37 R 44 R",
          "R 37 49 R 37 R 37 52 R 37 R 49 37 R 47 R",
          "R 37 37 R 37 R 49 37 R 37 40 R 37 R 44 R" },

        { "Sequences", "Berlin Echoes", "Berlin School through a slow phaser and three tape heads: the sequence multiplies into a 70s Berlin wall of notes.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "cutoff", 0.486f }, { "resonance", 0.55f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.05f }, { "release", 0.459f }, { "reverbMix", 0.35f }, { "lfoRate", 0.0f }, { "lfoCutoffAmt", 0.24f }, { "modWheel", 1.0f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.673f }, { "arpFreeStep", 2 }, { "volume", 1.0f }, { "phaOn", 1 }, { "phaMode", 3 }, { "phaRate", 0.08f }, { "phaDepth", 0.7f }, { "phaFeedback", 0.3f }, { "phaCenter", 0.4f }, { "phaStereo", 0.6f }, { "phaMix", 0.45f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.3f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } },
          "",
          "40 47 52 47 55 47 52 47 40 47 52 54 55 54 52 47",
          "40 52 47 52 43 52 47 52",
          "40 40 52 40 50 40 47 40" },

        { "Sequences", "Acid Line Drive", "Seq Acid Line through diode distortion and a dotted-eighth echo.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "mixOsc1", 0.65f }, { "cutoff", 0.366f }, { "resonance", 0.7f }, { "envAmt", 0.6f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.1f }, { "release", 0.433f }, { "reverbMix", 0.12f }, { "glide", 0.435f }, { "legatoGlide", 1 }, { "attenuator", 0.3f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.703f }, { "arpFreeStep", 2 }, { "volume", 0.668f }, { "satOn", 1 }, { "satMode", 3 }, { "satDrive", 0.75f }, { "satBias", 0.2f }, { "satBody", 0.35f }, { "satTone", 0.6f }, { "satMix", 0.85f }, { "satOutput", 0.45f }, { "tapOn", 1 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.3f }, { "tapMix", 0.22f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapHeads", 0 }, { "tapWow", 0.15f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } },
          "kbVelOut:attIn attOut:filterCutoffIn",
          "36 36 48 36A 39 R 43 36 36T 48 36A 46 R 41 43 36",
          "48 52 55 60 55 52 48 55",
          "36 R 36 R 43 R 41 R" },

        { "Sequences", "Dub Techno Rumble", "Techno Rumble with chorus and a long dub echo: the deep, smeared chords of dub techno.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", -0.1f }, { "mixOsc1", 0.8f }, { "mixOsc2", 0.5f }, { "mixNoise", 0.08f }, { "cutoff", 0.342f }, { "resonance", 0.45f }, { "envAmt", 0.4f }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.0f }, { "release", 0.399f }, { "reverbMix", 0.2f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.715f }, { "arpFreeStep", 2 }, { "volume", 0.799f }, { "choOn", 1 }, { "choMode", 0 }, { "choRate", 0.4236f }, { "choDepth", 0.6f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.3f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f }, { "tapSync", 0 } },
          "",
          "R 36 36 36 R 36 36 36 R 36 36 36 R 36 39 36",
          "R 36A 36 R R 36A 36 R R 36A 36 R R 36A 34 R",
          "36 R R 36 R R 36 R" },

        { "Sequences", "Ambient Tape Steps", "Ambient Steps in a wide chorus and an endless tape wash.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.3f }, { "cutoff", 0.725f }, { "kbdTrack", 1 }, { "attack", 0.25f }, { "decay", 0.717f }, { "sustain", 0.2f }, { "release", 0.751f }, { "reverbMix", 0.6f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.485f }, { "arpFreeStep", 1 }, { "volume", 0.435f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.355f }, { "choDepth", 0.85f }, { "choMix", 0.6f }, { "choNoise", 0.15f }, { "choAge", 0.2f }, { "choWidth", 1.0f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.4f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f }, { "tapSync", 0 } },
          "",
          "60 62 67 69 72 69 67 64",
          "60 R 67 R 64 R 72 R",
          "60 64 67 71 72 71 67 64" },

        { "Sequences", "Disco Phase", "Octave Disco through the four-stage phaser: the swirling disco bass.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.45f }, { "cutoff", 0.476f }, { "resonance", 0.3f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.499f }, { "sustain", 0.0f }, { "release", 0.433f }, { "arpPlay", 1 }, { "arpMode", 1 }, { "arpOctSeq", 0 }, { "arpRate", 0.691f }, { "arpFreeStep", 2 }, { "volume", 0.888f }, { "phaOn", 1 }, { "phaMix", 0.45f }, { "phaMode", 0 }, { "phaRate", 0.3f }, { "phaDepth", 0.8f }, { "phaFeedback", 0.2f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f } },
          "",
          "36 48 36 48 39 51 39 51 41 53 41 53 43 55 43 55",
          "36 48 36 48 36 48 36 48",
          "36 48 43 48 46 48 43 48" },

        // ---------------------------------------------------------------- Drums & Percussion
        { "Drums & Percussion", "Analog Kick", "Triangle with a fast pitch drop from the envelope (cables: + ENV OUT -> attenuator -> PITCH IN). Play around C2-C3.",
          { { "osc1Octave", 1 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.9f }, { "mixOsc2", 0.15f }, { "cutoff", 0.486f }, { "resonance", 0.1f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.0f }, { "release", 0.528f }, { "attenuator", 0.3f }, { "volume", 0.8f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "Drums & Percussion", "Boom Kick", "Long 808-style kick: self-oscillating filter dropping in pitch with the envelope.",
          { { "osc1Octave", 1 }, { "osc1Wave", 0 }, { "mixOsc1", 0.35f }, { "cutoff", 0.314f }, { "resonance", 0.9f }, { "envAmt", 0.35f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.636f }, { "sustain", 0.0f }, { "release", 0.636f }, { "volume", 1.0f } } },

        { "Drums & Percussion", "Snare", "Noise and a triangle body, fast decay, a little spring.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "mixOsc1", 0.45f }, { "mixNoise", 0.8f }, { "cutoff", 0.814f }, { "resonance", 0.15f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.499f }, { "sustain", 0.0f }, { "release", 0.499f }, { "reverbMix", 0.15f }, { "attenuator", 0.08f }, { "volume", 1.0f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "Drums & Percussion", "Closed Hat", "Noise through the high-pass (cables: MIXER OUT -> HIGH PASS IN -> VCA IN), very short.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 0.9f }, { "cutoff", 0.599f }, { "attack", 0.0f }, { "decay", 0.352f }, { "sustain", 0.0f }, { "release", 0.352f }, { "hpCutoff", 0.954f }, { "volume", 1.0f } },
          "mixerOut:hpfIn hpfOut:vcaIn" },

        { "Drums & Percussion", "Open Hat", "Noise through the high-pass, longer decay (cables: MIXER OUT -> HIGH PASS IN -> VCA IN).",
          { { "mixOsc1", 0.0f }, { "mixNoise", 0.9f }, { "cutoff", 0.599f }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.0f }, { "release", 0.606f }, { "hpCutoff", 0.924f }, { "volume", 0.993f } },
          "mixerOut:hpfIn hpfOut:vcaIn" },

        { "Drums & Percussion", "Clap", "Noise burst chopped by a fast square LFO on the filter: the flam of a hand clap, into the spring.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.757f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.514f }, { "sustain", 0.0f }, { "release", 0.514f }, { "reverbMix", 0.25f }, { "lfoRate", 0.61f }, { "lfoWave", 3 }, { "lfoCutoffAmt", 0.6f }, { "modWheel", 1.0f }, { "volume", 1.0f } } },

        { "Drums & Percussion", "Tom", "Self-oscillating filter tom with a short pitch fall; play different keys for the toms.",
          { { "osc1Octave", 1 }, { "osc1Wave", 0 }, { "mixOsc1", 0.3f }, { "cutoff", 0.387f }, { "resonance", 0.9f }, { "envAmt", 0.25f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.567f }, { "sustain", 0.0f }, { "release", 0.567f }, { "reverbMix", 0.15f }, { "volume", 1.0f } } },

        { "Drums & Percussion", "Cowbell", "Two squares about a fifth apart through a resonant filter, short: the 808 bell. Play around C5.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 6.8f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.643f }, { "resonance", 0.55f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.447f }, { "sustain", 0.0f }, { "release", 0.481f }, { "volume", 1.0f } } },

        { "Drums & Percussion", "Rimshot", "Very short resonant click with a bit of noise.",
          { { "osc1Octave", 3 }, { "osc1Wave", 0 }, { "mixOsc1", 0.9f }, { "mixNoise", 0.4f }, { "cutoff", 0.788f }, { "resonance", 0.55f }, { "envAmt", 0.3f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.352f }, { "sustain", 0.0f }, { "release", 0.352f }, { "reverbMix", 0.2f }, { "volume", 1.0f } } },

        { "Drums & Percussion", "Zap", "Laser zap: the self-oscillating filter sweeps down fast from very high.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "mixOsc1", 0.75f }, { "mixNoise", 0.05f }, { "cutoff", 0.256f }, { "resonance", 0.8f }, { "envAmt", 0.95f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.0f }, { "release", 0.481f }, { "reverbMix", 0.2f }, { "volume", 1.0f } } },

        { "Drums & Percussion", "Shaker", "Noise through the high-pass with a soft attack (cables: MIXER OUT -> HIGH PASS IN -> VCA IN).",
          { { "mixOsc1", 0.0f }, { "mixNoise", 0.9f }, { "cutoff", 0.599f }, { "attack", 0.325f }, { "decay", 0.433f }, { "sustain", 0.0f }, { "release", 0.433f }, { "hpCutoff", 0.867f }, { "volume", 1.0f } },
          "mixerOut:hpfIn hpfOut:vcaIn" },

        { "Drums & Percussion", "Iron Kick", "Analog Kick through a saturating transformer: a fatter, rounder thump.",
          { { "osc1Octave", 1 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.9f }, { "mixOsc2", 0.15f }, { "cutoff", 0.486f }, { "resonance", 0.1f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.528f }, { "sustain", 0.0f }, { "release", 0.528f }, { "attenuator", 0.3f }, { "volume", 0.583f }, { "satOn", 1 }, { "satMode", 2 }, { "satDrive", 0.6f }, { "satBias", 0.1f }, { "satBody", 0.75f }, { "satTone", 0.4f }, { "satMix", 1.0f }, { "satOutput", 0.5f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "Drums & Percussion", "Dub Snare", "The snare thrown into a dub echo: one hit, a whole fill of repeats.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "mixOsc1", 0.45f }, { "mixNoise", 0.8f }, { "cutoff", 0.814f }, { "resonance", 0.15f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.499f }, { "sustain", 0.0f }, { "release", 0.499f }, { "reverbMix", 0.15f }, { "attenuator", 0.08f }, { "volume", 1.0f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.72f }, { "tapMix", 0.4f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f }, { "tapSync", 0 } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "Drums & Percussion", "Jet Hat", "Open Hat through the resonant OTA phaser: the jet-plane hats of 70s records.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 0.9f }, { "cutoff", 0.599f }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.0f }, { "release", 0.606f }, { "hpCutoff", 0.924f }, { "volume", 0.895f }, { "phaOn", 1 }, { "phaMode", 2 }, { "phaRate", 0.22f }, { "phaDepth", 1.0f }, { "phaFeedback", 0.75f }, { "phaCenter", 0.45f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } },
          "mixerOut:hpfIn hpfOut:vcaIn" },

        { "Drums & Percussion", "Tape Clap", "Clap squashed by hot tape with a short slap behind it.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.757f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.514f }, { "sustain", 0.0f }, { "release", 0.514f }, { "reverbMix", 0.25f }, { "lfoRate", 0.61f }, { "lfoWave", 3 }, { "lfoCutoffAmt", 0.6f }, { "modWheel", 1.0f }, { "volume", 0.705f }, { "satOn", 1 }, { "satMode", 1 }, { "satDrive", 0.8f }, { "satBias", 0.1f }, { "satBody", 0.5f }, { "satTone", 0.38f }, { "satOutput", 0.45f }, { "satMix", 1.0f }, { "tapOn", 1 }, { "tapMix", 0.3f }, { "tapHeads", 0 }, { "tapSync", 0 }, { "tapTime", 0.3736f }, { "tapFeedback", 0.12f }, { "tapWow", 0.15f }, { "tapFlutter", 0.2f }, { "tapAge", 0.3f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } } },

        // ---------------------------------------------------------------- FX
        { "FX", "Wind", "Noise in DRONE mode, resonant filter moved by the slow sine LFO: wind through a gap. No key needed.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.626f }, { "resonance", 0.5f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.055f }, { "lfoCutoffAmt", 0.4f }, { "modWheel", 1.0f }, { "volume", 1.0f } } },

        { "FX", "Surf", "Noise swelling and falling with the slow LFO on the VCA (cable: MOD WAVE OUT -> VCA AMT IN), DRONE mode: waves on a beach.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.757f }, { "resonance", 0.1f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.036f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.2f }, { "modWheel", 1.0f }, { "volume", 1.0f } },
          "modWaveOut:vcaAmtIn" },

        { "FX", "Droid Chatter", "Random pitches from S/H on both oscillators at a fast rate: a chattering little robot. Hold a key.",
          { { "osc1Octave", 3 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.4f }, { "cutoff", 0.856f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.459f }, { "reverbMix", 0.2f }, { "lfoRate", 0.494f }, { "attenuator", 0.6f }, { "volume", 0.47f } },
          "modShOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "FX", "Siren", "Triangle LFO sweeps the pitch up and down an octave (wheel controls the depth). Hold a key.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.4f }, { "cutoff", 0.788f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "reverbMix", 0.25f }, { "lfoRate", 0.2f }, { "lfoWave", 0 }, { "lfoPitchAmt", 0.707f }, { "modWheel", 1.0f }, { "volume", 0.472f } } },

        { "FX", "UFO", "Sync tone wobbling fast from the LFO plus glide: a flying saucer.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 5.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.2f }, { "mixOsc2", 0.7f }, { "cutoff", 0.814f }, { "resonance", 0.4f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "reverbMix", 0.35f }, { "glide", 0.677f }, { "lfoRate", 0.469f }, { "lfoPitchAmt", 0.354f }, { "modWheel", 1.0f }, { "volume", 0.845f } } },

        { "FX", "Laser Gun", "Very fast downward pitch sweep from the envelope (cables: + ENV OUT -> attenuator -> PITCH IN).",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.3f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.887f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 0.0f }, { "release", 0.507f }, { "reverbMix", 0.3f }, { "attenuator", 0.45f }, { "volume", 0.995f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "FX", "Rising Alarm", "Ramp LFO on pitch: a repeating rising sweep. Hold a key.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "mixOsc1", 0.6f }, { "cutoff", 0.814f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "reverbMix", 0.2f }, { "lfoRate", 0.312f }, { "lfoWave", 2 }, { "lfoPitchAmt", 0.707f }, { "modWheel", 1.0f }, { "volume", 0.644f } } },

        { "FX", "Geiger Counter", "Random clicks: the S/H out triggers the envelope (cable: MOD S/H OUT -> TRIGGER IN) of a short noise click. DRONE off: hold a key.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.92f }, { "resonance", 0.8f }, { "envAmt", 0.3f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.378f }, { "sustain", 0.0f }, { "release", 0.378f }, { "lfoRate", 0.617f }, { "volume", 1.0f } },
          "modShOut:envTriggerIn" },

        { "FX", "Thunder", "Low rumbling noise in a long envelope into the spring tank.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.519f }, { "resonance", 0.35f }, { "envAmt", 0.3f }, { "attack", 0.425f }, { "decay", 0.892f }, { "sustain", 0.0f }, { "release", 0.892f }, { "reverbMix", 0.55f }, { "volume", 1.0f } } },

        { "FX", "Spring Crash", "A short burst of noise slammed into 100 % wet spring: the sound of kicking the tank.",
          { { "mixOsc1", 0.3f }, { "mixNoise", 1.0f }, { "cutoff", 0.856f }, { "envAmt", 0.2f }, { "attack", 0.0f }, { "decay", 0.433f }, { "sustain", 0.0f }, { "release", 0.433f }, { "reverbMix", 1.0f }, { "volume", 1.0f } } },

        { "FX", "Riser", "Hold a key: pitch and filter climb together over about six seconds, noise underneath. Cables: + ENV OUT -> attenuator -> both PITCH IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.2f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.55f }, { "mixNoise", 0.3f }, { "cutoff", 0.387f }, { "resonance", 0.35f }, { "envAmt", 0.75f }, { "attack", 0.945f }, { "decay", 0.811f }, { "sustain", 1.0f }, { "release", 0.67f }, { "reverbMix", 0.35f }, { "attenuator", 0.3f }, { "volume", 1.0f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "FX", "Downlifter", "Starts two octaves up and falls away over three seconds while the filter closes. Cables: + ENV OUT -> attenuator -> both PITCH IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.15f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.5f }, { "mixNoise", 0.2f }, { "cutoff", 0.428f }, { "resonance", 0.3f }, { "envAmt", 0.7f }, { "attack", 0.0f }, { "decay", 0.859f }, { "sustain", 0.0f }, { "release", 0.859f }, { "reverbMix", 0.4f }, { "attenuator", 0.25f }, { "volume", 0.803f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "FX", "Noise Sweep Up", "White noise through the resonant filter, opening over three seconds while the key is held: the classic build-up.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.355f }, { "resonance", 0.5f }, { "envAmt", 0.95f }, { "attack", 0.869f }, { "decay", 0.73f }, { "sustain", 1.0f }, { "release", 0.703f }, { "reverbMix", 0.35f }, { "volume", 1.0f } } },

        { "FX", "Noise Sweep Down", "White noise that starts bright and sweeps down over four seconds: a wash after a drop.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.256f }, { "resonance", 0.55f }, { "envAmt", 0.95f }, { "attack", 0.0f }, { "decay", 0.892f }, { "sustain", 0.0f }, { "release", 0.892f }, { "reverbMix", 0.45f }, { "volume", 1.0f } } },

        { "FX", "Sub Drop", "A deep triangle that falls two octaves over a second and a half: hit it on the downbeat. Cables: + ENV OUT -> attenuator -> both PITCH IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 0 }, { "mixOsc1", 0.9f }, { "mixOsc2", 0.5f }, { "cutoff", 0.585f }, { "attack", 0.0f }, { "decay", 0.785f }, { "sustain", 0.0f }, { "release", 0.785f }, { "attenuator", 0.25f }, { "volume", 0.848f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "FX", "Impact Boom", "Low pitch-dropping thump plus a noise burst into a 70 % wet spring: a cinematic hit. Cables: + ENV OUT -> attenuator -> both PITCH IN.",
          { { "osc1Octave", 0 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.9f }, { "mixOsc2", 0.3f }, { "mixNoise", 0.5f }, { "cutoff", 0.527f }, { "resonance", 0.2f }, { "envAmt", 0.4f }, { "attack", 0.0f }, { "decay", 0.717f }, { "sustain", 0.0f }, { "release", 0.717f }, { "reverbMix", 0.7f }, { "attenuator", 0.3f }, { "volume", 0.813f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "FX", "Reverse Swell", "Swells in over 1.5 s and cuts off dead, into the spring: sounds like a reversed note. Release the key on the beat.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.46f }, { "resonance", 0.3f }, { "envAmt", 0.6f }, { "attack", 0.794f }, { "decay", 0.189f }, { "sustain", 0.0f }, { "release", 0.189f }, { "reverbMix", 0.55f }, { "volume", 0.461f } } },

        { "FX", "Laser Riser", "Synced oscillator whose pitch rises with a slow envelope: a screaming sweep. Cables: + ENV OUT -> attenuator -> OSC 2 PITCH IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Sync", 1 }, { "mixOsc1", 0.2f }, { "mixOsc2", 0.85f }, { "cutoff", 0.856f }, { "resonance", 0.2f }, { "attack", 0.901f }, { "decay", 0.73f }, { "sustain", 1.0f }, { "release", 0.622f }, { "reverbMix", 0.3f }, { "attenuator", 0.35f }, { "volume", 0.874f } },
          "envPosOut:attIn attOut:osc2PitchIn" },

        { "FX", "Glitch Stutter", "Hold a key: a fast square LFO chops the sound (DRONE VCA, MOD WAVE OUT -> VCA AMT IN) while S/H throws random pitches (MOD S/H OUT -> attenuator -> PITCH IN).",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.4f }, { "cutoff", 0.814f }, { "resonance", 0.3f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "lfoRate", 0.523f }, { "lfoWave", 3 }, { "attenuator", 0.3f }, { "volume", 0.961f } },
          "modWaveOut:vcaAmtIn modShOut:attIn attOut:osc1PitchIn" },

        { "FX", "Radio Static", "Crackling short-wave noise: the S/H jumps the cutoff of a resonant noise filter. DRONE VCA, no key needed. Cables: MOD S/H OUT -> attenuator -> CUTOFF IN.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.742f }, { "resonance", 0.6f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.2f }, { "lfoRate", 0.575f }, { "attenuator", 0.5f }, { "volume", 1.0f } },
          "modShOut:attIn attOut:filterCutoffIn" },

        { "FX", "Heartbeat", "Low thumps re-triggered by the LFO square at about 70 BPM: hold a key. Cable: MOD WAVE OUT -> TRIGGER IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 0 }, { "mixOsc1", 0.9f }, { "cutoff", 0.413f }, { "resonance", 0.3f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.0f }, { "release", 0.481f }, { "lfoRate", 0.287f }, { "lfoWave", 3 }, { "volume", 1.0f } },
          "modWaveOut:envTriggerIn" },

        { "FX", "Bubbles", "The self-oscillating filter pinged by the LFO square at random pitches from S/H: underwater bubbles. Hold a key. Cables: MOD WAVE OUT -> TRIGGER IN, MOD S/H OUT -> attenuator -> CUTOFF IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 0 }, { "mixOsc1", 0.55f }, { "cutoff", 0.643f }, { "resonance", 0.92f }, { "envAmt", 0.4f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.399f }, { "sustain", 0.0f }, { "release", 0.399f }, { "reverbMix", 0.35f }, { "lfoRate", 0.469f }, { "lfoWave", 3 }, { "attenuator", 0.45f }, { "volume", 1.0f } },
          "modWaveOut:envTriggerIn modShOut:attIn attOut:filterCutoffIn" },

        { "FX", "Runaway Droid", "Droid Chatter into a tape echo on the edge of self-oscillation: the robot talks over its own echoes.",
          { { "osc1Octave", 3 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 0 }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.4f }, { "cutoff", 0.856f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.459f }, { "reverbMix", 0.2f }, { "lfoRate", 0.494f }, { "attenuator", 0.6f }, { "volume", 0.47f }, { "tapOn", 1 }, { "tapHeads", 4 }, { "tapTime", 0.435f }, { "tapFeedback", 0.97f }, { "tapMix", 0.4f }, { "tapWow", 0.35f }, { "tapFlutter", 0.3f }, { "tapAge", 0.5f }, { "tapBass", 0.55f }, { "tapTreble", 0.45f }, { "tapSync", 0 } },
          "modShOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "FX", "Jet Wind", "Wind through the resonant OTA phaser: a jet passing overhead.",
          { { "mixOsc1", 0.0f }, { "mixNoise", 1.0f }, { "cutoff", 0.626f }, { "resonance", 0.5f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.35f }, { "lfoRate", 0.055f }, { "lfoCutoffAmt", 0.4f }, { "modWheel", 1.0f }, { "volume", 0.726f }, { "phaOn", 1 }, { "phaMode", 2 }, { "phaRate", 0.22f }, { "phaDepth", 1.0f }, { "phaFeedback", 0.75f }, { "phaCenter", 0.45f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } } },

        { "FX", "Space Siren", "Siren in a wide chorus and a long tape wash: far away, in space.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.4f }, { "cutoff", 0.788f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "reverbMix", 0.25f }, { "lfoRate", 0.2f }, { "lfoWave", 0 }, { "lfoPitchAmt", 0.707f }, { "modWheel", 1.0f }, { "volume", 0.417f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.355f }, { "choDepth", 0.85f }, { "choMix", 0.6f }, { "choNoise", 0.15f }, { "choAge", 0.2f }, { "choWidth", 1.0f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.691f }, { "tapFeedback", 0.8f }, { "tapMix", 0.4f }, { "tapWow", 0.45f }, { "tapFlutter", 0.3f }, { "tapAge", 0.55f }, { "tapBass", 0.45f }, { "tapTreble", 0.3f }, { "tapSync", 0 } } },

        { "FX", "Dub Siren", "Rising Alarm thrown into a long dub echo: the sound-system siren.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "mixOsc1", 0.6f }, { "cutoff", 0.814f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "reverbMix", 0.2f }, { "lfoRate", 0.312f }, { "lfoWave", 2 }, { "lfoPitchAmt", 0.707f }, { "modWheel", 1.0f }, { "volume", 0.493f }, { "tapOn", 1 }, { "tapHeads", 5 }, { "tapTime", 0.575f }, { "tapFeedback", 0.8f }, { "tapMix", 0.5f }, { "tapWow", 0.3f }, { "tapFlutter", 0.25f }, { "tapAge", 0.45f }, { "tapBass", 0.65f }, { "tapTreble", 0.35f }, { "tapSync", 0 } } },

        { "FX", "UFO Phaser", "UFO through an eight-stage phaser and three tape heads.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 5.0f }, { "osc2Sync", 1 }, { "mixOsc1", 0.2f }, { "mixOsc2", 0.7f }, { "cutoff", 0.814f }, { "resonance", 0.4f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "reverbMix", 0.35f }, { "glide", 0.677f }, { "lfoRate", 0.469f }, { "lfoPitchAmt", 0.354f }, { "modWheel", 1.0f }, { "volume", 0.9f }, { "phaOn", 1 }, { "phaMode", 3 }, { "phaRate", 0.25f }, { "phaDepth", 0.85f }, { "phaFeedback", 0.5f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.3f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f } } },

        { "FX", "Crushed Impact", "Impact Boom through diode distortion: a torn, crushed hit.",
          { { "osc1Octave", 0 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.9f }, { "mixOsc2", 0.3f }, { "mixNoise", 0.5f }, { "cutoff", 0.527f }, { "resonance", 0.2f }, { "envAmt", 0.4f }, { "attack", 0.0f }, { "decay", 0.717f }, { "sustain", 0.0f }, { "release", 0.717f }, { "reverbMix", 0.7f }, { "attenuator", 0.3f }, { "volume", 0.74f }, { "satOn", 1 }, { "satMode", 3 }, { "satDrive", 0.75f }, { "satBias", 0.2f }, { "satBody", 0.35f }, { "satTone", 0.6f }, { "satOutput", 0.45f }, { "satMix", 1.0f } },
          "envPosOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        // ---------------------------------------------------------------- Patch Bay
        { "Patch Bay", "Gated Drone", "DRONE VCA chopped by the square LFO through VCA AMT IN. Cable: MOD WAVE OUT -> VCA AMT IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.607f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.25f }, { "lfoRate", 0.453f }, { "lfoWave", 3 }, { "volume", 0.7f } },
          "modWaveOut:vcaAmtIn" },

        { "Patch Bay", "Keytracked Wobble", "The LFO rate follows the keyboard (1 V/oct): higher notes wobble faster. Cable: KB OUT -> RATE IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "cutoff", 0.441f }, { "resonance", 0.5f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.567f }, { "vcaMode", 1 }, { "lfoRate", 0.453f }, { "lfoWave", 0 }, { "lfoCutoffAmt", 0.5f }, { "modWheel", 1.0f }, { "volume", 0.721f } },
          "kbOut:modRateIn" },

        { "Patch Bay", "Split Bass and Thin", "Oscillator 1 stays full, oscillator 2 goes through the high-pass first. Cables: OSC 2 WAVE OUT -> HIGH PASS IN -> MIXER OSC 2 IN.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 2 }, { "osc2Wave", 1 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.55f }, { "cutoff", 0.706f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.622f }, { "sustain", 0.8f }, { "release", 0.567f }, { "hpCutoff", 0.681f }, { "volume", 0.868f } },
          "osc2WaveOut:hpfIn hpfOut:mixOsc2In" },

        { "Patch Bay", "Random Notes", "Every key press picks a new random pitch: S/H sampled by GATE OUT -> SYNC IN, attenuated and sent to both oscillators through the mult.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.4f }, { "cutoff", 0.607f }, { "resonance", 0.35f }, { "envAmt", 0.3f }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.3f }, { "release", 0.541f }, { "reverbMix", 0.2f }, { "lfoRate", 0.0f }, { "attenuator", 0.2f }, { "volume", 1.0f } },
          "gateOut:modSyncIn modShOut:attIn attOut:mult1 mult2:osc1PitchIn mult3:osc2PitchIn" },

        { "Patch Bay", "Envelope PWM", "The envelope sweeps the pulse width of oscillator 1 on every note. Cables: + ENV OUT -> attenuator -> OSC 1 PWM IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 3 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.3f }, { "cutoff", 0.725f }, { "resonance", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.703f }, { "sustain", 0.3f }, { "release", 0.622f }, { "reverbMix", 0.2f }, { "attenuator", 0.5f }, { "volume", 0.835f } },
          "envPosOut:attIn attOut:osc1PwmIn" },

        { "Patch Bay", "Audio-Rate Filter FM", "Oscillator 2 modulates the cutoff at audio rate: metallic, vocal sidebands. Cables: OSC 2 WAVE OUT -> attenuator -> CUTOFF IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 7.0f }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.0f }, { "cutoff", 0.607f }, { "resonance", 0.55f }, { "envAmt", 0.2f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.6f }, { "release", 0.588f }, { "attenuator", 0.35f }, { "volume", 1.0f } },
          "osc2WaveOut:attIn attOut:filterCutoffIn" },

        { "Patch Bay", "Inverted Envelope", "The envelope through the attenuator turned negative closes the filter on every note instead of opening it: a reversed swell. Cables: + ENV OUT -> attenuator -> CUTOFF IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.77f }, { "resonance", 0.3f }, { "attack", 0.651f }, { "decay", 0.73f }, { "sustain", 0.4f }, { "release", 0.648f }, { "reverbMix", 0.25f }, { "attenuator", -0.3f }, { "volume", 0.717f } },
          "envPosOut:attIn attOut:filterCutoffIn" },

        { "Patch Bay", "Ring-ish FM Bass", "Linear FM from oscillator 1 at 32' into oscillator 2: growling metallic bass. Cable: OSC 1 WAVE OUT -> OSC 2 LIN FM IN.",
          { { "osc1Octave", 0 }, { "osc1Wave", 0 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.0f }, { "mixOsc1", 0.0f }, { "mixOsc2", 0.75f }, { "cutoff", 0.559f }, { "resonance", 0.35f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.588f }, { "sustain", 0.5f }, { "release", 0.459f }, { "volume", 0.791f } },
          "osc1WaveOut:osc2LinFmIn" },

        { "Patch Bay", "LFO from Oscillator", "Oscillator 2 is taken out of the mixer and used as a fast vibrato source for oscillator 1 (through the attenuator).",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 0 }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.0f }, { "cutoff", 0.742f }, { "resonance", 0.2f }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.9f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "attenuator", 0.02f }, { "volume", 0.884f } },
          "osc2WaveOut:attIn attOut:osc1PitchIn" },

        { "Patch Bay", "Clocked S/H Filter", "The S/H steps the cutoff on every LFO cycle: the classic random filter. Cables: MOD S/H OUT -> attenuator -> CUTOFF IN. Hold a key.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "cutoff", 0.559f }, { "resonance", 0.65f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.541f }, { "vcaMode", 1 }, { "reverbMix", 0.3f }, { "lfoRate", 0.482f }, { "attenuator", 0.5f }, { "volume", 0.725f } },
          "modShOut:attIn attOut:filterCutoffIn" },

        { "Patch Bay", "Envelope Re-trigger", "The LFO square re-triggers the envelope while a key is held: rhythmic plucks from one note. Cable: MOD WAVE OUT -> TRIGGER IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 3 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.45f }, { "cutoff", 0.486f }, { "resonance", 0.5f }, { "envAmt", 0.5f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.481f }, { "sustain", 0.0f }, { "release", 0.459f }, { "reverbMix", 0.3f }, { "lfoRate", 0.469f }, { "lfoWave", 3 }, { "volume", 1.0f } },
          "modWaveOut:envTriggerIn" },

        { "Patch Bay", "Filter as Oscillator", "The self-oscillating filter is the only voice, its frequency modulated by oscillator 1 for FM-like tones. Cables: OSC 1 WAVE OUT -> attenuator -> CUTOFF IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "mixOsc1", 0.0f }, { "cutoff", 0.466f }, { "resonance", 1.0f }, { "kbdTrack", 2 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.7f }, { "release", 0.588f }, { "reverbMix", 0.2f }, { "attenuator", 0.25f }, { "volume", 1.0f } },
          "osc1WaveOut:attIn attOut:filterCutoffIn" },

        { "Patch Bay", "S/H Pulse Width", "Random pulse width on every LFO step: a jittery, lively square. Cable: MOD S/H OUT -> OSC 1 PWM IN.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.07f }, { "mixOsc1", 0.65f }, { "mixOsc2", 0.4f }, { "cutoff", 0.742f }, { "resonance", 0.1f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.85f }, { "release", 0.588f }, { "reverbMix", 0.25f }, { "lfoRate", 0.453f }, { "volume", 0.665f } },
          "modShOut:osc1PwmIn" },

        { "Patch Bay", "Tape Warp", "The slow mod LFO wobbles the tape speed: the repeats bend in pitch like a warped tape. Cable: MOD WAVE OUT -> TAPE ECHO TIME CV.",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.742f }, { "resonance", 0.2f }, { "envAmt", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.67f }, { "sustain", 0.85f }, { "release", 0.567f }, { "reverbMix", 0.18f }, { "glide", 0.411f }, { "lfoPitchAmt", 0.091f }, { "volume", 0.648f }, { "tapOn", 1 }, { "tapHeads", 6 }, { "tapTime", 0.338f }, { "tapFeedback", 0.45f }, { "tapMix", 0.4f }, { "tapWow", 0.25f }, { "tapFlutter", 0.25f }, { "tapAge", 0.35f }, { "tapTreble", 0.5f }, { "tapSync", 0 }, { "tapBass", 0.5f }, { "lfoRate", 0.2f }, { "lfoWave", 0 } },
          "modWaveOut:tapCv" },

        { "Patch Bay", "Envelope Drive", "The envelope opens the saturator drive on every note: the attack bites, the tail cleans up. Cable: + ENV OUT -> SATURATOR DRIVE CV.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "osc2Freq", 0.08f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.6f }, { "cutoff", 0.387f }, { "resonance", 0.35f }, { "envAmt", 0.45f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.606f }, { "sustain", 0.45f }, { "release", 0.481f }, { "volume", 1.0f }, { "satOn", 1 }, { "satDrive", 0.35f }, { "satBias", 0.55f }, { "satBody", 0.3f }, { "satTone", 0.62f }, { "satOutput", 0.45f }, { "satMode", 0 }, { "satMix", 1.0f } },
          "envPosOut:satCv" },

        { "Patch Bay", "Random Chorus", "The S/H steps the chorus rate on every LFO cycle: a restless, unstable BBD. Cable: MOD S/H OUT -> CHORUS RATE CV.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 1 }, { "osc2Wave", 2 }, { "osc2Freq", 0.12f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.544f }, { "resonance", 0.15f }, { "envAmt", 0.25f }, { "attack", 0.695f }, { "decay", 0.777f }, { "sustain", 0.75f }, { "release", 0.777f }, { "reverbMix", 0.35f }, { "lfoRate", 0.177f }, { "lfoPwAmt", 0.4f }, { "modWheel", 1.0f }, { "volume", 0.466f }, { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.519f }, { "choDepth", 0.75f }, { "choMix", 0.5f }, { "choWidth", 1.0f }, { "choNoise", 0.25f }, { "choAge", 0.3f } },
          "modShOut:choCv" },

        { "Patch Bay", "Keyed Phaser", "The keyboard sets the phaser speed: low notes swirl slowly, high notes fast. Cable: KB OUT -> PHASER RATE CV.",
          { { "osc1Octave", 2 }, { "osc1Wave", 2 }, { "osc2Octave", 2 }, { "osc2Wave", 2 }, { "osc2Freq", 0.05f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.35f }, { "cutoff", 0.715f }, { "resonance", 0.15f }, { "envAmt", 0.15f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.648f }, { "sustain", 0.8f }, { "release", 0.567f }, { "reverbMix", 0.15f }, { "glide", 0.383f }, { "lfoRate", 0.438f }, { "lfoPitchAmt", 0.087f }, { "volume", 0.703f }, { "phaOn", 1 }, { "phaMode", 0 }, { "phaRate", 0.3f }, { "phaDepth", 0.8f }, { "phaFeedback", 0.2f }, { "phaCenter", 0.5f }, { "phaStereo", 0.3f }, { "phaMix", 0.5f } },
          "kbOut:phaCv" },

        { "Patch Bay", "Gated Echo Drive", "Gated Drone saturated by tape, the chopped notes thrown into a dotted-eighth echo; the gate also drives harder. Cables: MOD WAVE OUT -> VCA AMT IN, MOD WAVE OUT -> SATURATOR DRIVE CV.",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.5f }, { "mixOsc2", 0.5f }, { "cutoff", 0.607f }, { "attack", 0.0f }, { "decay", 0.507f }, { "sustain", 1.0f }, { "release", 0.481f }, { "vcaMode", 2 }, { "reverbMix", 0.25f }, { "lfoRate", 0.453f }, { "lfoWave", 3 }, { "volume", 0.661f }, { "satOn", 1 }, { "satMode", 1 }, { "satDrive", 0.5f }, { "satBias", 0.0f }, { "satBody", 0.45f }, { "satTone", 0.42f }, { "satMix", 1.0f }, { "satOutput", 0.5f }, { "tapOn", 1 }, { "tapSync", 3 }, { "tapTime", 0.5f }, { "tapFeedback", 0.4f }, { "tapMix", 0.35f }, { "tapFlutter", 0.15f }, { "tapAge", 0.2f }, { "tapHeads", 0 }, { "tapWow", 0.15f }, { "tapBass", 0.5f }, { "tapTreble", 0.55f } },
          "modWaveOut:vcaAmtIn modWaveOut:satCv" },

        // The Minimoog trick of feeding the output back into the mixer ("depth and welly"): the loop pushes the
        // fundamental up and rounds off the top. Grandmother has no VCA OUT jack, so the loop starts at FILTER OUT; the
        // mixer knob of that input sets the amount. (EURORACK OUT is not used: it comes after the decimator, and its
        // delay turns the loop into a comb filter.)
        { "Patch Bay", "Feedback Loop Bass", "The filter fed back into its own mixer, as on a Minimoog: a deeper, rounder bass. Cable: FILTER OUT -> MIXER NOISE IN (the Noise knob sets the feedback).",
          { { "osc1Octave", 1 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 2 }, { "osc2Freq", 0.06f }, { "mixOsc1", 0.6f }, { "mixOsc2", 0.5f }, { "mixNoise", 0.8f }, { "cutoff", 0.36f }, { "resonance", 0.3f }, { "envAmt", 0.4f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.55f }, { "sustain", 0.5f }, { "release", 0.45f }, { "volume", 0.75f } },
          "filterOut:mixNoiseIn" },

        { "Patch Bay", "Feedback Loop Lead", "Two saws with the filter fed back into the mixer: the fundamental swells, the top gets rounder, as the Minimoog loop. Cable: FILTER OUT -> MIXER NOISE IN (the Noise knob sets the feedback).",
          { { "osc1Octave", 2 }, { "osc1Wave", 1 }, { "osc2Octave", 1 }, { "osc2Wave", 1 }, { "osc2Freq", 0.1f }, { "mixOsc1", 0.55f }, { "mixOsc2", 0.45f }, { "mixNoise", 1.0f }, { "cutoff", 0.75f }, { "resonance", 0.25f }, { "envAmt", 0.2f }, { "kbdTrack", 1 }, { "attack", 0.0f }, { "decay", 0.6f }, { "sustain", 0.75f }, { "release", 0.45f }, { "reverbMix", 0.15f }, { "glide", 0.3f }, { "volume", 0.7f } },
          "filterOut:mixNoiseIn" },

        { "Patch Bay", "Double Loop Drone", "A 32' saw with the filter fed back into two mixer inputs at once: a huge, round drone that slowly breathes. Less resonance means more feedback; with Resonance near zero the loop runs away into a slow, heavy throb. Cables: FILTER OUT -> mult -> MIXER NOISE IN and MIXER OSC 2 IN.",
          { { "osc1Octave", 0 }, { "osc1Wave", 1 }, { "osc2Octave", 0 }, { "osc2Wave", 1 }, { "mixOsc1", 0.7f }, { "mixOsc2", 0.8f }, { "mixNoise", 0.8f }, { "cutoff", 0.42f }, { "resonance", 0.45f }, { "attack", 0.0f }, { "decay", 0.5f }, { "sustain", 1.0f }, { "release", 0.5f }, { "vcaMode", 2 }, { "reverbMix", 0.3f }, { "lfoRate", 0.05f }, { "lfoCutoffAmt", 0.2f }, { "modWheel", 1.0f }, { "volume", 0.78f } },
          "filterOut:mult1 mult2:mixNoiseIn mult3:mixOsc2In" },
    };
    return presets;
}
} // namespace babushka
