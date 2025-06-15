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
// Squadmonster  functions
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "saverestore.h"
#include "squadmonster.h"
#include "plane.h"
#include "game.h"

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	CSquadMonster::m_SaveData[] =
{
	DEFINE_FIELD( CSquadMonster, m_hSquadLeader, FIELD_EHANDLE ),
	DEFINE_ARRAY( CSquadMonster, m_hSquadMember, FIELD_EHANDLE, MAX_SQUAD_MEMBERS - 1 ),

	// DEFINE_FIELD( CSquadMonster, m_afSquadSlots, FIELD_INTEGER ), // these need to be reset after transitions!
	DEFINE_FIELD( CSquadMonster, m_fEnemyEluded, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSquadMonster, m_flLastEnemySightTime, FIELD_TIME ),

	DEFINE_FIELD( CSquadMonster, m_iMySlot, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSquadMonster, CBaseMonster )

//=========================================================
// OccupySlot - if any slots of the passed slots are 
// available, the monster will be assigned to one.
//=========================================================
bool CSquadMonster::OccupySlot( int iDesiredSlots )
{
	int i;
	int iMask;
	int iSquadSlots;

	if( !InSquad() )
	{
		return true;
	}

	if( SquadEnemySplit() )
	{
		// if the squad members aren't all fighting the same enemy, slots are disabled
		// so that a squad member doesn't get stranded unable to engage his enemy because
		// all of the attack slots are taken by squad members fighting other enemies.
		m_iMySlot = bits_SLOT_SQUAD_SPLIT;
		return true;
	}

	CSquadMonster *pSquadLeader = MySquadLeader();

	if( !( iDesiredSlots ^ pSquadLeader->m_afSquadSlots ) )
	{
		// none of the desired slots are available. 
		return false;
	}

	iSquadSlots = pSquadLeader->m_afSquadSlots;

	for( i = 0; i < NUM_SLOTS; i++ )
	{
		iMask = 1 << i;
		if( iDesiredSlots & iMask ) // am I looking for this bit?
		{
			if( !( iSquadSlots & iMask ) )	// Is it already taken?
			{
				// No, use this bit
				pSquadLeader->m_afSquadSlots |= iMask;
				m_iMySlot = iMask;
				//ALERT( at_aiconsole, "Took slot %d - %d\n", i, m_hSquadLeader->m_afSquadSlots );
				return true;
			}
		}
	}

	return false;
}

//=========================================================
// VacateSlot
//=========================================================
void CSquadMonster::VacateSlot()
{
	if( m_iMySlot != bits_NO_SLOT && InSquad() )
	{
		//ALERT( at_aiconsole, "Vacated Slot %d - %d\n", m_iMySlot, m_hSquadLeader->m_afSquadSlots );
		MySquadLeader()->m_afSquadSlots &= ~m_iMySlot;
		m_iMySlot = bits_NO_SLOT;
	}
}

//=========================================================
// ScheduleChange
//=========================================================
void CSquadMonster::ScheduleChange ( void )
{
	VacateSlot();
}

void CSquadMonster::OnDying(bool gibbed)
{
	VacateSlot();

	if( InSquad() )
	{
		CSquadMonster* pSquadLeader = MySquadLeader();

		for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
		{
			CSquadMonster* pSquadMember = pSquadLeader->MySquadMember( i );
			if( pSquadMember && pSquadMember != this )
			{
				if (pSquadMember->m_IdealMonsterState == pSquadMember->m_MonsterState &&
						pSquadMember->IsFullyAlive() &&
						pSquadMember->m_hEnemy == 0 && (pSquadMember->m_MonsterState == MONSTERSTATE_IDLE ||
									   pSquadMember->m_MonsterState == MONSTERSTATE_ALERT))
				{
					pSquadMember->m_IdealMonsterState = MONSTERSTATE_HUNT;
					pSquadMember->m_vecEnemyLKP = pev->origin;
					pSquadMember->Remember(bits_MEMORY_SHOULD_GO_TO_LKP);
				}
			}
		}

		pSquadLeader->SquadRemove( this );
	}
	CBaseMonster::OnDying(gibbed);
}

// These functions are still awaiting conversion to CSquadMonster 


//=========================================================
//
// SquadRemove(), remove pRemove from my squad.
// If I am pRemove, promote m_pSquadNext to leader
//
//=========================================================
void CSquadMonster::SquadRemove( CSquadMonster *pRemove )
{
	ASSERT( pRemove!=NULL );
	ASSERT( this->IsLeader() );
	ASSERT( pRemove->m_hSquadLeader == this );

	// If I'm the leader, get rid of my squad
	if( pRemove == MySquadLeader() )
	{
		int squadCountLeft = 0;
		CSquadMonster* newLeader = NULL;

		if (g_modFeatures.monsters_delegate_squad_leadership)
		{
			for( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
			{
				CSquadMonster *pMember = MySquadMember( i );
				if (pMember && pMember->IsFullyAlive())
				{
					squadCountLeft++;

					// choose the healthiest member as a new leader
					if (!newLeader || pMember->pev->health > newLeader->pev->health)
						newLeader = pMember;
				}
			}
		}

		if (newLeader && squadCountLeft > 1)
		{
			newLeader->m_hSquadLeader = newLeader;

			for( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
			{
				CSquadMonster *pMember = MySquadMember( i );
				if ( pMember && pMember != newLeader )
				{
					if (pMember->IsFullyAlive()) {
						pMember->m_hSquadLeader = NULL;
						newLeader->SquadAdd(pMember);
					}
				}
				m_hSquadMember[i] = NULL;
			}
		}
		else
		{
			for( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
			{
				CSquadMonster *pMember = MySquadMember( i );
				if( pMember )
				{
					pMember->m_hSquadLeader = NULL;
					m_hSquadMember[i] = NULL;
				}
			}
		}
	}
	else
	{
		CSquadMonster *pSquadLeader = MySquadLeader();
		if( pSquadLeader )
		{
			for( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
			{
				if( pSquadLeader->m_hSquadMember[i] == pRemove )
				{
					pSquadLeader->m_hSquadMember[i] = NULL;
					break;
				}
			}
		}
	}

	pRemove->m_hSquadLeader = NULL;
}

//=========================================================
//
// SquadAdd(), add pAdd to my squad
//
//=========================================================
bool CSquadMonster::SquadAdd( CSquadMonster *pAdd )
{
	ASSERT( pAdd != NULL );
	ASSERT( !pAdd->InSquad() );
	ASSERT( this->IsLeader() );

	for( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
	{
		if( m_hSquadMember[i] == 0 )
		{
			m_hSquadMember[i] = pAdd;
			pAdd->m_hSquadLeader = this;
			return true;
		}
	}
	return false;
	// should complain here
}

//=========================================================
// 
// SquadPasteEnemyInfo - called by squad members that have
// current info on the enemy so that it can be stored for 
// members who don't have current info.
//
//=========================================================
void CSquadMonster::SquadPasteEnemyInfo( void )
{
	CSquadMonster *pSquadLeader = MySquadLeader();
	if( pSquadLeader )
	{
		pSquadLeader->m_vecEnemyLKP = m_vecEnemyLKP;
		pSquadLeader->m_flLastTimeObservedEnemy = gpGlobals->time;
	}
}

//=========================================================
//
// SquadCopyEnemyInfo - called by squad members who don't
// have current info on the enemy. Reads from the same fields
// in the leader's data that other squad members write to,
// so the most recent data is always available here.
//
//=========================================================
void CSquadMonster::SquadCopyEnemyInfo( void )
{
	CSquadMonster *pSquadLeader = MySquadLeader();
	if( pSquadLeader )
	{
		m_vecEnemyLKP = pSquadLeader->m_vecEnemyLKP;
		m_flLastEnemySightTime = pSquadLeader->m_flLastEnemySightTime;
	}
}

//=========================================================
// 
// SquadMakeEnemy - makes everyone in the squad angry at
// the same entity.
//
//=========================================================
void CSquadMonster::SquadMakeEnemy( CBaseEntity *pEnemy )
{
	if( !InSquad() )
		return;

	if( !pEnemy )
	{
		ALERT( at_console, "ERROR: SquadMakeEnemy() - pEnemy is NULL!\n" );
		return;
	}

	if ( !pEnemy->IsFullyAlive() )
	{
		ALERT( at_console, "ERROR: SquadMakeEnemy() - pEnemy is dead!\n" );
		return;
	}

	CSquadMonster *pSquadLeader = MySquadLeader();
	for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		CSquadMonster *pMember = pSquadLeader->MySquadMember( i );
		if( pMember )
		{
			// reset members who aren't activly engaged in fighting
			if( pMember->m_hEnemy != pEnemy && !pMember->HasConditions( bits_COND_SEE_ENEMY )
					&& ( pMember->m_pSchedule && (pMember->m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY) )
					// My enemy might be not an enemy for member of my squad, e.g. if I was provoked by player.
					&& pMember->IRelationship(pEnemy) >= R_DL )
			{
				// give them a new enemy
				pMember->SetEnemy(pEnemy);
				pMember->SetConditions( bits_COND_NEW_ENEMY );
			}
		}
	}
}

//=========================================================
//
// SquadCount(), return the number of members of this squad
// callable from leaders & followers
//
//=========================================================
int CSquadMonster::SquadCount( void )
{
	if( !InSquad() )
		return 0;

	CSquadMonster *pSquadLeader = MySquadLeader();
	int squadCount = 0;
	for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		if( pSquadLeader->MySquadMember( i ) != NULL )
			squadCount++;
	}

	return squadCount;
}

//=========================================================
//
// SquadRecruit(), get some monsters of my classification and
// link them as a group.  returns the group size
//
//=========================================================
int CSquadMonster::SquadRecruit( int searchRadius, int maxMembers )
{
	int squadCount;
	int iMyClass = Classify();// cache this monster's class

	// Don't recruit if I'm already in a group
	if( InSquad() )
		return 0;

	if( maxMembers < 2 )
		return 0;

	// I am my own leader
	m_hSquadLeader = this;
	squadCount = 1;

	CBaseEntity *pEntity = NULL;

	if( !FStringNull( pev->netname ) )
	{
		// I have a netname, so unconditionally recruit everyone else with that name.
		pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ) );
		while( pEntity )
		{
			CSquadMonster *pRecruit = pEntity->MySquadMonsterPointer();

			if( pRecruit )
			{
				const int rel = pRecruit->IRelationship(this);
				if( !pRecruit->InSquad() && (rel == R_AL || rel == R_NO) && pRecruit != this && (FBitSet(m_afCapability, bits_CAP_SQUAD_ALLOW_OTHER_CLASSIFY) || pRecruit->Classify() == iMyClass) )
				{
					// minimum protection here against user error.in worldcraft. 
					if( !SquadAdd( pRecruit ) )
						break;
					squadCount++;
				}
			}

			pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ) );
		}
	}
	else 
	{
		while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, searchRadius ) ) != NULL )
		{
			CSquadMonster *pRecruit = pEntity->MySquadMonsterPointer();

			if( pRecruit && pRecruit != this && pRecruit->IsFullyAlive() && !pRecruit->m_pCine && !FBitSet(pRecruit->pev->spawnflags, SF_MONSTER_PRISONER) )
			{
				const int rel = pRecruit->IRelationship(this);
				// Can we recruit this guy?
				if( !pRecruit->InSquad() && (rel == R_AL || rel == R_NO) && FStringNull(pRecruit->pev->netname) && !FBitSet(pRecruit->m_afCapability, bits_CAP_SQUAD_DENY) &&
					(FBitSet(m_afCapability, bits_CAP_SQUAD_ALLOW_OTHER_CLASSIFY) || pRecruit->Classify() == iMyClass) &&
					((!FBitSet(m_afCapability, bits_CAP_SQUAD_SAME_CLASSNAME) && !FBitSet(pRecruit->m_afCapability, bits_CAP_SQUAD_SAME_CLASSNAME)) || FStrEq( STRING( pev->classname ), STRING( pRecruit->pev->classname ) )) &&
					((!FBitSet(m_afCapability, bits_CAP_SQUAD_SAME_TEMPLATE) && !FBitSet(pRecruit->m_afCapability, bits_CAP_SQUAD_SAME_TEMPLATE)) || FStrEq(m_entTemplate, pRecruit->m_entTemplate)) )
				{
					TraceResult tr;
					UTIL_TraceLine( pev->origin + pev->view_ofs, pRecruit->pev->origin + pev->view_ofs, ignore_monsters, pRecruit->edict(), &tr );// try to hit recruit with a traceline.
					if( tr.flFraction == 1.0f )
					{
						if( !SquadAdd( pRecruit ) )
							break;

						squadCount++;
					}
				}
			}
		}
	}

	// no single member squads
	if( squadCount == 1 )
	{
		m_hSquadLeader = NULL;
	}

	return squadCount;
}

//=========================================================
// CheckEnemy
//=========================================================
bool CSquadMonster::CheckEnemy( CBaseEntity *pEnemy )
{
	bool iUpdatedLKP = CBaseMonster::CheckEnemy( pEnemy );

	// communicate with squad members about the enemy IF this individual has the same enemy as the squad leader.
	if( InSquad() && pEnemy == MySquadLeader()->m_hEnemy )
	{
		if( iUpdatedLKP )
		{
			// have new enemy information, so paste to the squad.
			SquadPasteEnemyInfo();
		}
		else
		{
			// enemy unseen, copy from the squad knowledge.
			SquadCopyEnemyInfo();
		}
	}

	return iUpdatedLKP;
}

//=========================================================
// StartMonster
//=========================================================
void CSquadMonster::StartMonster( void )
{
	CBaseMonster::StartMonster();

	if( ( FBitSet(m_afCapability, bits_CAP_SQUAD) ) && !InSquad() )
	{
		if( !FStringNull( pev->netname ) )
		{
			// if I have a groupname, I can only recruit if I'm flagged as leader
			if( !( pev->spawnflags & SF_SQUADMONSTER_LEADER ) )
			{
				return;
			}
		}
		else // Don't allow prisoners form squads
		{
			if (FBitSet(pev->spawnflags, SF_MONSTER_PRISONER))
				return;
		}

		// try to form squads now.
		int iSquadSize = SquadRecruit( 1024, 4 );

		if( iSquadSize > 1 )
		{
			ALERT( at_aiconsole, "Squad of %d monsters formed. Leader is %s\n", iSquadSize, STRING( pev->classname ) );
		}

		if( IsLeader() )
		{
			OnBecomingLeader();
		}
	}
}

//=========================================================
// NoFriendlyFire - checks for possibility of friendly fire
//
// Builds a large box in front of the grunt and checks to see 
// if any squad members are in that box. 
//=========================================================
bool CSquadMonster::NoFriendlyFire( void )
{
	//!!!BUGBUG - to fix this, the planes must be aligned to where the monster will be firing its gun, not the direction it is facing!!!
	if( m_hEnemy != 0 )
	{
		UTIL_MakeVectors( UTIL_VecToAngles( m_hEnemy->Center() - pev->origin ) );
	}
	else
	{
		// if there's no enemy, pretend there's a friendly in the way, so the monster won't shoot.
		return false;
	}

	CBaseEntity* pEnemy = m_hEnemy;
	const Vector enemyCenter = pEnemy->Center();
	const Vector gunPos = GetGunPosition();
	const Vector posVecs[3] = {gunPos, gunPos + gpGlobals->v_right * pev->size.x * 1, gpGlobals->v_right * pev->size.x * (-1)};
	const Vector enemyVec[3] = {enemyCenter, enemyCenter + gpGlobals->v_right * pEnemy->pev->size.x * 0.5, enemyCenter + gpGlobals->v_right * pEnemy->pev->size.x * -0.5};
	for (int j=0; j<3; ++j)
	{
		TraceResult tr;
		UTIL_TraceLine(posVecs[j], enemyVec[j], dont_ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction != 1.0 && tr.pHit != 0)
		{
			CBaseMonster* monster = GetMonsterPointer(tr.pHit);
			if (monster != 0 && FBitSet(monster->pev->flags, FL_MONSTER|FL_CLIENT) && monster->pev->deadflag != DEAD_DEAD && IRelationship(monster) == R_AL)
			{
				//ALERT(at_aiconsole, "%s: Ally %s at fire line. Don't shoot!\n", STRING(pev->classname), STRING(monster->pev->classname));
				return false;
			}
		}
	}

	const bool inSquad = InSquad();
	CBaseEntity* player = UTIL_PlayerByIndex(1);
	const bool friendWithPlayer = player != 0 && IRelationship(player) == R_AL; // TODO: better check for player relationship
	if( !inSquad && !friendWithPlayer )
	{
		return true;
	}

	CPlane backPlane;
	CPlane leftPlane;
	CPlane rightPlane;
	CPlane frontPlane;

	Vector vecLeftSide;
	Vector vecRightSide;
	Vector v_left;
	Vector v_dir;

	v_dir = gpGlobals->v_right * ( pev->size.x * 1.5f );
	vecLeftSide = pev->origin - v_dir;
		vecRightSide = pev->origin + v_dir;

	v_left = gpGlobals->v_right * -1.0f;

	leftPlane.InitializePlane( gpGlobals->v_right, vecLeftSide );
	rightPlane.InitializePlane( v_left, vecRightSide );
	backPlane.InitializePlane( gpGlobals->v_forward, pev->origin );
	frontPlane.InitializePlane( gpGlobals->v_forward * -1, enemyCenter + gpGlobals->v_forward * pEnemy->pev->size.Length2D() / 2 );
/*
	ALERT( at_console, "LeftPlane: %f %f %f : %f\n", leftPlane.m_vecNormal.x, leftPlane.m_vecNormal.y, leftPlane.m_vecNormal.z, leftPlane.m_flDist );
	ALERT( at_console, "RightPlane: %f %f %f : %f\n", rightPlane.m_vecNormal.x, rightPlane.m_vecNormal.y, rightPlane.m_vecNormal.z, rightPlane.m_flDist );
	ALERT( at_console, "BackPlane: %f %f %f : %f\n", backPlane.m_vecNormal.x, backPlane.m_vecNormal.y, backPlane.m_vecNormal.z, backPlane.m_flDist );
*/
	const bool enemyIsAlive = pEnemy->IsFullyAlive();
	if (inSquad)
	{
		CSquadMonster *pSquadLeader = MySquadLeader();
		for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
		{
			CSquadMonster *pMember = pSquadLeader->MySquadMember( i );
			if( pMember && pMember != this )
			{
				if( backPlane.PointInFront( pMember->pev->origin ) &&
					leftPlane.PointInFront( pMember->pev->origin ) &&
					rightPlane.PointInFront( pMember->pev->origin ) )
				{
					// this guy is in the check volume! Don't shoot!
					if (frontPlane.PointInFront( pMember->pev->origin ))
						return false;
					else if (!enemyIsAlive) // don't shoot when ally is behind the dying enemy
						return false;
				}
			}
		}
	}
	for( int k = 1; k <= gpGlobals->maxClients; k++ )
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(k);
		if (pPlayer && pPlayer->IsPlayer() && IRelationship(pPlayer) == R_AL)
		{
			if( backPlane.PointInFront( pPlayer->pev->origin ) &&
				leftPlane.PointInFront( pPlayer->pev->origin ) &&
				rightPlane.PointInFront( pPlayer->pev->origin ) )
			{
				//ALERT(at_aiconsole, "%s: Ally player at fire plane!\n", STRING(pev->classname));
				// player is in the check volume! Don't shoot!
				if (frontPlane.PointInFront( pPlayer->pev->origin ))
					return false;
				else if (!enemyIsAlive) // don't shoot when ally is behind the dying enemy
					return false;
			}
		}
	}

	return true;
}

//=========================================================
// GetIdealState - surveys the Conditions information available
// and finds the best new state for a monster.
//=========================================================
MONSTERSTATE CSquadMonster::GetIdealState ( void )
{
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
		if( HasConditions( bits_COND_NEW_ENEMY ) && InSquad() )
		{
			SquadMakeEnemy( m_hEnemy );
		}
		break;
	default:
		break;
	}

	return CBaseMonster::GetIdealState();
}

//=========================================================
// FValidateCover - determines whether or not the chosen
// cover location is a good one to move to. (currently based
// on proximity to others in the squad)
//=========================================================
bool CSquadMonster::FValidateCover( const Vector &vecCoverLocation )
{
	if( !InSquad() )
	{
		return true;
	}

	if( AllyMonsterInRange( vecCoverLocation, 128 ) )
	{
		// another squad member is too close to this piece of cover.
		return false;
	}

	return true;
}

//=========================================================
// SquadEnemySplit- returns true if not all squad members
// are fighting the same enemy. 
//=========================================================
bool CSquadMonster::SquadEnemySplit( void )
{
	if( !InSquad() )
		return false;

	CSquadMonster *pSquadLeader = MySquadLeader();
	CBaseEntity *pEnemy = pSquadLeader->m_hEnemy;

	for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		CSquadMonster *pMember = pSquadLeader->MySquadMember( i );
		if( pMember != NULL && pMember->m_hEnemy != 0 && pMember->m_hEnemy != pEnemy )
		{
			return true;
		}
	}
	return false;
}

//=========================================================
// SquadMemberInRange - determines whether or not squad members
// or ally monsters are in the chosen location.
//=========================================================
bool CSquadMonster::AllyMonsterInRange( const Vector &vecLocation, float flDist )
{
	if( InSquad() )
	{
		CSquadMonster *pSquadLeader = MySquadLeader();

		for( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
		{
			CSquadMonster *pSquadMember = pSquadLeader->MySquadMember( i );
			if( pSquadMember && ( vecLocation - pSquadMember->pev->origin ).IsLength2DLessThanOrEqual(flDist) )
				return true;
		}
	}
	CBaseEntity* pEntity = NULL;
	while ( (pEntity = UTIL_FindEntityInSphere(pEntity, vecLocation, flDist)) != NULL )
	{
		CBaseMonster* monster = pEntity->MyMonsterPointer();
		if (monster != 0 && FBitSet(monster->pev->flags, FL_MONSTER|FL_CLIENT) && monster->pev->deadflag != DEAD_DEAD && IRelationship(monster) == R_AL)
		{
			ALERT(at_aiconsole, "%s: Ally %s at search radius.\n", STRING(pev->classname), STRING(monster->pev->classname));
			return true;
		}

	}
	return false;
}

extern Schedule_t slChaseEnemyFailed[];

Schedule_t *CSquadMonster::GetScheduleOfType( int iType )
{
	switch( iType )
	{
	case SCHED_CHASE_ENEMY_FAILED:
		{
			return &slChaseEnemyFailed[0];
		}
	default:
		return CBaseMonster::GetScheduleOfType( iType );
	}
}

void CSquadMonster::StartTask(Task_t *pTask)
{
	switch (pTask->iTask) {
	case TASK_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_NOFIRE );
		}
		TaskComplete();
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CSquadMonster::ReportAIState(ALERT_TYPE level)
{
	CBaseMonster::ReportAIState(level);
	ALERT(at_console, "\n");
	if( !InSquad() )
	{
		ALERT( level, "Not In Squad. " );
	}
	else
	{
		if (pev->netname)
		{
			ALERT( level, "In Squad '%s' ", STRING(pev->netname) );
		}
		else
		{
			ALERT( level, "In Squad " );
		}

		ALERT( level, "of %d members, ", SquadCount() );

		if( IsLeader() )
		{
			ALERT( level, "Squad Leader. " );
		}
		else
		{
			CSquadMonster* myLeader = MySquadLeader();
			ALERT( level, "My Squad Leader: '%s'. ", FStringNull(myLeader->pev->targetname) ? STRING(myLeader->pev->classname) : STRING(myLeader->pev->targetname) );
		}
	}
}
