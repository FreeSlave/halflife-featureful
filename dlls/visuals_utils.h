#pragma once
#ifndef VISUALS_UTILS_H
#define VISUALS_UTILS_H

#include "customentity.h"
#include "effects.h"
#include "visuals.h"

CSprite* CreateSpriteFromVisual(const Visual* visual, const Vector& origin, bool once = false);

CBeam* CreateBeamFromVisual(const Visual* visual);

void WriteBeamFollowVisual(const Visual* visual);

void SendDynLight(const Vector& vecOrigin, const Visual* visual);
void SendEntLight(int entIndex, const Vector& vecOrigin, const Visual* visual, int attachment = 0);
void SendSprite(const Vector& vecOrigin, const Visual* visual);
void SendSpray(const Vector& position, const Vector& direction, const Visual* visual, int count, int speed, int noise);
void SendSmoke(const Vector& position, const Visual* visual);
void SendBeamFollow(int entIndex, const Visual* visual);
void SendBeam(int entIndexAndAttachment, const Vector& endPos, const Visual* visual, int msgType = MSG_BROADCAST, const float* origin = nullptr);
void SendBeam(const Vector& startPos, const Vector& endPos, const Visual* visual, int msgType = MSG_BROADCAST, const float* origin = nullptr);
void SendBeam(int entIndexAndAttachment, int entIndexAndAttachment2, const Visual* visual, int msgType = MSG_BROADCAST, const float* origin = nullptr);

void SendBeamWave(const Vector& vecSrc, float radius, const Visual* visual, int msgType = MSG_BROADCAST, const float* origin = nullptr);

float AnimateWithFramerate(float frame, float maxFrame, float framerate, float* pLastTime = nullptr);

namespace NPC
{
extern const NamedVisual ropeVisual;
}
#endif
