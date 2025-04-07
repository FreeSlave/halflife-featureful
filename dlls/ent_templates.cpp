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

bool EntTemplateSystem::ReadFromDocument(rapidjson::Document& document, const char* fileName)
{
	for (auto templateIt = document.MemberBegin(); templateIt != document.MemberEnd(); ++templateIt)
	{
		const char* name = templateIt->name.GetString();
		Value& value = templateIt->value;
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

static bool UpdateSizesFromJSON(rapidjson::Value& value, Vector& mins, Vector& maxs)
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

bool EntTemplate::UpdateDamageInfoFromJSON(rapidjson::Value &value, DamageInfo &damageInfo)
{
	UpdatePropertyFromJson(damageInfo.damage, value, "damage");

	{
		auto it = value.FindMember("type");
		if (it != value.MemberEnd())
		{
			int damageType = 0;
			if (it->value.IsArray())
			{
				Value::Array arr = it->value.GetArray();
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
				const char* damageTypeName = it->value.GetString();
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
		}
	}

	{
		auto it = value.FindMember("type_policy");
		if (it != value.MemberEnd())
		{
			const char* typePolicyName = it->value.GetString();
			if (strcmp(typePolicyName, "add") == 0)
			{
				damageInfo.typePolicy = EntTemplate::DamageInfo::ADD_DAMAGE_TYPE;
			}
			else if (strcmp(typePolicyName, "replace") == 0)
			{
				damageInfo.typePolicy = EntTemplate::DamageInfo::REPLACE_DAMAGE_TYPE;
			}
		}
	}

	UpdatePropertyFromJson(damageInfo.nonLethal, value, "nonlethal");
	UpdatePropertyFromJson(damageInfo.ignoreArmor, value, "ignore_armor");

	{
		auto it = value.FindMember("gib");
		if (it != value.MemberEnd())
		{
			const char* gibPolicyName = it->value.GetString();
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
		}
	}

	return true;
}

bool EntTemplateSystem::AddTemplateFromJsonValue(Value& allTemplatesJsonValue, const char* name, Value& value, const char* fileName, std::vector<std::string> inheritanceChain)
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

void EntTemplateSystem::AddTemplateFromJsonValue(const char* name, rapidjson::Value& value, const char* fileName)
{
	const std::string templateName = name;

	EntTemplate entTemplate;

	auto inheritsIt = value.FindMember("inherits");
	if (inheritsIt != value.MemberEnd())
	{
		const char* parentName = inheritsIt->value.GetString();
		auto existingTemplateIt = _entTemplates.find(parentName);
		if (existingTemplateIt != _entTemplates.end())
		{
			entTemplate = existingTemplateIt->second;
		}
		else
		{
			g_errorCollector.AddFormattedError("%s: couldn't find a parent entity template '%s' for '%s'", fileName, parentName, name);
		}
	}

	AddTemplateFromJsonValueImpl(templateName, value, entTemplate);
}

void EntTemplateSystem::AddTemplateFromJsonValueImpl(const std::string& templateName, rapidjson::Value& value, EntTemplate& entTemplate)
{
	{
		auto it = value.FindMember("own_visual");
		if (it != value.MemberEnd())
		{
			if (it->value.IsString())
			{
				entTemplate.SetOwnVisualName(it->value.GetString());
			}
			else if (it->value.IsObject())
			{
				std::string ownVisualName = templateName + "##own_visual";
				entTemplate.SetOwnVisualName(ownVisualName);
				_visualSystem->AddVisualFromJsonValue(ownVisualName.c_str(), it->value);
			}
		}
	}

	{
		auto it = value.FindMember("gib_visual");
		if (it != value.MemberEnd())
		{
			if (it->value.IsString())
			{
				entTemplate.SetGibVisualName(it->value.GetString());
			}
			else if (it->value.IsObject())
			{
				std::string gibVisualName = templateName + "##gib_visual";
				entTemplate.SetGibVisualName(gibVisualName);
				_visualSystem->AddVisualFromJsonValue(gibVisualName.c_str(), it->value);
			}
		}
	}

	{
		auto it = value.FindMember("soundscripts");
		if (it != value.MemberEnd())
		{
			Value& soundScriptsObj = it->value;
			for (auto scriptIt = soundScriptsObj.MemberBegin(); scriptIt != soundScriptsObj.MemberEnd(); ++scriptIt)
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
		}
	}

	{
		auto it = value.FindMember("visuals");
		if (it != value.MemberEnd())
		{
			Value& visualsObj = it->value;
			for (auto visualIt = visualsObj.MemberBegin(); visualIt != visualsObj.MemberEnd(); ++visualIt)
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
		}
	}

	{
		auto it = value.FindMember("sound_replacement");
		if (it != value.MemberEnd())
		{
			Value& soundReplacement = it->value;
			for (auto sIt = soundReplacement.MemberBegin(); sIt != soundReplacement.MemberEnd(); ++sIt)
			{
				const char* originalSample = sIt->name.GetString();
				const char* replacementSample = sIt->value.GetString();
				entTemplate.SetSoundReplacement(originalSample, replacementSample);
			}
		}
	}

	{
		auto it = value.FindMember("precached_sounds");
		if (it != value.MemberEnd())
		{
			Value::Array arr = it->value.GetArray();
			std::vector<std::string> sounds;
			for (size_t i=0; i<arr.Size(); ++i)
			{
				sounds.push_back(arr[i].GetString());
			}
			entTemplate.SetPrecachedSounds(std::move(sounds));
		}
	}

	{
		auto it = value.FindMember("precached_soundscripts");
		if (it != value.MemberEnd())
		{
			Value::Array arr = it->value.GetArray();
			std::vector<std::string> soundsScripts;
			for (size_t i=0; i<arr.Size(); ++i)
			{
				soundsScripts.push_back(arr[i].GetString());
			}
			entTemplate.SetPrecachedSoundScripts(std::move(soundsScripts));
		}
	}

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

	{
		auto it = value.FindMember("classify");
		if (it != value.MemberEnd())
		{
			const char* classifyName = it->value.GetString();
			int classify = ClassifyFromName(classifyName);
			if (classify < 0)
			{
				LOG("Unknown classification '%s'\n", classifyName);
			}
			else
			{
				entTemplate.SetClassify(classify);
			}
		}
	}

	{
		auto it = value.FindMember("blood");
		if (it != value.MemberEnd())
		{
			const char* bloodType = it->value.GetString();
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
		}
	}

	{
		auto it = value.FindMember("health");
		if (it != value.MemberEnd())
		{
			entTemplate.SetHealth(it->value.GetFloat());
		}
	}

	{
		auto it = value.FindMember("field_of_view");
		if (it != value.MemberEnd())
		{
			if (it->value.IsNumber())
			{
				entTemplate.SetFieldOfView(it->value.GetFloat());
			}
			else if (it->value.IsString())
			{
				const char* fovType = it->value.GetString();
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
		}
	}

	{
		auto it = value.FindMember("size");
		if (it != value.MemberEnd())
		{
			Vector mins, maxs;
			if (UpdateSizesFromJSON(it->value, mins, maxs))
			{
				entTemplate.SetSize(mins, maxs);
			}
		}
	}

	{
		auto it = value.FindMember("collision_box");
		if (it != value.MemberEnd())
		{
			Vector mins, maxs;
			if (UpdateSizesFromJSON(it->value, mins, maxs))
			{
				entTemplate.SetCollisionBox(mins, maxs);
			}
		}
	}

	{
		auto it = value.FindMember("size_for_grapple");
		if (it != value.MemberEnd())
		{
			const char* targetType = it->value.GetString();
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
		}
	}

	{
		auto it = value.FindMember("speech_prefix");
		if (it != value.MemberEnd())
		{
			entTemplate.SetSpeechPrefix(it->value.GetString());
		}
	}

	{
		auto it = value.FindMember("squad_capability");
		if (it != value.MemberEnd())
		{
			if (it->value.IsBool())
			{
				SquadCapabilities caps = entTemplate.GetSquadCapabilities();
				caps.canRecruit = it->value.GetBool();
				entTemplate.SetSquadCapabilities(caps);
			}
			else if (it->value.IsObject())
			{
				Value& obj = it->value;
				SquadCapabilities caps = entTemplate.GetSquadCapabilities();
				UpdatePropertyFromJson(caps.canRecruit, obj, "can_recruit");
				UpdatePropertyFromJson(caps.denyRecruiting, obj, "deny_recruiting");
				UpdatePropertyFromJson(caps.allowDifferentClassification, obj, "allow_different_classification");
				UpdatePropertyFromJson(caps.requireSameClassname, obj, "require_same_classname");
				UpdatePropertyFromJson(caps.requireSameEntTemplate, obj, "require_same_ent_template");
				entTemplate.SetSquadCapabilities(caps);
			}
		}
	}

	{
		auto it = value.FindMember("open_door_capability");
		if (it != value.MemberEnd())
		{
			entTemplate.SetCanOpenDoors(it->value.GetBool());
		}
	}

	{
		auto it = value.FindMember("check_melee_attack1");
		if (it != value.MemberEnd())
		{
			Value& obj = it->value;
			EntTemplate::CheckMeleeAttack check = entTemplate.GetCheckMeleeAttack1();
			UpdatePropertyFromJson(check.distance, obj, "distance");
			UpdatePropertyFromJson(check.dot, obj, "dot");
			entTemplate.SetCheckMeleeAttack1(check);
		}
	}

	{
		auto it = value.FindMember("check_melee_attack2");
		if (it != value.MemberEnd())
		{
			Value& obj = it->value;
			EntTemplate::CheckMeleeAttack check = entTemplate.GetCheckMeleeAttack2();
			UpdatePropertyFromJson(check.distance, obj, "distance");
			UpdatePropertyFromJson(check.dot, obj, "dot");
			entTemplate.SetCheckMeleeAttack2(check);
		}
	}

	{
		auto it = value.FindMember("trace_hull_attacks");
		if (it != value.MemberEnd())
		{
			Value& attacks = it->value;
			for (auto attackIt = attacks.MemberBegin(); attackIt != attacks.MemberEnd(); ++attackIt)
			{
				const char* eventIndexStr = attackIt->name.GetString();
				const int eventIndex = atoi(eventIndexStr);
				Value& attackValue = attackIt->value;
				const EntTemplate::TraceHullAttack* existingAttack = entTemplate.GetTraceHullAttackForEvent(eventIndex);
				EntTemplate::TraceHullAttack traceHullAttack = existingAttack ? *existingAttack : EntTemplate::TraceHullAttack();
				UpdatePropertyFromJson(traceHullAttack.distance, attackValue, "distance");

				auto heightIt = attackValue.FindMember("height");
				if (heightIt != attackValue.MemberEnd())
				{
					if (heightIt->value.IsString())
					{
						const char* heightStr = heightIt->value.GetString();
						if (*heightStr == '*')
						{
							traceHullAttack.height = atof(heightStr + 1);
							traceHullAttack.heightIsFactor = true;
						}
					}
					else if (heightIt->value.IsNumber())
					{
						traceHullAttack.height = heightIt->value.GetFloat();
						traceHullAttack.heightIsFactor = false;
					}
				}

				auto punchAngleIt = attackValue.FindMember("punchangle");
				if (punchAngleIt != attackValue.MemberEnd())
				{
					Value& punchAngleValue = punchAngleIt->value;
					UpdatePropertyFromJson(traceHullAttack.punchAngle.pitch, punchAngleValue, "pitch");
					UpdatePropertyFromJson(traceHullAttack.punchAngle.yaw, punchAngleValue, "yaw");
					UpdatePropertyFromJson(traceHullAttack.punchAngle.roll, punchAngleValue, "roll");
				}

				auto knockIt = attackValue.FindMember("knock");
				if (knockIt != attackValue.MemberEnd())
				{
					Value& knockValue = knockIt->value;
					UpdatePropertyFromJson(traceHullAttack.knock.forward, knockValue, "forward");
					UpdatePropertyFromJson(traceHullAttack.knock.right, knockValue, "right");
					UpdatePropertyFromJson(traceHullAttack.knock.up, knockValue, "up");
					UpdatePropertyFromJson(traceHullAttack.knock.playerOnly, knockValue, "player_only");
				}

				auto damageInfoIt = attackValue.FindMember("damage_info");
				if (damageInfoIt != attackValue.MemberEnd())
				{
					EntTemplate::UpdateDamageInfoFromJSON(damageInfoIt->value, traceHullAttack.damageInfo);
				}

				UpdatePropertyFromJson(traceHullAttack.spawnBlood, attackValue, "spawn_blood");

				auto setSoundScript = [&](const char* propertyName, std::string& outStr)
				{
					auto it = attackValue.FindMember(propertyName);
					if (it != attackValue.MemberEnd())
					{
						if (it->value.IsString())
						{
							std::string soundScriptName = it->value.GetString();
							entTemplate.AddPrecachedSoundScript(soundScriptName);
							outStr = soundScriptName;
						}
						else if (it->value.IsObject())
						{
							std::string soundScriptName = templateName + "#trace_hull_attacks#" + eventIndexStr + '#' + propertyName;
							_soundScriptSystem->AddSoundScriptFromJsonValue(soundScriptName.c_str(), it->value, CHAN_WEAPON);
							entTemplate.AddPrecachedSoundScript(soundScriptName);
							outStr = soundScriptName;
						}
					}
				};
				setSoundScript("hit_soundscript", traceHullAttack.hitSoundScript);
				setSoundScript("miss_soundscript", traceHullAttack.missSoundScript);

				entTemplate.SetTraceHullAttackForEvent(eventIndex, traceHullAttack);
			}
		}
	}

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
