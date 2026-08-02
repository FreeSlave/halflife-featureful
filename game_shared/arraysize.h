#pragma once
#ifndef ARRAYSIZE_H
#define ARRAYSIZE_H

#include <cstddef>

#undef ARRAYSIZE
template <typename T, size_t N>
constexpr size_t ARRAYSIZE(T (&arr)[N]) noexcept
{
	return N;
}

#endif
