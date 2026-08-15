#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "monsters.h"
#include "animation.h"
#include "saverestore.h"
#include "followingmonster.h"
#include "scripted.h"
#include "soundent.h"
#include "gamerules.h"
#include "player.h"

float CFollowingMonster::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once

TYPEDESCRIPTION	CFollowingMonster::m_SaveData[] =
{
	DEFINE_FIELD( CFollowingMonster, m_followFailPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CFollowingMonster, m_followagePolicy, FIELD_SHORT ),
	DEFINE_FIELD( CFollowingMonster, m_cClipSize, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFollowingMonster, CSquadMonster )

Task_t tlFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_FOLLOW_FAILED },
	{ TASK_MOVE_NEAREST_TO_TARGET_RANGE, (float)128.0f },	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE_CHECK_JUMP },
};

Schedule_t slFollow[] =
{
	{
		tlFollow,
		ARRAYSIZE( tlFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t tlFollowCautious[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_FOLLOW_FAILED },
	{ TASK_MOVE_NEAREST_TO_TARGET_RANGE, (float)128.0f },	// Move within 128 of target ent (client)
};

Schedule_t slFollowCautious[] =
{
	{
		tlFollowCautious,
		ARRAYSIZE( tlFollowCautious ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow (Cautious)"
	},
};

Task_t tlFollowTargetNearest[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_FOLLOW_NEAREST_FAILED },
	{ TASK_GET_NEAREST_PATH_TO_TARGET, 64.0f },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slFollowTargetNearest[] =
{
	{
		tlFollowTargetNearest,
		ARRAYSIZE( tlFollowTargetNearest ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FollowTargetNearest"
	},
};

Task_t tlFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t slFaceTarget[] =
{
	{
		tlFaceTarget,
		ARRAYSIZE( tlFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t tlMoveAway[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_MOVE_AWAY_FAIL },
	{ TASK_STORE_LASTPOSITION, 0.0f },
	{ TASK_MOVE_AWAY_PATH, 100.0f },
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_PLAYER, 0.5f },
};

Schedule_t slMoveAway[] =
{
	{
		tlMoveAway,
		ARRAYSIZE( tlMoveAway ),
		bits_COND_NEW_ENEMY|
		bits_COND_HEAR_SOUND|
		bits_COND_SCHEDULE_SUGGESTED|
		bits_COND_LIGHT_DAMAGE|
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"MoveAway"
	},
};

Task_t tlMoveAwayFail[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_PLAYER, 0.5f },
};

Schedule_t slMoveAwayFail[] =
{
	{
		tlMoveAwayFail,
		ARRAYSIZE( tlMoveAwayFail ),
		bits_COND_NEW_ENEMY|
		bits_COND_HEAR_SOUND|
		bits_COND_SCHEDULE_SUGGESTED|
		bits_COND_LIGHT_DAMAGE|
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"MoveAwayFail"
	},
};

Task_t tlMoveAwayFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_REACHED },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_MOVE_AWAY_PATH, (float)100 },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_REACHED },
};

Schedule_t slMoveAwayFollow[] =
{
	{
		tlMoveAwayFollow,
		ARRAYSIZE( tlMoveAwayFollow ),
		bits_COND_NEW_ENEMY|
		bits_COND_HEAR_SOUND|
		bits_COND_SCHEDULE_SUGGESTED|
		bits_COND_LIGHT_DAMAGE|
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"MoveAwayFollow"
	},
};

Task_t tlStopFollowing[] =
{
	{ TASK_CANT_FOLLOW, 0.0f },
};

Schedule_t slStopFollowing[] =
{
	{
		tlStopFollowing,
		ARRAYSIZE( tlStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};

// Like regular fail, but without waiting for PVS
Task_t tlPVSIndependentFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 },
};

Schedule_t slPVSIndependentFail[] =
{
	{
		tlPVSIndependentFail,
		ARRAYSIZE( tlPVSIndependentFail ),
		bits_COND_CAN_ATTACK,
		0,
		"PVS Independent Fail"
	},
};

Task_t tlJumpFollowing[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_JUMP, (float)0 },
	{ TASK_FALL_TO_GROUND_FOLLOWING, (float)0 },
};

Schedule_t slJumpFollowing[] =
{
	{
		tlJumpFollowing,
		ARRAYSIZE( tlJumpFollowing ),
		0,
		0,
		"JumpFollowing"
	},
};

DEFINE_CUSTOM_SCHEDULES( CFollowingMonster )
{
	slFollow,
	slFollowTargetNearest,
	slFollowCautious,
	slFaceTarget,
	slMoveAway,
	slMoveAwayFollow,
	slMoveAwayFail,
	slStopFollowing,
	slPVSIndependentFail,
	slJumpFollowing,
};

IMPLEMENT_CUSTOM_SCHEDULES( CFollowingMonster, CSquadMonster )

void CFollowingMonster::FollowerTouch( CBaseEntity *pOther )
{
	// Did the player touch me?
	if( pOther->IsPlayer() )
	{
		if( !CanBeMadeMoveAway(pOther) )
			return;

		// Heuristic for determining if the player is pushing me away
		float speed = fabs( pOther->pev->velocity.x ) + fabs( pOther->pev->velocity.y );
		if( speed > 50.0f )
		{
			SetConditions( bits_COND_CLIENT_PUSH );
			if ( m_MonsterState != MONSTERSTATE_SCRIPT )
				MakeIdealYaw( pOther->pev->origin );
		}
	}
}

void CFollowingMonster::OnDying(bool gibbed, CBaseEntity* pKiller)
{
	ClearFollowedPlayer();
	SetUse( NULL );
	CSquadMonster::OnDying(gibbed, pKiller);
}

int CFollowingMonster::ObjectCaps()
{
	int caps = CSquadMonster::ObjectCaps();
	if ((m_afCapability & bits_CAP_USABLE) && IsFullyAlive())
	{
		caps |= FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE;
	}
	return caps;
}

void CFollowingMonster::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "followfailpolicy" ) )
	{
		m_followFailPolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "followage_policy" ) )
	{
		m_followagePolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CSquadMonster::KeyValue( pkvd );
}

Schedule_t *CFollowingMonster::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_MOVE_AWAY:
		return slMoveAway;
	case SCHED_MOVE_AWAY_FOLLOW:
		return slMoveAwayFollow;
	case SCHED_MOVE_AWAY_FAIL:
	{
		MakeMyBlockerMoveAway();
		return slMoveAwayFail;
	}
	case SCHED_TARGET_FACE:
	case SCHED_TARGET_REACHED:
		return slFaceTarget;
	case SCHED_TARGET_FACE_CHECK_JUMP:
	{
		if (NpcFollowNearest() && m_hTargetEnt != 0 && CanJumpFreely() && (m_hTargetEnt->pev->origin - pev->origin).IsLength2DGreaterThanOrEqual(128.0f))
		{
			if (m_flNextJump <= gpGlobals->time)
			{
				m_flNextJump = gpGlobals->time + 0.5f;
				if (FindFollowJump())
					return GetScheduleOfType(SCHED_JUMP_FOLLOWING);
			}
		}
		return GetScheduleOfType(SCHED_TARGET_FACE);
	}
	case SCHED_JUMP_FOLLOWING:
		return slJumpFollowing;
	case SCHED_TARGET_CHASE:
	case SCHED_FOLLOW:
		return slFollow;
	case SCHED_FOLLOW_CAUTIOUS:
		return slFollowCautious;
	case SCHED_FOLLOW_NEAREST:
		return slFollowTargetNearest;
	case SCHED_FOLLOW_FAILED:
	{
		FOLLOW_FAIL_POLICY failPolicy = FollowFailPolicy();
		if (failPolicy == FOLLOW_FAIL_STOP)
		{
			return GetScheduleOfType(SCHED_CANT_FOLLOW);
		}
		else if (failPolicy == FOLLOW_FAIL_TRY_NEAREST)
		{
			MakeMyBlockerMoveAway();
			if (m_flNextJump <= gpGlobals->time)
			{
				m_flNextJump = gpGlobals->time + 0.5f;
				if (CanJumpFreely() && FindFollowJump())
					return GetScheduleOfType(SCHED_JUMP_FOLLOWING);
			}
			return GetScheduleOfType(SCHED_FOLLOW_NEAREST);
		}
		else
		{
			MakeMyBlockerMoveAway();
			if (m_flNextJump <= gpGlobals->time)
			{
				m_flNextJump = gpGlobals->time + 0.5f;
				if (CanJumpFreely() && FindFollowJump())
					return GetScheduleOfType(SCHED_JUMP_FOLLOWING);
			}
			return GetScheduleOfType(SCHED_FAIL_PVS_INDEPENDENT);
		}
	}
	case SCHED_FOLLOW_NEAREST_FAILED:
	{
		if (m_flNextJump <= gpGlobals->time)
		{
			m_flNextJump = gpGlobals->time + 0.5f;
			if (CanJumpFreely() && FindFollowJump())
				return GetScheduleOfType(SCHED_JUMP_FOLLOWING);
		}
		if (m_hTargetEnt != 0 && FVisible(m_hTargetEnt))
		{
			if (RANDOM_LONG(0, 1) == 1)
			{
				SuggestSchedule(SCHED_RETREAT_FROM_SPOT, this, 0.0f, 64.0f, SUGGEST_SCHEDULE_FLAG_WALK|SUGGEST_SCHEDULE_FLAG_ON_FAIL);
				return GetScheduleOfType(m_suggestedSchedule);
			}
			return GetScheduleOfType(SCHED_FAIL_PVS_INDEPENDENT);
		}
		else
		{
			SuggestSchedule(SCHED_RETREAT_FROM_SPOT, this, 0.0f, 64.0f, SUGGEST_SCHEDULE_FLAG_RUN|SUGGEST_SCHEDULE_FLAG_ON_FAIL);
			return GetScheduleOfType(m_suggestedSchedule);
		}
	}
	case SCHED_CANT_FOLLOW:
	{
		return slStopFollowing;
	}
	case SCHED_FAIL_PVS_INDEPENDENT:
	{
		if (NpcFollowOutOfPvs()) {
			return slPVSIndependentFail;
		}
		return GetScheduleOfType(SCHED_FAIL);
	}
	default:
		return CSquadMonster::GetScheduleOfType(Type);
	}
}

void CFollowingMonster::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FACE_PLAYER:
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		break;
	case TASK_MOVE_AWAY_PATH:
	case TASK_MOVE_AWAY_PATH_RUNNING:
		{
			Activity movementActivity = pTask->iTask == TASK_MOVE_AWAY_PATH_RUNNING ? ACT_RUN : ACT_WALK;
			Vector dir = pev->angles;
			dir.y = pev->ideal_yaw + 180;
			Vector vecBackward;
			Vector vecLeft;
			bool success = false;

			m_lastMoveBlocker = 0;

			const float waitTime = 2.0f;
			UTIL_MakeVectorsPrivate( dir, vecBackward, vecLeft, NULL );
			if( MoveToLocation( movementActivity, waitTime, pev->origin + vecBackward * pTask->flData, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
			{
				success = true;
			}
			if( !success && MoveToLocation( movementActivity, waitTime, pev->origin + vecBackward * pTask->flData * 0.5f, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
			{
				success = true;
			}
			else
			{
				HandleBlocker(CBaseEntity::Instance( gpGlobals->trace_ent ), false);
			}

			if (!success)
			{
				if( MoveToLocation( movementActivity, waitTime, pev->origin + vecLeft * pTask->flData * 0.5f, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
				{
					success = true;
				}
				if( !success && MoveToLocation( movementActivity, waitTime, pev->origin - vecLeft * pTask->flData * 0.5f, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
				{
					success = true;
				}
				else
				{
					if (m_lastMoveBlocker == 0)
						HandleBlocker(CBaseEntity::Instance( gpGlobals->trace_ent ), false);
				}
			}

			if( !success )
			{
				if ( FindSpotAway( pev->origin, 0, Q_max(256.0f, pTask->flData), pTask->iTask == TASK_MOVE_AWAY_PATH_RUNNING ? FINDSPOTAWAY_RUN : FINDSPOTAWAY_WALK ) )
				{
					// then try for plain ole cover
					m_flMoveWaitFinished = gpGlobals->time + waitTime;
					success = true;
				}
				else
				{
					// nowhere to go?
					TaskFail("can't move away");
				}
			}
		}
		break;
	case TASK_FIND_MOVE_AWAY:
		// The task is not used now. It's for future improvements
		{
			Vector vecSpot;

			float minDist = COVER_DELTA;
			float maxDist = 140.0f;

			CalcSuggestedSpot(&vecSpot);

			if (vecSpot == pev->origin)
			{
				Vector dir = pev->angles;
				dir.y = pev->ideal_yaw + 180;
				Vector move;
				UTIL_MakeVectorsPrivate( dir, move, NULL, NULL );

				vecSpot = pev->origin - move;

				maxDist = 100.0f;
			}

			const int moveFlag = FBitSet(m_suggestedScheduleFlags, SUGGEST_SCHEDULE_FLAG_RUN) ? FINDSPOTAWAY_RUN : FINDSPOTAWAY_WALK;
			if ( FindStraightSpotAway( vecSpot, SuggestedMinDist(minDist), SuggestedMaxDist(maxDist), moveFlag ) )
			{
				m_flMoveWaitFinished = gpGlobals->time;
				TaskComplete();
			}
			else if( FindLateralSpotAway( vecSpot, SuggestedMinDist(minDist), SuggestedMaxDist(maxDist), moveFlag ) )
			{
				m_flMoveWaitFinished = gpGlobals->time;
				TaskComplete();
			}
			else if ( FindSpotAway( vecSpot, SuggestedMinDist(minDist), SuggestedMaxDist(maxDist), moveFlag ) )
			{
				m_flMoveWaitFinished = gpGlobals->time;
				TaskComplete();
			}
			else
			{
				TaskFail("can't move away");
			}
		}
		break;
	case TASK_GET_NEAREST_PATH_TO_TARGET:
		{
			CBaseEntity* pTarget = m_hTargetEnt;
			if (pTarget == 0)
				TaskFail("no target ent");
			else if( BuildNearestRoute( pTarget->pev->origin, pev->view_ofs, pTask->flData, ( pTarget->pev->origin - pev->origin ).Length2D() ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("can't build nearest route to target");
			}
		}
		break;
	case TASK_CANT_FOLLOW:
		StopFollowing( false, false );
		TaskComplete();
		break;
	case TASK_FALL_TO_GROUND_FOLLOWING:
		break;
	default:
		CSquadMonster::StartTask( pTask );
		break;
	}
}

void CFollowingMonster::RunTask( Task_t *pTask )
{
	CBaseEntity *pPlayer;
	switch( pTask->iTask )
	{
	case TASK_FACE_PLAYER:
		{
			// Get edict for one player
			pPlayer = PlayerToFace();

			if( pPlayer )
			{
				MakeIdealYaw( pPlayer->pev->origin );
				ChangeYaw( pev->yaw_speed );
				IdleHeadTurn( pPlayer->pev->origin );
				if( gpGlobals->time > m_flWaitFinished && FlYawDiff() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail("no player to face");
			}
		}
		break;
	case TASK_MOVE_AWAY_PATH:
	case TASK_MOVE_AWAY_PATH_RUNNING:
		{
			// Walk path until far enough away
			if( ( m_vecLastPosition - pev->origin ).IsLength2DGreaterThan(pTask->flData) || MovementIsComplete() )
			{
				TaskComplete();
				//RouteClear();		Called by TASK_STOP_MOVING
			}
		}
		break;
	case TASK_WAIT_PVS:
		{
			if ( IsFollowingPlayer() )
			{
				TaskComplete();
			}
			else
			{
				CSquadMonster::RunTask( pTask );
			}
		}
		break;
	case TASK_FALL_TO_GROUND_FOLLOWING:
		{
			Vector vecTarget;
			CBaseEntity* pLeader = FollowedPlayer();
			if (pLeader)
			{
				vecTarget = pLeader->pev->origin;
				HandleJumpFallTask(&vecTarget, false);
			}
			else
			{
				HandleJumpFallTask(nullptr, false);
			}
		}
		break;
	default:
		CSquadMonster::RunTask( pTask );
		break;
	}
}

void CFollowingMonster::PrescheduleThink()
{
	if (IsFollowingPlayer() && ShouldDeclineFollowing())
	{
		StopFollowing(true, false);
	}
	CSquadMonster::PrescheduleThink();
}

TakeDamageResult CFollowingMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	TakeDamageResult ret = CSquadMonster::TakeDamage(pevInflictor, pevAttacker, damageInfo);
	if (ret.TookDamageToHealth() && IsFullyAlive() && pevAttacker && m_MonsterState != MONSTERSTATE_PRONE && FBitSet(pevAttacker->flags, FL_CLIENT)
		&& IDefaultRelationship(CLASS_PLAYER) == R_AL)
	{
		PlayFriendlyFireComplaint();
	}
	return ret;
}

void CFollowingMonster::FollowingMonsterInit()
{
	MonsterInit();
	if (IDefaultRelationship(CLASS_PLAYER) == R_AL) {
		m_afCapability |= bits_CAP_USABLE;
		SetUse( &CFollowingMonster::FollowerUse );
		SetTouch( &CFollowingMonster::FollowerTouch );
	}
}

// turn head towards supplied origin
void CFollowingMonster::IdleHeadTurn( Vector &vecFriend )
{
	// turn head in desired direction only if ent has a turnable head
	if( m_afCapability & bits_CAP_TURN_HEAD )
	{
		float yaw = VecToYaw( vecFriend - pev->origin ) - pev->angles.y;

		if( yaw > 180 )
			yaw -= 360;
		if( yaw < -180 )
			yaw += 360;

		// turn towards vector
		SetBoneController( 0, yaw );
	}
}

void CFollowingMonster::StopFollowing(bool clearSchedule , bool saySentence)
{
	if( IsFollowingPlayer() )
	{
		if( saySentence && !( m_afMemory & bits_MEMORY_PROVOKED ) )
		{
			PlayUnUseSentence();
		}

		if( (m_movementGoal & MOVEGOAL_TARGETENT) && m_hTargetEnt == FollowedPlayer() )
			RouteClear(); // Stop him from walking toward the player
		ClearFollowedPlayer();
		if( clearSchedule )
			ClearSchedule();
		if( m_hEnemy != 0 )
			m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
}

void CFollowingMonster::StartFollowing(CBaseEntity *pLeader , bool saySentence)
{
	StopScript();

	m_hTargetEnt = pLeader;
	if (saySentence)
	{
		PlayUseSentence();
	}

	ClearConditions( bits_COND_CLIENT_PUSH );
	if (!HasConditions(bits_COND_SEE_ENEMY))
		ClearSchedule();
}

void CFollowingMonster::LimitFollowers( CBaseEntity *pPlayer, int maxFollowers )
{
	return;
}

bool CFollowingMonster::CanFollow()
{
	return AbleToFollow() && !IsFollowingPlayer();
}

bool CFollowingMonster::AbleToFollow()
{
	if( m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT )
	{
		if( !m_pCine )
			return false;
		if( !m_pCine->CanInterruptByPlayerCall() )
			return false;
	}

	if( !IsFullyAlive() )
		return false;
	return true;
}

bool CFollowingMonster::IsFollowingPlayer(CBaseEntity *pLeader)
{
	return FollowedPlayer() == pLeader;
}

bool CFollowingMonster::IsFollowingPlayer()
{
	return FollowedPlayer() != 0;
}

CBaseEntity* CFollowingMonster::FollowedPlayer()
{
	if (m_hTargetEnt != 0 && m_hTargetEnt->IsPlayer())
		return m_hTargetEnt;
	return NULL;
}

void CFollowingMonster::ClearFollowedPlayer()
{
	m_hTargetEnt = 0;
}

bool CFollowingMonster::InScriptedSentence()
{
	return false;
}

bool CFollowingMonster::AllowUseDuringScriptedSentence()
{
	return false;
}

Schedule_t* CFollowingMonster::GetFollowingSchedule(bool ignoreEnemy)
{
	if( (ignoreEnemy || m_hEnemy == 0 || !m_hEnemy->IsFullyAlive()) && IsFollowingPlayer() )
	{
		if( !FollowedPlayer()->IsAlive() )
		{
			// UNDONE: Comment about the recently dead player here?
			StopFollowing( false, false );
			return NULL;
		}
		else
		{
			if( HasConditions( bits_COND_CLIENT_PUSH ) )
			{
				return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
			}
			return GetScheduleOfType( SCHED_TARGET_FACE );
		}
	}

	if( HasConditions( bits_COND_CLIENT_PUSH ) )
	{
		return GetScheduleOfType( SCHED_MOVE_AWAY );
	}
	return NULL;
}

Schedule_t* CFollowingMonster::GetUtilitySchedule()
{
	Schedule_t* regenSchedule = GetRegenerationSchedule();
	if (regenSchedule)
		return regenSchedule;

	Schedule_t* followingSchedule = GetFollowingSchedule();
	if (followingSchedule)
		return followingSchedule;

	return nullptr;
}

void CFollowingMonster::FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_followagePolicy == FOLLOWAGE_SCRIPTED_ONLY)
		return;
	if (m_followagePolicy == FOLLOWAGE_SCRIPTED_ONLY_DECLINE_USE)
	{
		if (!InScriptedSentence() && !ShouldDiscardFollowing(pCaller))
		{
			DeclineFollowing(pCaller);
		}
		return;
	}
	int result = DoFollowerUse(pCaller, true, USE_TOGGLE);
	if (result == FOLLOWING_NOTREADY && AllowUseDuringScriptedSentence())
	{
		DoFollowerUse(pCaller, false, USE_TOGGLE, true);
	}
}

bool CFollowingMonster::ShouldDeclineFollowing()
{
	return IsLockedByMaster() || (pev->spawnflags & SF_MONSTER_PREDISASTER && !m_sMaster);
}

bool CFollowingMonster::ShouldDiscardFollowing(CBaseEntity *pCaller)
{
	const int rel = IRelationship(pCaller);
	return (rel >= R_DL || rel == R_FR);
}

int CFollowingMonster::DoFollowerUse(CBaseEntity *pCaller, bool saySentence, USE_TYPE useType, bool ignoreScriptedSentence)
{
	if (!IsFullyAlive())
	{
		return FOLLOWING_DEAD;
	}
	if( pCaller != NULL && pCaller->IsPlayer() )
	{
		if (!AbleToFollow())
			return FOLLOWING_BUSYINSCRIPT;

		if (!ignoreScriptedSentence && InScriptedSentence())
			return FOLLOWING_NOTREADY;

		if (ShouldDiscardFollowing(pCaller))
			return FOLLOWING_DISCARDED;

		// Pre-disaster followers can't be used unless they've got a master to override their behaviour...
		if (ShouldDeclineFollowing())
		{
			if (saySentence)
				DeclineFollowing(pCaller);
			return FOLLOWING_DECLINED;
		}

		const bool isFollowing = IsFollowingPlayer();
		if (isFollowing && useType == USE_ON)
		{
			return FOLLOWING_NOCHANGE;
		}
		if (!isFollowing && useType == USE_OFF)
		{
			return FOLLOWING_NOCHANGE;
		}
		if (!isFollowing && (useType == USE_TOGGLE || useType == USE_ON))
		{
			LimitFollowers( pCaller, MaxFollowers() );

			if( m_afMemory & bits_MEMORY_PROVOKED )
			{
				ALERT( at_aiconsole, "%s: I'm not following you, you evil person!\n", STRING(pev->classname) );
				return FOLLOWING_DISCARDED;
			}
			else
			{
				StartFollowing( pCaller, saySentence );
				return FOLLOWING_STARTED;
			}
		}
		if (isFollowing && (useType == USE_TOGGLE || useType == USE_OFF))
		{
			StopFollowing( true, saySentence );
			return FOLLOWING_STOPPED;
		}
	}
	return FOLLOWING_INVALID;
}

void CFollowingMonster::JustSpoke()
{
	g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5f, 2.0f);
}

CBaseEntity* CFollowingMonster::PlayerToFace()
{
	return g_pGameRules->EffectiveAlivePlayer(FollowedPlayer());
}

void CFollowingMonster::StopScript()
{
	if (m_pCine)
	{
		m_pCine->CancelScript(SCRIPT_CANCELLATION_REASON_STARTED_FOLLOWING);
		if (m_pCine) { // in case it was not cleared out for some reason
			CineCleanup();
		}
	}
}

Schedule_t *CFollowingMonster::GetIdleReloadSchedule()
{
	if (HasConditions(bits_COND_NO_AMMO_LOADED))
	{
		return GetScheduleOfType(SCHED_RELOAD);
	}
	else if (m_cClipSize > 0 && m_cAmmoLoaded <= m_cClipSize/2)
	{
		return GetScheduleOfType(SCHED_RELOAD_NOT_EMPTY);
	}
	return nullptr;
}

void CFollowingMonster::CheckAmmo()
{
	if (m_cClipSize > 0 && m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

void CFollowingMonster::CompleteReloadTask()
{
	//ALERT(at_console, "CompleteReloadTask. Time: %g. Frame: %g\n", gpGlobals->time, pev->frame);
	if (m_cClipSize > 0)
	{
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
	}
}

void CFollowingMonster::ReportAIState(ALERT_TYPE level)
{
	CSquadMonster::ReportAIState(level);
	if (IsFollowingPlayer())
		ALERT(level, "Following a player. ");
	ALERT(level, "Follow fail policy: ");
	switch (FollowFailPolicy()) {
	case FOLLOW_FAIL_STOP:
		ALERT(level, "Stop on fail. ");
		break;
	case FOLLOW_FAIL_TRY_NEAREST:
		ALERT(level, "Try nearest node on fail. ");
		break;
	default:
		ALERT(level, "Regular. ");
		break;
	}

	if (m_cClipSize > 0)
	{
		ALERT(level, "Ammo loaded: %d / %d. ", m_cAmmoLoaded, m_cClipSize);
	}
}

bool CFollowingMonster::CanRoamAfterCombat()
{
	if (IsFollowingPlayer())
		return false;
	return CSquadMonster::CanRoamAfterCombat();
}

bool CFollowingMonster::IsUsefulToDisplayHint(CBaseEntity* pPlayer)
{
	if (!(m_afCapability & bits_CAP_USABLE))
		return false;
	if (ShouldDiscardFollowing(pPlayer))
		return false;
	if (ShouldDeclineFollowing())
		return false;
	if (m_followagePolicy == FOLLOWAGE_SCRIPTED_ONLY || m_followagePolicy == FOLLOWAGE_SCRIPTED_ONLY_DECLINE_USE)
		return false;
	return true;
}

bool CFollowingMonster::FindFollowJump()
{
	return FindJumpToEntity(FollowedPlayer());
}

bool CFollowingMonster::SomeoneIsTalking()
{
	return gpGlobals->time <= g_talkWaitTime;
}

void CFollowingMonster::ResetTalkWaitTime()
{
	g_talkWaitTime = 0;
}

void CFollowingMonster::DelayTalkWaitTime(float delay)
{
	g_talkWaitTime = gpGlobals->time + delay;
}
