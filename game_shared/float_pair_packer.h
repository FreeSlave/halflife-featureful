#pragma once
#ifndef FLOAT_PAIR_PACKER_H
#define FLOAT_PAIR_PACKER_H

#include "clamp.h"

template<int threshold, int precision>
struct FloatPairPacker
{
private:
	static constexpr int MASK() {
		return (1<<precision)-1;
	}
	static constexpr float FACTOR() {
		return (float)MASK();
	}
	static constexpr float RANGE() {
		return threshold * 2.0f;
	}
	static constexpr float SHIFT() {
		return (float)threshold;
	}
public:
	static float encode(float a, float b)
	{
		a = clamp(a, -SHIFT(), SHIFT());
		b = clamp(b, -SHIFT(), SHIFT());

		int ai = int((a + SHIFT()) * (FACTOR() / RANGE()));
		int bi = int((b + SHIFT()) * (FACTOR() / RANGE()));
		int packed = (ai << precision) | bi;
		return *(float*)&packed;
	}

	static void decode(float encoded, float* a, float* b)
	{
		int packed = *(int*)&encoded;
		int ai = (packed >> precision) & MASK();
		int bi = packed & MASK();
		*a = (ai * (RANGE() / FACTOR())) - SHIFT();
		*b = (bi * (RANGE() / FACTOR())) - SHIFT();
	}
};

#endif
