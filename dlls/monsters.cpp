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

===== monsters.cpp ========================================================

  Monster-related utility code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "monsters.h"
#include "animation.h"
#include "saverestore.h"
#include "combat.h"
#include "global_models.h"
#include "scripted.h"
#include "followingmonster.h"
#include "decals.h"
#include "soundent.h"
#include "gamerules.h"
#include "mod_features.h"
#include "game.h"
#include "common_soundscripts.h"
#include "visuals_utils.h"
#include "inventory.h"
#include "classify.h"
#include "studio.h"
#include "clamp.h"
#include "ai_debug.h"
#include "graphic_debug.h"

#define MONSTER_CUT_CORNER_DIST		8 // 8 means the monster's bounding box is contained without the box of the node in WC

Vector VecBModelOrigin( entvars_t *pevBModel );

static void ReportRouteType(ALERT_TYPE level, int routeType)
{
	if (routeType & bits_MF_TO_TARGETENT)
		ALERT(level, "To TargetEnt; ");
	if (routeType & bits_MF_TO_ENEMY)
		ALERT(level, "To Enemy; ");
	if (routeType & bits_MF_TO_DETOUR)
		ALERT(level, "Detour; ");
	if (routeType & bits_MF_TO_PATHCORNER)
		ALERT(level, "Path Corner; ");
	if (routeType & bits_MF_TO_NODE)
		ALERT(level, "Node; ");
	if (routeType & bits_MF_TO_LOCATION)
		ALERT(level, "Location; ");
	if (routeType & bits_MF_IS_GOAL)
		ALERT(level, "Goal; ");
	if (routeType & bits_MF_DONT_SIMPLIFY)
		ALERT(level, "Don't simplify; ");
	if (routeType & bits_MF_NEAREST_PATH)
		ALERT(level, "Nearest; ");
}

// Global Savedata for monster
// UNDONE: Save schedule data?  Can this be done?  We may
// lose our enemy pointer or other data (goal ent, target, etc)
// that make the current schedule invalid, perhaps it's best
// to just pick a new one when we start up again.
TYPEDESCRIPTION	CBaseMonster::m_SaveData[] =
{
	DEFINE_FIELD( CBaseMonster, m_hEnemy, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hTargetEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hMoveGoalEnt, FIELD_EHANDLE ),
	DEFINE_ARRAY( CBaseMonster, m_hOldEnemy, FIELD_EHANDLE, MAX_OLD_ENEMIES ),
	DEFINE_ARRAY( CBaseMonster, m_vecOldEnemy, FIELD_POSITION_VECTOR, MAX_OLD_ENEMIES ),
	DEFINE_FIELD( CBaseMonster, m_flFieldOfView, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flWaitFinished, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_flMoveWaitFinished, FIELD_TIME ),

	DEFINE_FIELD( CBaseMonster, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_IdealActivity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_LastHitGroup, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_MonsterState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_IdealMonsterState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_iTaskStatus, FIELD_INTEGER ),

	//Schedule_t			*m_pSchedule;

	DEFINE_FIELD( CBaseMonster, m_iScheduleIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afConditions, FIELD_INTEGER ),
	//WayPoint_t			m_Route[ ROUTE_SIZE ];
	//DEFINE_FIELD( CBaseMonster, m_movementGoal, FIELD_INTEGER ),
	//DEFINE_FIELD( CBaseMonster, m_iRouteIndex, FIELD_INTEGER ),
	//DEFINE_FIELD( CBaseMonster, m_moveWaitTime, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseMonster, m_vecMoveGoal, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_movementActivity, FIELD_INTEGER ),

	//int					m_iAudibleList; // first index of a linked list of sounds that the monster can hear.
	//DEFINE_FIELD( CBaseMonster, m_afSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_vecLastPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_iHintNode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afMemory, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_vecEnemyLKP, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_cAmmoLoaded, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afCapability, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_bitsDamageType, FIELD_INTEGER ),
	DEFINE_ARRAY( CBaseMonster, m_rgbTimeBasedDamage, FIELD_CHARACTER, CDMG_TIMEBASED ),
	DEFINE_FIELD( CBaseMonster, m_bloodColor, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_failSchedule, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_flHungryTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_flDistTooFar, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flDistLook, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_iTriggerCondition, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_iTriggerAltCondition, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_iszTriggerTarget, FIELD_STRING ),

	DEFINE_FIELD( CBaseMonster, m_HackedGunPos, FIELD_VECTOR ),

	DEFINE_FIELD( CBaseMonster, m_scriptState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_pCine, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBaseMonster, m_iClass, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_gibModel, FIELD_STRING ),
	DEFINE_FIELD( CBaseMonster, m_reverseRelationship, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_glowShellTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_glowShellUpdate, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBaseMonster, m_prevRenderAmt, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_prevRenderColor, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_prevRenderFx, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_prevRenderMode, FIELD_SHORT ),

	DEFINE_FIELD( CBaseMonster, m_nextPatrolPathCheck, FIELD_TIME ),

	DEFINE_FIELD( CBaseMonster, m_customSoundMask, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_prisonerTo, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_ignoredBy, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_freeRoam, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_activeAfterCombat, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_huntActivitiesCount, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_flLastTimeObservedEnemy, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_sizeForGrapple, FIELD_SHORT ),

	DEFINE_FIELD( CBaseMonster, m_suggestedSchedule, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_suggestedScheduleEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_suggestedScheduleOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_suggestedScheduleMinDist, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_suggestedScheduleMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_suggestedScheduleFlags, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_gibPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CBaseMonster, m_bForceConditionsGather, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_equalDislikeTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_lootRandomSeed, FIELD_INTEGER ),
};

//IMPLEMENT_SAVERESTORE( CBaseMonster, CBaseToggle )

int CBaseMonster::Save( CSave &save )
{
	if( !CBaseToggle::Save( save ) )
		return 0;
	return save.WriteFields( "CBaseMonster", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CBaseMonster::Restore( CRestore &restore )
{
	if( !CBaseToggle::Restore( restore ) )
		return 0;
	int status = restore.ReadFields( "CBaseMonster", this, m_SaveData, ARRAYSIZE( m_SaveData ) );

	// We don't save/restore routes yet
	RouteClear();

	// We don't save/restore schedules yet
	m_pSchedule = NULL;
	m_iTaskStatus = TASKSTATUS_NEW;

	// Reset animation
	m_Activity = ACT_RESET;

	// If we don't have an enemy, clear conditions like see enemy, etc.
	if( m_hEnemy == 0 )
		m_afConditions = 0;

	return status;
}

//=========================================================
// Eat - makes a monster full for a little while.
//=========================================================
void CBaseMonster::Eat( float flFullDuration )
{
	m_flHungryTime = gpGlobals->time + flFullDuration;
}

//=========================================================
// FShouldEat - returns true if a monster is hungry.
//=========================================================
bool CBaseMonster::FShouldEat( void )
{
	if( m_flHungryTime > gpGlobals->time )
	{
		return false;
	}
	return true;
}

//=========================================================
// BarnacleVictimBitten - called
// by Barnacle victims when the barnacle pulls their head
// into its mouth
//=========================================================
void CBaseMonster::BarnacleVictimBitten( entvars_t *pevBarnacle )
{
	Schedule_t	*pNewSchedule;

	pNewSchedule = GetScheduleOfType( SCHED_BARNACLE_VICTIM_CHOMP );

	if( pNewSchedule )
	{
		ChangeSchedule( pNewSchedule );
	}
}

//=========================================================
// BarnacleVictimReleased - called by barnacle victims when
// the host barnacle is killed.
//=========================================================
void CBaseMonster::BarnacleVictimReleased( void )
{
	m_IdealMonsterState = MONSTERSTATE_IDLE;

	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_STEP;
}

//=========================================================
// Listen - monsters dig through the active sound list for
// any sounds that may interest them. (smells, too!)
//=========================================================
void CBaseMonster::Listen( void )
{
	int	iSound;
	int	iMySounds;
	float	hearingSensitivity;
	CSound	*pCurrentSound;

	m_iAudibleList = SOUNDLIST_EMPTY; 
	ClearConditions( bits_COND_HEAR_SOUND | bits_COND_SMELL | bits_COND_SMELL_FOOD );
	m_afSoundTypes = 0;

	iMySounds = ISoundMask();

	if( m_pSchedule )
	{
		//!!!WATCH THIS SPOT IF YOU ARE HAVING SOUND RELATED BUGS!
		// Make sure your schedule AND personal sound masks agree!
		iMySounds &= m_pSchedule->iSoundMask;
	}

	iSound = CSoundEnt::ActiveList();

	// UNDONE: Clear these here?
	ClearConditions( bits_COND_HEAR_SOUND | bits_COND_SMELL_FOOD | bits_COND_SMELL );
	hearingSensitivity = HearingSensitivity();

	while( iSound != SOUNDLIST_EMPTY )
	{
		pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );

		if( pCurrentSound &&
			( pCurrentSound->m_iType & iMySounds )	&&
			( pCurrentSound->m_vecOrigin - EarPosition() ).IsLengthLessThanOrEqual(pCurrentSound->m_iVolume * hearingSensitivity) )

		//if( ( g_pSoundEnt->m_SoundPool[iSound].m_iType & iMySounds ) && ( g_pSoundEnt->m_SoundPool[iSound].m_vecOrigin - EarPosition()).Length () <= g_pSoundEnt->m_SoundPool[iSound].m_iVolume * hearingSensitivity )
		{
 			// the monster cares about this sound, and it's close enough to hear.
			//g_pSoundEnt->m_SoundPool[iSound].m_iNextAudible = m_iAudibleList;
			pCurrentSound->m_iNextAudible = m_iAudibleList;

			if( pCurrentSound->FIsSound() )
			{
				// this is an audible sound.
				SetConditions( bits_COND_HEAR_SOUND );
			}
			else
			{
				// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
				//if( g_pSoundEnt->m_SoundPool[iSound].m_iType & ( bits_SOUND_MEAT | bits_SOUND_CARCASS ) )
				if( pCurrentSound->m_iType & ( bits_SOUND_MEAT | bits_SOUND_CARCASS ) )
				{
					// the detected scent is a food item, so set both conditions.
					// !!!BUGBUG - maybe a virtual function to determine whether or not the scent is food?
					SetConditions( bits_COND_SMELL_FOOD );
					SetConditions( bits_COND_SMELL );
				}
				else
				{
					// just a normal scent. 
					SetConditions( bits_COND_SMELL );
				}
			}
			//m_afSoundTypes |= g_pSoundEnt->m_SoundPool[iSound].m_iType;
			m_afSoundTypes |= pCurrentSound->m_iType;

			m_iAudibleList = iSound;
		}

		//iSound = g_pSoundEnt->m_SoundPool[iSound].m_iNext;
		if( pCurrentSound )
			iSound = pCurrentSound->m_iNext;
	}
}

//=========================================================
// FLSoundVolume - subtracts the volume of the given sound
// from the distance the sound source is from the caller, 
// and returns that value, which is considered to be the 'local' 
// volume of the sound. 
//=========================================================
float CBaseMonster::FLSoundVolume( CSound *pSound )
{
	return ( pSound->m_iVolume - ( ( pSound->m_vecOrigin - pev->origin ).Length() ) );
}

//=========================================================
// FValidateHintType - tells use whether or not the monster cares
// about the type of Hint Node given
//=========================================================
bool CBaseMonster::FValidateHintType( short sHint )
{
	return false;
}

//=========================================================
// Look - Base class monster function to find enemies or 
// food by sight. iDistance is distance ( in units ) that the 
// monster can see.
//
// Sets the sight bits of the m_afConditions mask to indicate
// which types of entities were sighted.
// Function also sets the Looker's m_pLink 
// to the head of a link list that contains all visible ents.
// (linked via each ent's m_pLink field)
//
//=========================================================
void CBaseMonster::Look( int iDistance )
{
	int iSighted = 0;

	// DON'T let visibility information from last frame sit around!
	ClearConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR | bits_COND_SEE_NEMESIS | bits_COND_SEE_CLIENT );

	m_pLink = NULL;

	CBaseEntity *pSightEnt = NULL;// the current visible entity that we're dealing with

	// See no evil if prisoner is set
	if( !FBitSet( pev->spawnflags, SF_MONSTER_PRISONER ) )
	{
		CBaseEntity *pList[100];

		Vector delta = Vector( iDistance, iDistance, iDistance );

		// Find only monsters/clients in box, NOT limited to PVS
		int count = UTIL_EntitiesInBox( pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT | FL_MONSTER );
		for( int i = 0; i < count; i++ )
		{
			pSightEnt = pList[i];
			// !!!temporarily only considering other monsters and clients, don't see prisoners
			if( pSightEnt != this && 
				 !FBitSet( pSightEnt->pev->spawnflags, SF_MONSTER_PRISONER ) && 
				 (!m_prisonerTo || m_prisonerTo != pSightEnt->Classify()) &&
				 pSightEnt->pev->health > 0 )
			{
				const int myClassify = Classify();
				CBaseMonster* pSightMonster = pSightEnt->MyMonsterPointer();
				if (pSightMonster)
				{
					if (pSightMonster->m_prisonerTo != 0 && pSightMonster->m_prisonerTo == myClassify)
						continue;
					if (pSightMonster->m_ignoredBy != 0 && pSightMonster->m_ignoredBy == myClassify)
						continue;
				}

				const int iRelationship = IRelationship( pSightEnt );
				// the looker will want to consider this entity
				// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
				if( iRelationship != R_NO && FInViewCone( pSightEnt ) && !FBitSet( pSightEnt->pev->flags, FL_NOTARGET ) && FVisible( pSightEnt ) )
				{
					if( pSightEnt->IsPlayer() )
					{
						if( pev->spawnflags & SF_MONSTER_WAIT_TILL_SEEN )
						{
							CBaseMonster *pClient = pSightEnt->MyMonsterPointer();

							// don't link this client in the list if the monster is wait till seen and the player isn't facing the monster
							if( pClient && !pClient->FInViewCone( this ) )
							{
								// we're not in the player's view cone. 
								continue;
							}
							else
							{
								// player sees us, become normal now.
								pev->spawnflags &= ~SF_MONSTER_WAIT_TILL_SEEN;
							}
						}

						// if we see a client, remember that (mostly for scripted AI)
						iSighted |= bits_COND_SEE_CLIENT;
					}

					pSightEnt->m_pLink = m_pLink;
					m_pLink = pSightEnt;

					if( pSightEnt == m_hEnemy )
					{
						// we know this ent is visible, so if it also happens to be our enemy, store that now.
						iSighted |= bits_COND_SEE_ENEMY;
					}

					// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
					// we see monsters other than the Enemy.
					switch( iRelationship )
					{
					case R_NM:
						iSighted |= bits_COND_SEE_NEMESIS;		
						break;
					case R_HT:
						iSighted |= bits_COND_SEE_HATE;		
						break;
					case R_DL:
						iSighted |= bits_COND_SEE_DISLIKE;
						break;
					case R_FR:
						iSighted |= bits_COND_SEE_FEAR;
						break;
					case R_AL:
						break;
					default:
						ALERT( at_aiconsole, "%s can't assess %s\n", STRING( pev->classname ), STRING( pSightEnt->pev->classname ) );
						break;
					}
				}
			}
		}
	}

	SetConditions( iSighted );
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CBaseMonster::DefaultISoundMask( void )
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER;
}

int CBaseMonster::ISoundMask()
{
	if (m_customSoundMask == 0)
		return DefaultISoundMask();
	if (m_customSoundMask == -1)
		return 0;
	if (FBitSet(m_customSoundMask, bits_SOUND_REMOVE_FROM_DEFAULT))
	{
		int defaultMask = DefaultISoundMask();
		return defaultMask & ~m_customSoundMask;
	}
	return m_customSoundMask;
}

//=========================================================
// PBestSound - returns a pointer to the sound the monster 
// should react to. Right now responds only to nearest sound.
//=========================================================
CSound *CBaseMonster::PBestSound( void )
{	
	int iThisSound; 
	int iBestSound = -1;
	float flBestDist = 8192;// so first nearby sound will become best so far.
	float flDist;
	CSound *pSound;

	iThisSound = m_iAudibleList; 

	if( iThisSound == SOUNDLIST_EMPTY )
	{
		ALERT( at_aiconsole, "ERROR! monster %s has no audible sounds!\n", STRING( pev->classname ) );
#if _DEBUG
		ALERT( at_error, "NULL Return from PBestSound\n" );
#endif
		return NULL;
	}

	while( iThisSound != SOUNDLIST_EMPTY )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iThisSound );

		if( pSound )
		{
			if( pSound->FIsSound() )
			{
				flDist = ( pSound->m_vecOrigin - EarPosition() ).Length();

				if( flDist < flBestDist )
				{
					iBestSound = iThisSound;
					flBestDist = flDist;
				}
			}

			iThisSound = pSound->m_iNextAudible;
		}
	}
	if( iBestSound >= 0 )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iBestSound );
		return pSound;
	}
#if _DEBUG
	ALERT( at_error, "NULL Return from PBestSound\n" );
#endif
	return NULL;
}

//=========================================================
// PBestScent - returns a pointer to the scent the monster 
// should react to. Right now responds only to nearest scent
//=========================================================
CSound *CBaseMonster::PBestScent( void )
{
	int iThisScent; 
	int iBestScent = -1;
	float flBestDist = 8192;// so first nearby smell will become best so far.
	float flDist;
	CSound *pSound;

	iThisScent = m_iAudibleList;// smells are in the sound list.

	if( iThisScent == SOUNDLIST_EMPTY )
	{
		ALERT( at_aiconsole, "ERROR! PBestScent() has empty soundlist!\n" );
#if _DEBUG
		ALERT( at_error, "NULL Return from PBestSound\n" );
#endif
		return NULL;
	}

	while( iThisScent != SOUNDLIST_EMPTY )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iThisScent );

		if( pSound->FIsScent() )
		{
			flDist = ( pSound->m_vecOrigin - pev->origin ).Length();

			if( flDist < flBestDist )
			{
				iBestScent = iThisScent;
				flBestDist = flDist;
			}
		}

		iThisScent = pSound->m_iNextAudible;
	}
	if( iBestScent >= 0 )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iBestScent );

		return pSound;
	}
#if _DEBUG
	ALERT( at_error, "NULL Return from PBestScent\n" );
#endif
	return NULL;
}

//=========================================================
// Monster Think - calls out to core AI functions and handles this
// monster's specific animation events
//=========================================================
void CBaseMonster::MonsterThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;// keep monster thinking.

	RunAI();
	GlowShellUpdate();

	float flInterval = StudioFrameAdvance( ); // animate

	// start or end a fidget
	// This needs a better home -- switching animations over time should be encapsulated on a per-activity basis
	// perhaps MaintainActivity() or a ShiftAnimationOverTime() or something.
	if( m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_DEAD && m_Activity == ACT_IDLE && m_fSequenceFinished )
	{
		int iSequence;

		if( m_fSequenceLoops )
		{
			// animation does loop, which means we're playing subtle idle. Might need to 
			// fidget.
			iSequence = LookupActivity( m_Activity );
		}
		else
		{
			// animation that just ended doesn't loop! That means we just finished a fidget
			// and should return to our heaviest weighted idle (the subtle one)
			iSequence = LookupActivityHeaviest( m_Activity );
		}
		if( iSequence != ACTIVITY_NOT_AVAILABLE )
		{
			pev->sequence = iSequence;	// Set to new anim (if it's there)
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents( flInterval );

	if( !MovementIsComplete() )
	{
		Move( flInterval );
	}
#if _DEBUG	
	else 
	{
		if( !TaskIsRunning() && !TaskIsComplete() )
			ALERT( at_error, "Schedule stalled!!\n" );
	}
#endif
}

//=========================================================
// CBaseMonster - USE - will make a monster angry at whomever
// activated it.
//=========================================================
void CBaseMonster::MonsterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_MonsterState == MONSTERSTATE_IDLE)
		m_IdealMonsterState = MONSTERSTATE_ALERT;
}

//=========================================================
// Ignore conditions - before a set of conditions is allowed
// to interrupt a monster's schedule, this function removes
// conditions that we have flagged to interrupt the current
// schedule, but may not want to interrupt the schedule every
// time. (Pain, for instance)
//=========================================================
int CBaseMonster::IgnoreConditions( void )
{
	int iIgnoreConditions = 0;

	if( !FShouldEat() )
	{
		// not hungry? Ignore food smell.
		iIgnoreConditions |= bits_COND_SMELL_FOOD;
	}

	if( m_MonsterState == MONSTERSTATE_SCRIPT && m_pCine )
		iIgnoreConditions |= m_pCine->IgnoreConditions();

	return iIgnoreConditions;
}

//=========================================================
// 	RouteClear - zeroes out the monster's route array and goal
//=========================================================
void CBaseMonster::RouteClear( void )
{
	RouteNew();
	m_movementGoal = MOVEGOAL_NONE;
	m_movementActivity = ACT_IDLE;
	Forget( bits_MEMORY_MOVE_FAILED );
}

//=========================================================
// Route New - clears out a route to be changed, but keeps
//				goal intact.
//=========================================================
void CBaseMonster::RouteNew( void )
{
	for (auto& route : m_Route)
	{
		route.iType = 0;
	}
	m_iRouteIndex = 0;
}

//=========================================================
// FRouteClear - returns true is the Route is cleared out
// ( invalid )
//=========================================================
bool CBaseMonster::FRouteClear( void )
{
	if( m_Route[m_iRouteIndex].iType == 0 || m_movementGoal == MOVEGOAL_NONE )
		return true;

	return false;
}

//=========================================================
// FRefreshRoute - after calculating a path to the monster's
// target, this function copies as many waypoints as possible
// from that path to the monster's Route array
//=========================================================
extern cvar_t npc_patrol;

bool CBaseMonster::FRefreshRoute( int buildRouteFlags )
{
	CBaseEntity	*pPathCorner;
	int		i;
	bool returnCode = false;

	RouteNew();

	switch( m_movementGoal )
	{
		case MOVEGOAL_PATHCORNER:
			{
				// monster is on a path_corner loop
				pPathCorner = m_pGoalEnt;

				if (npc_patrol.value)
				{
					if (pPathCorner)
					{
						returnCode = BuildRoute( pPathCorner->pev->origin, bits_MF_TO_PATHCORNER, NULL, buildRouteFlags );
					}
				}
				else
				{
					i = 0;
					while( pPathCorner && i < ROUTE_SIZE )
					{
						m_Route[i].iType = bits_MF_TO_PATHCORNER;
						m_Route[i].vecLocation = pPathCorner->pev->origin;

						pPathCorner = pPathCorner->GetNextTarget();

						// Last path_corner in list?
						if( !pPathCorner )
							m_Route[i].iType |= bits_MF_IS_GOAL;
						i++;
					}
					returnCode = true;
				}
			}
			break;
		case MOVEGOAL_ENEMY:
		case MOVEGOAL_ENEMY_NEAREST:
			returnCode = BuildRoute( m_vecEnemyLKP, m_movementGoal, m_hEnemy, buildRouteFlags );
			break;
		case MOVEGOAL_LOCATION:
		case MOVEGOAL_LOCATION_NEAREST:
			returnCode = BuildRoute( m_vecMoveGoal, m_movementGoal, NULL, buildRouteFlags );
			break;
		case MOVEGOAL_TARGETENT:
		case MOVEGOAL_TARGETENT_NEAREST:
			if( m_hTargetEnt != 0 )
			{
				returnCode = BuildRoute( m_hTargetEnt->pev->origin, m_movementGoal, m_hTargetEnt, buildRouteFlags );
			}
			break;
		case MOVEGOAL_NODE:
			returnCode = FGetNodeRoute( m_vecMoveGoal );
			//if( returnCode )
			//	RouteSimplify( NULL );
			break;
	}

	return returnCode;
}

bool CBaseMonster::MoveToEnemy( Activity movementAct, float waitTime )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_ENEMY;
	return FRefreshRoute();
}

bool CBaseMonster::MoveToLocation( Activity movementAct, float waitTime, const Vector &goal, int buildRouteFlags )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_LOCATION;
	m_vecMoveGoal = goal;
	return FRefreshRoute(buildRouteFlags);
}

bool CBaseMonster::MoveToLocationClosest( Activity movementAct, float waitTime, const Vector &goal, int buildRouteFlags )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_LOCATION_NEAREST;
	m_vecMoveGoal = goal;
	return FRefreshRoute(buildRouteFlags);
}

bool CBaseMonster::MoveToTarget( Activity movementAct, float waitTime, bool closest )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = closest ? MOVEGOAL_TARGETENT_NEAREST : MOVEGOAL_TARGETENT;
	return FRefreshRoute();
}

bool CBaseMonster::MoveToNode( Activity movementAct, float waitTime, const Vector &goal )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_NODE;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}

void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b, int life = 1 )
{
	int i;

	if( m_Route[m_iRouteIndex].iType == 0 )
	{
		ALERT( at_aiconsole, "Can't draw route!\n" );
		return;
	}

	//UTIL_ParticleEffect ( m_Route[m_iRouteIndex].vecLocation, g_vecZero, 255, 25 );
	DrawBeamLine( pev->origin, m_Route[m_iRouteIndex].vecLocation, r, g, b, life, 16 );

	for( i = m_iRouteIndex; i < ROUTE_SIZE - 1; i++ )
	{
		if( ( m_Route[i].iType & bits_MF_IS_GOAL ) || ( m_Route[i + 1].iType == 0 ) )
			break;

		DrawBeamLine( m_Route[m_iRouteIndex].vecLocation, m_Route[i + 1].vecLocation, r, g, b, life, 8 );
		//UTIL_ParticleEffect( m_Route[i].vecLocation, g_vecZero, 255, 25 );
	}
}

void DrawRoute(CBaseMonster* pMonster, int iMoveFlag)
{
	int r, g, b;
	if (iMoveFlag & bits_MF_TO_ENEMY)
	{
		r = 255; g = 85; b = 0;
	}
	else if (iMoveFlag & bits_MF_TO_TARGETENT)
	{
		r = 0; g = 255; b = 127;
	}
	else
	{
		r = 255; g = 255; b = 255;
	}
	DrawRoute(pMonster->pev, pMonster->m_Route, pMonster->m_iRouteIndex, r, g, b, 25);
}

bool ShouldSimplify( int routeType )
{
	routeType &= ~bits_MF_IS_GOAL;

	if( ( routeType == bits_MF_TO_PATHCORNER ) || ( routeType & bits_MF_DONT_SIMPLIFY ) )
		return false;
	return true;
}

//=========================================================
// RouteSimplify
//
// Attempts to make the route more direct by cutting out
// unnecessary nodes & cutting corners.
//
//=========================================================
void CBaseMonster::RouteSimplify( CBaseEntity *pTargetEnt )
{
	// BUGBUG: this doesn't work 100% yet
	int		i, count, outCount;
	Vector		vecStart;
	WayPoint_t	outRoute[ROUTE_SIZE * 2];	// Any points except the ends can turn into 2 points in the simplified route

	count = 0;

	for( i = m_iRouteIndex; i < ROUTE_SIZE; i++ )
	{
		if( !m_Route[i].iType )
			break;
		else
			count++;
		if( m_Route[i].iType & bits_MF_IS_GOAL )
			break;
	}
	// Can't simplify a direct route!
	if( count < 2 )
	{
		//DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
		return;
	}

	outCount = 0;
	vecStart = pev->origin;
	for( i = 0; i < count - 1; i++ )
	{
		// Don't eliminate path_corners
		if( !ShouldSimplify( m_Route[m_iRouteIndex + i].iType ) )
		{
			outRoute[outCount] = m_Route[m_iRouteIndex + i];
			outCount++;
		}
		else if( CheckLocalMove( vecStart, m_Route[m_iRouteIndex+i + 1].vecLocation, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			// Skip vert
			continue;
		}
		else
		{
			Vector vecTest, vecSplit;

			// Halfway between this and next
			vecTest = ( m_Route[m_iRouteIndex + i + 1].vecLocation + m_Route[m_iRouteIndex + i].vecLocation ) * 0.5f;

			// Halfway between this and previous
			vecSplit = ( m_Route[m_iRouteIndex + i].vecLocation + vecStart ) * 0.5f;

			int iType = ( m_Route[m_iRouteIndex + i].iType | bits_MF_TO_DETOUR ) & ~bits_MF_NOT_TO_MASK;
			if( CheckLocalMove( vecStart, vecTest, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecTest;
			}
			else if( CheckLocalMove( vecSplit, vecTest, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecSplit;
				outRoute[outCount+1].iType = iType;
				outRoute[outCount+1].vecLocation = vecTest;
				outCount++; // Adding an extra point
			}
			else
			{
				outRoute[outCount] = m_Route[m_iRouteIndex + i];
			}
		}
		// Get last point
		vecStart = outRoute[outCount].vecLocation;
		outCount++;
	}
	ASSERT( i < count );
	outRoute[outCount] = m_Route[m_iRouteIndex + i];
	outCount++;

	// Terminate
	outRoute[outCount].iType = 0;
	ASSERT( outCount < ( ROUTE_SIZE * 2 ) );

	// Copy the simplified route, disable for testing
	m_iRouteIndex = 0;
	for( i = 0; i < ROUTE_SIZE && i < outCount; i++ )
	{
		m_Route[i] = outRoute[i];
	}

	// Terminate route
	if( i < ROUTE_SIZE )
		m_Route[i].iType = 0;

// Debug, test movement code
#if 0
//	if( CVAR_GET_FLOAT( "simplify" ) != 0 )
		DrawRoute( pev, outRoute, 0, 255, 0, 0 );
//	else
		DrawRoute( pev, m_Route, m_iRouteIndex, 0, 255, 0 );
#endif
}

//=========================================================
// FBecomeProne - tries to send a monster into PRONE state.
// right now only used when a barnacle snatches someone, so 
// may have some special case stuff for that.
//=========================================================
bool CBaseMonster::FBecomeProne( void )
{
	if (m_pCine && !m_pCine->CanInterruptByBarnacle())
	{
		return false;
	}

	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		pev->flags -= FL_ONGROUND;
	}

	m_IdealMonsterState = MONSTERSTATE_PRONE;
	return true;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
bool CBaseMonster::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist > 64.0f && flDist <= 784.0f && flDot >= 0.5f )
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
bool CBaseMonster::CheckRangeAttack2( float flDot, float flDist )
{
	if( flDist > 64.0f && flDist <= 512.0f && flDot >= 0.5f )
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
bool CBaseMonster::CheckMeleeAttackImpl(float flDot, float flDist, const CheckMeleeAttackParams& defaults, bool meleeAttack2)
{
	const EntTemplate* entTemplate = GetMyEntTemplate();
	CheckMeleeAttackParams params = defaults;
	if (entTemplate)
	{
		EntTemplate::CheckMeleeAttack check = meleeAttack2 ? entTemplate->GetCheckMeleeAttack2() : entTemplate->GetCheckMeleeAttack1();
		if (check.distance)
		{
			params.distance = *check.distance;
		}
		if (check.dot)
		{
			params.dot = *check.dot;
		}
	}
	return flDist <= params.distance && flDot >= params.dot;
}

bool CBaseMonster::CheckMeleeAttack1( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	// Note: the check for FL_ONGROUND actually causes problems. E.g. zombies can't attack sentry turrets and flying enemies.
	// Hornets are not seen as enemies by everything except the machines, so they're not a problem at all.
	// Disabling this check for now.
	return CheckMeleeAttackImpl(flDot, flDist, CheckMeleeAttackParams(), false) && m_hEnemy != 0 /*&& FBitSet( m_hEnemy->pev->flags, FL_ONGROUND )*/;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
bool CBaseMonster::CheckMeleeAttack2( float flDot, float flDist )
{
	return CheckMeleeAttackImpl(flDot, flDist, CheckMeleeAttackParams(), true);
}

//=========================================================
// CheckAttacks - sets all of the bits for attacks that the
// monster is capable of carrying out on the passed entity.
//=========================================================
void CBaseMonster::CheckAttacks(CBaseEntity *pTarget, float flDist, float flMeleeDist)
{
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( pTarget->pev->origin - pev->origin ).Make2D();
	vec2LOS.NormalizeInPlace();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	// we know the enemy is in front now. We'll find which attacks the monster is capable of by
	// checking for corresponding Activities in the model file, then do the simple checks to validate
	// those attack types.

	// Clear all attack conditions
	ClearConditions( bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 |bits_COND_CAN_MELEE_ATTACK2 );

	if (!NpcFixMeleeDistance())
		flMeleeDist = flDist;

	if( m_afCapability & bits_CAP_RANGE_ATTACK1 )
	{
		if( CheckRangeAttack1( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK1 );
	}
	if( m_afCapability & bits_CAP_RANGE_ATTACK2 )
	{
		if( CheckRangeAttack2( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK2 );
	}
	if( m_afCapability & bits_CAP_MELEE_ATTACK1 )
	{
		if( CheckMeleeAttack1( flDot, flMeleeDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK1 );
	}
	if( m_afCapability & bits_CAP_MELEE_ATTACK2 )
	{
		if( CheckMeleeAttack2( flDot, flMeleeDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK2 );
	}
}

//=========================================================
// CanCheckAttacks - prequalifies a monster to do more fine
// checking of potential attacks. 
//=========================================================
bool CBaseMonster::FCanCheckAttacks( void )
{
	if( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return true;
	}
	return false;
}

bool CBaseMonster::WantsToGetCloseToEnemy()
{
	return !FBitSet(m_afCapability, bits_CAP_RANGE_ATTACK1|bits_CAP_RANGE_ATTACK2) && FBitSet(m_afCapability, bits_CAP_MELEE_ATTACK1|bits_CAP_MELEE_ATTACK1);
}

//=========================================================
// CheckEnemy - part of the Condition collection process,
// gets and stores data and conditions pertaining to a monster's
// enemy. Returns true if Enemy LKP was updated.
//=========================================================
bool CBaseMonster::CheckEnemy( CBaseEntity *pEnemy )
{
	bool iUpdatedLKP;// set this to true if you update the EnemyLKP in this function.

	iUpdatedLKP = false;
	ClearConditions( bits_COND_ENEMY_FACING_ME | bits_COND_ENEMY_LOST );

	CBaseEntity* pOccluder;
	if( !FVisible( pEnemy, &pOccluder ) )
	{
		ASSERT( !HasConditions( bits_COND_SEE_ENEMY ) );
		if (pOccluder)
		{
			if (PerceiveEnemyAsOccluded(pEnemy, pOccluder))
				SetConditions( bits_COND_ENEMY_OCCLUDED );
			else
				ClearConditions( bits_COND_ENEMY_OCCLUDED );
		}
		else
			SetConditions( bits_COND_ENEMY_OCCLUDED );
	}
	else
		ClearConditions( bits_COND_ENEMY_OCCLUDED );

	const bool enemyIsDead = g_modFeatures.monsters_stop_attacking_dying_monsters ? !pEnemy->IsFullyAlive() : !pEnemy->IsAlive();

	if( enemyIsDead )
	{
		SetConditions( bits_COND_ENEMY_DEAD );
		ClearConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED );
		return false;
	}

	// My enemy is not actually my enemy anymore or I became prisoner (e.g. via trigger_configure_monster)
	bool shouldLoseEnemy = IRelationship(pEnemy) < R_DL || FBitSet(pev->spawnflags, SF_MONSTER_PRISONER) || (m_prisonerTo != 0 && m_prisonerTo == pEnemy->Classify());
	if (!shouldLoseEnemy)
	{
		CBaseMonster* pMonster = pEnemy->MyMonsterPointer();
		// Check if my enemy became prisoner
		if (pMonster)
		{
			if (FBitSet(pMonster->pev->spawnflags, SF_MONSTER_PRISONER))
			{
				shouldLoseEnemy = true;
			}
			else if (pMonster->m_prisonerTo != 0 && pMonster->m_prisonerTo == Classify())
			{
				shouldLoseEnemy = true;
			}
		}
	}

	// Doesn't care about this enemy anymore. Pretend to lose the enemy.
	if (shouldLoseEnemy)
	{
		SetConditions(bits_COND_ENEMY_LOST);
		ClearConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED );
		return false;
	}

	const float myFeetLevel = pev->origin.z + pev->mins.z;
	const float myHeight = pev->maxs.z - pev->mins.z;
	const float enemyFeetLevel = pEnemy->pev->origin.z + pEnemy->pev->mins.z;
	const float enemyHeight = pEnemy->pev->maxs.z - pEnemy->pev->mins.z;

	float flDistToEnemy = 1000000.0f;

	Vector myPos = pev->origin;
	myPos.z = myFeetLevel;
	myPos.z += myHeight * 0.5f;

	for (int j=0; j<4; ++j)
	{
		Vector enemyPos = pEnemy->pev->origin;
		enemyPos.z = enemyFeetLevel;
		enemyPos.z += enemyHeight * 0.25f * j;

		const float flDistToEnemy2 = (enemyPos - myPos).Length();
		flDistToEnemy = Q_min(flDistToEnemy, flDistToEnemy2);
	}

	float maxSideSize;
	float minSideSize;

	if (pEnemy->pev->size.x >= pev->size.x)
	{
		maxSideSize = pEnemy->pev->size.x;
		minSideSize = pev->size.x;
	}
	else
	{
		maxSideSize = pev->size.x;
		minSideSize = pEnemy->pev->size.x;
	}
	const float flMeleeDist = flDistToEnemy - maxSideSize/2 + minSideSize/2;

	if( HasConditions( bits_COND_SEE_ENEMY ) )
	{
		CBaseMonster *pEnemyMonster;

		iUpdatedLKP = true;
		m_vecEnemyLKP = pEnemy->pev->origin;
		m_flLastTimeObservedEnemy = gpGlobals->time;

		pEnemyMonster = pEnemy->MyMonsterPointer();

		if( pEnemyMonster )
		{
			if( pEnemyMonster->FInViewCone( this ) )
			{
				SetConditions( bits_COND_ENEMY_FACING_ME );
			}
			else
				ClearConditions( bits_COND_ENEMY_FACING_ME );
		}

		if( pEnemy->pev->velocity != Vector( 0, 0, 0 ) )
		{
			// trail the enemy a bit
			m_vecEnemyLKP = m_vecEnemyLKP - pEnemy->pev->velocity * RANDOM_FLOAT( -0.05f, 0.0f );
		}
		else
		{
			// UNDONE: use pev->oldorigin?
		}
	}
	else if( !HasConditions( bits_COND_ENEMY_OCCLUDED | bits_COND_SEE_ENEMY ) && ( flDistToEnemy <= 256 ) )
	{
		// if the enemy is not occluded, and unseen, that means it is behind or beside the monster.
		// if the enemy is near enough the monster, we go ahead and let the monster know where the
		// enemy is. 
		iUpdatedLKP = true;
		m_vecEnemyLKP = pEnemy->pev->origin;
		m_flLastTimeObservedEnemy = gpGlobals->time;
	}
	else
	{
		const float forgetEnemyTime = NpcForgetEnemyTime();
		if (forgetEnemyTime > 0 && m_flLastTimeObservedEnemy + forgetEnemyTime <= gpGlobals->time)
		{
			SetConditions( bits_COND_ENEMY_LOST );
			ClearConditions( bits_COND_ENEMY_OCCLUDED );
			return false;
		}
	}

	if( flDistToEnemy >= m_flDistTooFar )
	{
		// enemy is very far away from monster
		SetConditions( bits_COND_ENEMY_TOOFAR );
	}
	else
		ClearConditions( bits_COND_ENEMY_TOOFAR );

	if( FCanCheckAttacks() )	
	{
		CheckAttacks( m_hEnemy, flDistToEnemy, flMeleeDist );
	}

	if( m_movementGoal == MOVEGOAL_ENEMY || m_movementGoal == MOVEGOAL_ENEMY_NEAREST )
	{
		for( int i = m_iRouteIndex; i < ROUTE_SIZE; i++ )
		{
			if( m_Route[i].iType == (bits_MF_IS_GOAL|bits_MF_TO_ENEMY) || m_Route[i].iType == (bits_MF_IS_GOAL|bits_MF_TO_ENEMY|bits_MF_NEAREST_PATH) )
			{
				// UNDONE: Should we allow monsters to override this distance (80?)
				if( ( m_Route[i].vecLocation - m_vecEnemyLKP ).IsLengthGreaterThan(80.0f) )
				{
					// I'm already standing almost on enemy LKP. No point in building a path to the same spot.
					// Set the current route point as enemy LKP instead
					if ((pev->origin - m_vecEnemyLKP).IsLengthLessThanOrEqual(48.0f) && (pev->origin - m_vecEnemyLKP).IsLength2DLessThanOrEqual(16.0f))
					{
						m_vecEnemyLKP = m_Route[m_iRouteIndex].vecLocation;
					}
					// Refresh
					FRefreshRoute();
					return iUpdatedLKP;
				}
			}
		}
	}

	return iUpdatedLKP;
}

// SetEnemy - set main enemy
void CBaseMonster::SetEnemy(CBaseEntity *pNewEnemy)
{
	if (!pNewEnemy || pNewEnemy == m_hEnemy)
		return;

	CBaseEntity* pPreviousEnemy = m_hEnemy;
	Vector previousEnemyLKP = m_vecEnemyLKP;

	m_hEnemy = pNewEnemy;
	m_vecEnemyLKP = pNewEnemy->pev->origin;
	m_flLastTimeObservedEnemy = gpGlobals->time;

	if (ShouldReportAIChange(entindex()))
	{
		ALERT(at_aiconsole, "%s (%d): got %s as new enemy. Previous enemy: %s\n", STRING(pev->classname), entindex(), STRING(pNewEnemy->pev->classname), pPreviousEnemy ? STRING(pPreviousEnemy->pev->classname) : "none");
	}

	// Don't keep the new enemy in the list of old enemies
	for( int i = 0; i < MAX_OLD_ENEMIES; i++ )
	{
		if( m_hOldEnemy[i] == pNewEnemy )
		{
			m_hOldEnemy[i] = NULL;
			break;
		}
	}
	PushEnemy(pPreviousEnemy, previousEnemyLKP);
}

//=========================================================
// PushEnemy - remember the last few enemies
//=========================================================
void CBaseMonster::PushEnemy( CBaseEntity *pEnemy, const Vector &vecLastKnownPos )
{
	int i;

	if( pEnemy == NULL )
		return;

	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for( i = 0; i < MAX_OLD_ENEMIES; i++ )
	{
		if( m_hOldEnemy[i] == pEnemy )
		{
			// we already have this enemy, just update LKP
			m_vecOldEnemy[i] = vecLastKnownPos;
			return;
		}
		if( m_hOldEnemy[i] == 0 || !m_hOldEnemy[i]->IsAlive() ) // someone died, reuse their slot
			break;
	}
	if( i >= MAX_OLD_ENEMIES )
		return;

	m_hOldEnemy[i] = pEnemy;
	m_vecOldEnemy[i] = vecLastKnownPos;
	//ALERT(at_aiconsole, "%s pushed %s to its enemy queue\n", STRING(pev->classname), STRING(pEnemy->pev->classname));
}

//=========================================================
// PopEnemy - try remembering the last few enemies
//=========================================================
bool CBaseMonster::PopEnemy()
{
	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for( int i = MAX_OLD_ENEMIES - 1; i >= 0; i-- )
	{
		if( m_hOldEnemy[i] != 0 )
		{
			if( m_hOldEnemy[i]->IsFullyAlive()) // cheat and know when they die
			{
				m_hEnemy = m_hOldEnemy[i];
				m_vecEnemyLKP = m_vecOldEnemy[i];
				m_flLastTimeObservedEnemy = gpGlobals->time;
				ALERT( at_aiconsole, "%s remembering old enemy %s\n", STRING(pev->classname), STRING(m_hEnemy->pev->classname) );
				m_hOldEnemy[i] = NULL;
				return true;
			}
			else
			{
				m_hOldEnemy[i] = NULL;
			}
		}
	}
	return false;
}

//=========================================================
// SetActivity 
//=========================================================
extern cvar_t npc_range_attack_unlooped;

void CBaseMonster::SetActivity( Activity NewActivity )
{
	int iSequence = LookupActivity( NewActivity );

	Activity OldActivity = m_Activity;
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if( !( OldActivity == ACT_WALK || OldActivity == ACT_RUN ) || !( NewActivity == ACT_WALK || NewActivity == ACT_RUN ) )
				pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();

		if (m_fSequenceLoops && m_Activity == ACT_RANGE_ATTACK1 && npc_range_attack_unlooped.value)
		{
			//ALERT(at_aiconsole, "%s: forcing attack animation not to loop\n", STRING(pev->classname));
			m_fSequenceLoops = false;
		}
	}
	else
	{
		// Not available try to get default anim
		ALERT( at_aiconsole, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// SetSequenceByName
//=========================================================
void CBaseMonster::SetSequenceByName( const char *szSequence )
{
	int iSequence;

	iSequence = LookupSequence( szSequence );

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT( at_aiconsole, "%s has no sequence named:%f\n", STRING(pev->classname), szSequence );
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// CheckLocalMove - returns true if the caller can walk a
// straight line from its current origin to the given 
// location. If so, don't use the node graph!
//
// if a valid pointer to a int is passed, the function
// will fill that int with the distance that the check 
// reached before hitting something. THIS ONLY HAPPENS
// IF THE LOCAL MOVE CHECK FAILS!
//
// !!!PERFORMANCE - should we try to load balance this?
// DON"T USE SETORIGIN! 
//=========================================================
#define	LOCAL_STEP_SIZE	16
int CBaseMonster::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	Vector vecStartPos;// record monster's position before trying the move
	float flYaw;
	float flDist;
	float flStep, stepSize;
	int iReturn;

	vecStartPos = pev->origin;

	flYaw = UTIL_VecToYaw( vecEnd - vecStart );// build a yaw that points to the goal.
	flDist = ( vecEnd - vecStart ).Length2D();// get the distance.
	iReturn = LOCALMOVE_VALID;// assume everything will be ok.

	// move the monster to the start of the local move that's to be checked.
	UTIL_SetOrigin( pev, vecStart );// !!!BUGBUG - won't this fire triggers? - nope, SetOrigin doesn't fire

	if( !( pev->flags & ( FL_FLY | FL_SWIM ) ) )
	{
		DROP_TO_FLOOR( ENT( pev ) );//make sure monster is on the floor!
	}

	//pev->origin.z = vecStartPos.z;//!!!HACKHACK

	//pev->origin = vecStart;

/*
	if( flDist > 1024 )
	{
		// !!!PERFORMANCE - this operation may be too CPU intensive to try checks this large.
		// We don't lose much here, because a distance this great is very likely
		// to have something in the way.

		// since we've actually moved the monster during the check, undo the move.
		pev->origin = vecStartPos;
		return false;
	}
*/
	// this loop takes single steps to the goal.
	for( flStep = 0; flStep < flDist; flStep += LOCAL_STEP_SIZE )
	{
		stepSize = LOCAL_STEP_SIZE;

		if( ( flStep + LOCAL_STEP_SIZE ) >= ( flDist - 1 ) )
			stepSize = ( flDist - flStep ) - 1;

		//UTIL_ParticleEffect( pev->origin, g_vecZero, 255, 25 );

		if( !WALK_MOVE( ENT( pev ), flYaw, stepSize, WALKMOVE_CHECKONLY ) )
		{
			// can't take the next step, fail!
			if( pflDist != NULL )
			{
				*pflDist = flStep;
			}
			if( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			{
				// if this step hits target ent, the move is legal.
				iReturn = LOCALMOVE_VALID;
				break;
			}
			else
			{
				// If we're going toward an entity, and we're almost getting there, it's OK.
				//if( pTarget && fabs( flDist - iStep ) < LOCAL_STEP_SIZE )
				//	fReturn = true;
				//else
				iReturn = LOCALMOVE_INVALID;
				break;
			}

		}
	}

	if( iReturn == LOCALMOVE_VALID && !( pev->flags & ( FL_FLY | FL_SWIM ) ) && ( !pTarget || ( pTarget->pev->flags & FL_ONGROUND ) ) )
	{
		// The monster can move to a spot UNDER the target, but not to it. Don't try to triangulate, go directly to the node graph.
		// UNDONE: Magic # 64 -- this used to be pev->size.z but that won't work for small creatures like the headcrab
		if( fabs( vecEnd.z - pev->origin.z ) > 64.0f )
		{
			iReturn = LOCALMOVE_INVALID_DONT_TRIANGULATE;
		}
	}
	/*
	// uncommenting this block will draw a line representing the nearest legal move.
	WRITE_BYTE( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( MSG_BROADCAST, TE_SHOWLINE );
	WRITE_COORD( MSG_BROADCAST, pev->origin.x );
	WRITE_COORD( MSG_BROADCAST, pev->origin.y );
	WRITE_COORD( MSG_BROADCAST, pev->origin.z );
	WRITE_COORD( MSG_BROADCAST, vecStart.x );
	WRITE_COORD( MSG_BROADCAST, vecStart.y );
	WRITE_COORD( MSG_BROADCAST, vecStart.z );
	*/

	// since we've actually moved the monster during the check, undo the move.
	UTIL_SetOrigin( pev, vecStartPos );

	return iReturn;
}

float CBaseMonster::OpenDoorAndWait( entvars_t *pevDoor )
{
	float flTravelTime = 0;

	//ALERT( at_aiconsole, "A door. " );
	CBaseEntity *pcbeDoor = CBaseEntity::Instance( pevDoor );
	if( pcbeDoor && !pcbeDoor->IsLockedByMaster() )
	{
		//ALERT( at_aiconsole, "unlocked! " );
		flTravelTime = pcbeDoor->InputByMonster(this);
		//ALERT( at_aiconsole, "pevDoor->nextthink = %d ms\n", (int)( 1000 * pevDoor->nextthink ) );
		//ALERT( at_aiconsole, "pevDoor->ltime = %d ms\n", (int)( 1000 * pevDoor->ltime ) );
		//ALERT( at_aiconsole, "pev-> nextthink = %d ms\n", (int)( 1000 * pev->nextthink ) );
		//ALERT( at_aiconsole, "pev->ltime = %d ms\n", (int)( 1000 * pev->ltime ) );
		//ALERT( at_aiconsole, "Waiting %d ms\n", (int)( 1000 * flTravelTime ) );
		if( pcbeDoor->pev->targetname )
		{
			edict_t *pentTarget = NULL;
			for( ; ; )
			{
				pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING(pcbeDoor->pev->targetname ) );

				if( VARS( pentTarget ) != pcbeDoor->pev )
				{
					if( FNullEnt( pentTarget ) )
						break;

					if( FClassnameIs( pentTarget, STRING( pcbeDoor->pev->classname ) ) )
					{
						CBaseEntity *pDoor = Instance( pentTarget );
						if( pDoor )
							pDoor->InputByMonster(this);
					}
				}
			}
		}
	}

	return gpGlobals->time + flTravelTime;
}

//=========================================================
// AdvanceRoute - poorly named function that advances the 
// m_iRouteIndex. If it goes beyond ROUTE_SIZE, the route 
// is refreshed. 
//=========================================================
void CBaseMonster::AdvanceRoute( float distance )
{
	if( m_iRouteIndex == ROUTE_SIZE - 1 )
	{
		// time to refresh the route.
		if( !FRefreshRoute() )
		{
			ALERT( at_aiconsole, "Can't Refresh Route!!\n" );
		}
	}
	else
	{
		if( !( m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL ) )
		{
			// If we've just passed a path_corner, advance m_pGoalEnt
			if( ( m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK ) == bits_MF_TO_PATHCORNER )
				m_pGoalEnt = m_pGoalEnt->GetNextTarget();

			// IF both waypoints are nodes, then check for a link for a door and operate it.
			//
			if( ( m_Route[m_iRouteIndex].iType & bits_MF_TO_NODE ) == bits_MF_TO_NODE
			   && ( m_Route[m_iRouteIndex + 1].iType & bits_MF_TO_NODE ) == bits_MF_TO_NODE )
			{
				//ALERT( at_aiconsole, "SVD: Two nodes. " );

				int iSrcNode  = WorldGraph.FindNearestNode( m_Route[m_iRouteIndex].vecLocation, this );
				int iDestNode = WorldGraph.FindNearestNode( m_Route[m_iRouteIndex + 1].vecLocation, this );

				int iLink;
				WorldGraph.HashSearch( iSrcNode, iDestNode, iLink );

				if( iLink >= 0 && WorldGraph.m_pLinkPool[iLink].m_pLinkEnt != NULL )
				{
					//ALERT( at_aiconsole, "A link. " );
					const int afCapMask = m_afCapability | (FBitSet(pev->flags, FL_MONSTERCLIP) ? bits_CAP_MONSTERCLIPPED : 0);
					if( WorldGraph.HandleLinkEnt( iSrcNode, WorldGraph.m_pLinkPool[iLink].m_pLinkEnt, afCapMask, CGraph::NODEGRAPH_DYNAMIC ) == NLE_NEEDS_INPUT )
					{
						//ALERT( at_aiconsole, "usable." );
						entvars_t *pevDoor = WorldGraph.m_pLinkPool[iLink].m_pLinkEnt;
						if( pevDoor )
						{
							m_flMoveWaitFinished = OpenDoorAndWait( pevDoor );
							//ALERT( at_aiconsole, "Wating for door %.2f\n", m_flMoveWaitFinished-gpGlobals->time );
						}
					}
				}
				//ALERT( at_aiconsole, "\n" );
			}
			m_iRouteIndex++;
		}
		else	// At goal!!!
		{
			if( distance < m_flGroundSpeed * 0.2f /* FIX */ )
			{
				if (m_pGoalEnt != 0 && m_Route[m_iRouteIndex].iType & bits_MF_TO_PATHCORNER)
				{
					m_nextPatrolPathCheck = gpGlobals->time + m_pGoalEnt->GetDelay();
					pev->ideal_yaw = m_pGoalEnt->pev->angles.y;
					m_pGoalEnt = m_pGoalEnt->GetNextTarget();
				}
				MovementComplete();
			}
		}
	}
}

int CBaseMonster::RouteClassify( int iMoveFlag )
{
	int movementGoal;

	movementGoal = MOVEGOAL_NONE;

	if( iMoveFlag & bits_MF_TO_TARGETENT )
	{
		movementGoal = MOVEGOAL_TARGETENT;
		if( iMoveFlag & bits_MF_NEAREST_PATH )
			movementGoal = MOVEGOAL_TARGETENT_NEAREST;
	}
	else if( iMoveFlag & bits_MF_TO_ENEMY )
	{
		movementGoal = MOVEGOAL_ENEMY;
		if( iMoveFlag & bits_MF_NEAREST_PATH )
			movementGoal = MOVEGOAL_ENEMY_NEAREST;
	}
	else if( iMoveFlag & bits_MF_TO_PATHCORNER )
		movementGoal = MOVEGOAL_PATHCORNER;
	else if( iMoveFlag & bits_MF_TO_NODE )
		movementGoal = MOVEGOAL_NODE;
	else if( iMoveFlag & bits_MF_TO_LOCATION )
	{
		movementGoal = MOVEGOAL_LOCATION;
		if( iMoveFlag & bits_MF_NEAREST_PATH )
			movementGoal = MOVEGOAL_LOCATION_NEAREST;
	}

	return movementGoal;
}

//=========================================================
// BuildRoute
//=========================================================
bool CBaseMonster::BuildRoute( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget, int buildRouteFlags )
{
	float flDist;
	Vector vecApexes[3];
	int iLocalMove;

	int triangDepth = 1;

	if (!FBitSet(buildRouteFlags, BUILDROUTE_NO_TRIDEPTH))
	{
		bool shouldApplyTridepth = TridepthForAll() || (m_MonsterState == MONSTERSTATE_SCRIPT);
		if (!shouldApplyTridepth)
		{
			CFollowingMonster* followingMonster = MyFollowingMonsterPointer();
			shouldApplyTridepth = followingMonster != 0 && followingMonster->IsFollowingPlayer();
		}

		if (shouldApplyTridepth)
		{
			triangDepth = TridepthValue();
			triangDepth = clamp(triangDepth, 1, static_cast<int>(ARRAYSIZE(vecApexes)));
		}
	}

	RouteNew();
	const bool nearest = FBitSet(iMoveFlag, bits_MF_NEAREST_PATH);
	m_movementGoal = RouteClassify( iMoveFlag );
	ClearBits(iMoveFlag, bits_MF_NEAREST_PATH);

	// so we don't end up with no moveflags
	m_Route[0].vecLocation = vecGoal;
	m_Route[0].iType = iMoveFlag | bits_MF_IS_GOAL;

	if (!FBitSet(buildRouteFlags, BUILDROUTE_NODEROUTE_ONLY))
	{
		// check simple local move
		iLocalMove = CheckLocalMove( pev->origin, vecGoal, pTarget, &flDist );

		if( iLocalMove == LOCALMOVE_VALID )
		{
			// monster can walk straight there!
			return true;
		}

		// try to triangulate around any obstacles.
		else if( iLocalMove != LOCALMOVE_INVALID_DONT_TRIANGULATE && !FBitSet(buildRouteFlags, BUILDROUTE_NO_TRIANGULATION) )
		{
			int result = FTriangulate( pev->origin, vecGoal, flDist, pTarget, vecApexes, triangDepth );
			if (result)
			{
				//ALERT(at_aiconsole, "Triangulated %d times\n", result);
				// there is a slightly more complicated path that allows the monster to reach vecGoal
				for (int i=0; i<result; ++i)
				{
					m_Route[i].vecLocation = vecApexes[i];
					m_Route[i].iType = (iMoveFlag | bits_MF_TO_DETOUR);
				}
				m_Route[result].vecLocation = vecGoal;
				m_Route[result].iType = iMoveFlag | bits_MF_IS_GOAL;

				RouteSimplify( pTarget );
				return true;
			}
		}
	}

	auto nearestBuildRoute = [&]()
	{
		if (nearest && !FBitSet(buildRouteFlags, BUILDROUTE_NODEROUTE_ONLY))
		{
			SetBits(iMoveFlag, bits_MF_NEAREST_PATH);

			const Vector localMoveNearest = pev->origin + (vecGoal - pev->origin).Normalize() * flDist;

			Vector apex;
			const Vector triangulatedNearest = FTriangulateToNearest(pev->origin, vecGoal, flDist, pTarget, apex);

			if ((vecGoal - triangulatedNearest).Length2DSqr() < (vecGoal - localMoveNearest).Length2DSqr())
			{
				if ((apex - triangulatedNearest).IsLength2DLessThan(1))
				{
					if ((triangulatedNearest - pev->origin).IsLength2DGreaterThanOrEqual(1.0f))
					{
						m_Route[0].vecLocation = triangulatedNearest;
						m_Route[0].iType = iMoveFlag | bits_MF_IS_GOAL;
						m_vecMoveGoal = triangulatedNearest;
						return true;
					}
				}
				else
				{
					if ((triangulatedNearest - pev->origin).IsLength2DGreaterThanOrEqual(1.0f))
					{
						m_Route[0].vecLocation = apex;
						m_Route[0].iType = (iMoveFlag | bits_MF_TO_DETOUR);

						m_Route[1].vecLocation = triangulatedNearest;
						m_Route[1].iType = iMoveFlag | bits_MF_IS_GOAL;

						RouteSimplify( pTarget );
						m_vecMoveGoal = triangulatedNearest;
						return true;
					}
				}
			}
			else
			{
				// Prevent playing walk/run animation while standing in place
				if ((localMoveNearest - pev->origin).IsLength2DGreaterThanOrEqual(1.0f))
				{
					m_Route[0].vecLocation = localMoveNearest;
					m_Route[0].iType = iMoveFlag | bits_MF_IS_GOAL;

					m_vecMoveGoal = localMoveNearest;
					return true;
				}
			}
		}
		return false;
	};

	// If monster want to nearest route to enemy, it means he wants to get closer, so do this check before nodes
	if (FBitSet(iMoveFlag, bits_MF_TO_ENEMY))
	{
		if (nearestBuildRoute())
		{
			return true;
		}
	}

	// last ditch, try nodes
	if( !FBitSet(buildRouteFlags, BUILDROUTE_NO_NODEROUTE) && FGetNodeRoute( vecGoal, iMoveFlag & (bits_MF_TO_TARGETENT|bits_MF_TO_ENEMY) ) )
	{
		//ALERT( at_console, "Can get there on nodes\n" );
		m_vecMoveGoal = vecGoal;
		RouteSimplify( pTarget );
		return true;
	}

	if (!FBitSet(iMoveFlag, bits_MF_TO_ENEMY))
	{
		if (nearestBuildRoute())
		{
			return true;
		}
	}

	// b0rk
	return false;
}

//=========================================================
// InsertWaypoint - Rebuilds the existing route so that the
// supplied vector and moveflags are the first waypoint in
// the route, and fills the rest of the route with as much
// of the pre-existing route as possible
//=========================================================
void CBaseMonster::InsertWaypoint( Vector vecLocation, int afMoveFlags )
{
	int i, type;

	// we have to save some Index and Type information from the real
	// path_corner or node waypoint that the monster was trying to reach. This makes sure that data necessary 
	// to refresh the original path exists even in the new waypoints that don't correspond directy to a path_corner
	// or node. 
	type = afMoveFlags | ( m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK );

	for( i = ROUTE_SIZE - 1; i > 0; i-- )
		m_Route[i] = m_Route[i - 1];

	m_Route[m_iRouteIndex].vecLocation = vecLocation;
	m_Route[m_iRouteIndex].iType = type;
}

//=========================================================
// FTriangulate - tries to overcome local obstacles by 
// triangulating a path around them.
//
// iApexDist is how far the obstruction that we are trying
// to triangulate around is from the monster.
//=========================================================
int CBaseMonster::FTriangulate( const Vector &vecStart, const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApexes, int n, int tries, bool recursive )
{
	Vector		vecDir;
	Vector		vecLeft;// the spot we'll try to triangulate to on the left
	Vector		vecRight;// the spot we'll try to triangulate to on the right
	Vector		vecTop;// the spot we'll try to triangulate to on the top
	Vector		vecBottom;// the spot we'll try to triangulate to on the bottom
	float		sizeX, sizeZ;

	// If the hull width is less than 24, use 24 because CheckLocalMove uses a min of
	// 24.
	sizeX = pev->size.x;
	if( sizeX < 24.0f )
		sizeX = 24.0f;
	else if( sizeX > 48.0f )
		sizeX = 48.0f;
	sizeZ = pev->size.z;
	//if( sizeZ < 24.0f )
	//	sizeZ = 24.0f;

	const Vector vecForward = ( vecEnd - vecStart ).Normalize();

	Vector vecDirUp( 0, 0, 1 );
	vecDir = CrossProduct( vecForward, vecDirUp );

	// start checking right about where the object is, picking two equidistant starting points, one on
	// the left, one on the right. As we progress through the loop, we'll push these away from the obstacle, 
	// hoping to find a way around on either side. pev->size.x is added to the ApexDist in order to help select
	// an apex point that insures that the monster is sufficiently past the obstacle before trying to turn back
	// onto its original course.

	vecLeft = vecStart + ( vecForward * ( flDist + ( recursive ? 0 : sizeX ) ) ) - vecDir * ( recursive ? sizeX : sizeX * 3 );
	vecRight = vecStart + ( vecForward * ( flDist + ( recursive ? 0 : sizeX ) ) ) + vecDir * ( recursive ? sizeX : sizeX * 3 );
	if( pev->movetype == MOVETYPE_FLY )
	{
		vecTop = vecStart + ( vecForward * flDist ) + ( vecDirUp * (recursive ? sizeZ : sizeZ * 3) );
		vecBottom = vecStart + ( vecForward * flDist ) - ( vecDirUp * (recursive ? sizeZ : sizeZ * 3) );
	}

	const Vector vecFarSide = vecEnd;//m_Route[m_iRouteIndex].vecLocation; // since we use recursion these are not always the same anymore

	vecDir *= sizeX * 2;
	if( pev->movetype == MOVETYPE_FLY )
		vecDirUp *= sizeZ * 2;

	for( int i = 0; i < tries; i++ )
	{
// Debug, Draw the triangulation
#if 0
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_VECTOR( pev->origin );
			WRITE_VECTOR( vecRight );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE );
			WRITE_VECTOR( pev->origin );
			WRITE_VECTOR( vecLeft );
		MESSAGE_END();
#endif
#if 0
		if( pev->movetype == MOVETYPE_FLY )
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_VECTOR( pev->origin );
				WRITE_VECTOR( vecTop );
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_VECTOR( pev->origin );
				WRITE_VECTOR( vecBottom );
			MESSAGE_END();
		}
#endif
		int result = 0;
		float localMoveDist;
		if( CheckLocalMove( vecStart, vecRight, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
		{
			if( CheckLocalMove( vecRight, vecFarSide, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
			{
				*pApexes = vecRight;
				return 1;
			}
			else if (n>1)
			{
				result = FTriangulate(vecRight, vecFarSide, localMoveDist, pTargetEnt, pApexes+1, n-1, tries - 2, true);
				if (result)
				{
					*pApexes = vecRight;
					return result+1;
				}
			}
		}
		else if (n>1)
		{
			if( CheckLocalMove( vecRight, vecFarSide, pTargetEnt, nullptr ) == LOCALMOVE_VALID )
			{
				result = FTriangulate(vecStart, vecRight, localMoveDist, pTargetEnt, pApexes, n-1, tries - 2, true);
				if (result)
				{
					pApexes[n-1] = vecRight;
					return result+1;
				}
			}
		}
		if( CheckLocalMove( vecStart, vecLeft, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
		{
			if( CheckLocalMove( vecLeft, vecFarSide, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
			{
				*pApexes = vecLeft;
				return 1;
			}
			else if (n>1)
			{
				result = FTriangulate(vecLeft, vecFarSide, localMoveDist, pTargetEnt, pApexes+1, n-1, tries - 2, true);
				if (result)
				{
					*pApexes = vecLeft;
					return result+1;
				}
			}
		}
		else if (n>1)
		{
			if( CheckLocalMove( vecLeft, vecFarSide, pTargetEnt, nullptr ) == LOCALMOVE_VALID )
			{
				result = FTriangulate(vecStart, vecLeft, localMoveDist, pTargetEnt, pApexes, n-1, tries - 2, true);
				if (result)
				{
					pApexes[n-1] = vecLeft;
					return result+1;
				}
			}
		}

		if( pev->movetype == MOVETYPE_FLY )
		{
			const bool applyTridepth = TridepthVertical();

			if( CheckLocalMove( vecStart, vecTop, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID)
			{
				if( CheckLocalMove ( vecTop, vecFarSide, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
				{
					*pApexes = vecTop;
					return 1;
				}
				else if (n>1 && applyTridepth)
				{
					result = FTriangulate(vecTop, vecFarSide, localMoveDist, pTargetEnt, pApexes+1, n-1, tries - 3, true);
					if (result)
					{
						*pApexes = vecTop;
						return result+1;
					}
				}
			}
			else if (n>1 && applyTridepth)
			{
				if( CheckLocalMove( vecTop, vecFarSide, pTargetEnt, nullptr ) == LOCALMOVE_VALID )
				{
					result = FTriangulate(vecStart, vecTop, localMoveDist, pTargetEnt, pApexes, n-1, tries - 3, true);
					if (result)
					{
						pApexes[n-1] = vecTop;
						return result+1;
					}
				}
			}
			if( CheckLocalMove( vecStart, vecBottom, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
			{
				if( CheckLocalMove( vecBottom, vecFarSide, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
				{
					*pApexes = vecBottom;
					return 1;
				}
				else if (n>1 && applyTridepth)
				{
					result = FTriangulate(vecBottom, vecFarSide, localMoveDist, pTargetEnt, pApexes+1, n-1, tries - 3, true);
					if (result)
					{
						*pApexes = vecBottom;
						return result+1;
					}
				}
			}
			else if (n>1 && applyTridepth)
			{
				if( CheckLocalMove( vecBottom, vecFarSide, pTargetEnt, nullptr ) == LOCALMOVE_VALID )
				{
					result = FTriangulate(vecStart, vecBottom, localMoveDist, pTargetEnt, pApexes, n-1, tries - 3, true);
					if (result)
					{
						pApexes[n-1] = vecBottom;
						return result+1;
					}
				}
			}
		}

		vecRight += vecDir;
		vecLeft -= vecDir;
		if( pev->movetype == MOVETYPE_FLY )
		{
			vecTop += vecDirUp;
			vecBottom -= vecDirUp;
		}
	}

	return 0;
}

Vector CBaseMonster::FTriangulateToNearest(const Vector &vecStart , const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector& apex)
{
	Vector		vecDir;
	Vector		vecForward;
	Vector		vecLeft;// the spot we'll try to triangulate to on the left
	Vector		vecRight;// the spot we'll try to triangulate to on the right
	Vector		vecTop;// the spot we'll try to triangulate to on the top
	Vector		vecBottom;// the spot we'll try to triangulate to on the bottom
	Vector		vecFarSide;// the spot that we'll move to after hitting the triangulated point, before moving on to our normal goal.
	int		i;
	float		sizeX, sizeZ;

	// If the hull width is less than 24, use 24 because CheckLocalMove uses a min of
	// 24.
	sizeX = pev->size.x;
	if( sizeX < 24.0 )
		sizeX = 24.0;
	else if( sizeX > 48.0 )
		sizeX = 48.0;
	sizeZ = pev->size.z;
	//if( sizeZ < 24.0 )
	//	sizeZ = 24.0;

	vecForward = ( vecEnd - vecStart ).Normalize();

	Vector vecDirUp( 0, 0, 1 );
	vecDir = CrossProduct( vecForward, vecDirUp );

	// start checking right about where the object is, picking two equidistant starting points, one on
	// the left, one on the right. As we progress through the loop, we'll push these away from the obstacle,
	// hoping to find a way around on either side. pev->size.x is added to the ApexDist in order to help select
	// an apex point that insures that the monster is sufficiently past the obstacle before trying to turn back
	// onto its original course.

	vecLeft = vecStart + ( vecForward * ( flDist ) ) - vecDir * ( sizeX * 2 );
	vecRight = vecStart + ( vecForward * ( flDist ) ) + vecDir * ( sizeX * 2 );
	if( pev->movetype == MOVETYPE_FLY )
	{
		vecTop = vecStart + ( vecForward * flDist ) + ( vecDirUp * sizeZ * 3 );
		vecBottom = vecStart + ( vecForward * flDist ) - ( vecDirUp *  sizeZ * 3 );
	}

	vecFarSide = vecEnd;

	vecDir *= sizeX * 2;
	if( pev->movetype == MOVETYPE_FLY )
		vecDirUp *= sizeZ * 2;

	const int tries = 8;
	Vector vecNearest = vecStart;
	Vector vecTest;
	Vector vecBestApex = vecStart;
	float localMoveDist;
	for( i = 0; i < tries; i++ )
	{
		if( CheckLocalMove( vecStart, vecRight, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
		{
			if( CheckLocalMove( vecRight, vecFarSide, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
			{
				apex = vecRight;
				return vecFarSide;
			}
			else
			{
				vecTest = vecRight + (vecFarSide - vecRight).Normalize() * localMoveDist;
				if ((vecTest - vecFarSide).Length2DSqr() < (vecNearest - vecFarSide).Length2DSqr())
				{
					vecNearest = vecTest;
					vecBestApex = vecRight;
				}
			}
		}
		else
		{
			vecTest = vecStart + (vecRight - vecStart).Normalize() * localMoveDist;
			if ((vecTest - vecFarSide).Length2DSqr() < (vecNearest - vecFarSide).Length2DSqr())
			{
				vecNearest = vecTest;
				vecBestApex = vecNearest;
			}
		}
		if( CheckLocalMove( vecStart, vecLeft, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
		{
			if( CheckLocalMove( vecLeft, vecFarSide, pTargetEnt, &localMoveDist ) == LOCALMOVE_VALID )
			{
				apex = vecLeft;
				return vecFarSide;
			}
			else
			{
				vecTest = vecLeft + (vecFarSide - vecLeft).Normalize() * localMoveDist;
				if ((vecTest - vecFarSide).Length2DSqr() < (vecNearest - vecFarSide).Length2DSqr())
				{
					vecNearest = vecTest;
					vecBestApex = vecLeft;
				}
			}
		}
		else
		{
			vecTest = vecStart + (vecLeft - vecStart).Normalize() * localMoveDist;
			if ((vecTest - vecFarSide).Length2DSqr() < (vecNearest - vecFarSide).Length2DSqr())
			{
				vecNearest = vecTest;
				vecBestApex = vecNearest;
			}
		}

		if( pev->movetype == MOVETYPE_FLY )
		{
			if( CheckLocalMove( vecStart, vecTop, pTargetEnt, NULL ) == LOCALMOVE_VALID)
			{
				if( CheckLocalMove ( vecTop, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
				{
					apex = vecTop;
					return vecFarSide;
				}
			}
			if( CheckLocalMove( vecStart, vecBottom, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				if( CheckLocalMove( vecBottom, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
				{
					apex = vecBottom;
					return vecFarSide;
				}
			}
		}

		vecRight += vecDir;
		vecLeft -= vecDir;
		if( pev->movetype == MOVETYPE_FLY )
		{
			vecTop += vecDirUp;
			vecBottom -= vecDirUp;
		}
	}

	apex = vecBestApex;
	return vecNearest;
}

//=========================================================
// Move - take a single step towards the next ROUTE location
//=========================================================
#define DIST_TO_CHECK	200

void CBaseMonster::Move( float flInterval ) 
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if( FRouteClear() )
	{
		// If we still have a movement goal, then this is probably a route truncated by SimplifyRoute()
		// so refresh it.
		if( m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute() )
		{
			TaskFail("tried to move with no route");
			return;
		}
	}

	if( m_flMoveWaitFinished > gpGlobals->time )
		return;

// Debug, test movement code
#if 0
//	if( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 200, 0 );
#endif
	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	// local move to waypoint.
	vecDir = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Normalize();
	flWaypointDist = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Length2D();

	MakeIdealYaw( m_Route[m_iRouteIndex].vecLocation );
	ChangeYaw( pev->yaw_speed );

	// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
	if( flWaypointDist < DIST_TO_CHECK )
	{
		flCheckDist = flWaypointDist;
	}
	else
	{
		flCheckDist = DIST_TO_CHECK;
	}

	if( ( m_Route[m_iRouteIndex].iType & ( ~(bits_MF_NOT_TO_MASK|bits_MF_NEAREST_PATH) ) ) == bits_MF_TO_ENEMY )
	{
		// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
		pTargetEnt = m_hEnemy;
	}
	else if( ( m_Route[m_iRouteIndex].iType & ~(bits_MF_NOT_TO_MASK|bits_MF_NEAREST_PATH) ) == bits_MF_TO_TARGETENT )
	{
		pTargetEnt = m_hTargetEnt;
	}

	// !!!BUGBUG - CheckDist should be derived from ground speed.
	// If this fails, it should be because of some dynamic entity blocking this guy.
	// We've already checked this path, so we should wait and time out if the entity doesn't move
	flDist = 0;
	if( CheckLocalMove( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
	{
		// Can't move, stop
		Stop();

		// Blocking entity is in global trace_ent
		CBaseEntity *pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
		if( pBlocker )
		{
			DispatchBlocked( edict(), pBlocker->edict() );
		}

		if( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && ( gpGlobals->time-m_flMoveWaitFinished ) > 3.0f )
		{
			// Can we still move toward our target?
			if( flDist < m_flGroundSpeed )
			{
				// No, Wait for a second
				m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
				return;
			}
			// Ok, still enough room to take a step
		}
		else 
		{
			// try to triangulate around whatever is in the way.
			if( FTriangulate( pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist, pTargetEnt, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
				RouteSimplify( pTargetEnt );
			}
			else
			{
				//ALERT( at_aiconsole, "Couldn't Triangulate\n" );
				Stop();

				// Only do this once until your route is cleared
				if( m_moveWaitTime > 0 && !( m_afMemory & bits_MEMORY_MOVE_FAILED ) )
				{
					FRefreshRoute();
					if( FRouteClear() )
					{
						TaskFail("route is empty");
					}
					else
					{
						// Don't get stuck
						if( ( gpGlobals->time - m_flMoveWaitFinished ) < 0.2f )
							Remember( bits_MEMORY_MOVE_FAILED );

						m_flMoveWaitFinished = gpGlobals->time + 0.1f;

						if (m_pCine || m_hTargetEnt != 0)
						{
							if (HandleBlocker(pBlocker, true))
								MakeMyBlockerMoveAway();
						}
					}
				}
				else
				{
					if ((m_movementGoal == MOVEGOAL_ENEMY || m_movementGoal == MOVEGOAL_ENEMY_NEAREST) && pBlocker && pBlocker == m_hEnemy)
					{
						Remember(bits_MEMORY_BLOCKER_IS_ENEMY);
					}

					HandleBlocker(pBlocker, true);
					if (m_pCine) {
						m_pCine->OnMoveFail();
					}
					TaskFail("failed to move");
					if (DeveloperModeLevel() >= 4 && pBlocker && pBlocker->entindex() != 0)
					{
						ALERT(at_aiconsole, "%s: failed to move. Blocker is %s. Target is %s. Route waypoint type: ",
							STRING(pev->classname),
							STRING(pBlocker->pev->classname),
							pTargetEnt ? STRING(pTargetEnt->pev->classname) : "null");
						ReportRouteType(at_aiconsole, m_Route[m_iRouteIndex].iType);
						ALERT(at_aiconsole, "Schedule is \"%s\"\n", m_pSchedule ? m_pSchedule->pName : "null");
					}
					//ALERT( at_aiconsole, "%s Failed to move (%d)!\n", STRING( pev->classname ), HasMemory( bits_MEMORY_MOVE_FAILED ) );
					//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, ( pev->origin + ( vecDir * flCheckDist ) ).z, m_Route[m_iRouteIndex].vecLocation.z );
				}
				return;
			}
		}
	}

	// close enough to the target, now advance to the next target. This is done before actually reaching
	// the target so that we get a nice natural turn while moving.
	if( ShouldAdvanceRoute( flWaypointDist ) )///!!!BUGBUG- magic number
	{
		AdvanceRoute( flWaypointDist );
	}

	// Might be waiting for a door
	if( m_flMoveWaitFinished > gpGlobals->time )
	{
		Stop();
		return;
	}

	// UNDONE: this is a hack to quit moving farther than it has looked ahead.
	if( flCheckDist < m_flGroundSpeed * flInterval )
	{
		flInterval = flCheckDist / m_flGroundSpeed;
		// ALERT( at_console, "%.02f\n", flInterval );
	}
	MoveExecute( pTargetEnt, vecDir, flInterval );

	if( MovementIsComplete() )
	{
		Stop();
		RouteClear();
	}
}

bool CBaseMonster::ShouldAdvanceRoute( float flWaypointDist )
{
	if( flWaypointDist <= MONSTER_CUT_CORNER_DIST )
	{
		// ALERT( at_console, "cut %f\n", flWaypointDist );
		return true;
	}

	return false;
}

void CBaseMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	//float flYaw = UTIL_VecToYaw( m_Route[m_iRouteIndex].vecLocation - pev->origin );// build a yaw that points to the goal.
	//WALK_MOVE( ENT( pev ), flYaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );
	if( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	float flStep;
	while( flTotal > 0.001f )
	{
		// don't walk more than 16 units or stairs stop working
		flStep = Q_min( 16.0f, flTotal );
		UTIL_MoveToOrigin( ENT( pev ), m_Route[m_iRouteIndex].vecLocation, flStep, MOVE_NORMAL );
		flTotal -= flStep;
	}
	// ALERT( at_console, "dist %f\n", m_flGroundSpeed * pev->framerate * flInterval );
}

//=========================================================
// MonsterInit - after a monster is spawned, it needs to 
// be dropped into the world, checked for mobility problems,
// and put on the proper path, if any. This function does
// all of those things after the monster spawns. Any
// initialization that should take place for all monsters
// goes here.
//=========================================================
void CBaseMonster::MonsterInit( void )
{
	if( !g_pGameRules->FAllowMonsters() )
	{
		pev->flags |= FL_KILLME;		// Post this because some monster code modifies class data after calling this function
		//REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	// Set fields common to all monsters
	pev->effects		= 0;
	pev->takedamage		= DAMAGE_AIM;
	pev->ideal_yaw		= pev->angles.y;
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_NO;
	m_IdealMonsterState	= MONSTERSTATE_IDLE;// Assume monster will be idle, until proven otherwise

	m_IdealActivity = ACT_IDLE;

	SetBits( pev->flags, FL_MONSTER );
	if( pev->spawnflags & SF_MONSTER_HITMONSTERCLIP )
		pev->flags |= FL_MONSTERCLIP;

	ClearSchedule();
	RouteClear();
	InitBoneControllers( ); // FIX: should be done in Spawn

	m_iHintNode = NO_NODE;

	m_afMemory = MEMORY_CLEAR;

	m_hEnemy = NULL;

	m_flDistTooFar = 1024.0f;
	m_flDistLook = 2048.0f;

	// set eye position
	SetEyePosition();

	SetThink( &CBaseMonster::MonsterInitThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	SetUse( &CBaseMonster::MonsterUse );

	InitRandomSeeds();
}

void CBaseMonster::InitRandomSeeds()
{
	m_lootRandomSeed = RANDOM_LONG((1<<20), (1<<30));
	if (m_lootRandomSeed % 2 == 0)
		m_lootRandomSeed++;
}

//=========================================================
// MonsterInitThink - Calls StartMonster. Startmonster is 
// virtual, but this function cannot be 
//=========================================================
void CBaseMonster::MonsterInitThink( void )
{
	StartMonster();
}

Schedule_t* CBaseMonster::StartPatrol(CBaseEntity *path)
{
	if (path)
	{
		// JAY: How important is this error message?  Big Momma doesn't obey this rule, so I took it out.
#if 0
			// At this point, we expect only a path_corner as initial goal
			if( !FClassnameIs( m_pGoalEnt->pev, "path_corner" ) )
			{
				ALERT( at_warning, "ReadyMonster--monster's initial goal '%s' is not a path_corner\n", STRING( pev->target ) );
			}
#endif
		m_pGoalEnt = path;

		// Monster will start turning towards his destination
		MakeIdealYaw( m_pGoalEnt->pev->origin );

		// set the monster up to walk a path corner path.
		// !!!BUGBUG - this is a minor bit of a hack.
		// JAYJAY
		m_movementGoal = MOVEGOAL_PATHCORNER;

		if( pev->movetype == MOVETYPE_FLY )
			m_movementActivity = ACT_FLY;
		else if (m_pGoalEnt->pev->speed < 200)
			m_movementActivity = ACT_WALK;
		else
			m_movementActivity = ACT_RUN;

		if( FRefreshRoute() )
		{
			if (m_movementActivity == ACT_RUN)
				return GetScheduleOfType( SCHED_IDLE_RUN );
			else
				return GetScheduleOfType( SCHED_IDLE_WALK );
		}
		else
		{
			ALERT( at_aiconsole, "%s: couldn't create route. Can't patrol\n", STRING(pev->classname) );
		}
	}
	else
	{
		ALERT( at_error, "ReadyMonster()--%s couldn't find target %s\n", STRING( pev->classname ), STRING( pev->target ) );
	}
	return NULL;
}

//=========================================================
// StartMonster - final bit of initization before a monster 
// is turned over to the AI. 
//=========================================================
void CBaseMonster::StartMonster( void )
{
	// update capabilities
	if( LookupActivity( ACT_RANGE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK1;
	}
	if( LookupActivity( ACT_RANGE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK2;
	}
	if( LookupActivity( ACT_MELEE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK1;
	}
	if( LookupActivity( ACT_MELEE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK2;
	}

	// Raise monster off the floor one unit, then drop to floor
	if( pev->movetype != MOVETYPE_FLY && !FBitSet( pev->spawnflags, SF_MONSTER_FALL_TO_GROUND ) )
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR( ENT( pev ) );

		if (!FBitSet(pev->spawnflags, SF_MONSTER_NO_YELLOW_BLOBS|SF_MONSTER_NO_YELLOW_BLOBS_SPIRIT))
		{
			// Try to move the monster to make sure it's not stuck in a brush.
			if( !WALK_MOVE( ENT( pev ), 0, 0, WALKMOVE_NORMAL ) )
			{
				ALERT( at_error, "Monster %s stuck in wall--level design error\n", STRING( pev->classname ) );
				if( IsDeveloperModeOn() )
					pev->effects |= EF_BRIGHTFIELD;
			}
		}
	}
	else 
	{
		pev->flags &= ~FL_ONGROUND;
	}
	
	if( !FStringNull( pev->target ) )// this monster has a target
	{
		// Find the monster's initial target entity, stash it
		CBaseEntity* path = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) ) );

		Schedule_t* patrolSchedule = StartPatrol(path);
		if (patrolSchedule)
		{
			SetState( MONSTERSTATE_IDLE );
			ChangeSchedule( patrolSchedule );
		}
	}

	//SetState( m_IdealMonsterState );
	//SetActivity( m_IdealActivity );

	// Delay drop to floor to make sure each door in the level has had its chance to spawn
	// Spread think times so that they don't all happen at the same time (Carmack)
	SetThink( &CBaseMonster::CallMonsterThink );
	pev->nextthink += RANDOM_FLOAT( 0.1f, 0.4f ); // spread think times.

	// Vit_amiN: fixed -- now it doesn't touch any scripted_sequence target
	bool shouldWaitTrigger = !FStringNull( pev->targetname ) && !m_pCine // wait until triggered
							&& m_Activity != ACT_GLIDE; /* Don't affect repel grunts */
	shouldWaitTrigger = shouldWaitTrigger && (g_modFeatures.monsters_spawned_named_wait_trigger || pev->owner == 0); // Don't affect monsters coming from monstermaker
	if( shouldWaitTrigger )
	{
		SetState( MONSTERSTATE_IDLE );
		// UNDONE: Some scripted sequence monsters don't have an idle?
		SetActivity( ACT_IDLE );
		ChangeSchedule( GetScheduleOfType( SCHED_WAIT_TRIGGER ) );
	}
}

void CBaseMonster::MovementComplete( void ) 
{ 
	switch( m_iTaskStatus )
	{
	case TASKSTATUS_NEW:
	case TASKSTATUS_RUNNING:
		m_iTaskStatus = TASKSTATUS_RUNNING_TASK;
		break;
	case TASKSTATUS_RUNNING_MOVEMENT:
		TaskComplete();
		break;
	case TASKSTATUS_RUNNING_TASK:
		ALERT( at_error, "Movement completed twice!\n" );
		break;
	case TASKSTATUS_COMPLETE:		
		break;
	}
	m_movementGoal = MOVEGOAL_NONE;
}

bool CBaseMonster::TaskIsRunning( void )
{
	return ( m_iTaskStatus != TASKSTATUS_COMPLETE && m_iTaskStatus != TASKSTATUS_RUNNING_MOVEMENT );
}

//=========================================================
// IRelationship - returns an integer that describes the 
// relationship between two types of monster.
//=========================================================
int CBaseMonster::IRelationship( CBaseEntity *pTarget )
{
	return IDefaultRelationship(pTarget);
}

int CBaseMonster::IDefaultRelationship(CBaseEntity *pTarget)
{
	if (!pTarget) {
		ALERT(at_warning, "%s got null target in IRelationship!\n", STRING(pev->classname));
		return R_NO;
	}
	return IDefaultRelationship(Classify(), pTarget->Classify());
}

int CBaseMonster::IDefaultRelationship(int classify)
{
	return IDefaultRelationship(Classify(), classify);
}

#define R_OA (R_AL-1)
#define R_XA (R_AL-2)
#define R_PA (R_AL-3)
#define R_XG (R_AL-4)
#define R_AX (R_AL-5)

int CBaseMonster::IDefaultRelationship(int classify1, int classify2)
{
	static short iEnemy[CLASS_NUMBER_OF_CLASSES][CLASS_NUMBER_OF_CLASSES] =
	{			 //   NONE	 MACH	 PLYR	 HPASS	 HMIL	 AMIL	 APASS	 AMONST	APREY	 APRED	 INSECT	PLRALY	PBWPN	ABWPN	XPRED	XSHOCK	ALMIL	BLOPS	SNARK	GARG
	/*NONE*/		{ R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO},
	/*MACHINE*/		{ R_NO	,R_NO	,R_DL	,R_DL	,R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_DL,	R_DL,	R_DL,	R_DL,	R_DL,	R_DL,	R_DL,	R_DL,	R_DL},
	/*PLAYER*/		{ R_NO	,R_DL	,R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO,	R_DL,	R_DL,	R_DL,	R_DL,	R_NO,	R_DL,	R_DL,	R_DL},
	/*HUMANPASSIVE*/{ R_NO	,R_NO	,R_AL	,R_AL	,R_HT	,R_HT	,R_NO	,R_HT	,R_DL	,R_HT	,R_NO	,R_AL,	R_NO,	R_NO,	R_HT,	R_HT,	R_OA,	R_HT,	R_DL,	R_HT},
	/*HUMANMILITAR*/{ R_NO	,R_NO	,R_HT	,R_DL	,R_AL	,R_HT	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_HT,	R_NO,	R_NO,	R_HT,	R_HT,	R_DL,	R_DL,	R_HT,	R_HT},
	/*ALIENMILITAR*/{ R_NO	,R_DL	,R_HT	,R_DL	,R_HT	,R_AL	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_DL,	R_NO,	R_NO,	R_PA,	R_XA,	R_HT,	R_HT,	R_NO,	R_AL},
	/*ALIENPASSIVE*/{ R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO},
	/*ALIENMONSTER*/{ R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_DL,	R_NO,	R_NO,	R_AX,	R_AX,	R_DL,	R_DL,	R_NO,	R_NO},
	/*ALIENPREY   */{ R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO	,R_NO	,R_NO	,R_FR	,R_NO	,R_DL,	R_NO,	R_NO,	R_FR,	R_FR,	R_DL,	R_DL,	R_NO,	R_NO},
	/*ALIENPREDATO*/{ R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO	,R_NO	,R_HT	,R_DL	,R_NO	,R_DL,	R_NO,	R_NO,	R_DL,	R_AX,	R_DL,	R_DL,	R_NO,	R_NO},
	/*INSECT*/		{ R_FR	,R_FR	,R_FR	,R_FR	,R_FR	,R_NO	,R_FR	,R_FR	,R_FR	,R_FR	,R_NO	,R_FR,	R_NO,	R_NO,	R_NO,	R_NO,	R_FR,	R_FR,	R_NO,	R_FR},
	/*PLAYERALLY*/	{ R_NO	,R_DL	,R_AL	,R_AL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_AL,	R_NO,	R_NO,	R_DL,	R_DL,	R_OA,	R_DL,	R_HT,	R_DL},
	/*PBIOWEAPON*/	{ R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_DL,	R_NO,	R_DL,	R_DL,	R_DL,	R_DL,	R_DL,	R_DL,	R_DL},
	/*ABIOWEAPON*/	{ R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_AL	,R_NO	,R_DL	,R_DL	,R_NO	,R_NO	,R_DL,	R_DL,	R_NO,	R_DL,	R_DL,	R_DL,	R_DL,	R_NO,	R_AL},
	/*XPREDATOR*/	{ R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_PA	,R_NO	,R_AX	,R_DL	,R_DL	,R_NO	,R_DL,	R_NO,	R_NO,	R_AL,	R_AL,	R_DL,	R_DL,	R_NO,	R_XG},
	/*XSHOCK*/		{ R_NO	,R_DL	,R_HT	,R_DL	,R_HT	,R_XA	,R_NO	,R_AX	,R_AX	,R_AX	,R_NO	,R_DL,	R_NO,	R_NO,	R_AL,	R_AL,	R_HT,	R_HT,	R_NO,	R_XG},
	/*PLRALLYMIL*/	{ R_NO	,R_DL	,R_AL	,R_OA	,R_DL	,R_HT	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_OA,	R_NO,	R_NO,	R_DL,	R_HT,	R_AL,	R_DL,	R_HT,	R_HT},
	/*BLACKOPS*/	{ R_NO	,R_DL	,R_HT	,R_DL	,R_DL	,R_HT	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_HT,	R_NO,	R_NO,	R_HT,	R_HT,	R_DL,	R_AL,	R_HT,	R_HT},
	/*SNARK*/		{ R_NO	,R_NO	,R_HT	,R_DL	,R_HT	,R_NO	,R_NO	,R_DL	,R_DL	,R_NO	,R_NO	,R_DL,	R_NO,	R_NO,	R_DL,	R_DL,	R_HT,	R_HT,	R_NO,	R_DL},
	/*GARGANTUA*/	{ R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_AL	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_DL,	R_NO,	R_NO,	R_XG,	R_XG,	R_DL,	R_DL,	R_NO,	R_AL},
	};
	if (classify1 >= CLASS_NUMBER_OF_CLASSES || classify1 < 0 || classify2 >= CLASS_NUMBER_OF_CLASSES || classify2 < 0 )
	{
		ALERT(at_aiconsole, "Unknown classify for monster relationship %d,%d\n", classify1, classify2);
		return R_NO;
	}
	const int rel = iEnemy[classify1][classify2];
	switch (rel) {
	case R_OA:
		return g_modFeatures.opfor_grunts_dislike_civilians ? R_DL : R_AL;
	case R_XA:
	case R_PA:
		return g_modFeatures.racex_dislike_alien_military ? R_HT : R_NO;
	case R_XG:
		return g_modFeatures.racex_dislike_gargs ? R_HT : R_NO;
	case R_AX:
		return g_modFeatures.racex_dislike_alien_monsters ? R_DL : R_NO;
	default:
		return rel;
	}
}

//=========================================================
// FindCover - tries to find a nearby node that will hide
// the caller from its enemy. 
//
// If supplied, search will return a node at least as far
// away as MinDist, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//=========================================================
// UNDONE: Should this find the nearest node?

//float CGraph::PathLength( int iStart, int iDest, int iHull, int afCapMask )

bool CBaseMonster::FindSpotAway(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist, int flags , const char *displayName)
{
	if( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if( flMinDist > 0.5f * flMaxDist )
	{
#if _DEBUG
		ALERT( at_console, "FindCover MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5f * flMaxDist;
	}

	if( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		ALERT( at_aiconsole, "Graph not ready for %s!\n", displayName );
		return false;
	}

	const int iMyNode = WorldGraph.FindNearestNode( pev->origin, this );

	if( iMyNode == NO_NODE )
	{
		ALERT( at_aiconsole, "%s - %s has no nearest node!\n", displayName, STRING( pev->classname ) );
		return false;
	}

	const int iThreatNode = WorldGraph.FindNearestNode ( vecThreat, this );
	const int iMyHullIndex = WorldGraph.HullIndex( this );

	const Vector vecLookersSpot = vecThreat + vecViewOffset;// calculate location of enemy's eyes
	const bool mustTraceLooker = FBitSet(flags, FINDSPOTAWAY_TRACE_LOOKER);
	const bool dontAvoidThreatNode = vecThreat == pev->origin || FBitSet(flags, FINDSPOTAWAY_DONT_AVOID_THREAT_NODE);

	if ((!mustTraceLooker && !dontAvoidThreatNode) && iThreatNode != NO_NODE && iThreatNode == iMyNode)
	{
		ALERT( at_aiconsole, "%s - %s: my nearest node and threat nearest node are the same!\n", displayName, STRING( pev->classname ) );
		return false;
	}

	// we'll do a rough sample to find nodes that are relatively nearby
	for( int i = 0; i < WorldGraph.m_cNodes; i++ )
	{
		int nodeNumber = ( i + WorldGraph.m_iLastCoverSearch ) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );

		const float flDistSqr = ( pev->origin - node.m_vecOrigin ).LengthSqr();

		// DON'T do the trace check on a node that is farther away than a node that we've already found to 
		// provide cover! Also make sure the node is within the mins/maxs of the search.
		if( flDistSqr >= flMinDist*flMinDist && flDistSqr < flMaxDist*flMaxDist )
		{
			bool traceOk = true;
			if (mustTraceLooker)
			{
				TraceResult tr;
				UTIL_TraceLine( node.m_vecOrigin + vecViewOffset, vecLookersSpot, ignore_monsters, ignore_glass,  ENT( pev ), &tr );
				traceOk = tr.flFraction != 1.0f;
			}
			if( traceOk )
			{
				bool distanceOk = iThreatNode == NO_NODE;
				if (!distanceOk && (mustTraceLooker || dontAvoidThreatNode))
					distanceOk = iThreatNode == iMyNode;
				if (!distanceOk)
				{
					float myPathLength = WorldGraph.PathLength( iMyNode, nodeNumber, iMyHullIndex, m_afCapability );
					float threatPathLength = WorldGraph.PathLength( iThreatNode, nodeNumber, iMyHullIndex, m_afCapability );
					distanceOk = myPathLength <= threatPathLength || threatPathLength < 0;
				}
				if( distanceOk )
				{
					if( (!FBitSet(flags, FINDSPOTAWAY_CHECK_SPOT) || FValidateCover( node.m_vecOrigin )) && MoveToLocation( FBitSet(flags, FINDSPOTAWAY_RUN) ? ACT_RUN : ACT_WALK, 0, node.m_vecOrigin ) )
					{
						/*
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
							WRITE_BYTE( TE_SHOWLINE );

							WRITE_VECTOR( node.m_vecOrigin );

							WRITE_VECTOR( vecLookersOffset );
						MESSAGE_END();
						*/

						WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CBaseMonster::FindCover( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist, int flags )
{
	return FindSpotAway(vecThreat, vecViewOffset, flMinDist, flMaxDist, flags|FINDSPOTAWAY_TRACE_LOOKER, "FindCover()");
}

bool CBaseMonster::FindCover(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist)
{
	return FindCover( vecThreat, vecViewOffset, flMinDist, flMaxDist, FINDSPOTAWAY_RUN|FINDSPOTAWAY_CHECK_SPOT );
}

bool CBaseMonster::FindSpotAway( Vector vecThreat, float flMinDist, float flMaxDist, int flags )
{
	return FindSpotAway(vecThreat, g_vecZero, flMinDist, flMaxDist, flags, "FindSpotAway()");
}

//=========================================================
// BuildNearestRoute - tries to build a route as close to the target
// as possible, even if there isn't a path to the final point.
//
// If supplied, search will return a node at least as far
// away as MinDist from vecThreat, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//=========================================================
bool CBaseMonster::BuildNearestRoute( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist )
{
	int i;
	int iMyHullIndex;
	int iMyNode;
	float flDist;
	Vector vecLookersOffset;
	TraceResult tr;

	if( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if( flMinDist > 0.5f * flMaxDist )
	{
#if _DEBUG
		ALERT( at_console, "FindCover MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5f * flMaxDist;
	}

	if( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		ALERT( at_aiconsole, "Graph not ready for BuildNearestRoute!\n" );
		return false;
	}

	iMyNode = WorldGraph.FindNearestNode( pev->origin, this );

	iMyHullIndex = WorldGraph.HullIndex( this );

	if( iMyNode == NO_NODE )
	{
		ALERT( at_aiconsole, "BuildNearestRoute() - %s has no nearest node!\n", STRING( pev->classname ) );
		return false;
	}

	vecLookersOffset = vecThreat + vecViewOffset;// calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for( i = 0; i < WorldGraph.m_cNodes; i++ )
	{
		int nodeNumber = ( i + WorldGraph.m_iLastCoverSearch ) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );

		// can I get there?
		if( WorldGraph.NextNodeInRoute( iMyNode, nodeNumber, iMyHullIndex, 0 ) != iMyNode )
		{
			flDist = ( vecThreat - node.m_vecOrigin ).Length();

			// is it close?
			if( flDist > flMinDist && flDist < flMaxDist )
			{
				// can I see where I want to be from there?
				UTIL_TraceLine( node.m_vecOrigin + pev->view_ofs, vecLookersOffset, ignore_monsters, edict(), &tr );

				if( tr.flFraction == 1.0f )
				{
					// try to actually get there
					if( BuildRoute( node.m_vecOrigin, bits_MF_TO_LOCATION, NULL, BUILDROUTE_NO_TRIDEPTH ) )
					{
						// flMaxDist = flDist;
						m_vecMoveGoal = node.m_vecOrigin;
						WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.
						return true; // UNDONE: keep looking for something closer!
					}
				}
			}
		}
	}

	return false;
}

//=========================================================
// BestVisibleEnemy - this functions searches the link
// list whose head is the caller's m_pLink field, and returns
// a pointer to the enemy entity in that list that is nearest the 
// caller.
//
// !!!UNDONE - currently, this only returns the closest enemy.
// we'll want to consider distance, relationship, attack types, back turned, etc.
//=========================================================
CBaseEntity *CBaseMonster::BestVisibleEnemy( void )
{
	CBaseEntity	*pReturn = NULL;
	CBaseEntity	*pNextEnt = m_pLink;
	int		iNearestSqr = 8192 * 8192;// so first visible entity will become the closest.
	int		iBestRelationship = R_NO;

	while( pNextEnt != NULL )
	{
		if( pNextEnt->IsFullyAlive() )
		{
			const int relationship = IRelationship( pNextEnt);
			if (relationship >= R_DL && m_equalDislikeTime > gpGlobals->time)
			{
				iBestRelationship = relationship;
				const int iDistSqr = ( pNextEnt->pev->origin - pev->origin ).LengthSqr();
				if( iDistSqr <= iNearestSqr )
				{
					iNearestSqr = iDistSqr;
					pReturn = pNextEnt;
				}
			}
			else if( relationship > iBestRelationship )
			{
				// this entity is disliked MORE than the entity that we 
				// currently think is the best visible enemy. No need to do 
				// a distance check, just get mad at this one for now.
				iBestRelationship = relationship;
				iNearestSqr = ( pNextEnt->pev->origin - pev->origin ).LengthSqr();
				pReturn = pNextEnt;
			}
			else if( relationship == iBestRelationship )
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				const int iDistSqr = ( pNextEnt->pev->origin - pev->origin ).LengthSqr();
				
				if( iDistSqr <= iNearestSqr )
				{
					iNearestSqr = iDistSqr;
					pReturn = pNextEnt;
				}
			}
		}

		pNextEnt = pNextEnt->m_pLink;
	}

	return pReturn;
}

//=========================================================
// MakeIdealYaw - gets a yaw value for the caller that would
// face the supplied vector. Value is stuffed into the monster's
// ideal_yaw
//=========================================================
void CBaseMonster::MakeIdealYaw( Vector vecTarget )
{
	Vector vecProjection;

	// strafing monster needs to face 90 degrees away from its goal
	if( m_movementActivity == ACT_STRAFE_LEFT )
	{
		vecProjection.x = -vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else if( m_movementActivity == ACT_STRAFE_RIGHT )
	{
		vecProjection.x = vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else
	{
		pev->ideal_yaw = UTIL_VecToYaw( vecTarget - pev->origin );
	}
}

//=========================================================
// FlYawDiff - returns the difference ( in degrees ) between
// monster's current yaw and ideal_yaw
//
// Positive result is left turn, negative is right turn
//=========================================================
float CBaseMonster::FlYawDiff( void )
{
	float flCurrentYaw;

	flCurrentYaw = UTIL_AngleMod( pev->angles.y );

	if( flCurrentYaw == pev->ideal_yaw )
	{
		return 0;
	}

	return UTIL_AngleDiff( pev->ideal_yaw, flCurrentYaw );
}

//=========================================================
// Changeyaw - turns a monster towards its ideal_yaw
//=========================================================
float CBaseMonster::ChangeYaw( int yawSpeed )
{
	float		ideal, current, move, speed;

	current = UTIL_AngleMod( pev->angles.y );
	ideal = pev->ideal_yaw;
	if( current != ideal )
	{
		if( monsteryawspeedfix.value )
		{
			if( m_flLastYawTime == 0.f )
				m_flLastYawTime = gpGlobals->time - gpGlobals->frametime;

			float delta = Q_min( gpGlobals->time - m_flLastYawTime, 0.25f );

			m_flLastYawTime = gpGlobals->time;

			speed = (float)yawSpeed * delta * 2;
		}
		else
			speed = (float)yawSpeed * gpGlobals->frametime * 10;

		move = ideal - current;

		if( ideal > current )
		{
			if( move >= 180 )
				move = move - 360;
		}
		else
		{
			if( move <= -180 )
				move = move + 360;
		}

		if( move > 0 )
		{
			// turning to the monster's left
			if( move > speed )
				move = speed;
		}
		else
		{
			// turning to the monster's right
			if( move < -speed )
				move = -speed;
		}

		pev->angles.y = UTIL_AngleMod( current + move );

		// turn head in desired direction only if they have a turnable head
		if( m_afCapability & bits_CAP_TURN_HEAD )
		{
			float yaw = pev->ideal_yaw - pev->angles.y;
			if( yaw > 180 )
				yaw -= 360;
			if( yaw < -180 )
				yaw += 360;
			// yaw *= 0.8;
			SetBoneController( 0, yaw );
		}
	}
	else
		move = 0;

	return move;
}

//=========================================================
// VecToYaw - turns a directional vector into a yaw value
// that points down that vector.
//=========================================================
float CBaseMonster::VecToYaw( Vector vecDir )
{
	if( vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0 )
		return pev->angles.y;

	return UTIL_VecToYaw( vecDir );
}

//=========================================================
// SetEyePosition
//
// queries the monster's model for $eyeposition and copies
// that vector to the monster's view_ofs
//
//=========================================================
void CBaseMonster::SetEyePosition( void )
{
	Vector  vecEyePosition;
	void	*pmodel = GET_MODEL_PTR( ENT(pev) );

	GetEyePosition( pmodel, vecEyePosition );

	pev->view_ofs = vecEyePosition;

	if( pev->view_ofs == g_vecZero )
	{
		ALERT( at_aiconsole, "%s has no view_ofs! Fallback to %.2f\n", STRING( pev->classname ), pev->maxs.z );
		pev->view_ofs = Vector(0, 0, pev->maxs.z);
	}
}

void CBaseMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SCRIPT_EVENT_DEAD:
		if( m_MonsterState == MONSTERSTATE_SCRIPT )
		{
			pev->deadflag = DEAD_DYING;
			// Kill me now! (and fade out when CineCleanup() is called)
#if _DEBUG
			ALERT( at_aiconsole, "Death event: %s\n", STRING( pev->classname ) );
#endif
			pev->health = 0;
		}
#if _DEBUG
		else
			ALERT( at_aiconsole, "INVALID death event:%s\n", STRING( pev->classname ) );
#endif
		break;
	case SCRIPT_EVENT_NOT_DEAD:
		if( m_MonsterState == MONSTERSTATE_SCRIPT )
		{
			pev->deadflag = DEAD_NO;

			// This is for life/death sequences where the player can determine whether a character is dead or alive after the script 
			pev->health = pev->max_health;
		}
		break;
	case SCRIPT_EVENT_SOUND:			// Play a named wave file
		EmitSound( CHAN_BODY, pEvent->options, VOL_NORM, ATTN_IDLE );
		break;
	case SCRIPT_EVENT_SOUND_VOICE:
		EmitSound( CHAN_VOICE, pEvent->options, VOL_NORM, ATTN_IDLE );
		break;
	case SCRIPT_EVENT_SOUND_VOICE_BODY:
		EmitSound( CHAN_BODY, pEvent->options, VOL_NORM, ATTN_NORM );
		break;
	case SCRIPT_EVENT_SOUND_VOICE_VOICE:
		EmitSound( CHAN_VOICE, pEvent->options, VOL_NORM, ATTN_NORM );
		break;
	case SCRIPT_EVENT_SOUND_VOICE_WEAPON:
		EmitSound( CHAN_WEAPON, pEvent->options, VOL_NORM, ATTN_NORM );
		break;
	case SCRIPT_EVENT_SOUNDSCRIPT:
		EmitSoundScript(pEvent->options);
		break;
	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 33% of the time
		if( RANDOM_LONG( 0, 2 ) == 0 )
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:			// Play a named sentence group
		SENTENCEG_PlayRndSz( edict(), pEvent->options, 1.0, ATTN_IDLE, 0, 100 );
		break;
	case SCRIPT_EVENT_FIREEVENT:		// Fire a trigger
		FireTargets( pEvent->options, this, this );
		break;
	case SCRIPT_EVENT_NOINTERRUPT:		// Can't be interrupted from now on
		if( m_pCine )
			m_pCine->AllowInterrupt( false );
		break;
	case SCRIPT_EVENT_CANINTERRUPT:		// OK to interrupt now
		if( m_pCine )
			m_pCine->AllowInterrupt( true );
		break;
#if 0
	case SCRIPT_EVENT_INAIR:			// Don't DROP_TO_FLOOR()
	case SCRIPT_EVENT_ENDANIMATION:		// Set ending animation sequence to
		break;
#endif
	case MONSTER_EVENT_BODYDROP_HEAVY:
		if( pev->flags & FL_ONGROUND )
		{
			EmitSoundScript(NPC::bodyDropHeavySoundScript);
		}
		break;
	case MONSTER_EVENT_BODYDROP_LIGHT:
		if( pev->flags & FL_ONGROUND )
		{
			EmitSoundScript(NPC::bodyDropLightSoundScript);
		}
		break;
	case MONSTER_EVENT_SWISHSOUND:
		{
			// NO MONSTER may use this anim event unless that monster's precache precaches this sound!!!
			EmitSoundScript(NPC::swishSoundScript);
			break;
		}
	default:
		{
			TraceHullAttackParams params;
			if (SetTraceHullAttackParamsFromTemplate(pEvent->event, params))
			{
				PerformTraceHullAttack(params);
			}
			else
			{
				ALERT( at_aiconsole, "Unhandled animation event %d for %s\n", pEvent->event, STRING( pev->classname ) );
			}
		}
		break;
	}
}

// Combat
Vector CBaseMonster::GetGunPosition()
{
	UTIL_MakeVectors( pev->angles );

	// Vector vecSrc = pev->origin + gpGlobals->v_forward * 10;
	//vecSrc.z = pevShooter->absmin.z + pevShooter->size.z * 0.7;
	//vecSrc.z = pev->origin.z + (pev->view_ofs.z - 4);
	Vector vecSrc = pev->origin 
					+ gpGlobals->v_forward * m_HackedGunPos.y 
					+ gpGlobals->v_right * m_HackedGunPos.x 
					+ gpGlobals->v_up * m_HackedGunPos.z;

	return vecSrc;
}

//=========================================================
// NODE GRAPH
//=========================================================

//=========================================================
// FGetNodeRoute - tries to build an entire node path from
// the callers origin to the passed vector. If this is 
// possible, ROUTE_SIZE waypoints will be copied into the
// callers m_Route. true is returned if the operation
// succeeds (path is valid) or false if failed (no path
// exists )
//=========================================================
bool CBaseMonster::FGetNodeRoute( Vector vecDest, int goalMoveFlag )
{
	int iPath[ MAX_PATH_SIZE ];
	int iSrcNode, iDestNode;
	int iResult;
	int i;
	int iNumToCopy;

	if( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		ALERT( at_aiconsole, "FGetNodeRoute: Graph not ready!\n" );
		return false;
	}

	iSrcNode = WorldGraph.FindNearestNode( pev->origin, this );
	iDestNode = WorldGraph.FindNearestNode( vecDest, this );

	if( iSrcNode == -1 )
	{
		// no node nearest self
		//ALERT( at_aiconsole, "FGetNodeRoute: No valid node near self!\n" );
		return false;
	}
	else if( iDestNode == -1 )
	{
		// no node nearest target
		//ALERT( at_aiconsole, "FGetNodeRoute: No valid node near target!\n" );
		return false;
	}

	// valid src and dest nodes were found, so it's safe to proceed with
	// find shortest path
	int iNodeHull = WorldGraph.HullIndex( this ); // make this a monster virtual function

	const int afCapMask = m_afCapability | (FBitSet(pev->flags, FL_MONSTERCLIP) ? bits_CAP_MONSTERCLIPPED : 0);
	iResult = WorldGraph.FindShortestPath( iPath, MAX_PATH_SIZE, iSrcNode, iDestNode, iNodeHull, afCapMask, true );

	if( !iResult )
	{
#if 1
		ALERT( at_aiconsole, "No Path from %d to %d!\n", iSrcNode, iDestNode );
		return false;
#else
		qboolean bRoutingSave = WorldGraph.m_fRoutingComplete;
		WorldGraph.m_fRoutingComplete = 0;
		iResult = WorldGraph.FindShortestPath( iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability );
		WorldGraph.m_fRoutingComplete = bRoutingSave;
		if( !iResult )
		{
			ALERT( at_aiconsole, "No Path from %d to %d!\n", iSrcNode, iDestNode );
			return false;
		}
		else
		{
			ALERT( at_aiconsole, "Routing is inconsistent!" );
		}
#endif
	}

	// there's a valid path within iPath now, so now we will fill the route array
	// up with as many of the waypoints as it will hold.
	
	// don't copy ROUTE_SIZE entries if the path returned is shorter
	// than ROUTE_SIZE!!!
	if( iResult < ROUTE_SIZE )
	{
		iNumToCopy = iResult;
	}
	else
	{
		iNumToCopy = ROUTE_SIZE;
	}

	for( i = 0 ; i < iNumToCopy; i++ )
	{
		m_Route[i].vecLocation = WorldGraph.m_pNodes[iPath[i]].m_vecOrigin;
		m_Route[i].iType = bits_MF_TO_NODE;
	}

	if( iNumToCopy < ROUTE_SIZE )
	{
		m_Route[iNumToCopy].vecLocation = vecDest;
		m_Route[iNumToCopy].iType = goalMoveFlag|bits_MF_IS_GOAL;
	}

	return true;
}

//=========================================================
// FindHintNode
//=========================================================
int CBaseMonster::FindHintNode( void )
{
	int i;
	TraceResult tr;

	if( !WorldGraph.m_fGraphPresent )
	{
		ALERT( at_aiconsole, "find_hintnode: graph not ready!\n" );
		return NO_NODE;
	}

	if( WorldGraph.m_iLastActiveIdleSearch >= WorldGraph.m_cNodes )
	{
		WorldGraph.m_iLastActiveIdleSearch = 0;
	}

	for( i = 0; i < WorldGraph.m_cNodes; i++ )
	{
		int nodeNumber = ( i + WorldGraph.m_iLastActiveIdleSearch) % WorldGraph.m_cNodes;
		CNode &node = WorldGraph.Node( nodeNumber );

		if( node.m_sHintType )
		{
			// this node has a hint. Take it if it is visible, the monster likes it, and the monster has an animation to match the hint's activity.
			if( FValidateHintType( node.m_sHintType ) )
			{
				if( !node.m_sHintActivity || LookupActivity( node.m_sHintActivity ) != ACTIVITY_NOT_AVAILABLE )
				{
					UTIL_TraceLine( pev->origin + pev->view_ofs, node.m_vecOrigin + pev->view_ofs, ignore_monsters, ENT( pev ), &tr );

					if( tr.flFraction == 1.0f )
					{
						WorldGraph.m_iLastActiveIdleSearch = nodeNumber + 1; // next monster that searches for hint nodes will start where we left off.
						return nodeNumber;// take it!
					}
				}
			}
		}
	}

	WorldGraph.m_iLastActiveIdleSearch = 0;// start at the top of the list for the next search.

	return NO_NODE;
}

const char* CBaseMonster::MonsterStateDisplayString(MONSTERSTATE monsterState)
{
	switch (monsterState) {
	case MONSTERSTATE_NONE:
		return "None";
	case MONSTERSTATE_IDLE:
		return "Idle";
	case MONSTERSTATE_COMBAT:
		return "Combat";
	case MONSTERSTATE_ALERT:
		return "Alert";
	case MONSTERSTATE_HUNT:
		return "Hunt";
	case MONSTERSTATE_PRONE:
		return "Prone";
	case MONSTERSTATE_SCRIPT:
		return "Scripted";
	case MONSTERSTATE_PLAYDEAD:
		return "PlayDead";
	case MONSTERSTATE_DEAD:
		return "Dead";
	default:
		return "Unknown";
	}
}

void CBaseMonster::ReportAIState( ALERT_TYPE level )
{
	const bool shouldReportRoute = DeveloperModeLevel() >= 3;
	if (shouldReportRoute && !FRouteClear())
	{
		DrawRoute(this, m_movementGoal);
	}

	static const char *pDeadNames[] = {"No", "Dying", "Dead", "Respawnable", "DiscardBody"};

	if (FStringNull(pev->targetname)) {
		ALERT( level, "%s: ", STRING(pev->classname) );
	} else {
		ALERT( level, "%s (%s): ", STRING(pev->classname), STRING(pev->targetname) );
	}
	ALERT( level, "Entindex: %d. ", entindex() );
	if (!FStringNull(m_entTemplate))
		ALERT( level , "Template: %s. ", STRING(m_entTemplate) );
	const int classify = Classify();
	ALERT( level, "Classify: %s (%d), ", ClassifyDisplayName(classify), classify );

	ALERT( level, "State: %s, ", MonsterStateDisplayString(m_MonsterState) );

	if( pev->deadflag < static_cast<int>(ARRAYSIZE( pDeadNames )) )
		ALERT( level, "Dead flag: %s, ", pDeadNames[pev->deadflag] );
	else
		ALERT( level, "Dead flag: %d, ", pev->deadflag );

	if ( HasMemory( bits_MEMORY_KILLED ) )
		ALERT(level, "Has MEMORY_KILLED, ");
	if ( HasMemory( bits_MEMORY_PROVOKED ) )
		ALERT(level, "Has MEMORY_PROVOKED, ");
	else if ( HasMemory( bits_MEMORY_SUSPICIOUS ) )
		ALERT(level, "Has MEMORY_SUSPICIOUS, ");

	int i = 0;
	while( activity_map[i].type != 0 )
	{
		if( activity_map[i].type == (int)m_Activity )
		{
			ALERT( level, "Activity %s, ", activity_map[i].name );
			break;
		}
		i++;
	}
	void *pmodel = GET_MODEL_PTR( ENT( pev ) );
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pev->sequence >= 0 && pev->sequence < pstudiohdr->numseq)
	{
		mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)( (byte *)pstudiohdr + pstudiohdr->seqindex );
		ALERT(at_console, "Playing sequence %s (index %d, frame %g), ", pseqdesc[pev->sequence].label, pev->sequence, pev->frame);
	}

	if( m_pSchedule )
	{
		const char *pName = NULL;
		pName = m_pSchedule->pName;
		if( !pName )
			pName = "Unknown";
		ALERT( level, "Schedule %s, ", pName );
		Task_t *pTask = GetTask();
		if( pTask )
			ALERT( level, "Task %d (#%d), ", pTask->iTask, m_iScheduleIndex );
	}
	else
		ALERT( level, "No Schedule, " );

	if( m_hEnemy != 0 )
		ALERT( level, "Enemy is %s (%s, ent: %d, LKP: (%g, %g, %g)). ",
			STRING(m_hEnemy->pev->classname), m_hEnemy->IsAlive() ? "alive" : "dead", m_hEnemy->entindex(),
			m_vecEnemyLKP.x, m_vecEnemyLKP.y, m_vecEnemyLKP.z );
	else
		ALERT( level, "No enemy. " );

	for (i=0; i<MAX_OLD_ENEMIES; ++i)
	{
		if (m_hOldEnemy[i] != 0)
		{
			ALERT( level, "Old enemy is %s (%s, ent: %d, LKP: (%g, %g, %g)). ",
				STRING(m_hOldEnemy[i]->pev->classname), m_hOldEnemy[i]->IsAlive() ? "alive" : "dead", m_hOldEnemy[i]->entindex(),
				m_vecOldEnemy[i].x, m_vecOldEnemy[i].y, m_vecOldEnemy[i].z );
		}
	}

	if ( m_hTargetEnt != 0 )
		ALERT( level, "Target ent: %s. ", STRING( m_hTargetEnt->pev->classname ) );

	if ( m_pCine )
		ALERT( level, "Scripted sequence entity: \"%s\". ", STRING(m_pCine->pev->targetname) );

	if( IsMoving() )
	{
		ALERT( level, "Moving" );
		if( m_flMoveWaitFinished > gpGlobals->time )
			ALERT( level, ": Stopped for %.2f", m_flMoveWaitFinished - gpGlobals->time );
		else if( m_IdealActivity == GetStoppedActivity() )
			ALERT( level, ": In stopped anim" );
		ALERT( level, ". " );
	}

	ALERT( level, "Yaw speed: %3.1f, Current Yaw: %3.1f, Ideal Yaw: %3.1f, ", pev->yaw_speed, UTIL_AngleMod( pev->angles.y ), pev->ideal_yaw );
	ALERT( level, "Health: %3.1f / %3.1f, ", pev->health, pev->max_health );
	ALERT( level, "Field of View: %3.1f. ", m_flFieldOfView );
	ALERT( level, "Origin: (%g, %g, %g). ", pev->origin.x, pev->origin.y, pev->origin.z );
	ALERT( level, "Boundbox: (%g, %g, %g), (%g, %g, %g). Size: (%g, %g, %g). ",
		   pev->mins.x, pev->mins.y, pev->mins.z,
		   pev->maxs.x, pev->maxs.y, pev->maxs.z,
		   pev->size.x, pev->size.y, pev->size.z);

	if (pev->model)
		ALERT(level, "Model: %s. ", STRING(pev->model));
	const char* gibModel = GibModel();
	if (gibModel)
		ALERT(level, "Gib Model: %s. ", gibModel);

	ALERT(level, "Rendermode: %s. Color: (%g, %g, %g). Alpha: %g. Renderfx: %s. ",
		  RenderModeToString(pev->rendermode),
		  pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z,
		  pev->renderamt,
		  RenderFxToString(pev->renderfx));

	if (pev->scale)
		ALERT(level, "Scale: %g. ", pev->scale);

	ALERT(level, "Illumination: %d. ", GETENTITYILLUM(edict()));

	const char* targetForGrapple = nullptr;
	switch (SizeForGrapple()) {
	case GRAPPLE_NOT_A_TARGET:
		targetForGrapple = "Not a target";
		break;
	case GRAPPLE_SMALL:
		targetForGrapple = "Small";
		break;
	case GRAPPLE_MEDIUM:
		targetForGrapple = "Medium";
		break;
	case GRAPPLE_LARGE:
		targetForGrapple = "Large";
		break;
	case GRAPPLE_FIXED:
		targetForGrapple = "Fixed";
		break;
	default:
		targetForGrapple = "Unknown";
		break;
	}
	ALERT( level, "Target for Grapple: %s. ", targetForGrapple );

	if( pev->spawnflags & SF_MONSTER_PRISONER )
		ALERT( level, "PRISONER! " );
	if( pev->spawnflags & SF_MONSTER_PREDISASTER )
		ALERT( level, "Pre-Disaster! " );
	if ( pev->flags & FL_MONSTERCLIP )
		ALERT( level, "Monsterclip. " );
	if ( pev->spawnflags & SF_MONSTER_ACT_OUT_OF_PVS )
		ALERT( level, "Can act out of client PVS. " );

	if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		ALERT( level, "Can melee attack 1; " );
	if (HasConditions(bits_COND_CAN_MELEE_ATTACK2))
		ALERT( level, "Can melee attack 2; " );
	if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		ALERT( level, "Can range attack 1; " );
	if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		ALERT( level, "Can range attack 2; " );
	if (HasConditions(bits_COND_SEE_ENEMY))
		ALERT(level, "Sees enemy; ");

	if (FBitSet(m_afCapability, bits_CAP_DOORS_GROUP))
		ALERT(level, "Can open doors; ");

	if (FBitSet(m_afCapability, bits_CAP_SQUAD))
		ALERT(level, "Can form squads; ");

	if (shouldReportRoute)
	{
		int iMyNode = WorldGraph.FindNearestNode( pev->origin, this );
		if (iMyNode != NO_NODE)
		{
			ALERT(level, "Nearest node: %d. ", iMyNode);

			CNode &node = WorldGraph.Node( iMyNode );
			DrawBeamLine(node.m_vecOrigin, node.m_vecOrigin + Vector(0,0,72), 0, 0, 200, 25, 16);
		}
		else
		{
			ALERT(level, "No nearest node. ");
		}

		if (!FRouteClear())
		{
			ALERT(level, "\nRoute:\n");
			for (size_t j = 0; m_Route[j].iType && j < ARRAYSIZE(m_Route); ++j)
			{
				ALERT(level, "%d: ", static_cast<int>(j));
				ReportRouteType(level, m_Route[j].iType);
				ALERT(level, "(%g, %g, %g)\n", m_Route[j].vecLocation.x, m_Route[j].vecLocation.y, m_Route[j].vecLocation.z);
			}
		}
	}
}

//=========================================================
// KeyValue
//
// !!! netname entvar field is used in squadmonster for groupname!!!
//=========================================================
void CBaseMonster::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "TriggerTarget" ) )
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
	else if ( FStrEq( pkvd->szKeyName, "bloodcolor" ) )
	{
		m_bloodColor = atoi( pkvd->szValue );
		// Check for values 1 and 2 for Sven Co-op compatibility
		if (m_bloodColor == 1)
			m_bloodColor = BLOOD_COLOR_RED;
		else if (m_bloodColor == 2)
			m_bloodColor = BLOOD_COLOR_YELLOW;
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "field_of_view" ) )
	{
		m_flFieldOfView = atof( pkvd->szValue );
		if (m_flFieldOfView < -1.0f || m_flFieldOfView >= 1.0f) {
			ALERT(at_warning, "Invalid field of view for monster %s: %3.1f\n", STRING(pev->classname), m_flFieldOfView);
			m_flFieldOfView = 0.0f;
		}
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "classify" ) )
	{
		m_iClass = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName , "gibmodel" ) || FStrEq( pkvd->szKeyName, "m_iszGibModel" ) )
	{
		m_gibModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "is_player_ally" ) )
	{
		m_reverseRelationship = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "minhullsize" ) )
	{
		UTIL_StringToVector((float*)m_minHullSize, pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "maxhullsize" ) )
	{
		UTIL_StringToVector((float*)m_maxHullSize, pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "soundmask" ) )
	{
		m_customSoundMask = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "prisonerto" ) )
	{
		m_prisonerTo = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "ignoredby" ) )
	{
		m_ignoredBy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "freeroam" ) )
	{
		m_freeRoam = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "active_alert" ) )
	{
		m_activeAfterCombat = (short)atoi( pkvd->szValue );
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
	else
	{
		CBaseToggle::KeyValue( pkvd );
	}
}

void CBaseMonster::Activate()
{
	CBaseToggle::Activate();

	if (!g_modFeatures.dying_monsters_block_player && pev->deadflag == DEAD_DYING && HasMemory(bits_MEMORY_KILLED)) {
		pev->iuser3 = -1;
	}
}

void CBaseMonster::SetMySize(const Vector &vecMin, const Vector &vecMax)
{
	Vector vecMins = vecMin;
	Vector vecMaxs = vecMax;
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate && entTemplate->IsSizeDefined())
	{
		vecMins = entTemplate->MinSize();
		vecMaxs = entTemplate->MaxSize();
	}
	UTIL_SetSize(pev, m_minHullSize == g_vecZero ? vecMins : m_minHullSize, m_maxHullSize == g_vecZero ? vecMaxs : m_maxHullSize);
}

static void SetCapFromTriBool(int& ret, tribool b, int cap)
{
	if (!indeterminate(b))
	{
		if (b)
		{
			SetBits(ret, cap);
		}
		else
		{
			ClearBits(ret, cap);
		}
	}
}

void CBaseMonster::SetMySquadCapabilities(int defaultCaps)
{
	m_afCapability |= defaultCaps;
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate)
	{
		SquadCapabilities squadCaps = entTemplate->GetSquadCapabilities();
		SetCapFromTriBool(m_afCapability, squadCaps.canRecruit, bits_CAP_SQUAD);
		SetCapFromTriBool(m_afCapability, squadCaps.denyRecruiting, bits_CAP_SQUAD_DENY);
		SetCapFromTriBool(m_afCapability, squadCaps.allowDifferentClassification, bits_CAP_SQUAD_ALLOW_OTHER_CLASSIFY);
		SetCapFromTriBool(m_afCapability, squadCaps.requireSameClassname, bits_CAP_SQUAD_SAME_CLASSNAME);
		SetCapFromTriBool(m_afCapability, squadCaps.requireSameEntTemplate, bits_CAP_SQUAD_SAME_TEMPLATE);
	}
}

void CBaseMonster::SetMyCanOpenDoors(bool enable)
{
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate && entTemplate->IsOpenDoorCapabilityDefined())
	{
		enable = entTemplate->CanOpenDoors();
	}
	if (enable)
		m_afCapability |= bits_CAP_DOORS_GROUP;
}

//=========================================================
// FCheckAITrigger - checks the monster's AI Trigger Conditions,
// if there is a condition, then checks to see if condition is 
// met. If yes, the monster's TriggerTarget is fired.
//
// Returns true if the target is fired.
//=========================================================
bool CBaseMonster::FCheckAITrigger( short condition )
{
	if( condition == AITRIGGER_NONE )
	{
		// no conditions, so this trigger is never fired.
		return false;
	}

	bool fFireTarget = false;

	switch( condition )
	{
	case AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER:
		if( m_hEnemy != 0 && m_hEnemy->IsPlayer() && HasConditions( bits_COND_SEE_ENEMY ) )
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_SEEPLAYER_UNCONDITIONAL:
		if( HasConditions( bits_COND_SEE_CLIENT ) )
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_SEEPLAYER_NOT_IN_COMBAT:
		if( HasConditions( bits_COND_SEE_CLIENT ) &&
			 m_MonsterState != MONSTERSTATE_COMBAT	&&
			 m_MonsterState != MONSTERSTATE_PRONE	&&
			 m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_TAKEDAMAGE:
		if( m_afConditions & ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_DEATH:
		if( pev->deadflag != DEAD_NO )
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_HALFHEALTH:
		if( IsAlive() && pev->health <= ( pev->max_health / 2 ) )
		{
			fFireTarget = true;
		}
		break;
/*

  // !!!UNDONE - no persistant game state that allows us to track these two.

	case AITRIGGER_SQUADMEMBERDIE:
		break;
	case AITRIGGER_SQUADLEADERDIE:
		break;
*/
	case AITRIGGER_HEARWORLD:
		if( m_afConditions & bits_COND_HEAR_SOUND && m_afSoundTypes & bits_SOUND_WORLD )
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_HEARPLAYER:
		if( m_afConditions & bits_COND_HEAR_SOUND && m_afSoundTypes & bits_SOUND_PLAYER )
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_HEARCOMBAT:
		if( m_afConditions & bits_COND_HEAR_SOUND && m_afSoundTypes & bits_SOUND_COMBAT )
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_PROVOKED_BY_PLAYER:
		{
			if (HasMemory(bits_MEMORY_PROVOKED))
			{
				fFireTarget = true;
			}
		}
		break;
	case AITRIGGER_GOTHEALED:
		{
			if (HasMemory(bits_MEMORY_GOT_HEALED_RECENTLY))
			{
				fFireTarget = true;
			}
		}
		break;
	}

	if( fFireTarget )
	{
		// fire the target, then set the trigger conditions to NONE so we don't fire again
		if (m_iszTriggerTarget)
			ALERT( at_aiconsole, "%s: AI Trigger Fire Target %s\n", STRING(pev->classname), STRING(m_iszTriggerTarget) );
		FireTargets( STRING( m_iszTriggerTarget ), this, this );
		m_iTriggerCondition = AITRIGGER_NONE;
		m_iTriggerAltCondition = AITRIGGER_NONE;
		return true;
	}

	Forget(bits_MEMORY_GOT_HEALED_RECENTLY);
	return false;
}

bool CBaseMonster::FCheckAITrigger( void )
{
	bool ret = FCheckAITrigger( m_iTriggerCondition );
	if (!ret)
		return FCheckAITrigger( m_iTriggerAltCondition );
	return ret;
}

//=========================================================	
// CanPlaySequence - determines whether or not the monster
// can play the scripted sequence or AI sequence that is 
// trying to possess it. If DisregardState is set, the monster
// will be sucked into the script no matter what state it is
// in. ONLY Scripted AI ents should allow this.
//=========================================================	
bool CBaseMonster::CanPlaySequence( int interruptFlags )
{
	if( m_pCine )
	{
		if ( interruptFlags & SS_INTERRUPT_SCRIPTS )
		{
			return true;
		}
		else
		{
			// monster is already running a scripted sequence or dead!
			return false;
		}
	}
	else if (!IsFullyAlive() || m_MonsterState == MONSTERSTATE_PRONE)
	{
		return false;
	}
	
	if( interruptFlags & SS_INTERRUPT_ANYSTATE )
	{
		// ok to go, no matter what the monster state. (scripted AI)
		return true;
	}

	if( m_MonsterState == MONSTERSTATE_NONE || m_MonsterState == MONSTERSTATE_IDLE || m_IdealMonsterState == MONSTERSTATE_IDLE )
	{
		// ok to go, but only in these states
		return true;
	}
	
	if( (m_MonsterState == MONSTERSTATE_ALERT || m_MonsterState == MONSTERSTATE_HUNT) && (interruptFlags & SS_INTERRUPT_ALERT) )
		return true;

	// unknown situation
	return false;
}

//=========================================================
// FindLateralCover - attempts to locate a spot in the world
// directly to the left or right of the caller that will
// conceal them from view of pSightEnt
//=========================================================

bool CBaseMonster::FindLateralSpotAway( const Vector& vecThreat, float minDist, float maxDist, int flags )
{
	const bool threatIsMyself = pev->origin == vecThreat;

	Vector vecRight{};
	if (threatIsMyself)
	{
		UTIL_MakeVectors(pev->angles);
		Vector vecRight = gpGlobals->v_right;
		vecRight.z = 0;
	}
	else
	{
		Vector vecFromThreat = pev->origin - vecThreat;
		vecFromThreat.z = 0;

		if (vecFromThreat == g_vecZero)
			return false;

		vecFromThreat.NormalizeInPlace();

		const float sideAngleRad = M_PI_F * 0.5f;
		vecRight.x = vecFromThreat.x * std::cos(sideAngleRad) - vecFromThreat.y * std::sin(sideAngleRad);
		vecRight.y = vecFromThreat.x * std::sin(sideAngleRad) + vecFromThreat.y * std::cos(sideAngleRad);
	}

	const Vector vecStepRight = vecRight * COVER_DELTA;
	const Vector vecStart = pev->origin;

	const Activity movementActivity = FBitSet(flags, FINDSPOTAWAY_RUN) ? ACT_RUN : ACT_WALK;

	minDist = Q_max(minDist, COVER_DELTA);
	maxDist = Q_max(maxDist, COVER_DELTA);
	const Vector startOffset = vecRight * minDist;
	const int coverChecks = (int)((maxDist - minDist) / COVER_DELTA) + 1; // at least one check

	const float distToThreatSqr = (vecStart - vecThreat).LengthSqr();

	for( int i = 1; i <= coverChecks; i++ )
	{
		const Vector vecLeftTest = vecStart - startOffset - vecStepRight * ( coverChecks - i );
		const Vector vecRightTest = vecStart + startOffset + vecStepRight * ( coverChecks - i );

		if (!threatIsMyself || (vecLeftTest - vecThreat).LengthSqr() > distToThreatSqr)
		{
			if( (!FBitSet(flags, FINDSPOTAWAY_CHECK_SPOT) || FValidateCover( vecLeftTest )) )
			{
				if( MoveToLocation( movementActivity, 0, vecLeftTest, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
				{
					return true;
				}
			}
		}

		if (!threatIsMyself || (vecRightTest - vecThreat).LengthSqr() > distToThreatSqr)
		{
			if( (!FBitSet(flags, FINDSPOTAWAY_CHECK_SPOT) || FValidateCover( vecRightTest )) )
			{
				if( MoveToLocation( movementActivity, 0, vecRightTest, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CBaseMonster::FindStraightSpotAway( const Vector& vecThreat, float minDist, float maxDist, int flags )
{
	const Vector vecStart = pev->origin;

	Vector vecDiff = pev->origin - vecThreat;
	vecDiff.z = 0;

	if (vecDiff == g_vecZero)
		return false;

	const Vector vecDirection = vecDiff.Normalize();
	const Vector vecStep = vecDirection * COVER_DELTA;

	const Activity movementActivity = FBitSet(flags, FINDSPOTAWAY_RUN) ? ACT_RUN : ACT_WALK;

	minDist = Q_max(minDist, COVER_DELTA);
	maxDist = Q_max(maxDist, COVER_DELTA);
	const Vector startOffset = vecDirection * minDist;
	const int coverChecks = (int)((maxDist - minDist) / COVER_DELTA) + 1; // at least one check

	for( int i = 1; i <= coverChecks; i++ )
	{
		const Vector move = startOffset + vecStep * ( coverChecks - i );
		const Vector vecTest = vecStart + move;

		if( (!FBitSet(flags, FINDSPOTAWAY_CHECK_SPOT) || FValidateCover( vecTest )) )
		{
			if( MoveToLocation( movementActivity, 0, vecTest, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
			{
				return true;
			}
		}
	}

	return false;
}

bool CBaseMonster::FindLateralCover( const Vector &vecThreat, const Vector &vecViewOffset, float minDist, float maxDist, int flags )
{
	TraceResult tr;
	UTIL_MakeVectors( pev->angles );
	Vector vecRight = gpGlobals->v_right;
	vecRight.z = 0;
	const Vector vecStepRight = vecRight * COVER_DELTA;
	const Vector vecStart = pev->origin;

	const Activity movementActivity = FBitSet(flags, FINDSPOTAWAY_RUN) ? ACT_RUN : ACT_WALK;

	minDist = Q_max(minDist, COVER_DELTA);
	maxDist = Q_max(maxDist, COVER_DELTA);
	const Vector startOffset = vecRight * minDist;
	const int coverChecks = (int)((maxDist - minDist) / COVER_DELTA) + 1; // at least one check

	for( int i = 0; i < coverChecks; i++ )
	{
		const Vector vecLeftTest = vecStart - startOffset - vecStepRight * i;
		const Vector vecRightTest = vecStart + startOffset + vecStepRight * i;

		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine( vecThreat + vecViewOffset, vecLeftTest + pev->view_ofs, ignore_monsters, ignore_glass, ENT( pev )/*pentIgnore*/, &tr );

		if( tr.flFraction != 1.0f )
		{
			if( (!FBitSet(flags, FINDSPOTAWAY_CHECK_SPOT) || FValidateCover( vecLeftTest )) )
			{
				if( MoveToLocation( movementActivity, 0, vecLeftTest, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
				{
					return true;
				}
			}
		}
		
		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine( vecThreat + vecViewOffset, vecRightTest + pev->view_ofs, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr );

		if( tr.flFraction != 1.0f )
		{
			if( (!FBitSet(flags, FINDSPOTAWAY_CHECK_SPOT) || FValidateCover( vecRightTest )) )
			{
				if( MoveToLocation( movementActivity, 0, vecRightTest, BUILDROUTE_NO_NODEROUTE|BUILDROUTE_NO_TRIANGULATION ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CBaseMonster::FindLateralCover( const Vector &vecThreat, const Vector &vecViewOffset )
{
	return FindLateralCover( vecThreat, vecViewOffset, COVER_DELTA, COVER_DELTA * COVER_CHECKS, FINDSPOTAWAY_RUN|FINDSPOTAWAY_CHECK_SPOT );
}

Vector CBaseMonster::ShootAtEnemy( const Vector &shootOrigin )
{
	CBaseEntity *pEnemy = m_hEnemy;

	if (m_pCine != 0 && m_hTargetEnt != 0 && (m_pCine->m_fTurnType == SCRIPT_TURN_FACE))
	{
		return ( m_hTargetEnt->Center() - shootOrigin ).Normalize();
	}
	else if( pEnemy )
	{
		return( ( pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin ) + m_vecEnemyLKP - shootOrigin ).Normalize();
	}
	else
		return gpGlobals->v_forward;
}

Vector CBaseMonster::SpitAtEnemy(const Vector& vecSpitOrigin, float dirRandomDeviation, float *distance)
{
	Vector vecEnemyPosition;
	if (m_pCine && m_hTargetEnt != 0 && m_pCine->PreciseAttack()) // LRC- are we being told to do this by a scripted_action?
		vecEnemyPosition = m_hTargetEnt->pev->origin;
	else if (m_hEnemy != 0)
		vecEnemyPosition = m_hEnemy->BodyTarget(pev->origin);
	else
		vecEnemyPosition = m_vecEnemyLKP;
	const Vector vecDiff = (vecEnemyPosition - vecSpitOrigin);
	if (distance)
	{
		*distance = vecDiff.Length();
	}
	Vector vecSpitDir = vecDiff.Normalize();
	if (dirRandomDeviation > 0)
	{
		vecSpitDir.x += RANDOM_FLOAT( -dirRandomDeviation, dirRandomDeviation );
		vecSpitDir.y += RANDOM_FLOAT( -dirRandomDeviation, dirRandomDeviation );
		vecSpitDir.z += RANDOM_FLOAT( -dirRandomDeviation, 0.0f );
	}
	return vecSpitDir;
}

//=========================================================
// FacingIdeal - tells us if a monster is facing its ideal
// yaw. Created this function because many spots in the 
// code were checking the yawdiff against this magic
// number. Nicer to have it in one place if we're gonna
// be stuck with it.
//=========================================================
bool CBaseMonster::FacingIdeal( void )
{
	return fabs( FlYawDiff() ) <= 0.006f;//!!!BUGBUG - no magic numbers!!!
}

//=========================================================
// FCanActiveIdle
//=========================================================
bool CBaseMonster::FCanActiveIdle( void )
{
	/*
	if( m_MonsterState == MONSTERSTATE_IDLE && m_IdealMonsterState == MONSTERSTATE_IDLE && !IsMoving() )
	{
		return true;
	}
	*/
	return false;
}

void CBaseMonster::CorpseFallThink( void )
{
	if( pev->flags & FL_ONGROUND )
	{
		SetThink( NULL );

		SetSequenceBox( );
		UTIL_SetOrigin( pev, pev->origin );// link into world.
	}
	else
		pev->nextthink = gpGlobals->time + 0.1f;
}

// Call after animation/pose is set up
void CBaseMonster::MonsterInitDead( void )
{
	InitBoneControllers();

	pev->solid		= SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS;// so he'll fall to ground

	pev->frame = 0;
	ResetSequenceInfo();
	pev->framerate = 0;

	// Copy health
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_DEAD;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );

	// Setup health counters, etc.
	BecomeDead();

	if (FBitSet(pev->spawnflags, SF_DEADMONSTER_DONT_DROP))
	{
		pev->movetype = MOVETYPE_FLY;
		SetThink(NULL);
		SetSequenceBox();
		UTIL_SetOrigin( pev, pev->origin );
	}
	else
	{
		SetThink( &CBaseMonster::CorpseFallThink );
		pev->nextthink = gpGlobals->time + 0.5f;
	}

	if ((pev->spawnflags & SF_DEADMONSTER_NOTSOLID) && MyDeadMonsterPointer() != NULL)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
}

//=========================================================
// BBoxIsFlat - check to see if the monster's bounding box
// is lying flat on a surface (traces from all four corners
// are same length.)
//=========================================================
bool CBaseMonster::BBoxFlat( void )
{
	TraceResult	tr;
	Vector		vecPoint;
	float		flXSize, flYSize;
	float		flLengthSqr;
	float		flLength2Sqr;

	flXSize = pev->size.x / 2;
	flYSize = pev->size.y / 2;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	vecPoint.z = pev->origin.z;

	UTIL_TraceLine( vecPoint, vecPoint - Vector( 0, 0, 100 ), ignore_monsters, ENT( pev ), &tr );
	flLengthSqr = ( vecPoint - tr.vecEndPos ).LengthSqr();

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y - flYSize;

	UTIL_TraceLine( vecPoint, vecPoint - Vector( 0, 0, 100 ), ignore_monsters, ENT( pev ), &tr );
	flLength2Sqr = ( vecPoint - tr.vecEndPos ).LengthSqr();
	if( flLength2Sqr > flLengthSqr )
	{
		return false;
	}
	flLengthSqr = flLength2Sqr;

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	UTIL_TraceLine ( vecPoint, vecPoint - Vector( 0, 0, 100 ), ignore_monsters, ENT( pev ), &tr );
	flLength2Sqr = ( vecPoint - tr.vecEndPos ).LengthSqr();
	if( flLength2Sqr > flLengthSqr )
	{
		return false;
	}
	flLengthSqr = flLength2Sqr;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y - flYSize;
	UTIL_TraceLine( vecPoint, vecPoint - Vector( 0, 0, 100 ), ignore_monsters, ENT( pev ), &tr );
	flLength2Sqr = ( vecPoint - tr.vecEndPos ).LengthSqr();
	if( flLength2Sqr > flLengthSqr )
	{
		return false;
	}
	// flLength = flLength2;

	return true;
}

//=========================================================
// Get Enemy - tries to find the best suitable enemy for the monster.
//=========================================================
bool CBaseMonster::GetEnemy( bool forcePopping )
{
	CBaseEntity *pNewEnemy;

	if( HasConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS ) )
	{
		pNewEnemy = BestVisibleEnemy();

		if( pNewEnemy != m_hEnemy && pNewEnemy != NULL )
		{
			// DO NOT mess with the monster's m_hEnemy pointer unless the schedule the monster is currently running will be interrupted
			// by COND_NEW_ENEMY. This will eliminate the problem of monsters getting a new enemy while they are in a schedule that doesn't care,
			// and then not realizing it by the time they get to a schedule that does. I don't feel this is a good permanent fix. 

			if( m_pSchedule )
			{
				if( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
				{
					SetEnemy(pNewEnemy);
					SetConditions( bits_COND_NEW_ENEMY );
				}
				// if the new enemy has an owner, take that one as well
				if( pNewEnemy->pev->owner != NULL )
				{
					CBaseEntity *pOwner = GetMonsterPointer( pNewEnemy->pev->owner );
					if( pOwner && ( pOwner->pev->flags & FL_MONSTER ) && IRelationship( pOwner ) >= R_DL )
						PushEnemy( pOwner, m_vecEnemyLKP );
				}
			}
		}
	}

	// remember old enemies
	if( m_hEnemy == 0 )
	{
		if (forcePopping)
		{
			if (PopEnemy() && m_pSchedule != NULL && (m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY))
			{
				SetConditions( bits_COND_NEW_ENEMY );
			}
		}
		else if (m_pSchedule != NULL && (m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY))
		{
			if (PopEnemy())
				SetConditions( bits_COND_NEW_ENEMY );
		}
	}

	return m_hEnemy != 0;
}

//=========================================================
// DropItem - dead monster drops named item 
//=========================================================
CBaseEntity *CBaseMonster::DropItem( const char *pszItemName, const Vector &vecPos, const Vector &vecAng )
{
	CBaseEntity *pItem = CBaseEntity::Create( pszItemName, vecPos, vecAng, edict() );

	if( pItem )
	{
		// do we want this behavior to be default?! (sjb)
		pItem->pev->velocity = pev->velocity;
		pItem->pev->avelocity = Vector( 0, RANDOM_FLOAT( 0, 100 ), 0 );

		// Dropped items should never respawn (unless this rule changes in the future). - Solokiller
		pItem->pev->spawnflags |= SF_NORESPAWN;
		return pItem;
	}
	else
	{
		ALERT( at_console, "DropItem() - Didn't create!\n" );
		return NULL;
	}
}

void CBaseMonster::FixupDropItemPosition(Vector &pos)
{
	TraceResult tr;
	UTIL_TraceHull(pos, pos + Vector(0,0,1), ignore_monsters, head_hull, edict(), &tr);
	if (tr.fStartSolid || tr.fAllSolid)
		pos = Center();
}

bool CBaseMonster::IsFullyAlive()
{
	return !HasMemory(bits_MEMORY_KILLED) && CBaseToggle::IsFullyAlive();
}

bool CBaseMonster::ShouldFadeOnDeath( void )
{
	// if flagged to fade out or I have an owner (I came from a monster spawner)
	if( ( pev->spawnflags & SF_MONSTER_FADECORPSE ) || !FNullEnt( pev->owner ) )
		return true;

	return false;
}

bool CBaseMonster::ShouldCollide(CBaseEntity *pOther)
{
	if (pev->deadflag == DEAD_DEAD && FBitSet(pev->flags, FL_MONSTER))
		return pOther->ShouldCollideWithCorpses();
	return true;
}

bool CBaseMonster::ShouldCollideWithCorpses()
{
	if (FBitSet(pev->flags, FL_MONSTER))
		return false;
	return true;
}

const char* CBaseMonster::MyNonDefaultGibModel()
{
	if (!FStringNull(m_gibModel))
		return STRING(m_gibModel);

	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate)
	{
		const Visual* gibVisual = g_VisualSystem.GetVisual(entTemplate->GibVisualName());
		if (gibVisual && gibVisual->model)
			return gibVisual->model;
	}

	return nullptr;
}

const Visual* CBaseMonster::MyGibVisual()
{
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate)
		return g_VisualSystem.GetVisual(entTemplate->GibVisualName());
	return nullptr;
}

int CBaseMonster::PrecacheMyGibModel(const char *model)
{
	const char* nonDefaultModel = MyNonDefaultGibModel();
	if (nonDefaultModel)
	{
		return PRECACHE_MODEL(nonDefaultModel);
	}
	if (model)
		return PRECACHE_MODEL(model);
	return 0;
}

void CBaseMonster::SetMyBloodColor(int defaultBloodColor)
{
	if (!m_bloodColor) {
		const EntTemplate* entTemplate = GetMyEntTemplate();
		if (entTemplate && entTemplate->IsBloodDefined())
			m_bloodColor = entTemplate->BloodColor();
		else
			m_bloodColor = defaultBloodColor;
	}
}

void CBaseMonster::SetMyFieldOfView(const float defaultFieldOfView)
{
	if (!m_flFieldOfView) {
		const EntTemplate* entTemplate = GetMyEntTemplate();
		if (entTemplate && entTemplate->IsFielfOfViewDefined())
			m_flFieldOfView = entTemplate->FieldOfView();
		else
			m_flFieldOfView = defaultFieldOfView;
	}
}

int CBaseMonster::Classify()
{
	if (m_iClass == -1)
		return CLASS_NONE;
	if (m_iClass)
		return m_iClass;

	const EntTemplate* entTemplate = GetMyEntTemplate();
	const int defaultClassify = (entTemplate && entTemplate->IsClassifyDefined()) ? entTemplate->Classify() : DefaultClassify();

	if (m_reverseRelationship)
	{
		switch(defaultClassify)
		{
		case CLASS_HUMAN_PASSIVE:
		case CLASS_PLAYER_ALLY:
		case CLASS_PLAYER_ALLY_MILITARY:
			return CLASS_HUMAN_MILITARY;
		case CLASS_NONE:
			return CLASS_NONE;
		default:
			return CLASS_PLAYER_ALLY;
		}
	}
	return defaultClassify;
}

int CBaseMonster::DefaultClassify()
{
	return CLASS_NONE;
}

Vector CBaseMonster::DefaultMinHullSize()
{
	return g_vecZero;
}

Vector CBaseMonster::DefaultMaxHullSize()
{
	return g_vecZero;
}

int CBaseMonster::SizeForGrapple()
{
	if (m_sizeForGrapple < 0)
		return GRAPPLE_NOT_A_TARGET;
	else if (m_sizeForGrapple > 0 && m_sizeForGrapple <= GRAPPLE_FIXED)
		return m_sizeForGrapple;
	else
	{
		const EntTemplate* entTemplate = GetMyEntTemplate();
		if (entTemplate && entTemplate->IsSizeForGrappleDefined())
			return entTemplate->SizeForGrapple();
	}
	return DefaultSizeForGrapple();
}

bool CBaseMonster::HandleBlocker(CBaseEntity* pBlocker, bool duringMovement)
{
	if (!pBlocker)
		return false;

	CBaseMonster* blockerMonster = pBlocker->MyMonsterPointer();
	if (blockerMonster && blockerMonster->CanBeMadeMoveAway(this)) {
		if (DeveloperModeLevel() >= 4)
			ALERT(at_console, "%s (%s) sets %s as blocker (%s)\n", STRING(pev->classname), m_pSchedule ? m_pSchedule->pName : "", STRING(pBlocker->pev->classname), duringMovement ? "movement" : "path searching");
		m_lastMoveBlocker = pBlocker;
		return true;
	}
	return false;
}

bool CBaseMonster::CanBeMadeMoveAway(CBaseEntity *pPusher)
{
	if (FBitSet(pev->flags, FL_CLIENT))
		return false;
	if (FBitSet(pev->spawnflags, SF_MONSTER_IGNORE_PUSH))
		return false;
	int rel = IRelationship(pPusher);
	if (rel == R_AL)
		return true;
	CBaseMonster* pMonster = pPusher->MyMonsterPointer();
	if (pMonster && pMonster->m_pCine)
		return rel == R_NO;
	return false;
}

bool CBaseMonster::MakeMyBlockerMoveAway()
{
	if (m_lastMoveBlocker != 0)
	{
		bool success = false;
		CBaseMonster* blockerMonster = m_lastMoveBlocker->MyMonsterPointer();
		if (blockerMonster && blockerMonster->CanBeMadeMoveAway(this)) {
			const int flags = SUGGEST_SCHEDULE_FLAG_RUN;
			CBaseEntity* pGoalEntity = this;
			if (m_hMoveGoalEnt != 0)
				pGoalEntity = m_hMoveGoalEnt;
			success = blockerMonster->SuggestSchedule(SCHED_RETREAT_FROM_SPOT, pGoalEntity, 0.0f, 256.0f, flags);
		}
		m_lastMoveBlocker = 0;
		return success;
	}
	return false;
}

bool CBaseMonster::IsFreeToManipulate()
{
	return IsFullyAlive() && m_IdealMonsterState != MONSTERSTATE_SCRIPT &&
			m_IdealMonsterState != MONSTERSTATE_PRONE &&
				 (m_MonsterState == MONSTERSTATE_ALERT ||
				  m_MonsterState == MONSTERSTATE_IDLE ||
				  m_MonsterState == MONSTERSTATE_HUNT ||
				  m_MonsterState == MONSTERSTATE_NONE);
}

bool CBaseMonster::HandleDoorBlockage(CBaseEntity *pDoor)
{
	if (g_modFeatures.DoorsFadeCorpsesWhenBlocked())
	{
		if (pev->deadflag == DEAD_DEAD && pev->movetype == MOVETYPE_TOSS && pev->takedamage == DAMAGE_YES) {
			SUB_StartFadeOut();
			return true;
		}
	}
	return false;
}

int CBaseMonster::SharedRandomLong(int low, int high)
{
	int result = UTIL_SharedRandomLong(static_cast<unsigned int>(m_lootRandomSeed), low, high);
	m_lootRandomSeed = UTIL_LastRandomSeed();
	return result;
}

float CBaseMonster::SharedRandomFloat(float low, float high)
{
	float result = UTIL_SharedRandomFloat(static_cast<unsigned int>(m_lootRandomSeed), low, high);
	m_lootRandomSeed = UTIL_LastRandomSeed();
	return result;
}

void CBaseMonster::GlowShellOn(const Visual* visual)
{
	if (!m_glowShellUpdate)
	{
		m_prevRenderColor = pev->rendercolor;
		m_prevRenderAmt = pev->renderamt;
		m_prevRenderFx = pev->renderfx;
		m_prevRenderMode = pev->rendermode;

		if (visual->HasDefined(Visual::ALPHA_DEFINED))
			pev->renderamt = visual->renderamt;
		if (visual->HasDefined(Visual::COLOR_DEFINED))
			pev->rendercolor = VectorFromColor(visual->rendercolor);
		if (visual->HasDefined(Visual::RENDERFX_DEFINED))
			pev->renderfx = visual->renderfx;
		if (visual->HasDefined(Visual::RENDERMODE_DEFINED))
			pev->rendermode = visual->rendermode;

		m_glowShellUpdate = true;
	}
	m_glowShellTime = gpGlobals->time + RandomizeNumberFromRange(visual->life);
}

void CBaseMonster::GlowShellOff( void )
{
	if (m_glowShellUpdate)
	{
		pev->renderamt = m_prevRenderAmt;
		pev->rendercolor = m_prevRenderColor;
		pev->renderfx = m_prevRenderFx;
		pev->rendermode = m_prevRenderMode;

		m_glowShellTime = 0.0f;

		m_glowShellUpdate = false;
	}
}
void CBaseMonster::GlowShellUpdate( void )
{
	if( m_glowShellUpdate )
	{
		if( gpGlobals->time > m_glowShellTime || pev->deadflag == DEAD_DEAD )
			GlowShellOff();
	}
}

void CDeadMonster::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

void CDeadMonster::Precache()
{
	PrecacheMyModel(DefaultModel());
	PrecacheMyGibModel();
}

void CDeadMonster::SpawnHelper(const char* defaultModel, int bloodColor, int health)
{
	Precache();
	SetMyModel(defaultModel);

	pev->effects &= EF_INVLIGHT;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	SetMyBloodColor( bloodColor );

	const char* seqName = getPos(m_iPose);
	pev->sequence = LookupSequence( seqName );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "%s with bad pose (no %s animation in %s)\n", STRING(pev->classname), seqName, defaultModel );
	}
	SetMyHealth( health );
}

void CDeadMonster::SpawnHelper(int bloodColor, int health)
{
	SpawnHelper(DefaultModel(), bloodColor, health);
}

bool CDeadMonster::ShouldCollide(CBaseEntity* pOther)
{
	return pOther->ShouldCollideWithCorpses();
}

#if FEATURE_SKELETON
class CSkeleton : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/skeleton.mdl"; }
	int	DefaultClassify() { return	CLASS_NONE; }
	TakeDamageResult TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo) override;

	const char* getPos(int pos) const;
	static const char *m_szPoses[4];
};

const char *CSkeleton::m_szPoses[] = { "s_onback", "s_sitting", "dead_against_wall", "dead_stomach" };

const char* CSkeleton::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS(monster_skeleton_dead, CSkeleton)

void CSkeleton::Spawn(void)
{
	SpawnHelper(DONT_BLEED);
	MonsterInitDead();
}

TakeDamageResult CSkeleton::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo)
{
	return TakeDamageResult();
}
#endif

//LRC - an entity for monsters to shoot at.
#define SF_MONSTERTARGET_OFF 1
class CMonsterTarget : public CBaseEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int Classify( void ) { return pev->frags; };
};
LINK_ENTITY_TO_CLASS( monster_target, CMonsterTarget );
LINK_ENTITY_TO_CLASS( monster_bullseye, CMonsterTarget ); // for parity with npc_bullseye from Source games

void CMonsterTarget :: Spawn ( void )
{
	if (pev->spawnflags & SF_MONSTERTARGET_OFF)
		pev->health = 0;
	else
		pev->health = 1; // Don't ignore me, I'm not dead. I'm quite well really. I think I'll go for a walk...
	SetBits (pev->flags, FL_MONSTER);
}

void CMonsterTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (ShouldToggle( useType, pev->health > 0.0f ))
	{
		if (pev->health)
			pev->health = 0;
		else
			pev->health = 1;
	}
}
