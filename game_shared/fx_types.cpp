#include "fx_types.h"

SparkEffectParams::SparkEffectParams():
	streakCount(0),
	streakVelocity(0),
	sparkModelIndex(0),
	sparkDuration(0.0f),
	sparkScaleMin(0.0f),
	sparkScaleMax(0.0f),
	flags(0) {}

StreakParams::StreakParams():
	color(5),
	count(8),
	speed(0),
	velocityMin(0),
	velocityMax(0),
	minLife(0.1f),
	maxLife(0.1f),
	particleType(pt_grav),
	length(1.0f) {}
