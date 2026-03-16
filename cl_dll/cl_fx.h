#pragma once
#ifndef CL_FX_H
#define CL_FX_H

#include "com_model.h"
#include "fx_types.h"
#include "vector.h"
#include "visual_object.h"

void LoadDefaultSprites();

void FX_Streaks(Vector pos, Vector dir, const StreakParams& streakParams, bool isDirectional);
void FX_RicochetSprite(Vector pos, model_t *pmodel, float duration, float scale);
void FX_SparkEffect(Vector pos, const SparkEffectParams& params);
void FX_SparkShower(Vector pos, const SparkEffectParams& params);
void FX_Spray(Vector pos, Vector dir, int modelIndex, int count, int speed, float noise, int rendermode, color24 color, int renderamt, int renderfx, float scale, float framerate, int flags);
void FX_Spray(Vector pos, Vector dir, int modelIndex, int count, int speed, float noise, const Visual& visual, int flags);

#endif
