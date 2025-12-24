#pragma once
#ifndef SKILLDATA_H
#define SKILLDATA_H

#include <string>
#include <unordered_map>
#include "fixed_string.h"
#include "optional.h"

#define SKILL_FALLBACK_LIMIT 2

struct SkillVariable
{
	enum
	{
		BAD = -1,
		COMMON = 0,
		EASY,
		MEDIUM,
		HARD
	};

	void SetValue(int category, float value)
	{
		GetVar(category) = value;
	}
	optional<float> ValueForSkillLevel(int level) const;
	bool HasValue(int category) const
	{
		return GetVar(category).has_value();
	}
	float GetValue(int category) const
	{
		return GetVar(category).value_or(0.0f);
	}
	void Reset()
	{
		_common.reset();
		_easy.reset();
		_medium.reset();
		_hard.reset();
		_fallback.clear();
		_multiplier.reset();
	}
	const char* Fallback() const {
		if (_fallback.empty())
			return nullptr;
		return _fallback.c_str();
	}
	void SetFallback(const char* name) {
		_fallback = name;
	}
	void SetFallbackWithMultiplier(const char* name, float factor) {
		_fallback = name;
		_multiplier = factor;
	}
	const optional<float> FallbackMultiplier() const {
		return _multiplier;
	}
private:
	optional<float>& GetVar(int category)
	{
		switch(category)
		{
		case EASY:
			return _easy;
		case MEDIUM:
			return _medium;
		case HARD:
			return _hard;
		default:
			return _common;
		}
	}
	const optional<float>& GetVar(int category) const
	{
		return const_cast<SkillVariable*>(this)->GetVar(category);
	}

	optional<float> _common;
	optional<float> _easy;
	optional<float> _medium;
	optional<float> _hard;
	std::string _fallback;
	optional<float> _multiplier;
};

struct SkillData
{
public:
	void SetVariableValue(const char* name, int category, float value);
	void SetVariableValue(std::string&& name, int category, float value);
	const SkillVariable* GetSkillVariable(const char* name) const;
	float GetValueForSkillLevel(const char* name, int level) const;
	void Clear() {
		_map.clear();
	}
	void ProvideFallback(const char* name, const char* fallback);
	void ProvideFallback(const char* name, float fallback);
	void ProvideFallback(const char* name, float fallbackOnEasy, float fallbackOnMedium, float fallbackOnHard);
	void ProvideFallbackWithFactor(const char* name, const char* fallback, float factor);
	void ForceValue(const char* name, float value);
private:
	SkillVariable* AccessSkillVariable(const char* name, bool createIfNotExist = false);
	SkillVariable* EnsureSkillVariable(const char* name);

	typedef std::string keytype;
	std::unordered_map<std::string, SkillVariable> _map;
};

extern SkillData g_SkillData;

#endif
