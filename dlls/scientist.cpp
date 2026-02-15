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
// human scientist (passive lab worker)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"game.h"

#define FEATURE_SCIENTIST_PLFEAR 0

#define SF_SCI_SITTING_DONT_DROP SF_MONSTER_SPECIAL_FLAG // Don't drop to the floor. We can re-use the same value as sitting scientists can't follow

enum
{
	HEAD_GLASSES = 0,
	HEAD_EINSTEIN = 1,
	HEAD_LUTHER = 2,
	HEAD_SLICK = 3,
	HEAD_EINSTEIN_WITH_BOOK = 4,
	HEAD_SLICK_WITH_STICK = 5
};

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
	SCHED_HEAL
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
	TASK_DRAW_NEEDLE,
	TASK_PUTAWAY_NEEDLE
};

enum
{
	TLK_HEAL = TLK_CGROUPS,
	TLK_SCREAM,
	TLK_FEAR,
	TLK_PLFEAR,
	SC_TLK_CGROUPS,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL	( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================
class CScientist : public CTalkMonster
{
public:
	int GetDefaultVoicePitch() override;
	void Spawn() override;
	void Precache() override;
	void CalcTotalHeadCount();

	void SetYawSpeed() override;
	int DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Scientist"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void RunTask( Task_t *pTask ) override;
	void StartTask( Task_t *pTask ) override;
	int DefaultToleranceLevel() override { return TOLERANCE_ZERO; }
	void SetActivity( Activity newActivity ) override;
	Activity GetStoppedActivity() override;
	int DefaultISoundMask() override;
	void DeclineFollowing( CBaseEntity* pCaller ) override;

	float CoverRadius() override {
		if (!IsFollowingPlayer())
			return 1200; // Need more room for cover because scientists want to get far away!
		return CTalkMonster::CoverRadius(); // Don't run too far when following the player
	}
	bool DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || ( gpGlobals->time - m_fearTime ) > 15; }
	bool CanTolerateWhileFollowing( CBaseEntity* pEnemy );

	virtual bool AbleToHeal() { return true; }
	bool CanHeal();
	void StartFollowingHealTarget(CBaseEntity* pTarget) override;
	bool ReadyToHeal() override;
	void Heal();
	void Scream();

	// Override these to set behavior
	Schedule_t *GetScheduleOfType( int Type ) override;
	Schedule_t *GetSchedule() override;
	MONSTERSTATE GetIdealState() override;

	FOLLOW_FAIL_POLICY DefaultFollowFailPolicy() override {
		return FOLLOW_FAIL_STOP;
	}

	void DeathSound() override;
	void PainSound() override;

	const char* DefaultSentenceGroup(int group) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

	void ReportAIState(ALERT_TYPE level) override;
	virtual int RandomHeadCount() {
		return g_modFeatures.scientist_random_heads;
	}
	virtual int TotalHeadCount() {
		return m_totalHeadCount > 0 ? m_totalHeadCount : 4;
	}
	bool NeedleIsEquiped() {
		return pev->body >= TotalHeadCount();
	}

	static const NamedSoundScript painSoundScript;
	static constexpr const char* dieSoundScript = "Scientist.Die";
	static const NamedSoundScript healSoundScript;

protected:
	void SciSpawnHelper(const char* modelName, float health);

	float m_healTime;
	float m_fearTime;

	// Don't save
	int m_totalHeadCount;
};

LINK_ENTITY_TO_CLASS( monster_scientist, CScientist )

TYPEDESCRIPTION	CScientist::m_SaveData[] =
{
	DEFINE_FIELD( CScientist, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_fearTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CScientist, CTalkMonster )

const NamedSoundScript CScientist::painSoundScript = {
	CHAN_VOICE,
	{"scientist/sci_pain1.wav", "scientist/sci_pain2.wav", "scientist/sci_pain3.wav", "scientist/sci_pain4.wav", "scientist/sci_pain5.wav"},
	"Scientist.Pain"
};

const NamedSoundScript CScientist::healSoundScript = {
	CHAN_WEAPON,
	{"items/medshot4.wav"},
	0.75f,
	ATTN_STATIC,
	"Scientist.Heal"
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Task_t tlFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED, 128.0f },	// Move within 128 of target ent (client)
	//{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t slFollowScared[] =
{
	{
		tlFollowScared,
		ARRAYSIZE( tlFollowScared ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t tlFaceTargetScared[] =
{
	{ TASK_FACE_TARGET, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t slFaceTargetScared[] =
{
	{
		tlFaceTargetScared,
		ARRAYSIZE( tlFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t tlHeal[] =
{
	{ TASK_CATCH_WITH_TARGET_RANGE, 50.0f },	// Move within 50 of target ent (client)
	{ TASK_FACE_IDEAL, 0.0f },
	{ TASK_SAY_HEAL, 0.0f },
	{ TASK_DRAW_NEEDLE, 0.0f },			// Whip out the needle
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_HEAL },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_HEAL, 0.0f },	// Put it in the target
	{ TASK_CLEAR_FAIL_SCHEDULE, 0.0f },
	{ TASK_PUTAWAY_NEEDLE, 0.0f },			// Put away the needle
};

Schedule_t slHeal[] =
{
	{
		tlHeal,
		ARRAYSIZE( tlHeal ),
		bits_COND_LIGHT_DAMAGE|
		bits_COND_HEAVY_DAMAGE|
		bits_COND_HEAR_SOUND|
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"Heal"
	},
};

Task_t tlSciFaceTarget[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_TARGET, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t slSciFaceTarget[] =
{
	{
		tlSciFaceTarget,
		ARRAYSIZE( tlSciFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Sci FaceTarget"
	},
};

Task_t tlSciPanic[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_SCREAM, 0.0f },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_EXCITED },	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slSciPanic[] =
{
	{
		tlSciPanic,
		ARRAYSIZE( tlSciPanic ),
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"SciPanic"
	},
};

Task_t tlIdleSciStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2.0f }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t slIdleSciStand[] =
{
	{
		tlIdleSciStand,
		ARRAYSIZE( tlIdleSciStand ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_CLIENT_PUSH |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |// sound flags
		//bits_SOUND_PLAYER |
		//bits_SOUND_WORLD |
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleSciStand"

	},
};

Task_t tlScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FIND_SPOT_AWAY_FROM_ENEMY, 0.0f },
	{ TASK_RUN_PATH_SCARED, 0.0f },
	{ TASK_TURN_LEFT, 179.0f },
	{ TASK_SET_SCHEDULE, (float)SCHED_HIDE },
};

Schedule_t slScientistCover[] =
{
	{
		tlScientistCover,
		ARRAYSIZE( tlScientistCover ),
		bits_COND_NEW_ENEMY|
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"ScientistCover"
	},
};

Task_t tlScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },	// FIXME: This looks lame
	{ TASK_WAIT, 1.0f },
	{ TASK_WAIT_RANDOM, 2.0f },
};

Schedule_t slScientistHide[] =
{
	{
		tlScientistHide,
		ARRAYSIZE( tlScientistHide ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"ScientistHide"
	},
};

Task_t tlScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM, 0.3f },				// Scream 30% of the time
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_RANDOM_SCREAM, 0.1f },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 1.0f },
};

Schedule_t slScientistStartle[] =
{
	{
		tlScientistStartle,
		ARRAYSIZE( tlScientistStartle ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE|
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"ScientistStartle"
	},
};

Task_t tlFear[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_SAY_FEAR, 0.0f },
	//{ TASK_PLAY_SEQUENCE, (float)ACT_FEAR_DISPLAY },
};

Schedule_t slFear[] =
{
	{
		tlFear,
		ARRAYSIZE( tlFear ),
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};

Task_t	tlDisarmNeedle[] =
{
	{ TASK_PUTAWAY_NEEDLE,		(float)0	},			// Put away the needle
};

Schedule_t	slDisarmNeedle[] =
{
	{
		tlDisarmNeedle,
		ARRAYSIZE ( tlDisarmNeedle ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"DisarmNeedle"
	},
};

DEFINE_CUSTOM_SCHEDULES( CScientist )
{
	slSciFaceTarget,
	slFear,
	slScientistCover,
	slScientistHide,
	slScientistStartle,
	slHeal,
	slSciPanic,
	slFollowScared,
	slFaceTargetScared,
	slDisarmNeedle,
};

IMPLEMENT_CUSTOM_SCHEDULES( CScientist, CTalkMonster )

void CScientist::DeclineFollowing(CBaseEntity *pCaller )
{
	Talk( 10 );
	CTalkMonster::DeclineFollowing(pCaller);
}

void CScientist::Scream()
{
	if( FOkToSpeak(SPEAK_DISREGARD_ENEMY|SPEAK_DISREGARD_OTHER_SPEAKING) )
	{
		m_hTalkTarget = m_hEnemy;
		PlaySentence( SentenceGroup(TLK_SCREAM), RANDOM_FLOAT( 3.0f, 6.0f ), VOL_NORM, ATTN_NORM );
	}
}

Activity CScientist::GetStoppedActivity()
{ 
	if( m_hEnemy != 0 ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}

void CScientist::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:
		if (m_hTargetEnt == 0)
			TaskFail("no target ent");
		else if (m_hTargetEnt->pev->deadflag != DEAD_NO)
		{
			// The guy we wanted to heal is dying or just died. Probably a good place for some scared sentence?
			TaskFail("target ent is dead");
		}
		else
		{
			if (!InScriptedSentence())
			{
				m_hTalkTarget = m_hTargetEnt;
				PlaySentence( SentenceGroup(TLK_HEAL), 2, VOL_NORM, ATTN_IDLE );
			}
			TaskComplete();
		}
		break;
	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;
	case TASK_RANDOM_SCREAM:
		if( RANDOM_FLOAT( 0.0f, 1.0f ) < pTask->flData )
			Scream();
		TaskComplete();
		break;
	case TASK_SAY_FEAR:
		if( FOkToSpeak(SPEAK_DISREGARD_ENEMY) )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;

			//The enemy can be null here. - Solokiller
			//Discovered while testing the barnacle grapple on headcrabs with scientists in view.
			if( m_hEnemy != 0 && m_hEnemy->IsPlayer() )
				PlaySentence( SentenceGroup(TLK_PLFEAR), 5, VOL_NORM, ATTN_NORM );
			else
				PlaySentence( SentenceGroup(TLK_FEAR), 5, VOL_NORM, ATTN_NORM );
		}
		TaskComplete();
		break;
	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if( m_hTargetEnt == 0 )
				TaskFail("no target ent");
			else
			{
				if( ( m_hTargetEnt->pev->origin - pev->origin ).IsLengthLessThan(1.0f) )
				{
					TaskComplete();
				}
				else
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					if( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
						TaskFail("can't build path to target");
				}
			}
		}
		break;
	case TASK_DRAW_NEEDLE:
		if (NeedleIsEquiped())
		{
			TaskComplete();
		}
		else
		{
			m_IdealActivity = ACT_ARM;
		}
		break;
	case TASK_PUTAWAY_NEEDLE:
		if (NeedleIsEquiped())
		{
			m_IdealActivity = ACT_DISARM;
		}
		else
		{
			TaskComplete();
		}
		break;
	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CScientist::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if( MovementIsComplete() )
			TaskComplete();
		if( RANDOM_LONG( 0, 31 ) < 8 )
			Scream();
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if( RANDOM_LONG( 0, 63 ) < 8 )
				Scream();

			if( m_hTargetEnt == 0 )
			{
				TaskFail("no target ent");
			}
			else if( m_hEnemy == 0 )
			{
				TaskFail("no enemy");
			}
			else
			{
				float distance;

				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if( ( distance < pTask->flData ) || ( m_vecMoveGoal - m_hTargetEnt->pev->origin ).IsLengthGreaterThan(pTask->flData * 0.5f) )
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					FRefreshRoute();
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if( distance < pTask->flData )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else if( distance < 190 && m_movementActivity != ACT_WALK_SCARED )
					m_movementActivity = ACT_WALK_SCARED;
				else if( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
					m_movementActivity = ACT_RUN_SCARED;
			}
		}
		break;
	case TASK_HEAL:
		if( m_fSequenceFinished )
		{
			TaskComplete();
		}
		else
		{
			if( TargetDistance() > 90 )
			{
				TaskFail("target ent is too far");
			}
			if (m_hTargetEnt != 0)
			{
				pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
				ChangeYaw( pev->yaw_speed );
			}
		}
		break;
	case TASK_DRAW_NEEDLE:
	case TASK_PUTAWAY_NEEDLE:
		{
			CBaseEntity *pTarget = m_hTargetEnt;
			if( pTarget )
			{
				pev->ideal_yaw = UTIL_VecToYaw( pTarget->pev->origin - pev->origin );
				ChangeYaw( pev->yaw_speed );
			}
			if( m_fSequenceFinished )
				TaskComplete();
		}
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CScientist::DefaultClassify()
{
	return CLASS_HUMAN_PASSIVE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CScientist::SetYawSpeed()
{
	int ys;

	ys = 90;

	switch( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	default:
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CScientist::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{		
	case SCIENTIST_AE_HEAL:		// Heal my target (if within range)
		Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
		{
			const int totalHeadCount = TotalHeadCount();
			int oldBody = pev->body;
			pev->body = ( oldBody % totalHeadCount ) + totalHeadCount * 1;
		}
		break;
	case SCIENTIST_AE_NEEDLEOFF:
		{
			const int totalHeadCount = TotalHeadCount();
			int oldBody = pev->body;
			pev->body = ( oldBody % totalHeadCount ) + totalHeadCount * 0;
		}
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CScientist::SciSpawnHelper(const char* modelName, float health)
{
	// We need to set it before precache so the right voice will be chosen
	if( pev->body == -1 )
	{
		// -1 chooses a random head
		pev->body = RANDOM_LONG( 0, RandomHeadCount() - 1 );// pick a head, any head
	}

	Precache();

	SetMyModel( modelName );
	SetMySize();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( health );
	pev->view_ofs = Vector( 0, 0, 50 );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE); // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//m_flDistTooFar = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;
	SetMySquadCapabilities();
	SetMyCanOpenDoors(true);
}

int CScientist::GetDefaultVoicePitch()
{
	// get voice for head
	switch( pev->body % TotalHeadCount() )
	{
	default:
	case HEAD_GLASSES:
		return 105;
	case HEAD_EINSTEIN:
	case HEAD_EINSTEIN_WITH_BOOK:
		return 100;
	case HEAD_LUTHER:
		return 95;
	case HEAD_SLICK:
	case HEAD_SLICK_WITH_STICK:
		return 100;
	}
}

void CScientist::Spawn()
{
	SciSpawnHelper("models/scientist.mdl", GetSkillValue("scientist_health"));
	CalcTotalHeadCount();

	// White hands
	pev->skin = 0;

	// Luther is black, make his hands black
	if( pev->body == HEAD_LUTHER )
		pev->skin = 1;

	TalkMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CScientist::Precache()
{
	PrecacheMyModel( "models/scientist.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, painSoundScript);
	RegisterAndPrecacheSoundScript(healSoundScript);

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
	RegisterTalkMonster();
	RegisterMedic();

	CalcTotalHeadCount();
}

void CScientist::CalcTotalHeadCount()
{
	if (pev->modelindex)
	{
		// Divide by 2 to account for body variants with the needle
		m_totalHeadCount = GetBodyCount( GET_MODEL_PTR(ENT(pev)) ) / 2;
	}
}

const char* CScientist::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "SC_ANSWER";
	case TLK_QUESTION: return "SC_QUESTION";
	case TLK_IDLE: return "SC_IDLE";
	case TLK_STARE: return "SC_STARE";
	case TLK_USE: return "SC_OK";
	case TLK_UNUSE: return "SC_WAIT";
	case TLK_DECLINE: return "SC_POK";
	case TLK_STOP: return "SC_STOP";
	case TLK_NOSHOOT: return "SC_SCARED";
	case TLK_HELLO: return "SC_HELLO";
	case TLK_PLHURT1: return "!SC_CUREA";
	case TLK_PLHURT2: return "!SC_CUREB";
	case TLK_PLHURT3: return "!SC_CUREC";
	case TLK_PHELLO: return "SC_PHELLO";
	case TLK_PIDLE: return "SC_PIDLE";
	case TLK_PQUESTION: return "SC_PQUEST";
	case TLK_SMELL: return "SC_SMELL";
	case TLK_WOUND: return "SC_WOUND";
	case TLK_MORTAL: return "SC_MORTAL";
	case TLK_SHOT: return NULL;
#if FEATURE_SCIENTIST_PLFEAR
	case TLK_MAD: return "SC_PLFEAR";
#else
	case TLK_MAD: return NULL;
#endif
	case TLK_HEAL: return "SC_HEAL";
	case TLK_SCREAM: return "SC_SCREAM";
	case TLK_FEAR: return "SC_FEAR";
	case TLK_PLFEAR: return "SC_PLFEAR";
	default: return NULL;
	}
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CScientist::DefaultISoundMask()
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

//=========================================================
// PainSound
//=========================================================
void CScientist::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

//=========================================================
// DeathSound 
//=========================================================
void CScientist::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

void CScientist::SetActivity( Activity newActivity )
{
	int iSequence;

	iSequence = LookupActivity( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}

Schedule_t *CScientist::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		return slSciFaceTarget;
	case SCHED_PANIC:
		return slSciPanic;
	case SCHED_TARGET_CHASE:
		return CTalkMonster::GetScheduleOfType(SCHED_FOLLOW_CAUTIOUS);
	case SCHED_TARGET_CHASE_SCARED:
		return slFollowScared;
	case SCHED_TARGET_FACE_SCARED:
		return slFaceTargetScared;
	case SCHED_HIDE:
		return slScientistHide;
	case SCHED_STARTLE:
		return slScientistStartle;
	case SCHED_FEAR:
		return slFear;
	case SCHED_HEAL:
		if (CanHeal())
			return slHeal;
		else
			return slDisarmNeedle;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CScientist::GetSchedule()
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	CSound *pSound = NULL;
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
		{
			Scream();
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}

	const bool wantsToDisarm = NeedleIsEquiped();

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
		if( pEnemy )
		{
			if( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = 0;
				pEnemy = 0;
			}
		}

		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && CanIdleFlinch() )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// Cower when you hear something scary
		if( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			if( pSound )
			{
				if( pSound->m_iType & ( bits_SOUND_DANGER | bits_SOUND_COMBAT ) )
				{
					if( gpGlobals->time - m_fearTime > 3 )	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType( SCHED_STARTLE );	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if( m_hTargetEnt != 0 && FollowedPlayer() == m_hTargetEnt )
		{
			if( !FollowedPlayer()->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( false, false );
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if( pEnemy != NULL )
				relationship = IRelationship( pEnemy );

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if( relationship == R_NO )
			{
				// If I'm already close enough to my target
				if( TargetDistance() <= 128 )
				{
					if( CanHeal() )	// Heal opportunistically
						return GetScheduleOfType( SCHED_HEAL );
					if( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
						return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				if (wantsToDisarm)
					return slDisarmNeedle;
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
			else	// UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
			{
				if( HasConditions( bits_COND_NEW_ENEMY ) ) // I just saw something new and scary, react
					return GetScheduleOfType( SCHED_FEAR );					// React to something scary
				return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );	// face and follow, but I'm scared!
			}
		}
		// was called by other ally
		else if ( CanHeal() )
		{
			return GetScheduleOfType( SCHED_HEAL );
		}

		if( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		if (wantsToDisarm)
			return slDisarmNeedle;

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		if( HasConditions( bits_COND_NEW_ENEMY ) )
			return GetScheduleOfType( SCHED_FEAR );					// Point and scream!
		if( HasConditions( bits_COND_SEE_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			return slScientistCover;		// Take Cover

		if( HasConditions( bits_COND_HEAR_SOUND ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );	// Cower and panic from the scary sound!

		if (!IsFollowingPlayer())
			return slScientistCover;			// Run & Cower
		break;
	default:
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

bool CScientist::CanTolerateWhileFollowing(CBaseEntity *pEnemy)
{
	if (!pEnemy)
		return true;
	// TODO: better check. Should tolerate enemies that are not very dangerous and easily avoidable (like headcrabs)
	if (IRelationship( pEnemy ) == R_DL)
		return true;
	return false;
}

MONSTERSTATE CScientist::GetIdealState()
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
		if( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if( IsFollowingPlayer() )
			{
				if( CanTolerateWhileFollowing(m_hEnemy) && !HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					// Don't go to combat if you're following the player
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
			}
		}
		else if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// Stop following if you take damage
			if( IsFollowingPlayer() )
				StopFollowing( true, false );
		}
		break;
	case MONSTERSTATE_COMBAT:
		{
			CBaseEntity *pEnemy = m_hEnemy;
			if( pEnemy != NULL )
			{
				if( HasConditions( bits_COND_SEE_ENEMY ) )
				{
					m_fearTime = gpGlobals->time;
					m_IdealMonsterState = MONSTERSTATE_COMBAT;
					return m_IdealMonsterState;
				}
				else if( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
				{
					// Strip enemy when going to alert
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					m_hEnemy = 0;
					return m_IdealMonsterState;
				}
			}
		}
		break;
	default:
		break;
	}

	return CTalkMonster::GetIdealState();
}

bool CScientist::CanHeal()
{
	if (!AbleToHeal())
		return false;
	if( ( m_healTime > gpGlobals->time ) || ( m_hTargetEnt == 0 ) || ( !m_hTargetEnt->IsFullyAlive() ) ||
			( m_hTargetEnt->IsPlayer() ? m_hTargetEnt->pev->health > ( m_hTargetEnt->pev->max_health * 0.5f ) :
			  m_hTargetEnt->pev->health >= m_hTargetEnt->pev->max_health ) )
		return false;

	return true;
}

void CScientist::StartFollowingHealTarget(CBaseEntity *pTarget)
{
	StopScript();

	m_hTargetEnt = pTarget;
	ClearConditions( bits_COND_CLIENT_PUSH );
	ClearSchedule();
	ALERT(at_aiconsole, "Scientist started to follow injured %s\n", STRING(pTarget->pev->classname));
}

bool CScientist::ReadyToHeal()
{
	return AbleToHeal() && AbleToFollow() && ( m_healTime <= gpGlobals->time ) && m_pSchedule != slHeal;
}

void CScientist::Heal()
{
	if( !CanHeal() )
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if( target.IsLengthGreaterThan(100.0f) )
		return;

	m_hTargetEnt->TakeHealth(this, GetSkillValue("scientist_heal"), DMG_GENERIC );
	EmitSoundScript(healSoundScript);

	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + GetSkillValue("scientist_heal_time");
}

void CScientist::ReportAIState(ALERT_TYPE level)
{
	CTalkMonster::ReportAIState(level);
	if (AbleToHeal())
	{
		ALERT(level, "Is able to heal. ");
		if (m_healTime <= gpGlobals->time)
		{
			ALERT(level, "Can heal now. ");
		}
		else
		{
			ALERT(level, "Can heal in %3.1f seconds. ", m_healTime - gpGlobals->time);
		}
	}
	else
	{
		ALERT(level, "Is not able to heal. ");
	}
}

//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadScientist : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/scientist.mdl"; }
	int	DefaultClassify() override { return	CLASS_HUMAN_PASSIVE; }

	const char* getPos(int pos) const override;
	static const char *m_szPoses[7];
};
const char *CDeadScientist::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

const char* CDeadScientist::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_scientist_dead, CDeadScientist )

//
// ********** DeadScientist SPAWN **********
//
void CDeadScientist::Spawn()
{
	SpawnHelper();

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, g_modFeatures.scientist_random_heads-1);// pick a head, any head
	}
	// Luther is black, make his hands black
	if ( pev->body == HEAD_LUTHER )
		pev->skin = 1;
	else
		pev->skin = 0;

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}

//=========================================================
// Sitting Scientist PROP
//=========================================================
class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn() override;
	void Precache() override;

	void EXPORT SittingThink();
	int DefaultClassify() override;
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool SetAnswerQuestion( CTalkMonster *pSpeaker ) override;

	int DefaultSizeForGrapple() override { return GRAPPLE_FIXED; }
	Vector DefaultMinHullSize() override { return Vector(-14.0f, -14.0f, 0.0f); }
	Vector DefaultMaxHullSize() override { return Vector(14.0f, 14.0f, 36.0f); }

	bool FIdleSpeak();
	int m_baseSequence;	
	int m_headTurn;
	float m_flResponseDelay;

protected:
	void SciSpawnHelper(const char* modelName);
};

LINK_ENTITY_TO_CLASS( monster_sitting_scientist, CSittingScientist )

TYPEDESCRIPTION	CSittingScientist::m_SaveData[] =
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingScientist, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingScientist, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingScientist, CScientist )

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;

//
// ********** Scientist SPAWN **********
//
void CSittingScientist::SciSpawnHelper(const char* modelName)
{
	PrecacheMyModel( modelName );
	SetMyModel( modelName );
	Precache();
	InitBoneControllers();

	SetMySize();

	pev->solid = SOLID_SLIDEBOX;
	if (FBitSet(pev->spawnflags, SF_SCI_SITTING_DONT_DROP))
		pev->movetype = MOVETYPE_FLY;
	else
		pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	SetMyHealth( 50 );
	
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyFieldOfView(VIEW_FIELD_WIDE); // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability= bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	SetBits( pev->spawnflags, SF_MONSTER_PREDISASTER ); // predisaster only!

	if( pev->body == -1 )
	{
		// -1 chooses a random head
		pev->body = RANDOM_LONG( 0, g_modFeatures.scientist_random_heads - 1 );// pick a head, any head
	}

	// Luther is black, make his hands black
	if( pev->body == HEAD_LUTHER )
		pev->skin = 1;

	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG( 0, 4 );
	ResetSequenceInfo();

	SetThink( &CSittingScientist::SittingThink );
	pev->nextthink = gpGlobals->time + 0.1f;

	if (!FBitSet(pev->spawnflags, SF_SCI_SITTING_DONT_DROP))
		DROP_TO_FLOOR( ENT( pev ) );
}

void CSittingScientist::Spawn()
{
	SciSpawnHelper("models/scientist.mdl");
	CalcTotalHeadCount();
	// Luther is black, make his hands black
	if ( pev->body == HEAD_LUTHER )
		pev->skin = 1;
}

void CSittingScientist::Precache()
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
	RegisterTalkMonster(false);
	CalcTotalHeadCount();
}

//=========================================================
// ID as a passive human
//=========================================================
int CSittingScientist::DefaultClassify()
{
	return CLASS_HUMAN_PASSIVE;
}

//=========================================================
// sit, do stuff
//=========================================================
void CSittingScientist::SittingThink()
{
	CBaseEntity *pent;

	StudioFrameAdvance();

	// try to greet player
	if( FIdleHello() )
	{
		pent = FindNearestFriend( true );
		if( pent )
		{
			float yaw = VecToYaw( pent->pev->origin - pev->origin ) - pev->angles.y;

			if( yaw > 180 )
				yaw -= 360;
			if( yaw < -180 )
				yaw += 360;

			if( yaw > 0 )
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
			else
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

		ResetSequenceInfo();
		pev->frame = 0;
		SetBoneController( 0, 0 );
		}
	}
	else if( m_fSequenceFinished )
	{
		int i = RANDOM_LONG( 0, 99 );
		m_headTurn = 0;
		
		if( m_flResponseDelay && gpGlobals->time > m_flResponseDelay )
		{
			// respond to question
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if( i < 30 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	

			// turn towards player or nearest friend and speak

			if( !FBitSet( m_bitsSaid, bit_saidHelloPlayer ) )
				pent = FindNearestFriend( true );
			else
				pent = FindNearestFriend( false );

			if( !FIdleSpeak() || !pent )
			{
				m_headTurn = RANDOM_LONG( 0, 8 ) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
			else
			{
				// only turn head if we spoke
				float yaw = VecToYaw( pent->pev->origin - pev->origin ) - pev->angles.y;

				if( yaw > 180 )
					yaw -= 360;
				if( yaw < -180 )
					yaw += 360;

				if( yaw > 0 )
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
				else
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

				//ALERT( at_console, "sitting speak\n" );
			}
		}
		else if( i < 60 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	
			m_headTurn = RANDOM_LONG( 0, 8 ) * 10 - 40;
			if( RANDOM_LONG( 0, 99 ) < 5 )
			{
				//ALERT( at_console, "sitting speak2\n" );
				FIdleSpeak();
			}
		}
		else if( i < 80 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		}
		else if( i < 100 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		}

		ResetSequenceInfo();
		pev->frame = 0;
		SetBoneController( 0, m_headTurn );
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}

// prepare sitting scientist to answer a question
bool CSittingScientist::SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT( 3.0f, 4.0f );
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
	return true;
}

//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
bool CSittingScientist::FIdleSpeak()
{ 
	// try to start a conversation, or make statement
	if( !FOkToSpeak() )
		return false;

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting scientists nearby
	CBaseEntity *pFriend = FindNearestFriend( false );

	if( pFriend && RANDOM_LONG( 0, 1 ) )
	{
		PlaySentence( SentenceGroup(TLK_PQUESTION), RANDOM_FLOAT( 4.8, 5.2 ), VOL_NORM, ATTN_IDLE);

		CBaseMonster* pMonster = pFriend->MyMonsterPointer();
		if (pMonster)
		{
			CTalkMonster *pTalkMonster = pMonster->MyTalkMonsterPointer();
			if (pTalkMonster && pTalkMonster->SetAnswerQuestion( this ))
				pTalkMonster->m_flStopTalkTime = m_flStopTalkTime;
		}
		IdleHeadTurn( pFriend->pev->origin );
		return true;
	}

	// otherwise, play an idle statement
	if( RANDOM_LONG( 0, 1 ) )
	{
		PlaySentence( SentenceGroup(TLK_PIDLE), RANDOM_FLOAT( 4.8, 5.2 ), VOL_NORM, ATTN_IDLE);
		return true;
	}

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return false;
}

class CCleansuitScientist : public CScientist
{
public:
	int GetDefaultVoicePitch() override
	{
		switch( pev->body )
		{
		default:
		case HEAD_GLASSES:
			return 105;
		case HEAD_EINSTEIN:
			return 100;
		case HEAD_LUTHER:
			return 95;
		case HEAD_SLICK:
			return 100;
		}
	}
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("cleansuit_scientist"); }
	const char* DefaultDisplayName() override { return "Cleansuit Scientist"; }
	bool AbleToHeal() override { return false; }
	void ReportAIState(ALERT_TYPE level) override;

	static constexpr const char* painSoundScript = "CleansuitScientist.Pain";
	static constexpr const char* dieSoundScript = "CleansuitScientist.Die";

	void PainSound() override {
		EmitSoundScriptTalk(painSoundScript);
	}
	void DeathSound() override {
		EmitSoundScriptTalk(dieSoundScript);
	}
};

LINK_ENTITY_TO_CLASS( monster_cleansuit_scientist, CCleansuitScientist )

void CCleansuitScientist::Spawn()
{
	SciSpawnHelper("models/cleansuit_scientist.mdl", GetSkillValue("cleansuit_scientist_health"));
	TalkMonsterInit();
}

void CCleansuitScientist::Precache()
{
	PrecacheMyModel("models/cleansuit_scientist.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript, CScientist::painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, CScientist::dieSoundScript, CScientist::painSoundScript);

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

void CCleansuitScientist::ReportAIState(ALERT_TYPE level)
{
	CTalkMonster::ReportAIState(level);
}

class CDeadCleansuitScientist : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/cleansuit_scientist.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("cleansuit_scientist"); }
	int	DefaultClassify() override { return	CLASS_HUMAN_PASSIVE; }

	const char* getPos(int pos) const override;
	static const char *m_szPoses[9];
};
const char *CDeadCleansuitScientist::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3", "scientist_deadpose1", "dead_against_wall" };

const char* CDeadCleansuitScientist::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_cleansuit_scientist_dead, CDeadCleansuitScientist )

void CDeadCleansuitScientist::Spawn()
{
	SpawnHelper();
	if ( pev->body == -1 ) {
		pev->body = RANDOM_LONG(0, g_modFeatures.scientist_random_heads-1);
	}
	MonsterInitDead();
}

class CSittingCleansuitScientist : public CSittingScientist
{
public:
	void Spawn() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("cleansuit_scientist"); }
};

void CSittingCleansuitScientist::Spawn()
{
	SciSpawnHelper("models/cleansuit_scientist.mdl");
}

LINK_ENTITY_TO_CLASS( monster_sitting_cleansuit_scientist, CSittingCleansuitScientist )

#define FEATURE_ROSENBERG_DECAY 0

class CRosenberg : public CScientist
{
public:
	int GetDefaultVoicePitch() override { return 100; }
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("rosenberg"); }
	const char* DefaultDisplayName() override { return "Dr. Rosenberg"; }
	const char* DefaultSentenceGroup(int group) override;
	int DefaultToleranceLevel() override { return TOLERANCE_ABSOLUTE; }
	void PainSound() override;
	void DeathSound() override;

#if FEATURE_ROSENBERG_DECAY
	bool AbleToHeal() override { return false; }
#endif

	static const NamedSoundScript painSoundScript;
	static constexpr const char* dieSoundScript = "Rosenberg.Die";
};

LINK_ENTITY_TO_CLASS( monster_rosenberg, CRosenberg )

const NamedSoundScript CRosenberg::painSoundScript = {
	CHAN_VOICE,
	{
		"rosenberg/ro_pain0.wav", "rosenberg/ro_pain1.wav", "rosenberg/ro_pain2.wav",
		"rosenberg/ro_pain3.wav", "rosenberg/ro_pain4.wav", "rosenberg/ro_pain5.wav",
		"rosenberg/ro_pain6.wav", "rosenberg/ro_pain7.wav", "rosenberg/ro_pain8.wav"
	},
	"Rosenberg.Pain"
};

void CRosenberg::Spawn()
{
#if FEATURE_ROSENBERG_DECAY
	SciSpawnHelper("models/scientist_rosenberg.mdl", gSkillData.scientistHealth * 2);
#else
	SciSpawnHelper("models/scientist.mdl", GetSkillValue("scientist_health") * 2);
	CalcTotalHeadCount();
	pev->body = 3;
#endif
	TalkMonsterInit();
}

void CRosenberg::Precache()
{
#if FEATURE_ROSENBERG_DECAY
	PrecacheMyModel("models/scientist_rosenberg.mdl");
#else
	PrecacheMyModel("models/scientist.mdl");
	CalcTotalHeadCount();
#endif
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, painSoundScript);
	RegisterAndPrecacheSoundScript(CScientist::healSoundScript);

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

const char* CRosenberg::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "RO_ANSWER";
	case TLK_QUESTION: return "RO_QUESTION";
	case TLK_IDLE: return "RO_IDLE";
	case TLK_STARE: return "RO_STARE";
	case TLK_USE: return "RO_OK";
	case TLK_UNUSE: return "RO_WAIT";
	case TLK_DECLINE: return "RO_POK";
	case TLK_STOP: return "RO_STOP";
	case TLK_NOSHOOT: return "RO_SCARED";
	case TLK_HELLO: return "RO_HELLO";
	case TLK_PLHURT1: return "!RO_CUREA";
	case TLK_PLHURT2: return "!RO_CUREB";
	case TLK_PLHURT3: return "!RO_CUREC";
	case TLK_PHELLO: return "RO_PHELLO";
	case TLK_PIDLE: return "RO_PIDLE";
	case TLK_PQUESTION: return "RO_PQUEST";
	case TLK_SMELL: return "RO_SMELL";
	case TLK_WOUND: return "RO_WOUND";
	case TLK_MORTAL: return "RO_MORTAL";
	case TLK_SHOT: return NULL;
#if FEATURE_SCIENTIST_PLFEAR
	case TLK_MAD: return "RO_PLFEAR";
#else
	case TLK_MAD: return NULL;
#endif
	case TLK_HEAL: return "RO_HEAL";
	case TLK_SCREAM: return "RO_SCREAM";
	case TLK_FEAR: return "RO_FEAR";
	case TLK_PLFEAR: return "RO_PLFEAR";
	default: return NULL;
	}
}

void CRosenberg::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

void CRosenberg::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

class CCivilian : public CScientist
{
public:
	int GetDefaultVoicePitch() override { return 100; }
	void Spawn() override
	{
		SciSpawnHelper("models/scientist.mdl", GetSkillValue("civilian_health"));
		TalkMonsterInit();
	}
	void Precache() override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override {
		CTalkMonster::HandleAnimEvent(pEvent);
	}
	const char* DefaultDisplayName() override { return "Civilian"; }
	bool AbleToHeal() override { return false; }

	static constexpr const char* painSoundScript = "Civilian.Pain";
	static constexpr const char* dieSoundScript = "Civilian.Die";

	void PainSound() override {
		EmitSoundScriptTalk(painSoundScript);
	}
	void DeathSound() override {
		EmitSoundScriptTalk(dieSoundScript);
	}
};

LINK_ENTITY_TO_CLASS( monster_civilian, CCivilian )

void CCivilian::Precache()
{
	PrecacheMyModel("models/scientist.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript, CScientist::painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, CScientist::dieSoundScript, CScientist::painSoundScript);

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

class CKeller : public CScientist
{
public:
	int GetDefaultVoicePitch() override { return 100; }
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("keller"); }
	const char* DefaultDisplayName() override { return "Richard Keller"; }
	const char* DefaultSentenceGroup(int group) override;
	int DefaultToleranceLevel() override { return TOLERANCE_ABSOLUTE; }
	void PainSound() override;
	void DeathSound() override;

	bool AbleToHeal() override { return false; }

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
};

LINK_ENTITY_TO_CLASS( monster_wheelchair, CKeller )

const NamedSoundScript CKeller::painSoundScript = {
	CHAN_VOICE,
	{
		"keller/dk_pain1.wav", "keller/dk_pain2.wav", "keller/dk_pain3.wav", "keller/dk_pain4.wav",
		"keller/dk_pain5.wav", "keller/dk_pain6.wav", "keller/dk_pain7.wav"
	},
	"Keller.Pain"
};

const NamedSoundScript CKeller::dieSoundScript = {
	CHAN_VOICE,
	{
		"keller/dk_die1.wav", "keller/dk_die2.wav", "keller/dk_die3.wav", "keller/dk_die4.wav",
		"keller/dk_die5.wav", "keller/dk_die6.wav", "keller/dk_die7.wav"
	},
	"Keller.Die"
};

void CKeller::Spawn()
{
	SciSpawnHelper("models/wheelchair_sci.mdl", GetSkillValue("scientist_health") * 2);
	TalkMonsterInit();
}

void CKeller::Precache()
{
	PrecacheMyModel("models/wheelchair_sci.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);

	PRECACHE_SOUND( "wheelchair/wheelchair_jog.wav" );
	PRECACHE_SOUND( "wheelchair/wheelchair_run.wav" );
	PRECACHE_SOUND( "wheelchair/wheelchair_walk.wav" );

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

const char* CKeller::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "DK_ANSWER";
	case TLK_QUESTION: return "DK_QUESTION";
	case TLK_IDLE: return "DK_IDLE";
	case TLK_STARE: return "DK_STARE";
	case TLK_USE: return "DK_OK";
	case TLK_UNUSE: return "DK_WAIT";
	case TLK_DECLINE: return "DK_POK";
	case TLK_STOP: return "DK_STOP";
	case TLK_NOSHOOT: return "DK_SCARED";
	case TLK_HELLO: return "DK_HELLO";
	case TLK_PLHURT1: return "!DK_CUREA";
	case TLK_PLHURT2: return "!DK_CUREB";
	case TLK_PLHURT3: return "!DK_CUREC";
	case TLK_PHELLO: return "DK_PHELLO";
	case TLK_PIDLE: return "DK_PIDLE";
	case TLK_PQUESTION: return "DK_PQUEST";
	case TLK_SMELL: return "DK_SMELL";
	case TLK_WOUND: return "DK_WOUND";
	case TLK_MORTAL: return "DK_MORTAL";
	case TLK_SHOT: return "DK_PLFEAR";
#if FEATURE_SCIENTIST_PLFEAR
	case TLK_MAD: return "DK_PLFEAR";
#else
	case TLK_MAD: return NULL;
#endif
	case TLK_HEAL: return "DK_HEAL";
	case TLK_SCREAM: return "DK_SCREAM";
	case TLK_FEAR: return "DK_FEAR";
	case TLK_PLFEAR: return "DK_PLFEAR";
	default: return NULL;
	}
}

void CKeller::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

void CKeller::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}
