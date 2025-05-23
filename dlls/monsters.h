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
#pragma once
#if !defined(MONSTERS_H)
#include "skill.h"
#define MONSTERS_H

#include "cbase.h"
#include "hitgroup.h"

/*

===== monsters.h ========================================================

  Header file for monster-related utility code

*/

// CHECKLOCALMOVE result types 
#define	LOCALMOVE_INVALID					0 // move is not possible
#define LOCALMOVE_INVALID_DONT_TRIANGULATE	1 // move is not possible, don't try to triangulate
#define LOCALMOVE_VALID						2 // move is possible

// Monster Spawnflags
#define	SF_MONSTER_WAIT_TILL_SEEN		1// spawnflag that makes monsters wait until player can see them before attacking.
#define	SF_MONSTER_GAG				2 // no idle noises from this monster
#define SF_MONSTER_HITMONSTERCLIP		4
//										8
#define SF_MONSTER_PRISONER			16 // monster won't attack anyone, no one will attacke him.
//										32
//										64
#define	SF_MONSTER_WAIT_FOR_SCRIPT		128 //spawnflag that makes monsters wait to check for attacking until the script is done or they've been attacked
#define SF_MONSTER_NO_YELLOW_BLOBS_SPIRIT 128 // Wait for script is not used anyway. Add compatibility with spirit.
#define SF_MONSTER_PREDISASTER			256	//this is a predisaster scientist or barney. Influences how they speak.
#define SF_MONSTER_FADECORPSE			512 // Fade out corpse after death
#define SF_MONSTER_DONT_DROP_GUN		1024
#define SF_MONSTER_NO_YELLOW_BLOBS		( 1 << 13 )
#define SF_MONSTER_SPECIAL_FLAG			( 1 << 15 )
#define SF_MONSTER_NONSOLID_CORPSE		( 1 << 16 )
#define SF_MONSTER_IGNORE_PLAYER_PUSH	( 1 << 19 )
#define SF_MONSTER_ACT_OUT_OF_PVS		( 1 << 20 )

#define SF_MONSTER_FALL_TO_GROUND		0x80000000

#define SF_DEADMONSTER_DONT_DROP 2 // dead corpse don't fall on the ground

// specialty spawnflags
#define SF_MONSTER_TURRET_AUTOACTIVATE	32
#define SF_MONSTER_TURRET_STARTINACTIVE	64
#define SF_MONSTER_WAIT_UNTIL_PROVOKED	64 // don't attack the player unless provoked

// MoveToOrigin stuff
#define MOVE_START_TURN_DIST		64 // when this far away from moveGoal, start turning to face next goal
#define MOVE_STUCK_DIST			32 // if a monster can't step this far, it is stuck.

// MoveToOrigin stuff
#define MOVE_NORMAL			0// normal move in the direction monster is facing
#define MOVE_STRAFE			1// moves in direction specified, no matter which way monster is facing

// spawn flags 256 and above are already taken by the engine
extern void UTIL_MoveToOrigin( edict_t* pent, const Vector &vecGoal, float flDist, int iMoveType ); 

Vector VecCheckToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj = 1.0 );
Vector VecCheckThrow( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0 );
extern DLL_GLOBAL Vector g_vecAttackDir;
extern void EjectBrass(const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype );
extern void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count );

bool FBoxVisible( entvars_t *pevLooker, entvars_t *pevTarget, Vector &vecTargetOrigin, float flSize = 0.0 );

// these bits represent the monster's memory
#define MEMORY_CLEAR					0
#define bits_MEMORY_PROVOKED			( 1 << 0 )// right now only used for houndeyes.
#define bits_MEMORY_INCOVER				( 1 << 1 )// monster knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS			( 1 << 2 )// Ally is suspicious of the player, and will move to provoked more easily
#define bits_MEMORY_PATH_FINISHED		( 1 << 3 )// Finished monster path (just used by big momma for now)
#define bits_MEMORY_ON_PATH				( 1 << 4 )// Moving on a path
#define bits_MEMORY_MOVE_FAILED			( 1 << 5 )// Movement has already failed
#define bits_MEMORY_FLINCHED			( 1 << 6 )// Has already flinched
#define bits_MEMORY_KILLED				( 1 << 7 )// HACKHACK -- remember that I've already called my Killed()
#define bits_MEMORY_SHOULD_ROAM_IN_ALERT	( 1 << 8 )
#define bits_MEMORY_ACTIVE_AFTER_COMBAT	( 1 << 9 )
#define bits_MEMORY_SHOULD_GO_TO_LKP	( 1 << 10 )
#define bits_MEMORY_BLOCKER_IS_ENEMY	( 1 << 11 )
#define bits_MEMORY_GOT_HEALED_RECENTLY	( 1 << 12 )
#define bits_MEMORY_CUSTOM5				( 1 << 27 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM4				( 1 << 28 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM3				( 1 << 29 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM2				( 1 << 30 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM1				( 1 << 31 )	// Monster-specific memory

// trigger conditions for scripted AI
// these MUST match the CHOICES interface in halflife.fgd for the base monster
enum 
{
	AITRIGGER_NONE = 0,
	AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER,
	AITRIGGER_TAKEDAMAGE,
	AITRIGGER_HALFHEALTH,
	AITRIGGER_DEATH,
	AITRIGGER_SQUADMEMBERDIE,
	AITRIGGER_SQUADLEADERDIE,
	AITRIGGER_HEARWORLD,
	AITRIGGER_HEARPLAYER,
	AITRIGGER_HEARCOMBAT,
	AITRIGGER_SEEPLAYER_UNCONDITIONAL,
	AITRIGGER_SEEPLAYER_NOT_IN_COMBAT,
	AITRIGGER_PROVOKED_BY_PLAYER,
	AITRIGGER_GOTHEALED
};
/*
		0 : "No Trigger"
		1 : "See Player"
		2 : "Take Damage"
		3 : "50% Health Remaining"
		4 : "Death"
		5 : "Squad Member Dead"
		6 : "Squad Leader Dead"
		7 : "Hear World"
		8 : "Hear Player"
		9 : "Hear Combat"
*/

#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks

#define FINDSPOTAWAY_WALK 0
#define FINDSPOTAWAY_CHECK_SPOT 1
#define FINDSPOTAWAY_RUN 2
#define FINDSPOTAWAY_TRACE_LOOKER 4

#define SUGGEST_SCHEDULE_FLAG_WALK ( 1 << 0 )
#define SUGGEST_SCHEDULE_FLAG_RUN ( 1 << 1 )
#define SUGGEST_SCHEDULE_FLAG_SPOT_IS_POSITION ( 1 << 2 )
#define SUGGEST_SCHEDULE_FLAG_SPOT_IS_ENTITY ( 1 << 3 )

// utility flags
#define SUGGEST_SCHEDULE_FLAG_SPOT_IS_INVALID ( 1 << 24 )
#define SUGGEST_SCHEDULE_FLAG_SPOT_ENTITY_IS_PROVIDED ( 1 << 25 )

enum
{
	GIBBING_POLICY_DEFAULT = 0,
	GIBBING_POLICY_PREFER_GIB,
	GIBBING_POLICY_PREFER_NOGIB,
};

//
// A gib is a chunk of a body, or a piece of wood/metal/rocks/etc.
//
class CGib : public CBaseEntity
{
public:
	void Spawn( const char *szGibModel, const Visual* visual = nullptr );
	void FinalizeGibSpawn();
	void EXPORT BounceGibTouch( CBaseEntity *pOther );
	void EXPORT StickyGibTouch( CBaseEntity *pOther );
	void EXPORT WaitTillLand( void );
	void EXPORT StartFadeOut ( void );
	void LimitVelocity( void );

	virtual int ObjectCaps( void ) { return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_DONT_SAVE; }
	static void SpawnHeadGib( entvars_t *pevVictim, const Visual* visual = nullptr );
	static void SpawnHumanGibs(entvars_t *pevVictim, int cGibs = 4, const Visual* visual = nullptr );
	static void SpawnRandomGibs( entvars_t *pevVictim, int cGibs, const char* gibModel, int gibBodiesNum = 0, int startGibIndex = 0, const Visual* visual = nullptr );
	static void SpawnRandomGibs( entvars_t *pevVictim, int cGibs, const char* gibModel, const Visual* visual );
	static void SpawnStickyGibs( entvars_t *pevVictim, Vector vecOrigin, int cGibs );
	static void SpawnRandomClientGibs(entvars_t *pevVictim, int cGibs, const char* gibModel, int gibBodiesNum = 0, int startGibIndex = 0 );

	int m_bloodColor;
	int m_cBloodDecals;
	int m_material;
	float m_lifeTime;
	float m_bornTime;
};

void AddScoreForDamage(entvars_t *pevAttacker, CBaseEntity* victim, const float damage);

#define CUSTOM_SCHEDULES\
		virtual Schedule_t *ScheduleFromName( const char *pName );\
		static Schedule_t *m_scheduleList[];

#define DEFINE_CUSTOM_SCHEDULES(derivedClass)\
	Schedule_t *derivedClass::m_scheduleList[] =

#define IMPLEMENT_CUSTOM_SCHEDULES(derivedClass, baseClass)\
		Schedule_t *derivedClass::ScheduleFromName( const char *pName )\
		{\
			Schedule_t *pSchedule = ScheduleInList( pName, m_scheduleList, ARRAYSIZE( m_scheduleList ) );\
			if( !pSchedule )\
				return baseClass::ScheduleFromName( pName );\
			return pSchedule;\
		}
#endif	//MONSTERS_H
