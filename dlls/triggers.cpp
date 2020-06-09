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

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"			// trigger_camera has train functionality
#include "gamerules.h"
#include "skill.h"
#include "monsters.h"
#include "followingmonster.h"
#include "locus.h"

#define FEATURE_TRIGGER_RANDOM 1
#define FEATURE_TRIGGER_RESPAWN 0
#define FEATURE_TRIGGER_KILL_MONSTER 0
#define FEATURE_TRIGGER_TIMER 0
#define FEATURE_TRIGGER_CHANGE_VALUE 0
#define FEATURE_TRIGGER_COMMAND 0
#define FEATURE_TRIGGER_MOTION 0
#define FEATURE_TRIGGER_HURT_REMOTE 0
#define FEATURE_TRIGGER_ENTITY_ITERATOR 0

#define SF_TRIGGER_PUSH_ONCE		1
#define SF_TRIGGER_PUSH_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_PUSH_NO_CLIENTS	8
#define SF_TRIGGER_PUSH_NO_MONSTERS	16

#define SF_TRIGGER_HURT_TARGETONCE	1// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF	2//spawnflag that makes trigger_hurt spawn turned OFF
#define	SF_TRIGGER_HURT_NO_CLIENTS	8//don't trigger on clients
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32// only clients may touch this trigger.

extern DLL_GLOBAL BOOL		g_fGameOver;

extern void SetMovedir(entvars_t* pev);
extern Vector VecBModelOrigin( entvars_t* pevBModel );

class CFrictionModifier : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ChangeFriction( CBaseEntity *pOther );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static TYPEDESCRIPTION m_SaveData[];

	float m_frictionFraction;		// Sorry, couldn't resist this name :)
};

LINK_ENTITY_TO_CLASS( func_friction, CFrictionModifier )

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CFrictionModifier::m_SaveData[] =
{
	DEFINE_FIELD( CFrictionModifier, m_frictionFraction, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CFrictionModifier, CBaseEntity )

// Modify an entity's friction
void CFrictionModifier::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch( &CFrictionModifier::ChangeFriction );
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::ChangeFriction( CBaseEntity *pOther )
{
	if( pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE )
		pOther->pev->friction = m_frictionFraction;
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "modifier" ) )
	{
		m_frictionFraction = atof( pkvd->szValue ) / 100.0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE		0x0001

class CAutoTrigger : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void Think( void );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_globalstate;
	USE_TYPE triggerType;
};

LINK_ENTITY_TO_CLASS( trigger_auto, CAutoTrigger )

TYPEDESCRIPTION	CAutoTrigger::m_SaveData[] =
{
	DEFINE_FIELD( CAutoTrigger, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CAutoTrigger, triggerType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAutoTrigger, CBaseDelay )

void CAutoTrigger::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "globalstate" ) )
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CAutoTrigger::Spawn( void )
{
	Precache();
}

void CAutoTrigger::Precache( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CAutoTrigger::Think( void )
{
	if( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON )
	{
		SUB_UseTargets( this, triggerType, 0 );
		if( pev->spawnflags & SF_AUTO_FIREONCE )
			UTIL_Remove( this );
	}
}

#define SF_RELAY_FIREONCE		0x0001
#define SF_RELAY_FORWARDACTIVATOR		0x0040

class CTriggerRelay : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

private:
	USE_TYPE triggerType;
};

LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay )

TYPEDESCRIPTION	CTriggerRelay::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerRelay, triggerType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerRelay, CBaseDelay )

void CTriggerRelay::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerRelay::Spawn( void )
{
}

void CTriggerRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SUB_UseTargets( FBitSet(pev->spawnflags, SF_RELAY_FORWARDACTIVATOR) ? pActivator : this, triggerType, 0 );
	if( pev->spawnflags & SF_RELAY_FIREONCE )
		UTIL_Remove( this );
}

//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE		0x80000000
#define SF_MULTIMAN_THREAD		0x00000001

class CMultiManager : public CBaseToggle
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void EXPORT ManagerThink( void );
	void EXPORT ManagerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

#if _DEBUG
	void EXPORT ManagerReport( void );
#endif
	BOOL HasTarget( string_t targetname );

	int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	int m_cTargets; // the total number of targets in this manager's fire list.
	int m_index;	// Current target
	float m_startTime;// Time we started firing
	string_t m_iTargetName[MAX_MULTI_TARGETS];// list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS];// delay (in seconds) from time of manager fire to target fire
private:
	inline BOOL IsClone( void ) { return ( pev->spawnflags & SF_MULTIMAN_CLONE ) ? TRUE : FALSE; }
	inline BOOL ShouldClone( void )
	{
		if( IsClone() )
			return FALSE;

		return ( pev->spawnflags & SF_MULTIMAN_THREAD ) ? TRUE : FALSE;
	}

	CMultiManager *Clone( void );
};

LINK_ENTITY_TO_CLASS( multi_manager, CMultiManager )

// Global Savedata for multi_manager
TYPEDESCRIPTION	CMultiManager::m_SaveData[] =
{
	DEFINE_FIELD( CMultiManager, m_cTargets, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_index, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_startTime, FIELD_TIME ),
	DEFINE_ARRAY( CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS ),
};

IMPLEMENT_SAVERESTORE( CMultiManager, CBaseToggle )

void CMultiManager::KeyValue( KeyValueData *pkvd )
{
	// UNDONE: Maybe this should do something like this:
	//CBaseToggle::KeyValue( pkvd );
	// if( !pkvd->fHandled )
	// ... etc.

	if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if( m_cTargets < MAX_MULTI_TARGETS )
		{
			char tmp[128];

			UTIL_StripToken( pkvd->szKeyName, tmp );
			m_iTargetName[m_cTargets] = ALLOC_STRING( tmp );
			m_flTargetDelay[m_cTargets] = atof( pkvd->szValue );
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
	}
}

void CMultiManager::Spawn( void )
{
	pev->solid = SOLID_NOT;
	SetUse( &CMultiManager::ManagerUse );
	SetThink( &CMultiManager::ManagerThink );

	// Sort targets
	// Quick and dirty bubble sort
	int swapped = 1;

	while( swapped )
	{
		swapped = 0;
		for( int i = 1; i < m_cTargets; i++ )
		{
			if( m_flTargetDelay[i] < m_flTargetDelay[i - 1] )
			{
				// Swap out of order elements
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i - 1];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_iTargetName[i - 1] = name;
				m_flTargetDelay[i - 1] = delay;
				swapped = 1;
			}
		}
	}
}

BOOL CMultiManager::HasTarget( string_t targetname )
{
	for( int i = 0; i < m_cTargets; i++ )
		if( FStrEq( STRING( targetname ), STRING( m_iTargetName[i] ) ) )
			return TRUE;

	return FALSE;
}

// Designers were using this to fire targets that may or may not exist --
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager::ManagerThink( void )
{
	float time;

	time = gpGlobals->time - m_startTime;
	while( m_index < m_cTargets && m_flTargetDelay[m_index] <= time )
	{
		FireTargets( STRING( m_iTargetName[m_index] ), m_hActivator, this, USE_TOGGLE, 0 );
		m_index++;
	}

	if( m_index >= m_cTargets )// have we fired all targets?
	{
		SetThink( NULL );
		if( IsClone() )
		{
			UTIL_Remove( this );
			return;
		}
		SetUse( &CMultiManager::ManagerUse );// allow manager re-use
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[m_index];
}

CMultiManager *CMultiManager::Clone( void )
{
	CMultiManager *pMulti = GetClassPtr( (CMultiManager *)NULL );

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy( pMulti->pev, pev, sizeof(*pev) );
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy( pMulti->m_iTargetName, m_iTargetName, sizeof( m_iTargetName ) );
	memcpy( pMulti->m_flTargetDelay, m_flTargetDelay, sizeof( m_flTargetDelay ) );

	return pMulti;
}

// The USE function builds the time table and starts the entity thinking.
void CMultiManager::ManagerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if( ShouldClone() )
	{
		CMultiManager *pClone = Clone();
		pClone->ManagerUse( pActivator, pCaller, useType, value );
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse( NULL );// disable use until all targets have fired

	SetThink( &CMultiManager::ManagerThink );
	pev->nextthink = gpGlobals->time;
}

#if _DEBUG
void CMultiManager::ManagerReport( void )
{
	int cIndex;

	for( cIndex = 0; cIndex < m_cTargets; cIndex++ )
	{
		ALERT( at_console, "%s %f\n", STRING( m_iTargetName[cIndex] ), m_flTargetDelay[cIndex] );
	}
}
#endif

class CMultiTrigger : public CMultiManager
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( multi_trigger, CMultiTrigger )

void CMultiTrigger::KeyValue( KeyValueData *pkvd )
{
	int num, index;
	if ( strncmp(pkvd->szKeyName, "target", 6) == 0 && isdigit(pkvd->szKeyName[6]))
	{
		num = atoi(pkvd->szKeyName + 6);
		if (num <= 0 || num > MAX_MULTI_TARGETS)
			return;

		index = num - 1;
		m_iTargetName[index] = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( strncmp(pkvd->szKeyName, "delay", 5) == 0 && isdigit(pkvd->szKeyName[5]))
	{
		num = atoi(pkvd->szKeyName + 5);
		if (num <= 0 || num > MAX_MULTI_TARGETS)
			return;
		index = num - 1;
		m_flTargetDelay[index] = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
}

void CMultiTrigger::Spawn( void )
{
	for (int i=0; i<MAX_MULTI_TARGETS; ++i)
	{
		if (!m_iTargetName[i])
		{
			for (int j=i+1; j<MAX_MULTI_TARGETS; ++j)
			{
				if (m_iTargetName[j])
				{
					m_iTargetName[i] = m_iTargetName[j];
					m_flTargetDelay[i] = m_flTargetDelay[j];
					m_iTargetName[j] = 0;
					break;
				}
			}
		}
		if (m_iTargetName[i])
			m_cTargets = i+1;
		else
			break;
	}
	CMultiManager::Spawn();
	/* Set multi_manager classname so multisource will check this entity too
	 * This should be ok, since save/restore data is the same.
	 */
	pev->classname = MAKE_STRING("multi_manager");
}

//***********************************************************
//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//

// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX	( 1 << 0 )
#define SF_RENDER_MASKAMT	( 1 << 1 )
#define SF_RENDER_MASKMODE	( 1 << 2 )
#define SF_RENDER_MASKCOLOR	( 1 << 3 )

class CRenderFxManager : public CBaseEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_render, CRenderFxManager )

void CRenderFxManager::Spawn( void )
{
	pev->solid = SOLID_NOT;
}

void CRenderFxManager::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !FStringNull( pev->target ) )
	{
		edict_t *pentTarget = NULL;
		while( 1 )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->target ) );
			if( FNullEnt( pentTarget ) )
				break;

			entvars_t *pevTarget = VARS( pentTarget );
			if( !FBitSet( pev->spawnflags, SF_RENDER_MASKFX ) )
				pevTarget->renderfx = pev->renderfx;
			if( !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
				pevTarget->renderamt = pev->renderamt;
			if( !FBitSet( pev->spawnflags, SF_RENDER_MASKMODE ) )
				pevTarget->rendermode = pev->rendermode;
			if( !FBitSet( pev->spawnflags, SF_RENDER_MASKCOLOR ) )
				pevTarget->rendercolor = pev->rendercolor;
		}
	}
}

class CBaseTrigger : public CBaseToggle
{
public:
	void KeyValue( KeyValueData *pkvd );
	void EXPORT MultiTouch( CBaseEntity *pOther );
	void EXPORT HurtTouch( CBaseEntity *pOther );
	void EXPORT CDAudioTouch( CBaseEntity *pOther );
	void ActivateMultiTrigger( CBaseEntity *pActivator );
	void EXPORT MultiWaitOver( void );
	void EXPORT CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void InitTrigger( void );
	bool CanTouch( entvars_t *pevToucher );

	virtual int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger )

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger()
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if( pev->angles != g_vecZero )
		SetMovedir( pev );
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );    // set size and link into world
	if( CVAR_GET_FLOAT( "showtriggers" ) == 0 )
		SetBits( pev->effects, EF_NODRAW );
}

bool CBaseTrigger::CanTouch(entvars_t *pevToucher)
{
	if ( FStringNull(pev->netname) )
	{
		// Only touch clients, monsters, or pushables (depending on flags)
		if (pevToucher->flags & FL_CLIENT)
			return !(pev->spawnflags & SF_TRIGGER_NOCLIENTS);
		else if (pevToucher->flags & FL_MONSTER)
			return pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS;
		else if (FClassnameIs(pevToucher,"func_pushable"))
			return pev->spawnflags & SF_TRIGGER_PUSHABLES;
		else
			return pev->spawnflags & SF_TRIGGER_EVERYTHING;
	}
	else
	{
		// If netname is set, it's an entity-specific trigger; we ignore the spawnflags.
		if (!FClassnameIs(pevToucher, STRING(pev->netname)) &&
			(!pevToucher->targetname || !FStrEq(STRING(pevToucher->targetname), STRING(pev->netname))))
			return false;
	}
	return true;
}

//
// Cache user-entity-field values until spawn is called.
//
void CBaseTrigger::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "count" ) )
	{
		m_cTriggersLeft = (int)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "damagetype" ) )
	{
		m_bitsDamageInflict = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT RadiationThink( void );
};

LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt )

//
// trigger_monsterjump
//
class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void Think( void );
};

LINK_ENTITY_TO_CLASS( trigger_monsterjump, CTriggerMonsterJump )

void CTriggerMonsterJump::Spawn( void )
{
	SetMovedir( pev );

	InitTrigger();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if( !FStringNull( pev->targetname ) )
	{
		// if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin( pev, pev->origin ); // Unlink from trigger list
		SetUse( &CBaseTrigger::ToggleUse );
	}
}

void CTriggerMonsterJump::Think( void )
{
	pev->solid = SOLID_NOT;// kill the trigger for now !!!UNDONE
	UTIL_SetOrigin( pev, pev->origin ); // Unlink from trigger list
	SetThink( NULL );
}

void CTriggerMonsterJump::Touch( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	if( !FBitSet( pevOther->flags, FL_MONSTER ) )
	{
		// touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1.0f;

	if( FBitSet( pevOther->flags, FL_ONGROUND ) )
	{
		// clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}

//=====================================
//
// trigger_cdaudio - starts/stops cd audio tracks
//
class CTriggerCDAudio : public CBaseTrigger
{
public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void PlayTrack( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_cdaudio, CTriggerCDAudio )

//
// Changes tracks or stops CD when player touches
//
// !!!HACK - overloaded HEALTH to avoid adding new field
void CTriggerCDAudio::Touch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() )
	{
		// only clients may trigger these events
		return;
	}

	PlayTrack();
}

void CTriggerCDAudio::Spawn( void )
{
	InitTrigger();
}

void CTriggerCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	PlayTrack();
}

void PlayCDTrack( int iTrack )
{
	edict_t *pClient;

	// manually find the single player.
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	// Can't play if the client is not connected!
	if( !pClient )
		return;

	if( iTrack < -1 || iTrack > 30 )
	{
		ALERT( at_console, "TriggerCDAudio - Track %d out of range\n" );
		return;
	}

	if( iTrack == -1 )
	{
		CLIENT_COMMAND( pClient, "cd stop\n" );
	}
	else
	{
		char string[64];

		sprintf( string, "cd play %3d\n", iTrack );
		CLIENT_COMMAND( pClient, string );
	}
}

// only plays for ONE client, so only use in single play!
void CTriggerCDAudio::PlayTrack( void )
{
	PlayCDTrack( (int)pev->health );

	SetTouch( NULL );
	UTIL_Remove( this );
}

// This plays a CD track when fired or when the player enters it's radius
class CTargetCDAudio : public CPointEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	void Play( void );
};

LINK_ENTITY_TO_CLASS( target_cdaudio, CTargetCDAudio )

void CTargetCDAudio::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		pev->scale = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTargetCDAudio::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if( pev->scale > 0 )
		pev->nextthink = gpGlobals->time + 1.0f;
}

void CTargetCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Play();
}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::Think( void )
{
	edict_t *pClient;

	// manually find the single player.
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	// Can't play if the client is not connected!
	if( !pClient )
		return;

	pev->nextthink = gpGlobals->time + 0.5f;

	if( ( pClient->v.origin - pev->origin ).Length() <= pev->scale )
		Play();
}

void CTargetCDAudio::Play( void )
{
	PlayCDTrack( (int)pev->health );
	UTIL_Remove( this );
}

//=====================================
//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

void CTriggerHurt::Spawn( void )
{
	InitTrigger();
	SetTouch( &CBaseTrigger::HurtTouch );

	if( !FStringNull( pev->targetname ) )
	{
		SetUse( &CBaseTrigger::ToggleUse );
	}
	else
	{
		SetUse( NULL );
	}

	if( m_bitsDamageInflict & DMG_RADIATION )
	{
		SetThink( &CTriggerHurt::RadiationThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.0, 0.5 );
	}

	if( FBitSet( pev->spawnflags, SF_TRIGGER_HURT_START_OFF ) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin( pev, pev->origin );		// Link into the list
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger
void CTriggerHurt::RadiationThink( void )
{
	edict_t *pentPlayer;
	CBasePlayer *pPlayer = NULL;
	float flRange;
	entvars_t *pevTarget;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = ( pev->absmin + pev->absmax ) * 0.5f;
	pev->view_ofs = pev->view_ofs * 0.0f;

	pentPlayer = FIND_CLIENT_IN_PVS( edict() );

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin
	if( !FNullEnt( pentPlayer ) )
	{
		pPlayer = GetClassPtr( (CBasePlayer *)VARS( pentPlayer ) );

		pevTarget = VARS( pentPlayer );

		// get range to player;
		vecSpot1 = ( pev->absmin + pev->absmax ) * 0.5f;
		vecSpot2 = ( pevTarget->absmin + pevTarget->absmax ) * 0.5f;

		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length();

		// if player's current geiger counter range is larger
		// than range to this trigger hurt, reset player's
		// geiger counter range

		if( pPlayer->m_flgeigerRange >= flRange )
			pPlayer->m_flgeigerRange = flRange;
	}

	pev->nextthink = gpGlobals->time + 0.25f;
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CBaseTrigger::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->solid == SOLID_NOT )
	{
		// if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;

		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{
		// turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin( pev, pev->origin );
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger::HurtTouch( CBaseEntity *pOther )
{
	float fldmg;

	if( !pOther->pev->takedamage )
		return;

	if( ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH ) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if( ( pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS ) && pOther->IsPlayer() )
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if( g_pGameRules->IsMultiplayer() )
	{
		if( pev->dmgtime > gpGlobals->time )
		{
			if( gpGlobals->time != pev->pain_finished )
			{
				// too early to hurt again, and not same frame with a different entity
				if( pOther->IsPlayer() )
				{
					int playerMask = 1 << ( pOther->entindex() - 1 );

					// If I've already touched this player (this time), then bail out
					if( pev->impulse & playerMask )
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if( pOther->IsPlayer() )
			{
				int playerMask = 1 << ( pOther->entindex() - 1 );

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else	// Original code -- single player
	{
		if( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
		{
			// too early to hurt again, and not same frame with a different entity
			return;
		}
	}

	// If this is time_based damage (poison, radiation), override the pev->dmg with a
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	fldmg = pev->dmg * 0.5f;	// 0.5 seconds worth of damage, pev->dmg is damage/second

	// JAY: Cut this because it wasn't fully realized.  Damage is simpler now.
#if 0
	switch( m_bitsDamageInflict )
	{
	default:
		break;
	case DMG_POISON:
		fldmg = POISON_DAMAGE / 4;
		break;
	case DMG_NERVEGAS:
		fldmg = NERVEGAS_DAMAGE / 4;
		break;
	case DMG_RADIATION:
		fldmg = RADIATION_DAMAGE / 4;
		break;
	case DMG_PARALYZE: // UNDONE: cut this? should slow movement to 50%
		fldmg = PARALYZE_DAMAGE / 4;
		break;
	case DMG_ACID:
		fldmg = ACID_DAMAGE / 4;
		break;
	case DMG_SLOWBURN:
		fldmg = SLOWBURN_DAMAGE / 4;
		break;
	case DMG_SLOWFREEZE:
		fldmg = SLOWFREEZE_DAMAGE / 4;
		break;
	}
#endif
	if( fldmg < 0 )
		pOther->TakeHealth( this, -fldmg, m_bitsDamageInflict );
	else
		pOther->TakeDamage( pev, pev, fldmg, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5f;// half second delay until this trigger can hurt toucher again

	if( pev->target )
	{
		// trigger has a target it wants to fire.
		if( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE )
		{
			// if the toucher isn't a client, don't fire the target!
			if( !pOther->IsPlayer() )
			{
				return;
			}
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}

// trigger_multiple
class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple )

void CTriggerMultiple::Spawn( void )
{
	if( m_flWait == 0 )
		m_flWait = 0.2f;

	InitTrigger();

	ASSERTSZ( pev->health == 0, "trigger_multiple with health" );
	SetTouch( &CBaseTrigger::MultiTouch );
}

/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)      secret
2)      beep beep
3)      large switch
4)
*/
class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce )

void CTriggerOnce::Spawn( void )
{
	m_flWait = -1;

	CTriggerMultiple::Spawn();
}

void CBaseTrigger::MultiTouch( CBaseEntity *pOther )
{
	entvars_t *pevToucher;

	pevToucher = pOther->pev;

	// Only touch clients, monsters, or pushables (depending on flags)
	if( CanTouch(pevToucher) )
	{

#if 0
		// if the trigger has an angles field, check player's facing direction
		if( pev->movedir != g_vecZero )
		{
			UTIL_MakeVectors( pevToucher->angles );
			if( DotProduct( gpGlobals->v_forward, pev->movedir ) < 0 )
				return;         // not facing the right way
		}
#endif
		ActivateMultiTrigger( pOther );
	}
}

//
// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
//
void CBaseTrigger::ActivateMultiTrigger( CBaseEntity *pActivator )
{
	if( pev->nextthink > gpGlobals->time )
		return;         // still waiting for reset time

	if( !UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
		return;

	if( FClassnameIs( pev, "trigger_secret" ) )
	{
		if( pev->enemy == NULL || !FClassnameIs( pev->enemy, "player" ) )
			return;
		gpGlobals->found_secrets++;
	}

	if( !FStringNull( pev->noise ) )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM );

	// don't trigger again until reset
	// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );

	if( pev->message && pActivator->IsPlayer() )
	{
		UTIL_ShowMessage( STRING( pev->message ), pActivator );
		//CLIENT_PRINTF( ENT( pActivator->pev ), print_center, STRING( pev->message ) );
	}

	if( m_flWait > 0 )
	{
		SetThink( &CBaseTrigger::MultiWaitOver );
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		pev->nextthink = gpGlobals->time + 0.1f;
		SetThink( &CBaseEntity::SUB_Remove );
	}
}

// the wait time has passed, so set back up for another activation
void CBaseTrigger::MultiWaitOver( void )
{
	/*if( pev->max_health )
	{
		pev->health = pev->max_health;
		pev->takedamage	= DAMAGE_YES;
		pev->solid = SOLID_BBOX;
	}*/
	SetThink( NULL );
}

// ========================= COUNTING TRIGGER =====================================

//
// GLOBALS ASSUMED SET:  g_eoActivator
//
void CBaseTrigger::CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if( m_cTriggersLeft < 0 )
		return;

	BOOL fTellActivator =
		( m_hActivator != 0 ) &&
		m_hActivator->IsPlayer() &&
		!FBitSet( pev->spawnflags, SPAWNFLAG_NOMESSAGE );
	if( m_cTriggersLeft != 0 )
	{
		if( fTellActivator )
		{
			// UNDONE: I don't think we want these Quakesque messages
			switch( m_cTriggersLeft )
			{
			case 1:
				ALERT( at_console, "Only 1 more to go..." );
				break;
			case 2:
				ALERT( at_console, "Only 2 more to go..." );
				break;
			case 3:
				ALERT( at_console, "Only 3 more to go..." );
				break;
			default:
				ALERT( at_console, "There are more to go..." );
				break;
			}
		}
		return;
	}

	// !!!UNDONE: I don't think we want these Quakesque messages
	if( fTellActivator )
		ALERT( at_console, "Sequence completed!" );

	ActivateMultiTrigger( m_hActivator );
}

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
*/
class CTriggerCounter : public CBaseTrigger
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_counter, CTriggerCounter )

void CTriggerCounter::Spawn( void )
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if( m_cTriggersLeft == 0 )
		m_cTriggersLeft = 2;
	SetUse( &CBaseTrigger::CounterUse );
}

// ====================== TRIGGER_CHANGELEVEL ================================

class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_transition, CTriggerVolume )

// Define space that travels across a level transition
void CTriggerVolume::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );    // set size and link into world
	pev->model = 0;
	pev->modelindex = 0;
}

// Fires a target after level transition and then dies
class CFireAndDie : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Think( void );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; }	// Always go across transitions
};

LINK_ENTITY_TO_CLASS( fireanddie, CFireAndDie )

void CFireAndDie::Spawn( void )
{
	pev->classname = MAKE_STRING( "fireanddie" );
	// Don't call Precache() - it should be called on restore
}

void CFireAndDie::Precache( void )
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CFireAndDie::Think( void )
{
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	UTIL_Remove( this );
}

#define SF_CHANGELEVEL_USEONLY		0x0002
class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn( void );
	void Precache();
	void KeyValue( KeyValueData *pkvd );
	void EXPORT UseChangeLevel( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerChangeLevel( void );
	void EXPORT ExecuteChangeLevel( void );
	void EXPORT TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	static edict_t *FindLandmark( const char *pLandmarkName );
	static int ChangeList( LEVELLIST *pLevelList, int maxList );
	static int AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
	string_t m_changeTarget;
	float m_changeTargetDelay;
};

LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel )

// Global Savedata for changelevel trigger
TYPEDESCRIPTION	CChangeLevel::m_SaveData[] =
{
	DEFINE_ARRAY( CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_ARRAY( CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_FIELD( CChangeLevel, m_changeTarget, FIELD_STRING ),
	DEFINE_FIELD( CChangeLevel, m_changeTargetDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CChangeLevel, CBaseTrigger )

//
// Cache user-entity-field values until spawn is called.
//
void CChangeLevel::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "map" ) )
	{
		if( strlen( pkvd->szValue ) >= cchMapNameMost )
			ALERT( at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue );
		strcpy( m_szMapName, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "landmark" ) )
	{
		if( strlen( pkvd->szValue ) >= cchMapNameMost )
			ALERT( at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue );
		strcpy( m_szLandmarkName, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "changetarget" ) )
	{
		m_changeTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "changedelay" ) )
	{
		m_changeTargetDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

FILE_GLOBAL char st_szPrevMap[cchMapNameMost];

/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/
void CChangeLevel::Spawn( void )
{
	Precache();
	const int solid = pev->solid;
	if( FStrEq( m_szMapName, "" ) )
		ALERT( at_console, "a trigger_changelevel doesn't have a map" );

	if( FStrEq( m_szLandmarkName, "" ) )
		ALERT( at_console, "trigger_changelevel to %s doesn't have a landmark\n", m_szMapName );

	if( !FStringNull( pev->targetname ) )
	{
		SetUse( &CChangeLevel::UseChangeLevel );
	}
	InitTrigger();
	if (solid == SOLID_BSP)
		pev->solid = solid;
	if( !( pev->spawnflags & SF_CHANGELEVEL_USEONLY ) && pev->solid != SOLID_BSP )
		SetTouch( &CChangeLevel::TouchChangeLevel );
	//ALERT( at_console, "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );
}

void CChangeLevel::Precache()
{
	if (!( pev->spawnflags & SF_CHANGELEVEL_USEONLY ) && g_pGameRules->IsCoOp())
	{
		if (*st_szPrevMap)
		{
			if (FStrEq(st_szPrevMap, m_szMapName))
			{
				ALERT(at_aiconsole, "Next map is the same as previous\n");
				pev->solid = SOLID_BSP;
			}
		}
	}
}

void CChangeLevel::ExecuteChangeLevel( void )
{
	MESSAGE_BEGIN( MSG_ALL, SVC_CDTRACK );
		WRITE_BYTE( 3 );
		WRITE_BYTE( 3 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, SVC_INTERMISSION );
	MESSAGE_END();
}

FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t *CChangeLevel::FindLandmark( const char *pLandmarkName )
{
	edict_t	*pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING( NULL, "targetname", pLandmarkName );
	while( !FNullEnt( pentLandmark ) )
	{
		// Found the landmark
		if( FClassnameIs( pentLandmark, "info_landmark" ) )
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING( pentLandmark, "targetname", pLandmarkName );
	}
	ALERT( at_error, "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}

//=========================================================
// CChangeLevel :: Use - allows level transitions to be
// triggered by buttons, etc.
//
//=========================================================
void CChangeLevel::UseChangeLevel( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ChangeLevelNow( pActivator );
}

void CChangeLevel::ChangeLevelNow( CBaseEntity *pActivator )
{
	edict_t	*pentLandmark;
	//LEVELLIST levels[16];

	ASSERT( !FStrEq( m_szMapName, "" ) );

	// Don't work in deathmatch
	if( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() )
		return;

	// Some people are firing these multiple times in a frame, disable
	if( gpGlobals->time == pev->dmgtime )
		return;

	pev->dmgtime = gpGlobals->time;

	CBaseEntity *pPlayer = NULL;
	if (g_pGameRules->IsMultiplayer() && pActivator->IsPlayer())
	{
		pPlayer = pActivator;
	}
	else
	{
		pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );;
	}

	if (!g_pGameRules->IsCoOp())
	{
		if( !InTransitionVolume( pPlayer, m_szLandmarkName ) )
		{
			ALERT( at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName );
			return;
		}
	}

	// Create an entity to fire the changetarget
	if( m_changeTarget )
	{
		CFireAndDie *pFireAndDie = GetClassPtr( (CFireAndDie *)NULL );
		if( pFireAndDie )
		{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;

			// Call spawn
			DispatchSpawn( pFireAndDie->edict() );
		}
	}

	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy( st_szNextMap, m_szMapName );
	strcpy( st_szPrevMap, STRING(gpGlobals->mapname) );

	m_hActivator = pActivator;
	SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	st_szNextSpot[0] = 0;	// Init landmark to NULL

	// look for a landmark entity
	pentLandmark = FindLandmark( m_szLandmarkName );
	if( !FNullEnt( pentLandmark ) )
	{
		strcpy( st_szNextSpot, m_szLandmarkName );
		gpGlobals->vecLandmarkOffset = VARS( pentLandmark )->origin;
	}
	//ALERT( at_console, "Level touches %d levels\n", ChangeList( levels, 16 ) );
	ALERT( at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot );
	g_pGameRules->BeforeChangeLevel(st_szNextMap);
	if (g_pGameRules->IsMultiplayer())
	{
		if (g_pGameRules->IsCoOp())
		{
			SERVER_COMMAND( UTIL_VarArgs( "changelevel %s\n", st_szNextMap ) );
		}
	}
	else
	{
		CHANGE_LEVEL( st_szNextMap, st_szNextSpot );
	}
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel::TouchChangeLevel( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() )
		return;

	ChangeLevelNow( pOther );
}

// Add a transition to the list, but ignore duplicates
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark )
{
	int i;

	if( !pLevelList || !pMapName || !pLandmarkName || !pentLandmark )
		return 0;

	for( i = 0; i < listCount; i++ )
	{
		if( pLevelList[i].pentLandmark == pentLandmark && strcmp( pLevelList[i].mapName, pMapName ) == 0 )
			return 0;
	}
	strcpy( pLevelList[listCount].mapName, pMapName );
	strcpy( pLevelList[listCount].landmarkName, pLandmarkName );
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS( pentLandmark )->origin;

	return 1;
}

int BuildChangeList( LEVELLIST *pLevelList, int maxList )
{
	return CChangeLevel::ChangeList( pLevelList, maxList );
}

int CChangeLevel::InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName )
{
	edict_t	*pentVolume;

	if( pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION )
		return 1;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if( pEntity->pev->movetype == MOVETYPE_FOLLOW )
	{
		if( pEntity->pev->aiment != NULL )
			pEntity = CBaseEntity::Instance( pEntity->pev->aiment );
	}

	int inVolume = 1;	// Unless we find a trigger_transition, everything is in the volume

	pentVolume = FIND_ENTITY_BY_TARGETNAME( NULL, pVolumeName );
	while( !FNullEnt( pentVolume ) )
	{
		CBaseEntity *pVolume = CBaseEntity::Instance( pentVolume );

		if( pVolume && FClassnameIs( pVolume->pev, "trigger_transition" ) )
		{
			if( pVolume->Intersects( pEntity ) )	// It touches one, it's in the volume
				return 1;
			else
				inVolume = 0;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pentVolume = FIND_ENTITY_BY_TARGETNAME( pentVolume, pVolumeName );
	}

	return inVolume;
}

// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList( LEVELLIST *pLevelList, int maxList )
{
	edict_t	*pentChangelevel, *pentLandmark;
	int i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = FIND_ENTITY_BY_STRING( NULL, "classname", "trigger_changelevel" );
	if( FNullEnt( pentChangelevel ) )
		return 0;
	while( !FNullEnt( pentChangelevel ) )
	{
		CChangeLevel *pTrigger;

		pTrigger = GetClassPtr((CChangeLevel *)VARS(pentChangelevel));
		if( pTrigger )
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark( pTrigger->m_szLandmarkName );
			if( pentLandmark )
			{
				// Build a list of unique transitions
				if( AddTransitionToList( pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark ) )
				{
					count++;
					if( count >= maxList )		// FULL!!
						break;
				}
			}
		}
		pentChangelevel = FIND_ENTITY_BY_STRING( pentChangelevel, "classname", "trigger_changelevel" );
	}

	if( gpGlobals->pSaveData && ( (SAVERESTOREDATA *)gpGlobals->pSaveData)->pTable )
	{
		CSave saveHelper( (SAVERESTOREDATA *)gpGlobals->pSaveData );

		for( i = 0; i < count; i++ )
		{
			int j, entityCount = 0;
			CBaseEntity *pEntList[MAX_ENTITY];
			int entityFlags[MAX_ENTITY];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t *pent = UTIL_EntitiesInPVS( pLevelList[i].pentLandmark );

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while( !FNullEnt( pent ) )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance( pent );
				if( pEntity )
				{
					//ALERT( at_console, "Trying %s\n", STRING( pEntity->pev->classname ) );
					int caps = pEntity->ObjectCaps();
					if( !(caps & FCAP_DONT_SAVE ) )
					{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if( caps & FCAP_ACROSS_TRANSITION )
							flags |= FENTTABLE_MOVEABLE;
						if( pEntity->pev->globalname && !pEntity->IsDormant() )
							flags |= FENTTABLE_GLOBAL;
						if( flags )
						{
							pEntList[entityCount] = pEntity;
							entityFlags[entityCount] = flags;
							entityCount++;
							if( entityCount > MAX_ENTITY )
								ALERT( at_error, "Too many entities across a transition!" );
						}
						//else
						//	ALERT( at_console, "Failed %s\n", STRING( pEntity->pev->classname ) );
					}
					//else
					//	ALERT( at_console, "DON'T SAVE %s\n", STRING( pEntity->pev->classname ) );
				}
				pent = pent->v.chain;
			}

			for( j = 0; j < entityCount; j++ )
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if( entityFlags[j] && InTransitionVolume( pEntList[j], pLevelList[i].landmarkName ) )
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex( pEntList[j] );

					// Flag it with the level number
					saveHelper.EntityFlagsSet( index, entityFlags[j] | ( 1 << i ) );
				}
				//else
				//	ALERT( at_console, "Screened out %s\n", STRING( pEntList[j]->pev->classname ) );
			}
		}
	}

	return count;
}

/*
go to the next level for deathmatch
only called if a time or frag limit has expired
*/
void NextLevel( void )
{
	edict_t* pent;
	CChangeLevel *pChange;

	// find a trigger_changelevel
	pent = FIND_ENTITY_BY_CLASSNAME( NULL, "trigger_changelevel" );

	// go back to start if no trigger_changelevel
	if( FNullEnt( pent ) )
	{
		gpGlobals->mapname = MAKE_STRING( "start" );
		pChange = GetClassPtr( (CChangeLevel *)NULL );
		strcpy( pChange->m_szMapName, "start" );
	}
	else
		pChange = GetClassPtr( (CChangeLevel *)VARS( pent ) );

	strcpy( st_szNextMap, pChange->m_szMapName );
	g_fGameOver = TRUE;

	if( pChange->pev->nextthink < gpGlobals->time )
	{
		pChange->SetThink( &CChangeLevel::ExecuteChangeLevel );
		pChange->pev->nextthink = gpGlobals->time + 0.1f;
	}
}

// ============================== LADDER =======================================

class CLadder : public CBaseTrigger
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( func_ladder, CLadder )

void CLadder::KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}

//=========================================================
// func_ladder - makes an area vertically negotiable
//=========================================================
void CLadder::Precache( void )
{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;
	if( CVAR_GET_FLOAT( "showtriggers" ) == 0 )
	{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}
	pev->effects &= ~EF_NODRAW;
}

void CLadder::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), STRING( pev->model ) );    // set size and link into world
	pev->movetype = MOVETYPE_PUSH;
}

// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszPushVel;
	string_t m_iszPushSpeed;
};

LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush )

TYPEDESCRIPTION	CTriggerPush::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerPush, m_iszPushVel, FIELD_STRING ),
	DEFINE_FIELD( CTriggerPush, m_iszPushSpeed, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerPush,CBaseTrigger)

void CTriggerPush::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszPushSpeed"))
	{
		m_iszPushSpeed = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPushVel"))
	{
		m_iszPushVel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

/*QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
Pushes the player
*/

void CTriggerPush::Spawn()
{
	if( pev->angles == g_vecZero )
		pev->angles.y = 360;
	InitTrigger();

	if( pev->speed == 0 )
		pev->speed = 100;

	// this flag was changed and flying barrels on c2a5 stay broken
	if( FStrEq( STRING( gpGlobals->mapname ), "c2a5" ) && pev->spawnflags & 4 )
		pev->spawnflags |= SF_TRIGGER_PUSH_ONCE;

	if( FBitSet( pev->spawnflags, SF_TRIGGER_PUSH_START_OFF ) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse( &CBaseTrigger::ToggleUse );

	UTIL_SetOrigin( pev, pev->origin );		// Link into the list
}

void CTriggerPush::Touch( CBaseEntity *pOther )
{
	entvars_t *pevToucher = pOther->pev;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch( pevToucher->movetype )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if ( FBitSet( pev->spawnflags, SF_TRIGGER_PUSH_NO_CLIENTS ) && FBitSet(pOther->pev->flags, FL_CLIENT) )
		return;
	if ( FBitSet( pev->spawnflags, SF_TRIGGER_PUSH_NO_MONSTERS ) && FBitSet(pOther->pev->flags, FL_MONSTER) )
		return;

	Vector vecPush;
	if (!FStringNull(m_iszPushVel))
	{
		bool evaluated;
		vecPush = CalcLocus_Velocity( this, pOther, STRING(m_iszPushVel), &evaluated);
		if (!evaluated)
			return;
	}
	else
		vecPush = pev->movedir;

	if (!FStringNull(m_iszPushSpeed))
	{
		bool evaluated;
		float factor = CalcLocus_Ratio( pOther, STRING(m_iszPushSpeed), &evaluated );
		if (evaluated)
			vecPush = vecPush * factor;
	}

	if (pev->speed)
		vecPush = vecPush * pev->speed;
	else
		vecPush = vecPush * 100;

	if( pevToucher->solid != SOLID_NOT && pevToucher->solid != SOLID_BSP )
	{
		// Instant trigger, just transfer velocity and remove
		if( FBitSet( pev->spawnflags, SF_TRIGGER_PUSH_ONCE ) )
		{
			pevToucher->velocity = pevToucher->velocity + vecPush;
			if( pevToucher->velocity.z > 0 )
				pevToucher->flags &= ~FL_ONGROUND;
			UTIL_Remove( this );
		}
		else
		{
			// Push field, transfer to base velocity
			if( pevToucher->flags & FL_BASEVELOCITY )
				vecPush = vecPush + pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
			//ALERT( at_console, "Vel %f, base %f\n", pevToucher->velocity.z, pevToucher->basevelocity.z );
		}
	}
}

//======================================
// teleport trigger
//
//

#define SF_TELEPORT_KEEPANGLES 256
#define SF_TELEPORT_KEEPVELOCITY 512

class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT TeleportTouch( CBaseEntity *pOther );
	void EXPORT TeleportUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual edict_t* GetTeleportTarget();
	bool TeleportTouchImpl( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL m_fInactive;
};

LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport )

TYPEDESCRIPTION	CTriggerTeleport::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerTeleport, m_fInactive, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CTriggerTeleport, CBaseTrigger)

void CTriggerTeleport::Spawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerTeleport::TeleportTouch );
	if (m_fInactive)
	{
		SetUse( &CTriggerTeleport::TeleportUse );
		if (FStringNull(pev->targetname))
		{
			ALERT(at_warning, "Inactive %s without a name!\n", STRING(pev->classname));
		}
	}
}

void CTriggerTeleport::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "teleport_start_inactive"))
	{
		m_fInactive = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

void CTriggerTeleport::TeleportUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, !m_fInactive))
	{
		m_fInactive = !m_fInactive;
	}
}

bool CTriggerTeleport::TeleportTouchImpl( CBaseEntity *pOther )
{
	entvars_t *pevToucher = pOther->pev;
	edict_t	*pentTarget = NULL;

	// Only teleport monsters or clients
	if( !FBitSet( pevToucher->flags, FL_CLIENT | FL_MONSTER ) )
		return false;

	if (m_fInactive)
		return false;

	if( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return false;

	if( !( pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS ) )
	{
		// no monsters allowed!
		if( FBitSet( pevToucher->flags, FL_MONSTER ) )
		{
			return false;
		}
	}

	if( ( pev->spawnflags & SF_TRIGGER_NOCLIENTS ) )
	{
		// no clients allowed
		if( pOther->IsPlayer() )
		{
			return false;
		}
	}

	pentTarget = GetTeleportTarget();
	if( FNullEnt( pentTarget ) )
		return false;

	Vector tmp = VARS( pentTarget )->origin;

	if( pOther->IsPlayer() )
	{
		tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin( pevToucher, tmp );

	if (!FBitSet(pev->spawnflags, SF_TELEPORT_KEEPANGLES))
	{
		pevToucher->angles = pentTarget->v.angles;

		if( pOther->IsPlayer() )
		{
			pevToucher->v_angle = pentTarget->v.angles;
		}

		pevToucher->fixangle = TRUE;
	}

	if (!FBitSet(pev->spawnflags, SF_TELEPORT_KEEPVELOCITY))
	{
		pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
	}

	return true;
}

void CTriggerTeleport::TeleportTouch( CBaseEntity *pOther )
{
	TeleportTouchImpl(pOther);
}

edict_t* CTriggerTeleport::GetTeleportTarget()
{
	return FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
}

LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity )

class CTriggerSave : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT SaveTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_autosave, CTriggerSave )

void CTriggerSave::Spawn( void )
{
	if( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	InitTrigger();
	SetTouch( &CTriggerSave::SaveTouch );
}

void CTriggerSave::SaveTouch( CBaseEntity *pOther )
{
	if( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	// Only save on clients
	if( !pOther->IsPlayer() )
		return;

	SetTouch( NULL );
	UTIL_Remove( this );
	SERVER_COMMAND( "autosave\n" );
}

#define SF_ENDSECTION_USEONLY		0x0001

class CTriggerEndSection : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT EndSectionTouch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( trigger_endsection, CTriggerEndSection )

void CTriggerEndSection::EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Only save on clients
	if( pActivator && !pActivator->IsNetClient() )
		return;

	SetUse( NULL );

	if( pev->message )
	{
		g_engfuncs.pfnEndSection( STRING( pev->message ) );
	}
	UTIL_Remove( this );
}

void CTriggerEndSection::Spawn( void )
{
	if( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	InitTrigger();

	SetUse( &CTriggerEndSection::EndSectionUse );

	// If it is a "use only" trigger, then don't set the touch function.
	if( !( pev->spawnflags & SF_ENDSECTION_USEONLY ) )
		SetTouch( &CTriggerEndSection::EndSectionTouch );
}

void CTriggerEndSection::EndSectionTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if( !pOther->IsNetClient() )
		return;

	SetTouch( NULL );

	if( pev->message )
	{
		g_engfuncs.pfnEndSection( STRING( pev->message ) );
	}
	UTIL_Remove( this );
}

void CTriggerEndSection::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "section" ) )
	{
		//m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

class CTriggerGravity : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT GravityTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity )

void CTriggerGravity::Spawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerGravity::GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if( !pOther->IsPlayer() )
		return;

	pOther->pev->gravity = pev->gravity;
}

// this is a really bad idea.
class CTriggerChangeTarget : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_iszNewTarget;
};

LINK_ENTITY_TO_CLASS( trigger_changetarget, CTriggerChangeTarget )

TYPEDESCRIPTION	CTriggerChangeTarget::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CTriggerChangeTarget,CBaseDelay )

void CTriggerChangeTarget::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_iszNewTarget" ) )
	{
		m_iszNewTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerChangeTarget::Spawn( void )
{
}

void CTriggerChangeTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ), pActivator );

	if( pTarget )
	{
		if (UTIL_TargetnameIsActivator(m_iszNewTarget))
		{
			if (pActivator)
				pTarget->pev->target = pActivator->pev->targetname;
			else
				ALERT(at_error, "%s \"%s\" requires a locus!\n", STRING(pev->classname), STRING(pev->targetname));
		}
		else
			pTarget->pev->target = m_iszNewTarget;

		CBaseMonster *pMonster = pTarget->MyMonsterPointer();
		if( pMonster )
		{
			pMonster->m_pGoalEnt = NULL;
		}
	}
}

class CTriggerChangeValue : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszNewValue;
};
LINK_ENTITY_TO_CLASS( trigger_changevalue, CTriggerChangeValue )

TYPEDESCRIPTION	CTriggerChangeValue::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerChangeValue, m_iszNewValue, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeValue, CBaseDelay)

void CTriggerChangeValue::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewValue"))
	{
		m_iszNewValue = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszValueName"))
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerChangeValue::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = NULL;

	while ((pTarget = UTIL_FindEntityByTargetname( pTarget, STRING( pev->target ), pActivator )) != NULL)
	{
		KeyValueData mypkvd;
		mypkvd.szKeyName = STRING(pev->netname);
		mypkvd.szValue = STRING(m_iszNewValue);
		mypkvd.fHandled = FALSE;
		pTarget->KeyValue(&mypkvd);
	}

	if (!FStringNull(pev->message))
	{
		FireTargets(STRING(pev->message), pActivator, this, USE_TOGGLE, 0.0f);
	}
}

#define SF_CAMERA_PLAYER_POSITION	1
#define SF_CAMERA_PLAYER_TARGET		2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseDelay
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget( void );
	void Move( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	virtual int ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pentPath;
	string_t m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int m_state;
};

LINK_ENTITY_TO_CLASS( trigger_camera, CTriggerCamera )

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CTriggerCamera::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerCamera, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_pentPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTriggerCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_state, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerCamera, CBaseDelay )

void CTriggerCamera::Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_initialSpeed = pev->speed;
	if( m_acceleration == 0 )
		m_acceleration = 500;
	if( m_deceleration == 0 )
		m_deceleration = 500;
}

void CTriggerCamera::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "moveto" ) )
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "acceleration" ) )
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "deceleration" ) )
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_state ) )
		return;

	// Toggle state
	m_state = !m_state;
	if( m_state == 0 )
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}
	if( !pActivator || !pActivator->IsPlayer() )
	{
		pActivator = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	}

	m_hPlayer = pActivator;

	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// Nothing to look at!
	if( m_hTarget == 0 )
	{
		return;
	}

	if( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		( (CBasePlayer *)pActivator )->EnableControl( FALSE );
	}

	if( m_sPath )
	{
		m_pentPath = Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_sPath ) ) );
	}
	else
	{
		m_pentPath = NULL;
	}

	m_flStopTime = gpGlobals->time;
	if( m_pentPath )
	{
		if( m_pentPath->pev->speed != 0 )
			m_targetSpeed = m_pentPath->pev->speed;

		m_flStopTime += m_pentPath->GetDelay();
	}

	// copy over player information
	if( FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( pev, pActivator->pev->origin + pActivator->pev->view_ofs );
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
	{
		pev->velocity = Vector( 0, 0, 0 );
	}

	SET_VIEW( pActivator->edict(), edict() );

	SET_MODEL( ENT( pev ), STRING( pActivator->pev->model ) );

	// follow the player down
	SetThink( &CTriggerCamera::FollowTarget );
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move();
}

void CTriggerCamera::FollowTarget()
{
	if( m_hPlayer == 0 )
		return;

	if( m_hTarget == 0 || m_flReturnTime < gpGlobals->time )
	{
		if( m_hPlayer->IsAlive() )
		{
			SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
			( (CBasePlayer *)( (CBaseEntity *)m_hPlayer ) )->EnableControl( TRUE );
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
		pev->avelocity = Vector( 0, 0, 0 );
		m_state = 0;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles( m_hTarget->pev->origin - pev->origin );
	vecGoal.x = -vecGoal.x;

	if( pev->angles.y > 360 )
		pev->angles.y -= 360;

	if( pev->angles.y < 0 )
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if( dx < -180 )
		dx += 360;
	if( dx > 180 )
		dx = dx - 360;

	if( dy < -180 )
		dy += 360;
	if( dy > 180 )
		dy = dy - 360;

	pev->avelocity.x = dx * 40 * gpGlobals->frametime;
	pev->avelocity.y = dy * 40 * gpGlobals->frametime;

	if( !( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) ) )
	{
		pev->velocity = pev->velocity * 0.8f;
		if( pev->velocity.Length() < 10.0f )
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;

	Move();
}

void CTriggerCamera::Move()
{
	// Not moving on a path, return
	if( !m_pentPath )
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if( m_moveDistance <= 0 )
	{
		// Fire the passtarget if there is one
		if( m_pentPath->pev->message )
		{
			FireTargets( STRING( m_pentPath->pev->message ), this, this, USE_TOGGLE, 0 );
			if( FBitSet( m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE ) )
				m_pentPath->pev->message = 0;
		}

		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget();

		// Set up next corner
		if( !m_pentPath )
		{
			pev->velocity = g_vecZero;
		}
		else
		{
			if( m_pentPath->pev->speed != 0 )
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if( m_flStopTime > gpGlobals->time )
		pev->speed = UTIL_Approach( 0, pev->speed, m_deceleration * gpGlobals->frametime );
	else
		pev->speed = UTIL_Approach( m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime );

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ( ( pev->movedir * pev->speed ) * fraction ) + ( pev->velocity * ( 1 - fraction ) );
}

#if FEATURE_TRIGGER_RANDOM

#define TRIGGER_RANDOM_MAX_COUNT 16

#define SF_TRIGGER_RANDOM_START_ON 1
#define SF_TRIGGER_RANDOM_ONCE 2
#define SF_TRIGGER_RANDOM_REUSABLE 4
#define SF_TRIGGER_RANDOM_TIMED 8
#define SF_TRIGGER_RANDOM_UNIQUE 16
#define SF_TRIGGER_RANDOM_DONT_REPEAT 32

class CTriggerRandom : public CBaseEntity
{
public:
	void Spawn();
	void KeyValue( KeyValueData *pkvd );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TimedThink();

	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t ChooseTarget();
	float GetRandomDelay();
	int TargetCount();

	int m_targetCount;
	string_t m_targets[TRIGGER_RANDOM_MAX_COUNT];
	int m_uniqueTargetsLeft;
	int m_triggerNumberLimit;
	int m_triggerCounter;
	float m_minDelay;
	float m_maxDelay;
	string_t m_lastTarget;

	inline bool IsActive() { return pev->spawnflags & SF_TRIGGER_RANDOM_START_ON; }
	inline void SetActive(bool active)
	{
		if (active)
		{
			SetBits(pev->spawnflags, SF_TRIGGER_RANDOM_START_ON);
		}
		else
		{
			ClearBits(pev->spawnflags, SF_TRIGGER_RANDOM_START_ON);
		}
	}
};

LINK_ENTITY_TO_CLASS( trigger_random, CTriggerRandom )

// Sven Co-op compatibility
LINK_ENTITY_TO_CLASS( trigger_random_time, CTriggerRandom )
LINK_ENTITY_TO_CLASS( trigger_random_unique, CTriggerRandom )

TYPEDESCRIPTION	CTriggerRandom::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerRandom, m_targetCount, FIELD_INTEGER ),
	DEFINE_ARRAY( CTriggerRandom, m_targets, FIELD_STRING, TRIGGER_RANDOM_MAX_COUNT ),
	DEFINE_FIELD( CTriggerRandom, m_uniqueTargetsLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_triggerNumberLimit, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_triggerCounter, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_minDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerRandom, m_maxDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerRandom, m_lastTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CTriggerRandom, CBaseEntity )

void CTriggerRandom::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "min_delay") ) {
		m_minDelay = atof( pkvd->szValue );
		if (m_minDelay < 0) {
			m_minDelay = 0;
		}
		pkvd->fHandled = TRUE;
	} else if ( FStrEq( pkvd->szKeyName, "max_delay") ) {
		m_maxDelay = atof( pkvd->szValue );
		if (m_maxDelay < 0) {
			m_maxDelay = 0;
		}
		pkvd->fHandled = TRUE;
	} else if ( FStrEq( pkvd->szKeyName, "trigger_number") ) {
		m_triggerNumberLimit = atoi( pkvd->szValue );
		if (m_triggerNumberLimit < 0) {
			m_triggerNumberLimit = 0;
		}
		pkvd->fHandled = TRUE;
	} else if ( FStrEq( pkvd->szKeyName, "target_count" ) ) { // Sven Co-op compatibility
		m_targetCount = atoi( pkvd->szValue );
		if (m_targetCount < 0) {
			m_targetCount = 0;
		} else if (m_targetCount > TRIGGER_RANDOM_MAX_COUNT) {
			m_targetCount = TRIGGER_RANDOM_MAX_COUNT;
		}
		pkvd->fHandled = TRUE;
	} else if ( strncmp(pkvd->szKeyName, "target", 6) == 0 && isdigit(pkvd->szKeyName[6])) {
		pkvd->fHandled = FALSE;
		char buf[10] = "target";
		for (int i=0; i<TRIGGER_RANDOM_MAX_COUNT; ++i) {
			sprintf(buf+6, "%d", i+1);
			if (strcmp(buf+6, pkvd->szKeyName+6) == 0) {
				m_targets[i] = ALLOC_STRING( pkvd->szValue );
				pkvd->fHandled = TRUE;
				break;
			}
		}
		if (pkvd->fHandled == FALSE) {
			CBaseEntity::KeyValue( pkvd );
		}
	} else {
		CBaseEntity::KeyValue( pkvd );
	}
}

void CTriggerRandom::Spawn()
{
	// Sven Co-op compatibility
	if (FClassnameIs(pev, "trigger_random_time"))
	{
		SetBits(pev->spawnflags, SF_TRIGGER_RANDOM_TIMED);
	}
	if (FClassnameIs(pev, "trigger_random_unique"))
	{
		SetBits(pev->spawnflags, SF_TRIGGER_RANDOM_UNIQUE);
		if (FBitSet(pev->spawnflags, 1))
		{
			SetBits(pev->spawnflags, SF_TRIGGER_RANDOM_REUSABLE);
			ClearBits(pev->spawnflags, 1);
		}
	}

	m_triggerCounter = 0;
	if (FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_UNIQUE)) {
		m_uniqueTargetsLeft = TargetCount();
	}
	if (FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_TIMED)) {

		if (IsActive()) {
			SetThink(&CTriggerRandom::TimedThink);
			pev->nextthink = gpGlobals->time + GetRandomDelay() + 0.1;
		}
	}
}

void CTriggerRandom::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pev->spawnflags & SF_TRIGGER_RANDOM_TIMED) {
		SetActive(!IsActive());
		if (IsActive()) {
			SetThink(&CTriggerRandom::TimedThink);
			pev->nextthink = gpGlobals->time + GetRandomDelay();
		} else {
			SetThink(NULL);
			m_triggerCounter = 0;
		}
	} else {
		const int chosenTarget = ChooseTarget();
		if (!FStringNull(chosenTarget)) {
			FireTargets(STRING(chosenTarget), pActivator, this, USE_TOGGLE, value);
		}
	}
}

void CTriggerRandom::TimedThink()
{
	if (IsActive()) {
		int chosenTarget = ChooseTarget();
		if (!FStringNull(chosenTarget)) {
			FireTargets(STRING(chosenTarget), this, this, USE_TOGGLE, 0);
			if (m_triggerNumberLimit) {
				m_triggerCounter++;
				if (m_triggerCounter >= m_triggerNumberLimit) {
					SetActive(false);
					m_triggerCounter = 0;
				}
			}
		}

		if (pev->spawnflags & SF_TRIGGER_RANDOM_ONCE)
			SetActive(false);

		if (IsActive()) {
			pev->nextthink = gpGlobals->time + GetRandomDelay();
		}
	}
}

string_t CTriggerRandom::ChooseTarget()
{
	int chosenTargetIndex = 0;
	string_t chosenTarget = iStringNull;

	if (pev->spawnflags & SF_TRIGGER_RANDOM_UNIQUE) {
		if (m_uniqueTargetsLeft) {
			chosenTargetIndex = RANDOM_LONG(0, m_uniqueTargetsLeft - 1);
			chosenTarget = m_targets[chosenTargetIndex];

			if (chosenTarget == m_lastTarget && FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_DONT_REPEAT) && m_uniqueTargetsLeft > 1)
			{
				chosenTargetIndex = (chosenTargetIndex+1) % m_uniqueTargetsLeft;
				chosenTarget = m_targets[chosenTargetIndex];
			}

			m_targets[chosenTargetIndex] = m_targets[m_uniqueTargetsLeft-1];
			m_targets[m_uniqueTargetsLeft-1] = chosenTarget;
			m_uniqueTargetsLeft--;

			if (!m_uniqueTargetsLeft && (pev->spawnflags & SF_TRIGGER_RANDOM_REUSABLE) ) {
				m_uniqueTargetsLeft = TargetCount();
			}
			m_lastTarget = chosenTarget;
			return chosenTarget;
		}
	} else {
		const int targetCount = TargetCount();
		if (targetCount) {
			chosenTargetIndex = RANDOM_LONG(0, targetCount - 1);
			chosenTarget = m_targets[chosenTargetIndex];
			if (chosenTarget == m_lastTarget && FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_DONT_REPEAT) && targetCount > 1)
			{
				chosenTargetIndex = (chosenTargetIndex+1) % targetCount;
				chosenTarget = m_targets[chosenTargetIndex];
			}

			m_lastTarget = chosenTarget;
			return chosenTarget;
		}
	}
	return iStringNull;
}

float CTriggerRandom::GetRandomDelay()
{
	return RANDOM_FLOAT(m_minDelay, Q_max(m_maxDelay, m_minDelay));
}

int CTriggerRandom::TargetCount()
{
	if (m_targetCount) {
		return m_targetCount;
	} else {
		for (int i = ARRAYSIZE(m_targets) - 1; i>=0; --i ) {
			if (!FStringNull(m_targets[i])) {
				m_targetCount = i+1;
				return m_targetCount;
			}
		}
	}
	return 0;
}
#endif

#if FEATURE_TRIGGER_RESPAWN

#define SF_TRIGGERRESPAWN_DONT_MOVE_LIVING_PLAYERS 4

class CTriggerRespawn : public CBaseEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

extern void respawn( entvars_t *pev, BOOL fCopyCorpse );

void CTriggerRespawn::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->IsPlayer())
		{
			if (pPlayer->IsAlive())
			{
				if (!FBitSet(pev->spawnflags, SF_TRIGGERRESPAWN_DONT_MOVE_LIVING_PLAYERS))
				{
					g_pGameRules->GetPlayerSpawnSpot(pPlayer);
					pPlayer->pev->health = pPlayer->pev->max_health;
				}
			}
			else
			{
				respawn( pPlayer->pev, !( pPlayer->m_afPhysicsFlags & PFLAG_OBSERVER ) );
			}
		}
	}
}

LINK_ENTITY_TO_CLASS(trigger_respawn, CTriggerRespawn)
#endif

#if FEATURE_DISPLACER
class CDisplacerTarget : public CPointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

void CDisplacerTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	const BOOL enabled = !FBitSet(pev->spawnflags, SF_DISPLACER_TARGET_DISABLED);
	if (!ShouldToggle(useType, enabled))
		return;
	if (enabled)
	{
		SetBits(pev->spawnflags, SF_DISPLACER_TARGET_DISABLED);
	}
	else
	{
		ClearBits(pev->spawnflags, SF_DISPLACER_TARGET_DISABLED);
	}
}

LINK_ENTITY_TO_CLASS(info_displacer_xen_target, CDisplacerTarget)
LINK_ENTITY_TO_CLASS(info_displacer_earth_target, CDisplacerTarget)

class CTriggerXenReturn : public CTriggerTeleport
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT TeleportTouch( CBaseEntity *pOther );

	virtual edict_t* GetTeleportTarget();
};

LINK_ENTITY_TO_CLASS(trigger_xen_return, CTriggerXenReturn)

void CTriggerXenReturn::Spawn(void)
{
	CTriggerXenReturn::Precache();
	CTriggerTeleport::Spawn();

	SetTouch(&CTriggerXenReturn::TeleportTouch);
}

void CTriggerXenReturn::Precache()
{
	PRECACHE_SOUND( "debris/beamstart7.wav" );
}

void CTriggerXenReturn::TeleportTouch(CBaseEntity* pOther)
{
	if (TeleportTouchImpl(pOther))
	{
		if (pOther->IsPlayer())
		{
			// Ensure the current player is marked as being
			// on earth.
			((CBasePlayer*)pOther)->m_fInXen = FALSE;

			// Reset gravity to default.
			pOther->pev->gravity = 1.0f;
		}

		// Play teleport sound.
		EMIT_SOUND(ENT(pOther->pev), CHAN_STATIC, "debris/beamstart7.wav", 1, ATTN_NORM );
	}
}

edict_t* CTriggerXenReturn::GetTeleportTarget()
{
	edict_t* earthTarget = NULL;
	while((earthTarget = FIND_ENTITY_BY_CLASSNAME(earthTarget, "info_displacer_earth_target")) != NULL)
	{
		if (!FBitSet(earthTarget->v.spawnflags, SF_DISPLACER_TARGET_DISABLED))
		{
			return earthTarget;
		}
	}
	return NULL;
}

#endif

class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS( trigger_playerfreeze, CTriggerPlayerFreeze )

void CTriggerPlayerFreeze::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !pActivator || !pActivator->IsPlayer() )
		pActivator = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if( pActivator && (pActivator->pev->flags & FL_FROZEN) )
		( (CBasePlayer *)( (CBaseEntity *)pActivator ) )->EnableControl( TRUE );
	else
		( (CBasePlayer *)( (CBaseEntity *)pActivator ) )->EnableControl( FALSE );
}

#if FEATURE_TRIGGER_KILL_MONSTER

#define SF_KILLMONSTER_FIREONCE 1
#define SF_KILLMONSTER_KILL_BY_TARGETNAME 2
#define SF_KILLMONSTER_KILL_BY_CLASSNAME 4
#define SF_KILLMONSTER_KILL_BY_SQUADNAME 8
#define SF_KILLMONSTER_GIBALWAYS 16

class CTriggerKillMonster : public CBaseEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void OldUse();
protected:
	void KillMonster(CBaseEntity* pEntity);
	bool CheckTarget(CBaseEntity* pEntity);
};

LINK_ENTITY_TO_CLASS( trigger_killmonster, CTriggerKillMonster )

void CTriggerKillMonster::Spawn()
{
	pev->effects |= EF_NODRAW;
}

void CTriggerKillMonster::KillMonster(CBaseEntity *pEntity)
{
	CBaseMonster* pMonster = pEntity->MyMonsterPointer();
	if (pEntity->IsAlive() && pMonster)
	{
		switch (RANDOM_LONG(0,4)) {
		case 0:
			pMonster->m_LastHitGroup = HITGROUP_GENERIC;
			break;
		case 1:
			pMonster->m_LastHitGroup = HITGROUP_STOMACH;
			break;
		case 2:
			pMonster->m_LastHitGroup = HITGROUP_STOMACH;
			break;
		case 3:
			pMonster->m_LastHitGroup = HITGROUP_GENERIC;
			break;
		default:
			break;
		}
		pEntity->pev->health = 0;
		int damageType = DMG_GENERIC;
		if (pev->spawnflags & SF_KILLMONSTER_GIBALWAYS)
			damageType |= DMG_ALWAYSGIB;
		pEntity->TakeDamage(pev, pev, 1, damageType );
	}
}

bool CTriggerKillMonster::CheckTarget(CBaseEntity *pEntity)
{
	if (!FStringNull(pev->target))
	{
		if (FStringNull(pEntity->pev->targetname))
			return false;
		if (!FStrEq(STRING(pev->target), STRING(pEntity->pev->targetname)))
			return false;
	}

	if (!FStringNull(pev->message))
	{
		if (FStringNull(pEntity->pev->classname))
			return false;
		if (!FClassnameIs(pEntity->pev, STRING(pev->message)))
			return false;
	}

	if (!FStringNull(pev->netname))
	{
		if (FStringNull(pEntity->pev->netname))
			return false;
		if (!FStrEq(STRING(pev->netname), STRING(pEntity->pev->netname)))
			return false;
	}

	return true;
}

void CTriggerKillMonster::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FBitSet(pev->spawnflags, SF_KILLMONSTER_KILL_BY_TARGETNAME|SF_KILLMONSTER_KILL_BY_CLASSNAME|SF_KILLMONSTER_KILL_BY_SQUADNAME))
	{
		ALERT(at_console, "Using old trigger_killmonster behavior\n");
		OldUse();
	}
	else
	{
		CBaseEntity* pEntity = NULL;
		if (!FStringNull(pev->target))
		{
			while((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->target))) != NULL)
			{
				if (CheckTarget(pEntity))
					KillMonster(pEntity);
			}
		}
		else if (!FStringNull(pev->message))
		{
			while((pEntity = UTIL_FindEntityByClassname(pEntity, STRING(pev->message))) != NULL)
			{
				if (CheckTarget(pEntity))
					KillMonster(pEntity);
			}
		}
		else if (!FStringNull(pev->netname))
		{
			while((pEntity = UTIL_FindEntityByString(pEntity, "netname", STRING(pev->netname))) != NULL)
			{
				if (CheckTarget(pEntity))
					KillMonster(pEntity);
			}
		}
	}

	if( pev->spawnflags & SF_RELAY_FIREONCE )
		UTIL_Remove( this );
}

void CTriggerKillMonster::OldUse()
{
	CBaseEntity* pEntity;
	if ( pev->spawnflags & SF_KILLMONSTER_KILL_BY_TARGETNAME )
	{
		pEntity = NULL;
		while((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->target))) != NULL)
		{
			KillMonster(pEntity);
		}
	}

	if ( pev->spawnflags & SF_KILLMONSTER_KILL_BY_CLASSNAME )
	{
		pEntity = NULL;
		while((pEntity = UTIL_FindEntityByClassname(pEntity, STRING(pev->target))) != NULL)
		{
			KillMonster(pEntity);
		}
	}

	if ( pev->spawnflags & SF_KILLMONSTER_KILL_BY_SQUADNAME )
	{
		pEntity = NULL;
		while((pEntity = UTIL_FindEntityByString(pEntity, "netname", STRING(pev->target))) != NULL)
		{
			KillMonster(pEntity);
		}
	}
}
#endif

#if FEATURE_TRIGGER_TIMER

#define SF_TRIGGER_TIMER_START_ON 1
#define SF_TRIGGER_TIMER_NO_FIRST_DELAY 32

class CTriggerTimer : public CBaseEntity
{
public:
	void Spawn();
	void KeyValue( KeyValueData *pkvd );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Think();

	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float GetRandomDelay();
	void SetActive(BOOL active);

	int m_triggerNumberLimit;
	int m_triggerCounter;
	float m_minDelay;
	float m_maxDelay;
	BOOL m_active;
};

LINK_ENTITY_TO_CLASS( trigger_timer, CTriggerTimer )

TYPEDESCRIPTION	CTriggerTimer::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerTimer, m_triggerNumberLimit, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerTimer, m_triggerCounter, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerTimer, m_minDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerTimer, m_maxDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerTimer, m_active, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CTriggerTimer, CBaseEntity )

void CTriggerTimer::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "min_delay") ) {
		m_minDelay = atof( pkvd->szValue );
		if (m_minDelay < 0) {
			m_minDelay = 0;
		}
		pkvd->fHandled = TRUE;
	} else if ( FStrEq( pkvd->szKeyName, "max_delay") ) {
		m_maxDelay = atof( pkvd->szValue );
		if (m_maxDelay < 0) {
			m_maxDelay = 0;
		}
		pkvd->fHandled = TRUE;
	} else if ( FStrEq( pkvd->szKeyName, "trigger_number") ) {
		m_triggerNumberLimit = atoi( pkvd->szValue );
		if (m_triggerNumberLimit < 0) {
			m_triggerNumberLimit = 0;
		}
		pkvd->fHandled = TRUE;
	} else {
		CBaseEntity::KeyValue( pkvd );
	}
}

void CTriggerTimer::Spawn()
{
	m_triggerCounter = 0;
	m_active = FALSE;
	if (pev->spawnflags & SF_TRIGGER_TIMER_START_ON) {
		SetActive(TRUE);
		pev->nextthink += 0.1; // some little delay of spawn
	}
}

void CTriggerTimer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	switch (useType) {
	case USE_OFF:
		SetActive(FALSE);
		break;
	case USE_ON:
		SetActive(TRUE);
		break;
	default:
		SetActive(!m_active);
		break;
	}
}

void CTriggerTimer::Think()
{
	if (m_active) {
		if (!FStringNull(pev->target)) {
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
			if (m_triggerNumberLimit) {
				m_triggerCounter++;
				if (m_triggerCounter >= m_triggerNumberLimit) {
					SetActive(FALSE);
					return;
				}
			}
		}

		pev->nextthink = gpGlobals->time + GetRandomDelay();
	}
}

float CTriggerTimer::GetRandomDelay()
{
	return RANDOM_FLOAT(m_minDelay, Q_max(m_maxDelay, m_minDelay));
}

void CTriggerTimer::SetActive(BOOL active)
{
	if (m_active == active)
		return;
	m_active = active;
	if (m_active)
	{
		if (FBitSet(pev->spawnflags, SF_TRIGGER_TIMER_NO_FIRST_DELAY))
			pev->nextthink = gpGlobals->time;
		else
			pev->nextthink = gpGlobals->time + GetRandomDelay();
	}
	else
	{
		m_triggerCounter = 0;
	}
}
#endif

#if FEATURE_GENEWORM
//=========================================================
// CTriggerGenewormHit
//=========================================================

class CTriggerGenewormHit : public CBaseTrigger
{
public:
	void Spawn();
	void Precache();
	void EXPORT GeneWormTouch(CBaseEntity *pOther);

	static const char* pAttackSounds[];

	static TYPEDESCRIPTION m_SaveData[];

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	float m_flLastDamageTime;
};

TYPEDESCRIPTION CTriggerGenewormHit::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerGenewormHit, m_flLastDamageTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CTriggerGenewormHit, CBaseTrigger)

const char *CTriggerGenewormHit::pAttackSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav"
};

void CTriggerGenewormHit::Spawn()
{
	Precache();
	InitTrigger();

	SetTouch(&CTriggerGenewormHit::GeneWormTouch);

	if(pev->targetname)
		SetUse(&CBaseTrigger::ToggleUse);


	if(pev->spawnflags & SF_TRIGGER_HURT_START_OFF)
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);
	pev->dmg = gSkillData.gwormDmgHit;
	m_flLastDamageTime = gpGlobals->time;
}

void CTriggerGenewormHit::Precache()
{
	PRECACHE_SOUND_ARRAY(pAttackSounds);
}

void CTriggerGenewormHit::GeneWormTouch(CBaseEntity *pOther)
{
	if( gpGlobals->time - m_flLastDamageTime < 2 || !pOther->pev->takedamage )
		return;

	if( ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH ) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if( ( pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS ) && pOther->IsPlayer() )
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if( g_pGameRules->IsMultiplayer() )
	{
		if( pev->dmgtime > gpGlobals->time )
		{
			if( gpGlobals->time != pev->pain_finished )
			{
				// too early to hurt again, and not same frame with a different entity
				if( pOther->IsPlayer() )
				{
					int playerMask = 1 << ( pOther->entindex() - 1 );

					// If I've already touched this player (this time), then bail out
					if( pev->impulse & playerMask )
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if( pOther->IsPlayer() )
			{
				int playerMask = 1 << ( pOther->entindex() - 1 );

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else	// Original code -- single player
	{
		if( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
		{
			// too early to hurt again, and not same frame with a different entity
			return;
		}
	}

	// If this is time_based damage (poison, radiation), override the pev->dmg with a
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	pOther->TakeDamage( pev, pev, gSkillData.gwormDmgHit, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;// half second delay until this trigger can hurt toucher again

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, RANDOM_SOUND_ARRAY(pAttackSounds), VOL_NORM, 0.1, 0, 100 + RANDOM_FLOAT(-5,5));
	m_flLastDamageTime = gpGlobals->time;

	if( pev->target )
	{
		// trigger has a target it wants to fire.
		if( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE )
		{
			// if the toucher isn't a client, don't fire the target!
			if( !pOther->IsPlayer() )
			{
				return;
			}
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}

LINK_ENTITY_TO_CLASS(trigger_geneworm_hit, CTriggerGenewormHit)
#endif

//===========================================================
//LRC- trigger_startpatrol
//===========================================================
class CTriggerSetPatrol : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszPath;
};
LINK_ENTITY_TO_CLASS( trigger_startpatrol, CTriggerSetPatrol )

TYPEDESCRIPTION	CTriggerSetPatrol::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerSetPatrol, m_iszPath, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerSetPatrol, CBaseDelay)

void CTriggerSetPatrol::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszPath"))
	{
		m_iszPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerSetPatrol::Spawn( void )
{
	pev->effects |= EF_NODRAW;
}


void CTriggerSetPatrol::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = NULL;

	if (UTIL_TargetnameIsActivator(STRING(pev->target)))
	{
		if (pActivator != 0 && pActivator->MyMonsterPointer() && FBitSet(pActivator->pev->flags, FL_MONSTER))
		{
			pTarget = pActivator;
		}
		else
		{
			return;
		}
	}
	else
	{
		pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	}

	CBaseEntity *pPath = UTIL_FindEntityByTargetname( NULL, STRING( m_iszPath ) );

	if (pTarget && pPath)
	{
		CBaseMonster *pMonster = pTarget->MyMonsterPointer();
		if (pMonster)
		{
			Schedule_t* patrolSchedule = pMonster->StartPatrol(pPath);
			if (patrolSchedule)
			{
				CFollowingMonster* followingMonster = pMonster->MyFollowingMonsterPointer();
				if (followingMonster->IsFollowingPlayer())
				{
					followingMonster->StopFollowing(TRUE, false);
				}
				pMonster->SetState( MONSTERSTATE_IDLE );
				pMonster->ChangeSchedule( patrolSchedule );
			}
		}
	}
}

#if FEATURE_TRIGGER_MOTION
//===========================================================
//LRC- trigger_motion
//===========================================================
#define SF_MOTION_DEBUG 1
class CTriggerMotion : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	KeyValue( KeyValueData *pkvd );

	string_t m_iszPosition;
	int m_iPosMode;
	string_t m_iszAngles;
	int m_iAngMode;
	string_t m_iszVelocity;
	int m_iVelMode;
	string_t m_iszAVelocity;
	int m_iAVelMode;
};
LINK_ENTITY_TO_CLASS( trigger_motion, CTriggerMotion )

TYPEDESCRIPTION	CTriggerMotion::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerMotion, m_iszPosition, FIELD_STRING ),
	DEFINE_FIELD( CTriggerMotion, m_iPosMode, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerMotion, m_iszAngles, FIELD_STRING ),
	DEFINE_FIELD( CTriggerMotion, m_iAngMode, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerMotion, m_iszVelocity, FIELD_STRING ),
	DEFINE_FIELD( CTriggerMotion, m_iVelMode, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerMotion, m_iszAVelocity, FIELD_STRING ),
	DEFINE_FIELD( CTriggerMotion, m_iAVelMode, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CTriggerMotion, CPointEntity)

void CTriggerMotion::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPosMode"))
	{
		m_iPosMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszAngles"))
	{
		m_iszAngles = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAngMode"))
	{
		m_iAngMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelocity"))
	{
		m_iszVelocity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iVelMode"))
	{
		m_iVelMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszAVelocity"))
	{
		m_iszAVelocity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAVelMode"))
	{
		m_iAVelMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerMotion::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING(pev->target), pActivator );
	if (pTarget == NULL || pActivator == NULL) return;

	if (pev->spawnflags & SF_MOTION_DEBUG)
		ALERT(at_console, "DEBUG: trigger_motion affects %s \"%s\":\n", STRING(pTarget->pev->classname), STRING(pTarget->pev->targetname));

	if (m_iszPosition)
	{
		switch (m_iPosMode)
		{
		case 0:
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set origin from %f %f %f ", pTarget->pev->origin.x, pTarget->pev->origin.y, pTarget->pev->origin.z);
			pTarget->pev->origin = CalcLocus_Position( this, pActivator, STRING(m_iszPosition) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->origin.x, pTarget->pev->origin.y, pTarget->pev->origin.z);
			pTarget->pev->flags &= ~FL_ONGROUND;
			break;
		case 1:
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set origin from %f %f %f ", pTarget->pev->origin.x, pTarget->pev->origin.y, pTarget->pev->origin.z);
			pTarget->pev->origin = pTarget->pev->origin + CalcLocus_Velocity( this, pActivator, STRING(m_iszPosition) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->origin.x, pTarget->pev->origin.y, pTarget->pev->origin.z);
			pTarget->pev->flags &= ~FL_ONGROUND;
			break;
		}
	}

	Vector vecTemp;
	Vector vecVelAngles;
	if (m_iszAngles)
	{
		switch (m_iAngMode)
		{
		case 0:
			vecTemp = CalcLocus_Velocity( this, pActivator, STRING(m_iszAngles) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set angles from %f %f %f ", pTarget->pev->angles.x, pTarget->pev->angles.y, pTarget->pev->angles.z);
			pTarget->pev->angles = UTIL_VecToAngles( vecTemp );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->angles.x, pTarget->pev->angles.y, pTarget->pev->angles.z);
			break;
		case 1:
			vecTemp = CalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Rotate angles from %f %f %f ", pTarget->pev->angles.x, pTarget->pev->angles.y, pTarget->pev->angles.z);
			pTarget->pev->angles = pTarget->pev->angles + UTIL_VecToAngles( vecTemp );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->angles.x, pTarget->pev->angles.y, pTarget->pev->angles.z);
			break;
		case 2:
			UTIL_StringToRandomVector( vecTemp, STRING(m_iszAngles) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Rotate angles from %f %f %f ", pTarget->pev->angles.x, pTarget->pev->angles.y, pTarget->pev->angles.z);
			pTarget->pev->angles = pTarget->pev->angles + vecTemp;
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->angles.x, pTarget->pev->angles.y, pTarget->pev->angles.z);
			break;
		}
	}

	if (m_iszVelocity)
	{
		switch (m_iVelMode)
		{
		case 0:
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set velocity from %f %f %f ", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			pTarget->pev->velocity = CalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			break;
		case 1:
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set velocity from %f %f %f ", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			pTarget->pev->velocity = pTarget->pev->velocity + CalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			break;
		case 2:
			vecTemp = CalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity) );
			vecVelAngles = UTIL_VecToAngles( vecTemp ) + UTIL_VecToAngles( pTarget->pev->velocity );
			UTIL_MakeVectors( vecVelAngles );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Rotate velocity from %f %f %f ", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			pTarget->pev->velocity = pTarget->pev->velocity.Length() * gpGlobals->v_forward;
			pTarget->pev->velocity.z = -pTarget->pev->velocity.z; //vecToAngles reverses the z angle
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			break;
		case 3:
			UTIL_StringToRandomVector( vecTemp, STRING(m_iszVelocity) );
			vecVelAngles = vecTemp + UTIL_VecToAngles( pTarget->pev->velocity );
			UTIL_MakeVectors( vecVelAngles );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Rotate velocity from %f %f %f ", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			pTarget->pev->velocity = pTarget->pev->velocity.Length() * gpGlobals->v_forward;
			pTarget->pev->velocity.z = -pTarget->pev->velocity.z; //vecToAngles reverses the z angle
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", pTarget->pev->velocity.x, pTarget->pev->velocity.y, pTarget->pev->velocity.z);
			break;
		}
	}

	switch (m_iAVelMode)
	{
	case 0:
		UTIL_StringToRandomVector( vecTemp, STRING(m_iszAVelocity) );
		if (pev->spawnflags & SF_MOTION_DEBUG)
			ALERT(at_console, "DEBUG: Set avelocity from %f %f %f ", pTarget->pev->avelocity.x, pTarget->pev->avelocity.y, pTarget->pev->avelocity.z);
		pTarget->pev->avelocity = vecTemp;
		if (pev->spawnflags & SF_MOTION_DEBUG)
			ALERT(at_console, "to %f %f %f\n", pTarget->pev->avelocity.x, pTarget->pev->avelocity.y, pTarget->pev->avelocity.z);
		break;
	case 1:
		UTIL_StringToRandomVector( vecTemp, STRING(m_iszAVelocity) );
		if (pev->spawnflags & SF_MOTION_DEBUG)
			ALERT(at_console, "DEBUG: Set avelocity from %f %f %f ", pTarget->pev->avelocity.x, pTarget->pev->avelocity.y, pTarget->pev->avelocity.z);
		pTarget->pev->avelocity = pTarget->pev->avelocity + vecTemp;
		if (pev->spawnflags & SF_MOTION_DEBUG)
			ALERT(at_console, "to %f %f %f\n", pTarget->pev->avelocity.x, pTarget->pev->avelocity.y, pTarget->pev->avelocity.z);
		break;
	}
}

//===========================================================
//LRC- motion_manager
//===========================================================
class CMotionThread : public CPointEntity
{
public:
	void Spawn();
	void EXPORT MotionThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszPosition;
	int m_iPosMode;
	string_t m_iszFacing;
	int m_iFaceMode;
	EHANDLE m_hLocus;
	EHANDLE m_hTarget;
};
LINK_ENTITY_TO_CLASS( motion_thread, CMotionThread )

TYPEDESCRIPTION	CMotionThread::m_SaveData[] =
{
	DEFINE_FIELD( CMotionThread, m_iszPosition, FIELD_STRING ),
	DEFINE_FIELD( CMotionThread, m_iPosMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMotionThread, m_iszFacing, FIELD_STRING ),
	DEFINE_FIELD( CMotionThread, m_iFaceMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMotionThread, m_hLocus, FIELD_EHANDLE ),
	DEFINE_FIELD( CMotionThread, m_hTarget, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE(CMotionThread, CPointEntity)

void CMotionThread::Spawn()
{
	SetThink( &CMotionThread::MotionThink );
	pev->classname = MAKE_STRING("motion_thread");
}

void CMotionThread::MotionThink( void )
{
	if( m_hLocus == 0 || m_hTarget == 0 )
	{
		if (pev->spawnflags & SF_MOTION_DEBUG)
			ALERT(at_console, "motion_thread expires\n");
		SetThink(&CMotionThread:: SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	else
	{
		pev->nextthink = gpGlobals->time; // think every frame
	}

	if (pev->spawnflags & SF_MOTION_DEBUG)
		ALERT(at_console, "motion_thread affects %s \"%s\":\n", STRING(m_hTarget->pev->classname), STRING(m_hTarget->pev->targetname));

	Vector vecTemp;

	if (m_iszPosition)
	{
		switch (m_iPosMode)
		{
		case 0: // set position
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set origin from %f %f %f ", m_hTarget->pev->origin.x, m_hTarget->pev->origin.y, m_hTarget->pev->origin.z);
			UTIL_AssignOrigin(m_hTarget, CalcLocus_Position( this, m_hLocus, STRING(m_iszPosition) ));
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->origin.x, m_hTarget->pev->origin.y, m_hTarget->pev->origin.z);
			m_hTarget->pev->flags &= ~FL_ONGROUND;
			break;
		case 1: // offset position (= fake velocity)
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Offset origin from %f %f %f ", m_hTarget->pev->origin.x, m_hTarget->pev->origin.y, m_hTarget->pev->origin.z);
			UTIL_AssignOrigin(m_hTarget, m_hTarget->pev->origin + gpGlobals->frametime * CalcLocus_Velocity( this, m_hLocus, STRING(m_iszPosition) ));
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->origin.x, m_hTarget->pev->origin.y, m_hTarget->pev->origin.z);
			m_hTarget->pev->flags &= ~FL_ONGROUND;
			break;
		case 2: // set velocity
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set velocity from %f %f %f ", m_hTarget->pev->velocity.x, m_hTarget->pev->velocity.y, m_hTarget->pev->velocity.z);
			UTIL_SetVelocity(m_hTarget, CalcLocus_Velocity( this, m_hLocus, STRING(m_iszPosition) ));
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->velocity.x, m_hTarget->pev->velocity.y, m_hTarget->pev->velocity.z);
			break;
		case 3: // accelerate
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Accelerate from %f %f %f ", m_hTarget->pev->velocity.x, m_hTarget->pev->velocity.y, m_hTarget->pev->velocity.z);
			UTIL_SetVelocity(m_hTarget, m_hTarget->pev->velocity + gpGlobals->frametime * CalcLocus_Velocity( this, m_hLocus, STRING(m_iszPosition) ));
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->velocity.x, m_hTarget->pev->velocity.y, m_hTarget->pev->velocity.z);
			break;
		case 4: // follow position
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set velocity (path) from %f %f %f ", m_hTarget->pev->velocity.x, m_hTarget->pev->velocity.y, m_hTarget->pev->velocity.z);
			UTIL_SetVelocity(m_hTarget, CalcLocus_Position( this, m_hLocus, STRING(m_iszPosition) ) - m_hTarget->pev->origin);
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->velocity.x, m_hTarget->pev->velocity.y, m_hTarget->pev->velocity.z);
			break;
		}
	}

	Vector vecVelAngles;

	if (m_iszFacing)
	{
		switch (m_iFaceMode)
		{
		case 0: // set angles
			vecTemp = CalcLocus_Velocity( this, m_hLocus, STRING(m_iszFacing) );
			if (vecTemp != g_vecZero) // if the vector is 0 0 0, don't use it
			{
				if (pev->spawnflags & SF_MOTION_DEBUG)
					ALERT(at_console, "DEBUG: Set angles from %f %f %f ", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
				UTIL_SetAngles(m_hTarget, UTIL_VecToAngles( vecTemp ));
				if (pev->spawnflags & SF_MOTION_DEBUG)
					ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
			}
			else if (pev->spawnflags & SF_MOTION_DEBUG)
			{
				ALERT(at_console, "Zero velocity, don't change angles\n");
			}
			break;
		case 1: // offset angles (= fake avelocity)
			vecTemp = CalcLocus_Velocity( this, m_hLocus, STRING(m_iszFacing) );
			if (vecTemp != g_vecZero) // if the vector is 0 0 0, don't use it
			{
				if (pev->spawnflags & SF_MOTION_DEBUG)
					ALERT(at_console, "DEBUG: Offset angles from %f %f %f ", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
				UTIL_SetAngles(m_hTarget, m_hTarget->pev->angles + gpGlobals->frametime * UTIL_VecToAngles( vecTemp ));
				if (pev->spawnflags & SF_MOTION_DEBUG)
					ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
			}
			else if (pev->spawnflags & SF_MOTION_DEBUG)
			{
				ALERT(at_console, "Zero velocity, don't change angles\n");
			}
			break;
		case 2: // offset angles (= fake avelocity)
			UTIL_StringToRandomVector( vecVelAngles, STRING(m_iszFacing) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Rotate angles from %f %f %f ", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
			UTIL_SetAngles(m_hTarget, m_hTarget->pev->angles + gpGlobals->frametime * vecVelAngles);
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
			break;
		case 3: // set avelocity
			UTIL_StringToRandomVector( vecTemp, STRING(m_iszFacing) );
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "DEBUG: Set avelocity from %f %f %f ", m_hTarget->pev->avelocity.x, m_hTarget->pev->avelocity.y, m_hTarget->pev->avelocity.z);
			UTIL_SetAvelocity(m_hTarget, vecTemp);
			if (pev->spawnflags & SF_MOTION_DEBUG)
				ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->avelocity.x, m_hTarget->pev->avelocity.y, m_hTarget->pev->avelocity.z);
			break;
		}
	}
}


class CMotionManager : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
	void Affect(CBaseEntity *pTarget, CBaseEntity *pActivator );
	void Activate( void ); // TODO: change to PostSpawn
	void UpdateOnRemove();
	void RemoveThreads();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszPosition;
	int m_iPosMode;
	string_t m_iszFacing;
	int m_iFaceMode;
	BOOL m_activated;
};
LINK_ENTITY_TO_CLASS( motion_manager, CMotionManager )

TYPEDESCRIPTION	CMotionManager::m_SaveData[] =
{
	DEFINE_FIELD( CMotionManager, m_iszPosition, FIELD_STRING ),
	DEFINE_FIELD( CMotionManager, m_iPosMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMotionManager, m_iszFacing, FIELD_STRING ),
	DEFINE_FIELD( CMotionManager, m_iFaceMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMotionManager, m_activated, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CMotionManager,CPointEntity)

void CMotionManager::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPosMode"))
	{
		m_iPosMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszFacing"))
	{
		m_iszFacing = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFaceMode"))
	{
		m_iFaceMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CMotionManager::Activate( void )
{
	if (m_activated)
		return;
	if (FStringNull(pev->targetname))
		Use( this, this, USE_ON, 0 );
	m_activated = TRUE;
}

void CMotionManager::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = NULL;
	if (useType == USE_OFF || useType == USE_TOGGLE)
	{
		RemoveThreads();
	}
	if (useType == USE_ON || useType == USE_TOGGLE)
	{
		if (pev->target)
		{
			pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target), pActivator);
			if (pTarget == NULL)
				ALERT(at_error, "motion_manager \"%s\" can't find entity \"%s\" to affect\n", STRING(pev->targetname), STRING(pev->target));
			else
			{
				do
				{
					Affect( pTarget, pActivator );
					pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(pev->target), pActivator);
				} while ( pTarget );
			}
		}
	}
}

void CMotionManager::Affect( CBaseEntity *pTarget, CBaseEntity *pActivator )
{
	if (pev->spawnflags & SF_MOTION_DEBUG)
		ALERT(at_console, "DEBUG: Creating MotionThread for %s \"%s\"\n", STRING(pTarget->pev->classname), STRING(pTarget->pev->targetname));

	CMotionThread *pThread = GetClassPtr( (CMotionThread*)NULL );
	if (pThread == NULL) return; //error?
	pThread->Spawn();
	pThread->pev->targetname = pev->targetname;
	pThread->m_hLocus = pActivator;
	pThread->m_hTarget = pTarget;
	pThread->m_iszPosition = m_iszPosition;
	pThread->m_iPosMode = m_iPosMode;
	pThread->m_iszFacing = m_iszFacing;
	pThread->m_iFaceMode = m_iFaceMode;
	pThread->pev->spawnflags = pev->spawnflags;
	pThread->pev->nextthink = gpGlobals->time;
}

void CMotionManager::UpdateOnRemove()
{
	RemoveThreads();
	CPointEntity::UpdateOnRemove();
}

void CMotionManager::RemoveThreads()
{
	CBaseEntity* pEntity = NULL;
	if (!FStringNull(pev->targetname))
	{
		while ((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->targetname))) != NULL)
		{
			if (FClassnameIs(pEntity->pev, "motion_thread"))
			{
				CMotionThread* motionThread = (CMotionThread*)pEntity;
				motionThread->m_hTarget = 0;
				motionThread->m_hLocus = 0;
			}
		}
	}
}
#endif

#if FEATURE_TRIGGER_COMMAND
//=====================================================
// trigger_command: activate a console command
//=====================================================
class CTriggerCommand : public CBaseEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};
LINK_ENTITY_TO_CLASS( trigger_command, CTriggerCommand )

void CTriggerCommand::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	char szCommand[256];

	if (pev->netname)
	{
		sprintf( szCommand, "%s\n", STRING(pev->netname) );
		SERVER_COMMAND( szCommand );
	}
}
#endif

#if FEATURE_TRIGGER_HURT_REMOTE

#define SF_TRIGGER_HURT_REMOTE_INSTANT_KILL 1
#define SF_TRIGGER_HURT_REMOTE_CONSTANT 2
#define SF_TRIGGER_HURT_REMOTE_STARTON 4

class CTriggerHurtRemote : public CPointEntity
{
public:
	void Spawn();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT PeriodicHurt();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hActivator;

protected:
	void DoDamage();
	void DoDamage(CBaseEntity* pTarget);
	string_t TargetClass() const { return pev->netname; }
	int DamageType() const { return pev->weapons; }
	float Delay() const { return pev->frags ? pev->frags : 0.1; }
	bool IsActive() const { return FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_STARTON); }
};

LINK_ENTITY_TO_CLASS( trigger_hurt_remote, CTriggerHurtRemote )

TYPEDESCRIPTION	CTriggerHurtRemote::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerHurtRemote, m_hActivator, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CTriggerHurtRemote, CPointEntity )

void CTriggerHurtRemote::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "targetclass"))
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay"))
	{
		pev->frags = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		pev->weapons = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerHurtRemote::Spawn()
{
	CPointEntity::Spawn();
	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_CONSTANT) && IsActive())
	{
		SetThink(&CTriggerHurtRemote::PeriodicHurt);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CTriggerHurtRemote::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_CONSTANT))
	{
		if (ShouldToggle(useType, IsActive()))
		{
			if (IsActive())
			{
				ClearBits(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_STARTON);
				SetThink(NULL);
				pev->nextthink = 0.0f;
				m_hActivator = 0;
			}
			else
			{
				SetBits(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_STARTON);
				SetThink(&CTriggerHurtRemote::PeriodicHurt);
				pev->nextthink = gpGlobals->time;
				m_hActivator = pActivator;
			}
		}
	}
	else
	{
		if (useType != USE_OFF)
		{
			m_hActivator = pActivator;
			DoDamage();
		}
	}
}

void CTriggerHurtRemote::PeriodicHurt()
{
	DoDamage();
	pev->nextthink = gpGlobals->time + Delay();
}

void CTriggerHurtRemote::DoDamage()
{
	CBaseEntity *pTarget = NULL;
	CBaseEntity *pActivator = m_hActivator;
	while ( (pTarget = UTIL_FindEntityByTargetname( pTarget, STRING( pev->target ), pActivator )) != NULL )
	{
		DoDamage(pTarget);
	}

	string_t targetClass = TargetClass();
	if (!FStringNull(targetClass))
	{
		pTarget = NULL;
		while ( (pTarget = UTIL_FindEntityByClassname( pTarget, STRING( targetClass ) )) != NULL )
		{
			DoDamage(pTarget);
		}
	}
}

void CTriggerHurtRemote::DoDamage(CBaseEntity* pTarget)
{
	if (!pTarget->IsAlive())
		return;

	CBaseEntity* pActivator = m_hActivator;
	if (pev->dmg >= 0)
	{
		entvars_t* pevAttacker = pActivator != 0 ? pActivator->pev : pev;
		if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_INSTANT_KILL))
		{
			if (pTarget->IsPlayer())
			{
				pTarget->TakeDamage(pTarget->pev, pevAttacker, pTarget->pev->health + pTarget->pev->armorvalue / ARMOR_BONUS, DamageType());
			}
			else
			{
				pTarget->TakeDamage(pTarget->pev, pevAttacker, pTarget->pev->health, DamageType() | DMG_ALWAYSGIB);
			}
		}
		else
		{
			pTarget->TakeDamage(pTarget->pev, pevAttacker, pev->dmg, DamageType());
		}
	}
	else
	{
		CBaseEntity* healer = pActivator != 0 ? pActivator : this;
		if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_INSTANT_KILL))
		{
			pTarget->TakeHealth(healer, Q_max( pTarget->pev->max_health - pTarget->pev->health, 0 ), DamageType());
		}
		else
		{
			pTarget->TakeHealth(healer, -pev->dmg, DamageType());
		}
	}
}
#endif

#if FEATURE_TRIGGER_ENTITY_ITERATOR
class CTriggerEntityIterator : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	void Iterate(CBaseEntity* pEntity);

	string_t m_nameFilter;
	string_t m_classnameFilter;
	short m_statusFilter;
	short m_triggerState;
	string_t m_triggerAfterRun;
	int m_maxRuns;
};

LINK_ENTITY_TO_CLASS( trigger_entity_iterator, CTriggerEntityIterator )

TYPEDESCRIPTION	CTriggerEntityIterator::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerEntityIterator, m_nameFilter, FIELD_STRING ),
	DEFINE_FIELD( CTriggerEntityIterator, m_classnameFilter, FIELD_STRING ),
	DEFINE_FIELD( CTriggerEntityIterator, m_statusFilter, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerEntityIterator, m_triggerState, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerEntityIterator, m_triggerAfterRun, FIELD_STRING ),
	DEFINE_FIELD( CTriggerEntityIterator, m_maxRuns, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerEntityIterator, CPointEntity )

void CTriggerEntityIterator::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "name_filter"))
	{
		m_nameFilter = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "classname_filter"))
	{
		m_classnameFilter = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "status_filter"))
	{
		m_statusFilter = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		m_triggerState = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "trigger_after_run"))
	{
		m_triggerAfterRun = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "maximum_runs"))
	{
		m_maxRuns = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerEntityIterator::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pEntity;
	if (!FStringNull(m_nameFilter))
	{
		pEntity = NULL;
		while ( (pEntity = UTIL_FindEntityByTargetname( pEntity, STRING( m_nameFilter ) )) != NULL )
		{
			if (FStringNull(m_classnameFilter) || FClassnameIs(pEntity->pev, STRING(m_classnameFilter)))
			{
				Iterate(pEntity);
			}
		}
	}
	else if (!FStringNull(m_classnameFilter))
	{
		pEntity = NULL;
		while ( (pEntity = UTIL_FindEntityByClassname( pEntity, STRING( m_classnameFilter ) )) != NULL )
		{
			if (FStringNull(m_nameFilter) ||
					(!FStringNull(pEntity->pev->targetname) && FStrEq(STRING(pEntity->pev->targetname), STRING(m_nameFilter))) )
			{
				Iterate(pEntity);
			}
		}
	}

	if (!FStringNull(m_triggerAfterRun))
	{
		FireTargets(STRING(m_triggerAfterRun), pActivator, this, USE_TOGGLE, 0.0f);
	}

	if (m_maxRuns > 0)
	{
		m_maxRuns--;
		if (m_maxRuns == 0)
			UTIL_Remove(this);
	}
}

void CTriggerEntityIterator::Iterate(CBaseEntity *pEntity)
{
	if (m_statusFilter)
	{
		if (m_statusFilter == 1)
		{
			if (pEntity->pev->deadflag != DEAD_NO)
				return;
		}
		else if (m_statusFilter == 2)
		{
			if (pEntity->pev->deadflag == DEAD_NO)
				return;
		}
	}

	USE_TYPE useType;
	switch (m_triggerState) {
	case 0:
		useType = USE_OFF;
		break;
	case 1:
		useType = USE_ON;
		break;
	default:
		useType = USE_TOGGLE;
		break;
	}
	SUB_UseTargets(pEntity, useType, 0.0f);
}
#endif
