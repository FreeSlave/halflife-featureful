#pragma once
#ifndef SKILLDATA_H
#define SKILLDATA_H

#include <string>
#include <unordered_map>
#include "fixed_string.h"
#include "optional.h"
#include "template_property_types.h"

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

	void SetValue(int category, FloatRange value)
	{
		GetVar(category) = value;
	}
	optional<FloatRange> ValueForSkillLevel(int level) const;
	bool HasValue(int category) const
	{
		return GetVar(category).has_value();
	}
	FloatRange GetValue(int category) const
	{
		return GetVar(category).value_or(FloatRange{});
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
	optional<FloatRange>& GetVar(int category)
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
	const optional<FloatRange>& GetVar(int category) const
	{
		return const_cast<SkillVariable*>(this)->GetVar(category);
	}

	optional<FloatRange> _common;
	optional<FloatRange> _easy;
	optional<FloatRange> _medium;
	optional<FloatRange> _hard;
	std::string _fallback;
	optional<float> _multiplier;
};

struct SkillData
{
public:
	void SetVariableValue(const char* name, int category, FloatRange value);
	void SetVariableValue(std::string&& name, int category, FloatRange value);
	const SkillVariable* GetSkillVariable(const char* name) const;
	FloatRange GetValueForSkillLevel(const char* name, int level) const;
	void Clear() {
		_map.clear();
	}
	void ProvideFallback(const char* name, const char* fallback);
	void ProvideFallback(const char* name, FloatRange fallback);
	void ProvideFallback(const char* name, FloatRange fallbackOnEasy, FloatRange fallbackOnMedium, FloatRange fallbackOnHard);
	void ProvideFallbackWithFactor(const char* name, const char* fallback, float factor);
	void ForceValue(const char* name, FloatRange value);
private:
	SkillVariable* AccessSkillVariable(const char* name, bool createIfNotExist = false);
	SkillVariable* EnsureSkillVariable(const char* name);

	typedef std::string keytype;
	std::unordered_map<std::string, SkillVariable> _map;
};

void ParseSkillCfg(unsigned char *pMemFile, int fileSize, const char* fileName);

extern SkillData g_SkillData;

#endif
