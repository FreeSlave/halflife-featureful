#include "ent_templates.h"
#include "icase_compare.h"

#include "blood_types.h"
#include "classify.h"
#include "grapple_target.h"
#include "hull_sizes.h"
#include "dmg_types.h"
#include "gib.h"

#include <algorithm>
#include <set>
#include <utility>

#include "json_utils.h"
#include "logger.h"
#include "error_collector.h"

using namespace rapidjson;

const char* entTemplatesSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"$ref": "definitions.json#/entity_template"
	}
}
)";

static std::string g_tempString;

const char* EntTemplate::OwnVisualName() const
{
	return _ownVisual.empty() ? nullptr : _ownVisual.c_str();
}

const char* EntTemplate::GibVisualName() const
{
	return _gibVisual.empty() ? nullptr : _gibVisual.c_str();
}

const char* EntTemplate::GetSoundScriptNameOverride(const char *name) const
{
	g_tempString = name;
	auto it = _soundScripts.find(g_tempString);
	if (it != _soundScripts.end())
		return it->second.c_str();
	return nullptr;
}

const char* EntTemplate::GetVisualNameOverride(const char *name) const
{
	g_tempString = name;
	auto it = _visuals.find(g_tempString);
	if (it != _visuals.end())
		return it->second.c_str();
	return nullptr;
}

void EntTemplate::SetSoundScriptReplacement(const char *soundScript, const std::string& replacement)
{
	_soundScripts[soundScript] = replacement;
}

void EntTemplate::SetVisualReplacement(const char *visual, const std::string& replacement)
{
	_visuals[visual] = replacement;
}

const char* EntTemplate::GetSoundReplacement(const char *originalSample) const
{
	if (_soundReplacements.empty())
		return nullptr;
	auto it = _soundReplacements.find(originalSample);
	if (it == _soundReplacements.end())
		return nullptr;
	return it->second.c_str();
}

void EntTemplate::SetSoundReplacement(const char *originalSample, const char *replacementSample)
{
	_soundReplacements[originalSample] = replacementSample;
}

void EntTemplate::SetPrecachedSounds(std::vector<std::string> &&sounds)
{
	_precachedSounds = sounds;
}

void EntTemplate::SetPrecachedSoundScripts(std::vector<std::string> &&soundScripts)
{
	_precachedSoundScripts = soundScripts;
}

void EntTemplate::AddPrecachedSoundScript(const std::string& soundScript)
{
	if (std::find(_precachedSoundScripts.begin(), _precachedSoundScripts.end(), soundScript) == _precachedSoundScripts.end())
		_precachedSoundScripts.push_back(soundScript);
}

const char* EntTemplate::SpeechPrefix() const
{
	return _speechPrefix.empty() ? nullptr : _speechPrefix.c_str();
}

static std::string GenerateResourceName(const std::string& templateName, const char* resourceName)
{
	return templateName + '#' + resourceName;
}

const char* EntTemplateSystem::Schema() const
{
	return entTemplatesSchema;
}

bool EntTemplateSystem::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	for (auto templateIt = document.MemberBegin(); templateIt != document.MemberEnd(); ++templateIt)
	{
		const char* name = templateIt->name.GetString();
		const Value& value = templateIt->value;
		if (value.IsObject())
		{
			bool success = AddTemplateFromJsonValue(document, templateIt->name.GetString(), value, fileName);
			if (!success)
				return false;
		}
		else
			g_errorCollector.AddFormattedError("%s: entity template '%s' is not an object!\n", fileName, name);
	}

	return true;
}

static bool UpdateSizesFromJSON(const rapidjson::Value& value, Vector& mins, Vector& maxs)
{
	if (value.IsString())
	{
		const char* sizePreset = value.GetString();
		if (stricmp(sizePreset, "snark") == 0)
		{
			mins = Vector(-4.0f, -4.0f, 0.0f);
			maxs = Vector(4.0f, 4.0f, 8.0f);
			return true;
		}
		else if (stricmp(sizePreset, "headcrab") == 0)
		{
			mins = Vector(-12.0f, -12.0f, 0.0f);
			maxs = Vector(12.0f, 12.0f, 24.0f);
			return true;
		}
		else if (stricmp(sizePreset, "small") == 0)
		{
			mins = Vector(-16.0f, -16.0f, 0.0f);
			maxs = Vector( 16.0f, 16.0f, 36.0f );
			return true;
		}
		else if (stricmp(sizePreset, "human") == 0)
		{
			mins = VEC_HUMAN_HULL_MIN;
			maxs = VEC_HUMAN_HULL_MAX;
			return true;
		}
		else if (stricmp(sizePreset, "large") == 0 || stricmp(sizePreset, "wide") == 0)
		{
			mins = Vector(-32.0f, -32.0f, 0.0f);
			maxs = Vector(32.0f, 32.0f, 64.0f);
			return true;
		}
		else
		{
			LOG_WARNING("Unknown size preset '%s'\n", sizePreset);
			return false;
		}
	}
	else if (value.IsObject())
	{
		return UpdatePropertyFromJson(mins, value, "mins") && UpdatePropertyFromJson(maxs, value, "maxs");
	}
	return false;
}

int EntTemplate::ParseDamageType(const char *type)
{
	if (stricmp(type, "generic") == 0)
	{
		return DMG_GENERIC;
	}
	else if (stricmp(type, "crush") == 0)
	{
		return DMG_CRUSH;
	}
	else if (stricmp(type, "bullet") == 0)
	{
		return DMG_BULLET;
	}
	else if (stricmp(type, "slash") == 0)
	{
		return DMG_SLASH;
	}
	else if (stricmp(type, "burn") == 0)
	{
		return DMG_BURN;
	}
	else if (stricmp(type, "freeze") == 0)
	{
		return DMG_FREEZE;
	}
	else if (stricmp(type, "blast") == 0)
	{
		return DMG_BLAST;
	}
	else if (stricmp(type, "club") == 0)
	{
		return DMG_CLUB;
	}
	else if (stricmp(type, "shock") == 0)
	{
		return DMG_SHOCK;
	}
	else if (stricmp(type, "sonic") == 0)
	{
		return DMG_SONIC;
	}
	else if (stricmp(type, "energybeam") == 0)
	{
		return DMG_ENERGYBEAM;
	}
	else if (stricmp(type, "paralyze") == 0)
	{
		return DMG_PARALYZE;
	}
	else if (stricmp(type, "nervegas") == 0)
	{
		return DMG_NERVEGAS;
	}
	else if (stricmp(type, "poison") == 0)
	{
		return DMG_POISON;
	}
	else if (stricmp(type, "radiation") == 0)
	{
		return DMG_RADIATION;
	}
	else if (stricmp(type, "acid") == 0)
	{
		return DMG_ACID;
	}
	else if (stricmp(type, "slowburn") == 0)
	{
		return DMG_SLOWBURN;
	}
	else if (stricmp(type, "slowfreeze") == 0)
	{
		return DMG_SLOWFREEZE;
	}
	return -1;
}

bool EntTemplate::UpdateDamageInfoFromJSON(const rapidjson::Value &value, DamageInfo &damageInfo)
{
	UpdatePropertyFromJson(damageInfo.damage, value, "damage");

	HandleJSONMember(value, "type", [&damageInfo](const Value& value) {
		int damageType = 0;
		if (value.IsArray())
		{
			Value::ConstArray arr = value.GetArray();
			for (size_t i=0; i<arr.Size(); ++i)
			{
				const char* damageTypeName = arr[i].GetString();
				int subType = ParseDamageType(damageTypeName);
				if (subType >= 0)
				{
					damageType |= subType;
				}
				else
				{
					LOG_WARNING("Unknown damage type '%s'\n", damageTypeName);
				}
			}
		}
		else
		{
			const char* damageTypeName = value.GetString();
			int subType = ParseDamageType(damageTypeName);
			if (subType >= 0)
			{
				damageType |= subType;
			}
			else
			{
				LOG_WARNING("Unknown damage type '%s'\n", damageTypeName);
			}
		}

		damageInfo.type = damageType;
	});

	HandleJSONMember(value, "type_policy", [&damageInfo](const Value& value) {
		const char* typePolicyName = value.GetString();
		if (strcmp(typePolicyName, "add") == 0)
		{
			damageInfo.typePolicy = EntTemplate::DamageInfo::ADD_DAMAGE_TYPE;
		}
		else if (strcmp(typePolicyName, "replace") == 0)
		{
			damageInfo.typePolicy = EntTemplate::DamageInfo::REPLACE_DAMAGE_TYPE;
		}
	});

	UpdatePropertyFromJson(damageInfo.nonLethal, value, "nonlethal");
	UpdatePropertyFromJson(damageInfo.ignoreArmor, value, "ignore_armor");

	HandleJSONMember(value, "gib", [&](const Value& value) {
		const char* gibPolicyName = value.GetString();
		if (strcmp(gibPolicyName, "always") == 0)
		{
			damageInfo.gibPolicy = GIB_ALWAYS;
		}
		else if (strcmp(gibPolicyName, "never") == 0)
		{
			damageInfo.gibPolicy = GIB_NEVER;
		}
		else if (strcmp(gibPolicyName, "normal") == 0)
		{
			damageInfo.gibPolicy = GIB_NORMAL;
		}
	});

	return true;
}

bool EntTemplateSystem::AddTemplateFromJsonValue(const Value& allTemplatesJsonValue, const char* name, const Value& value, const char* fileName, std::vector<std::string> inheritanceChain)
{
	const std::string templateName = name;

	if (std::find(inheritanceChain.begin(), inheritanceChain.end(), templateName) != inheritanceChain.end())
	{
		std::string chainString;
		for (auto it = inheritanceChain.begin(); it != inheritanceChain.end(); it++)
		{
			chainString += "'" + *it + "' -> ";
		}
		chainString += "'";
		chainString += templateName;
		chainString += "'";
		g_errorCollector.AddFormattedError("%s: cycle in entity template inheritance detected: %s", fileName, chainString.c_str());
		return false;
	}

	auto existingTemplateIt = _entTemplates.find(templateName);
	if (existingTemplateIt != _entTemplates.end())
	{
		// Already added, has been used as parent for another template
		return true;
	}

	EntTemplate entTemplate;

	auto inheritsIt = value.FindMember("inherits");
	if (inheritsIt != value.MemberEnd())
	{
		const char* parentName = inheritsIt->value.GetString();
		existingTemplateIt = _entTemplates.find(parentName);
		if (existingTemplateIt != _entTemplates.end())
		{
			entTemplate = existingTemplateIt->second;
		}
		else
		{
			auto parentIt = allTemplatesJsonValue.FindMember(parentName);
			if (parentIt != allTemplatesJsonValue.MemberEnd())
			{
				inheritanceChain.push_back(templateName);
				if (AddTemplateFromJsonValue(allTemplatesJsonValue, parentName, parentIt->value, fileName, inheritanceChain))
				{
					existingTemplateIt = _entTemplates.find(parentName);
					if (existingTemplateIt != _entTemplates.end())
					{
						entTemplate = existingTemplateIt->second;
					}
				}
				else
					return false;
			}
			else
			{
				g_errorCollector.AddFormattedError("%s: couldn't find a parent entity template '%s' for '%s'", fileName, parentName, name);
			}
		}
	}

	AddTemplateFromJsonValueImpl(templateName, value, entTemplate);
	return true;
}

void EntTemplateSystem::AddTemplateFromJsonValue(const char* name, const rapidjson::Value& value, const char* fileName)
{
	const std::string templateName = name;

	EntTemplate entTemplate;

	HandleJSONMember(value, "inherits", [&entTemplate, this, fileName, name](const Value& value) {
		const char* parentName = value.GetString();
		auto existingTemplateIt = _entTemplates.find(parentName);
		if (existingTemplateIt != _entTemplates.end())
		{
			entTemplate = existingTemplateIt->second;
		}
		else
		{
			g_errorCollector.AddFormattedError("%s: couldn't find a parent entity template '%s' for '%s'", fileName, parentName, name);
		}
	});

	AddTemplateFromJsonValueImpl(templateName, value, entTemplate);
}

void EntTemplateSystem::AddTemplateFromJsonValueImpl(const std::string& templateName, const rapidjson::Value& value, EntTemplate& entTemplate)
{
	HandleJSONMember(value, "own_visual", [&entTemplate, &templateName, this](const Value& value) {
		if (value.IsString())
		{
			entTemplate.SetOwnVisualName(value.GetString());
		}
		else if (value.IsObject())
		{
			std::string ownVisualName = templateName + "##own_visual";
			entTemplate.SetOwnVisualName(ownVisualName);
			_visualSystem->AddVisualFromJsonValue(ownVisualName.c_str(), value);
		}
	});

	HandleJSONMember(value, "gib_visual", [&entTemplate, &templateName, this](const Value& value) {
		if (value.IsString())
		{
			entTemplate.SetGibVisualName(value.GetString());
		}
		else if (value.IsObject())
		{
			std::string gibVisualName = templateName + "##gib_visual";
			entTemplate.SetGibVisualName(gibVisualName);
			_visualSystem->AddVisualFromJsonValue(gibVisualName.c_str(), value);
		}
	});

	HandleJSONMember(value, "soundscripts", [&entTemplate, &templateName, this](const Value& value) {
		for (auto scriptIt = value.MemberBegin(); scriptIt != value.MemberEnd(); ++scriptIt)
		{
			const char* soundScriptName = scriptIt->name.GetString();
			if (scriptIt->value.IsString())
			{
				const char* replacement = scriptIt->value.GetString();
				entTemplate.SetSoundScriptReplacement(soundScriptName, replacement);
			}
			else if (scriptIt->value.IsObject())
			{
				std::string replacement = GenerateResourceName(templateName, soundScriptName);
				entTemplate.SetSoundScriptReplacement(soundScriptName, replacement);
				_soundScriptSystem->AddSoundScriptFromJsonValue(replacement.c_str(), scriptIt->value);
			}
		}
	});

	HandleJSONMember(value, "visuals", [&entTemplate, &templateName, this](const Value& value) {
		for (auto visualIt = value.MemberBegin(); visualIt != value.MemberEnd(); ++visualIt)
		{
			const char* visualName = visualIt->name.GetString();
			if (visualIt->value.IsString())
			{
				const char* replacement = visualIt->value.GetString();
				entTemplate.SetVisualReplacement(visualName, replacement);
			}
			else if (visualIt->value.IsObject())
			{
				std::string replacement = GenerateResourceName(templateName, visualName);
				entTemplate.SetVisualReplacement(visualName, replacement);
				_visualSystem->AddVisualFromJsonValue(replacement.c_str(), visualIt->value);
			}
		}
	});

	HandleJSONMember(value, "sound_replacement", [&entTemplate](const Value& value) {
		for (auto sIt = value.MemberBegin(); sIt != value.MemberEnd(); ++sIt)
		{
			const char* originalSample = sIt->name.GetString();
			const char* replacementSample = sIt->value.GetString();
			entTemplate.SetSoundReplacement(originalSample, replacementSample);
		}
	});

	HandleJSONMember(value, "precached_sounds", [&entTemplate](const Value& value) {
		Value::ConstArray arr = value.GetArray();
		std::vector<std::string> sounds;
		sounds.reserve(arr.Size());
		for (size_t i=0; i<arr.Size(); ++i)
		{
			sounds.push_back(arr[i].GetString());
		}
		entTemplate.SetPrecachedSounds(std::move(sounds));
	});

	HandleJSONMember(value, "precached_soundscripts", [&entTemplate](const Value& value) {
		Value::ConstArray arr = value.GetArray();
		std::vector<std::string> soundsScripts;
		soundsScripts.reserve(arr.Size());
		for (size_t i=0; i<arr.Size(); ++i)
		{
			soundsScripts.push_back(arr[i].GetString());
		}
		entTemplate.SetPrecachedSoundScripts(std::move(soundsScripts));
	});

	{
		bool b;
		if (UpdatePropertyFromJson(b, value, "autoprecache_sounds"))
		{
			entTemplate.SetAutoPrecacheSounds(b);
		}
	}

	{
		bool b;
		if (UpdatePropertyFromJson(b, value, "autoprecache_soundscripts"))
		{
			entTemplate.SetAutoPrecacheSoundScripts(b);
		}
	}

	HandleJSONMember(value, "classify", [&entTemplate](const Value& value) {
		const char* classifyName = value.GetString();
		int classify = ClassifyFromName(classifyName);
		if (classify < 0)
		{
			LOG("Unknown classification '%s'\n", classifyName);
		}
		else
		{
			entTemplate.SetClassify(classify);
		}
	});

	HandleJSONMember(value, "blood", [&entTemplate](const Value& value) {
		const char* bloodType = value.GetString();
		if (stricmp(bloodType, "red") == 0)
		{
			entTemplate.SetBloodColor(BLOOD_COLOR_RED);
		}
		else if (stricmp(bloodType, "yellow") == 0)
		{
			entTemplate.SetBloodColor(BLOOD_COLOR_YELLOW);
		}
		else if (stricmp(bloodType, "no") == 0)
		{
			entTemplate.SetBloodColor(DONT_BLEED);
		}
		else
		{
			LOG_WARNING("Unknown blood type '%s'\n", bloodType);
		}
	});

	HandleJSONMember(value, "health", [&entTemplate](const Value& value) {
		entTemplate.SetHealth(value.GetFloat());
	});

	HandleJSONMember(value, "field_of_view", [&entTemplate](const Value& value) {
		if (value.IsNumber())
		{
			entTemplate.SetFieldOfView(value.GetFloat());
		}
		else if (value.IsString())
		{
			const char* fovType = value.GetString();
			const std::pair<const char*, float> fovPairs[] = {
				{"full", -1.0f},
				{"wide", -0.7f},
				{"average", 0.2f},
				{"tunnel", 0.5f},
				{"narrow", 0.7f},
			};

			bool foundFov = false;
			for (auto p: fovPairs)
			{
				if (stricmp(p.first, fovType) == 0)
				{
					foundFov = true;
					entTemplate.SetFieldOfView(p.second);
					break;
				}
			}

			if (!foundFov)
				LOG_WARNING("Unknown FOV type '%s'\n", fovType);
		}
	});

	HandleJSONMember(value, "size", [&entTemplate](const Value& value) {
		Vector mins, maxs;
		if (UpdateSizesFromJSON(value, mins, maxs))
		{
			entTemplate.SetSize(mins, maxs);
		}
	});

	HandleJSONMember(value, "collision_box", [&entTemplate](const Value& value) {
		Vector mins, maxs;
		if (UpdateSizesFromJSON(value, mins, maxs))
		{
			entTemplate.SetCollisionBox(mins, maxs);
		}
	});

	HandleJSONMember(value, "size_for_grapple", [&entTemplate](const Value& value) {
		const char* targetType = value.GetString();
		const std::pair<const char*, int> sizeValues[] = {
			{"no", GRAPPLE_NOT_A_TARGET},
			{"small", GRAPPLE_SMALL},
			{"medium", GRAPPLE_MEDIUM},
			{"large", GRAPPLE_LARGE},
			{"fixed", GRAPPLE_FIXED},
		};

		bool found = false;
		for (auto p: sizeValues)
		{
			if (stricmp(p.first, targetType) == 0)
			{
				found = true;
				entTemplate.SetSizeForGrapple(p.second);
				break;
			}
		}

		if (!found)
			LOG_WARNING("Unknown grapple target type '%s'\n", targetType);
	});

	HandleJSONMember(value, "speech_prefix", [&entTemplate](const Value& value) {
		entTemplate.SetSpeechPrefix(value.GetString());
	});

	HandleJSONMember(value, "squad_capability", [&entTemplate](const Value& value) {
		if (value.IsBool())
		{
			SquadCapabilities caps = entTemplate.GetSquadCapabilities();
			caps.canRecruit = value.GetBool();
			entTemplate.SetSquadCapabilities(caps);
		}
		else if (value.IsObject())
		{
			SquadCapabilities caps = entTemplate.GetSquadCapabilities();
			UpdatePropertyFromJson(caps.canRecruit, value, "can_recruit");
			UpdatePropertyFromJson(caps.denyRecruiting, value, "deny_recruiting");
			UpdatePropertyFromJson(caps.allowDifferentClassification, value, "allow_different_classification");
			UpdatePropertyFromJson(caps.requireSameClassname, value, "require_same_classname");
			UpdatePropertyFromJson(caps.requireSameEntTemplate, value, "require_same_ent_template");
			entTemplate.SetSquadCapabilities(caps);
		}
	});

	HandleJSONMember(value, "open_door_capability", [&entTemplate](const Value& value) {
		entTemplate.SetCanOpenDoors(value.GetBool());
	});

	HandleJSONMember(value, "check_melee_attack1", [&entTemplate](const Value& value) {
		EntTemplate::CheckMeleeAttack check = entTemplate.GetCheckMeleeAttack1();
		UpdatePropertyFromJson(check.distance, value, "distance");
		UpdatePropertyFromJson(check.dot, value, "dot");
		entTemplate.SetCheckMeleeAttack1(check);
	});

	HandleJSONMember(value, "check_melee_attack2", [&entTemplate](const Value& value) {
		EntTemplate::CheckMeleeAttack check = entTemplate.GetCheckMeleeAttack2();
		UpdatePropertyFromJson(check.distance, value, "distance");
		UpdatePropertyFromJson(check.dot, value, "dot");
		entTemplate.SetCheckMeleeAttack2(check);
	});

	HandleJSONMember(value, "trace_hull_attacks", [&entTemplate, &templateName, this](const Value& value) {
		for (auto attackIt = value.MemberBegin(); attackIt != value.MemberEnd(); ++attackIt)
		{
			const char* eventIndexStr = attackIt->name.GetString();
			const int eventIndex = atoi(eventIndexStr);
			const Value& attackValue = attackIt->value;
			const EntTemplate::TraceHullAttack* existingAttack = entTemplate.GetTraceHullAttackForEvent(eventIndex);
			EntTemplate::TraceHullAttack traceHullAttack = existingAttack ? *existingAttack : EntTemplate::TraceHullAttack();
			UpdatePropertyFromJson(traceHullAttack.distance, attackValue, "distance");

			HandleJSONMember(attackValue, "height", [&traceHullAttack](const Value& value) {
				if (value.IsString())
				{
					const char* heightStr = value.GetString();
					if (*heightStr == '*')
					{
						traceHullAttack.height = atof(heightStr + 1);
						traceHullAttack.heightIsFactor = true;
					}
				}
				else if (value.IsNumber())
				{
					traceHullAttack.height = value.GetFloat();
					traceHullAttack.heightIsFactor = false;
				}
			});

			HandleJSONMember(attackValue, "punchangle", [&traceHullAttack](const Value& value) {
				UpdatePropertyFromJson(traceHullAttack.punchAngle.pitch, value, "pitch");
				UpdatePropertyFromJson(traceHullAttack.punchAngle.yaw, value, "yaw");
				UpdatePropertyFromJson(traceHullAttack.punchAngle.roll, value, "roll");
			});

			HandleJSONMember(attackValue, "knock", [&traceHullAttack](const Value& value) {
				UpdatePropertyFromJson(traceHullAttack.knock.forward, value, "forward");
				UpdatePropertyFromJson(traceHullAttack.knock.right, value, "right");
				UpdatePropertyFromJson(traceHullAttack.knock.up, value, "up");
				UpdatePropertyFromJson(traceHullAttack.knock.playerOnly, value, "player_only");
			});

			HandleJSONMember(attackValue, "damage_info", [&traceHullAttack](const Value& value) {
				EntTemplate::UpdateDamageInfoFromJSON(value, traceHullAttack.damageInfo);
			});

			UpdatePropertyFromJson(traceHullAttack.spawnBlood, attackValue, "spawn_blood");

			auto setSoundScript = [&](const char* propertyName, std::string& outStr)
			{
				HandleJSONMember(attackValue, propertyName, [&](const Value& value) {
					if (value.IsString())
					{
						std::string soundScriptName = value.GetString();
						entTemplate.AddPrecachedSoundScript(soundScriptName);
						outStr = soundScriptName;
					}
					else if (value.IsObject())
					{
						std::string soundScriptName = templateName + "#trace_hull_attacks#" + eventIndexStr + '#' + propertyName;
						_soundScriptSystem->AddSoundScriptFromJsonValue(soundScriptName.c_str(), value, CHAN_WEAPON);
						entTemplate.AddPrecachedSoundScript(soundScriptName);
						outStr = soundScriptName;
					}
				});
			};
			setSoundScript("hit_soundscript", traceHullAttack.hitSoundScript);
			setSoundScript("miss_soundscript", traceHullAttack.missSoundScript);

			entTemplate.SetTraceHullAttackForEvent(eventIndex, traceHullAttack);
		}
	});

	_entTemplates[templateName] = entTemplate;
}

const EntTemplate* EntTemplateSystem::GetTemplate(const char *name)
{
	if (!name || *name == '\0')
		return nullptr;
	_temp = name;
	auto it = _entTemplates.find(_temp);
	if (it != _entTemplates.end())
		return &it->second;
	return nullptr;
}

void EntTemplateSystem::EnsureVisualReplacementForTemplate(const char* templateName, const char* visualName)
{
	if (!templateName || *templateName == '\0')
		return;
	_temp = templateName;
	auto it = _entTemplates.find(_temp);
	if (it != _entTemplates.end())
	{
		EntTemplate* entTemplate = &it->second;
		if (!entTemplate->GetVisualNameOverride(visualName))
		{
			std::string replacement = GenerateResourceName(it->first, visualName);
			entTemplate->SetVisualReplacement(visualName, replacement);
			_visualSystem->EnsureVisualExists(replacement);
		}
	}
}

void EntTemplateSystem::EnsureSoundScriptReplacementForTemplate(const char* templateName, const char* soundScriptName)
{
	if (!templateName || *templateName == '\0')
		return;
	_temp = templateName;
	auto it = _entTemplates.find(_temp);
	if (it != _entTemplates.end())
	{
		EntTemplate* entTemplate = &it->second;
		if (!entTemplate->GetSoundScriptNameOverride(soundScriptName))
		{
			std::string replacement = GenerateResourceName(it->first, soundScriptName);
			entTemplate->SetSoundScriptReplacement(soundScriptName, replacement);
			_soundScriptSystem->EnsureSoundScriptExists(replacement);
		}
	}
}

EntTemplateSystem g_EntTemplateSystem;
