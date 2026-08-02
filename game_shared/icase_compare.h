#pragma once
#ifndef ICASE_COMPARE_H
#define ICASE_COMPARE_H

#include <cstring>
#include <string>
#include "fixed_string.h"

struct CaseInsensitiveCompare
{
	bool operator()(const std::string& lhs, const std::string& rhs) const noexcept
	{
		return stricmp(lhs.c_str(), rhs.c_str()) < 0;
	}

	template<size_t N>
	bool operator()(const fixed_string<N>& lhs, const fixed_string<N>& rhs) const noexcept
	{
		return stricmp(lhs.c_str(), rhs.c_str()) < 0;
	}
};

#endif
