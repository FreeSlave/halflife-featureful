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
#include "game.h"
#include "skill.h"
#include "monsters.h"
#include "talkmonster.h"
#include "locus.h"
#include "common_soundscripts.h"
#include "string_utils.h"

#define FEATURE_TRIGGER_RANDOM 1
#define FEATURE_TRIGGER_RESPAWN 1
#define FEATURE_TRIGGER_KILL_MONSTER 1
#define FEATURE_TRIGGER_TIMER 1
#define FEATURE_TRIGGER_CHANGE_VALUE 1
#define FEATURE_TRIGGER_COMMAND 1
#define FEATURE_TRIGGER_MOTION 1
#define FEATURE_TRIGGER_HURT_REMOTE 1
#define FEATURE_TRIGGER_ENTITY_ITERATOR 1

#define SF_TRIGGER_PUSH_ONCE		1
#define SF_TRIGGER_PUSH_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_PUSH_NO_CLIENTS	8
#define SF_TRIGGER_PUSH_NO_MONSTERS	16

#define SF_TRIGGER_HURT_TARGETONCE	1// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF	2//spawnflag that makes trigger_hurt spawn turned OFF
#define	SF_TRIGGER_HURT_NO_CLIENTS	8//don't trigger on clients
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32// only clients may touch this trigger.
#define SF_TRIGGER_HURT_AFFECT_NON_MOVING_MONSTERS 64 // hack to affect non-moving monsters
#define SF_TRIGGER_HURT_FULL_DAMAGE_EVERY_HALF_SECOND 128
#define SF_TRIGGER_HURT_IGNORE_ARMOR 256
#define SF_TRIGGER_HURT_NO_PUNCH 512

extern DLL_GLOBAL bool		g_fGameOver;

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
		pkvd->fHandled = true;
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
		pkvd->fHandled = true;
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
		pkvd->fHandled = true;
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
		SUB_UseTargets( this, triggerType );
		if( pev->spawnflags & SF_AUTO_FIREONCE )
			UTIL_Remove( this );
	}
}

#define SF_RELAY_FIREONCE		0x0001
#define SF_RELAY_FORWARDACTIVATOR		0x0040
#define SF_RELAY_PASS_CALLER_AS_ACTIVATOR		0x0080

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

protected:
	USE_TYPE MyTriggerType(USE_TYPE useType);

private:
	int triggerType;
	float m_flDelayBeforeReset;
	float m_flResetTime;
};

LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay )

TYPEDESCRIPTION	CTriggerRelay::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerRelay, triggerType, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRelay, m_flDelayBeforeReset, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerRelay, m_flResetTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CTriggerRelay, CBaseDelay )

void CTriggerRelay::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		triggerType = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flDelayBeforeReset" ) )
	{
		m_flDelayBeforeReset = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerRelay::Spawn( void )
{
}

void CTriggerRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_flDelayBeforeReset > 0 && m_flResetTime > gpGlobals->time)
	{
		return;
	}
	CBaseEntity* targetActivator = this;
	if (FBitSet(pev->spawnflags, SF_RELAY_PASS_CALLER_AS_ACTIVATOR))
	{
		targetActivator = pCaller;
	}
	else if (FBitSet(pev->spawnflags, SF_RELAY_FORWARDACTIVATOR))
	{
		targetActivator = pActivator;
	}
	SUB_UseTargets( targetActivator, MyTriggerType(useType) );
	if (m_flDelayBeforeReset > 0) {
		m_flResetTime = gpGlobals->time + m_flDelayBeforeReset;
	}
	if( pev->spawnflags & SF_RELAY_FIREONCE )
		UTIL_Remove( this );
}

USE_TYPE CTriggerRelay::MyTriggerType(USE_TYPE useType)
{
	switch( triggerType )
	{
	case -2:
	{
		if (useType == USE_OFF)
			return USE_ON;
		else if (useType == USE_ON)
			return USE_OFF;
		return useType;
	}
	case -1:
		return useType;
	case 0:
		return USE_OFF;
	case 2:
		return USE_TOGGLE;
	default:
		return USE_ON;
	}
}

//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE		0x80000000
#define SF_MULTIMAN_THREAD		0x00000001
#define SF_MULTIMAN_ONLYONCE	0x00000008
#define SF_MULTIMAN_INSTANT		0x00000400

enum
{
	MM_USE_TOGGLE = 0,
	MM_USE_ON,
	MM_USE_OFF,
	MM_USE_KILL,
	MM_USE_FORWARD,
	MM_USE_REVERSE,
};

static void ParseMMDelay(const char* value, float& delay, short& mmUseType)
{
	char* endPtr;
	delay = (float)strtod(value, &endPtr);
	mmUseType = MM_USE_TOGGLE;
	if (*endPtr == '#')
	{
		endPtr++;
		const char digit = *endPtr;
		if (digit == '0' || strcmp(endPtr, "off") == 0)
		{
			mmUseType = MM_USE_OFF;
		}
		else if (digit == '1' || strcmp(endPtr, "on") == 0)
		{
			mmUseType = MM_USE_ON;
		}
		else if (digit == '2' || strcmp(endPtr, "kill") == 0)
		{
			mmUseType = MM_USE_KILL;
		}
		else if (strcmp(endPtr, "toggle") == 0)
		{
			mmUseType = MM_USE_TOGGLE;
		}
		else if (strcmp(endPtr, "forward") == 0)
		{
			mmUseType = MM_USE_FORWARD;
		}
		else if (strcmp(endPtr, "reverse") == 0)
		{
			mmUseType = MM_USE_REVERSE;
		}
	}
}

#define MAX_MULTI_TARGETS	16 // maximum number of targets a single multi_manager entity may be assigned.

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
	bool HasTarget( string_t targetname ) override;

	int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	int m_cTargets; // the total number of targets in this manager's fire list.
	int m_index;	// Current target
	float m_startTime;// Time we started firing
	string_t m_iTargetName[MAX_MULTI_TARGETS];// list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS];// delay (in seconds) from time of manager fire to target fire
	short m_iTargetUseType[MAX_MULTI_TARGETS];
	int m_inputUseType;
private:
	inline bool IsClone() { return FBitSet(pev->spawnflags, SF_MULTIMAN_CLONE); }
	inline bool ShouldClone()
	{
		if( IsClone() )
			return false;

		return FBitSet(pev->spawnflags, SF_MULTIMAN_THREAD);
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
	DEFINE_ARRAY( CMultiManager, m_iTargetUseType, FIELD_SHORT, MAX_MULTI_TARGETS ),
	DEFINE_FIELD( CMultiManager, m_inputUseType, FIELD_INTEGER ),
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
		pkvd->fHandled = true;
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if( m_cTargets < MAX_MULTI_TARGETS )
		{
			char tmp[128];

			UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ));
			m_iTargetName[m_cTargets] = ALLOC_STRING( tmp );
			float delay;
			short mmUseType;
			ParseMMDelay(pkvd->szValue, delay, mmUseType);
			m_flTargetDelay[m_cTargets] = delay;
			m_iTargetUseType[m_cTargets] = mmUseType;
			m_cTargets++;
			pkvd->fHandled = true;
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
				string_t name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				short useType = m_iTargetUseType[i];

				m_iTargetName[i] = m_iTargetName[i - 1];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_iTargetUseType[i] = m_iTargetUseType[i - 1];

				m_iTargetName[i - 1] = name;
				m_flTargetDelay[i - 1] = delay;
				m_iTargetUseType[i - 1] = useType;
				swapped = 1;
			}
		}
	}
}

bool CMultiManager::HasTarget( string_t targetname )
{
	for( int i = 0; i < m_cTargets; i++ )
		if( FStrEq( STRING( targetname ), STRING( m_iTargetName[i] ) ) )
			return true;

	return false;
}

// Designers were using this to fire targets that may or may not exist --
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager::ManagerThink( void )
{
	float time;

	time = gpGlobals->time - m_startTime;
	while( m_index < m_cTargets && m_flTargetDelay[m_index] <= time )
	{
		short mmUseType = m_iTargetUseType[m_index];
		if (mmUseType == MM_USE_KILL)
		{
			KillTargets(STRING(m_iTargetName[m_index]));
		}
		else
		{
			USE_TYPE useType = USE_TOGGLE;
			if (mmUseType == MM_USE_OFF)
				useType = USE_OFF;
			else if (mmUseType == MM_USE_ON)
				useType = USE_ON;
			else if (mmUseType == MM_USE_FORWARD)
				useType = (USE_TYPE)m_inputUseType;
			else if (mmUseType == MM_USE_REVERSE)
			{
				if (m_inputUseType == USE_ON)
					useType = USE_OFF;
				else if (m_inputUseType == USE_OFF)
					useType = USE_ON;
			}
			FireTargets( STRING( m_iTargetName[m_index] ), m_hActivator, this, useType );
		}
		m_index++;
	}

	if( m_index >= m_cTargets )// have we fired all targets?
	{
		SetThink( NULL );
		if( IsClone() || FBitSet(pev->spawnflags, SF_MULTIMAN_ONLYONCE) )
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

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE | (pev->spawnflags & SF_MULTIMAN_INSTANT);
	pMulti->m_cTargets = m_cTargets;
	memcpy( pMulti->m_iTargetName, m_iTargetName, sizeof( m_iTargetName ) );
	memcpy( pMulti->m_flTargetDelay, m_flTargetDelay, sizeof( m_flTargetDelay ) );
	memcpy( pMulti->m_iTargetUseType, m_iTargetUseType, sizeof( m_iTargetUseType ) );
	pMulti->m_inputUseType = m_inputUseType;

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
	m_inputUseType = useType;

	SetUse( NULL );// disable use until all targets have fired

	SetThink( &CMultiManager::ManagerThink );
	if (FBitSet(pev->spawnflags, SF_MULTIMAN_INSTANT) && m_cTargets > 0 && m_flTargetDelay[0] <= 0.0f)
	{
		ManagerThink();
	}
	else
	{
		pev->nextthink = gpGlobals->time;
	}
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
		pkvd->fHandled = true;
	}
	else if ( strncmp(pkvd->szKeyName, "delay", 5) == 0 && isdigit(pkvd->szKeyName[5]))
	{
		num = atoi(pkvd->szKeyName + 5);
		if (num <= 0 || num > MAX_MULTI_TARGETS)
			return;
		index = num - 1;
		float delay;
		short mmUseType;
		ParseMMDelay(pkvd->szValue, delay, mmUseType);
		m_flTargetDelay[index] = delay;
		m_iTargetUseType[index] = mmUseType;
		pkvd->fHandled = true;
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
					m_iTargetUseType[i] = m_iTargetUseType[j];
					m_iTargetName[j] = iStringNull;
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

class CMultiSequence : public CMultiTrigger
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( multi_sequence, CMultiSequence )

void CMultiSequence::Spawn()
{
	int i;
	for (i=0; i<MAX_MULTI_TARGETS; ++i)
	{
		if (!m_iTargetName[i])
		{
			for (int j=i+1; j<MAX_MULTI_TARGETS; ++j)
			{
				if (m_iTargetName[j])
				{
					m_iTargetName[i] = m_iTargetName[j];
					m_flTargetDelay[i] = m_flTargetDelay[j];
					m_iTargetUseType[i] = m_iTargetUseType[j];
					m_iTargetName[j] = iStringNull;
					break;
				}
			}
		}
		if (m_iTargetName[i])
			m_cTargets = i+1;
		else
			break;
	}

	// Adjust delays so they work as in multi_manager
	for (i=1; i<m_cTargets; ++i)
	{
		m_flTargetDelay[i] += m_flTargetDelay[i-1];
	}

	pev->solid = SOLID_NOT;
	SetUse( &CMultiManager::ManagerUse );
	SetThink( &CMultiManager::ManagerThink );

	//for (int i=0; i<m_cTargets; ++i) {
	//	ALERT(at_console, "Target: %s. Adjusted delay: %f\n", STRING(m_iTargetName[i]), m_flTargetDelay[i]);
	//}

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
//LRC
#define SF_RENDER_KILLTARGET	(1<<5)
#define SF_RENDER_ONLYONCE		(1<<6)


//LRC-  RenderFxFader, a subsidiary entity for RenderFxManager
class CRenderFxFader : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT FadeThink ( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];

	float m_flStartTime;
	float m_flDuration;
	float m_flCoarseness;
	int m_iStartAmt;
	int m_iOffsetAmt;
	Vector m_vecStartColor;
	Vector m_vecOffsetColor;
	float m_fStartScale;
	float m_fOffsetScale;
	EHANDLE m_hTarget;

	int m_iszAmtFactor;
};
LINK_ENTITY_TO_CLASS( render_fader, CRenderFxFader )

TYPEDESCRIPTION	CRenderFxFader::m_SaveData[] =
{
	DEFINE_FIELD( CRenderFxFader, m_flStartTime, FIELD_FLOAT),
	DEFINE_FIELD( CRenderFxFader, m_flDuration, FIELD_FLOAT),
	DEFINE_FIELD( CRenderFxFader, m_flCoarseness, FIELD_FLOAT),
	DEFINE_FIELD( CRenderFxFader, m_iStartAmt, FIELD_INTEGER),
	DEFINE_FIELD( CRenderFxFader, m_iOffsetAmt, FIELD_INTEGER ),
	DEFINE_FIELD( CRenderFxFader, m_vecStartColor, FIELD_VECTOR ),
	DEFINE_FIELD( CRenderFxFader, m_vecOffsetColor, FIELD_VECTOR ),
	DEFINE_FIELD( CRenderFxFader, m_fStartScale, FIELD_FLOAT),
	DEFINE_FIELD( CRenderFxFader, m_fOffsetScale, FIELD_FLOAT ),
	DEFINE_FIELD( CRenderFxFader, m_hTarget, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE(CRenderFxFader, CBaseEntity)

void CRenderFxFader::Spawn( void )
{
	SetThink(&CRenderFxFader::FadeThink );
	pev->classname = MAKE_STRING("render_fader");
}

void CRenderFxFader::FadeThink( void )
{
	if (m_hTarget == 0)
	{
//		ALERT(at_console, "render_fader removed\n");
		SUB_Remove();
		return;
	}

	float flDegree = (gpGlobals->time - m_flStartTime)/m_flDuration;

	if (flDegree >= 1)
	{
//		ALERT(at_console, "render_fader removes self\n");

		m_hTarget->pev->renderamt = m_iStartAmt + m_iOffsetAmt;
		m_hTarget->pev->rendercolor = m_vecStartColor + m_vecOffsetColor;
		m_hTarget->pev->scale = m_fStartScale + m_fOffsetScale;

		SUB_UseTargets( m_hTarget );

		if (pev->spawnflags & SF_RENDER_KILLTARGET)
		{
			m_hTarget->SetThink(&CRenderFxFader::SUB_Remove);
			m_hTarget->pev->nextthink = gpGlobals->time + 0.1f;
		}

		m_hTarget = 0;

		pev->nextthink = gpGlobals->time + 0.1f;
		SetThink(&CRenderFxFader::SUB_Remove);
	}
	else
	{
		m_hTarget->pev->renderamt = m_iStartAmt + m_iOffsetAmt * flDegree;

		m_hTarget->pev->rendercolor.x = m_vecStartColor.x + m_vecOffsetColor.x * flDegree;
		m_hTarget->pev->rendercolor.y = m_vecStartColor.y + m_vecOffsetColor.y * flDegree;
		m_hTarget->pev->rendercolor.z = m_vecStartColor.z + m_vecOffsetColor.z * flDegree;

		m_hTarget->pev->scale = m_fStartScale + m_fOffsetScale * flDegree;

		pev->nextthink = gpGlobals->time + m_flCoarseness; //?
	}
}

class CRenderFxManager : public CBaseEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Affect(CBaseEntity *pEntity, bool bIsLocus, CBaseEntity *pActivator );
	void KeyValue( KeyValueData *pkvd );
};

LINK_ENTITY_TO_CLASS( env_render, CRenderFxManager )

void CRenderFxManager::Spawn( void )
{
	pev->solid = SOLID_NOT;
}

void CRenderFxManager :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_fScale"))
{
		pev->scale = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CRenderFxManager::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !FStringNull( pev->target ) )
	{
		CBaseEntity* pTarget = UTIL_FindEntityByTargetname( NULL, STRING(pev->target), pActivator);
		bool first = true;
		while ( pTarget )
		{
			Affect( pTarget, first, pActivator );
			first = false;
			pTarget = UTIL_FindEntityByTargetname( pTarget, STRING(pev->target), pActivator );
		}
	}

	if (pev->spawnflags & SF_RENDER_ONLYONCE)
	{
		SetThink(&CRenderFxManager::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CRenderFxManager::Affect( CBaseEntity *pTarget, bool bIsFirst, CBaseEntity *pActivator )
{
	entvars_t *pevTarget = pTarget->pev;

	float fAmtFactor = 1;
	if ( pev->message && !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
		TryCalcLocus_Ratio(pActivator, STRING(pev->message), fAmtFactor);

	if( !FBitSet( pev->spawnflags, SF_RENDER_MASKFX ) )
		pevTarget->renderfx = pev->renderfx;
	if( !FBitSet( pev->spawnflags, SF_RENDER_MASKMODE ) )
	{
		//LRC - amt is often 0 when mode is normal. Set it to be fully visible, for fade purposes.
		if (pev->frags && pevTarget->renderamt == 0 && pevTarget->rendermode == kRenderNormal)
			pevTarget->renderamt = 255;
		pevTarget->rendermode = pev->rendermode;
	}
	if (pev->frags == 0) // not fading?
	{
		if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
			pevTarget->renderamt = pev->renderamt * fAmtFactor;
		if( !FBitSet( pev->spawnflags, SF_RENDER_MASKCOLOR ) )
			pevTarget->rendercolor = pev->rendercolor;
		if ( pev->scale )
			pevTarget->scale = pev->scale;

		if (bIsFirst)
			FireTargets( STRING(pev->netname), pTarget, this );
	}
	else
	{
		//LRC - fade the entity in/out!
		// (We create seperate fader entities to do this, one for each entity that needs fading.)
		CRenderFxFader *pFader = GetClassPtr( (CRenderFxFader *)NULL );
		pFader->m_hTarget = pTarget;
		pFader->m_iStartAmt = pevTarget->renderamt;
		pFader->m_vecStartColor = pevTarget->rendercolor;
		pFader->m_fStartScale = pevTarget->scale;
		if (pFader->m_fStartScale == 0)
			pFader->m_fStartScale = 1; // When we're scaling, 0 is treated as 1. Use 1 as the number to fade from.
		pFader->pev->spawnflags = pev->spawnflags;

		if (bIsFirst)
			pFader->pev->target = pev->netname;

		if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
			pFader->m_iOffsetAmt = (pev->renderamt * fAmtFactor) - pevTarget->renderamt;
		else
			pFader->m_iOffsetAmt = 0;

		if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKCOLOR ) )
		{
			pFader->m_vecOffsetColor.x = pev->rendercolor.x - pevTarget->rendercolor.x;
			pFader->m_vecOffsetColor.y = pev->rendercolor.y - pevTarget->rendercolor.y;
			pFader->m_vecOffsetColor.z = pev->rendercolor.z - pevTarget->rendercolor.z;
		}
		else
		{
			pFader->m_vecOffsetColor = g_vecZero;
		}

		if ( pev->scale )
			pFader->m_fOffsetScale = pev->scale - pevTarget->scale;
		else
			pFader->m_fOffsetScale = 0;

		pFader->m_flStartTime = gpGlobals->time;
		pFader->m_flDuration = pev->frags;
		pFader->m_flCoarseness = pev->armorvalue;
		pFader->pev->nextthink = gpGlobals->time;
		pFader->Spawn();
	}
}

class CBaseTrigger : public CBaseToggle
{
public:
	void KeyValue( KeyValueData *pkvd );
	void EXPORT MultiTouch( CBaseEntity *pOther );
	void EXPORT CDAudioTouch( CBaseEntity *pOther );
	void ActivateMultiTrigger( CBaseEntity *pActivator );
	void EXPORT MultiWaitOver( void );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void InitTrigger( void );
	bool CanTouch( entvars_t *pevToucher );

	virtual int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual USE_TYPE UseType() { return USE_TOGGLE; }
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
			return !FBitSet(pev->spawnflags, SF_TRIGGER_NOCLIENTS);
		else if (pevToucher->flags & FL_MONSTER)
			return FBitSet(pev->spawnflags, SF_TRIGGER_ALLOWMONSTERS);
		else if (FClassnameIs(pevToucher,"func_pushable"))
			return FBitSet(pev->spawnflags, SF_TRIGGER_PUSHABLES);
		else
			return FBitSet(pev->spawnflags, SF_TRIGGER_EVERYTHING);
	}
	else
	{
		// If netname is set, it's an entity-specific trigger; we ignore the spawnflags.
		return UTIL_HasClassnameOrTargetname( pevToucher, STRING(pev->netname) );
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
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "damagetype" ) )
	{
		m_bitsDamageInflict = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

class CTriggerHurt : public CBaseTrigger
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void EXPORT HurtTouch( CBaseEntity *pOther );
	void EXPORT HurtToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT RadiationThink( void );
	void HurtNonMovingMonsters( void );
	void EXPORT HurtNonMovingMonstersThink( void );
	bool CanHurt( CBaseEntity* pOther );
	virtual float DamageAmount();
	void DoDamage( CBaseEntity* pTarget, float fldmg );
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
		pkvd->fHandled = true;
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

	if( ( pClient->v.origin - pev->origin ).IsLengthLessThanOrEqual(pev->scale) )
		Play();
}

void CTargetCDAudio::Play( void )
{
	PlayCDTrack( (int)pev->health );
	UTIL_Remove( this );
}

#define SF_TRIGGER_MP3_AUDIO_REMOVE_ON_FIRE (1 << 0)
#define SF_TRIGGER_MP3_AUDIO_LOOPED (1 << 1)
#define SF_TRIGGER_MP3_AUDIO_PLAYING (1 << 24)

extern int gmsgPlayMP3;

class CTriggerMp3Audio : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

protected:
	void StopMp3(CBasePlayer* pPlayer);
};

LINK_ENTITY_TO_CLASS( trigger_mp3audio, CTriggerMp3Audio )

void CTriggerMp3Audio::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}

void CTriggerMp3Audio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
	if( !pPlayer || !pPlayer->IsNetClient())
		return;

	if (FStringNull(pev->message))
	{
		StopMp3(pPlayer);
		return;
	}

	bool shouldStop = false;
	if (useType == USE_OFF)
	{
		shouldStop = true;
	}
	else if (useType != USE_ON)
	{
		shouldStop = FBitSet(pev->spawnflags, SF_TRIGGER_MP3_AUDIO_PLAYING);
	}

	if (shouldStop)
	{
		ClearBits(pev->spawnflags, SF_TRIGGER_MP3_AUDIO_PLAYING);
		StopMp3(pPlayer);
		return;
	}

	SetBits(pev->spawnflags, SF_TRIGGER_MP3_AUDIO_PLAYING);
	const int looped = FBitSet(pev->spawnflags, SF_TRIGGER_MP3_AUDIO_LOOPED);

	if (gmsgPlayMP3)
	{
		// issue the play/loop command
		MESSAGE_BEGIN( MSG_ONE, gmsgPlayMP3, NULL, pPlayer->edict() );
			WRITE_STRING( STRING( pev->message ) );
			WRITE_BYTE( looped );
		MESSAGE_END();
	}

	if (looped)
		pPlayer->SetLoopedMp3(pev->message);
	else
		pPlayer->SetLoopedMp3(iStringNull);

	// remove if set
	if( FBitSet( pev->spawnflags, SF_TRIGGER_MP3_AUDIO_REMOVE_ON_FIRE ) )
		UTIL_Remove( this );
}

void CTriggerMp3Audio::StopMp3(CBasePlayer *pPlayer)
{
	if (gmsgPlayMP3)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgPlayMP3, NULL, pPlayer->edict() );
			WRITE_STRING( "" );
			WRITE_BYTE( 0 );
		MESSAGE_END();
	}
	pPlayer->SetLoopedMp3(iStringNull);
}

//=====================================
//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

void CTriggerHurt::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "untouchable_ent_name" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "gib_policy" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "min_health" ) )
	{
		pev->dmg_save = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

void CTriggerHurt::Spawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerHurt::HurtTouch );

	if( !FStringNull( pev->targetname ) )
	{
		SetUse( &CTriggerHurt::HurtToggleUse );
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
	else if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_AFFECT_NON_MOVING_MONSTERS))
	{
		SetThink( &CTriggerHurt::HurtNonMovingMonstersThink );
		pev->nextthink = gpGlobals->time + 0.1f;
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

void CTriggerHurt::HurtNonMovingMonsters()
{
	if (pev->solid == SOLID_NOT || pev->dmgtime > gpGlobals->time) {
		return;
	}

	CBaseEntity* pList[32];
	const int count = UTIL_EntitiesInBox( pList, ARRAYSIZE(pList), pev->absmin, pev->absmax, FL_MONSTER );
	for (int i=0; i<count; ++i) {
		CBaseMonster* pMonster = pList[i]->MyMonsterPointer();
		if (pMonster && CanHurt(pMonster) && !pMonster->IsMoving()) {
			const float flDmg = FBitSet(pev->spawnflags, SF_TRIGGER_HURT_FULL_DAMAGE_EVERY_HALF_SECOND) ? pev->dmg : pev->dmg * 0.5f;
			DoDamage(pMonster, flDmg);
		}
	}
}

void CTriggerHurt::HurtNonMovingMonstersThink()
{
	if (pev->dmgtime > gpGlobals->time) {
		pev->nextthink = Q_min(gpGlobals->time + 0.5f, pev->dmgtime);
	} else {
		pev->nextthink = gpGlobals->time + 0.5f;
	}

	HurtNonMovingMonsters();
}

void CTriggerHurt::HurtToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	ToggleUse(pActivator, pCaller, useType, value);
	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_AFFECT_NON_MOVING_MONSTERS))
	{
		pev->nextthink = gpGlobals->time;
	}
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

bool CTriggerHurt::CanHurt(CBaseEntity *pOther)
{
	if( !pOther->pev->takedamage )
		return false;

	if( ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH ) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return false;
	}

	if( ( pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS ) && pOther->IsPlayer() )
		return false;

	// ignore target with a specified name
	if ( !FStringNull( pev->netname ) && UTIL_HasClassnameOrTargetname( pOther->pev, STRING(pev->netname) ) ) {
		return false;
	}
	return true;
}

float CTriggerHurt::DamageAmount()
{
	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_FULL_DAMAGE_EVERY_HALF_SECOND))
		return pev->dmg;
	else
		return pev->dmg * 0.5f;	// 0.5 seconds worth of damage, pev->dmg is damage/second
}

void CTriggerHurt::DoDamage(CBaseEntity *pTarget, float fldmg)
{
	if( fldmg < 0 )
	{
		if( !( g_pGameRules->IsMultiplayer() && pTarget->IsPlayer() && pTarget->pev->deadflag ))
			pTarget->TakeHealth( this, -fldmg, HEAL_GENERIC );
	}
	else
	{
		DamageInfo damageInfo{fldmg, m_bitsDamageInflict};
		switch (pev->impulse) {
		case 1:
			damageInfo.SetGibPolicy(GIB_ALWAYS);
			break;
		case 2:
			damageInfo.SetGibPolicy(GIB_NEVER);
			break;
		default:
			break;
		}

		if (pev->spawnflags & SF_TRIGGER_HURT_IGNORE_ARMOR)
			damageInfo.SetIgnoreArmor();
		if (pev->spawnflags & SF_TRIGGER_HURT_NO_PUNCH)
			damageInfo.SetNoPunch();

		const float minHealthThreshold = pev->dmg_save;
		if (minHealthThreshold > 0) {
			pTarget->m_healthMinThreshold = minHealthThreshold;
		}

		pTarget->TakeDamage( pev, pev, damageInfo );
	}
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CTriggerHurt::HurtTouch( CBaseEntity *pOther )
{
	if ( !CanHurt(pOther) )
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

	float fldmg = DamageAmount();

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
	DoDamage(pOther, fldmg);

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

		SUB_UseTargets( pOther );
		if( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}

// trigger_multiple
enum
{
	TRIGGER_USE_OFF = -1,
	TRIGGER_USE_TOGGLE = 0,
	TRIGGER_USE_ON = 1,
};

class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );

	USE_TYPE UseType();
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

void CTriggerMultiple::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "usetype" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

USE_TYPE CTriggerMultiple::UseType()
{
	if (pev->impulse < 0)
		return USE_OFF;
	else if (pev->impulse > 0)
		return USE_ON;
	else
		return USE_TOGGLE;
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

	if( !FStringNull( pev->noise ) )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM );

	// don't trigger again until reset
	// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, UseType() );

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

//===========================================================
//LRC - trigger_inout, a trigger which fires _only_ when
// the player enters or leaves it.
//   If there's more than one entity it can trigger off, then
// it will trigger for each one that enters and leaves.
//===========================================================
class CTriggerInOut;

class CInOutRegister : public CPointEntity
{
public:
	// returns true if found in the list
	bool IsRegistered(CBaseEntity* pValue);
	// remove all invalid entries from the list, trigger their targets as appropriate
	// returns the new list
	CInOutRegister* Prune();
	// adds a new entry to the list
	CInOutRegister* Add(CBaseEntity* pValue);
	bool IsEmpty() { return !m_pNext; }

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	CTriggerInOut* m_pField;
	CInOutRegister* m_pNext;
	EHANDLE m_hValue;
};

class CTriggerInOut : public CBaseTrigger
{
public:
	void Spawn() override;
	void UpdateOnRemove() override;
	void EXPORT Touch(CBaseEntity* pOther) override;
	void EXPORT Think() override;
	void FireOnEntry(CBaseEntity* pOther);
	void FireOnLeaving(CBaseEntity* pOther);

	void KeyValue(KeyValueData* pkvd) override;
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszAltTarget;
	string_t m_iszBothTarget;
	CInOutRegister* m_pRegister;
};

TYPEDESCRIPTION CInOutRegister::m_SaveData[] =
{
	DEFINE_FIELD(CInOutRegister, m_pField, FIELD_CLASSPTR),
	DEFINE_FIELD(CInOutRegister, m_pNext, FIELD_CLASSPTR),
	DEFINE_FIELD(CInOutRegister, m_hValue, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CInOutRegister, CPointEntity)
LINK_ENTITY_TO_CLASS(inout_register, CInOutRegister)

bool CInOutRegister::IsRegistered(CBaseEntity* pValue)
{
	if (m_hValue == pValue)
		return true;
	else if (m_pNext)
		return m_pNext->IsRegistered(pValue);
	else
		return false;
}

CInOutRegister* CInOutRegister::Add(CBaseEntity* pValue)
{
	if (m_hValue == pValue)
	{
		// it's already in the list, don't need to do anything
		return this;
	}
	else if (m_pNext)
	{
		// keep looking
		m_pNext = m_pNext->Add(pValue);
		return this;
	}
	else
	{
		// reached the end of the list; add the new entry, and trigger
		CInOutRegister* pResult = GetClassPtr((CInOutRegister*)NULL);
		pResult->m_hValue = pValue;
		pResult->m_pNext = this;
		pResult->m_pField = m_pField;
		pResult->pev->classname = MAKE_STRING("inout_register");

		//		ALERT(at_console, "adding; max %.2f %.2f %.2f, min %.2f %.2f %.2f is inside max %.2f %.2f %.2f, min %.2f %.2f %.2f\n", pResult->m_hValue->pev->absmax.x, pResult->m_hValue->pev->absmax.y, pResult->m_hValue->pev->absmax.z, pResult->m_hValue->pev->absmin.x, pResult->m_hValue->pev->absmin.y, pResult->m_hValue->pev->absmin.z, pResult->m_pField->pev->absmax.x, pResult->m_pField->pev->absmax.y, pResult->m_pField->pev->absmax.z, pResult->m_pField->pev->absmin.x, pResult->m_pField->pev->absmin.y, pResult->m_pField->pev->absmin.z); //LRCT

		m_pField->FireOnEntry(pValue);
		return pResult;
	}
}

CInOutRegister* CInOutRegister::Prune()
{
	if (m_hValue)
	{
		ASSERTSZ(m_pNext != NULL, "invalid InOut registry terminator\n");
		if (m_pField->Intersects(m_hValue))
		{
			// this entity is still inside the field, do nothing
			m_pNext = m_pNext->Prune();
			return this;
		}
		else
		{
			//			ALERT(at_console, "removing; max %.2f %.2f %.2f, min %.2f %.2f %.2f is outside max %.2f %.2f %.2f, min %.2f %.2f %.2f\n", m_hValue->pev->absmax.x, m_hValue->pev->absmax.y, m_hValue->pev->absmax.z, m_hValue->pev->absmin.x, m_hValue->pev->absmin.y, m_hValue->pev->absmin.z, m_pField->pev->absmax.x, m_pField->pev->absmax.y, m_pField->pev->absmax.z, m_pField->pev->absmin.x, m_pField->pev->absmin.y, m_pField->pev->absmin.z); //LRCT

			// this entity has just left the field, trigger
			m_pField->FireOnLeaving(m_hValue);
			SetThink(&CInOutRegister::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1f;
			return m_pNext->Prune();
		}
	}
	else
	{ // this register has a missing or null value
		if (m_pNext)
		{
			// this is an invalid list entry, remove it
			SetThink(&CInOutRegister::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1f;
			return m_pNext->Prune();
		}
		else
		{
			// this is the list terminator, leave it.
			return this;
		}
	}
}

LINK_ENTITY_TO_CLASS(trigger_inout, CTriggerInOut)

TYPEDESCRIPTION CTriggerInOut::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerInOut, m_iszAltTarget, FIELD_STRING),
	DEFINE_FIELD(CTriggerInOut, m_iszBothTarget, FIELD_STRING),
	DEFINE_FIELD(CTriggerInOut, m_pRegister, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CTriggerInOut, CBaseTrigger)

void CTriggerInOut::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszAltTarget"))
	{
		m_iszAltTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszBothTarget"))
	{
		m_iszBothTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
		CBaseTrigger::KeyValue(pkvd);
}

void CTriggerInOut::Spawn()
{
	InitTrigger();
	// create a null-terminator for the registry
	m_pRegister = GetClassPtr((CInOutRegister*)NULL);
	m_pRegister->m_hValue = NULL;
	m_pRegister->m_pNext = NULL;
	m_pRegister->m_pField = this;
	m_pRegister->pev->classname = MAKE_STRING("inout_register");
}

void CTriggerInOut::UpdateOnRemove()
{
	CInOutRegister* pRegister = m_pRegister;
	while (pRegister)
	{
		pRegister->SetThink(&CInOutRegister::SUB_Remove);
		pRegister->pev->nextthink = gpGlobals->time + 0.1f;
		pRegister = pRegister->m_pNext;
	}
}

void CTriggerInOut::Touch(CBaseEntity* pOther)
{
	if (!CanTouch(pOther->pev))
		return;

	m_pRegister = m_pRegister->Add(pOther);

	if (pev->nextthink <= 0 && !m_pRegister->IsEmpty())
		pev->nextthink = gpGlobals->time + 0.1f;
}

void CTriggerInOut::Think()
{
	// Prune handles all Intersects tests and fires targets as appropriate
	m_pRegister = m_pRegister->Prune();

	if (m_pRegister->IsEmpty())
		pev->nextthink = -1;
	else
		pev->nextthink = gpGlobals->time + 0.1f;
}

void CTriggerInOut::FireOnEntry(CBaseEntity* pOther)
{
	if (UTIL_IsMasterTriggered(m_sMaster, pOther))
	{
		FireTargets(STRING(m_iszBothTarget), pOther, this, USE_ON, 0);
		FireTargets(STRING(pev->target), pOther, this, USE_TOGGLE, 0);
	}
}

void CTriggerInOut::FireOnLeaving(CBaseEntity* pEnt)
{
	if (UTIL_IsMasterTriggered(m_sMaster, pEnt))
	{
		FireTargets(STRING(m_iszBothTarget), pEnt, this, USE_OFF, 0);
		FireTargets(STRING(m_iszAltTarget), pEnt, this, USE_TOGGLE, 0);
	}
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
	void KeyValue( KeyValueData *pkvd );
	bool CalcNumber( CBaseEntity *pLocus, float* outResult );
	void EXPORT CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int m_cTriggersLeft; // trigger_counter only, # of activations remaining
};

LINK_ENTITY_TO_CLASS( trigger_counter, CTriggerCounter )

TYPEDESCRIPTION	CTriggerCounter::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerCounter, m_cTriggersLeft, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerCounter, CBaseTrigger )

void CTriggerCounter::Spawn( void )
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if( m_cTriggersLeft <= 0 )
		m_cTriggersLeft = 2;
	SetUse( &CTriggerCounter::CounterUse );
}

void CTriggerCounter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "count" ) )
	{
		m_cTriggersLeft = (int)atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

bool CTriggerCounter::CalcNumber( CBaseEntity *pLocus, float* outResult )
{
	*outResult = static_cast<float>(m_cTriggersLeft);
	return true;
}

void CTriggerCounter::CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( m_cTriggersLeft <= 0 )
		return;

	m_cTriggersLeft--;
	if ( m_cTriggersLeft != 0 )
		return;

	m_hActivator = pActivator;
	ActivateMultiTrigger( m_hActivator );
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
	SUB_UseTargets( this );
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
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "landmark" ) )
	{
		if( strlen( pkvd->szValue ) >= cchMapNameMost )
			ALERT( at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue );
		strcpy( m_szLandmarkName, pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "changetarget" ) )
	{
		m_changeTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "changedelay" ) )
	{
		m_changeTargetDelay = atof( pkvd->szValue );
		pkvd->fHandled = true;
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
		ALERT( at_console, "a trigger_changelevel doesn't have a map\n" );

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

	CBaseEntity *pPlayer = g_pGameRules->EffectivePlayer(pActivator);
	if (!pPlayer)
	{
		ALERT(at_aiconsole, "Could not find a player who activated the changelevel, transition not allowed\n");
		return;
	}
	if (!pPlayer->IsAlive() && !g_pGameRules->IsMultiplayer())
	{
		ALERT(at_aiconsole, "The player who activated the changelevel has died, transition not allowed\n");
		return;
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
	SUB_UseTargets( pActivator );
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
								ALERT( at_error, "Too many entities across a transition!\n" );
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
	g_fGameOver = true;

	if( pChange->pev->nextthink < gpGlobals->time )
	{
		pChange->SetThink( &CChangeLevel::ExecuteChangeLevel );
		pChange->pev->nextthink = gpGlobals->time + 0.1f;
	}
}

// ============================== LADDER =======================================

#define SF_LADDER_OFF 1

class CLadder : public CBaseTrigger
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void SetEnabled(bool enabled);
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
	if (FBitSet(pev->spawnflags, SF_LADDER_OFF))
		pev->skin = CONTENTS_SOLID;
	else
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

void CLadder::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	const bool ladderState = !FBitSet(pev->spawnflags, SF_LADDER_OFF);
	if (ShouldToggle(useType, ladderState))
	{
		SetEnabled(!ladderState);
	}
}

void CLadder::SetEnabled(bool enabled)
{
	if (enabled)
	{
		pev->skin = CONTENTS_LADDER;
		ClearBits(pev->spawnflags, SF_LADDER_OFF);
	}
	else
	{
		pev->skin = CONTENTS_SOLID;
		SetBits(pev->spawnflags, SF_LADDER_OFF);
	}
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
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPushVel"))
	{
		m_iszPushVel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
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
		if (!TryCalcLocus_Velocity( this, pOther, STRING(m_iszPushVel), vecPush))
			return;
	}
	else
		vecPush = pev->movedir;

	if (!FStringNull(m_iszPushSpeed))
	{
		float factor;
		if (TryCalcLocus_Ratio( pOther, STRING(m_iszPushSpeed), factor ))
			vecPush *= factor;
	}

	if (pev->speed)
		vecPush *= pev->speed;
	else
		vecPush *= 100;

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
				vecPush += pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
			//ALERT( at_console, "Vel %f, base %f\n", pevToucher->velocity.z, pevToucher->basevelocity.z );
		}
	}
}

//===========================================================
//LRC- trigger_bounce
//===========================================================
#define SF_BOUNCE_CUTOFF 16

class CTriggerBounce : public CBaseTrigger
{
public:
	void Spawn();
	void Touch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS( trigger_bounce, CTriggerBounce )


void CTriggerBounce::Spawn()
{
	SetMovedir(pev);
	InitTrigger();
}

void CTriggerBounce::Touch(CBaseEntity *pOther)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;
	if (!CanTouch(pOther->pev))
		return;

	float dot = DotProduct(pev->movedir, pOther->pev->velocity);
	if (dot < -pev->armorvalue)
	{
		if (FBitSet(pev->spawnflags, SF_BOUNCE_CUTOFF))
			pOther->pev->velocity -= (dot + pev->frags*(dot+pev->armorvalue))*pev->movedir;
		else
			pOther->pev->velocity -= (dot + pev->frags*dot)*pev->movedir;
		SUB_UseTargets(pOther);
	}
}

//======================================
// teleport trigger
//
//

#define SF_TELEPORT_RANDOM_DESTINATION 64
#define SF_TELEPORT_RELATIVE_TELEPORT 128
#define SF_TELEPORT_KEEPANGLES 256
#define SF_TELEPORT_KEEPVELOCITY 512

// info_teleport_destination flags
#define SF_TELEPORT_DESTINATION_TRIGGER_ON_ARRIVAL 32

class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT TeleportTouch( CBaseEntity *pOther );
	void EXPORT TeleportUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual edict_t* GetTeleportTarget(CBaseEntity *pToucher);
	virtual CBaseEntity* GetLandmark();
	bool TeleportToDestination( CBaseEntity *pOther );
	void LetGoRope(CBaseEntity* pOther);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	bool m_fInactive;
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
		pkvd->fHandled = true;
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

bool CTriggerTeleport::TeleportToDestination( CBaseEntity *pOther )
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

	pentTarget = GetTeleportTarget(pOther);
	if( FNullEnt( pentTarget ) )
		return false;

	//LRC - landmark based teleports
	CBaseEntity* pLandmark = GetLandmark();
	if (pLandmark)
	{
		CBaseEntity* pTarget = CBaseEntity::Instance(pentTarget);
		Vector vecOriginOffs = pTarget->pev->origin - pLandmark->pev->origin;

		// do we need to rotate the entity?
		if (pLandmark->pev->angles != pTarget->pev->angles)
		{
			float ydiff = pTarget->pev->angles.y - pLandmark->pev->angles.y;

			// set new angle to face
			pOther->pev->angles.y = UTIL_AngleMod(pOther->pev->angles.y + ydiff);
			if (pOther->IsPlayer())
			{
				pOther->pev->angles.x = pOther->pev->v_angle.x;
				pOther->pev->angles.z = pOther->pev->v_angle.z = 0;
				pOther->pev->fixangle = 1;
			}

			// set new velocity
			Vector vecVA = UTIL_VecToAngles(pOther->pev->velocity);
			vecVA.y += ydiff;
			UTIL_MakeVectors(vecVA);
			pOther->pev->velocity = gpGlobals->v_forward * pOther->pev->velocity.Length();
			// fix the ugly "angle to vector" behaviour - a legacy from Quake
			pOther->pev->velocity.z = -pOther->pev->velocity.z;

			// set new origin
			Vector vecPlayerOffs = pOther->pev->origin - pLandmark->pev->origin;
			vecVA = UTIL_VecToAngles(vecPlayerOffs);
			vecVA.y += ydiff;
			UTIL_MakeVectors(vecVA);
			Vector vecPlayerOffsNew = gpGlobals->v_forward * vecPlayerOffs.Length();
			vecPlayerOffsNew.z = -vecPlayerOffsNew.z;

			vecOriginOffs += vecPlayerOffsNew - vecPlayerOffs;
		}

		LetGoRope(pOther);
		UTIL_SetOrigin(pOther->pev, pOther->pev->origin + vecOriginOffs);
	}
	else
	{
		Vector tmp = VARS( pentTarget )->origin;

		if (FBitSet(pev->spawnflags, SF_TELEPORT_RELATIVE_TELEPORT))
		{
			Vector subjectPos = pOther->pev->origin;
			if (pOther->IsPlayer())
				subjectPos.z += pOther->pev->mins.z;
			Vector offset = subjectPos - pev->origin;
			tmp += offset;
		}

		if( pOther->IsPlayer() )
		{
			tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
		}
		tmp.z++;

		LetGoRope(pOther);
		UTIL_SetOrigin( pevToucher, tmp );

		if (!FBitSet(pev->spawnflags, SF_TELEPORT_KEEPANGLES))
		{
			pevToucher->angles = pentTarget->v.angles;

			if( pOther->IsPlayer() )
			{
				pevToucher->v_angle = pentTarget->v.angles;
			}

			pevToucher->fixangle = 1;
		}

		if (!FBitSet(pev->spawnflags, SF_TELEPORT_KEEPVELOCITY))
		{
			pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
		}
	}

	pevToucher->flags &= ~FL_ONGROUND;

	if (FBitSet(pentTarget->v.spawnflags, SF_TELEPORT_DESTINATION_TRIGGER_ON_ARRIVAL) &&
			!FStringNull(pentTarget->v.target) &&
			FClassnameIs(pentTarget, "info_teleport_destination"))
	{
		FireTargets(STRING(pentTarget->v.target), pOther, this);
	}

	return true;
}

void CTriggerTeleport::LetGoRope(CBaseEntity *pOther)
{
	if( pOther->IsPlayer() )
	{
#if FEATURE_ROPE
		CBasePlayer* pPlayer = (CBasePlayer*)pOther;
		if( (pPlayer->m_afPhysicsFlags & PFLAG_ONROPE) )
			pPlayer->LetGoRope();
#endif
	}
}

void CTriggerTeleport::TeleportTouch( CBaseEntity *pOther )
{
	TeleportToDestination(pOther);
}

edict_t* CTriggerTeleport::GetTeleportTarget(CBaseEntity* pToucher)
{
	const char* szName = STRING( pev->target );
	if (FBitSet(pev->spawnflags, SF_TELEPORT_RANDOM_DESTINATION))
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
		return pEntity ? pEntity->edict() : nullptr;
	}
	return FIND_ENTITY_BY_TARGETNAME( NULL, szName );
}

CBaseEntity* CTriggerTeleport::GetLandmark()
{
	return FStringNull(pev->message) ? NULL : UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
}

class CTriggerTeleportPlayer : public CTriggerTeleport
{
public:
	void Spawn( void );
	void EXPORT TeleportPlayerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	edict_t* GetTeleportTarget(CBaseEntity* pToucher) override;
};

LINK_ENTITY_TO_CLASS( trigger_player_teleport, CTriggerTeleportPlayer )

void CTriggerTeleportPlayer::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetUse(&CTriggerTeleportPlayer::TeleportPlayerUse);
}

void CTriggerTeleportPlayer::TeleportPlayerUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
	if (pPlayer) {
		TeleportToDestination(pPlayer);
	}
}

edict_t* CTriggerTeleportPlayer::GetTeleportTarget(CBaseEntity* pToucher)
{
	if (FStringNull(pev->target)) {
		return edict();
	} else {
		return CTriggerTeleport::GetTeleportTarget(pToucher);
	}
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

	const float minimumHealth = pev->health;
	SetTouch( NULL );
	UTIL_Remove( this );

	if (minimumHealth > 0) {
		if (minimumHealth > pOther->pev->health)
			return;
	}

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
		pkvd->fHandled = true;
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
		pkvd->fHandled = true;
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


#define SF_LOGIC_DONT_USE_X 1
#define SF_LOGIC_DONT_USE_Y 2
#define SF_LOGIC_DONT_USE_Z 4

enum
{
	FLT2STR_6DP = 0, // 6 decimal places when converted to string
	FLT2STR_5DP = 1,
	FLT2STR_4DP = 4,
	FLT2STR_3DP = 7,
	FLT2STR_2DP = 10,
	FLT2STR_1DP = 13,
	FLT2STR_ROUND = 16, // round to nearest whole number when converted to string/int
	FLT2STR_CEIL = 17,
	FLT2STR_FLOOR = 18,
};

class CBaseLogic : public CPointEntity
{
public:
	virtual int GetVectorDontUseFlags() { return 0; }

	float VectorToFloat(Vector v);

	std::string FloatToString(float f, int flt2str_mode);
	std::string VectorToString(Vector v, int flt2str_mode);

	const char* GetTargetname() const {
		return pev->targetname ? STRING(pev->targetname) : "";
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hActivator;
};

TYPEDESCRIPTION	CBaseLogic::m_SaveData[] =
{
	DEFINE_FIELD( CBaseLogic, m_hActivator, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE(CBaseLogic, CPointEntity)

float CBaseLogic::VectorToFloat(Vector v)
{
	int dont_use_coords = GetVectorDontUseFlags();

	if (dont_use_coords == (SF_LOGIC_DONT_USE_Y | SF_LOGIC_DONT_USE_Z)) {
		return v.x;
	}
	if (dont_use_coords == (SF_LOGIC_DONT_USE_X | SF_LOGIC_DONT_USE_Z)) {
		return v.y;
	}
	if (dont_use_coords == (SF_LOGIC_DONT_USE_X | SF_LOGIC_DONT_USE_Y)) {
		return v.z;
	}

	if (dont_use_coords & SF_LOGIC_DONT_USE_X) {
		v.x = 0;
	}
	if (dont_use_coords & SF_LOGIC_DONT_USE_Y) {
		v.y = 0;
	}
	if (dont_use_coords & SF_LOGIC_DONT_USE_Z) {
		v.z = 0;
	}

	return v.Length();
}

std::string CBaseLogic::FloatToString(float f, int flt2str_mode)
{
	switch (flt2str_mode) {
	case FLT2STR_5DP: return UTIL_VarArgs("%.5f", f);
	case FLT2STR_4DP: return UTIL_VarArgs("%.4f", f);
	case FLT2STR_3DP: return UTIL_VarArgs("%.3f", f);
	case FLT2STR_2DP: return UTIL_VarArgs("%.2f", f);
	case FLT2STR_1DP: return UTIL_VarArgs("%.1f", f);
	case FLT2STR_ROUND: return UTIL_VarArgs("%d", (int)(f + 0.5f));
	case FLT2STR_CEIL: return UTIL_VarArgs("%d", (int)ceilf(f));
	case FLT2STR_FLOOR: return UTIL_VarArgs("%d", (int)f);
	case FLT2STR_6DP:
	default:
		return UTIL_VarArgs("%f", f);
	}
}

std::string CBaseLogic::VectorToString(Vector v, int flt2str_mode)
{
	int dont_use_coords = GetVectorDontUseFlags();

	std::string s;

	if (!(dont_use_coords & SF_LOGIC_DONT_USE_X)) {
		s += FloatToString(v.x, flt2str_mode);
	}
	if (!(dont_use_coords & SF_LOGIC_DONT_USE_Y)) {
		if (s.length()) {
			s += " ";
		}
		s += FloatToString(v.y, flt2str_mode);
	}
	if (!(dont_use_coords & SF_LOGIC_DONT_USE_Z)) {
		if (s.length()) {
			s += " ";
		}
		s += FloatToString(v.z, flt2str_mode);
	}

	return s;
}

enum
{
	CHANGEVALUE_LITERAL = 0,
	CHANGEVALUE_CALCRATIO = 1,
	CHANGEVALUE_CALCPOSITION = 2,
	CHANGEVALUE_CALCVELOCITY = 3,
};

#define SF_TRIGGER_CHANGEVALUE_NO_X (1<<0)
#define SF_TRIGGER_CHANGEVALUE_NO_Y (1<<1)
#define SF_TRIGGER_CHANGEVALUE_NO_Z (1<<2)

class CTriggerChangeValue : public CBaseLogic
{
public:
	enum
	{
		ACTION_REPLACE = 0,
		ACTION_ADD = 1,
		ACTION_MUL = 2,
		ACTION_SUB = 3,
		ACTION_DIV = 4,
		ACTION_MOD = 12,
		ACTION_POW = 16,
		ACTION_AND = 5,
		ACTION_OR = 6,
		ACTION_XOR = 13,
		ACTION_REMOVE_BITS = 15,
		ACTION_APPEND = 11,
		ACTION_MIN = 30,
		ACTION_MAX = 31,
	};

	enum
	{
		TCV_NO_ERROR = 0,
		TCV_ERROR_DIV_BY_ZERO,
		TCV_ERROR_UNSUPPORTED_OPERATION
	};

	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int GetVectorDontUseFlags() override {
		return pev->spawnflags & (SF_TRIGGER_CHANGEVALUE_NO_X|SF_TRIGGER_CHANGEVALUE_NO_Y|SF_TRIGGER_CHANGEVALUE_NO_Z);
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	static const char* OperationName(int operation);
	static const char* KeyTypeName(short keyType);
	int OperateInteger(int operation, int oldInteger, int sourceInteger, int& error);
	float OperateFloat(int operation, float oldFloat, float sourceFloat, int& error);
	void ApplySourceValue(CBaseEntity* pTarget, const char* sourceValue, const Vector& sourceVector, const float sourceFloat, const int sourceInteger);

	string_t m_iszNewValue;
	string_t m_iszValueName;
	int m_iszValueType;
};
LINK_ENTITY_TO_CLASS( trigger_changevalue, CTriggerChangeValue )

TYPEDESCRIPTION	CTriggerChangeValue::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerChangeValue, m_iszNewValue, FIELD_STRING ),
	DEFINE_FIELD( CTriggerChangeValue, m_iszValueName, FIELD_STRING ),
	DEFINE_FIELD( CTriggerChangeValue, m_iszValueType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeValue, CBaseLogic)

void CTriggerChangeValue::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewValue"))
	{
		m_iszNewValue = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszValueName"))
	{
		m_iszValueName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszValueType"))
	{
		m_iszValueType = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "value_type"))
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseLogic::KeyValue( pkvd );
}

const char* CTriggerChangeValue::OperationName(int operation)
{
	switch (operation) {
	case ACTION_REPLACE: return "Replace";
	case ACTION_ADD: return "Add";
	case ACTION_MUL: return "Mul";
	case ACTION_SUB: return "Sub";
	case ACTION_DIV: return "Div";
	case ACTION_MOD: return "Mod";
	case ACTION_AND: return "AND";
	case ACTION_OR: return "OR";
	case ACTION_XOR: return "XOR";
	case ACTION_REMOVE_BITS: return "Clear Bits";
	case ACTION_APPEND: return "Append";
	case ACTION_MIN: return "Min";
	case ACTION_MAX: return "Max";
	default: return "Unknown";
	}
}

const char* CTriggerChangeValue::KeyTypeName(short keyType)
{
	switch (keyType) {
	case KEY_TYPE_INT: return "Integer";
	case KEY_TYPE_FLOAT: return "Float";
	case KEY_TYPE_VECTOR: return "Vector";
	case KEY_TYPE_STRING: return "String";
	case KEY_TYPE_EDICT: return "Edict";
	default: return "Unknown";
	}
}

int CTriggerChangeValue::OperateInteger(int operation, int oldInteger, int sourceInteger, int& error)
{
	switch(operation)
	{
	case ACTION_REPLACE:
		return sourceInteger;
	case ACTION_ADD:
		return oldInteger + sourceInteger;
	case ACTION_MUL:
		return oldInteger * sourceInteger;
	case ACTION_SUB:
		return oldInteger - sourceInteger;
	case ACTION_DIV:
		if (sourceInteger != 0)
			return oldInteger / sourceInteger;
		else
			error = TCV_ERROR_DIV_BY_ZERO;
		break;
	case ACTION_MOD:
		if (sourceInteger != 0)
			return oldInteger % sourceInteger;
		else
			error = TCV_ERROR_DIV_BY_ZERO;
		break;
	case ACTION_POW:
		return (int)pow(oldInteger, sourceInteger);
	case ACTION_AND:
		return oldInteger & sourceInteger;
	case ACTION_OR:
		return oldInteger | sourceInteger;
	case ACTION_REMOVE_BITS:
		return oldInteger & ~sourceInteger;
	case ACTION_XOR:
		return oldInteger ^ sourceInteger;
	case ACTION_MIN:
		return Q_min(oldInteger, sourceInteger);
	case ACTION_MAX:
		return Q_max(oldInteger, sourceInteger);
	default:
		error = TCV_ERROR_UNSUPPORTED_OPERATION;
		break;
	}
	return oldInteger;
}

float CTriggerChangeValue::OperateFloat(int operation, float oldFloat, float sourceFloat, int& error)
{
	switch(operation)
	{
	case ACTION_REPLACE:
		return sourceFloat;
	case ACTION_ADD:
		return oldFloat + sourceFloat;
	case ACTION_MUL:
		return oldFloat * sourceFloat;
	case ACTION_SUB:
		return oldFloat - sourceFloat;
	case ACTION_DIV:
		if (sourceFloat != 0)
			return oldFloat / sourceFloat;
		else
			error = TCV_ERROR_DIV_BY_ZERO;
		break;
	case ACTION_MOD:
		if ((int)sourceFloat != 0)
			return (int)oldFloat / (int)sourceFloat;
		else
			error = TCV_ERROR_DIV_BY_ZERO;
		break;
	case ACTION_POW:
		return pow(oldFloat, sourceFloat);
	case ACTION_MIN:
		return Q_min(oldFloat, sourceFloat);
	case ACTION_MAX:
		return Q_max(oldFloat, sourceFloat);
	default:
		error = TCV_ERROR_UNSUPPORTED_OPERATION;
		break;
	}
	return oldFloat;
}

void CTriggerChangeValue::ApplySourceValue(CBaseEntity* pTarget, const char* sourceValue, const Vector& sourceVector, const float sourceFloat, const int sourceInteger)
{
	const char* keyName = STRING(m_iszValueName);
	const CKeyValue keyValue = ReadEntvarKeyvalue(pTarget->pev, keyName);
	char newValueBuf[256] = {'\0'};
	const char* newValue = sourceValue;

	int error = TCV_NO_ERROR;

	switch (keyValue.keyType) {
	case KEY_TYPE_VECTOR:
	{
		Vector newVector = keyValue.vVal;
		if (!FBitSet(pev->spawnflags, SF_TRIGGER_CHANGEVALUE_NO_X))
		{
			newVector.x = OperateFloat(m_iszValueType, keyValue.vVal.x, sourceVector.x, error);
		}
		if (!FBitSet(pev->spawnflags, SF_TRIGGER_CHANGEVALUE_NO_Y))
		{
			newVector.y = OperateFloat(m_iszValueType, keyValue.vVal.y, sourceVector.y, error);
		}
		if (!FBitSet(pev->spawnflags, SF_TRIGGER_CHANGEVALUE_NO_Z))
		{
			newVector.z = OperateFloat(m_iszValueType, keyValue.vVal.z, sourceVector.z, error);
		}
		snprintf(newValueBuf, sizeof(newValueBuf), "%g %g %g", newVector.x, newVector.y, newVector.z);
		newValue = newValueBuf;
	}
		break;
	case KEY_TYPE_INT:
	{
		int newInteger = OperateInteger(m_iszValueType, keyValue.iVal, sourceInteger, error);
		snprintf(newValueBuf, sizeof(newValueBuf), "%d", newInteger);
		newValue = newValueBuf;
	}
		break;
	case KEY_TYPE_FLOAT:
	{
		float newFloat = OperateFloat(m_iszValueType, keyValue.fVal, sourceFloat, error);
		snprintf(newValueBuf, sizeof(newValueBuf), "%g", newFloat);
		newValue = newValueBuf;
	}
		break;
	case KEY_TYPE_STRING:
	{
		switch (m_iszValueType) {
		case ACTION_REPLACE:
			break;
		case ACTION_APPEND:
		{
			if (!FStringNull(keyValue.sVal))
			{
				strncpyEnsureTermination(newValueBuf, STRING(keyValue.sVal));
			}
			size_t bufLen = strlen(newValueBuf);
			strncpyEnsureTermination(newValueBuf + bufLen, newValue, sizeof(newValueBuf) - bufLen);
			newValue = newValueBuf;
		}
			break;
		default:
			error = TCV_ERROR_UNSUPPORTED_OPERATION;
			break;
		}
	}
		break;
	default:
		error = TCV_ERROR_UNSUPPORTED_OPERATION;
		break;
	}

	switch (error) {
	case TCV_NO_ERROR:
	{
		KeyValueData mypkvd;
		mypkvd.szClassName = STRING(pTarget->pev->classname);
		mypkvd.szKeyName = keyName;
		mypkvd.szValue = newValue;
		mypkvd.fHandled = false;

		DispatchKeyValue(pTarget->edict(), &mypkvd);
		ALERT(at_aiconsole, "'%s' (%s): dispatched value '%s' to key '%s' of entity '%s'\n", GetTargetname(), STRING(pev->classname), newValue, keyName, STRING(pTarget->pev->classname));
	}
		break;
	case TCV_ERROR_DIV_BY_ZERO:
		ALERT(at_warning, "'%s' (%s) attempted to divide by zero\n", GetTargetname(), STRING(pev->classname));
		break;
	case TCV_ERROR_UNSUPPORTED_OPERATION:
	{
		ALERT(at_warning, "'%s' (%s) can't do operation %s on key '%s' of type %s\n",
			  GetTargetname(), STRING(pev->classname), OperationName(m_iszValueType), keyName, KeyTypeName(keyValue.keyType));
	}
		break;
	default:
		break;
	}
}

void CTriggerChangeValue::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	char sourceValueBuf[256] = {'\0'};
	const char* sourceValue = STRING(m_iszNewValue);

	Vector sourceVector{};
	float sourceFloat = 0.0f;
	int sourceInteger = 0;

	const int treatValueAs = pev->impulse;
	if (treatValueAs == CHANGEVALUE_CALCRATIO)
	{
		if (TryCalcLocus_Ratio(pActivator, STRING(m_iszNewValue), sourceFloat))
		{
			sourceInteger = (int)sourceFloat;
			snprintf(sourceValueBuf, sizeof(sourceValueBuf), "%g", sourceFloat);
			sourceValue = sourceValueBuf;
		}
		else
		{
			return;
		}
	}
	else if (treatValueAs == CHANGEVALUE_CALCPOSITION)
	{
		if (TryCalcLocus_Position(this, pActivator, STRING(m_iszNewValue), sourceVector))
		{
			snprintf(sourceValueBuf, sizeof(sourceValueBuf), "%g %g %g", sourceVector.x, sourceVector.y, sourceVector.z);
			sourceValue = sourceValueBuf;
		}
		else
		{
			return;
		}
	}
	else if (treatValueAs == CHANGEVALUE_CALCVELOCITY)
	{
		if (TryCalcLocus_Velocity(this, pActivator, STRING(m_iszNewValue), sourceVector))
		{
			snprintf(sourceValueBuf, sizeof(sourceValueBuf), "%g %g %g", sourceVector.x, sourceVector.y, sourceVector.z);
			sourceValue = sourceValueBuf;
		}
		else
		{
			return;
		}
	}
	else
	{
		// Try reading as vector in case we want to treat it as vector later
		int componentsRead = 0;
		UTIL_StringToVector((float*)sourceVector, sourceValue, &componentsRead);
		sourceFloat = atof(sourceValue);
		sourceInteger = atoi(sourceValue);
		if (componentsRead < 2)
		{
			sourceVector.x = sourceVector.y = sourceVector.z = sourceFloat;
		}
	}

	CBaseEntity* pTarget = nullptr;
	while ((pTarget = UTIL_FindEntityByTargetname( pTarget, STRING( pev->target ), pActivator )) != nullptr)
	{
		ApplySourceValue(pTarget, sourceValue, sourceVector, sourceFloat, sourceInteger);
	}

	if (!FStringNull(pev->message))
	{
		FireTargets(STRING(pev->message), pActivator, this);
	}
}

#define SF_TRIGGER_CONDITION_OFF (1 << 0)
#define SF_TRIGGER_CONDITION_DONT_USE_X (1 << 1)
#define SF_TRIGGER_CONDITION_DONT_USE_Y (1 << 2)
#define SF_TRIGGER_CONDITION_DONT_USE_Z (1 << 3)
#define SF_TRIGGER_CONDITION_CYCLIC (1 << 5)
#define SF_TRIGGER_CONDITION_KEEP_ACTIVATOR (1 << 6)
#define SF_TRIGGER_CONDITION_IGNORE_FIRST_RESULT (1 << 7)
#define SF_TRIGGER_CONDITION_DONT_DELAY_FIRST_CHECK (1 << 9)

constexpr float VEC_EQ_EPSILON = 0.03125f;

class CTriggerCondition : public CBaseLogic
{
	enum
	{
		COMPARE_EQUAL,
		COMPARE_NEQUAL,
		COMPARE_LESS,
		COMPARE_GREATER,
		COMPARE_LEQUAL,
		COMPARE_GEQUAL,
		COMPARE_BITSET,

		COMPARE_TYPES,
	};

	enum
	{
		MODE_WAIT_BOTH, // only fire target when result changes
		MODE_WAIT_AFTER_FALSE, // always fire true, only fire false when last result was true
		MODE_WAIT_AFTER_TRUE, // always fire false, only fire true when last result was false
		MODE_WAIT_NEVER, // always fire true/false

		CONSTANT_MODES
	};

public:
	void Spawn() override;
	void KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT MonitorThink();

	bool IsActive() const {
		return !FBitSet(pev->spawnflags, SF_TRIGGER_CONDITION_OFF);
	}
	void SetIsActive(bool enable) {
		if (enable)
			ClearBits(pev->spawnflags, SF_TRIGGER_CONDITION_OFF);
		else
			SetBits(pev->spawnflags, SF_TRIGGER_CONDITION_OFF);
	}

	void Evaluate();
	bool Compare(const CKeyValue& monitorKey, const CKeyValue& compareKey);

	int getValueAsInt(const CKeyValue& key);
	float getValueAsFloat(const CKeyValue& key);
	const char* getValueAsString(const CKeyValue& key);

	bool CompareFloats(const CKeyValue& monitorKey, const CKeyValue& compareKey);
	bool CompareInts(const CKeyValue& monitorKey, const CKeyValue& compareKey);
	bool CompareVectors(const CKeyValue& monitorKey, const CKeyValue& compareKey);
	bool CompareStrings(const CKeyValue& monitorKey, const CKeyValue& compareKey);

	int GetVectorDontUseFlags() override {
		return (pev->spawnflags & (SF_TRIGGER_CONDITION_DONT_USE_X | SF_TRIGGER_CONDITION_DONT_USE_Y | SF_TRIGGER_CONDITION_DONT_USE_Z)) >> 1;
	}

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszValueName;
	string_t m_iszSourceName;
	string_t m_iszSourceKey;
	string_t m_iszCheckValue;
	int m_iCheckType;
	int m_iCheckBehavior;
	float m_fCheckInterval;

	EHANDLE m_hActivator;

	bool m_checkedFirstResult;
	bool m_lastResult;
	bool m_lastResultEvaluated;
};

LINK_ENTITY_TO_CLASS( trigger_condition, CTriggerCondition )

TYPEDESCRIPTION	CTriggerCondition::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerCondition, m_iszValueName, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCondition, m_iszSourceName, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCondition, m_iszSourceKey, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCondition, m_iszCheckValue, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCondition, m_iCheckType, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerCondition, m_iCheckBehavior, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerCondition, m_fCheckInterval, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCondition, m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCondition, m_checkedFirstResult, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerCondition, m_lastResult, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerCondition, m_lastResultEvaluated, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CTriggerCondition, CBaseLogic)

void CTriggerCondition::Spawn()
{
	CBaseLogic::Spawn();

	if (!FBitSet(pev->spawnflags, SF_TRIGGER_CONDITION_OFF|SF_TRIGGER_CONDITION_CYCLIC))
	{
		SetThink(&CTriggerCondition::MonitorThink);
		pev->nextthink = gpGlobals->time;
	}
}

void CTriggerCondition::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszValueName"))
	{
		m_iszValueName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSourceName"))
	{
		m_iszSourceName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSourceKey"))
	{
		m_iszSourceKey = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszCheckValue"))
	{
		m_iszCheckValue = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iCheckType"))
	{
		m_iCheckType = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iCheckBehaviour"))
	{
		m_iCheckBehavior = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fCheckInterval"))
	{
		m_fCheckInterval = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else {
		CBaseLogic::KeyValue(pkvd);
	}
}

void CTriggerCondition::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType == USE_ON)
		SetIsActive(true);
	else if (useType == USE_OFF)
		SetIsActive(false);
	else
		SetIsActive(!IsActive());

	m_hActivator = pActivator;

	m_checkedFirstResult = false;
	m_lastResultEvaluated = false;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_CONDITION_CYCLIC))
	{
		Evaluate();
	}
	else
	{
		if (IsActive())
		{
			SetThink(&CTriggerCondition::MonitorThink);
			if (FBitSet(pev->spawnflags, SF_TRIGGER_CONDITION_DONT_DELAY_FIRST_CHECK))
				pev->nextthink = gpGlobals->time;
			else
				pev->nextthink = gpGlobals->time + m_fCheckInterval;
		}
		else
		{
			SetThink(NULL);
			pev->nextthink = 0;
		}
	}
}

void CTriggerCondition::MonitorThink()
{
	Evaluate();
	pev->nextthink = gpGlobals->time + m_fCheckInterval;
}

void CTriggerCondition::Evaluate()
{
	if (FStringNull(pev->target)) {
		ALERT(at_console, "'%s' (%s): missing monitored entity\n", GetTargetname(), STRING(pev->classname));
		return;
	}
	if (FStringNull(m_iszValueName)) {
		ALERT(at_console, "'%s' (%s): missing monitored key\n", GetTargetname(), STRING(pev->classname));
		return;
	}
	if (m_iCheckType < 0 || m_iCheckType >= COMPARE_TYPES) {
		ALERT(at_console, "'%s' (%s): invalid check type %d\n", GetTargetname(), STRING(pev->classname), m_iCheckType);
		return;
	}

	if (m_iCheckBehavior < 0 || m_iCheckBehavior >= CONSTANT_MODES) {
		ALERT(at_console, "'%s' (%s): invalid constant mode behavior %d\n", GetTargetname(), STRING(pev->classname), m_iCheckType);
		return;
	}

	CBaseEntity* pEntity = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target), m_hActivator);
	if (!pEntity) {
		ALERT(at_console, "'%s' (%s): monitored entity %s is not found\n", GetTargetname(), STRING(pev->classname), STRING(pev->target));
		return;
	}

	CKeyValue monitorKey = ReadEntvarKeyvalue(pEntity->pev, STRING(m_iszValueName));
	if (!monitorKey.keyType)
		return;

	CKeyValue compareKey;

	if (!FStringNull(m_iszSourceName) && !FStringNull(m_iszSourceKey))
	{
		CBaseEntity* pSourceEntity = UTIL_FindEntityByTargetname(nullptr, STRING(m_iszSourceName), m_hActivator);
		if (pSourceEntity)
		{
			compareKey = ReadEntvarKeyvalue(pSourceEntity->pev, STRING(m_iszSourceKey));
			if (!compareKey.keyType)
				return;
		}
		else
			return;
	}
	else
	{
		if (m_iszCheckValue)
		{
			compareKey.keyName = "m_iszCheckValue";
			compareKey.keyType = KEY_TYPE_STRING;
			compareKey.sVal = m_iszCheckValue;
		}
		else
		{
			compareKey.keyName = "m_iszCheckValue";
			compareKey.keyType = KEY_TYPE_INT;
			compareKey.iVal = 0;
		}
	}

	bool result = Compare(monitorKey, compareKey);
	bool isCyclic = pev->spawnflags & SF_TRIGGER_CONDITION_CYCLIC;
	bool ignoreFirstResult = pev->spawnflags & SF_TRIGGER_CONDITION_IGNORE_FIRST_RESULT;
	bool shouldFireResultTarget = !ignoreFirstResult || m_checkedFirstResult || isCyclic;

	if (!isCyclic && shouldFireResultTarget)
	{
		switch (m_iCheckBehavior) {
		case MODE_WAIT_BOTH:
			shouldFireResultTarget = !m_lastResultEvaluated || m_lastResult != result;
			break;
		case MODE_WAIT_AFTER_FALSE:
			shouldFireResultTarget = !m_lastResultEvaluated || m_lastResult != result || result == true;
			break;
		case MODE_WAIT_AFTER_TRUE:
			shouldFireResultTarget = !m_lastResultEvaluated || m_lastResult != result || result == false;
			break;
		case MODE_WAIT_NEVER:
		default:
			break;
		}
	}

	if (shouldFireResultTarget)
	{
		EHANDLE h_oldActivator = m_hActivator;

		if (!(pev->spawnflags & SF_TRIGGER_CONDITION_KEEP_ACTIVATOR)) {
			m_hActivator = this;
		}

		if (result && pev->netname) {
			if (DeveloperModeLevel() >= 4)
				ALERT(at_aiconsole, "'%s' (%s): Firing TRUE target %s\n", GetTargetname(), STRING(pev->classname), STRING(pev->netname));
			FireTargets(STRING(pev->netname), m_hActivator, this);
		}
		else if (!result && pev->message) {
			if (DeveloperModeLevel() >= 4)
				ALERT(at_aiconsole, "'%s' (%s): Firing FALSE target %s\n", GetTargetname(), STRING(pev->classname), STRING(pev->message));
			FireTargets(STRING(pev->message), m_hActivator, this);
		}

		m_hActivator = h_oldActivator;
	}

	m_checkedFirstResult = true;
	m_lastResult = result;
	m_lastResultEvaluated = true;
}

bool CTriggerCondition::Compare(const CKeyValue& monitorKey, const CKeyValue& compareKey)
{
	// TODO: does this ent follow c promotion rules? Would an int compared to a float mean
	// the int is promoted to float, regardless of which side the float was on?

	switch (monitorKey.keyType) {
	case KEY_TYPE_FLOAT:
		return CompareFloats(monitorKey, compareKey);
	case KEY_TYPE_INT:
		return CompareInts(monitorKey, compareKey);
	case KEY_TYPE_VECTOR:
		return CompareVectors(monitorKey, compareKey);
	case KEY_TYPE_STRING:
		return CompareStrings(monitorKey, compareKey);
	default:
		ALERT(at_console, "'%s' (%s): cannot compare key %s (invalid type)\n",
			  GetTargetname(), STRING(pev->classname), monitorKey.keyName);
		return false;
	}
}

bool CTriggerCondition::CompareFloats(const CKeyValue& monitorKey, const CKeyValue& compareKey)
{
	switch (m_iCheckType) {
	case COMPARE_EQUAL: return monitorKey.fVal == getValueAsFloat(compareKey);
	case COMPARE_NEQUAL: return monitorKey.fVal != getValueAsFloat(compareKey);
	case COMPARE_LESS: return monitorKey.fVal < getValueAsFloat(compareKey);
	case COMPARE_GREATER: return monitorKey.fVal > getValueAsFloat(compareKey);
	case COMPARE_LEQUAL: return monitorKey.fVal <= getValueAsFloat(compareKey);
	case COMPARE_GEQUAL: return monitorKey.fVal >= getValueAsFloat(compareKey);
	case COMPARE_BITSET:
	default:
		ALERT(at_console, "'%s' (%s): invalid compare type %d used on float key %s\n",
			  GetTargetname(), STRING(pev->classname), m_iCheckType, monitorKey.keyName);
		return false;
	}
}

bool CTriggerCondition::CompareInts(const CKeyValue& monitorKey, const CKeyValue& compareKey)
{
	switch (m_iCheckType) {
	case COMPARE_EQUAL: return monitorKey.iVal == getValueAsInt(compareKey);
	case COMPARE_NEQUAL: return monitorKey.iVal != getValueAsInt(compareKey);
	case COMPARE_LESS: return monitorKey.iVal < getValueAsInt(compareKey);
	case COMPARE_GREATER: return monitorKey.iVal > getValueAsInt(compareKey);
	case COMPARE_LEQUAL: return monitorKey.iVal <= getValueAsInt(compareKey);
	case COMPARE_GEQUAL: return monitorKey.iVal >= getValueAsInt(compareKey);
	case COMPARE_BITSET: return monitorKey.iVal & getValueAsInt(compareKey);
	default:
		ALERT(at_console, "'%s' (%s): invalid compare type %d used on int key %s\n",
			  GetTargetname(), STRING(pev->classname), m_iCheckType, monitorKey.keyName);
		return false;
	}
}

bool CTriggerCondition::CompareVectors(const CKeyValue& monitorKey, const CKeyValue& compareKey)
{
	Vector monitorVector = monitorKey.vVal;
	Vector compareVector = compareKey.vVal;
	bool compareKeyIsVector = compareKey.keyType == KEY_TYPE_VECTOR;

	if (!compareKeyIsVector && compareKey.keyType == KEY_TYPE_STRING) {
		int componentsRead;
		Vector temp;
		UTIL_StringToVector(temp, STRING(compareKey.sVal), &componentsRead);
		if (componentsRead >= 3)
		{
			compareVector = temp;
			compareKeyIsVector = true;
		}
	}

	if (pev->spawnflags & SF_TRIGGER_CONDITION_DONT_USE_X) {
		monitorVector.x = 0;
		compareVector.x = 0;
	}
	if (pev->spawnflags & SF_TRIGGER_CONDITION_DONT_USE_Y) {
		monitorVector.y = 0;
		compareVector.y = 0;
	}
	if (pev->spawnflags & SF_TRIGGER_CONDITION_DONT_USE_Z) {
		monitorVector.z = 0;
		compareVector.z = 0;
	}

	// lengths or single components are compared if the compare value is not a vector,
	// or if check type is anything but ==/!=
	float monitorFloat = VectorToFloat(monitorVector);
	float compareFloat = getValueAsFloat(compareKey);

	switch (m_iCheckType) {
	case COMPARE_EQUAL:
		if (compareKeyIsVector) {
			Vector d = monitorVector - compareVector;
			return fabs(d.x) <= VEC_EQ_EPSILON && fabs(d.y) <= VEC_EQ_EPSILON && fabs(d.z) <= VEC_EQ_EPSILON;
		}
		else {
			return monitorFloat == compareFloat;
		}
	case COMPARE_NEQUAL:
		if (compareKeyIsVector) {
			Vector d = monitorVector - compareVector;
			return fabs(d.x) > VEC_EQ_EPSILON || fabs(d.y) > VEC_EQ_EPSILON || fabs(d.z) > VEC_EQ_EPSILON;
		}
		else {
			return monitorFloat != compareFloat;
		}
	case COMPARE_LESS: return monitorFloat < compareFloat;
	case COMPARE_GREATER: return monitorFloat > compareFloat;
	case COMPARE_LEQUAL: return monitorFloat <= compareFloat;
	case COMPARE_GEQUAL: return monitorFloat >= compareFloat;
	default:
		ALERT(at_console, "'%s' (%s): invalid compare type %d used on vector key %s\n",
			  GetTargetname(), STRING(pev->classname), m_iCheckType, monitorKey.keyName);
		return false;
	}
}

bool CTriggerCondition::CompareStrings(const CKeyValue& monitorKey, const CKeyValue& compareKey)
{
	switch (m_iCheckType) {
	case COMPARE_EQUAL: return strcmp(STRING(monitorKey.sVal), getValueAsString(compareKey)) == 0;
	case COMPARE_NEQUAL: return strcmp(STRING(monitorKey.sVal), getValueAsString(compareKey)) != 0;
	default:
		ALERT(at_console, "'%s' (%s): invalid compare type %d used on string key %s\n",
			  GetTargetname(), STRING(pev->classname), m_iCheckType, monitorKey.keyName);
		return false;
	}
}

float CTriggerCondition::getValueAsFloat(const CKeyValue& key)
{
	switch (key.keyType) {
	case KEY_TYPE_FLOAT:
		return key.fVal;
	case KEY_TYPE_INT:
		return key.iVal;
	case KEY_TYPE_VECTOR:
		return VectorToFloat(key.vVal);
	case KEY_TYPE_STRING:
	{
		int componentsRead;
		Vector vTemp;
		UTIL_StringToVector(vTemp, STRING(key.sVal), &componentsRead);
		if (componentsRead >= 3)
			return VectorToFloat(vTemp);
		else
			return atof(STRING(key.sVal));
	}
	default:
		ALERT(at_console, "'%s' (%s): cannot convert key %s to float (invalid type)\n",
			  GetTargetname(), STRING(pev->classname), key.keyName);
		return 0;
	}
}

int CTriggerCondition::getValueAsInt(const CKeyValue& key)
{
	switch (key.keyType) {
	case KEY_TYPE_INT:
		return key.iVal;
	case KEY_TYPE_FLOAT:
	case KEY_TYPE_VECTOR:
		return (int)getValueAsFloat(key);
	case KEY_TYPE_STRING:
	{
		int componentsRead;
		Vector vTemp;
		UTIL_StringToVector(vTemp, STRING(key.sVal), &componentsRead);
		if (componentsRead >= 3)
			return (int)VectorToFloat(vTemp);
		else
			return atoi(STRING(key.sVal));
	}
	default:
		ALERT(at_console, "'%s' (%s): cannot convert key %s to int (invalid type)\n",
			  GetTargetname(), STRING(pev->classname), key.keyName);
		return 0;
	}
}

const char* CTriggerCondition::getValueAsString(const CKeyValue& key)
{
	switch (key.keyType) {
	case KEY_TYPE_FLOAT:
		return UTIL_VarArgs("%f", key.fVal);
	case KEY_TYPE_INT:
		return UTIL_VarArgs("%d", key.iVal);
	case KEY_TYPE_VECTOR:
	{
		std::string temp = VectorToString(key.vVal, FLT2STR_6DP);
		return UTIL_VarArgs("%s", temp.c_str());
	}
	case KEY_TYPE_STRING:
		return STRING(key.sVal);
	default:
		ALERT(at_console, "'%s' (%s): cannot convert key %s to string (invalid type)\n",
			  GetTargetname(), STRING(pev->classname), key.keyName);
		return "";
	}
}

#define SF_CAMERA_PLAYER_POSITION	1
#define SF_CAMERA_PLAYER_TARGET		2
#define SF_CAMERA_PLAYER_TAKECONTROL 4
#define SF_CAMERA_PLAYER_INVULRENABLE 256
#define SF_CAMERA_IGNORE_HOLD_TIME 512
#define SF_CAMERA_PLAYER_ALIVE_ONLY 1024
#define SF_CAMERA_DONT_FIRE_LOOK_TARGET (1<<12)
#define SF_CAMERA_DONT_SLOW_DOWN_IF_NOT_FREEZE (1<<13)
#define SF_CAMERA_DONT_SKIP_FIRST_PATH_CORNER (1<<14)

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
	void UpdateOnRemove();
	void ReleasePlayer();
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
	string_t m_iszTurnedOffTarget;
	int m_stopByPlayerInput; // don't save, this is going to spawnflags
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
	DEFINE_FIELD( CTriggerCamera, m_iszTurnedOffTarget, FIELD_STRING ),
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

	if (m_stopByPlayerInput != 0)
	{
		pev->spawnflags |= SF_CAMERA_STOP_BY_PLAYER_INPUT_USE;
	}
}

void CTriggerCamera::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "moveto" ) )
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "acceleration" ) )
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "deceleration" ) )
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "stop_by_player_input" ) )
	{
		m_stopByPlayerInput = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "m_iszTurnedOffTarget" ) )
	{
		m_iszTurnedOffTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
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
	CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
	if (!pPlayer)
		return;

	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_ALIVE_ONLY) && !pPlayer->IsAlive())
		return;

	pPlayer->m_cameraFlags = 0;
	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_INVULRENABLE))
	{
		SetBits(pPlayer->m_cameraFlags, PLAYER_CAMERA_INVULNERABLE);
	}

	pActivator = pPlayer;
	m_hPlayer = pPlayer;
	pPlayer->m_camera = this;

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
		pPlayer->EnableControl( false );
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

	pPlayer->m_hViewEntity = this;

	SET_MODEL( ENT( pev ), STRING( pActivator->pev->model ) );

	// follow the player down
	SetThink( &CTriggerCamera::FollowTarget );
	pev->nextthink = gpGlobals->time;

	if (m_pentPath && FBitSet(pev->spawnflags, SF_CAMERA_DONT_SKIP_FIRST_PATH_CORNER))
	{
		Vector delta = m_pentPath->pev->origin - pev->origin;
		m_moveDistance = delta.Length();
		pev->movedir = delta.Normalize();
	}
	else
	{
		m_moveDistance = 0;
	}
	Move();
}

void CTriggerCamera::FollowTarget()
{
	if( m_hPlayer == 0 )
		return;

	CBasePlayer* player = static_cast<CBasePlayer*>(static_cast<CBaseEntity*>(m_hPlayer));
	const bool playerIsAlive = player->IsAlive();
	const bool shouldTurnOff = !playerIsAlive && FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_ALIVE_ONLY);
	const bool shouldGoOff = FBitSet(pev->spawnflags, SF_CAMERA_IGNORE_HOLD_TIME) ? m_state == 0 : m_flReturnTime < gpGlobals->time;

	if( m_hTarget == 0 || shouldGoOff || shouldTurnOff )
	{
		const bool shouldReset = !playerIsAlive && FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL );

		if( playerIsAlive || shouldReset || shouldTurnOff )
		{
			ReleasePlayer();
		}

		player->m_camera = 0;
		player->m_cameraFlags = 0;
		player->m_hViewEntity = 0;
		player->m_bResetViewEntity = false;

		if (!FBitSet(pev->spawnflags, SF_CAMERA_DONT_FIRE_LOOK_TARGET))
			SUB_UseTargets( this );
		if (!FStringNull(m_iszTurnedOffTarget))
			FireTargets(STRING(m_iszTurnedOffTarget), player, this);
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

	pev->avelocity.x = dx * 40 * 0.01f;
	pev->avelocity.y = dy * 40 * 0.01f;

	if (!FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL) && !FBitSet(pev->spawnflags, SF_CAMERA_DONT_SLOW_DOWN_IF_NOT_FREEZE))
	{
		pev->velocity = pev->velocity * 0.8f;
		if( pev->velocity.IsLengthLessThan(10.0f) )
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
			FireTargets( STRING( m_pentPath->pev->message ), this, this );
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

void CTriggerCamera::UpdateOnRemove()
{
	if (m_state)
	{
		ReleasePlayer();
		m_state = 0;
	}
	CBaseDelay::UpdateOnRemove();
}

void CTriggerCamera::ReleasePlayer()
{
	CBasePlayer* player = static_cast<CBasePlayer*>(static_cast<CBaseEntity*>(m_hPlayer));
	if (player)
	{
		SET_VIEW( player->edict(), player->edict() );
		if (FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ))
			player->EnableControl( true );
	}
}

#if FEATURE_TRIGGER_RANDOM

#define TRIGGER_RANDOM_MAX_COUNT 16

#define SF_TRIGGER_RANDOM_START_ON 1
#define SF_TRIGGER_RANDOM_ONCE 2
#define SF_TRIGGER_RANDOM_REUSABLE 4
#define SF_TRIGGER_RANDOM_TIMED 8
#define SF_TRIGGER_RANDOM_UNIQUE 16
#define SF_TRIGGER_RANDOM_DONT_REPEAT 32
#define SF_TRIGGER_RANDOM_PREDETERMINED 64
#define SF_TRIGGER_RANDOM_PREDETERMINED_TIMED 128

class CTriggerRandom : public CPointEntity
{
public:
	void Spawn();
	void KeyValue( KeyValueData *pkvd );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TimedThink();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t ChooseTarget();
	float GetRandomDelay();
	int TargetCount();
	void DoUnique();

	int m_targetCount;
	string_t m_targets[TRIGGER_RANDOM_MAX_COUNT];
	int m_uniqueTargetsLeft;
	int m_triggerNumberLimit;
	int m_triggerCounter;
	float m_minDelay;
	float m_maxDelay;
	string_t m_lastTarget;
	string_t m_triggerOnLimit;
	string_t m_triggerOnExhaust;
	int m_firstPreferredTarget;

	unsigned int m_randomSeed;
	unsigned int m_delayRandomSeed;

	int RandomizeIndex(int low, int high) {
		if (m_firstPreferredTarget > 0 && m_firstPreferredTarget <= TargetCount()) {
			const int result = m_firstPreferredTarget - 1;
			m_firstPreferredTarget = 0;
			return result;
		}

		if (HasPredeterminedTargets()) {
			const int index = UTIL_SharedRandomLong(m_randomSeed, low, high);
			m_randomSeed = UTIL_SharedRandomLong(m_randomSeed, 0, 1<<15);
			return index;
		} else {
			return RANDOM_LONG(low, high);
		}
	}

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

	bool HasPredeterminedTargets() const {
		return FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_PREDETERMINED);
	}
	bool HasPrederminedDelays() const {
		return FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_PREDETERMINED_TIMED);
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
	DEFINE_FIELD( CTriggerRandom, m_triggerOnLimit, FIELD_STRING ),
	DEFINE_FIELD( CTriggerRandom, m_triggerOnExhaust, FIELD_STRING ),
	DEFINE_FIELD( CTriggerRandom, m_firstPreferredTarget, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_randomSeed, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_delayRandomSeed, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerRandom, CPointEntity )

void CTriggerRandom::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "min_delay") ) {
		m_minDelay = atof( pkvd->szValue );
		if (m_minDelay < 0) {
			m_minDelay = 0;
		}
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "max_delay") ) {
		m_maxDelay = atof( pkvd->szValue );
		if (m_maxDelay < 0) {
			m_maxDelay = 0;
		}
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "trigger_number") ) {
		m_triggerNumberLimit = atoi( pkvd->szValue );
		if (m_triggerNumberLimit < 0) {
			m_triggerNumberLimit = 0;
		}
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "target_count" ) ) { // Sven Co-op compatibility
		m_targetCount = atoi( pkvd->szValue );
		if (m_targetCount < 0) {
			m_targetCount = 0;
		} else if (m_targetCount > TRIGGER_RANDOM_MAX_COUNT) {
			m_targetCount = TRIGGER_RANDOM_MAX_COUNT;
		}
		pkvd->fHandled = true;
	} else if ( strncmp(pkvd->szKeyName, "target", 6) == 0 && isdigit(pkvd->szKeyName[6])) {
		const int num = atoi(pkvd->szKeyName+6);
		if (num <= 0 || num > TRIGGER_RANDOM_MAX_COUNT)
			return;

		const int index = num - 1;
		m_targets[index] = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "trigger_on_limit") ) {
		m_triggerOnLimit = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "trigger_on_exhaust") ) {
		m_triggerOnExhaust = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "first_target") ) {
		m_firstPreferredTarget = atoi( pkvd->szValue );
		pkvd->fHandled = true;
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
	TargetCount(); // call in case it needs to be evaluated
	if (FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_UNIQUE)) {
		m_uniqueTargetsLeft = TargetCount();
	}
	if (FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_TIMED)) {

		if (IsActive()) {
			SetThink(&CTriggerRandom::TimedThink);
			pev->nextthink = gpGlobals->time + GetRandomDelay() + 0.1;
		}
	}

	if (HasPredeterminedTargets()) {
		m_randomSeed = RANDOM_LONG(0, (1<<15));
	}
	if (HasPrederminedDelays()) {
		m_delayRandomSeed = RANDOM_LONG(0, (1<<15));
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
		string_t chosenTarget = ChooseTarget();
		if (!FStringNull(chosenTarget)) {
			FireTargets(STRING(chosenTarget), pActivator, this);
		}
		DoUnique();
	}
}

void CTriggerRandom::TimedThink()
{
	if (IsActive()) {
		string_t chosenTarget = ChooseTarget();
		if (!FStringNull(chosenTarget)) {
			FireTargets(STRING(chosenTarget), this, this);
		}
		DoUnique();

		if (m_triggerNumberLimit) {
			m_triggerCounter++;
			if (m_triggerCounter >= m_triggerNumberLimit) {
				SetActive(false);
				m_triggerCounter = 0;

				if (!FStringNull(m_triggerOnLimit))
					FireTargets(STRING(m_triggerOnLimit), this, this);
			}
		}

		if ((pev->spawnflags & SF_TRIGGER_RANDOM_UNIQUE) && m_uniqueTargetsLeft <= 0)
			SetActive(false);
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
		if (m_uniqueTargetsLeft > 0) {
			chosenTargetIndex = RandomizeIndex(0, m_uniqueTargetsLeft - 1);
			chosenTarget = m_targets[chosenTargetIndex];

			if (chosenTarget == m_lastTarget && FBitSet(pev->spawnflags, SF_TRIGGER_RANDOM_DONT_REPEAT) && m_uniqueTargetsLeft > 1)
			{
				chosenTargetIndex = (chosenTargetIndex+1) % m_uniqueTargetsLeft;
				chosenTarget = m_targets[chosenTargetIndex];
			}

			m_targets[chosenTargetIndex] = m_targets[m_uniqueTargetsLeft-1];
			m_targets[m_uniqueTargetsLeft-1] = chosenTarget;

			m_lastTarget = chosenTarget;
			return chosenTarget;
		}
	} else {
		const int targetCount = TargetCount();
		if (targetCount) {
			chosenTargetIndex = RandomizeIndex(0, targetCount - 1);
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
	const float minDelay = m_minDelay;
	const float maxDelay = Q_max(m_maxDelay, m_minDelay);
	if (HasPrederminedDelays())
	{
		const float delay = UTIL_SharedRandomFloat(m_delayRandomSeed, minDelay, maxDelay);
		m_delayRandomSeed = UTIL_SharedRandomLong(m_delayRandomSeed, 0, 1<<15);
		return delay;
	}
	return RANDOM_FLOAT(minDelay, maxDelay);
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

void CTriggerRandom::DoUnique()
{
	if ((pev->spawnflags & SF_TRIGGER_RANDOM_UNIQUE) && m_uniqueTargetsLeft > 0) {
		m_uniqueTargetsLeft--;

		if (!m_uniqueTargetsLeft) {
			if (!FStringNull(m_triggerOnExhaust))
				FireTargets(STRING(m_triggerOnExhaust), this, this);
			if ((pev->spawnflags & SF_TRIGGER_RANDOM_REUSABLE))
				m_uniqueTargetsLeft = TargetCount();
		}
	}
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

extern void respawn( entvars_t *pev, bool fCopyCorpse );

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
	const bool enabled = !FBitSet(pev->spawnflags, SF_DISPLACER_TARGET_DISABLED);
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

CBaseEntity* GetDisplacerEarthTarget(CBaseEntity* pOther)
{
	float flMinDist = 8192;
	CBaseEntity* pEarthTarget = nullptr;
	CBasePlayer* pPlayer = nullptr;
	if (pOther && pOther->IsPlayer())
	{
		pPlayer = (CBasePlayer*)pOther;
	}
	CBaseEntity* pDestination = nullptr;
	while ((pDestination = UTIL_FindEntityByClassname(pDestination, "info_displacer_earth_target")) != nullptr)
	{
		if (FBitSet(pDestination->pev->spawnflags, SF_DISPLACER_TARGET_DISABLED))
			continue;
		if (pPlayer)
		{
			const float flDist = (pPlayer->m_DisplacerReturn - pDestination->pev->origin).Length();
			if (flDist <= flMinDist)
			{
				pEarthTarget = pDestination;
				flMinDist = flDist;
			}
		}
		else
		{
			pEarthTarget = pDestination;
			break;
		}
	}
	return pEarthTarget;
}

class CTriggerXenReturn : public CTriggerTeleport
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT TeleportTouch( CBaseEntity *pOther );

	edict_t* GetTeleportTarget(CBaseEntity* pToucher) override;
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
	PRECACHE_SOUND( "weapons/displacer_self.wav" );
}

void CTriggerXenReturn::TeleportTouch(CBaseEntity* pOther)
{
	if (TeleportToDestination(pOther))
	{
		if (pOther->IsPlayer())
		{
			CBasePlayer* pPlayer = (CBasePlayer*)pOther;
			pPlayer->m_fInXen = false;
			pPlayer->pev->gravity = 1.0f;
			pPlayer->m_SndRoomtype = pPlayer->m_DisplacerSndRoomtype;
		}

		// Play teleport sound.
		EMIT_SOUND(ENT(pOther->pev), CHAN_WEAPON, "weapons/displacer_self.wav", 1, ATTN_NORM );
	}
}

edict_t* CTriggerXenReturn::GetTeleportTarget(CBaseEntity* pToucher)
{
	CBaseEntity* pEarthTarget = GetDisplacerEarthTarget(pToucher);
	return pEarthTarget ? pEarthTarget->edict() : nullptr;
}
#endif

enum
{
	USE_FREEZE_OFF = -1,
	USE_FREEZE_TOGGLE = 0,
	USE_FREEZE_ON = 1,
	USE_FREEZE_COPY_INPUT = 2,
};

class CTriggerPlayerFreeze : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
};

LINK_ENTITY_TO_CLASS( trigger_playerfreeze, CTriggerPlayerFreeze )

void CTriggerPlayerFreeze::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "use_type") ) {
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	} else {
		CPointEntity::KeyValue( pkvd );
	}
}

void CTriggerPlayerFreeze::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer* pPlayer = g_pGameRules->EffectiveAlivePlayer(pActivator);

	if (pPlayer)
	{
		int freezeType = pev->impulse;
		if (pev->impulse == USE_FREEZE_COPY_INPUT)
		{
			if (useType == USE_TOGGLE)
			{
				freezeType = USE_FREEZE_TOGGLE;
			}
			else if (useType == USE_OFF)
			{
				freezeType = USE_FREEZE_OFF;
			}
			else if (useType == USE_ON)
			{
				freezeType = USE_FREEZE_ON;
			}
			else
			{
				ALERT(at_console, "%s: Can't interpret input use type: %d\n", STRING(pev->classname), (int)useType);
				return;
			}
		}

		switch (freezeType) {
		case USE_FREEZE_OFF:
			pPlayer->EnableControl(true);
			break;
		case USE_FREEZE_ON:
			pPlayer->EnableControl(false);
			break;
		case USE_FREEZE_TOGGLE:
			if( pPlayer->pev->flags & FL_FROZEN )
				pPlayer->EnableControl( true );
			else
				pPlayer->EnableControl( false );
			break;
		default:
			ALERT(at_console, "%s: Unknown bad use type: %d\n", STRING(pev->classname), (int)freezeType);
			break;
		}
	}
}

#if FEATURE_TRIGGER_KILL_MONSTER

#define SF_KILLMONSTER_FIREONCE 1
#define SF_KILLMONSTER_GIBALWAYS 16

class CTriggerKillMonster : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

protected:
	void KillMonster(CBaseEntity* pEntity);
	bool CheckTarget(CBaseEntity* pEntity);
};

LINK_ENTITY_TO_CLASS( trigger_killmonster, CTriggerKillMonster )

void CTriggerKillMonster::Spawn()
{
	CPointEntity::Spawn();
	pev->effects |= EF_NODRAW;
}

void CTriggerKillMonster::KillMonster(CBaseEntity *pEntity)
{
	CBaseMonster* pMonster = pEntity->MyMonsterPointer();
	if (pMonster && pMonster->IsFullyAlive())
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
		pMonster->pev->health = 0;
		DamageInfo damageInfo{1, DMG_GENERIC};
		if (pev->spawnflags & SF_KILLMONSTER_GIBALWAYS)
			damageInfo.SetGibPolicy(GIB_ALWAYS);
		pMonster->TakeDamage(pev, pev, damageInfo );
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

	if( pev->spawnflags & SF_KILLMONSTER_FIREONCE )
		UTIL_Remove( this );
}
#endif

#if FEATURE_TRIGGER_TIMER

#define SF_TRIGGER_TIMER_START_ON 1
#define SF_TRIGGER_TIMER_NO_FIRST_DELAY 32
#define SF_TRIGGER_TIMER_FORWARD_ACTIVATOR 64
#define SF_TRIGGER_TIMER_PREDETERMINED_TIMED 128
#define SF_TRIGGER_TIMER_DONT_RESET_TRIGGER_COUNTER 256

class CTriggerTimer : public CPointEntity
{
public:
	void Spawn();
	void KeyValue( KeyValueData *pkvd );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TimerThink();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float GetRandomDelay();
	void SetActive(bool active, CBaseEntity* pActivator = NULL);

	bool HasPredeterminedDelays() const {
		return FBitSet(pev->spawnflags, SF_TRIGGER_TIMER_PREDETERMINED_TIMED);
	}

	int m_triggerNumberLimit;
	int m_triggerCounter;
	float m_minDelay;
	float m_maxDelay;
	bool m_active;
	string_t m_triggerOnLimit;
	unsigned int m_delayRandomSeed;
	EHANDLE m_hActivator;
};

LINK_ENTITY_TO_CLASS( trigger_timer, CTriggerTimer )

TYPEDESCRIPTION	CTriggerTimer::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerTimer, m_triggerNumberLimit, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerTimer, m_triggerCounter, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerTimer, m_minDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerTimer, m_maxDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerTimer, m_active, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerTimer, m_triggerOnLimit, FIELD_STRING ),
	DEFINE_FIELD( CTriggerTimer, m_delayRandomSeed, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerTimer, m_hActivator, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CTriggerTimer, CPointEntity )

void CTriggerTimer::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "min_delay") ) {
		m_minDelay = atof( pkvd->szValue );
		if (m_minDelay < 0) {
			m_minDelay = 0;
		}
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "max_delay") ) {
		m_maxDelay = atof( pkvd->szValue );
		if (m_maxDelay < 0) {
			m_maxDelay = 0;
		}
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "trigger_number") ) {
		m_triggerNumberLimit = atoi( pkvd->szValue );
		if (m_triggerNumberLimit < 0) {
			m_triggerNumberLimit = 0;
		}
		pkvd->fHandled = true;
	} else if ( FStrEq( pkvd->szKeyName, "trigger_on_limit") ) {
		m_triggerOnLimit = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	} else {
		CBaseEntity::KeyValue( pkvd );
	}
}

void CTriggerTimer::Spawn()
{
	m_triggerCounter = 0;
	m_active = false;
	SetThink(&CTriggerTimer::TimerThink);

	if (HasPredeterminedDelays()) {
		m_delayRandomSeed = RANDOM_LONG(0, (1<<15));
	}

	if (pev->spawnflags & SF_TRIGGER_TIMER_START_ON) {
		SetActive(true);
		pev->nextthink += 0.1; // some little delay of spawn
	}
}

void CTriggerTimer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	switch (useType) {
	case USE_OFF:
		SetActive(false);
		break;
	case USE_ON:
		SetActive(true, pActivator);
		break;
	default:
		SetActive(!m_active, pActivator);
		break;
	}
}

void CTriggerTimer::TimerThink()
{
	if (m_active) {
		CBaseEntity* pActivator = this;
		if (FBitSet(pev->spawnflags, SF_TRIGGER_TIMER_FORWARD_ACTIVATOR))
			pActivator = m_hActivator;

		if (!FStringNull(pev->target)) {
			FireTargets(STRING(pev->target), pActivator, this);
		}

		if (m_triggerNumberLimit) {
			m_triggerCounter++;
			if (m_triggerCounter >= m_triggerNumberLimit) {
				SetActive(false);
				m_triggerCounter = 0;
				if (!FStringNull(m_triggerOnLimit))
				{
					FireTargets(STRING(m_triggerOnLimit), pActivator, this);
				}
				return;
			}
		}

		pev->nextthink = gpGlobals->time + GetRandomDelay();
	}
}

float CTriggerTimer::GetRandomDelay()
{
	const float minDelay = m_minDelay;
	const float maxDelay = Q_max(m_maxDelay, m_minDelay);
	if (HasPredeterminedDelays())
	{
		const float delay = UTIL_SharedRandomFloat(m_delayRandomSeed, minDelay, maxDelay);
		m_delayRandomSeed = UTIL_SharedRandomLong(m_delayRandomSeed, 0, 1<<15);
		return delay;
	}
	return RANDOM_FLOAT(minDelay, maxDelay);
}

void CTriggerTimer::SetActive(bool active, CBaseEntity* pActivator)
{
	if (m_active == active)
		return;
	m_active = active;
	if (m_active)
	{
		m_hActivator = pActivator;
		if (FBitSet(pev->spawnflags, SF_TRIGGER_TIMER_NO_FIRST_DELAY))
			pev->nextthink = gpGlobals->time;
		else
			pev->nextthink = gpGlobals->time + GetRandomDelay();
	}
	else
	{
		m_hActivator = 0;
		if (!FBitSet(pev->spawnflags, SF_TRIGGER_TIMER_DONT_RESET_TRIGGER_COUNTER))
			m_triggerCounter = 0;
	}
}
#endif

#if FEATURE_GENEWORM
//=========================================================
// CTriggerGenewormHit
//=========================================================

class CTriggerGenewormHit : public CTriggerHurt
{
public:
	void Spawn();
	void Precache();
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("geneworm"); }
	float DamageAmount() override {
		return pev->dmg ? pev->dmg : gSkillData.gwormDmgHit;
	}
	void EXPORT GeneWormTouch(CBaseEntity *pOther);

	static TYPEDESCRIPTION m_SaveData[];

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	float m_flLastDamageTime;

	static constexpr const char* attackHitSoundScript = "GeneWorm.AttackHit";
};

LINK_ENTITY_TO_CLASS(trigger_geneworm_hit, CTriggerGenewormHit)

TYPEDESCRIPTION CTriggerGenewormHit::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerGenewormHit, m_flLastDamageTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CTriggerGenewormHit, CTriggerHurt)

void CTriggerGenewormHit::Spawn()
{
	Precache();

	if (m_bitsDamageInflict)
		m_bitsDamageInflict = DMG_CRUSH;
	m_flLastDamageTime = gpGlobals->time;

	CTriggerHurt::Spawn();
	SetTouch(&CTriggerGenewormHit::GeneWormTouch);
}

void CTriggerGenewormHit::Precache()
{
	SoundScriptParamOverride paramOverride;
	paramOverride.OverrideAttenuationAbsolute(0.1f);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript, paramOverride);
}

void CTriggerGenewormHit::GeneWormTouch(CBaseEntity *pOther)
{
	if( gpGlobals->time - m_flLastDamageTime < 2 )
		return;

	CTriggerHurt::HurtTouch(pOther);

	EmitSoundScript(attackHitSoundScript);
	m_flLastDamageTime = gpGlobals->time;
}

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
		pkvd->fHandled = true;
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
	auto checkMonster = [](CBaseEntity* pEntity) -> CBaseMonster*
	{
		if (!pEntity)
			return nullptr;
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if (pMonster && FBitSet(pMonster->pev->flags, FL_MONSTER))
		{
			if (pMonster->IsFreeToManipulate())
				return pMonster;
		}
		return nullptr;
	};

	CBaseEntity *pPath = UTIL_FindEntityByTargetname( NULL, STRING( m_iszPath ) );
	if (!pPath)
		return;

	CBaseEntity *pTarget = nullptr;

	if (UTIL_TargetnameIsActivator(STRING(pev->target)))
	{
		pTarget = pActivator;
	}
	else
	{
		pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	}

	CBaseMonster *pMonster = checkMonster(pTarget);
	if (pMonster)
	{
		Schedule_t* patrolSchedule = pMonster->StartPatrol(pPath);
		if (patrolSchedule)
		{
			CFollowingMonster* followingMonster = pMonster->MyFollowingMonsterPointer();
			if (followingMonster->IsFollowingPlayer())
			{
				followingMonster->StopFollowing(true, false);
			}
			pMonster->SetState( MONSTERSTATE_IDLE );
			pMonster->ChangeSchedule( patrolSchedule );
		}
	}
}

static void Motion_PrintVectors(const char* text, const Vector& oldVec, const Vector& newVec)
{
	ALERT(at_console, "%s from %f %f %f to %f %f %f\n", text, oldVec.x, oldVec.y, oldVec.z, newVec.x, newVec.y, newVec.z);
}

#if FEATURE_TRIGGER_MOTION
//===========================================================
//LRC- trigger_motion
//===========================================================
#define SF_MOTION_DEBUG 1

class CTriggerMotion : public CPointEntity
{
public:
	enum {
		POSMODE_SET = 0,
		POSMODE_ADDOFFSET
	};

	enum {
		ANGMOD_SET = 0,
		ANGMOD_ROTATE,
		ANGMOD_ROTATE_BY_VALUES,
	};

	enum {
		VELMODE_SET = 0,
		VELMODE_ADD,
		VELMODE_ROTATE,
		VELMODE_ROTATE_BY_VALUES,
	};

	enum {
		AVELMODE_SET = 0,
		AVELMODE_ADD,
	};

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
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPosMode"))
	{
		m_iPosMode = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszAngles"))
	{
		m_iszAngles = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAngMode"))
	{
		m_iAngMode = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelocity"))
	{
		m_iszVelocity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iVelMode"))
	{
		m_iVelMode = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszAVelocity"))
	{
		m_iszAVelocity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAVelMode"))
	{
		m_iAVelMode = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerMotion::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING(pev->target), pActivator );
	if (!pTarget)
	{
		ALERT(at_warning, "%s \"%s\" can't find entity \"%s\" to affect\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->target));
		return;
	}

	const bool debug = pev->spawnflags & SF_MOTION_DEBUG;

	if (debug)
		ALERT(at_console, "DEBUG: trigger_motion affects %s \"%s\":\n", STRING(pTarget->pev->classname), STRING(pTarget->pev->targetname));

	Vector vecTemp = g_vecZero;
	Vector vecOld = g_vecZero;

	if (m_iszPosition)
	{
		switch (m_iPosMode)
		{
		case POSMODE_SET:
			if (TryCalcLocus_Position( this, pActivator, STRING(m_iszPosition), vecTemp )) {
				if (debug)
					Motion_PrintVectors("DEBUG: Set origin", pTarget->pev->origin, vecTemp);
				UTIL_SetOrigin(pTarget->pev, vecTemp);
			}
			pTarget->pev->flags &= ~FL_ONGROUND;
			break;
		case POSMODE_ADDOFFSET:
			if (TryCalcLocus_Velocity( this, pActivator, STRING(m_iszPosition), vecTemp )) {
				vecOld = pTarget->pev->origin;
				pTarget->pev->origin = pTarget->pev->origin + vecTemp;
				if (debug) {
					Motion_PrintVectors("DEBUG: Set origin", vecOld, pTarget->pev->origin);
				}
			}
			pTarget->pev->flags &= ~FL_ONGROUND;
			break;
		}
	}

	Vector vecVelAngles;
	vecTemp = g_vecZero;

	if (m_iszAngles)
	{
		switch (m_iAngMode)
		{
		case ANGMOD_SET:
			if (TryCalcLocus_Velocity( this, pActivator, STRING(m_iszAngles), vecTemp )) {
				vecOld = pTarget->pev->angles;
				pTarget->pev->angles = UTIL_VecToAngles( vecTemp );
				if (debug)
					Motion_PrintVectors("DEBUG: Set angles", vecOld, pTarget->pev->angles);
			}
			break;
		case ANGMOD_ROTATE:
			if (TryCalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity), vecTemp )) {
				vecOld = pTarget->pev->angles;
				pTarget->pev->angles = pTarget->pev->angles + UTIL_VecToAngles( vecTemp );
				if (debug)
					Motion_PrintVectors("DEBUG: Rotate angles", vecOld, pTarget->pev->angles);
			}
			break;
		case ANGMOD_ROTATE_BY_VALUES:
			UTIL_StringToRandomVector( vecTemp, STRING(m_iszAngles) );
			vecOld = pTarget->pev->angles + vecTemp;
			pTarget->pev->angles = pTarget->pev->angles + vecTemp;
			if (debug)
				Motion_PrintVectors("DEBUG: Rotate angles", vecOld, pTarget->pev->angles);
			break;
		}
	}

	vecTemp = g_vecZero;

	if (m_iszVelocity)
	{
		switch (m_iVelMode)
		{
		case VELMODE_SET:
			if (TryCalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity), vecTemp )) {
				if (debug)
					Motion_PrintVectors("DEBUG: Set velocity", pTarget->pev->velocity, vecTemp);
				pTarget->pev->velocity = vecTemp;
			}
			break;
		case VELMODE_ADD:
			if (TryCalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity), vecTemp )) {
				vecOld = pTarget->pev->velocity;
				pTarget->pev->velocity = vecOld + vecTemp;
				if (debug)
					Motion_PrintVectors("DEBUG: Set velocity", vecOld, pTarget->pev->velocity);
			}
			break;
		case VELMODE_ROTATE:
			if (TryCalcLocus_Velocity( this, pActivator, STRING(m_iszVelocity), vecTemp )) {
				vecVelAngles = UTIL_VecToAngles( vecTemp ) + UTIL_VecToAngles( pTarget->pev->velocity );
				UTIL_MakeVectors( vecVelAngles );
				vecOld = pTarget->pev->velocity;
				pTarget->pev->velocity = vecOld.Length() * gpGlobals->v_forward;
				pTarget->pev->velocity.z = -pTarget->pev->velocity.z; //vecToAngles reverses the z angle
				if (debug)
					Motion_PrintVectors("DEBUG: Rotate velocity", vecOld, pTarget->pev->velocity);
			}
			break;
		case VELMODE_ROTATE_BY_VALUES:
			UTIL_StringToRandomVector( vecTemp, STRING(m_iszVelocity) );
			vecVelAngles = vecTemp + UTIL_VecToAngles( pTarget->pev->velocity );
			UTIL_MakeVectors( vecVelAngles );
			vecOld = pTarget->pev->velocity;
			pTarget->pev->velocity = vecOld.Length() * gpGlobals->v_forward;
			pTarget->pev->velocity.z = -pTarget->pev->velocity.z; //vecToAngles reverses the z angle
			if (debug)
				Motion_PrintVectors("DEBUG: Rotate velocity", vecOld, pTarget->pev->velocity);
			break;
		}
	}

	vecTemp = g_vecZero;

	switch (m_iAVelMode)
	{
	case AVELMODE_SET:
		UTIL_StringToRandomVector( vecTemp, STRING(m_iszAVelocity) );
		if (debug)
			Motion_PrintVectors("DEBUG: Set avelocity", pTarget->pev->avelocity, vecTemp);
		pTarget->pev->avelocity = vecTemp;
		break;
	case AVELMODE_ADD:
		UTIL_StringToRandomVector( vecTemp, STRING(m_iszAVelocity) );
		vecOld = pTarget->pev->avelocity;
		pTarget->pev->avelocity = vecOld + vecTemp;
		if (debug)
			Motion_PrintVectors("DEBUG: Set avelocity", vecOld, pTarget->pev->avelocity);
		break;
	}
}

//===========================================================
//LRC- motion_manager
//===========================================================
#define SF_MOTION_REQUIRES_ACTIVATOR ( 1 << 24 )

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

	bool m_reportedPosError; // don't save
	bool m_reportedDirError; // don't save

	enum {
		POSMODE_SET = 0,
		POSMODE_OFFSET,
		POSMODE_SETVEL,
		POSMODE_ACCELERATE,
		POSMODE_FOLLOW,
	};

	enum {
		FACEMODE_DIRECTION = 0,
		FACEMODE_ROTATE,
		FACEMODE_ROTATE_BY_VALUES,
		FACEMODE_SET_ANGULAR_VELOCITY,
		FACEMODE_SET_VELOCITY,
	};
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
	const bool debug = pev->spawnflags & SF_MOTION_DEBUG;

	if( (m_hLocus == 0 && FBitSet(pev->spawnflags, SF_MOTION_REQUIRES_ACTIVATOR)) || m_hTarget == 0 )
	{
		if (debug)
			ALERT(at_console, "motion_thread \"%s\" expires\n", STRING(pev->targetname));
		SetThink(&CMotionThread::SUB_Remove);
		pev->nextthink = gpGlobals->time;
		return;
	}
	else
	{
		pev->nextthink = gpGlobals->time; // think every frame
	}

	if (debug)
		ALERT(at_console, "motion_thread \"%s\" affects %s \"%s\" (time: %g):\n", STRING(pev->targetname), STRING(m_hTarget->pev->classname), STRING(m_hTarget->pev->targetname), gpGlobals->time);

	Vector vecTemp = g_vecZero;
	Vector vecOld = g_vecZero;

	if (m_iszPosition)
	{
		switch (m_iPosMode)
		{
		case POSMODE_SET: // set position
			if (TryCalcLocus_Position( this, m_hLocus, STRING(m_iszPosition), vecTemp, !m_reportedPosError )) {
				if (debug)
					Motion_PrintVectors("DEBUG: Set origin", m_hTarget->pev->origin, vecTemp);
				UTIL_AssignOrigin(m_hTarget, vecTemp);
			}
			else
				m_reportedPosError = true;
			m_hTarget->pev->flags &= ~FL_ONGROUND;
			break;
		case POSMODE_OFFSET: // offset position (= fake velocity)
			if (TryCalcLocus_Velocity( this, m_hLocus, STRING(m_iszPosition), vecTemp, !m_reportedPosError )) {
				vecOld = m_hTarget->pev->origin;
				UTIL_AssignOrigin(m_hTarget, vecOld + gpGlobals->frametime * vecTemp);
				if (debug)
					Motion_PrintVectors("DEBUG: Set origin", vecOld, m_hTarget->pev->origin);
			}
			else
				m_reportedPosError = true;
			m_hTarget->pev->flags &= ~FL_ONGROUND;
			break;
		case POSMODE_SETVEL: // set velocity
			if (TryCalcLocus_Velocity( this, m_hLocus, STRING(m_iszPosition), vecTemp, !m_reportedPosError )) {
				if (debug)
					Motion_PrintVectors("DEBUG: Set velocity", m_hTarget->pev->velocity, vecTemp);
				UTIL_SetVelocity(m_hTarget, vecTemp);
			}
			else
				m_reportedPosError = true;
			break;
		case POSMODE_ACCELERATE: // accelerate
			if (TryCalcLocus_Velocity( this, m_hLocus, STRING(m_iszPosition), vecTemp, !m_reportedPosError )) {
				vecOld = m_hTarget->pev->velocity;
				UTIL_SetVelocity(m_hTarget, vecOld + gpGlobals->frametime * vecTemp);
				if (debug)
					Motion_PrintVectors("DEBUG: Accelerate", vecOld, m_hTarget->pev->velocity);
			}
			else
				m_reportedPosError = true;
			break;
		case POSMODE_FOLLOW: // follow position
			if (TryCalcLocus_Position( this, m_hLocus, STRING(m_iszPosition), vecTemp, !m_reportedPosError )) {
				vecOld = m_hTarget->pev->velocity;
				UTIL_SetVelocity(m_hTarget, vecTemp - m_hTarget->pev->origin);
				if (debug)
					Motion_PrintVectors("DEBUG: Set velocity", vecOld, m_hTarget->pev->velocity);
			}
			else
				m_reportedPosError = true;
			break;
		}
	}

	Vector vecVelAngles;
	vecTemp = g_vecZero;

	if (m_iszFacing)
	{
		switch (m_iFaceMode)
		{
		case FACEMODE_DIRECTION: // set angles
			if (TryCalcLocus_Velocity( this, m_hLocus, STRING(m_iszFacing), vecTemp, !m_reportedDirError ))
			{
				if (vecTemp != g_vecZero) // if the vector is 0 0 0, don't use it
				{
					vecOld = m_hTarget->pev->angles;
					UTIL_SetAngles(m_hTarget, UTIL_VecToAngles( vecTemp ));
					if (debug)
						Motion_PrintVectors("DEBUG: Set angles", vecOld, m_hTarget->pev->angles);
				}
				else if (debug)
				{
					ALERT(at_console, "Zero velocity, don't change angles\n");
				}
			}
			else
				m_reportedDirError = true;
			break;
		case FACEMODE_ROTATE: // offset angles (= fake avelocity)
			if (TryCalcLocus_Velocity( this, m_hLocus, STRING(m_iszFacing), vecTemp, !m_reportedDirError ))
			{
				if (vecTemp != g_vecZero) // if the vector is 0 0 0, don't use it
				{
					vecOld = m_hTarget->pev->angles;
					UTIL_SetAngles(m_hTarget, m_hTarget->pev->angles + gpGlobals->frametime * UTIL_VecToAngles( vecTemp ));
					if (debug)
						Motion_PrintVectors("DEBUG: Offset angles", vecOld, m_hTarget->pev->angles);
				}
				else if (debug)
				{
					ALERT(at_console, "Zero velocity, don't change angles\n");
				}
			}
			else
				m_reportedDirError = true;
			break;
		case FACEMODE_ROTATE_BY_VALUES: // offset angles (= fake avelocity)
			UTIL_StringToRandomVector( vecVelAngles, STRING(m_iszFacing) );
			vecOld = m_hTarget->pev->angles;
			UTIL_SetAngles(m_hTarget, m_hTarget->pev->angles + gpGlobals->frametime * vecVelAngles);
			if (debug)
				Motion_PrintVectors("DEBUG: Rotate angles", vecOld, m_hTarget->pev->angles);
			break;
		case FACEMODE_SET_ANGULAR_VELOCITY: // set avelocity
			UTIL_StringToRandomVector( vecTemp, STRING(m_iszFacing) );
			if (debug)
				Motion_PrintVectors("DEBUG: Set avelocity", m_hTarget->pev->avelocity, vecTemp);
			UTIL_SetAvelocity(m_hTarget, vecTemp);
			break;
		case FACEMODE_SET_VELOCITY:
		{
			CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, STRING(m_iszFacing), m_hLocus);
			if (pCalc != NULL)
			{
				if (pev->spawnflags & SF_MOTION_DEBUG)
					ALERT(at_console, "DEBUG: Set angles from %f %f %f ", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
				UTIL_SetAngles(m_hTarget, pCalc->pev->angles);
				if (pev->spawnflags & SF_MOTION_DEBUG)
					ALERT(at_console, "to %f %f %f\n", m_hTarget->pev->angles.x, m_hTarget->pev->angles.y, m_hTarget->pev->angles.z);
			}
			else
			{
				if (!m_reportedDirError)
				{
					ALERT(at_error, "Could not find \"%s\" to set angles for \"%s\"", STRING(m_iszFacing), STRING(m_hTarget->pev->targetname));
					m_reportedDirError = true;
				}
			}
		}
			break;
		}
	}
}

#define SF_MOTION_MANAGER_OBEY_TRIGGER_MODE 16
#define SF_MOTION_MANAGER_START_ON 32

class CMotionManager : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
	void Affect(CBaseEntity *pTarget, CBaseEntity *pActivator );
	void AffectTargets(CBaseEntity* pActivator);
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
	bool m_activated;
	bool m_active;
};
LINK_ENTITY_TO_CLASS( motion_manager, CMotionManager )

TYPEDESCRIPTION	CMotionManager::m_SaveData[] =
{
	DEFINE_FIELD( CMotionManager, m_iszPosition, FIELD_STRING ),
	DEFINE_FIELD( CMotionManager, m_iPosMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMotionManager, m_iszFacing, FIELD_STRING ),
	DEFINE_FIELD( CMotionManager, m_iFaceMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMotionManager, m_activated, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMotionManager, m_active, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CMotionManager,CPointEntity)

void CMotionManager::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPosMode"))
	{
		m_iPosMode = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszFacing"))
	{
		m_iszFacing = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFaceMode"))
	{
		m_iFaceMode = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CMotionManager::Activate( void )
{
	if (m_activated)
		return;
	if (FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_MOTION_MANAGER_START_ON))
	{
		Use( this, this, USE_ON, 0 );
	}
	m_activated = true;
}

void CMotionManager::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (FBitSet(pev->spawnflags, SF_MOTION_MANAGER_OBEY_TRIGGER_MODE))
	{
		if (ShouldToggle(useType, m_active))
		{
			if (m_active)
			{
				m_active = false;
				RemoveThreads();
			}
			else
			{
				m_active = true;
				AffectTargets(pActivator);
			}
		}
		return;
	}

	if (useType == USE_OFF || useType == USE_TOGGLE)
	{
		RemoveThreads();
	}
	if (useType == USE_ON || useType == USE_TOGGLE)
	{
		AffectTargets(pActivator);
	}
}

void CMotionManager::AffectTargets(CBaseEntity* pActivator)
{
	if (pev->target)
	{
		CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target), pActivator);
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

void CMotionManager::Affect( CBaseEntity *pTarget, CBaseEntity *pActivator )
{
	if (pev->spawnflags & SF_MOTION_DEBUG)
		ALERT(at_console, "DEBUG: Creating MotionThread \"%s\" for %s \"%s\". Time: %g\n", STRING(pev->targetname), STRING(pTarget->pev->classname), STRING(pTarget->pev->targetname), gpGlobals->time);

	CMotionThread *pThread = GetClassPtr( (CMotionThread*)NULL );
	if (pThread == NULL) return; //error?
	pThread->pev->owner = edict();
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

	if (UTIL_TargetnameIsActivator(m_iszPosition) || UTIL_TargetnameIsActivator(m_iszFacing))
	{
		pThread->pev->spawnflags |= SF_MOTION_REQUIRES_ACTIVATOR;
	}
}

void CMotionManager::UpdateOnRemove()
{
	RemoveThreads();
	CPointEntity::UpdateOnRemove();
}

void CMotionManager::RemoveThreads()
{
	CBaseEntity* pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "motion_thread")) != NULL)
	{
		if (pEntity->pev->owner == edict())
		{
			CMotionThread* motionThread = (CMotionThread*)pEntity;
			CBaseEntity* pTarget = motionThread->m_hTarget;

			motionThread->m_hTarget = 0;
			motionThread->m_hLocus = 0;

			if (pTarget && (pev->spawnflags & SF_MOTION_DEBUG))
				ALERT(at_console, "DEBUG: Removing MotionThread \"%s\" for %s \"%s\"\n", STRING(pev->targetname), STRING(pTarget->pev->classname), STRING(pTarget->pev->targetname));
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

class CTriggerChangeClass : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	void Affect(CBaseEntity* pEntity, USE_TYPE useType);

	int m_iClass;
};

LINK_ENTITY_TO_CLASS( trigger_change_class, CTriggerChangeClass )

TYPEDESCRIPTION	CTriggerChangeClass::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerChangeClass, m_iClass, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerChangeClass, CPointEntity )

void CTriggerChangeClass::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "classify"))
	{
		m_iClass = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerChangeClass::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(pev->target))
		return;
	CBaseEntity* pEntity = NULL;
	while((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->target), pActivator)) != NULL)
	{
		Affect(pEntity, useType);
	}
}

void CTriggerChangeClass::Affect(CBaseEntity *pEntity, USE_TYPE useType)
{
	if (!pEntity)
		return;
	if (pEntity->IsPlayer())
		return; // don't change the class of player
	CBaseMonster* pMonster = pEntity->MyMonsterPointer();
	if (pMonster)
	{
		if (useType == USE_OFF)
			pMonster->m_iClass = 0;
		else
			pMonster->m_iClass = m_iClass;
	}
}

#if FEATURE_TRIGGER_HURT_REMOTE

#define SF_TRIGGER_HURT_REMOTE_INSTANT_KILL 1
#define SF_TRIGGER_HURT_REMOTE_CONSTANT 2
#define SF_TRIGGER_HURT_REMOTE_STARTON 4
#define SF_TRIGGER_HURT_REMOTE_DO_ARMOR_DAMAGE 8

#define SF_TRIGGER_HURT_REMOTE_IGNORE_ARMOR 256
#define SF_TRIGGER_HURT_REMOTE_NO_PUNCH 512

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
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "delay"))
	{
		pev->frags = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		pev->weapons = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "gib_policy" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "min_health" ) )
	{
		pev->dmg_save = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "armordmg"))
	{
		pev->armorvalue = atof( pkvd->szValue );
		pkvd->fHandled = true;
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

	if (pev->armorvalue != 0 && FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_DO_ARMOR_DAMAGE))
	{
		CBaseEntity* charger = pActivator != 0 ? pActivator : this;
		pTarget->TakeArmor(charger, -pev->armorvalue);
	}

	if (pev->dmg >= 0)
	{
		DamageInfo damageInfo{pev->dmg, pev->weapons};

		switch (pev->impulse) {
		case 1:
			damageInfo.SetGibPolicy(GIB_ALWAYS);
			break;
		case 2:
			damageInfo.SetGibPolicy(GIB_NEVER);
			break;
		default:
			break;
		}
		if (pev->spawnflags & SF_TRIGGER_HURT_REMOTE_IGNORE_ARMOR)
			damageInfo.SetIgnoreArmor();
		if (pev->spawnflags & SF_TRIGGER_HURT_REMOTE_NO_PUNCH)
			damageInfo.SetNoPunch();
		damageInfo.noPlayerPush = true;

		const float minHealthThreshold = pev->dmg_save;
		if (minHealthThreshold > 0) {
			pTarget->m_healthMinThreshold = minHealthThreshold;
		}

		entvars_t* pevAttacker = pActivator != 0 ? pActivator->pev : pev;
		if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_INSTANT_KILL))
		{
			if (pTarget->IsPlayer())
			{
				CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pTarget);
				damageInfo.damage = pTarget->pev->health + pTarget->pev->armorvalue * pPlayer->ArmorStrength();
				pTarget->TakeDamage(pTarget->pev, pevAttacker, damageInfo);
			}
			else
			{
				damageInfo.damage = pTarget->pev->health;
				damageInfo.SetGibPolicy(GIB_ALWAYS);
				pTarget->TakeDamage(pTarget->pev, pevAttacker, damageInfo);
			}
		}
		else
		{
			pTarget->TakeDamage(pTarget->pev, pevAttacker, damageInfo);
		}
	}
	else
	{
		CBaseEntity* healer = pActivator != 0 ? pActivator : this;
		if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_REMOTE_INSTANT_KILL))
		{
			pTarget->TakeHealth(healer, Q_max( pTarget->pev->max_health - pTarget->pev->health, 0 ), HEAL_GENERIC);
		}
		else
		{
			pTarget->TakeHealth(healer, -pev->dmg, HEAL_GENERIC);
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
		pkvd->fHandled = true;
	}
	if (FStrEq(pkvd->szKeyName, "classname_filter"))
	{
		m_classnameFilter = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "status_filter"))
	{
		m_statusFilter = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		m_triggerState = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "trigger_after_run"))
	{
		m_triggerAfterRun = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "maximum_runs"))
	{
		m_maxRuns = atoi( pkvd->szValue );
		pkvd->fHandled = true;
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
		FireTargets(STRING(m_triggerAfterRun), pActivator, this);
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
	SUB_UseTargets(pEntity, useType);
}
#endif

class CTriggerConfigureMonster : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	template<typename T>
	static void MaySetResettableValue(T& lvalue, T value)
	{
		if (value < 0)
			lvalue = 0;
		else if (value > 0)
			lvalue = value;
	}
	template<typename T>
	static void MaySetResettableValue2(T& lvalue, T value)
	{
		if (value == -2)
			lvalue = 0;
		else if (value != 0)
			lvalue = value;
	}
	void Affect(CBaseEntity* pEntity);

	// For all these variables zero means no change
	short m_gagFlag;
	short m_monsterClipFlag;
	short m_prisonerFlag;
	short m_predisasterFlag;
	short m_dontDropGunFlag;
	short m_actOutOfPVS;
	int m_iClass;
	int m_soundMask;
	short m_bloodColor;

	short m_prisonerTo;
	short m_freeRoam;
	short m_sizeForGrapple;
	short m_gibPolicy;
	short m_ignoredBy;

	short m_iTriggerCondition;
	short m_iTriggerAltCondition;
	string_t m_iszTriggerTarget;

	// Talk monsters
	string_t m_iszUse;
	string_t m_iszUnUse;
	string_t m_iszDecline;
	short m_provokedState;
	short m_iTolerance;

	short m_ignorePushFlag;
};

LINK_ENTITY_TO_CLASS( trigger_configure_monster, CTriggerConfigureMonster )

TYPEDESCRIPTION	CTriggerConfigureMonster::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerConfigureMonster, m_gagFlag, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_monsterClipFlag, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_prisonerFlag, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_predisasterFlag, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_dontDropGunFlag, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_actOutOfPVS, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iClass, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_soundMask, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_bloodColor, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_prisonerTo, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_freeRoam, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_sizeForGrapple, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_gibPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_ignoredBy, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iTriggerCondition, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iTriggerAltCondition, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iszTriggerTarget, FIELD_STRING ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iszUnUse, FIELD_STRING ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iszDecline, FIELD_STRING ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_provokedState, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_iTolerance, FIELD_SHORT ),
	DEFINE_FIELD( CTriggerConfigureMonster, m_ignorePushFlag, FIELD_SHORT ),
};

IMPLEMENT_SAVERESTORE( CTriggerConfigureMonster, CPointEntity )

void CTriggerConfigureMonster::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "gag_flag"))
	{
		m_gagFlag = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "monsterclip_flag"))
	{
		m_monsterClipFlag = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "prisoner_flag"))
	{
		m_prisonerFlag = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "predisaster_flag"))
	{
		m_predisasterFlag = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "dont_drop_gun_flag"))
	{
		m_dontDropGunFlag = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "act_out_of_pvs"))
	{
		m_actOutOfPVS = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "soundmask"))
	{
		m_soundMask = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "bloodcolor" ) )
	{
		m_bloodColor = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "classify"))
	{
		m_iClass = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "prisonerto"))
	{
		m_prisonerTo = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "freeroam" ) )
	{
		m_freeRoam = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "size_for_grapple" ) )
	{
		m_sizeForGrapple = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "gib_policy" ) )
	{
		m_gibPolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "ignoredby" ) )
	{
		m_ignoredBy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerTarget" ) )
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerCondition" ) )
	{
		m_iTriggerCondition = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerAltCondition" ) )
	{
		m_iTriggerAltCondition = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "UseSentence" ) )
	{
		m_iszUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "UnUseSentence" ) )
	{
		m_iszUnUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq( pkvd->szKeyName, "RefusalSentence" ))
	{
		m_iszDecline = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "provoked_state"))
	{
		m_provokedState = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "tolerance"))
	{
		m_iTolerance = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "ignore_push_flag"))
	{
		m_ignorePushFlag = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CTriggerConfigureMonster::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(pev->target))
	{
		ALERT(at_console, "%s with null target!\n", STRING(pev->classname));
		return;
	}
	CBaseEntity* pEntity = NULL;
	while((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->target), pActivator)) != NULL)
	{
		Affect(pEntity);
	}
}

struct ConfiguredFlag
{
	ConfiguredFlag(short myValue, int myFlagBit) : value(myValue), flagBit(myFlagBit) {}
	short value;
	int flagBit;
};

void CTriggerConfigureMonster::Affect(CBaseEntity *pEntity)
{
	if (!pEntity)
		return;
	if (pEntity->IsPlayer())
		return; // refuse to change player's parameters
	CBaseMonster* pMonster = pEntity->MyMonsterPointer();
	if (!pMonster)
		return;

	const ConfiguredFlag flagParams[] = {
		ConfiguredFlag(m_gagFlag, SF_MONSTER_GAG),
		ConfiguredFlag(m_prisonerFlag, SF_MONSTER_PRISONER),
		ConfiguredFlag(m_predisasterFlag, SF_MONSTER_PREDISASTER),
		ConfiguredFlag(m_dontDropGunFlag, SF_MONSTER_DONT_DROP_GUN),
		ConfiguredFlag(m_actOutOfPVS, SF_MONSTER_ACT_OUT_OF_PVS),
		ConfiguredFlag(m_ignorePushFlag, SF_MONSTER_IGNORE_PUSH),
	};

	for (const auto& flagParam : flagParams)
	{
		if (flagParam.value < 0)
			ClearBits(pMonster->pev->spawnflags, flagParam.flagBit);
		else if (flagParam.value > 0)
			SetBits(pMonster->pev->spawnflags, flagParam.flagBit);
	}

	if (m_monsterClipFlag < 0)
	{
		ClearBits(pMonster->pev->spawnflags, SF_MONSTER_HITMONSTERCLIP);
		ClearBits(pMonster->pev->flags, FL_MONSTERCLIP);
	}
	else if (m_monsterClipFlag > 0)
	{
		SetBits(pMonster->pev->spawnflags, SF_MONSTER_HITMONSTERCLIP);
		SetBits(pMonster->pev->flags, FL_MONSTERCLIP);
	}

	MaySetResettableValue2(pMonster->m_iClass, m_iClass);
	MaySetResettableValue2(pMonster->m_customSoundMask, m_soundMask);

	if (m_bloodColor)
		pMonster->m_bloodColor = m_bloodColor;

	MaySetResettableValue(pMonster->m_prisonerTo, m_prisonerTo);
	MaySetResettableValue(pMonster->m_freeRoam, m_freeRoam);
	MaySetResettableValue2(pMonster->m_sizeForGrapple, m_sizeForGrapple);
	MaySetResettableValue(pMonster->m_gibPolicy, m_gibPolicy);
	MaySetResettableValue(pMonster->m_ignoredBy, m_ignoredBy);

	MaySetResettableValue(pMonster->m_iTriggerCondition, m_iTriggerCondition);
	MaySetResettableValue(pMonster->m_iTriggerAltCondition, m_iTriggerAltCondition);

	if (!FStringNull(m_iszTriggerTarget))
	{
		if (FStrEq(STRING(m_iszTriggerTarget), "null"))
			pMonster->m_iszTriggerTarget = iStringNull;
		else
			pMonster->m_iszTriggerTarget = m_iszTriggerTarget;
	}

	CTalkMonster* pTalkMonster = pMonster->MyTalkMonsterPointer();
	if (pTalkMonster)
	{
		if (m_provokedState < 0)
		{
			pTalkMonster->Forget(bits_MEMORY_PROVOKED|bits_MEMORY_SUSPICIOUS);
		}
		else if (m_provokedState > 0)
		{
			pTalkMonster->Remember( bits_MEMORY_PROVOKED );
			pTalkMonster->StopFollowing( true );
		}

		if (!FStringNull(m_iszUse))
		{
			pTalkMonster->m_iszUse = m_iszUse;
		}

		if (!FStringNull(m_iszUnUse))
		{
			pTalkMonster->m_iszUnUse = m_iszUnUse;
		}

		if (!FStringNull(m_iszDecline))
		{
			pTalkMonster->m_iszDecline = m_iszDecline;
		}

		MaySetResettableValue(pTalkMonster->m_iTolerance, m_iTolerance);
	}
}

#define SF_TRIGGERLOOK_FIREONCE 1

class CTriggerLook : public CBaseDelay
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn();
	int ObjectCaps() { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Touch(CBaseEntity* pOther);

	EHANDLE m_hLookTarget;
	float m_flFieldOfView;
	float m_flLookTime;
	float m_flLookTimeTotal;
	float m_flLookTimeLast;
	float m_flTouchTimeLast;
	string_t m_sMaster;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( trigger_look, CTriggerLook )

TYPEDESCRIPTION	CTriggerLook::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerLook, m_hLookTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerLook, m_flFieldOfView, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerLook, m_flLookTime, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerLook, m_flLookTimeTotal, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerLook, m_flLookTimeLast, FIELD_TIME ),
	DEFINE_FIELD( CTriggerLook, m_flTouchTimeLast, FIELD_TIME ),
	DEFINE_FIELD( CTriggerLook, m_sMaster, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CTriggerLook, CBaseDelay )

void CTriggerLook::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "field_of_view" ) )
	{
		m_flFieldOfView = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "look_time" ) )
	{
		m_flLookTime = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerLook::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_flLookTimeTotal = -1.0f;
}

void CTriggerLook::Touch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer() || FStringNull(pev->netname))
	{
		return;
	}
	if( !FStringNull(m_sMaster) && !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
	{
		return;
	}

	if (m_hLookTarget == 0)
	{
		m_hLookTarget = UTIL_FindEntityByTargetname(0, STRING(pev->netname));
		if (m_hLookTarget == 0)
		{
			return;
		}
	}

	if (gpGlobals->time - m_flTouchTimeLast >= 0.2f )
	{
		// haven't been touched for a while
		m_flLookTimeTotal = -1.0f;
	}

	m_flTouchTimeLast = gpGlobals->time;

	UTIL_MakeVectors( pOther->pev->v_angle );
	const Vector vLookDir = gpGlobals->v_forward;
	const Vector vTargetDir = (m_hLookTarget->pev->origin - pOther->EyePosition()).Normalize();

	const float fDotPr = DotProduct(vLookDir, vTargetDir);
	if (fDotPr > m_flFieldOfView)
	{
		if (m_flLookTimeTotal == -1.0f)
		{
			m_flLookTimeLast = gpGlobals->time;
			m_flLookTimeTotal = 0;
		}
		else
		{
			m_flLookTimeTotal += gpGlobals->time - m_flLookTimeLast;
			m_flLookTimeLast = gpGlobals->time;
		}

		if (m_flLookTimeTotal >= m_flLookTime)
		{
			SUB_UseTargets(pOther);
			m_flLookTimeTotal = -1.0f;
			if (FBitSet(pev->spawnflags, SF_TRIGGERLOOK_FIREONCE))
			{
				SetThink(&CBaseEntity::SUB_Remove);
				pev->nextthink = gpGlobals->time;
			}
		}
	}
	else
	{
		m_flLookTimeTotal = -1.0f;
	}
}

class CTriggerCheckState : public CPointEntity
{
public:
	void KeyValue(KeyValueData *pkvd)
	{
		if(FStrEq(pkvd->szKeyName, "entity"))
		{
			m_entity = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if(FStrEq(pkvd->szKeyName, "fire_if_off"))
		{
			m_fireIfOff = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if(FStrEq(pkvd->szKeyName, "fire_if_on"))
		{
			m_fireIfOn = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if(FStrEq(pkvd->szKeyName, "fire_if_absent"))
		{
			m_fireIfAbsent = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
		if (FStringNull(m_entity))
			return;
		CBaseEntity* pEntity = UTIL_FindEntityByTargetname(NULL, STRING(m_entity), this);
		if (pEntity)
		{
			const bool state = pEntity->IsTriggered(pActivator);
			if (pev->target)
				FireTargets(STRING(pev->target), pActivator, this, state ? USE_ON : USE_OFF);
			if (m_fireIfOff && !state)
				FireTargets(STRING(m_fireIfOff), pActivator, this);
			if (m_fireIfOn && state)
				FireTargets(STRING(m_fireIfOn), pActivator, this);
		}
		else
		{
			if (m_fireIfAbsent)
				FireTargets(STRING(m_fireIfAbsent), pActivator, this);
		}
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
protected:
	string_t m_entity;
	string_t m_fireIfOff;
	string_t m_fireIfOn;
	string_t m_fireIfAbsent;
};

TYPEDESCRIPTION CTriggerCheckState::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerCheckState, m_entity, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCheckState, m_fireIfOff, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCheckState, m_fireIfOn, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCheckState, m_fireIfAbsent, FIELD_STRING ),
};
IMPLEMENT_SAVERESTORE( CTriggerCheckState, CPointEntity )

LINK_ENTITY_TO_CLASS( trigger_check_state, CTriggerCheckState )

enum
{
	COMPARISON_TEST_EQUAL = 0,
	COMPARISON_TEST_GTE,
	COMPARISON_TEST_GREATER,
	COMPARISON_TEST_LTE,
	COMPARISON_TEST_LESS,
	COMPARISON_TEST_NOT_EQUAL,
};

class CTriggerCompare : public CPointEntity
{
public:
	int ObjectCaps( void ) { return CPointEntity::ObjectCaps() | FCAP_MASTER; }
	void KeyValue(KeyValueData *pkvd)
	{
		if( FStrEq( pkvd->szKeyName, "testtype" ) )
		{
			pev->impulse = atoi( pkvd->szValue );
			pkvd->fHandled = true;
		}
		else if (FStrEq(pkvd->szKeyName, "pass_target"))
		{
			m_onComparisonPass = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if (FStrEq(pkvd->szKeyName, "fail_target"))
		{
			m_onComparisonFail = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if( FStrEq( pkvd->szKeyName, "trigger_on_equal" ) )
		{
			m_onEqual = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = true;
		}
		else if( FStrEq( pkvd->szKeyName, "trigger_on_not_equal" ) )
		{
			m_onNotEqual = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = true;
		}
		else if( FStrEq( pkvd->szKeyName, "trigger_on_less" ) )
		{
			m_onLessThan = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = true;
		}
		else if( FStrEq( pkvd->szKeyName, "trigger_on_greater" ) )
		{
			m_onGreaterThan = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = true;
		}
		else if( FStrEq( pkvd->szKeyName, "trigger_on_fail" ) )
		{
			m_onFail = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = true;
		}
		else if( FStrEq( pkvd->szKeyName, "initial_value" ) )
		{
			if (IsLikelyNumber(pkvd->szValue))
			{
				m_value = atof( pkvd->szValue );
				m_valueIsNumber = 1;
			}
			else
			{
				m_valueSource = ALLOC_STRING(pkvd->szValue);
				m_valueIsNumber = 0;
			}
			pkvd->fHandled = true;
		}
		else if( FStrEq( pkvd->szKeyName, "compare_value" ) )
		{
			if (IsLikelyNumber(pkvd->szValue))
			{
				m_compareValue = atof( pkvd->szValue );
				m_compareValueIsNumber = 1;
			}
			else
			{
				m_compareValueSource = ALLOC_STRING(pkvd->szValue);
				m_compareValueIsNumber = 0;
			}
			pkvd->fHandled = true;
		}
		else
			CPointEntity::KeyValue( pkvd );
	}

	float CalcBaseValue(bool& success, CBaseEntity* pActivator)
	{
		return CalcValue(m_value, m_valueSource, m_valueIsNumber, success, pActivator);
	}
	float CalcCompareValue(bool& success, CBaseEntity* pActivator)
	{
		return CalcValue(m_compareValue, m_compareValueSource, m_compareValueIsNumber, success, pActivator);
	}

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
		bool valueSuccess;
		const float baseValue = CalcBaseValue(valueSuccess, pActivator);
		bool compareValueSuccess;
		const float compareValue = CalcCompareValue(compareValueSuccess, pActivator);

		if (!valueSuccess || !compareValueSuccess)
		{
			if (m_onFail)
				FireTargets(STRING(m_onFail), pActivator, this);
			return;
		}

		const int compareResult = DoCompareValues(baseValue, compareValue);

		if (m_onComparisonPass || m_onComparisonFail)
		{
			bool passed = IsComparisonPassed(compareResult);
			if (passed && m_onComparisonPass) {
				FireTargets(STRING(m_onComparisonPass), pActivator, this);
			}
			if (!passed && m_onComparisonFail) {
				FireTargets(STRING(m_onComparisonFail), pActivator, this);
			}
		}

		if (compareResult == 0 && m_onEqual)
		{
			FireTargets(STRING(m_onEqual), pActivator, this);
		}
		if (compareResult != 0 && m_onNotEqual)
		{
			FireTargets(STRING(m_onNotEqual), pActivator, this);
		}
		if (compareResult < 0 && m_onLessThan)
		{
			FireTargets(STRING(m_onLessThan), pActivator, this);
		}
		if (compareResult > 0 && m_onGreaterThan)
		{
			FireTargets(STRING(m_onGreaterThan), pActivator, this);
		}
	}

	bool IsTriggered(CBaseEntity *pActivator)
	{
		bool valueSuccess;
		const float baseValue = CalcBaseValue(valueSuccess, pActivator);
		bool compareValueSuccess;
		const float compareValue = CalcCompareValue(compareValueSuccess, pActivator);

		if (!valueSuccess || !compareValueSuccess) {
			return false;
		}

		const int compareResult = DoCompareValues(baseValue, compareValue);
		return IsComparisonPassed(compareResult);
	}

	string_t m_onComparisonPass;
	string_t m_onComparisonFail;

	string_t m_onEqual;
	string_t m_onNotEqual;
	string_t m_onLessThan;
	string_t m_onGreaterThan;

	string_t m_onFail;
	float m_value;
	float m_compareValue;
	string_t m_valueSource;
	string_t m_compareValueSource;
	byte m_valueIsNumber;
	byte m_compareValueIsNumber;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

private:
	float CalcValue(float numValue, string_t valueSource, byte valueIsNumber, bool& success, CBaseEntity* pActivator)
	{
		if (valueIsNumber)
		{
			success = true;
			return numValue;
		}
		else
		{
			float result = 0;
			success = TryCalcLocus_Ratio(pActivator, STRING(valueSource), result);
			return result;
		}
	}
	int DoCompareValues(float a, float b)
	{
		if (a == b) {
			return 0;
		} else if (a < b){
			return -1;
		} else {
			return 1;
		}
	}
	bool IsComparisonPassed(int compareResult)
	{
		bool success = false;
		switch (pev->impulse) {
		case COMPARISON_TEST_EQUAL:
			success = compareResult == 0;
			break;
		case COMPARISON_TEST_GREATER:
			success = compareResult > 0;
			break;
		case COMPARISON_TEST_GTE:
			success = compareResult >= 0;
			break;
		case COMPARISON_TEST_LESS:
			success = compareResult < 0;
			break;
		case COMPARISON_TEST_LTE:
			success = compareResult <= 0;
			break;
		case COMPARISON_TEST_NOT_EQUAL:
			success = compareResult != 0;
			break;
		default:
			ALERT(at_error, "Unknown test type in %s: %d\n", STRING(pev->classname), pev->impulse);
			break;
		}
		return success;
	}
};

LINK_ENTITY_TO_CLASS( trigger_compare, CTriggerCompare )

TYPEDESCRIPTION	CTriggerCompare::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerCompare, m_onComparisonPass, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_onComparisonFail, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_onEqual, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_onNotEqual, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_onLessThan, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_onGreaterThan, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_onFail, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_value, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCompare, m_compareValue, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCompare, m_valueSource, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_compareValueSource, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCompare, m_valueIsNumber, FIELD_CHARACTER ),
	DEFINE_FIELD( CTriggerCompare, m_compareValueIsNumber, FIELD_CHARACTER ),
};
IMPLEMENT_SAVERESTORE( CTriggerCompare, CPointEntity )

class CTriggerSkillTest : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool CalcRatio(CBaseEntity *pLocus, float *outResult) {
		*outResult = g_iSkillLevel;
		return true;
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_triggerOnEasy;
	string_t m_triggerOnMedium;
	string_t m_triggerOnHard;
};

LINK_ENTITY_TO_CLASS( trigger_skill_test, CTriggerSkillTest )

TYPEDESCRIPTION	CTriggerSkillTest::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerSkillTest, m_triggerOnEasy, FIELD_STRING ),
	DEFINE_FIELD( CTriggerSkillTest, m_triggerOnMedium, FIELD_STRING ),
	DEFINE_FIELD( CTriggerSkillTest, m_triggerOnHard, FIELD_STRING ),
};
IMPLEMENT_SAVERESTORE( CTriggerSkillTest, CPointEntity )

void CTriggerSkillTest::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "testtype" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "comparand" ) )
	{
		pev->button = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "target_on_fail" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_on_easy" ) )
	{
		m_triggerOnEasy = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_on_medium" ) )
	{
		m_triggerOnMedium = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_on_hard" ) )
	{
		m_triggerOnHard = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTriggerSkillTest::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	const int skillLevel = g_iSkillLevel;
	const int comparand = pev->button;

	bool success = false;

	switch (pev->impulse) {
	case COMPARISON_TEST_EQUAL:
		success = skillLevel == comparand;
		break;
	case COMPARISON_TEST_GREATER:
		success = skillLevel > comparand;
		break;
	case COMPARISON_TEST_GTE:
		success = skillLevel >= comparand;
		break;
	case COMPARISON_TEST_LESS:
		success = skillLevel < comparand;
		break;
	case COMPARISON_TEST_LTE:
		success = skillLevel <= comparand;
		break;
	case COMPARISON_TEST_NOT_EQUAL:
		success = skillLevel != comparand;
		break;
	default:
		ALERT(at_error, "Unknown test type in %s: %d\n", STRING(pev->classname), pev->impulse);
		return;
	}

	if (success && pev->target) {
		FireTargets(STRING(pev->target), pActivator, this);
	}
	if (!success && pev->message) {
		FireTargets(STRING(pev->message), pActivator, this);
	}

	const std::pair<int, string_t> skillTriggers[] = {
		{SKILL_EASY, m_triggerOnEasy},
		{SKILL_MEDIUM, m_triggerOnMedium},
		{SKILL_HARD, m_triggerOnHard},
	};

	for (const auto& skillTrigger : skillTriggers)
	{
		if (skillTrigger.second && skillTrigger.first == skillLevel)
		{
			FireTargets(STRING(skillTrigger.second), pActivator, this);
			break;
		}
	}
}
