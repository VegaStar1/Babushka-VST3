// Babushka — character tags of the factory presets (the CHARACTER filter of the preset browser).
//
// Tags are written by hand from how each preset sounds; a preset has one to three of them. User presets keep their
// own tags in the preset file (chosen when saving).
#pragma once

#include <string_view>

namespace babushka
{
// The tags the browser offers, in the order they are shown.
inline constexpr const char* kCharacterTags[] = { "DARK", "BRIGHT", "WARM", "FAT", "AGGRESSIVE", "PLUCKY", "EVOLVING",
                                                  "RESONANT", "NOISY", "CLEAN" };

// Space-separated tags of a factory preset, "" for an unknown name.
inline const char* factoryPresetTags (std::string_view name)
{
    struct Entry { const char* name; const char* tags; };
    static constexpr Entry table[] = {
        // Basic
        { "Init", "CLEAN" }, { "Two Saws", "WARM FAT" }, { "Sine Oscillator", "CLEAN" }, { "Mixer Overdrive", "AGGRESSIVE NOISY FAT" },
        { "Ladder Sweep", "RESONANT EVOLVING" }, { "Spring Tank", "PLUCKY BRIGHT" },
        // Legends
        { "Munich '77", "PLUCKY WARM" }, { "Midnight Chase", "DARK EVOLVING" }, { "Dark Pursuit", "DARK AGGRESSIVE FAT" },
        { "Black Glory Lead", "AGGRESSIVE BRIGHT" }, { "Upside Down Arp", "EVOLVING WARM" }, { "Hawkins Pulse", "DARK EVOLVING" },
        { "Lucky Lead", "RESONANT WARM" }, { "Funky Worm", "BRIGHT" }, { "G-Funk Whine", "CLEAN BRIGHT" },
        { "Flash Light Bass", "FAT WARM" }, { "Chameleon Bass", "RESONANT FAT" }, { "French Touch Lead", "AGGRESSIVE BRIGHT" },
        { "Grid Runner", "BRIGHT PLUCKY" }, { "On The Run", "EVOLVING NOISY" }, { "Oxygen Lead", "WARM EVOLVING" },
        { "Switched-On", "BRIGHT CLEAN" }, { "Nightdrive", "DARK FAT" }, { "BFG Bass", "AGGRESSIVE NOISY DARK" },
        { "Carpenter Pulse", "DARK EVOLVING" }, { "Blade Brass", "WARM FAT EVOLVING" }, { "Sandworm Drone", "DARK EVOLVING NOISY" },
        { "Braam", "FAT AGGRESSIVE DARK" }, { "Thesis On Sound", "WARM" }, { "Chicago Acid", "RESONANT AGGRESSIVE" },
        { "Tape Memory", "WARM DARK EVOLVING" },
        // Cinematic
        { "Braam Titan", "FAT AGGRESSIVE DARK" }, { "Braam Distorted", "AGGRESSIVE NOISY RESONANT" }, { "Braam Slow Swell", "FAT EVOLVING DARK" },
        { "Braam Hit", "FAT AGGRESSIVE" }, { "Fifth Braam", "FAT BRIGHT" }, { "Dissonant Braam", "AGGRESSIVE DARK" },
        { "Horn of Doom", "DARK RESONANT FAT" }, { "War Horn", "AGGRESSIVE FAT" }, { "Titan Brass", "FAT WARM EVOLVING" },
        { "Epic Brass Stab", "PLUCKY BRIGHT FAT" }, { "Dark Brass Swell", "DARK EVOLVING" }, { "Trailer Rise", "EVOLVING BRIGHT" },
        { "Low String Ostinato", "DARK PLUCKY" }, { "Chase Pulse", "DARK EVOLVING" },
        // Bass
        { "Classic Bass", "WARM FAT" }, { "Sub Square", "DARK CLEAN" }, { "Round Moog Bass", "WARM FAT" }, { "Pluck Bass", "PLUCKY" },
        { "Funk Rubber", "RESONANT PLUCKY" }, { "Overdriven Saw Bass", "AGGRESSIVE FAT" }, { "Reese", "DARK FAT EVOLVING" },
        { "Hollow Pulse Bass", "BRIGHT CLEAN" }, { "Wobble", "EVOLVING FAT" }, { "Acid Squelch", "RESONANT AGGRESSIVE" },
        { "Deep Triangle", "DARK CLEAN" }, { "Synced Growl", "AGGRESSIVE EVOLVING" }, { "Mono Synth Pop", "BRIGHT PLUCKY" },
        { "Fat Taurus", "FAT DARK WARM" }, { "Noise Bass", "NOISY AGGRESSIVE RESONANT" }, { "Bouncy Octaves", "PLUCKY WARM" },
        { "Soft Sub Pad Bass", "DARK WARM EVOLVING" }, { "Resonant Thump", "RESONANT PLUCKY" }, { "Stab Bass", "PLUCKY BRIGHT" },
        { "Velocity Bass", "WARM PLUCKY" },
        // Lead
        { "Sync Lead", "BRIGHT AGGRESSIVE" }, { "Fat Mono Lead", "FAT WARM" }, { "Square Lead", "WARM CLEAN" }, { "Whistle", "CLEAN BRIGHT" },
        { "Singing Resonance", "RESONANT BRIGHT" }, { "Screaming Sync", "AGGRESSIVE BRIGHT" }, { "Soft Flute", "WARM CLEAN NOISY" },
        { "Nasal Pulse Lead", "BRIGHT RESONANT" }, { "PWM Lead", "WARM EVOLVING" }, { "Fifths Lead", "FAT BRIGHT" },
        { "Glide Solo", "WARM" }, { "Brassy Lead", "WARM FAT" }, { "Thin Sync Whine", "BRIGHT" }, { "Chiptune Square", "BRIGHT CLEAN" },
        { "Dirty Lead", "AGGRESSIVE RESONANT" }, { "Wah Lead", "RESONANT EVOLVING" }, { "Expressive Velocity Lead", "BRIGHT WARM" },
        // Keys & Plucks
        { "Spring Pluck", "PLUCKY BRIGHT" }, { "Analog Clav", "PLUCKY RESONANT BRIGHT" }, { "Mallet", "PLUCKY CLEAN" },
        { "Harpsichord", "PLUCKY BRIGHT" }, { "Kalimba", "PLUCKY CLEAN" }, { "Plucked String", "PLUCKY WARM" },
        { "Organ Drawbar", "CLEAN WARM" }, { "Glass Bell FM", "BRIGHT CLEAN" }, { "Dub Chord", "PLUCKY DARK" }, { "Music Box", "PLUCKY BRIGHT CLEAN" },
        // Pads & Drones
        { "Warm Pad", "WARM EVOLVING" }, { "Slow Strings", "WARM FAT EVOLVING" }, { "Dark Drone", "DARK EVOLVING FAT" },
        { "Breathing Pad", "EVOLVING WARM" }, { "Choir Vowel", "RESONANT NOISY EVOLVING" }, { "Sub Drone", "DARK CLEAN EVOLVING" },
        { "Shimmer Swell", "BRIGHT EVOLVING" }, { "Ocean Pad", "NOISY EVOLVING" }, { "Organ Drone", "WARM EVOLVING" },
        { "Tension Rise", "EVOLVING DARK" }, { "Cosmic Drift", "EVOLVING CLEAN" },
        // Brass & Strings
        { "Brass Swell", "WARM FAT RESONANT" }, { "Brass Stab", "PLUCKY BRIGHT" }, { "French Horn", "WARM DARK" }, { "Solo Cello", "WARM EVOLVING" },
        { "Trumpet", "BRIGHT" },
        // Arpeggios
        { "Arp Minor Pluck", "PLUCKY DARK" }, { "Trance Gate Arp", "BRIGHT PLUCKY" }, { "Random Bleeps", "BRIGHT CLEAN" },
        { "Slow Eighths", "WARM" }, { "Synthwave Arp", "WARM BRIGHT" }, { "Resonant Arp", "RESONANT AGGRESSIVE" }, { "Bass Arp", "DARK FAT" },
        { "Pluck Cascade", "PLUCKY BRIGHT" }, { "Sync Arp", "BRIGHT EVOLVING" }, { "Quarter Pulse", "WARM EVOLVING" }, { "Chip Arp", "BRIGHT CLEAN" },
        // Sequences
        { "Seq Acid Line", "RESONANT AGGRESSIVE" }, { "Berlin School", "EVOLVING DARK" }, { "Octave Disco", "PLUCKY BRIGHT" },
        { "Techno Rumble", "DARK FAT" }, { "Electro Funk", "PLUCKY RESONANT" }, { "Ambient Steps", "CLEAN WARM" }, { "Minimal Pulse", "DARK EVOLVING" },
        { "Dark Pulse Seq", "DARK" }, { "Gallop", "FAT AGGRESSIVE" }, { "Bleep Melody", "BRIGHT CLEAN" }, { "Funk Seq", "WARM PLUCKY" },
        { "Flying Chirps", "RESONANT BRIGHT PLUCKY" }, { "Flying Bass", "FAT PLUCKY" },
        // Drums & Percussion
        { "Analog Kick", "DARK PLUCKY" }, { "Boom Kick", "DARK FAT" }, { "Snare", "NOISY PLUCKY" }, { "Closed Hat", "NOISY BRIGHT PLUCKY" },
        { "Open Hat", "NOISY BRIGHT" }, { "Clap", "NOISY PLUCKY" }, { "Tom", "PLUCKY RESONANT" }, { "Cowbell", "BRIGHT RESONANT" },
        { "Rimshot", "PLUCKY BRIGHT" }, { "Zap", "RESONANT BRIGHT" }, { "Shaker", "NOISY BRIGHT" },
        // FX
        { "Wind", "NOISY EVOLVING" }, { "Surf", "NOISY EVOLVING" }, { "Droid Chatter", "BRIGHT EVOLVING" }, { "Siren", "BRIGHT" },
        { "UFO", "BRIGHT EVOLVING" }, { "Laser Gun", "BRIGHT AGGRESSIVE" }, { "Rising Alarm", "BRIGHT EVOLVING" },
        { "Geiger Counter", "NOISY PLUCKY" }, { "Thunder", "DARK NOISY" }, { "Spring Crash", "NOISY AGGRESSIVE" },
        { "Riser", "EVOLVING NOISY BRIGHT" }, { "Downlifter", "EVOLVING DARK" }, { "Noise Sweep Up", "NOISY RESONANT EVOLVING" },
        { "Noise Sweep Down", "NOISY EVOLVING" }, { "Sub Drop", "DARK CLEAN" }, { "Impact Boom", "DARK FAT NOISY" },
        { "Reverse Swell", "EVOLVING WARM" }, { "Laser Riser", "BRIGHT AGGRESSIVE EVOLVING" }, { "Glitch Stutter", "AGGRESSIVE BRIGHT" },
        { "Radio Static", "NOISY RESONANT" }, { "Heartbeat", "DARK PLUCKY" }, { "Bubbles", "RESONANT PLUCKY" },
        // Patch Bay
        { "Gated Drone", "EVOLVING FAT" }, { "Keytracked Wobble", "EVOLVING" }, { "Split Bass and Thin", "BRIGHT FAT" },
        { "Random Notes", "BRIGHT EVOLVING" }, { "Envelope PWM", "EVOLVING WARM" }, { "Audio-Rate Filter FM", "AGGRESSIVE RESONANT" },
        { "Inverted Envelope", "EVOLVING DARK" }, { "Ring-ish FM Bass", "AGGRESSIVE DARK" }, { "LFO from Oscillator", "BRIGHT EVOLVING" },
        { "Clocked S/H Filter", "RESONANT EVOLVING" }, { "Envelope Re-trigger", "PLUCKY EVOLVING" }, { "Filter as Oscillator", "CLEAN RESONANT" },
        { "S/H Pulse Width", "EVOLVING BRIGHT" },
        // with the pedals
        { "Two Saws Chorus", "WARM FAT" },
        { "Tube Saws", "AGGRESSIVE WARM" },
        { "Phased Ladder", "RESONANT EVOLVING" },
        { "Echo Tank", "PLUCKY BRIGHT" },
        { "Tape Memory Echo", "DARK EVOLVING" },
        { "Nightdrive Chorus", "DARK FAT" },
        { "Oxygen Phase", "WARM EVOLVING" },
        { "Munich Echo", "PLUCKY WARM" },
        { "Black Glory Drive", "AGGRESSIVE BRIGHT" },
        { "Lucky Lead Echo", "RESONANT WARM" },
        { "Braam Iron Titan", "FAT AGGRESSIVE DARK" },
        { "Horn of Doom Echo", "DARK FAT" },
        { "Titan Brass Ensemble", "WARM FAT" },
        { "Chase Pulse Echo", "DARK EVOLVING" },
        { "Trailer Rise Phased", "EVOLVING AGGRESSIVE" },
        { "Tube Bass", "WARM FAT" },
        { "Tape Sub", "FAT DARK" },
        { "Iron Reese", "FAT DARK EVOLVING" },
        { "Acid Drive", "RESONANT AGGRESSIVE" },
        { "Phased Funk", "RESONANT EVOLVING" },
        { "Dub Bass", "WARM DARK" },
        { "Tube Sync Lead", "AGGRESSIVE BRIGHT" },
        { "Chorus Mono Lead", "WARM FAT" },
        { "Space Whistle", "CLEAN EVOLVING" },
        { "Phased Square", "WARM EVOLVING" },
        { "Dirty Echo Lead", "AGGRESSIVE NOISY" },
        { "Glide Solo Delay", "WARM EVOLVING" },
        { "Phased Clav", "PLUCKY BRIGHT" },
        { "Dub Chord Echo", "DARK EVOLVING" },
        { "Ensemble Harpsichord", "BRIGHT PLUCKY" },
        { "Music Box Tape", "PLUCKY EVOLVING" },
        { "Mallet Echoes", "PLUCKY CLEAN" },
        { "Polysynth Pad", "WARM EVOLVING" },
        { "Ensemble Strings", "WARM EVOLVING" },
        { "Phased Drone", "DARK EVOLVING" },
        { "Tape Drift Pad", "EVOLVING NOISY" },
        { "Shimmer Echo", "BRIGHT EVOLVING" },
        { "Rotary Organ Drone", "WARM EVOLVING" },
        { "Ensemble Brass", "WARM FAT" },
        { "Tube Brass Stab", "AGGRESSIVE BRIGHT" },
        { "Cello Hall", "WARM EVOLVING" },
        { "Horn Echo", "WARM" },
        { "Synthwave Arp Echo", "BRIGHT EVOLVING" },
        { "Jet Minor Pluck", "PLUCKY RESONANT" },
        { "Dub Arp", "WARM EVOLVING" },
        { "Acid Arp Drive", "RESONANT AGGRESSIVE" },
        { "Cassette Arp", "DARK EVOLVING" },
        { "Berlin Echoes", "EVOLVING DARK" },
        { "Acid Line Drive", "RESONANT AGGRESSIVE" },
        { "Dub Techno Rumble", "DARK EVOLVING" },
        { "Ambient Tape Steps", "WARM EVOLVING" },
        { "Disco Phase", "FAT EVOLVING" },
        { "Iron Kick", "FAT DARK" },
        { "Dub Snare", "NOISY EVOLVING" },
        { "Jet Hat", "NOISY BRIGHT" },
        { "Tape Clap", "NOISY AGGRESSIVE" },
        { "Runaway Droid", "NOISY EVOLVING" },
        { "Jet Wind", "NOISY EVOLVING" },
        { "Space Siren", "EVOLVING BRIGHT" },
        { "Dub Siren", "BRIGHT EVOLVING" },
        { "UFO Phaser", "EVOLVING RESONANT" },
        { "Crushed Impact", "AGGRESSIVE DARK" },
        { "Tape Warp", "WARM EVOLVING" },
        { "Envelope Drive", "AGGRESSIVE WARM" },
        { "Random Chorus", "WARM EVOLVING" },
        { "Keyed Phaser", "WARM EVOLVING" },
        { "Gated Echo Drive", "AGGRESSIVE EVOLVING" },
        { "Feedback Loop Bass", "FAT DARK" },
        { "Feedback Loop Lead", "FAT WARM" },
        { "Double Loop Drone", "FAT DARK" },
    };
    for (const auto& e : table) if (name == e.name) return e.tags;
    return "";
}
} // namespace babushka
