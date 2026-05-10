#pragma once
#ifndef SKILLBASEDVALUE_H
#define SKILLBASEDVALUE_H

#include <string>
#include "template_property_types.h"

struct SkillBasedValue
{
	enum : short
	{
		STRING,
		COMMON,
		DIFFICULTIES
	};

	std::string skillVariable;
	FloatRange easy{0.0f};
	FloatRange medium{0.0f};
	FloatRange hard{0.0f};
	short type{STRING};

	inline bool IsDefined() const {
		return type == COMMON || type == DIFFICULTIES || !skillVariable.empty();
	}
};
#endif
