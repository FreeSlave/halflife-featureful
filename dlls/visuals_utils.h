#pragma once
#ifndef VISUALS_UTILS_H
#define VISUALS_UTILS_H

#include "customentity.h"
#include "effects.h"
#include "visuals.h"

CSprite* CreateSpriteFromVisual(const Visual* visual, const Vector& origin, int spawnFlags = 0);

CBeam* CreateBeamFromVisual(const Visual* visual);

void WriteBeamVisual(const Visual* visual);

void WriteBeamFollowVisual(const Visual* visual);

void SendDynLight(const Vector& vecOrigin, const Visual* visual);
void SendEntLight(int entIndex, const Vector& vecOrigin, const Visual* visual, int attachment = 0);
void SendSprite(const Vector& vecOrigin, const Visual* visual);
void SendSpray(const Vector& position, const Vector& direction, const Visual* visual, int count, int speed, int noise);
void SendSmoke(const Vector& position, const Visual* visual);
void SendBeamFollow(int entIndex, const Visual* visual);

float AnimateWithFramerate(float frame, float maxFrame, float framerate, float* pLastTime = nullptr);

namespace NPC
{
extern const NamedVisual ropeVisual;
}
#endif
