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

===== explode.cpp ========================================================

  Explosion-related code

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"
#include "locus.h"
#include "combat.h"

// Spark Shower
class CShower : public CBaseEntity
{
	void Spawn() override;
	void Think() override;
	void Touch( CBaseEntity *pOther ) override;
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS( spark_shower, CShower )

void CShower::Spawn()
{
	pev->velocity = RANDOM_FLOAT( 200.0f, 300.0f ) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT( -100.0f, 100.0f );
	pev->velocity.y += RANDOM_FLOAT( -100.0f, 100.0f );
	if( pev->velocity.z >= 0 )
		pev->velocity.z += 200.0f;
	else
		pev->velocity.z -= 200.0f;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "models/grenade.mdl" );	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT( 0.5f, 1.5f );

	pev->angles = g_vecZero;
}

void CShower::Think()
{
	UTIL_Sparks( pev->origin );

	pev->speed -= 0.1f;
	if( pev->speed > 0.0f )
		pev->nextthink = gpGlobals->time + 0.1f;
	else
		UTIL_Remove( this );
	pev->flags &= ~FL_ONGROUND;
}

void CShower::Touch( CBaseEntity *pOther )
{
	if( pev->flags & FL_ONGROUND )
		pev->velocity = pev->velocity * 0.1f;
	else
		pev->velocity = pev->velocity * 0.6f;

	if( ( pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y ) < 10.0f )
		pev->speed = 0.0f;
}

class CEnvExplosion : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override;
	void EXPORT Smoke();
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iMagnitude;// how large is the fireball? how much damage?
	int m_spriteScale; // what's the exact fireball sprite scale? 
	int m_iRadius;
	string_t m_smokeSprite;

	int m_iFireball;
	int m_iSmoke;
};

TYPEDESCRIPTION	CEnvExplosion::m_SaveData[] =
{
	DEFINE_FIELD( CEnvExplosion, m_iMagnitude, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosion, m_spriteScale, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosion, m_iRadius, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvExplosion, CBaseEntity )
LINK_ENTITY_TO_CLASS( env_explosion, CEnvExplosion )

void CEnvExplosion::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "iMagnitude" ) )
	{
		m_iMagnitude = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "iRadius" ) )
	{
		m_iRadius = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "smokesprite") )
	{
		m_smokeSprite = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvExplosion::Precache()
{
	if (!FStringNull(pev->model))
		m_iFireball = PRECACHE_MODEL(STRING(pev->model));
	if (!FStringNull(m_smokeSprite))
		m_iSmoke = PRECACHE_MODEL(STRING(m_smokeSprite));
}

void CEnvExplosion::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	pev->movetype = MOVETYPE_NONE;
	/*
	if( m_iMagnitude > 250 )
	{
		m_iMagnitude = 250;
	}
	*/

	if (pev->scale == 0.0f)
	{
		float flSpriteScale;
		flSpriteScale = ( m_iMagnitude - 50 ) * 0.6f;

		/*
		if( flSpriteScale > 50.0f )
		{
			flSpriteScale = 50.0f;
		}
		*/
		if( flSpriteScale < 10.0f )
		{
			flSpriteScale = 10.0f;
		}

		m_spriteScale = (int)flSpriteScale;
	}
	else
	{
		m_spriteScale = (int)(pev->scale * 10.0f);
		if (m_spriteScale > 255)
			m_spriteScale = 255;
	}
}

void CEnvExplosion::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	TraceResult tr;

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	Vector vecSpot;// trace starts here!

	//LRC
	if (FStringNull(pev->target))
	{
		vecSpot = pev->origin;
	}
	else
	{
		if (!TryCalcLocus_Position(this, pActivator, STRING(pev->target), vecSpot))
			return;
	}

	UTIL_TraceLine( vecSpot + Vector( 0.0f, 0.0f, 8.0f ), vecSpot + Vector( 0.0f, 0.0f, -32.0f ),  ignore_monsters, ENT( pev ), &tr );

	// Pull out of the wall a bit
	if( tr.flFraction != 1.0f )
	{
		pev->origin = tr.vecEndPos + ( tr.vecPlaneNormal * ( m_iMagnitude - 24 ) * 0.6f );
	}
	else
	{
		pev->origin = vecSpot;
	}

	// draw decal
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NODECAL ) )
	{
		if( RANDOM_FLOAT( 0.0f, 1.0f ) < 0.5f )
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
		}
		else
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
		}
	}

	int explosionFlags = TE_EXPLFLAG_NONE;
	if (FBitSet(pev->spawnflags, SF_ENVEXPLOSION_NOSOUND)) {
		explosionFlags |= TE_EXPLFLAG_NOSOUND;
	}

	const int fireballIndex = m_iFireball ? m_iFireball : g_sModelIndexFireball;

	// draw fireball
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_VECTOR( pev->origin );
			WRITE_SHORT( fireballIndex );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 15 ); // framerate
			WRITE_BYTE( explosionFlags );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_VECTOR( pev->origin );
			WRITE_SHORT( fireballIndex );
			WRITE_BYTE( 0 ); // no sprite
			WRITE_BYTE( 15 ); // framerate
			WRITE_BYTE( explosionFlags );
		MESSAGE_END();
	}

	// do damage
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE ) )
	{
		entvars_t* pevAttacker = pev;
		if (FBitSet(pev->spawnflags, SF_ENVEXPLOSION_ACTIVATOR_IS_ATTACKER) && pActivator) {
			pevAttacker = pActivator->pev;
		}
		const float radius = m_iRadius > 0 ? m_iRadius : m_iMagnitude * DEFAULT_EXPLOSION_RADIUS_MULTIPLIER;
		::RadiusDamage( pev->origin, pev, pevAttacker, DamageInfo{(float)m_iMagnitude, DMG_BLAST}, radius, CLASS_NONE );
	}

	SetThink( &CEnvExplosion::Smoke );
	pev->nextthink = gpGlobals->time + 0.3f;

	// draw sparks
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS ) )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );

		for( int i = 0; i < sparkCount; i++ )
		{
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
		}
	}
}

void CEnvExplosion::Smoke()
{
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_VECTOR( pev->origin );
			WRITE_SHORT( m_iSmoke ? m_iSmoke : g_sModelIndexSmoke );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}

	if( !( pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE ) )
	{
		UTIL_Remove( this );
	}
}

// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate(const Vector &center, const Vector &angles, edict_t *pOwner, int magnitude, bool doDamage , entvars_t *pevAttacker)
{
	KeyValueData kvd;
	char buf[128];

	CBaseEntity *pExplosion = CBaseEntity::Create( "env_explosion", center, angles, pOwner );
	sprintf( buf, "%3d", magnitude );
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue( &kvd );
	if( !doDamage )
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	if (pevAttacker) {
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_ACTIVATOR_IS_ATTACKER;
	}

	pExplosion->Spawn();
	pExplosion->Use( pevAttacker ? CBaseEntity::Instance(pevAttacker) : NULL, NULL, USE_TOGGLE, 0 );
}
