#include "ent_templates.h"
#include "icase_compare.h"

#include "blood_types.h"
#include "classify.h"
#include "grapple_target.h"
#include "hull_sizes.h"
#include "dmg_types.h"
#include "gib.h"
#include "hitgroup.h"

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

static DamageTypeMatch ParseDamageTypeMatch(const char* str)
{
	if (stricmp(str, "one") == 0)
		return DamageTypeMatch::ONE;
	else if (stricmp(str, "all") == 0)
		return DamageTypeMatch::ALL;
	else if (stricmp(str, "none") == 0)
		return DamageTypeMatch::NONE;
	else if (stricmp(str, "exact") == 0)
		return DamageTypeMatch::EXACT;
	return DamageTypeMatch::INVALID;
}

static std::pair<ValueComparison, float> ParseValueComparison(const char* str)
{
	if (strncmp(str, "<=", 2) == 0)
		return std::make_pair(ValueComparison::LESS_OR_EQUAL, (float)atof(str+2));
	else if (strncmp(str, ">=", 2) == 0)
		return std::make_pair(ValueComparison::GREATER_OR_EQUAL, (float)atof(str+2));
	else if (strncmp(str, "<", 1) == 0)
		return std::make_pair(ValueComparison::LESS, (float)atof(str+1));
	else if (strncmp(str, ">", 1) == 0)
		return std::make_pair(ValueComparison::GREATER, (float)atof(str+1));
	return std::make_pair(ValueComparison::UNKNOWN, 0.0f);
}

DropItemSet DropItemSet::FromJSON(const Value &value)
{
	DropItemSet result;

	auto readItemList = [](const Value& value) {
		std::vector<DropItemInfo> items;
		Value::ConstArray arr = value.GetArray();
		items.reserve(arr.Size());

		for (auto& item : arr)
		{
			DropItemInfo itemInfo;

			if (item.IsString())
			{
				itemInfo.classname = item.GetString();
			}
			else
			{
				UpdatePropertyFromJson(itemInfo.classname, item, "classname");
				UpdatePropertyFromJson(itemInfo.entTemplate, item, "ent_template");
				UpdatePropertyFromJson(itemInfo.pickupName, item, "pickup_name");
				UpdatePropertyFromJson(itemInfo.chance, item, "chance");
				UpdatePropertyFromJson(itemInfo.weight, item, "weight");
			}
			items.push_back(std::move(itemInfo));
		}

		return std::move(items);
	};

	if (value.IsArray())
	{
		result.items = readItemList(value);
	}
	else
	{
		HandleJSONMember(value, "items", [&result, &readItemList](const Value& value) {
			result.items = readItemList(value);
		});
		UpdatePropertyFromJson(result.maxWeight, value, "max_weight");
	}

	return result;
}

ChildVariant ChildVariant::FromJSON(const rapidjson::Value& value)
{
	ChildVariant result;
	UpdatePropertyFromJson(result.classname, value, "classname");
	UpdatePropertyFromJson(result.chance, value, "chance");

	HandleJSONMember(value, "parameters", [&result](const Value& value) {
		for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
		{
			const char* name = it->name.GetString();
			const Value& paramValue = it->value;
			if (paramValue.IsString())
				result.parameters[name] = paramValue.GetString();
			else if (paramValue.IsInt())
				result.parameters[name] = std::to_string(paramValue.GetInt());
			else if (paramValue.IsFloat())
				result.parameters[name] = std::to_string(paramValue.GetFloat());
		}
	});

	return result;
}

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
	if (_soundScripts.empty())
		return nullptr;
	tempString = name;
	auto it = _soundScripts.find(tempString);
	if (it != _soundScripts.end())
		return it->second.c_str();
	return nullptr;
}

const char* EntTemplate::GetVisualNameOverride(const char *name) const
{
	if (_visuals.empty())
		return nullptr;
	tempString = name;
	auto it = _visuals.find(tempString);
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

std::pair<std::vector<EntTemplate::TraceAttackRule>::const_iterator, std::vector<EntTemplate::TraceAttackRule>::const_iterator> EntTemplate::TraceAttackRulesRange() const
{
	return std::make_pair(_traceAttackRules.cbegin(), _traceAttackRules.cend());
}

void EntTemplate::SetTraceAttackRules(std::vector<EntTemplate::TraceAttackRule>&& traceAttackRules)
{
	_traceAttackRules = traceAttackRules;
	_traceAttackRulesDefined = true;
}

std::pair<std::vector<EntTemplate::TakeDamageRule>::const_iterator, std::vector<EntTemplate::TakeDamageRule>::const_iterator> EntTemplate::TakeDamageRulesRange() const
{
	return std::make_pair(_takeDamageRules.cbegin(), _takeDamageRules.cend());
}

void EntTemplate::SetTakeDamageRules(std::vector<EntTemplate::TakeDamageRule>&& takeDamageRules)
{
	_takeDamageRules = takeDamageRules;
	_takeDamageRulesDefined = true;
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

int EntTemplate::ParseGibPolicy(const char *gibPolicyName)
{
	if (stricmp(gibPolicyName, "always") == 0)
	{
		return GIB_ALWAYS;
	}
	else if (stricmp(gibPolicyName, "never") == 0)
	{
		return GIB_NEVER;
	}
	return GIB_NORMAL;
}

int EntTemplate::DamageTypeFromJSON(const Value& value)
{
	return JSONStringSetToFlags(value, [](const char* damageTypeName) {
		int subType = ParseDamageType(damageTypeName);
		if (subType >= 0)
		{
			return subType;
		}
		else
		{
			LOG_WARNING("Unknown damage type '%s'\n", damageTypeName);
			return 0;
		}
	});
}

bool EntTemplate::UpdateDamageInfoFromJSON(const rapidjson::Value &value, DamageInfo &damageInfo)
{
	UpdatePropertyFromJson(damageInfo.damage, value, "damage");

	HandleJSONMember(value, "type", [&damageInfo](const Value& value) {
		damageInfo.type = DamageTypeFromJSON(value);
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
		damageInfo.gibPolicy = ParseGibPolicy(value.GetString());
	});

	return true;
}

EntityFilter EntTemplate::EntityFilterFromJSON(const Value &value)
{
	EntityFilter filter;

	HandleJSONMember(value, "classname", [&filter](const Value& value) {
		auto handleClassname = [&filter](const char* classname)
		{
			if (stricmp(classname, "same") == 0)
			{
				filter.sameClassname = true;
			}
			else
			{
				filter.classnames.push_back(classname);
			}
		};

		if (value.IsArray())
		{
			Value::ConstArray arr = value.GetArray();
			filter.classnames.reserve(arr.Size());
			for (auto& item : arr)
			{
				handleClassname(item.GetString());
			}
		}
		else
		{
			handleClassname(value.GetString());
		}
	});

	HandleJSONMember(value, "ent_template", [&filter](const Value& value) {
		auto handleEntTemplate = [&filter](const char* entTemplate)
		{
			if (stricmp(entTemplate, "same") == 0)
			{
				filter.sameEntTemplate = true;
			}
			else
			{
				filter.entTemplates.push_back(entTemplate);
			}
		};

		if (value.IsArray())
		{
			Value::ConstArray arr = value.GetArray();
			filter.entTemplates.reserve(arr.Size());
			for (auto& item : arr)
			{
				handleEntTemplate(item.GetString());
			}
		}
		else
		{
			handleEntTemplate(value.GetString());
		}
	});

	HandleJSONMember(value, "classify", [&filter](const Value& value) {
		auto handleClassify = [&filter](const char* classifyName)
		{
			if (stricmp(classifyName, "same") == 0)
			{
				filter.sameClassify = true;
			}
			else
			{
				int classify = ClassifyFromName(classifyName);
				if (classify >= 0)
					filter.classifications.push_back(classify);
				else
					LOG("Unknown classification '%s'\n", classifyName);
			}
		};

		if (value.IsArray())
		{
			Value::ConstArray arr = value.GetArray();
			filter.classifications.reserve(arr.Size());
			for (auto& item : arr)
			{
				handleClassify(item.GetString());
			}
		}
		else
		{
			handleClassify(value.GetString());
		}
	});

	UpdatePropertyFromJson(filter.isCombatCharacter, value, "is_combat_character");

	HandleJSONMember(value, "life_state", [&filter](const Value& value) {
		filter.lifeState = static_cast<EntityFilter::LifeState>(JSONStringSetToFlags(value, [](const char* name) {
			if (stricmp(name, "alive") == 0)
				return EntityFilter::LifeState::ALIVE;
			else if (stricmp(name, "dead") == 0)
				return EntityFilter::LifeState::DEAD;
			else if (stricmp(name, "dying") == 0)
				return EntityFilter::LifeState::DYING;
			return EntityFilter::LifeState::ANY_LIFESTATE;
		}));
	});

	HandleJSONMember(value, "body", [&filter](const Value& value) {
		auto readBodyFilter = [](const Value& value) {
			if (value.IsObject())
			{
				int bodyGroup = value.FindMember("bodygroup")->value.GetInt();
				int submodel = value.FindMember("submodel")->value.GetInt();
				return EntityFilter::BodyFilter(bodyGroup, submodel);
			}
			else
			{
				return EntityFilter::BodyFilter(value.GetInt());
			}
		};

		if (value.IsArray())
		{
			Value::ConstArray arr = value.GetArray();
			filter.bodyFilter.reserve(arr.Size());

			for (auto& item : arr)
			{
				filter.bodyFilter.push_back(readBodyFilter(item));
			}
		}
		else
		{
			filter.bodyFilter.push_back(readBodyFilter(value));
		}
	});

	UpdatePropertyFromJson(filter.invertBodyCheck, value, "invert_body_check");

	UpdatePropertyFromJson(filter.negate, value, "negate");

	return filter;
}

void EntTemplate::DamageConditions::UpdateFromJSON(const Value &value)
{
	HandleJSONMember(value, "dmg_type", [this](const Value& value) {
		dmgType = DamageTypeFromJSON(value);
	});

	HandleJSONMember(value, "dmg_type_match", [this](const Value& value) {
		auto dmgTypeMatchResult = ParseDamageTypeMatch(value.GetString());
		if (dmgTypeMatchResult != DamageTypeMatch::INVALID)
			dmgTypeMatch = dmgTypeMatchResult;
	});

	HandleJSONMember(value, "dmg", [this](const Value& value) {
		auto result = ParseValueComparison(value.GetString());
		dmgComparison = result.first;
		dmg = result.second;
	});

	HandleJSONMember(value, "inflictor", [this](const Value& value) {
		inflictorFilter = EntityFilterFromJSON(value);
	});

	HandleJSONMember(value, "attacker", [this](const Value& value) {
		attackerFilter = EntityFilterFromJSON(value);
	});

	HandleJSONMember(value, "self", [this](const Value& value) {
		selfFilter = EntityFilterFromJSON(value);
	});

	HandleJSONMember(value, "attack_affinity", [this](const Value& value) {
		attackAffinity = JSONStringSetToFlags(value, [](const char* str) {
			if (stricmp(str, "enemy") == 0)
			{
				return ENEMY;
			}
			else if (stricmp(str, "friendly") == 0)
			{
				return FRIENDLY;
			}
			else if (stricmp(str, "self") == 0)
			{
				return SELF;
			}
			else if (stricmp(str, "neutral") == 0)
			{
				return NEUTRAL;
			}
			return ANY_SOURCE;
		});
	});

	HandleJSONMember(value, "gib", [this](const Value& value) {
		gibPolicy = ParseGibPolicy(value.GetString());
	});
}

void EntTemplate::DamageInfoModifier::UpdateFromJSON(const Value &value)
{
	HandleJSONMember(value, "dmg", [this](const Value& value) {
		const char* str = value.GetString();

		if (strncmp(str, "=", 1) == 0)
		{
			dmgModifier = ValueModifier::SET;
		}
		else if (strncmp(str, "*", 1) == 0)
		{
			dmgModifier = ValueModifier::FACTOR;
		}
		else if (strncmp(str, "-", 1) == 0)
		{
			dmgModifier = ValueModifier::SUBSTRUCT;
		}
		else if (strncmp(str, "+", 1) == 0)
		{
			dmgModifier = ValueModifier::ADD;
		}
		else
			return;
		str++;
		if (strcmp(str, "health") == 0)
		{
			useHealthAsDmg = true;
		}
		else
		{
			dmg = atof(str);
		}
	});

	UpdatePropertyFromJson(dmgMinThreshold, value, "dmg_min_threshold");
	UpdatePropertyFromJson(skip, value, "skip_damage");
	UpdatePropertyFromJson(noBlood, value, "no_blood");

	HandleJSONMember(value, "gib", [this](const Value& value) {
		gibPolicy = ParseGibPolicy(value.GetString());
	});
}

void EntTemplate::TraceAttackRule::Conditions::UpdateFromJSON(const Value &value)
{
	DamageConditions::UpdateFromJSON(value);

	HandleJSONMember(value, "hitgroup", [this](const Value& value) {
		hitgroups = HitgroupSetFromJSON(value);
	});

	UpdatePropertyFromJson(invertHitgroupCheck, value, "invert_hitgroup_check");
}

void EntTemplate::TraceAttackRule::Modifier::UpdateFromJSON(const Value &value)
{
	DamageInfoModifier::UpdateFromJSON(value);

	HandleJSONMember(value, "hitgroup", [this](const Value& value) {
		hitgroup = HitgroupFromJSON(value);
	});
}

int EntTemplate::HitgroupFromJSON(const Value &value)
{
	if (value.IsInt())
	{
		return value.GetInt();
	}
	else if (value.IsString())
	{
		const char* str = value.GetString();
		if (stricmp(str, "generic") == 0)
			return HITGROUP_GENERIC;
		else if (stricmp(str, "head") == 0)
			return HITGROUP_HEAD;
		else if (stricmp(str, "chest") == 0)
			return HITGROUP_CHEST;
		else if (stricmp(str, "stomach") == 0)
			return HITGROUP_STOMACH;
		else if (stricmp(str, "left_arm") == 0 || stricmp(str, "left arm") == 0)
			return HITGROUP_LEFTARM;
		else if (stricmp(str, "right_arm") == 0 || stricmp(str, "right arm") == 0)
			return HITGROUP_RIGHTARM;
		else if (stricmp(str, "left_leg") == 0 || stricmp(str, "left leg") == 0)
			return HITGROUP_LEFTLEG;
		else if (stricmp(str, "right_leg") == 0 || stricmp(str, "right leg") == 0)
			return HITGROUP_RIGHTLEG;
	}
	return -1;
}

std::vector<int> EntTemplate::HitgroupSetFromJSON(const Value &value)
{
	std::vector<int> vec;

	if (value.IsArray())
	{
		Value::ConstArray arr = value.GetArray();
		vec.reserve(arr.Size());
		for (auto& item : arr)
		{
			const int hitgroup = HitgroupFromJSON(item);
			if (hitgroup >= 0)
				vec.push_back(hitgroup);
		}
	}
	else
	{
		const int hitgroup = HitgroupFromJSON(value);
		if (hitgroup >= 0)
			vec.push_back(hitgroup);
	}

	return vec;
}

EntTemplate::TraceAttackRule EntTemplate::TraceAttackRule::FromJSON(const Value &value)
{
	TraceAttackRule rule;

	HandleJSONMember(value, "conditions", [&rule](const Value& value) {
		rule.conditions.UpdateFromJSON(value);
	});

	HandleJSONMember(value, "modifier", [&rule](const Value& value) {
		rule.modifier.UpdateFromJSON(value);
	});

	auto parseEffects = [](const Value& value)
	{
		TraceAttackRule::Effects effects;

		HandleJSONMember(value, "ricochet", [&effects](const Value& value) {
			TraceAttackRule::Effects::Ricochet ricochet;

			UpdatePropertyFromJson(ricochet.chance, value, "chance");
			UpdatePropertyFromJson(ricochet.scale, value, "scale");
			UpdatePropertyFromJson(ricochet.certainOnNewFrame, value, "certain_on_new_frame");

			effects.ricochet = ricochet;
		});

		HandleJSONMember(value, "tracer", [&effects](const Value& value) {
			TraceAttackRule::Effects::Tracer tracer;

			UpdatePropertyFromJson(tracer.chance, value, "chance");
			UpdatePropertyFromJson(tracer.certainOnNewFrame, value, "certain_on_new_frame");
			UpdatePropertyFromJson(tracer.variance, value, "variance");

			effects.tracer = tracer;
		});

		return effects;
	};

	HandleJSONMember(value, "effects", [&rule, &parseEffects](const Value& value) {
		rule.effects = parseEffects(value);
	});

	HandleJSONMember(value, "threshold_effects", [&rule, &parseEffects](const Value& value) {
		rule.thresholdEffects = parseEffects(value);
	});

	return rule;
}

EntTemplate::TakeDamageRule EntTemplate::TakeDamageRule::FromJSON(const Value &value)
{
	TakeDamageRule rule;

	HandleJSONMember(value, "conditions", [&rule](const Value& value) {
		rule.conditions.UpdateFromJSON(value);
	});

	HandleJSONMember(value, "modifier", [&rule](const Value& value) {
		rule.modifier.UpdateFromJSON(value);
	});

	return rule;
}

void EntTemplate::UpdatePainSoundRule(::PainSoundRule &rule) const
{
	if (_painSoundRule.delay.has_value())
	{
		rule.delay = *_painSoundRule.delay;
	}
	if (_painSoundRule.chance.has_value())
	{
		rule.chance = *_painSoundRule.chance;
	}
	if (_painSoundRule.lowerBound.has_value())
	{
		rule.lowerBound = *_painSoundRule.lowerBound;
	}
	if (!indeterminate(_painSoundRule.allowWhenDying))
	{
		rule.allowWhenDying = (bool)_painSoundRule.allowWhenDying;
	}
}

void EntTemplate::SetSkillReplacement(const char* name, const SkillReplacement& replacement) {
	if (strncmp(name, "sk_", 3) == 0)
	{
		name += 3;
	}
	if (*name == '\0')
		return;
	_skillReplacements[name] = replacement;
}

const SkillReplacement* EntTemplate::GetSkillReplacement(const char* name) const {
	if (_skillReplacements.empty())
		return nullptr;
	if (strncmp(name, "sk_", 3) == 0)
	{
		name += 3;
	}
	if (*name == '\0')
		return nullptr;
	auto it = _skillReplacements.find(name);
	if (it != _skillReplacements.end())
	{
		return &it->second;
	}
	return nullptr;
}

void EntTemplate::SetDisplayName(std::string &&name)
{
	_displayName = std::move(name);
}

void EntTemplate::SetDisplayName(const char *name)
{
	if (name && *name)
		_displayName = name;
	else
		_displayName.clear();
}

const char* EntTemplate::GetDisplayName() const
{
	if (!_displayName.empty())
		return _displayName.c_str();
	return nullptr;
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
		for (auto& item : arr)
		{
			sounds.push_back(item.GetString());
		}
		entTemplate.SetPrecachedSounds(std::move(sounds));
	});

	HandleJSONMember(value, "precached_soundscripts", [&entTemplate](const Value& value) {
		Value::ConstArray arr = value.GetArray();
		std::vector<std::string> soundsScripts;
		soundsScripts.reserve(arr.Size());
		for (auto& item : arr)
		{
			soundsScripts.push_back(item.GetString());
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
		const char* bloodTypeName = value.GetString();
		const int bloodType = BloodTypeFromName(bloodTypeName);
		if (bloodType == BLOOD_COLOR_INVALID)
		{
			LOG_WARNING("Unknown blood type '%s'\n", bloodTypeName);
		}
		else
		{
			entTemplate.SetBloodColor(bloodType);
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

	HandleJSONMember(value, "touch_attack", [&entTemplate, this](const Value& value) {
		HandleJSONMember(value, "damage_info", [&entTemplate](const Value& value) {
			auto touchAttack = entTemplate.GetTouchAttack();
			EntTemplate::UpdateDamageInfoFromJSON(value, touchAttack.damageInfo);
			entTemplate.SetTouchAttack(touchAttack);
		});
	});

	HandleJSONMember(value, "take_damage", [&entTemplate](const Value& value) {
		std::vector<EntTemplate::TakeDamageRule> takeDamageRules;
		Value::ConstArray arr = value.GetArray();
		takeDamageRules.reserve(arr.Size());

		for (auto& item : arr)
		{
			takeDamageRules.push_back(EntTemplate::TakeDamageRule::FromJSON(item));
		}

		entTemplate.SetTakeDamageRules(std::move(takeDamageRules));
	});

	HandleJSONMember(value, "trace_attack", [&entTemplate](const Value& value) {
		std::vector<EntTemplate::TraceAttackRule> traceAttackRules;
		Value::ConstArray arr = value.GetArray();
		traceAttackRules.reserve(arr.Size());

		for (auto& item : arr)
		{
			traceAttackRules.push_back(EntTemplate::TraceAttackRule::FromJSON(item));
		}

		entTemplate.SetTraceAttackRules(std::move(traceAttackRules));
	});

	HandleJSONMember(value, "loot_drop", [&entTemplate](const Value& value) {
		entTemplate.SetLootDrop(DropItemSet::FromJSON(value));
	});

	HandleJSONMember(value, "children", [&entTemplate](const Value& value) {
		if (value.IsArray())
		{
			ChildrenInfo childrenInfo;
			Value::ConstArray arr = value.GetArray();
			childrenInfo.variants.reserve(arr.Size());

			for (auto& item : arr)
			{
				childrenInfo.variants.push_back(ChildVariant::FromJSON(item));
			}
			entTemplate.SetChildrenInfo(std::move(childrenInfo));
		}
		else if (value.IsObject())
		{
			ChildrenInfo childrenInfo;
			childrenInfo.variants.push_back(ChildVariant::FromJSON(value));
			entTemplate.SetChildrenInfo(std::move(childrenInfo));
		}
	});

	HandleJSONMember(value, "pain", [&entTemplate](const Value& value) {
		EntTemplate::PainSoundRule rule = entTemplate.GetPainSoundRule();
		UpdatePropertyFromJson(rule.delay, value, "delay");
		UpdatePropertyFromJson(rule.chance, value, "chance");
		UpdatePropertyFromJson(rule.lowerBound, value, "lower_bound_dmg");
		UpdatePropertyFromJson(rule.allowWhenDying, value, "allow_when_dying");
		entTemplate.SetPainSoundRule(rule);
	});

	HandleJSONMember(value, "skill", [&entTemplate](const Value& value) {
		for (auto skillIt = value.MemberBegin(); skillIt != value.MemberEnd(); ++skillIt)
		{
			const char* skillName = skillIt->name.GetString();
			const Value& skillValue = skillIt->value;

			if (*skillName)
			{
				SkillReplacement replacement;
				if (skillValue.IsString())
				{
					const char* str = skillValue.GetString();
					if (*str == '*')
					{
						const float multiplier = atof(str+1);
						replacement.easy = replacement.medium = replacement.hard = multiplier;
						replacement.type = SkillReplacement::MULTIPLIER;
					}
					else
					{
						replacement.replacement = skillValue.GetString();
						replacement.type = SkillReplacement::STRING;
					}
				}
				else if (skillValue.IsNumber())
				{
					replacement.easy = replacement.medium = replacement.hard = skillValue.GetFloat();
					replacement.type = SkillReplacement::COMMON;
				}
				else if (skillValue.IsArray())
				{
					Value::ConstArray arr = skillValue.GetArray();
					if (arr.Size() == 3)
					{
						replacement.type = SkillReplacement::DIFFICULTIES;
						replacement.easy = arr[0].GetFloat();
						replacement.medium = arr[1].GetFloat();
						replacement.hard = arr[2].GetFloat();
					}
				}
				entTemplate.SetSkillReplacement(skillName, replacement);
			}
		}
	});

	std::string displayName;
	if (UpdatePropertyFromJson(displayName, value, "displayname"))
	{
		entTemplate.SetDisplayName(std::move(displayName));
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
