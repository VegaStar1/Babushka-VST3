// Babushka — preset library: factory presets as files, user presets in folders, tags, import / export.
#include "PluginProcessor.h"

namespace
{
juce::StringArray splitTags (const juce::String& text)
{
    auto t = juce::StringArray::fromTokens (text.toUpperCase(), " ,", "");
    t.removeEmptyStrings();
    t.removeDuplicates (false);
    return t;
}

// A folder path under User from user input: each part made a legal file name, "." / ".." and empty parts dropped.
juce::String cleanFolder (const juce::String& folder)
{
    juce::StringArray parts;
    for (auto part : juce::StringArray::fromTokens (folder.replaceCharacter ('\\', '/'), "/", ""))
    {
        part = juce::File::createLegalFileName (part.trim());
        if (part.isNotEmpty() && part != "." && part != "..") parts.add (part);
    }
    return parts.joinIntoString ("/");
}

constexpr const char* kFactoryVersion = "factory-presets-1";   // bump when the factory set changes
}

// ---- factory presets ------------------------------------------------------------------------------------------------
void BabushkaProcessor::applyFactoryPreset (int index)
{
    const auto& all = babushka::factoryPresets();
    if (index < 0 || index >= (int) all.size()) return;
    const auto& pr = all[(size_t) index];

    // Back to defaults (except the global settings), then the preset's own values.
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! keptOnLoad (rp->paramID))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, value] : pr.values)
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
        {
            if (! keptOnLoad (rp->paramID)) rp->setValueNotifyingHost (rp->convertTo0to1 (value));
        }
        else jassertfalse;   // unknown parameter id in a factory preset

    {
        const juce::SpinLock::ScopedLockType lock (patchLock);
        patchState = babushka::dsp::PatchState::fromString (pr.patch);
    }
    patchDirty.store (true);
    ++patchVersion;

    if (pr.seq1[0] != 0 || pr.seq2[0] != 0 || pr.seq3[0] != 0)
    {
        std::array<babushka::dsp::Sequence, 3> seqs;
        { const juce::SpinLock::ScopedLockType lock (seqLock); seqs = pendingSequences; }
        const char* texts[3] = { pr.seq1, pr.seq2, pr.seq3 };
        for (int i = 0; i < 3; ++i) if (texts[i][0] != 0) seqs[(size_t) i] = sequenceFromString (texts[i]);
        setSequences (seqs);
    }
    currentPresetName = pr.name;
    currentPresetFile = juce::File();
    soundModified = randomSound = false;
    addHistoryStep ("Preset: " + currentPresetName);
}

// The state a factory preset sets, built without touching the live sound: every parameter at its default, then the
// preset's values; its cables, sequences, name and tags.
std::unique_ptr<juce::XmlElement> BabushkaProcessor::factoryPresetXml (int index) const
{
    const auto& all = babushka::factoryPresets();
    if (index < 0 || index >= (int) all.size()) return nullptr;
    const auto& pr = all[(size_t) index];
    auto state = const_cast<juce::AudioProcessorValueTreeState&> (apvts).copyState();
    auto setValue = [&state] (const juce::String& id, float v) {
        auto child = state.getChildWithProperty ("id", id);
        if (child.isValid()) child.setProperty ("value", v, nullptr); };
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            setValue (rp->paramID, rp->convertFrom0to1 (rp->getDefaultValue()));
    for (auto& [id, value] : pr.values) setValue (id, value);
    state.setProperty ("seq1", pr.seq1, nullptr);
    state.setProperty ("seq2", pr.seq2, nullptr);
    state.setProperty ("seq3", pr.seq3, nullptr);
    state.setProperty ("patch", pr.patch, nullptr);
    state.setProperty ("presetName", pr.name, nullptr);
    state.setProperty ("presetModified", false, nullptr);
    state.setProperty ("presetRandom", false, nullptr);
    state.setProperty ("presetFile", "", nullptr);
    state.setProperty ("category", pr.category, nullptr);
    state.setProperty ("tags", babushka::factoryPresetTags (pr.name), nullptr);
    state.setProperty ("description", pr.description, nullptr);
    return state.createXml();
}

int BabushkaProcessor::writeFactoryPresetFiles (const juce::File& dir, bool overwrite) const
{
    int written = 0;
    const auto& all = babushka::factoryPresets();
    for (int i = 0; i < (int) all.size(); ++i)
    {
        const auto& pr = all[(size_t) i];
        auto file = dir.getChildFile (juce::File::createLegalFileName (pr.category))
                       .getChildFile (juce::File::createLegalFileName (pr.name) + ".babushka");
        if (file.existsAsFile() && ! overwrite) continue;
        if (auto xml = factoryPresetXml (i))
            if (file.getParentDirectory().createDirectory().wasOk() && xml->writeTo (file)) ++written;
    }
    return written;
}

// ---- the library folder ---------------------------------------------------------------------------------------------
juce::File BabushkaProcessor::getPresetRoot() const
{
    if (presetRoot != juce::File()) return presetRoot;
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Babushka").getChildFile ("Presets");
}

void BabushkaProcessor::ensurePresetLibrary()
{
    const auto user = getUserPresetDirectory(), factory = getFactoryPresetDirectory();
    user.createDirectory();
    factory.createDirectory();

    // Presets saved by earlier versions (application-data folder, no sub-folders) move into User once.
    if (presetRoot == juce::File())
    {
        const auto old = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Babushka").getChildFile ("Presets");
        if (old.isDirectory() && old != getPresetRoot())
            for (auto& f : old.findChildFiles (juce::File::findFiles, false, "*.babushka"))
                if (! user.getChildFile (f.getFileName()).exists()) f.moveFileTo (user.getChildFile (f.getFileName()));
    }

    const auto marker = factory.getChildFile (".version");
    if (marker.loadFileAsString().trim() != kFactoryVersion)
    {
        writeFactoryPresetFiles (factory, false);
        marker.replaceWithText (kFactoryVersion);
    }
}

// ---- user presets ---------------------------------------------------------------------------------------------------
juce::File BabushkaProcessor::userPresetFile (const juce::String& name, const juce::String& folder) const
{
    auto dir = getUserPresetDirectory();
    const auto f = cleanFolder (folder);
    if (f.isNotEmpty()) dir = dir.getChildFile (f);
    return dir.getChildFile (juce::File::createLegalFileName (name.trim()) + ".babushka");
}

juce::File BabushkaProcessor::saveUserPreset (const juce::String& rawName, const juce::String& folder, const juce::StringArray& tags)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return {};
    const auto file = userPresetFile (name, folder);
    if (! file.getParentDirectory().createDirectory().wasOk()) return {};
    const auto oldName = currentPresetName;
    const bool oldModified = soundModified, oldRandom = randomSound;
    currentPresetName = name;
    soundModified = randomSound = false;
    auto xml = createStateXml();
    if (xml != nullptr) xml->removeAttribute ("presetFile");
    if (xml != nullptr && ! tags.isEmpty()) xml->setAttribute ("tags", tags.joinIntoString (" "));
    if (xml == nullptr || ! xml->writeTo (file))
    {
        currentPresetName = oldName; soundModified = oldModified; randomSound = oldRandom;
        return {};
    }
    currentPresetFile = file;
    tagCache.erase (file.getFullPathName());
    if (historyPos >= 0) history[(size_t) historyPos].first = "Saved: " + name;   // same sound, now under its name
    return file;
}

bool BabushkaProcessor::keptOnLoad (const juce::String& paramId) const
{
    const auto id = paramId.toStdString();
    if (babushka::isGlobalParam (id.c_str())) return true;
    const int pedal = babushka::pedalOfParam (id);
    if (pedal == babushka::kNumPedals) return (fxLocks.load() & kLockAllFx) != 0;
    return pedal >= 0 && isPedalLocked (pedal);
}

// Loading a preset file keeps the user's global settings (tuning, quality, clock jacks, spring tail length)
// and the locked pedals.
void BabushkaProcessor::restoreKeepingGlobals (const juce::XmlElement& xml)
{
    juce::XmlElement copy (xml);
    for (auto* child : copy.getChildIterator())
    {
        const auto id = child->getStringAttribute ("id");
        if (id.isNotEmpty() && keptOnLoad (id))
            if (auto* raw = apvts.getRawParameterValue (id)) child->setAttribute ("value", (double) raw->load());
    }
    restoreFromXml (copy);
}

bool BabushkaProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::parseXML (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    restoreKeepingGlobals (*xml);
    currentPresetName = file.getFileNameWithoutExtension();
    currentPresetFile = file;
    soundModified = randomSound = false;
    addHistoryStep ("Preset: " + currentPresetName);
    return true;
}

bool BabushkaProcessor::deleteUserPreset (const juce::File& file)
{
    if (! file.isAChildOf (getUserPresetDirectory()) || ! file.hasFileExtension ("babushka")) return false;
    if (file == currentPresetFile) currentPresetFile = juce::File();
    return file.deleteFile();
}

bool BabushkaProcessor::createUserFolder (const juce::String& folder)
{
    const auto f = cleanFolder (folder);
    return f.isNotEmpty() && getUserPresetDirectory().getChildFile (f).createDirectory().wasOk();
}

juce::StringArray BabushkaProcessor::getUserFolders() const
{
    juce::StringArray out;
    const auto root = getUserPresetDirectory();
    for (auto& d : root.findChildFiles (juce::File::findDirectories, true))
        if (! d.isHidden() && ! d.getFileName().startsWithChar ('.')) out.add (d.getRelativePathFrom (root).replaceCharacter ('\\', '/'));
    out.sortNatural();
    return out;
}

juce::Array<juce::File> BabushkaProcessor::getUserPresetFiles() const
{
    const auto root = getUserPresetDirectory();
    auto files = root.findChildFiles (juce::File::findFiles, true, "*.babushka");
    std::sort (files.begin(), files.end(), [&root] (const juce::File& a, const juce::File& b) {
        const auto fa = a.getParentDirectory().getRelativePathFrom (root), fb = b.getParentDirectory().getRelativePathFrom (root);
        if (a.getParentDirectory() == root || b.getParentDirectory() == root)
            if (a.getParentDirectory() != b.getParentDirectory()) return a.getParentDirectory() == root;   // root first
        if (fa != fb) return fa.compareNatural (fb) < 0;
        return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0; });
    return files;
}

juce::StringArray BabushkaProcessor::readPresetTags (const juce::File& file) const
{
    const auto key = file.getFullPathName();
    const auto modified = file.getLastModificationTime();
    auto it = tagCache.find (key);
    if (it != tagCache.end() && it->second.modified == modified) return it->second.tags;
    juce::StringArray tags;
    if (auto xml = juce::parseXML (file)) tags = splitTags (xml->getStringAttribute ("tags"));
    tagCache[key] = { modified, tags };
    return tags;
}

int BabushkaProcessor::importPresets (const juce::Array<juce::File>& files, const juce::String& folder)
{
    int count = 0;
    for (auto& f : files)
    {
        auto xml = juce::parseXML (f);
        if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) continue;
        auto dest = userPresetFile (f.getFileNameWithoutExtension(), folder);
        if (! dest.getParentDirectory().createDirectory().wasOk()) continue;
        if (dest.exists()) dest = dest.getNonexistentSibling (true);
        if (f.copyFileTo (dest)) ++count;
    }
    return count;
}

bool BabushkaProcessor::exportCurrentSound (const juce::File& destination)
{
    auto xml = createStateXml();
    if (xml == nullptr) return false;
    xml->removeAttribute ("presetFile");
    if (currentPresetFile.existsAsFile())
    {
        const auto tags = readPresetTags (currentPresetFile);
        if (! tags.isEmpty()) xml->setAttribute ("tags", tags.joinIntoString (" "));
    }
    else if (! randomSound)
        xml->setAttribute ("tags", babushka::factoryPresetTags (currentPresetName.toStdString()));
    return xml->writeTo (destination.withFileExtension ("babushka"));
}

juce::File BabushkaProcessor::duplicateCurrentSound()
{
    juce::String folder;
    juce::StringArray tags;
    if (currentPresetFile.existsAsFile())
    {
        folder = currentPresetFile.getParentDirectory().getRelativePathFrom (getUserPresetDirectory());
        if (! currentPresetFile.getParentDirectory().isAChildOf (getUserPresetDirectory())) folder = {};
        tags = readPresetTags (currentPresetFile);
    }
    else if (! randomSound)
        tags = splitTags (babushka::factoryPresetTags (currentPresetName.toStdString()));
    auto name = currentPresetName + " copy";
    for (int n = 2; userPresetFile (name, folder).exists(); ++n) name = currentPresetName + " copy " + juce::String (n);
    return saveUserPreset (name, folder, tags);
}

// ---- the list the browser and the < > buttons use ----------------------------------------------------------------
std::vector<BabushkaProcessor::PresetRef> BabushkaProcessor::listPresets() const
{
    std::vector<PresetRef> list;
    const auto& all = babushka::factoryPresets();
    std::vector<int> order ((size_t) all.size());
    for (int i = 0; i < (int) all.size(); ++i) order[(size_t) i] = i;
    std::stable_sort (order.begin(), order.end(), [&all] (int a, int b) {
        return juce::String (all[(size_t) a].category).compareNatural (all[(size_t) b].category) < 0; });
    for (int i : order)
    {
        const auto& f = all[(size_t) i];
        PresetRef r; r.factory = true; r.factoryIndex = i; r.name = f.name; r.category = f.category; r.description = f.description;
        r.tags = splitTags (babushka::factoryPresetTags (f.name));
        list.push_back (std::move (r));
    }
    const auto root = getUserPresetDirectory();
    for (auto& file : getUserPresetFiles())
    {
        PresetRef r; r.factory = false; r.file = file; r.name = file.getFileNameWithoutExtension(); r.category = "User";
        if (file.getParentDirectory() != root) r.folder = file.getParentDirectory().getRelativePathFrom (root).replaceCharacter ('\\', '/');
        r.tags = readPresetTags (file);
        list.push_back (std::move (r));
    }
    return list;
}

void BabushkaProcessor::loadPreset (const PresetRef& ref)
{
    if (ref.factory) applyFactoryPreset (ref.factoryIndex); else loadUserPreset (ref.file);
}

void BabushkaProcessor::stepPreset (int delta)
{
    const auto list = listPresets();
    if (list.empty()) return;
    int cur = -1;
    for (int i = 0; i < (int) list.size(); ++i)
    {
        const auto& r = list[(size_t) i];
        if (r.factory ? (currentPresetFile == juce::File() && r.name == currentPresetName) : r.file == currentPresetFile) { cur = i; break; }
    }
    const int n = (int) list.size();
    const int next = cur < 0 ? (delta > 0 ? 0 : n - 1) : ((cur + delta) % n + n) % n;
    loadPreset (list[(size_t) next]);
}
