//========= Copyright © 2004-2008, Raven City Team, All rights reserved. ============//
//																					 //
// Purpose:																			 //
//																					 //
// $NoKeywords: $																	 //
//===================================================================================//

//=========================================================
// monster template
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"animation.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"customentity.h"
#include	"decals.h"
#include	"mod_features.h"

#if FEATURE_OPFOR_GRUNT
//=========================================================
//
//=========================================================
#define	FGRUNT_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define FGRUNT_LIMP_HEALTH				20

#define FGRUNT_MEDIC_WAIT				5 // Wait before calling for medic again.

#define FGRUNT_9MMAR					( 1 << 0)
#define FGRUNT_HANDGRENADE			( 1 << 1)
#define FGRUNT_GRENADELAUNCHER		( 1 << 2)
#define FGRUNT_SHOTGUN				( 1 << 3)
#define FGRUNT_M249					( 1 << 4)

// Torso group for weapons
#define	FG_TORSO_GROUP				2
#define FG_TORSO_DEFAULT			0
#define FG_TORSO_M249				1
#define FG_TORSO_FLAT				2
#define FG_TORSO_SHOTGUN			3

// Weapon group
#define FG_GUN_GROUP				3
#define FG_GUN_MP5					0
#define FG_GUN_SHOTGUN				1
#define FG_GUN_SAW					2
#define FG_GUN_NONE					3

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_FGRUNT_NOFIRE	( bits_COND_SPECIAL1 )
//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_HGRUNT_ALLY_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
	TASK_HGRUNT_ALLY_CHECK_FIRE,
	TASK_HGRUNT_ALLY_FIND_MEDIC,
	LAST_HGRUNT_ALLY_TASK
};
//=========================================================
// monster heads
//=========================================================

// Head group
#define FG_HEAD_GROUP				1
enum
{
	FG_HEAD_MASK,
	FG_HEAD_BERET,
	FG_HEAD_SHOTGUN,
	FG_HEAD_SAW,
	FG_HEAD_SAW_BLACK,
	FG_HEAD_MP,
	FG_HEAD_MAJOR,
	FG_HEAD_BERET_BLACK,
	FG_HEAD_COUNT
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HGRUNT_ALLY_AE_RELOAD		( 2 )
#define		HGRUNT_ALLY_AE_KICK			( 3 )
#define		HGRUNT_ALLY_AE_BURST1		( 4 )
#define		HGRUNT_ALLY_AE_BURST2		( 5 )
#define		HGRUNT_ALLY_AE_BURST3		( 6 )
#define		HGRUNT_ALLY_AE_GREN_TOSS	( 7 )
#define		HGRUNT_ALLY_AE_GREN_LAUNCH	( 8 )
#define		HGRUNT_ALLY_AE_GREN_DROP	( 9 )
#define		HGRUNT_ALLY_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		HGRUNT_ALLY_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.
//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_HGRUNT_ALLY_SUPPRESS = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_HGRUNT_ALLY_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_HGRUNT_ALLY_COVER_AND_RELOAD,
	SCHED_HGRUNT_ALLY_SWEEP,
	SCHED_HGRUNT_ALLY_FOUND_ENEMY,
	SCHED_HGRUNT_ALLY_REPEL,
	SCHED_HGRUNT_ALLY_REPEL_ATTACK,
	SCHED_HGRUNT_ALLY_REPEL_LAND,
	SCHED_HGRUNT_ALLY_WAIT_FACE_ENEMY,
	SCHED_HGRUNT_ALLY_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_HGRUNT_ALLY_ELOF_FAIL,
	SCHED_HGRUNT_ALLY_FIND_MEDIC,
};
class CHFGrunt : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	int  DefaultClassify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	void KeyValue( KeyValueData *pkvd );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	BOOL FCanCheckAttacks ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	void DeclineFollowing( void );
	void PrescheduleThink ( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void Shotgun ( void );
	void M249 ( void );
	// Override these to set behavior
	CBaseEntity	*Kick( void );
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	void GibMonster( void );
	void TalkInit( void );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	void DropMyItems(BOOL isGibbed);
	void DropMyItem(const char *entityName, const Vector &vecGunPos, const Vector &vecGunAngles, BOOL isGibbed);

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int IRelationship ( CBaseEntity *pTarget );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	bool CallForMedic(CBaseMonster *pMember);

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;
	float m_flMedicWaitTime;

	float	m_flLinkToggle;// how much pain has the player inflicted on me?

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int		m_iBrassShell;
	int		m_iShotgunShell;

	int		m_iSentence;
	int		m_iHead;

	int		m_iM249Shell;
	int		m_iM249Link;

	static const char *pGruntSentences[];

	CUSTOM_SCHEDULES

protected:
	void KickImpl(float kickDamage);
	void PrecacheHelper();
	void SpawnHelper(const char* defaultModel, float defaultHealth);
};

LINK_ENTITY_TO_CLASS( monster_human_grunt_ally, CHFGrunt )

class CMedic : public CHFGrunt
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void GibMonster();

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	Schedule_t *GetSchedule ( void );
	void SetAnswerQuestion(CTalkMonster *pSpeaker);

	void DropMyItems(BOOL isGibbed);

	void FirePistol ( const char* shotSound, Bullet bullet );
	bool Heal();
	void StartFollowingHealTarget(CBaseEntity* pTarget);
	void StopHealing();
	CBaseEntity* HealTarget();
	inline bool HasHealTarget() { return HealTarget() != 0; }
	inline bool HasHealCharge() { return m_flHealCharge >= 1; }
	bool CheckHealCharge();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES
	float m_flHealCharge;
	BOOL m_fDepleteLine;
	BOOL m_fHealing;
};

TYPEDESCRIPTION	CHFGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CHFGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHFGrunt, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CHFGrunt, m_flMedicWaitTime, FIELD_TIME ),
	DEFINE_FIELD( CHFGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHFGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHFGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHFGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHFGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHFGrunt, m_iHead, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHFGrunt, CTalkMonster )
const char *CHFGrunt::pGruntSentences[] =
{
	"FG_GREN", // grenade scared grunt
	"FG_ALERT", // sees player
	"FG_MONSTER", // sees monster
	"FG_COVER", // running to cover
	"FG_THROW", // about to throw grenade
	"FG_CHARGE",  // running out to get the enemy
	"FG_TAUNT", // say rude things
};

typedef enum
{
	FGRUNT_SENT_NONE = -1,
	FGRUNT_SENT_GREN = 0,
	FGRUNT_SENT_ALERT,
	FGRUNT_SENT_MONSTER,
	FGRUNT_SENT_COVER,
	FGRUNT_SENT_THROW,
	FGRUNT_SENT_CHARGE,
	FGRUNT_SENT_TAUNT,
} FGRUNT_SENTENCE_TYPES;

//=========================================================
// KeyValue
//
// !!! netname entvar field is used in squadmonster for groupname!!!
//=========================================================
void CHFGrunt :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CTalkMonster::KeyValue( pkvd );
	}
}
//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CHFGrunt :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	// if in the grip of a barnacle, don't speak
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
	{
		return FALSE;
	}

	// if not alive, certainly don't speak
	if ( pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	return TRUE;
}
//=========================================================
//=========================================================
void CHFGrunt :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = FGRUNT_SENT_NONE;
}
//=========================================================
// IRelationship - overridden because Male Assassins are
// Human Grunt's nemesis.
//=========================================================
int CHFGrunt::IRelationship ( CBaseEntity *pTarget )
{
	if ( IDefaultRelationship(pTarget) >= R_DL && FClassnameIs( pTarget->pev, "monster_male_assassin" ) )
	{
		return R_NM;
	}

	return CTalkMonster::IRelationship( pTarget );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlFGruntFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slFGruntFollow[] =
{
	{
		tlFGruntFollow,
		ARRAYSIZE ( tlFGruntFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};
Task_t	tlFGruntFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slFGruntFaceTarget[] =
{
	{
		tlFGruntFaceTarget,
		ARRAYSIZE ( tlFGruntFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlFGruntIdleStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slFGruntIdleStand[] =
{
	{
		tlFGruntIdleStand,
		ARRAYSIZE ( tlFGruntIdleStand ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};
//=========================================================
// FGruntFail
//=========================================================
Task_t	tlFGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slFGruntFail[] =
{
	{
		tlFGruntFail,
		ARRAYSIZE ( tlFGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"FGrunt Fail"
	},
};

//=========================================================
// FGrunt Combat Fail
//=========================================================
Task_t	tlFGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slFGruntCombatFail[] =
{
	{
		tlFGruntCombatFail,
		ARRAYSIZE ( tlFGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"FGrunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlFGruntVictoryDance[] =
{
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_FAIL			},
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slFGruntVictoryDance[] =
{
	{
		tlFGruntVictoryDance,
		ARRAYSIZE ( tlFGruntVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"FGruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlFGruntEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_HGRUNT_ALLY_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slFGruntEstablishLineOfFire[] =
{
	{
		tlFGruntEstablishLineOfFire,
		ARRAYSIZE ( tlFGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"FGruntEstablishLineOfFire"
	},
};

//=========================================================
// FGruntFoundEnemy - FGrunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlFGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slFGruntFoundEnemy[] =
{
	{
		tlFGruntFoundEnemy,
		ARRAYSIZE ( tlFGruntFoundEnemy ),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"FGruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlFGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HGRUNT_ALLY_SWEEP	},
};

Schedule_t	slFGruntCombatFace[] =
{
	{
		tlFGruntCombatFace1,
		ARRAYSIZE ( tlFGruntCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or FGrunt gets hurt.
//=========================================================
Task_t	tlFGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING,						0						},
	{ TASK_FACE_IDEAL,						(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
};

Schedule_t	slFGruntSignalSuppress[] =
{
	{
		tlFGruntSignalSuppress,
		ARRAYSIZE ( tlFGruntSignalSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_FGRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlFGruntSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slFGruntSuppress[] =
{
	{
		tlFGruntSuppress,
		ARRAYSIZE ( tlFGruntSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_FGRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// FGrunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlFGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slFGruntWaitInCover[] =
{
	{
		tlFGruntWaitInCover,
		ARRAYSIZE ( tlFGruntWaitInCover ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"FGruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlFGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_HGRUNT_ALLY_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HGRUNT_ALLY_WAIT_FACE_ENEMY	},
};

Schedule_t	slFGruntTakeCover[] =
{
	{
		tlFGruntTakeCover1,
		ARRAYSIZE ( tlFGruntTakeCover1 ),
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlFGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_HGRUNT_ALLY_WAIT_FACE_ENEMY	},
};

Schedule_t	slFGruntGrenadeCover[] =
{
	{
		tlFGruntGrenadeCover1,
		ARRAYSIZE ( tlFGruntGrenadeCover1 ),
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlFGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slFGruntTossGrenadeCover[] =
{
	{
		tlFGruntTossGrenadeCover1,
		ARRAYSIZE ( tlFGruntTossGrenadeCover1 ),
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlFGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slFGruntTakeCoverFromBestSound[] =
{
	{
		tlFGruntTakeCoverFromBestSound,
		ARRAYSIZE ( tlFGruntTakeCoverFromBestSound ),
		0,
		0,
		"FGruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlFGruntHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slFGruntHideReload[] =
{
	{
		tlFGruntHideReload,
		ARRAYSIZE ( tlFGruntHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"FGruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlFGruntSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slFGruntSweep[] =
{
	{
		tlFGruntSweep,
		ARRAYSIZE ( tlFGruntSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"FGrunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlFGruntRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slFGruntRangeAttack1A[] =
{
	{
		tlFGruntRangeAttack1A,
		ARRAYSIZE ( tlFGruntRangeAttack1A ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_FGRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlFGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HGRUNT_ALLY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slFGruntRangeAttack1B[] =
{
	{
		tlFGruntRangeAttack1B,
		ARRAYSIZE ( tlFGruntRangeAttack1B ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_FGRUNT_NOFIRE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlFGruntRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_HGRUNT_ALLY_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HGRUNT_ALLY_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slFGruntRangeAttack2[] =
{
	{
		tlFGruntRangeAttack2,
		ARRAYSIZE ( tlFGruntRangeAttack2 ),
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel
//=========================================================
Task_t	tlFGruntRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slFGruntRepel[] =
{
	{
		tlFGruntRepel,
		ARRAYSIZE ( tlFGruntRepel ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel"
	},
};


//=========================================================
// repel
//=========================================================
Task_t	tlFGruntRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slFGruntRepelAttack[] =
{
	{
		tlFGruntRepelAttack,
		ARRAYSIZE ( tlFGruntRepelAttack ),
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlFGruntRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slFGruntRepelLand[] =
{
	{
		tlFGruntRepelLand,
		ARRAYSIZE ( tlFGruntRepelLand ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel Land"
	},
};

//=========================================================
// Find medic. Grunt stops moving and calls the nearest medic,
// if none is around, we don't do much. I don't think I have much
// to put in here, other than to make the grunt stop moving, and
// run the medic calling task, I guess.
//=========================================================
Task_t	tlFGruntFindMedic[] =
{
	{ TASK_STOP_MOVING,					(float)0	},
	{ TASK_HGRUNT_ALLY_FIND_MEDIC,		(float)0	},
};

Schedule_t	slFGruntFindMedic[] =
{
	{
		tlFGruntFindMedic,
		ARRAYSIZE ( tlFGruntFindMedic ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,

		"FGrunt Find Medic"
	},
};


DEFINE_CUSTOM_SCHEDULES( CHFGrunt )
{
	slFGruntFollow,
	slFGruntFaceTarget,
	slFGruntIdleStand,
	slFGruntFail,
	slFGruntCombatFail,
	slFGruntVictoryDance,
	slFGruntEstablishLineOfFire,
	slFGruntFoundEnemy,
	slFGruntCombatFace,
	slFGruntSignalSuppress,
	slFGruntSuppress,
	slFGruntWaitInCover,
	slFGruntTakeCover,
	slFGruntGrenadeCover,
	slFGruntTossGrenadeCover,
	slFGruntTakeCoverFromBestSound,
	slFGruntHideReload,
	slFGruntSweep,
	slFGruntRangeAttack1A,
	slFGruntRangeAttack1B,
	slFGruntRangeAttack2,
	slFGruntRepel,
	slFGruntRepelAttack,
	slFGruntRepelLand,
	slFGruntFindMedic,
};


IMPLEMENT_CUSTOM_SCHEDULES( CHFGrunt, CTalkMonster )

bool CHFGrunt::CallForMedic(CBaseMonster *pMember)
{
	if ( pMember && pMember != this && pMember->pev->deadflag == DEAD_NO && FClassnameIs( pMember->pev, "monster_human_medic_ally" ) )
	{
		CMedic* medic = (CMedic*)pMember;
		ALERT( at_aiconsole, "Injured Grunt found Medic\n" );
		if ( medic->CanFollow() && medic->HasHealCharge() && !medic->HasHealTarget() )
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/medic.wav", 1, ATTN_NORM, 0, GetVoicePitch());
			ALERT( at_aiconsole, "Injured Grunt called for Medic\n" );
			medic->StartFollowingHealTarget(this);
			return true;
		}
	}
	return false;
}

void CHFGrunt :: StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_HGRUNT_ALLY_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_FGRUNT_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_HGRUNT_ALLY_FIND_MEDIC:
			// First try looking for a medic in my squad
			if ( InSquad() )
			{
				CSquadMonster *pSquadLeader = MySquadLeader( );
				if ( pSquadLeader ) for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
				{
					CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
					if (CallForMedic(pMember)) {
						TaskComplete();
					}
				}
			}
			// If not, search bsp.
			if ( !TaskIsComplete() )
			{
				CBaseEntity *pFriend = NULL;
				int i;

				// for each friend in this bsp...
				for ( i = 0; i < TLK_CFRIENDS; i++ )
				{
					while (pFriend = EnumFriends( pFriend, i, TRUE ))
					{
						CBaseMonster *pMonster = pFriend->MyMonsterPointer();
						if (CallForMedic(pMonster)) {
							TaskComplete();
						}
					}
				}
			}
			if ( !TaskIsComplete() )
			{
				TaskFail();
			}
			m_flMedicWaitTime = FGRUNT_MEDIC_WAIT + gpGlobals->time; // Call again in ten seconds anyway.
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CTalkMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_HGRUNT_ALLY_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CTalkMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		CTalkMonster :: StartTask( pTask );
		break;
	}
}

void CHFGrunt :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{

	case TASK_HGRUNT_ALLY_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CTalkMonster :: RunTask( pTask );
			break;
		}
	}
}
//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CHFGrunt :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( FG_GUN_GROUP ) != FG_GUN_NONE )
	{// throw a gun if the grunt has one
		DropMyItems(TRUE);
	}

	CTalkMonster::GibMonster();
}

void CHFGrunt::DropMyItem(const char* entityName, const Vector& vecGunPos, const Vector& vecGunAngles, BOOL isGibbed)
{
	CBaseEntity* pGun = DropItem(entityName, vecGunPos, vecGunAngles);
	if (pGun && isGibbed) {
		pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
		pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
	}
}

void CHFGrunt::DropMyItems(BOOL isGibbed)
{
	Vector vecGunPos;
	Vector vecGunAngles;
	GetAttachment( 0, vecGunPos, vecGunAngles );

	if (!isGibbed) {
		SetBodygroup( FG_GUN_GROUP, FG_GUN_NONE );
	}

	GetAttachment( 0, vecGunPos, vecGunAngles );

	if (FBitSet( pev->weapons, FGRUNT_SHOTGUN ))
	{
		DropMyItem( "weapon_shotgun", vecGunPos, vecGunAngles, isGibbed );
	}
	else if (FBitSet( pev->weapons, FGRUNT_9MMAR ))
	{
		DropMyItem( "weapon_9mmAR", vecGunPos, vecGunAngles, isGibbed );
	}
	else if (FBitSet( pev->weapons, FGRUNT_M249 ))
	{
		DropMyItem( M249_DROP_NAME, vecGunPos, vecGunAngles, isGibbed );
	}

	if (FBitSet( pev->weapons, FGRUNT_GRENADELAUNCHER ))
	{
		DropMyItem( "ammo_ARgrenades", isGibbed ? vecGunPos : BodyTarget( pev->origin ), vecGunAngles, isGibbed );
	}
	pev->weapons = 0;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CHFGrunt :: ISoundMask ( void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}
//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHFGrunt :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}
//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CHFGrunt :: DefaultClassify ( void )
{
#if FEATURE_OPFOR_ALLY_RELATIONSHIP
	return CLASS_PLAYER_ALLY_MILITARY;
#else
	return	CLASS_PLAYER_ALLY;
#endif
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHFGrunt :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHFGrunt :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != 0 )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
	CTalkMonster::PrescheduleThink();
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds.
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CHFGrunt :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CHFGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != 0 )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&&
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CHFGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() && ( GetBodygroup( 3 ) != 3 ) )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
BOOL CHFGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, (FGRUNT_HANDGRENADE | FGRUNT_GRENADELAUNCHER)) || FBitSet(pev->weapons, ( FGRUNT_M249 )) )
	{
		return FALSE;
	}

	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet( pev->weapons, FGRUNT_HANDGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.fgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
			return m_fThrowGrenade;	//AJH need this or it is overridden later.
		}
	}

	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}


	if (FBitSet( pev->weapons, FGRUNT_HANDGRENADE))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.fgruntGrenadeSpeed, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}



	return m_fThrowGrenade;
}
//=========================================================
//=========================================================
CBaseEntity *CHFGrunt :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

void CHFGrunt::KickImpl(float kickDamage)
{
	CBaseEntity *pHurt = Kick();

	if ( pHurt )
	{
		// SOUND HERE!
		UTIL_MakeVectors( pev->angles );
		pHurt->pev->punchangle.x = 15;
		pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
		pHurt->TakeDamage( pev, pev, kickDamage, DMG_CLUB );
	}
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CHFGrunt :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CHFGrunt :: Shoot ( void )
{
	if (m_hEnemy == 0 )
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_4DEGREES, 2048, BULLET_MONSTER_MP5 ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	//WeaponFlash ( vecShootOrigin );

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CHFGrunt :: Shotgun ( void )
{
	if (m_hEnemy == 0)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(gSkillData.fgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_9DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	//WeaponFlash ( vecShootOrigin );

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}
//=========================================================
// Shoot
//=========================================================
void CHFGrunt :: M249 ( void )
{
	if (m_hEnemy == 0 )
	{
		return;
	}

	switch ( RANDOM_LONG(0,2) )
	{
		case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/saw_fire1.wav", 1, ATTN_NORM ); break;
		case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/saw_fire2.wav", 1, ATTN_NORM ); break;
		case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/saw_fire3.wav", 1, ATTN_NORM ); break;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);

	m_flLinkToggle = !m_flLinkToggle;

	if (!m_flLinkToggle)
		EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iM249Shell, TE_BOUNCE_SHELL);
	else
		EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iM249Link, TE_BOUNCE_SHELL);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, 2048, BULLET_MONSTER_556 ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	//WeaponFlash ( vecShootOrigin );

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHFGrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HGRUNT_ALLY_AE_DROP_GUN:
		{
			DropMyItems(FALSE);
		}
		break;

		case HGRUNT_ALLY_AE_RELOAD:
			if (FBitSet( pev->weapons, FGRUNT_9MMAR | FGRUNT_SHOTGUN ))
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			}
			else if (FBitSet( pev->weapons, FGRUNT_M249 ))
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/saw_reload2.wav", 1, ATTN_NORM );
			}
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HGRUNT_ALLY_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case HGRUNT_ALLY_AE_GREN_LAUNCH:
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			if (g_iSkillLevel == SKILL_EASY)
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;

		case HGRUNT_ALLY_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case HGRUNT_ALLY_AE_BURST1:
		{
			if ( FBitSet( pev->weapons, FGRUNT_9MMAR ))
			{
				Shoot();
				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if( RANDOM_LONG( 0, 1 ) )
				{
					EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM );
				}
			}
			else if ( FBitSet( pev->weapons, FGRUNT_SHOTGUN ))
			{
				Shotgun( );

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
			}
			else
			{
				M249( );
			}

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case HGRUNT_ALLY_AE_BURST2:
		case HGRUNT_ALLY_AE_BURST3:
			if ( FBitSet( pev->weapons, FGRUNT_9MMAR ))
				Shoot();
			else if ( FBitSet( pev->weapons, FGRUNT_M249 ))
				M249();
			break;

		case HGRUNT_ALLY_AE_KICK:
		{
			KickImpl(gSkillData.fgruntDmgKick);
		}
		break;

		case HGRUNT_ALLY_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "FG_ALERT", VOL_NORM, ATTN_NORM, 0, m_voicePitch);
				JustSpoke();
			}

		}

		default:
			CTalkMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHFGrunt :: Spawn()
{
	Precache( );

	SpawnHelper("models/hgrunt_opfor.mdl", gSkillData.fgruntHealth);

	if ( m_iHead < 0 || m_iHead >= FG_HEAD_COUNT )
		m_iHead = 0;

	if ( pev->weapons == 0 || pev->weapons == -1 )
	{
		pev->weapons = FGRUNT_9MMAR;
	}
	if (FBitSet( pev->weapons, FGRUNT_SHOTGUN ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_SHOTGUN );
		SetBodygroup( FG_TORSO_GROUP, FG_TORSO_SHOTGUN );
		m_cClipSize		= 8;
	}
	if (FBitSet( pev->weapons, FGRUNT_9MMAR ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_MP5 );
		m_cClipSize	= FGRUNT_CLIP_SIZE;
	}
	if (FBitSet( pev->weapons, FGRUNT_M249 ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_SAW );
		SetBodygroup( FG_TORSO_GROUP, FG_TORSO_M249 );
		m_cClipSize	= FGRUNT_CLIP_SIZE;
	}

	SetBodygroup( FG_HEAD_GROUP, m_iHead );

	m_cAmmoLoaded		= m_cClipSize;

	MonsterInit();
	SetUse( &CHFGrunt::FollowerUse );
}

void CHFGrunt::SpawnHelper(const char *defaultModel, float defaultHealth)
{
	SetMyModel(defaultModel);
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( defaultHealth );
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHFGrunt :: Precache()
{
	PrecacheMyModel("models/hgrunt_opfor.mdl");

	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );

	PRECACHE_SOUND("weapons/saw_fire1.wav" );
	PRECACHE_SOUND("weapons/saw_fire2.wav" );
	PRECACHE_SOUND("weapons/saw_fire3.wav" );

	PrecacheHelper();

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/saw_reload2.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell
	m_iM249Shell = PRECACHE_MODEL ("models/saw_shell.mdl");// saw shell
	m_iM249Link = PRECACHE_MODEL ("models/saw_link.mdl");// saw link

	TalkInit();
	switch ( m_iHead ) {
	case FG_HEAD_SHOTGUN:
	case FG_HEAD_SAW_BLACK:
	case FG_HEAD_BERET_BLACK:
		m_voicePitch = 90;
		break;
	default:
		m_voicePitch = 100;
		break;
	}

	CTalkMonster::Precache();
}

void CHFGrunt::PrecacheHelper()
{
	PRECACHE_SOUND("fgrunt/gr_pain1.wav");
	PRECACHE_SOUND("fgrunt/gr_pain2.wav");
	PRECACHE_SOUND("fgrunt/gr_pain3.wav");
	PRECACHE_SOUND("fgrunt/gr_pain4.wav");
	PRECACHE_SOUND("fgrunt/gr_pain5.wav");
	PRECACHE_SOUND("fgrunt/gr_pain6.wav");

	PRECACHE_SOUND("fgrunt/death1.wav");
	PRECACHE_SOUND("fgrunt/death2.wav");
	PRECACHE_SOUND("fgrunt/death3.wav");
	PRECACHE_SOUND("fgrunt/death4.wav");
	PRECACHE_SOUND("fgrunt/death5.wav");
	PRECACHE_SOUND("fgrunt/death6.wav");

	PRECACHE_SOUND("fgrunt/medic.wav");

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

// Init talk data
void CHFGrunt :: TalkInit()
{
	CTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER]  =	"FG_ANSWER";
	m_szGrp[TLK_QUESTION] =	"FG_QUESTION";
	m_szGrp[TLK_IDLE] =		"FG_IDLE";
	m_szGrp[TLK_STARE] =	"FG_STARE";
	m_szGrp[TLK_USE] =		"FG_OK";
	m_szGrp[TLK_UNUSE] =	"FG_WAIT";
	m_szGrp[TLK_STOP] =		"FG_STOP";

	m_szGrp[TLK_NOSHOOT] =	"FG_SCARED";
	m_szGrp[TLK_HELLO] =	"FG_HELLO";

	m_szGrp[TLK_PLHURT1] =	"FG_CURE";
	m_szGrp[TLK_PLHURT2] =	"FG_CURE";
	m_szGrp[TLK_PLHURT3] =	"FG_CURE";

	m_szGrp[TLK_SMELL] =	"FG_SMELL";

	m_szGrp[TLK_WOUND] =	"FG_WOUND";
	m_szGrp[TLK_MORTAL] =	"FG_MORTAL";
}


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// PainSound
//=========================================================
void CHFGrunt :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,5) )
		{
			case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/gr_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/gr_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/gr_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/gr_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 4: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/gr_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 5: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/gr_pain6.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CHFGrunt :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,5))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/death1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/death2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/death3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/death4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/death5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 5: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "fgrunt/death6.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}
//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CHFGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		if (GetBodygroup( FG_HEAD_GROUP ) == FG_HEAD_MASK && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}
//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CHFGrunt :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) && IsFriendWithPlayerBeforeProvoked() )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == 0 )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "FG_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "FG_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "FG_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return CTalkMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CHFGrunt :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			// call base class default so that barney will talk
			// when 'used'
			psched = CTalkMonster::GetScheduleOfType(Type);

			if (psched == slIdleStand)
				return slFGruntFaceTarget;	// override this for different target face behavior
			else
				return psched;
		}
		break;
	case SCHED_TARGET_CHASE:
		{
			return slFGruntFollow;
		}
		break;
	case SCHED_IDLE_STAND:
		{
			psched = CTalkMonster::GetScheduleOfType(Type);

			if (psched == slIdleStand)
			{
				// just look straight ahead.
				return slFGruntIdleStand;
			}
			else
				return psched;
		}
		break;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slFGruntTakeCover[ 0 ];
		}
		break;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slFGruntTakeCoverFromBestSound[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_FIND_MEDIC:
		{
			return &slFGruntFindMedic[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
			else
			{
				return GetScheduleOfType ( SCHED_FAIL );
			}
		}
		break;
	case SCHED_HGRUNT_ALLY_ELOF_FAIL:
		{
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
		break;
	case SCHED_HGRUNT_ALLY_ESTABLISH_LINE_OF_FIRE:
		{
			return &slFGruntEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);

			if (m_fStanding)
				return &slFGruntRangeAttack1B[ 0 ];
			else
				return &slFGruntRangeAttack1A[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK2:
		{
			return &slFGruntRangeAttack2[ 0 ];
		}
		break;
	case SCHED_COMBAT_FACE:
		{
			return &slFGruntCombatFace[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_WAIT_FACE_ENEMY:
		{
			return &slFGruntWaitInCover[ 0 ];
		}
	case SCHED_HGRUNT_ALLY_SWEEP:
		{
			return &slFGruntSweep[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_COVER_AND_RELOAD:
		{
			return &slFGruntHideReload[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_FOUND_ENEMY:
		{
			return &slFGruntFoundEnemy[ 0 ];
		}
		break;
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slFGruntFail[ 0 ];
				}
			}
			if ( IsFollowing() )
			{
				return &slFGruntFail[ 0 ];
			}

			return &slFGruntVictoryDance[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_SUPPRESS:
		{
			if ( m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slFGruntSignalSuppress[ 0 ];
			}
			else
			{
				return &slFGruntSuppress[ 0 ];
			}
		}
		break;
	case SCHED_FAIL:
		{
			if ( m_hEnemy != 0 )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slFGruntCombatFail[ 0 ];
			}

			return &slFGruntFail[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slFGruntRepel[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slFGruntRepelAttack[ 0 ];
		}
		break;
	case SCHED_HGRUNT_ALLY_REPEL_LAND:
		{
			return &slFGruntRepelLand[ 0 ];
		}
		break;
	default:
		{
			return CTalkMonster :: GetScheduleOfType ( Type );
		}
	}
}
//=========================================================
// SetActivity
//=========================================================
void CHFGrunt :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, FGRUNT_SHOTGUN))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_shotgun" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_shotgun" );
			}
		}
		else if (FBitSet( pev->weapons, FGRUNT_M249 ))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_saw" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_saw" );
			}
		}
		else
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_mp5" );
			}
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & FGRUNT_HANDGRENADE )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		else if ( pev->weapons & FGRUNT_GRENADELAUNCHER )
		{
			// get launch anim
			iSequence = LookupSequence( "launchgrenade" );
		}
		break;
	case ACT_RUN:
		if ( pev->health <= FGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= FGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}
//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CHFGrunt :: GetSchedule ( void )
{
	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.

				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "FG_GREN", VOL_NORM, ATTN_NORM, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_HGRUNT_ALLY_REPEL_LAND );
		}
		else
		{
			// repel down a rope,
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_HGRUNT_ALLY_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_HGRUNT_ALLY_REPEL );
		}
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( "FG_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CTalkMonster::GetSchedule();
			}
// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_HGRUNT_ALLY_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_HGRUNT_ALLY_ESTABLISH_LINE_OF_FIRE );
						}
					}
					else
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight"
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != 0) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
									(m_hEnemy->Classify() != CLASS_HUMAN_MILITARY) &&
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "FG_ALERT", VOL_NORM, ATTN_NORM, 0, m_voicePitch);
							else if ( ( m_hEnemy != 0 ) && m_hEnemy->IsPlayer() )
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "FG_ATTACK", VOL_NORM, ATTN_NORM, 0, m_voicePitch);

							JustSpoke();
						}

						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_HGRUNT_ALLY_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_HGRUNT_ALLY_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo.
				// He's going to try to find cover to run to and reload, but rarely, if
				// none is available, he'll drop and reload in the open here.
				return GetScheduleOfType ( SCHED_HGRUNT_ALLY_COVER_AND_RELOAD );
			}

// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.

				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != 0 )
				{
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", VOL_NORM, ATTN_NORM, 0, m_voicePitch);
						m_iSentence = FGRUNT_SENT_COVER;
						//JustSpoke();
					}
					// only try to take cover if we actually have an enemy!

					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch

			else if ( FBitSet( pev->weapons, FGRUNT_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_HGRUNT_ALLY_FOUND_ENEMY );
					}
				}
				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "FG_THROW", VOL_NORM, ATTN_NORM, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					return GetScheduleOfType( SCHED_HGRUNT_ALLY_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "FG_TAUNT", VOL_NORM, ATTN_NORM, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_HGRUNT_ALLY_ESTABLISH_LINE_OF_FIRE );
			}
		}
		break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
		{
			return GetScheduleOfType ( SCHED_RELOAD );
		}
		if ( pev->health < pev->max_health/2 && ( m_flMedicWaitTime < gpGlobals->time ))
		{
			// Find a medic
			return GetScheduleOfType( SCHED_HGRUNT_ALLY_FIND_MEDIC ); // Unresolved
		}
		if ( m_hEnemy == 0 && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster :: GetSchedule();
}
MONSTERSTATE CHFGrunt :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}
void CHFGrunt::DeclineFollowing( void )
{
	PlaySentence( "FG_STOP", 2, VOL_NORM, ATTN_NORM );
}
//=========================================================
// CHFGruntRepel - when triggered, spawns a
// repelling down a line.
//=========================================================

class CHFGruntRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
	virtual int SizeForGrapple() { return GRAPPLE_LARGE; }
};

LINK_ENTITY_TO_CLASS( monster_hgrunt_ally_repel, CHFGruntRepel )

void CHFGruntRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse( &CHFGruntRepel::RepelUse );
}

void CHFGruntRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_human_grunt_ally" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CHFGruntRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);

	CBaseEntity *pEntity = Create( "monster_human_grunt_ally", pev->origin, pev->angles );
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer( );
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pGrunt->SetActivity( ACT_GLIDE );
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pGrunt->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( &CHFGruntRepel::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

	UTIL_Remove( this );
}

//=========================================================
// FGrunt Dead PROP
//=========================================================

class CDeadFGrunt : public CDeadMonster
{
public:
	void Spawn( void );
	int	DefaultClassify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );
	const char* getPos(int pos) const;

	int	m_iHead;
	static const char *m_szPoses[7];
};

const char *CDeadFGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting", "dead_on_back", "dead_headcrabbed", "hgrunt_dead_stomach", "dead_canyon" };

const char* CDeadFGrunt::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

void CDeadFGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CDeadMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_human_grunt_ally_dead, CDeadFGrunt )

//=========================================================
// ********** DeadFGrunt SPAWN **********
//=========================================================
void CDeadFGrunt :: Spawn( )
{
	SpawnHelper("models/hgrunt_opfor.mdl", "Dead fgrunt with bad pose");

	if ( pev->weapons == 0 || pev->weapons == -1 )
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_NONE );
	}
	if (FBitSet( pev->weapons, FGRUNT_SHOTGUN ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_SHOTGUN );
		SetBodygroup( FG_TORSO_GROUP, FG_TORSO_SHOTGUN );
	}
	if (FBitSet( pev->weapons, FGRUNT_9MMAR ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_MP5 );
	}
	if (FBitSet( pev->weapons, FGRUNT_M249 ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_SAW );
		SetBodygroup( FG_TORSO_GROUP, FG_TORSO_M249 );
	}

	if ( m_iHead < 0 || m_iHead >= FG_HEAD_COUNT )
		m_iHead = 0;

	SetBodygroup( FG_HEAD_GROUP, m_iHead );

	MonsterInitDead();
}

#define	TORCH_CLIP_SIZE					7

#define TORCH_EAGLE				( 1 << 0)
#define TORCH_BLOWTORCH			( 1 << 1)

// Weapon group
#define TORCH_GUN_GROUP					2
#define TORCH_GUN_EAGLE					0
#define TORCH_GUN_TORCH					1
#define TORCH_GUN_NONE					2

// Torch specific animation events
#define		TORCH_AE_SHOWGUN		( 17)
#define		TORCH_AE_SHOWTORCH		( 18)
#define		TORCH_AE_HIDETORCH		( 19)
#define		TORCH_AE_ONGAS			( 20)
#define		TORCH_AE_OFFGAS			( 21)

class CTorch : public CHFGrunt
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void GibMonster();
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void PrescheduleThink();

	void DropMyItems(BOOL isGibbed);

	void MakeGas( void );
	void UpdateGas( void );
	void KillGas( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CBeam *m_pBeam;
};

LINK_ENTITY_TO_CLASS( monster_human_torch_ally, CTorch )

TYPEDESCRIPTION	CTorch::m_SaveData[] =
{
	DEFINE_FIELD( CTorch, m_pBeam, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CTorch, CHFGrunt )

void CTorch::Spawn()
{
	Precache( );

	SpawnHelper("models/hgrunt_torch.mdl", gSkillData.torchHealth);

	if ( FBitSet( pev->weapons, TORCH_EAGLE ) )
	{
		SetBodygroup( TORCH_GUN_GROUP, TORCH_GUN_EAGLE );
	}
	if ( FBitSet( pev->weapons, TORCH_BLOWTORCH ) )
	{
		SetBodygroup( TORCH_GUN_GROUP, TORCH_GUN_TORCH );
	}
	m_cClipSize = TORCH_CLIP_SIZE;
	m_cAmmoLoaded		= m_cClipSize;
	m_pBeam = NULL;
	MonsterInit();
	SetUse( &CTorch::FollowerUse );
}

void CTorch::Precache()
{
	PrecacheMyModel("models/hgrunt_torch.mdl");
	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload.wav");
	PrecacheHelper();
	TalkInit();
	m_voicePitch = 95;
	CTalkMonster::Precache();
}

void CTorch::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch ( pEvent->event )
	{
	case TORCH_AE_SHOWTORCH:
		pev->body = 1;
		break;

	case TORCH_AE_SHOWGUN:
		if ( FBitSet( pev->weapons, TORCH_EAGLE ) )
			pev->body = 0;
		else
			pev->body = 1;
		break;

	case TORCH_AE_HIDETORCH:
		pev->body = 2;
		break;

	case TORCH_AE_ONGAS:
		MakeGas ();
		UpdateGas ();
		break;

	case TORCH_AE_OFFGAS:
		KillGas ();
		break;
	case HGRUNT_ALLY_AE_DROP_GUN:
		if ( FBitSet( pev->weapons, TORCH_EAGLE ) )
		{
			DropMyItems(FALSE);
		}
		break;
	case HGRUNT_ALLY_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_reload.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;
	case HGRUNT_ALLY_AE_BURST1:
	{
		UTIL_MakeVectors( pev->angles );
		Vector vecShootOrigin = GetGunPosition();
		Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

		Vector angDir = UTIL_VecToAngles( vecShootDir );
		SetBlending( 0, angDir.x );
		pev->effects = EF_MUZZLEFLASH;

		FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_357 );

		int pitchShift = RANDOM_LONG( 0, 20 );
		// Only shift about half the time
		if( pitchShift > 10 )
			pitchShift = 0;
		else
			pitchShift -= 5;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		m_cAmmoLoaded--;// take away a bullet!
	}
		break;
	case HGRUNT_ALLY_AE_BURST2:
	case HGRUNT_ALLY_AE_BURST3:
		break;
	case HGRUNT_ALLY_AE_KICK:
	{
		KickImpl(gSkillData.torchDmgKick);
	}
	break;
	default:
		CHFGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

BOOL CTorch::CheckRangeAttack1(float flDot, float flDist)
{
	return FBitSet( pev->weapons, TORCH_EAGLE ) && CHFGrunt::CheckRangeAttack1(flDot, flDist);
}

BOOL CTorch::CheckRangeAttack2(float flDot, float flDist)
{
	return FALSE;
}

void CTorch::GibMonster()
{
	if ( FBitSet( pev->weapons, TORCH_EAGLE ) && GetBodygroup(TORCH_GUN_GROUP) != TORCH_GUN_NONE )
	{// throw a gun if the grunt has one
		DropMyItems(TRUE);
	}
	CTalkMonster::GibMonster();
}

void CTorch::DropMyItems(BOOL isGibbed)
{
	if (!isGibbed) {
		SetBodygroup( TORCH_GUN_GROUP, TORCH_GUN_NONE );
	}
	Vector	vecGunPos;
	Vector	vecGunAngles;
	GetAttachment( 0, vecGunPos, vecGunAngles );
	DropMyItem(DESERT_EAGLE_DROP_NAME, vecGunPos, vecGunAngles, isGibbed);
}

void CTorch::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	TraceResult tr;
	// check for gas tank
	if (ptr->iHitgroup == 8)
	{
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB))
		{
			UTIL_Ricochet( ptr->vecEndPos, 1.0 );
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
				WRITE_COORD( ptr->vecEndPos.x );	// Send to PAS because of the sound
				WRITE_COORD( ptr->vecEndPos.y );
				WRITE_COORD( ptr->vecEndPos.z );
				WRITE_SHORT( g_sModelIndexFireball );
				WRITE_BYTE( 15  ); // scale * 10
				WRITE_BYTE( 15  ); // framerate
				WRITE_BYTE( TE_EXPLFLAG_NONE );
			MESSAGE_END();
			RadiusDamage ( pev, pev, 50, CLASS_NONE, DMG_BLAST );
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
		}
	}
	CHFGrunt::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CTorch::PrescheduleThink()
{
	if (m_pBeam)
	{
		UpdateGas();
	}
	CHFGrunt::PrescheduleThink();
}

//=========================================================
// AUTOGENE
//=========================================================
void CTorch::UpdateGas( void )
{
	TraceResult tr;
	Vector			posGun, angleGun;

	if ( m_pBeam )
	{
		UTIL_MakeVectors( pev->angles );

		GetAttachment( 2, posGun, angleGun );

		Vector vecEnd = (gpGlobals->v_forward * 5) + posGun;
		UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &tr );

		if ( tr.flFraction != 1.0 )
		{
			m_pBeam->DoSparks( tr.vecEndPos, posGun );
			UTIL_DecalTrace(&tr, DECAL_BIGSHOT1 + RANDOM_LONG(0,4));

				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
					WRITE_BYTE( TE_STREAK_SPLASH );
					WRITE_COORD( tr.vecEndPos.x );		// origin
					WRITE_COORD( tr.vecEndPos.y );
					WRITE_COORD( tr.vecEndPos.z );
					WRITE_COORD( tr.vecPlaneNormal.x );	// direction
					WRITE_COORD( tr.vecPlaneNormal.y );
					WRITE_COORD( tr.vecPlaneNormal.z );
					WRITE_BYTE( 10 );	// Streak color 6
					WRITE_SHORT( 60 );	// count
					WRITE_SHORT( 25 );
					WRITE_SHORT( 50 );	// Random velocity modifier
				MESSAGE_END();
		}
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( posGun.x );		// origin
			WRITE_COORD( posGun.y );
			WRITE_COORD( posGun.z );
			WRITE_BYTE( RANDOM_LONG(4, 16) );	// radius
			WRITE_BYTE( 251 );	// R
			WRITE_BYTE( 68 );	// G
			WRITE_BYTE( 36 );	// B
			WRITE_BYTE( 1 );	// life * 10
			WRITE_BYTE( 0 ); // decay
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_ELIGHT );
			WRITE_SHORT( entindex( ) + 0x1000 * 3 );		// entity, attachment
			WRITE_COORD( posGun.x );		// origin
			WRITE_COORD( posGun.y );
			WRITE_COORD( posGun.z );
			WRITE_COORD( RANDOM_LONG(8, 12) );	// radius
			WRITE_BYTE( 251 );	// R
			WRITE_BYTE( 68 );	// G
			WRITE_BYTE( 36 );	// B
			WRITE_BYTE( 1 );	// life * 10
			WRITE_COORD( 0 ); // decay
		MESSAGE_END();
	}
}

void CTorch::MakeGas( void )
{
	Vector		posGun, angleGun;
	TraceResult tr;
	Vector vecEndPos;

	UTIL_MakeVectors( pev->angles );
	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 7 );

	if ( m_pBeam )
		{
			GetAttachment( 4, posGun, angleGun );
			GetAttachment( 3, posGun, angleGun );
			UTIL_Sparks( posGun );
			Vector vecEnd = (gpGlobals->v_forward * 5) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &tr );

			m_pBeam->EntsInit( entindex(), entindex() );
			m_pBeam->SetColor( 24, 121, 239 );
			m_pBeam->SetBrightness( 190 );
			m_pBeam->SetScrollRate( 20 );
			m_pBeam->SetStartAttachment( 4 );
			m_pBeam->SetEndAttachment( 3 );
			m_pBeam->DoSparks( tr.vecEndPos, posGun );
			m_pBeam->SetFlags( BEAM_FSHADEIN );
			m_pBeam->pev->spawnflags = SF_BEAM_SPARKSTART | SF_BEAM_TEMPORARY;
			UTIL_Sparks( tr.vecEndPos );
		}
	return;
}

void CTorch :: KillGas( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	return;
}


#define MEDIC_CLIP_SIZE 17
#define MEDIC_CLIP_SIZE_EAGLE 7

#define MEDIC_EAGLE 1 << 0
#define MEDIC_HANDGUN 1 << 1
#define MEDIC_NEEDLE 1 << 2

// Weapon group
#define MEDIC_GUN_GROUP					3
#define MEDIC_GUN_EAGLE					0
#define MEDIC_GUN_PISTOL				1
#define MEDIC_GUN_NEEDLE				2
#define MEDIC_GUN_NONE					3

// Head group
#define MEDIC_HEAD_GROUP					2
enum {
	MEDIC_HEAD_WHITE,
	MEDIC_HEAD_BLACK,
	MEDIC_HEAD_COUNT
};

//=========================================================
// Medic specific animation events
//=========================================================
#define		MEDIC_AE_HIDEGUN		( 15)
#define		MEDIC_AE_SHOWNEEDLE		( 16)
#define		MEDIC_AE_HIDENEEDLE		( 17)
#define		MEDIC_AE_SHOWGUN		( 18)

enum
{
	TASK_MEDIC_SAY_HEAL = LAST_HGRUNT_ALLY_TASK + 1,
	TASK_MEDIC_HEAL,
};

Task_t	tlMedicHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,			(float)50		},	// Move within 50 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL,						(float)0		},
	{ TASK_MEDIC_SAY_HEAL,					(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_ARM	},			// Whip out the needle
	{ TASK_MEDIC_HEAL,						(float)0	},	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_DISARM	},			// Put away the needle
};

Schedule_t	slMedicHeal[] =
{
	{
		tlMedicHeal,
		ARRAYSIZE ( tlMedicHeal ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"Heal"
	},
};

Task_t	tlMedicDrawGun[] =
{
	{ TASK_PLAY_SEQUENCE,		(float)ACT_DISARM	},			// Put away the needle
};

Schedule_t	slMedicDrawGun[] =
{
	{
		tlMedicDrawGun,
		ARRAYSIZE ( tlMedicDrawGun ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"DrawGun"
	},
};

LINK_ENTITY_TO_CLASS( monster_human_medic_ally, CMedic )

TYPEDESCRIPTION	CMedic::m_SaveData[] =
{
	DEFINE_FIELD( CMedic, m_flHealCharge, FIELD_FLOAT ),
	DEFINE_FIELD( CMedic, m_fDepleteLine, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMedic, m_fHealing, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CMedic, CHFGrunt )

DEFINE_CUSTOM_SCHEDULES( CMedic )
{
	slMedicHeal,
	slMedicDrawGun,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMedic, CHFGrunt )

bool CMedic::Heal( void )
{
	if ( !HasHealCharge() || !HasHealTarget() )
		return false;

	float healAmount = Q_min(m_hTargetEnt->pev->max_health - m_hTargetEnt->pev->health, m_flHealCharge);
	if (healAmount > 10) {
		healAmount = 10;
	}

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if ( target.Length() > 100 )
		return false;

	if (m_hTargetEnt->TakeHealth( healAmount, DMG_GENERIC ))
		m_flHealCharge -= healAmount;
	ALERT(at_aiconsole, "Heal charge left: %f\n", m_flHealCharge);
	m_fHealing = TRUE;
	return true;
}

void CMedic::StartTask(Task_t *pTask)
{
	switch( pTask->iTask )
	{
	case TASK_MEDIC_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK2;
		if (Heal())
			EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "fgrunt/medic_give_shot.wav", 1, ATTN_NORM );
		break;
	case TASK_MEDIC_SAY_HEAL:
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "MG_HEAL", 2, VOL_NORM, ATTN_IDLE );
		TaskComplete();
		break;
	default:
		CHFGrunt::StartTask(pTask);
		break;
	}
}

void CMedic::RunTask(Task_t *pTask)
{
	switch ( pTask->iTask )
	{
	case TASK_MEDIC_HEAL:
		if ( m_fSequenceFinished )
		{
			if (HasHealTarget() && CheckHealCharge()) {
				m_IdealActivity = ACT_MELEE_ATTACK2;
				ALERT(at_aiconsole, "Medic continuing healing\n");
				Heal();
			} else {
				TaskComplete();
				StopHealing();
			}
		}
		else
		{
			if ( TargetDistance() > 90 ) {
				TaskComplete();
				StopHealing();
			}
			pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
			ChangeYaw( pev->yaw_speed );
		}
		break;
	default:
		CHFGrunt::RunTask(pTask);
		break;
	}
}

Schedule_t *CMedic::GetSchedule()
{
	if (m_fHealing) {
		StopHealing();
	}
	if ( FBitSet( pev->weapons, MEDIC_EAGLE|MEDIC_HANDGUN ) &&
		 GetBodygroup(MEDIC_GUN_GROUP) == MEDIC_GUN_NEEDLE || GetBodygroup(MEDIC_GUN_GROUP) == MEDIC_GUN_NONE) {
		return slMedicDrawGun;
	}
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( m_hEnemy == 0 )
		{
			if (IsFollowing()) {
				if ( TargetDistance() <= 128 )
				{
					if ( CheckHealCharge() && m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health * 0.75 ) {
						ALERT(at_aiconsole, "Medic is going to heal a player\n");
						return slMedicHeal;
					}
				}
			}
			// was called by other grunt
			else if (HasHealCharge() && HasHealTarget()) {
				return slMedicHeal;
			}
		}
	}
	return CHFGrunt::GetSchedule();
}

void CMedic::SetAnswerQuestion(CTalkMonster *pSpeaker)
{
	if (!m_fHealing) {
		CTalkMonster::SetAnswerQuestion(pSpeaker);
	}
}

void CMedic::Spawn()
{
	Precache( );

	SpawnHelper("models/hgrunt_medic.mdl", gSkillData.medicHealth);

	m_cClipSize = MEDIC_CLIP_SIZE_EAGLE;
	if ( FBitSet( pev->weapons, MEDIC_EAGLE ) )
	{
		SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_EAGLE );
	}
	else if ( FBitSet( pev->weapons, MEDIC_HANDGUN ) )
	{
		SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_PISTOL );
		m_cClipSize = MEDIC_CLIP_SIZE;
	}
	else if ( FBitSet( pev->weapons, MEDIC_NEEDLE ) )
	{
		SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_NEEDLE );
	}
	m_cAmmoLoaded		= m_cClipSize;

	if (m_iHead < 0 || m_iHead >= MEDIC_HEAD_COUNT) {
		m_iHead = RANDOM_LONG(MEDIC_HEAD_WHITE, MEDIC_HEAD_BLACK);
	}

	SetBodygroup(MEDIC_HEAD_GROUP, m_iHead);

	m_flHealCharge = gSkillData.medicHeal;
	MonsterInit();
	SetUse( &CMedic::FollowerUse );
}

void CMedic::Precache()
{
	PrecacheMyModel("models/hgrunt_medic.mdl");
	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload.wav");
	PRECACHE_SOUND("weapons/pl_gun3.wav");
	PRECACHE_SOUND("fgrunt/medic_give_shot.wav");
	PRECACHE_SOUND("fgrunt/medical.wav");
	PrecacheHelper();
	TalkInit();
	if (m_iHead == MEDIC_HEAD_BLACK)
		m_voicePitch = 95;
	else
		m_voicePitch = 105;
	CTalkMonster::Precache();
}

void CMedic::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch ( pEvent->event )
	{
	case MEDIC_AE_SHOWNEEDLE:
		SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_NEEDLE );
		break;

	case MEDIC_AE_SHOWGUN:
		if ( FBitSet( pev->weapons, MEDIC_EAGLE ) )
			SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_EAGLE );
		else if ( FBitSet( pev->weapons, MEDIC_HANDGUN ) )
			SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_PISTOL );
		else
			SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_NEEDLE );
		break;

	case MEDIC_AE_HIDENEEDLE:
		SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_NONE );
		break;
	case MEDIC_AE_HIDEGUN:
		SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_NONE );
		break;

	case HGRUNT_ALLY_AE_DROP_GUN:
		if ( FBitSet( pev->weapons, MEDIC_EAGLE | MEDIC_HANDGUN ) )
		{
			DropMyItems(FALSE);
		}
		break;
	case HGRUNT_ALLY_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_reload.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;
	case HGRUNT_ALLY_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MEDIC_EAGLE)) {
			FirePistol("weapons/desert_eagle_fire.wav", BULLET_MONSTER_357);
		} else if (FBitSet(pev->weapons, MEDIC_HANDGUN)) {
			FirePistol("weapons/pl_gun3.wav", BULLET_MONSTER_9MM);
		}
	}
		break;
	case HGRUNT_ALLY_AE_BURST2:
	case HGRUNT_ALLY_AE_BURST3:
		break;
	case HGRUNT_ALLY_AE_KICK:
	{
		KickImpl(gSkillData.medicDmgKick);
	}
	break;
	default:
		CHFGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

BOOL CMedic::CheckRangeAttack1(float flDot, float flDist)
{
	return FBitSet( pev->weapons, MEDIC_EAGLE | MEDIC_HANDGUN ) && CHFGrunt::CheckRangeAttack1(flDot, flDist);
}

BOOL CMedic::CheckRangeAttack2(float flDot, float flDist)
{
	return FALSE;
}

void CMedic::GibMonster()
{
	if ( FBitSet( pev->weapons, MEDIC_EAGLE | MEDIC_HANDGUN ) && GetBodygroup(MEDIC_GUN_GROUP) != MEDIC_GUN_NONE )
	{// throw a gun if the grunt has one
		DropMyItems(TRUE);
	}
	CTalkMonster::GibMonster();
}

void CMedic::DropMyItems(BOOL isGibbed)
{
	if (!isGibbed) {
		SetBodygroup( MEDIC_GUN_GROUP, MEDIC_GUN_NONE );
	}
	Vector	vecGunPos;
	Vector	vecGunAngles;
	GetAttachment( 0, vecGunPos, vecGunAngles );
	if (FBitSet(pev->weapons, MEDIC_EAGLE))
		DropMyItem(DESERT_EAGLE_DROP_NAME, vecGunPos, vecGunAngles, isGibbed);
	else if (FBitSet(pev->weapons, MEDIC_HANDGUN)) {
		DropMyItem("weapon_9mmhandgun", vecGunPos, vecGunAngles, isGibbed);
	}
}

void CMedic::FirePistol(const char *shotSound , Bullet bullet)
{
	UTIL_MakeVectors( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, bullet );

	int pitchShift = RANDOM_LONG( 0, 20 );
	// Only shift about half the time
	if( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, shotSound, 1, ATTN_NORM, 0, 100 + pitchShift );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	m_cAmmoLoaded--;// take away a bullet!
}

void CMedic::StartFollowingHealTarget(CBaseEntity *pTarget)
{
	if( m_pCine )
		m_pCine->CancelScript();

	if( m_hEnemy != 0 )
		m_IdealMonsterState = MONSTERSTATE_ALERT;

	m_hTargetEnt = pTarget;
	ClearConditions( bits_COND_CLIENT_PUSH );
	ClearSchedule();
	ALERT(at_aiconsole, "Medic started to follow injured grunt\n");
}

void CMedic::StopHealing()
{
	m_fHealing = FALSE;
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM );
	if (m_hTargetEnt != 0 && !m_hTargetEnt->IsPlayer()) {
		if( m_movementGoal == MOVEGOAL_TARGETENT )
			RouteClear(); // Stop him from walking toward the player
		m_hTargetEnt = 0;
		if( m_hEnemy != 0 )
			m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
}

CBaseEntity* CMedic::HealTarget()
{
	if (m_hTargetEnt != 0 && m_hTargetEnt->IsAlive() && (m_hTargetEnt->pev->health < m_hTargetEnt->pev->max_health) &&
			((m_hTargetEnt->MyMonsterPointer() && IRelationship(m_hTargetEnt) < R_DL) || m_hTargetEnt->IsPlayer())) {
		return m_hTargetEnt;
	}
	return 0;
}

bool CMedic::CheckHealCharge()
{
	if ( !HasHealCharge() )
	{
		if ( !m_fDepleteLine )
		{
			PlaySentence( "MG_NOTHEAL", 2, VOL_NORM, ATTN_IDLE );
			m_fDepleteLine = TRUE;
		}
		return false;
	}
	return true;
}

#endif
