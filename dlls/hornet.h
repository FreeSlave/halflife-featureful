/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// Hornets
//=========================================================
#pragma once
#if !defined(HORNET_H)
#define HORNET_H

#include "basemonster.h"

//=========================================================
// Hornet Defines
//=========================================================
#define HORNET_TYPE_RED			0
#define HORNET_TYPE_ORANGE		1
#define HORNET_RED_SPEED		(float)600
#define HORNET_ORANGE_SPEED		(float)800
#define	HORNET_BUZZ_VOLUME		(float)0.8

//=========================================================
// Hornet - this is the projectile that the Alien Grunt fires.
//=========================================================
class CHornet : public CBaseMonster
{
public:
	enum
	{
		TRACKING = 0,
		DART
	};

	void Spawn() override;
	void Precache() override;
	int DefaultClassify() override;
	int Classify() override;
	int IRelationship( CBaseEntity *pTarget ) override;
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;

	void IgniteTrail();
	void EXPORT StartTrack();
	void EXPORT StartDart();
	void EXPORT TrackTarget();
	void EXPORT TrackTouch( CBaseEntity *pOther );
	void EXPORT DartTouch( CBaseEntity *pOther );
	void EXPORT DieTouch( CBaseEntity *pOther );

	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;

	float m_flStopAttack;
	int m_iHornetType;
	float m_flFlySpeed;

	static const NamedSoundScript buzzSoundScript;
	static const NamedSoundScript dieSoundScript;

	static const NamedVisual modelVisual;
	static const NamedVisual sharedTrailVisual;
	static const NamedVisual trailVisual;
	static const NamedVisual trailAltVisual;
	static const NamedVisual puffVisual;
};
#endif // HORNET_H
