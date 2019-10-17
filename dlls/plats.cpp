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

===== plats.cpp ========================================================

  spawn, think, and touch functions for trains, etc

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "trains.h"
#include "saverestore.h"
#include "soundradius.h"
#include "movewith.h"

float SoundAttenuation(short soundRadius)
{
	switch (soundRadius) {
	case SOUND_RADIUS_SMALL:
		return ATTN_IDLE;
	case SOUND_RADIUS_MEDIUM:
		return ATTN_STATIC;
	case SOUND_RADIUS_LARGE:
		return ATTN_NORM;
	case SOUND_RADIUS_HUGE:
		return 0.5;
	case SOUND_RADIUS_ENORMOUS:
		return 0.25;
	case SOUND_RADIUS_EVERYWHERE:
		return ATTN_NONE;
	default:
		return ATTN_NORM;
	}
}

static void PlatSpawnInsideTrigger(entvars_t* pevPlatform);

#define SF_PLAT_TOGGLE		0x0001

class CBasePlatTrain : public CBaseToggle
{
public:
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void KeyValue( KeyValueData* pkvd);
	void Precache( void );

	// This is done to fix spawn flag collisions between this class and a derived class
	virtual BOOL IsTogglePlat( void ) { return ( pev->spawnflags & SF_PLAT_TOGGLE ) ? TRUE : FALSE; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BYTE m_bMoveSnd;			// sound a plat makes while moving
	BYTE m_bStopSnd;			// sound a plat makes when it stops
	float m_volume;			// Sound volume
	short m_soundRadius;

	float SoundAttenuation() const
	{
		return ::SoundAttenuation(m_soundRadius);
	}
};

TYPEDESCRIPTION	CBasePlatTrain::m_SaveData[] =
{
	DEFINE_FIELD( CBasePlatTrain, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlatTrain, m_bStopSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlatTrain, m_volume, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlatTrain, m_soundRadius, FIELD_SHORT ),
};

IMPLEMENT_SAVERESTORE( CBasePlatTrain, CBaseToggle )

void CBasePlatTrain::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "lip" ) )
	{
		m_flLip = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "height" ) )
	{
		m_flHeight = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "rotation" ) )
	{
		m_vecFinalAngle.x = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "movesnd" ) )
	{
		m_bMoveSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "stopsnd" ) )
	{
		m_bStopSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		m_volume = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "soundradius" ) )
	{
		m_soundRadius = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "custommovesnd" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "customstopsnd" ) )
	{
		pev->noise1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

#define noiseMoving noise
#define noiseArrived noise1

void CBasePlatTrain::Precache( void )
{
	const char *pszSound;
	BOOL NullSound = FALSE;

	if (FStringNull(pev->noiseMoving))
	{
		// set the plat's "in-motion" sound
		switch( m_bMoveSnd )
		{
		case 1:
			pszSound = "plats/bigmove1.wav";
			break;
		case 2:
			pszSound = "plats/bigmove2.wav";
			break;
		case 3:
			pszSound = "plats/elevmove1.wav";
			break;
		case 4:
			pszSound = "plats/elevmove2.wav";
			break;
		case 5:
			pszSound = "plats/elevmove3.wav";
			break;
		case 6:
			pszSound = "plats/freightmove1.wav";
			break;
		case 7:
			pszSound = "plats/freightmove2.wav";
			break;
		case 8:
			pszSound = "plats/heavymove1.wav";
			break;
		case 9:
			pszSound = "plats/rackmove1.wav";
			break;
		case 10:
			pszSound = "plats/railmove1.wav";
			break;
		case 11:
			pszSound = "plats/squeekmove1.wav";
			break;
		case 12:
			pszSound = "plats/talkmove1.wav";
			break;
		case 13:
			pszSound = "plats/talkmove2.wav";
			break;
		case 0:
		default:
			pszSound = "common/null.wav";
			NullSound = TRUE;
			break;
		}

		if( !NullSound )
			PRECACHE_SOUND( pszSound );
		pev->noiseMoving = MAKE_STRING( pszSound );
		NullSound = FALSE;
	}
	else
	{
		PRECACHE_SOUND(STRING(pev->noiseMoving));
	}

	if (FStringNull(pev->noiseArrived))
	{
		// set the plat's 'reached destination' stop sound
		switch( m_bStopSnd )
		{
		case 1:
			pszSound = "plats/bigstop1.wav";
			break;
		case 2:
			pszSound = "plats/bigstop2.wav";
			break;
		case 3:
			pszSound = "plats/freightstop1.wav";
			break;
		case 4:
			pszSound = "plats/heavystop2.wav";
			break;
		case 5:
			pszSound = "plats/rackstop1.wav";
			break;
		case 6:
			pszSound = "plats/railstop1.wav";
			break;
		case 7:
			pszSound = "plats/squeekstop1.wav";
			break;
		case 8:
			pszSound = "plats/talkstop1.wav";
			break;
		case 0:
		default:
			pszSound = "common/null.wav";
			NullSound = TRUE;
			break;
		}

		if( !NullSound )
			PRECACHE_SOUND( pszSound );
		pev->noiseArrived = MAKE_STRING( pszSound );
	}
	else
	{
		PRECACHE_SOUND(STRING(pev->noiseArrived));
	}
}

//
//====================== PLAT code ====================================================
//

#define noiseMovement noise
#define noiseStopMoving noise1

class CFuncPlat : public CBasePlatTrain
{
public:
	void Spawn( void );
	void Precache( void );
	void Setup( void );

	virtual void Blocked( CBaseEntity *pOther );

	void EXPORT PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void EXPORT CallGoUp( void ) { GoUp(); }
	void EXPORT CallGoDown( void ) { GoDown(); }
	void EXPORT CallHitTop( void  ) { HitTop(); }
	void EXPORT CallHitBottom( void ) { HitBottom(); }

	virtual void GoUp( void );
	virtual void GoDown( void );
	virtual void HitTop( void );
	virtual void HitBottom( void );
};

LINK_ENTITY_TO_CLASS( func_plat, CFuncPlat )

// UNDONE: Need to save this!!! It needs class & linkage
class CPlatTrigger : public CBaseEntity
{
public:
	virtual int ObjectCaps( void ) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }
	void SpawnInsideTrigger( CFuncPlat *pPlatform );
	void Touch( CBaseEntity *pOther );
	EHANDLE m_hPlatform;
};

/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed	default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in
the extended position until it is trigger, when it will lower and become a normal plat.

If the "height" key is set, that will determine the amount the plat moves, instead of
being implicitly determined by the model's height.

Set "sounds" to one of the following:
1) base fast
2) chain slow
*/

void CFuncPlat::Setup( void )
{
	//pev->noiseMovement = MAKE_STRING( "plats/platmove1.wav" );
	//pev->noiseStopMoving = MAKE_STRING( "plats/platstop1.wav" );

	if( m_flTLength == 0 )
		m_flTLength = 80;
	if( m_flTWidth == 0 )
		m_flTWidth = 10;
	
	pev->angles = g_vecZero;

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev), STRING( pev->model ) );

	// vecPosition1 is the top position, vecPosition2 is the bottom
	m_vecPosition1 = pev->origin;
	m_vecPosition2 = pev->origin;
	if( m_flHeight != 0 )
		m_vecPosition2.z = pev->origin.z - m_flHeight;
	else
		m_vecPosition2.z = pev->origin.z - pev->size.z + 8;
	if( pev->speed == 0 )
		pev->speed = 150;

	if( m_volume == 0 )
		m_volume = 0.85;
}

void CFuncPlat::Precache()
{
	CBasePlatTrain::Precache();
	//PRECACHE_SOUND( "plats/platmove1.wav" );
	//PRECACHE_SOUND( "plats/platstop1.wav" );
	if( !IsTogglePlat() )
		PlatSpawnInsideTrigger( pev );		// the "start moving" trigger
}

void CFuncPlat::Spawn()
{
	Setup();

	Precache();

	// If this platform is the target of some button, it starts at the TOP position,
	// and is brought down by that button.  Otherwise, it starts at BOTTOM.
	if( !FStringNull( pev->targetname ) )
	{
		if (m_pMoveWith)
			UTIL_AssignOrigin (this, m_vecPosition1 + m_pMoveWith->pev->origin);
		else
			UTIL_AssignOrigin (this, m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		SetUse( &CFuncPlat::PlatUse );
	}
	else
	{
		if (m_pMoveWith)
			UTIL_AssignOrigin (this, m_vecPosition2 + m_pMoveWith->pev->origin);
		else
			UTIL_AssignOrigin (this, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
	}
}

static void PlatSpawnInsideTrigger( entvars_t *pevPlatform )
{
	GetClassPtr( (CPlatTrigger *)NULL )->SpawnInsideTrigger( GetClassPtr( (CFuncPlat *)pevPlatform ) );
}
		
//
// Create a trigger entity for a platform.
//
void CPlatTrigger::SpawnInsideTrigger( CFuncPlat *pPlatform )
{
	m_hPlatform = pPlatform;
	// Create trigger entity, "point" it at the owning platform, give it a touch method
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->origin = pPlatform->pev->origin;

	// Establish the trigger field's size
	Vector vecTMin = pPlatform->pev->mins + Vector( 25, 25, 0 );
	Vector vecTMax = pPlatform->pev->maxs + Vector( 25, 25, 8 );
	vecTMin.z = vecTMax.z - ( pPlatform->m_vecPosition1.z - pPlatform->m_vecPosition2.z + 8 );
	if( pPlatform->pev->size.x <= 50 )
	{
		vecTMin.x = ( pPlatform->pev->mins.x + pPlatform->pev->maxs.x ) / 2;
		vecTMax.x = vecTMin.x + 1;
	}
	if( pPlatform->pev->size.y <= 50 )
	{
		vecTMin.y = ( pPlatform->pev->mins.y + pPlatform->pev->maxs.y ) / 2;
		vecTMax.y = vecTMin.y + 1;
	}
	UTIL_SetSize( pev, vecTMin, vecTMax );
}

//
// When the platform's trigger field is touched, the platform ???
//
void CPlatTrigger::Touch( CBaseEntity *pOther )
{
	// Ignore touches by non-players
	if( !pOther->IsPlayer() )
		return;

	CFuncPlat *pPlatform = (CFuncPlat*)(CBaseEntity*)m_hPlatform;

	if( !pPlatform )
	{
		// The target platform has been removed, remove myself as well. - Solokiller
		UTIL_Remove( this );
		return;
	}

	// Ignore touches by corpses
	if( !pOther->IsAlive() )
		return;

	// Make linked platform go up/down.
	if( pPlatform->m_toggle_state == TS_AT_BOTTOM )
		pPlatform->GoUp();
	else if( pPlatform->m_toggle_state == TS_AT_TOP )
		pPlatform->SetNextThink( 1 );// delay going down
}

//
// Used by SUB_UseTargets, when a platform is the target of a button.
// Start bringing platform down.
//
void CFuncPlat::PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( IsTogglePlat() )
	{
		// Top is off, bottom is on
		BOOL on = ( m_toggle_state == TS_AT_BOTTOM ) ? TRUE : FALSE;

		if( !ShouldToggle( useType, on ) )
			return;

		if( m_toggle_state == TS_AT_TOP )
		{
			SetNextThink( 0.01 );
			SetThink(&CFuncPlat::CallGoDown);
		}
		else if( m_toggle_state == TS_AT_BOTTOM )
		{
			SetNextThink( 0.01 );
			SetThink(&CFuncPlat::CallGoUp);
		}
	}
	else
	{
		SetUse( NULL );

		if( m_toggle_state == TS_AT_TOP )
		{
			SetNextThink( 0.01 );
			SetThink(&CFuncPlat::CallGoDown);
		}
	}
}

//
// Platform is at top, now starts moving down.
//
void CFuncPlat::GoDown( void )
{
	if( pev->noiseMovement )
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMovement ), m_volume, SoundAttenuation() );

	ASSERT( m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP );
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone( &CFuncPlat::CallHitBottom );
	LinearMove( m_vecPosition2, pev->speed );
}

//
// Platform has hit bottom. Stops and waits forever.
//
void CFuncPlat::HitBottom( void )
{
	if( pev->noiseMovement )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMovement ) );

	if( pev->noiseStopMoving )
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, STRING( pev->noiseStopMoving ), m_volume, SoundAttenuation() );

	ASSERT( m_toggle_state == TS_GOING_DOWN );
	m_toggle_state = TS_AT_BOTTOM;
}

//
// Platform is at bottom, now starts moving up
//
void CFuncPlat::GoUp( void )
{
	if( pev->noiseMovement )
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMovement ), m_volume, SoundAttenuation() );

	ASSERT( m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN );
	m_toggle_state = TS_GOING_UP;
	SetMoveDone( &CFuncPlat::CallHitTop );
	LinearMove(m_vecPosition1, pev->speed);
}

//
// Platform has hit top. Pauses, then starts back down again.
//
void CFuncPlat::HitTop( void )
{
	if( pev->noiseMovement )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMovement ) );

	if( pev->noiseStopMoving )
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, STRING( pev->noiseStopMoving ), m_volume, SoundAttenuation() );

	ASSERT( m_toggle_state == TS_GOING_UP );
	m_toggle_state = TS_AT_TOP;

	if( !IsTogglePlat() )
	{
		// After a delay, the platform will automatically start going down again.
		SetThink( &CFuncPlat::CallGoDown );
		SetNextThink( 3 );
	}
}

void CFuncPlat::Blocked( CBaseEntity *pOther )
{
	ALERT( at_aiconsole, "%s Blocked by %s\n", STRING( pev->classname ), STRING( pOther->pev->classname ) );
	// Hurt the blocker a little
	pOther->TakeDamage( pev, pev, 1, DMG_CRUSH );

	if( pev->noiseMovement )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMovement ) );

	// Send the platform back where it came from
	ASSERT( m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN );
	if( m_toggle_state == TS_GOING_UP )
	{
		SetNextThink( 0 );
		SetThink(&CFuncPlat::CallGoDown );
	}
	else if( m_toggle_state == TS_GOING_DOWN )
	{
		SetNextThink( 0 );
		SetThink(&CFuncPlat::CallGoUp );
	}
}

class CFuncPlatRot : public CFuncPlat
{
public:
	void Spawn( void );
	void SetupRotation( void );

	virtual void GoUp( void );
	virtual void GoDown( void );
	virtual void HitTop( void );
	virtual void HitBottom( void );

	void RotMove( Vector &destAngle, float time );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_end, m_start;
};

LINK_ENTITY_TO_CLASS( func_platrot, CFuncPlatRot )
TYPEDESCRIPTION	CFuncPlatRot::m_SaveData[] =
{
	DEFINE_FIELD( CFuncPlatRot, m_end, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncPlatRot, m_start, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFuncPlatRot, CFuncPlat )

void CFuncPlatRot::SetupRotation( void )
{
	if( m_vecFinalAngle.x != 0 )		// This plat rotates too!
	{
		CBaseToggle::AxisDir( pev );
		m_start	= pev->angles;
		m_end = pev->angles + pev->movedir * m_vecFinalAngle.x;
	}
	else
	{
		m_start = g_vecZero;
		m_end = g_vecZero;
	}
	if( !FStringNull( pev->targetname ) )	// Start at top
	{
		UTIL_SetAngles(this, m_end);
	}
}

void CFuncPlatRot::Spawn( void )
{
	CFuncPlat::Spawn();
	SetupRotation();
}

void CFuncPlatRot::GoDown( void )
{
	CFuncPlat::GoDown();
	RotMove( m_start, m_fNextThink - pev->ltime );
}

//
// Platform has hit bottom. Stops and waits forever.
//
void CFuncPlatRot::HitBottom( void )
{
	CFuncPlat::HitBottom();
	UTIL_SetAvelocity(this, g_vecZero);
	UTIL_SetAngles(this, m_start);
}

//
// Platform is at bottom, now starts moving up
//
void CFuncPlatRot::GoUp( void )
{
	CFuncPlat::GoUp();
	RotMove( m_end, m_fNextThink - pev->ltime );
}

//
// Platform has hit top. Pauses, then starts back down again.
//
void CFuncPlatRot::HitTop( void )
{
	CFuncPlat::HitTop();
	UTIL_SetAvelocity(this, g_vecZero);
	UTIL_SetAngles(this, m_end);
}

void CFuncPlatRot::RotMove( Vector &destAngle, float time )
{
	// set destdelta to the vector needed to move
	Vector vecDestDelta = destAngle - pev->angles;

	// Travel time is so short, we're practically there already;  so make it so.
	if( time >= 0.1f )
	{
		UTIL_SetAvelocity(this, vecDestDelta / time);
	}
	else
	{
		UTIL_SetAvelocity(this, vecDestDelta);
		SetNextThink( 1 );
	}
}

//
//====================== TRAIN code ==================================================
//
class CFuncTrain : public CBasePlatTrain
{
public:
	void Spawn( void );
	void Precache( void );
	void PostSpawn( void );
	void OverrideReset( void );

	void Blocked( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	void EXPORT ThinkWait( void );
	void EXPORT ThinkNext( void );
	virtual void Wait( void );
	virtual void Next( void );
	void EXPORT ThinkDoNext( void );
	void EXPORT SoundSetup( void );

	virtual void ThinkCorrection( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual void TrainThink() {}
	Vector DestinationVector(entvars_t* pevTarg)
	{
		if (FBitSet(pev->spawnflags, SF_TRAIN_SETORIGIN))
		{
			return pevTarg->origin;
		}
		else
		{
			return pevTarg->origin - ( pev->mins + pev->maxs ) * 0.5f;
		}
	}

	entvars_t *m_pevCurrentTarget;
	BOOL m_activated;
	BOOL m_iObeyTriggerMode;
	short m_iPitch;

protected:
	void SetDefaultTrainValues();
public:
	int m_sounds;
	STATE		m_iState;
	float m_fStoredThink;
	Vector m_vecAvelocity;
};

LINK_ENTITY_TO_CLASS( func_train, CFuncTrain )

TYPEDESCRIPTION	CFuncTrain::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTrain, m_pevCurrentTarget, FIELD_EVARS ),
	DEFINE_FIELD( CFuncTrain, m_activated, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFuncTrain, m_iObeyTriggerMode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFuncTrain, m_iPitch, FIELD_SHORT ),
	DEFINE_FIELD( CFuncTrain, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrain, m_fStoredThink, FIELD_TIME ),
	DEFINE_FIELD( CFuncTrain, m_vecAvelocity, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFuncTrain, CBasePlatTrain )

void CFuncTrain::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_iObeyTriggerMode" ) )
	{
		m_iObeyTriggerMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "soundpitch" ) )
	{
		m_iPitch = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBasePlatTrain::KeyValue( pkvd );
}

void CFuncTrain::Blocked( CBaseEntity *pOther )
{
	// Keep "movewith" entities in line
	UTIL_AssignOrigin(this, pev->origin);

	if( gpGlobals->time < m_flActivateFinished )
		return;

	m_flActivateFinished = gpGlobals->time + 0.5f;

	if (pev->dmg)
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );
}

void CFuncTrain::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iObeyTriggerMode && !ShouldToggle(useType, !FBitSet(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER)))
		return;

	if( pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER )
	{
		// Move toward my target
		pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
		Next();
	}
	else
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// Pop back to last target if it's available
		if( pev->enemy )
			pev->target = pev->enemy->v.targetname;
		DontThink();
		UTIL_SetVelocity(this, g_vecZero);
		UTIL_SetAvelocity(this, g_vecZero);
		m_iState = STATE_OFF;

		if ( pev->noiseMovement )
			STOP_SOUND( edict(), CHAN_STATIC, STRING(pev->noiseMovement) );

		if( pev->noiseStopMoving )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noiseStopMoving ), m_volume, SoundAttenuation() );
	}
}

void CFuncTrain::ThinkWait()
{
	Wait();
}

void CFuncTrain::Wait( void )
{
	// Fire the pass target if there is one
	if( m_pevCurrentTarget->message )
	{
		FireTargets( STRING( m_pevCurrentTarget->message ), this, this, USE_TOGGLE, 0 );
		if( FBitSet( m_pevCurrentTarget->spawnflags, SF_CORNER_FIREONCE ) )
			m_pevCurrentTarget->message = 0;
	}

	// need pointer to LAST target.
	if( FBitSet( m_pevCurrentTarget->spawnflags, SF_TRAIN_WAIT_RETRIGGER ) || ( pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER ) )
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		m_iState = STATE_OFF;

		// clear the sound channel.
		if( pev->noiseMovement )
			STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noiseMovement ) );
		if( pev->noiseStopMoving )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noiseStopMoving ), m_volume, SoundAttenuation() );

		UTIL_SetVelocity(this, g_vecZero);
		UTIL_SetAvelocity(this, g_vecZero);
		DontThink();
		return;
	}

	// ALERT( at_console, "%f\n", m_flWait );
	if( m_flWait != 0 )
	{
		m_iState = STATE_OFF;
		// -1 wait will wait forever!		
		UTIL_SetAvelocity(this, g_vecZero);
		UTIL_SetVelocity(this, g_vecZero);
		SetNextThink( m_flWait );
		if( pev->noiseMovement )
			STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noiseMovement ) );
		if( pev->noiseStopMoving )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noiseStopMoving ), m_volume, SoundAttenuation() );
		SetThink( &CFuncTrain::ThinkNext );
	}
	else
	{
		Next();// do it RIGHT now!
	}
}

//
// Train next - path corner needs to change to next target 
//
void CFuncTrain::Next( void )
{
	CBaseEntity *pTarg;

	// now find our next target
	pTarg = GetNextTarget();

	if( !pTarg )
	{
		m_iState = STATE_OFF;
		UTIL_SetVelocity(this, g_vecZero);
		UTIL_SetAvelocity(this, g_vecZero);

		if( pev->noiseMovement )
			STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noiseMovement ) );
		// Play stop sound
		if( pev->noiseStopMoving )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noiseStopMoving ), m_volume, SoundAttenuation() );
		return;
	}

	// Save last target in case we need to find it again
	pev->message = pev->target;

	pev->target = pTarg->pev->target;
	m_flWait = pTarg->GetDelay();

	if( m_pevCurrentTarget && m_pevCurrentTarget->speed != 0 )
	{
		// don't copy speed from target if it is 0 (uninitialized)
		pev->speed = m_pevCurrentTarget->speed;
		ALERT( at_aiconsole, "Train %s speed to %4.2f\n", STRING( pev->targetname ), (double)pev->speed );
	}
	m_pevCurrentTarget = pTarg->pev;// keep track of this since path corners change our target for us.

	pev->enemy = pTarg->edict();//hack

	if( FBitSet( m_pevCurrentTarget->spawnflags, SF_CORNER_TELEPORT ) )
	{
		// Path corner has indicated a teleport to the next corner.
		SetBits( pev->effects, EF_NOINTERP );

		if (m_pMoveWith)
		{
			if (pev->spawnflags & SF_TRAIN_SETORIGIN)
				UTIL_AssignOrigin(this, pTarg->pev->origin - m_pMoveWith->pev->origin );
			else
				UTIL_AssignOrigin(this, pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5 - m_pMoveWith->pev->origin );
		}
		else
		{
			if (pev->spawnflags & SF_TRAIN_SETORIGIN)
				UTIL_AssignOrigin(this, pTarg->pev->origin );
			else
				UTIL_AssignOrigin(this, pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5 );
		}

		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		// Normal linear move.
		
		// CHANGED this from CHAN_VOICE to CHAN_STATIC around OEM beta time because trains should
		// use CHAN_STATIC for their movement sounds to prevent sound field problems.
		// this is not a hack or temporary fix, this is how things should be. (sjb).
		if (m_iState == STATE_OFF)
		{
			STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noiseMovement ) );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMovement ), m_volume, SoundAttenuation(), 0, m_iPitch );
			if( pev->noiseMovement )
			{
				STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noiseMovement ) );
				EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMovement ), m_volume, SoundAttenuation() );
			}
		}

		ClearBits( pev->effects, EF_NOINTERP );
		SetMoveDone( &CFuncTrain::ThinkWait );

		UTIL_SetAvelocity(this, m_vecAvelocity);

		m_iState = STATE_ON;

		if (m_pMoveWith)
		{
			LinearMove( DestinationVector(pTarg->pev) - m_pMoveWith->pev->origin, pev->speed );
		}
		else
		{
			LinearMove( DestinationVector(pTarg->pev), pev->speed );
		}
	}
}

void CFuncTrain::ThinkNext()
{
	Next();
}

void CFuncTrain::PostSpawn( void )
{
	entvars_t *pevTarg = VARS( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) ) );

	m_iState = STATE_OFF;

	pev->target = pevTarg->target;
	m_pevCurrentTarget = pevTarg;// keep track of this since path corners change our target for us.

	if (pev->avelocity != g_vecZero)
	{
		m_vecAvelocity = pev->avelocity;
		UTIL_SetAvelocity(this, g_vecZero);
	}

	m_vecSpawnOffset = m_vecSpawnOffset + pevTarg->origin - pev->origin;
	if (m_pMoveWith)
		UTIL_AssignOrigin (this, DestinationVector(pevTarg) - m_pMoveWith->pev->origin );
	else
		UTIL_AssignOrigin (this, DestinationVector(pevTarg) );

	if( FStringNull( pev->targetname ) || (pev->spawnflags & SF_TRAIN_START_ON) )
	{	// not triggered, so start immediately
		SetNextThink( 0.1f );
		SetThink(&CFuncTrain::ThinkDoNext );
	}
	else
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
}

void CFuncTrain::ThinkDoNext( void )
{
	SetNextThink( 0.1 );
	if (gpGlobals->time != 1.0) // only go on if the game has properly started yet
		SetThink(&CFuncTrain::ThinkNext );
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal
*/
void CFuncTrain::Spawn( void )
{
	Precache();

	SetDefaultTrainValues();

	pev->movetype = MOVETYPE_PUSH;

	if( FBitSet( pev->spawnflags, SF_TRAIN_PASSABLE ) )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );
}

void CFuncTrain::SetDefaultTrainValues()
{
	if( pev->speed == 0 )
		pev->speed = 100;

	if( FStringNull(pev->target) )
		ALERT( at_console, "%s with no target\n", STRING(pev->classname) );

	if( pev->dmg == 0 && !FBitSet(pev->spawnflags, SF_TRAIN_NO_DAMAGE) )
		pev->dmg = 2;

	if (m_iPitch == 0)
		m_iPitch = 100;

	m_iState = STATE_OFF;

	if( m_volume == 0 )
		m_volume = 0.85f;
}

//LRC - making movement sounds which continue after a game is loaded.
void CFuncTrain::SoundSetup( void )
{
	EMIT_SOUND (ENT(pev), CHAN_STATIC, STRING(pev->noiseMovement), m_volume, ATTN_NORM);
	SetNextThink( m_fStoredThink - pev->ltime );
//	ALERT(at_console, "SoundSetup: mfNT %f, pevNT %f, stored was %f, time %f", m_fNextThink, pev->nextthink, m_fStoredThink, pev->ltime );
	m_fStoredThink = 0;
	SetThink(&CFuncTrain :: LinearMoveDone );
}

//LRC
void CFuncTrain::ThinkCorrection( void )
{
	if (m_fStoredThink && pev->nextthink != m_fPevNextThink)
	{
//		ALERT(at_console, "StoredThink Correction for train \"%s\", %f -> %f\n", STRING(pev->targetname), m_fStoredThink, m_fStoredThink + pev->nextthink - m_fPevNextThink);
		m_fStoredThink += pev->nextthink -  m_fPevNextThink;
	}

	CBasePlatTrain::ThinkCorrection();
}

void CFuncTrain::Precache( void )
{
	CBasePlatTrain::Precache();

	//LRC - continue the movement sound after loading a game
	if (m_iState == STATE_ON && pev->noiseMovement)
	{
		// we can't set up SFX during precache, so get a think to do it.
		// Unfortunately, since we're moving, we must be already thinking.
		// So we store the current think time, and will restore it after SFX are done.
		if (!m_fStoredThink)
			m_fStoredThink = m_fNextThink;
		SetNextThink( 0.1 );
//		ALERT(at_console, "preparing SoundSetup: stored %f, mfNT %f, pevNT %f, ltime %f", m_fStoredThink, m_fNextThink, pev->nextthink, pev->ltime);
		SetThink(&CFuncTrain::SoundSetup );
	}

#if 0	// obsolete
	// otherwise use preset sound
	switch( m_sounds )
	{
	case 0:
		pev->noise = 0;
		pev->noise1 = 0;
		break;
	case 1:
		PRECACHE_SOUND( "plats/train2.wav" );
		PRECACHE_SOUND( "plats/train1.wav" );
		pev->noise = MAKE_STRING( "plats/train2.wav" );
		pev->noise1 = MAKE_STRING( "plats/train1.wav" );
		break;
	case 2:
		PRECACHE_SOUND( "plats/platmove1.wav" );
		PRECACHE_SOUND( "plats/platstop1.wav" );
		pev->noise = MAKE_STRING( "plats/platstop1.wav" );
		pev->noise1 = MAKE_STRING( "plats/platmove1.wav" );
		break;
	}
#endif
}

void CFuncTrain::OverrideReset( void )
{
	CBaseEntity *pTarg;

	// Are we moving?
	if ( m_iState == STATE_ON )
	{
		pev->target = pev->message;
		// now find our next target
		pTarg = GetNextTarget();
		if( !pTarg )
		{
			DontThink();
			UTIL_SetVelocity(this, g_vecZero);
			m_iState = STATE_OFF;
		}
		else	// Keep moving for 0.1 secs, then find path_corner again and restart
		{
			SetThink( &CFuncTrain::ThinkNext );
			SetNextThink( 0.1f );
		}
	}
}

// ---------------------------------------------------------------------
//
// Track Train
//
// ---------------------------------------------------------------------

TYPEDESCRIPTION	CFuncTrackTrain::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTrackTrain, m_ppath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTrackTrain, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_speed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_dir, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_startSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_controlMins, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTrackTrain, m_controlMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTrackTrain, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackTrain, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_flBank, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_oldSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_customMoveSound, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CFuncTrackTrain, CBaseEntity )
LINK_ENTITY_TO_CLASS( func_tracktrain, CFuncTrackTrain )

void CFuncTrackTrain::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "wheels" ) )
	{
		m_length = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "height" ) )
	{
		m_height = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "startspeed" ) )
	{
		m_startSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "sounds" ) )
	{
		m_sounds = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		m_flVolume = (float)atoi( pkvd->szValue );
		m_flVolume *= 0.1f;
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bank" ) )
	{
		m_flBank = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CFuncTrackTrain::NextThink( float thinkTime, BOOL alwaysThink )
{
	if( alwaysThink )
		pev->flags |= FL_ALWAYSTHINK;
	else
		pev->flags &= ~FL_ALWAYSTHINK;

	SetNextThink( thinkTime, TRUE );
}

void CFuncTrackTrain::Blocked( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	// Blocker is on-ground on the train
	if( FBitSet( pevOther->flags, FL_ONGROUND ) && VARS( pevOther->groundentity ) == pev )
	{
		float deltaSpeed = fabs( pev->speed );
		if( deltaSpeed > 50 )
			deltaSpeed = 50;
		if( !pevOther->velocity.z )
			pevOther->velocity.z += deltaSpeed;
		return;
	}
	else
		pevOther->velocity = ( pevOther->origin - pev->origin ).Normalize() * pev->dmg;

	ALERT( at_aiconsole, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", STRING( pev->targetname ), STRING( pOther->pev->classname ), (double)pev->dmg );
	if( pev->dmg <= 0 )
		return;
	// we can't hurt this thing, so we're not concerned with it
	pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );
}

void CFuncTrackTrain::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( useType != USE_SET )
	{
		if( !ShouldToggle( useType, ( pev->speed != 0 ) ) )
			return;

		if( pev->speed == 0 )
		{
			pev->speed = m_speed * m_dir;
		
			PostponeNext();
		}
		else
		{
			pev->speed = 0;
			UTIL_SetVelocity(this, g_vecZero);
			UTIL_SetAvelocity(this, g_vecZero);
			StopSound();
			SetThink( NULL );
		}
	}
	else
	{
		float delta = value;

		delta = ( (int)( pev->speed * 4 ) / (int)m_speed ) * 0.25f + 0.25f * delta;
		if( delta > 1 )
			delta = 1;
		else if ( delta < -1 )
			delta = -1;
		if( pev->spawnflags & SF_TRACKTRAIN_FORWARDONLY )
		{
			if( delta < 0 )
				delta = 0;
		}
		pev->speed = m_speed * delta;
		PostponeNext();
		ALERT( at_aiconsole, "TRAIN(%s), speed to %.2f\n", STRING( pev->targetname ), (double)pev->speed );
	}
}

static float Fix( float angle )
{
	while( angle < 0 )
		angle += 360;
	while( angle > 360 )
		angle -= 360;

	return angle;
}

static void FixupAngles( Vector &v )
{
	v.x = Fix( v.x );
	v.y = Fix( v.y );
	v.z = Fix( v.z );
}

#define TRAIN_STARTPITCH	60
#define TRAIN_MAXPITCH		200
#define TRAIN_MAXSPEED		1000	// approx max speed for sound pitch calculation

void CFuncTrackTrain::StopSound( void )
{
	// if sound playing, stop it
	if( m_soundPlaying && pev->noise )
	{
		unsigned short us_encode;
		unsigned short us_sound  = ( (unsigned short)( m_sounds ) & 0x0007 ) << 12;

		us_encode = us_sound;

		if (!m_customMoveSound)
			PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0,
			g_vecZero, g_vecZero, 0.0, 0.0, us_encode, 0, 1, 0 );
		else
			STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noise ) );

		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, STRING(pev->noise1), m_flVolume, ATTN_NORM, 0, 100 );
	}

	m_soundPlaying = 0;
}

// update pitch based on speed, start sound if not playing
// NOTE: when train goes through transition, m_soundPlaying should go to 0, 
// which will cause the looped sound to restart.
void CFuncTrackTrain::UpdateSound( void )
{
	float flpitch;

	if( !pev->noise )
		return;

	flpitch = TRAIN_STARTPITCH + ( fabs( pev->speed ) * ( TRAIN_MAXPITCH - TRAIN_STARTPITCH ) / TRAIN_MAXSPEED );

	if( !m_soundPlaying )
	{
		// play startup sound for train
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, STRING(pev->noise2), m_flVolume, ATTN_NORM, 0, 100 );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, STRING( pev->noise ), m_flVolume, ATTN_NORM, 0, (int)flpitch );
		m_soundPlaying = 1;
	} 
	else
	{
		if (m_customMoveSound)
		{
			// update pitch
			EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, STRING( pev->noise ), m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, (int)flpitch );
		}
		else
		{
			// volume 0.0 - 1.0 - 6 bits
			// m_sounds 3 bits
			// flpitch = 6 bits
			// 15 bits total

			unsigned short us_encode;
			unsigned short us_sound  = ( ( unsigned short )( m_sounds ) & 0x0007 ) << 12;
			unsigned short us_pitch  = ( ( unsigned short )( flpitch / 10.0f ) & 0x003f ) << 6;
			unsigned short us_volume = ( ( unsigned short )( m_flVolume * 40.0f ) & 0x003f );

			us_encode = us_sound | us_pitch | us_volume;

			PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0f,
				g_vecZero, g_vecZero, 0.0f, 0.0f, us_encode, 0, 0, 0 );
		}
	}
}

void CFuncTrackTrain::PostponeNext( void )
{
	if (m_pMoveWith || m_pChildMoveWith)
		UTIL_DesiredAction(this);
	else
		Next();
}

void CFuncTrackTrain::DesiredAction( void )
{
	Next();
}

void CFuncTrackTrain::Next( void )
{
	float time = 0.5;

	if( !pev->speed )
	{
		UTIL_SetVelocity(this, g_vecZero);
		DontThink();
		ALERT( at_aiconsole, "TRAIN(%s): Speed is 0\n", STRING( pev->targetname ) );
		StopSound();
		return;
	}

	//if( !m_ppath )
	//	m_ppath = CPathTrack::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) ) );
	if( !m_ppath )
	{
		UTIL_SetVelocity(this, g_vecZero);
		DontThink();
		ALERT( at_aiconsole, "TRAIN(%s): Lost path\n", STRING( pev->targetname ) );
		StopSound();
		return;
	}

	UpdateSound();

	Vector nextPos = pev->origin;

	nextPos.z -= m_height;
	CPathTrack *pnext = m_ppath->LookAhead( &nextPos, pev->speed * 0.1f, 1 );
	nextPos.z += m_height;

	UTIL_SetVelocity( this, (nextPos - pev->origin) * 10 );
	Vector nextFront = pev->origin;

	nextFront.z -= m_height;
	if( m_length > 0 )
		m_ppath->LookAhead( &nextFront, m_length, 0 );
	else
		m_ppath->LookAhead( &nextFront, 100, 0 );
	nextFront.z += m_height;

	Vector delta = nextFront - pev->origin;
	Vector angles = UTIL_VecToAngles( delta );
	// The train actually points west
	angles.y += 180;

	// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
	FixupAngles( angles );
	FixupAngles( pev->angles );

	if( !pnext || ( delta.x == 0 && delta.y == 0 ) )
		angles = pev->angles;

	float vy, vx, vz = pev->avelocity.z;
	if( !( pev->spawnflags & SF_TRACKTRAIN_NOPITCH ) )
		vx = UTIL_AngleDistance( angles.x, pev->angles.x );
	else
		vx = 0;
	vy = UTIL_AngleDistance( angles.y, pev->angles.y );

	vy = vy * 10;
	vx = vx * 10;

	if( m_flBank != 0 )
	{
		if( pev->avelocity.y < -5 )
			vz = UTIL_AngleDistance( UTIL_ApproachAngle( -m_flBank, pev->angles.z, m_flBank * 2 ), pev->angles.z );
		else if( pev->avelocity.y > 5 )
			vz = UTIL_AngleDistance( UTIL_ApproachAngle( m_flBank, pev->angles.z, m_flBank * 2 ), pev->angles.z );
		else
			vz = UTIL_AngleDistance( UTIL_ApproachAngle( 0, pev->angles.z, m_flBank * 4 ), pev->angles.z ) * 4;
	}
	UTIL_SetAvelocity(this, Vector(vx, vy, vz));

	if( pnext )
	{
		if( pnext != m_ppath )
		{
			CPathTrack *pFire;
			if( pev->speed >= 0 )
				pFire = pnext;
			else 
				pFire = m_ppath;

			m_ppath = pnext;
			// Fire the pass target if there is one
			if( pFire->pev->message )
			{
				FireTargets( STRING( pFire->pev->message ), this, this, USE_TOGGLE, 0 );
				if( FBitSet( pFire->pev->spawnflags, SF_PATH_FIREONCE ) )
					pFire->pev->message = 0;
			}

			if( pFire->pev->spawnflags & SF_PATH_DISABLE_TRAIN )
				pev->spawnflags |= SF_TRACKTRAIN_NOCONTROL;

			// Don't override speed if under user control
			if( pev->spawnflags & SF_TRACKTRAIN_NOCONTROL )
			{
				if( pFire->pev->speed != 0 )
				{
					// don't copy speed from target if it is 0 (uninitialized)
					pev->speed = pFire->pev->speed;
					ALERT( at_aiconsole, "TrackTrain %s speed to %4.2f\n", STRING( pev->targetname ), (double)pev->speed );
				}
			}

		}
		SetThink( &CFuncTrackTrain::PostponeNext );
		NextThink( time, TRUE );
	}
	else // end of path, stop
	{
		StopSound();
		Vector vecTemp = nextPos - pev->origin;
		pev->avelocity = g_vecZero;
		float distance = pev->velocity.Length();
		m_oldSpeed = pev->speed;

		pev->speed = 0;

		// Move to the dead end

		// Are we there yet?
		if( distance > 0 )
		{
			// no, how long to get there?
			time = distance / m_oldSpeed;
			UTIL_SetVelocity(this, vecTemp * ( m_oldSpeed / distance ));
			SetThink( &CFuncTrackTrain::DeadEnd );
			NextThink( time, FALSE );
		}
		else
		{
			UTIL_SetVelocity(this, vecTemp);
			DeadEnd();
		}
	}
}

void CFuncTrackTrain::DeadEnd( void )
{
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;

	pTrack = m_ppath;

	ALERT( at_aiconsole, "TRAIN(%s): Dead end ", STRING( pev->targetname ) );
	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if( pTrack )
	{
		if( m_oldSpeed < 0 )
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetPrevious(), TRUE );
				if( pNext )
					pTrack = pNext;
			} while( pNext );
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetNext(), TRUE );
				if( pNext )
					pTrack = pNext;
			} while( pNext );
		}
	}

	UTIL_SetVelocity( this, g_vecZero );
	UTIL_SetAvelocity(this, g_vecZero );
	if( pTrack )
	{
		ALERT( at_aiconsole, "at %s\n", STRING( pTrack->pev->targetname ) );
		if( pTrack->pev->netname )
			FireTargets( STRING( pTrack->pev->netname ), this, this, USE_TOGGLE, 0 );
	}
	else
		ALERT( at_aiconsole, "\n" );
}

void CFuncTrackTrain::SetControls( entvars_t *pevControls )
{
	Vector offset = pevControls->origin - pev->oldorigin;

	m_controlMins = pevControls->mins + offset;
	m_controlMaxs = pevControls->maxs + offset;
}

BOOL CFuncTrackTrain::OnControls( entvars_t *pevTest )
{
	Vector offset = pevTest->origin - pev->origin;

	if( pev->spawnflags & SF_TRACKTRAIN_NOCONTROL )
		return FALSE;

	// Transform offset into local coordinates
	UTIL_MakeVectors( pev->angles );
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = -DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	if( local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		 local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z )
		 return TRUE;

	return FALSE;
}

void CFuncTrackTrain::Find( void )
{
	m_ppath = CPathTrack::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) ) );
	if( !m_ppath )
		return;

	entvars_t *pevTarget = m_ppath->pev;
	if( !FClassnameIs( pevTarget, "path_track" ) )
	{
		ALERT( at_error, "func_track_train must be on a path of path_track\n" );
		m_ppath = NULL;
		return;
	}

	Vector nextPos = pevTarget->origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead( &look, m_length, 0 );
	look.z += m_height;

	Vector vTemp = UTIL_VecToAngles( look - nextPos );
	vTemp = UTIL_VecToAngles( look - nextPos );
	// The train actually points west
	vTemp.y += 180;

	if( pev->spawnflags & SF_TRACKTRAIN_NOPITCH )
		vTemp.x = 0;

	UTIL_SetAngles(this, vTemp);
	UTIL_AssignOrigin ( this, nextPos );
	NextThink( 0.1f, FALSE );
	SetThink( &CFuncTrackTrain::Next );
	pev->speed = m_startSpeed;

	UpdateSound();
}

void CFuncTrackTrain::NearestPath( void )
{
	CBaseEntity *pTrack = NULL;
	CBaseEntity *pNearest = NULL;
	float dist, closest;

	closest = 1024;

	while( ( pTrack = UTIL_FindEntityInSphere( pTrack, pev->origin, 1024 ) ) != NULL )
	{
		// filter out non-tracks
		if( !( pTrack->pev->flags & ( FL_CLIENT | FL_MONSTER ) ) && FClassnameIs( pTrack->pev, "path_track" ) )
		{
			dist = ( pev->origin - pTrack->pev->origin ).Length();
			if( dist < closest )
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if( !pNearest )
	{
		ALERT( at_console, "Can't find a nearby track !!!\n" );
		SetThink( NULL );
		return;
	}

	ALERT( at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING( pev->targetname ), STRING( pNearest->pev->targetname ) );
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ( (CPathTrack *)pNearest )->GetNext();
	if( pTrack )
	{
		if( ( pev->origin - pTrack->pev->origin ).Length() < ( pev->origin - pNearest->pev->origin ).Length() )
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack *)pNearest;

	if( pev->speed != 0 )
	{
		NextThink( 0.1f, FALSE );
		SetThink( &CFuncTrackTrain::PostponeNext );
	}
}

void CFuncTrackTrain::OverrideReset( void )
{
	NextThink( 0.1f, FALSE );
	SetThink( &CFuncTrackTrain::NearestPath );
}

CFuncTrackTrain *CFuncTrackTrain::Instance( edict_t *pent )
{ 
	if( FClassnameIs( pent, "func_tracktrain" ) )
		return (CFuncTrackTrain *)GET_PRIVATE( pent );
	return NULL;
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal
*/

void CFuncTrackTrain::Spawn( void )
{
	if (!FStringNull(pev->noise))
	{
		m_customMoveSound = TRUE;
	}

	if( pev->speed == 0 )
		m_speed = 100;
	else
		m_speed = pev->speed;

	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->impulse = (int)m_speed;

	m_dir = 1;

	if( FStringNull( pev->target ) )
		ALERT( at_console, "%s with no target\n", STRING(pev->classname) );

	if( pev->spawnflags & SF_TRACKTRAIN_PASSABLE )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	// Cache off placed origin for train controls
	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;

	// start trains on the next frame, to make sure their targets have had
	// a chance to spawn/activate
	NextThink( 0.1, FALSE );
	SetThink( &CFuncTrackTrain::Find );
	Precache();
}

void CFuncTrackTrain::Precache( void )
{
	const char *pszSound;

	if( m_flVolume == 0.0f )
		m_flVolume = 1.0f;

	if (FStringNull( pev->noise ))
	{
		switch( m_sounds )
		{
		default:
			// no sound
			pszSound = NULL;
			break;
		case 1:
			pszSound = "plats/ttrain1.wav";
			break;
		case 2:
			pszSound = "plats/ttrain2.wav";
			break;
		case 3:
			pszSound = "plats/ttrain3.wav";
			break;
		case 4:
			pszSound = "plats/ttrain4.wav";
			break;
		case 5:
			pszSound = "plats/ttrain6.wav";
			break;
		case 6:
			pszSound = "plats/ttrain7.wav";
			break;
		}

		if( pszSound )
		{
			PRECACHE_SOUND( pszSound );
			pev->noise = MAKE_STRING( pszSound );
		}
	}
	else
	{
		PRECACHE_SOUND( STRING(pev->noise) );
	}

	if (FStringNull(pev->noise1))
		pev->noise1 = MAKE_STRING("plats/ttrain_brake1.wav");
	PRECACHE_SOUND( STRING(pev->noise1) );

	if (FStringNull(pev->noise2))
		pev->noise2 = MAKE_STRING("plats/ttrain_start1.wav");
	PRECACHE_SOUND( STRING(pev->noise2) );

	m_usAdjustPitch = PRECACHE_EVENT( 1, "events/train.sc" );
}

// This class defines the volume of space that the player must stand in to control the train
class CFuncTrainControls : public CBaseEntity
{
public:
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Spawn( void );
	void EXPORT Find( void );
};

LINK_ENTITY_TO_CLASS( func_traincontrols, CFuncTrainControls )

void CFuncTrainControls::Find( void )
{
	edict_t *pTarget = NULL;

	do 
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING( pev->target ) );
	} while( !FNullEnt(pTarget) && !FClassnameIs( pTarget, "func_tracktrain" ) );

	if( FNullEnt( pTarget ) )
	{
		ALERT( at_console, "No train %s\n", STRING( pev->target ) );
		return;
	}

	CFuncTrackTrain *ptrain = CFuncTrackTrain::Instance( pTarget );
	ptrain->SetControls( pev );
	UTIL_Remove( this );
}

void CFuncTrainControls::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CFuncTrainControls::Find );
	SetNextThink(0.0f);
}

// ----------------------------------------------------------------------------
//
// Track changer / Train elevator
//
// ----------------------------------------------------------------------------
#define SF_TRACK_ACTIVATETRAIN			0x00000001
#define SF_TRACK_RELINK				0x00000002
#define SF_TRACK_ROTMOVE			0x00000004
#define SF_TRACK_STARTBOTTOM			0x00000008
#define SF_TRACK_DONT_MOVE			0x00000010

//
// This entity is a rotating/moving platform that will carry a train to a new track.
// It must be larger in X-Y planar area than the train, since it must contain the
// train within these dimensions in order to operate when the train is near it.
//
typedef enum
{
	TRAIN_SAFE,
	TRAIN_BLOCKING,
	TRAIN_FOLLOWING
}TRAIN_CODE;

class CFuncTrackChange : public CFuncPlatRot
{
public:
	void Spawn( void );
	void Precache( void );

	//virtual void Blocked( void );
	virtual void EXPORT GoUp( void );
	virtual void EXPORT GoDown( void );

	void KeyValue( KeyValueData* pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT Find( void );
	TRAIN_CODE EvaluateTrain( CPathTrack *pcurrent );
	void UpdateTrain( Vector &dest );
	virtual void HitBottom( void );
	virtual void HitTop( void );
	void Touch( CBaseEntity *pOther );
	virtual void UpdateAutoTargets( int toggleState );
	virtual	BOOL IsTogglePlat( void ) { return TRUE; }

	void DisableUse( void ) { m_use = 0; }
	void EnableUse( void ) { m_use = 1; }
	int UseEnabled( void ) { return m_use; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual void OverrideReset( void );

	CPathTrack *m_trackTop;
	CPathTrack *m_trackBottom;

	CFuncTrackTrain	*m_train;

	string_t m_trackTopName;
	string_t m_trackBottomName;
	string_t m_trainName;
	TRAIN_CODE m_code;
	int m_targetState;
	int m_use;
};

LINK_ENTITY_TO_CLASS( func_trackchange, CFuncTrackChange )

TYPEDESCRIPTION	CFuncTrackChange::m_SaveData[] =
{
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackTop, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackBottom, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_train, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackTopName, FIELD_STRING ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackBottomName, FIELD_STRING ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trainName, FIELD_STRING ),
	DEFINE_FIELD( CFuncTrackChange, m_code, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackChange, m_targetState, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackChange, m_use, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncTrackChange, CFuncPlatRot )

void CFuncTrackChange::Spawn( void )
{
	Setup();
	if( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
		m_vecPosition2.z = pev->origin.z;

	SetupRotation();

	if( FBitSet( pev->spawnflags, SF_TRACK_STARTBOTTOM ) )
	{
		UTIL_SetOrigin( pev, m_vecPosition2 );
		m_toggle_state = TS_AT_BOTTOM;
		pev->angles = m_start;
		m_targetState = TS_AT_TOP;
	}
	else
	{
		UTIL_SetOrigin( pev, m_vecPosition1 );
		m_toggle_state = TS_AT_TOP;
		pev->angles = m_end;
		m_targetState = TS_AT_BOTTOM;
	}

	EnableUse();
	pev->nextthink = pev->ltime + 2.0f;
	SetThink( &CFuncTrackChange::Find );
	Precache();
}

void CFuncTrackChange::Precache( void )
{
	// Can't trigger sound
	PRECACHE_SOUND( "buttons/button11.wav" );

	CFuncPlatRot::Precache();
}

// UNDONE: Filter touches before re-evaluating the train.
void CFuncTrackChange::Touch( CBaseEntity *pOther )
{
#if 0
	TRAIN_CODE code;
	entvars_t *pevToucher = pOther->pev;
#endif
}

void CFuncTrackChange::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "train" ) )
	{
		m_trainName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "toptrack" ) )
	{
		m_trackTopName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bottomtrack" ) )
	{
		m_trackBottomName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CFuncPlatRot::KeyValue( pkvd );		// Pass up to base class
	}
}

void CFuncTrackChange::OverrideReset( void )
{
	pev->nextthink = pev->ltime + 1.0f;
	SetThink( &CFuncTrackChange::Find );
}

void CFuncTrackChange::Find( void )
{
	// Find track entities
	edict_t *target;

	target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_trackTopName ) );
	if( !FNullEnt( target ) )
	{
		m_trackTop = CPathTrack::Instance( target );
		target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_trackBottomName ) );
		if( !FNullEnt( target ) )
		{
			m_trackBottom = CPathTrack::Instance( target );
			target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_trainName ) );
			if( !FNullEnt( target ) )
			{
				m_train = CFuncTrackTrain::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_trainName ) ) );
				if( !m_train )
				{
					ALERT( at_error, "Can't find train for track change! %s\n", STRING( m_trainName ) );
					return;
				}
				Vector center = ( pev->absmin + pev->absmax ) * 0.5f;
				m_trackBottom = m_trackBottom->Nearest( center );
				m_trackTop = m_trackTop->Nearest( center );
				UpdateAutoTargets( m_toggle_state );
				SetThink( NULL );
				return;
			}
			else
			{
				ALERT( at_error, "Can't find train for track change! %s\n", STRING( m_trainName ) );
				// target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_trainName ) );
			}
		}
		else
			ALERT( at_error, "Can't find bottom track for track change! %s\n", STRING( m_trackBottomName ) );
	}
	else
		ALERT( at_error, "Can't find top track for track change! %s\n", STRING( m_trackTopName ) );
}

TRAIN_CODE CFuncTrackChange::EvaluateTrain( CPathTrack *pcurrent )
{
	// Go ahead and work, we don't have anything to switch, so just be an elevator
	if( !pcurrent || !m_train )
		return TRAIN_SAFE;

	if( m_train->m_ppath == pcurrent || ( pcurrent->m_pprevious && m_train->m_ppath == pcurrent->m_pprevious ) ||
		 ( pcurrent->m_pnext && m_train->m_ppath == pcurrent->m_pnext ) )
	{
		if( m_train->pev->speed != 0 )
			return TRAIN_BLOCKING;

		Vector dist = pev->origin - m_train->pev->origin;
		float length = dist.Length2D();
		if( length < m_train->m_length )		// Empirically determined close distance
			return TRAIN_FOLLOWING;
		else if( length > ( 150 + m_train->m_length ) )
			return TRAIN_SAFE;

		return TRAIN_BLOCKING;
	}

	return TRAIN_SAFE;
}

void CFuncTrackChange::UpdateTrain( Vector &dest )
{
	float time = ( pev->nextthink - pev->ltime );

	m_train->pev->velocity = pev->velocity;
	m_train->pev->avelocity = pev->avelocity;
	m_train->NextThink( m_train->pev->ltime + time, FALSE );

	// Attempt at getting the train to rotate properly around the origin of the trackchange
	if( time <= 0 )
		return;

	Vector offset = m_train->pev->origin - pev->origin;
	Vector delta = dest - pev->angles;
	// Transform offset into local coordinates
	UTIL_MakeInvVectors( delta, gpGlobals );
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	local = local - offset;
	m_train->pev->velocity = pev->velocity + ( local * ( 1.0f / time ) );
}

void CFuncTrackChange::GoDown( void )
{
	if( m_code == TRAIN_BLOCKING )
		return;

	// HitBottom may get called during CFuncPlat::GoDown(), so set up for that
	// before you call GoDown()

	UpdateAutoTargets( TS_GOING_DOWN );
	// If ROTMOVE, move & rotate
	if( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		SetMoveDone( &CFuncPlat::CallHitBottom );
		m_toggle_state = TS_GOING_DOWN;
		AngularMove( m_start, pev->speed );
	}
	else
	{
		CFuncPlat::GoDown();
		SetMoveDone( &CFuncPlat::CallHitBottom );
		RotMove( m_start, pev->nextthink - pev->ltime );
	}
	// Otherwise, rotate first, move second

	// If the train is moving with the platform, update it
	if( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_start );
		m_train->m_ppath = NULL;
	}
}

//
// Platform is at bottom, now starts moving up
//
void CFuncTrackChange::GoUp( void )
{
	if( m_code == TRAIN_BLOCKING )
		return;

	// HitTop may get called during CFuncPlat::GoUp(), so set up for that
	// before you call GoUp();
	UpdateAutoTargets( TS_GOING_UP );
	if( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		m_toggle_state = TS_GOING_UP;
		SetMoveDone( &CFuncPlat::CallHitTop );
		AngularMove( m_end, pev->speed );
	}
	else
	{
		// If ROTMOVE, move & rotate
		CFuncPlat::GoUp();
		SetMoveDone( &CFuncPlat::CallHitTop );
		RotMove( m_end, pev->nextthink - pev->ltime );
	}

	// Otherwise, move first, rotate second

	// If the train is moving with the platform, update it
	if( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_end );
		m_train->m_ppath = NULL;
	}
}

// Normal track change
void CFuncTrackChange::UpdateAutoTargets( int toggleState )
{
	if( !m_trackTop || !m_trackBottom )
		return;

	if( toggleState == TS_AT_TOP )
		ClearBits( m_trackTop->pev->spawnflags, SF_PATH_DISABLED );
	else
		SetBits( m_trackTop->pev->spawnflags, SF_PATH_DISABLED );

	if( toggleState == TS_AT_BOTTOM )
		ClearBits( m_trackBottom->pev->spawnflags, SF_PATH_DISABLED );
	else
		SetBits( m_trackBottom->pev->spawnflags, SF_PATH_DISABLED );
}

void CFuncTrackChange::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( m_toggle_state != TS_AT_TOP && m_toggle_state != TS_AT_BOTTOM )
		return;

	// If train is in "safe" area, but not on the elevator, play alarm sound
	if( m_toggle_state == TS_AT_TOP )
		m_code = EvaluateTrain( m_trackTop );
	else if( m_toggle_state == TS_AT_BOTTOM )
		m_code = EvaluateTrain( m_trackBottom );
	else
		m_code = TRAIN_BLOCKING;
	if( m_code == TRAIN_BLOCKING )
	{
		// Play alarm and return
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "buttons/button11.wav", 1, ATTN_NORM );
		return;
	}

	// Otherwise, it's safe to move
	// If at top, go down
	// at bottom, go up
	DisableUse();
	if( m_toggle_state == TS_AT_TOP )
		GoDown();
	else
		GoUp();
}

//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange::HitBottom( void )
{
	CFuncPlatRot::HitBottom();
	if( m_code == TRAIN_FOLLOWING )
	{
		//UpdateTrain();
		m_train->SetTrack( m_trackBottom );
	}
	SetThink( NULL );
	pev->nextthink = -1;

	UpdateAutoTargets( m_toggle_state );

	EnableUse();
}

//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange::HitTop( void )
{
	CFuncPlatRot::HitTop();
	if( m_code == TRAIN_FOLLOWING )
	{
		//UpdateTrain();
		m_train->SetTrack( m_trackTop );
	}

	// Don't let the plat go back down
	SetThink( NULL );
	pev->nextthink = -1;
	UpdateAutoTargets( m_toggle_state );
	EnableUse();
}

class CFuncTrackAuto : public CFuncTrackChange
{
public:
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	UpdateAutoTargets( int toggleState );
};

LINK_ENTITY_TO_CLASS( func_trackautochange, CFuncTrackAuto )

// Auto track change
void CFuncTrackAuto::UpdateAutoTargets( int toggleState )
{
	CPathTrack *pTarget, *pNextTarget;

	if( !m_trackTop || !m_trackBottom )
		return;

	if( m_targetState == TS_AT_TOP )
	{
		pTarget = m_trackTop->GetNext();
		pNextTarget = m_trackBottom->GetNext();
	}
	else
	{
		pTarget = m_trackBottom->GetNext();
		pNextTarget = m_trackTop->GetNext();
	}
	if( pTarget )
	{
		ClearBits( pTarget->pev->spawnflags, SF_PATH_DISABLED );
		if( m_code == TRAIN_FOLLOWING && m_train && m_train->pev->speed == 0 )
			m_train->Use( this, this, USE_ON, 0 );
	}

	if( pNextTarget )
		SetBits( pNextTarget->pev->spawnflags, SF_PATH_DISABLED );
}

void CFuncTrackAuto::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CPathTrack *pTarget;

	if( !UseEnabled() )
		return;

	if( m_toggle_state == TS_AT_TOP )
		pTarget = m_trackTop;
	else if( m_toggle_state == TS_AT_BOTTOM )
		pTarget = m_trackBottom;
	else
		pTarget = NULL;

	if( FClassnameIs( pActivator->pev, "func_tracktrain" ) )
	{
		m_code = EvaluateTrain( pTarget );

		// Safe to fire?
		if( m_code == TRAIN_FOLLOWING && m_toggle_state != m_targetState )
		{
			DisableUse();
			if( m_toggle_state == TS_AT_TOP )
				GoDown();
			else
				GoUp();
		}
	}
	else
	{
		if( pTarget )
			pTarget = pTarget->GetNext();
		if( pTarget && m_train->m_ppath != pTarget && ShouldToggle( useType, m_targetState ) )
		{
			if( m_targetState == TS_AT_TOP )
				m_targetState = TS_AT_BOTTOM;
			else
				m_targetState = TS_AT_TOP;
		}

		UpdateAutoTargets( m_targetState );
	}
}

// ----------------------------------------------------------
//
//
// pev->speed is the travel speed
// pev->health is current health
// pev->max_health is the amount to reset to each time it starts

#define FGUNTARGET_START_ON			0x0001

class CGunTarget : public CBaseMonster
{
public:
	void		Spawn( void );
	void		Activate( void );
	void EXPORT	Next( void );
	void EXPORT	Start( void );
	void EXPORT	Wait( void );
	void		Stop( void );

	int		BloodColor( void ) { return DONT_BLEED; }
	int		Classify( void ) { return CLASS_MACHINE; }
	int		TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	Vector		BodyTarget( const Vector &posSrc ) { return pev->origin; }

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

private:
	BOOL m_on;
};

LINK_ENTITY_TO_CLASS( func_guntarget, CGunTarget )

TYPEDESCRIPTION	CGunTarget::m_SaveData[] = 
{
	DEFINE_FIELD( CGunTarget, m_on, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CGunTarget, CBaseMonster )

void CGunTarget::Spawn( void )
{
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->speed == 0 )
		pev->speed = 100;

	// Don't take damage until "on"
	pev->takedamage = DAMAGE_NO;
	pev->flags |= FL_MONSTER;

	m_on = FALSE;
	pev->max_health = pev->health;

	if( pev->spawnflags & FGUNTARGET_START_ON )
	{
		SetThink( &CGunTarget::Start );
		SetNextThink( 0.3f );
	}
}

void CGunTarget::Activate( void )
{
	CBaseEntity *pTarg;

	// now find our next target
	pTarg = GetNextTarget();
	if( pTarg )
	{
		m_hTargetEnt = pTarg;
		UTIL_SetOrigin( pev, pTarg->pev->origin - ( pev->mins + pev->maxs ) * 0.5f );
	}
}

void CGunTarget::Start( void )
{
	Use( this, this, USE_ON, 0 );
}

void CGunTarget::Next( void )
{
	SetThink( NULL );

	m_hTargetEnt = GetNextTarget();
	CBaseEntity *pTarget = m_hTargetEnt;

	if( !pTarget )
	{
		Stop();
		return;
	}
	SetMoveDone( &CGunTarget::Wait );
	LinearMove( pTarget->pev->origin - ( pev->mins + pev->maxs ) * 0.5f, pev->speed );
}

void CGunTarget::Wait( void )
{
	CBaseEntity *pTarget = m_hTargetEnt;

	if( !pTarget )
	{
		Stop();
		return;
	}

	// Fire the pass target if there is one
	if( pTarget->pev->message )
	{
		FireTargets( STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0 );
		if( FBitSet( pTarget->pev->spawnflags, SF_CORNER_FIREONCE ) )
			pTarget->pev->message = 0;
	}

	m_flWait = pTarget->GetDelay();

	pev->target = pTarget->pev->target;
	SetThink( &CGunTarget::Next );
	if( m_flWait != 0 )
	{
		// -1 wait will wait forever!		
		SetNextThink( m_flWait );
	}
	else
	{
		Next();// do it RIGHT now!
	}
}

void CGunTarget::Stop( void )
{
	pev->velocity = g_vecZero;
	DontThink();
	pev->takedamage = DAMAGE_NO;
}

int CGunTarget::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( pev->health > 0 )
	{
		pev->health -= flDamage;
		if( pev->health <= 0 )
		{
			pev->health = 0;
			Stop();
			if( pev->message )
				FireTargets( STRING( pev->message ), this, this, USE_TOGGLE, 0 );
		}
	}
	return 0;
}

void CGunTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_on ) )
		return;

	if( m_on )
	{
		Stop();
	}
	else
	{
		pev->takedamage = DAMAGE_AIM;
		m_hTargetEnt = GetNextTarget();
		if( m_hTargetEnt == 0 )
			return;
		pev->health = pev->max_health;
		Next();
	}
}

class CTrainThinker : public CBaseEntity
{
	void Spawn()
	{
		pev->effects = EF_NODRAW;
		pev->nextthink = gpGlobals->time;
		if (pev->owner)
			SetThink( &CTrainThinker::TrainThink );
		else
			SetThink( &CBaseEntity::SUB_Remove );
	}
	void EXPORT TrainThink()
	{
		if (pev->owner)
		{
			CBaseEntity* owner = CBaseEntity::Instance(pev->owner);
			if (owner)
			{
				CFuncTrain* train = (CFuncTrain*)owner;
				train->TrainThink();
				pev->nextthink = gpGlobals->time + 0.1;
			}
			else
			{
				SetThink( &CBaseEntity::SUB_Remove );
				pev->nextthink = gpGlobals->time + 0.1;
			}
		}
	}
};

LINK_ENTITY_TO_CLASS(trainthinker, CTrainThinker)

class CSpriteTrain : public CFuncTrain
{
public:

	void Spawn( void );
	void Precache( void );
	void Animate( float frames );
	void TrainThink();
	float m_maxFrame;
	float m_lastTime;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];
};

TYPEDESCRIPTION	CSpriteTrain::m_SaveData[] =
{
	DEFINE_FIELD( CSpriteTrain, m_maxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CSpriteTrain, m_lastTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSpriteTrain, CFuncTrain )

LINK_ENTITY_TO_CLASS(env_spritetrain, CSpriteTrain)

void CSpriteTrain::Spawn(void)
{
	Precache();

	SetDefaultTrainValues();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;
	pev->effects = 0;

	if (FStringNull(pev->model))
	{
		ALERT(at_console, "Spawning %s without model!\n", STRING(pev->classname));
	}
	else
	{
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	}

	if (!pev->rendermode)
		pev->rendermode = kRenderTransAdd;
	if (!pev->renderamt)
		pev->renderamt = 255;
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	m_lastTime = gpGlobals->time;
	pev->frame = 0;

	UTIL_SetOrigin( pev, pev->origin );

	Create("trainthinker", pev->origin, pev->angles, edict());
}

void CSpriteTrain::Precache(void)
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL( STRING( pev->model ) );
	if (!FStringNull(pev->noiseMovement))
		PRECACHE_SOUND( STRING(pev->noiseMovement) );
	if (!FStringNull(pev->noiseStopMoving))
		PRECACHE_SOUND( STRING(pev->noiseStopMoving) );
}

void CSpriteTrain::Animate( float frames )
{
	if( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame + frames, m_maxFrame );
}

void CSpriteTrain::TrainThink()
{
	if( pev->framerate && m_maxFrame > 1.0 )
	{
		Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );
		m_lastTime = gpGlobals->time;
	}
}

class CModelTrain : public CFuncTrain
{
public:
	void Spawn( void );
	void Precache( void );
	void AdvanceAnimation(void);
	void Next( void );
	void TrainThink();
};

LINK_ENTITY_TO_CLASS(env_modeltrain, CModelTrain)

void CModelTrain::Spawn( void )
{
	Precache();

	SetDefaultTrainValues();

	pev->movetype = MOVETYPE_PUSH;

	pev->solid = SOLID_BBOX;

	if (FStringNull(pev->model))
	{
		ALERT(at_console, "Spawning %s without model!\n", STRING(pev->classname));
	}
	else
	{
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	}
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	pev->sequence = 0;
	ResetSequenceInfo();

	Create("trainthinker", pev->origin, pev->angles, edict());
}

void CModelTrain::Precache(void)
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL( STRING( pev->model ) );
	if (!FStringNull(pev->noiseMovement))
		PRECACHE_SOUND( STRING(pev->noiseMovement) );
	if (!FStringNull(pev->noiseStopMoving))
		PRECACHE_SOUND( STRING(pev->noiseStopMoving) );
}

void CModelTrain::AdvanceAnimation(void)
{
	// Advance frames and dispatch events.
	StudioFrameAdvance();
	DispatchAnimEvents();

	// Restart sequence
	if (m_fSequenceFinished)
	{
		pev->frame = 0;
		ResetSequenceInfo();

		if (!m_fSequenceLoops)
		{
			m_fSequenceFinished = TRUE;
			return;
		}
		else
		{
			pev->frame = 0;
			ResetSequenceInfo();
		}
	}
}

void CModelTrain::Next( void )
{
	CFuncTrain::Next();
	if (m_pevCurrentTarget)
		pev->angles = UTIL_VecToAngles(m_pevCurrentTarget->origin - pev->origin);
}

void CModelTrain::TrainThink()
{
	AdvanceAnimation();
}
