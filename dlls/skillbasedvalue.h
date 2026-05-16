#pragma once
#ifndef SKILLBASEDVALUE_H
#define SKILLBASEDVALUE_H

#include <string>
#include "template_property_types.h"

struct SkillBasedValue
{
	SkillBasedValue() = default;
	SkillBasedValue(const FloatRange& range) {
		easy = medium = hard = range;
		type = COMMON;
	}
	SkillBasedValue(float value) {
		easy = medium = hard = value;
		type = COMMON;
	}

	enum : short
	{
		UNDEFINED,
		STRING,
		COMMON,
		DIFFICULTIES
	};

	std::string skillVariable;
	FloatRange easy{0.0f};
	FloatRange medium{0.0f};
	FloatRange hard{0.0f};
	short type{UNDEFINED};

	inline bool IsDefined() const {
		return type == COMMON || type == DIFFICULTIES || (type == STRING && !skillVariable.empty());
	}
};
#endif
