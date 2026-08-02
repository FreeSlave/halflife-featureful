#pragma once
#ifndef CL_FX_H
#define CL_FX_H

#include "com_model.h"
#include "fx_types.h"
#include "vector.h"
#include "visual_object.h"

enum
{
	WALLIMPACT_HL = 0,
	WALLIMPACT_QUAKE = 1,
	WALLIMPACT_QUAKE2 = 2,
};

extern int GetWallImpactStyle();

void LoadDefaultSprites();

struct ColorRandomizer
{
	ColorRandomizer(int r, int g, int b, int variance = 6);
	color24 operator()() const;

private:
	color24 darkest;
	color24 steps;
	int myVariance;
};

void FX_Streaks(Vector pos, Vector dir, const StreakParams& streakParams, bool isDirectional);
void FX_RicochetSprite(Vector pos, model_t *pmodel, float duration, float scale);
void FX_SparkEffect(Vector pos, const SparkEffectParams& params);
void FX_SparkShower(Vector pos, const SparkEffectParams& params);
void FX_Spray(Vector pos, Vector dir, int modelIndex, int count, int speed, float noise, int rendermode, color24 color, IntRange renderamt, int renderfx, float scale, float framerate, int flags,  const FloatRange& life);
void FX_Spray(Vector pos, Vector dir, int modelIndex, int count, int speed, float noise, const Visual& visual, int flags);
void FX_BloodSpray(const Vector& org, int colorIndex, int modelIndex, float size);
void FX_BloodSplatter(const Vector& org, int colorIndex, int modelIndex, int amount);
void FX_BloodStream(const Vector& org, const Vector& ndir, const IntRange& colorRange, int speed);
void FX_BloodLegacy(const Vector& org, const Vector& ndir, const IntRange& colorRange, int amount);
void FX_BloodParticles(const Vector& org, const IntRange& colorRange, int count);
void FX_QuakeParticles(const Vector& org, const Vector& ndir, const IntRange& colorRange, int count);
void FX_DotParticles(const Vector& org, const Vector& ndir, const IntRange& colorRange, int count);
void FX_DotParticles(const Vector& org, const Vector& ndir, const ColorRandomizer& colorRandomizer, int count);
void FX_ImpactParticles(const Vector& pos, int baseColor);
void FX_WallImpact(const Vector& pos, const Vector& dir, int particleColor, int style);
void FX_GunshotDecal(const Vector& pos, const Vector& dir, int decalIndex, int entIndex, int particleColor);
void FX_BreakModel(const Vector& pos, const Vector& size, const Vector& dir, float random, float life, int count, int modelIndex, char flags, float customScale);

#endif
