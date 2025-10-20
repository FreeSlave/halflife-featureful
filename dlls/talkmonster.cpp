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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"talkmonster.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"animation.h"
#include	"string_utils.h"

//=========================================================
// Talking monster base class
// Used for scientists and barneys
//=========================================================
float CTalkMonster::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once

#define SF_TALKMONSTER_DONTGREET_PLAYER (1 << 17)
#define SF_TALKMONSTER_DONT_TALK_TO_PLAYER (1 << 18)

#define CALL_MEDIC_DELAY				5 // Wait before calling for medic again.

#define FLINCH_DELAY 2

TYPEDESCRIPTION	CTalkMonster::m_SaveData[] =
{
	DEFINE_FIELD( CTalkMonster, m_bitsSaid, FIELD_INTEGER ),
	DEFINE_FIELD( CTalkMonster, m_nSpeak, FIELD_INTEGER ),

	DEFINE_FIELD( CTalkMonster, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CTalkMonster, m_useTime, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonster, m_iszUnUse, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonster, m_iszSpeakAs, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonster, m_flLastSaidSmelled, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_flStopTalkTime, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTalkMonster, m_fStartSuspicious, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTalkMonster, m_iszDecline, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonster, m_flMedicWaitTime, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_iTolerance, FIELD_SHORT ),
	DEFINE_FIELD( CTalkMonster, m_alertFriendsPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CTalkMonster, m_alertableByFriends, FIELD_SHORT ),
	DEFINE_FIELD( CTalkMonster, m_flLastHitByPlayer, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_iPlayerHits, FIELD_INTEGER ),
	DEFINE_FIELD( CTalkMonster, m_flStopLookTime, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_allowUseScriptedSentence, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CTalkMonster, CFollowingMonster )

CTalkMonster::TalkFriend CTalkMonster::m_szFriends[TLK_CFRIENDS] = {};
char CTalkMonster::m_szMedics[NUM_MEDICS][64] = {};

//=========================================================
// AI Schedules Specific to talking monsters
//=========================================================
Task_t tlIdleResponse[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },	// Stop and listen
	{ TASK_WAIT, 0.5f },	// Wait until sure it's me they are talking to
	{ TASK_TLK_EYECONTACT, 0.0f },	// Wait until speaker is done
	{ TASK_TLK_RESPOND, 0.0f },	// Wait and then say my response
	{ TASK_TLK_IDEALYAW, 0.0f },	// look at who I'm talking to
	{ TASK_FACE_IDEAL, 0.0f }, 
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_TLK_EYECONTACT, 0.0f },	// Wait until speaker is done
};

Schedule_t slIdleResponse[] =
{
	{
		tlIdleResponse,
		ARRAYSIZE( tlIdleResponse ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"Idle Response"
	},
};

Task_t tlIdleSpeak[] =
{
	{ TASK_TLK_SPEAK, 0.0f },// question or remark
	{ TASK_TLK_IDEALYAW, 0.0f },// look at who I'm talking to
	{ TASK_FACE_IDEAL, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE	},
	{ TASK_TLK_EYECONTACT, 0.0f },
	{ TASK_WAIT_RANDOM, 0.5f },
};

Schedule_t slIdleSpeak[] =
{
	{
		tlIdleSpeak,
		ARRAYSIZE( tlIdleSpeak ),
		bits_COND_NEW_ENEMY |
		bits_COND_CLIENT_PUSH |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Speak"
	},
};

Task_t	tlIdleSpeakWait[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },// Stop and talk
	{ TASK_TLK_SPEAK, 0.0f },// question or remark
	{ TASK_TLK_EYECONTACT, 0.0f },// 
	{ TASK_WAIT, 2.0f },// wait - used when sci is in 'use' mode to keep head turned
};

Schedule_t slIdleSpeakWait[] =
{
	{
		tlIdleSpeakWait,
		ARRAYSIZE( tlIdleSpeakWait ),
		bits_COND_NEW_ENEMY |
		bits_COND_CLIENT_PUSH |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Speak Wait"
	},
};

Task_t tlIdleHello[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },// Stop and talk
	{ TASK_TLK_HELLO, 0.0f },// Try to say hello to player
	{ TASK_TLK_EYECONTACT, 0.0f },
	{ TASK_WAIT, 0.5f },// wait a bit
	{ TASK_TLK_HELLO, 0.0f },// Try to say hello to player
	{ TASK_TLK_EYECONTACT, 0.0f },
	{ TASK_WAIT, 0.5f },// wait a bit
	{ TASK_TLK_HELLO, 0.0f },// Try to say hello to player
	{ TASK_TLK_EYECONTACT, 0.0f },
	{ TASK_WAIT, 0.5f },// wait a bit
	{ TASK_TLK_HELLO, 0.0f },// Try to say hello to player
	{ TASK_TLK_EYECONTACT, 0.0f },
	{ TASK_WAIT, 0.5f },// wait a bit
};

Schedule_t slIdleHello[] =
{
	{
		tlIdleHello,
		ARRAYSIZE( tlIdleHello ),
		bits_COND_NEW_ENEMY |
		bits_COND_CLIENT_PUSH |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Idle Hello"
	},
};

Task_t tlIdleStopShooting[] =
{
	{ TASK_TLK_STOPSHOOTING, 0.0f },// tell player to stop shooting friend
	// { TASK_TLK_EYECONTACT, 0.0f },// look at the player
};

Schedule_t slIdleStopShooting[] =
{
	{
		tlIdleStopShooting,
		ARRAYSIZE( tlIdleStopShooting ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		0,
		"Idle Stop Shooting"
	},
};

Task_t tlIdleTlkStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t slIdleTlkStand[] =
{
	{
		tlIdleTlkStand,
		ARRAYSIZE( tlIdleTlkStand ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"IdleTlkStand"
	},
};

Task_t tlTlkIdleWatchClient[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_TLK_LOOK_AT_CLIENT, 6.0f },
};

Task_t tlTlkIdleWatchClientStare[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_TLK_CLIENT_STARE, 6.0f },
	{ TASK_TLK_STARE, 0.0f },
	{ TASK_TLK_IDEALYAW, 0.0f },// look at who I'm talking to
	{ TASK_FACE_IDEAL, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_TLK_EYECONTACT, 0.0f },
};

Schedule_t slTlkIdleWatchClient[] =
{
	{
		tlTlkIdleWatchClient,
		ARRAYSIZE( tlTlkIdleWatchClient ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_CLIENT_PUSH |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_CLIENT_UNSEEN |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"TlkIdleWatchClient"
	},

	{
		tlTlkIdleWatchClientStare,
		ARRAYSIZE( tlTlkIdleWatchClientStare ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_CLIENT_PUSH |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_CLIENT_UNSEEN |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		bits_SOUND_PLAYER_IF_NOT_ALLY |
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"TlkIdleWatchClientStare"
	},
};

Task_t	tlTlkIdleEyecontact[] =
{
	{ TASK_TLK_IDEALYAW, 0.0f },// look at who I'm talking to
	{ TASK_FACE_IDEAL, 0.0f }, 
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_TLK_EYECONTACT, 0.0f },// Wait until speaker is done
};

Schedule_t slTlkIdleEyecontact[] =
{
	{
		tlTlkIdleEyecontact,
		ARRAYSIZE( tlTlkIdleEyecontact ), 
		bits_COND_NEW_ENEMY |
		bits_COND_CLIENT_PUSH |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"TlkIdleEyecontact"
	},
};

DEFINE_CUSTOM_SCHEDULES( CTalkMonster )
{
	slIdleResponse,
	slIdleSpeak,
	slIdleHello,
	slIdleSpeakWait,
	slIdleStopShooting,
	slIdleTlkStand,
	slTlkIdleWatchClient,
	&slTlkIdleWatchClient[1],
	slTlkIdleEyecontact,
};

IMPLEMENT_CUSTOM_SCHEDULES( CTalkMonster, CFollowingMonster )

void CTalkMonster::SetActivity( Activity newActivity )
{
//	if( newActivity == ACT_IDLE && IsTalking() )
//		newActivity = ACT_SIGNAL3;

//	if( newActivity == ACT_SIGNAL3 && ( LookupActivity( ACT_SIGNAL3 ) == ACTIVITY_NOT_AVAILABLE ) )
//		newActivity = ACT_IDLE;

	CFollowingMonster::SetActivity( newActivity );
}

bool CTalkMonster::CanCallThisMedic(CSquadMonster* pOther)
{
	if ( pOther != 0 && pOther != this && pOther->pev->deadflag == DEAD_NO )
	{
		if ( pOther->ReadyToHeal() && pOther->IsFreeToManipulate() )
		{
			return true;
		}
	}
	return false;
}

void CTalkMonster::PlayCallForMedic()
{
	if (IsHeavilyWounded() && !FBitSet( m_bitsSaid, bit_saidWoundHeavy ))
	{
		PlaySentence( SentenceGroup(TLK_MORTAL), 2, VOL_NORM, ATTN_NORM );
		SetBits( m_bitsSaid, bit_saidWoundHeavy );
	}
	else if (!FBitSet( m_bitsSaid, bit_saidWoundLight ))
	{
		PlaySentence( SentenceGroup(TLK_WOUND), 2, VOL_NORM, ATTN_NORM );
		SetBits( m_bitsSaid, bit_saidWoundLight );
	}
}

void CTalkMonster::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_TLK_SPEAK:
		// ask question or make statement
		FIdleSpeak();
		TaskComplete();
		break;
	case TASK_TLK_RESPOND:
		// respond to question
		IdleRespond();
		TaskComplete();
		break;
	case TASK_TLK_HELLO:
		// greet player
		FIdleHello();
		TaskComplete();
		break;
	case TASK_TLK_STARE:
		// let the player know I know he's staring at me.
		FIdleStare();
		TaskComplete();
		break;
	case TASK_FACE_PLAYER:
	case TASK_TLK_LOOK_AT_CLIENT:
	case TASK_TLK_CLIENT_STARE:
		// track head to the client for a while.
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		break;
	case TASK_TLK_EYECONTACT:
		break;
	case TASK_TLK_IDEALYAW:
		if( m_hTalkTarget != 0 )
		{
			pev->yaw_speed = 60;
			float yaw = VecToYaw( m_hTalkTarget->pev->origin - pev->origin ) - pev->angles.y;

			if( yaw > 180 )
				yaw -= 360;
			if( yaw < -180 )
				yaw += 360;

			if( yaw < 0 )
			{
				pev->ideal_yaw = Q_min( yaw + 45, 0 ) + pev->angles.y;
			}
			else
			{
				pev->ideal_yaw = Q_max( yaw - 45, 0 ) + pev->angles.y;
			}
		}
		TaskComplete();
		break;
	case TASK_TLK_HEADRESET:
		// reset head position after looking at something
		if (m_flStopLookTime <= gpGlobals->time)
			m_hTalkTarget = NULL;
		TaskComplete();
		break;
	case TASK_TLK_STOPSHOOTING:
		// tell player to stop shooting
		PlaySentence( SentenceGroup(TLK_NOSHOOT), RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_NORM );
		TaskComplete();
		break;
	case TASK_CANT_FOLLOW:
		StopFollowing( false, false );
		PlaySentence( SentenceGroup(TLK_STOP), RANDOM_FLOAT( 2, 2.5 ), VOL_NORM, ATTN_NORM );
		TaskComplete();
		break;
	case TASK_PLAY_SCRIPT:
		if ( !m_pCine || !FBitSet(m_pCine->pev->spawnflags, SF_SCRIPT_DONT_RESET_HEAD) ) {
			m_hTalkTarget = NULL;
		}
		CFollowingMonster::StartTask( pTask );
		break;
	case TASK_SMALL_FLINCH:
		m_flNextFlinch = gpGlobals->time + FLINCH_DELAY;
		CFollowingMonster::StartTask( pTask );
		break;
	case TASK_MOVE_NEAREST_TO_TARGET_RANGE:
		CFollowingMonster::StartTask( pTask );
		if (!HasConditions(bits_COND_TASK_FAILED) && GotIdleSpeakChance())
		{
			FIdleSpeak();
		}
		break;
	default:
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

void CTalkMonster::RunTask( Task_t *pTask )
{
	CBaseEntity *pPlayer;

	switch( pTask->iTask )
	{
	case TASK_TLK_CLIENT_STARE:
	case TASK_TLK_LOOK_AT_CLIENT:
		// track head to the client for a while.
		if( m_MonsterState == MONSTERSTATE_IDLE &&
			 !IsMoving() &&
			 !IsTalking() )
		{
			pPlayer = PlayerToFace();

			if( pPlayer )
			{
				IdleHeadTurn( pPlayer->pev->origin );
			}
		}
		else
		{
			// started moving or talking
			TaskFail("started moving or talking");
			return;
		}

		if( pTask->iTask == TASK_TLK_CLIENT_STARE )
		{
			if (!pPlayer)
			{
				TaskFail("no player");
				return;
			}

			// fail out if the player looks away or moves away.
			if( ( pPlayer->pev->origin - pev->origin ).IsLength2DGreaterThan(TLK_STARE_DIST) )
			{
				// player moved away.
				TaskFail("player moved away");
			}

			UTIL_MakeVectors( pPlayer->pev->angles );
			if( UTIL_DotPoints( pPlayer->pev->origin, pev->origin, gpGlobals->v_forward ) < m_flFieldOfView )
			{
				// player looked away
				TaskFail("player looked away");
			}
		}

		if( gpGlobals->time > m_flWaitFinished )
		{
			TaskComplete();
		}
		break;
	case TASK_TLK_EYECONTACT:
		if( !IsMoving() && IsTalking() && m_hTalkTarget != 0 )
		{
			// ALERT( at_console, "waiting %f\n", m_flStopTalkTime - gpGlobals->time );
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			TaskComplete();
		}
		break;
	case TASK_WAIT_FOR_MOVEMENT:
		if( IsTalking() && m_hTalkTarget != 0 )
		{
			// ALERT( at_console, "walking, talking\n" );
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			IdleHeadTurn( pev->origin );
			// override so that during walk, a scientist may talk and greet player
			FIdleHello();
			if( GotIdleSpeakChance() )
			{
				FIdleSpeak();
			}
		}

		CFollowingMonster::RunTask( pTask );
		if( TaskIsComplete() )
			IdleHeadTurn( pev->origin );
		break;
	default:
		if( (IsTalking() || m_flStopLookTime > gpGlobals->time) && m_hTalkTarget != 0 )
		{
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			SetBoneController( 0, 0 );
		}
		CFollowingMonster::RunTask( pTask );
	}
}

KilledResult CTalkMonster::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	// If a client killed me (unless I was already Barnacle'd), make everyone else mad/afraid of him
	if( ( pevAttacker->flags & FL_CLIENT) && m_MonsterState != MONSTERSTATE_PRONE
			&& !HasMemory( bits_MEMORY_KILLED ) // corpses don't alert friends upon gibbing
			&& ShouldAlertFriendsOnDeath()
			&& IsFriendWithPlayerBeforeProvoked() ) // no point in alerting friends if player is already foe
	{
		AlertFriends();
		LimitFollowers( CBaseEntity::Instance( pevAttacker ), 0 );
	}
	return CFollowingMonster::Killed( pevInflictor, pevAttacker, iGib );
}

void CTalkMonster::OnDying(bool gibbed)
{
	// Don't finish that sentence
	StopTalking();
	CFollowingMonster::OnDying(gibbed);
}

void CTalkMonster::StartMonster()
{
	CFollowingMonster::StartMonster();
	if (m_fStartSuspicious) {
		ALERT(at_console, "%s Pre-Provoked\n", STRING(pev->classname));
		Remember(bits_MEMORY_PROVOKED);
	}
}

CBaseEntity *CTalkMonster::EnumFriends(CBaseEntity *pPrevious, int listNumber, bool bTrace )
{
	TalkFriend &talkFriend = m_szFriends[ listNumber ];
	const char *pszFriend = talkFriend.name;
	return EnumFriends(pPrevious, pszFriend, bTrace);
}

CBaseEntity *CTalkMonster::EnumFriends(CBaseEntity *pPrevious, const char* pszFriend, bool bTrace )
{
	CBaseEntity *pFriend = pPrevious;
	TraceResult tr;
	Vector vecCheck;

	if (!pszFriend || !*pszFriend)
		return NULL;

	while( ( pFriend = UTIL_FindEntityByClassname( pFriend, pszFriend ) ) )
	{
		if( pFriend == this || !pFriend->IsFullyAlive() )
			// don't talk to self or dead people
			continue;
		// has friend classname, but not friend really
		const int rel = IRelationship(pFriend);
		if ( rel >= R_DL || rel == R_FR )
			continue;
		if( bTrace )
		{
			vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			UTIL_TraceLine( pev->origin, vecCheck, ignore_monsters, ENT( pev ), &tr );
		}
		else
			tr.flFraction = 1.0f;

		if( tr.flFraction == 1.0f )
		{
			return pFriend;
		}
	}

	return NULL;
}

void CTalkMonster::AlertFriends()
{
	int i;

	// for each friend in this bsp...
	for( i = 0; i < TLK_CFRIENDS; i++ )
	{
		CBaseEntity *pFriend = NULL;
		while( ( pFriend = EnumFriends( pFriend, i, true ) ) != NULL )
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if( pMonster && pMonster->IsFullyAlive() )
			{
				// don't provoke a friend that's playing a death animation. They're a goner
				CTalkMonster* pTalkMonster = pMonster->MyTalkMonsterPointer();
				if (pTalkMonster->AlertableByFriendDeath())
					pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
			}
		}
	}
}

void CTalkMonster::ShutUpFriends()
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while( ( pFriend = EnumFriends( pFriend, i, true ) ) )
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if( pMonster )
			{
				pMonster->SentenceStop();
			}
		}
	}
}

void CTalkMonster::StartFollowing(CBaseEntity *pLeader, bool saySentence)
{
	CFollowingMonster::StartFollowing(pLeader, saySentence);
	SetBits( m_bitsSaid, bit_saidHelloPlayer );	// Don't say hi after you've started following
}

// UNDONE: Keep a follow time in each follower, make a list of followers in this function and do LRU
// UNDONE: Check this in Restore to keep restored monsters from joining a full list of followers
void CTalkMonster::LimitFollowers( CBaseEntity *pPlayer, int maxFollowers )
{
	if (maxFollowers < 0)
		return;
	int i, count = 0;

	// for each friend in this bsp...
	for( i = 0; i < TLK_CFRIENDS; i++ )
	{
		TalkFriend& talkFriend = m_szFriends[i];
		if (!talkFriend.name[0])
			break;
		if (!talkFriend.canFollow) // no sense in limiting friends who can't move, like sitting scientists
			continue;
		if (maxFollowers && talkFriend.category != TalkFriendCategory()) // so scientists and security guards won't limit soldiers
			continue;
		CBaseEntity *pFriend = NULL;
		while( ( pFriend = EnumFriends( pFriend, talkFriend.name, false ) ) )
		{
			CBaseMonster* pMonster = pFriend->MyMonsterPointer();
			if (pMonster)
			{
				CTalkMonster *pTalkMonster = pMonster->MyTalkMonsterPointer();
				if( pTalkMonster )
				{
					if( pTalkMonster->FollowedPlayer() == pPlayer )
					{
						count++;
						if( count > maxFollowers )
							pTalkMonster->StopFollowing( true );
					}
				}
			}

		}
	}
}

bool CTalkMonster::InScriptedSentence()
{
	// Don't allow use during a scripted_sentence
	return m_useTime > gpGlobals->time;
}

bool CTalkMonster::AllowUseDuringScriptedSentence()
{
	return m_allowUseScriptedSentence;
}

void CTalkMonster::PlayUseSentence()
{
	if (PlaySentence( SentenceGroup(TLK_USE), RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE ))
		m_hTalkTarget = FollowedPlayer();
}

void CTalkMonster::PlayUnUseSentence()
{
	if (PlaySentence( SentenceGroup(TLK_UNUSE), RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE ))
		m_hTalkTarget = FollowedPlayer();
}

void CTalkMonster::DeclineFollowing(CBaseEntity *pCaller)
{
	if (PlaySentence( SentenceGroup(TLK_DECLINE), 2.0f, VOL_NORM, ATTN_NORM ))
		m_hTalkTarget = pCaller;
}

float CTalkMonster::TargetDistance()
{
	// If we lose the player, or he dies, return a really large distance
	if( m_hTargetEnt == 0 || !m_hTargetEnt->IsFullyAlive() )
		return 1e6;

	return ( m_hTargetEnt->pev->origin - pev->origin ).Length();
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTalkMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 25% of the time
		if( RANDOM_LONG( 0, 99 ) < 75 )
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:				// Play a named sentence group
		ShutUpFriends();
		PlaySentence( pEvent->options, RANDOM_FLOAT( 2.8f, 3.4f ), VOL_NORM, ATTN_IDLE );
		//ALERT( at_console, "script event speak\n" );
		break;
	default:
		CFollowingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

// monsters derived from ctalkmonster should call this in precache()
void CTalkMonster::TalkInit()
{
	// every new talking monster must reset this global, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	CTalkMonster::g_talkWaitTime = 0;

	if (FBitSet(pev->spawnflags, SF_TALKMONSTER_DONTGREET_PLAYER))
		SetBits(m_bitsSaid, bit_saidHelloPlayer);
}	
//=========================================================
// FindNearestFriend
// Scan for nearest, visible friend. If fPlayer is true, look for
// nearest player
//=========================================================
CBaseEntity *CTalkMonster::FindNearestFriend(bool fPlayer )
{
	CBaseEntity *pFriend = NULL;
	CBaseEntity *pNearest = NULL;
	float range = 10000000.0;
	TraceResult tr;
	Vector vecStart = pev->origin;
	Vector vecCheck;
	const char *pszFriend;
	int cfriends;

	vecStart.z = pev->absmax.z;

	if( fPlayer ) {
		if (IsFriendWithPlayerBeforeProvoked()) {
			cfriends = 1;
		} else {
			return NULL;
		}
	} else {
		cfriends = TLK_CFRIENDS;
	}

	// for each type of friend...
	for( int i = 0; i < cfriends; ++i )
	{
		if( fPlayer )
			pszFriend = "player";
		else
			pszFriend = m_szFriends[ i ].name;

		if( !pszFriend || !*pszFriend )
			break;

		// for each friend in this bsp...
		while( ( pFriend = UTIL_FindEntityByClassname( pFriend, pszFriend ) ) )
		{
			if( pFriend == this || !pFriend->IsFullyAlive() )
				// don't talk to self or dead people
				continue;

			CBaseMonster *pMonster = pFriend->MyMonsterPointer();

			// If not a monster for some reason, or in a script, or prone
			if( !pMonster || pMonster->m_MonsterState == MONSTERSTATE_SCRIPT || pMonster->m_MonsterState == MONSTERSTATE_PRONE )
				continue;
			
			// has friend classname, but not friend really
			const int rel = IRelationship(pMonster);
			if ( rel >= R_DL || rel == R_FR ) {
				continue;
			}

			vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			// if closer than previous friend, and in range, see if he's visible
			if( range > ( vecStart - vecCheck ).Length())
			{
				UTIL_TraceLine( vecStart, vecCheck, ignore_monsters, ENT( pev ), &tr );

				if( tr.flFraction == 1.0f )
				{
					// visible and in range, this is the new nearest scientist
					if( ( vecStart - vecCheck ).IsLengthLessThan(TALKRANGE_MIN) )
					{
						pNearest = pFriend;
						range = ( vecStart - vecCheck ).Length();
					}
				}
			}
		}
	}
	return pNearest;
}

int CTalkMonster::GetVoicePitch()
{
	return (m_voicePitch ? m_voicePitch : GetDefaultVoicePitch()) + RANDOM_LONG( 0, 3 );
}

void CTalkMonster::PrepareVoicePitch()
{
	if (m_voicePitch <= 0)
		m_voicePitch = GetDefaultVoicePitch();
}

bool CTalkMonster::CanBeMadeMoveAway(CBaseEntity *pPusher)
{
	// Stay put during speech
	return CFollowingMonster::CanBeMadeMoveAway(pPusher) && !InScriptedSentence();
}

//=========================================================
// IdleRespond
// Respond to a previous question
//=========================================================
void CTalkMonster::IdleRespond()
{
	// play response
	PlaySentence( SentenceGroup(TLK_ANSWER), RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE );
}

bool CTalkMonster::AskQuestion(float duration)
{
	const char *szQuestionGroup;

	if( FBitSet( pev->spawnflags, SF_MONSTER_PREDISASTER ) )
		szQuestionGroup = SentenceGroup(TLK_PQUESTION);
	else
		szQuestionGroup = SentenceGroup(TLK_QUESTION);

	return PlaySentence( szQuestionGroup, duration, VOL_NORM, ATTN_IDLE );
}

void CTalkMonster::MakeIdleStatement()
{
	const char *szIdleGroup;

	// set idle groups based on pre/post disaster
	if( FBitSet( pev->spawnflags, SF_MONSTER_PREDISASTER ) )
		szIdleGroup = SentenceGroup(TLK_PIDLE);
	else
		szIdleGroup = SentenceGroup(TLK_IDLE);

	PlaySentence( szIdleGroup, RandomSentenceDuraion(), VOL_NORM, ATTN_IDLE );
}

float CTalkMonster::RandomSentenceDuraion()
{
	if( FBitSet( pev->spawnflags, SF_MONSTER_PREDISASTER ) )
		return RANDOM_FLOAT( 4.8f, 5.2f );
	else
		return RANDOM_FLOAT( 2.8f, 3.2f );
}

bool CTalkMonster::FOkToSpeak(int speakFlags )
{
	// if in the grip of a barnacle, don't speak
	if( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
	{
		return false;
	}

	// if not alive, certainly don't speak
	if( pev->deadflag != DEAD_NO )
	{
		return false;
	}

	if( pev->spawnflags & SF_MONSTER_GAG )
		return false;

	// if someone else is talking, don't speak
	if ( FBitSet(speakFlags, SPEAK_DISREGARD_OTHER_SPEAKING) )
	{
		if (IsTalking())
			return false;
	}
	else if( CTalkMonster::SomeoneIsTalking() )
		return false;

	// if player is not in pvs, don't speak
	if( !IsFullyAlive() || FNullEnt(FIND_CLIENT_IN_PVS( edict() ) ) )
		return false;

	// don't talk if you're in combat
	if( !FBitSet(speakFlags, SPEAK_DISREGARD_ENEMY) && m_hEnemy != 0 && FVisible( m_hEnemy ) )
		return false;

	return true;
}

bool CTalkMonster::CanPlaySentence( bool fDisregardState )
{ 
	if( fDisregardState )
		return CFollowingMonster::CanPlaySentence( fDisregardState );
	return FOkToSpeak(); 
}

PainSoundRule CTalkMonster::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = FloatRange{0.5f, 0.75f};
	return rule;
}

//=========================================================
// FIdleStare
//=========================================================
bool CTalkMonster::FIdleStare()
{
	if( !FOkToSpeak() )
		return false;

	if (FBitSet(pev->spawnflags, SF_TALKMONSTER_DONT_TALK_TO_PLAYER))
		return false;

	PlaySentence( SentenceGroup(TLK_STARE), RANDOM_FLOAT(5, 7.5), VOL_NORM, ATTN_IDLE );

	m_hTalkTarget = FindNearestFriend( true );
	return true;
}

//=========================================================
// IdleHello
// Try to greet player first time he's seen
//=========================================================
bool CTalkMonster::FIdleHello()
{
	if( !FOkToSpeak() )
		return false;

	// if this is first time scientist has seen player, greet him
	if( !FBitSet( m_bitsSaid, bit_saidHelloPlayer ) )
	{
		// get a player
		CBaseEntity *pPlayer = FindNearestFriend( true );

		if( pPlayer )
		{
			if( FInViewCone( pPlayer ) && FVisible( pPlayer ) )
			{
				m_hTalkTarget = pPlayer;

				if( FBitSet(pev->spawnflags, SF_MONSTER_PREDISASTER ) )
					PlaySentence( SentenceGroup(TLK_PHELLO), RANDOM_FLOAT( 3.0f, 3.5f ), VOL_NORM,  ATTN_IDLE );
				else
					PlaySentence( SentenceGroup(TLK_HELLO), RANDOM_FLOAT( 3.0f, 3.5f ), VOL_NORM,  ATTN_IDLE );

				SetBits( m_bitsSaid, bit_saidHelloPlayer );

				return true;
			}
		}
	}
	return false;
}

//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
bool CTalkMonster::GotIdleSpeakChance()
{
	return RANDOM_LONG( 0, m_nSpeak * 2 ) == 0;
}

bool CTalkMonster::FIdleSpeak()
{ 
	// try to start a conversation, or make statement

	if( !FOkToSpeak() )
		return false;

	// player using this entity is alive and wounded?
	CBaseEntity *pTarget = m_hTargetEnt;

	if( pTarget != NULL )
	{
		if( pTarget->IsPlayer() )
		{
			if( pTarget->IsAlive() )
			{
				m_hTalkTarget = m_hTargetEnt;
				if( !FBitSet(m_bitsSaid, bit_saidDamageHeavy ) && 
					( m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health / 8 ) )
				{
					PlaySentence( SentenceGroup(TLK_PLHURT3), RandomSentenceDuraion(), VOL_NORM, ATTN_IDLE );
					SetBits( m_bitsSaid, bit_saidDamageHeavy );
					return true;
				}
				else if( !FBitSet( m_bitsSaid, bit_saidDamageMedium ) && 
					( m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health / 4 ) )
				{
					PlaySentence( SentenceGroup(TLK_PLHURT2), RandomSentenceDuraion(), VOL_NORM, ATTN_IDLE );
					SetBits( m_bitsSaid, bit_saidDamageMedium );
					return true;
				}
				else if( !FBitSet( m_bitsSaid, bit_saidDamageLight) &&
					( m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health / 2 ) )
				{
					PlaySentence( SentenceGroup(TLK_PLHURT1), RandomSentenceDuraion(), VOL_NORM, ATTN_IDLE );
					SetBits( m_bitsSaid, bit_saidDamageLight );
					return true;
				}
			}
			else
			{
				//!!!KELLY - here's a cool spot to have the talkmonster talk about the dead player if we want.
				// "Oh dear, Gordon Freeman is dead!" -Scientist
				// "Damn, I can't do this without you." -Barney
			}
		}
	}

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return
	CBaseEntity *pFriend = FindNearestFriend( false );

	if( pFriend && !( pFriend->IsMoving() ) && ( RANDOM_LONG( 0, 99 ) < 75 ) )
	{
		float duration = RandomSentenceDuraion();
		// force friend to answer
		CBaseMonster* pMonster = pFriend->MyMonsterPointer();
		if (pMonster)
		{
			CTalkMonster *pTalkMonster = pMonster->MyTalkMonsterPointer();
			if (pTalkMonster && pTalkMonster->m_flStopTalkTime <= gpGlobals->time + duration &&
					!FBitSet(pTalkMonster->pev->spawnflags, SF_MONSTER_GAG) && pMonster->IsFreeToManipulate() && AskQuestion(duration))
			{
				if (pTalkMonster->SetAnswerQuestion( this )) // UNDONE: This is EVIL!!!
					pTalkMonster->m_flStopTalkTime = m_flStopTalkTime;

				m_hTalkTarget = pFriend;

				m_nSpeak++;
				return true;
			}
		}
	}

	// otherwise, play an idle statement, try to face client when making a statement.
	if( !FBitSet(pev->spawnflags, SF_TALKMONSTER_DONT_TALK_TO_PLAYER) && RANDOM_LONG( 0, 1 ) )
	{
		pFriend = FindNearestFriend( true );

		if( pFriend )
		{
			m_hTalkTarget = pFriend;
			MakeIdleStatement();
			m_nSpeak++;
			return true;
		}
	}

	// didn't speak
	Talk( 0 );
	CTalkMonster::g_talkWaitTime = 0;
	return false;
}

void CTalkMonster::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity *pListener )
{
	if( !bConcurrent )
		ShutUpFriends();

	ClearConditions( bits_COND_CLIENT_PUSH );	// Forget about moving!  I've got something to say!
	m_flStopLookTime = m_useTime = gpGlobals->time + duration;
	PlaySentence( pszSentence, duration, volume, attenuation, true );

	m_hTalkTarget = pListener;
}

bool CTalkMonster::PlaySentence(const char *pszSentence, float duration, float volume, float attenuation, bool subtitle)
{
	if( !pszSentence )
		return false;

	Talk( duration );

	bool status = false;
	if( pszSentence[0] == '!' )
	{
		if (subtitle)
			status = EMIT_SOUND_DYN_SUB( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, GetVoicePitch(), ceil(duration)+1 );
		else
			status = EMIT_SOUND_DYN( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, GetVoicePitch() );
	}
	else
	{
		if (subtitle)
			status = SENTENCEG_PlayRndSzSub( edict(), pszSentence, volume, attenuation, 0, GetVoicePitch(), ceil(duration)+1 ) >= 0;
		else
			status = SENTENCEG_PlayRndSz( edict(), pszSentence, volume, attenuation, 0, GetVoicePitch() ) >= 0;
	}

	if (status)
		CTalkMonster::g_talkWaitTime = gpGlobals->time + duration + 2.0f;

	// If you say anything, don't greet the player - you may have already spoken to them
	SetBits( m_bitsSaid, bit_saidHelloPlayer );
	return status;
}

//=========================================================
// Talk - set a timer that tells us when the monster is done
// talking.
//=========================================================
void CTalkMonster::Talk( float flDuration )
{
	if( flDuration <= 0 )
	{
		// no duration :( 
		m_flStopTalkTime = gpGlobals->time + 3.0f;
	}
	else
	{
		m_flStopTalkTime = gpGlobals->time + flDuration;
	}
}

static const char* ConstructSentenceWithPrefix(const char* prefix, const char* defaultSentence)
{
	static char buf[32];
	const char* foundUnderscore = strchr(defaultSentence, '_');
	if (foundUnderscore)
	{
		char* str = buf;
		int size = sizeof(buf);
		if (defaultSentence && *defaultSentence == '!')
		{
			strcpy(buf, "!");
			str++;
			size--;
		}
		safe_snprintf( str, size, "%s%s", prefix, foundUnderscore );
		return str;
	}
	else
	{
		ALERT(at_aiconsole, "Can't use Custom Speech Group '%s' because default sentence '%s' has no underscore character!\n", prefix, defaultSentence);
		return NULL;
	}
}

const char* CTalkMonster::SentenceGroup(int group)
{
	if (group == TLK_USE && m_iszUse)
		return GetRedefinedSentence(m_iszUse);
	if (group == TLK_UNUSE && m_iszUnUse)
		return GetRedefinedSentence(m_iszUnUse);
	if (group == TLK_DECLINE && m_iszDecline)
		return GetRedefinedSentence(m_iszDecline);
	const char* defaultSentence = DefaultSentenceGroup(group);
	if (!defaultSentence || *defaultSentence == '\0')
		return nullptr;

	const char* prefix = nullptr;
	if (!FStringNull(m_iszSpeakAs))
	{
		prefix = STRING(m_iszSpeakAs);
	}
	else
	{
		const EntTemplate* entTemplate = GetMyEntTemplate();
		if (entTemplate)
		{
			prefix = entTemplate->SpeechPrefix();
		}
	}

	if (prefix)
		return ConstructSentenceWithPrefix(prefix, defaultSentence);
	return defaultSentence;
}

// Prepare this talking monster to answer question
bool CTalkMonster::SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	if( !m_pCine )
	{
		ChangeSchedule( slIdleResponse );
		m_hTalkTarget = (CBaseMonster *)pSpeaker;
		return true;
	}
	return false;
}

TakeDamageResult CTalkMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	TakeDamageResult ret = CFollowingMonster::TakeDamage( pevInflictor, pevAttacker, damageInfo );
	if( IsAlive() )
	{
		// if player damaged this entity, have other friends talk about it
		if( pevAttacker && m_MonsterState != MONSTERSTATE_PRONE && FBitSet( pevAttacker->flags, FL_CLIENT ) 
				&& IsFriendWithPlayerBeforeProvoked() ) // no point in alerting friends if player is already foe
		{
			CBaseEntity *pFriend = FindNearestFriend( false );

			// only if not dead or dying!
			if( pFriend && pFriend->IsFullyAlive() )
			{
				CBaseMonster* pMonster = pFriend->MyMonsterPointer();
				if (pMonster)
				{
					CTalkMonster* pTalkMonster = pMonster->MyTalkMonsterPointer();
					if (pTalkMonster && !pTalkMonster->IsTalking())
						pTalkMonster->PlaySentence( pTalkMonster->SentenceGroup(TLK_NOSHOOT), RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_NORM );
				}
			}

			if ( ret.TookDamageToHealth() && IsFullyAlive() ) {
				ReactToPlayerHit(pevInflictor, pevAttacker, damageInfo);
			}
		}
	}
	return ret;
}

static bool IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = reference - pevTest->origin;
	vecDir.z = 0.0f;
	vecDir.NormalizeInPlace();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );

	// He's facing me, he meant it
	if( DotProduct( forward, vecDir ) > 0.96f )	// +/- 15 degrees or so
	{
		return true;
	}
	return false;
}

void CTalkMonster::ReactToPlayerHit(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfos)
{
	// don't say anything if already provoked
	if (HasMemory(bits_MEMORY_PROVOKED))
		return;

	CBaseEntity* pAttacker = pevAttacker ? CBaseEntity::Instance(pevAttacker) : NULL;
	const int myTolerance = MyToleranceLevel();
	if ( myTolerance <= TOLERANCE_ZERO )
	{
		m_hTalkTarget = pAttacker;
		PlaySentence( SentenceGroup(TLK_MAD), 4, VOL_NORM, ATTN_NORM );
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( true );
		return;
	}
	// This is a heurstic to determine if the player intended to harm me
	// If I have an enemy, we can't establish intent (may just be crossfire)
	if( m_hEnemy == 0 )
	{
		bool getMad = false;
		// If the player was facing directly at me, or I'm already suspicious
		const bool intendedHit = ( m_afMemory & bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin );
		if (myTolerance <= TOLERANCE_LOW)
		{
			getMad = intendedHit;
		}
		else if (myTolerance <= TOLERANCE_AVERAGE)
		{
			getMad = intendedHit && m_iPlayerHits >= 2;
		}
		else if (myTolerance <= TOLERANCE_HIGH)
		{
			getMad = intendedHit && gpGlobals->time - m_flLastHitByPlayer < 4.0 && m_iPlayerHits >= 3;
		}
		else
		{
			getMad = false;
		}
		if( getMad )
		{
			// Alright, now I'm pissed!
			m_hTalkTarget = pAttacker;
			PlaySentence( SentenceGroup(TLK_MAD), 4, VOL_NORM, ATTN_NORM );

			Remember( bits_MEMORY_PROVOKED );
			StopFollowing( true );
		}
		else
		{
			if ( myTolerance >= TOLERANCE_HIGH && gpGlobals->time - m_flLastHitByPlayer >= 4.0 )
				m_iPlayerHits = 0;
			m_iPlayerHits++;

			m_flLastHitByPlayer = gpGlobals->time;
			// Hey, be careful with that
			if (myTolerance < TOLERANCE_ABSOLUTE || !IsTalking())
			{
				m_hTalkTarget = pAttacker;
				PlaySentence( SentenceGroup(TLK_SHOT), 4, VOL_NORM, ATTN_NORM );
			}
			Remember( bits_MEMORY_SUSPICIOUS );
		}
	}
	else if( !m_hEnemy->IsPlayer() && pev->deadflag == DEAD_NO )
	{
		m_hTalkTarget = pAttacker;
		PlaySentence( SentenceGroup(TLK_SHOT), 4, VOL_NORM, ATTN_NORM );
	}
}

void CTalkMonster::TalkMonsterInit()
{
	PrepareVoicePitch();
	MonsterInit();
	if (IsFriendWithPlayerBeforeProvoked()) {
		m_afCapability |= bits_CAP_USABLE;
		SetUse( &CTalkMonster::FollowerUse );
	}
}

bool CTalkMonster::IsWounded()
{
	return pev->health <= pev->max_health * 0.75f;
}

bool CTalkMonster::IsHeavilyWounded()
{
	return pev->health <= pev->max_health * 0.5f;
}

int CTalkMonster::TakeHealth(CBaseEntity *pHealer, float flHealth, int bitsDamageType)
{
	int ret = CFollowingMonster::TakeHealth(pHealer, flHealth, bitsDamageType);

	if (pHealer && pHealer->IsPlayer() && IsFriendWithPlayerBeforeProvoked())
	{
		// Got healed by a player. Be nice again if was provoked.
		Forget(bits_MEMORY_PROVOKED|bits_MEMORY_SUSPICIOUS);
		if (m_hEnemy != 0 && m_hEnemy->IsPlayer()) {
			m_hEnemy = NULL;
			SetState(MONSTERSTATE_ALERT);
		}
	}

	// Clear bits upon healing so monster could say it again when injured again
	if ( !IsHeavilyWounded() )
		ClearBits( m_bitsSaid, bit_saidWoundHeavy );
	if ( !IsWounded() )
		ClearBits( m_bitsSaid, bit_saidWoundLight );
	return ret;
}

Schedule_t *CTalkMonster::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_IDLE_STAND:
		{	
			// if never seen player, try to greet him
			if( !FBitSet( m_bitsSaid, bit_saidHelloPlayer ) )
			{
				return slIdleHello;
			}

			// sustained light wounds?
			if( !FBitSet( m_bitsSaid, bit_saidWoundLight ) && IsWounded() )
			{
				if (!IsTalking())
				{
					PlaySentence( SentenceGroup(TLK_WOUND), RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE );
					SetBits( m_bitsSaid, bit_saidWoundLight );
					return slIdleTlkStand;
				}
			}
			// sustained heavy wounds?
			else if( !FBitSet( m_bitsSaid, bit_saidWoundHeavy ) && IsHeavilyWounded() )
			{
				if (!IsTalking())
				{
					PlaySentence( SentenceGroup(TLK_MORTAL), RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE );
					SetBits( m_bitsSaid, bit_saidWoundHeavy );
					return slIdleTlkStand;
				}
			}

			// talk about world
			if( FOkToSpeak() && GotIdleSpeakChance() )
			{
				//ALERT ( at_console, "standing idle speak\n" );
				return slIdleSpeak;
			}
			
			if( !IsTalking() && HasConditions( bits_COND_SEE_CLIENT ) && RANDOM_LONG( 0, 6 ) == 0 )
			{
				CBaseEntity *pPlayer = PlayerToFace();

				if( pPlayer )
				{
					// watch the client.
					UTIL_MakeVectors( pPlayer->pev->angles );
					if( ( pPlayer->pev->origin - pev->origin ).IsLength2DLessThan(TLK_STARE_DIST) &&
						UTIL_DotPoints( pPlayer->pev->origin, pev->origin, gpGlobals->v_forward ) >= m_flFieldOfView )
					{
						// go into the special STARE schedule if the player is close, and looking at me too.
						return &slTlkIdleWatchClient[1];
					}

					return slTlkIdleWatchClient;
				}
			}
			else
			{
				if( IsTalking() )
					// look at who we're talking to
					return slTlkIdleEyecontact;
				else
					// regular standing idle
					return slIdleTlkStand;
			}

			// NOTE - caller must first CTalkMonster::GetScheduleOfType, 
			// then check result and decide what to return ie: if sci gets back
			// slIdleStand, return slIdleSciStand
		}
		break;
	case SCHED_TARGET_REACHED:
		{
			return GetScheduleOfType(SCHED_TARGET_FACE);
		}
		break;
	}

	return CFollowingMonster::GetScheduleOfType( Type );
}

//=========================================================
// IsTalking - am I saying a sentence right now?
//=========================================================
bool CTalkMonster::IsTalking()
{
	return m_flStopTalkTime > gpGlobals->time;
}

//=========================================================
// If there's a player around, watch him.
//=========================================================
void CTalkMonster::PrescheduleThink()
{
	if( !HasConditions( bits_COND_SEE_CLIENT ) )
	{
		SetConditions( bits_COND_CLIENT_UNSEEN );
	}
	if (IsFreeToManipulate())
	{
		WantsToCallMedic() && FindAndCallMedic();
	}
	CFollowingMonster::PrescheduleThink();
}

bool CTalkMonster::WantsToCallMedic()
{
	return IsHeavilyWounded() && ( m_flMedicWaitTime < gpGlobals->time );
}

bool CTalkMonster::FindAndCallMedic()
{
	CSquadMonster* foundMedic = NULL;
	// First try looking for a medic in my squad
	if ( InSquad() )
	{
		CSquadMonster *pSquadLeader = MySquadLeader();
		if ( pSquadLeader )
		{
			for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
			{
				CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
				if (FVisible(pMember) && CanCallThisMedic(pMember))
				{
					foundMedic = pMember;
					break;
				}
			}
		}
	}
	// If not, search bsp.
	if ( !foundMedic )
	{
		// for each medic in this bsp...
		for( int i = 0; i < NUM_MEDICS; i++ )
		{
			const char* medicName = m_szMedics[i];
			if (!medicName || !*medicName)
				break;
			CBaseEntity *pFriend = NULL;
			while ((pFriend = EnumFriends( pFriend, medicName, true )) != NULL)
			{
				CSquadMonster* friendMedic = pFriend->MySquadMonsterPointer();
				if (CanCallThisMedic(friendMedic))
				{
					foundMedic = friendMedic;
					break;
				}
			}
		}
	}

	m_flMedicWaitTime = CALL_MEDIC_DELAY + gpGlobals->time;

	if (foundMedic)
	{
		// Don't break sentence if already talking
		if (!IsTalking())
			PlayCallForMedic();

		Forget(bits_MEMORY_SHOULD_ROAM_IN_ALERT);
		ALERT( at_aiconsole, "Injured %s called for %s\n", STRING(pev->classname), STRING(foundMedic->pev->classname) );
		foundMedic->StartFollowingHealTarget(this);

		return true;
	}
	else
	{
		return false;
	}
}

// try to smell something
void CTalkMonster::TrySmellTalk()
{
	if( !FOkToSpeak() )
		return;

	// clear smell bits periodically
	if( gpGlobals->time > m_flLastSaidSmelled )
	{
		//ALERT( at_aiconsole, "Clear smell bits\n" );
		ClearBits( m_bitsSaid, bit_saidSmelled );
	}

	// smelled something?
	if( !FBitSet( m_bitsSaid, bit_saidSmelled ) && HasConditions( bits_COND_SMELL ) )
	{
		PlaySentence( SentenceGroup(TLK_SMELL), RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE );
		m_flLastSaidSmelled = gpGlobals->time + 60.0f;// don't talk about the stinky for a while.
		SetBits( m_bitsSaid, bit_saidSmelled );
	}
}

int CTalkMonster::IRelationship( CBaseEntity *pTarget )
{
	if( pTarget->IsPlayer() )
		if( m_afMemory & bits_MEMORY_PROVOKED )
			return R_HT;
	return CFollowingMonster::IRelationship( pTarget );
}

bool CTalkMonster::IsFriendWithPlayerBeforeProvoked()
{
	const int relation = IDefaultRelationship(CLASS_PLAYER);
	return !m_fStartSuspicious && relation == R_AL;
}

void CTalkMonster::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "UseSentence" ) )
	{
		m_iszUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "UnUseSentence" ) )
	{
		m_iszUnUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq( pkvd->szKeyName, "RefusalSentence" )) // Same name as in Spirit
	{
		m_iszDecline = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "SpeakAs")) //LRC
	{
		m_iszSpeakAs = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "suspicious" ) )
	{
		m_fStartSuspicious = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "tolerance" ) )
	{
		m_iTolerance = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "alert_friends" ) )
	{
		m_alertFriendsPolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "alertable_by_friends" ) )
	{
		m_alertableByFriends = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "voicepitch" ) )
	{
		m_voicePitch = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else 
		CFollowingMonster::KeyValue( pkvd );
}

const char* CTalkMonster::GetRedefinedSentence(string_t sentence)
{
	if (FStrEq(STRING(sentence), "null"))
		return NULL;
	else
		return STRING(sentence);
}

static const char* ToleranceLevelDisplayName(int tolerance)
{
	switch (tolerance) {
	case TOLERANCE_ZERO:
		return "Zero";
	case TOLERANCE_LOW:
		return "Low";
	case TOLERANCE_AVERAGE:
		return "Average";
	case TOLERANCE_HIGH:
		return "High";
	case TOLERANCE_ABSOLUTE:
	case TOLERANCE_ABSOLUTE_NO_ALERTS:
		return "Absolute";
	default:
		return "Unknown";
	}
}

static const char* AlertFriendsPolicyDisplayName(short alertFriendsPolicy)
{
	switch (alertFriendsPolicy) {
	case ALERT_FRIENDS_ON_DEATH_ALWAYS:
		return "Always";
	case ALERT_FRIENDS_ON_DEATH_IF_NOT_IN_COMBAT:
		return "If not in combat";
	case ALERT_FRIENDS_ON_DEATH_NEVER:
		return "Never";
	default:
		return "Unknown";
	}
}

void CTalkMonster::ReportAIState(ALERT_TYPE level)
{
	CFollowingMonster::ReportAIState(level);
	if ( m_hTalkTarget != 0 )
		ALERT( level, "Speaking to: %s. ", STRING( m_hTalkTarget->pev->classname ) );
	if (m_fStartSuspicious)
		ALERT( level, "Start pre-provoked. " );
	ALERT( level, "Voice pitch: %d. ", m_voicePitch );
	ALERT( level, "Tolerance level: %s. ", ToleranceLevelDisplayName(MyToleranceLevel()) );
	ALERT( level, "I will alert friends if player kills me: %s. ", AlertFriendsPolicyDisplayName(AlertFriendsPolicy()) );
	ALERT( level, "Friends can alert me if player kills them: %s. ", AlertableByFriendDeath() ? "yes" : "no" );
}

bool CTalkMonster::SomeoneIsTalking()
{
	return gpGlobals->time <= CTalkMonster::g_talkWaitTime;
}

bool CTalkMonster::EmitSoundScriptTalk(const char* name)
{
	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchRelative(GetVoicePitch());
	return EmitSoundScript(name, paramOverride);
}

void CTalkMonster::RegisterTalkMonster(const char *className, bool canFollow, short followerCategory)
{
	int i;
	for (i=0; i<TLK_CFRIENDS; ++i)
	{
		if (!m_szFriends[i].name[0])
		{
			strncpyEnsureTermination(m_szFriends[i].name, className);
			m_szFriends[i].canFollow = canFollow;
			m_szFriends[i].category = followerCategory;

			ALERT(at_aiconsole, "Registered %s as talk friend\n", className);
			return;
		}
		else if (FStrEq(m_szFriends[i].name, className))
		{
			return;
		}
	}
	if (i >= TLK_CFRIENDS)
	{
		ALERT(at_console, "Too many talk friends classes! Can't register %s\n", className);
	}
}

void CTalkMonster::RegisterTalkMonster(bool canFollow)
{
	CTalkMonster::RegisterTalkMonster(STRING(pev->classname), canFollow, TalkFriendCategory());
}

void CTalkMonster::RegisterMedic(const char* className)
{
	int i;
	for (i=0; i<NUM_MEDICS; ++i)
	{
		if (!m_szMedics[i][0])
		{
			strncpyEnsureTermination(m_szMedics[i], className);

			ALERT(at_aiconsole, "Registered %s as medic\n", className);
			return;
		}
		else if (FStrEq(m_szMedics[i], className))
		{
			return;
		}
	}
	if (i >= NUM_MEDICS)
	{
		ALERT(at_console, "Too many medic classes! Can't register %s\n", className);
	}
}

void CTalkMonster::RegisterMedic()
{
	CTalkMonster::RegisterMedic(STRING(pev->classname));
}

short CTalkMonster::AlertFriendsDefaultPolicy()
{
	const int tolerance = MyToleranceLevel();
	if (tolerance == TOLERANCE_ABSOLUTE_NO_ALERTS)
		return ALERT_FRIENDS_ON_DEATH_NEVER;
	if (TalkFriendCategory() == TALK_FRIEND_SOLDIER)
		return ALERT_FRIENDS_ON_DEATH_IF_NOT_IN_COMBAT;
	else
		return ALERT_FRIENDS_ON_DEATH_ALWAYS;
}

bool CTalkMonster::ShouldAlertFriendsOnDeath()
{
	switch (AlertFriendsPolicy()) {
	case ALERT_FRIENDS_ON_DEATH_ALWAYS:
		return true;
	case ALERT_FRIENDS_ON_DEATH_IF_NOT_IN_COMBAT:
		return m_hEnemy == 0;
	case ALERT_FRIENDS_ON_DEATH_NEVER:
		return false;
	default:
		return false;
	}
}

bool CTalkMonster::AlertableByFriendDeathDefault()
{
	return MyToleranceLevel() < TOLERANCE_ABSOLUTE;
}

bool CTalkMonster::AlertableByFriendDeath()
{
	switch (m_alertableByFriends) {
	case ALERTED_BY_FRIEND_DEATH_YES:
		return true;
	case ALERTED_BY_FRIEND_DEATH_NO:
		return false;
	default:
		return AlertableByFriendDeathDefault();
	}
}
