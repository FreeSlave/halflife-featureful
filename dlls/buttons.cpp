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

===== buttons.cpp ========================================================

  button-related code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "doors.h"
#include "soundradius.h"
#include "game.h"
#include "common_soundscripts.h"
#include "error_collector.h"

#define SF_BUTTON_DONTMOVE		1
#define SF_ROTBUTTON_NOTSOLID		1
#define SF_BUTTON_ONLYDIRECT	16	// sohl compatibility flag
#define	SF_BUTTON_TOGGLE		32	// button stays pushed until reactivated
#define	SF_BUTTON_SPARK_IF_OFF		64	// button sparks in OFF state
#define SF_BUTTON_TOUCH_ONLY		256	// button only fires as a result of USE key.
#define SF_BUTTON_PLAYER_CANT_USE_OLD	512
#define SF_BUTTON_CHECK_MASTER_ON_TOGGLE_RETURN 1024 // Check master and play locked and unlocked sounds on toggle return
#define SF_BUTTON_PLAYER_CANT_USE	16384 // Player can't impulse use this button

#define SF_GLOBAL_SET			1	// Set global state to initial state on spawn
#define SF_GLOBAL_ACT_AS_MASTER 4

constexpr FloatRange sparkVolumeRange(0.1f, 0.3f);

class CEnvGlobal : public CPointEntity
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int	ObjectCaps() override {
		int caps = CPointEntity::ObjectCaps();
		if (FBitSet(pev->spawnflags, SF_GLOBAL_ACT_AS_MASTER))
			caps |= FCAP_MASTER;
		return caps;
	}

	bool IsTriggered(CBaseEntity *pActivator) override {
		if (FBitSet(pev->spawnflags, SF_GLOBAL_ACT_AS_MASTER))
		{
			return gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON;
		}
		return CPointEntity::IsTriggered(pActivator);
	}
	bool CalcRatio( CBaseEntity *pLocus, float* outResult ) override {
		const globalentity_t *pEnt = gGlobalState.EntityFromTable( m_globalstate );
		if( pEnt )
		{
			*outResult = pEnt->value;
			return true;
		}
		return false;
	}

	string_t m_globalstate;
	int m_triggermode;
	int m_initialstate;
	int m_initialvalue;
};

TYPEDESCRIPTION CEnvGlobal::m_SaveData[] =
{
	DEFINE_FIELD( CEnvGlobal, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CEnvGlobal, m_triggermode, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvGlobal, m_initialstate, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvGlobal, m_initialvalue, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvGlobal, CPointEntity )

LINK_ENTITY_TO_CLASS( env_global, CEnvGlobal )

void CEnvGlobal::KeyValue( KeyValueData *pkvd )
{
	pkvd->fHandled = true;

	if( FStrEq( pkvd->szKeyName, "globalstate" ) )		// State name
		m_globalstate = ALLOC_STRING( pkvd->szValue );
	else if( FStrEq( pkvd->szKeyName, "triggermode" ) )
		m_triggermode = atoi( pkvd->szValue );
	else if( FStrEq( pkvd->szKeyName, "initialstate" ) )
		m_initialstate = atoi( pkvd->szValue );
	else if( FStrEq( pkvd->szKeyName, "initialvalue" ) )
		m_initialvalue = atoi( pkvd->szValue );
	else
		CPointEntity::KeyValue( pkvd );
}

void CEnvGlobal::Spawn()
{
	if( !m_globalstate )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}
	if( FBitSet( pev->spawnflags, SF_GLOBAL_SET ) )
	{
		if( !gGlobalState.EntityInTable( m_globalstate ) )
			gGlobalState.EntityAdd( m_globalstate, gpGlobals->mapname, (GLOBALESTATE)m_initialstate, m_initialvalue );
	}
}

typedef enum
{
	GLOBAL_TRIGGER_MODE_OFF,
	GLOBAL_TRIGGER_MODE_ON,
	GLOBAL_TRIGGER_MODE_DEAD,
	GLOBAL_TRIGGER_MODE_TOGGLE,
	GLOBAL_TRIGGER_MODE_MODIFY_VALUE,
	GLOBAL_TRIGGER_MODE_OBEY_USE_TYPE,
} GLOBAL_TRIGGER_MODE;

void CEnvGlobal::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_triggermode == GLOBAL_TRIGGER_MODE_MODIFY_VALUE)
	{
		if( !gGlobalState.EntityInTable( m_globalstate ) )
		{
			gGlobalState.EntityAdd( m_globalstate, gpGlobals->mapname, (GLOBALESTATE)m_initialstate, m_initialvalue );
		}
		if (useType == USE_ON || useType == USE_TOGGLE)
		{
			gGlobalState.IncrementValue( m_globalstate );
		}
		else if (useType == USE_OFF)
		{
			gGlobalState.DecrementValue( m_globalstate );
		}
		else if (useType == USE_SET)
		{
			gGlobalState.SetValue( m_globalstate, value );
		}
	}
	else
	{
		GLOBALESTATE oldState = gGlobalState.EntityGetState( m_globalstate );
		GLOBALESTATE newState;

		int triggerMode = m_triggermode;
		if (triggerMode == GLOBAL_TRIGGER_MODE_OBEY_USE_TYPE)
		{
			if (useType == USE_OFF)
			{
				triggerMode = GLOBAL_TRIGGER_MODE_OFF;
			}
			else if (useType == USE_ON)
			{
				triggerMode = GLOBAL_TRIGGER_MODE_ON;
			}
			else
			{
				triggerMode = GLOBAL_TRIGGER_MODE_TOGGLE;
			}
		}

		switch( triggerMode )
		{
		case GLOBAL_TRIGGER_MODE_OFF:
			newState = GLOBAL_OFF;
			break;
		case GLOBAL_TRIGGER_MODE_ON:
			newState = GLOBAL_ON;
			break;
		case GLOBAL_TRIGGER_MODE_DEAD:
			newState = GLOBAL_DEAD;
			break;
		default:
		case GLOBAL_TRIGGER_MODE_TOGGLE:
			if( oldState == GLOBAL_ON )
				newState = GLOBAL_OFF;
			else if( oldState == GLOBAL_OFF )
				newState = GLOBAL_ON;
			else
				newState = oldState;
		}

		if( gGlobalState.EntityInTable( m_globalstate ) )
			gGlobalState.EntitySetState( m_globalstate, newState );
		else
			gGlobalState.EntityAdd( m_globalstate, gpGlobals->mapname, newState, m_initialvalue );
	}
}

//==================================================
//LRC- a simple entity, just maintains a state
//==================================================

#define SF_ENVSTATE_START_ON		1
#define SF_ENVSTATE_DEBUG			2

enum
{
	STATE_OFF,
	STATE_TURN_OFF,
	STATE_ON,
	STATE_TURN_ON,
};

class CEnvState : public CPointEntity
{
public:
	void	Spawn() override;
	void	Think() override;
	void	KeyValue( KeyValueData *pkvd ) override;
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int		ObjectCaps() override { return CPointEntity::ObjectCaps() | FCAP_MASTER; }

	bool	IsLockedByMaster() override { return !UTIL_IsMasterTriggered(m_sMaster, NULL); }

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	bool	IsTriggered(CBaseEntity *pActivator) override { return m_iState == STATE_ON; }

	int			m_iState;
	float		m_fTurnOnTime;
	float		m_fTurnOffTime;
	string_t	m_sMaster;
	string_t	m_fireWhenOn;
	string_t	m_fireWhenOff;
};

void CEnvState::Spawn()
{
	if (pev->spawnflags & SF_ENVSTATE_START_ON)
		m_iState = STATE_ON;
	else
		m_iState = STATE_OFF;
	m_fireWhenOn = pev->noise1;
	pev->noise1 = iStringNull;
	m_fireWhenOff = pev->noise2;
	pev->noise2 = iStringNull;
}

TYPEDESCRIPTION CEnvState::m_SaveData[] =
{
	DEFINE_FIELD( CEnvState, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvState, m_fTurnOnTime, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvState, m_fTurnOffTime, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvState, m_sMaster, FIELD_STRING ),
	DEFINE_FIELD( CEnvState, m_fireWhenOn, FIELD_STRING ),
	DEFINE_FIELD( CEnvState, m_fireWhenOff, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CEnvState, CPointEntity )

LINK_ENTITY_TO_CLASS( env_state, CEnvState )

void CEnvState::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq(pkvd->szKeyName, "turnontime") )
	{
		m_fTurnOnTime = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "turnofftime") )
	{
		m_fTurnOffTime = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "master") )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CEnvState::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!ShouldToggle(useType, m_iState == STATE_ON) || IsLockedByMaster())
	{
		if (pev->spawnflags & SF_ENVSTATE_DEBUG)
		{
			ALERT(at_console,"DEBUG: env_state \"%s\" ",STRING(pev->targetname));
			if (IsLockedByMaster())
				ALERT(at_console,"ignored trigger %s; locked by master \"%s\".\n",UseTypeToString(useType),STRING(m_sMaster));
			else if (useType == USE_ON)
				ALERT(at_console,"ignored trigger USE_ON; already on\n");
			else if (useType == USE_OFF)
				ALERT(at_console,"ignored trigger USE_OFF; already off\n");
			else
				ALERT(at_console,"ignored trigger %s.\n",UseTypeToString(useType));
		}
		return;
	}

	switch (m_iState)
	{
	case STATE_ON:
	case STATE_TURN_ON:
		if (m_fTurnOffTime)
		{
			m_iState = STATE_TURN_OFF;
			if (pev->spawnflags & SF_ENVSTATE_DEBUG)
			{
				ALERT(at_console,"DEBUG: env_state \"%s\" triggered; will turn off in %f seconds.\n", STRING(pev->targetname), m_fTurnOffTime);
			}
			pev->nextthink = gpGlobals->time + m_fTurnOffTime;
		}
		else
		{
			m_iState = STATE_OFF;
			if (pev->spawnflags & SF_ENVSTATE_DEBUG)
			{
				ALERT(at_console,"DEBUG: env_state \"%s\" triggered, turned off", STRING(pev->targetname));
				if (pev->target)
				{
					ALERT(at_console,": firing \"%s\"",STRING(pev->target));
					if (m_fireWhenOff)
						ALERT(at_console," and \"%s\"",STRING(m_fireWhenOff));
				}
				else if (m_fireWhenOff)
					ALERT(at_console,": firing \"%s\"",STRING(m_fireWhenOff));
				ALERT(at_console,".\n");
			}
			FireTargets(STRING(pev->target),pActivator,this,USE_OFF);
			FireTargets(STRING(m_fireWhenOff),pActivator,this);
			pev->nextthink = -1;
		}
		break;
	case STATE_OFF:
	case STATE_TURN_OFF:
		if (m_fTurnOnTime)
		{
			m_iState = STATE_TURN_ON;
			if (pev->spawnflags & SF_ENVSTATE_DEBUG)
			{
				ALERT(at_console,"DEBUG: env_state \"%s\" triggered; will turn on in %f seconds.\n", STRING(pev->targetname), m_fTurnOnTime);
			}
			pev->nextthink = gpGlobals->time + m_fTurnOnTime;
		}
		else
		{
			m_iState = STATE_ON;
			if (pev->spawnflags & SF_ENVSTATE_DEBUG)
			{
				ALERT(at_console,"DEBUG: env_state \"%s\" triggered, turned on",STRING(pev->targetname));
				if (pev->target)
				{
					ALERT(at_console,": firing \"%s\"",STRING(pev->target));
					if (m_fireWhenOn)
						ALERT(at_console," and \"%s\"",STRING(m_fireWhenOn));
				}
				else if (m_fireWhenOn)
					ALERT(at_console,": firing \"%s\"", STRING(m_fireWhenOn));
				ALERT(at_console,".\n");
			}
			FireTargets(STRING(pev->target),pActivator,this,USE_ON);
			FireTargets(STRING(m_fireWhenOn),pActivator,this);
			pev->nextthink = -1;
		}
		break;
	default:
		break;
	}
}

void CEnvState::Think()
{
	if (m_iState == STATE_TURN_ON)
	{
		m_iState = STATE_ON;
		if (pev->spawnflags & SF_ENVSTATE_DEBUG)
		{
			ALERT(at_console,"DEBUG: env_state \"%s\" turned itself on",STRING(pev->targetname));
			if (pev->target)
			{
				ALERT(at_console,": firing %s",STRING(pev->target));
				if (m_fireWhenOn)
					ALERT(at_console," and %s",STRING(m_fireWhenOn));
			}
			else if (m_fireWhenOn)
				ALERT(at_console,": firing %s",STRING(m_fireWhenOn));
			ALERT(at_console,".\n");
		}
		FireTargets(STRING(pev->target),this,this,USE_ON);
		FireTargets(STRING(m_fireWhenOn),this,this);
	}
	else if (m_iState == STATE_TURN_OFF)
	{
		m_iState = STATE_OFF;
		if (pev->spawnflags & SF_ENVSTATE_DEBUG)
		{
			ALERT(at_console,"DEBUG: env_state \"%s\" turned itself off",STRING(pev->targetname));
			if (pev->target)
			{
				ALERT(at_console,": firing %s",STRING(pev->target));
				if (m_fireWhenOff)
					ALERT(at_console," and %s",STRING(m_fireWhenOff));
			}
			else if (m_fireWhenOff)
				ALERT(at_console,": firing %s",STRING(m_fireWhenOff));
			ALERT(at_console,".\n");
		}
		FireTargets(STRING(pev->target),this,this,USE_OFF);
		FireTargets(STRING(m_fireWhenOff),this,this);
	}
}

class CCalcState : public CPointEntity
{
public:
	enum {
		STATE_LOGIC_AND = 0,
		STATE_LOGIC_OR,
		STATE_LOGIC_NAND,
		STATE_LOGIC_NOR,
		STATE_LOGIC_XOR,
		STATE_LOGIC_NXOR
	};

	static const char* OperationDisplayName(int operationId)
	{
		switch (operationId) {
		case STATE_LOGIC_AND:
			return "AND";
		case STATE_LOGIC_OR:
			return "OR";
		case STATE_LOGIC_NAND:
			return "NAND";
		case STATE_LOGIC_NOR:
			return "NOR";
		case STATE_LOGIC_XOR:
			return "XOR";
		case STATE_LOGIC_NXOR:
			return "NXOR";
		default:
			return "Invalid";
		}
	}

	enum {
		STATE_FALLBACK_ERROR,
		STATE_FALLBACK_OFF,
		STATE_FALLBACK_ON
	};

	void KeyValue( KeyValueData *pkvd ) override;
	bool IsTriggered(CBaseEntity *pActivator) override { return CalcState(pActivator, false); }
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	int	ObjectCaps() override { return CPointEntity::ObjectCaps() | FCAP_MASTER; }

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

protected:
	bool CalcState(CBaseEntity *pActivator, bool isUse);
	bool DoOperation(bool leftOp, bool rightOp, int operationId);

	string_t m_left;
	string_t m_right;
	int m_operation;
	byte m_leftFallback;
	byte m_rightFallback;
	string_t m_fireWhenFalse;
	string_t m_fireWhenTrue;
};

TYPEDESCRIPTION CCalcState::m_SaveData[] =
{
	DEFINE_FIELD( CCalcState, m_left, FIELD_STRING ),
	DEFINE_FIELD( CCalcState, m_right, FIELD_STRING ),
	DEFINE_FIELD( CCalcState, m_operation, FIELD_INTEGER ),
	DEFINE_FIELD( CCalcState, m_leftFallback, FIELD_CHARACTER ),
	DEFINE_FIELD( CCalcState, m_rightFallback, FIELD_CHARACTER ),
	DEFINE_FIELD( CCalcState, m_fireWhenFalse, FIELD_STRING ),
	DEFINE_FIELD( CCalcState, m_fireWhenTrue, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcState, CPointEntity )

LINK_ENTITY_TO_CLASS( calc_state, CCalcState )

void CCalcState::KeyValue(KeyValueData *pkvd)
{
	if(strcmp(pkvd->szKeyName, "operation") == 0)
	{
		m_operation = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(strcmp(pkvd->szKeyName, "left_operand") == 0)
	{
		m_left = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(strcmp(pkvd->szKeyName, "right_operand") == 0)
	{
		m_right = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(strcmp(pkvd->szKeyName, "left_fallback") == 0)
	{
		m_leftFallback = (byte)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(strcmp(pkvd->szKeyName, "right_fallback") == 0)
	{
		m_rightFallback = (byte)atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(strcmp(pkvd->szKeyName, "fire_when_false") == 0)
	{
		m_fireWhenFalse = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if(strcmp(pkvd->szKeyName, "fire_when_true") == 0)
	{
		m_fireWhenTrue = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CCalcState::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CalcState(pActivator, true);
}

bool CCalcState::CalcState(CBaseEntity* pActivator, bool isUse)
{
	if (FStringNull(m_left) || FStringNull(m_right))
	{
		ALERT(at_error, "%s needs both left and right operands defined\n", STRING(pev->classname));
		return false;
	}

	CBaseEntity *pLeft = UTIL_FindEntityByTargetname(NULL, STRING(m_left), pActivator);
	CBaseEntity *pRight = UTIL_FindEntityByTargetname(NULL, STRING(m_right), pActivator);

	bool leftOp, rightOp;

	if (pLeft)
	{
		leftOp = pLeft->IsTriggered(pActivator);
	}
	else
	{
		if (m_leftFallback == STATE_FALLBACK_OFF)
		{
			leftOp = false;
		}
		else if (m_leftFallback == STATE_FALLBACK_ON)
		{
			leftOp = true;
		}
		else
		{
			ALERT(at_error, "%s: left operand '%s' doesn't exist!\n", STRING(pev->classname), STRING(m_left));
			return false;
		}
	}
	if (isUse && FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
	{
		ALERT(at_console, "%s '%s': left operand ('%s') evaluated to %s\n", STRING(pev->classname), STRING(pev->targetname), STRING(m_left), leftOp ? "true" : "false");
	}

	if (pRight)
	{
		rightOp = pRight->IsTriggered(pActivator);
	}
	else
	{
		if (m_rightFallback == STATE_FALLBACK_OFF)
		{
			rightOp = false;
		}
		else if (m_rightFallback == STATE_FALLBACK_ON)
		{
			rightOp = true;
		}
		else
		{
			ALERT(at_error, "%s: right operand '%s' doesn't exist!\n", STRING(pev->classname), STRING(m_right));
			return false;
		}
	}
	if (isUse && FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
	{
		ALERT(at_console, "%s '%s': right operand ('%s') evaluated to %s\n", STRING(pev->classname), STRING(pev->targetname), STRING(m_right), rightOp ? "true" : "false");
	}

	const bool result = DoOperation(leftOp, rightOp, m_operation);

	if (isUse && FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
	{
		ALERT(at_console, "%s '%s': operation %s evaluated to %s\n", STRING(pev->classname), STRING(pev->targetname), OperationDisplayName(m_operation), result ? "true" : "false");
	}

	if (isUse)
	{
		if (pev->target)
			FireTargets(STRING(pev->target), pActivator, this, result ? USE_ON : USE_OFF);
		if (m_fireWhenFalse && !result)
			FireTargets(STRING(m_fireWhenFalse), pActivator, this);
		if (m_fireWhenTrue && result)
			FireTargets(STRING(m_fireWhenTrue), pActivator, this);
	}
	return result;
}

bool CCalcState::DoOperation(bool leftOp, bool rightOp, int operationId)
{
	switch (operationId) {
	case STATE_LOGIC_AND:
		return leftOp && rightOp;
	case STATE_LOGIC_OR:
		return leftOp || rightOp;
	case STATE_LOGIC_NAND:
		return !(leftOp && rightOp);
	case STATE_LOGIC_NOR:
		return !(leftOp || rightOp);
	case STATE_LOGIC_XOR:
		return leftOp ^ rightOp;
	case STATE_LOGIC_NXOR:
		return !(leftOp ^ rightOp);
	default:
		ALERT(at_error, "%s: unknown operation id %d\n", STRING(pev->classname), operationId);
		return false;
	}
}

//
// MultiSouce
//

#define MS_MAX_TARGETS		32

class CMultiSource : public CPointEntity
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override { return ( CPointEntity::ObjectCaps() | FCAP_MASTER ); }
	bool IsTriggered( CBaseEntity *pActivator ) override;
	void EXPORT Register();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_rgEntities[MS_MAX_TARGETS];
	int m_rgTriggered[MS_MAX_TARGETS];

	int m_iTotal;
	string_t m_globalstate;
};

TYPEDESCRIPTION CMultiSource::m_SaveData[] =
{
	//!!!BUGBUG FIX
	DEFINE_ARRAY( CMultiSource, m_rgEntities, FIELD_EHANDLE, MS_MAX_TARGETS ),
	DEFINE_ARRAY( CMultiSource, m_rgTriggered, FIELD_INTEGER, MS_MAX_TARGETS ),
	DEFINE_FIELD( CMultiSource, m_iTotal, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiSource, m_globalstate, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CMultiSource, CPointEntity )

LINK_ENTITY_TO_CLASS( multisource, CMultiSource )

//
// Cache user-entity-field values until spawn is called.
//
void CMultiSource::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "style" ) ||
				FStrEq( pkvd->szKeyName, "height" ) ||
				FStrEq( pkvd->szKeyName, "killtarget" ) ||
				FStrEq( pkvd->szKeyName, "value1" ) ||
				FStrEq( pkvd->szKeyName, "value2" ) ||
				FStrEq( pkvd->szKeyName, "value3" ) )
		pkvd->fHandled = true;
	else if( FStrEq( pkvd->szKeyName, "globalstate" ) )
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

#define SF_MULTI_INIT		1

void CMultiSource::Spawn()
{ 
	// set up think for later registration

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->spawnflags |= SF_MULTI_INIT;	// Until it's initialized
	SetThink( &CMultiSource::Register );
}

void CMultiSource::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	int i = 0;

	// Find the entity in our list
	while( i < m_iTotal )
		if( m_rgEntities[i++] == pCaller )
			break;

	// if we didn't find it, report error and leave
	if( i > m_iTotal )
	{
		ALERT( at_console, "MultiSrc:Used by non member %s.\n", STRING( pCaller->pev->classname ) );
		return;	
	}

	// CONSIDER: a Use input to the multisource always toggles.  Could check useType for ON/OFF/TOGGLE
	m_rgTriggered[i - 1] ^= 1;

	// 
	if( IsTriggered( pActivator ) )
	{
		ALERT( at_aiconsole, "Multisource %s enabled (%d inputs)\n", STRING( pev->targetname ), m_iTotal );
		useType = USE_TOGGLE;
		if( m_globalstate )
			useType = USE_ON;
		SUB_UseTargets( NULL, useType, 0 );
	}
}

bool CMultiSource::IsTriggered( CBaseEntity * )
{
	// Is everything triggered?
	int i = 0;

	// Still initializing?
	if( pev->spawnflags & SF_MULTI_INIT )
		return 0;

	while( i < m_iTotal )
	{
		if( m_rgTriggered[i] == 0 )
			break;
		i++;
	}

	if( i == m_iTotal )
	{
		if( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON )
			return 1;
	}

	return 0;
}

void CMultiSource::Register()
{ 
	edict_t *pentTarget = NULL;

	m_iTotal = 0;
	memset( m_rgEntities, 0, MS_MAX_TARGETS * sizeof(EHANDLE) );

	SetThink( &CBaseEntity::SUB_DoNothing );

	// search for all entities which target this multisource (pev->targetname)
	pentTarget = FIND_ENTITY_BY_STRING( NULL, "target", STRING( pev->targetname ) );

	while( !FNullEnt( pentTarget ) && ( m_iTotal < MS_MAX_TARGETS ) )
	{
		CBaseEntity *pTarget = CBaseEntity::Instance( pentTarget );
		if( pTarget )
			m_rgEntities[m_iTotal++] = pTarget;

		pentTarget = FIND_ENTITY_BY_STRING( pentTarget, "target", STRING( pev->targetname ) );
	}

	pentTarget = FIND_ENTITY_BY_STRING( NULL, "classname", "multi_manager" );
	while( !FNullEnt( pentTarget ) && ( m_iTotal < MS_MAX_TARGETS ) )
	{
		CBaseEntity *pTarget = CBaseEntity::Instance( pentTarget );
		if( pTarget && pTarget->HasTarget( pev->targetname ) )
			m_rgEntities[m_iTotal++] = pTarget;

		pentTarget = FIND_ENTITY_BY_STRING( pentTarget, "classname", "multi_manager" );
	}
	pev->spawnflags &= ~SF_MULTI_INIT;
}

enum
{
	BUTTON_USE_OFF = -1,
	BUTTON_USE_TOGGLE = 0,
	BUTTON_USE_ON = 1,
	BUTTON_USE_ON_OFF = 2,
	BUTTON_USE_OFF_ON = 3,
};

//
// Generic Button
//
class CBaseButton : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void RotSpawn();
	void KeyValue( KeyValueData* pkvd) override;

	void ButtonActivate();
	void SparkSoundCache();

	void EXPORT ButtonShot();
	void EXPORT ButtonTouch( CBaseEntity *pOther );
	void EXPORT ButtonSpark();
	void EXPORT TriggerAndWait();
	void EXPORT ButtonReturn();
	void EXPORT ButtonBackHome();
	void EXPORT ButtonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ButtonUse_IgnorePlayer( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	TakeDamageResult TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo ) override;
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	enum BUTTON_CODE { BUTTON_NOTHING, BUTTON_ACTIVATE, BUTTON_RETURN };
	BUTTON_CODE ButtonResponseToTouch();
	void OnLocked();
	bool PrepareActivation(bool doActivationCheck);
	bool IsSparkingButton();
	USE_TYPE UseType(bool returning);

	// Buttons that don't take damage can be IMPULSE used
	int ObjectCaps() override;

	bool IsAllowedToSpeak() override { return true; }
	bool IsUsefulToDisplayHint(CBaseEntity *pPlayer) override {
		return !FBitSet(pev->spawnflags, SF_BUTTON_PLAYER_CANT_USE|SF_BUTTON_PLAYER_CANT_USE_OLD);
	}

	bool m_fStayPushed;	// button stays pushed in until touched again?
	bool m_fRotating;		// a rotating button?  default is a sliding button.

	string_t m_strChangeTarget;	// if this field is not null, this is an index into the engine string array.
							// when this button is touched, it's target entity's TARGET field will be set
							// to the button's ChangeTarget. This allows you to make a func_train switch paths, etc.

	locksound_t m_ls;			// door lock sounds

	BYTE m_bLockedSound;		// ordinals from entity selection
	BYTE m_bLockedSentence;
	BYTE m_bUnlockedSound;
	BYTE m_bUnlockedSentence;
	int m_sounds;
	string_t m_targetOnLocked;
	float m_targetOnLockedTime;
	string_t m_lockedSoundOverride;
	string_t m_unlockedSoundOverride;
	string_t m_lockedSentenceOverride;
	string_t m_unlockedSentenceOverride;

	string_t m_triggerOnReturn;
	string_t m_triggerBeforeMove;

	float m_waitBeforeToggleAgain;
	float m_toggleAgainTime;

	short m_iDirectUse;
	bool m_fNonMoving;
};

static constexpr const char* sparkSoundScript = "DoSpark";

int CBaseButton::ObjectCaps()
{
	int objectCaps = (CBaseToggle:: ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	if (!pev->takedamage && !FBitSet(pev->spawnflags, SF_BUTTON_PLAYER_CANT_USE|SF_BUTTON_PLAYER_CANT_USE_OLD))
		objectCaps |= FCAP_IMPULSE_USE;
	if (FBitSet(pev->spawnflags, SF_BUTTON_ONLYDIRECT) || m_iDirectUse == PLAYER_USE_POLICY_DIRECT)
		objectCaps |= FCAP_ONLYDIRECT_USE;
	if (m_iDirectUse == PLAYER_USE_POLICY_VISIBLE)
		objectCaps |= FCAP_ONLYVISIBLE_USE;
	return objectCaps;
}

// CBaseButton
TYPEDESCRIPTION CBaseButton::m_SaveData[] =
{
	DEFINE_FIELD( CBaseButton, m_fStayPushed, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseButton, m_fRotating, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseButton, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseButton, m_bLockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseButton, m_bLockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseButton, m_bUnlockedSound, FIELD_CHARACTER ),	
	DEFINE_FIELD( CBaseButton, m_bUnlockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseButton, m_strChangeTarget, FIELD_STRING ),
	//DEFINE_FIELD( CBaseButton, m_ls, FIELD_??? ),   // This is restored in Precache()
	DEFINE_FIELD( CBaseButton, m_targetOnLocked, FIELD_STRING ),
	DEFINE_FIELD( CBaseButton, m_targetOnLockedTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseButton, m_lockedSoundOverride, FIELD_STRING ),
	DEFINE_FIELD( CBaseButton, m_unlockedSoundOverride, FIELD_STRING ),
	DEFINE_FIELD( CBaseButton, m_lockedSentenceOverride, FIELD_STRING ),
	DEFINE_FIELD( CBaseButton, m_unlockedSentenceOverride, FIELD_STRING ),
	DEFINE_FIELD( CBaseButton, m_triggerOnReturn, FIELD_STRING ),
	DEFINE_FIELD( CBaseButton, m_triggerBeforeMove, FIELD_STRING ),
	DEFINE_FIELD( CBaseButton, m_waitBeforeToggleAgain, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseButton, m_toggleAgainTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseButton, m_iDirectUse, FIELD_SHORT ),
	DEFINE_FIELD( CBaseButton, m_fNonMoving, FIELD_BOOLEAN ),
};
	
IMPLEMENT_SAVERESTORE( CBaseButton, CBaseToggle )

void CBaseButton::Precache()
{
	const char *pszSound;

	if( IsSparkingButton() )// this button should spark in OFF state
	{
		SoundScriptParamOverride param;
		param.OverrideVolumeRelative(sparkVolumeRange);
		RegisterAndPrecacheSoundScript(sparkSoundScript, ::sparkBaseSoundScript, param);
	}

	// get door button sounds, for doors which require buttons to open
	if (!FStringNull(m_lockedSoundOverride))
	{
		pszSound = STRING( m_lockedSoundOverride );
		PRECACHE_SOUND( pszSound );
		m_ls.sLockedSound = m_lockedSoundOverride;
	}
	else if( m_bLockedSound )
	{
		pszSound = ButtonSound( (int)m_bLockedSound );
		PRECACHE_SOUND( pszSound );
		m_ls.sLockedSound = MAKE_STRING( pszSound );
	}

	if (!FStringNull(m_unlockedSoundOverride))
	{
		pszSound = STRING( m_unlockedSoundOverride );
		PRECACHE_SOUND( pszSound );
		m_ls.sUnlockedSound = m_unlockedSoundOverride;
	}
	else if( m_bUnlockedSound )
	{
		pszSound = ButtonSound( (int)m_bUnlockedSound );
		PRECACHE_SOUND( pszSound );
		m_ls.sUnlockedSound = MAKE_STRING( pszSound );
	}

	// get sentence group names, for doors which are directly 'touched' to open
	if (!FStringNull(m_lockedSentenceOverride))
	{
		m_ls.sLockedSentence = m_lockedSentenceOverride;
	}
	else
	{
		switch( m_bLockedSentence )
		{
			case 1: // access denied
				m_ls.sLockedSentence = MAKE_STRING( "NA" );
				break;
			case 2: // security lockout
				m_ls.sLockedSentence = MAKE_STRING( "ND" );
				break;
			case 3: // blast door
				m_ls.sLockedSentence = MAKE_STRING( "NF" );
				break;
			case 4: // fire door
				m_ls.sLockedSentence = MAKE_STRING( "NFIRE" );
				break;
			case 5: // chemical door
				m_ls.sLockedSentence = MAKE_STRING( "NCHEM" );
				break;
			case 6: // radiation door
				m_ls.sLockedSentence = MAKE_STRING( "NRAD" );
				break;
			case 7: // gen containment
				m_ls.sLockedSentence = MAKE_STRING( "NCON" );
				break;
			case 8: // maintenance door
				m_ls.sLockedSentence = MAKE_STRING( "NH" );
				break;
			case 9: // broken door
				m_ls.sLockedSentence = MAKE_STRING( "NG" );
				break;
			default:
				m_ls.sLockedSentence = 0;
				break;
		}
	}

	if (!FStringNull(m_unlockedSentenceOverride))
	{
		m_ls.sUnlockedSentence = m_unlockedSentenceOverride;
	}
	else
	{
		switch( m_bUnlockedSentence )
		{
			case 1: // access granted
				m_ls.sUnlockedSentence = MAKE_STRING( "EA" );
				break;
			case 2: // security door
				m_ls.sUnlockedSentence = MAKE_STRING( "ED" );
				break;
			case 3: // blast door
				m_ls.sUnlockedSentence = MAKE_STRING( "EF" );
				break;
			case 4: // fire door
				m_ls.sUnlockedSentence = MAKE_STRING( "EFIRE" );
				break;
			case 5: // chemical door
				m_ls.sUnlockedSentence = MAKE_STRING( "ECHEM" );
				break;
			case 6: // radiation door
				m_ls.sUnlockedSentence = MAKE_STRING( "ERAD" );
				break;
			case 7: // gen containment
				m_ls.sUnlockedSentence = MAKE_STRING( "ECON" );
				break;
			case 8: // maintenance door
				m_ls.sUnlockedSentence = MAKE_STRING( "EH" );
				break;
			default:
				m_ls.sUnlockedSentence = 0;
				break;
		}
	}
}

//
// Cache user-entity-field values until spawn is called.
//
void CBaseButton::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "changetarget" ) )
	{
		m_strChangeTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}	
	else if( FStrEq( pkvd->szKeyName, "locked_sound" ) )
	{
		m_bLockedSound = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sentence" ) )
	{
		m_bLockedSentence = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sound" ) )
	{
		m_bUnlockedSound = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sentence" ) )
	{
		m_bUnlockedSentence = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "sounds" ) )
	{
		m_sounds = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "target_on_locked" ) )
	{
		m_targetOnLocked = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sound_override" ) )
	{
		m_lockedSoundOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sound_override" ) )
	{
		m_unlockedSoundOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sentence_override" ) )
	{
		m_lockedSentenceOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sentence_override" ) )
	{
		m_unlockedSentenceOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_on_return" ) )
	{
		m_triggerOnReturn = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_before_move" ) )
	{
		m_triggerBeforeMove = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "wait_toggle" ) )
	{
		m_waitBeforeToggleAgain = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "directuse"))
	{
		m_iDirectUse = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "usetype" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else 
		CBaseToggle::KeyValue( pkvd );
}

//
// ButtonShot
//
TakeDamageResult CBaseButton::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo )
{
	if (!pev->takedamage)
		return TakeDamageResult();

	DamageInfo damageInfo = TransformDamageInfo(pevInflictor, pevAttacker, inputDamageInfo);
	if (damageInfo.mustSkip)
		return TakeDamageResult();

	BUTTON_CODE code = ButtonResponseToTouch();

	if( code == BUTTON_NOTHING )
		return TakeDamageResult();
	// Temporarily disable the touch function, until movement is finished.
	SetTouch( NULL );

	m_hActivator = CBaseEntity::Instance( pevAttacker );
	if( m_hActivator == 0 )
		return TakeDamageResult();

	if( code == BUTTON_RETURN )
	{
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM );

		// Toggle buttons fire when they get back to their "home" position
		if( !( pev->spawnflags & SF_BUTTON_TOGGLE ) )
			SUB_UseTargets( m_hActivator, UseType(true) );
		ButtonReturn();
	}
	else // code == BUTTON_ACTIVATE
		ButtonActivate();

	return TakeDamageResult();
}

LINK_ENTITY_TO_CLASS( func_button, CBaseButton )

void CBaseButton::Spawn()
{
	if (FBitSet(pev->spawnflags, SF_BUTTON_PLAYER_CANT_USE_OLD))
	{
		if (FStrEq(STRING(gpGlobals->mapname), "ba_teleport2"))
		{
			ALERT(at_console, "Blue Shift map detected. Removing the deprecated spawnflag\n");
			ClearBits(pev->spawnflags, SF_BUTTON_PLAYER_CANT_USE_OLD);
		}
	}

	const char *pszSound;

	//----------------------------------------------------
	//determine sounds for buttons
	//a sound of 0 should not make a sound
	//----------------------------------------------------
	if (!FStringNull(pev->noise))
	{
		pszSound = STRING(pev->noise);
		PRECACHE_SOUND( pszSound );
	}
	else
	{
		pszSound = ButtonSound( m_sounds );
		if (pszSound)
		{
			PRECACHE_SOUND( pszSound );
			pev->noise = MAKE_STRING( pszSound );
		}
	}

	Precache();

	if( IsSparkingButton() )// this button should spark in OFF state
	{
		SetThink( &CBaseButton::ButtonSpark );
		pev->nextthink = gpGlobals->time + 0.5f;// no hurry, make sure everything else spawns
	}

	SetMovedir( pev );

	pev->movetype = MOVETYPE_PUSH;
	pev->solid = SOLID_BSP;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	
	if( pev->speed == 0.0f )
		pev->speed = 40.0f;

	if( pev->health > 0 )
	{
		pev->takedamage = DAMAGE_YES;
	}

	if( m_flWait == 0.0f )
		m_flWait = 1.0f;
	if( m_flLip == 0.0f )
		m_flLip = 4.0f;

	m_toggle_state = TS_AT_BOTTOM;
	m_vecPosition1 = pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2	= m_vecPosition1 + ( pev->movedir * ( fabs( pev->movedir.x * ( pev->size.x - 2.0f ) ) + fabs( pev->movedir.y * ( pev->size.y - 2.0f ) ) + fabs( pev->movedir.z * ( pev->size.z - 2.0f ) ) - m_flLip ) );

	// Is this a non-moving button?
	if( ( ( m_vecPosition2 - m_vecPosition1 ).IsLengthLessThan(1.0f) ) || ( pev->spawnflags & SF_BUTTON_DONTMOVE ) )
		m_vecPosition2 = m_vecPosition1;

	if ( FBitSet( pev->spawnflags, SF_BUTTON_DONTMOVE ) )
		m_fNonMoving = true;

	m_fStayPushed = m_flWait == -1.0f;
	m_fRotating = false;

	// if the button is flagged for USE button activation only, take away it's touch function and add a use function
	if( FBitSet( pev->spawnflags, SF_BUTTON_TOUCH_ONLY ) ) // touchable button
	{
		SetTouch( &CBaseButton::ButtonTouch );
	}
	else 
	{
		SetTouch( NULL );
		if (FBitSet(pev->spawnflags, SF_BUTTON_PLAYER_CANT_USE|SF_BUTTON_PLAYER_CANT_USE_OLD))
			SetUse( &CBaseButton::ButtonUse_IgnorePlayer );
		else
			SetUse( &CBaseButton::ButtonUse );
	}
}

void CBaseButton::Activate()
{
	if (FBitSet(pev->spawnflags, SF_BUTTON_PLAYER_CANT_USE_OLD))
	{
		const Vector center = Center();
		g_errorCollector.AddFormattedDeprecation("%s (center: %g, %g, %g) has the spawnflag %d. This will be removed/replaced in future. Use spawnflag %d instead.",
												 STRING(pev->classname), center.x, center.y, center.z, SF_BUTTON_PLAYER_CANT_USE_OLD, SF_BUTTON_PLAYER_CANT_USE);
	}
	CBaseToggle::Activate();
}

// Button sound table. 
// Also used by CBaseDoor to get 'touched' door lock/unlock sounds

const char *ButtonSound( int sound )
{ 
	const char *pszSound = NULL;

	switch( sound )
	{
		case 0:
			pszSound = "common/null.wav";
			break;
		case 1:
			pszSound = "buttons/button1.wav";
			break;
		case 2:
			pszSound = "buttons/button2.wav";
			break;
		case 3:
			pszSound = "buttons/button3.wav";
			break;
		case 4:
			pszSound = "buttons/button4.wav";
			break;
		case 5:
			pszSound = "buttons/button5.wav";
			break;
		case 6:
			pszSound = "buttons/button6.wav";
			break;
		case 7:
			pszSound = "buttons/button7.wav";
			break;
		case 8:
			pszSound = "buttons/button8.wav";
			break;
		case 9:
			pszSound = "buttons/button9.wav";
			break;
		case 10:
			pszSound = "buttons/button10.wav";
			break;
		case 11:
			pszSound = "buttons/button11.wav";
			break;
		case 12:
			pszSound = "buttons/latchlocked1.wav";
			break;
		case 13:
			pszSound = "buttons/latchunlocked1.wav";
			break;
		case 14:
			pszSound = "buttons/lightswitch2.wav";
			break;

		// next 6 slots reserved for any additional sliding button sounds we may add

		case 21:
			pszSound = "buttons/lever1.wav";
			break;
		case 22:
			pszSound = "buttons/lever2.wav";
			break;
		case 23:
			pszSound = "buttons/lever3.wav";
			break;
		case 24:
			pszSound = "buttons/lever4.wav";
			break;
		case 25:
			pszSound = "buttons/lever5.wav";
			break;
		default:
			pszSound = "buttons/button9.wav";
			break;
	}

	return pszSound;
}

void DoSpark( CBaseEntity *pEntity, const Vector &location, const SoundScriptParamOverride soundParams = SoundScriptParamOverride() )
{
	Vector tmp = location + pEntity->pev->size * 0.5f;
	UTIL_Sparks( tmp );
	pEntity->EmitSoundScript(sparkSoundScript, soundParams);
}

void DoSparkShower( CBaseEntity *pEntity, const Vector &location, const SparkEffectParams& params, const SoundScriptParamOverride soundParams = SoundScriptParamOverride() )
{
	Vector tmp = location + pEntity->pev->size * 0.5f;
	UTIL_SparkShower( tmp, params );
	pEntity->EmitSoundScript(sparkSoundScript, soundParams);
}

void CBaseButton::ButtonSpark()
{
	SetThink( &CBaseButton::ButtonSpark );
	pev->nextthink = pev->ltime + 0.1f + RANDOM_FLOAT( 0.0f, 1.5f );// spark again at random interval

	DoSpark( this, pev->absmin );
}

//
// Button's Use function
//
void CBaseButton::ButtonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	// UNDONE: Should this use ButtonResponseToTouch() too?
	if( m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN )
		return;

	if (FBitSet( pev->spawnflags, SF_BUTTON_TOGGLE ) && m_toggleAgainTime > gpGlobals->time)
		return;

	m_hActivator = pActivator;
	if( m_toggle_state == TS_AT_TOP )
	{
		if( !m_fStayPushed && FBitSet( pev->spawnflags, SF_BUTTON_TOGGLE ) )
		{
			if (PrepareActivation(FBitSet(pev->spawnflags, SF_BUTTON_CHECK_MASTER_ON_TOGGLE_RETURN)))
				ButtonReturn();
		}
	}
	else
		ButtonActivate();
}

void CBaseButton::ButtonUse_IgnorePlayer( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !pCaller || !pCaller->IsPlayer() )
		ButtonUse( pActivator, pCaller, useType, value );
}

CBaseButton::BUTTON_CODE CBaseButton::ButtonResponseToTouch()
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	if( m_toggle_state == TS_GOING_UP ||
		m_toggle_state == TS_GOING_DOWN ||
		( m_toggle_state == TS_AT_TOP && !m_fStayPushed && !FBitSet(pev->spawnflags, SF_BUTTON_TOGGLE ) ) )
		return BUTTON_NOTHING;

	if( m_toggle_state == TS_AT_TOP )
	{
		if( ( FBitSet( pev->spawnflags, SF_BUTTON_TOGGLE ) ) && !m_fStayPushed )
		{
			return BUTTON_RETURN;
		}
	}
	else
		return BUTTON_ACTIVATE;

	return BUTTON_NOTHING;
}

//
// Touching a button simply "activates" it.
//
void CBaseButton::ButtonTouch( CBaseEntity *pOther )
{
	// Ignore touches by anything but players
	if( !pOther->IsPlayer() )
		return;

	m_hActivator = pOther;

	BUTTON_CODE code = ButtonResponseToTouch();

	if( code == BUTTON_NOTHING )
		return;

	if( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
	{
		OnLocked();
		// play button locked sound
		PlayLockSounds( pev, &m_ls, true, true );
		return;
	}

	// Temporarily disable the touch function, until movement is finished.
	SetTouch( NULL );

	if( code == BUTTON_RETURN )
	{
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM );
		SUB_UseTargets( m_hActivator, UseType(true) );
		ButtonReturn();
	}
	else	// code == BUTTON_ACTIVATE
		ButtonActivate();
}

//
// Starts the button moving "in/up".
//
void CBaseButton::ButtonActivate()
{
	if (!PrepareActivation(true))
		return;

	ASSERT( m_toggle_state == TS_AT_BOTTOM );
	m_toggle_state = TS_GOING_UP;
	
	if (!FStringNull(m_triggerBeforeMove))
	{
		FireTargets(STRING(m_triggerBeforeMove), m_hActivator, this );
	}

	if (m_fNonMoving)
	{
		TriggerAndWait();
	}
	else
	{
		SetMoveDone( &CBaseButton::TriggerAndWait );
		if( !m_fRotating )
			LinearMove( m_vecPosition2, pev->speed );
		else
			AngularMove( m_vecAngle2, pev->speed );
	}
}

void CBaseButton::OnLocked()
{
	if (!FStringNull(m_targetOnLocked))
	{
		if (m_targetOnLockedTime < gpGlobals->time)
		{
			FireTargets(STRING(m_targetOnLocked), m_hActivator, this);
			m_targetOnLockedTime = gpGlobals->time + 2.0f;
		}
	}
}

bool CBaseButton::PrepareActivation(bool doActivationCheck)
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1.0f, ATTN_NORM );

	if (doActivationCheck)
	{
		if( !UTIL_IsMasterTriggered( m_sMaster, m_hActivator ) )
		{
			OnLocked();
			// button is locked, play locked sound
			PlayLockSounds( pev, &m_ls, true, true );
			return false;
		}
		else
		{
			// button is unlocked, play unlocked sound
			PlayLockSounds( pev, &m_ls, false, true );
		}
	}
	return true;
}

//
// Button has reached the "in/up" position.  Activate its "targets", and pause before "popping out".
//
void CBaseButton::TriggerAndWait()
{
	ASSERT( m_toggle_state == TS_GOING_UP );

	if( !UTIL_IsMasterTriggered( m_sMaster, m_hActivator ) )
		return;

	m_toggle_state = TS_AT_TOP;

	// If button automatically comes back out, start it moving out.
	// Else re-instate touch method
	if( m_fStayPushed || FBitSet( pev->spawnflags, SF_BUTTON_TOGGLE ) )
	{
		if( !FBitSet( pev->spawnflags, SF_BUTTON_TOUCH_ONLY ) ) // this button only works if USED, not touched!
		{
		// ALL buttons are now use only
		SetTouch( NULL );
		}
		else
			SetTouch( &CBaseButton::ButtonTouch );
	}
	else
	{
		pev->nextthink = pev->ltime + m_flWait;
		SetThink( &CBaseButton::ButtonReturn );
	}

	pev->frame = 1;			// use alternate textures

	SUB_UseTargets( m_hActivator, UseType(false) );
	if (FBitSet( pev->spawnflags, SF_BUTTON_TOGGLE ))
		m_toggleAgainTime = gpGlobals->time + m_waitBeforeToggleAgain;
}

//
// Starts the button moving "out/down".
//
void CBaseButton::ButtonReturn()
{
	ASSERT( m_toggle_state == TS_AT_TOP );
	m_toggle_state = TS_GOING_DOWN;

	if (m_fNonMoving)
	{
		ButtonBackHome();
	}
	else
	{
		SetMoveDone( &CBaseButton::ButtonBackHome );
		if( !m_fRotating )
			LinearMove( m_vecPosition1, pev->speed );
		else
			AngularMove( m_vecAngle1, pev->speed );
	}

	pev->frame = 0;			// use normal textures
}

//
// Button has returned to start state.  Quiesce it.
//
void CBaseButton::ButtonBackHome()
{
	ASSERT( m_toggle_state == TS_GOING_DOWN );
	m_toggle_state = TS_AT_BOTTOM;

	if( FBitSet( pev->spawnflags, SF_BUTTON_TOGGLE ) )
	{
		//EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM );
		
		SUB_UseTargets( m_hActivator, UseType(true) );
		m_toggleAgainTime = gpGlobals->time + m_waitBeforeToggleAgain;
	}

	if( !FStringNull( pev->target ) )
	{
		edict_t *pentTarget = NULL;
		for( ;; )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->target ) );

			if( FNullEnt( pentTarget ) )
				break;

			if( !FClassnameIs( pentTarget, "multisource" ) )
				continue;
			CBaseEntity *pTarget = CBaseEntity::Instance( pentTarget );

			if( pTarget )
				pTarget->Use( m_hActivator, this, USE_TOGGLE, 0 );
		}
	}

	// Re-instate touch method, movement cycle is complete.
	if( !FBitSet( pev->spawnflags, SF_BUTTON_TOUCH_ONLY ) ) // this button only works if USED, not touched!
	{
		// All buttons are now use only	
		SetTouch( NULL );
	}
	else
		SetTouch( &CBaseButton::ButtonTouch );

	// reset think for a sparking button
	if( IsSparkingButton() )
	{
		SetThink( &CBaseButton::ButtonSpark );
		pev->nextthink = gpGlobals->time + 0.5f;// no hurry.
	}

	if (!FStringNull(m_triggerOnReturn))
		FireTargets(STRING(m_triggerOnReturn), m_hActivator, this);
}

bool CBaseButton::IsSparkingButton()
{
	return FBitSet( pev->spawnflags, SF_BUTTON_SPARK_IF_OFF )
			&& !FClassnameIs(pev, "func_rot_button"); // there's a clash in flags, don't enable sparking for rotating buttons
}

USE_TYPE CBaseButton::UseType(bool returning)
{
	if (pev->impulse == BUTTON_USE_ON)
	{
		return USE_ON;
	}
	else if (pev->impulse == BUTTON_USE_OFF)
	{
		return USE_OFF;
	}
	if ( FBitSet(pev->spawnflags, SF_BUTTON_TOGGLE) )
	{
		if (pev->impulse == BUTTON_USE_ON_OFF)
		{
			if (returning)
				return USE_OFF;
			else
				return USE_ON;
		}
		else if (pev->impulse == BUTTON_USE_OFF_ON)
		{
			if (returning)
				return USE_ON;
			else
				return USE_OFF;
		}
	}
	return USE_TOGGLE;
}

//
// Rotating button (aka "lever")
//
class CRotButton : public CBaseButton
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS( func_rot_button, CRotButton )

void CRotButton::Spawn()
{
	const char *pszSound;
	//----------------------------------------------------
	//determine sounds for buttons
	//a sound of 0 should not make a sound
	//----------------------------------------------------
	if (!FStringNull(pev->noise))
	{
		pszSound = STRING(pev->noise);
		PRECACHE_SOUND( pszSound );
	}
	else
	{
		pszSound = ButtonSound( m_sounds );
		if (pszSound)
		{
			PRECACHE_SOUND( pszSound );
			pev->noise = MAKE_STRING( pszSound );
		}
	}

	Precache();

	// set the axis of rotation
	CBaseToggle::AxisDir( pev );

	// check for clockwise rotation
	if( FBitSet( pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS ) )
		pev->movedir = pev->movedir * -1.0f;

	pev->movetype = MOVETYPE_PUSH;
	
	if( pev->spawnflags & SF_ROTBUTTON_NOTSOLID )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	
	if( pev->speed == 0.0f )
		pev->speed = 40.0f;

	if( m_flWait == 0 )
		m_flWait = 1;

	if( pev->health > 0 )
	{
		pev->takedamage = DAMAGE_YES;
	}

	m_toggle_state = TS_AT_BOTTOM;
	m_vecAngle1 = pev->angles;
	m_vecAngle2 = pev->angles + pev->movedir * m_flMoveDistance;
	ASSERTSZ( m_vecAngle1 != m_vecAngle2, "rotating button start/end positions are equal" );

	m_fStayPushed = m_flWait == -1.0f;
	m_fRotating = true;

	// if the button is flagged for USE button activation only, take away it's touch function and add a use function
	if( !FBitSet( pev->spawnflags, SF_BUTTON_TOUCH_ONLY ) )
	{
		SetTouch( NULL );
		if (FBitSet(pev->spawnflags, SF_BUTTON_PLAYER_CANT_USE|SF_BUTTON_PLAYER_CANT_USE_OLD))
			SetUse( &CBaseButton::ButtonUse_IgnorePlayer );
		else
			SetUse( &CBaseButton::ButtonUse );
	}
	else // touchable button
		SetTouch( &CBaseButton::ButtonTouch );

	//SetTouch( &ButtonTouch );
}

// Make this button behave like a door (HACKHACK)
// This will disable use and make the button solid
// rotating buttons were made SOLID_NOT by default since their were some
// collision problems with them...
#define SF_MOMENTARY_DOOR		0x0001
#define SF_MOMENTARY_AUTO_RETURN		16

class CMomentaryRotButton : public CBaseToggle
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData *pkvd ) override;
	int ObjectCaps()  override
	{ 
		int flags = CBaseToggle::ObjectCaps() & ( ~FCAP_ACROSS_TRANSITION );
		if( pev->spawnflags & SF_MOMENTARY_DOOR )
			return flags;
		return flags | FCAP_CONTINUOUS_USE;
	}
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void EXPORT Off();
	void EXPORT Return();
	void UpdateSelf( float value );
	void UpdateSelfReturn( float value );
	void UpdateAllButtons( float value, int start );

	void PlaySound();
	void UpdateTarget( float value );

	static CMomentaryRotButton *Instance( edict_t *pent ) { return (CMomentaryRotButton *)GET_PRIVATE( pent ); }
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_lastUsed;
	int m_direction;
	float m_returnSpeed;
	Vector m_start;
	Vector m_end;
	int m_sounds;
};

TYPEDESCRIPTION CMomentaryRotButton::m_SaveData[] =
{
	DEFINE_FIELD( CMomentaryRotButton, m_lastUsed, FIELD_INTEGER ),
	DEFINE_FIELD( CMomentaryRotButton, m_direction, FIELD_INTEGER ),
	DEFINE_FIELD( CMomentaryRotButton, m_returnSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CMomentaryRotButton, m_start, FIELD_VECTOR ),
	DEFINE_FIELD( CMomentaryRotButton, m_end, FIELD_VECTOR ),
	DEFINE_FIELD( CMomentaryRotButton, m_sounds, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMomentaryRotButton, CBaseToggle )

LINK_ENTITY_TO_CLASS( momentary_rot_button, CMomentaryRotButton )

void CMomentaryRotButton::Spawn()
{
	CBaseToggle::AxisDir( pev );

	if( pev->speed == 0.0f )
		pev->speed = 100.0f;

	if( m_flMoveDistance < 0.0f ) 
	{
		m_start = pev->angles + pev->movedir * m_flMoveDistance;
		m_end = pev->angles;
		m_direction = 1;		// This will toggle to -1 on the first use()
		m_flMoveDistance = -m_flMoveDistance;
	}
	else
	{
		m_start = pev->angles;
		m_end = pev->angles + pev->movedir * m_flMoveDistance;
		m_direction = -1;		// This will toggle to +1 on the first use()
	}

	if( pev->spawnflags & SF_MOMENTARY_DOOR )
		pev->solid = SOLID_BSP;
	else
		pev->solid = SOLID_NOT;

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	const char *pszSound = ButtonSound( m_sounds );
	PRECACHE_SOUND( pszSound );
	pev->noise = MAKE_STRING( pszSound );
	m_lastUsed = 0;
}

void CMomentaryRotButton::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "returnspeed" ) )
	{
		m_returnSpeed = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "sounds" ) )
	{
		m_sounds = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CMomentaryRotButton::PlaySound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM );
}

// BUGBUG: This design causes a latentcy.  When the button is retriggered, the first impulse
// will send the target in the wrong direction because the parameter is calculated based on the
// current, not future position.
void CMomentaryRotButton::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsLockedByMaster())
		return;

	pev->ideal_yaw = CBaseToggle::AxisDelta( pev->spawnflags, pev->angles, m_start ) / m_flMoveDistance;

	UpdateAllButtons( pev->ideal_yaw, 1 );

	// Calculate destination angle and use it to predict value, this prevents sending target in wrong direction on retriggering
	Vector dest = pev->angles + pev->avelocity * ( pev->nextthink - pev->ltime );
	float value1 = CBaseToggle::AxisDelta( pev->spawnflags, dest, m_start ) / m_flMoveDistance;
	UpdateTarget( value1 );
}

void CMomentaryRotButton::UpdateAllButtons( float value, int start )
{
	// Update all rot buttons attached to the same target
	edict_t *pentTarget = NULL;
	for( ; ; )
	{
		pentTarget = FIND_ENTITY_BY_STRING( pentTarget, "target", STRING( pev->target ) );
		if( FNullEnt( pentTarget ) )
			break;

		if( FClassnameIs( VARS( pentTarget ), "momentary_rot_button" ) )
		{
			CMomentaryRotButton *pEntity = CMomentaryRotButton::Instance( pentTarget );
			if( pEntity )
			{
				if( start )
					pEntity->UpdateSelf( value );
				else
					pEntity->UpdateSelfReturn( value );
			}
		}
	}
}

void CMomentaryRotButton::UpdateSelf( float value )
{
	bool fplaysound = false;

	if( !m_lastUsed )
	{
		fplaysound = true;
		m_direction = -m_direction;
	}
	m_lastUsed = 1;

	pev->nextthink = pev->ltime + 0.1f;
	if( m_direction > 0 && value >= 1.0f )
	{
		pev->avelocity = g_vecZero;
		pev->angles = m_end;
		return;
	}
	else if( m_direction < 0 && value <= 0.0f )
	{
		pev->avelocity = g_vecZero;
		pev->angles = m_start;
		return;
	}

	if( fplaysound )
		PlaySound();

	// HACKHACK -- If we're going slow, we'll get multiple player packets per frame, bump nexthink on each one to avoid stalling
	if( pev->nextthink < pev->ltime )
		pev->nextthink = pev->ltime + 0.1f;
	else
		pev->nextthink += 0.1f;
	
	pev->avelocity = m_direction * pev->speed * pev->movedir;
	SetThink( &CMomentaryRotButton::Off );
}

void CMomentaryRotButton::UpdateTarget( float value )
{
	if( !FStringNull( pev->target ) )
	{
		edict_t *pentTarget= NULL;
		for( ; ; )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->target ) );
			if( FNullEnt( pentTarget ) )
				break;
			CBaseEntity *pEntity = CBaseEntity::Instance( pentTarget );
			if( pEntity )
			{
				pEntity->Use( this, this, USE_SET, value );
			}
		}
	}
}

void CMomentaryRotButton::Off()
{
	pev->avelocity = g_vecZero;
	m_lastUsed = 0;
	if( FBitSet( pev->spawnflags, SF_MOMENTARY_AUTO_RETURN ) && m_returnSpeed > 0 )
	{
		SetThink( &CMomentaryRotButton::Return );
		pev->nextthink = pev->ltime + 0.1f;
		m_direction = -1;
	}
	else
		SetThink( NULL );
}

void CMomentaryRotButton::Return()
{
	float value = CBaseToggle::AxisDelta( pev->spawnflags, pev->angles, m_start ) / m_flMoveDistance;

	UpdateAllButtons( value, 0 );	// This will end up calling UpdateSelfReturn() n times, but it still works right
	if( value > 0.0f )
		UpdateTarget( value );
}

void CMomentaryRotButton::UpdateSelfReturn( float value )
{
	if( value <= 0.0f )
	{
		pev->avelocity = g_vecZero;
		pev->angles = m_start;
		pev->nextthink = -1.0f;
		SetThink( NULL );
	}
	else
	{
		pev->avelocity = -m_returnSpeed * pev->movedir;
		pev->nextthink = pev->ltime + 0.1f;
	}
}

//----------------------------------------------------------------
// Spark
//----------------------------------------------------------------

#define SF_SPARK_CYCLIC 16
#define SF_SPARK_TOGGLE 32
#define SF_SPARK_START_ON 64
#define SF_SPARK_NO_STREAK 128
#define SF_SPARK_NO_SOUND 4096

class CEnvSpark : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int ObjectCaps() override
	{
		int caps = CBaseEntity::ObjectCaps();
		if (g_modFeatures.env_spark_transit)
			return caps;
		return caps & ~FCAP_ACROSS_TRANSITION;
	}
	void EXPORT SparkThink();
	void EXPORT SparkWait();
	void EXPORT SparkCyclic(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT SparkStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT SparkStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd ) override;
	void MakeSpark();
	void SetSilent(bool silent)
	{
		if (silent)
			SetBits(pev->spawnflags, SF_SPARK_NO_SOUND);
		else
			ClearBits(pev->spawnflags, SF_SPARK_NO_SOUND);
	}
	bool IsSilent() const
	{
		return FBitSet(pev->spawnflags, SF_SPARK_NO_SOUND);
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flDelay;
	short m_soundRadius;
	int m_streakCount;
	int m_streakVelocity;
	float m_sparkDuration;
	float m_sparkScaleMin;
	float m_sparkScaleMax;
	float m_volume;
	bool m_silent; // don't save, goes to spawnflags

	int m_modelIndex;
};

TYPEDESCRIPTION CEnvSpark::m_SaveData[] =
{
	DEFINE_FIELD( CEnvSpark, m_flDelay, FIELD_FLOAT),
	DEFINE_FIELD( CEnvSpark, m_soundRadius, FIELD_SHORT),
	DEFINE_FIELD( CEnvSpark, m_streakCount, FIELD_INTEGER),
	DEFINE_FIELD( CEnvSpark, m_streakVelocity, FIELD_INTEGER),
	DEFINE_FIELD( CEnvSpark, m_sparkDuration, FIELD_FLOAT),
	DEFINE_FIELD( CEnvSpark, m_sparkScaleMin, FIELD_FLOAT),
	DEFINE_FIELD( CEnvSpark, m_sparkScaleMax, FIELD_FLOAT),
	DEFINE_FIELD( CEnvSpark, m_volume, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE( CEnvSpark, CBaseEntity )

LINK_ENTITY_TO_CLASS( env_spark, CEnvSpark )
LINK_ENTITY_TO_CLASS( env_debris, CEnvSpark )

void CEnvSpark::Spawn()
{
	SetThink( NULL );
	SetUse( NULL );

	if (FBitSet(pev->spawnflags, SF_SPARK_CYCLIC))
	{
		SetUse(&CEnvSpark::SparkCyclic);
	}
	else if( FBitSet( pev->spawnflags, SF_SPARK_TOGGLE ) ) // Use for on/off
	{
		if( FBitSet( pev->spawnflags, SF_SPARK_START_ON ) ) // Start on
		{
			SetThink( &CEnvSpark::SparkThink );	// start sparking
			SetUse( &CEnvSpark::SparkStop );		// set up +USE to stop sparking
		}
		else
			SetUse( &CEnvSpark::SparkStart );
	}
	else
		SetThink( &CEnvSpark::SparkThink );

	if( !FBitSet(pev->spawnflags, SF_SPARK_CYCLIC) )
	{
		pev->nextthink = gpGlobals->time + 0.1 + RANDOM_FLOAT( 0.0f, 1.5f );

		if( m_flDelay <= 0 )
			m_flDelay = 1.5f;
	}

	SetSilent(m_silent);

	Precache();
}

void CEnvSpark::Precache()
{
	SoundScriptParamOverride param;
	param.OverrideVolumeRelative(sparkVolumeRange);
	RegisterAndPrecacheSoundScript(sparkSoundScript, ::sparkBaseSoundScript, param);
	if (!FStringNull(pev->model))
	{
		m_modelIndex = PRECACHE_MODEL(STRING(pev->model));
	}
}

void CEnvSpark::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "MaxDelay" ) )
	{
		m_flDelay = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "soundradius" ) )
	{
		m_soundRadius = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "streak_count" ) )
	{
		m_streakCount = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "streak_velocity" ) )
	{
		m_streakVelocity = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spark_duration" ) )
	{
		m_sparkDuration = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spark_scale_min" ) )
	{
		m_sparkScaleMin = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spark_scale_max" ) )
	{
		m_sparkScaleMax = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "spark_volume" ) )
	{
		m_volume = atof( pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "spark_silent" ) )
	{
		m_silent = atoi( pkvd->szValue) != 0;
		SetSilent(m_silent);
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "style" ) ||
				FStrEq( pkvd->szKeyName, "height" ) ||
				FStrEq( pkvd->szKeyName, "killtarget" ) ||
				FStrEq( pkvd->szKeyName, "value1" ) ||
				FStrEq( pkvd->szKeyName, "value2" ) ||
				FStrEq( pkvd->szKeyName, "value3" ) )
		pkvd->fHandled = true;
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvSpark::SparkCyclic(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_pfnThink == NULL)
	{
		MakeSpark();
		SetThink(&CEnvSpark::SparkWait );
		pev->nextthink = gpGlobals->time + m_flDelay;
	}
	else
	{
		SetThink(&CEnvSpark::SparkThink ); // if we're on SparkWait, change to actually spark at the specified time.
	}
}

void CEnvSpark::SparkWait()
{
	SetThink( NULL );
}

void CEnvSpark::SparkThink()
{
	MakeSpark();
	if (pev->spawnflags & SF_SPARK_CYCLIC)
	{
		SetThink( NULL );
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1f + RANDOM_FLOAT( 0.0f, m_flDelay );
	}
}

void CEnvSpark::SparkStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( &CEnvSpark::SparkStop );
	SetThink( &CEnvSpark::SparkThink );
	pev->nextthink = gpGlobals->time + 0.1f + RANDOM_FLOAT( 0.0f, m_flDelay );
}

void CEnvSpark::SparkStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( &CEnvSpark::SparkStart);
	SetThink( NULL );
}

void CEnvSpark::MakeSpark()
{
	SparkEffectParams params;
	params.sparkModelIndex = m_modelIndex;
	params.streakCount = m_streakCount;
	params.streakVelocity = m_streakVelocity;
	params.sparkDuration = m_sparkDuration;
	params.sparkScaleMin = m_sparkScaleMin;
	params.sparkScaleMax = m_sparkScaleMax;
	if (FBitSet(pev->spawnflags, SF_SPARK_NO_STREAK))
	{
		params.flags |= SPARK_EFFECT_NO_STREAK;
	}
	SoundScriptParamOverride soundParams;
	if (IsSilent())
	{
		soundParams.OverrideVolumeAbsolute(0.0f);
	}
	else
	{
		if (m_volume > 0.0f)
		{
			soundParams.OverrideVolumeRelative(m_volume);
		}
	}
	if (m_soundRadius)
	{
		soundParams.OverrideAttenuationAbsolute(::SoundAttenuation(m_soundRadius));
	}
	DoSparkShower(this, pev->origin, params, soundParams);
}

#define SF_BTARGET_USE		0x0001
#define SF_BTARGET_ON		0x0002

class CButtonTarget : public CBaseEntity
{
public:
	void Spawn() override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	TakeDamageResult TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo ) override;
	int ObjectCaps() override;
};

LINK_ENTITY_TO_CLASS( button_target, CButtonTarget )

void CButtonTarget::Spawn()
{
	pev->movetype = MOVETYPE_PUSH;
	pev->solid = SOLID_BSP;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	pev->takedamage = DAMAGE_YES;

	if( FBitSet( pev->spawnflags, SF_BTARGET_ON ) )
		pev->frame = 1;
}

void CButtonTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, (int)pev->frame != 0 ) )
		return;
	pev->frame = 1 - pev->frame;
	if( pev->frame )
		SUB_UseTargets( pActivator, USE_ON, 0 );
	else
		SUB_UseTargets( pActivator, USE_OFF, 0 );
}

int CButtonTarget::ObjectCaps()
{
	int caps = CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;

	if( FBitSet( pev->spawnflags, SF_BTARGET_USE ) )
		return caps | FCAP_IMPULSE_USE;
	else
		return caps;
}

TakeDamageResult CButtonTarget::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo )
{
	DamageInfo damageInfo = TransformDamageInfo(pevInflictor, pevAttacker, inputDamageInfo);
	if (damageInfo.mustSkip)
		return TakeDamageResult();

	Use( Instance( pevAttacker ), this, USE_TOGGLE, 0 );

	return TakeDamageResult();
}
