/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// bullsquid - big, spotty tentacle-mouthed meanie.
//=========================================================

#pragma once
#ifndef BULLSQUID_H
#define BULLSQUID_H

#include "monsters.h"

#define SQUIDSPIT_SPEED 900.0f

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CSquidSpit : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;

	void Touch(CBaseEntity *pOther) override;
	void EXPORT Animate();
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	void SendMessages(CBaseEntity* pClient) override {
		SendProjectileTracer(pClient);
	}

	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;

	static constexpr const char* spitTouchSoundScript = "Bullsquid.SpitTouch";
	static constexpr const char* spitHitSoundScript = "Bullsquid.SpitHit";

	static const NamedVisual spitVisual;
	static const NamedVisual fleckVisual;
protected:
	void SpawnHelper(const char* className, const char* spitVisualName);
};

#define SQUIDSPIT_TOXIC_SPIT 600.0f

class CSquidToxicSpit : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;

	void Touch( CBaseEntity *pOther ) override;
	void EXPORT Animate();
	CBaseMonster* GetSpitOwner();
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	void SendMessages(CBaseEntity* pClient) override {
		SendProjectileTracer(pClient);
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxFrame;

	static const NamedSoundScript acidSoundScript;
	static const NamedSoundScript spithitSoundScript;

	static const NamedVisual toxicSpitVisual;
	static const NamedVisual fleckVisual;
	static const NamedVisual particleVisual;
};

#endif // BULLSQUID_H
