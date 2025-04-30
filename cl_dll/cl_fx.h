#pragma once
#ifndef CL_FX_H
#define CL_FX_H

#include "com_model.h"
#include "fx_types.h"
#include "vector.h"

void LoadDefaultSprites();

void FX_Streaks(Vector pos, Vector dir, const StreakParams& streakParams, bool isDirectional);
void FX_RicochetSprite(Vector pos, model_t *pmodel, float duration, float scale);
void FX_SparkEffect(Vector pos, const SparkEffectParams& params);
void FX_SparkShower(Vector pos, const SparkEffectParams& params);

#endif
