// Babushka — MIDI controller mapping: knobs of a hardware keyboard → plugin parameters.
//
// A binding is (MIDI channel or omni, CC number) → parameter. The map belongs to the user's controller, not to a
// project or a preset, so it is kept in one file per user (Babushka/MidiMap.xml) and shared by every instance.
//
// Setting up a map:
//   - Profiles: factory CC layouts of popular keyboards, each mapped onto the main sound parameters. While the map
//     is in "auto" mode (the user has not edited it) the connected MIDI inputs are scanned when the plugin loads and
//     the profile of a known keyboard is applied by itself; with no known keyboard the General MIDI sound
//     controllers (CC 71 … 77, 91) are used, which many keyboards send out of the box.
//   - Learn: pick a parameter, move a knob — the next CC that arrives is bound to it.
//   - Quick assign: move the keyboard's knobs one after another — each new CC goes to the next of the main
//     parameters (Cutoff, Resonance, Env Amt, Mod Rate, Attack, Decay, Sustain, Release, …). Works with any
//     keyboard whatever its CC numbers are.
//   - Soft takeover (on by default): a hardware knob that stands somewhere else than the parameter does not make it
//     jump; the parameter follows only once the knob has reached (or passed) its value. Switches and selectors
//     follow at once.
//
// Threads: the audio thread only reads a lock-free lookup table (handleController); everything else runs on the
// message thread. A learn request is passed to the audio thread through atomics and its result picked up by polling.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <array>
#include <atomic>
#include <vector>

namespace babushka
{
struct MidiBinding
{
    int channel = 0;              // 1 … 16, or 0 = any channel
    int cc = -1;
    juce::String paramId;
};

struct ControllerProfile
{
    const char* name;
    const char* shortName;                                // for the little display on the rear panel
    std::vector<const char*> deviceMatch;                 // lower-case pieces of the MIDI input name
    std::vector<std::pair<int, const char*>> ccs;         // CC number → parameter id
    const char* note;                                     // how the keyboard must be set up
};

// The main sound parameters, in the order Quick assign and the profiles hand them out.
inline const std::vector<const char*>& mainParameters()
{
    static const std::vector<const char*> ids = { "cutoff", "resonance", "envAmt", "lfoRate", "attack", "decay", "sustain", "release",
                                                  "mixOsc1", "mixOsc2", "mixNoise", "osc2Freq", "lfoPitchAmt", "lfoCutoffAmt", "glide", "reverbMix",
                                                  "volume" };
    return ids;
}

// Factory CC numbers as the keyboards ship (first program / preset, knobs in their plain "CC" mode).
inline const std::vector<ControllerProfile>& controllerProfiles()
{
    static const std::vector<ControllerProfile> profiles = {
        { "AKAI MPK Mini mk3", "MPK MINI 3", { "mpk mini 3", "mpk mini mk3", "mpkmini3", "mpk mini iii" },
          { { 70, "cutoff" }, { 71, "resonance" }, { 72, "envAmt" }, { 73, "lfoRate" }, { 74, "attack" }, { 75, "decay" }, { 76, "sustain" }, { 77, "release" } },
          "Program 1 (PGM: MPC) of the MPK Mini: knobs K1 … K8 send CC 70 … 77." },
        { "M-Audio Oxygen 25 / 49 / 61 (MK V, Pro)", "OXYGEN MK V", { "oxygen" },
          { { 44, "cutoff" }, { 45, "resonance" }, { 46, "envAmt" }, { 47, "lfoRate" }, { 62, "attack" }, { 63, "decay" }, { 75, "sustain" }, { 76, "release" } },
          "Knobs in Preset mode (DAW button off), factory Preset 01: knobs 1 … 8 send CC 44 … 47, 62, 63, 75, 76. In DAW mode the knobs send mixer messages; older Oxygen models use other CCs (use Quick assign)." },
        { "Arturia MiniLab 3", "MINILAB 3", { "minilab3", "minilab 3" },
          { { 74, "cutoff" }, { 71, "resonance" }, { 76, "envAmt" }, { 77, "lfoRate" }, { 93, "attack" }, { 18, "decay" }, { 19, "sustain" }, { 16, "release" },
            { 82, "mixOsc1" }, { 83, "mixOsc2" }, { 85, "mixNoise" }, { 17, "reverbMix" } },
          "Factory user mode: knobs 1 … 8 and the four faders." },
        { "Arturia MiniLab mkII", "MINILAB MKII", { "minilab mkii", "minilab mk2", "minilab mk ii", "minilabmkii" },
          { { 74, "cutoff" }, { 71, "resonance" }, { 76, "envAmt" }, { 77, "lfoRate" }, { 93, "mixOsc2" }, { 73, "attack" }, { 75, "decay" },
            { 18, "lfoCutoffAmt" }, { 19, "lfoPitchAmt" }, { 16, "osc2Freq" }, { 17, "glide" }, { 91, "reverbMix" }, { 79, "sustain" }, { 72, "release" } },
          "Factory preset 1 (Analog Lab). Knobs 1 and 9 are kept by the MiniLab for browsing." },
        { "Novation Launchkey (MK3)", "LAUNCHKEY", { "launchkey" },
          { { 21, "cutoff" }, { 22, "resonance" }, { 23, "envAmt" }, { 24, "lfoRate" }, { 25, "attack" }, { 26, "decay" }, { 27, "sustain" }, { 28, "release" } },
          "Pots in Custom mode (Shift + the pot mode pad): pots 1 … 8 send CC 21 … 28." },
        { "Korg nanoKONTROL2", "NANOKONTROL2", { "nanokontrol2" },
          { { 16, "cutoff" }, { 17, "resonance" }, { 18, "envAmt" }, { 19, "lfoRate" }, { 20, "attack" }, { 21, "decay" }, { 22, "sustain" }, { 23, "release" },
            { 0, "mixOsc1" }, { 1, "mixOsc2" }, { 2, "mixNoise" }, { 3, "osc2Freq" }, { 4, "lfoPitchAmt" }, { 5, "lfoCutoffAmt" }, { 6, "glide" }, { 7, "reverbMix" } },
          "Factory scene: knobs CC 16 … 23, faders CC 0 … 7." },
        { "Generic (General MIDI sound controllers)", "GENERIC", {},
          { { 74, "cutoff" }, { 71, "resonance" }, { 73, "attack" }, { 75, "decay" }, { 72, "release" }, { 76, "lfoRate" }, { 77, "lfoPitchAmt" },
            { 91, "reverbMix" }, { 5, "glide" } },
          "Standard CC numbers many keyboards and DAWs use for brightness (74), resonance (71), attack (73), decay (75), release (72), vibrato (76/77), reverb (91) and portamento (5)." },
    };
    return profiles;
}

inline int genericProfileIndex() { return (int) controllerProfiles().size() - 1; }

class MidiMapper
{
public:
    explicit MidiMapper (juce::AudioProcessor& processor)
    {
        for (auto* p : processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) params.push_back (rp);
        for (auto& t : table) t.store (-1);
    }

    // ---- audio thread ----------------------------------------------------------------------------------------------
    // Returns true when the controller was used by the map (or by a learn in progress) and must not be handled further.
    bool handleController (int channel, int cc, int value) noexcept
    {
        if (cc < 0 || cc > 127) return false;
        lastChannel.store (channel); lastCc.store (cc); lastValue.store (value); ++activity;
        if (learning.load() && learnable (cc))
        {
            learnedEvent.store ((channel << 8) | cc);
            return true;
        }
        int slot = channel * 128 + cc;
        int idx = table[(size_t) slot].load();
        if (idx < 0) { slot = cc; idx = table[(size_t) slot].load(); }     // omni
        if (idx < 0 || idx >= (int) params.size()) return false;
        auto* p = params[(size_t) idx];
        const float v = (float) value / 127.0f, cur = p->getValue();
        if (softTakeover.load() && ! p->isDiscrete())
        {
            auto& st = pickup[(size_t) slot];
            const int gen = generation.load();
            if (st.generation != gen) st = { gen, false, -1.0f, -1.0f };
            constexpr float near = 1.5f / 127.0f;
            const bool following = st.picked && std::abs (cur - st.lastSet) < near;      // nobody else moved it meanwhile
            if (! following)
            {
                const bool reached = std::abs (v - cur) <= near || (st.lastIn >= 0.0f && (st.lastIn - cur) * (v - cur) <= 0.0f);
                st.lastIn = v;
                if (! reached) { st.picked = false; return true; }                         // the knob has not caught up yet
                st.picked = true;
            }
            st.lastIn = v;
        }
        if (std::abs (cur - v) > 1.0e-6f) p->setValueNotifyingHost (v);
        pickup[(size_t) slot].lastSet = p->getValue();
        return true;
    }

    // ---- message thread ------------------------------------------------------------------------------------------
    const std::vector<MidiBinding>& getBindings() const { return bindings; }
    juce::String getProfileName() const { return profileName; }
    juce::String getShortName() const
    {
        for (auto& p : controllerProfiles()) if (profileName == p.name) return p.shortName;
        return bindings.empty() ? "NO MAP" : "CUSTOM";
    }
    bool isAutoMode() const { return autoMode; }
    bool getSoftTakeover() const { return softTakeover.load(); }
    void setSoftTakeover (bool on) { softTakeover.store (on); ++generation; save(); }
    int numParameters() const { return (int) params.size(); }

    juce::String parameterName (const juce::String& id) const
    {
        for (auto* p : params) if (p->paramID == id) return p->getName (40);
        return id;
    }

    // Binds a CC to a parameter. A CC drives one parameter and a parameter listens to one CC, so older bindings of
    // either are replaced.
    void bind (int channel, int cc, const juce::String& id)
    {
        removeIf ([&] (const MidiBinding& b) { return b.paramId == id || (b.cc == cc && (b.channel == channel || b.channel == 0 || channel == 0)); });
        bindings.push_back ({ channel, cc, id });
        edited();
    }
    void unbindParameter (const juce::String& id) { removeIf ([&] (const MidiBinding& b) { return b.paramId == id; }); edited(); }
    void removeBinding (int index)
    {
        if (index >= 0 && index < (int) bindings.size()) { bindings.erase (bindings.begin() + index); edited(); }
    }
    void clear() { bindings.clear(); edited(); }
    int ccFor (const juce::String& id) const { for (auto& b : bindings) if (b.paramId == id) return b.cc; return -1; }

    void applyProfile (int index, bool byUser = true)
    {
        const auto& all = controllerProfiles();
        if (index < 0 || index >= (int) all.size()) return;
        bindings.clear();
        for (auto& [cc, id] : all[(size_t) index].ccs)
            if (findParam (id) >= 0) bindings.push_back ({ 0, cc, id });
        profileName = all[(size_t) index].name;
        if (byUser) autoMode = false;
        rebuild();
        save();
    }

    // Names of the MIDI inputs the system knows about, and the profile of the first one we recognise (−1 = none).
    static juce::StringArray connectedInputs()
    {
        juce::StringArray names;
        for (auto& d : juce::MidiInput::getAvailableDevices()) names.add (d.name);
        return names;
    }
    static int profileForDevices (const juce::StringArray& names, juce::String* deviceName = nullptr)
    {
        const auto& all = controllerProfiles();
        for (auto& n : names)
        {
            const auto lower = n.toLowerCase();
            for (int i = 0; i < (int) all.size(); ++i)
                for (auto* m : all[(size_t) i].deviceMatch)
                    if (lower.contains (m)) { if (deviceName) *deviceName = n; return i; }
        }
        return -1;
    }

    // Applies the profile of the connected keyboard (or the generic one). Returns the detected device name, if any.
    juce::String detectAndApply (bool byUser)
    {
        juce::String device;
        const int idx = profileForDevices (connectedInputs(), &device);
        applyProfile (idx >= 0 ? idx : genericProfileIndex(), byUser);
        if (! byUser) { autoMode = true; save(); }
        return device;
    }

    // Loads the user's map; on the first run, or while the map is still automatic, follows the connected keyboard.
    void initialise()
    {
        if (! load() || autoMode) detectAndApply (false);
    }

    // Learn: the next CC that arrives is bound to `id`. Poll pollLearn() from a timer.
    void startLearn (const juce::String& id) { learnTarget = id; learnedEvent.store (-1); learning.store (true); }
    void stopLearn() { learning.store (false); learnTarget = {}; learnedEvent.store (-1); }
    bool isLearning() const { return learning.load(); }
    juce::String getLearnTarget() const { return learnTarget; }

    // Quick assign: every new CC goes to the next of mainParameters().
    void startQuickAssign() { bindings.clear(); quickIndex = 0; quickSeen.clear(); learnTarget = mainParameters()[0]; learnedEvent.store (-1); learning.store (true); quick = true; edited(); }
    bool isQuickAssign() const { return quick && learning.load(); }
    int quickAssignCount() const { return quickIndex; }

    // Returns true when a learn produced a binding (the learn ends; quick assign moves on to the next parameter).
    bool pollLearn()
    {
        const int ev = learnedEvent.exchange (-1);
        if (ev < 0 || ! learning.load()) return false;
        const int channel = ev >> 8, cc = ev & 127;
        if (quick)
        {
            if (quickSeen.contains (cc)) return false;            // the same knob turned again
            quickSeen.add (cc);
            bind (0, cc, learnTarget);
            if (++quickIndex >= (int) mainParameters().size()) { quick = false; stopLearn(); }
            else learnTarget = mainParameters()[(size_t) quickIndex];
            return true;
        }
        bind (channel, cc, learnTarget);
        stopLearn();
        return true;
    }
    void endQuickAssign() { quick = false; stopLearn(); }

    struct Activity { int channel, cc, value; uint32_t counter; };
    Activity lastActivity() const { return { lastChannel.load(), lastCc.load(), lastValue.load(), activity.load() }; }

    // A copy of the bindings to come back to when a learn session is cancelled.
    struct Snapshot { std::vector<MidiBinding> bindings; juce::String profile; bool autoMode; };
    Snapshot snapshot() const { return { bindings, profileName, autoMode }; }
    void restore (const Snapshot& s) { bindings = s.bindings; profileName = s.profile; autoMode = s.autoMode; rebuild(); save(); }

    // ---- storage -------------------------------------------------------------------------------------------------
    static juce::File defaultFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Babushka").getChildFile ("MidiMap.xml");
    }
    void setFile (const juce::File& f) { file = f; }        // tests

    bool load()
    {
        auto xml = juce::parseXML (file);
        if (xml == nullptr || ! xml->hasTagName ("MIDIMAP")) return false;
        bindings.clear();
        profileName = xml->getStringAttribute ("profile");
        autoMode = xml->getBoolAttribute ("auto", false);
        softTakeover.store (xml->getBoolAttribute ("softTakeover", true));
        for (auto* e : xml->getChildWithTagNameIterator ("BIND"))
        {
            const int cc = e->getIntAttribute ("cc", -1), ch = e->getIntAttribute ("channel", 0);
            const auto id = e->getStringAttribute ("param");
            if (cc >= 0 && cc < 128 && ch >= 0 && ch <= 16 && findParam (id) >= 0) bindings.push_back ({ ch, cc, id });
        }
        rebuild();
        return true;
    }

    bool save() const
    {
        juce::XmlElement xml ("MIDIMAP");
        xml.setAttribute ("profile", profileName);
        xml.setAttribute ("auto", autoMode);
        xml.setAttribute ("softTakeover", softTakeover.load());
        for (auto& b : bindings)
        {
            auto* e = xml.createNewChildElement ("BIND");
            e->setAttribute ("channel", b.channel); e->setAttribute ("cc", b.cc); e->setAttribute ("param", b.paramId);
        }
        file.getParentDirectory().createDirectory();
        return xml.writeTo (file);
    }

private:
    std::vector<juce::RangedAudioParameter*> params;
    std::array<std::atomic<int>, 17 * 128> table;          // [channel * 128 + cc] → index into params; channel 0 = omni
    std::vector<MidiBinding> bindings;
    juce::String profileName, learnTarget;
    bool autoMode = true, quick = false;
    int quickIndex = 0;
    juce::Array<int> quickSeen;
    std::atomic<bool> learning { false };
    std::atomic<int> learnedEvent { -1 };
    std::atomic<int> lastChannel { 0 }, lastCc { -1 }, lastValue { 0 };
    std::atomic<uint32_t> activity { 0 };
    std::atomic<bool> softTakeover { true };
    std::atomic<int> generation { 0 };                     // bumped when the map changes: pick-up starts again
    struct Pickup { int generation = -1; bool picked = false; float lastIn = -1.0f, lastSet = -1.0f; };
    std::array<Pickup, 17 * 128> pickup {};                // audio thread only
    juce::File file { defaultFile() };

    // Sustain pedal, data entry of (N)RPN and the channel-mode messages are never learnt.
    static bool learnable (int cc) noexcept { return cc != 64 && (cc < 98 || cc > 101) && cc < 120; }

    int findParam (const juce::String& id) const
    {
        for (size_t i = 0; i < params.size(); ++i) if (params[i]->paramID == id) return (int) i;
        return -1;
    }
    template <typename Pred> void removeIf (Pred pred) { bindings.erase (std::remove_if (bindings.begin(), bindings.end(), pred), bindings.end()); }

    void edited()
    {
        autoMode = false;
        profileName = "Custom";
        rebuild();
        save();
    }

    void rebuild()
    {
        std::array<int, 17 * 128> next;
        next.fill (-1);
        for (auto& b : bindings)
            if (b.cc >= 0 && b.cc < 128 && b.channel >= 0 && b.channel <= 16)
                next[(size_t) (b.channel * 128 + b.cc)] = findParam (b.paramId);
        for (size_t i = 0; i < next.size(); ++i) table[i].store (next[i]);
        ++generation;
    }
};
} // namespace babushka
