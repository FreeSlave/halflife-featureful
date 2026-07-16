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
// Default behaviors.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"soundent.h"
#include	"scripted.h"

extern cvar_t npc_combat_fail_schedule;

//=========================================================
// Fail
//=========================================================
Task_t tlFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slFail[] =
{
	{
		tlFail,
		ARRAYSIZE( tlFail ),
		bits_COND_CAN_ATTACK,
		0,
		"Fail"
	},
};

Task_t tlCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, (float)2 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slCombatFail[] =
{
	{
		tlCombatFail,
		ARRAYSIZE( tlCombatFail ),
		bits_COND_CAN_ATTACK,
		0,
		"CombatFail"
	},
};

Schedule_t slScriptMoveFail[] =
{
	{
		tlFail,
		ARRAYSIZE( tlFail ),
		bits_COND_CAN_ATTACK|
		SCRIPT_BREAK_CONDITIONS,
		0,
		"ScriptMoveFail"
	},
};

//=========================================================
//	Idle Schedules
//=========================================================
Task_t tlIdleStand1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)5 }, // repick IDLESTAND every five seconds. gives us a chance to pick an active idle, fidget, etc.
};

Schedule_t slIdleStand[] =
{
	{
		tlIdleStand1,
		ARRAYSIZE( tlIdleStand1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL_FOOD |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags
		bits_SOUND_WORLD |
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_DANGER |

		bits_SOUND_MEAT	|// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

Task_t tlIdlePatrolTurning[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_PATROL_TURNING, (float)0 },
};

Schedule_t slIdlePatrolTurning[] =
{
	{
		tlIdlePatrolTurning,
		ARRAYSIZE( tlIdlePatrolTurning ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL_FOOD |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags
		bits_SOUND_WORLD |
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_DANGER |

		bits_SOUND_MEAT	|// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleTurning"
	},
};

Schedule_t slIdleTrigger[] =
{
	{
		tlIdleStand1,
		ARRAYSIZE( tlIdleStand1 ),
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Trigger"
	},
};

Task_t tlIdleWalk1[] =
{
	{ TASK_WALK_PATH, (float)9999 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slIdleWalk[] =
{
	{
		tlIdleWalk1,
		ARRAYSIZE( tlIdleWalk1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL_FOOD |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"Idle Walk"
	},
};

Task_t tlIdleRun[] =
{
	{ TASK_RUN_PATH, (float)9999 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slIdleRun[] =
{
	{
		tlIdleRun,
		ARRAYSIZE( tlIdleRun ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL_FOOD |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"Idle Run"
	},
};

//=========================================================
// Ambush - monster stands in place and waits for a new 
// enemy, or chance to attack an existing enemy.
//=========================================================
Task_t tlAmbush[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_INDEFINITE, (float)0 },
};

Schedule_t slAmbush[] =
{
	{
		tlAmbush,
		ARRAYSIZE( tlAmbush ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED,
		0,
		"Ambush"
	},
};

//=========================================================
// ActiveIdle schedule - !!!BUGBUG - if this schedule doesn't
// complete on its own, the monster's HintNode will not be 
// cleared, and the rest of the monster's group will avoid
// that node because they think the group member that was 
// previously interrupted is still using that node to active
// idle.
///=========================================================
Task_t tlActiveIdle[] =
{
	{ TASK_FIND_HINTNODE, (float)0 },
	{ TASK_GET_PATH_TO_HINTNODE, (float)0 },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_HINTNODE, (float)0 },
	{ TASK_PLAY_ACTIVE_IDLE, (float)0 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
	{ TASK_CLEAR_HINTNODE, (float)0 },
};

Schedule_t slActiveIdle[] =
{
	{
		tlActiveIdle,
		ARRAYSIZE( tlActiveIdle ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_WORLD |
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_DANGER,
		"Active Idle"
	}
};

//=========================================================
//	Wake Schedules
//=========================================================
Task_t tlWakeAngry1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SOUND_WAKE, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
};

Schedule_t slWakeAngry[] =
{
	{
		tlWakeAngry1,
		ARRAYSIZE( tlWakeAngry1 ),
		0,
		0,
		"Wake Angry"
	}
};

//=========================================================
// AlertFace Schedules
//=========================================================
Task_t tlAlertFace1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_IDEAL, (float)0 },
};

Schedule_t slAlertFace[] =
{
	{
		tlAlertFace1,
		ARRAYSIZE( tlAlertFace1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED,
		0,
		"Alert Face"
	},
};

//=========================================================
// AlertSmallFlinch Schedule - shot, but didn't see attacker,
// flinch then face
//=========================================================
Task_t tlAlertSmallFlinch[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_FLINCHED },
	{ TASK_SMALL_FLINCH, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_ALERT_FACE },
};

Schedule_t slAlertSmallFlinch[] =
{
	{
		tlAlertSmallFlinch,
		ARRAYSIZE( tlAlertSmallFlinch ),
		0,
		0,
		"Alert Small Flinch"
	},
};

//=========================================================
// AlertIdle Schedules
//=========================================================
Task_t tlAlertStand1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)20 },
	{ TASK_SUGGEST_STATE, (float)MONSTERSTATE_IDLE },
};

Schedule_t slAlertStand[] =
{
	{
		tlAlertStand1,
		ARRAYSIZE( tlAlertStand1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED |
		bits_COND_SMELL |
		bits_COND_SMELL_FOOD |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |// sound flags
		bits_SOUND_WORLD |
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scent flags
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"Alert Stand"
	},
};

//=========================================================
// InvestigateSound - sends a monster to the location of the
// sound that was just heard, to check things out. 
//=========================================================
Task_t tlInvestigateSound[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSOUND, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_IDLE },
	{ TASK_WAIT, (float)10 },
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slInvestigateSound[] =
{
	{
		tlInvestigateSound,
		ARRAYSIZE( tlInvestigateSound ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"InvestigateSound"
	},
};

Task_t tlInvestigateSpot[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_SPOT, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WALK_OR_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_IDLE },
	{ TASK_WAIT, (float)10 },
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0 },
	{ TASK_WALK_OR_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slInvestigateSpot[] =
{
	{
		tlInvestigateSpot,
		ARRAYSIZE( tlInvestigateSpot ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"InvestigateSpot"
	},
};

Task_t tlMoveToSpot[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_GET_PATH_TO_SPOT, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WALK_OR_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slMoveToSpot[] =
{
	{
		tlMoveToSpot,
		ARRAYSIZE( tlMoveToSpot ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"MoveToSpot"
	},
};

//=========================================================
// CombatIdle Schedule
//=========================================================
Task_t tlCombatStand1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_INDEFINITE, (float)0 },
};

Schedule_t slCombatStand[] =
{
	{
		tlCombatStand1,
		ARRAYSIZE( tlCombatStand1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_ATTACK, 
		0,
		"Combat Stand"
	},
};

//=========================================================
// CombatFace Schedule
//=========================================================
Task_t tlCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_ENEMY, (float)0 },
};

Schedule_t slCombatFace[] =
{
	{ 
		tlCombatFace1,
		ARRAYSIZE( tlCombatFace1 ),
		bits_COND_CAN_ATTACK |
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST,
		0,
		"Combat Face"
	},
};

//=========================================================
// Standoff schedule. Used in combat when a monster is 
// hiding in cover or the enemy has moved out of sight. 
// Should we look around in this schedule?
//=========================================================
Task_t tlStandoff[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, (float)2 },
};

Schedule_t slStandoff[] = 
{
	{
		tlStandoff,
		ARRAYSIZE( tlStandoff ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Standoff"
	}
};

//=========================================================
// Arm weapon (draw gun)
//=========================================================
Task_t	tlArmWeapon[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_ARM }
};

Schedule_t slArmWeapon[] =
{
	{
		tlArmWeapon,
		ARRAYSIZE( tlArmWeapon ),
		0,
		0,
		"Arm Weapon"
	}
};

//=========================================================
// reload schedule
//=========================================================
Task_t	tlReload[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_RELOAD, 0 },
};

Schedule_t slReload[] =
{
	{
		tlReload,
		ARRAYSIZE( tlReload ),
		bits_COND_HEAVY_DAMAGE,
		0,
		"Reload"
	}
};

Schedule_t slReloadNotEmpty[] =
{
	{
		tlReload,
		ARRAYSIZE( tlReload ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"ReloadNotEmpty"
	}
};

//=========================================================
//	Attack Schedules
//=========================================================

// primary range attack
Task_t tlRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slRangeAttack1[] =
{
	{
		tlRangeAttack1,
		ARRAYSIZE( tlRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Range Attack1"
	},
};

// secondary range attack
Task_t tlRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK2, (float)0 },
};

Schedule_t slRangeAttack2[] =
{
	{
		tlRangeAttack2,
		ARRAYSIZE( tlRangeAttack2 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Range Attack2"
	},
};

// primary melee attack
Task_t tlPrimaryMeleeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_MELEE_ATTACK1, (float)0 },
};

Schedule_t slPrimaryMeleeAttack[] =
{
	{
		tlPrimaryMeleeAttack1,
		ARRAYSIZE( tlPrimaryMeleeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Primary Melee Attack"
	},
};

// secondary melee attack
Task_t tlSecondaryMeleeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_MELEE_ATTACK2, (float)0},
};

Schedule_t slSecondaryMeleeAttack[] =
{
	{
		tlSecondaryMeleeAttack1,
		ARRAYSIZE( tlSecondaryMeleeAttack1 ), 
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Secondary Melee Attack"
	},
};

// special attack1
Task_t tlSpecialAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SPECIAL_ATTACK1, (float)0 },
};

Schedule_t slSpecialAttack1[] =
{
	{
		tlSpecialAttack1,
		ARRAYSIZE( tlSpecialAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Special Attack1"
	},
};

// special attack2
Task_t tlSpecialAttack2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SPECIAL_ATTACK2, (float)0 },
};

Schedule_t slSpecialAttack2[] =
{
	{
		tlSpecialAttack2,
		ARRAYSIZE( tlSpecialAttack2 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Special Attack2"
	},
};

// Chase enemy schedule
Task_t tlChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CHASE_ENEMY_FAILED },
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slChaseEnemy[] =
{
	{
		tlChaseEnemy1,
		ARRAYSIZE( tlChaseEnemy1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Chase Enemy"
	},
};


// Chase enemy failure schedule
Task_t tlChaseEnemyFailed[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FIND_SPOT_AWAY_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	//{ TASK_TURN_LEFT, (float)179 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, (float)0.5 },
};

Schedule_t slChaseEnemyFailed[] =
{
	{
		tlChaseEnemyFailed,
		ARRAYSIZE( tlChaseEnemyFailed ),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Chase Enemy Failed"
	},
};

//=========================================================
// small flinch, played when minor damage is taken.
//=========================================================
Task_t tlSmallFlinch[] =
{
	{ TASK_REMEMBER, (float)bits_MEMORY_FLINCHED },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SMALL_FLINCH, 0 },
};

Schedule_t slSmallFlinch[] =
{
	{
		tlSmallFlinch,
		ARRAYSIZE( tlSmallFlinch ),
		0,
		0,
		"Small Flinch"
	},
};

//=========================================================
// Die!
//=========================================================
Task_t tlDie1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SOUND_DIE, (float)0 },
	{ TASK_DIE, (float)0 },
};

Schedule_t slDie[] =
{
	{
		tlDie1,
		ARRAYSIZE( tlDie1 ),
		0,
		0,
		"Die"
	},
};

//=========================================================
// Victory Dance
//=========================================================
Task_t tlVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_WAIT, (float)0 },
};

Schedule_t slVictoryDance[] =
{
	{
		tlVictoryDance,
		ARRAYSIZE( tlVictoryDance ),
		0,
		0,
		"Victory Dance"
	},
};

//=========================================================
// BarnacleVictimGrab - barnacle tongue just hit the monster,
// so play a hit animation, then play a cycling pull animation
// as the creature is hoisting the monster.
//=========================================================
Task_t tlBarnacleVictimGrab[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_BARNACLE_HIT },
	{ TASK_SET_ACTIVITY, (float)ACT_BARNACLE_PULL },
	{ TASK_WAIT_INDEFINITE,	(float)0 },// just cycle barnacle pull anim while barnacle hoists. 
};

Schedule_t slBarnacleVictimGrab[] =
{
	{
		tlBarnacleVictimGrab,
		ARRAYSIZE( tlBarnacleVictimGrab ),
		0,
		0,
		"Barnacle Victim"
	}
};

//=========================================================
// BarnacleVictimChomp - barnacle has pulled the prey to its
// mouth. Victim should play the BARNCLE_CHOMP animation 
// once, then loop the BARNACLE_CHEW animation indefinitely
//=========================================================
Task_t tlBarnacleVictimChomp[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_BARNACLE_CHOMP },
	{ TASK_SET_ACTIVITY, (float)ACT_BARNACLE_CHEW },
	{ TASK_WAIT_INDEFINITE,	(float)0 },// just cycle barnacle pull anim while barnacle hoists.
};

Schedule_t slBarnacleVictimChomp[] =
{
	{
		tlBarnacleVictimChomp,
		ARRAYSIZE( tlBarnacleVictimChomp ),
		0,
		0,
		"Barnacle Chomp"
	}
};

// Universal Error Schedule
Task_t tlError[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_WAIT_INDEFINITE, (float)0 },
};

Schedule_t slError[] =
{
	{
		tlError,
		ARRAYSIZE( tlError ),
		0,
		0,
		"Error"
	},
};

Task_t tlScriptedWalk[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_AISCRIPT_MOVE_FAILED },
	{ TASK_WALK_TO_SCRIPT, (float)TARGET_MOVE_SCRIPTED },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLANT_ON_SCRIPT, (float)0 },
	{ TASK_FACE_SCRIPT, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_ENABLE_SCRIPT, (float)0 },
	{ TASK_WAIT_FOR_SCRIPT, (float)0 },
	{ TASK_PLAY_SCRIPT, (float)0 },
};

Schedule_t slWalkToScript[] =
{
	{
		tlScriptedWalk,
		ARRAYSIZE( tlScriptedWalk ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"WalkToScript"
	},
};

Task_t tlScriptedWalkToRadius[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_AISCRIPT_MOVE_FAILED },
	{ TASK_WALK_TO_SCRIPT_RADIUS, (float)0 },
	{ TASK_FACE_SCRIPT, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_ENABLE_SCRIPT, (float)0 },
	{ TASK_WAIT_FOR_SCRIPT, (float)0 },
	{ TASK_PLAY_SCRIPT, (float)0 },
};

Schedule_t slWalkToScriptRadius[] =
{
	{
		tlScriptedWalkToRadius,
		ARRAYSIZE( tlScriptedWalkToRadius ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"WalkToScriptRadius"
	},
};

Task_t tlScriptedRun[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_AISCRIPT_MOVE_FAILED },
	{ TASK_RUN_TO_SCRIPT, (float)TARGET_MOVE_SCRIPTED },
	{ TASK_WAIT_FOR_MOVEMENT,(float)0 },
	{ TASK_PLANT_ON_SCRIPT, (float)0 },
	{ TASK_FACE_SCRIPT, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_ENABLE_SCRIPT, (float)0 },
	{ TASK_WAIT_FOR_SCRIPT, (float)0 },
	{ TASK_PLAY_SCRIPT, (float)0 },
};

Schedule_t slRunToScript[] =
{
	{
		tlScriptedRun,
		ARRAYSIZE( tlScriptedRun ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"RunToScript"
	},
};

Task_t tlScriptedRunToRadius[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_AISCRIPT_MOVE_FAILED },
	{ TASK_RUN_TO_SCRIPT_RADIUS, (float)0 },
	{ TASK_FACE_SCRIPT, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_ENABLE_SCRIPT, (float)0 },
	{ TASK_WAIT_FOR_SCRIPT, (float)0 },
	{ TASK_PLAY_SCRIPT, (float)0 },
};

Schedule_t slRunToScriptRadius[] =
{
	{
		tlScriptedRunToRadius,
		ARRAYSIZE( tlScriptedRunToRadius ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"RunToScriptRadius"
	},
};

Task_t tlScriptedWait[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_WAIT_FOR_SCRIPT, (float)0 },
	{ TASK_PLAY_SCRIPT, (float)0 },
};

Schedule_t slWaitScript[] =
{
	{
		tlScriptedWait,
		ARRAYSIZE( tlScriptedWait ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"WaitForScript"
	},
};

Task_t tlScriptedFace[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_SCRIPT, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WAIT_FOR_SCRIPT, (float)0 },
	{ TASK_PLAY_SCRIPT, (float)0 },
};

Schedule_t slFaceScript[] =
{
	{
		tlScriptedFace,
		ARRAYSIZE( tlScriptedFace ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"FaceScript"
	},
};

//LRC
Task_t tlScriptedTeleport[] =
{
	{ TASK_PLANT_ON_SCRIPT,		(float)0		},
	{ TASK_WAIT_FOR_SCRIPT,		(float)0		},
	{ TASK_PLAY_SCRIPT,			(float)0		},
	//{ TASK_END_SCRIPT,			(float)0		},
};

//LRC
Schedule_t slTeleportToScript[] =
{
	{
		tlScriptedTeleport,
		ARRAYSIZE ( tlScriptedTeleport ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"TeleportToScript"
	},
};

Task_t tlScriptedForcedTeleport[] =
{
	{ TASK_FORCED_PLANT_ON_SCRIPT,		(float)0		},
	{ TASK_ENABLE_SCRIPT, (float)0 },
	{ TASK_WAIT_FOR_SCRIPT,		(float)0		},
	{ TASK_PLAY_SCRIPT,			(float)0		},
	//{ TASK_END_SCRIPT,			(float)0		},
};

Schedule_t slForcedTeleportToScript[] =
{
	{
		tlScriptedForcedTeleport,
		ARRAYSIZE ( tlScriptedForcedTeleport ),
		SCRIPT_BREAK_CONDITIONS,
		0,
		"Forced Teleport To Script"
	},
};

//=========================================================
// Cower - this is what is usually done when attempts
// to escape danger fail.
//=========================================================
Task_t tlCower[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_COWER },
};

Schedule_t slCower[] =
{
	{
		tlCower,
		ARRAYSIZE( tlCower ),
		0,
		0,
		"Cower"
	},
};

//=========================================================
// move away from where you're currently standing. 
//=========================================================
Task_t tlTakeCoverFromOrigin[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_ORIGIN, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, (float)179 },
};

Schedule_t slTakeCoverFromOrigin[] =
{
	{
		tlTakeCoverFromOrigin,
		ARRAYSIZE( tlTakeCoverFromOrigin ),
		bits_COND_NEW_ENEMY,
		0,
		"TakeCoverFromOrigin"
	},
};

Task_t tlTakeCoverFromSpot[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_SPOT, (float)0 },
	{ TASK_RUN_OR_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, (float)179 },
};

Schedule_t slTakeCoverFromSpot[] =
{
	{
		tlTakeCoverFromSpot,
		ARRAYSIZE( tlTakeCoverFromSpot ),
		bits_COND_NEW_ENEMY,
		0,
		"TakeCoverFromSpot"
	},
};

//=========================================================
// hide from the loudest sound source
//=========================================================
Task_t tlTakeCoverFromBestSound[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, (float)179 },
};

Schedule_t slTakeCoverFromBestSound[] =
{
	{
		tlTakeCoverFromBestSound,
		ARRAYSIZE( tlTakeCoverFromBestSound ),
		bits_COND_NEW_ENEMY,
		0,
		"TakeCoverFromBestSound"
	},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t tlTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	//{ TASK_TURN_LEFT, (float)179 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, (float)0.5 },
};

Schedule_t slTakeCoverFromEnemy[] =
{
	{
		tlTakeCoverFromEnemy,
		ARRAYSIZE( tlTakeCoverFromEnemy ),
		bits_COND_NEW_ENEMY,
		0,
		"Take Cover From Enemy"
	},
};
Task_t tlRetreaFromEnemy[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RETREAT_FROM_ENEMY_FAILED },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FIND_SPOT_AWAY_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, (float)0.5 },
};

Schedule_t slRetreatFromEnemy[] =
{
	{
		tlRetreaFromEnemy,
		ARRAYSIZE( tlRetreaFromEnemy ),
		bits_COND_NEW_ENEMY,
		0,
		"Retreat From Enemy"
	},
};

Task_t tlRetreatFromSpot[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RETREAT_FROM_SPOT_FAILED },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FIND_SPOT_AWAY, (float)0 },
	{ TASK_WALK_OR_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slRetreatFromSpot[] =
{
	{
		tlRetreatFromSpot,
		ARRAYSIZE( tlRetreatFromSpot ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Move Away From Spot"
	},
};

Task_t tlFreeroam[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT_RANDOM, 0.5f },
	{ TASK_GET_PATH_TO_FREEROAM_NODE, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 0.5f },
	{ TASK_WAIT_RANDOM, 0.5f },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slFreeroam[] =
{
	{
		tlFreeroam,
		ARRAYSIZE( tlFreeroam ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Free Roaming"
	},
};

Schedule_t slFreeroamAlert[] =
{
	{
		tlFreeroam,
		ARRAYSIZE( tlFreeroam ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_COMBAT |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Free Roaming (alert)"
	},
};

Task_t tlMoveToEnemyLKP[] =
{
	{ TASK_GET_PATH_TO_ENEMY_LKP, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slMoveToEnemyLKP[] =
{
	{
		tlMoveToEnemyLKP,
		ARRAYSIZE( tlMoveToEnemyLKP ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_ATTACK |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Move to Enemy LKP"
	},
};

Task_t tlIdleFace[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_SCHEDULED, (float)0 },
};

Schedule_t slIdleFace[] =
{
	{
		tlIdleFace,
		ARRAYSIZE( tlIdleFace ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Idle Face"
	},
};

Task_t tlRegen[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_REGENERATION, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE }
};

Schedule_t slRegen[] =
{
	{
		tlRegen,
		ARRAYSIZE(tlRegen),
		bits_COND_HEAVY_DAMAGE|
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Regeneration"
	}
};

//=========================================================
// repel land
//=========================================================
Task_t tlRepelLand[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_LAND },
};

Schedule_t slRepelLand[] =
{
	{
		tlRepelLand,
		ARRAYSIZE( tlRepelLand ),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Repel Land"
	},
};

Task_t tlRepelLandAndMove[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_LAND },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slRepelLandAndMove[] =
{
	{
		tlRepelLandAndMove,
		ARRAYSIZE( tlRepelLandAndMove ),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Repel Land And Move"
	},
};

Schedule_t *CBaseMonster::m_scheduleList[] =
{
	slIdleStand,
	slIdlePatrolTurning,
	slIdleTrigger,
	slIdleWalk,
	slIdleRun,
	slAmbush,
	slActiveIdle,
	slWakeAngry,
	slAlertFace,
	slAlertSmallFlinch,
	slAlertStand,
	slInvestigateSound,
	slInvestigateSpot,
	slMoveToSpot,
	slCombatStand,
	slCombatFace,
	slStandoff,
	slArmWeapon,
	slReload,
	slReloadNotEmpty,
	slRangeAttack1,
	slRangeAttack2,
	slPrimaryMeleeAttack,
	slSecondaryMeleeAttack,
	slSpecialAttack1,
	slSpecialAttack2,
	slChaseEnemy,
	slChaseEnemyFailed,
	slSmallFlinch,
	slDie,
	slVictoryDance,
	slBarnacleVictimGrab,
	slBarnacleVictimChomp,
	slError,
	slWalkToScript,
	slRunToScript,
	slWaitScript,
	slFaceScript,
	slTeleportToScript,
	slForcedTeleportToScript,
	slCower,
	slTakeCoverFromOrigin,
	slTakeCoverFromSpot,
	slTakeCoverFromBestSound,
	slTakeCoverFromEnemy,
	slFreeroam,
	slFreeroamAlert,
	slMoveToEnemyLKP,
	slRetreatFromEnemy,
	slRetreatFromSpot,
	slIdleFace,
	slRegen,
	slRepelLand,
	slRepelLandAndMove,
	slFail,
	slCombatFail
};

Schedule_t *CBaseMonster::ScheduleFromName( const char *pName )
{
	return ScheduleInList( pName, m_scheduleList, ARRAYSIZE( m_scheduleList ) );
}

Schedule_t *CBaseMonster::ScheduleInList( const char *pName, Schedule_t **pList, int listCount )
{
	int i;

	if( !pName )
	{
		ALERT( at_console, "%s set to unnamed schedule!\n", STRING( pev->classname ) );
		return NULL;
	}

	for( i = 0; i < listCount; i++ )
	{
		if( !pList[i]->pName )
		{
			ALERT( at_console, "Unnamed schedule in %s!\n", STRING( pev->classname ) );
			continue;
		}
		if( stricmp( pName, pList[i]->pName ) == 0 )
			return pList[i];
	}
	return NULL;
}

//=========================================================
// GetScheduleOfType - returns a pointer to one of the 
// monster's available schedules of the indicated type.
//=========================================================
Schedule_t* CBaseMonster::GetScheduleOfType( int Type ) 
{
	//ALERT( at_console, "Sched Type:%d\n", Type );
	switch( Type )
	{
	// This is the schedule for scripted sequences AND scripted AI
	case SCHED_AISCRIPT:
		{
			ASSERT( m_pCine != NULL );
			if( !m_pCine )
			{
				ALERT( at_aiconsole, "Script failed for %s\n", STRING( pev->classname ) );
				CineCleanup();
				return GetScheduleOfType( SCHED_IDLE_STAND );
			}
			//else
			//	ALERT( at_aiconsole, "Starting script %s for %s\n", STRING( m_pCine->m_iszPlay ), STRING( pev->classname ) );

			switch( m_pCine->m_fMoveTo )
			{
				case SCRIPT_MOVE_NO:
				case SCRIPT_MOVE_INSTANT:
					return slWaitScript;
				case SCRIPT_MOVE_WALK:
				{
					if (m_pCine->MoveFailAttemptsExceeded())
						return slForcedTeleportToScript;
					if (m_pCine->m_flMoveToRadius >= 1.0f)
						return slWalkToScriptRadius;
					return slWalkToScript;
				}
				case SCRIPT_MOVE_RUN:
				{
					if (m_pCine->MoveFailAttemptsExceeded())
						return slForcedTeleportToScript;
					if (m_pCine->m_flMoveToRadius >= 1.0f)
						return slRunToScriptRadius;
					return slRunToScript;
				}
				case SCRIPT_MOVE_FACE:
					return slFaceScript;
				case SCRIPT_MOVE_TELEPORT:
					return slTeleportToScript;
			}
			break;
		}
	case SCHED_AISCRIPT_MOVE_FAILED:
		{
			MakeMyBlockerMoveAway();
			return slScriptMoveFail;
		}
	case SCHED_IDLE_STAND:
		{
			if( RANDOM_LONG( 0, 14 ) == 0 && FCanActiveIdle() )
			{
				return &slActiveIdle[0];
			}

			return &slIdleStand[0];
		}
	case SCHED_IDLE_PATROL_TURNING:
		{
			return slIdlePatrolTurning;
		}
	case SCHED_IDLE_WALK:
		{
			return &slIdleWalk[0];
		}
	case SCHED_IDLE_RUN:
		{
			return &slIdleRun[0];
		}
	case SCHED_WAIT_TRIGGER:
		{
			return &slIdleTrigger[0];
		}
	case SCHED_WAKE_ANGRY:
		{
			return &slWakeAngry[0];
		}
	case SCHED_ALERT_FACE:
		{
			return &slAlertFace[0];
		}
	case SCHED_ALERT_STAND:
		{
			return &slAlertStand[0];
		}
	case SCHED_COMBAT_STAND:
		{
			return &slCombatStand[0];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slCombatFace[0];
		}
	case SCHED_CHASE_ENEMY:
		{
			return &slChaseEnemy[0];
		}
	case SCHED_CHASE_ENEMY_FAILED:
		{
			return GetScheduleOfType(SCHED_FAIL);
		}
	case SCHED_SMALL_FLINCH:
		{
			return &slSmallFlinch[0];
		}
	case SCHED_ALERT_SMALL_FLINCH:
		{
			return &slAlertSmallFlinch[0];
		}
	case SCHED_RELOAD:
		{
			return &slReload[0];
		}
	case SCHED_RELOAD_NOT_EMPTY:
		{
			return &slReloadNotEmpty[0];
		}
	case SCHED_ARM_WEAPON:
		{
			return &slArmWeapon[0];
		}
	case SCHED_STANDOFF:
		{
			return &slStandoff[0];
		}
	case SCHED_RANGE_ATTACK1:
		{
			return &slRangeAttack1[0];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slRangeAttack2[0];
		}
	case SCHED_MELEE_ATTACK1:
		{
			return &slPrimaryMeleeAttack[0];
		}
	case SCHED_MELEE_ATTACK2:
		{
			return &slSecondaryMeleeAttack[0];
		}
	case SCHED_SPECIAL_ATTACK1:
		{
			return &slSpecialAttack1[0];
		}
	case SCHED_SPECIAL_ATTACK2:
		{
			return &slSpecialAttack2[0];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slTakeCoverFromBestSound[0];
		}
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slTakeCoverFromEnemy[0];
		}
	case SCHED_COWER:
		{
			return &slCower[0];
		}
	case SCHED_AMBUSH:
		{
			return &slAmbush[0];
		}
	case SCHED_BARNACLE_VICTIM_GRAB:
		{
			return &slBarnacleVictimGrab[0];
		}
	case SCHED_BARNACLE_VICTIM_CHOMP:
		{
			return &slBarnacleVictimChomp[0];
		}
	case SCHED_INVESTIGATE_SOUND:
		{
			return &slInvestigateSound[0];
		}
	case SCHED_INVESTIGATE_SPOT:
		{
			return &slInvestigateSpot[0];
		}
	case SCHED_MOVE_TO_SPOT:
		{
			return &slMoveToSpot[0];
		}
	case SCHED_DIE:
		{
			return &slDie[0];
		}
	case SCHED_TAKE_COVER_FROM_ORIGIN:
		{
			return &slTakeCoverFromOrigin[0];
		}
	case SCHED_TAKE_COVER_FROM_SPOT:
		{
			return &slTakeCoverFromSpot[0];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slVictoryDance[0];
		}
	case SCHED_FAIL:
		{
			if (npc_combat_fail_schedule.value != 0 && m_MonsterState == MONSTERSTATE_COMBAT && m_hEnemy != 0)
				return slCombatFail;
			return slFail;
		}
	case SCHED_FREEROAM:
		{
			return slFreeroam;
		}
	case SCHED_FREEROAM_ALERT:
		{
			return slFreeroamAlert;
		}
	case SCHED_MOVE_TO_ENEMY_LKP:
		{
			return slMoveToEnemyLKP;
		}
	case SCHED_RETREAT_FROM_ENEMY:
		{
			return slRetreatFromEnemy;
		}
	case SCHED_RETREAT_FROM_ENEMY_FAILED:
		{
			return GetScheduleOfType(SCHED_FAIL);
		}
	case SCHED_RETREAT_FROM_SPOT:
		{
			return slRetreatFromSpot;
		}
	case SCHED_RETREAT_FROM_SPOT_FAILED:
		{
			MakeMyBlockerMoveAway();
			return GetScheduleOfType(SCHED_FAIL);
		}
	case SCHED_IDLE_FACE:
		{
			return slIdleFace;
		}
	case SCHED_REGENERATION:
		{
			return slRegen;
		}
	case SCHED_REPEL_LAND:
		{
			if (HasMemory(bits_MEMORY_SPAWNED_FROM_AIRCRAFT))
				return slRepelLandAndMove;
			else
				return slRepelLand;
		}
	default:
		{
			ALERT( at_console, "GetScheduleOfType()\nNo CASE for Schedule Type %d!\n", Type );

			return &slIdleStand[0];
			break;
		}
	}

	return NULL;
}
