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
#pragma once
#if !defined(FUNC_BREAK_H)
#define FUNC_BREAK_H

#include "cbase.h"

typedef enum
{
	expRandom,
	expDirected
} Explosions;
typedef enum
{
	matGlass = 0,
	matWood,
	matMetal,
	matFlesh,
	matCinderBlock,
	matCeilingTile,
	matComputer,
	matUnbreakableGlass,
	matRocks,
	matNone,
	matLastMaterial
} Materials;

#define	NUM_SHARDS 6 // this many shards spawned when breakable objects break;

#define SF_BREAK_TRIGGER_ONLY	1// may only be broken by trigger
#define	SF_BREAK_TOUCH			2// can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE		4// can be broken by a player standing on it
#define SF_BREAKABLE_INVERT		16
#define SF_BREAK_SHOW_HUD_INFO	32
#define SF_BREAK_CROWBAR		256// instant break if hit with crowbar
#define SF_BREAK_EXPLOSIVES_ONLY		512// can be damaged only by DMG_BLAST
#define SF_BREAK_OP4MORTAR_ONLY	1024 // can be damaged only by op4mortar rockets
#define SF_BREAK_NOT_SOLID 4096 // breakable is not solid
#define SF_BREAK_SMOKE_TRAILS 8192
#define SF_BREAK_TRANSPARENT_GIBS 16384

// func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE		128

class CBreakable : public CBaseDelay
{
public:
	// basic functions
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void EXPORT BreakTouch( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	NODE_LINKENT HandleLinkEnt(int afCapMask, bool nodeQueryStatic);
	void DamageSound( void );

	static void BreakModel(const Vector& vecSpot, const Vector& size, const Vector &vecVelocity, int shardModelIndex, int iGibs, char cFlag);

	// breakables use an overridden takedamage
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;
	// To spark when hit
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;

	bool IsBreakable( void );

	int DamageDecal( int bitsDamageType );
	const char* DefaultDisplayName() override { return "Breakable"; }
	bool MustDisplayHUDInfo() const override { return (pev->spawnflags & SF_BREAK_SHOW_HUD_INFO) != 0; }
	bool IsDestroyableObstacle() override;

	void EXPORT Die( void );
	void DieToActivator(CBaseEntity* pActivator);
	void UpdateOnRemove() override;
	virtual int ObjectCaps( void ) { return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ); }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	inline bool Explodable( void ) { return ExplosionMagnitude() > 0; }
	inline int ExplosionMagnitude( void ) { return pev->impulse; }
	inline void ExplosionSetMagnitude( int magnitude ) { pev->impulse = magnitude; }

	static void MaterialSoundPrecache( Materials precacheMaterial );
	static void MaterialSoundRandom( edict_t *pEdict, Materials soundMaterial, float volume );
	static const char **MaterialSoundList( Materials precacheMaterial, int &soundCount );

	static const char *pSoundsWood[];
	static const char *pSoundsFlesh[];
	static const char *pSoundsGlass[];
	static const char *pSoundsMetal[];
	static const char *pSoundsConcrete[];
	static const char *pSpawnObjects[];

	static TYPEDESCRIPTION m_SaveData[];

	Materials m_Material;
	Explosions m_Explosion;
	int m_idShard;
	float m_angle;
	string_t m_iszGibModel;
	string_t m_iszSpawnObject;

	short m_targetActivator;
	int m_iGibs;

	string_t m_iszWhenHit; // locus trigger
	CPointEntity* m_pHitProxy;

	CBaseEntity* GetHitProxy();
};
#endif	// FUNC_BREAK_H
