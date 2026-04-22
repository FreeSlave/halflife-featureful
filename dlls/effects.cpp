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
#include "decals.h"
#include "func_break.h"
#include "shake.h"
#include "bullsquid.h"
#include "soundradius.h"
#include "studio.h"
#include "gamerules.h"
#include "player.h"
#include "locus.h"
#include "game.h"
#include "particledef.h"
#include "soundent.h"
#include "fx_flags.h"
#include "warpball.h"
#include "combat.h"
#include "visuals_utils.h"

#define FEATURE_ENV_WARPBALL 1
#define FEATURE_ENV_XENMAKER 1

#define SF_GIBSHOOTER_REPEATABLE		1 // allows a gibshooter to be refired
#define SF_GIBSHOOTER_INSTANT_START		8

#define SF_FUNNEL_REVERSE			1 // funnel effect repels particles instead of attracting them.
#define SF_FUNNEL_REPEATABLE		2 // allows a funnel to be refired

// Lightning target, just alias landmark
LINK_ENTITY_TO_CLASS( info_target, CPointEntity )

class CBubbling : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData *pkvd ) override;

	void EXPORT FizzThink();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	const char* BubbleModel();

	int m_density;
	int m_frequency;
	int m_bubbleModel;
	bool m_state;
	string_t m_bubbleModelStr;
};

LINK_ENTITY_TO_CLASS( env_bubbles, CBubbling )

TYPEDESCRIPTION	CBubbling::m_SaveData[] =
{
	DEFINE_FIELD( CBubbling, m_density, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_frequency, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_state, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBubbling, m_bubbleModelStr, FIELD_STRING ),
	// Let spawn restore this!
	//DEFINE_FIELD( CBubbling, m_bubbleModel, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBubbling, CBaseEntity )

#define SF_BUBBLES_STARTOFF		0x0001

const char* CBubbling::BubbleModel()
{
	return FStringNull(m_bubbleModelStr) ? "sprites/bubble.spr" : STRING(m_bubbleModelStr);
}

void CBubbling::Spawn()
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
		m_state = true;
	}
	else 
		m_state = false;
}

void CBubbling::Precache()
{
	m_bubbleModel = PRECACHE_MODEL(BubbleModel());			// Precache bubble sprite
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
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		m_frequency = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "current" ) )
	{
		pev->speed = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "bubble_model" ) )
	{
		m_bubbleModelStr = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CBubbling::FizzThink()
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

void CBeam::Spawn()
{
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();
}

void CBeam::Precache()
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
const Vector &CBeam::GetStartPos()
{
	if( GetType() == BEAM_ENTS )
	{
		edict_t *pent =  g_engfuncs.pfnPEntityOfEntIndex( GetStartEntity() );
		return pent->v.origin;
	}
	return pev->origin;
}

const Vector &CBeam::GetEndPos()
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

void CBeam::EntsInit(int startIndex, int endIndex, int startAttachment, int endAttachment)
{
	SetType( BEAM_ENTS );
	SetStartEntity( startIndex );
	SetEndEntity( endIndex );
	SetStartAttachment( startAttachment );
	SetEndAttachment( endAttachment );
	RelinkBeam();
}

void CBeam::RelinkBeam()
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
void CBeam::SetObjectCollisionBox()
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

extern int gmsgCustomBeam;

class CLightning : public CBeam
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Activate() override;

	void EXPORT StrikeThink();
	void EXPORT DamageThink();
	void RandomArea();
	void RandomPoint( Vector &vecSrc );
	void Zap( const Vector &vecSrc, const Vector &vecDest );
	void EXPORT StrikeUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	inline bool ServerSide()
	{
		if( m_life == 0 && !( pev->spawnflags & SF_BEAM_RING ) )
			return true;
		return false;
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void BeamUpdateVars();
	void BeamUpdateFlags();

	void SendRingBeam(CBaseEntity* pClient);
	void SendMessages(CBaseEntity* pClient) override;

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
	void Spawn();
};

LINK_ENTITY_TO_CLASS( trip_beam, CTripBeam )

void CTripBeam::Spawn()
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

void CLightning::Spawn()
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
		BeamUpdateFlags();
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

void CLightning::Precache()
{
	m_spriteTexture = PRECACHE_MODEL( STRING( m_iszSpriteName ) );
	CBeam::Precache();
}

void CLightning::Activate()
{
	if( ServerSide() )
		BeamUpdateVars();
}

void CLightning::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "LightningStart" ) )
	{
		m_iszStartEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "LightningEnd" ) )
	{
		m_iszEndEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "life" ) )
	{
		m_life = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "BoltWidth" ) )
	{
		m_boltWidth = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "NoiseAmplitude" ) )
	{
		m_noiseAmplitude = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TextureScroll" ) )
	{
		m_speed = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "StrikeTime" ) )
	{
		m_restrike = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		m_iszSpriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "framestart" ) )
	{
		m_frameStart = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "Radius" ) )
	{
		m_radius = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = true;
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

void CLightning::StrikeThink()
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

		MESSAGE_BEGIN( MSG_BROADCAST, gmsgCustomBeam );
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
					WRITE_VECTOR( pEnd->pev->origin );
				}
				else
				{
					WRITE_BYTE( TE_BEAMPOINTS );
					WRITE_VECTOR( pStart->pev->origin );
					WRITE_VECTOR( pEnd->pev->origin );
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
			WRITE_COLOR( pev->rendercolor );   // r, g, b
			WRITE_BYTE( (int)pev->renderamt );	// brightness
			WRITE_BYTE( m_speed );		// speed
			WRITE_BYTE( GetFlags() );
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
			DamageInfo damageInfo{pev->dmg * ( gpGlobals->time - pev->dmgtime ), DMG_ENERGYBEAM};
			if (FBitSet(pev->spawnflags, SF_BEAM_NO_PUNCH))
				damageInfo.SetNoPunch();
			pHit->ApplyTraceAttack( pev, pevAttacker ? pevAttacker : pev, damageInfo, ( ptr->vecEndPos - pev->origin ).Normalize(), ptr );
			if( pev->spawnflags & SF_BEAM_DECALS )
			{
				if( pHit->IsBSPModel() )
					UTIL_DecalTrace( ptr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );
			}
		}
	}
	pev->dmgtime = gpGlobals->time;
}

void CLightning::DamageThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	TraceResult tr;
	UTIL_TraceLine( GetStartPos(), GetEndPos(), dont_ignore_monsters, NULL, &tr );
	BeamDamage( &tr );
}

void CLightning::Zap( const Vector &vecSrc, const Vector &vecDest )
{
#if 1
	MESSAGE_BEGIN( MSG_BROADCAST, gmsgCustomBeam );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_VECTOR( vecSrc );
		WRITE_VECTOR( vecDest );
		WRITE_SHORT( m_spriteTexture );
		WRITE_BYTE( m_frameStart ); // framestart
		WRITE_BYTE( (int)pev->framerate ); // framerate
		WRITE_BYTE( (int)( m_life * 10.0f ) ); // life
		WRITE_BYTE( m_boltWidth );  // width
		WRITE_BYTE( m_noiseAmplitude );   // noise
		WRITE_COLOR( pev->rendercolor );   // r, g, b
		WRITE_BYTE( (int)pev->renderamt );	// brightness
		WRITE_BYTE( m_speed );		// speed
		WRITE_BYTE( GetFlags() );
	MESSAGE_END();
#else
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_LIGHTNING );
		WRITE_VECTOR( vecSrc );
		WRITE_VECTOR( vecDest );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 50 );
		WRITE_BYTE( 40 );
		WRITE_SHORT( m_spriteTexture );
	MESSAGE_END();
#endif
	DoSparks( vecSrc, vecDest );
}

void CLightning::RandomArea()
{
	int iLoops;

	for( iLoops = 0; iLoops < 10; iLoops++ )
	{
		Vector vecSrc = pev->origin;

		const Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) ).Normalize();
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, ENT( pev ), &tr1 );

		if( tr1.flFraction == 1.0f )
			continue;

		Vector vecDir2;
		do
		{
			vecDir2 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) );
		} while( DotProduct( vecDir1, vecDir2 ) > 0.0f );
		vecDir2.NormalizeInPlace();
		TraceResult tr2;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir2 * m_radius, ignore_monsters, ENT( pev ), &tr2 );

		if( tr2.flFraction == 1.0f )
			continue;

		if( ( tr1.vecEndPos - tr2.vecEndPos ).IsLengthLessThan(m_radius * 0.1f) )
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
		const Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ) ).Normalize();
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, ENT( pev ), &tr1 );

		if( ( tr1.vecEndPos - vecSrc ).IsLengthLessThan(m_radius * 0.1f) )
			continue;

		if( tr1.flFraction == 1.0f )
			continue;

		Zap( vecSrc, tr1.vecEndPos );
		break;
	}
}

void CLightning::BeamUpdateVars()
{
	int beamType;
	int pointStart, pointEnd;

	edict_t *pStart = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszStartEntity ) );
	edict_t *pEnd = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( m_iszEndEntity ) );

	CBaseEntity* pEntStart = CBaseEntity::Instance( pStart );
	CBaseEntity* pEntEnd = CBaseEntity::Instance( pEnd );

	pointStart = pEntStart == this ? 1 : IsPointEntity( pEntStart );
	pointEnd = pEntEnd == this ? 1 : IsPointEntity( pEntEnd );

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
	BeamUpdateFlags();
}

void CLightning::BeamUpdateFlags()
{
	int flags = 0;
	if( pev->spawnflags & SF_BEAM_SHADEIN )
		flags |= BEAM_FSHADEIN;
	if( pev->spawnflags & SF_BEAM_SHADEOUT )
		flags |= BEAM_FSHADEOUT;
	if ( pev->spawnflags & SF_BEAM_SOLID )
		flags |= BEAM_FSOLID;
	if ( pev->spawnflags & SF_BEAM_SINE )
		flags |= BEAM_FSINE;
	SetFlags(flags);
}

void CLightning::SendRingBeam(CBaseEntity *pClient)
{
	CBaseEntity *pStart = RandomTargetname( STRING( m_iszStartEntity ) );
	CBaseEntity *pEnd = RandomTargetname( STRING( m_iszEndEntity ) );

	const int msgType = pClient ? MSG_ONE : MSG_BROADCAST;
	edict_t* pClientEdict = pClient ? pClient->edict() : NULL;

	if( pStart != NULL && pEnd != NULL )
	{
		if( IsPointEntity( pStart ) || IsPointEntity( pEnd ) )
		{
			// don't work
			return;
		}

		MESSAGE_BEGIN( msgType, gmsgCustomBeam, NULL, pClientEdict );
			WRITE_BYTE( TE_BEAMRING );
			WRITE_SHORT( pStart->entindex() );
			WRITE_SHORT( pEnd->entindex() );

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
			WRITE_BYTE( GetFlags() );
		MESSAGE_END();
	}
}

void CLightning::SendMessages(CBaseEntity *pClient)
{
	if ( m_active && m_life == 0 && pev->spawnflags & SF_BEAM_RING )
	{
		SendRingBeam(pClient);
	}
}

LINK_ENTITY_TO_CLASS( env_laser, CLaser )

TYPEDESCRIPTION	CLaser::m_SaveData[] =
{
	DEFINE_FIELD( CLaser, m_pStartSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CLaser, m_pEndSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CLaser, m_iszStartSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLaser, m_iszEndSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLaser, m_firePosition, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CLaser, CBeam )

CSprite* CLaser::CreateTerminalSprite(string_t spriteName)
{
	CSprite* pSprite = nullptr;
	if(spriteName)
	{
		pSprite = CSprite::SpriteCreate( STRING(spriteName), pev->origin );
		if (pSprite)
		{
			pSprite->SetTransparency( kRenderGlow, (int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z, (int)pev->renderamt, (int)pev->renderfx );
		}
	}
	return pSprite;
}

void CLaser::Spawn()
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

	m_pStartSprite = CreateTerminalSprite(m_iszStartSpriteName);
	m_pEndSprite = CreateTerminalSprite(m_iszEndSpriteName);

	if( pev->targetname && !( pev->spawnflags & SF_BEAM_STARTON ) )
		TurnOff();
	else
		TurnOn();
}

void CLaser::Precache()
{
	pev->modelindex = PRECACHE_MODEL( STRING( pev->model ) );
	if( m_iszEndSpriteName )
		PRECACHE_MODEL( STRING( m_iszEndSpriteName ) );
}

void CLaser::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "LaserTarget" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "width" ) )
	{
		SetWidth( (int)atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "NoiseAmplitude" ) )
	{
		SetNoise( atoi( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TextureScroll" ) )
	{
		SetScrollRate( atoi( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "StartSprite"))
	{
		m_iszStartSpriteName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "EndSprite" ) )
	{
		m_iszEndSpriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "framestart" ) )
	{
		pev->frame = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBeam::KeyValue( pkvd );
}

int CLaser::IsOn()
{
	if( pev->effects & EF_NODRAW )
		return 0;
	return 1;
}

void CLaser::TurnOff()
{
	pev->effects |= EF_NODRAW;
	pev->nextthink = 0;
	if( m_pStartSprite )
		m_pStartSprite->TurnOff();
	if( m_pEndSprite )
		m_pEndSprite->TurnOff();
}

void CLaser::TurnOn()
{
	pev->effects &= ~EF_NODRAW;
	if( m_pStartSprite )
		m_pStartSprite->TurnOn();
	if( m_pEndSprite )
		m_pEndSprite->TurnOn();
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
	if( m_pStartSprite )
		UTIL_SetOrigin( m_pStartSprite->pev, pev->origin );
	if( m_pEndSprite )
		UTIL_SetOrigin( m_pEndSprite->pev, tr.vecEndPos );

	BeamDamage( &tr, pevAttacker );
	DoSparks( GetStartPos(), tr.vecEndPos );
}

void CLaser::StrikeThink()
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
	void Spawn() override;
	void Think() override;
	void Animate( float frames );
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
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

void CGlow::Spawn()
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

void CGlow::Think()
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

void CSprite::Spawn()
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
	if( !FBitSet(pev->spawnflags, SF_SPRITE_DONT_MESS_YAW) && pev->angles.y != 0 && pev->angles.z == 0 )
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
}

void CSprite::Precache()
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

static CSprite* CreateAndInitSprite(const char *pSpriteName, const Vector &origin, int spawnflags = 0)
{
	CSprite *pSprite = GetClassPtr((CSprite *)nullptr);
	pSprite->pev->spawnflags = spawnflags;
	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->pev->classname = MAKE_STRING("env_sprite");
	pSprite->pev->solid = SOLID_NOT;
	pSprite->pev->movetype = MOVETYPE_NOCLIP;
	return pSprite;
}

CSprite *CSprite::SpriteCreate( const char *pSpriteName, const Vector &origin )
{
	CSprite *pSprite = CreateAndInitSprite(pSpriteName, origin);
	pSprite->TurnOn();

	return pSprite;
}

CSprite *CSprite::SpriteCreateAndAnimate(const char *pSpriteName, const Vector &origin, float framerate)
{
	CSprite *pSprite = CreateAndInitSprite(pSpriteName, origin);
	pSprite->pev->framerate = framerate;
	if (pSprite->pev->framerate > 0.0f)
		pSprite->TurnOn();

	return pSprite;
}

CSprite *CSprite::SpriteCreateAndAnimateOnce(const char *pSpriteName, const Vector &origin, float framerate)
{
	CSprite *pSprite = CreateAndInitSprite(pSpriteName, origin, SF_SPRITE_ONCE_AND_REMOVE);
	pSprite->pev->framerate = framerate;
	pSprite->TurnOn();

	return pSprite;
}

void CSprite::AnimateThink()
{
	Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );

	pev->nextthink = gpGlobals->time + 0.1f;
	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead()
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

void CSprite::ExpandThink()
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

void CSprite::TurnOff()
{
	pev->effects = EF_NODRAW;
	pev->nextthink = 0;
}

void CSprite::TurnOn()
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
	bool on = pev->effects != EF_NODRAW;
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
#define SF_ENVMODEL_CLIENTSIDEANIM	16
#define SF_ENVMODEL_IGNORE_PVS	32

class CEnvModel : public CBaseAnimating
{
	void Spawn() override;
	void Precache() override;
	void Think() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int	ObjectCaps() override { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void SetSequence();

	string_t m_iszSequence_On;
	string_t m_iszSequence_Off;
	int m_iAction_On;
	int m_iAction_Off;
	float m_flFramerate_On;
	float m_flFramerate_Off;

	float m_savedFrame;
	float m_savedAnimTime;
};

TYPEDESCRIPTION CEnvModel::m_SaveData[] =
{
	DEFINE_FIELD( CEnvModel, m_iszSequence_On, FIELD_STRING ),
	DEFINE_FIELD( CEnvModel, m_iszSequence_Off, FIELD_STRING ),
	DEFINE_FIELD( CEnvModel, m_iAction_On, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvModel, m_iAction_Off, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvModel, m_flFramerate_On, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvModel, m_flFramerate_Off, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvModel, m_savedFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvModel, m_savedAnimTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CEnvModel, CBaseAnimating )
LINK_ENTITY_TO_CLASS( env_model, CEnvModel )

void CEnvModel::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszSequence_On"))
	{
		m_iszSequence_On = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSequence_Off"))
	{
		m_iszSequence_Off = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_On"))
	{
		m_iAction_On = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_Off"))
	{
		m_iAction_Off = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flFramerate_On"))
	{
		m_flFramerate_On = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flFramerate_Off"))
	{
		m_flFramerate_Off = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseAnimating::KeyValue( pkvd );
	}
}

void CEnvModel::Spawn()
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

	if (FBitSet(pev->spawnflags, SF_ENVMODEL_IGNORE_PVS))
	{
		m_EFlags |= EFLAG_ALWAYS_SEND;
	}

	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );

	const float startingFrame = pev->frame;
	SetSequence();
	if (startingFrame < 0)
		pev->frame = RANDOM_LONG(0, 255);
	else
		pev->frame = startingFrame;

	pev->nextthink = gpGlobals->time + 0.1;
}

void CEnvModel::Precache()
{
	PRECACHE_MODEL( STRING(pev->model) );
	pev->frame = m_savedFrame;
	pev->animtime = m_savedAnimTime;
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

void CEnvModel::Think()
{
	int iTemp;

//	ALERT(at_console, "env_model Think fr=%f\n", pev->framerate);

	if (!FBitSet(pev->spawnflags, SF_ENVMODEL_CLIENTSIDEANIM))
		StudioFrameAdvance(); // set m_fSequenceFinished if necessary
	else
	{
		// Still do calculations, but save result to m_savedFrame
		float flInterval = (gpGlobals->time - m_savedAnimTime);
		if (flInterval <= 0.001)
		{
			m_savedAnimTime = gpGlobals->time;
		}
		else
		{
			if (!m_savedAnimTime)
				flInterval = 0.0;
			m_savedFrame += flInterval * m_flFrameRate * pev->framerate;
			m_savedAnimTime = gpGlobals->time;

			if (m_savedFrame < 0.0 || m_savedFrame >= 256.0)
			{
				if (m_fSequenceLoops)
					m_savedFrame -= (int)(m_savedFrame / 256.0) * 256.0;
				else
					m_savedFrame = (m_savedFrame < 0.0) ? 0 : 255;
				m_fSequenceFinished = true;
			}
		}
	}

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
//			m_fSequenceFinished = false;
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

void CEnvModel :: SetSequence()
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
			ALERT( at_error, "env_model: unknown sequence \"%s\"\n", STRING( iszSeq ));
		pev->sequence = 0;
	}

	pev->frame = 0;
	ResetSequenceInfo();

	if ((pev->spawnflags & SF_ENVMODEL_OFF) && m_flFramerate_Off)
		pev->framerate = m_flFramerate_Off;
	else if (m_flFramerate_On)
		pev->framerate = m_flFramerate_On;

	m_savedFrame = pev->frame;
	m_savedAnimTime = pev->animtime;

	if (pev->spawnflags & SF_ENVMODEL_OFF)
	{
		if (m_iAction_Off == 1)
			m_fSequenceLoops = true;
		else
			m_fSequenceLoops = false;
	}
	else
	{
		if (m_iAction_On == 1)
			m_fSequenceLoops = true;
		else
			m_fSequenceLoops = false;
	}
}

class CGibShooter : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void EXPORT ShootThink();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	virtual CBaseEntity *CreateGib(const Vector& vecPos, const Vector& vecVel, float lifeTime);

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iGibs;
	int m_iGibCapacity;
	int m_iGibMaterial;
	int m_iGibModelIndex;
	float m_flGibVelocity;
	float m_flVariance;
	float m_flGibLife;
	string_t m_iszTargetname;
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
	DEFINE_FIELD( CGibShooter, m_iszTargetname, FIELD_STRING),
	DEFINE_FIELD( CGibShooter, m_iszPosition, FIELD_STRING),
	DEFINE_FIELD( CGibShooter, m_iszVelocity, FIELD_STRING),
	DEFINE_FIELD( CGibShooter, m_iszVelFactor, FIELD_STRING),
	DEFINE_FIELD( CGibShooter, m_iszSpawnTarget, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE( CGibShooter, CBaseDelay )
LINK_ENTITY_TO_CLASS( gibshooter, CGibShooter )

void CGibShooter::Precache()
{
	m_iGibModelIndex = PRECACHE_MODEL( "models/hgibs.mdl" );
}

void CGibShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_iGibs" ) )
	{
		m_iGibs = m_iGibCapacity = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_iszVelFactor = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flVariance" ) )
	{
		m_flVariance = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flGibLife" ) )
	{
		m_flGibLife = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszTargetName"))
	{
		m_iszTargetname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelocity"))
	{
		m_iszVelocity = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelFactor"))
	{
		m_iszVelFactor = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSpawnTarget"))
	{
		m_iszSpawnTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CGibShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	if (FBitSet(pev->spawnflags, SF_GIBSHOOTER_INSTANT_START))
	{
		ShootThink();
	}
	else
	{
		SetThink( &CGibShooter::ShootThink );
		pev->nextthink = gpGlobals->time;
	}
}

void CGibShooter::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	// TODO: we comply to HL behavior here. Maybe should add a spawnflag to allow changing it?
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

CBaseEntity *CGibShooter::CreateGib(const Vector& vecPos, const Vector& vecVel, float lifeTime)
{
	if( violence_hgibs->value == 0 )
		return NULL;

	CGib *pGib = GetClassPtr( (CGib *)NULL );
	if (!pGib)
		return NULL;

	pGib->pev->origin = vecPos;
	pGib->pev->velocity = vecVel;
	pGib->m_ownerEntTemplate = m_entTemplate;

	pGib->Spawn( "models/hgibs.mdl" );
	pGib->m_lifeTime = lifeTime;
	pGib->m_bloodColor = BLOOD_COLOR_RED;

	if( pev->body <= 1 )
	{
		ALERT( at_aiconsole, "GibShooter Body is <= 1!\n" );
	}

	pGib->pev->body = RANDOM_LONG( 1, pev->body - 1 );// avoid throwing random amounts of the 0th gib. (skull).

	pGib->FinalizeGibSpawn();
	return pGib;
}

void CGibShooter::ShootThink()
{
	int i;
	if (m_flDelay <= 0) // LRC - delay is 0, fire them all at once.
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
		flGibVelocity *= baseShootDir.NormalizeInPlace();
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

		vecShootDir += gpGlobals->v_right * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;
		vecShootDir += gpGlobals->v_forward * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;
		vecShootDir += gpGlobals->v_up * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;

		vecShootDir.NormalizeInPlace();

		const float lifeTime = ( m_flGibLife * RANDOM_FLOAT( 0.95f, 1.05f ) );	// +/- 5%
		CBaseEntity *pGib = CreateGib(vecPos, vecShootDir * flGibVelocity, lifeTime);

		if( pGib )
		{
			pGib->pev->targetname = m_iszTargetname;
			if (m_iszSpawnTarget)
				FireTargets( STRING(m_iszSpawnTarget), pGib, this );
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

// Shooter particle
class CShot : public CSprite
{
public:
	void Touch(CBaseEntity* pOther) override;
	void Activate() override {
		CPointEntity::Activate();
	}
};

void CShot::Touch(CBaseEntity* pOther)
{
	if (pev->teleport_time > gpGlobals->time)
		return;
	// don't fire too often in collisions!
	// teleport_time is the soonest this can be touched again.
	pev->teleport_time = gpGlobals->time + 0.1f;

	if (pev->netname)
		FireTargets(STRING(pev->netname), this, this);
	if (pev->message && pOther && pOther->entindex() != 0)
		FireTargets(STRING(pev->message), pOther, this);
}

LINK_ENTITY_TO_CLASS(shot, CShot)

#define SF_ENVSHOOTER_SCALEMODELS 2
#define SF_ENVSHOOTER_DONT_WAIT_TILL_LAND 4

class CEnvShooter : public CGibShooter
{
	void Precache() override;
	void KeyValue( KeyValueData *pkvd ) override;

	CBaseEntity *CreateGib(const Vector& vecPos, const Vector& vecVel, float lifeTime) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszTouch;
	string_t m_iszTouchOther;
	int m_iPhysics;
	float m_fFriction;
	Vector m_vecSize;
	int m_bloodColor;
	int m_startGibBody;
};

TYPEDESCRIPTION CEnvShooter::m_SaveData[] =
{
	DEFINE_FIELD(CEnvShooter, m_iszTouch, FIELD_STRING),
	DEFINE_FIELD(CEnvShooter, m_iszTouchOther, FIELD_STRING),
	DEFINE_FIELD(CEnvShooter, m_iPhysics, FIELD_INTEGER),
	DEFINE_FIELD(CEnvShooter, m_fFriction, FIELD_FLOAT),
	DEFINE_FIELD(CEnvShooter, m_vecSize, FIELD_VECTOR),
	DEFINE_FIELD(CEnvShooter, m_bloodColor, FIELD_INTEGER),
	DEFINE_FIELD(CEnvShooter, m_startGibBody, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE( CEnvShooter, CGibShooter )
LINK_ENTITY_TO_CLASS( env_shooter, CEnvShooter )

void CEnvShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "shootmodel" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "shootsounds" ) )
	{
		int iNoise = atoi( pkvd->szValue );
		pkvd->fHandled = true;
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
	else if (FStrEq(pkvd->szKeyName, "m_iszTouch"))
	{
		m_iszTouch = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszTouchOther"))
	{
		m_iszTouchOther = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPhysics"))
	{
		m_iPhysics = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fFriction"))
	{
		m_fFriction = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vecSize"))
	{
		UTIL_StringToVector((float*)m_vecSize, pkvd->szValue);
		m_vecSize = m_vecSize / 2;
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "bloodcolor"))
	{
		m_bloodColor = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "start_gib_body"))
	{
		m_startGibBody = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CGibShooter::KeyValue( pkvd );
	}
}

void CEnvShooter::Precache()
{
	m_iGibModelIndex = PRECACHE_MODEL( STRING( pev->model ) );
	CGib::PrecacheMaterialSounds(this, m_iGibMaterial);
}

CBaseEntity *CEnvShooter::CreateGib(const Vector& vecPos, const Vector& vecVel, float lifeTime)
{
	const char* model = STRING(pev->model);
	const char* found = strstr(model, ".mdl");
	const bool isMdl = found && strlen(found) == 4;

	if (m_iPhysics <= 1)
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );
		if (!pGib)
			return NULL;

		pGib->pev->origin = vecPos;
		pGib->pev->velocity = vecVel;
		pGib->m_ownerEntTemplate = m_entTemplate;

		pGib->Spawn( STRING( pev->model ) );
		if (m_iPhysics) // sticky gib
		{
			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize(pGib->pev, Vector(0, 0, 0), Vector(0, 0, 0));
			pGib->SetTouch(&CGib::StickyGibTouch);
		}
		pGib->m_lifeTime = lifeTime;

		if (FBitSet(pev->spawnflags, SF_ENVSHOOTER_DONT_WAIT_TILL_LAND))
		{
			pGib->SetThink( &CGib::StartFadeOut );
			pGib->pev->nextthink = gpGlobals->time + lifeTime;
		}

		if (isMdl)
		{
			int bodyPart = 0;

			if (pev->body > 1)
				bodyPart = RANDOM_LONG(m_startGibBody, pev->body - 1);

			pGib->pev->body = bodyPart;
		}

		pGib->m_bloodColor = m_bloodColor > 0 ? m_bloodColor : DONT_BLEED;
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
		if (isMdl)
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

		if (pev->skin < 0)
		{
			studiohdr_t *pstudiohdr = (studiohdr_t *)GET_MODEL_PTR( pGib->edict() );
			if (pstudiohdr && pstudiohdr->ident == IDSTUDIOHEADER && pstudiohdr->numskinfamilies > 0)
				pGib->pev->skin = RANDOM_LONG(0, pstudiohdr->numskinfamilies-1);
		}
		else
		{
			pGib->pev->skin = pev->skin;
		}

		pGib->FinalizeGibSpawn();
		return pGib;
	}
	else
	{
		// special shot
		CShot* pShot = GetClassPtr((CShot*)NULL);
		//if (!m_iPhysics)
		//	pShot->pev->movetype = MOVETYPE_BOUNCE;
		pShot->pev->classname = MAKE_STRING("shot");
		pShot->pev->solid = SOLID_SLIDEBOX;
		pShot->pev->origin = vecPos;
		pShot->pev->velocity = vecVel;
		SET_MODEL(ENT(pShot->pev), STRING(pev->model));
		UTIL_SetSize(pShot->pev, -m_vecSize, m_vecSize);
		pShot->pev->renderamt = pev->renderamt;
		pShot->pev->rendermode = pev->rendermode;
		pShot->pev->rendercolor = pev->rendercolor;
		pShot->pev->renderfx = pev->renderfx;
		pShot->pev->netname = m_iszTouch;
		pShot->pev->message = m_iszTouchOther;
		if (isMdl)
		{
			pShot->pev->skin = pev->skin;
			pShot->pev->body = pev->body;
		}
		pShot->pev->scale = pev->scale;
		pShot->pev->frame = pev->frame;
		pShot->pev->framerate = pev->framerate;
		pShot->pev->friction = m_fFriction;

		switch (m_iPhysics)
		{
		case 2:
			pShot->pev->movetype = MOVETYPE_NOCLIP;
			pShot->pev->solid = SOLID_NOT;
			break;
		case 3:
			pShot->pev->movetype = MOVETYPE_FLYMISSILE;
			break;
		case 4:
			pShot->pev->movetype = MOVETYPE_BOUNCEMISSILE;
			break;
		case 5:
			pShot->pev->movetype = MOVETYPE_TOSS;
			break;
		case 6:
			pShot->pev->movetype = MOVETYPE_BOUNCE;
			break;
		}

		if (pShot->pev->framerate)
		{
			pShot->m_maxFrame = (float)MODEL_FRAMES(pShot->pev->modelindex) - 1;
			if (pShot->m_maxFrame > 1.0)
			{
				if (m_flGibLife)
				{
					pShot->pev->dmgtime = gpGlobals->time + m_flGibLife;
					pShot->SetThink(&CShot::AnimateUntilDead);
				}
				else
				{
					pShot->SetThink(&CShot::AnimateThink);
				}
				pShot->pev->nextthink = gpGlobals->time;
				pShot->m_lastTime = gpGlobals->time;
				return pShot;
			}
		}

		// if it's not animating
		if (m_flGibLife)
		{
			pShot->SetThink(&CShot::SUB_Remove);
			pShot->pev->nextthink = gpGlobals->time + m_flGibLife;
		}
		return pShot;
	}
}

class CTestEffect : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	// void	KeyValue( KeyValueData *pkvd );
	void EXPORT TestThink();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	int m_iLoop;
	int m_iBeam;
	CBeam *m_pBeam[24];
	float m_flBeamTime[24];
	float m_flStartTime;
};

LINK_ENTITY_TO_CLASS( test_effect, CTestEffect )

void CTestEffect::Spawn()
{
	Precache();
}

void CTestEffect::Precache()
{
	PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CTestEffect::TestThink()
{
	int i;
	float t = gpGlobals->time - m_flStartTime;

	if( m_iBeam < 24 )
	{
		CBeam *pbeam = CBeam::BeamCreate( "sprites/lgtning.spr", 100 );

		TraceResult tr;

		const Vector vecSrc = pev->origin;
		const Vector vecDir = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) ).Normalize();
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
			WRITE_VECTOR( vecMid );
			WRITE_BYTE( 20 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 100 );		// b
			WRITE_BYTE( 20 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END();
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
	void Spawn() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void KeyValue( KeyValueData *pkvd ) override;

	inline int Color()
	{
		return pev->impulse;
	}

	inline float BloodAmount()
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

void CBlood::Spawn()
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
		switch(color)
		{
		case 0:
			SetColor(BLOOD_COLOR_RED);
			break;
		case 1:
			SetColor(BLOOD_COLOR_YELLOW);
			break;
		default:
			SetColor(color);
			break;
		}

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "amount" ) )
	{
		SetBloodAmount( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
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
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if( pPlayer ) {
			bloodPos = ( pPlayer->pev->origin + pPlayer->pev->view_ofs ) + Vector( RANDOM_FLOAT( -10.0f, 10.0f ), RANDOM_FLOAT( -10.0f, 10.0f ), RANDOM_FLOAT( -10.0f, 10.0f ) );
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
			SendBloodEffect( bloodPos, bloodDir, Color(), (int)BloodAmount(), BLOOD_FORCED_TYPE_ONLYSTREAM );
	} else {
		if (CheckBloodPosition( pActivator, bloodPos ) && CheckBloodDirection( pActivator, bloodDir ))
			SendBloodEffect( bloodPos, bloodDir, Color(), (int)BloodAmount(), BLOOD_FORCED_TYPE_ONLYDRIPS );
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
	void Spawn() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void KeyValue( KeyValueData *pkvd ) override;

	inline float Amplitude()
	{
		return pev->scale;
	}

	inline float Frequency()
	{
		return pev->dmg_save;
	}

	inline float Duration()
	{
		return pev->dmg_take;
	}

	inline float Radius()
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

void CShake::Spawn()
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
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		SetFrequency( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		SetRadius( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CShake::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector pos = pev->origin;
	if (!FStringNull(pev->message))
	{
		if (!TryCalcLocus_Position(this, pActivator, STRING(pev->message), pos))
			return;
	}
	UTIL_ScreenShake( pos, Amplitude(), Frequency(), Duration(), Radius() );
}


class CFade : public CPointEntity
{
public:
	void Spawn() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void KeyValue( KeyValueData *pkvd ) override;

	inline float Duration()
	{
		return pev->dmg_take;
	}

	inline float HoldTime()
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

void CFade::Spawn()
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
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		SetHoldTime( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
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
				UTIL_ScreenFade( pev->origin, pActivator, pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags, true );
			}
			else
			{
				UTIL_ScreenFade( pActivator, pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags, true );
			}
		}
	}
	else
	{
		if (FBitSet(pev->spawnflags, SF_FADE_BLINDDIRECT))
		{
			UTIL_ScreenFadeAll( pev->origin, pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags, true );
		}
		else
		{
			UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags, true );
		}
	}
	SUB_UseTargets( this );
}

class CMessage : public CPointEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void KeyValue( KeyValueData *pkvd ) override;
private:
};

LINK_ENTITY_TO_CLASS( env_message, CMessage )

void CMessage::Spawn()
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

void CMessage::Precache()
{
	if( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );
}

void CMessage::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "messagesound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "messagevolume" ) )
	{
		pev->scale = atof( pkvd->szValue ) * 0.1;
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "messageattenuation" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CMessage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->spawnflags & SF_MESSAGE_ALL )
		UTIL_ShowMessageAll( STRING( pev->message ) );
	else
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if( pPlayer )
			UTIL_ShowMessage( STRING( pev->message ), pPlayer );
	}

	if( pev->noise )
		EMIT_SOUND( edict(), CHAN_BODY, STRING( pev->noise ), pev->scale, pev->speed );

	if( pev->spawnflags & SF_MESSAGE_ONCE )
		UTIL_Remove( this );

	SUB_UseTargets( this );
}

//=========================================================
// FunnelEffect
//=========================================================
class CEnvFunnel : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	int m_iSprite;	// Don't save, precache
};

void CEnvFunnel::Precache()
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
		WRITE_VECTOR( vecPos );
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

void CEnvFunnel::Spawn()
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
		pkvd->fHandled = true;
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
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
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
		WRITE_VECTOR( vecPos );
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
	void Spawn() override;
	void Precache() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	EntityOverrides SodacanOverrides() const {
		EntityOverrides entOverrides;
		entOverrides.model = pev->model;
		entOverrides.ownerEntTemplate = m_entTemplate;
		return entOverrides;
	}
};

void CEnvBeverage::Precache()
{
	UTIL_PrecacheOther("item_sodacan", SodacanOverrides());
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

	CBaseEntity *pCan = CBaseEntity::CreateNoSpawn( "item_sodacan", vecPos, pev->angles, edict(), SodacanOverrides() );

	if (pCan)
	{
		pCan->pev->model = pev->model;
		if (DispatchSpawnAutoClean(pCan))
		{
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
	}

	pev->frags = 1;
	pev->health--;

	//SetThink( &SUB_Remove );
	//pev->nextthink = gpGlobals->time;
}

void CEnvBeverage::Spawn()
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
	void Spawn() override;
	void Precache() override;
	void EXPORT CanThink();
	void EXPORT CanTouch( CBaseEntity *pOther );

	static const NamedSoundScript bounceSoundScript;
	static const NamedSoundScript drinkSoundScript;
};

const NamedSoundScript CItemSoda::bounceSoundScript = {
	CHAN_WEAPON,
	{"weapons/g_bounce3.wav"},
	"ItemSoda.Bounce"
};

const NamedSoundScript CItemSoda::drinkSoundScript = {
	CHAN_STATIC,
	{},
	"ItemSoda.Drink"
};

void CItemSoda::Precache()
{
	PrecacheMyModel(DEFAULT_CAN_MODEL);
	RegisterAndPrecacheSoundScript(bounceSoundScript);
	RegisterAndPrecacheSoundScript(drinkSoundScript);
}

LINK_ENTITY_TO_CLASS( item_sodacan, CItemSoda )

void CItemSoda::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;

	SetMyModel(DEFAULT_CAN_MODEL);
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	
	SetThink( &CItemSoda::CanThink );
	pev->nextthink = gpGlobals->time + 0.5f;
}

void CItemSoda::CanThink()
{
	EmitSoundScript(bounceSoundScript);

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
	pOther->EmitSoundScript(GetSoundScript(drinkSoundScript));
	pOther->TakeHealth( this, pev->health ? pev->health : GetSkillValue("soda"), DMG_GENERIC );// a bit of health.

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

void DrawChaoticBeam(Vector vecOrigin, Vector vecDest, const BeamParams& params)
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_VECTOR( vecOrigin );
		WRITE_VECTOR( vecDest );
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

void DrawChaoticBeams(Vector vecOrigin, edict_t* pentIgnore, int radius, const BeamParams& params, int iBeams)
{
	int iTimes = 0;
	int iDrawn = 0;
	while( iDrawn < iBeams && iTimes < ( iBeams * 2 ) )
	{
		TraceResult tr;
		const Vector vecDest = vecOrigin + radius * ( Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize() );
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

constexpr int WARPBALL_DAMAGE_RADIUS = 48;
constexpr float WARPBALL_DAMAGE = 300.0f;

class CEnvWarpBall : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override { Precache(); }
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	inline float Radius() { 
		return m_baseRadius > 0 ? m_baseRadius : 192;
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
		return m_maxBeamCount > 0 ? m_maxBeamCount : 20;
	}
	inline float SoundVolume() {
		return (m_soundVolume > 0.0f && m_soundVolume <= 1.0f) ? m_soundVolume : 1.0f;
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

	inline void SetRadius( int radius ) {
		m_baseRadius = radius;
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
		m_maxBeamCount = beamCount;
	}
	inline void SetSoundVolume( float volume ) {
		m_soundVolume = volume;
	}
	inline void SetWarpTarget( string_t warpTarget ) {
		pev->message = warpTarget;
	}

	inline const char* WarpballSound1() {
		if (!FStringNull(pev->noise1))
			return STRING(pev->noise1);
		return g_modFeatures.alien_teleport_sound ? ALIEN_TELEPORT_SOUND : WARPBALL_SOUND1;
	}
	inline const char* WarpballSound2() {
		if (FStringNull(pev->noise2))
		{
			if (g_modFeatures.alien_teleport_sound)
				return nullptr;
			return FStringNull(pev->noise1) ? WARPBALL_SOUND2 : nullptr;
		}
		else
			return STRING(pev->noise2);
	}
	inline float SoundAttenuation() {
		return ::SoundAttenuation(m_soundRadius);
	}
	inline int SpriteFramerate() {
		return pev->framerate ? pev->framerate : 12;
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxBeamCount;
	int m_baseRadius;
	int m_aiSound;
	float m_aiSoundDuration;
	float m_soundVolume;
	short m_soundRadius;

	string_t model2;
	Vector rendercolor2;
	int renderamt2;
	float scale2;
	float framerate2;

	int m_beamTexture;
};

LINK_ENTITY_TO_CLASS( env_warpball, CEnvWarpBall )

TYPEDESCRIPTION	CEnvWarpBall::m_SaveData[] =
{
	DEFINE_FIELD(CEnvWarpBall, m_maxBeamCount, FIELD_INTEGER),
	DEFINE_FIELD(CEnvWarpBall, m_baseRadius, FIELD_INTEGER),
	DEFINE_FIELD(CEnvWarpBall, m_aiSound, FIELD_INTEGER),
	DEFINE_FIELD(CEnvWarpBall, m_aiSoundDuration, FIELD_FLOAT),
	DEFINE_FIELD(CEnvWarpBall, m_soundVolume, FIELD_FLOAT),
	DEFINE_FIELD(CEnvWarpBall, m_soundRadius, FIELD_SHORT),
	DEFINE_FIELD(CEnvWarpBall, model2, FIELD_STRING),
	DEFINE_FIELD(CEnvWarpBall, rendercolor2, FIELD_VECTOR),
	DEFINE_FIELD(CEnvWarpBall, renderamt2, FIELD_INTEGER),
	DEFINE_FIELD(CEnvWarpBall, scale2, FIELD_FLOAT),
	DEFINE_FIELD(CEnvWarpBall, framerate2, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE( CEnvWarpBall, CBaseEntity )

void CEnvWarpBall::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "amplitude" ) )
	{
		SetAmplitude( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		SetFrequency( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		SetRadius(atoi( pkvd->szValue ));
		pkvd->fHandled = true;
	} 
	else if( FStrEq( pkvd->szKeyName, "warp_target" ) )
	{
		SetWarpTarget( ALLOC_STRING( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "damage_delay" ) )
	{
		SetDamageDelay( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "beamcolor" ) ) 
	{
		UTIL_StringToVector( pev->punchangle, pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "beamcount" ) )
	{
		SetMaxBeamCount( atoi(pkvd->szValue) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "soundradius" ) )
	{
		m_soundRadius = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		SetSoundVolume( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "aisound" ) )
	{
		m_aiSound = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "aisound_duration" ) )
	{
		m_aiSoundDuration = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "model2" ) )
	{
		model2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "rendercolor2" ) )
	{
		UTIL_StringToVector( rendercolor2, pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "renderamt2" ) )
	{
		renderamt2 = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "scale2" ) )
	{
		scale2 = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "framerate2" ) )
	{
		framerate2 = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvWarpBall::Precache()
{
	m_beamTexture = PRECACHE_MODEL( WARPBALL_BEAM );
	if (pev->model)
		PRECACHE_MODEL( STRING(pev->model) );
	else
		PRECACHE_MODEL( WARPBALL_SPRITE );

	if (model2)
	{
		PRECACHE_MODEL( STRING(model2) );
	}

	PRECACHE_SOUND(WarpballSound1());

	const char* sound2 = WarpballSound2();
	if (sound2 != nullptr)
		PRECACHE_SOUND(sound2);

	UTIL_PrecacheOther("warpball_hurt");
}

class CWarpballHurt : public CPointEntity
{
public:
	void Think() override;
	static void SelfCreate(const Vector& vecOrigin, float dmg, int radius, float delay, edict_t* pOwner = 0);
};

LINK_ENTITY_TO_CLASS( warpball_hurt, CWarpballHurt )

void CWarpballHurt::Think()
{
	::RadiusDamage(pev->origin, pev, pev, DamageInfo{pev->dmg, DMG_SHOCK}, pev->button, CLASS_NONE);
	UTIL_Remove(this);
}

void CWarpballHurt::SelfCreate(const Vector& vecOrigin, float dmg, int radius, float delay, edict_t* pOwner)
{
	CWarpballHurt *pWarpballHurt = GetClassPtr((CWarpballHurt *)0);
	if (pWarpballHurt)
	{
		pWarpballHurt->Spawn();
		pWarpballHurt->pev->classname = MAKE_STRING("warpball_hurt");
		UTIL_SetOrigin(pWarpballHurt->pev, vecOrigin);
		pWarpballHurt->pev->dmg = dmg;
		pWarpballHurt->pev->button = radius;
		pWarpballHurt->pev->owner = pOwner;
		pWarpballHurt->pev->nextthink = gpGlobals->time + delay;
	}
}

void CEnvWarpBall::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector vecOrigin;
	edict_t* playSoundEnt = edict();
	string_t warpTarget = WarpTarget();
	int inflictedRadius = WARPBALL_DAMAGE_RADIUS;

	if (useType == USE_SET && pev->dmg_inflictor != NULL)
	{
		vecOrigin = pev->vuser1;
		playSoundEnt = pev->dmg_inflictor;
	}
	else if( !FStringNull( warpTarget ) )
	{
		if (!TryCalcLocus_Position(this, pActivator, STRING(warpTarget), vecOrigin))
			return;
	}
	else
	{
		//use myself as center
		vecOrigin = pev->origin;
	}

	WarpballTemplate w;

	if (!FBitSet(pev->spawnflags, SF_WARPBALL_NOSOUND))
	{
		w.sound1.sound = WarpballSound1();
		w.sound1.volume = SoundVolume();
		w.sound1.attenuation = SoundAttenuation();

		if (WarpballSound2())
		{
			w.sound2.sound = WarpballSound2();
			w.sound2.volume = SoundVolume();
			w.sound2.attenuation = SoundAttenuation();
		}
	}
	
	if (!(pev->spawnflags & SF_WARPBALL_NOSHAKE)) {
		w.shake.amplitude = Amplitude();
		w.shake.frequency = Frequency();
		w.shake.duration = Duration();
		w.shake.radius = Radius();
	}

	w.sprite1.sprite = pev->model ? STRING(pev->model) : WARPBALL_SPRITE;
	w.sprite1.framerate = SpriteFramerate();

	int red = pev->rendercolor.x;
	int green = pev->rendercolor.y;
	int blue = pev->rendercolor.z;
	if (!red && !green && !blue) {
		red = WARPBALL_RED_DEFAULT;
		green = WARPBALL_GREEN_DEFAULT;
		blue = WARPBALL_BLUE_DEFAULT;
	}
	w.sprite1.color = Color3(red, green, blue);
	w.sprite1.rendermode = RenderMode();
	w.sprite1.alpha = RenderAmount();
	w.sprite1.renderfx = RenderFx();
	w.sprite1.scale = Scale();

	if (!FStringNull(model2)) {
		w.sprite2.sprite = STRING(model2);
		w.sprite2.framerate = framerate2 > 0 ? framerate2 : SpriteFramerate();
		int red2, green2, blue2;
		if (rendercolor2 == g_vecZero)
		{
			red2 = red;
			green2 = green;
			blue2 = blue;
		}
		else
		{
			red2 = rendercolor2.x;
			green2 = rendercolor2.y;
			blue2 = rendercolor2.z;
		}
		w.sprite2.color = Color3(red2, green2, blue2);
		w.sprite2.rendermode = RenderMode();
		w.sprite2.alpha = renderamt2 > 0 ? renderamt2 : RenderAmount();
		w.sprite2.renderfx = RenderFx();
		w.sprite2.scale = scale2 > 0 ? scale2 : Scale();
	}

	if (pev->spawnflags & SF_WARPBALL_DYNLIGHT)
	{
		w.light.radius = 200 * Scale();
		w.light.color = Color3(red, green, blue);
		w.light.life = 1.5f;
	}

	int beamRed = pev->punchangle.x;
	int beamGreen = pev->punchangle.y;
	int beamBlue = pev->punchangle.z;

	if (!beamRed && !beamGreen && !beamBlue) {
		beamRed = WARPBALL_BEAM_RED_DEFAULT;
		beamGreen = WARPBALL_BEAM_GREEN_DEFAULT;
		beamBlue = WARPBALL_BEAM_BLUE_DEFAULT;
	}

	w.beamCount = IntRange(MaxBeamCount()/2, MaxBeamCount());
	w.beamRadius = Radius();

	w.beam.texture = m_beamTexture;
	w.beam.width = 30;
	w.beam.noise = 65;
	w.beam.color = Color3(beamRed, beamGreen, beamBlue);
	w.beam.alpha = 220;

	if (m_aiSound)
	{
		w.aiSound.type = m_aiSound;
		w.aiSound.radius = Radius();
		w.aiSound.duration = m_aiSoundDuration > 0.0f ? m_aiSoundDuration : 0.3f;
	}
	PlayWarpballEffect(this, w, vecOrigin, playSoundEnt);

	SUB_UseTargets( this );

	if( pev->spawnflags & SF_KILL_CENTER )
	{
		const float damageDelay = DamageDelay();
		if (damageDelay == 0)
		{
			::RadiusDamage(vecOrigin, pev, pev, DamageInfo{WARPBALL_DAMAGE, DMG_SHOCK}, inflictedRadius, CLASS_NONE);
		}
		else
		{
			CWarpballHurt::SelfCreate(vecOrigin, WARPBALL_DAMAGE, inflictedRadius, damageDelay, edict());
		}
	}

	if( pev->spawnflags & SF_REMOVE_ON_FIRE )
		UTIL_Remove( this );
}

class CEnvWarpballTemplated : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override { Precache(); }
	void KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;

	inline string_t WarpTarget() {
		return pev->message;
	}
	inline void SetWarpTarget(string_t warpTarget) {
		pev->message = warpTarget;
	}
	inline string_t WarpballName() {
		return pev->netname;
	}
	inline void SetWarpballName(string_t warpballTemplate) {
		pev->netname = warpballTemplate;
	}
	inline float DamageDelay() {
		return pev->frags;
	}
	inline void SetDamageDelay(float delay) {
		pev->frags = delay;
	}

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_simulatedEntityClassname;
};

LINK_ENTITY_TO_CLASS( env_warpball_templated, CEnvWarpballTemplated )

TYPEDESCRIPTION	CEnvWarpballTemplated::m_SaveData[] =
{
	DEFINE_FIELD(CEnvWarpballTemplated, m_simulatedEntityClassname, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE( CEnvWarpballTemplated, CBaseEntity )

void CEnvWarpballTemplated::Precache()
{
	if (!FStringNull(WarpballName()))
	{
		const char* entityClassname = FStringNull(m_simulatedEntityClassname) ? nullptr : STRING(m_simulatedEntityClassname);
		g_WarpballCatalog.PrecacheWarpballTemplate(STRING(WarpballName()), entityClassname);
	}

	UTIL_PrecacheOther("warpball_hurt");
}

void CEnvWarpballTemplated::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "warpball_template"))
	{
		SetWarpballName(ALLOC_STRING(pkvd->szValue));
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "warp_target"))
	{
		SetWarpTarget(ALLOC_STRING(pkvd->szValue));
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "damage_delay"))
	{
		SetDamageDelay(atof(pkvd->szValue));
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "simulated_classname"))
	{
		m_simulatedEntityClassname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvWarpballTemplated::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Vector vecOrigin = pev->origin;
	edict_t* playSoundEnt = edict();
	string_t warpTarget = WarpTarget();
	string_t warpballName = WarpballName();

	if (FStringNull(warpballName))
	{
		ALERT(at_error, "Triggering '%s' without a warpball template\n", STRING(pev->classname));
		return;
	}

	if (!FStringNull(warpTarget))
	{
		if (!TryCalcLocus_Position(this, pActivator, STRING(warpTarget), vecOrigin))
			return;
	}

	const char* entityClassname = FStringNull(m_simulatedEntityClassname) ? nullptr : STRING(m_simulatedEntityClassname);
	const WarpballTemplate* warpballTemplate = g_WarpballCatalog.FindWarpballTemplate(STRING(warpballName), entityClassname);
	if (warpballTemplate)
	{
		PlayWarpballEffect(this, *warpballTemplate, vecOrigin, playSoundEnt);
	}
	else
	{
		ALERT(at_warning, "%s: couldn't find warpball template '%s'\n", STRING(pev->classname), STRING(warpballName));
	}

	if (FBitSet(pev->spawnflags, SF_KILL_CENTER))
	{
		int inflictedRadius = WARPBALL_DAMAGE_RADIUS;
		const float damageDelay = DamageDelay();
		if (damageDelay == 0)
		{
			::RadiusDamage(vecOrigin, pev, pev, DamageInfo{WARPBALL_DAMAGE, DMG_SHOCK}, inflictedRadius, CLASS_NONE);
		}
		else
		{
			CWarpballHurt::SelfCreate(vecOrigin, WARPBALL_DAMAGE, inflictedRadius, damageDelay, edict());
		}
	}

	if (FBitSet(pev->spawnflags, SF_REMOVE_ON_FIRE))
		UTIL_Remove(this);
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
#define XENMAKER_SOUND1 "debris/beamstart7.wav"
#define XENMAKER_SOUND2 "debris/beamstart2.wav"

class CEnvXenMaker : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override { Precache(); }

	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void PlaySecondSound( edict_t* posEnt );

	void EXPORT TrySpawn();
	void EXPORT PlaySecondSoundThink();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
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
		UTIL_PrecacheMonster(STRING(m_iszMonsterClassname), false, &m_defaultMinHullSize, &m_defaultMaxHullSize);
	}
	if (g_modFeatures.alien_teleport_sound)
	{
		PRECACHE_SOUND(ALIEN_TELEPORT_SOUND);
	}
	else
	{
		PRECACHE_SOUND(XENMAKER_SOUND1);
		PRECACHE_SOUND(XENMAKER_SOUND2);
	}
	PRECACHE_MODEL(XENMAKER_SPRITE1);
	PRECACHE_MODEL(XENMAKER_SPRITE2);
}

void CEnvXenMaker::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "monstertype"))
	{
		m_iszMonsterClassname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flBeamRadius"))
	{
		m_flBeamRadius = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iBeamAlpha"))
	{
		m_iBeamAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iBeamCount"))
	{
		m_iBeamCount = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vBeamColor"))
	{
		UTIL_StringToVector(m_vBeamColor, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flLightRadius"))
	{
		m_flLightRadius = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vLightColor"))
	{
		UTIL_StringToVector(m_vLightColor, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flStartSpriteFramerate"))
	{
		m_flStartSpriteFramerate = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flStartSpriteScale"))
	{
		m_flStartSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartSpriteAlpha"))
	{
		m_iStartSpriteAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vStartSpriteColor"))
	{
		UTIL_StringToVector(m_vStartSpriteColor, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flEndSpriteFramerate"))
	{
		m_flEndSpriteFramerate = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flEndSpriteScale"))
	{
		m_flEndSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iEndSpriteAlpha"))
	{
		m_iEndSpriteAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vEndSpriteColor"))
	{
		UTIL_StringToVector(m_vEndSpriteColor, pkvd->szValue);
		pkvd->fHandled = true;
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

	if (!FBitSet(pev->spawnflags, SF_XENMAKER_NOSPAWN) && !FStringNull(m_iszMonsterClassname)
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

		DispatchSpawnAutoClean(OwnInstance(pent));
	}

	WarpballTemplate w;

	w.sprite1.sprite = XENMAKER_SPRITE1;
	w.sprite1.color = Color3(m_vStartSpriteColor.x, m_vStartSpriteColor.y, m_vStartSpriteColor.z);
	w.sprite1.alpha = m_iStartSpriteAlpha;
	w.sprite1.scale = m_flStartSpriteScale;
	w.sprite1.framerate = m_flStartSpriteFramerate;

	w.sprite2.sprite = XENMAKER_SPRITE2;
	w.sprite2.color = Color3(m_vEndSpriteColor.x, m_vEndSpriteColor.y, m_vEndSpriteColor.z);
	w.sprite2.alpha = m_iEndSpriteAlpha;
	w.sprite2.scale = m_flEndSpriteScale;
	w.sprite2.framerate = m_flEndSpriteFramerate;

	w.beam.texture = m_beamTexture;
	w.beam.width = 25;
	w.beam.noise = 50;
	w.beam.color = Color3(m_vBeamColor.x, m_vBeamColor.y, m_vBeamColor.z);
	w.beam.alpha = m_iBeamAlpha;

	w.beamCount = IntRange(m_iBeamCount);

	w.light.color = Color3(m_vLightColor.x, m_vLightColor.y, m_vLightColor.z);
	w.light.radius = m_flLightRadius;

	if (g_modFeatures.alien_teleport_sound)
		w.sound1.sound = ALIEN_TELEPORT_SOUND;
	else
		w.sound1.sound = XENMAKER_SOUND1;

	if (!g_modFeatures.alien_teleport_sound)
	{
		if (asTemplate)
		{
			w.sound2.sound = XENMAKER_SOUND2;
		}
		else
		{
			SetThink(&CEnvXenMaker::PlaySecondSoundThink);
			pev->nextthink = gpGlobals->time + 0.8;
		}
	}

	PlayWarpballEffect(this, w, vecOrigin, posEnt);
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
	BLOWERCANNON_NOPROJECTILE = -1,
	BLOWERCANNON_SQUIDSPIT = 0,
	BLOWERCANNON_SPOREROCKET = 1,
	BLOWERCANNON_SPOREGRENADE,
	BLOWERCANNON_SHOCKBEAM,
	BLOWERCANNON_DISPLACERBALL,
	BLOWERCANNON_SQUIDTOXICSPIT
};

enum
{
	BLOWERCANNON_TOGGLE = 1,
	BLOWERCANNON_FIRE,
};

#define SF_BLOWERCANNON_NO_FIRST_DELAY 32

class CBlowerCannon : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue(KeyValueData* pkvd) override;
	const char* ProjectileName();
	void EXPORT BlowerCannonThink();
	void EXPORT BlowerCannonStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT BlowerCannonStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	short m_iWeapType;
	short m_iFireType;
	int m_iZOffset;
	string_t m_iszOwner;
	string_t m_projectileName;
	float m_detonationDelay;
};

LINK_ENTITY_TO_CLASS(env_blowercannon, CBlowerCannon)

TYPEDESCRIPTION	CBlowerCannon::m_SaveData[] =
{
	DEFINE_FIELD(CBlowerCannon, m_iFireType, FIELD_SHORT),
	DEFINE_FIELD(CBlowerCannon, m_iWeapType, FIELD_SHORT),
	DEFINE_FIELD(CBlowerCannon, m_iZOffset, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_iszOwner, FIELD_STRING),
	DEFINE_FIELD(CBlowerCannon, m_projectileName, FIELD_STRING),
	DEFINE_FIELD(CBlowerCannon, m_detonationDelay, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE( CBlowerCannon, CBaseDelay )


void CBlowerCannon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "firetype"))
	{
		m_iFireType = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "weaptype"))
	{
		m_iWeapType = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "zoffset"))
	{
		m_iZOffset = (int)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "position"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "direction"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "projectile_owner"))
	{
		m_iszOwner = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "projectile_name"))
	{
		m_projectileName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "detonation_delay"))
	{
		m_detonationDelay = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CBlowerCannon::Spawn()
{
	Precache();
	if (m_flDelay <= 0.0f && m_iFireType != BLOWERCANNON_FIRE)
		m_flDelay = 1.0f;
	SetUse( &CBlowerCannon::BlowerCannonStart );
}

void CBlowerCannon::Precache()
{
	const char* projectileName = ProjectileName();
	if (projectileName)
	{
		int variant = 0;
		projectileName = GetRealProjectileClassname(projectileName, variant);

		EntityOverrides entityOverrides;
		entityOverrides.entTemplate = m_entTemplate;
		UTIL_PrecacheOther(projectileName, entityOverrides);
	}
}

const char* CBlowerCannon::ProjectileName()
{
	if (!FStringNull(m_projectileName))
		return STRING(m_projectileName);

	switch (m_iWeapType) {
	case BLOWERCANNON_SQUIDSPIT:
		return "squidspit";
	case BLOWERCANNON_SHOCKBEAM:
		return "shock_beam";
	case BLOWERCANNON_DISPLACERBALL:
		return "displacer_ball";
	case BLOWERCANNON_SPOREGRENADE:
		return "spore bouncy";
	case BLOWERCANNON_SPOREROCKET:
		return "spore";
	case BLOWERCANNON_SQUIDTOXICSPIT:
		return "squidtoxicspit";
	default:
		return nullptr;
	}
}

void CBlowerCannon::BlowerCannonStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	SetUse( &CBlowerCannon::BlowerCannonStop );
	SetThink( &CBlowerCannon::BlowerCannonThink );
	const float delay = FBitSet(pev->spawnflags, SF_BLOWERCANNON_NO_FIRST_DELAY) ? 0.0f : m_flDelay;
	pev->nextthink = gpGlobals->time + delay;
}

void CBlowerCannon::BlowerCannonStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStart );
	SetThink( NULL );
}

void CBlowerCannon::BlowerCannonThink()
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
				direction.NormalizeInPlace();
				angles = UTIL_VecToAngles( direction );
				angles.z = -angles.z;
			}
		}
		else
		{
			if (!FStringNull(pev->target))
			{
				CBaseEntity *pTarget = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target), m_hActivator);
				if (pTarget)
				{
					direction = pTarget->pev->origin - position;
					direction.z += m_iZOffset;
					direction.NormalizeInPlace();
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

			const char* projectileName = ProjectileName();
			if (projectileName)
			{
				int variant = 0;
				projectileName = GetRealProjectileClassname(projectileName, variant);

				EntityOverrides entityOverrides;
				entityOverrides.entTemplate = m_entTemplate;

				ProjectileParameters params(projectileName, position, angles, direction, owner, entityOverrides);
				params.variant = variant;
				if (m_detonationDelay > 0)
					params.time = m_detonationDelay;
				if (pev->speed > 0)
					params.speedOverride = pev->speed;
				CBaseEntity::CreateAndLaunchAsProjectile(params);
			}
			else
			{
				ALERT(at_console, "Unknown projectile type in blowercannon: %d\n", m_iWeapType);
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
	void	Precache() override;
	void	Spawn() override { Precache(); }
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void	KeyValue( KeyValueData *pkvd ) override;
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

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

void CEnvShockwave::Precache()
{
	m_iSpriteTexture = PRECACHE_MODEL( STRING(pev->netname) );
}

void CEnvShockwave::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iTime"))
	{
		m_iTime = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iRadius"))
	{
		m_iRadius = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iHeight"))
	{
		m_iHeight = atoi(pkvd->szValue)/2; //LRC- the actual height is doubled when drawn
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iScrollRate"))
	{
		m_iScrollRate = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iNoise"))
	{
		m_iNoise = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFrameRate"))
	{
		m_iFrameRate = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartFrame"))
	{
		m_iStartFrame = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_cType"))
	{
		m_cType = atoi(pkvd->szValue);
		pkvd->fHandled = true;
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
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecPos );
		WRITE_BYTE( type );
		WRITE_CIRCLE( vecPos, m_iRadius );
		WRITE_SHORT( m_iSpriteTexture ); // short (sprite index)
		WRITE_BYTE( m_iStartFrame ); // byte (starting frame)
		WRITE_BYTE( m_iFrameRate ); // byte (frame rate in 0.1's)
		WRITE_BYTE( m_iTime ); // byte (life in 0.1's)
		WRITE_BYTE( m_iHeight );  // byte (line width in 0.1's)
		WRITE_BYTE( m_iNoise );   // byte (noise amplitude in 0.01's)
		WRITE_COLOR( pev->rendercolor );   // byte,byte,byte (color)
		WRITE_BYTE( pev->renderamt );  // byte (brightness)
		WRITE_BYTE( m_iScrollRate );	// byte (scroll speed in 0.1's)
	MESSAGE_END();

	if (!(pev->spawnflags & SF_SHOCKWAVE_REPEATABLE))
	{
		SetThink(&CEnvShockwave:: SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

//==================================================================
//LRC- env_dlight; Dynamic Entity Light creator
//==================================================================
#define SF_DLIGHT_ONLYONCE 1
#define SF_DLIGHT_STARTON  2
#define SF_DLIGHT_TRACK_POSITION_UPDATE 4
#define SF_DLIGHT_POS_IS_LOCUS_ENTITY 8
#define SF_DLIGHT_POS_VALID ( 1 << 24 )
class CEnvDLight : public CPointEntity
{
public:
	void	Activate() override;
	void SendMessages(CBaseEntity* pClient) override;
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void EXPORT TurnOffThink();
	void EXPORT TrackPositionThink();
	void CalcMyPosition(CBaseEntity *pActivator);
	void TurnOn(CBaseEntity* pClient = 0);
	void TurnOff();
	void SetupThink();
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
	bool IsActive() {
		return pev->spawnflags & SF_DLIGHT_STARTON;
	}

	Vector m_vecPos;
	bool m_activated;
	EHANDLE m_hActivator;
	EHANDLE m_posEntity;

private:
	static bool EntityPositionIsKnownOnClient(CBaseEntity* pEntity);
};

LINK_ENTITY_TO_CLASS( env_dlight, CEnvDLight )

TYPEDESCRIPTION	CEnvDLight::m_SaveData[] =
{
	DEFINE_FIELD( CEnvDLight, m_vecPos, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvDLight, m_activated, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEnvDLight, m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( CEnvDLight, m_posEntity, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CEnvDLight, CPointEntity );

void CEnvDLight::Activate()
{
	if (!m_activated)
	{
		// each env_dlight uses its own key to reference the light on the client
		m_activated = true;

		if (FStringNull(pev->targetname) || IsActive())
		{
			CalcMyPosition(this);
			SetupThink();
		}
	}
}

void CEnvDLight::SendMessages(CBaseEntity *pClient)
{
	if (FStringNull(pev->targetname) || IsActive())
	{
		TurnOn(pClient);
	}
}

void CEnvDLight::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!ShouldToggle(useType, IsActive()))
	{
		return;
	}
	if (IsActive())
	{
		TurnOff();
		pev->nextthink = -1.0f;
		return;
	}

	m_hActivator = pActivator;
	CalcMyPosition(pActivator);
	TurnOn();
	SetupThink();
}

void CEnvDLight::CalcMyPosition(CBaseEntity* pActivator)
{
	if (!FStringNull(pev->message))
	{
		if(FBitSet(pev->spawnflags, SF_DLIGHT_POS_IS_LOCUS_ENTITY))
		{
			m_posEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message), pActivator);
			if (m_posEntity != 0)
			{
				m_vecPos = m_posEntity->pev->origin;
			}
			else
				return;
		}
		else
		{
			if (!TryCalcLocus_Position( this, pActivator, STRING(pev->message), m_vecPos ))
				return;
		}
	}
	else
	{
		m_vecPos = pev->origin;
	}
	pev->spawnflags |= SF_DLIGHT_POS_VALID;
}

extern int gmsgKeyedDLight;

void CEnvDLight::TurnOn(CBaseEntity *pClient)
{
	if (!FBitSet(pev->spawnflags, SF_DLIGHT_POS_VALID))
		return;

	const int msgType = pClient ? MSG_ONE : MSG_ALL;
	edict_t* pClientEdict = pClient ? pClient->edict() : NULL;
	int entindex = 0;
	if (m_posEntity != 0 && EntityPositionIsKnownOnClient(m_posEntity))
	{
		entindex = m_posEntity->entindex();
	}

	SetBits(pev->spawnflags, SF_DLIGHT_STARTON);
	MESSAGE_BEGIN( msgType, gmsgKeyedDLight, NULL, pClientEdict );
		WRITE_SHORT( this->entindex() );
		WRITE_BYTE( 1 );
		WRITE_VECTOR( m_vecPos );
		WRITE_SHORT( pev->renderamt );		// radius
		WRITE_COLOR( pev->rendercolor );
		WRITE_SHORT( entindex );
	MESSAGE_END();
}

void CEnvDLight::TurnOff()
{
	if (!FBitSet(pev->spawnflags, SF_DLIGHT_POS_VALID))
		return;

	ClearBits(pev->spawnflags, SF_DLIGHT_STARTON);
	MESSAGE_BEGIN( MSG_ALL, gmsgKeyedDLight );
		WRITE_SHORT( entindex() );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	m_posEntity = 0;
}

void CEnvDLight::TurnOffThink()
{
	TurnOff();

	if (pev->spawnflags & SF_DLIGHT_ONLYONCE)
	{
		SetThink( &CEnvDLight::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

void CEnvDLight::TrackPositionThink()
{
	if (!FStringNull(pev->message))
	{
		bool shouldSendPosition = true;
		if (FBitSet(pev->spawnflags, SF_DLIGHT_POS_IS_LOCUS_ENTITY))
		{
			CBaseEntity* pPosEntity = m_posEntity;
			if (pPosEntity != 0)
			{
				m_vecPos = pPosEntity->pev->origin;
				if (EntityPositionIsKnownOnClient(pPosEntity))
				{
					// For visible entities the position is updated automatically on the client
					pPosEntity->m_EFlags |= EFLAG_ALWAYS_SEND;
					shouldSendPosition = false;
				}
			}
		}
		else
		{
			CalcMyPosition(m_hActivator);
		}
		if (shouldSendPosition)
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgKeyedDLight );
				WRITE_SHORT( entindex() );
				WRITE_BYTE( 2 );
				WRITE_VECTOR( m_vecPos );
			MESSAGE_END();
		}
	}
	if (pev->health > 0.0f && pev->dmgtime + pev->health <= gpGlobals->time)
	{
		TurnOffThink();
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CEnvDLight::SetupThink()
{
	const bool shouldTrackPosition = !FStringNull(pev->message) && FBitSet(pev->spawnflags, SF_DLIGHT_TRACK_POSITION_UPDATE);
	if (pev->health > 0)
	{
		if (shouldTrackPosition)
		{
			SetThink(&CEnvDLight::TrackPositionThink);
			pev->dmgtime = gpGlobals->time;
			pev->nextthink = gpGlobals->time + 0.1f;
		}
		else
		{
			SetThink(&CEnvDLight::TurnOffThink);
			pev->nextthink = gpGlobals->time + pev->health;
		}
	}
	else
	{
		if (shouldTrackPosition)
		{
			SetThink(&CEnvDLight::TrackPositionThink);
			pev->nextthink = gpGlobals->time + 0.1f;
		}
	}
}

bool CEnvDLight::EntityPositionIsKnownOnClient(CBaseEntity *pEntity)
{
	return pEntity->pev->modelindex && !FBitSet(pEntity->pev->effects, EF_NODRAW);
}

#define SF_ENVSTREAK_REMOVE_ON_FIRE 1
#define SF_ENVSTREAK_DIRECTIONAL 2

class CEnvStreak : public CPointEntity
{
	void KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
};

LINK_ENTITY_TO_CLASS( env_streak, CEnvStreak )

void CEnvStreak::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "minlife"))
	{
		pev->health = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "maxlife"))
	{
		pev->max_health = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "tracer_color"))
	{
		pev->skin = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		pev->body = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "randomness"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "pt_type"))
	{
		pev->impulse = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CEnvStreak::Spawn()
{
	CPointEntity::Spawn();
	SetMovedir( pev );
	if (pev->health == 0.0f)
	{
		pev->health = 0.1f;
		if (pev->max_health == 0.0f)
			pev->max_health = 0.5f;
	}
}

void CEnvStreak::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	extern int gmsgStreaks;

	Vector origin = pev->origin;
	if (pev->message)
	{
		if (!TryCalcLocus_Position( this, pActivator, STRING(pev->message), origin )) {
			return;
		}
	}

	bool isDirectional = FBitSet(pev->spawnflags, SF_ENVSTREAK_DIRECTIONAL);
	Vector direction = pev->movedir;

	if (pev->netname)
	{
		if (TryCalcLocus_Velocity(this, pActivator, STRING(pev->netname), direction))
			isDirectional = true;
		else
			return;
	}

	MESSAGE_BEGIN( MSG_PVS, gmsgStreaks, origin );
		WRITE_BYTE( isDirectional ? 1 : 0 );
		WRITE_VECTOR( origin );		// origin
		if (isDirectional)
			WRITE_VECTOR( direction );	// direction
		WRITE_BYTE( pev->skin ); // color
		WRITE_SHORT( pev->body );	// count
		WRITE_SHORT( pev->speed );
		WRITE_SHORT( pev->dmg );	// Random velocity modifier
		WRITE_BYTE( pev->health * 10 );
		WRITE_BYTE( pev->max_health * 10 );
		WRITE_BYTE( pev->impulse > 0 ? pev->impulse : pt_grav );
		WRITE_BYTE( pev->scale > 0 ? pev->scale * 10 : 10 );
	MESSAGE_END();

	if (FBitSet(pev->spawnflags, SF_ENVSTREAK_REMOVE_ON_FIRE))
		UTIL_Remove(this);
}


//=========================================================
// LRC - Decal effect
//=========================================================
class CEnvDecal : public CPointEntity
{
public:
	void	Spawn() override;
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
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
		case 9:
		{
			if (gDecals[DECAL_SPR_SPLT1].index >= 0)
				iTexture = DECAL_SPR_SPLT1 + RANDOM_LONG(0,2);
			else
				iTexture = DECAL_YBLOOD5 + RANDOM_LONG(0,1);
		}
		break;
		case 10:
		{
			if (gDecals[DECAL_OPFOR_SCORCH1].index >= 0)
				iTexture = DECAL_OPFOR_SCORCH1 + RANDOM_LONG(0,2);
			else
				iTexture = DECAL_SCORCH1 + RANDOM_LONG(0,1);
		}
		break;
		case 11:
		{
			if (gDecals[DECAL_OPFOR_SMALLSCORCH1].index >= 0)
				iTexture = DECAL_OPFOR_SMALLSCORCH1 + RANDOM_LONG(0,2);
			else
				iTexture = DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2);
		}
		break;
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
		WRITE_VECTOR( trace.vecEndPos );
		WRITE_SHORT( iTexture );
		WRITE_SHORT( entityIndex );
		if ( entityIndex )
			WRITE_SHORT( (int)VARS(trace.pHit)->modelindex );
	MESSAGE_END();
}

void CEnvDecal::Spawn()
{
	if (pev->impulse == 0)
	{
		pev->skin = DECAL_INDEX( STRING(pev->noise) );
		pev->noise = iStringNull;

		if ( pev->skin == 0 )
			ALERT( at_console, "%s \"%s\" can't find decal \"%s\"\n", STRING(pev->classname), STRING(pev->noise) );
	}
}

//=========================================================
// LRC - env_fog, extended a bit from the DMC version
//=========================================================
#define SF_FOG_ACTIVE 1
#define SF_FOG_AFFECT_SKYBOX 2
#define SF_FOG_FADING 0x8000

class CEnvFog : public CPointEntity
{
public:
	void Spawn() override;
	void EXPORT ResumeThink();
	void EXPORT TurnOn();
	void EXPORT TurnOff();
	void EXPORT FadeInDone();
	void EXPORT FadeOutDone();
	void SendMessages(CBaseEntity *pClient) override;
	void SendData() { SendData(pev->rendercolor, 0, m_iStartDist, m_iEndDist, m_density); }
	void SendDataDeactivate() { SendData(g_vecZero, 0, 0, 0, 0); }
	void SendData(Vector col, int iFadeTime, int iStartDist, int iEndDist, float density);
	void SendDataToOne(CBaseEntity *pClient, Vector col, int iFadeTime, int iStartDist, int iEndDist, float density);
	void KeyValue( KeyValueData *pkvd ) override;
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	int m_iStartDist;
	int m_iEndDist;
	float m_iFadeIn;
	float m_iFadeOut;
	float m_fHoldTime;
	float m_fFadeStart; // if we're fading in/out, then when did the fade start?

	float m_density;
	short m_fogType;
};

TYPEDESCRIPTION	CEnvFog::m_SaveData[] =
{
	DEFINE_FIELD( CEnvFog, m_iStartDist, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_iEndDist, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_iFadeIn, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_iFadeOut, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_fHoldTime, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_fFadeStart, FIELD_TIME ),
	DEFINE_FIELD( CEnvFog, m_density, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_fogType, FIELD_SHORT ),
};

IMPLEMENT_SAVERESTORE( CEnvFog, CBaseEntity )

void CEnvFog :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "startdist"))
	{
		m_iStartDist = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "enddist"))
	{
		m_iEndDist = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_iFadeIn = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_iFadeOut = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		m_fHoldTime = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_density = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "fogtype"))
	{
		m_fogType = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvFog::Spawn ()
{
	pev->effects |= EF_NODRAW;

	if (pev->targetname == 0)
		pev->spawnflags |= SF_FOG_ACTIVE;

	if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		SetThink(&CEnvFog :: TurnOn );
	}

	// things get messed up if we try to draw fog with a startdist
	// or an enddist of 0, so we don't allow it.
	if (m_iStartDist == 0) m_iStartDist = 1;
	if (m_iEndDist == 0) m_iEndDist = 1;
}

extern int gmsgSetFog;

void CEnvFog :: TurnOn ()
{
//	ALERT(at_console, "Fog turnon %f\n", gpGlobals->time);

	pev->spawnflags |= SF_FOG_ACTIVE;

	if( m_iFadeIn )
	{
		pev->spawnflags |= SF_FOG_FADING;
		SendData( pev->rendercolor, m_iFadeIn, m_iStartDist, m_iEndDist, m_density);
		pev->nextthink = gpGlobals->time + m_iFadeIn;
		SetThink(&CEnvFog :: FadeInDone );
	}
	else
	{
		pev->spawnflags &= ~SF_FOG_FADING;
		SendData();
		if (m_fHoldTime)
		{
			pev->nextthink = gpGlobals->time + m_fHoldTime;
			SetThink(&CEnvFog :: TurnOff );
		}
	}
}

void CEnvFog :: TurnOff ()
{
//	ALERT(at_console, "Fog turnoff\n");

	pev->spawnflags &= ~SF_FOG_ACTIVE;

	if( m_iFadeOut )
	{
		pev->spawnflags |= SF_FOG_FADING;
		SendData( pev->rendercolor, -m_iFadeOut, m_iStartDist, m_iEndDist, m_density);
		pev->nextthink = gpGlobals->time  + m_iFadeOut;
		SetThink(&CEnvFog :: FadeOutDone );
	}
	else
	{
		pev->spawnflags &= ~SF_FOG_FADING;
		SendDataDeactivate();
		//DontThink();
	}
}

//yes, this intermediate think function is necessary.
// the engine seems to ignore the nextthink time when starting up.
// So this function gets called immediately after the precache finishes,
// regardless of what nextthink time is specified.
void CEnvFog :: ResumeThink ()
{
//	ALERT(at_console, "Fog resume %f\n", gpGlobals->time);
	SetThink(&CEnvFog ::FadeInDone);
	pev->nextthink = gpGlobals->time;
}

void CEnvFog :: FadeInDone ()
{
	pev->spawnflags &= ~SF_FOG_FADING;
	SendData();

	if (m_fHoldTime)
	{
		pev->nextthink = gpGlobals->time + m_fHoldTime;
		SetThink(&CEnvFog :: TurnOff );
	}
}

void CEnvFog :: FadeOutDone ()
{
	pev->spawnflags &= ~SF_FOG_FADING;
	SendDataDeactivate();
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

void CEnvFog::SendMessages(CBaseEntity *pClient)
{
	if (FBitSet(pev->spawnflags, SF_FOG_ACTIVE) && pClient)
	{
		SendDataToOne(pClient, pev->rendercolor, 0, m_iStartDist, m_iEndDist, m_density);
	}
}

void CEnvFog::SendData ( Vector col, int iFadeTime, int iStartDist, int iEndDist, float density )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			SendDataToOne(pPlayer, col, iFadeTime, iStartDist, iEndDist, density);
		}
	}
}

void CEnvFog::SendDataToOne(CBaseEntity *pClient, Vector col, int iFadeTime, int iStartDist, int iEndDist, float density)
{
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFog, NULL, pClient->pev );
		WRITE_BYTE ( col.x );
		WRITE_BYTE ( col.y );
		WRITE_BYTE ( col.z );
		WRITE_SHORT ( iFadeTime );
		WRITE_SHORT ( iStartDist );
		WRITE_SHORT ( iEndDist );
		WRITE_LONG ( density * 10000 );
		WRITE_BYTE ( m_fogType );
		WRITE_BYTE ( FBitSet(pev->spawnflags, SF_FOG_AFFECT_SKYBOX) );
	MESSAGE_END();
}

LINK_ENTITY_TO_CLASS( env_fog, CEnvFog )

extern int gmsgRain;
extern int gmsgWeatherPos;

class CBaseWeather : public CBaseEntity
{
public:
	void Spawn() override;
	void Activate() override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void SendMessages(CBaseEntity* pClient) override;

	bool IsActive() const {
		if (FBitSet(pev->spawnflags, SF_WEATHER_ACTIVE))
			return true;
		return false;
	}
	void SetActive(bool active) {
		if (active)
			SetBits(pev->spawnflags, SF_WEATHER_ACTIVE);
		else
			ClearBits(pev->spawnflags, SF_WEATHER_ACTIVE);
	}

	virtual void SendWeather(CBaseEntity* pClient = nullptr) = 0;
	virtual void SendClearWeather() = 0;

	optional<Vector> GetDynamicPosition();
	void StartWeatherPosThink();
	void EXPORT WeatherPosThink();
	float WeatherPosUpdateDelay() const;

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

protected:
	void WriteBaseWeaterData(int flags, const optional<Vector>& dynamicPos);

	short m_intensity;
	float m_updatePeriod;
	short m_distance;

	short m_minHeight;
	short m_maxHeight;

	string_t m_position;
	Vector m_lastPos;
};

TYPEDESCRIPTION CBaseWeather::m_SaveData[] =
{
	DEFINE_FIELD( CBaseWeather, m_intensity, FIELD_SHORT ),
	DEFINE_FIELD( CBaseWeather, m_updatePeriod, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseWeather, m_distance, FIELD_SHORT ),
	DEFINE_FIELD( CBaseWeather, m_minHeight, FIELD_SHORT ),
	DEFINE_FIELD( CBaseWeather, m_maxHeight, FIELD_SHORT ),
	DEFINE_FIELD( CBaseWeather, m_position, FIELD_STRING ),
	DEFINE_FIELD( CBaseWeather, m_lastPos, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CBaseWeather, CBaseEntity )

void CBaseWeather::Spawn()
{
	pev->effects |= EF_NODRAW;
	if (FStringNull(pev->targetname))
		pev->spawnflags |= SF_WEATHER_ACTIVE;
}

void CBaseWeather::Activate()
{
	auto dynPos = GetDynamicPosition();
	if (dynPos.has_value())
		m_lastPos = *dynPos;
	else
		m_lastPos = pev->origin;

	StartWeatherPosThink();
}

void CBaseWeather::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "intensity"))
	{
		m_intensity = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "update_period"))
	{
		m_updatePeriod = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "distance"))
	{
		m_distance = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "min_height"))
	{
		m_minHeight = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "max_height"))
	{
		m_maxHeight = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "position"))
	{
		m_position = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CBaseWeather::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	bool active = IsActive();
	if (ShouldToggle(useType, active))
	{
		if (active)
		{
			SetActive(false);
			SendClearWeather();
			SetThink(nullptr);
		}
		else
		{
			SetActive(true);
			SendWeather();
			StartWeatherPosThink();
		}
	}
}

void CBaseWeather::SendMessages(CBaseEntity *pClient)
{
	if (!IsActive())
		return;
	SendWeather(pClient);
}

void CBaseWeather::WriteBaseWeaterData(int flags, const optional<Vector>& dynamicPos)
{
	WRITE_LONG(entindex());
	WRITE_SHORT(flags);
	WRITE_SHORT(m_intensity);
	WRITE_SHORT((short)(m_updatePeriod * 100.0f));

	if (FBitSet(flags, WEATHER_BRUSH_ENTITY))
	{
		if (dynamicPos.has_value())
		{
			WRITE_VECTOR(*dynamicPos + pev->mins);
			WRITE_VECTOR(*dynamicPos + pev->maxs);
		}
		else
		{
			WRITE_VECTOR(pev->absmin);
			WRITE_VECTOR(pev->absmax);
		}
	}
	else
	{
		WRITE_SHORT(m_distance);
		WRITE_SHORT(m_minHeight);
		WRITE_SHORT(m_maxHeight);

		if (FBitSet(flags, SF_WEATHER_LOCALIZED))
		{
			const Vector origin = dynamicPos.value_or(pev->origin);
			WRITE_VECTOR(origin);
		}
	}
}

optional<Vector> CBaseWeather::GetDynamicPosition()
{
	if (FBitSet(pev->spawnflags, SF_WEATHER_LOCALIZED|WEATHER_BRUSH_ENTITY) && !FStringNull(m_position))
	{
		Vector result;
		if (TryCalcLocus_Position(this, this, STRING(m_position), result))
			return optional<Vector>(result);
	}
	return optional<Vector>();
}

void CBaseWeather::StartWeatherPosThink()
{
	if (IsActive() && FBitSet(pev->spawnflags, SF_WEATHER_LOCALIZED|WEATHER_BRUSH_ENTITY))
	{
		SetThink(&CBaseWeather::WeatherPosThink);
		pev->nextthink = gpGlobals->time + WeatherPosUpdateDelay();
	}
}

void CBaseWeather::WeatherPosThink()
{
	auto dynPos = GetDynamicPosition();
	const Vector& currentPos = dynPos.value_or(pev->origin);

	if (currentPos != m_lastPos)
	{
		m_lastPos = currentPos;

		MESSAGE_BEGIN(MSG_ALL, gmsgWeatherPos);
		WRITE_LONG(entindex());

		if (FBitSet(pev->spawnflags, WEATHER_BRUSH_ENTITY))
		{
			WRITE_BYTE(1);

			if (dynPos.has_value())
			{
				WRITE_VECTOR(currentPos + pev->mins);
				WRITE_VECTOR(currentPos + pev->maxs);
			}
			else
			{
				WRITE_VECTOR(pev->absmin);
				WRITE_VECTOR(pev->absmax);
			}
		}
		else
		{
			WRITE_BYTE(0);
			WRITE_VECTOR(currentPos);
		}

		MESSAGE_END();
	}
	pev->nextthink = gpGlobals->time + WeatherPosUpdateDelay();
}

float CBaseWeather::WeatherPosUpdateDelay() const
{
	if (m_updatePeriod <= 0.0f)
		return 0.3f;
	return Q_max(m_updatePeriod, 0.1f);
}

class CEnvRain : public CBaseWeather
{
public:
	void KeyValue(KeyValueData *pkvd) override;
	void SendWeather(CBaseEntity* pClient = NULL) override;
	void SendClearWeather() override;

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

private:
	short m_raindropWidth;
	short m_raindropHeight;

	byte m_raindropLightMode;
	short m_raindropMinSpeed;
	short m_raindropMaxSpeed;
	float m_raindropLife;

	short m_windPuffMinSize;
	short m_windPuffMaxSize;
	short m_windPuffRenderMode;
	short m_windPuffBrightness;
	Vector m_windPuffColor;
	byte m_windPuffLightMode;
	float m_windPuffLife;

	short m_splashMinSize;
	short m_splashMaxSize;
	short m_splashRenderMode;
	short m_splashBrightness;
	Vector m_splashColor;
	byte m_splashLightMode;

	short m_rippleSize;
	short m_rippleRenderMode;
	short m_rippleBrightness;
	Vector m_rippleColor;
	byte m_rippleLightMode;

	string_t m_raindropSprite;
	string_t m_windpuffSprite;
	string_t m_splashSprite;
	string_t m_rippleSprite;
};

LINK_ENTITY_TO_CLASS( env_rain, CEnvRain )

TYPEDESCRIPTION CEnvRain::m_SaveData[] =
{
	DEFINE_FIELD( CEnvRain, m_raindropWidth, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_raindropHeight, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_raindropLightMode, FIELD_CHARACTER ),
	DEFINE_FIELD( CEnvRain, m_raindropMinSpeed, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_raindropMaxSpeed, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_raindropLife, FIELD_FLOAT ),

	DEFINE_FIELD( CEnvRain, m_windPuffMinSize, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_windPuffMaxSize, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_windPuffRenderMode, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_windPuffBrightness, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_windPuffColor, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvRain, m_windPuffLightMode, FIELD_CHARACTER ),
	DEFINE_FIELD( CEnvRain, m_windPuffLife, FIELD_FLOAT ),

	DEFINE_FIELD( CEnvRain, m_splashMinSize, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_splashMaxSize, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_splashRenderMode, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_splashBrightness, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_splashColor, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvRain, m_splashLightMode, FIELD_CHARACTER ),

	DEFINE_FIELD( CEnvRain, m_rippleSize, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_rippleRenderMode, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_rippleBrightness, FIELD_SHORT ),
	DEFINE_FIELD( CEnvRain, m_rippleColor, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvRain, m_rippleLightMode, FIELD_CHARACTER ),

	DEFINE_FIELD( CEnvRain, m_raindropSprite, FIELD_STRING ),
	DEFINE_FIELD( CEnvRain, m_windpuffSprite, FIELD_STRING ),
	DEFINE_FIELD( CEnvRain, m_splashSprite, FIELD_STRING ),
	DEFINE_FIELD( CEnvRain, m_rippleSprite, FIELD_STRING ),
};
IMPLEMENT_SAVERESTORE( CEnvRain, CBaseWeather )

void CEnvRain::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "raindrop_width"))
	{
		m_raindropWidth = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "raindrop_height"))
	{
		m_raindropHeight = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "raindrop_light_mode"))
	{
		m_raindropLightMode = (byte)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "raindrop_min_speed"))
	{
		m_raindropMinSpeed = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "raindrop_max_speed"))
	{
		m_raindropMaxSpeed = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "raindrop_life"))
	{
		m_raindropLife = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_min_size"))
	{
		m_windPuffMinSize = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_max_size"))
	{
		m_windPuffMaxSize = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_rendermode"))
	{
		m_windPuffRenderMode = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_renderamt"))
	{
		m_windPuffBrightness = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_rendercolor"))
	{
		UTIL_StringToVector(m_windPuffColor, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_light_mode"))
	{
		m_windPuffLightMode = (byte)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_life"))
	{
		m_windPuffLife = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "splash_min_size"))
	{
		m_splashMinSize = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "splash_max_size"))
	{
		m_splashMaxSize = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "splash_rendermode"))
	{
		m_splashRenderMode = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "splash_renderamt"))
	{
		m_splashBrightness = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "splash_rendercolor"))
	{
		UTIL_StringToVector(m_splashColor, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "splash_light_mode"))
	{
		m_splashLightMode = (byte)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "ripple_size"))
	{
		m_rippleSize = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "ripple_rendermode"))
	{
		m_rippleRenderMode = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "ripple_renderamt"))
	{
		m_rippleBrightness = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "ripple_rendercolor"))
	{
		UTIL_StringToVector(m_rippleColor, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "ripple_light_mode"))
	{
		m_rippleLightMode = (byte)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "raindrop_sprite"))
	{
		m_raindropSprite = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "windpuff_sprite"))
	{
		m_windpuffSprite = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "splash_sprite"))
	{
		m_splashSprite = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "ripple_sprite"))
	{
		m_rippleSprite = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseWeather::KeyValue(pkvd);
}

void CEnvRain::SendWeather(CBaseEntity *pClient)
{
	const int msgType = pClient ? MSG_ONE : MSG_ALL;
	edict_t* pClientEdict = pClient ? pClient->edict() : NULL;
	auto dynPos = GetDynamicPosition();

	MESSAGE_BEGIN(msgType, gmsgRain, NULL, pClientEdict);
		WriteBaseWeaterData(pev->spawnflags, dynPos);

		WRITE_SHORT(m_raindropWidth);
		WRITE_SHORT(m_raindropHeight);
		WRITE_BYTE(pev->rendermode);
		WRITE_BYTE(pev->renderamt);
		WRITE_COLOR(pev->rendercolor);
		WRITE_BYTE(m_raindropLightMode);
		WRITE_SHORT(m_raindropMinSpeed);
		WRITE_SHORT(m_raindropMaxSpeed);
		WRITE_SHORT((short)(m_raindropLife * 100));

		WRITE_SHORT(m_windPuffMinSize);
		WRITE_SHORT(m_windPuffMaxSize);
		WRITE_BYTE(m_windPuffRenderMode);
		WRITE_BYTE(m_windPuffBrightness);
		WRITE_BYTE((int)m_windPuffColor.x);
		WRITE_BYTE((int)m_windPuffColor.y);
		WRITE_BYTE((int)m_windPuffColor.z);
		WRITE_BYTE(m_windPuffLightMode);
		WRITE_SHORT((short)(m_windPuffLife * 100));

		WRITE_SHORT(m_splashMinSize);
		WRITE_SHORT(m_splashMaxSize);
		WRITE_BYTE(m_splashRenderMode);
		WRITE_BYTE(m_splashBrightness);
		WRITE_BYTE((int)m_splashColor.x);
		WRITE_BYTE((int)m_splashColor.y);
		WRITE_BYTE((int)m_splashColor.z);
		WRITE_BYTE(m_splashLightMode);

		WRITE_SHORT(m_rippleSize);
		WRITE_BYTE(m_rippleRenderMode);
		WRITE_BYTE(m_rippleBrightness);
		WRITE_BYTE((int)m_rippleColor.x);
		WRITE_BYTE((int)m_rippleColor.y);
		WRITE_BYTE((int)m_rippleColor.z);
		WRITE_BYTE(m_rippleLightMode);

		WRITE_STRING(m_raindropSprite ? STRING(m_raindropSprite) : "");
		WRITE_STRING(m_windpuffSprite ? STRING(m_windpuffSprite) : "");
		WRITE_STRING(m_splashSprite ? STRING(m_splashSprite) : "");
		WRITE_STRING(m_rippleSprite ? STRING(m_rippleSprite) : "");
	MESSAGE_END();
}

void CEnvRain::SendClearWeather()
{
	MESSAGE_BEGIN(MSG_ALL, gmsgRain);
		WRITE_LONG(entindex());
		WRITE_SHORT(0);
	MESSAGE_END();
}

class CFuncRainVolume : public CEnvRain
{
public:
	void Spawn() override {
		CEnvRain::Spawn();
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
		pev->spawnflags |= RAIN_BRUSH_ENTITY;
	}
};

LINK_ENTITY_TO_CLASS( func_rainvolume, CFuncRainVolume )

extern int gmsgSnow;

class CEnvSnow : public CBaseWeather
{
public:
	void KeyValue(KeyValueData *pkvd) override;
	void SendWeather(CBaseEntity* pClient = NULL) override;
	void SendClearWeather() override;

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

private:
	short m_snowflakeMinSize;
	short m_snowflakeMaxSize;
	byte m_snowflakeLightMode;
	short m_snowflakeMinSpeed;
	short m_snowflakeMaxSpeed;
	float m_snowflakeLife;

	string_t m_snowflakeSprite;
};

LINK_ENTITY_TO_CLASS( env_snow, CEnvSnow )

TYPEDESCRIPTION CEnvSnow::m_SaveData[] =
{
	DEFINE_FIELD( CEnvSnow, m_snowflakeMinSize, FIELD_SHORT ),
	DEFINE_FIELD( CEnvSnow, m_snowflakeMaxSize, FIELD_SHORT ),
	DEFINE_FIELD( CEnvSnow, m_snowflakeLightMode, FIELD_CHARACTER ),
	DEFINE_FIELD( CEnvSnow, m_snowflakeMinSpeed, FIELD_SHORT ),
	DEFINE_FIELD( CEnvSnow, m_snowflakeMaxSpeed, FIELD_SHORT ),
	DEFINE_FIELD( CEnvSnow, m_snowflakeLife, FIELD_FLOAT ),

	DEFINE_FIELD( CEnvSnow, m_snowflakeSprite, FIELD_STRING ),
};
IMPLEMENT_SAVERESTORE( CEnvSnow, CBaseWeather )

void CEnvSnow::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "snowflake_min_size"))
	{
		m_snowflakeMinSize = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "snowflake_max_size"))
	{
		m_snowflakeMaxSize = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "snowflake_light_mode"))
	{
		m_snowflakeLightMode = (byte)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "snowflake_min_speed"))
	{
		m_snowflakeMinSpeed = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "snowflake_max_speed"))
	{
		m_snowflakeMaxSpeed = (short)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "snowflake_life"))
	{
		m_snowflakeLife = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "snowflake_sprite"))
	{
		m_snowflakeSprite = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseWeather::KeyValue(pkvd);
}

void CEnvSnow::SendWeather(CBaseEntity *pClient)
{
	const int msgType = pClient ? MSG_ONE : MSG_ALL;
	edict_t* pClientEdict = pClient ? pClient->edict() : NULL;
	auto dynPos = GetDynamicPosition();

	MESSAGE_BEGIN(msgType, gmsgSnow, NULL, pClientEdict);
		WriteBaseWeaterData(pev->spawnflags, dynPos);

		WRITE_SHORT(m_snowflakeMinSize);
		WRITE_SHORT(m_snowflakeMaxSize);
		WRITE_BYTE(pev->rendermode);
		WRITE_BYTE(pev->renderamt);
		WRITE_COLOR(pev->rendercolor);
		WRITE_BYTE(m_snowflakeLightMode);
		WRITE_SHORT(m_snowflakeMinSpeed);
		WRITE_SHORT(m_snowflakeMaxSpeed);
		WRITE_SHORT((short)(m_snowflakeLife * 100));

		WRITE_STRING(m_snowflakeSprite ? STRING(m_snowflakeSprite) : "");
	MESSAGE_END();
}

void CEnvSnow::SendClearWeather()
{
	MESSAGE_BEGIN(MSG_ALL, gmsgSnow);
		WRITE_LONG(entindex());
		WRITE_SHORT(0);
	MESSAGE_END();
}

class CFuncSnowVolume : public CEnvSnow
{
public:
	void Spawn() override {
		CEnvSnow::Spawn();
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
		pev->spawnflags |= SNOW_BRUSH_ENTITY;
	}
};

LINK_ENTITY_TO_CLASS( func_snowvolume, CFuncSnowVolume )

#define SF_BEAMTRAIL_OFF 1
class CEnvBeamTrail : public CPointEntity
{
public:
	void	Spawn() override;
	void	Precache() override;
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void	EXPORT StartTrailThink ();
	void	Affect( CBaseEntity *pTarget, USE_TYPE useType );

	int		m_iSprite;	// Don't save, precache
};

void CEnvBeamTrail::Precache ()
{
	//if (pev->target)
	//	PRECACHE_MODEL("sprites/null.spr");
	if (pev->netname)
		m_iSprite = PRECACHE_MODEL ( STRING(pev->netname) );
}

LINK_ENTITY_TO_CLASS( env_beamtrail, CEnvBeamTrail )

void CEnvBeamTrail::StartTrailThink ()
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
			WRITE_COLOR( pev->rendercolor );
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

void CEnvBeamTrail::Spawn()
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

class CParticleShooter : public CPointEntity
{
public:
	void Precache() override;
	void Spawn() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void EXPORT ShootParticle();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_particleCount;
	int m_particleCountLeft;
	float m_fireDelay;

	float m_particleLife;
	float m_particleSpeed;

	float m_velVariance;

	float m_fadeSpeed;
	float m_scaleSpeed;

	string_t m_iszPosition;
	string_t m_iszDirection;
	EHANDLE m_hActivator;

	int m_iSprite;
};

LINK_ENTITY_TO_CLASS( particle_shooter, CParticleShooter )

TYPEDESCRIPTION	CParticleShooter::m_SaveData[] =
{
	DEFINE_FIELD( CParticleShooter, m_particleCount, FIELD_INTEGER ),
	DEFINE_FIELD( CParticleShooter, m_particleCountLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CParticleShooter, m_fireDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CParticleShooter, m_particleLife, FIELD_FLOAT ),
	DEFINE_FIELD( CParticleShooter, m_particleSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CParticleShooter, m_velVariance, FIELD_FLOAT ),
	DEFINE_FIELD( CParticleShooter, m_fadeSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CParticleShooter, m_scaleSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CParticleShooter, m_iszPosition, FIELD_STRING ),
	DEFINE_FIELD( CParticleShooter, m_iszDirection, FIELD_STRING ),
	DEFINE_FIELD( CParticleShooter, m_hActivator, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CParticleShooter, CPointEntity )

void CParticleShooter::Spawn()
{
	Precache();
	CPointEntity::Spawn();

	m_particleCountLeft = m_particleCount;

	if (m_fireDelay <= 0.0f)
		m_fireDelay = 0.1f;

	if (FBitSet(pev->spawnflags, SF_PARTICLESHOOTER_STARTON))
	{
		SetThink(&CParticleShooter::ShootParticle);
		pev->nextthink = gpGlobals->time + 0.2f;
	}
}

void CParticleShooter::Precache()
{
	m_iSprite = PRECACHE_MODEL(STRING(pev->model));
}

void CParticleShooter::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "num_particles" ) )
	{
		m_particleCount = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "fire_delay" ) )
	{
		m_fireDelay = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "particle_life" ) )
	{
		m_particleLife = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "particle_speed" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "vel_variance" ) )
	{
		m_velVariance = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "particle_size" ) )
	{
		pev->scale = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "colour_red" ) )
	{
		pev->rendercolor.x = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "colour_green" ) )
	{
		pev->rendercolor.y = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "colour_blue" ) )
	{
		pev->rendercolor.z = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "brightness" ) )
	{
		pev->renderamt = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "fade_speed" ) )
	{
		m_fadeSpeed = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "scale_speed" ) )
	{
		m_scaleSpeed = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "position"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "direction"))
	{
		m_iszDirection = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CParticleShooter::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	const bool active = FBitSet(pev->spawnflags, SF_PARTICLESHOOTER_STARTON);
	if (ShouldToggle(useType, active))
	{
		if (active)
		{
			ClearBits(pev->spawnflags, SF_PARTICLESHOOTER_STARTON);
			SetThink(NULL);
		}
		else
		{
			SetBits(pev->spawnflags, SF_PARTICLESHOOTER_STARTON);
			SetThink(&CParticleShooter::ShootParticle);
			pev->nextthink = gpGlobals->time;
		}
		m_hActivator = pActivator;
	}
}

void CParticleShooter::ShootParticle()
{
	extern int gmsgParticleShooter;

	if (FStringNull(pev->target) && FStringNull(m_iszDirection))
	{
		ALERT(at_error, "%s have neither a target nor a direction!\n", STRING(pev->classname));
		return;
	}

	pev->nextthink = gpGlobals->time + m_fireDelay;

	Vector position = pev->origin;

	if (!FStringNull(m_iszPosition))
	{
		if (!TryCalcLocus_Position(this, m_hActivator, STRING(m_iszPosition), position))
		{
			ALERT(at_error, "%s can't calc its position from '%s'!\n", STRING(pev->classname), STRING(m_iszPosition));
			return;
		}
	}

	Vector vecShootDir;
	if (!FStringNull(m_iszDirection))
	{
		if (!TryCalcLocus_Velocity(this, m_hActivator, STRING(m_iszDirection), vecShootDir))
		{
			ALERT(at_error, "%s can't calc its direction from '%s'!\n", STRING(pev->classname), STRING(m_iszDirection));
			return;
		}
		vecShootDir.NormalizeInPlace();
	}
	else
	{
		Vector vecShootAtPos;
		if (!TryCalcLocus_Position(this, m_hActivator, STRING(pev->target), vecShootAtPos))
		{
			ALERT(at_error, "%s can't calc position to shoot at from '%s'!\n", STRING(pev->classname), STRING(pev->target));
			return;
		}
		vecShootDir = (vecShootAtPos - position).Normalize();
	}

	float teta = acos(vecShootDir.z);
	float fi = (vecShootDir.x == 0 && vecShootDir.y == 0) ? RANDOM_FLOAT(0, M_PI_F * 2) : atan2(vecShootDir.y, vecShootDir.x);

	teta += RANDOM_FLOAT(-1.0f,1.0f) * m_velVariance * M_PI_F / 180.0f;
	fi += RANDOM_FLOAT(-1.0f,1.0f) * m_velVariance * M_PI_F / 180.0f;

	vecShootDir.x = cos(fi) * sin(teta);
	vecShootDir.y = sin(fi) * sin(teta);
	vecShootDir.z = cos(teta);

	/*vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT( -1.0f, 1.0f ) * m_velVariance;;
	vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT( -1.0f, 1.0f ) * m_velVariance;;
	vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT( -1.0f, 1.0f ) * m_velVariance;;
	vecShootDir = vecShootDir.Normalize();*/

	const Vector velocity = vecShootDir * pev->speed;

	if (gmsgParticleShooter)
	{
		MESSAGE_BEGIN( MSG_PVS, gmsgParticleShooter, position );
			WRITE_VECTOR(position);
			WRITE_VECTOR(velocity);
			WRITE_BYTE(pev->rendermode);
			WRITE_BYTE(m_particleLife * 10);
			WRITE_SHORT(pev->scale * 10);
			WRITE_COLOR(pev->rendercolor);
			WRITE_BYTE(pev->renderamt);
			WRITE_BYTE(pev->gravity * 100);
			WRITE_BYTE(m_fadeSpeed * 10);
			WRITE_BYTE(m_scaleSpeed * 10);
			WRITE_BYTE(pev->framerate);
			WRITE_BYTE(pev->spawnflags);
			WRITE_SHORT(m_iSprite);
		MESSAGE_END();
	}

	if (m_particleCount > 0)
	{
		m_particleCountLeft--;
		if (m_particleCountLeft <= 0)
		{
			ClearBits(pev->spawnflags, SF_PARTICLESHOOTER_STARTON);
			SetThink(NULL);
			if (FBitSet(pev->spawnflags, SF_PARTICLESHOOTER_REPEATABLE)) {
				m_particleCountLeft = m_particleCount;
			} else {
				SetThink(&CBaseEntity::SUB_Remove);
				pev->nextthink = gpGlobals->time;
			}
		}
	}
}

#define SF_EXTINGUISHER_REPEATABLE 1

class CEnvExtinguisher : public CPointEntity
{
public:
	void	Spawn() override;
	void	Precache() override;

	void EXPORT ExtinguisherUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TurnOff();

	static const NamedSoundScript startSoundScript;
	static const NamedSoundScript sizzleSoundScript;

	static const NamedVisual spriteVisual;
};

LINK_ENTITY_TO_CLASS( env_extinguisher, CEnvExtinguisher )

const NamedSoundScript CEnvExtinguisher::startSoundScript = {
	CHAN_WEAPON,
	{"weapons/explode3.wav"},
	1.0f,
	ATTN_STATIC,
	"Extinguisher.Start"
};

const NamedSoundScript CEnvExtinguisher::sizzleSoundScript = {
	CHAN_VOICE,
	{"ambience/steamjet1.wav"},
	1.0f,
	ATTN_STATIC,
	"Extinguisher.Sizzle"
};

const NamedVisual CEnvExtinguisher::spriteVisual = BuildVisual::Animated("Extinguisher.Sprite")
	.Model("sprites/xsmoke1.spr")
	.Scale(1.0f)
	.RenderMode(kRenderTransAdd)
	.Alpha(125);

void CEnvExtinguisher::Spawn()
{
	Precache();
	CPointEntity::Spawn();
	SetUse(&CEnvExtinguisher::ExtinguisherUse);
}

void CEnvExtinguisher::Precache()
{
	RegisterAndPrecacheSoundScript(startSoundScript);
	RegisterAndPrecacheSoundScript(sizzleSoundScript);
	RegisterVisual(spriteVisual);
}

void CEnvExtinguisher::ExtinguisherUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	EmitSoundScript(startSoundScript);
	EmitSoundScript(sizzleSoundScript);

	UTIL_MakeVectors(pev->angles);

	const Visual* pVisual = GetVisual(spriteVisual);

	const float baseScale = RandomizeNumberFromRange(pVisual->scale);

	const float scales[3] = {baseScale, baseScale * 1.5f,  baseScale * 1.75f};
	const float turnoffTime = gpGlobals->time + 1.5f;

	for (size_t i=0; i<ARRAYSIZE(scales); ++i)
	{
		CSprite* blastSprite = CreateSpriteFromVisual(pVisual, pev->origin + i*(gpGlobals->v_forward * 16.0f));
		if (blastSprite)
		{
			blastSprite->SetScale(scales[i]);
			blastSprite->AnimateForDurationAndDie(1.5f);
		}
	}

	SetUse(NULL);
	if (!FStringNull(pev->target))
	{
		FireTargets(STRING(pev->target), this, this, USE_ON, 0.0f);
	}

	SetThink(&CEnvExtinguisher::TurnOff);
	pev->nextthink = turnoffTime;
}

void CEnvExtinguisher::TurnOff()
{
	if (!FStringNull(pev->target))
	{
		FireTargets(STRING(pev->target), this, this, USE_OFF, 0.0f);
	}

	StopSoundScript(sizzleSoundScript);

	SetThink(NULL);
	if (FBitSet(pev->spawnflags, SF_EXTINGUISHER_REPEATABLE))
	{
		SetUse(&CEnvExtinguisher::ExtinguisherUse);
	}
	else
	{
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
}

//=========================================================
// G-Cont - env_mirror, mirroring only models
//=========================================================

#define SF_MIRROR_DRAWPLAYER 0x01

extern int gmsgMirror;

class CEnvMirror : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void KeyValue(KeyValueData* pkvd) override;
	int ObjectCaps() override { return CBaseEntity ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	void SendMessages(CBaseEntity* pClient);

	int m_iRadius;
	bool m_iActive;
	static TYPEDESCRIPTION m_SaveData[];
};

TYPEDESCRIPTION CEnvMirror::m_SaveData[] =
{
	DEFINE_FIELD(CEnvMirror, m_iRadius, FIELD_INTEGER),
	DEFINE_FIELD(CEnvMirror, m_iActive, FIELD_BOOLEAN),
};
IMPLEMENT_SAVERESTORE(CEnvMirror, CBaseEntity)

void CEnvMirror::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_iRadius = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(env_mirror, CEnvMirror)

void CEnvMirror::Spawn(void)
{
	pev->flags |= FL_WORLDBRUSH;

	pev->angles = g_vecZero;
	pev->movetype = MOVETYPE_PUSH; // so it doesn't get pushed by anything

	pev->solid = SOLID_BSP;

	m_iActive = true;

	Precache();

	SET_MODEL(ENT(pev), STRING(pev->model));

	if (pev->spawnflags & SF_MIRROR_DRAWPLAYER)
		CBaseEntity::Create("player_marker", VecBModelOrigin(pev), pev->angles);

	if (m_iRadius <= 0)
		m_iRadius = 330;
	if (!pev->frags) //Smart field system. g-cont
	{
		if (pev->size.y > pev->size.x && pev->size.z > pev->size.x)
			pev->frags = 0;
		if (pev->size.x > pev->size.y && pev->size.z > pev->size.y)
			pev->frags = 1;
		if (pev->size.y > pev->size.z && pev->size.x > pev->size.z)
			pev->frags = 2;
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

void CEnvMirror::SendMessages(CBaseEntity *pClient)
{
	edict_t* pClientEdict = pClient ? pClient->edict() : nullptr;
	const int msgType = pClient ? MSG_ONE : MSG_ALL;

	MESSAGE_BEGIN(msgType, gmsgMirror, nullptr, pClientEdict);
		WRITE_BYTE(m_iActive ? 1 : 0);
		WRITE_VECTOR(Center());
		WRITE_SHORT((int)m_iRadius);
		WRITE_BYTE((byte)pev->frags);
	MESSAGE_END();
}

void CEnvMirror::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, m_iActive))
	{
		m_iActive = !m_iActive;
		SendMessages(nullptr);
	}
}
