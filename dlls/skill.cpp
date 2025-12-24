/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// skill.cpp - code for skill level concerns
//=========================================================
#include	"skill.h"
#include	"skilldata.h"
#include	"ent_templates.h"

float GetSkillValue(const char* name, const EntTemplate* entTemplate, const char* entTemplateName, const EntTemplate* ownerEntTemplate, const char* ownerEntTemplateName)
{
	typedef std::pair<const SkillVariable*, optional<float>> variable_and_value;

	auto variableOrValueForNameFromTemplate = [&](const char* name, const EntTemplate* entTemplate, const char* entTemplateName) -> variable_and_value
	{
		const SkillReplacement* replacement = entTemplate->GetSkillReplacement(name);
		if (replacement)
		{
			switch(replacement->type)
			{
			case SkillReplacement::STRING:
			{
				const SkillVariable* variable = g_SkillData.GetSkillVariable(replacement->replacement.c_str());
				if (!variable)
				{
					ALERT(at_warning, "Entity template \"%s\": skill %s is set to be replaced with %s, but the replacement doesn't exist\n",
						entTemplateName, name, replacement->replacement.c_str());
				}
				return variable_and_value(variable, optional<float>());
			}
			case SkillReplacement::COMMON:
			{
				return variable_and_value(nullptr, optional<float>(replacement->medium));
			}
			case SkillReplacement::DIFFICULTIES:
			{
				float value;
				if (g_iSkillLevel >= SKILL_HARD)
				{
					value = replacement->hard;
				}
				else if (g_iSkillLevel == SKILL_MEDIUM)
				{
					value = replacement->medium;
				}
				else if (g_iSkillLevel <= SKILL_EASY)
				{
					value = replacement->easy;
				}
				return variable_and_value(nullptr, optional<float>(value));
			}
			case SkillReplacement::MULTIPLIER:
			{
				const SkillVariable* variable = g_SkillData.GetSkillVariable(name);
				if (variable)
				{
					optional<float> value = variable->ValueForSkillLevel(g_iSkillLevel);
					if (value.has_value())
					{
						*value *= replacement->medium;
					}
					return variable_and_value(variable, value);
				}
				break;
			}
			}
		}
		return variable_and_value(nullptr, optional<float>());
	};

	auto variableOrValueForName = [&](const char* name)
	{
		variable_and_value pair;
		if (entTemplate)
		{
			pair = variableOrValueForNameFromTemplate(name, entTemplate, entTemplateName);
			if (pair.first || pair.second.has_value())
				return pair;
		}
		else if (ownerEntTemplate)
		{
			pair = variableOrValueForNameFromTemplate(name, ownerEntTemplate, ownerEntTemplateName);
			if (pair.first || pair.second.has_value())
				return pair;
		}

		pair.first = g_SkillData.GetSkillVariable(name);
		pair.second.reset();
		return pair;
	};

	optional<float> multiplier;
	for (int limiter = 0; limiter <= SKILL_FALLBACK_LIMIT; ++limiter)
	{
		auto pair = variableOrValueForName(name);
		if (pair.second.has_value())
		{
			if (multiplier.has_value())
				return *pair.second * *multiplier;
			return *pair.second;
		}

		const SkillVariable* variable = pair.first;
		if (variable)
		{
			optional<float> value = variable->ValueForSkillLevel(g_iSkillLevel);
			if (value.has_value())
			{
				if (multiplier.has_value())
					return *value * *multiplier;
				return *value;
			}
			const char* fallback = variable->Fallback();
			multiplier = variable->FallbackMultiplier();
			if (fallback)
			{
				//ALERT(at_aiconsole, "%s: checking fallback %s for %s\n", STRING(pev->classname), fallback, name);
				name = fallback;
			}
			else
			{
				break;
			}
		}
	}
	return 0.0f;
}
