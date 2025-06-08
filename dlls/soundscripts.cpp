#include "soundscripts.h"
#include "error_collector.h"
#include "logger.h"
#include "min_and_max.h"
#include "random_utils.h"

#include <map>
#include <set>
#include <string>

#include "json_utils.h"

using namespace rapidjson;

const char* soundScriptsSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"$ref": "definitions.json#/soundscript"
	}
}
)";

SoundScript::SoundScript(): waves(), channel(CHAN_AUTO), volume(VOL_NORM), attenuation(ATTN_NORM), pitch(PITCH_NORM) {}

SoundScript::SoundScript(int soundChannel, std::initializer_list<const char*> sounds, FloatRange soundVolume, float soundAttenuation, IntRange soundPitch)
	: waves(sounds), channel(soundChannel), volume(soundVolume), attenuation(soundAttenuation), pitch(soundPitch)
{}

SoundScript::SoundScript(int soundChannel, std::initializer_list<const char *> sounds, IntRange soundPitch)
	: waves(sounds), channel(soundChannel), volume(VOL_NORM), attenuation(ATTN_NORM), pitch(soundPitch)
{}

const char* SoundScript::Wave() const
{
	if (waves.size() > 1)
	{
		return waves[RandomInt(0, waves.size() - 1)];
	}
	else if (waves.size() == 1)
	{
		return waves[0];
	}
	return nullptr;
}

const char* SoundScript::Wave(int index) const
{
	if (index >= 0 && static_cast<unsigned int>(index) < waves.size())
		return waves[index];
	return nullptr;
}

void SoundScriptParamOverride::OverrideVolumeAbsolute(FloatRange newVolume)
{
	volumeOverride = OVERRIDE_ABSOLUTE;
	volume = newVolume;
}

void SoundScriptParamOverride::OverrideVolumeRelative(FloatRange newVolume)
{
	volumeOverride = OVERRIDE_RELATIVE;
	volume = newVolume;
}

void SoundScriptParamOverride::OverrideAttenuationAbsolute(float newAttenuation)
{
	attenuationOverride = OVERRIDE_ABSOLUTE;
	attenuation = newAttenuation;
}

void SoundScriptParamOverride::OverrideAttenuationRelative(float newAttenuation)
{
	attenuationOverride = OVERRIDE_RELATIVE;
	attenuation = newAttenuation;
}

void SoundScriptParamOverride::OverridePitchAbsolute(IntRange newPitch)
{
	pitchOverride = OVERRIDE_ABSOLUTE;
	pitch = newPitch;
}

void SoundScriptParamOverride::OverridePitchRelative(IntRange newPitch)
{
	pitchOverride = OVERRIDE_RELATIVE;
	pitch = newPitch;
}

void SoundScriptParamOverride::OverridePitchShifted(int pitchShift)
{
	pitchOverride = OVERRIDE_SHIFT;
	pitch = pitchShift;
}

void SoundScriptParamOverride::OverrideChannel(int newChannel)
{
	channelOverride = OVERRIDE_ABSOLUTE;
	channel = newChannel;
}

void SoundScriptParamOverride::ApplyOverride(int& origChannel, FloatRange &origVolume, float &origAttenuation, IntRange &origPitch) const
{
	if (channelOverride == OVERRIDE_ABSOLUTE)
	{
		origChannel = channel;
	}
	if (volumeOverride == OVERRIDE_ABSOLUTE)
	{
		origVolume = volume;
	}
	else if (volumeOverride == OVERRIDE_RELATIVE)
	{
		origVolume = FloatRange(origVolume.min * volume.min, origVolume.max * volume.max);
	}

	if (attenuationOverride == OVERRIDE_ABSOLUTE)
	{
		origAttenuation = attenuation;
	}
	else if (attenuationOverride == OVERRIDE_RELATIVE)
	{
		origAttenuation *= attenuation;
	}

	if (pitchOverride == OVERRIDE_ABSOLUTE)
	{
		origPitch = pitch;
	}
	else if (pitchOverride == OVERRIDE_RELATIVE)
	{
		origPitch = IntRange(pitch.min * origPitch.min / 100, pitch.max * origPitch.max / 100);
	}
	else if (pitchOverride == OVERRIDE_SHIFT)
	{
		origPitch = IntRange(origPitch.min + pitch.min, origPitch.max + pitch.max);
	}
}

static bool ParseChannel(const char* str, int& channel)
{
	constexpr std::pair<const char*, int> channels[] = {
		{"auto", CHAN_AUTO},
		{"weapon", CHAN_WEAPON},
		{"voice", CHAN_VOICE},
		{"item", CHAN_ITEM},
		{"body", CHAN_BODY},
		{"static", CHAN_STATIC},
	};

	for (auto& p : channels)
	{
		if (stricmp(str, p.first) == 0)
		{
			channel = p.second;
			return true;
		}
	}
	return false;
}

static const char* ChannelToString(int channel)
{
	switch (channel) {
	case CHAN_AUTO:
		return "auto";
	case CHAN_WEAPON:
		return "weapon";
	case CHAN_VOICE:
		return "voice";
	case CHAN_ITEM:
		return "item";
	case CHAN_BODY:
		return "body";
	case CHAN_STATIC:
		return "static";
	default:
		return "unknown";
	}
}

const char* SoundScriptSystem::Schema() const
{
	return soundScriptsSchema;
}

bool SoundScriptSystem::ReadFromDocument(const Document& document, const char* fileName)
{
	for (auto scriptIt = document.MemberBegin(); scriptIt != document.MemberEnd(); ++scriptIt)
	{
		const char* name = scriptIt->name.GetString();

		const Value& value = scriptIt->value;
		if (value.IsObject())
			AddSoundScriptFromJsonValue(name, value);
		else
			g_errorCollector.AddFormattedError("%s: soundscript '%s' is not an object!\n", fileName, name);
	}

	return true;
}

void SoundScriptSystem::AddSoundScriptFromJsonValue(const char *name, const Value &value, int defaultChannel)
{
	SoundScript soundScript;
	SoundScriptMeta soundScriptMeta;

	HandleJSONMember(value, "waves", [&soundScript, &soundScriptMeta, this](const Value& value) {
		Value::ConstArray arr = value.GetArray();
		for (auto& item : arr)
		{
			std::string str = item.GetString();
			auto strIt = _waveStringSet.find(str);
			if (strIt == _waveStringSet.end())
			{
				auto p = _waveStringSet.insert(str);
				strIt = p.first;
			}
			soundScript.waves.push_back(strIt->c_str());
		}
		soundScriptMeta.wavesSet = true;
	});

	{
		auto it = value.FindMember("channel");
		if (it != value.MemberEnd())
		{
			soundScriptMeta.channelSet = ParseChannel(it->value.GetString(), soundScript.channel);
		}
		else if (defaultChannel != -1)
		{
			soundScript.channel = defaultChannel;
			soundScriptMeta.channelSet = true;
		}
	}

	soundScriptMeta.volumeSet = UpdatePropertyFromJson(soundScript.volume, value, "volume");

	HandleJSONMember(value, "attenuation", [&soundScript, &soundScriptMeta, this](const Value& value) {
		soundScriptMeta.attenuationSet = UpdateAttenuationFromJson(soundScript.attenuation, value);
	});

	soundScriptMeta.pitchSet = UpdatePropertyFromJson(soundScript.pitch, value, "pitch");

	_soundScripts[name] = std::make_pair(soundScript, soundScriptMeta);
}

void SoundScriptSystem::EnsureSoundScriptExists(const std::string& name)
{
	auto it = _soundScripts.find(name);
	if (it == _soundScripts.end())
		_soundScripts[name] = std::make_pair(SoundScript(), SoundScriptMeta());
}

const SoundScript* SoundScriptSystem::GetSoundScript(const char *name)
{
	if (!name || *name == '\0')
		return nullptr;
	_temp = name; // reuse the same std::string for search to avoid reallocation
	auto it = _soundScripts.find(_temp);
	if (it != _soundScripts.end())
		return &it->second.first;
	return nullptr;
}

static void MarkSoundScriptAllDefined(SoundScriptMeta& meta)
{
	meta.defaultSet = true;
	meta.wavesSet = true;
	meta.channelSet = true;
	meta.volumeSet = true;
	meta.attenuationSet = true;
	meta.pitchSet = true;
}

void SoundScriptSystem::EnsureExistingScriptDefined(SoundScript& existing, SoundScriptMeta& meta, const SoundScript& soundScript)
{
	if (!meta.defaultSet)
	{
		if (!meta.wavesSet)
		{
			existing.waves = soundScript.waves;
		}

		if (!meta.channelSet)
			existing.channel = soundScript.channel;

		if (!meta.volumeSet)
			existing.volume = soundScript.volume;

		if (!meta.attenuationSet)
			existing.attenuation = soundScript.attenuation;

		if (!meta.pitchSet)
			existing.pitch = soundScript.pitch;

		MarkSoundScriptAllDefined(meta);
	}
}

const SoundScript* SoundScriptSystem::ProvideDefaultSoundScript(const char *name, const SoundScript &soundScript)
{
	_temp = name;
	auto it = _soundScripts.find(_temp);
	if (it != _soundScripts.end())
	{
		SoundScript& existing = it->second.first;
		SoundScriptMeta& meta = it->second.second;
		EnsureExistingScriptDefined(existing, meta, soundScript);
		return &existing;
	}
	else
	{
		SoundScriptMeta meta;
		MarkSoundScriptAllDefined(meta);
		auto inserted = _soundScripts.insert(std::make_pair(_temp, std::make_pair(soundScript, meta)));
		if (inserted.second)
		{
			return &inserted.first->second.first;
		}
		return nullptr;
	}
}

const SoundScript* SoundScriptSystem::ProvideDefaultSoundScript(const char *derivative, const char *base, const SoundScript &soundScript, const SoundScriptParamOverride paramOverride)
{
	_temp = derivative;
	auto it = _soundScripts.find(_temp);
	if (it != _soundScripts.end())
	{
		SoundScript& existing = it->second.first;
		SoundScriptMeta& meta = it->second.second;

		if (!meta.defaultSet)
		{
			const SoundScript* baseScript = ProvideDefaultSoundScript(base, soundScript);
			if (baseScript)
			{
				if (paramOverride.HasOverrides())
				{
					SoundScript overrideSoundScript = *baseScript;
					paramOverride.ApplyOverride(overrideSoundScript.channel, overrideSoundScript.volume, overrideSoundScript.attenuation, overrideSoundScript.pitch);
					EnsureExistingScriptDefined(existing, meta, overrideSoundScript);
				}
				else
				{
					EnsureExistingScriptDefined(existing, meta, *baseScript);
				}
			}
		}
		return &existing;
	}
	else
	{
		const SoundScript* baseScript = ProvideDefaultSoundScript(base, soundScript);
		if (baseScript)
		{
			if (paramOverride.HasOverrides())
			{
				SoundScript overrideSoundScript = *baseScript;
				paramOverride.ApplyOverride(overrideSoundScript.channel, overrideSoundScript.volume, overrideSoundScript.attenuation, overrideSoundScript.pitch);
				return ProvideDefaultSoundScript(derivative, overrideSoundScript);
			}
			else
			{
				return ProvideDefaultSoundScript(derivative, *baseScript);
			}
		}
	}
	return nullptr;
}

void SoundScriptSystem::DumpSoundScriptImpl(const char *name, const SoundScript &soundScript, const SoundScriptMeta &meta) const
{
	LOG("%s:\n", name);

	LOG("Waves: ");
	if (meta.wavesSet)
	{
		for (const auto& wave : soundScript.waves)
		{
			LOG("\"%s\"; ", wave);
		}
		LOG("\n");
	}
	else
	{
		LOG("%s\n", notDefinedYet);
	}

	LOG("Channel: %s. ", meta.channelSet ? ChannelToString(soundScript.channel) : notDefinedYet);

	LOG("Volume: ");
	if (meta.volumeSet)
	{
		if (soundScript.volume.max <= soundScript.volume.min)
		{
			LOG("%g. ", soundScript.volume.min);
		}
		else
		{
			LOG("%g-%g. ", soundScript.volume.min, soundScript.volume.max);
		}
	}
	else
	{
		LOG("%s. ", notDefinedYet);
	}

	LOG("Attenuation: ");
	if (meta.attenuationSet)
	{
		LOG("%g. ", soundScript.attenuation);
	}
	else
	{
		LOG("%s. ", notDefinedYet);
	}

	LOG("Pitch: ");
	if (meta.pitchSet)
	{
		if (soundScript.pitch.max <= soundScript.pitch.min)
		{
			LOG("%d\n\n", soundScript.pitch.min);
		}
		else
		{
			LOG("%d-%d\n\n", soundScript.pitch.min, soundScript.pitch.max);
		}
	}
	else
	{
		LOG("%s\n\n", notDefinedYet);
	}
}

void SoundScriptSystem::DumpSoundScripts() const
{
	for (const auto& p : _soundScripts)
	{
		DumpSoundScriptImpl(p.first.c_str(),  p.second.first, p.second.second);
	}
}

void SoundScriptSystem::DumpSoundScript(const char *name) const
{
	if (!*name)
		return;
	std::string temp = name;
	if (temp[temp.size()-1] == '.' || temp[temp.size()-1] == '#')
	{
		bool foundSomething = false;
		for (const auto& p : _soundScripts)
		{
			if (strnicmp(p.first.c_str(), temp.c_str(), temp.size()) == 0)
			{
				foundSomething = true;
				DumpSoundScriptImpl(p.first.c_str(),  p.second.first, p.second.second);
			}
		}
		if (foundSomething)
			return;
	}
	else
	{
		auto it = _soundScripts.find(temp);
		if (it != _soundScripts.end())
		{
			DumpSoundScriptImpl(name, it->second.first, it->second.second);
			return;
		}
	}
	LOG("Couldn't find a sound script for %s\n", name);
}

SoundScriptSystem g_SoundScriptSystem;
