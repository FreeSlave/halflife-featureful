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
/*

===== mortar.cpp ========================================================

  the "LaBuznik" mortar device

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "combat.h"
#include "ggrenade.h"
#include "global_models.h"
#include "decals.h"
#include "soundent.h"
#include "skill.h"
#include "visuals_utils.h"

#define SF_MORTARFIELD_DONT_SET_ACTIVATOR_AS_ATTACKER 1

class CFuncMortarField : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData *pkvd ) override;

	// Bmodels don't go across transitions
	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void EXPORT FieldUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	string_t m_iszXController;
	string_t m_iszYController;
	float m_flSpread;
	int m_iCount;
	int m_fControl;

	static const NamedSoundScript launchSoundScript;

	EntityOverrides MortarStrikeOverrides() {
		EntityOverrides entityOverrides;
		entityOverrides.ownerEntTemplate = m_entTemplate;
		return entityOverrides;
	}
};

LINK_ENTITY_TO_CLASS( func_mortar_field, CFuncMortarField )

TYPEDESCRIPTION	CFuncMortarField::m_SaveData[] =
{
	DEFINE_FIELD( CFuncMortarField, m_iszXController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_iszYController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_flSpread, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMortarField, m_iCount, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncMortarField, m_fControl, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncMortarField, CBaseToggle )

const NamedSoundScript CFuncMortarField::launchSoundScript = {
	CHAN_VOICE,
	{"weapons/mortar.wav"},
	1.0f,
	ATTN_NONE,
	IntRange(95, 124),
	"Mortar.Launch"
};

void CFuncMortarField::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_iszXController" ) )
	{
		m_iszXController = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iszYController" ) )
	{
		m_iszYController = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flSpread" ) )
	{
		m_flSpread = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_fControl" ) )
	{
		m_fControl = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iCount" ) )
	{
		m_iCount = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "delay" ) )
	{
		m_flDelay = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
}

// Drop bombs from above
void CFuncMortarField::Spawn()
{
	pev->solid = SOLID_NOT;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetBits( pev->effects, EF_NODRAW );
	SetUse( &CFuncMortarField::FieldUse );
	Precache();
}

void CFuncMortarField::Precache()
{
	RegisterAndPrecacheSoundScript(launchSoundScript);
#if 0
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
#endif
	UTIL_PrecacheOther("monster_mortar", GetProjectileOverrides());
}

// If connected to a table, then use the table controllers, else hit where the trigger is.
void CFuncMortarField::FieldUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector vecStart;

	vecStart.x = RANDOM_FLOAT( pev->mins.x, pev->maxs.x );
	vecStart.y = RANDOM_FLOAT( pev->mins.y, pev->maxs.y );
	vecStart.z = pev->maxs.z;

	switch( m_fControl )
	{
	case 0:
		// random
		break;
	case 1:
		// Trigger Activator
		if( pActivator != NULL )
		{
			vecStart.x = pActivator->pev->origin.x;
			vecStart.y = pActivator->pev->origin.y;
		}
		break;
	case 2:
		// table
		{
			CBaseEntity *pController;

			if( !FStringNull( m_iszXController ) )
			{
				pController = UTIL_FindEntityByTargetname( NULL, STRING( m_iszXController ) );
				if( pController != NULL )
				{
					vecStart.x = pev->mins.x + pController->pev->ideal_yaw * ( pev->size.x );
				}
			}
			if( !FStringNull( m_iszYController ) )
			{
				pController = UTIL_FindEntityByTargetname( NULL, STRING( m_iszYController ) );
				if( pController != NULL )
				{
					vecStart.y = pev->mins.y + pController->pev->ideal_yaw * ( pev->size.y );
				}
			}
		}
		break;
	}

	EmitSoundScript(launchSoundScript);

	float t = m_flDelay == 0.0f ? 2.5 : m_flDelay;
	for( int i = 0; i < m_iCount; i++ )
	{
		Vector vecSpot = vecStart;
		vecSpot.x += RANDOM_FLOAT( -m_flSpread, m_flSpread );
		vecSpot.y += RANDOM_FLOAT( -m_flSpread, m_flSpread );

		TraceResult tr;
		UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -1 ) * 4096, ignore_monsters, ENT( pev ), &tr );

		edict_t *pentOwner = NULL;
		if( pActivator && !FBitSet(pev->spawnflags, SF_MORTARFIELD_DONT_SET_ACTIVATOR_AS_ATTACKER) )
			pentOwner = pActivator->edict();

		CBaseEntity *pMortar = Create( "monster_mortar", tr.vecEndPos, Vector( 0, 0, 0 ), pentOwner, GetProjectileOverrides() );
		pMortar->pev->nextthink = gpGlobals->time + t;
		t += RANDOM_FLOAT( 0.2, 0.5 );

		if( i == 0 )
			InsertAISound( bits_SOUND_DANGER, tr.vecEndPos, 400, 0.3 );
	}
}

class CMortar : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;

	void EXPORT MortarExplode();

	static const NamedVisual beamVisual;
	static const NamedVisual circleVisual;
};

LINK_ENTITY_TO_CLASS( monster_mortar, CMortar )

const NamedVisual CMortar::beamVisual = BuildVisual("Mortar.Beam")
	.Model("sprites/lgtning.spr")
	.RenderColor(255, 160, 100)
	.Alpha(128)
	.Framerate(0)
	.Life(0.1f)
	.BeamWidth(40);

const NamedVisual CMortar::circleVisual = BuildVisual("Mortar.Circle")
	.RenderColor(255, 160, 100)
	.Alpha(255)
	.Framerate(0)
	.Life(0.2f)
	.BeamWidth(12);

void CMortar::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->dmg = GetSkillValue("mortar");

	SetThink( &CMortar::MortarExplode );
	pev->nextthink = 0;

	Precache();
}

void CMortar::Precache()
{
	RegisterVisual(beamVisual);
	RegisterVisual(circleVisual);
}

void CMortar::MortarExplode()
{
	const Visual* bVisual = GetVisual(beamVisual);

	if (bVisual->modelIndex)
	{
		// mortar beam
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_VECTOR( pev->origin );
		WRITE_VECTOR( pev->origin + Vector(0, 0, 1024) );
		WriteBeamVisual(bVisual);
		MESSAGE_END();
	}

	const Visual* cVisual = GetVisual(circleVisual);
	if (cVisual->modelIndex)
	{
		// blast circle
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMTORUS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 32 );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 32 + pev->dmg * 2 / .2 ); // reach damage radius over .3 seconds
		WriteBeamVisual(cVisual);
		MESSAGE_END();
	}

	TraceResult tr;
	UTIL_TraceLine( pev->origin + Vector( 0, 0, 1024 ), pev->origin - Vector( 0, 0, 1024 ), dont_ignore_monsters, ENT( pev ), &tr );

	Explode( &tr, DMG_BLAST | DMG_MORTAR );
	UTIL_ScreenShake( tr.vecEndPos, 25.0, 150.0, 1.0, 750 );
#if 0
	int pitch = RANDOM_LONG( 95, 124 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/mortarhit.wav", 1.0, 0.55, 0, pitch );

	// ForceSound( SNDRADIUS_MP5, bits_SOUND_COMBAT );

	// ExplodeModel( pev->origin, 400, g_sModelIndexShrapnel, 30 );

	RadiusDamage( pev, VARS( pev->owner ), pev->dmg, CLASS_NONE, DMG_BLAST );

	/*
	if( RANDOM_FLOAT( 0, 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}
	*/

	SetThink( &SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
#endif
}

#if 0
void CMortar::ShootTimed( EVARS *pevOwner, Vector vecStart, float time )
{
	CMortar *pMortar = GetClassPtr( (CMortar *)NULL );
	pMortar->Spawn();

	TraceResult tr;
	UTIL_TraceLine( vecStart, vecStart + Vector( 0, 0, -1 ) * 4096, ignore_monsters, ENT( pMortar->pev ), &tr );

	pMortar->pev->nextthink = gpGlobals->time + time;

	UTIL_SetOrigin( pMortar->pev, tr.vecEndPos );
}
#endif
