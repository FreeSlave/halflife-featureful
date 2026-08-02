#pragma once
#ifndef FX_TYPES_H
#define FX_TYPES_H

#include "mathlib.h"
#include "particledef.h"

#define SPARK_EFFECT_NO_STREAK (1<<0)

struct SparkEffectParams
{
	SparkEffectParams();
	int streakCount;
	int streakVelocity;
	int sparkModelIndex;
	float sparkDuration;
	float sparkScaleMin;
	float sparkScaleMax;
	int flags;
};

struct StreakParams
{
	StreakParams();
	int color;
	int count;
	float speed;
	float velocityMin;
	float velocityMax;
	float minLife;
	float maxLife;
	ptype_t particleType;
	float length;
};

#endif
