/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
/*


===== scripted.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "schedule.h"
#include "scripted.h"
#include "talkmonster.h"
#include "player.h"
#include "gamerules.h"
#include "locus.h"

enum
{
	REQUIRED_STATE_ANY = 0,
	REQUIRED_STATE_IDLE,
	REQUIRED_STATE_COMBAT,
	REQUIRED_STATE_ALERT,
	REQUIRED_STATE_IDLE_OR_ALERT,
};

static bool MatchingMonsterState(MONSTERSTATE state, int requiredState)
{
	switch (requiredState) {
	case REQUIRED_STATE_ANY:
		return true;
	case REQUIRED_STATE_IDLE:
		return state == MONSTERSTATE_IDLE;
	case REQUIRED_STATE_COMBAT:
		return state == MONSTERSTATE_COMBAT;
	case REQUIRED_STATE_ALERT:
		return state == MONSTERSTATE_ALERT || state == MONSTERSTATE_HUNT;
	case REQUIRED_STATE_IDLE_OR_ALERT:
		return state == MONSTERSTATE_IDLE || state == MONSTERSTATE_ALERT || state == MONSTERSTATE_HUNT;
	default:
		return false;
	}
}

enum
{
	CHECKFAIL_NOFAIL = 0,
	CHECKFAIL_TARGET_IS_NULL,
	CHECKFAIL_TARGET_IS_BUSY,
	CHECKFAIL_UNMATCHING_MONSTERSTATE,
	CHECKFAIL_UNMATCHING_FOLLOWERSTATE,
	CHECKFAIL_TARGET_IS_TOOFAR,
};

static const char* ScriptCheckFailMessage(int checkFail)
{
	switch (checkFail) {
	case CHECKFAIL_NOFAIL:
		return "target is ok!";
	case CHECKFAIL_TARGET_IS_NULL:
		return "target is null (not a monster?)";
	case CHECKFAIL_TARGET_IS_BUSY:
		return "target is busy";
	case CHECKFAIL_UNMATCHING_MONSTERSTATE:
		return "target has unmatching monster state";
	case CHECKFAIL_UNMATCHING_FOLLOWERSTATE:
		return "target has unmatching follower state";
	case CHECKFAIL_TARGET_IS_TOOFAR:
		return "target is too far";
	default:
		return "unknown";
	}
}

/*
classname "scripted_sequence"
targetname "me" - there can be more than one with the same name, and they act in concert
target "the_entity_I_want_to_start_playing" or "class entity_classname" will pick the closest inactive scientist
play "name_of_sequence"
idle "name of idle sequence to play before starting"
donetrigger "whatever" - can be any other triggerable entity such as another sequence, train, door, or a special case like "die" or "remove"
moveto - if set the monster first moves to this nodes position
range # - only search this far to find the target
spawnflags - (stop if blocked, stop if player seen)
*/

//
// Cache user-entity-field values until spawn is called.
//
void CCineMonster::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_iszIdle" ) )
	{
		m_iszIdle = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iszPlay" ) )
	{
		m_iszPlay = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iszEntity" ) )
	{
		m_iszEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszAttack"))
	{
		m_iszAttack = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszMoveTarget"))
	{
		m_iszMoveTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszFireOnBegin"))
	{
		m_iszFireOnBegin = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_fMoveTo" ) )
	{
		m_fMoveTo = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	/*else if( FStrEq( pkvd->szKeyName, "m_flRepeat" ) )
	{
		m_flRepeat = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}*/
	else if( FStrEq( pkvd->szKeyName, "m_flRadius" ) )
	{
		m_flRadius = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iFinishSchedule" ) )
	{
		m_iFinishSchedule = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPriority"))
	{
		m_iPriority = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iszFireOnAnimStart" ) )
	{
		m_iszFireOnAnimStart = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iszFireOnPossessed" ) )
	{
		m_iszFireOnPossessed = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "m_targetActivator" ) )
	{
		m_targetActivator = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "moveto_radius" ) )
	{
		m_flMoveToRadius = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_fTurnType" ) )
	{
		m_fTurnType = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fAction"))
	{
		m_fAction = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_requiredFollowerState" ) )
	{
		m_requiredFollowerState = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_applySearchRadius" ) )
	{
		m_applySearchRadius = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_maxMoveFailAttempts" ) )
	{
		m_maxMoveFailAttempts = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iRepeats"))
	{
		m_iRepeats = atoi( pkvd->szValue );
		m_iRepeatsLeft = m_iRepeats;
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fRepeatFrame"))
	{
		m_fRepeatFrame = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_interruptionPolicy" ) )
	{
		m_interruptionPolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_searchPolicy" ) )
	{
		m_searchPolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_takeDamagePolicy" ) )
	{
		m_takeDamagePolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "required_state" ) )
	{
		m_requiredState = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "master") )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "initial_search_delay"))
	{
		m_initialSearchDelay = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "search_delay"))
	{
		m_searchDelay = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_playFramerate" ) )
	{
		m_playFramerate = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

TYPEDESCRIPTION	CCineMonster::m_SaveData[] =
{
	DEFINE_FIELD( CCineMonster, m_hTargetEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( CCineMonster, m_iszIdle, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_iszPlay, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_iszEntity, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_fMoveTo, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_iszAttack, FIELD_STRING ), //LRC
	DEFINE_FIELD( CCineMonster, m_iszMoveTarget, FIELD_STRING ), //LRC
	DEFINE_FIELD( CCineMonster, m_iszFireOnBegin, FIELD_STRING ),
	//DEFINE_FIELD( CCineMonster, m_flRepeat, FIELD_FLOAT ),
	DEFINE_FIELD( CCineMonster, m_flRadius, FIELD_FLOAT ),

	DEFINE_FIELD( CCineMonster, m_iDelay, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_startTime, FIELD_TIME ),

	DEFINE_FIELD( CCineMonster, m_saved_movetype, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_saved_solid, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_saved_effects, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_iFinishSchedule, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_interruptable, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCineMonster, m_firedOnAnimStart, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCineMonster, m_iszFireOnAnimStart, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_iszFireOnPossessed, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_targetActivator, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_fTurnType, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_fAction, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_flMoveToRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CCineMonster, m_requiredFollowerState, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_applySearchRadius, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_maxMoveFailAttempts, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_moveFailCount, FIELD_SHORT ),

	DEFINE_FIELD( CCineMonster, m_iRepeats, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_iRepeatsLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_fRepeatFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CCineMonster, m_iPriority, FIELD_INTEGER ),

	DEFINE_FIELD( CCineMonster, m_interruptionPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_searchPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_requiredState, FIELD_SHORT ),
	DEFINE_FIELD( CCineMonster, m_takeDamagePolicy, FIELD_SHORT ),

	DEFINE_FIELD( CCineMonster, m_sMaster, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_initialSearchDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CCineMonster, m_searchDelay, FIELD_FLOAT ),

	DEFINE_FIELD( CCineMonster, m_playFramerate, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CCineMonster, CBaseDelay )

LINK_ENTITY_TO_CLASS( scripted_sequence, CCineMonster )
LINK_ENTITY_TO_CLASS( scripted_action, CCineMonster ) //LRC

LINK_ENTITY_TO_CLASS( aiscripted_sequence, CCineAI )

void CCineMonster::Spawn()
{
	// pev->solid = SOLID_TRIGGER;
	// UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );
	pev->solid = SOLID_NOT;

	const bool isAutoSearch = IsAutoSearch();
	const bool hasIdleAnim = !FStringNull(m_iszIdle);

	// if no targetname, start now
	if (isAutoSearch || hasIdleAnim)
	{
		SetThink( &CCineMonster::CineThink );
		const float delay = m_initialSearchDelay > 0.0f ? m_initialSearchDelay : 1.0f;
		pev->nextthink = gpGlobals->time + delay;
		// Wait to be used?
		if (!isAutoSearch || (hasIdleAnim && FBitSet(pev->spawnflags, SF_SCRIPT_ALLOW_UNNAMED_IDLE)))
			m_startTime = gpGlobals->time + (float)1E6;
	}
	if( ForcedNoInterruptions() )
		m_interruptable = false;
	else
		m_interruptable = true;
}

//=========================================================
// FCanOverrideState - returns false, scripted sequences
// cannot possess entities regardless of state.
//=========================================================
bool CCineMonster::FCanOverrideState()
{
	if( pev->spawnflags & SF_SCRIPT_OVERRIDESTATE )
		return true;
	return false;
}

//=========================================================
// FCanOverrideState - returns true because scripted AI can
// possess entities regardless of their state.
//=========================================================
bool CCineAI::FCanOverrideState()
{
	return true;
}

bool CCineMonster::ShouldResetOnGroundFlag()
{
	return false;
}

bool CCineAI::ShouldResetOnGroundFlag()
{
	return true;
}

//
// CineStart
//
void CCineMonster::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// do I already know who I should use
	CBaseEntity *pEntity = m_hTargetEnt;
	CBaseMonster *pTarget = NULL;

	if( pEntity )
		pTarget = pEntity->MyMonsterPointer();

	if( pTarget )
	{
		// am I already playing the script?
		if( pTarget->m_scriptState == CBaseMonster::SCRIPT_PLAYING )
			return;

		m_startTime = gpGlobals->time + 0.05f;
	}
	else
	{
		// if not, try finding them
		m_cantFindReported = false;
		m_cantPlayReported = false;
		m_hActivator = pActivator;

		if (FBitSet(pev->spawnflags, SF_SCRIPT_TRY_ONCE))
		{
			TryFindAndPossessEntity();
		}
		else
		{
			SetThink( &CCineMonster::CineThink );
			pev->nextthink = gpGlobals->time;
		}
	}
}

// This doesn't really make sense since only MOVETYPE_PUSH get 'Blocked' events
void CCineMonster::Blocked( CBaseEntity *pOther )
{

}

void CCineMonster::Touch( CBaseEntity *pOther )
{
/*
	ALERT( at_aiconsole, "Cine Touch\n" );
	if( m_pentTarget && OFFSET( pOther->pev ) == OFFSET( m_pentTarget ) )
	{
		CBaseMonster *pTarget = GetClassPtr( (CBaseMonster *)VARS( m_pentTarget ) );
		pTarget->m_monsterState == MONSTERSTATE_SCRIPT;
	}
*/
}

/*
	entvars_t *pevOther = VARS( gpGlobals->other );

	if( !FBitSet( pevOther->flags, FL_MONSTER ) )
	{
		// touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;

	if( FBitSet( pevOther->flags, FL_ONGROUND ) )
	{
		// clear the onground so physics don't bitch
		pevOther->flags -= FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;

	pev->solid = SOLID_NOT;// kill the trigger for now !!!UNDONE
}
*/


//
// ********** Cinematic DIE **********
//
void CCineMonster::Die()
{
	SetThink( &CBaseEntity::SUB_Remove );
}

//
// ********** Cinematic PAIN **********
//
void CCineMonster::Pain()
{

}

//
// ********** Cinematic Think **********
//

// find a viable entity
CBaseMonster *CCineMonster::FindEntity()
{
	CBaseMonster *pTarget = NULL;
	int checkFail;

	if (UTIL_TargetnameIsActivator(m_iszEntity))
	{
		if (m_hActivator != 0 && FBitSet(m_hActivator->pev->flags, FL_MONSTER) && (pTarget = m_hActivator->MyMonsterPointer()) != 0 )
		{
			if (IsAppropriateTarget(pTarget, m_iPriority | SS_INTERRUPT_ALERT, m_applySearchRadius == SCRIPT_APPLY_SEARCH_RADIUS_ALWAYS, &checkFail))
				return pTarget;
			MayReportInappropriateTarget(checkFail);
		}
		return NULL;
	}

	pTarget = NULL;
	bool failedCheckReported = false;

	if ( m_searchPolicy != SCRIPT_SEARCH_POLICY_CLASSNAME_ONLY )
	{
		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszEntity ) );
		while( !FNullEnt( pentTarget ) )
		{
			if( FBitSet( VARS( pentTarget )->flags, FL_MONSTER ) )
			{
				pTarget = GetMonsterPointer( pentTarget );

				if (IsAppropriateTarget(pTarget, m_iPriority | SS_INTERRUPT_ALERT, m_applySearchRadius == SCRIPT_APPLY_SEARCH_RADIUS_ALWAYS, &checkFail))
					return pTarget;
				failedCheckReported = failedCheckReported || MayReportInappropriateTarget(checkFail);
			}
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( m_iszEntity ) );
			pTarget = NULL;
		}
	}

	if( !pTarget )
	{
		if ( m_searchPolicy != SCRIPT_SEARCH_POLICY_TARGETNAME_ONLY )
		{
			CBaseEntity *pEntity = NULL;
			while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, m_flRadius ) ) != NULL )
			{
				if( FClassnameIs( pEntity->pev, STRING( m_iszEntity ) ) )
				{
					if( FBitSet( pEntity->pev->flags, FL_MONSTER ) )
					{
						pTarget = pEntity->MyMonsterPointer();
						if (IsAppropriateTarget(pTarget, m_iPriority, false, &checkFail))
							return pTarget;
						failedCheckReported = failedCheckReported || MayReportInappropriateTarget(checkFail);
					}
				}
			}
		}
	}

	if (!m_cantFindReported && !failedCheckReported && !IsAutoSearch())
	{
		ALERT( at_aiconsole, "script \"%s\" can't find monster \"%s\" (nonexistent or out of range)\n", STRING( pev->targetname ), STRING( m_iszEntity ) );
		m_cantFindReported = true;
	}
	return NULL;
}

bool CCineMonster::AcceptedFollowingState(CBaseMonster *pMonster)
{
	if (m_requiredFollowerState == SCRIPT_REQUIRED_FOLLOWER_STATE_UNSPECIFIED)
		return true;
	CFollowingMonster* pFollowingMonster = pMonster->MyFollowingMonsterPointer();
	if (pFollowingMonster)
	{
		if (m_requiredFollowerState == SCRIPT_REQUIRED_FOLLOWER_STATE_FOLLOWING) {
			return pFollowingMonster->IsFollowingPlayer();
		}
		if (m_requiredFollowerState == SCRIPT_REQUIRED_FOLLOWER_STATE_NOT_FOLLOWING) {
			return !pFollowingMonster->IsFollowingPlayer();
		}
	}
	return true;
}

// make the entity enter a scripted sequence
void CCineMonster::PossessEntity()
{
	CBaseEntity *pEntity = m_hTargetEnt;
	CBaseMonster *pTarget = NULL;
	if( pEntity )
		pTarget = pEntity->MyMonsterPointer();

	if( pTarget )
	{
		if (pTarget->m_pCine)
		{
			pTarget->m_pCine->CancelScript();
		}

		pTarget->SetScriptedMoveGoal(this);
		pTarget->m_pCine = this;
		pTarget->m_hTargetEnt = this;

		if (m_iszAttack)
		{
			// anything with that name?
			CBaseEntity* pTurnTargetEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszAttack), m_hActivator);
			if( pTurnTargetEnt == 0 )
			{	// nothing. Anything with that classname?
				CBaseEntity* pFoundEnt = NULL;
				while ((pFoundEnt = UTIL_FindEntityInSphere( pFoundEnt, pev->origin, m_flRadius )) != NULL)
				{
					if (pFoundEnt != pTarget && FClassnameIs( pFoundEnt->pev, STRING(m_iszAttack)))
					{
						pTurnTargetEnt = pFoundEnt;
						break;
					}
				}
			}
			if (pTurnTargetEnt == 0)
			{	// nothing. Oh well.
				ALERT(at_console,"%s %s has a missing \"turn target\": %s\n",STRING(pev->classname),STRING(pev->targetname),STRING(m_iszAttack));
			}
			else
			{
				pTarget->m_hTargetEnt = pTurnTargetEnt;
			}
		}

		if (m_iszMoveTarget)
		{
			// anything with that name?
			CBaseEntity* pGoalEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszMoveTarget), m_hActivator);
			if( pGoalEnt == 0 )
			{	// nothing. Oh well.
				ALERT(at_console,"%s %s has a missing \"move target\": %s\n",STRING(pev->classname),STRING(pev->targetname),STRING(m_iszMoveTarget));
			} else {
				pTarget->SetScriptedMoveGoal(pGoalEnt);
			}
		}

		m_saved_movetype = pTarget->pev->movetype;
		m_saved_solid = pTarget->pev->solid;
		m_saved_effects = pTarget->pev->effects;
		pTarget->pev->effects |= pev->effects;

		switch( m_fMoveTo )
		{
		case SCRIPT_MOVE_NO:
		case SCRIPT_MOVE_FACE:
			pTarget->m_scriptState = CBaseMonster::SCRIPT_WAIT;
			break;
		case SCRIPT_MOVE_WALK:
			pTarget->m_scriptState = CBaseMonster::SCRIPT_WALK_TO_MARK;
			DelayStart( 1 );
			break;
		case SCRIPT_MOVE_RUN:
			pTarget->m_scriptState = CBaseMonster::SCRIPT_RUN_TO_MARK;
			DelayStart( 1 );
			break;
		case SCRIPT_MOVE_INSTANT:
			UTIL_SetOrigin( pTarget->pev, pev->origin );
			pTarget->pev->ideal_yaw = pev->angles.y;
			pTarget->pev->avelocity = Vector( 0, 0, 0 );
			pTarget->pev->velocity = Vector( 0, 0, 0 );
			pTarget->pev->effects |= EF_NOINTERP;
			pTarget->pev->angles.y = pev->angles.y;
			pTarget->m_scriptState = CBaseMonster::SCRIPT_WAIT;
			m_startTime = gpGlobals->time + (float)1E6;
			// UNDONE: Add a flag to do this so people can fixup physics after teleporting monsters
			if (ShouldResetOnGroundFlag())
				pTarget->pev->flags &= ~FL_ONGROUND;
			break;
		case SCRIPT_MOVE_TELEPORT:
			pTarget->m_scriptState = CBaseMonster::SCRIPT_WAIT;
			if (ShouldResetOnGroundFlag())
				pTarget->pev->flags &= ~FL_ONGROUND;
			break;
		}
		//ALERT( at_aiconsole, "\"%s\" found and used (INT: %s)\n", STRING( pTarget->pev->targetname ), FBitSet( pev->spawnflags, SF_SCRIPT_NOINTERRUPT )? "No" : "Yes" );

		m_moveFailCount = 0;
		m_firedOnAnimStart = false;
		pTarget->m_IdealMonsterState = MONSTERSTATE_SCRIPT;
//		if( m_iszIdle )
//		{
//			StartSequence( pTarget, m_iszIdle, false );
//			if( FStrEq( STRING( m_iszIdle ), STRING( m_iszPlay ) ) )
//			{
//				pTarget->pev->framerate = 0;
//			}
//		}
		if( !FStringNull( m_iszFireOnPossessed ) )
		{
			CBaseEntity* pActivator = GetActivator(pTarget);
			FireTargets( STRING( m_iszFireOnPossessed ), pActivator, this );
		}
	}
}

void CCineMonster::CineThink()
{
	if (IsLockedByMaster() || !TryFindAndPossessEntity())
	{
		const float delay = m_searchDelay > 0.0f ? m_searchDelay : 0.0f;
		pev->nextthink = gpGlobals->time + delay;
	}
}

bool CCineMonster::TryFindAndPossessEntity()
{
	if( (m_hTargetEnt = FindEntity()) != 0 )
	{
		PossessEntity();
		ALERT( at_aiconsole, "script \"%s\" using monster \"%s\"\n", STRING( pev->targetname ), STRING( m_iszEntity ) );
		return true;
	}
	else
	{
		CancelScript();
		return false;
	}
}

bool CCineMonster::MayReportInappropriateTarget(int checkFail)
{
	if (!m_cantPlayReported && !IsAutoSearch())
	{
		ALERT( at_console, "script \"%s\" found \"%s\", but can't play! Reason: %s\n", STRING(pev->targetname), STRING( m_iszEntity ), ScriptCheckFailMessage(checkFail) );
		if (!FBitSet(pev->spawnflags, SF_SCRIPT_TRY_ONCE))
			m_cantPlayReported = true;
		return true;
	}
	return false;
}

bool CCineMonster::IsAppropriateTarget(CBaseMonster *pTarget, int interruptFlags, bool shouldCheckRadius, int *pCheckFail)
{
	if (FCanOverrideState())
		interruptFlags |= SS_INTERRUPT_ANYSTATE;

	int searchFail = CHECKFAIL_NOFAIL;
	if (!pTarget)
	{
		searchFail = CHECKFAIL_TARGET_IS_NULL;
	}
	else if (!pTarget->CanPlaySequence( interruptFlags ))
	{
		searchFail = CHECKFAIL_TARGET_IS_BUSY;
	}
	else if (!MatchingMonsterState(pTarget->m_MonsterState, m_requiredState))
	{
		searchFail = CHECKFAIL_UNMATCHING_MONSTERSTATE;
	}
	else if (!AcceptedFollowingState(pTarget))
	{
		searchFail = CHECKFAIL_UNMATCHING_FOLLOWERSTATE;
	}
	else if (shouldCheckRadius)
	{
		if ((pev->origin - pTarget->pev->origin).IsLengthGreaterThan(m_flRadius))
		{
			searchFail = CHECKFAIL_TARGET_IS_TOOFAR;
		}
	}
	if (pCheckFail)
		*pCheckFail = searchFail;
	return searchFail == CHECKFAIL_NOFAIL;
}

typedef enum
{
	STA_NO = 0,
	STA_SCRIPT = 1,
	STA_MONSTER = 2,
	STA_FORWARD = 3,
} SCRIPT_TARGET_ACTIVATOR;

void CCineMonster::FireOnAnimStart(CBaseMonster *pTarget)
{
	if (!m_firedOnAnimStart && !FStringNull(m_iszFireOnAnimStart))
	{
		CBaseEntity* pActivator = GetActivator(pTarget);
		FireTargets(STRING(m_iszFireOnAnimStart), pActivator, this);
	}
	m_firedOnAnimStart = true;
}

// lookup a sequence name and setup the target monster to play it
bool CCineMonster::StartSequence(CBaseMonster *pTarget, string_t iszSeq, bool completeOnEmpty )
{
	if( !iszSeq && completeOnEmpty )
	{
		// no sequence was provided. Just let the monster proceed, however, we still have to fire any Sequence target
		// and remove any non-repeatable CineAI entities here ( because there is code elsewhere that handles those tasks, but
		// not until the animation sequence is finished. We have to manually take care of these things where there is no sequence.

		SequenceDone( pTarget );
		return false;
	}

	if ( m_iszPlay != 0 && iszSeq == m_iszPlay )
	{
		FireOnAnimStart(pTarget);
	}

	pTarget->pev->sequence = pTarget->LookupSequence( STRING( iszSeq ) );
	if( pTarget->pev->sequence == -1 )
	{
		ALERT( at_error, "%s: unknown scripted sequence \"%s\"\n", STRING( pTarget->pev->targetname ), STRING( iszSeq ) );
		pTarget->pev->sequence = 0;
		// return false;
	}
#if 0
	char *s;
	if( pev->spawnflags & SF_SCRIPT_NOINTERRUPT )
		s = "No";
	else
		s = "Yes";

	ALERT( at_console, "%s (%s): started \"%s\":INT:%s\n", STRING( pTarget->pev->targetname ), STRING( pTarget->pev->classname ), STRING( iszSeq ), s );
#endif
	pTarget->pev->frame = 0;
	pTarget->ResetSequenceInfo();
	if (m_playFramerate && iszSeq == m_iszPlay)
		pTarget->pev->framerate = m_playFramerate;
	return true;
}

//=========================================================
// SequenceDone - called when a scripted sequence animation
// sequence is done playing ( or when an AI Scripted Sequence
// doesn't supply an animation sequence to play ). Expects
// the CBaseMonster pointer to the monster that the sequence
// possesses.
//=========================================================
void CCineMonster::SequenceDone( CBaseMonster *pMonster )
{
	//ALERT( at_aiconsole, "Sequence %s finished\n", STRING( m_pCine->m_iszPlay ) );

	m_iRepeatsLeft = m_iRepeats;

	if( !( pev->spawnflags & SF_SCRIPT_REPEATABLE ) )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1f;
	}

	// This is done so that another sequence can take over the monster when triggered by the first

	pMonster->CineCleanup();

	FixScriptMonsterSchedule( pMonster );

	// This may cause a sequence to attempt to grab this guy NOW, so we have to clear him out
	// of the existing sequence
	CBaseEntity* pActivator = GetActivator(pMonster);
	SUB_UseTargets( pActivator );
}

//=========================================================
// When a monster finishes a scripted sequence, we have to
// fix up its state and schedule for it to return to a
// normal AI monster.
//
// AI Scripted sequences will, depending on what the level
// designer selects:
//
// -Dirty the monster's schedule and drop out of the
//  sequence in their current state.
//
// -Select a specific AMBUSH schedule, regardless of state.
//=========================================================
void CCineMonster::FixScriptMonsterSchedule( CBaseMonster *pMonster )
{
	if( pMonster->m_IdealMonsterState != MONSTERSTATE_DEAD )
		pMonster->m_IdealMonsterState = MONSTERSTATE_IDLE;

	switch ( m_iFinishSchedule )
	{
		case SCRIPT_FINISHSCHED_DEFAULT:
			pMonster->ClearSchedule();
			break;
		case SCRIPT_FINISHSCHED_AMBUSH:
			pMonster->ChangeSchedule( pMonster->GetScheduleOfType( SCHED_AMBUSH ) );
			break;
		default:
			ALERT( at_aiconsole, "FixScriptMonsterSchedule - no case!\n" );
			pMonster->ClearSchedule();
			break;
	}
}

bool CBaseMonster::ExitScriptedSequence()
{
	if( pev->deadflag == DEAD_DYING )
	{
		// is this legal?
		// BUGBUG -- This doesn't call Killed()
		m_IdealMonsterState = MONSTERSTATE_DEAD;
		return false;
	}

	if( m_pCine )
	{
		m_pCine->CancelScript(SCRIPT_CANCELLATION_REASON_INTERRUPTED);
	}

	return true;
}

bool CCineMonster::ForcedNoInterruptions()
{
	return (pev->spawnflags & SF_SCRIPT_NOINTERRUPT) || m_interruptionPolicy == SCRIPT_INTERRUPTION_POLICY_NO_INTERRUPTIONS;
}

void CCineMonster::AllowInterrupt( bool fAllow )
{
	if( ForcedNoInterruptions() )
		return;
	m_interruptable = fAllow;
}

bool CCineMonster::CanInterrupt()
{
	if( !m_interruptable )
		return false;

	CBaseEntity *pTarget = m_hTargetEnt;

	if( pTarget != NULL && pTarget->pev->deadflag == DEAD_NO )
		return true;

	return false;
}

bool CCineMonster::CanInterruptByPlayerCall()
{
	return m_interruptionPolicy != SCRIPT_INTERRUPTION_POLICY_ONLY_DEATH && CanInterrupt();
}

bool CCineMonster::CanInterruptByBarnacle()
{
	// Same as CanInterruptByPlayerCall for now.
	// Barnacle actually can kill its victim, but might be killed before that as well.
	return m_interruptionPolicy != SCRIPT_INTERRUPTION_POLICY_ONLY_DEATH && CanInterrupt();
}

int CCineMonster::IgnoreConditions()
{
	if( CanInterrupt() )
		return 0;
	return SCRIPT_BREAK_CONDITIONS;
}

void ScriptEntityCancel( edict_t *pentCine, int cancellationReason )
{
	// make sure they are a scripted_sequence
	if( FClassnameIs( pentCine, "scripted_sequence" ) || FClassnameIs( pentCine, "scripted_action" ) )
	{
		CCineMonster *pCineTarget = GetClassPtr( (CCineMonster *)VARS( pentCine ) );

		// make sure they have a monster in mind for the script
		CBaseEntity *pEntity = pCineTarget->m_hTargetEnt;
		CBaseMonster *pTarget = NULL;
		if( pEntity )
			pTarget = pEntity->MyMonsterPointer();

		if( pTarget )
		{
			// make sure their monster is actually playing a script
			if( pTarget->m_MonsterState == MONSTERSTATE_SCRIPT || pTarget->m_IdealMonsterState == MONSTERSTATE_SCRIPT )
			{
				// tell them do die
				pTarget->m_scriptState = CBaseMonster::SCRIPT_CLEANUP;
				// do it now
				pTarget->CineCleanup();
				//LRC - clean up so that if another script is starting immediately, the monster will notice it.
				pTarget->ClearSchedule();
			}
		}

		if (cancellationReason && FBitSet(pCineTarget->pev->spawnflags, SF_SCRIPT_REMOVE_ON_INTERRUPTION))
		{
			pCineTarget->SetThink(&CBaseEntity::SUB_Remove);
			pCineTarget->pev->nextthink = gpGlobals->time;
		}
	}
}

// find all the cinematic entities with my targetname and stop them from playing
void CCineMonster::CancelScript(int cancellationReason)
{
	//ALERT( at_aiconsole, "Cancelling script: %s\n", STRING( m_iszPlay ) );

	if( !pev->targetname )
	{
		ScriptEntityCancel( edict(), cancellationReason );
		return;
	}

	edict_t *pentCineTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->targetname ) );

	while( !FNullEnt( pentCineTarget ) )
	{
		ScriptEntityCancel( pentCineTarget, cancellationReason );
		pentCineTarget = FIND_ENTITY_BY_TARGETNAME( pentCineTarget, STRING( pev->targetname ) );
	}
}

// find all the cinematic entities with my targetname and tell them to wait before starting
void CCineMonster::DelayStart( int state )
{
	// Need this to use m_iszFireOnBegin with unnamed scripts
	if ( FStringNull( pev->targetname ) )
	{
		if (state == 0 && !FStringNull(m_iszFireOnBegin))
		{
			CBaseEntity* pActivator = GetActivator(m_hTargetEnt);
			FireTargets(STRING(m_iszFireOnBegin), pActivator, this);
		}
		return;
	}

	edict_t *pentCine = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->targetname ) );

	while( !FNullEnt( pentCine ) )
	{
		if( FClassnameIs( pentCine, "scripted_sequence" ) || FClassnameIs( pentCine, "scripted_action" ) )
		{
			CCineMonster *pTarget = GetClassPtr( ( CCineMonster *)VARS( pentCine ) );
			if( state )
			{
				pTarget->m_iDelay++;
			}
			else
			{
				pTarget->m_iDelay--;
				if( pTarget->m_iDelay <= 0 )
				{
					pTarget->m_startTime = gpGlobals->time + 0.05f;

					CBaseEntity* pActivator = GetActivator(m_hTargetEnt);
					FireTargets(STRING(m_iszFireOnBegin), pActivator, this); //LRC
				}
			}
		}
		pentCine = FIND_ENTITY_BY_TARGETNAME( pentCine, STRING( pev->targetname ) );
	}
}

// Find an entity that I'm interested in and precache the sounds he'll need in the sequence.
void CCineMonster::Activate()
{
	edict_t *pentTarget;
	CBaseMonster *pTarget;

	if (UTIL_TargetnameIsActivator(m_iszEntity))
	{
		// Can't precache anything because depends on the activator
		return;
	}

	// The entity name could be a target name or a classname
	// Check the targetname
	pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszEntity ) );
	pTarget = NULL;

	while( !pTarget && !FNullEnt( pentTarget ) )
	{
		if( FBitSet( VARS( pentTarget )->flags, FL_MONSTER ) )
		{
			pTarget = GetMonsterPointer( pentTarget );
		}
		pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( m_iszEntity ) );
	}

	// If no entity with that targetname, check the classname
	if ( !pTarget )
	{
		pentTarget = FIND_ENTITY_BY_CLASSNAME(NULL, STRING( m_iszEntity ) );
		while( !pTarget && !FNullEnt( pentTarget ) )
		{
			pTarget = GetMonsterPointer( pentTarget );
			pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING( m_iszEntity ) );
		}
	}

	// Found a compatible entity
	if ( pTarget )
	{
		void *pmodel;
		pmodel = GET_MODEL_PTR( pTarget->edict() );
		if( pmodel )
		{
			// Look through the event list for stuff to precache
			SequencePrecache( pmodel, STRING( m_iszIdle ) );
			SequencePrecache( pmodel, STRING( m_iszPlay ) );
		}
	}
}

void CCineMonster::UpdateOnRemove()
{
	CBaseEntity* pEntity = m_hTargetEnt;
	if (pEntity)
	{
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if (pMonster && pMonster->m_pCine == this)
		{
			ALERT(at_aiconsole, "%s %s is removed. Calling CineCleanup on %s\n", STRING(pev->classname), STRING(pev->targetname), STRING(pMonster->pev->classname));
			pMonster->CineCleanup();
		}
	}

	CBaseDelay::UpdateOnRemove();
}

bool CBaseMonster::CineCleanup()
{
	CCineMonster *pOldCine = m_pCine;

	// am I linked to a cinematic?
	if( m_pCine )
	{
		// okay, reset me to what it thought I was before
		m_pCine->m_hTargetEnt = NULL;
		m_pCine->m_firedOnAnimStart = false;
		pev->movetype = m_pCine->m_saved_movetype;
		pev->solid = m_pCine->m_saved_solid;
		pev->effects = m_pCine->m_saved_effects;
		m_pCine->m_iDelay = 0;
	}
	else
	{
		// arg, punt
		pev->movetype = MOVETYPE_STEP;// this is evil
		pev->solid = SOLID_SLIDEBOX;
	}
	m_pCine = NULL;
	m_hTargetEnt = NULL;
	SetScriptedMoveGoal(NULL);
	if( pev->deadflag == DEAD_DYING )
	{
		// last frame of death animation?
		pev->health = 0;
		pev->framerate = 0.0;
		pev->solid = SOLID_NOT;
		SetState( MONSTERSTATE_DEAD );
		pev->deadflag = DEAD_DEAD;
		UTIL_SetSize( pev, pev->mins, Vector( pev->maxs.x, pev->maxs.y, pev->mins.z + 2 ) );

		OnDying(false);
		if( pOldCine && FBitSet( pOldCine->pev->spawnflags, SF_SCRIPT_LEAVECORPSE ) )
		{
			SetUse( NULL );
			SetThink( NULL );	// This will probably break some stuff
			SetTouch( NULL );
		}
		else
			SUB_StartFadeOut(); // SetThink( &SUB_DoNothing );
		// This turns off animation & physics in case their origin ends up stuck in the world or something
		StopAnimation();
		pev->movetype = MOVETYPE_NONE;
		pev->effects |= EF_NOINTERP;	// Don't interpolate either, assume the corpse is positioned in its final resting place
		return false;
	}

	// If we actually played a sequence
	if( pOldCine && pOldCine->m_iszPlay )
	{
		if( !( pOldCine->pev->spawnflags & SF_SCRIPT_NOSCRIPTMOVEMENT ) )
		{
			// reset position
			Vector new_origin, new_angle;
			GetBonePosition( 0, new_origin, new_angle );

			// Figure out how far they have moved
			// We can't really solve this problem because we can't query the movement of the origin relative
			// to the sequence.  We can get the root bone's position as we do here, but there are
			// cases where the root bone is in a different relative position to the entity's origin
			// before/after the sequence plays.  So we are stuck doing this:

			// !!!HACKHACK: Float the origin up and drop to floor because some sequences have
			// irregular motion that can't be properly accounted for.

			// UNDONE: THIS SHOULD ONLY HAPPEN IF WE ACTUALLY PLAYED THE SEQUENCE.
			Vector oldOrigin = pev->origin;

			// UNDONE: ugly hack.  Don't move monster if they don't "seem" to move
			// this really needs to be done with the AX,AY,etc. flags, but that aren't consistantly
			// being set, so animations that really do move won't be caught.
			if( ( oldOrigin - new_origin).IsLength2DLessThan(8.0f) )
				new_origin = oldOrigin;

			pev->origin.x = new_origin.x;
			pev->origin.y = new_origin.y;
			pev->origin.z += 1;

			if (FBitSet(pOldCine->pev->spawnflags, SF_SCRIPT_APPLYNEWANGLES))
			{
				pev->angles = new_angle;
				pev->ideal_yaw = UTIL_AngleMod( pev->angles.y );
			}

			if (FBitSet(pOldCine->pev->spawnflags, SF_SCRIPT_DONT_DROP_TO_FLOOR))
			{
				TraceResult tr;
				TRACE_MONSTER_HULL(edict(), pev->origin, pev->origin, dont_ignore_monsters, edict(), &tr);

				if (tr.fAllSolid)
				{
					if (!FBitSet(pOldCine->pev->spawnflags, SF_SCRIPT_ALLOW_STUCK))
						pev->origin = oldOrigin;
					else
						pev->origin.z = new_origin.z;
				}
				else
				{
					pev->origin.z = new_origin.z;
					pev->flags &= ~FL_ONGROUND;
				}
			}
			else
			{
				pev->flags |= FL_ONGROUND;
				int drop = DROP_TO_FLOOR( ENT( pev ) );

				// Origin in solid?  Set to org at the end of the sequence
				if (drop < 0)
				{
					if (!FBitSet(pOldCine->pev->spawnflags, SF_SCRIPT_ALLOW_STUCK))
						pev->origin = oldOrigin;
					else
						pev->origin.z = new_origin.z;
				}
				else if (drop == 0) // Hanging in air?
				{
					pev->origin.z = new_origin.z;
					pev->flags &= ~FL_ONGROUND;
				}
				// else entity hit floor, leave there
			}

			// pEntity->pev->origin.z = new_origin.z + 5.0; // damn, got to fix this

			UTIL_SetOrigin( pev, pev->origin );
			pev->effects |= EF_NOINTERP;
		}

		// We should have some animation to put these guys in, but for now it's idle.
		// Due to NOINTERP above, there won't be any blending between this anim & the sequence
		if (!FBitSet(pOldCine->pev->spawnflags, SF_SCRIPT_CONTINUOUS))
			m_Activity = ACT_RESET;
	}
	// set them back into a normal state
	pev->enemy = NULL;
	if( pev->health > 0 )
	{
		m_IdealMonsterState = MONSTERSTATE_IDLE; // m_previousState;

		if (pOldCine && FStringNull(pOldCine->m_iszPlay) && !FStringNull(pOldCine->m_iszIdle))
		{
			m_IdealActivity = ACT_IDLE;
			m_Activity = ACT_RESET;
		}
	}
	else
	{
		// Dropping out because he got killed
		// Now we call OnDying instead
		m_IdealMonsterState = MONSTERSTATE_DEAD;
		OnDying(false);
		SetConditions( bits_COND_LIGHT_DAMAGE );
		pev->deadflag = DEAD_DYING;
		FCheckAITrigger();
		pev->deadflag = DEAD_NO;
	}

	//	SetAnimation( m_MonsterState );
	ClearBits( pev->spawnflags, SF_MONSTER_WAIT_FOR_SCRIPT );

	return true;
}

void CBaseMonster::SetScriptedMoveGoal(CBaseEntity *pEntity)
{
	m_hMoveGoalEnt = m_pGoalEnt = pEntity;
}

CBaseEntity* CBaseMonster::ScriptedMoveGoal()
{
	return m_hMoveGoalEnt;
}

void CCineMonster::OnMoveFail() {
	if (m_maxMoveFailAttempts > 0 && m_moveFailCount < m_maxMoveFailAttempts) {
		m_moveFailCount++;
	}
}

bool CCineMonster::MoveFailAttemptsExceeded() const {
	return m_maxMoveFailAttempts > 0 && m_moveFailCount >= m_maxMoveFailAttempts;
}

bool CCineMonster::IsAutoSearch() const
{
	return FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_SCRIPT_AUTOSEARCH);
}

CBaseEntity* CCineMonster::GetActivator(CBaseEntity* pMonster)
{
	CBaseEntity* pActivator = NULL;
	if (m_targetActivator == STA_SCRIPT)
	{
		pActivator = this;
	}
	else if (m_targetActivator == STA_MONSTER)
	{
		pActivator = pMonster;
	}
	else if (m_targetActivator == STA_FORWARD)
	{
		pActivator = m_hActivator;
	}
	return pActivator;
}

class CScriptedSentence : public CBaseDelay
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void EXPORT FindThink();
	void EXPORT DelayThink();
	int ObjectCaps() override { return ( CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ); }

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	CBaseToggle *FindEntity();
	bool AcceptableSpeaker( CBaseToggle *pTarget );
	bool InterlocutorIsInRange(CBaseEntity* pTarget, float flRadius, const Vector& searchOrigin);
	bool StartSentence( CBaseToggle *pTarget );

	float SpeakerSearchRadius() const {
		return m_flRadius;
	}
	float ListenerSearchRadius() const {
		return m_flListenerRadius > 0.0f ? m_flListenerRadius : m_flRadius;
	}

private:
	string_t m_iszSentence;		// string index for idle animation
	string_t m_iszEntity;	// entity that is wanted for this sentence
	float m_flRadius;		// range to search
	float m_flDuration;	// How long the sentence lasts
	float m_flRepeat;	// repeat rate
	float m_flAttenuation;
	float m_flVolume;
	bool m_active;
	string_t m_iszListener; // name of entity to look at while talking
	short m_requiredState;
	short m_followAction;
	short m_targetActivator;
	float m_flListenerRadius;

	short m_searchPolicy;
	short m_applySearchRadius;
	string_t m_searchOrigin;
	short m_allowUse;
};

#define SF_SENTENCE_ONCE	0x0001
#define SF_SENTENCE_FOLLOWERS	0x0002	// only say if following player
#define SF_SENTENCE_INTERRUPT	0x0004	// force talking except when dead
#define SF_SENTENCE_CONCURRENT	0x0008	// allow other people to keep talking
#define SF_SENTENCE_REQUIRE_LISTENER 0x0010 // require presense of the listener
#define SF_SENTENCE_LISTENER_LOOKS_AT_SPEAKER	0x0020
#define SF_SENTENCE_SPEAKER_TURNS_TO_LISTENER 0x0100
#define SF_SENTENCE_LISTENER_TURNS_TO_SPEAKER 0x0200
#define SF_SENTENCE_TRY_ONCE 0x1000 // try finding monster once, don't go into the loop

enum
{
	FOLLOW_NO = 0,
	FOLLOW_START,
	FOLLOW_STOP,
};

TYPEDESCRIPTION	CScriptedSentence::m_SaveData[] =
{
	DEFINE_FIELD( CScriptedSentence, m_iszSentence, FIELD_STRING ),
	DEFINE_FIELD( CScriptedSentence, m_iszEntity, FIELD_STRING ),
	DEFINE_FIELD( CScriptedSentence, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flRepeat, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flAttenuation, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_active, FIELD_BOOLEAN ),
	DEFINE_FIELD( CScriptedSentence, m_iszListener, FIELD_STRING ),
	DEFINE_FIELD( CScriptedSentence, m_requiredState, FIELD_SHORT ),
	DEFINE_FIELD( CScriptedSentence, m_followAction, FIELD_SHORT ),
	DEFINE_FIELD( CScriptedSentence, m_targetActivator, FIELD_SHORT ),
	DEFINE_FIELD( CScriptedSentence, m_flListenerRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_searchPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CScriptedSentence, m_applySearchRadius, FIELD_SHORT ),
	DEFINE_FIELD( CScriptedSentence, m_searchOrigin, FIELD_STRING ),
	DEFINE_FIELD( CScriptedSentence, m_allowUse, FIELD_SHORT ),
};

IMPLEMENT_SAVERESTORE( CScriptedSentence, CBaseDelay )

LINK_ENTITY_TO_CLASS( scripted_sentence, CScriptedSentence )

void CScriptedSentence::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "sentence" ) )
	{
		m_iszSentence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "entity" ) )
	{
		m_iszEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		m_flDuration = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		m_flRadius = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "refire") )
	{
		m_flRepeat = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "attenuation" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		m_flVolume = atof( pkvd->szValue ) * 0.1f;
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "listener" ) )
	{
		m_iszListener = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "required_state" ) )
	{
		m_requiredState = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "follow_action" ) )
	{
		m_followAction = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "target_activator" ) )
	{
		m_targetActivator = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "listener_radius" ) )
	{
		m_flListenerRadius = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_searchPolicy" ) )
	{
		m_searchPolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_applySearchRadius" ) )
	{
		m_applySearchRadius = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "search_origin" ) )
	{
		m_searchOrigin = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "allow_use" ) )
	{
		m_allowUse = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CScriptedSentence::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !m_active )
		return;
	//ALERT( at_console, "Firing sentence: %s\n", STRING( m_iszSentence ) );
	m_hActivator = pActivator;
	SetThink( &CScriptedSentence::FindThink );
	pev->nextthink = gpGlobals->time;
}

void CScriptedSentence::Spawn()
{
	pev->solid = SOLID_NOT;

	m_active = true;
	// if no targetname, start now
	if( !pev->targetname )
	{
		SetThink( &CScriptedSentence::FindThink );
		pev->nextthink = gpGlobals->time + 1.0f;
	}

	switch( pev->impulse )
	{
	case 1:
		// Medium radius
		m_flAttenuation = ATTN_STATIC;
		break;
	case 2:
		// Large radius
		m_flAttenuation = ATTN_NORM;
		break;
	case 3:
		//EVERYWHERE
		m_flAttenuation = ATTN_NONE;
		break;
	case 4:
		m_flAttenuation = 0.5;
		break;
	case 5:
		m_flAttenuation = 0.25;
		break;
	default:
	case 0:
		// Small radius
		m_flAttenuation = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if( m_flVolume <= 0.0f )
		m_flVolume = 1.0f;
}

void CScriptedSentence::FindThink()
{
	CBaseToggle *pTarget = FindEntity();
	if( pTarget && StartSentence( pTarget ) )
	{
		if( pev->spawnflags & SF_SENTENCE_ONCE )
			UTIL_Remove( this );
		SetThink( &CScriptedSentence::DelayThink );
		pev->nextthink = gpGlobals->time + m_flDuration + m_flRepeat;
		m_active = false;
		//ALERT( at_console, "%s: found target %s\n", STRING( m_iszSentence ), STRING( m_iszEntity ) );
	}
	else
	{
		//ALERT( at_console, "%s: can't find monster %s\n", STRING( m_iszSentence ), STRING( m_iszEntity ) );
		if (!FBitSet(pev->spawnflags, SF_SENTENCE_TRY_ONCE))
			pev->nextthink = gpGlobals->time + m_flRepeat + 0.5f;
	}
}

void CScriptedSentence::DelayThink()
{
	m_active = true;
	if( !pev->targetname )
		pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CScriptedSentence::FindThink );
}

bool CScriptedSentence::AcceptableSpeaker( CBaseToggle *pTarget )
{
	if( pTarget )
	{
		CBaseMonster *pMonster = pTarget->MyMonsterPointer();
		if( pMonster )
		{
			if (!MatchingMonsterState(pMonster->m_MonsterState, m_requiredState))
				return false;
			if( pev->spawnflags & SF_SENTENCE_FOLLOWERS )
			{
				if( pMonster->m_hTargetEnt == 0 || !pMonster->m_hTargetEnt->IsPlayer() )
					return false;
			}

			bool override;
			if( pev->spawnflags & SF_SENTENCE_INTERRUPT )
				override = true;
			else
				override = false;

			if( pMonster->CanPlaySentence( override ) )
				return true;
		}
		else
			return pTarget->IsAllowedToSpeak();
	}
	return false;
}

bool CScriptedSentence::InterlocutorIsInRange(CBaseEntity *pTarget, float flRadius, const Vector& searchOrigin)
{
	if (!pTarget)
		return false;
	return (searchOrigin - pTarget->pev->origin).IsLengthLessThanOrEqual(flRadius);
}

CBaseToggle *CScriptedSentence::FindEntity()
{
	CBaseToggle *pTarget = NULL;
	Vector searchOrigin = pev->origin;
	if (!FStringNull(m_searchOrigin))
	{
		if (!TryCalcLocus_Position(this, m_hActivator, STRING(m_searchOrigin), searchOrigin))
			return NULL;
	}

	if (UTIL_TargetnameIsActivator(m_iszEntity))
	{
		if (m_hActivator != 0 && FBitSet(m_hActivator->pev->flags, FL_MONSTER) && (pTarget = m_hActivator->MyTogglePointer()) != 0 )
		{
			if( AcceptableSpeaker( pTarget ) )
			{
				if (m_applySearchRadius != SCRIPT_APPLY_SEARCH_RADIUS_ALWAYS || InterlocutorIsInRange(pTarget, SpeakerSearchRadius(), searchOrigin))
					return pTarget;
			}
		}
		return NULL;
	}

	edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszEntity ) );
	pTarget = NULL;

	if ( m_searchPolicy != SCRIPT_SEARCH_POLICY_CLASSNAME_ONLY )
	{
		while( !FNullEnt( pentTarget ) )
		{
			pTarget = CBaseEntity::Instance( pentTarget )->MyTogglePointer();
			if( pTarget != NULL )
			{
				if( AcceptableSpeaker( pTarget ) )
				{
					if (m_applySearchRadius != SCRIPT_APPLY_SEARCH_RADIUS_ALWAYS || InterlocutorIsInRange(pTarget, SpeakerSearchRadius(), searchOrigin))
						return pTarget;
				}
				//ALERT( at_console, "%s (%s), not acceptable\n", STRING( pMonster->pev->classname ), STRING( pMonster->pev->targetname ) );
			}
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( m_iszEntity ) );
		}
	}

	if ( m_searchPolicy != SCRIPT_SEARCH_POLICY_TARGETNAME_ONLY )
	{
		CBaseEntity *pEntity = NULL;
		while( ( pEntity = UTIL_FindEntityInSphere( pEntity, searchOrigin, SpeakerSearchRadius() ) ) != NULL )
		{
			if( FClassnameIs( pEntity->pev, STRING( m_iszEntity ) ) )
			{
				if( FBitSet( pEntity->pev->flags, FL_MONSTER ) )
				{
					pTarget = pEntity->MyMonsterPointer();
					if( AcceptableSpeaker( pTarget ) )
						return pTarget;
				}
			}
		}
	}

	return NULL;
}

bool CScriptedSentence::StartSentence( CBaseToggle *pTarget )
{
	if( !pTarget )
	{
		ALERT( at_aiconsole, "Not Playing sentence %s\n", STRING( m_iszSentence ) );
		return false;
	}

	bool bConcurrent = false;
	if( !( pev->spawnflags & SF_SENTENCE_CONCURRENT ) )
		bConcurrent = true;

	CBaseEntity *pListener = NULL;
	if( !FStringNull( m_iszListener ) )
	{
		float radius = ListenerSearchRadius();
		const bool listenerRequired = FBitSet(pev->spawnflags, SF_SENTENCE_REQUIRE_LISTENER);

		if (UTIL_TargetnameIsActivator(m_iszListener))
		{
			CBaseEntity* pActivator = m_hActivator;
			if (InterlocutorIsInRange(pActivator, radius, pTarget->pev->origin))
			{
				pListener = pActivator;
			}
		}
		else if (UTIL_IsPlayerReference(STRING(m_iszListener)))
		{
			CBasePlayer* pPlayer = g_pGameRules->EffectiveAlivePlayer(m_hActivator);
			if (InterlocutorIsInRange(pPlayer, radius, pTarget->pev->origin))
			{
				pListener = pPlayer;
			}
		}
		else
		{
			if (!listenerRequired && FStrEq( STRING( m_iszListener ), "player" ))
				radius = 4096;	// Always find the player unless listener is explicitly required

			pListener = UTIL_FindEntityGeneric( STRING( m_iszListener ), pTarget->pev->origin, radius );
		}

		if (!pListener && listenerRequired)
		{
			return false;
		}
	}

	CBaseMonster* pMonster = pTarget->MyMonsterPointer();
	if (pMonster)
	{
		CTalkMonster* pTalkMonster = pMonster->MyTalkMonsterPointer();
		if (pTalkMonster)
		{
			pTalkMonster->m_allowUseScriptedSentence = m_allowUse != 0;
		}
	}
	pTarget->PlayScriptedSentence( STRING( m_iszSentence ), m_flDuration,  m_flVolume, m_flAttenuation, bConcurrent, pListener );
	if (pMonster != 0 && m_followAction)
	{
		CFollowingMonster* followingMonster = pMonster->MyFollowingMonsterPointer();
		if (followingMonster)
		{
			if (m_followAction == FOLLOW_START)
			{
				if (!followingMonster->IsFollowingPlayer())
				{
					CBaseEntity* pPlayer = UTIL_ClosestAlivePlayer(followingMonster->Center());
					followingMonster->DoFollowerUse(pPlayer, false, USE_ON, true);
				}
			}
			else if (m_followAction == FOLLOW_STOP)
			{
				followingMonster->DoFollowerUse(followingMonster->FollowedPlayer(), false, USE_OFF, true);
			}
		}
	}
	ALERT( at_aiconsole, "Playing sentence %s (%.1f)\n", STRING( m_iszSentence ), m_flDuration );

	CBaseEntity* pActivator = NULL;
	if (m_targetActivator == STA_SCRIPT)
	{
		pActivator = this;
	}
	else if (m_targetActivator == STA_MONSTER)
	{
		pActivator = pTarget;
	}
	else if (m_targetActivator == STA_FORWARD)
	{
		pActivator = m_hActivator;
	}
	SUB_UseTargets( pActivator );

	if (pListener)
	{
		if (pMonster != 0 && FBitSet(pev->spawnflags, SF_SENTENCE_SPEAKER_TURNS_TO_LISTENER)) {
			pMonster->SuggestSchedule(SCHED_IDLE_FACE, pListener);
		}
		if (FBitSet(pev->spawnflags, SF_SENTENCE_LISTENER_TURNS_TO_SPEAKER)) {
			CBaseMonster* pMonsterListener = pListener->MyMonsterPointer();
			if (pMonsterListener) {
				pMonsterListener->SuggestSchedule(SCHED_IDLE_FACE, pTarget);
			}
		}

		if (FBitSet(pev->spawnflags, SF_SENTENCE_LISTENER_LOOKS_AT_SPEAKER)) {
			CBaseMonster* pListenerMonster = pListener->MyMonsterPointer();
			if (pListenerMonster) {
				CTalkMonster* pTalkMonster = pListenerMonster->MyTalkMonsterPointer();
				if (pTalkMonster && pTalkMonster->m_flStopLookTime <= gpGlobals->time) {
					pTalkMonster->m_hTalkTarget = this;
					pTalkMonster->m_flStopLookTime = gpGlobals->time + m_flDuration;
				}
			}
		}
	}
	return true;
}

//=========================================================
// Furniture - this is the cool comment I cut-and-pasted
//=========================================================
class CFurniture : public CBaseMonster
{
public:
	void Spawn() override;
	int DefaultClassify() override;
	int ObjectCaps() override { return (CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
};

LINK_ENTITY_TO_CLASS( monster_furniture, CFurniture )

//=========================================================
// This used to have something to do with bees flying, but
// now it only initializes moving furniture in scripted sequences
//=========================================================
void CFurniture::Spawn()
{
	PRECACHE_MODEL( STRING( pev->model ) );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	pev->health = 80000;
	pev->takedamage = DAMAGE_AIM;
	pev->effects = 0;
	pev->yaw_speed = 0;
	pev->sequence = 0;
	pev->frame = 0;

	//pev->nextthink += 1.0f;
	//SetThink( &WalkMonsterDelay );

	ResetSequenceInfo();
	pev->frame = 0;
	MonsterInit();
}

//=========================================================
// ID's Furniture as neutral (noone will attack it)
//=========================================================
int CFurniture::DefaultClassify()
{
	return CLASS_NONE;
}

enum
{
	SCRIPTED_SCHEDULE_MOVE_AUTOMATIC = 0,
	SCRIPTED_SCHEDULE_MOVE_WALK,
	SCRIPTED_SCHEDULE_MOVE_RUN
};

enum
{
	SCRIPTED_SCHEDULE_SPOT_AUTOMATIC = 0,
	SCRIPTED_SCHEDULE_SPOT_POSITION,
	SCRIPTED_SCHEDULE_SPOT_ENTITY,
};

enum
{
	SCRIPTED_SCHEDULE_CLEAR = 0,
	SCRIPTED_SCHEDULE_MOVE_AWAY,
	SCRIPTED_SCHEDULE_MOVE_TO_COVER,
	SCRIPTED_SCHEDULE_INVESTIGATE_SPOT,
	SCRIPTED_SCHEDULE_TURN_TO_SPOT,
	SCRIPTED_SCHEDULE_MOVE_TO_SPOT,
};

class CScriptedSchedule : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	int KnownSchedule() const;
	int ScheduleType() const;
	float MinDist() const { return pev->health; }
	float MaxDist() const { return pev->max_health; }
	int MovePreference() const { return pev->impulse; }
	int SpotPreference() const { return pev->button; }
};

int CScriptedSchedule::ScheduleType() const
{
	return pev->weapons;
}

int CScriptedSchedule::KnownSchedule() const
{
	switch (pev->weapons) {
	case SCRIPTED_SCHEDULE_CLEAR:
		return SCHED_NONE;
	case SCRIPTED_SCHEDULE_MOVE_AWAY:
		return SCHED_RETREAT_FROM_SPOT;
	case SCRIPTED_SCHEDULE_MOVE_TO_COVER:
		return SCHED_TAKE_COVER_FROM_SPOT;
	case SCRIPTED_SCHEDULE_INVESTIGATE_SPOT:
		return SCHED_INVESTIGATE_SPOT;
	case SCRIPTED_SCHEDULE_TURN_TO_SPOT:
		return SCHED_IDLE_FACE;
	case SCRIPTED_SCHEDULE_MOVE_TO_SPOT:
		return SCHED_MOVE_TO_SPOT;
	default:
		ALERT(at_aiconsole, "Unknown schedule type for scripted_schedule: %d\n", pev->weapons);
		return SCHED_NONE;
	}
}

void  CScriptedSchedule::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq( pkvd->szKeyName, "entity" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "schedule" ) )
	{
		pev->weapons = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "mindist" ) )
	{
		pev->health = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "maxdist" ) )
	{
		pev->max_health = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "spot_entity" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "move_preference" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "spot_preference" ) )
	{
		pev->button = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CScriptedSchedule::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pev->netname) {
		CBaseEntity* pSpotEntity = NULL;
		if (pev->message) {
			pSpotEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
			if (!pSpotEntity) {
				ALERT(at_aiconsole, "%s specifies \"%s\" as spot entity, but couldn't find it!\n", STRING(pev->classname), STRING(pev->message));
				return;
			}
		} else {
			const int scheduleType = ScheduleType();
			if (scheduleType == SCRIPTED_SCHEDULE_INVESTIGATE_SPOT || scheduleType == SCRIPTED_SCHEDULE_TURN_TO_SPOT || scheduleType == SCRIPTED_SCHEDULE_MOVE_TO_SPOT)
				pSpotEntity = this;
		}

		int flags = 0;
		switch (MovePreference()) {
		case SCRIPTED_SCHEDULE_MOVE_WALK:
			flags |= SUGGEST_SCHEDULE_FLAG_WALK;
			break;
		case SCRIPTED_SCHEDULE_MOVE_RUN:
			flags |= SUGGEST_SCHEDULE_FLAG_RUN;
			break;
		default:
			break;
		}

		switch (SpotPreference()) {
		case SCRIPTED_SCHEDULE_SPOT_POSITION:
			flags |= SUGGEST_SCHEDULE_FLAG_SPOT_IS_POSITION;
			break;
		case SCRIPTED_SCHEDULE_SPOT_ENTITY:
			flags |= SUGGEST_SCHEDULE_FLAG_SPOT_IS_ENTITY;
			break;
		default:
			break;
		}

		CBaseEntity* pEntity = 0;
		while( (pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->netname))) )
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster) {
				if (ScheduleType() == SCRIPTED_SCHEDULE_CLEAR) {
					pMonster->ClearSuggestedSchedule();
				} else {
					const int knownSchedule = KnownSchedule();
					if (knownSchedule != SCHED_NONE)
					{
						int schedFlags = flags;
						if (pev->weapons == SCRIPTED_SCHEDULE_MOVE_AWAY && (!pSpotEntity || pSpotEntity == pMonster))
							schedFlags |= SUGGEST_SCHEDULE_FLAG_DONT_AVOID_THREAT_NODE;
						pMonster->SuggestSchedule( knownSchedule, pSpotEntity, MinDist(), MaxDist(), schedFlags );
					}
				}
			}
		}
	} else {
		ALERT(at_aiconsole, "%s does not specify the affected monster!\n", STRING(pev->netname));
	}
}

LINK_ENTITY_TO_CLASS( scripted_schedule, CScriptedSchedule )

#define SF_SCRIPTED_FOLLOWING_ONCE (1 << 0)

enum
{
	SCRIPTED_FOLLOW_STOP = -1,
	SCRIPTED_FOLLOW_FORWARD = 0,
	SCRIPTED_FOLLOW_START = 1,
};

class CScriptedFollowing : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void EXPORT FindThink();

	int MakeFollowing(CBasePlayer* pPlayer, USE_TYPE useType);

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hActivator;
	string_t m_onStartFollowing;
	string_t m_onStopFollowing;
};

LINK_ENTITY_TO_CLASS( scripted_following, CScriptedFollowing )

TYPEDESCRIPTION	CScriptedFollowing::m_SaveData[] =
{
	DEFINE_FIELD( CScriptedFollowing, m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( CScriptedFollowing, m_onStartFollowing, FIELD_STRING ),
	DEFINE_FIELD( CScriptedFollowing, m_onStopFollowing, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CScriptedFollowing, CPointEntity )

void  CScriptedFollowing::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq( pkvd->szKeyName, "entity" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "on_start_following" ) )
	{
		m_onStartFollowing = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "on_stop_following" ) )
	{
		m_onStopFollowing = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "use_type") )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

int CScriptedFollowing::MakeFollowing(CBasePlayer* pPlayer, USE_TYPE useType)
{
	int result = FOLLOWING_INVALID;
	CBaseEntity* pEntity = 0;
	while( (pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->netname))) )
	{
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if (pMonster) {
			CFollowingMonster* pFollowingMonster = pMonster->MyFollowingMonsterPointer();
			if (pFollowingMonster)
			{
				result = pFollowingMonster->DoFollowerUse(pPlayer, false, useType, true);
				break;
			}
		}
	}
	if (result == FOLLOWING_STARTED)
	{
		if (!FStringNull(m_onStartFollowing))
			FireTargets(STRING(m_onStartFollowing), pPlayer, this);
	}
	if (result == FOLLOWING_STOPPED)
	{
		if (!FStringNull(m_onStopFollowing))
			FireTargets(STRING(m_onStopFollowing), pPlayer, this);
	}
	return result;
}

void CScriptedFollowing::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pev->netname) {
		CBasePlayer* pPlayer = g_pGameRules->EffectiveAlivePlayer(pActivator);
		if (!pPlayer)
			return;

		switch(pev->impulse)
		{
		case SCRIPTED_FOLLOW_STOP:
			useType = USE_OFF;
			break;
		case SCRIPTED_FOLLOW_START:
			useType = USE_ON;
			break;
		default:
			break;
		}

		SetThink(NULL);
		int result = MakeFollowing(pPlayer, useType);
		if (result == FOLLOWING_BUSYINSCRIPT && useType == USE_ON)
		{
			SetThink(&CScriptedFollowing::FindThink);
			pev->nextthink = gpGlobals->time + 0.1f;
		}
		else
		{
			if (FBitSet(pev->spawnflags, SF_SCRIPTED_FOLLOWING_ONCE))
				UTIL_Remove(this);
		}
	} else {
		ALERT(at_aiconsole, "%s does not specify the affected monster!\n", STRING(pev->classname));
	}
}

void CScriptedFollowing::FindThink()
{
	CBasePlayer* pPlayer = g_pGameRules->EffectiveAlivePlayer(m_hActivator);
	if (!pPlayer)
	{
		SetThink(NULL);
		return;
	}
	int result = MakeFollowing(pPlayer, USE_ON);
	if (result == FOLLOWING_BUSYINSCRIPT)
	{
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		SetThink(NULL);
		if (FBitSet(pev->spawnflags, SF_SCRIPTED_FOLLOWING_ONCE))
			UTIL_Remove(this);
	}
}
