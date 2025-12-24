#include "skilldata.h"
#include "skill.h"

SkillData g_SkillData;

optional<float> SkillVariable::ValueForSkillLevel(int level) const
{
	if (level >= SKILL_HARD)
	{
		if (_hard.has_value())
		{
			return _hard;
		}
	}
	else if (level == SKILL_MEDIUM)
	{
		if (_medium.has_value())
		{
			return _medium;
		}
	}
	else if (level <= SKILL_EASY)
	{
		if (_easy.has_value())
		{
			return _easy;
		}
	}
	return _common;
}

void SkillData::SetVariableValue(const char* name, int category, float value)
{
	SetVariableValue(std::string(name), category, value);
}

void SkillData::SetVariableValue(std::string&& key, int category, float value)
{
	if (key.compare(0, 3, "sk_") == 0)
	{
		key.erase(0, 3);
	}

	if (key.empty())
		return;

	auto it = _map.find(key);
	SkillVariable* variable = nullptr;
	if (it != _map.end())
	{
		variable = &it->second;
	}
	else
	{
		auto inserted = _map.insert({std::move(key), SkillVariable()});
		variable = &inserted.first->second;
	}
	variable->SetValue(category, value);
}

const SkillVariable* SkillData::GetSkillVariable(const char *name) const
{
	return const_cast<SkillData*>(this)->AccessSkillVariable(name);
}

SkillVariable* SkillData::AccessSkillVariable(const char *name, bool createIfNotExist)
{
	if (strncmp(name, "sk_", 3) == 0)
	{
		name += 3;
	}
	if (*name == '\0')
		return nullptr;

	keytype key{name};
	auto it = _map.find(key);
	if (it != _map.end())
	{
		return &it->second;
	}
	else if (createIfNotExist)
	{
		auto inserted = _map.insert({std::move(key), SkillVariable()});
		return &inserted.first->second;
	}
	return nullptr;
}

float SkillData::GetValueForSkillLevel(const char *name, int level) const
{
	const SkillVariable* variable = GetSkillVariable(name);
	if (!variable)
		return 0.0f;

	auto getForCategory = [](const SkillVariable* variable, int category)
	{
		optional<float> value;
		if (variable->HasValue(category))
		{
			value = variable->GetValue(category);
		}
		else if (variable->HasValue(SkillVariable::COMMON))
		{
			value = variable->GetValue(SkillVariable::COMMON);
		}
		return value;
	};

	int limiter = 0;

	int category;
	if (level <= SKILL_EASY)
		category = SkillVariable::EASY;
	else if (level == SKILL_MEDIUM)
		category = SkillVariable::MEDIUM;
	else if (level >= SKILL_HARD)
		category = SkillVariable::HARD;

	do {
		optional<float> value = getForCategory(variable, category);
		if (value.has_value())
		{
			return *value;
		}
		else
		{
			const std::string& fallback = variable->Fallback();
			if (!fallback.empty())
			{
				variable = GetSkillVariable(fallback.c_str());
				if (!variable)
					break;
			}
			else
			{
				break;
			}
		}
		limiter++;
	} while(limiter <= 2);

	return 0.0f;
}

void SkillData::ProvideFallback(const char *name, const char *fallback)
{
	SkillVariable* variable = AccessSkillVariable(name, true);
	if (variable)
		variable->SetFallback(fallback);
}

void SkillData::ProvideFallback(const char *name, float fallback)
{
	SkillVariable* variable = AccessSkillVariable(name, true);

	if (variable && !variable->HasValue(SkillVariable::COMMON))
		variable->SetValue(SkillVariable::COMMON, fallback);
}

void SkillData::ProvideFallback(const char *name, float fallbackOnEasy, float fallbackOnMedium, float fallbackOnHard)
{
	SkillVariable* variable = AccessSkillVariable(name, true);

	if (variable && !variable->HasValue(SkillVariable::COMMON))
	{
		if (!variable->HasValue(SkillVariable::EASY))
			variable->SetValue(SkillVariable::EASY, fallbackOnEasy);
		if (!variable->HasValue(SkillVariable::MEDIUM))
			variable->SetValue(SkillVariable::MEDIUM, fallbackOnMedium);
		if (!variable->HasValue(SkillVariable::HARD))
			variable->SetValue(SkillVariable::HARD, fallbackOnHard);
	}
}

void SkillData::ProvideFallbackWithFactor(const char *name, const char *fallback, float factor)
{
	SkillVariable* variable = AccessSkillVariable(name, true);
	if (variable)
		variable->SetFallbackWithMultiplier(fallback, factor);
}

void SkillData::ForceValue(const char *name, float value)
{
	SkillVariable* variable = AccessSkillVariable(name, true);
	if (variable)
	{
		variable->Reset();
		variable->SetValue(SkillVariable::COMMON, value);
	}
}
