#include "skilldata.h"
#include "skill.h"
#include "logger.h"
#include "parsetext.h"
#include "util_shared.h"

int g_iSkillLevel = SKILL_EASY;

SkillData g_SkillData;

optional<FloatRange> SkillVariable::ValueForSkillLevel(int level) const
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

void SkillData::SetVariableValue(const char* name, int category, FloatRange value)
{
	SetVariableValue(std::string(name), category, value);
}

void SkillData::SetVariableValue(std::string&& key, int category, FloatRange value)
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

FloatRange SkillData::GetValueForSkillLevel(const char *name, int level) const
{
	const SkillVariable* variable = GetSkillVariable(name);
	if (!variable)
		return 0.0f;

	auto getForCategory = [](const SkillVariable* variable, int category)
	{
		optional<FloatRange> value;
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
		optional<FloatRange> value = getForCategory(variable, category);
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

void SkillData::ProvideFallback(const char *name, FloatRange fallback)
{
	SkillVariable* variable = AccessSkillVariable(name, true);

	if (variable && !variable->HasValue(SkillVariable::COMMON))
		variable->SetValue(SkillVariable::COMMON, fallback);
}

void SkillData::ProvideFallback(const char *name, FloatRange fallbackOnEasy, FloatRange fallbackOnMedium, FloatRange fallbackOnHard)
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

void SkillData::ForceValue(const char *name, FloatRange value)
{
	SkillVariable* variable = AccessSkillVariable(name, true);
	if (variable)
	{
		variable->Reset();
		variable->SetValue(SkillVariable::COMMON, value);
	}
}

void ParseSkillCfg(unsigned char* pMemFile, int fileSize, const char* fileName)
{
	int filePos = 0;
	char buffer[512] = { 0 };
	int lineNum = 0;

	while(memfgets(pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1)) {
		lineNum++;
		int i = 0;
		SkipSpaceCharacters(buffer, i, sizeof(buffer));
		if (buffer[i] == '\0') // it's an empty line, skip
			continue;
		if (buffer[i] == '/') // it's a comment, skip
			continue;

		const int strStart = i;
		ConsumeNonSpaceCharacters(buffer, i, sizeof(buffer));
		int strEnd = i;

		if (strEnd - strStart <= 0)
			continue;

		const char lastDigit = buffer[strEnd-1];
		const bool lastIsDigit = isdigit(lastDigit);
		if (lastIsDigit)
		{
			strEnd--;
		}

		if (strEnd - strStart <= 0)
		{
			LOG_ERROR("%s: bad skill value name on line %d\n", fileName, lineNum);
			continue;
		}

		std::string skillName(buffer+strStart, buffer+strEnd);

		SkipSpaceCharacters(buffer, i, sizeof(buffer));

		int valueStart;
		int valueEnd;
		if (!ConsumePossiblyQuotedString(buffer, i, sizeof(buffer), valueStart, valueEnd))
		{
			LOG_ERROR("%s: error parsing the skill value for %s on line %d\n", fileName, skillName.c_str(), lineNum);
			continue;
		}

		std::string valueStr(buffer+valueStart, buffer+valueEnd);
		FloatRange value;
		if (ParseFloatRange(valueStr.c_str(), value))
		{
			int category = SkillVariable::COMMON;
			if (lastIsDigit)
			{
				switch(lastDigit)
				{
				case '1':
					category = SkillVariable::EASY;
					break;
				case '2':
					category = SkillVariable::MEDIUM;
					break;
				case '3':
					category = SkillVariable::HARD;
					break;
				default:
					category = SkillVariable::BAD;
					break;
				}

				if (category == SkillVariable::BAD)
				{
					LOG_ERROR("%s: unknown skill value digit suffix %c for %s on line %d\n", fileName, lastDigit, skillName.c_str(), lineNum);
					continue;
				}
			}

			//LOG("Setting skill variable %s to value %g in category %d\n", skillName.c_str(), value, category);
			g_SkillData.SetVariableValue(std::move(skillName), category, value);
		}
		else
		{
			LOG_WARNING("%s: couldn't parse skill value for %s from %s\n", fileName, skillName.c_str(), valueStr.c_str());
		}
	}
}
