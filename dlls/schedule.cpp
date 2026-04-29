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
//=========================================================
// schedule.cpp - functions and data pertaining to the 
// monsters' AI scheduling system.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "scripted.h"
#include "nodes.h"
#include "defaultai.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "ai_debug.h"

extern cvar_t npc_lateral_retreat;

//=========================================================
// FHaveSchedule - Returns true if monster's m_pSchedule
// is anything other than NULL.
//=========================================================
bool CBaseMonster::FHaveSchedule()
{
	return m_pSchedule != nullptr;
}

//=========================================================
// ClearSchedule - blanks out the caller's schedule pointer
// and index.
//=========================================================
void CBaseMonster::ClearSchedule()
{
	m_iTaskStatus = TASKSTATUS_NEW;
	m_pSchedule = NULL;
	m_iScheduleIndex = 0;

	Forget(bits_MEMORY_SHOULD_ROAM_IN_ALERT);
}

//=========================================================
// FScheduleDone - Returns true if the caller is on the
// last task in the schedule
//=========================================================
bool CBaseMonster::FScheduleDone()
{
	ASSERT( m_pSchedule != NULL );
	return m_iScheduleIndex == m_pSchedule->cTasks;
}

//=========================================================
// ChangeSchedule - replaces the monster's schedule pointer
// with the passed pointer, and sets the ScheduleIndex back
// to 0
//=========================================================
void CBaseMonster::ChangeSchedule( Schedule_t *pNewSchedule, bool isSuggested )
{
	ASSERT( pNewSchedule != NULL );

	if (isSuggested) {
		m_suggestedSchedule = SCHED_NONE; // don't loop
		m_suggestedScheduleFlags |= SUGGEST_SCHEDULE_FLAG_PLAYING;
	} else {
		// clear overrides
		ClearSuggestedSchedule();
	}

	if (ShouldReportAIChange(entindex()))
	{
		ALERT(at_aiconsole, "%s (%d): changing schedule to %s\n", STRING(pev->classname), entindex(), pNewSchedule->pName);
	}

	OnChangeSchedule( pNewSchedule );

	if (m_MonsterState == MONSTERSTATE_HUNT)
	{
		m_huntActivitiesCount++;
	}

	m_pSchedule = pNewSchedule;
	m_iScheduleIndex = 0;
	m_iTaskStatus = TASKSTATUS_NEW;
	m_afConditions = 0;// clear all of the conditions
	m_failSchedule = SCHED_NONE;

	if( m_pSchedule->iInterruptMask & bits_COND_HEAR_SOUND && !(m_pSchedule->iSoundMask) )
	{
		ALERT( at_aiconsole, "COND_HEAR_SOUND with no sound mask! (classname: %s; schedule: %s)\n",
			   STRING(pev->classname), m_pSchedule->pName );
	}
	else if( m_pSchedule->iSoundMask && !(m_pSchedule->iInterruptMask & bits_COND_HEAR_SOUND) )
	{
		ALERT( at_aiconsole, "Sound mask without COND_HEAR_SOUND! (classname: %s; schedule: %s\n",
			   STRING(pev->classname), m_pSchedule->pName);
	}

#if _DEBUG
	if( !ScheduleFromName( pNewSchedule->pName ) )
	{
		ALERT( at_console, "Schedule %s not in table!!!\n", pNewSchedule->pName );
	}
#endif
	
// this is very useful code if you can isolate a test case in a level with a single monster. It will notify
// you of every schedule selection the monster makes.
#if 0
	if( FClassnameIs( pev, "monster_human_grunt" ) )
	{
		Task_t *pTask = GetTask();

		if( pTask )
		{
			const char *pName = NULL;

			if( m_pSchedule )
			{
				pName = m_pSchedule->pName;
			}
			else
			{
				pName = "No Schedule";
			}

			if( !pName )
			{
				pName = "Unknown";
			}

			ALERT( at_aiconsole, "%s: picked schedule %s\n", STRING( pev->classname ), pName );
		}
	}
#endif// 0
}

//=========================================================
// NextScheduledTask - increments the ScheduleIndex
//=========================================================
void CBaseMonster::NextScheduledTask()
{
	ASSERT( m_pSchedule != NULL );

	m_iTaskStatus = TASKSTATUS_NEW;
	m_iScheduleIndex++;

	if( FScheduleDone() )
	{
		// just completed last task in schedule, so make it invalid by clearing it.
		SetConditions( bits_COND_SCHEDULE_DONE );
		//ClearSchedule();	
	}
}

//=========================================================
// IScheduleFlags - returns an integer with all Conditions
// bits that are currently set and also set in the current
// schedule's Interrupt mask.
//=========================================================
int CBaseMonster::IScheduleFlags()
{
	if( !m_pSchedule )
	{
		return 0;
	}

	// strip off all bits excepts the ones capable of breaking this schedule.
	return m_afConditions & m_pSchedule->iInterruptMask;
}

static const std::pair<int, const char*> g_ConditionsNames[] = {
	{bits_COND_NO_AMMO_LOADED, "No ammo loaded"},
	{bits_COND_SEE_HATE, "Sees someone who they hate"},
	{bits_COND_SEE_FEAR, "Sees someone who they fear"},
	{bits_COND_SEE_DISLIKE, "Sees someone who they dislike"},
	{bits_COND_SEE_ENEMY, "Sees enemy"},
	{bits_COND_ENEMY_OCCLUDED, "Enemy is occluded"},
	{bits_COND_SMELL_FOOD, "Smells food"},
	{bits_COND_ENEMY_TOOFAR, "Enemy is too far"},
	{bits_COND_LIGHT_DAMAGE, "Got light damage"},
	{bits_COND_HEAVY_DAMAGE, "Got heavy damage"},
	{bits_COND_CAN_RANGE_ATTACK1, "Can Range Attack 1"},
	{bits_COND_CAN_MELEE_ATTACK1, "Can Melee Attack 1"},
	{bits_COND_CAN_RANGE_ATTACK2, "Can Range Attack 2"},
	{bits_COND_CAN_MELEE_ATTACK2, "Can Melee Attack 2"},
	{bits_COND_PROVOKED, "Provoked"},
	{bits_COND_NEW_ENEMY, "New enemy"},
	{bits_COND_HEAR_SOUND, "Hears sound"},
	{bits_COND_SMELL, "Smells something"},
	{bits_COND_ENEMY_FACING_ME, "Enemy is facing me"},
	{bits_COND_ENEMY_DEAD, "Enemy is dead"},
	{bits_COND_SEE_CLIENT, "Sees a player"},
	{bits_COND_SEE_NEMESIS, "Sees nemesis"},
	{bits_COND_ENEMY_LOST, "Enemy is lost"},
	{bits_COND_SCHEDULE_SUGGESTED, "Schedule is suggested"},
	{bits_COND_CLIENT_PUSH, "Player is trying to push me"},
	{bits_COND_NOFIRE, "Stop possible friendly fire"},
	{bits_COND_SPECIAL1, "Special condition 1"},
	{bits_COND_SPECIAL2, "Special conditlion 2"},
	{bits_COND_TASK_FAILED, "Task failed"},
	{bits_COND_SCHEDULE_DONE, "Schedule done"},
};

//=========================================================
// FScheduleValid - returns true as long as the current
// schedule is still the proper schedule to be executing,
// taking into account all conditions
//=========================================================
bool CBaseMonster::FScheduleValid()
{
	if( m_pSchedule == NULL )
	{
		// schedule is empty, and therefore not valid.
		return false;
	}

	if( HasConditions( bits_COND_SCHEDULE_DONE | bits_COND_TASK_FAILED ) )
	{
#if DEBUG
		if( HasConditions( bits_COND_TASK_FAILED ) && m_failSchedule == SCHED_NONE )
		{
			// fail! Send a visual indicator.
			ALERT( at_aiconsole, "Schedule: %s Failed\n", m_pSchedule->pName );

			Vector tmp = pev->origin;
			tmp.z = pev->absmax.z + 16;
			UTIL_Sparks( tmp );
		}
#endif // DEBUG
		// some task failed, or the schedule is done
		return false;
	}
	else if ( HasConditions( m_pSchedule->iInterruptMask ) )
	{
		if (ShouldReportAIChange(entindex()))
		{
			ALERT(at_console, "%s (%d): schedule %s has been interrupted at task #%d. Conditions: ",
				STRING(pev->classname), entindex(), m_pSchedule->pName, m_iScheduleIndex);
			const int conditions = m_afConditions & m_pSchedule->iInterruptMask;
			for (const auto& condition : g_ConditionsNames)
			{
				if (FBitSet(conditions, condition.first))
				{
					ALERT(at_console, "%s; ", condition.second);
				}
			}
			if (FBitSet(conditions, bits_COND_HEAR_SOUND|bits_COND_SMELL|bits_COND_SMELL_FOOD))
			{
				CSound *pSound = PBestSound();
				if (pSound)
				{
					const int soundMask = pSound->m_iType & m_pSchedule->iSoundMask;
					if (soundMask)
					{
						ALERT(at_console, "Sounds/smells: ");
						for (const auto& sound : g_SoundNames)
						{
							if (FBitSet(soundMask, sound.first))
							{
								ALERT(at_console, "%s; ", sound.second);
							}
						}
					}
				}
				ALERT(at_console, "\n");
			}
		}

		// some condition has interrupted the schedule
		taskFailReason = "interrupted";
		return false;
	}
	
	return true;
}

bool CBaseMonster::ShouldGetIdealState()
{
	// Don't get ideal state if you are supposed to be dead.
	if ( m_IdealMonsterState == MONSTERSTATE_DEAD )
		return false;

	// If I'm supposed to be in scripted state, but i'm not yet, do not allow
	// GetIdealState() to be called, because it doesn't know how to determine
	// that a NPC should be in SCRIPT state and will stomp it with some other state.
	if ( m_IdealMonsterState == MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_SCRIPT )
		return false;

	// conditions bits (excluding SCHEDULE_DONE) indicate interruption
	if ( m_afConditions && !HasConditions( bits_COND_SCHEDULE_DONE ) )
		return true;

	// schedule is done but schedule indicates it wants GetIdealState called
	// after successful completion (by setting bits_COND_SCHEDULE_DONE in iInterruptMask)
	if ( m_pSchedule && (m_pSchedule->iInterruptMask & bits_COND_SCHEDULE_DONE ) )
		return true;

	// in COMBAT state with no enemy (it died?)
	if ( ( m_MonsterState == MONSTERSTATE_COMBAT ) && ( m_hEnemy == 0 ) )
		return true;

	// Always get ideal state in Hunt state, to check if monster wants to stop hunting
	if ( m_MonsterState == MONSTERSTATE_HUNT )
		return true;

	// Got an enemy from somewhere else (e.g. from squad member) while in non-combat state
	if ( (m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT ||
		  m_MonsterState == MONSTERSTATE_HUNT) && m_hEnemy != 0 )
		return true;

	return false;
}

//=========================================================
// MaintainSchedule - does all the per-think schedule maintenance.
// ensures that the monster leaves this function with a valid
// schedule!
//=========================================================
extern cvar_t npc_run_task_instant;

void CBaseMonster::MaintainSchedule()
{
	Schedule_t *pNewSchedule;
	int i;

	// UNDONE: Tune/fix this 10... This is just here so infinite loops are impossible
	for( i = 0; i < 10; i++ )
	{
		if( m_pSchedule != NULL && TaskIsComplete() )
		{
			NextScheduledTask();
		}

		// validate existing schedule 
		if( !FScheduleValid() || m_MonsterState != m_IdealMonsterState )
		{
			// if we come into this block of code, the schedule is going to have to be changed.
			// if the previous schedule was interrupted by a condition, GetIdealState will be 
			// called. Else, a schedule finished normally.

			// Notify the monster that his schedule is changing
			ScheduleChange();

			if( ShouldGetIdealState() )
			{
				GetIdealState();
			}
			if( HasConditions( bits_COND_TASK_FAILED ) && m_MonsterState == m_IdealMonsterState )
			{
				// schedule was invalid because the current task failed to start or complete
				ALERT( at_aiconsole, "Schedule Failed at %d! (monster: %s, schedule: %s, reason: %s)\n", m_iScheduleIndex, STRING(pev->classname),
					   m_pSchedule ? m_pSchedule->pName : "unknown", taskFailReason ? taskFailReason : "unspecified" );

				if( m_failSchedule != SCHED_NONE )
					pNewSchedule = GetScheduleOfType( m_failSchedule );
				else
					pNewSchedule = GetScheduleOfType( SCHED_FAIL );

				ChangeSchedule( pNewSchedule, FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_ON_FAIL) );
			}
			else
			{
				SetState( m_IdealMonsterState );
				bool isSuggested = false;
				if( m_MonsterState == MONSTERSTATE_SCRIPT || m_MonsterState == MONSTERSTATE_DEAD )
					pNewSchedule = CBaseMonster::GetSchedule();
				else
				{
					pNewSchedule = GetSuggestedSchedule();
					if (pNewSchedule) {
						isSuggested = true;
					} else {
						pNewSchedule = GetSchedule();
					}
				}
				ChangeSchedule( pNewSchedule, isSuggested );
			}
		}

		if( m_iTaskStatus == TASKSTATUS_NEW )
		{	
			Task_t *pTask = GetTask();
			ASSERT( pTask != NULL );
			TaskBegin();
			StartTask( pTask );
		}

		// UNDONE: Twice?!!!
		if( m_Activity != m_IdealActivity )
		{
			SetActivity( m_IdealActivity );
		}

		if( !TaskIsComplete() && m_iTaskStatus != TASKSTATUS_NEW )
		{
			if ( npc_run_task_instant.value && TaskIsRunning() && !HasConditions( bits_COND_TASK_FAILED ) )
			{
				Task_t *pTask = GetTask();
				ASSERT( pTask != NULL );
				RunTask( pTask );

				if ( !TaskIsComplete() )
					break;
			}
			else
			{
				break;
			}
		}
	}

	if( !npc_run_task_instant.value && TaskIsRunning() )
	{
		Task_t *pTask = GetTask();
		ASSERT( pTask != NULL );
		RunTask( pTask );
	}

	// UNDONE: We have to do this so that we have an animation set to blend to if RunTask changes the animation
	// RunTask() will always change animations at the end of a script!
	// Don't do this twice
	if( m_Activity != m_IdealActivity )
	{
		SetActivity( m_IdealActivity );
	}
}

extern cvar_t anim_attack_reset_fix;

//=========================================================
// RunTask 
//=========================================================
void CBaseMonster::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_TURN_RIGHT:
	case TASK_TURN_LEFT:
		{
			ChangeYaw( pev->yaw_speed );

			if( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
		{
			CBaseEntity *pTarget;

			if( pTask->iTask == TASK_PLAY_SEQUENCE_FACE_TARGET )
				pTarget = m_hTargetEnt;
			else
				pTarget = m_hEnemy;
			if( pTarget )
			{
				pev->ideal_yaw = UTIL_VecToYaw( pTarget->pev->origin - pev->origin );
				ChangeYaw( pev->yaw_speed );
			}
			if( m_fSequenceFinished )
				TaskComplete();
		}
		break;
	case TASK_PLAY_SEQUENCE:
	case TASK_PLAY_ACTIVE_IDLE:
		{
			if( m_fSequenceFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_FACE_ENEMY:
		{
			MakeIdealYaw( m_vecEnemyLKP );

			ChangeYaw( pev->yaw_speed );

			if( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_FACE_HINTNODE:
	case TASK_FACE_LASTPOSITION:
	case TASK_FACE_TARGET:
	case TASK_FACE_IDEAL:
	case TASK_FACE_ROUTE:
	case TASK_FACE_SCHEDULED:
		{
			ChangeYaw( pev->yaw_speed );

			if( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_PVS:
		{
			if( FBitSet(pev->spawnflags, SF_MONSTER_ACT_OUT_OF_PVS) || !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_INDEFINITE:
		{
			// don't do anything.
			break;
		}
	case TASK_WAIT:
	case TASK_WAIT_RANDOM:
	case TASK_WAIT_PATROL_TURNING:
		{
			if (pTask->iTask == TASK_WAIT_PATROL_TURNING)
			{
				ChangeYaw( pev->yaw_speed );
			}
			if( gpGlobals->time >= m_flWaitFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_FACE_ENEMY:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed ); 

			if( gpGlobals->time >= m_flWaitFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_CATCH_WITH_TARGET_RANGE:
	case TASK_MOVE_NEAREST_TO_TARGET_RANGE:
		{
			float distance;

			if( m_hTargetEnt == 0 )
				TaskFail("no target ent");
			else
			{
				float checkDistance = pTask->flData;
				if ((m_Route[m_iRouteIndex].iType & bits_MF_NEAREST_PATH))
				{
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					checkDistance = checkDistance/2;
				}
				else
				{
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					// Re-evaluate when you think your finished, or the target has moved too far
					if( ( distance < checkDistance ) || ( m_vecMoveGoal - m_hTargetEnt->pev->origin ).IsLengthGreaterThan(pTask->flData * 0.5f) )
					{
						m_vecMoveGoal = m_hTargetEnt->pev->origin;
						distance = ( m_vecMoveGoal - pev->origin ).Length2D();
						FRefreshRoute();
					}
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if( distance < checkDistance )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else if ( pTask->iTask == TASK_CATCH_WITH_TARGET_RANGE && m_hTargetEnt->IsMoving() )
					m_movementActivity = ACT_RUN;
				else if( distance < 190 && m_movementActivity != ACT_WALK )
					m_movementActivity = ACT_WALK;
				else if( distance >= 270 && m_movementActivity != ACT_RUN )
					m_movementActivity = ACT_RUN;
			}
			break;
		}
	case TASK_WAIT_FOR_MOVEMENT:
		{
			if( MovementIsComplete() )
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
			break;
		}
	case TASK_DIE:
		{
			if( m_fSequenceFinished && pev->frame >= 255 )
			{
				pev->deadflag = DEAD_DEAD;

				SetThink( NULL );
				StopAnimation();

				if (FBitSet(pev->spawnflags, SF_MONSTER_NONSOLID_CORPSE))
				{
					pev->solid = SOLID_NOT;
					UTIL_SetSize(pev, Vector(0,0,0), Vector(0,0,0));
				}
				else if( !BBoxFlat() )
				{
					// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that it can be attacked will
					// block the player on a slope or stairs, the corpse is made nonsolid. 
					//pev->solid = SOLID_NOT;
					UTIL_SetSize( pev, Vector( -4, -4, 0 ), Vector( 4, 4, 1 ) );
				}
				else // !!!HACKHACK - put monster in a thin, wide bounding box until we fix the solid type/bounding volume problem
					UTIL_SetSize( pev, Vector( pev->mins.x, pev->mins.y, pev->mins.z ), Vector( pev->maxs.x, pev->maxs.y, pev->mins.z + 1 ) );

				if( ShouldFadeOnDeath() )
				{
					// this monster was created by a monstermaker... fade the corpse out.
					SUB_StartFadeOut();
				}
				else
				{
					// body is gonna be around for a while, so have it stink for a bit.
					if (BloodColor() != DONT_BLEED)
						InsertAISound( bits_SOUND_CARCASS, 384, 30 );
				}
			}
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK2_NOTURN:
	case TASK_RANGE_ATTACK2_NOTURN:
	case TASK_RELOAD_NOTURN:
		{
			if( m_fSequenceFinished )
			{
				m_Activity = ACT_RESET;
				if (anim_attack_reset_fix.value)
					m_IdealActivity = ACT_RESET;
				TaskComplete();
			}
			break;
		}
	case TASK_RANGE_ATTACK1:
	case TASK_MELEE_ATTACK1:
	case TASK_MELEE_ATTACK2:
	case TASK_RANGE_ATTACK2:
	case TASK_SPECIAL_ATTACK1:
	case TASK_SPECIAL_ATTACK2:
	case TASK_RELOAD:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if( m_fSequenceFinished )
			{
				m_Activity = ACT_RESET;
				if (anim_attack_reset_fix.value)
					m_IdealActivity = ACT_RESET;
				TaskComplete();
			}
			break;
		}
	case TASK_SMALL_FLINCH:
		{
			if( m_fSequenceFinished )
			{
				TaskComplete();
			}
		}
		break;
	case TASK_WAIT_FOR_SCRIPT:
		{
			if (m_pCine)
			{
				if( m_pCine->m_iDelay <= 0 && gpGlobals->time >= m_pCine->m_startTime )
				{
					TaskComplete();
					bool startedSequence = false;
					if (m_pCine->IsAction())
					{
						bool tryFireOnAnimStart = true;
						switch( m_pCine->m_fAction )
						{
						case SCRIPT_ACT_RANGE_ATTACK:
							m_IdealActivity = ACT_RANGE_ATTACK1;
							break;
						case SCRIPT_ACT_RANGE_ATTACK2:
							m_IdealActivity = ACT_RANGE_ATTACK2;
							break;
						case SCRIPT_ACT_MELEE_ATTACK:
							m_IdealActivity = ACT_MELEE_ATTACK1;
							break;
						case SCRIPT_ACT_MELEE_ATTACK2:
							m_IdealActivity = ACT_MELEE_ATTACK2;
							break;
						case SCRIPT_ACT_SPECIAL_ATTACK:
							m_IdealActivity = ACT_SPECIAL_ATTACK1;
							break;
						case SCRIPT_ACT_SPECIAL_ATTACK2:
							m_IdealActivity = ACT_SPECIAL_ATTACK2;
							break;
						case SCRIPT_ACT_RELOAD:
							m_IdealActivity = ACT_RELOAD;
							break;
						case SCRIPT_ACT_JUMP:
							m_IdealActivity = ACT_HOP;
							break;
						default:
							tryFireOnAnimStart = false;
							break;
						}
						if (tryFireOnAnimStart)
							m_pCine->FireOnAnimStart(this);
					}
					else
					{
						startedSequence = m_pCine->StartSequence( this, m_pCine->m_iszPlay, true );
						if( m_fSequenceFinished )
							ClearSchedule();
					}
					if (!startedSequence)
						pev->framerate = 1.0f; // TODO: not sure if this is needed at all. Just preserving the original behavior
					//ALERT( at_aiconsole, "Script %s has begun for %s\n", STRING( m_pCine->m_iszPlay ), STRING( pev->classname ) );
				}
				else if ( FBitSet(m_pCine->pev->spawnflags, SF_SCRIPT_FORCE_IDLE_LOOPING) && !FStringNull( m_pCine->m_iszIdle) && !m_pCine->IsAction() )
				{
					if ( m_fSequenceFinished )
						m_pCine->StartSequence( this, m_pCine->m_iszIdle, false );
				}
			}
			break;
		}
	case TASK_PLAY_SCRIPT:
		{
			if( m_fSequenceFinished )
			{
				if (m_pCine)
				{
					if( m_pCine->m_iRepeatsLeft > 0 )
					{
						m_pCine->m_iRepeatsLeft--;
						pev->frame = m_pCine->m_fRepeatFrame;
						ResetSequenceInfo();
					}
					else
					{
						m_pCine->SequenceDone( this );
					}
				}
				else
					TaskComplete();
			}
			break;
		}
	case TASK_RUN_TO_SCRIPT_RADIUS:
	case TASK_WALK_TO_SCRIPT_RADIUS:
		{
			CBaseEntity* pGoalEnt = ScriptedMoveGoal();
			if( pGoalEnt == 0 )
				TaskFail("no target ent");
			else
			{
				float checkDistance = pTask->flData;
				if (m_pCine != 0 && m_pCine->m_flMoveToRadius >= 1.0f)
				{
					checkDistance = m_pCine->m_flMoveToRadius;
				}

				float distance = ( pGoalEnt->pev->origin - pev->origin ).Length2D();

				if( distance <= checkDistance )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else if (MovementIsComplete())
				{
					TaskFail("completed movement before reaching the radius");
					RouteClear();
				}
				else if ( pTask->iTask == TASK_RUN_TO_SCRIPT_RADIUS )
					m_movementActivity = ACT_RUN;
				else
					m_movementActivity = ACT_WALK;
			}
			break;
		}
	}
}

//=========================================================
// SetTurnActivity - measures the difference between the way
// the monster is facing and determines whether or not to
// select one of the 180 turn animations.
//=========================================================
void CBaseMonster::SetTurnActivity()
{
	float flYD;
	flYD = FlYawDiff();

	if( flYD <= -45 && LookupActivity( ACT_TURN_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// big right turn
		m_IdealActivity = ACT_TURN_RIGHT;
	}
	else if( flYD > 45 && LookupActivity( ACT_TURN_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// big left turn
		m_IdealActivity = ACT_TURN_LEFT;
	}
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//=========================================================
void CBaseMonster::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_TURN_RIGHT:
		{
			float flCurrentYaw;

			flCurrentYaw = UTIL_AngleMod( pev->angles.y );
			pev->ideal_yaw = UTIL_AngleMod( flCurrentYaw - pTask->flData );
			SetTurnActivity();
			break;
		}
	case TASK_TURN_LEFT:
		{
			float flCurrentYaw;

			flCurrentYaw = UTIL_AngleMod( pev->angles.y );
			pev->ideal_yaw = UTIL_AngleMod( flCurrentYaw + pTask->flData );
			SetTurnActivity();
			break;
		}
	case TASK_REMEMBER:
		{
			Remember( (int)pTask->flData );
			TaskComplete();
			break;
		}
	case TASK_FORGET:
		{
			Forget( (int)pTask->flData );
			TaskComplete();
			break;
		}
	case TASK_FIND_HINTNODE:
		{
			m_iHintNode = FindHintNode();

			if( m_iHintNode != NO_NODE )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("could not find hintnode");
			}
			break;
		}
	case TASK_STORE_LASTPOSITION:
		{
			m_vecLastPosition = pev->origin;
			TaskComplete();
			break;
		}
	case TASK_CLEAR_LASTPOSITION:
		{
			m_vecLastPosition = g_vecZero;
			TaskComplete();
			break;
		}
	case TASK_CLEAR_HINTNODE:
		{
			m_iHintNode = NO_NODE;
			TaskComplete();
			break;
		}
	case TASK_STOP_MOVING:
		{
			if( m_IdealActivity == m_movementActivity )
			{
				m_IdealActivity = GetStoppedActivity();
			}

			RouteClear();
			TaskComplete();
			break;
		}
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
	case TASK_PLAY_SEQUENCE:
		{
			m_IdealActivity = (Activity)(int)pTask->flData;
			break;
		}
	case TASK_PLAY_ACTIVE_IDLE:
		{
			// monsters verify that they have a sequence for the node's activity BEFORE
			// moving towards the node, so it's ok to just set the activity without checking here.
			m_IdealActivity = (Activity)WorldGraph.m_pNodes[m_iHintNode].m_sHintActivity;
			break;
		}
	case TASK_SET_SCHEDULE:
		{
			Schedule_t *pNewSchedule;

			pNewSchedule = GetScheduleOfType( (int)pTask->flData );

			if( pNewSchedule )
			{
				ChangeSchedule( pNewSchedule );
			}
			else
			{
				TaskFail("no schedule of specified type");
			}
			break;
		}
	case TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY:
		{
			if( m_hEnemy == 0 )
			{
				TaskFail("no enemy");
				return;
			}

			if( FindCover( m_hEnemy->pev->origin, m_hEnemy->pev->view_ofs, 0, pTask->flData ) )
			{
				// try for cover farther than the FLData from the schedule.
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever.
				TaskFail("no cover found");
			}
			break;
		}
	case TASK_FIND_FAR_NODE_COVER_FROM_ENEMY:
		{
			if( m_hEnemy == 0 )
			{
				TaskFail("no enemy");
				return;
			}

			if( FindCover( m_hEnemy->pev->origin, m_hEnemy->pev->view_ofs, pTask->flData, CoverRadius() ) )
			{
				// try for cover farther than the FLData from the schedule.
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever.
				TaskFail("no cover found");
			}
			break;
		}
	case TASK_FIND_NODE_COVER_FROM_ENEMY:
		{
			if( m_hEnemy == 0 )
			{
				TaskFail("no enemy");
				return;
			}

			if( FindCover( m_hEnemy->pev->origin, m_hEnemy->pev->view_ofs, 0, CoverRadius() ) )
			{
				// try for cover farther than the FLData from the schedule.
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever.
				TaskFail("no cover found");
			}
			break;
		}
	case TASK_FIND_COVER_FROM_ENEMY:
		{
			entvars_t *pevCover;

			if( m_hEnemy == 0 )
			{
				// Find cover from self if no enemy available
				pevCover = pev;
				//TaskFail();
				//return;
			}
			else
				pevCover = m_hEnemy->pev;

			if( FindLateralCover( pevCover->origin, pevCover->view_ofs ) )
			{
				// try lateral first
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else if( FindCover( pevCover->origin, pevCover->view_ofs, 0, CoverRadius() ) )
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever.
				TaskFail("no cover found");
			}
			break;
		}
	case TASK_FIND_COVER_FROM_ORIGIN:
		{
			if( FindCover( pev->origin, pev->view_ofs, 0, CoverRadius() ) )
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else
			{
				// no cover!
				TaskFail("no cover found");
			}
		}
		break;
	case TASK_FIND_COVER_FROM_SPOT:
		{
			Vector vecSpot;
			Vector viewOffset;
			if (CalcSuggestedSpot(&vecSpot, &viewOffset))
			{
				const int moveFlag = FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_WALK) ? FINDSPOTAWAY_WALK : FINDSPOTAWAY_RUN;
				if( FindLateralCover( vecSpot, viewOffset, SuggestedMinDist(COVER_DELTA), SuggestedMaxDist(COVER_DELTA * COVER_CHECKS), moveFlag|FINDSPOTAWAY_CHECK_SPOT ) )
				{
					m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
					TaskComplete();
				}
				else if( FindCover( vecSpot, viewOffset, SuggestedMinDist(0), SuggestedMaxDist(CoverRadius()), moveFlag|FINDSPOTAWAY_CHECK_SPOT ) )
				{
					m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
					TaskComplete();
				}
				else if ( FindSpotAway( vecSpot, SuggestedMinDist(64), SuggestedMaxDist(CoverRadius()), moveFlag ) )
				{
					m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
					TaskComplete();
				}
				else
				{
					TaskFail("no cover found");
				}
			}
			else
			{
				TaskFail("no valid spot");
			}
		}
		break;
	case TASK_FIND_COVER_FROM_BEST_SOUND:
		{
			CSound *pBestSound = PBestSound();

			ASSERT( pBestSound != NULL );
			/*
			if( pBestSound && FindLateralCover( pBestSound->m_vecOrigin, g_vecZero ) )
			{
				// try lateral first
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			*/

			if( pBestSound )
			{
				if (FindCover( pBestSound->m_vecOrigin, g_vecZero, pBestSound->m_iVolume, CoverRadius() ))
				{
					// then try for plain ole cover
					m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
					TaskComplete();
				}

				// The point is to just run away from danger. Try to find a node without actual cover.
				else if (FindSpotAway( pBestSound->m_vecOrigin, pBestSound->m_iVolume, CoverRadius(), FINDSPOTAWAY_CHECK_SPOT|FINDSPOTAWAY_RUN ))
				{
					//ALERT(at_aiconsole, "Using run away\n");
					m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
					TaskComplete();
				}
				// The last resort, just run straight away
				else
				{
					Vector dir = pev->origin - pBestSound->m_vecOrigin;
					float distance = dir.Length();
					distance = Q_max(pBestSound->m_iVolume - distance, 384);
					dir.z = 0;
					Vector targetLocation = pev->origin + dir.Normalize() * distance;

					if( MoveToLocation( ACT_RUN, 0, targetLocation ) )
					{
						//ALERT(at_aiconsole, "Using the last resort to run away\n");
						TaskComplete();
					}
				}
				// no coverwhatsoever. or no sound in list
				if (!TaskIsComplete())
					TaskFail("no cover found");
			}
			else
			{
				TaskFail("no sound to cover from");
			}

			break;
		}
	case TASK_FACE_HINTNODE:
		{
			pev->ideal_yaw = WorldGraph.m_pNodes[m_iHintNode].m_flHintYaw;
			SetTurnActivity();
			break;
		}
	case TASK_FACE_LASTPOSITION:
		MakeIdealYaw( m_vecLastPosition );
		SetTurnActivity(); 
		break;
	case TASK_FACE_TARGET:
		if( m_hTargetEnt != 0 )
		{
			MakeIdealYaw( m_hTargetEnt->pev->origin );
			SetTurnActivity(); 
		}
		else
			TaskFail("no target ent");
		break;
	case TASK_FACE_ENEMY:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			SetTurnActivity();
			break;
		}
	case TASK_FACE_SCHEDULED:
		{
			Vector vecSpot;
			if (CalcSuggestedSpot(&vecSpot))
			{
				MakeIdealYaw(vecSpot);
			}
			SetTurnActivity();
			break;
		}
	case TASK_FACE_IDEAL:
		{
			SetTurnActivity();
			break;
		}
	case TASK_FACE_ROUTE:
		{
			if( FRouteClear() )
			{
				TaskFail("no route to face");
			}
			else
			{
				MakeIdealYaw( m_Route[m_iRouteIndex].vecLocation );
				SetTurnActivity();
			}
			break;
		}
	case TASK_WAIT_PVS:
	case TASK_WAIT_INDEFINITE:
		{
			// don't do anything.
			break;
		}
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
		{
			// set a future time that tells us when the wait is over.
			m_flWaitFinished = gpGlobals->time + pTask->flData;	
			break;
		}
	case TASK_WAIT_PATROL_TURNING:
		{
			m_flWaitFinished = m_nextPatrolPathCheck;
			break;
		}
	case TASK_WAIT_RANDOM:
		{
			// set a future time that tells us when the wait is over.
			m_flWaitFinished = gpGlobals->time + RANDOM_FLOAT( 0.1, pTask->flData );
			break;
		}
	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_CATCH_WITH_TARGET_RANGE:
	case TASK_MOVE_NEAREST_TO_TARGET_RANGE:
		{
			if ( m_hTargetEnt == 0 )
				TaskFail("no target ent");
			else if( ( m_hTargetEnt->pev->origin - pev->origin ).IsLengthLessThan(1) )
				TaskComplete();
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				const bool closest = pTask->iTask == TASK_MOVE_NEAREST_TO_TARGET_RANGE && NpcFollowNearest();
				if( !MoveToTarget( pTask->iTask == TASK_CATCH_WITH_TARGET_RANGE ? ACT_RUN : ACT_WALK, 2, closest ) )
					TaskFail("failed to reach target ent");
			}
			break;
		}
	case TASK_RUN_TO_SCRIPT:
	case TASK_WALK_TO_SCRIPT:
		{
			Activity newActivity;
			CBaseEntity* pGoalEnt = ScriptedMoveGoal();

			if ( pGoalEnt == 0 )
				TaskFail("no move target ent");
			else if( ( pGoalEnt->pev->origin - pev->origin ).IsLengthLessThan(1) )
				TaskComplete();
			else
			{
				if( pTask->iTask == TASK_WALK_TO_SCRIPT )
					newActivity = ACT_WALK;
				else
					newActivity = ACT_RUN;

				// This monster can't do this!
				if( LookupActivity( newActivity ) == ACTIVITY_NOT_AVAILABLE )
					TaskComplete();
				else 
				{
					if (pGoalEnt != 0)
					{
						const Vector vecDest = pGoalEnt->pev->origin;
						if( !MoveToLocation( newActivity, 2, vecDest ) )
						{
							if (m_pCine) {
								m_pCine->OnMoveFail();
							}
							TaskFail("failed to reach script");
							RouteClear();
						}
					}
					else
					{
						TaskFail("no move target ent");
						RouteClear();
					}
				}
			}
			TaskComplete();
			break;
		}
	case TASK_RUN_TO_SCRIPT_RADIUS:
	case TASK_WALK_TO_SCRIPT_RADIUS:
		{
			float radius = pTask->flData;
			if (m_pCine != 0 && m_pCine->m_flMoveToRadius >= 1.0f)
			{
				radius = m_pCine->m_flMoveToRadius;
			}
			if (radius < 1.0f)
				radius = 1.0f;

			Activity newActivity;
			CBaseEntity* pGoalEnt = ScriptedMoveGoal();

			if ( pGoalEnt == 0 )
				TaskFail("no move target ent");
			else if( ( pGoalEnt->pev->origin - pev->origin ).IsLength2DLessThanOrEqual(radius) )
				TaskComplete();
			else
			{
				if( pTask->iTask == TASK_RUN_TO_SCRIPT_RADIUS )
					newActivity = ACT_RUN;
				else
					newActivity = ACT_WALK;

				// This monster can't do this!
				if( LookupActivity( newActivity ) == ACTIVITY_NOT_AVAILABLE )
					TaskComplete();
				else
				{
					if( pGoalEnt == 0 || !MoveToLocationClosest( newActivity, 2, pGoalEnt->pev->origin ) )
					{
						HandleBlocker(CBaseEntity::Instance( gpGlobals->trace_ent ), false);
						TaskFail("failed to reach move target ent");
						RouteClear();
					}
				}
			}
			break;
		}
	case TASK_CLEAR_MOVE_WAIT:
		{
			m_flMoveWaitFinished = gpGlobals->time;
			TaskComplete();
			break;
		}
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK1:
		{
			m_IdealActivity = ACT_MELEE_ATTACK1;
			break;
		}
	case TASK_MELEE_ATTACK2_NOTURN:
	case TASK_MELEE_ATTACK2:
		{
			m_IdealActivity = ACT_MELEE_ATTACK2;
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
	case TASK_RANGE_ATTACK2_NOTURN:
	case TASK_RANGE_ATTACK2:
		{
			m_IdealActivity = ACT_RANGE_ATTACK2;
			break;
		}
	case TASK_RELOAD_NOTURN:
	case TASK_RELOAD:
		{
			m_IdealActivity = ACT_RELOAD;
			break;
		}
	case TASK_SPECIAL_ATTACK1:
		{
			m_IdealActivity = ACT_SPECIAL_ATTACK1;
			break;
		}
	case TASK_SPECIAL_ATTACK2:
		{
			m_IdealActivity = ACT_SPECIAL_ATTACK2;
			break;
		}
	case TASK_SET_ACTIVITY:
		{
			m_IdealActivity = (Activity)(int)pTask->flData;
			TaskComplete();
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			if( BuildRoute( m_vecEnemyLKP, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else if( BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, 0, ( m_vecEnemyLKP - pev->origin ).Length() ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to enemy last known position");
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if( pEnemy == NULL )
			{
				TaskFail("no enemy");
				return;
			}

			int routeFlags = bits_MF_TO_ENEMY;
			if (NpcGetToEnemyNearest() && WantsToGetCloseToEnemy() && !HasConditions(bits_COND_CAN_RANGE_ATTACK1|bits_COND_CAN_RANGE_ATTACK2))
			{
				routeFlags |= bits_MF_NEAREST_PATH;
			}
			if( BuildRoute( pEnemy->pev->origin, routeFlags, pEnemy ) )
			{
				TaskComplete();
			}
			else if( BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, 0, ( pEnemy->pev->origin - pev->origin ).Length() ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to enemy");
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_CORPSE:
		{
			UTIL_MakeVectors( pev->angles );
			if( BuildRoute( m_vecEnemyLKP - gpGlobals->v_forward * pTask->flData, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("can't build path to enemy corpse");
			}
		}
		break;
	case TASK_GET_PATH_TO_PLAYER:
		{
			CBaseEntity *pPlayer = CBaseEntity::Instance( FIND_ENTITY_BY_CLASSNAME( NULL, "player" ) );
			if( BuildRoute( m_vecMoveGoal, bits_MF_TO_LOCATION, pPlayer ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to player");
			}
			break;
		}
	case TASK_GET_PATH_TO_TARGET:
		{
			RouteClear();
			if( m_hTargetEnt != 0 && MoveToTarget( m_movementActivity, 1 ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("failed to reach target ent");
			}
			break;
		}
	case TASK_GET_PATH_TO_HINTNODE:
		// for active idles!
		{
			if( MoveToLocation( m_movementActivity, 2, WorldGraph.m_pNodes[m_iHintNode].m_vecOrigin ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to hintnode");
			}
			break;
		}
	case TASK_GET_PATH_TO_LASTPOSITION:
		{
			m_vecMoveGoal = m_vecLastPosition;

			if( MoveToLocation( m_movementActivity, 2, m_vecMoveGoal ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to last position");
			}
			break;
		}
	case TASK_GET_PATH_TO_BESTSOUND:
		{
			CSound *pSound = PBestSound();

			if(pSound)
			{
				if( MoveToLocation( m_movementActivity, 2, pSound->m_vecOrigin ) )
				{
					TaskComplete();
				}
				else if( BuildNearestRoute( pSound->m_vecOrigin, pev->view_ofs, 0, pSound->m_iVolume ) )
				{
					TaskComplete();
				}
				else
				{
					// no way to get there =(
					TaskFail("can't build path to best sound");
				}
			}
			else
			{
				TaskFail("no sound detected");
			}
			break;
		}
	case TASK_GET_PATH_TO_BESTSCENT:
		{
			CSound *pScent = PBestScent();

			if( pScent && MoveToLocation( m_movementActivity, 2, pScent->m_vecOrigin ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to best scent");
			}
			break;
		}
	case TASK_GET_PATH_TO_SPOT:
		{
			Vector vecSpot;
			if (CalcSuggestedSpot(&vecSpot))
			{
				UTIL_MakeVectors( pev->angles );
				if( BuildRoute( vecSpot - gpGlobals->v_forward * SuggestedMinDist(0), bits_MF_TO_LOCATION, NULL ) )
				{
					TaskComplete();
				}
				else
				{
					TaskFail("can't build path to spot");
				}
			}
			else
			{
				TaskFail("no valid spot");
			}
			break;
		}
	case TASK_RUN_PATH:
		{
			// UNDONE: This is in some default AI and some monsters can't run? -- walk instead?
			if( LookupActivity( ACT_RUN ) != ACTIVITY_NOT_AVAILABLE )
			{
				m_movementActivity = ACT_RUN;
			}
			else
			{
				m_movementActivity = ACT_WALK;
			}
			TaskComplete();
			break;
		}
	case TASK_WALK_PATH:
		{
			if( pev->movetype == MOVETYPE_FLY )
			{
				m_movementActivity = ACT_FLY;
			}
			if( LookupActivity( ACT_WALK ) != ACTIVITY_NOT_AVAILABLE )
			{
				m_movementActivity = ACT_WALK;
			}
			else
			{
				m_movementActivity = ACT_RUN;
			}
			TaskComplete();
			break;
		}
	case TASK_RUN_OR_WALK_PATH:
		{
			m_movementActivity = GetSuggestedMovementActivity(ACT_RUN);
			TaskComplete();
			break;
		}
	case TASK_WALK_OR_RUN_PATH:
		{
			m_movementActivity = GetSuggestedMovementActivity(ACT_WALK);
			TaskComplete();
			break;
		}
	case TASK_STRAFE_PATH:
		{
			Vector2D vec2DirToPoint; 
			Vector2D vec2RightSide;

			// to start strafing, we have to first figure out if the target is on the left side or right side
			UTIL_MakeVectors( pev->angles );

			vec2DirToPoint = ( m_Route[0].vecLocation - pev->origin ).Make2D().Normalize();
			vec2RightSide = gpGlobals->v_right.Make2D().Normalize();

			if( DotProduct ( vec2DirToPoint, vec2RightSide ) > 0 )
			{
				// strafe right
				m_movementActivity = ACT_STRAFE_RIGHT;
			}
			else
			{
				// strafe left
				m_movementActivity = ACT_STRAFE_LEFT;
			}
			TaskComplete();
			break;
		}
	case TASK_WAIT_FOR_MOVEMENT:
		{
			if( FRouteClear() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_EAT:
		{
			Eat( pTask->flData );
			TaskComplete();
			break;
		}
	case TASK_SMALL_FLINCH:
		{
			m_IdealActivity = GetSmallFlinchActivity();
			break;
		}
	case TASK_DIE:
		{
			RouteClear();

			m_IdealActivity = GetDeathActivity();

			pev->deadflag = DEAD_DYING;
			break;
		}
	case TASK_SOUND_WAKE:
		{
			AlertSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_DIE:
		{
			DeathSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_IDLE:
		{
			IdleSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_PAIN:
		{
			PainSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_DEATH:
		{
			DeathSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_ANGRY:
		{
			// sounds are complete as soon as we get here, cause we've already played them.
			ALERT( at_aiconsole, "SOUND\n" );			
			TaskComplete();
			break;
		}
	case TASK_WAIT_FOR_SCRIPT:
		{
			if( m_pCine && m_pCine->m_iszIdle && !m_pCine->IsAction() )
			{
				m_pCine->StartSequence( (CBaseMonster *)this, m_pCine->m_iszIdle, false );
				if( FStrEq( STRING( m_pCine->m_iszIdle ), STRING( m_pCine->m_iszPlay ) ) )
				{
					pev->framerate = 0;
				}
			}
			else
				m_IdealActivity = ACT_IDLE;
			break;
		}
	case TASK_PLAY_SCRIPT:
		{
			pev->movetype = MOVETYPE_FLY;
			ClearBits( pev->flags, FL_ONGROUND );
			m_scriptState = SCRIPT_PLAYING;
			break;
		}
	case TASK_ENABLE_SCRIPT:
		{
			if (m_pCine)
				m_pCine->DelayStart( 0 );
			TaskComplete();
			break;
		}
	case TASK_PLANT_ON_SCRIPT:
		{
			if (m_pCine != NULL)
			{
				if (m_pCine->m_fMoveTo == SCRIPT_MOVE_TELEPORT)
				{
					if (m_hTargetEnt != 0)
					{
						UTIL_SetOrigin( pev, m_hTargetEnt->pev->origin );
						if (m_pCine->m_fTurnType == SCRIPT_TURN_MATCH_ANGLE)
							pev->angles.y = m_hTargetEnt->pev->angles.y;
						else if (m_pCine->m_fTurnType == SCRIPT_TURN_FACE)
							pev->angles.y = UTIL_VecToYaw(m_hTargetEnt->pev->origin - pev->origin);
						pev->ideal_yaw = pev->angles.y;

						pev->avelocity = Vector( 0, 0, 0 );
						pev->velocity = Vector( 0, 0, 0 );
						pev->effects |= EF_NOINTERP;
					}
				}
			}
			CBaseEntity* pGoalEnt = ScriptedMoveGoal();
			if( pGoalEnt != 0 )
			{
				pev->origin = pGoalEnt->pev->origin;
			}

			TaskComplete();
			break;
		}
	case TASK_FORCED_PLANT_ON_SCRIPT:
		{
			if (m_pCine != NULL)
			{
				if (m_hTargetEnt != 0)
				{
					if (m_pCine->m_fTurnType == SCRIPT_TURN_MATCH_ANGLE)
						pev->angles.y = m_hTargetEnt->pev->angles.y;
					else if (m_pCine->m_fTurnType == SCRIPT_TURN_FACE)
						pev->angles.y = UTIL_VecToYaw(m_hTargetEnt->pev->origin - pev->origin);
					pev->ideal_yaw = pev->angles.y;

					pev->avelocity = Vector( 0, 0, 0 );
					pev->velocity = Vector( 0, 0, 0 );
					pev->effects |= EF_NOINTERP;
				}
				CBaseEntity* pGoalEnt = ScriptedMoveGoal();
				if( pGoalEnt != 0 )
				{
					ALERT(at_aiconsole, "Forcibly teleporting the monster to script after %d attempts\n", m_pCine->m_moveFailCount );
					UTIL_SetOrigin( pev, pGoalEnt->pev->origin );
				}
				m_pCine->m_moveFailCount = 0;
			}
			TaskComplete();
			break;
		}
	case TASK_FACE_SCRIPT:
		{
			if ( m_pCine != 0 && m_pCine->m_fMoveTo != SCRIPT_MOVE_NO)
			{
				switch (m_pCine->m_fTurnType)
				{
				case SCRIPT_TURN_MATCH_ANGLE:
					pev->ideal_yaw = UTIL_AngleMod( m_pCine->pev->angles.y );
					break;
				case SCRIPT_TURN_FACE:
					if (m_hTargetEnt)
						MakeIdealYaw ( m_hTargetEnt->pev->origin );
					else
						MakeIdealYaw ( m_pCine->pev->origin );
					break;
				}
			}

			TaskComplete();
			m_IdealActivity = ACT_IDLE;
			RouteClear();
			break;
		}
	case TASK_SUGGEST_STATE:
		{
			m_IdealMonsterState = (MONSTERSTATE)(int)pTask->flData;
			TaskComplete();
			break;
		}
	case TASK_SET_FAIL_SCHEDULE:
		m_failSchedule = (int)pTask->flData;
		TaskComplete();
		break;
	case TASK_CLEAR_FAIL_SCHEDULE:
		m_failSchedule = SCHED_NONE;
		TaskComplete();
		break;
	case TASK_GET_HEALTH_FROM_FOOD:
		if (g_modFeatures.monsters_eat_for_health)
		{
			const float prevHealth = pev->health;
			TakeHealth(this, pev->max_health * pTask->flData, HEAL_GENERIC);
			ALERT(at_aiconsole, "%s eating. Health before: %g/%g. Health after: %g/%g\n", STRING(pev->classname), prevHealth, pev->max_health, pev->health, pev->max_health);
		}
		TaskComplete();
		break;
	case TASK_GET_PATH_TO_FREEROAM_NODE:
		{
			if( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
			{
				TaskFail("graph not ready for freeroam");
			}
			else
			{
				for( int i = 0; i < WorldGraph.m_cNodes; i++ )
				{
					int nodeNumber = ( i + WorldGraph.m_iLastActiveIdleSearch ) % WorldGraph.m_cNodes;

					CNode &node = WorldGraph.Node( nodeNumber );

					// Don't go to the node if already is close enough
					if ((node.m_vecOrigin - pev->origin).IsLengthLessThan(16.0f))
						continue;

					TraceResult tr;
					UTIL_TraceLine( pev->origin + pev->view_ofs, node.m_vecOrigin + pev->view_ofs, dont_ignore_monsters, ENT( pev ), &tr );

					if (tr.flFraction == 1.0f && MoveToLocation( ACT_WALK, 2, node.m_vecOrigin ))
					{
						TaskComplete();
						WorldGraph.m_iLastActiveIdleSearch = nodeNumber + 1;
						break;
					}
				}
				if (!TaskIsComplete())
				{
					TaskFail("Could not find node to freeroam");
				}
			}
		}
		break;
	case TASK_FIND_SPOT_AWAY_FROM_ENEMY:
		{
			entvars_t *pevThreat;
			if( m_hEnemy == 0 )
			{
				pevThreat = pev;
			}
			else
				pevThreat = m_hEnemy->pev;

			if( FindLateralCover( pevThreat->origin, pevThreat->view_ofs ) )
			{
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else if( FindCover( pevThreat->origin, pevThreat->view_ofs, 0, CoverRadius() ) )
			{
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else if (FindSpotAway( pevThreat->origin, 128, CoverRadius(), FINDSPOTAWAY_CHECK_SPOT|FINDSPOTAWAY_RUN ))
			{
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else if( npc_lateral_retreat.value != 0 && FindLateralSpotAway( pevThreat->origin, SuggestedMinDist(COVER_DELTA), SuggestedMaxDist(COVER_DELTA * COVER_CHECKS), FINDSPOTAWAY_CHECK_SPOT|FINDSPOTAWAY_RUN ) )
			{
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else
			{
				TaskFail("no spot found");
			}
		}
		break;
	case TASK_FIND_SPOT_AWAY:
		{
			Vector vecSpot;
			if (CalcSuggestedSpot(&vecSpot))
			{
				int moveFlag = FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_RUN) ? FINDSPOTAWAY_RUN : FINDSPOTAWAY_WALK;
				if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_DONT_AVOID_THREAT_NODE))
					moveFlag |= FINDSPOTAWAY_DONT_AVOID_THREAT_NODE;

				fixed_vector<CBaseEntity*, 3> pBlockers;

				auto addBlocker = [this, &pBlockers](const char* context) {
					CBaseEntity* pBlocker = CBaseEntity::Instance(gpGlobals->trace_ent);
					if (pBlocker->entindex() != 0)
					{
						pBlockers.push_back(pBlocker);
						//ALERT(at_aiconsole, "%s:  %s blocker is %s\n", STRING(pev->classname), context, STRING(pBlocker->pev->classname));
					}
				};

				if (FindSpotAway( vecSpot, SuggestedMinDist(64), SuggestedMaxDist(784), moveFlag ))
				{
					m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
					TaskComplete();
				}
				else
				{
					addBlocker("FindSpotAway");
				}

				if (!TaskIsComplete())
				{
					if (FindStraightSpotAway( vecSpot, SuggestedMinDist(COVER_DELTA), SuggestedMaxDist(COVER_DELTA * COVER_CHECKS), moveFlag ))
					{
						m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
						TaskComplete();
					}
					else
					{
						addBlocker("FindStraightSpotAway");
					}
				}

				if (!TaskIsComplete())
				{
					if (FindLateralSpotAway( vecSpot, SuggestedMinDist(COVER_DELTA), SuggestedMaxDist(COVER_DELTA * COVER_CHECKS), moveFlag ))
					{
						m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
						TaskComplete();
					}
					else
					{
						addBlocker("FindLateralSpotAway");
					}
				}

				if (!TaskIsComplete()) {
					for (auto pBlocker : pBlockers)
					{
						if (HandleBlocker(pBlocker, false))
							break;
					}
					TaskFail("no spot found");
				}
			}
			else
			{
				TaskFail("no valid spot");
			}
		}
		break;
	default:
		{
			ALERT( at_aiconsole, "No StartTask entry for %d\n", (SHARED_TASKS)pTask->iTask );
			break;
		}
	}
}

//=========================================================
// GetTask - returns a pointer to the current 
// scheduled task. NULL if there's a problem.
//=========================================================
Task_t *CBaseMonster::GetTask()
{
	if( m_iScheduleIndex < 0 || m_iScheduleIndex >= m_pSchedule->cTasks )
	{
		// m_iScheduleIndex is not within valid range for the monster's current schedule.
		return NULL;
	}
	else
	{
		return &m_pSchedule->pTasklist[m_iScheduleIndex];
	}
}

Schedule_t* CBaseMonster::GetFreeroamSchedule()
{
	if (m_freeRoam == FREEROAM_ALWAYS)
		return GetScheduleOfType( SCHED_FREEROAM );
	else if (m_freeRoam == FREEROAM_MAPDEFAULT)
	{
		entvars_t *pevWorld = VARS( INDEXENT( 0 ) );
		if (pevWorld->spawnflags & SF_WORLD_FREEROAM) {
			return  GetScheduleOfType( SCHED_FREEROAM );
		}
	}
	return NULL;
}

Schedule_t* CBaseMonster::GetSuggestedSchedule()
{
	if (m_suggestedSchedule && IsFreeToManipulate()) {
		return GetScheduleOfType(m_suggestedSchedule);
	}
	return NULL;
}

bool CBaseMonster::SuggestSchedule(int schedule, CBaseEntity* spotEntity, float minDist, float maxDist, int flags)
{
	Vector pos;
	if (spotEntity) {
		if (spotEntity->CalcPosition(NULL, &pos)) {
			m_suggestedScheduleOrigin = pos;
		} else {
			if (FBitSet(flags, SUGGEST_SCHEDULE_FLAG_SPOT_IS_POSITION)) {
				ALERT(at_aiconsole, "SuggestSchedule: couldn't calc position for %s\n", STRING(spotEntity->pev->classname));
				return false;
			}

			flags |= SUGGEST_SCHEDULE_FLAG_SPOT_IS_INVALID;
		}
		flags |= SUGGEST_SCHEDULE_FLAG_SPOT_ENTITY_IS_PROVIDED;
	}

	m_suggestedSchedule = schedule;
	m_suggestedScheduleEntity = spotEntity;
	m_suggestedScheduleMinDist = minDist;
	m_suggestedScheduleMaxDist = maxDist;
	m_suggestedScheduleFlags = flags;
	SetConditions(bits_COND_SCHEDULE_SUGGESTED);
	return true;
}

float CBaseMonster::SuggestedMinDist(float defaultValue) const
{
	if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_PLAYING) && m_suggestedScheduleMinDist > 0)
		return m_suggestedScheduleMinDist;
	return defaultValue;
}

float CBaseMonster::SuggestedMaxDist(float defaultValue) const
{
	if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_PLAYING) && m_suggestedScheduleMaxDist > 0)
		return m_suggestedScheduleMaxDist;
	return defaultValue;
}

static bool CalcSuggestedSpotEntity(CBaseMonster* pMonster, CBaseEntity* pSpotEntity, Vector *outVec, Vector* viewOffset)
{
	if (pSpotEntity)
	{
		*outVec = pSpotEntity->pev->origin;
		if (viewOffset)
			*viewOffset = pSpotEntity->pev->view_ofs;
		ALERT(at_aiconsole, "%s picked %s as spot for suggested schedule\n", STRING(pMonster->pev->classname), STRING(pSpotEntity->pev->classname));
		return true;
	}
	return false;
}

bool CBaseMonster::CalcSuggestedSpot(Vector *outVec, Vector* viewOffset)
{
	if (viewOffset)
		*viewOffset = g_vecZero;
	if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_SPOT_ENTITY_IS_PROVIDED))
	{
		if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_SPOT_IS_ENTITY)) {
			return CalcSuggestedSpotEntity(this, m_suggestedScheduleEntity, outVec, viewOffset);
		} else if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_SPOT_IS_POSITION)) {
			*outVec = m_suggestedScheduleOrigin;
			return true;
		}

		if (CalcSuggestedSpotEntity(this, m_suggestedScheduleEntity, outVec, viewOffset)) {
			return true;
		} else if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_SPOT_IS_INVALID)) {
			return false;
		} else {
			*outVec = m_suggestedScheduleOrigin;
			return true;
		}
	}
	else
	{
		*outVec = pev->origin;
		if (viewOffset)
			*viewOffset = pev->view_ofs;
		return true;
	}
}

Activity CBaseMonster::GetSuggestedMovementActivity(Activity defaultActivity)
{
	Activity preferedActivity = defaultActivity;
	if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_WALK)) {
		preferedActivity = ACT_WALK;
	} else if (FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_RUN)) {
		preferedActivity = ACT_RUN;
	}

	if (LookupActivity( preferedActivity ) != ACTIVITY_NOT_AVAILABLE) {
		return preferedActivity;
	} else {
		return preferedActivity == ACT_WALK ? ACT_RUN : ACT_WALK; // last resort
	}
}

void CBaseMonster::ClearSuggestedSchedule()
{
	m_suggestedSchedule = SCHED_NONE;
	m_suggestedScheduleEntity = 0;
	m_suggestedScheduleOrigin = g_vecZero;
	m_suggestedScheduleMinDist = 0.0f;
	m_suggestedScheduleMaxDist = 0.0f;
	m_suggestedScheduleFlags = 0;
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBaseMonster::GetSchedule()
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_PRONE:
		{
			return GetScheduleOfType( SCHED_BARNACLE_VICTIM_GRAB );
			break;
		}
	case MONSTERSTATE_NONE:
		{
			ALERT( at_aiconsole, "MONSTERSTATE IS NONE!\n" );
			break;
		}
	case MONSTERSTATE_IDLE:
		{
			if( HasConditions( bits_COND_HEAR_SOUND ) )
			{
				return GetScheduleOfType( SCHED_ALERT_FACE );
			}
			else if( FRouteClear() )
			{
				if (m_pGoalEnt != 0 )
				{
					if (m_nextPatrolPathCheck <= gpGlobals->time)
					{
						Schedule_t* patrolSchedule = StartPatrol(m_pGoalEnt);
						if (patrolSchedule)
							return patrolSchedule;
					}
					else
					{
						return GetScheduleOfType( SCHED_IDLE_PATROL_TURNING );
					}
				}
				// no valid route!
				Schedule_t* freeroamSchedule = GetFreeroamSchedule();
				if (freeroamSchedule)
					return freeroamSchedule;
				return GetScheduleOfType( SCHED_IDLE_STAND );
			}
			else
			{
				// valid route. Get moving
				return GetScheduleOfType( SCHED_IDLE_WALK );
			}
			break;
		}
	case MONSTERSTATE_ALERT:
		{
			if( HasConditions( bits_COND_ENEMY_DEAD ) && LookupActivity( ACT_VICTORY_DANCE ) != ACTIVITY_NOT_AVAILABLE )
			{
				return GetScheduleOfType( SCHED_VICTORY_DANCE );
			}
			if ( HasConditions( bits_COND_ENEMY_LOST ) )
			{
				return GetScheduleOfType( SCHED_MOVE_TO_ENEMY_LKP );
			}

			if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			{
				if( fabs( FlYawDiff() ) < ( 1.0f - m_flFieldOfView ) * 60.0f ) // roughly in the correct direction
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN );
				}
				else
				{
					return GetScheduleOfType( SCHED_ALERT_SMALL_FLINCH );
				}
			}
			if (m_activeAfterCombat == ACTIVE_ALERT_ALWAYS || (m_activeAfterCombat == ACTIVE_ALERT_DEFAULT && NpcActiveAfterCombat()))
			{
				if( HasConditions ( bits_COND_HEAR_SOUND ) )
				{
					if (HasMemory(bits_MEMORY_ACTIVE_AFTER_COMBAT))
					{
						CSound *pSound = PBestSound();
						if (pSound)
						{
							const int type = pSound->m_iType;
							const bool isCombat = FBitSet(type, bits_SOUND_COMBAT);
							const bool isDanger = FBitSet(type, bits_SOUND_DANGER);
							const bool isPlayer = FBitSet(type, bits_SOUND_PLAYER);
							if (isCombat && // it's combat sound
									!isDanger && // but not danger
									( !isPlayer || IDefaultRelationship(CLASS_PLAYER) != R_AL )) // and it's not combat sound produced by ally player
							{
								ALERT(at_aiconsole, "%s trying to investigate sound after combat\n", STRING(pev->classname));
								return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
							}
						}
					}
					return GetScheduleOfType( SCHED_ALERT_FACE );
				}
				else if (HasMemory(bits_MEMORY_SHOULD_ROAM_IN_ALERT))
				{
					ALERT(at_aiconsole, "%s trying to freeroam after combat\n", STRING(pev->classname));
					Forget(bits_MEMORY_SHOULD_ROAM_IN_ALERT);
					return GetScheduleOfType( SCHED_FREEROAM_ALERT );
				}
			}

			Schedule_t* freeroamSchedule = GetFreeroamSchedule();
			if (freeroamSchedule)
				return freeroamSchedule;

			if( HasConditions ( bits_COND_HEAR_SOUND ) )
			{
				return GetScheduleOfType( SCHED_ALERT_FACE );
			}
			else
			{
				return GetScheduleOfType( SCHED_ALERT_STAND );
			}
			break;
		}
	case MONSTERSTATE_COMBAT:
		{
			if( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// clear the current (dead) enemy and try to find another.
				m_hEnemy = NULL;

				if( GetEnemy(true) )
				{
					ClearConditions( bits_COND_ENEMY_DEAD );
					return GetSchedule();
				}
				else
				{
					SetState( MONSTERSTATE_ALERT );
					return GetSchedule();
				}
			}

			if ( HasConditions( bits_COND_ENEMY_LOST ) )
			{
				ALERT(at_aiconsole, "%s did not see an enemy %s for a while. Just forget about it\n", STRING(pev->classname), m_hEnemy != 0 ? STRING(m_hEnemy->pev->classname) : "");
				m_hEnemy = NULL;

				if( GetEnemy(true) )
				{
					ClearConditions( bits_COND_ENEMY_LOST );
					return GetSchedule();
				}
				else
				{
					SetState( MONSTERSTATE_ALERT );
					return GetSchedule();
				}
			}

			if( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}
			else if( HasConditions( bits_COND_LIGHT_DAMAGE ) && !HasMemory( bits_MEMORY_FLINCHED ) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			else if( !HasConditions( bits_COND_SEE_ENEMY ) )
			{
				// we can't see the enemy
				if( !HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					// enemy is unseen, but not occluded!
					// turn to face enemy
					return GetScheduleOfType( SCHED_COMBAT_FACE );
				}
				else
				{
					// chase!
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
			}
			else  
			{
				// we can see the enemy
				if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				if( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
				}
				if( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
				{
					return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
				}
				if( !HasConditions( bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					// if we can see enemy but can't use either attack type, we must need to get closer to enemy
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
				else if( !FacingIdeal() )
				{
					//turn
					return GetScheduleOfType( SCHED_COMBAT_FACE );
				}
				else
				{
					ALERT( at_aiconsole, "No suitable combat schedule!\n" );
				}
			}
			break;
		}
	case MONSTERSTATE_DEAD:
		{
			return GetScheduleOfType( SCHED_DIE );
			break;
		}
	case MONSTERSTATE_SCRIPT:
		{
			if( !m_pCine )
			{
				ALERT( at_aiconsole, "Script failed for %s\n", STRING( pev->classname ) );
				CineCleanup();
				return GetScheduleOfType( SCHED_IDLE_STAND );
			}

			return GetScheduleOfType( SCHED_AISCRIPT );
		}
	case MONSTERSTATE_HUNT:
		{
			if( HasConditions ( bits_COND_HEAR_SOUND ) )
			{
				CSound *pSound = PBestSound();
				if (pSound)
				{
					const int type = pSound->m_iType;
					const bool isCombat = FBitSet(type, bits_SOUND_COMBAT);
					const bool isDanger = FBitSet(type, bits_SOUND_DANGER);
					const bool isPlayer = FBitSet(type, bits_SOUND_PLAYER);
					if (isCombat && // it's combat sound
							!isDanger && // but not danger
							( !isPlayer || IDefaultRelationship(CLASS_PLAYER) != R_AL )) // and it's not combat sound produced by ally player
					{
						return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
					}
				}
			}
			if (HasMemory(bits_MEMORY_SHOULD_GO_TO_LKP))
			{
				Forget(bits_MEMORY_SHOULD_GO_TO_LKP);
				return GetScheduleOfType( SCHED_MOVE_TO_ENEMY_LKP );
			}
			return GetScheduleOfType( SCHED_FREEROAM_ALERT );
		}
	default:
		{
			ALERT( at_aiconsole, "Invalid State for GetSchedule!\n" );
			break;
		}
	}

	return &slError[0];
}
