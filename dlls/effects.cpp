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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"
#include "bullsquid.h"
#include "soundradius.h"
#include "locus.h"
#include "mod_features.h"

#define FEATURE_ENV_WARPBALL 0
#define FEATURE_ENV_XENMAKER 1

// use a single sound (debris/alien_teleport.wav) for teleportation effect instead of two
#define FEATURE_ALIEN_TELEPORT_SOUND 0

#define	SF_GIBSHOOTER_REPEATABLE		1 // allows a gibshooter to be refired

#define SF_FUNNEL_REVERSE			1 // funnel effect repels particles instead of attracting them.
#define SF_FUNNEL_REPEATABLE		2 // allows a funnel to be refired

// Lightning target, just alias landmark
LINK_ENTITY_TO_CLASS( info_target, CPointEntity )

class CBubbling : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );

	void EXPORT FizzThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	int m_density;
	int m_frequency;
	int m_bubbleModel;
	int m_state;
};

LINK_ENTITY_TO_CLASS( env_bubbles, CBubbling )

TYPEDESCRIPTION	CBubbling::m_SaveData[] =
{
	DEFINE_FIELD( CBubbling, m_density, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_frequency, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_state, FIELD_INTEGER ),
	// Let spawn restore this!
	//DEFINE_FIELD( CBubbling, m_bubbleModel, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBubbling, CBaseEntity )

#define SF_BUBBLES_STARTOFF		0x0001

void CBubbling::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );		// Set size

	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;
	int speed = fabs( pev->speed );

	// HACKHACK!!! - Speed in rendercolor
	pev->rendercolor.x = speed >> 8;
	pev->rendercolor.y = speed & 255;
	pev->rendercolor.z = ( pev->speed < 0 ) ? 1 : 0;

	if( !( pev->spawnflags & SF_BUBBLES_STARTOFF ) )
	{
		SetThink( &CBubbling::FizzThink );
		pev->nextthink = gpGlobals->time + 2.0f;
		m_state = 1;
	}
	else 
		m_state = 0;
}

void CBubbling::Precache( void )
{
	m_bubbleModel = PRECACHE_MODEL( "sprites/bubble.spr" );			// Precache bubble sprite
}

void CBubbling::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( ShouldToggle( useType, m_state ) )
		m_state = !m_state;

	if( m_state )
	{
		SetThink( &CBubbling::FizzThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		SetThink( NULL );
		pev->nextthink = 0;
	}
}

void CBubbling::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "density" ) )
	{
		m_density = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		m_frequency = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "current" ) )
	{
		pev->speed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CBubbling::FizzThink( void )
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, VecBModelOrigin( pev ) );
		WRITE_BYTE( TE_FIZZ );
		WRITE_SHORT( (short)ENTINDEX( edict() ) );
		WRITE_SHORT( (short)m_bubbleModel );
		WRITE_BYTE( m_density );
	MESSAGE_END();

	if( m_frequency > 19 )
		pev->nextthink = gpGlobals->time + 0.5f;
	else
		pev->nextthink = gpGlobals->time + 2.5f - ( 0.1f * m_frequency );
}

// --------------------------------------------------
// 
// Beams
//
// --------------------------------------------------

LINK_ENTITY_TO_CLASS( beam, CBeam )

void CBeam::Spawn( void )
{
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();
}

void CBeam::Precache( void )
{
	if( pev->owner )
		SetStartEntity( ENTINDEX( pev->owner ) );
	if( pev->aiment )
		SetEndEntity( ENTINDEX( pev->aiment ) );
}

void CBeam::SetStartEntity( int entityIndex )
{
	pev->sequence = ( entityIndex & 0x0FFF ) | ( pev->sequence & 0xF000 );
	pev->owner = g_engfuncs.pfnPEntityOfEntIndex( entityIndex );
}

void CBeam::SetEndEntity( int entityIndex )
{
	pev->skin = ( entityIndex & 0x0FFF ) | ( pev->skin & 0xF000 );
	pev->aiment = g_engfuncs.pfnPEntityOfEntIndex( entityIndex );
}

// These don't take attachments into account
const Vector &CBeam::GetStartPos( void )
{
	if( GetType() == BEAM_ENTS )
	{
		edict_t *pent =  g_engfuncs.pfnPEntityOfEntIndex( GetStartEntity() );
		return pent->v.origin;
	}
	return pev->origin;
}

const Vector &CBeam::GetEndPos( void )
{
	int type = GetType();
	if( type == BEAM_POINTS || type == BEAM_HOSE )
	{
		return pev->angles;
	}

	edict_t *pent =  g_engfuncs.pfnPEntityOfEntIndex( GetEndEntity() );
	if( pent )
		return pent->v.origin;
	return pev->angles;
}

CBeam *CBeam::BeamCreate( const char *pSpriteName, int width )
{
	// Create a new entity with CBeam private data
	CBeam *pBeam = GetClassPtr( (CBeam *)NULL );
	pBeam->pev->classname = MAKE_STRING( "beam" );

	pBeam->BeamInit( pSpriteName, width );

	return pBeam;
}

void CBeam::BeamInit( const char *pSpriteName, int width )
{
	pev->flags |= FL_CUSTOMENTITY;
	SetColor( 255, 255, 255 );
	SetBrightness( 255 );
	SetNoise( 0 );
	SetFrame( 0 );
	SetScrollRate( 0 );
	pev->model = MAKE_STRING( pSpriteName );
	SetTexture( PRECACHE_MODEL( pSpriteName ) );
	SetWidth( width );
	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
}

void CBeam::PointsInit( const Vector &start, const Vector &end )
{
	SetType( BEAM_POINTS );
	SetStartPos( start );
	SetEndPos( end );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::HoseInit( const Vector &start, const Vector &direction )
{
	SetType( BEAM_HOSE );
	SetStartPos( start );
	SetEndPos( direction );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::PointEntInit(const Vector &start, int endIndex , int endAttachment)
{
	SetType( BEAM_ENTPOINT );
	SetStartPos( start );
	SetEndEntity( endIndex );
	SetStartAttachment( 0 );
	SetEndAttachment( endAttachment );
	RelinkBeam();
}

void CBeam::EntsInit(int startIndex, int endIndex , int startAttachment, int endAttachment)
{
	SetType( BEAM_ENTS );
	SetStartEntity( startIndex );
	SetEndEntity( endIndex );
	SetStartAttachment( startAttachment );
	SetEndAttachment( endAttachment );
	RelinkBeam();
}

void CBeam::RelinkBeam( void )
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	pev->mins.x = Q_min( startPos.x, endPos.x );
	pev->mins.y = Q_min( startPos.y, endPos.y );
	pev->mins.z = Q_min( startPos.z, endPos.z );
	pev->maxs.x = Q_max( startPos.x, endPos.x );
	pev->maxs.y = Q_max( startPos.y, endPos.y );
	pev->maxs.z = Q_max( startPos.z, endPos.z );
	pev->mins = pev->mins - pev->origin;
	pev->maxs = pev->maxs - pev->origin;

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );
}

#if 0
void CBeam::SetObjectCollisionBox( void )
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	pev->absmin.x = min( startPos.x, endPos.x );
	pev->absmin.y = min( startPos.y, endPos.y );
	pev->absmin.z = min( startPos.z, endPos.z );
	pev->absmax.x = max( startPos.x, endPos.x );
	pev->absmax.y = max( startPos.y, endPos.y );
	pev->absmax.z = max( startPos.z, endPos.z );
}
#endif

void CBeam::TriggerTouch( CBaseEntity *pOther )
{
	if( pOther->pev->flags & ( FL_CLIENT | FL_MONSTER ) )
	{
		if( pev->owner )
		{
			CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
			pOwner->Use( pOther, this, USE_TOGGLE, 0 );
		}
		ALERT( at_console, "Firing targets!!!\n" );
	}
}

CBaseEntity *CBeam::RandomTargetname( const char *szName )
{
	int total = 0;

	CBaseEntity *pEntity = NULL;
	CBaseEntity *pNewEntity = NULL;
	while( ( pNewEntity = UTIL_FindEntityByTargetname( pNewEntity, szName ) ) != NULL )
	{
		total++;
		if( RANDOM_LONG( 0, total - 1 ) < 1 )
			pEntity = pNewEntity;
	}
	return pEntity;
}

void CBeam::DoSparks( const Vector &start, const Vector &end )
{
	if( pev->spawnflags & ( SF_BEAM_SPARKSTART | SF_BEAM_SPARKEND ) )
	{
		if( pev->spawnflags & SF_BEAM_SPARKSTART )
		{
			UTIL_Sparks( start );
		}
		if( pev->spawnflags & SF_BEAM_SPARKEND )
		{
			UTIL_Sparks( end );
		}
	}
}

class CLightning : public CBeam
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Activate( void );

	void EXPORT StrikeThink( void );
	void EXPORT DamageThink( void );
	void RandomArea( void );
	void RandomPoint( Vector &vecSrc );
	void Zap( const Vector &vecSrc, const Vector &vecDest );
	void EXPORT StrikeUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	inline BOOL ServerSide( void )
	{
		if( m_life == 0 && !( pev->spawnflags & SF_BEAM_RING ) )
			return TRUE;
		return FALSE;
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void BeamUpdateVars( void );

	int	m_active;
	string_t	m_iszStartEntity;
	string_t	m_iszEndEntity;
	float	m_life;
	int	m_boltWidth;
	int	m_noiseAmplitude;
	int	m_brightness;
	int	m_speed;
	float	m_restrike;
	int	m_spriteTexture;
	string_t	m_iszSpriteName;
	int	m_frameStart;

	float	m_radius;
};

LINK_ENTITY_TO_CLASS( env_lightning, CLightning )
LINK_ENTITY_TO_CLASS( env_beam, CLightning )

// UNDONE: Jay -- This is only a test
#if _DEBUG
class CTripBeam : public CLightning
{
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trip_beam, CTripBeam )

void CTripBeam::Spawn( void )
{
	CLightning::Spawn();
	SetTouch( &CLightning::TriggerTouch );
	pev->solid = SOLID_TRIGGER;
	RelinkBeam();
}
#endif

TYPEDESCRIPTION	CLightning::m_SaveData[] =
{
	DEFINE_FIELD( CLightning, m_active, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_iszStartEntity, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_iszEndEntity, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_life, FIELD_FLOAT ),
	DEFINE_FIELD( CLightning, m_boltWidth, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_noiseAmplitude, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_brightness, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_speed, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_restrike, FIELD_FLOAT ),
	DEFINE_FIELD( CLightning, m_spriteTexture, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_iszSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_frameStart, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_radius, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CLightning, CBeam )

void CLightning::Spawn( void )
{
	if( FStringNull( m_iszSpriteName ) )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		return;
	}
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();

	pev->dmgtime = gpGlobals->time;

	if( ServerSide() )
	{
		SetThink( NULL );
		if( pev->dmg > 0 )
		{
			SetThink( &CLightning::DamageThink );
			pev->nextthink = gpGlobals->time + 0.1f;
		}
		if( pev->targetname )
		{
			if( !( pev->spawnflags & SF_BEAM_STARTON ) )
			{
				pev->effects = EF_NODRAW;
				m_active = 0;
				pev->nextthink = 0;
			}
			else
				m_active = 1;
		
			SetUse( &CLightning::ToggleUse );
		}
	}
	else
	{
		m_active = 0;
		if( !FStringNull( pev->targetname ) )
		{
			SetUse( &CLightning::StrikeUse );
		}
		if( FStringNull( pev->targetname ) || FBitSet( pev->spawnflags, SF_BEAM_STARTON ) )
		{
			SetThink( &CLightning::StrikeThink );
			pev->nextthink = gpGlobals->time + 1.0f;
		}
	}
}

void CLightning::Precache( void )
{
	m_spriteTexture = PRECACHE_MODEL( STRING( m_iszSpriteName ) );
	CBeam::Precache();
}

void CLightning::Activate( void )
{
	if( ServerSide() )
		BeamUpdateVars();
}

void CLightning::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "LightningStart" ) )
	{
		m_iszStartEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "LightningEnd" ) )
	{
		m_iszEndEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "life" ) )
	{
		m_life = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "BoltWidth" ) )
	{
		m_boltWidth = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "NoiseAmplitude" ) )
	{
		m_noiseAmplitude = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TextureScroll" ) )
	{
		m_speed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "StrikeTime" ) )
	{
		m_restrike = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		m_iszSpriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "framestart" ) )
	{
		m_frameStart = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "Radius" ) )
	{
		m_radius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBeam::KeyValue( pkvd );
}

void CLightning::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_active ) )
		return;
	if( m_active )
	{
		m_active = 0;
		pev->effects |= EF_NODRAW;
		pev->nextthink = 0;
	}
	else
	{
		m_active = 1;
		pev->effects &= ~EF_NODRAW;
		DoSparks( GetStartPos(), GetEndPos() );
		if( pev->dmg > 0 )
		{
			pev->nextthink = gpGlobals->time;
			pev->dmgtime = gpGlobals->time;
		}
	}
}

void CLightning::StrikeUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_active ) )
		return;

	if( m_active )
	{
		m_active = 0;
		SetThink( NULL );
	}
	else
	{
		SetThink( &CLightning::StrikeThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}

	if( !FBitSet( pev->spawnflags, SF_BEAM_TOGGLE ) )
		SetUse( NULL );
}

int IsPointEntity( CBaseEntity *pEnt )
{
	if( !pEnt->pev->modelindex )
		return 1;
	if( FClassnameIs( pEnt->pev, "info_target" ) || FClassnameIs( pEnt->pev, "info_landmark" ) || FClassnameIs( pEnt->pev, "path_corner" ) )
		return 1;

	return 0;
}

void CLightning::StrikeThink( void )
{
	if( m_life != 0 )
	{
		if( pev->spawnflags & SF_BEAM_RANDOM )
			pev->nextthink = gpGlobals->time + m_life + RANDOM_FLOAT( 0.0f, m_restrike );
		else
			pev->nextthink = gpGlobals->time + m_life + m_restrike;
	}
	m_active = 1;

	if( FStringNull( m_iszEndEntity ) )
	{
		if( FStringNull( m_iszStartEntity ) )
		{
			RandomArea();
		}
		else
		{
			CBaseEntity *pStart = RandomTargetname( STRING( m_iszStartEntity ) );
			if( pStart != NULL )
				RandomPoint( pStart->pev->origin );
			else
				ALERT( at_console, "env_beam: unknown entity \"%s\"\n", STRING( m_iszStartEntity ) );
		}
		return;
	}

	CBaseEntity *pStart = RandomTargetname( STRING( m_iszStartEntity ) );
	CBaseEntity *pEnd = RandomTargetname( STRING( m_iszEndEntity ) );

	if( pStart != NULL && pEnd != NULL )
	{
		if( IsPointEntity( pStart ) || IsPointEntity( pEnd ) )
		{
			if( pev->spawnflags & SF_BEAM_RING )
			{
				// don't work
				return;
			}
		}

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			if( IsPointEntity( pStart ) || IsPointEntity( pEnd ) )
			{
				if( !IsPointEntity( pEnd ) )	// One point entity must be in pEnd
				{
					CBaseEntity *pTemp;
					pTemp = pStart;
					pStart = pEnd;
					pEnd = pTemp;
				}
				if( !IsPointEntity( pStart ) )	// One sided
				{
					WRITE_BYTE( TE_BEAMENTPOINT );
					WRITE_SHORT( pStart->entindex() );
					WRITE_COORD( pEnd->pev->origin.x );
					WRITE_COORD( pEnd->pev->origin.y );
					WRITE_COORD( pEnd->pev->origin.z );
				}
				else
				{
					WRITE_BYTE( TE_BEAMPOINTS );
					WRITE_COORD( pStart->pev->origin.x );
					WRITE_COORD( pStart->pev->origin.y );
					WRITE_COORD( pStart->pev->origin.z );
					WRITE_COORD( pEnd->pev->origin.x );
					WRITE_COORD( pEnd->pev->origin.y );
					WRITE_COORD( pEnd->pev->origin.z );
				}

			}
			else
			{
				if( pev->spawnflags & SF_BEAM_RING )
					WRITE_BYTE( TE_BEAMRING );
				else
					WRITE_BYTE( TE_BEAMENTS );
				WRITE_SHORT( pStart->entindex() );
				WRITE_SHORT( pEnd->entindex() );
			}

			WRITE_SHORT( m_spriteTexture );
			WRITE_BYTE( m_frameStart ); // framestart
			WRITE_BYTE( (int)pev->framerate ); // framerate
			WRITE_BYTE( (int)( m_life * 10.0f ) ); // life
			WRITE_BYTE( m_boltWidth );  // width
			WRITE_BYTE( m_noiseAmplitude );   // noise
			WRITE_BYTE( (int)pev->rendercolor.x );   // r, g, b
			WRITE_BYTE( (int)pev->rendercolor.y );   // r, g, b
			WRITE_BYTE( (int)pev->rendercolor.z );   // r, g, b
			WRITE_BYTE( (int)pev->renderamt );	// brightness
			WRITE_BYTE( m_speed );		// speed
		MESSAGE_END();
		DoSparks( pStart->pev->origin, pEnd->pev->origin );
		if( pev->dmg > 0 )
		{
			TraceResult tr;
			UTIL_TraceLine( pStart->pev->origin, pEnd->pev->origin, dont_ignore_monsters, NULL, &tr );
			BeamDamageInstant( &tr, pev->dmg );
		}
	}
}

void CBeam::BeamDamage(TraceResult *ptr , entvars_t *pevAttacker)
{
	RelinkBeam();
	if( ptr->flFraction != 1.0f && ptr->pHit != NULL )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( ptr->pHit );
		if( pHit )
		{
			ClearMultiDamage();
			pHit->TraceAttack( pev, pev->dmg * ( gpGlobals->time - pev->dmgtime ), ( ptr->vecEndPos - pev->origin ).Normalize(), ptr, DMG_ENERGYBEAM );
			ApplyMultiDamage( pev, pevAttacker ? pevAttacker : pev );
			if( pev->spawnflags & SF_BEAM_DECALS )
			{
				if( pHit->IsBSPModel() )
					UTIL_DecalTrace( ptr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );
			}
		}
	}
	pev->dmgtime = gpGlobals->time;
}

void CLightning::DamageThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;
	TraceResult tr;
	UTIL_TraceLine( GetStartPos(), GetEndPos(), dont_ignore_monsters, NULL, &tr );
	BeamDamage( &tr );
}

void CLightning::Zap( const Vector &vecSrc, const Vector &vecDest )
{
#if 1
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( vecDest.x );
		WRITE_COORD( vecDest.y );
		WRITE_COORD( vecDest.z );
		WRITE_SHORT( m_spriteTexture );
		WRITE_BYTE( m_frameStart ); // framestart
		WRITE_BYTE( (int)pev->framerate ); // framerate
		WRITE_BYTE( (int)( m_life * 10.0f ) ); // life
		WRITE_BYTE( m_boltWidth );  // width
		WRITE_BYTE( m_noiseAmplitude );   // noise
		WRITE_BYTE( (int)pev->rendercolor.x );   // r, g, b
		WRITE_BYTE( (int)pev->rendercolor.y );   // r, g, b
		WRITE_BYTE( (int)pev->rendercolor.z );   // r, g, b
		WRITE_BYTE( (int)pev->renderamt );	// brightness
		WRITE_BYTE( m_speed );		// speed
	MESSAGE_END();
#else
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_LIGHTNING );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( vecDest.x );
		WRITE_COORD( vecDest.y );
		WRITE_COORD( vecDest.z );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 50 );
		WRITE_BYTE( 40 );
		WRITE_SHORT( m_spriteTexture );
	MESSAGE_END();
#endif
	DoSparks( vecSrc, vecDest );
}

void CLightning::RandomArea( void )
{
	int iLoops;

	for( iLoops = 0; iLoops < 10; iLoops++ )
	{
		Vector vecSrc = pev->origin;

		Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) );
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, ENT( pev ), &tr1 );

		if( tr1.flFraction == 1.0f )
			continue;

		Vector vecDir2;
		do
		{
			vecDir2 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) );
		} while( DotProduct( vecDir1, vecDir2 ) > 0.0f );
		vecDir2 = vecDir2.Normalize();
		TraceResult tr2;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir2 * m_radius, ignore_monsters, ENT( pev ), &tr2 );

		if( tr2.flFraction == 1.0f )
			continue;

		if( ( tr1.vecEndPos - tr2.vecEndPos ).Length() < m_radius * 0.1f )
			continue;

		UTIL_TraceLine( tr1.vecEndPos, tr2.vecEndPos, ignore_monsters, ENT( pev ), &tr2 );

		if( tr2.flFraction != 1.0f )
			continue;

		Zap( tr1.vecEndPos, tr2.vecEndPos );

		break;
	}
}

void CLightning::RandomPoint( Vector &vecSrc )
{
	int iLoops;

	for( iLoops = 0; iLoops < 10; iLoops++ )
	{
		Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ) );
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, ENT( pev ), &tr1 );

		if( ( tr1.vecEndPos - vecSrc ).Length() < m_radius * 0.1f )
			continue;

		if( tr1.flFraction == 1.0f )
			continue;

		Zap( vecSrc, tr1.vecEndPos );
		break;
	}
}

void CLightning::BeamUpdateVars( void )
{
	int beamType;
	int pointStart, pointEnd;

	edict_t *pStart = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszStartEntity ) );
	edict_t *pEnd = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( m_iszEndEntity ) );
	pointStart = IsPointEntity( CBaseEntity::Instance( pStart ) );
	pointEnd = IsPointEntity( CBaseEntity::Instance( pEnd ) );

	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
	pev->flags |= FL_CUSTOMENTITY;
	pev->model = m_iszSpriteName;
	SetTexture( m_spriteTexture );

	beamType = BEAM_ENTS;
	if( pointStart || pointEnd )
	{
		if( !pointStart )	// One point entity must be in pStart
		{
			edict_t *pTemp;
			// Swap start & end
			pTemp = pStart;
			pStart = pEnd;
			pEnd = pTemp;
			int swap = pointStart;
			pointStart = pointEnd;
			pointEnd = swap;
		}
		if( !pointEnd )
			beamType = BEAM_ENTPOINT;
		else
			beamType = BEAM_POINTS;
	}

	SetType( beamType );
	if( beamType == BEAM_POINTS || beamType == BEAM_ENTPOINT || beamType == BEAM_HOSE )
	{
		SetStartPos( pStart->v.origin );
		if( beamType == BEAM_POINTS || beamType == BEAM_HOSE )
			SetEndPos( pEnd->v.origin );
		else
			SetEndEntity( ENTINDEX( pEnd ) );
	}
	else
	{
		SetStartEntity( ENTINDEX( pStart ) );
		SetEndEntity( ENTINDEX( pEnd ) );
	}

	RelinkBeam();

	SetWidth( m_boltWidth );
	SetNoise( m_noiseAmplitude );
	SetFrame( m_frameStart );
	SetScrollRate( m_speed );
	if( pev->spawnflags & SF_BEAM_SHADEIN )
		SetFlags( BEAM_FSHADEIN );
	else if( pev->spawnflags & SF_BEAM_SHADEOUT )
		SetFlags( BEAM_FSHADEOUT );
}

LINK_ENTITY_TO_CLASS( env_laser, CLaser )

TYPEDESCRIPTION	CLaser::m_SaveData[] =
{
	DEFINE_FIELD( CLaser, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CLaser, m_iszSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLaser, m_firePosition, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CLaser, CBeam )

void CLaser::Spawn( void )
{
	if( FStringNull( pev->model ) )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		return;
	}
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();

	SetThink( &CLaser::StrikeThink );
	pev->flags |= FL_CUSTOMENTITY;

	PointsInit( pev->origin, pev->origin );

	if( !m_pSprite && m_iszSpriteName )
		m_pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteName ), pev->origin, TRUE );
	else
		m_pSprite = NULL;

	if( m_pSprite )
		m_pSprite->SetTransparency( kRenderGlow, (int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z, (int)pev->renderamt, (int)pev->renderfx );

	if( pev->targetname && !( pev->spawnflags & SF_BEAM_STARTON ) )
		TurnOff();
	else
		TurnOn();
}

void CLaser::Precache( void )
{
	pev->modelindex = PRECACHE_MODEL( STRING( pev->model ) );
	if( m_iszSpriteName )
		PRECACHE_MODEL( STRING( m_iszSpriteName ) );
}

void CLaser::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "LaserTarget" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "width" ) )
	{
		SetWidth( (int)atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "NoiseAmplitude" ) )
	{
		SetNoise( atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TextureScroll" ) )
	{
		SetScrollRate( atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "EndSprite" ) )
	{
		m_iszSpriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "framestart" ) )
	{
		pev->frame = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBeam::KeyValue( pkvd );
}

int CLaser::IsOn( void )
{
	if( pev->effects & EF_NODRAW )
		return 0;
	return 1;
}

void CLaser::TurnOff( void )
{
	pev->effects |= EF_NODRAW;
	pev->nextthink = 0;
	if( m_pSprite )
		m_pSprite->TurnOff();
}

void CLaser::TurnOn( void )
{
	pev->effects &= ~EF_NODRAW;
	if( m_pSprite )
		m_pSprite->TurnOn();
	pev->dmgtime = gpGlobals->time;
	pev->nextthink = gpGlobals->time;
}

void CLaser::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int active = IsOn();

	if( !ShouldToggle( useType, active ) )
		return;
	if( active )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}

void CLaser::FireAtPoint(TraceResult &tr , entvars_t *pevAttacker)
{
	SetEndPos( tr.vecEndPos );
	if( m_pSprite )
		UTIL_SetOrigin( m_pSprite->pev, tr.vecEndPos );

	BeamDamage( &tr, pevAttacker );
	DoSparks( GetStartPos(), tr.vecEndPos );
}

void CLaser::StrikeThink( void )
{
	CBaseEntity *pEnd = RandomTargetname( STRING( pev->message ) );

	if( pEnd )
		m_firePosition = pEnd->pev->origin;

	TraceResult tr;

	UTIL_TraceLine( pev->origin, m_firePosition, dont_ignore_monsters, NULL, &tr );
	FireAtPoint( tr );
	pev->nextthink = gpGlobals->time + 0.1f;
}

class CGlow : public CPointEntity
{
public:
	void Spawn( void );
	void Think( void );
	void Animate( float frames );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_lastTime;
	float m_maxFrame;
};

LINK_ENTITY_TO_CLASS( env_glow, CGlow )

TYPEDESCRIPTION	CGlow::m_SaveData[] =
{
	DEFINE_FIELD( CGlow, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CGlow, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGlow, CPointEntity )

void CGlow::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	PRECACHE_MODEL( STRING( pev->model ) );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	if( m_maxFrame > 1.0f && pev->framerate != 0 )
		pev->nextthink	= gpGlobals->time + 0.1f;

	m_lastTime = gpGlobals->time;
}

void CGlow::Think( void )
{
	Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );

	pev->nextthink = gpGlobals->time + 0.1f;
	m_lastTime = gpGlobals->time;
}

void CGlow::Animate( float frames )
{ 
	if( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame + frames, m_maxFrame );
}

LINK_ENTITY_TO_CLASS( env_sprite, CSprite )

TYPEDESCRIPTION	CSprite::m_SaveData[] =
{
	DEFINE_FIELD( CSprite, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CSprite, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSprite, CPointEntity )

void CSprite::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	if( pev->targetname && !( pev->spawnflags & SF_SPRITE_STARTON ) )
		TurnOff();
	else
		TurnOn();

	// Worldcraft only sets y rotation, copy to Z
	if( pev->angles.y != 0 && pev->angles.z == 0 )
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
}

void CSprite::Precache( void )
{
	PRECACHE_MODEL( STRING( pev->model ) );

	// Reset attachment after save/restore
	if( pev->aiment )
		SetAttachment( pev->aiment, pev->body );
	else
	{
		// Clear attachment
		pev->skin = 0;
		pev->body = 0;
	}
}

void CSprite::Activate()
{
	AttachToEntity();
	CPointEntity::Activate();
}

void CSprite::SpriteInit( const char *pSpriteName, const Vector &origin )
{
	pev->model = MAKE_STRING( pSpriteName );
	pev->origin = origin;
	Spawn();
}

CSprite *CSprite::SpriteCreate( const char *pSpriteName, const Vector &origin, BOOL animate )
{
	CSprite *pSprite = GetClassPtr( (CSprite *)NULL );
	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->pev->classname = MAKE_STRING( "env_sprite" );
	pSprite->pev->solid = SOLID_NOT;
	pSprite->pev->movetype = MOVETYPE_NOCLIP;
	if( animate )
		pSprite->TurnOn();

	return pSprite;
}

void CSprite::AnimateThink( void )
{
	Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );

	pev->nextthink = gpGlobals->time + 0.1f;
	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead( void )
{
	if( gpGlobals->time > pev->dmgtime )
		UTIL_Remove( this );
	else
	{
		AnimateThink();
		pev->nextthink = gpGlobals->time;
	}
}

void CSprite::Expand( float scaleSpeed, float fadeSpeed )
{
	pev->speed = scaleSpeed;
	pev->health = fadeSpeed;
	SetThink( &CSprite::ExpandThink );

	pev->nextthink	= gpGlobals->time;
	m_lastTime = gpGlobals->time;
}

void CSprite::ExpandThink( void )
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt -= pev->health * frametime;
	if( pev->renderamt <= 0 )
	{
		pev->renderamt = 0;
		UTIL_Remove( this );
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		m_lastTime = gpGlobals->time;
	}
}

void CSprite::Animate( float frames )
{ 
	pev->frame += frames;
	if( pev->frame > m_maxFrame )
	{
		if( FBitSet( pev->spawnflags, SF_SPRITE_ONCE|SF_SPRITE_ONCE_AND_REMOVE ) )
		{
			TurnOff();
			if (FBitSet(pev->spawnflags, SF_SPRITE_ONCE_AND_REMOVE))
			{
				SetThink(&CBaseEntity::SUB_Remove);
				pev->nextthink = gpGlobals->time;
			}
		}
		else
		{
			if( m_maxFrame > 0 )
				pev->frame = fmod( pev->frame, m_maxFrame );
		}
	}
}

void CSprite::TurnOff( void )
{
	pev->effects = EF_NODRAW;
	pev->nextthink = 0;
}

void CSprite::TurnOn( void )
{
	pev->effects = 0;
	AttachToEntity();
	if( ( pev->framerate && m_maxFrame > 1.0f ) || FBitSet( pev->spawnflags, SF_SPRITE_ONCE|SF_SPRITE_ONCE_AND_REMOVE ) )
	{
		SetThink( &CSprite::AnimateThink );
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	if ( pev->impulse == 0 )
		pev->frame = 0;
	else if ( pev->impulse < 0 )
		pev->frame = RANDOM_LONG(0, (int)m_maxFrame-1);
	else
		pev->frame = Q_min((int)m_maxFrame-1, pev->impulse);
}

void CSprite::AttachToEntity()
{
	if (pev->message)
	{
		CBaseEntity *pTemp = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
		if (pTemp)
			SetAttachment(pTemp->edict(), (int)pev->frags);
	}
}

void CSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int on = pev->effects != EF_NODRAW;
	if( ShouldToggle( useType, on ) )
	{
		if( on )
		{
			TurnOff();
		}
		else
		{
			TurnOn();
		}
	}
}

//=================================================================
// env_model: like env_sprite, except you can specify a sequence.
//=================================================================
#define SF_ENVMODEL_OFF			1
#define SF_ENVMODEL_DROPTOFLOOR	2
#define SF_ENVMODEL_SOLID		4

class CEnvModel : public CBaseAnimating
{
	void Spawn( void );
	void Precache( void );
	void Think( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void SetSequence( void );

	string_t m_iszSequence_On;
	string_t m_iszSequence_Off;
	int m_iAction_On;
	int m_iAction_Off;
	float m_flFramerate_On;
	float m_flFramerate_Off;
};

TYPEDESCRIPTION CEnvModel::m_SaveData[] =
{
	DEFINE_FIELD( CEnvModel, m_iszSequence_On, FIELD_STRING ),
	DEFINE_FIELD( CEnvModel, m_iszSequence_Off, FIELD_STRING ),
	DEFINE_FIELD( CEnvModel, m_iAction_On, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvModel, m_iAction_Off, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvModel, m_flFramerate_On, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvModel, m_flFramerate_Off, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvModel, CBaseAnimating )
LINK_ENTITY_TO_CLASS( env_model, CEnvModel )

void CEnvModel::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszSequence_On"))
	{
		m_iszSequence_On = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSequence_Off"))
	{
		m_iszSequence_Off = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_On"))
	{
		m_iAction_On = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_Off"))
	{
		m_iAction_Off = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flFramerate_On"))
	{
		m_flFramerate_On = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flFramerate_Off"))
	{
		m_flFramerate_Off = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseAnimating::KeyValue( pkvd );
	}
}

void CEnvModel :: Spawn( void )
{
	Precache();
	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetOrigin(pev, pev->origin);

	if (pev->spawnflags & SF_ENVMODEL_SOLID)
	{
		pev->solid = SOLID_SLIDEBOX;
		UTIL_SetSize(pev, Vector(-10, -10, -10), Vector(10, 10, 10));	//LRCT
	}

	if (pev->spawnflags & SF_ENVMODEL_DROPTOFLOOR)
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR ( ENT(pev) );
	}

	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );

	SetSequence();

	pev->nextthink = gpGlobals->time + 0.1;
}

void CEnvModel::Precache( void )
{
	PRECACHE_MODEL( STRING(pev->model) );
}

void CEnvModel::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (ShouldToggle(useType, !(pev->spawnflags & SF_ENVMODEL_OFF)))
	{
		if (pev->spawnflags & SF_ENVMODEL_OFF)
			pev->spawnflags &= ~SF_ENVMODEL_OFF;
		else
			pev->spawnflags |= SF_ENVMODEL_OFF;

		SetSequence();
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CEnvModel::Think( void )
{
	int iTemp;

//	ALERT(at_console, "env_model Think fr=%f\n", pev->framerate);

	StudioFrameAdvance ( ); // set m_fSequenceFinished if necessary

//	if (m_fSequenceLoops)
//	{
//		SetNextThink( 1E6 );
//		return; // our work here is done.
//	}
	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		if (pev->spawnflags & SF_ENVMODEL_OFF)
			iTemp = m_iAction_Off;
		else
			iTemp = m_iAction_On;

		switch (iTemp)
		{
//		case 1: // loop
//			pev->animtime = gpGlobals->time;
//			m_fSequenceFinished = FALSE;
//			m_flLastEventCheck = gpGlobals->time;
//			pev->frame = 0;
//			break;
		case 2: // change state
			if (pev->spawnflags & SF_ENVMODEL_OFF)
				pev->spawnflags &= ~SF_ENVMODEL_OFF;
			else
				pev->spawnflags |= SF_ENVMODEL_OFF;
			SetSequence();
			break;
		default: //remain frozen
			return;
		}
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CEnvModel :: SetSequence( void )
{
	string_t iszSeq;

	if (pev->spawnflags & SF_ENVMODEL_OFF)
		iszSeq = m_iszSequence_Off;
	else
		iszSeq = m_iszSequence_On;

	if (FStringNull(iszSeq))
		return;
	pev->sequence = LookupSequence( STRING( iszSeq ));

	if (pev->sequence == -1)
	{
		if (pev->targetname)
			ALERT( at_error, "env_model %s: unknown sequence \"%s\"\n", STRING( pev->targetname ), STRING( iszSeq ));
		else
			ALERT( at_error, "env_model: unknown sequence \"%s\"\n", STRING( pev->targetname ), STRING( iszSeq ));
		pev->sequence = 0;
	}

	pev->frame = 0;
	ResetSequenceInfo( );

	if ((pev->spawnflags & SF_ENVMODEL_OFF) && m_flFramerate_Off)
		pev->framerate = m_flFramerate_Off;
	else if (m_flFramerate_On)
		pev->framerate = m_flFramerate_On;

	if (pev->spawnflags & SF_ENVMODEL_OFF)
	{
		if (m_iAction_Off == 1)
			m_fSequenceLoops = 1;
		else
			m_fSequenceLoops = 0;
	}
	else
	{
		if (m_iAction_On == 1)
			m_fSequenceLoops = 1;
		else
			m_fSequenceLoops = 0;
	}
}

class CGibShooter : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ShootThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual CGib *CreateGib( float lifeTime );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_iGibs;
	int m_iGibCapacity;
	int m_iGibMaterial;
	int m_iGibModelIndex;
	float m_flGibVelocity;
	float m_flVariance;
	float m_flGibLife;
	string_t m_iszPosition;
	string_t m_iszVelocity;
	string_t m_iszVelFactor;
	string_t m_iszSpawnTarget;
};

TYPEDESCRIPTION CGibShooter::m_SaveData[] =
{
	DEFINE_FIELD( CGibShooter, m_iGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibCapacity, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibMaterial, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibModelIndex, FIELD_INTEGER ),
	//DEFINE_FIELD( CGibShooter, m_flGibVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter, m_flVariance, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter, m_flGibLife, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter, m_iszPosition, FIELD_STRING),
	DEFINE_FIELD( CGibShooter, m_iszVelocity, FIELD_STRING),
	DEFINE_FIELD( CGibShooter, m_iszVelFactor, FIELD_STRING),
	DEFINE_FIELD( CGibShooter, m_iszSpawnTarget, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE( CGibShooter, CBaseDelay )
LINK_ENTITY_TO_CLASS( gibshooter, CGibShooter )

void CGibShooter::Precache( void )
{
	if( g_Language == LANGUAGE_GERMAN )
	{
		m_iGibModelIndex = PRECACHE_MODEL( "models/germanygibs.mdl" );
	}
	else
	{
		m_iGibModelIndex = PRECACHE_MODEL( "models/hgibs.mdl" );
	}
}

void CGibShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_iGibs" ) )
	{
		m_iGibs = m_iGibCapacity = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_iszVelFactor = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flVariance" ) )
	{
		m_flVariance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flGibLife" ) )
	{
		m_flGibLife = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelocity"))
	{
		m_iszVelocity = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelFactor"))
	{
		m_iszVelFactor = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSpawnTarget"))
	{
		m_iszSpawnTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CGibShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	SetThink( &CGibShooter::ShootThink );
	pev->nextthink = gpGlobals->time;
}

void CGibShooter::Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if( m_flDelay == 0 )
	{
		m_flDelay = 0.1;
	}

	if( m_flGibLife == 0 )
	{
		m_flGibLife = 25;
	}

	SetMovedir( pev );
	pev->body = MODEL_FRAMES( m_iGibModelIndex );
}

CGib *CGibShooter::CreateGib( float lifeTime )
{
	if( CVAR_GET_FLOAT( "violence_hgibs" ) == 0 )
		return NULL;

	CGib *pGib = GetClassPtr( (CGib *)NULL );
	if (!pGib)
		return NULL;

	pGib->Spawn( "models/hgibs.mdl" );
	pGib->m_lifeTime = lifeTime;
	pGib->m_bloodColor = BLOOD_COLOR_RED;

	if( pev->body <= 1 )
	{
		ALERT( at_aiconsole, "GibShooter Body is <= 1!\n" );
	}

	pGib->pev->body = RANDOM_LONG( 1, pev->body - 1 );// avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}

void CGibShooter::ShootThink( void )
{
	int i;
	if (m_flDelay == 0) // LRC - delay is 0, fire them all at once.
	{
		i = m_iGibs;
	}
	else
	{
		i = 1;
		pev->nextthink = gpGlobals->time + m_flDelay;
	}

	float flGibVelocity;
	Vector baseShootDir;

	if (!FStringNull(m_iszVelFactor))
	{
		if (!TryCalcLocus_Ratio(m_hActivator, STRING(m_iszVelFactor), flGibVelocity))
			return;
	}
	else
		flGibVelocity = 1;

	if (!FStringNull(m_iszVelocity))
	{
		if (!TryCalcLocus_Velocity(this, m_hActivator, STRING(m_iszVelocity), baseShootDir))
			return;
		flGibVelocity = flGibVelocity * baseShootDir.Length();
		baseShootDir = baseShootDir.Normalize();
	}
	else
		baseShootDir = pev->movedir;

	Vector vecPos;
	if (!FStringNull(m_iszPosition))
	{
		if (!TryCalcLocus_Position(this, m_hActivator, STRING(m_iszPosition), vecPos))
			return;
	}
	else
		vecPos = pev->origin;

	while (i > 0)
	{
		Vector vecShootDir = baseShootDir;

		vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;
		vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;
		vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;

		vecShootDir = vecShootDir.Normalize();

		const float lifeTime = ( m_flGibLife * RANDOM_FLOAT( 0.95f, 1.05f ) );	// +/- 5%
		CGib *pGib = CreateGib(lifeTime);

		if( pGib )
		{
			pGib->pev->origin = vecPos;
			pGib->pev->velocity = vecShootDir * flGibVelocity;

			pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

			float thinkTime = pGib->pev->nextthink - gpGlobals->time;

			if( pGib->m_lifeTime < thinkTime )
			{
				pGib->pev->nextthink = gpGlobals->time + pGib->m_lifeTime;
				pGib->m_lifeTime = 0;
			}

			if (m_iszSpawnTarget)
				FireTargets( STRING(m_iszSpawnTarget), pGib, this, USE_TOGGLE, 0 );
		}

		i--;
		m_iGibs--;
	}

	if( m_iGibs <= 0 )
	{
		if( pev->spawnflags & SF_GIBSHOOTER_REPEATABLE )
		{
			m_iGibs = m_iGibCapacity;
			SetThink( NULL );
			pev->nextthink = gpGlobals->time;
		}
		else
		{
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time;
		}
	}
}

#define SF_ENVSHOOTER_SCALEMODELS 2
#define SF_ENVSHOOTER_DONT_WAIT_TILL_LAND 4

class CEnvShooter : public CGibShooter
{
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );

	CGib *CreateGib( float lifeTime );
};

LINK_ENTITY_TO_CLASS( env_shooter, CEnvShooter )

void CEnvShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "shootmodel" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "shootsounds" ) )
	{
		int iNoise = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
		switch( iNoise )
		{
		case 0:
			m_iGibMaterial = matGlass;
			break;
		case 1:
			m_iGibMaterial = matWood;
			break;
		case 2:
			m_iGibMaterial = matMetal;
			break;
		case 3:
			m_iGibMaterial = matFlesh;
			break;
		case 4:
			m_iGibMaterial = matRocks;
			break;
		default:
		case -1:
			m_iGibMaterial = matNone;
			break;
		}
	}
	else
	{
		CGibShooter::KeyValue( pkvd );
	}
}

void CEnvShooter::Precache( void )
{
	m_iGibModelIndex = PRECACHE_MODEL( STRING( pev->model ) );
	CBreakable::MaterialSoundPrecache( (Materials)m_iGibMaterial );
}

CGib *CEnvShooter::CreateGib( float lifeTime )
{
	CGib *pGib = GetClassPtr( (CGib *)NULL );
	if (!pGib)
		return NULL;

	pGib->Spawn( STRING( pev->model ) );
	pGib->m_lifeTime = lifeTime;

	if (FBitSet(pev->spawnflags, SF_ENVSHOOTER_DONT_WAIT_TILL_LAND))
	{
		pGib->SetThink( &CGib::StartFadeOut );
		pGib->pev->nextthink = gpGlobals->time + lifeTime;
	}

	int bodyPart = 0;

	if( pev->body > 1 )
		bodyPart = RANDOM_LONG( 0, pev->body - 1 );

	pGib->pev->body = bodyPart;
	pGib->m_bloodColor = DONT_BLEED;
	pGib->m_material = m_iGibMaterial;

	pGib->pev->rendermode = pev->rendermode;
	pGib->pev->renderamt = pev->renderamt;
	pGib->pev->rendercolor = pev->rendercolor;
	pGib->pev->renderfx = pev->renderfx;

	/*
	 * Some env_shooters in Half-Life maps have a custom scale value.
	 * It did not have any effect in original Half-Life because models did not get scaled.
	 * Now we have spirit-like scaling which may cause visual issues.
	 * To avoid unintended scaling we allow model scaling only when the corresponding flag is set.
	*/
	const char* model = STRING(pev->model);
	const char* found = strstr(model, ".mdl");
	if (found && strlen(found) == 4)
	{
		if (FBitSet(pev->spawnflags, SF_ENVSHOOTER_SCALEMODELS))
		{
			pGib->pev->scale = pev->scale;
		}
	}
	else
	{
		pGib->pev->scale = pev->scale;
	}

	pGib->pev->skin = pev->skin;

	return pGib;
}

class CTestEffect : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	// void	KeyValue( KeyValueData *pkvd );
	void EXPORT TestThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int m_iLoop;
	int m_iBeam;
	CBeam *m_pBeam[24];
	float m_flBeamTime[24];
	float m_flStartTime;
};

LINK_ENTITY_TO_CLASS( test_effect, CTestEffect )

void CTestEffect::Spawn( void )
{
	Precache();
}

void CTestEffect::Precache( void )
{
	PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CTestEffect::TestThink( void )
{
	int i;
	float t = gpGlobals->time - m_flStartTime;

	if( m_iBeam < 24 )
	{
		CBeam *pbeam = CBeam::BeamCreate( "sprites/lgtning.spr", 100 );

		TraceResult tr;

		Vector vecSrc = pev->origin;
		Vector vecDir = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) );
		vecDir = vecDir.Normalize();
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 128, ignore_monsters, ENT( pev ), &tr );

		pbeam->PointsInit( vecSrc, tr.vecEndPos );
		// pbeam->SetColor( 80, 100, 255 );
		pbeam->SetColor( 255, 180, 100 );
		pbeam->SetWidth( 100 );
		pbeam->SetScrollRate( 12 );
		
		m_flBeamTime[m_iBeam] = gpGlobals->time;
		m_pBeam[m_iBeam] = pbeam;
		m_iBeam++;
#if 0
		Vector vecMid = ( vecSrc + tr.vecEndPos ) * 0.5f;
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( vecMid.x );	// X
			WRITE_COORD( vecMid.y );	// Y
			WRITE_COORD( vecMid.z );	// Z
			WRITE_BYTE( 20 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 100 );		// b
			WRITE_BYTE( 20 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );
#endif
	}

	if( t < 3.0f )
	{
		for( i = 0; i < m_iBeam; i++ )
		{
			t = ( gpGlobals->time - m_flBeamTime[i] ) / ( 3.0f + m_flStartTime - m_flBeamTime[i] );
			m_pBeam[i]->SetBrightness( (int)( 255.0f * t ) );
			// m_pBeam[i]->SetScrollRate( 20 * t );
		}
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		for( i = 0; i < m_iBeam; i++ )
		{
			UTIL_Remove( m_pBeam[i] );
		}
		m_flStartTime = gpGlobals->time;
		m_iBeam = 0;
		// pev->nextthink = gpGlobals->time;
		SetThink( NULL );
	}
}

void CTestEffect::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CTestEffect::TestThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_flStartTime = gpGlobals->time;
}

// Blood effects
class CBlood : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	inline int Color( void )
	{
		return pev->impulse;
	}

	inline float BloodAmount( void )
	{
		return pev->dmg;
	}

	inline void SetColor( int color )
	{
		pev->impulse = color;
	}
	
	inline void SetBloodAmount( float amount )
	{
		pev->dmg = amount;
	}
	
	bool CheckBloodDirection(CBaseEntity *pActivator , Vector &bloodDir);
	bool CheckBloodPosition( CBaseEntity *pActivator, Vector& bloodPos );
private:
};

LINK_ENTITY_TO_CLASS( env_blood, CBlood )

#define SF_BLOOD_RANDOM		0x0001
#define SF_BLOOD_STREAM		0x0002
#define SF_BLOOD_PLAYER		0x0004
#define SF_BLOOD_DECAL		0x0008

void CBlood::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	SetMovedir( pev );
}

void CBlood::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "color" ) )
	{
		int color = atoi( pkvd->szValue );
		switch( color )
		{
		case 1:
			SetColor( BLOOD_COLOR_YELLOW );
			break;
		default:
			SetColor( BLOOD_COLOR_RED );
			break;
		}

		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "amount" ) )
	{
		SetBloodAmount( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

bool CBlood::CheckBloodDirection( CBaseEntity *pActivator, Vector& bloodDir )
{
	if( pev->spawnflags & SF_BLOOD_RANDOM ) {
		bloodDir = UTIL_RandomBloodVector();
		return true;
	}
	else if (pev->netname) {
		return TryCalcLocus_Velocity(this, pActivator, STRING(pev->netname), bloodDir);
	}
	else {
		bloodDir = pev->movedir;
		return true;
	}
}

bool CBlood::CheckBloodPosition( CBaseEntity *pActivator, Vector& bloodPos )
{
	if( pev->spawnflags & SF_BLOOD_PLAYER )
	{
		edict_t *pPlayer;
		if( pActivator && pActivator->IsPlayer() )
		{
			pPlayer = pActivator->edict();
		}
		else
			pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );
		if( pPlayer ) {
			bloodPos = ( pPlayer->v.origin + pPlayer->v.view_ofs ) + Vector( RANDOM_FLOAT( -10.0f, 10.0f ), RANDOM_FLOAT( -10.0f, 10.0f ), RANDOM_FLOAT( -10.0f, 10.0f ) );
			return true;
		}
		return false;
	}
	else if (pev->target)
	{
		return TryCalcLocus_Position(this, pActivator, STRING(pev->target), bloodPos);
	}

	bloodPos = pev->origin;
	return true;
}

void CBlood::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector bloodPos;
	Vector bloodDir;

	if( pev->spawnflags & SF_BLOOD_STREAM ) {
		if (CheckBloodPosition( pActivator, bloodPos ) && CheckBloodDirection( pActivator, bloodDir ))
			UTIL_BloodStream( bloodPos, bloodDir, ( Color() == BLOOD_COLOR_RED ) ? 70 : Color(), (int)BloodAmount() );
	} else {
		if (CheckBloodPosition( pActivator, bloodPos ) && CheckBloodDirection( pActivator, bloodDir ))
			UTIL_BloodDrips( bloodPos, bloodDir, Color(), (int)BloodAmount() );
	}

	if( pev->spawnflags & SF_BLOOD_DECAL )
	{
		Vector forward;
		if (CheckBloodDirection( pActivator, forward ))
		{
			Vector start;
			if (CheckBloodPosition( pActivator, start )) {
				TraceResult tr;

				UTIL_TraceLine( start, start + forward * BloodAmount() * 2, ignore_monsters, NULL, &tr );
				if( tr.flFraction != 1.0f )
					UTIL_BloodDecalTrace( &tr, Color() );
			}
		}

	}
}

// Screen shake
class CShake : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	inline float Amplitude( void )
	{
		return pev->scale;
	}

	inline float Frequency( void )
	{
		return pev->dmg_save;
	}

	inline float Duration( void )
	{
		return pev->dmg_take;
	}

	inline float Radius( void )
	{
		return pev->dmg;
	}

	inline void SetAmplitude( float amplitude )
	{
		pev->scale = amplitude;
	}

	inline void SetFrequency( float frequency )
	{
		pev->dmg_save = frequency;
	}

	inline void SetDuration( float duration )
	{
		pev->dmg_take = duration;
	}

	inline void SetRadius( float radius )
	{
		pev->dmg = radius;
	}
private:
};

LINK_ENTITY_TO_CLASS( env_shake, CShake )

// pev->scale is amplitude
// pev->dmg_save is frequency
// pev->dmg_take is duration
// pev->dmg is radius
// radius of 0 means all players
// NOTE: UTIL_ScreenShake() will only shake players who are on the ground

#define SF_SHAKE_EVERYONE	0x0001		// Don't check radius
// UNDONE: These don't work yet
#define SF_SHAKE_DISRUPT	0x0002		// Disrupt controls
#define SF_SHAKE_INAIR		0x0004		// Shake players in air

void CShake::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	if( pev->spawnflags & SF_SHAKE_EVERYONE )
		pev->dmg = 0;
}

void CShake::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "amplitude" ) )
	{
		SetAmplitude( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		SetFrequency( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		SetRadius( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CShake::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenShake( pev->origin, Amplitude(), Frequency(), Duration(), Radius() );
}


class CFade : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	inline float Duration( void )
	{
		return pev->dmg_take;
	}

	inline float HoldTime( void )
	{
		return pev->dmg_save;
	}

	inline void SetDuration( float duration )
	{
		pev->dmg_take = duration;
	}

	inline void SetHoldTime( float hold )
	{
		pev->dmg_save = hold;
	}
private:
};

LINK_ENTITY_TO_CLASS( env_fade, CFade )

// pev->dmg_take is duration
// pev->dmg_save is hold duration
#define SF_FADE_IN			0x0001		// Fade in, not out
#define SF_FADE_MODULATE		0x0002		// Modulate, don't blend
#define SF_FADE_ONLYONE			0x0004
#define SF_FADE_BLINDDIRECT		0x0008

void CFade::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
}

void CFade::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		SetHoldTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CFade::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int fadeFlags = 0;
	
	if( !( pev->spawnflags & SF_FADE_IN ) )
		fadeFlags |= FFADE_OUT;

	if( pev->spawnflags & SF_FADE_MODULATE )
		fadeFlags |= FFADE_MODULATE;

	if( pev->spawnflags & SF_FADE_ONLYONE )
	{
		if( pActivator->IsNetClient() )
		{
			if (FBitSet(pev->spawnflags, SF_FADE_BLINDDIRECT))
			{
				UTIL_ScreenFade( pev->origin, pActivator, pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags );
			}
			else
			{
				UTIL_ScreenFade( pActivator, pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags );
			}
		}
	}
	else
	{
		if (FBitSet(pev->spawnflags, SF_FADE_BLINDDIRECT))
		{
			UTIL_ScreenFadeAll( pev->origin, pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags );
		}
		else
		{
			UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags );
		}
	}
	SUB_UseTargets( this, USE_TOGGLE, 0 );
}

class CMessage : public CPointEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
private:
};

LINK_ENTITY_TO_CLASS( env_message, CMessage )

void CMessage::Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	switch( pev->impulse )
	{
	case 1:
		// Medium radius
		pev->speed = ATTN_STATIC;
		break;
	case 2:
		// Large radius
		pev->speed = ATTN_NORM;
		break;
	case 3:
		//EVERYWHERE
		pev->speed = ATTN_NONE;
		break;
	default:
	case 0: // Small radius
		pev->speed = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if( pev->scale <= 0.0f )
		pev->scale = 1.0f;
}

void CMessage::Precache( void )
{
	if( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );
}

void CMessage::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "messagesound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "messagevolume" ) )
	{
		pev->scale = atof( pkvd->szValue ) * 0.1;
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "messageattenuation" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CMessage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pPlayer = NULL;

	if( pev->spawnflags & SF_MESSAGE_ALL )
		UTIL_ShowMessageAll( STRING( pev->message ) );
	else
	{
		if( pActivator && pActivator->IsPlayer() )
			pPlayer = pActivator;
		else
			pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

		if( pPlayer )
			UTIL_ShowMessage( STRING( pev->message ), pPlayer );
	}

	if( pev->noise )
		EMIT_SOUND( edict(), CHAN_BODY, STRING( pev->noise ), pev->scale, pev->speed );

	if( pev->spawnflags & SF_MESSAGE_ONCE )
		UTIL_Remove( this );

	SUB_UseTargets( this, USE_TOGGLE, 0 );
}

//=========================================================
// FunnelEffect
//=========================================================
class CEnvFunnel : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int m_iSprite;	// Don't save, precache
};

void CEnvFunnel::Precache( void )
{
	if (pev->netname)
			m_iSprite = PRECACHE_MODEL ( STRING(pev->netname) );
		else
			m_iSprite = PRECACHE_MODEL( "sprites/flare6.spr" );
}

LINK_ENTITY_TO_CLASS( env_funnel, CEnvFunnel )

void CEnvFunnel::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector vecPos;
	if (pev->message)
	{
		if (!TryCalcLocus_Position( this, pActivator, STRING(pev->message), vecPos ))
			return;
	}
	else
		vecPos = pev->origin;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_LARGEFUNNEL );
		WRITE_COORD( vecPos.x );
		WRITE_COORD( vecPos.y );
		WRITE_COORD( vecPos.z );
		WRITE_SHORT( m_iSprite );

		if( pev->spawnflags & SF_FUNNEL_REVERSE )// funnel flows in reverse?
		{
			WRITE_SHORT( 1 );
		}
		else
		{
			WRITE_SHORT( 0 );
		}

	MESSAGE_END();

	if (!(pev->spawnflags & SF_FUNNEL_REPEATABLE))
	{
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

void CEnvFunnel::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

void CEnvFunnel::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "sprite" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

//=========================================================
// LRC -  All the particle effects from Quake 1
//=========================================================
#define SF_QUAKEFX_REPEATABLE 1
class CEnvQuakeFx : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_quakefx, CEnvQuakeFx );

void CEnvQuakeFx::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector vecPos;
	if (pev->message)
	{
		if (!TryCalcLocus_Position( this, pActivator, STRING(pev->message), vecPos )) {
			return;
		}
	}
	else
		vecPos = pev->origin;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( pev->impulse );
		WRITE_COORD( vecPos.x );
		WRITE_COORD( vecPos.y );
		WRITE_COORD( vecPos.z );
		if (pev->impulse == TE_PARTICLEBURST)
		{
			WRITE_SHORT( pev->armortype );  // radius
			WRITE_BYTE( pev->frags );		// particle colour
			WRITE_BYTE( pev->health * 10 ); // duration
		}
		else if (pev->impulse == TE_EXPLOSION2)
		{
			// these fields seem to have no effect - except that it
			// crashes when I send "0" for the number of colours..
			WRITE_BYTE( 0 ); // colour
			WRITE_BYTE( 1 ); // number of colours
		}
		else if (pev->impulse == TE_IMPLOSION)
		{
			WRITE_BYTE( pev->armortype );  // radius
			WRITE_BYTE( pev->armorvalue );  // count
			WRITE_BYTE( pev->health * 10 ); // duration
		}
	MESSAGE_END();

	if (!(pev->spawnflags & SF_QUAKEFX_REPEATABLE))
	{
		SetThink(&CEnvQuakeFx::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

//=========================================================
// Beverage Dispenser
// overloaded pev->frags, is now a flag for whether or not a can is stuck in the dispenser. 
// overloaded pev->health, is now how many cans remain in the machine.
//=========================================================
#define DEFAULT_CAN_MODEL "models/can.mdl"

class CEnvBeverage : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

void CEnvBeverage::Precache( void )
{
	PRECACHE_MODEL( FStringNull( pev->model ) ? DEFAULT_CAN_MODEL : STRING( pev->model ) );
	PRECACHE_SOUND( "weapons/g_bounce3.wav" );
}

LINK_ENTITY_TO_CLASS( env_beverage, CEnvBeverage )

void CEnvBeverage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->frags != 0 || pev->health <= 0 )
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	Vector vecPos;
	if (pev->target)
	{
		if (!TryCalcLocus_Position( this, pActivator, STRING(pev->target), vecPos ))
			return;
	}
	else
		vecPos = pev->origin;

	CBaseEntity *pCan = CBaseEntity::CreateNoSpawn( "item_sodacan", vecPos, pev->angles, edict() );

	if (pCan)
	{
		pCan->pev->model = pev->model;
		DispatchSpawn( pCan->edict() );

		pCan->pev->health = pev->weapons;

		if( pev->skin == 6 )
		{
			// random
			pCan->pev->skin = RANDOM_LONG( 0, 5 );
		}
		else
		{
			pCan->pev->skin = pev->skin;
		}
	}

	pev->frags = 1;
	pev->health--;

	//SetThink( &SUB_Remove );
	//pev->nextthink = gpGlobals->time;
}

void CEnvBeverage::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->frags = 0;

	if( pev->health == 0 )
	{
		pev->health = 10;
	}
}

//=========================================================
// Soda can
//=========================================================
class CItemSoda : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT CanThink( void );
	void EXPORT CanTouch( CBaseEntity *pOther );
};

void CItemSoda::Precache( void )
{
	PRECACHE_MODEL( FStringNull( pev->model ) ? DEFAULT_CAN_MODEL : STRING( pev->model ) );
	PRECACHE_SOUND( "weapons/g_bounce3.wav" );
}

LINK_ENTITY_TO_CLASS( item_sodacan, CItemSoda )

void CItemSoda::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;

	SET_MODEL( ENT( pev ), FStringNull( pev->model ) ? DEFAULT_CAN_MODEL : STRING( pev->model ) );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	
	SetThink( &CItemSoda::CanThink );
	pev->nextthink = gpGlobals->time + 0.5f;
}

void CItemSoda::CanThink( void )
{
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/g_bounce3.wav", 1, ATTN_NORM );

	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 8 ) );
	SetThink( NULL );
	SetTouch( &CItemSoda::CanTouch );
}

void CItemSoda::CanTouch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() )
	{
		return;
	}

	// spoit sound here
	pOther->TakeHealth( this, pev->health ? pev->health : gSkillData.sodaHeal, DMG_GENERIC );// a bit of health.

	if( !FNullEnt( pev->owner ) )
	{
		// tell the machine the can was taken
		pev->owner->v.frags = 0;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	SetTouch( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

struct BeamParams
{
	int texture;
	int lifeMin;
	int lifeMax;
	int width;
	int noise;
	int red, green, blue, alpha;
};

static void DrawChaoticBeam(Vector vecOrigin, Vector vecDest, const BeamParams& params)
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( vecOrigin.x );
		WRITE_COORD( vecOrigin.y );
		WRITE_COORD( vecOrigin.z );
		WRITE_COORD( vecDest.x );
		WRITE_COORD( vecDest.y );
		WRITE_COORD( vecDest.z );
		WRITE_SHORT( params.texture );
		WRITE_BYTE( 0 ); // framestart
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( RANDOM_LONG(params.lifeMin, params.lifeMax) ); // life
		WRITE_BYTE( params.width );  // width
		WRITE_BYTE( params.noise );   // noise
		WRITE_BYTE( params.red );   // r, g, b
		WRITE_BYTE( params.green );   // r, g, b
		WRITE_BYTE( params.blue );   // r, g, b
		WRITE_BYTE( params.alpha );	// brightness
		WRITE_BYTE( 35 );		// speed
	MESSAGE_END();
}

static void DrawChaoticBeams(Vector vecOrigin, edict_t* pentIgnore, int radius, const BeamParams& params, int iBeams)
{
	int iTimes = 0;
	int iDrawn = 0;
	while( iDrawn < iBeams && iTimes < ( iBeams * 2 ) )
	{
		TraceResult tr;
		Vector vecDest = vecOrigin + radius * ( Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize() );
		UTIL_TraceLine( vecOrigin, vecDest, ignore_monsters, pentIgnore, &tr );
		if( tr.flFraction != 1.0 )
		{
			// we hit something.
			iDrawn++;
			DrawChaoticBeam(vecOrigin, tr.vecEndPos, params);
		}
		iTimes++;
	}

	// If drew less than half of requested beams, just draw beams without respect to the walls, but with smaller radius.
	if ( iDrawn < iBeams/2 )
	{
		iBeams = Q_min(iBeams-iDrawn, iBeams/2);
		for (int i=0; i<iBeams; ++i)
		{
			Vector vecDest = vecOrigin + radius*0.5f * Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) );
			DrawChaoticBeam(vecOrigin, vecDest, params);
		}
	}
}

//=========================================================
// env_warpball
//=========================================================
#if FEATURE_ENV_WARPBALL

#define SF_REMOVE_ON_FIRE	0x0001
#define SF_KILL_CENTER		0x0002
#define SF_WARPBALL_NOSHAKE	0x0004
#define SF_WARPBALL_DYNLIGHT	0x0008
#define SF_WARPBALL_NOSOUND	0x0010

#define WARPBALL_SPRITE "sprites/fexplo1.spr"
#define WARPBALL_BEAM "sprites/lgtning.spr"
#if FEATURE_ALIEN_TELEPORT_SOUND
#define WARPBALL_SOUND1 "debris/alien_teleport.wav"
#else
#define WARPBALL_SOUND1 "debris/beamstart2.wav"
#endif
#define WARPBALL_SOUND2 "debris/beamstart7.wav"

class CEnvWarpBall : public CBaseEntity
{
public:
	void Precache( void );
	void Spawn( void ) { Precache(); }
	void Think( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	inline float Radius() { 
		return pev->button ? pev->button : 192;
	}
	inline float Amplitude() {
		return pev->friction ? pev->friction : 6;
	}
	inline float Frequency() {
		return pev->dmg_save ? pev->dmg_save : 160;
	}
	inline float Duration() {
		return pev->dmg_take ? pev->dmg_take : 1;
	}
	inline float DamageDelay() {
		return pev->frags;
	}
	inline float Scale() {
		return pev->scale > 0 ? pev->scale : 1;
	}
	inline int MaxBeamCount() {
		return pev->team > 0 ? pev->team : 20;
	}
	inline float SoundVolume() {
		return (pev->armortype > 0.0f && pev->armortype <= 1.0f) ? pev->armortype : 1.0f;
	}
	inline string_t WarpTarget() {
		return pev->message;
	}

	inline float RenderAmount() {
		return pev->renderamt ? pev->renderamt : 255;
	}
	inline float RenderFx() {
		return pev->renderfx ? pev->renderfx : kRenderFxNoDissipation;
	}
	inline int RenderMode() {
		return pev->rendermode ? pev->rendermode : kRenderGlow;
	}
	const char* SpriteModel() {
		return pev->model ? STRING(pev->model) : WARPBALL_SPRITE;
	}

	inline void SetRadius( int radius ) {
		pev->button = radius;
	}
	inline void SetAmplitude( float amplitude ) {
		pev->friction = amplitude;
	}
	inline void SetFrequency( float frequency ) {
		pev->dmg_save = frequency;
	}
	inline void SetDuration( float duration ) {
		pev->dmg_take = duration;
	}
	inline void SetDamageDelay( float delay ) {
		pev->frags = delay;
	}
	inline void SetMaxBeamCount( int beamCount ) {
		pev->team = beamCount;
	}
	inline void SetSoundVolume( float volume ) {
		pev->armortype = volume;
	}
	inline void SetWarpTarget( string_t warpTarget ) {
		pev->message = warpTarget;
	}


	inline const char* WarpballSound1() {
		return pev->noise1 ? STRING(pev->noise1) : WARPBALL_SOUND1;
	}
	inline const char* WarpballSound2() {
#if FEATURE_ALIEN_TELEPORT_SOUND
		return pev->noise2 ? STRING(pev->noise2) : NULL;
#else
		return pev->noise2 ? STRING(pev->noise2) : WARPBALL_SOUND2;
#endif
	}
	inline float SoundAttenuation() {
		return ::SoundAttenuation((short)pev->impulse);
	}
	inline int SpriteFramerate() {
		return pev->framerate ? pev->framerate : 12;
	}

	Vector vecOrigin;
	int m_beamTexture;
};

LINK_ENTITY_TO_CLASS( env_warpball, CEnvWarpBall )

void CEnvWarpBall::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "amplitude" ) )
	{
		SetAmplitude( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		SetFrequency( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		SetRadius(atoi( pkvd->szValue ));
		pkvd->fHandled = TRUE;
	} 
	else if( FStrEq( pkvd->szKeyName, "warp_target" ) )
	{
		SetWarpTarget( ALLOC_STRING( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "damage_delay" ) )
	{
		SetDamageDelay( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "beamcolor" ) ) 
	{
		float red, green, blue;
		if (sscanf( pkvd->szValue, "%f %f %f", &red, &green, &blue) == 3) {
			pev->punchangle = Vector(red, green, blue);
		}
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "beamcount" ) )
	{
		SetMaxBeamCount( atoi(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "soundradius" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		SetSoundVolume( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvWarpBall::Precache( void )
{
	m_beamTexture = PRECACHE_MODEL( WARPBALL_BEAM );
	if (pev->model)
		PRECACHE_MODEL( STRING(pev->model) );
	else
		PRECACHE_MODEL( WARPBALL_SPRITE );

	if (pev->noise1)
		PRECACHE_SOUND(STRING(pev->noise1));
	else
		PRECACHE_SOUND( WARPBALL_SOUND1 );
	if (pev->noise2)
		PRECACHE_SOUND(STRING(pev->noise2));
#if !FEATURE_ALIEN_TELEPORT_SOUND
	else
		PRECACHE_SOUND( WARPBALL_SOUND2 );
#endif
}

void CEnvWarpBall::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	edict_t* playSoundEnt = NULL;
	bool playSoundOnMyself = false;
	string_t warpTarget = WarpTarget();

	if (useType == USE_SET && pev->dmg_inflictor != NULL)
	{
		vecOrigin = pev->vuser1;
		playSoundEnt = pev->dmg_inflictor;
	}
	else if( !FStringNull( warpTarget ) )
	{
		CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, STRING( warpTarget ), pActivator );
		if (pEntity)
		{
			vecOrigin = pEntity->pev->origin;
			playSoundEnt = pEntity->edict();
		}
		else
		{
			ALERT(at_error, "Could not find a warp target %s for %s\n", STRING(warpTarget), STRING(pev->classname));
			return;
		}
	}
	else
	{
		//use myself as center
		vecOrigin = pev->origin;
		playSoundEnt = edict();
		playSoundOnMyself = true;
	}

	if (!FBitSet(pev->spawnflags, SF_WARPBALL_NOSOUND))
	{
		if (playSoundOnMyself)
			EMIT_SOUND( edict(), CHAN_BODY, WarpballSound1(), SoundVolume(), SoundAttenuation() );
		else
			//EMIT_SOUND( playSoundEnt, CHAN_BODY, WarpballSound1(), SoundVolume(), SoundAttenuation() );
			UTIL_EmitAmbientSound( playSoundEnt, vecOrigin, WarpballSound1(), SoundVolume(), SoundAttenuation(), 0, 100 );
	}
	
	if (!(pev->spawnflags & SF_WARPBALL_NOSHAKE)) {
		UTIL_ScreenShake( vecOrigin, Amplitude(), Frequency(), Duration(), Radius() );
	}

	CSprite *pSpr = CSprite::SpriteCreate( SpriteModel(), vecOrigin, TRUE );
	pSpr->AnimateAndDie( SpriteFramerate() );

	int red = pev->rendercolor.x;
	int green = pev->rendercolor.y;
	int blue = pev->rendercolor.z;
	if (!red && !green && !blue) {
		red = 77;
		green = 210;
		blue = 130;
	}

	pSpr->SetTransparency( RenderMode(),  red, green, blue, RenderAmount(), RenderFx() );
	pSpr->SetScale(Scale());

	if (pev->spawnflags & SF_WARPBALL_DYNLIGHT)
	{
		const int lifeTime = 15;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( vecOrigin.x );	// X
			WRITE_COORD( vecOrigin.y );	// Y
			WRITE_COORD( vecOrigin.z );	// Z
			WRITE_BYTE( 20 * Scale() );		// radius * 0.1
			WRITE_BYTE( red );		// r
			WRITE_BYTE( green );		// g
			WRITE_BYTE( blue );		// b
			WRITE_BYTE( lifeTime );		// time * 10
			WRITE_BYTE( lifeTime/2 );		// decay * 0.1
		MESSAGE_END();
	}

	if (!FBitSet(pev->spawnflags, SF_WARPBALL_NOSOUND))
	{
		const char* warpballSound2 = WarpballSound2();
		if (warpballSound2)
		{
			if (playSoundOnMyself)
				EMIT_SOUND( edict(), CHAN_ITEM, warpballSound2, SoundVolume(), SoundAttenuation() );
			else
				//EMIT_SOUND( playSoundEnt, CHAN_ITEM, warpballSound2, SoundVolume(), SoundAttenuation() );
				UTIL_EmitAmbientSound( playSoundEnt, vecOrigin, warpballSound2, SoundVolume(), SoundAttenuation(), 0, 100 );
		}
	}

	int beamRed = pev->punchangle.x;
	int beamGreen = pev->punchangle.y;
	int beamBlue = pev->punchangle.z;

	if (!beamRed && !beamGreen && !beamBlue) {
		beamRed = 20;
		beamGreen = 243;
		beamBlue = 20;
	}

	const int iBeams = RANDOM_LONG( MaxBeamCount()/2, MaxBeamCount() );
	BeamParams beamParams;
	beamParams.texture = m_beamTexture;
	beamParams.lifeMin = 5;
	beamParams.lifeMax = 16;
	beamParams.width = 30;
	beamParams.noise = 65;
	beamParams.red = beamRed;
	beamParams.green = beamGreen;
	beamParams.blue = beamBlue;
	beamParams.alpha = 220;
	DrawChaoticBeams(vecOrigin, ENT(pev), Radius(), beamParams, iBeams);

	pev->nextthink = gpGlobals->time + DamageDelay();
}

void CEnvWarpBall::Think( void )
{
	SUB_UseTargets( this, USE_TOGGLE, 0 );

	if( pev->spawnflags & SF_KILL_CENTER )
	{
		CBaseEntity *pMonster = NULL;

		while( ( pMonster = UTIL_FindEntityInSphere( pMonster, vecOrigin, 72 ) ) != NULL )
		{
			if( FBitSet( pMonster->pev->flags, FL_MONSTER ) || FClassnameIs( pMonster->pev, "player" ) )
				pMonster->TakeDamage ( pev, pev, 100, DMG_GENERIC );
		}
	}
	if( pev->spawnflags & SF_REMOVE_ON_FIRE )
		UTIL_Remove( this );
}
#endif
//=========================================================
// env_xenmaker
//=========================================================
#if FEATURE_ENV_XENMAKER

#define SF_XENMAKER_TRYONCE 1
#define SF_XENMAKER_NOSPAWN 2

#define XENMAKER_SPRITE1 "sprites/fexplo1.spr"
#define XENMAKER_SPRITE2 "sprites/xflare1.spr"
#define XENMAKER_BEAM "sprites/lgtning.spr"
#if FEATURE_ALIEN_TELEPORT_SOUND
#define XENMAKER_SOUND1 "debris/alien_teleport.wav"
#else
#define XENMAKER_SOUND1 "debris/beamstart7.wav"
#endif
#define XENMAKER_SOUND2 "debris/beamstart2.wav"

class CEnvXenMaker : public CBaseEntity
{
public:
	void Precache( void );
	void Spawn( void ) { Precache(); }

	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void PlaySecondSound( edict_t* posEnt );

	void EXPORT TrySpawn();
	void EXPORT PlaySecondSoundThink();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMonsterClassname;

	int m_flBeamRadius;
	int m_iBeamAlpha;
	int m_iBeamCount;
	Vector m_vBeamColor;

	int m_flLightRadius;
	Vector m_vLightColor;

	int m_flStartSpriteFramerate;
	float m_flStartSpriteScale;
	int m_iStartSpriteAlpha;
	Vector m_vStartSpriteColor;

	int m_flEndSpriteFramerate;
	float m_flEndSpriteScale;
	int m_iEndSpriteAlpha;
	Vector m_vEndSpriteColor;

	float m_flGround;

	Vector m_defaultMinHullSize;
	Vector m_defaultMaxHullSize;

	int m_beamTexture;
};

LINK_ENTITY_TO_CLASS(env_xenmaker, CEnvXenMaker)

TYPEDESCRIPTION	CEnvXenMaker::m_SaveData[] =
{
	DEFINE_FIELD(CEnvXenMaker, m_iszMonsterClassname, FIELD_STRING),
	DEFINE_FIELD(CEnvXenMaker, m_flBeamRadius, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_iBeamAlpha, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_iBeamCount, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_vBeamColor, FIELD_VECTOR),
	DEFINE_FIELD(CEnvXenMaker, m_flLightRadius, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_vLightColor, FIELD_VECTOR),
	DEFINE_FIELD(CEnvXenMaker, m_flStartSpriteFramerate, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_flStartSpriteScale, FIELD_FLOAT),
	DEFINE_FIELD(CEnvXenMaker, m_iStartSpriteAlpha, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_vStartSpriteColor, FIELD_VECTOR),
	DEFINE_FIELD(CEnvXenMaker, m_flEndSpriteFramerate, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_flEndSpriteScale, FIELD_FLOAT),
	DEFINE_FIELD(CEnvXenMaker, m_iEndSpriteAlpha, FIELD_INTEGER),
	DEFINE_FIELD(CEnvXenMaker, m_vEndSpriteColor, FIELD_VECTOR),
	DEFINE_FIELD(CEnvXenMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD(CEnvXenMaker, m_defaultMinHullSize, FIELD_VECTOR),
	DEFINE_FIELD(CEnvXenMaker, m_defaultMaxHullSize, FIELD_VECTOR),
};
IMPLEMENT_SAVERESTORE( CEnvXenMaker, CBaseEntity )

void CEnvXenMaker::Precache()
{
	m_beamTexture = PRECACHE_MODEL(XENMAKER_BEAM);
	if (!FBitSet(pev->spawnflags, SF_XENMAKER_NOSPAWN) && !FStringNull(m_iszMonsterClassname))
	{
		UTIL_PrecacheMonster(STRING(m_iszMonsterClassname), FALSE, &m_defaultMinHullSize, &m_defaultMaxHullSize);
	}
	PRECACHE_SOUND(XENMAKER_SOUND1);
#if !FEATURE_ALIEN_TELEPORT_SOUND
	PRECACHE_SOUND(XENMAKER_SOUND2);
#endif
	PRECACHE_MODEL(XENMAKER_SPRITE1);
	PRECACHE_MODEL(XENMAKER_SPRITE2);
}

void CEnvXenMaker::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "monstertype"))
	{
		m_iszMonsterClassname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flBeamRadius"))
	{
		m_flBeamRadius = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iBeamAlpha"))
	{
		m_iBeamAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iBeamCount"))
	{
		m_iBeamCount = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vBeamColor"))
	{
		UTIL_StringToVector(m_vBeamColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flLightRadius"))
	{
		m_flLightRadius = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vLightColor"))
	{
		UTIL_StringToVector(m_vLightColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flStartSpriteFramerate"))
	{
		m_flStartSpriteFramerate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flStartSpriteScale"))
	{
		m_flStartSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartSpriteAlpha"))
	{
		m_iStartSpriteAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vStartSpriteColor"))
	{
		UTIL_StringToVector(m_vStartSpriteColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flEndSpriteFramerate"))
	{
		m_flEndSpriteFramerate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flEndSpriteScale"))
	{
		m_flEndSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iEndSpriteAlpha"))
	{
		m_iEndSpriteAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vEndSpriteColor"))
	{
		UTIL_StringToVector(m_vEndSpriteColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvXenMaker::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	TrySpawn();
}

void CEnvXenMaker::TrySpawn()
{
	//use myself as center
	edict_t *posEnt = edict();
	Vector vecOrigin = pev->origin;

	const bool asTemplate = pev->dmg_inflictor != NULL;

	if (asTemplate)
	{
		// used as template
		posEnt = pev->dmg_inflictor;
		vecOrigin = pev->vuser1;
	}

	if (!FBitSet(pev->spawnflags, SF_XENMAKER_NOSPAWN)
			&& !asTemplate) // never spawn if xenmaker is used as a template for monstermaker
	{
		if( !m_flGround )
		{
			// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
			TraceResult tr;

			UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
			m_flGround = tr.vecEndPos.z;
		}

		Vector minHullSize = Vector( -34, -34, 0 );
		Vector maxHullSize = Vector( 34, 34, 0 );

		if (m_defaultMinHullSize != g_vecZero)
		{
			minHullSize = Vector( m_defaultMinHullSize.x, m_defaultMinHullSize.y, 0 );
		}
		if (m_defaultMaxHullSize != g_vecZero)
		{
			maxHullSize = Vector( m_defaultMaxHullSize.x, m_defaultMaxHullSize.y, 0 );
		}

		Vector mins = pev->origin + minHullSize;
		Vector maxs = pev->origin + maxHullSize;
		maxs.z = pev->origin.z;
		mins.z = m_flGround;

		CBaseEntity *pList[2];
		int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
		if( count )
		{
			// don't build a stack of monsters!
			if (!FBitSet(pev->spawnflags, SF_XENMAKER_TRYONCE))
			{
				SetThink(&CEnvXenMaker::TrySpawn);
				pev->nextthink = gpGlobals->time + 0.3;
			}
			return;
		}

		edict_t	*pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );

		if( FNullEnt( pent ) )
		{
			ALERT ( at_console, "NULL Ent in env_xenmaker!\n" );
			return;
		}

		entvars_t *pevCreate = VARS( pent );
		pevCreate->origin = pev->origin;
		pevCreate->angles = pev->angles;
		SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );

		DispatchSpawn( ENT( pevCreate ) );
	}

	CSprite *pSpr = CSprite::SpriteCreate( XENMAKER_SPRITE1, vecOrigin, TRUE );
	pSpr->SetTransparency( kRenderGlow, m_vStartSpriteColor.x, m_vStartSpriteColor.y, m_vStartSpriteColor.z, m_iStartSpriteAlpha, kRenderFxNoDissipation );
	pSpr->SetScale(m_flStartSpriteScale);
	pSpr->AnimateAndDie( m_flStartSpriteFramerate );

	CSprite *pSpr2 = CSprite::SpriteCreate( XENMAKER_SPRITE2, vecOrigin, TRUE );
	pSpr2->SetTransparency( kRenderGlow, m_vEndSpriteColor.x, m_vEndSpriteColor.y, m_vEndSpriteColor.z, m_iEndSpriteAlpha, kRenderFxNoDissipation );
	pSpr2->SetScale(m_flEndSpriteScale);
	pSpr2->AnimateAndDie( m_flEndSpriteFramerate );

	BeamParams beamParams;
	beamParams.texture = m_beamTexture;
	beamParams.lifeMin = 5;
	beamParams.lifeMax = 16;
	beamParams.width = 25;
	beamParams.noise = 50;
	beamParams.red = m_vBeamColor.x;
	beamParams.green = m_vBeamColor.y;
	beamParams.blue = m_vBeamColor.z;
	beamParams.alpha = m_iBeamAlpha;

	DrawChaoticBeams(vecOrigin, ENT(pev), m_flBeamRadius, beamParams, m_iBeamCount);

	int red = m_vLightColor.x;
	int green = m_vLightColor.y;
	int blue = m_vLightColor.z;

	const int lifeTime = 15;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecOrigin.x );	// X
		WRITE_COORD( vecOrigin.y );	// Y
		WRITE_COORD( vecOrigin.z );	// Z
		WRITE_BYTE( m_flLightRadius * 0.1 );		// radius * 0.1
		WRITE_BYTE( red );		// r
		WRITE_BYTE( green );		// g
		WRITE_BYTE( blue );		// b
		WRITE_BYTE( lifeTime );		// time * 10
		WRITE_BYTE( lifeTime/2 );		// decay * 0.1
	MESSAGE_END();

	EMIT_SOUND( posEnt, CHAN_ITEM, XENMAKER_SOUND1, 1, ATTN_NORM );

#if !FEATURE_ALIEN_TELEPORT_SOUND
	if (asTemplate)
	{
		PlaySecondSound(posEnt);
	}
	else
	{
		SetThink(&CEnvXenMaker::PlaySecondSoundThink);
		pev->nextthink = gpGlobals->time + 0.8;
	}
#endif
}

void CEnvXenMaker::PlaySecondSoundThink()
{
	PlaySecondSound(edict());
}

void CEnvXenMaker::PlaySecondSound(edict_t* posEnt)
{
	EMIT_SOUND( posEnt, CHAN_BODY, XENMAKER_SOUND2, 1, ATTN_NORM );
}
#endif


#include "displacerball.h"
#include "shockbeam.h"
#include "spore.h"

enum
{
	BLOWERCANNON_SQUIDSPIT = 0,
	BLOWERCANNON_SPOREROCKET = 1,
	BLOWERCANNON_SPOREGRENADE,
	BLOWERCANNON_SHOCKBEAM,
	BLOWERCANNON_DISPLACERBALL,
};

enum
{
	BLOWERCANNON_TOGGLE = 1,
	BLOWERCANNON_FIRE,
};

class CBlowerCannon : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue(KeyValueData* pkvd);
	void EXPORT BlowerCannonThink( void );
	void EXPORT BlowerCannonStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT BlowerCannonStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	static TYPEDESCRIPTION m_SaveData[];

	short m_iWeapType;
	short m_iFireType;
	int m_iZOffset;
	string_t m_iszOwner;
};

LINK_ENTITY_TO_CLASS(env_blowercannon, CBlowerCannon)

TYPEDESCRIPTION	CBlowerCannon::m_SaveData[] =
{
	DEFINE_FIELD(CBlowerCannon, m_iFireType, FIELD_SHORT),
	DEFINE_FIELD(CBlowerCannon, m_iWeapType, FIELD_SHORT),
	DEFINE_FIELD(CBlowerCannon, m_iZOffset, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_iszOwner, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE( CBlowerCannon, CBaseDelay )


void CBlowerCannon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "firetype"))
	{
		m_iFireType = (short)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weaptype"))
	{
		m_iWeapType = (short)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "zoffset"))
	{
		m_iZOffset = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "position"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "direction"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "projectile_owner"))
	{
		m_iszOwner = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CBlowerCannon::Spawn(void)
{
	Precache();
	if (m_flDelay <= 0.0f && m_iFireType != BLOWERCANNON_FIRE)
		m_flDelay = 1.0f;
	SetUse( &CBlowerCannon::BlowerCannonStart );
}

void CBlowerCannon::Precache( void )
{
	switch (m_iWeapType) {
	case BLOWERCANNON_SQUIDSPIT:
		UTIL_PrecacheOther("squidspit");
		break;
	case BLOWERCANNON_SHOCKBEAM:
#if FEATURE_SHOCKBEAM
		UTIL_PrecacheOther( "shock_beam" );
#endif
		break;
	case BLOWERCANNON_DISPLACERBALL:
#if FEATURE_DISPLACER
		UTIL_PrecacheOther( "displacer_ball" );
#endif
		break;
	case BLOWERCANNON_SPOREGRENADE:
	case BLOWERCANNON_SPOREROCKET:
#if FEATURE_SPOREGRENADE
		UTIL_PrecacheOther( "spore" );
#endif
		break;
	default:
		break;
	}
}

void CBlowerCannon::BlowerCannonStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	SetUse( &CBlowerCannon::BlowerCannonStop );
	SetThink( &CBlowerCannon::BlowerCannonThink );
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CBlowerCannon::BlowerCannonStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStart );
	SetThink( NULL );
}

void CBlowerCannon::BlowerCannonThink( void )
{
	Vector position;
	Vector direction;
	Vector angles;
	bool evaluated = true;

	if (pev->message)
	{
		evaluated = TryCalcLocus_Position(this, m_hActivator, STRING(pev->message), position);
	}
	else
	{
		position = pev->origin;
	}

	if( evaluated )
	{
		if (pev->netname)
		{
			evaluated = TryCalcLocus_Velocity(this, m_hActivator, STRING(pev->netname), direction);
			if (evaluated)
			{
				direction.z += m_iZOffset;
				direction = direction.Normalize();
				angles = UTIL_VecToAngles( direction );
				angles.z = -angles.z;
			}
		}
		else
		{
			if (!FStringNull(pev->target))
			{
				CBaseEntity *pTarget = GetNextTarget();
				if (pTarget)
				{
					direction = pTarget->pev->origin - position;
					direction.z += m_iZOffset;
					direction = direction.Normalize();
					angles = UTIL_VecToAngles( direction );
					angles.z = -angles.z;
				}
				else
				{
					evaluated = false;
				}
			}
			else
			{
				angles = pev->angles;
				UTIL_MakeVectors(angles);
				direction = gpGlobals->v_forward;
			}
		}

		if ( evaluated )
		{
			CBaseEntity* owner = NULL;
			if (m_iszOwner)
			{
				owner = UTIL_FindEntityByTargetname(NULL, STRING(m_iszOwner), m_hActivator);
			}
			if (!owner)
				owner = this;

			switch (m_iWeapType)
			{
			case BLOWERCANNON_SQUIDSPIT:
				CSquidSpit::Shoot(owner->pev, position, direction * CSquidSpit::SpitSpeed());
				break;
#if FEATURE_SPOREGRENADE
			case BLOWERCANNON_SPOREROCKET:
				CSpore::ShootContact(owner, position, angles, direction * CSpore::SporeRocketSpeed());
				break;
			case BLOWERCANNON_SPOREGRENADE:
				CSpore::ShootTimed(owner, position, angles, direction * CSpore::SporeGrenadeSpeed());
				break;
#endif
#if FEATURE_SHOCKBEAM
			case BLOWERCANNON_SHOCKBEAM:
				CShock::Shoot(owner->pev, angles, position, direction * CShock::ShockSpeed());
				break;
#endif
#if FEATURE_DISPLACER
			case BLOWERCANNON_DISPLACERBALL:
				CDisplacerBall::Shoot(owner->pev, position, direction * CDisplacerBall::BallSpeed(), angles);
				break;
#endif
			default:
				ALERT(at_console, "Unknown projectile type in blowercannon: %d\n", m_iWeapType);
				break;
			}
		}
	}
	if( m_iFireType == BLOWERCANNON_FIRE )
	{
		SetUse( &CBlowerCannon::BlowerCannonStart );
		SetThink( NULL );
	}

	pev->nextthink = gpGlobals->time + m_flDelay;
}

//==================================================================
//LRC- Shockwave effect, like when a Houndeye attacks.
//==================================================================
#define SF_SHOCKWAVE_CENTERED 1
#define SF_SHOCKWAVE_REPEATABLE 2

class CEnvShockwave : public CPointEntity
{
public:
	void	Precache( void );
	void	Spawn( void ) { Precache(); }
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void DoEffect( Vector vecPos );

	int m_iTime;
	int m_iRadius;
	int	m_iHeight;
	int m_iScrollRate;
	int m_iNoise;
	int m_iFrameRate;
	int m_iStartFrame;
	int m_iSpriteTexture;
	char m_cType;
	string_t m_iszPosition;
};

LINK_ENTITY_TO_CLASS( env_shockwave, CEnvShockwave )

TYPEDESCRIPTION	CEnvShockwave::m_SaveData[] =
{
	DEFINE_FIELD( CEnvShockwave, m_iHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iTime, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iRadius, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iScrollRate, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iNoise, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iFrameRate, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iStartFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iSpriteTexture, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_cType, FIELD_CHARACTER ),
	DEFINE_FIELD( CEnvShockwave, m_iszPosition, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CEnvShockwave, CBaseEntity )

void CEnvShockwave::Precache( void )
{
	m_iSpriteTexture = PRECACHE_MODEL( STRING(pev->netname) );
}

void CEnvShockwave::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iTime"))
	{
		m_iTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iRadius"))
	{
		m_iRadius = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iHeight"))
	{
		m_iHeight = atoi(pkvd->szValue)/2; //LRC- the actual height is doubled when drawn
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iScrollRate"))
	{
		m_iScrollRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iNoise"))
	{
		m_iNoise = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFrameRate"))
	{
		m_iFrameRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartFrame"))
	{
		m_iStartFrame = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_cType"))
	{
		m_cType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvShockwave::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector vecPos;
	if (!FStringNull(m_iszPosition))
	{
		if (!TryCalcLocus_Position( this, pActivator, STRING(m_iszPosition), vecPos ))
			return;
	}
	else
	{
		vecPos = pev->origin;
	}

	if (!(pev->spawnflags & SF_SHOCKWAVE_CENTERED))
		vecPos.z += m_iHeight;

	int type = m_cType;
	switch (type) {
	case TE_BEAMTORUS:
	case TE_BEAMDISK:
	case TE_BEAMCYLINDER:
		break;
	default:
		type = TE_BEAMCYLINDER;
		break;
	}

	// blast circle
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( type );
		WRITE_COORD( vecPos.x );// coord coord coord (center position)
		WRITE_COORD( vecPos.y );
		WRITE_COORD( vecPos.z );
		WRITE_COORD( vecPos.x );// coord coord coord (axis and radius)
		WRITE_COORD( vecPos.y );
		WRITE_COORD( vecPos.z + m_iRadius );
		WRITE_SHORT( m_iSpriteTexture ); // short (sprite index)
		WRITE_BYTE( m_iStartFrame ); // byte (starting frame)
		WRITE_BYTE( m_iFrameRate ); // byte (frame rate in 0.1's)
		WRITE_BYTE( m_iTime ); // byte (life in 0.1's)
		WRITE_BYTE( m_iHeight );  // byte (line width in 0.1's)
		WRITE_BYTE( m_iNoise );   // byte (noise amplitude in 0.01's)
		WRITE_BYTE( pev->rendercolor.x );   // byte,byte,byte (color)
		WRITE_BYTE( pev->rendercolor.y );
		WRITE_BYTE( pev->rendercolor.z );
		WRITE_BYTE( pev->renderamt );  // byte (brightness)
		WRITE_BYTE( m_iScrollRate );	// byte (scroll speed in 0.1's)
	MESSAGE_END();

	if (!(pev->spawnflags & SF_SHOCKWAVE_REPEATABLE))
	{
		SetThink(&CEnvShockwave:: SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

//=========================================================
// LRC - Decal effect
//=========================================================
class CEnvDecal : public CPointEntity
{
public:
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_decal, CEnvDecal )

void CEnvDecal::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int iTexture = 0;

	switch(pev->impulse)
	{
		case 1: iTexture = DECAL_GUNSHOT1	+	RANDOM_LONG(0,4); break;
		case 2: iTexture = DECAL_BLOOD1		+	RANDOM_LONG(0,5); break;
		case 3: iTexture = DECAL_YBLOOD1	+	RANDOM_LONG(0,5); break;
		case 4: iTexture = DECAL_GLASSBREAK1+	RANDOM_LONG(0,2); break;
		case 5: iTexture = DECAL_BIGSHOT1	+	RANDOM_LONG(0,4); break;
		case 6: iTexture = DECAL_SCORCH1	+	RANDOM_LONG(0,1); break;
		case 7: iTexture = DECAL_SPIT1		+	RANDOM_LONG(0,1); break;
		case 8: iTexture = DECAL_SMALLSCORCH1 +	RANDOM_LONG(0,2); break;
#if FEATURE_OPFOR_DECALS
		case 9: iTexture = DECAL_SPR_SPLT1 +	RANDOM_LONG(0,2); break;
		case 10: iTexture = DECAL_OPFOR_SCORCH1 +	RANDOM_LONG(0,2); break;
		case 11: iTexture = DECAL_OPFOR_SMALLSCORCH1 +	RANDOM_LONG(0,2); break;
#else
		case 9: iTexture = DECAL_YBLOOD5 +	RANDOM_LONG(0,1); break;
		case 10: iTexture = DECAL_SCORCH1 +	RANDOM_LONG(0,1); break;
		case 11: iTexture = DECAL_SMALLSCORCH1 +	RANDOM_LONG(0,2); break;
#endif
	}

	if (pev->impulse)
		iTexture = gDecals[ iTexture ].index;
	else
		iTexture = pev->skin; // custom texture

	Vector vecPos;
	if (!FStringNull(pev->target))
	{
		if (!TryCalcLocus_Position( this, pActivator, STRING(pev->target), vecPos ))
			return;
	}
	else
	{
		vecPos = pev->origin;
	}

	Vector vecOffs;
	if (!FStringNull(pev->netname))
	{
		if (!TryCalcLocus_Velocity( this, pActivator, STRING(pev->netname), vecOffs ))
			return;
	}
	else
	{
		UTIL_MakeVectors(pev->angles);
		vecOffs = gpGlobals->v_forward;
	}

	vecOffs = vecOffs.Normalize() * 4000;

	TraceResult trace;
	int			entityIndex;

	UTIL_TraceLine( vecPos, vecPos+vecOffs, ignore_monsters, NULL, &trace );

	if (trace.flFraction == 1.0)
		return; // didn't hit anything, oh well

	entityIndex = (short)ENTINDEX(trace.pHit);

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE( TE_BSPDECAL );
		WRITE_COORD( trace.vecEndPos.x );
		WRITE_COORD( trace.vecEndPos.y );
		WRITE_COORD( trace.vecEndPos.z );
		WRITE_SHORT( iTexture );
		WRITE_SHORT( entityIndex );
		if ( entityIndex )
			WRITE_SHORT( (int)VARS(trace.pHit)->modelindex );
	MESSAGE_END();
}

void CEnvDecal::Spawn( void )
{
	if (pev->impulse == 0)
	{
		pev->skin = DECAL_INDEX( STRING(pev->noise) );
		pev->noise = iStringNull;

		if ( pev->skin == 0 )
			ALERT( at_console, "%s \"%s\" can't find decal \"%s\"\n", STRING(pev->classname), STRING(pev->noise) );
	}
}

#if FEATURE_FOG
//=========================================================
// LRC - env_fog, extended a bit from the DMC version
//=========================================================
#define SF_FOG_ACTIVE 1
#define SF_FOG_FADING 0x8000

class CEnvFog : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT ResumeThink( void );
	void EXPORT TurnOn( void );
	void EXPORT TurnOff( void );
	void EXPORT FadeInDone( void );
	void EXPORT FadeOutDone( void );
	void SendData( Vector col, int fFadeTime, int StartDist, int iEndDist);
	void KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int m_iStartDist;
	int m_iEndDist;
	float m_iFadeIn;
	float m_iFadeOut;
	float m_fHoldTime;
	float m_fFadeStart; // if we're fading in/out, then when did the fade start?
};

TYPEDESCRIPTION	CEnvFog::m_SaveData[] =
{
	DEFINE_FIELD( CEnvFog, m_iStartDist, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_iEndDist, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_iFadeIn, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_iFadeOut, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_fHoldTime, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_fFadeStart, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CEnvFog, CBaseEntity )

void CEnvFog :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "startdist"))
	{
		m_iStartDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "enddist"))
	{
		m_iEndDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_iFadeIn = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_iFadeOut = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		m_fHoldTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvFog :: Spawn ( void )
{
	pev->effects |= EF_NODRAW;

	if (pev->targetname == 0)
		pev->spawnflags |= SF_FOG_ACTIVE;

	if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		SetThink(&CEnvFog :: TurnOn );
	}

// Precache is now used only to continue after a game has loaded.
//	Precache();

	// things get messed up if we try to draw fog with a startdist
	// or an enddist of 0, so we don't allow it.
	if (m_iStartDist == 0) m_iStartDist = 1;
	if (m_iEndDist == 0) m_iEndDist = 1;
}

void CEnvFog :: Precache ( void )
{
	if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		SetThink(&CEnvFog :: ResumeThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

extern int gmsgSetFog;

void CEnvFog :: TurnOn ( void )
{
//	ALERT(at_console, "Fog turnon %f\n", gpGlobals->time);

	pev->spawnflags |= SF_FOG_ACTIVE;

	if( m_iFadeIn )
	{
		pev->spawnflags |= SF_FOG_FADING;
		SendData( pev->rendercolor, m_iFadeIn, m_iStartDist, m_iEndDist);
		pev->nextthink = gpGlobals->time + m_iFadeIn;
		SetThink(&CEnvFog :: FadeInDone );
	}
	else
	{
		pev->spawnflags &= ~SF_FOG_FADING;
		SendData( pev->rendercolor, 0, m_iStartDist, m_iEndDist);
		if (m_fHoldTime)
		{
			pev->nextthink = gpGlobals->time + m_fHoldTime;
			SetThink(&CEnvFog :: TurnOff );
		}
	}
}

void CEnvFog :: TurnOff ( void )
{
//	ALERT(at_console, "Fog turnoff\n");

	pev->spawnflags &= ~SF_FOG_ACTIVE;

	if( m_iFadeOut )
	{
		pev->spawnflags |= SF_FOG_FADING;
		SendData( pev->rendercolor, -m_iFadeOut, m_iStartDist, m_iEndDist);
		pev->nextthink = gpGlobals->time  + m_iFadeOut;
		SetThink(&CEnvFog :: FadeOutDone );
	}
	else
	{
		pev->spawnflags &= ~SF_FOG_FADING;
		SendData( g_vecZero, 0, 0, 0 );
		//DontThink();
	}
}

//yes, this intermediate think function is necessary.
// the engine seems to ignore the nextthink time when starting up.
// So this function gets called immediately after the precache finishes,
// regardless of what nextthink time is specified.
void CEnvFog :: ResumeThink ( void )
{
//	ALERT(at_console, "Fog resume %f\n", gpGlobals->time);
	SetThink(&CEnvFog ::FadeInDone);
	pev->nextthink = gpGlobals->time;
}

void CEnvFog :: FadeInDone ( void )
{
	pev->spawnflags &= ~SF_FOG_FADING;
	SendData( pev->rendercolor, 0, m_iStartDist, m_iEndDist);

	if (m_fHoldTime)
	{
		pev->nextthink = gpGlobals->time + m_fHoldTime;
		SetThink(&CEnvFog :: TurnOff );
	}
}

void CEnvFog :: FadeOutDone ( void )
{
	pev->spawnflags &= ~SF_FOG_FADING;
	SendData( g_vecZero, 0, 0, 0);
}

void CEnvFog :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (ShouldToggle(useType, pev->spawnflags & SF_FOG_ACTIVE))
	{
		if (pev->spawnflags & SF_FOG_ACTIVE)
			TurnOff();
		else
			TurnOn();
	}
}

void CEnvFog :: SendData ( Vector col, int iFadeTime, int iStartDist, int iEndDist )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgSetFog, NULL, pPlayer->pev );
				WRITE_BYTE ( col.x );
				WRITE_BYTE ( col.y );
				WRITE_BYTE ( col.z );
				WRITE_SHORT ( iFadeTime );
				WRITE_SHORT ( iStartDist );
				WRITE_SHORT ( iEndDist );
			MESSAGE_END();
		}
	}
}

LINK_ENTITY_TO_CLASS( env_fog, CEnvFog )
#endif

#define SF_BEAMTRAIL_OFF 1
class CEnvBeamTrail : public CPointEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	EXPORT StartTrailThink ( void );
	void	Affect( CBaseEntity *pTarget, USE_TYPE useType );

	int		m_iSprite;	// Don't save, precache
};

void CEnvBeamTrail::Precache ( void )
{
	//if (pev->target)
	//	PRECACHE_MODEL("sprites/null.spr");
	if (pev->netname)
		m_iSprite = PRECACHE_MODEL ( STRING(pev->netname) );
}

LINK_ENTITY_TO_CLASS( env_beamtrail, CEnvBeamTrail )

void CEnvBeamTrail::StartTrailThink ( void )
{
	pev->spawnflags |= SF_BEAMTRAIL_OFF; // fake turning off, so the Use turns it on properly
	Use(this, this, USE_ON, 0);
}

void CEnvBeamTrail::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (pev->target)
	{
		CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target), pActivator );
		while (pTarget)
		{
			Affect(pTarget, useType);
			pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(pev->target), pActivator );
		}
	}
	else
	{
		if (!ShouldToggle( useType, !(pev->spawnflags & SF_BEAMTRAIL_OFF) ))
			return;
		Affect(this, useType);
	}

	if (useType == USE_ON)
		pev->spawnflags &= ~SF_BEAMTRAIL_OFF;
	else if (useType == USE_OFF)
		pev->spawnflags |= SF_BEAMTRAIL_OFF;
	else
	{
		if (pev->spawnflags & SF_BEAMTRAIL_OFF)
			pev->spawnflags &= ~SF_BEAMTRAIL_OFF;
		else
			pev->spawnflags |= SF_BEAMTRAIL_OFF;
	}
}

void CEnvBeamTrail::Affect( CBaseEntity *pTarget, USE_TYPE useType )
{
	if (useType == USE_ON || pev->spawnflags & SF_BEAMTRAIL_OFF)
	{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT(pTarget->entindex());	// entity
			WRITE_SHORT( m_iSprite );	// model
			WRITE_BYTE( pev->health*10 ); // life
			WRITE_BYTE( pev->armorvalue );  // width
			WRITE_BYTE( pev->rendercolor.x );   // r, g, b
			WRITE_BYTE( pev->rendercolor.y );   // r, g, b
			WRITE_BYTE( pev->rendercolor.z );   // r, g, b
			WRITE_BYTE( pev->renderamt );	// brightness
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE(TE_KILLBEAM);
			WRITE_SHORT(pTarget->entindex());
		MESSAGE_END();
	}
}

void CEnvBeamTrail::Spawn( void )
{
	Precache();

	//SET_MODEL(ENT(pev), "sprites/null.spr");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	if (!(pev->spawnflags & SF_BEAMTRAIL_OFF))
	{
		SetThink(&CEnvBeamTrail::StartTrailThink);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}
