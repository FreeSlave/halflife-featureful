#pragma once
#ifndef FOLLOWINGMOSNTER_H
#define FOLLOWINGMONSTER_H
#ifndef MONSTERS_H
#include "monsters.h"
#endif
#include "squadmonster.h"

enum
{
	SCHED_MOVE_AWAY = LAST_COMMON_SCHEDULE + 1,		// Try to get out of the player's way
	SCHED_MOVE_AWAY_FOLLOW,	// same, but follow afterward
	SCHED_MOVE_AWAY_FAIL,	// Turn back toward player
	SCHED_TARGET_REACHED, // Schedule to set after reaching a followed target, gives a chance to do something else besides facing a target again
	SCHED_TARGET_FACE_CHECK_JUMP,
	SCHED_FOLLOW_FAILED,
	SCHED_FOLLOW_NEAREST,
	SCHED_CANT_FOLLOW,
	SCHED_FOLLOW_CAUTIOUS,
	SCHED_FAIL_PVS_INDEPENDENT,
	SCHED_FOLLOW_NEAREST_FAILED,

	LAST_FOLLOWINGMONSTER_SCHEDULE		// MUST be last
};

enum
{
	TASK_MOVE_AWAY_PATH = LAST_COMMON_TASK + 1,
	TASK_MOVE_AWAY_PATH_RUNNING,
	TASK_WALK_PATH_FOR_UNITS,
	TASK_FIND_MOVE_AWAY,
	TASK_FACE_PLAYER,		// Face the player
	TASK_GET_NEAREST_PATH_TO_TARGET, // Find path to the nearest node to target
	TASK_CANT_FOLLOW,

	LAST_FOLLOWINGMONSTER_TASK			// MUST be last
};

enum
{
	FOLLOWING_INVALID,
	FOLLOWING_BUSYINSCRIPT,
	FOLLOWING_DECLINED,
	FOLLOWING_STARTED,
	FOLLOWING_STOPPED,
	FOLLOWING_NOCHANGE,
	FOLLOWING_NOTREADY,
	FOLLOWING_DISCARDED,
	FOLLOWING_DEAD,
};

typedef enum
{
	FOLLOW_FAIL_DEFAULT = 0,
	FOLLOW_FAIL_REGULAR,
	FOLLOW_FAIL_STOP,
	FOLLOW_FAIL_TRY_NEAREST,
} FOLLOW_FAIL_POLICY;

typedef enum
{
	FOLLOWAGE_REGULAR = 0,
	FOLLOWAGE_SCRIPTED_ONLY,
	FOLLOWAGE_SCRIPTED_ONLY_DECLINE_USE,
} FOLLOWAGE_POLICY;

class CFollowingMonster : public CSquadMonster
{
public:
	// Base Monster functions
	void Touch(	CBaseEntity *pOther ) override;
	void OnDying(bool gibbed, CBaseEntity* pKiller) override;
	int ObjectCaps() override;
	void KeyValue( KeyValueData *pkvd ) override;

	// AI functions
	Schedule_t *GetScheduleOfType ( int Type ) override;
	void StartTask( Task_t *pTask ) override;
	void RunTask( Task_t *pTask ) override;
	void PrescheduleThink() override;

	void FollowingMonsterInit();
	void IdleHeadTurn( Vector &vecFriend );

	// Following related
	bool	CanFollow();
	bool	AbleToFollow();
	bool	IsFollowingPlayer( CBaseEntity* pLeader );
	bool	IsFollowingPlayer();
	virtual	CBaseEntity* FollowedPlayer();
	virtual void ClearFollowedPlayer();
	void StopFollowing(bool clearSchedule, bool saySentence = true ) override;
	virtual void StartFollowing( CBaseEntity *pLeader, bool saySentence = true );
	virtual void DeclineFollowing( CBaseEntity* pCaller ) {}
	virtual void LimitFollowers( CBaseEntity *pPlayer, int maxFollowers );
	virtual int MaxFollowers() { return -1; }

	CFollowingMonster* MyFollowingMonsterPointer() override { return this; }
	virtual bool InScriptedSentence();
	virtual bool AllowUseDuringScriptedSentence();
	Schedule_t* GetFollowingSchedule(bool ignoreEnemy = false);
	Schedule_t* GetUtilitySchedule();
	void EXPORT FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int DoFollowerUse(CBaseEntity* pCaller, bool saySentence, USE_TYPE useType, bool ignoreScriptedSentence = false);
	bool ShouldDeclineFollowing();
	bool ShouldDiscardFollowing(CBaseEntity* pCaller);
	virtual FOLLOW_FAIL_POLICY DefaultFollowFailPolicy() {
		return FOLLOW_FAIL_REGULAR;
	}
	FOLLOW_FAIL_POLICY FollowFailPolicy() {
		FOLLOW_FAIL_POLICY failPolicy = m_followFailPolicy > 0 ? (FOLLOW_FAIL_POLICY)m_followFailPolicy : DefaultFollowFailPolicy();
		if (m_followagePolicy != 0 && failPolicy == FOLLOW_FAIL_STOP)
		{
			return FOLLOW_FAIL_REGULAR;
		}
		return failPolicy;
	}

	virtual void PlayUseSentence() {}
	virtual void PlayUnUseSentence() {}

	CBaseEntity* PlayerToFace();
	void StopScript();

	Schedule_t *GetIdleReloadSchedule();
	void CheckAmmo() override;
	void CompleteReloadTask() override;
	void ReportAIState(ALERT_TYPE level) override;

	bool CanRoamAfterCombat() override;
	bool IsUsefulToDisplayHint(CBaseEntity* pPlayer) override;

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	short m_followFailPolicy;
	short m_followagePolicy;

	int m_cClipSize;

	CUSTOM_SCHEDULES
};

#endif
