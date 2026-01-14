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
#include	"animation.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"scripted.h"
#include	"combat.h"
#include	"ggrenade.h"
#include	"global_models.h"
#include	"soundent.h"
#include	"customentity.h"
#include	"scripted.h"
#include	"decals.h"
#include	"hgrunt.h"
#include	"game.h"
#include	"gamerules.h"
#include	"studio.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

//=========================================================
//
//=========================================================
#define	FGRUNT_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define FGRUNT_LIMP_HEALTH				20
#define FGRUNT_SENTENCE_VOLUME			0.35

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
// monster-specific tasks
//=========================================================
enum
{
	TASK_HGRUNT_ALLY_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
	TASK_HGRUNT_ALLY_SPEAK_SENTENCE,
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
	SCHED_HGRUNT_ALLY_RELOAD_NOT_EMPTY,
	LAST_HGRUNT_ALLY_SCHEDULE,
};

enum
{
	SCHED_MEDIC_HEAL = LAST_HGRUNT_ALLY_SCHEDULE,
	SCHED_MEDIC_RESTORE_TARGET,
};

enum
{
	TLK_CHECK = TLK_CGROUPS,
	TLK_CLEAR,
	TLK_GREN,
	TLK_ALERT,
	TLK_MONSTER,
	TLK_COVER,
	TLK_THROW,
	TLK_CHARGE,
	TLK_TAUNT,
	FG_TLK_CGROUPS,
};

class CHFGrunt : public CTalkMonster
{
public:
	void Spawn() override;
	int GetDefaultVoicePitch() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_ally"); }
	void SetYawSpeed() override;
	int  DefaultISoundMask() override;
	int  DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Human Grunt"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void CheckAmmo() override;
	int LookupActivity(int activity) override;
	void RunTask( Task_t *pTask ) override;
	void StartTask( Task_t *pTask ) override;
	void KeyValue( KeyValueData *pkvd ) override;
	bool FCanCheckAttacks() override;
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	bool CheckRangeAttack2( float flDot, float flDist ) override;
	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	int MaxFollowers() override { return -1; }
	int TalkFriendCategory() override { return TALK_FRIEND_SOLDIER; }
	void PlayCallForMedic() override;
	void PrescheduleThink() override;
	Vector GetGunPosition() override;
	void Shoot();
	void Shotgun();
	void M249();
	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type ) override;
	Schedule_t *GetSchedule() override;
	Schedule_t *PrioritizedSchedule();
	Schedule_t *GetReloadSchedule();

	void AlertSound() override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;

	static const NamedSoundScript callMedicSoundScript;

	static constexpr const char* reloadSoundScript = "HGruntAlly.Reload";
	static constexpr const char* burst9mmSoundScript = "HGruntAlly.9MM";
	static constexpr const char* grenadeLaunchSoundScript = "HGruntAlly.GrenadeLaunch";
	static constexpr const char* shotgunSoundScript = "HGruntAlly.Shotgun";
	static constexpr const char* m249SoundScript = "HGruntAlly.M249";

	static const NamedSoundScript m249ReloadSoundScript;

	void GibMonster() override;
	void SpeakSentence();
	const char* DefaultSentenceGroup(int group) override;

	bool FOkToSpeak();
	void JustSpoke();

	void DropMyItems(bool isGibbed);
	CBaseEntity* DropMyItem(const char *entityName, const Vector &vecGunPos, const Vector &vecGunAngles, bool isGibbed);

	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;
	int DefaultToleranceLevel() override { return TOLERANCE_HIGH; }
	int IRelationship( CBaseEntity *pTarget ) override;

	void SetHead(int head) override;

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void ReportAIState(ALERT_TYPE level) override;

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;

	bool	m_flLinkToggle;

	Vector	m_vecTossVelocity;

	bool	m_fThrowGrenade;
	bool	m_fStanding;
	bool	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int		m_iSentence;
	int		m_iHead;

	int		m_iBrassShell;
	int		m_iShotgunShell;
	int		m_iM249Shell;
	int		m_iM249Link;

	static int g_fGruntAllyQuestion;

	CUSTOM_SCHEDULES

	void PlayGruntSentence(int group);
protected:
	void PerformKick(int eventIndex, float kickDamage);
	void PrecacheCommon();
	void SpawnHelper(const char* defaultModel, float defaultHealth);
	void SpeakCaughtEnemy();

	virtual bool HasWeaponEquiped();
	bool CheckRangeAttack2Impl(float grenadeSpeed, float flDot, float flDist , bool contact = false);

	virtual bool CanFireWhileRappelling();
};

LINK_ENTITY_TO_CLASS( monster_human_grunt_ally, CHFGrunt )

int CHFGrunt::g_fGruntAllyQuestion = 0;

const NamedSoundScript CHFGrunt::painSoundScript = {
	CHAN_VOICE,
	{
		"fgrunt/gr_pain1.wav", "fgrunt/gr_pain2.wav", "fgrunt/gr_pain3.wav",
		"fgrunt/gr_pain4.wav", "fgrunt/gr_pain5.wav", "fgrunt/gr_pain6.wav",
	},
	"HGruntAlly.Pain"
};

const NamedSoundScript CHFGrunt::dieSoundScript = {
	CHAN_VOICE,
	{
		"fgrunt/death1.wav", "fgrunt/death2.wav", "fgrunt/death3.wav",
		"fgrunt/death4.wav", "fgrunt/death5.wav", "fgrunt/death6.wav",
	},
	"HGruntAlly.Die"
};

const NamedSoundScript CHFGrunt::callMedicSoundScript = {
	CHAN_VOICE,
	{"fgrunt/medic.wav"},
	"HGruntAlly.CallMedic"
};

const NamedSoundScript CHFGrunt::m249ReloadSoundScript = {
	CHAN_WEAPON,
	{"weapons/saw_reload.wav"},
	"HGruntAlly.ReloadM249"
};

class CMedic : public CHFGrunt
{
public:
	void Spawn() override;
	int GetDefaultVoicePitch() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_medic"); }
	const char* DefaultDisplayName() override { return "Human Medic"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	bool CheckRangeAttack1 ( float flDot, float flDist ) override;
	bool CheckRangeAttack2 ( float flDot, float flDist ) override;
	void GibMonster() override;
	const char* DefaultSentenceGroup(int group) override;

	int LookupActivity(int activity) override;
	void RunTask( Task_t *pTask ) override;
	void StartTask( Task_t *pTask ) override;
	Schedule_t *GetSchedule() override;
	Schedule_t *GetScheduleOfType(int Type) override;
	void OnChangeSchedule( Schedule_t *pNewSchedule ) override;
	bool CanFireWhileRappelling() override { return false; }
	CBaseEntity* FollowedPlayer() override;
	void StopFollowing( bool clearSchedule, bool saySentence = true ) override;
	void ClearFollowedPlayer() override;
	bool SetAnswerQuestion(CTalkMonster *pSpeaker) override;

	void DropMyItems(bool isGibbed);

	void FirePistol(const char* shotSoundScript, float damage);
	bool Heal();
	void StartFollowingHealTarget(CBaseEntity* pTarget) override;
	bool ReadyToHeal() override;
	void RestoreTargetEnt();
	void StopHealing(bool clearTargetEnt = true);
	CBaseEntity* HealTarget();
	inline bool HasHealTarget() { return HealTarget() != 0; }
	inline bool HasHealCharge() { return m_flHealCharge >= 1; }
	bool InHealSchedule();
	bool CheckHealCharge();
	bool ShouldDrawGun();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void ReportAIState(ALERT_TYPE level) override;

	CUSTOM_SCHEDULES
	float m_flHealCharge;
	bool m_fDepleteLine;
	bool m_fHealing;
	EHANDLE m_hLeadingPlayer;
	bool m_fSaidHeal;

	static constexpr const char* painSoundScript = "MedicGrunt.Pain";
	static constexpr const char* dieSoundScript = "MedicGrunt.Die";
	static constexpr const char* callMedicSoundScript = "MedicGrunt.CallMedic";

	static constexpr const char* handgunSoundScript = "MedicGrunt.Handgun";
	static constexpr const char* reloadSoundScript = "MedicGrunt.Reload";
	static constexpr const char* desertEagleSoundScript = "MedicGrunt.DesertEagle";
	static constexpr const char* desertEagleReloadSoundScript = "MedicGrunt.ReloadDesertEagle";

	void PainSound() override { EmitSoundScriptTalk(painSoundScript); }
	void DeathSound() override { EmitSoundScriptTalk(dieSoundScript); }
	void PlayCallForMedic() override { EmitSoundScriptTalk(callMedicSoundScript); }

	static const NamedSoundScript healSoundScript;

protected:
	bool HasWeaponEquiped() override;

	void SetBodyGroupNumbers();
	int headGroup;
	int gunGroup;
};

TYPEDESCRIPTION	CHFGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CHFGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHFGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHFGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHFGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHFGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHFGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHFGrunt, m_iHead, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHFGrunt, CTalkMonster )

//=========================================================
// KeyValue
//
// !!! netname entvar field is used in squadmonster for groupname!!!
//=========================================================
void CHFGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CTalkMonster::KeyValue( pkvd );
	}
}
//=========================================================
// someone else is talking - don't speak
//=========================================================
bool CHFGrunt::FOkToSpeak()
{
// if someone else is talking, don't speak
	if ( CTalkMonster::SomeoneIsTalking() )
		return false;

	// if in the grip of a barnacle, don't speak
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
	{
		return false;
	}

	// if not alive, certainly don't speak
	if ( pev->deadflag != DEAD_NO )
	{
		return false;
	}

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return false;
		}
	}

	return true;
}
//=========================================================
//=========================================================
void CHFGrunt::JustSpoke()
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = -1;
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

void CHFGrunt::SetHead(int head)
{
	m_iHead = head;
}

void CHFGrunt::ReportAIState(ALERT_TYPE level)
{
	CTalkMonster::ReportAIState(level);
	ALERT(level, "Ammo loaded: %d / %d. ", m_cAmmoLoaded, m_cClipSize);
	ALERT(level, "Next grenade check: %g (current time is %g). ", m_flNextGrenadeCheck, gpGlobals->time);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

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
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							1.5f					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		64.0f					},
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
		bits_COND_HEAR_SOUND |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
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
	{ TASK_HGRUNT_ALLY_SPEAK_SENTENCE,(float)0						},
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
		bits_COND_ENEMY_LOST		|
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
		bits_COND_ENEMY_LOST			|
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
	{ TASK_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
	{ TASK_FACE_ENEMY,						(float)0				},
	{ TASK_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,					(float)0				},
};

Schedule_t	slFGruntSignalSuppress[] =
{
	{
		tlFGruntSignalSuppress,
		ARRAYSIZE ( tlFGruntSignalSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_LOST		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlFGruntSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slFGruntSuppress[] =
{
	{
		tlFGruntSuppress,
		ARRAYSIZE ( tlFGruntSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_LOST		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_NOFIRE		|
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
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_HGRUNT_ALLY_SPEAK_SENTENCE,	(float)0								},
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
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_STOP_MOVING,				(float)0					},
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
		bits_COND_ENEMY_DEAD	| // stop running away if enemy is already dead
		bits_COND_ENEMY_LOST	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"FGruntHideReload"
	}
};

Task_t	tlFGruntReloadNotEmpty[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, float(ACT_RELOAD) },
};

Schedule_t slFGruntReloadNotEmpty[] =
{
	{
		tlFGruntReloadNotEmpty,
		ARRAYSIZE( tlFGruntReloadNotEmpty ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"FGruntReloadNotEmpty"
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
		bits_SOUND_PLAYER_IF_NOT_ALLY,

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
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slFGruntRangeAttack1A[] =
{
	{
		tlFGruntRangeAttack1A,
		ARRAYSIZE ( tlFGruntRangeAttack1A ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_LOST		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_NOFIRE		|
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
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slFGruntRangeAttack1B[] =
{
	{
		tlFGruntRangeAttack1B,
		ARRAYSIZE ( tlFGruntRangeAttack1B ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_LOST		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_NOFIRE		|
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
		bits_SOUND_PLAYER_IF_NOT_ALLY,
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
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Repel Land"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHFGrunt )
{
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
	slFGruntReloadNotEmpty,
};


IMPLEMENT_CUSTOM_SCHEDULES( CHFGrunt, CTalkMonster )

void CHFGrunt::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HGRUNT_ALLY_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CTalkMonster::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_HGRUNT_ALLY_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CTalkMonster::StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CHFGrunt::RunTask( Task_t *pTask )
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
				TaskComplete();
			}
			break;
		}
	default:
		{
			CTalkMonster::RunTask( pTask );
			break;
		}
	}
}
//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CHFGrunt::GibMonster()
{
	if ( GetBodygroup( FG_GUN_GROUP ) != FG_GUN_NONE )
	{// throw a gun if the grunt has one
		DropMyItems(true);
	}

	CTalkMonster::GibMonster();
}

CBaseEntity* CHFGrunt::DropMyItem(const char* entityName, const Vector& vecGunPos, const Vector& vecGunAngles, bool isGibbed)
{
	CBaseEntity* pGun = DropItem(entityName, vecGunPos, vecGunAngles);
	if (pGun && isGibbed) {
		pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
		pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
	}
	return pGun;
}

void CHFGrunt::DropMyItems(bool isGibbed)
{
	if (g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );

		FixupDropItemPosition(vecGunPos);

		if (!isGibbed) {
			SetBodygroup( FG_GUN_GROUP, FG_GUN_NONE );
		}
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
			DropMyItem( g_modFeatures.M249DropName(), vecGunPos, vecGunAngles, isGibbed );
		}

		if (FBitSet( pev->weapons, FGRUNT_GRENADELAUNCHER ))
		{
			DropMyItem( "ammo_ARgrenades", isGibbed ? vecGunPos : BodyTarget( pev->origin ), vecGunAngles, isGibbed );
		}
#if FEATURE_MONSTERS_DROP_HANDGRENADES
		if ( FBitSet (pev->weapons, FGRUNT_HANDGRENADE ) ) {
			CBaseEntity* pGrenadeEnt = DropMyItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles, isGibbed );
			if (pGrenadeEnt)
			{
				CBasePlayerWeapon* pGrenadeWeap = pGrenadeEnt->MyWeaponPointer();
				if (pGrenadeWeap)
					pGrenadeWeap->m_iDefaultAmmo = 1;
			}
		}
#endif
	}
	pev->weapons = 0;
}

void CHFGrunt::SpeakSentence()
{
	if( m_iSentence < 0 )
	{
		// no sentence cued up.
		return;
	}

	if( FOkToSpeak() )
	{
		PlayGruntSentence(m_iSentence);
	}
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CHFGrunt::DefaultISoundMask()
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
void CHFGrunt::CheckAmmo()
{
	if ( m_cClipSize > 0 && m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}
//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CHFGrunt::DefaultClassify()
{
	return CLASS_PLAYER_ALLY_MILITARY;
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHFGrunt::SetYawSpeed()
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
void CHFGrunt::PrescheduleThink()
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
				MySquadLeader()->m_fEnemyEluded = true;
			}
		}
	}
	CTalkMonster::PrescheduleThink();
}

void CHFGrunt::PlayCallForMedic()
{
	EmitSoundScriptTalk(callMedicSoundScript);
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
bool CHFGrunt::FCanCheckAttacks()
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
bool CHFGrunt::CheckMeleeAttack1 ( float flDot, float flDist )
{
	return CTalkMonster::CheckMeleeAttack1(flDot, flDist);
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
bool CHFGrunt::HasWeaponEquiped()
{
	return GetBodygroup( FG_GUN_GROUP ) != FG_GUN_NONE;
}

bool CHFGrunt::CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && HasWeaponEquiped() && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return false;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return true;
		}
	}

	return false;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
bool CHFGrunt::CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( !FBitSet(pev->weapons, (FGRUNT_HANDGRENADE | FGRUNT_GRENADELAUNCHER)) )
	{
		return false;
	}
	return CheckRangeAttack2Impl(GetSkillValue("hgrunt_ally_gspeed"), flDot, flDist, FBitSet(pev->weapons, FGRUNT_GRENADELAUNCHER));
}

bool CHFGrunt::CheckRangeAttack2Impl( float grenadeSpeed, float flDot, float flDist, bool contact )
{
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == WL_NotInWater && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = false;
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
			vecTarget += ((vecTarget - pev->origin).Length() / grenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my allies near the intended grenade impact area?
	if (AllyMonsterInRange( vecTarget, 256 ))
	{
		// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	if ( ( vecTarget - pev->origin ).IsLength2DLessThanOrEqual(256) )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}


	if ( !contact )
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = false;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, GetSkillValue("hgrunt_ally_gspeed"), 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = false;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}

	return m_fThrowGrenade;
}
//=========================================================
//=========================================================
void CHFGrunt::PerformKick(int eventIndex, float kickDamage)
{
	TraceHullAttackParams params;
	params.punchAngle.x = 15;
	params.knockForward = 100;
	params.knockUp = 50;
	params.skipAllies = true;
	params.useAimVectors = false;
	params.damageInfo.damage = kickDamage;
	params.damageInfo.type = DMG_CLUB;
	SetTraceHullAttackParamsFromTemplate(eventIndex, params);

	PerformTraceHullAttack(params);
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CHFGrunt::GetGunPosition()
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
void CHFGrunt::Shoot()
{
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_4DEGREES, 2048, GetSkillValue("9mmAR_bullet") ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CHFGrunt::Shotgun()
{
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(GetSkillValue("hgrunt_ally_pellets"), vecShootOrigin, vecShootDir, VECTOR_CONE_9DEGREES, 2048, GetSkillValue("buckshot"), 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}
//=========================================================
// Shoot
//=========================================================
void CHFGrunt::M249()
{
	EmitSoundScript(m249SoundScript);

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	m_flLinkToggle = !m_flLinkToggle;

	if (!m_flLinkToggle)
	{
		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(100,250) + gpGlobals->v_up * RANDOM_FLOAT(100,150) + gpGlobals->v_forward * 25.0f;
		EjectBrass ( vecShootOrigin - vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iM249Shell, TE_BOUNCE_SHELL);
	}
	else
	{
		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(75,200) + gpGlobals->v_up * RANDOM_FLOAT(150,200) + gpGlobals->v_forward * 25.0f;
		EjectBrass ( vecShootOrigin - vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iM249Link, TE_BOUNCE_SHELL);
	}

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, 2048, GetSkillValue("556_bullet") ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHFGrunt::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HGRUNT_ALLY_AE_DROP_GUN:
		{
			if (GetBodygroup(FG_GUN_GROUP) != FG_GUN_NONE)
				DropMyItems(false);
		}
		break;

		case HGRUNT_ALLY_AE_RELOAD:
			if (FBitSet( pev->weapons, FGRUNT_9MMAR | FGRUNT_SHOTGUN ))
			{
				EmitSoundScript(reloadSoundScript);
			}
			else if (FBitSet( pev->weapons, FGRUNT_M249 ))
			{
				EmitSoundScript(m249ReloadSoundScript);
			}
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HGRUNT_ALLY_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			//LRC - a bit of a hack. Ideally the grunts would work out in advance whether it's ok to throw.
			if (m_pCine)
			{
				Vector vecToss = g_vecZero;
				if (m_hTargetEnt != 0 && m_pCine->PreciseAttack())
				{
					vecToss = VecCheckToss( pev, GetGunPosition(), m_hTargetEnt->pev->origin, 0.5 );
				}
				if (vecToss == g_vecZero)
				{
					vecToss = (gpGlobals->v_forward*0.5+gpGlobals->v_up*0.5).Normalize()*GetSkillValue("hgrunt_ally_gspeed");
				}
				CGrenade::ShootTimed( this, GetGunPosition(), vecToss, 3.5f, GetProjectileOverrides() );
			}
			else
				CGrenade::ShootTimed( this, GetGunPosition(), m_vecTossVelocity, 3.5f, GetProjectileOverrides() );

			m_fThrowGrenade = false;
			m_flNextGrenadeCheck = gpGlobals->time + GetSkillValue("hgrunt_ally_gren_throw_delay");
		}
		break;

		case HGRUNT_ALLY_AE_GREN_LAUNCH:
		{
			EmitSoundScript(grenadeLaunchSoundScript);
			//LRC: firing due to a script?
			if (m_pCine)
			{
				Vector vecToss;
				if (m_hTargetEnt != 0 && m_pCine->PreciseAttack())
					vecToss = VecCheckThrow( pev, GetGunPosition(), m_hTargetEnt->pev->origin, GetSkillValue("hgrunt_ally_gspeed"), 0.5 );
				else
				{
					// just shoot diagonally up+forwards
					UTIL_MakeVectors(pev->angles);
					vecToss = (gpGlobals->v_forward*0.5 + gpGlobals->v_up*0.5).Normalize() * GetSkillValue("hgrunt_ally_gspeed");
				}
				CGrenade::ShootContact( this, GetGunPosition(), vecToss, GetProjectileOverrides() );
			}
			else
				CGrenade::ShootContact( this, GetGunPosition(), m_vecTossVelocity, GetProjectileOverrides() );
			m_fThrowGrenade = false;
			m_flNextGrenadeCheck = gpGlobals->time + GetSkillValue("hgrunt_ally_gren_launch_delay");
		}
		break;

		case HGRUNT_ALLY_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( this, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3.0f, GetProjectileOverrides() );
		}
		break;

		case HGRUNT_ALLY_AE_BURST1:
		{
			ReportFireAnimEvent(pEvent->event);
			if ( FBitSet( pev->weapons, FGRUNT_9MMAR ))
			{
				Shoot();
				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				EmitSoundScript(burst9mmSoundScript);
			}
			else if ( FBitSet( pev->weapons, FGRUNT_SHOTGUN ))
			{
				Shotgun();
				EmitSoundScript(shotgunSoundScript);
			}
			else if ( FBitSet( pev->weapons, FGRUNT_M249 ))
			{
				M249();
			}

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case HGRUNT_ALLY_AE_BURST2:
		case HGRUNT_ALLY_AE_BURST3:
			ReportFireAnimEvent(pEvent->event);
			if ( FBitSet( pev->weapons, FGRUNT_9MMAR ))
				Shoot();
			else if ( FBitSet( pev->weapons, FGRUNT_M249 ))
				M249();
			break;

		case HGRUNT_ALLY_AE_KICK:
		{
			PerformKick(pEvent->event, GetSkillValue("hgrunt_ally_kick"));
		}
		break;

		case HGRUNT_ALLY_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SpeakCaughtEnemy();
			}
		}
		break;

		default:
			CTalkMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
int CHFGrunt::GetDefaultVoicePitch()
{
	switch ( m_iHead ) {
	case FG_HEAD_SHOTGUN:
	case FG_HEAD_SAW_BLACK:
	case FG_HEAD_BERET_BLACK:
		return 98;
	default:
		return 100;
	}
}

void CHFGrunt::Spawn()
{
	Precache();

	SpawnHelper("models/hgrunt_opfor.mdl", GetSkillValue("hgrunt_ally_health"));

	if ( m_iHead <= -2 )
	{
		// skip major and MP heads
		m_iHead = RANDOM_LONG(0, FG_HEAD_SAW_BLACK+1);
		if (m_iHead == FG_HEAD_SAW_BLACK+1) {
			m_iHead = FG_HEAD_BERET_BLACK;
		}
	}
	else if ( m_iHead == -1 )
	{
		if (FBitSet(pev->spawnflags, SF_SQUADMONSTER_LEADER))
		{
			m_iHead = RANDOM_LONG(0,1) ? FG_HEAD_BERET : FG_HEAD_BERET_BLACK;
		}
		else if (FBitSet(pev->weapons, FGRUNT_SHOTGUN))
		{
			m_iHead = FG_HEAD_SHOTGUN;
		}
		else if (FBitSet(pev->weapons, FGRUNT_9MMAR))
		{
			m_iHead = FG_HEAD_MASK;
		}
		else if (FBitSet(pev->weapons, FGRUNT_M249))
		{
			m_iHead = RANDOM_LONG(FG_HEAD_SAW, FG_HEAD_SAW_BLACK);
		}
		else if (pev->weapons == 0)
		{
			m_iHead = FG_HEAD_MP;
		}
		else
			m_iHead = FG_HEAD_MASK;
	}

	if (FBitSet( pev->weapons, FGRUNT_9MMAR ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_MP5 );
		m_cClipSize	= FGRUNT_CLIP_SIZE;
	}
	else if (FBitSet( pev->weapons, FGRUNT_SHOTGUN ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_SHOTGUN );
		SetBodygroup( FG_TORSO_GROUP, FG_TORSO_SHOTGUN );
		m_cClipSize		= 8;
	}
	else if (FBitSet( pev->weapons, FGRUNT_M249 ))
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_SAW );
		SetBodygroup( FG_TORSO_GROUP, FG_TORSO_M249 );
		m_cClipSize	= FGRUNT_CLIP_SIZE;
	}
	else
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_NONE );
		m_cClipSize = 0;
	}

	SetBodygroup( FG_HEAD_GROUP, m_iHead );

	m_cAmmoLoaded		= m_cClipSize;

	TalkMonsterInit();
}

void CHFGrunt::SpawnHelper(const char *defaultModel, float defaultHealth)
{
	SetMyModel(defaultModel);
	SetMySize();

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( defaultHealth );
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE); // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(true);

	m_fEnemyEluded		= false;
	m_fFirstEncounter	= true;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );
	m_iSentence = -1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHFGrunt::Precache()
{
	PrecacheMyModel("models/hgrunt_opfor.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(callMedicSoundScript);

	RegisterAndPrecacheSoundScript(reloadSoundScript, NPC::reloadSoundScript);
	RegisterAndPrecacheSoundScript(burst9mmSoundScript, NPC::burst9mmSoundScript);
	RegisterAndPrecacheSoundScript(grenadeLaunchSoundScript, NPC::grenadeLaunchSoundScript);
	RegisterAndPrecacheSoundScript(shotgunSoundScript, NPC::shotgunSoundScript);
	RegisterAndPrecacheSoundScript(m249SoundScript, NPC::m249SoundScript);
	RegisterAndPrecacheSoundScript(m249ReloadSoundScript);

	PrecacheCommon();

	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell
	m_iM249Shell = PRECACHE_MODEL ("models/saw_shell.mdl");// saw shell
	m_iM249Link = PRECACHE_MODEL ("models/saw_link.mdl");// saw link

	TalkInit();

	CTalkMonster::Precache();
	RegisterTalkMonster();
}

void CHFGrunt::PrecacheCommon()
{
	RegisterAndPrecacheSoundScript(NPC::swishSoundScript);// because we use the basemonster SWIPE animation event
	UTIL_PrecacheOther("grenade", GetProjectileOverrides());

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

const char* CHFGrunt::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "FG_ANSWER";
	case TLK_QUESTION: return "FG_QUESTION";
	case TLK_IDLE: return "FG_IDLE";
	case TLK_STARE: return "FG_STARE";
	case TLK_USE: return "FG_OK";
	case TLK_UNUSE: return "FG_WAIT";
	case TLK_DECLINE: return "FG_POK";
	case TLK_STOP: return "FG_STOP";
	/* FG_SCARED in opfor has sentences more suitable for FG_HEAR.
	 * Disabling for now.
	*/
	//case TLK_NOSHOOT: return "FG_SCARED";
	case TLK_HELLO: return "FG_HELLO";

	/* Note: cure sentences are different from other talk monsters.
	 * Opfor has FG_CURE group, but not FG_CUREA, FG_CUREB and FG_CUREC sentences.
	 */
	case TLK_PLHURT1: return "FG_CURE";
	case TLK_PLHURT2: return "FG_CURE";
	case TLK_PLHURT3: return "FG_CURE";

	case TLK_PHELLO: return "FG_PHELLO";
	case TLK_PIDLE: return "FG_PIDLE";
	case TLK_PQUESTION: return "FG_PQUEST";
	case TLK_SMELL: return "FG_SMELL";
	case TLK_WOUND: return "FG_WOUND";
	case TLK_MORTAL: return "FG_MORTAL";
	case TLK_SHOT: return "FG_SHOT";
	case TLK_MAD: return "FG_MAD";
	case TLK_KILL: return "FG_KILL";
	case TLK_ATTACK: return "FG_ATTACK";
	case TLK_CHECK: return "FG_CHECK";
	case TLK_CLEAR: return "FG_CLEAR";
	case TLK_GREN: return "FG_GREN";
	case TLK_ALERT: return "FG_ALERT";
	case TLK_MONSTER: return "FG_MONSTER";
	case TLK_COVER: return "FG_COVER";
	case TLK_THROW: return "FG_THROW";
	case TLK_CHARGE: return "FG_CHARGE";
	case TLK_TAUNT: return "FG_TAUNT";
	default: return NULL;
	}
}

void CHFGrunt::PlayGruntSentence(int group)
{
	if (SENTENCEG_PlayRndSz( ENT(pev), SentenceGroup(group), FGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch) >= 0)
		JustSpoke();
}

void CHFGrunt::SpeakCaughtEnemy()
{
	if ( m_hEnemy != 0 )
	{
		if (m_hEnemy != 0)
		{
			if (m_hEnemy->IsAlienMonster())
				PlayGruntSentence(TLK_MONSTER);
			else
				PlayGruntSentence(TLK_ALERT);
		}
	}
}

void CHFGrunt::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

void CHFGrunt::AlertSound()
{
	if (m_hEnemy !=0 && FOkToSpeak())
	{
		PlaySentence(SentenceGroup(TLK_ATTACK), RandomSentenceDuraion(), VOL_NORM, ATTN_NORM);
	}
}

void CHFGrunt::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

void CHFGrunt::IdleSound()
{
	if (FOkToSpeak() && InSquad() && (g_fGruntAllyQuestion || RANDOM_LONG(0,1)))
	{
		if (g_fGruntAllyQuestion)
		{
			switch (g_fGruntAllyQuestion) {
			case 1:
				PlaySentence( SentenceGroup(TLK_CLEAR), RandomSentenceDuraion(), FGRUNT_SENTENCE_VOLUME, ATTN_IDLE );
				break;
			case 2:
				PlaySentence( SentenceGroup(TLK_ANSWER), RandomSentenceDuraion(), FGRUNT_SENTENCE_VOLUME, ATTN_IDLE );
				break;
			default:
				break;
			}
			g_fGruntAllyQuestion = 0;
		}
		else
		{
			switch (RANDOM_LONG(0,2)) {
			case 0:
				PlaySentence( SentenceGroup(TLK_CHECK), RandomSentenceDuraion(), FGRUNT_SENTENCE_VOLUME, ATTN_IDLE );
				g_fGruntAllyQuestion = 1;
				break;
			case 1:
				PlaySentence( SentenceGroup(TLK_QUESTION), RandomSentenceDuraion(), FGRUNT_SENTENCE_VOLUME, ATTN_IDLE );
				g_fGruntAllyQuestion = 2;
				break;
			case 2:
				PlaySentence( SentenceGroup(TLK_IDLE), RandomSentenceDuraion(), FGRUNT_SENTENCE_VOLUME, ATTN_IDLE );
				break;
			default:
				break;
			}
		}
		m_iSentence = -1;
	}
}

DamageInfo CHFGrunt::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	DamageInfo damageInfo = inputDamageInfo;

	// reduce damage on vest
	if (ptr->iHitgroup == HITGROUP_CHEST || ptr->iHitgroup == HITGROUP_STOMACH)
	{
		if (damageInfo.type & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ))
		{
			damageInfo.damage *= 0.5;
		}
	}
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		if (damageInfo.type & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB))
		{
			// absorb damage
			damageInfo.damage -= 20;
			if (damageInfo.damage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				damageInfo.damage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}

	return damageInfo;
}

//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
TakeDamageResult CHFGrunt::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	Forget( bits_MEMORY_INCOVER );

	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, damageInfo);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CHFGrunt::GetScheduleOfType ( int Type )
{
	switch( Type )
	{
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
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
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
				m_fStanding = RANDOM_LONG(0,1) ? true : false;

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
			const bool inSquad = InSquad();
			if ( !inSquad || (inSquad && IsLeader()) )
			{
				return &slFGruntVictoryDance[ 0 ];
			}
			Schedule_t* reloadSched = GetReloadSchedule();
			if (reloadSched)
				return reloadSched;
			return GetScheduleOfType(SCHED_IDLE_STAND);
		}
		break;
	case SCHED_HGRUNT_ALLY_SUPPRESS:
		{
			if ( m_fFirstEncounter )
			{
				m_fFirstEncounter = false;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
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
	case SCHED_HGRUNT_ALLY_RELOAD_NOT_EMPTY:
		{
			return &slFGruntReloadNotEmpty[ 0 ];
		}
	default:
		{
			return CTalkMonster::GetScheduleOfType ( Type );
		}
	}
}
//=========================================================
// SetActivity
//=========================================================
int CHFGrunt::LookupActivity(int activity)
{
	switch ( activity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, FGRUNT_SHOTGUN))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				return LookupSequence( "standing_shotgun" );
			}
			else
			{
				// get crouching shoot
				return LookupSequence( "crouching_shotgun" );
			}
		}
		else if (FBitSet( pev->weapons, FGRUNT_M249 ))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				return LookupSequence( "standing_saw" );
			}
			else
			{
				// get crouching shoot
				return LookupSequence( "crouching_saw" );
			}
		}
		else
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				return LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				return LookupSequence( "crouching_mp5" );
			}
		}
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & FGRUNT_GRENADELAUNCHER )
		{
			// get launch anim
			return LookupSequence( "launchgrenade" );
		}
		else
		{
			return LookupSequence( "throwgrenade" );
		}
	case ACT_RUN:
		if ( pev->health <= FGRUNT_LIMP_HEALTH )
		{
			int seq = CTalkMonster::LookupActivity ( ACT_RUN_HURT );
			if (seq != ACTIVITY_NOT_AVAILABLE)
				return seq;
		}
		return CTalkMonster::LookupActivity ( activity );
	case ACT_WALK:
		if ( pev->health <= FGRUNT_LIMP_HEALTH )
		{
			int seq = CTalkMonster::LookupActivity ( ACT_WALK_HURT );
			if (seq != ACTIVITY_NOT_AVAILABLE)
				return seq;
		}
		return CTalkMonster::LookupActivity ( activity );
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			return CTalkMonster::LookupActivity( ACT_IDLE_ANGRY );
		}
		// pass through
	default:
		return CTalkMonster::LookupActivity ( activity );
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* CHFGrunt::PrioritizedSchedule()
{
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
			if ( m_MonsterState == MONSTERSTATE_COMBAT && CanFireWhileRappelling() )
				return GetScheduleOfType ( SCHED_HGRUNT_ALLY_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_HGRUNT_ALLY_REPEL );
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound = PBestSound();

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
					PlayGruntSentence(TLK_GREN);
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}
	return NULL;
}

bool CHFGrunt::CanFireWhileRappelling()
{
	return FBitSet(pev->weapons, FGRUNT_9MMAR|FGRUNT_M249);
}

Schedule_t *CHFGrunt::GetReloadSchedule()
{
	if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
	{
		return GetScheduleOfType ( SCHED_RELOAD );
	}
	else if ( m_cClipSize > 0 && m_cAmmoLoaded <= m_cClipSize/2 )
	{
		return GetScheduleOfType( SCHED_HGRUNT_ALLY_RELOAD_NOT_EMPTY );
	}
	return NULL;
}

Schedule_t *CHFGrunt::GetSchedule()
{
	Schedule_t* prioritizedSchedule = PrioritizedSchedule();
	if (prioritizedSchedule)
		return prioritizedSchedule;

	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( SentenceGroup(TLK_KILL), 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CTalkMonster::GetSchedule();
			}
// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = false;

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
							SpeakCaughtEnemy();
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
					if (FOkToSpeak())
					{
						m_iSentence = TLK_COVER;
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
						MySquadLeader()->m_fEnemyEluded = false;
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
						PlayGruntSentence(TLK_THROW);
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					if( FOkToSpeak() )
					{
						m_iSentence = TLK_CHARGE;
					}
					return GetScheduleOfType( SCHED_HGRUNT_ALLY_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						PlayGruntSentence(TLK_TAUNT);
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
	case MONSTERSTATE_HUNT:
	{
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && CanIdleFlinch() )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		Schedule_t* reloadSched = GetReloadSchedule();
		if (reloadSched)
			return reloadSched;

		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;

		// try to say something about smells
		TrySmellTalk();
	}
		break;
	default:
		break;
	}

	return CTalkMonster::GetSchedule();
}

//=========================================================
// CHFGruntRepel - when triggered, spawns a
// repelling down a line.
//=========================================================

class CTalkMonsterRepel : public CHGruntRepel
{
public:
	void KeyValue(KeyValueData* pkvd) override;
	void PrepareBeforeSpawn(CBaseEntity* pEntity) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszUse;
	string_t m_iszUnUse;
	string_t m_iszDecline;
};

TYPEDESCRIPTION	CTalkMonsterRepel::m_SaveData[] =
{
	DEFINE_FIELD( CTalkMonsterRepel, m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonsterRepel, m_iszUnUse, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonsterRepel, m_iszDecline, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CTalkMonsterRepel, CHGruntRepel )

void CTalkMonsterRepel::KeyValue(KeyValueData *pkvd)
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
	else if (FStrEq( pkvd->szKeyName, "RefusalSentence" ))
	{
		m_iszDecline = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CHGruntRepel::KeyValue( pkvd );
}

void CTalkMonsterRepel::PrepareBeforeSpawn(CBaseEntity *pEntity)
{
	CBaseMonster* pMonster = pEntity->MyMonsterPointer();
	if (pMonster)
	{
		CTalkMonster* pTalkMonster = pMonster->MyTalkMonsterPointer();
		if (pTalkMonster)
		{
			pTalkMonster->m_iszUse = m_iszUse;
			pTalkMonster->m_iszUnUse = m_iszUnUse;
			pTalkMonster->m_iszDecline = m_iszDecline;
		}
	}
}

class CHFGruntRepel : public CTalkMonsterRepel
{
public:
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_ally"); }
	void KeyValue(KeyValueData* pkvd) override;
	const char* TrooperName() override {
		return "monster_human_grunt_ally";
	}
	void PrepareBeforeSpawn(CBaseEntity* pEntity) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iGruntHead;
};

LINK_ENTITY_TO_CLASS( monster_grunt_ally_repel, CHFGruntRepel )

TYPEDESCRIPTION	CHFGruntRepel::m_SaveData[] =
{
	DEFINE_FIELD( CHFGruntRepel, m_iGruntHead, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHFGruntRepel, CTalkMonsterRepel )

void CHFGruntRepel::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "head" ) )
	{
		m_iGruntHead = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CTalkMonsterRepel::KeyValue( pkvd );
}

void CHFGruntRepel::PrepareBeforeSpawn(CBaseEntity *pEntity)
{
	CHFGrunt* grunt = (CHFGrunt*)pEntity;
	grunt->m_iHead = m_iGruntHead;
	CTalkMonsterRepel::PrepareBeforeSpawn(pEntity);
}

class CMedicRepel : public CHFGruntRepel
{
public:
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_medic"); }
	const char* TrooperName() override {
		return "monster_human_medic_ally";
	}
};

LINK_ENTITY_TO_CLASS( monster_medic_ally_repel, CMedicRepel )

class CTorchRepel : public CTalkMonsterRepel
{
public:
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_torch"); }
	const char* TrooperName() override {
		return "monster_human_torch_ally";
	}
};

LINK_ENTITY_TO_CLASS( monster_torch_ally_repel, CTorchRepel )

//=========================================================
// FGrunt Dead PROP
//=========================================================

class CDeadFGrunt : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/hgrunt_opfor.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_ally"); }
	int	DefaultClassify() override { return	CLASS_PLAYER_ALLY_MILITARY; }

	void KeyValue( KeyValueData *pkvd ) override;
	const char* getPos(int pos) const override;

	int	m_iHead;
	static const char *m_szPoses[7];
};

const char *CDeadFGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting", "dead_on_back", "hgrunt_dead_stomach", "dead_headcrabed", "dead_canyon" };

const char* CDeadFGrunt::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

void CDeadFGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CDeadMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_human_grunt_ally_dead, CDeadFGrunt )

//=========================================================
// ********** DeadFGrunt SPAWN **********
//=========================================================
void CDeadFGrunt::Spawn()
{
	SpawnHelper();

	if ( pev->weapons <= 0 )
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

	if ( m_iHead <= -2 )
	{
		// skip major and MP heads
		m_iHead = RANDOM_LONG(0, FG_HEAD_SAW_BLACK+1);
		if (m_iHead == FG_HEAD_SAW_BLACK+1) {
			m_iHead = FG_HEAD_BERET_BLACK;
		}
	}
	else if ( m_iHead < 0 )
		m_iHead = 0;

	SetBodygroup( FG_HEAD_GROUP, m_iHead );

	MonsterInitDead();
}

#define	TORCH_CLIP_SIZE					7

#define TORCH_EAGLE				( 1 << 0)
#define TORCH_BLOWTORCH			( 1 << 1)
#define TORCH_HANDGRENADE		( 1 << 2)

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
	void Spawn() override;
	int GetDefaultVoicePitch() override { return 95; }
	void Precache() override;
	void Activate() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_torch"); }
	const char* DefaultDisplayName() override { return "Human Torch"; }
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	int LookupActivity(int activity) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	void GibMonster() override;
	void OnDying(bool gibbed) override;
	void UpdateOnRemove() override;
	void TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr) override;
	void PrescheduleThink() override;
	bool CanFireWhileRappelling() override { return false; }

	void DropMyItems(bool isGibbed);

	void MakeGas( bool doSpark );
	void UpdateGas();
	void KillGas();

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	CBeam *m_pBeam;
	bool m_torchActive;
	bool m_gasTankExploded;

	static constexpr const char* painSoundScript = "TorchGrunt.Pain";
	static constexpr const char* dieSoundScript = "TorchGrunt.Die";
	static constexpr const char* callMedicSoundScript = "TorchGrunt.CallMedic";

	static constexpr const char* desertEagleSoundScript = "TorchGrunt.DesertEagle";
	static constexpr const char* desertEagleReloadSoundScript = "TorchGrunt.ReloadDesertEagle";

	void PainSound() override { EmitSoundScriptTalk(painSoundScript); }
	void DeathSound() override { EmitSoundScriptTalk(dieSoundScript); }
	void PlayCallForMedic() override { EmitSoundScriptTalk(callMedicSoundScript); }

	static const NamedVisual beamVisual;
	static const NamedVisual dynLightVisual;
	static const NamedVisual entLightVisual;
protected:
	bool HasWeaponEquiped() override;
};

LINK_ENTITY_TO_CLASS( monster_human_torch_ally, CTorch )

TYPEDESCRIPTION	CTorch::m_SaveData[] =
{
	DEFINE_FIELD( CTorch, m_torchActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTorch, m_gasTankExploded, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CTorch, CHFGrunt )

const NamedVisual CTorch::beamVisual = BuildVisual("TorchGrunt.Beam")
		.Model("sprites/xbeam3.spr")
		.BeamWidth(5)
		.RenderColor(0, 0, 255)
		.Alpha(255)
		.BeamScrollRate(20)
		.BeamFlags(BEAM_FSHADEIN);

const NamedVisual CTorch::dynLightVisual = BuildVisual("TorchGrunt.DynLight")
		.RenderColor(251, 68, 36)
		.Life(0.1f)
		.Radius({40, 120});

const NamedVisual CTorch::entLightVisual = BuildVisual("TorchGrunt.EntLight")
		.RenderColor(251, 68, 36)
		.Life(0.1f)
		.Radius({8, 12});

void CTorch::Spawn()
{
	Precache();

	SpawnHelper("models/hgrunt_torch.mdl", GetSkillValue("torch_ally_health"));

	if (!pev->weapons)
		pev->weapons = TORCH_EAGLE;

	if ( FBitSet( pev->weapons, TORCH_EAGLE ) )
	{
		pev->body = TORCH_GUN_EAGLE;
	}
	if ( FBitSet( pev->weapons, TORCH_BLOWTORCH ) )
	{
		pev->body = TORCH_GUN_TORCH;
	}
	m_cClipSize = TORCH_CLIP_SIZE;
	m_cAmmoLoaded		= m_cClipSize;
	m_pBeam = NULL;
	TalkMonsterInit();
}

void CTorch::Precache()
{
	PrecacheMyModel("models/hgrunt_torch.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript, CHFGrunt::painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, CHFGrunt::dieSoundScript);
	RegisterAndPrecacheSoundScript(callMedicSoundScript, CHFGrunt::callMedicSoundScript);

	RegisterAndPrecacheSoundScript(desertEagleSoundScript, NPC::desertEagleSoundScript);
	RegisterAndPrecacheSoundScript(desertEagleReloadSoundScript, NPC::desertEagleReloadSoundScript);

	PRECACHE_SOUND("fgrunt/torch_light.wav");
	PRECACHE_SOUND("fgrunt/torch_cut_loop.wav");

	RegisterVisual(beamVisual);
	RegisterVisual(dynLightVisual);
	RegisterVisual(entLightVisual);

	PrecacheCommon();

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

void CTorch::Activate()
{
	CHFGrunt::Activate();
	if (m_torchActive && !m_pBeam) {
		MakeGas(false);
	}
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
		MakeGas(true);
		break;

	case TORCH_AE_OFFGAS:
		KillGas();
		break;
	case HGRUNT_ALLY_AE_DROP_GUN:
		if ( FBitSet( pev->weapons, TORCH_EAGLE ) && pev->body != TORCH_GUN_NONE )
		{
			DropMyItems(false);
		}
		break;
	case HGRUNT_ALLY_AE_RELOAD:
		EmitSoundScript(desertEagleReloadSoundScript);
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;
	case HGRUNT_ALLY_AE_BURST1:
	{
		ReportFireAnimEvent(pEvent->event);
		UTIL_MakeVectors( pev->angles );
		Vector vecShootOrigin = GetGunPosition();
		Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

		Vector angDir = UTIL_VecToAngles( vecShootDir );
		SetBlending( 0, angDir.x );
		pev->effects |= EF_MUZZLEFLASH;

		FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, GetSkillValue("357_bullet") );

		// Only shift about half the time
		SoundScriptParamOverride soundParams;
		if (RANDOM_LONG(0,1) == 1)
		{
			soundParams.OverridePitchShifted(RANDOM_LONG(0,15));
		}
		EmitSoundScript(desertEagleSoundScript, soundParams);
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		m_cAmmoLoaded--;// take away a bullet!
	}
		break;
	case HGRUNT_ALLY_AE_BURST2:
	case HGRUNT_ALLY_AE_BURST3:
		break;
	case HGRUNT_ALLY_AE_KICK:
	{
		PerformKick(pEvent->event, GetSkillValue("torch_ally_kick"));
	}
	break;
	default:
		CHFGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

bool CTorch::HasWeaponEquiped()
{
	return FBitSet( pev->weapons, TORCH_EAGLE );
}

bool CTorch::CheckRangeAttack1(float flDot, float flDist)
{
	return CHFGrunt::CheckRangeAttack1(flDot, flDist);
}

bool CTorch::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, TORCH_HANDGRENADE))
		return false;
	return CheckRangeAttack2Impl(GetSkillValue("torch_ally_gspeed"), flDot, flDist);
}

int CTorch::LookupActivity(int activity)
{
	switch ( activity )
	{
	case ACT_RANGE_ATTACK1:
		if ( m_fStanding )
		{
			return LookupSequence( "standing_mp5" );
		}
		else
		{
			return LookupSequence( "crouching_mp5" );
		}
	case ACT_RANGE_ATTACK2:
		return LookupSequence( "throwgrenade" );
	default:
		return CHFGrunt::LookupActivity(activity);
	}
}

void CTorch::GibMonster()
{
	if ( FBitSet( pev->weapons, TORCH_EAGLE ) && pev->body != TORCH_GUN_NONE )
	{// throw a gun if the grunt has one
		DropMyItems(true);
	}
	KillGas();
	CTalkMonster::GibMonster();
}

void CTorch::OnDying(bool gibbed)
{
	KillGas();
	CHFGrunt::OnDying(gibbed);
}

void CTorch::UpdateOnRemove()
{
	KillGas();
	CHFGrunt::UpdateOnRemove();
}

void CTorch::DropMyItems(bool isGibbed)
{
	if (g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		if (!isGibbed) {
			pev->body = TORCH_GUN_NONE;
		}
		Vector	vecGunPos;
		Vector	vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		FixupDropItemPosition(vecGunPos);
		DropMyItem(g_modFeatures.DesertEagleDropName(), vecGunPos, vecGunAngles, isGibbed);
	}
}

void CTorch::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr)
{
	DamageInfo dmgInfo = damageInfo;
	//TODO: account for custom trace attack rules?
	// check for gas tank
	if (ptr->iHitgroup == 8)
	{
		if (dmgInfo.type & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB))
		{
			if (!m_gasTankExploded && g_pGameRules->FMonsterCanTakeDamage(this, CBaseEntity::Instance(pevAttacker)))
			{
				m_gasTankExploded = true;

				dmgInfo.type = DMG_BLAST;
				dmgInfo.SetGibPolicy(GIB_ALWAYS);
				dmgInfo.damage = pev->health + 1;
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
					WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
					WRITE_VECTOR( ptr->vecEndPos );	// Send to PAS because of the sound
					WRITE_SHORT( g_sModelIndexFireball );
					WRITE_BYTE( 15  ); // scale * 10
					WRITE_BYTE( 15  ); // framerate
					WRITE_BYTE( TE_EXPLFLAG_NONE );
				MESSAGE_END();
				::RadiusDamage ( pev->origin, pev, pev, DamageInfo{Q_min(pev->max_health, 75), DMG_BLAST}, 125, CLASS_NONE );
				Create( "spark_shower", pev->origin, ptr->vecPlaneNormal, NULL );
			}
		}
	}
	CHFGrunt::TraceAttack( pevInflictor, pevAttacker, dmgInfo, vecDir, ptr );
}

void CTorch::PrescheduleThink()
{
	if (m_torchActive)
	{
		UpdateGas();
	}
	CHFGrunt::PrescheduleThink();
}

//=========================================================
// AUTOGENE
//=========================================================
void CTorch::UpdateGas()
{
	if ( m_pBeam )
	{
		Vector vecTorchPos;
		Vector vecTorchAng;
		Vector vecEndPos;
		Vector vecEndAng;

		GetAttachment(2, vecTorchPos, vecTorchAng);
		GetAttachment(3, vecEndPos, vecEndAng);

		TraceResult tr;
		UTIL_TraceLine(vecTorchPos, (vecEndPos - vecTorchPos).Normalize() * 5 + vecTorchPos, ignore_monsters, edict(), &tr);

		if ( tr.flFraction != 1.0 )
		{
			m_pBeam->pev->spawnflags &= ~SF_BEAM_SPARKSTART;
			m_pBeam->DoSparks( tr.vecEndPos, tr.vecEndPos );
			UTIL_DecalTrace(&tr, DECAL_GUNSHOT1 + RANDOM_LONG(0,4));

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
				WRITE_BYTE( TE_STREAK_SPLASH );
				WRITE_VECTOR( tr.vecEndPos );		// origin
				WRITE_VECTOR( tr.vecPlaneNormal );	// direction
				WRITE_BYTE( 10 );	// Streak color 6
				WRITE_SHORT( 30 );	// count
				WRITE_SHORT( 25 );
				WRITE_SHORT( 50 );	// Random velocity modifier
			MESSAGE_END();
		}

		const Visual* bVisual = GetVisual(beamVisual);
		if (bVisual)
			m_pBeam->SetBrightness( RANDOM_LONG(bVisual->renderamt*3/4, bVisual->renderamt) );

		SendDynLight(vecEndPos, GetVisual(dynLightVisual));
		SendEntLight(entindex(), vecEndPos, GetVisual(entLightVisual), 3);
	}
}

void CTorch::MakeGas( bool doSpark )
{
	m_torchActive = true;

	m_pBeam = CreateBeamFromVisual(GetVisual(beamVisual));
	if ( m_pBeam )
	{
		Vector posGun, angleGun;
		GetAttachment( 2, posGun, angleGun );

		m_pBeam->EntsInit( entindex(), entindex() );
		m_pBeam->SetStartAttachment( 4 );
		m_pBeam->SetEndAttachment( 3 );
		if (doSpark)
			m_pBeam->DoSparks( posGun, posGun );
		m_pBeam->pev->spawnflags = SF_BEAM_SPARKSTART | SF_BEAM_TEMPORARY;
	}
}

void CTorch::KillGas()
{
	m_torchActive = false;
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}

//=========================================================
// Torch Dead PROP
//=========================================================

class CDeadTorch : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/hgrunt_torch.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_torch"); }
	int	DefaultClassify() override { return	CLASS_PLAYER_ALLY_MILITARY; }

	const char* getPos(int pos) const override;
	static const char *m_szPoses[3];
};

const char *CDeadTorch::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

const char* CDeadTorch::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_human_torch_ally_dead, CDeadTorch )

void CDeadTorch::Spawn()
{
	SpawnHelper();

	if ( pev->weapons <= 0 )
	{
		pev->body = TORCH_GUN_NONE;
	}
	else if ( FBitSet( pev->weapons, TORCH_EAGLE ) )
	{
		pev->body = TORCH_GUN_EAGLE;
	}
	else if ( FBitSet( pev->weapons, TORCH_BLOWTORCH ) )
	{
		pev->body = TORCH_GUN_TORCH;
	}
	MonsterInitDead();
}

#define MEDIC_CLIP_SIZE 17
#define MEDIC_CLIP_SIZE_EAGLE 7

#define MEDIC_EAGLE (1 << 0)
#define MEDIC_HANDGUN (1 << 1)
#define MEDIC_NEEDLE (1 << 2)
#define MEDIC_HANDGRENADE (1 << 3)

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
	TLK_HEAL = FG_TLK_CGROUPS,
	TLK_NOTHEAL,
};

enum
{
	TASK_MEDIC_SAY_HEAL = LAST_HGRUNT_ALLY_TASK + 1,
	TASK_MEDIC_HEAL,
	TASK_MEDIC_RESTORE_TARGET_ENT,
	TASK_MEDIC_DRAW_NEEDLE,
	TASK_MEDIC_DRAW_GUN,
};

Task_t	tlMedicHeal[] =
{
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_MEDIC_RESTORE_TARGET },
	{ TASK_CATCH_WITH_TARGET_RANGE,			(float)64		},	// Move within 64 of target ent
	{ TASK_FACE_IDEAL,						(float)0		},
	{ TASK_MEDIC_SAY_HEAL,					(float)0		},
	{ TASK_MEDIC_DRAW_NEEDLE,				(float)0		},	// Whip out the needle
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_MEDIC_HEAL }, // Catch up with a target if it got too far
	{ TASK_MEDIC_HEAL,						(float)0	},	// Put it in the target
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_MEDIC_RESTORE_TARGET }, // Catch up with a target if it got too far
	{ TASK_MEDIC_DRAW_GUN,					(float)0		},	// Put away the needle
	{ TASK_MEDIC_RESTORE_TARGET_ENT,		(float)0 }
};

Schedule_t	slMedicHeal[] =
{
	{
		tlMedicHeal,
		ARRAYSIZE ( tlMedicHeal ),
		bits_COND_LIGHT_DAMAGE|
		bits_COND_HEAVY_DAMAGE|
		bits_COND_HEAR_SOUND|
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"Heal"
	},
};

Task_t	tlMedicRestoreTarget[] =
{
	{ TASK_MEDIC_RESTORE_TARGET_ENT, (float)0 },
	{ TASK_MEDIC_DRAW_GUN, (float)0 },
};

Schedule_t	slMedicRestoreTarget[] =
{
	{
		tlMedicRestoreTarget,
		ARRAYSIZE ( tlMedicRestoreTarget ),
		0,
		0,
		"Restore Target"
	},
};

Task_t	tlMedicDrawGun[] =
{
	{ TASK_MEDIC_DRAW_GUN,		(float)0	},			// Put away the needle
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
	DEFINE_FIELD( CMedic, m_hLeadingPlayer, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CMedic, CHFGrunt )

DEFINE_CUSTOM_SCHEDULES( CMedic )
{
	slMedicHeal,
	slMedicRestoreTarget,
	slMedicDrawGun,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMedic, CHFGrunt )

const NamedSoundScript CMedic::healSoundScript = {
	CHAN_WEAPON,
	{"fgrunt/medic_give_shot.wav"},
	"MedicGrunt.Heal"
};

bool CMedic::Heal()
{
	if ( !HasHealTarget() || !CheckHealCharge() )
		return false;

	if ( TargetDistance() > 100 )
		return false;

	m_flHealCharge -= m_hTargetEnt->TakeHealth( this, Q_min(10, m_flHealCharge), DMG_GENERIC );
	ALERT(at_aiconsole, "Medic grunt heal charge left: %f\n", m_flHealCharge);
	m_fHealing = true;
	return true;
}

void CMedic::StartTask(Task_t *pTask)
{
	switch( pTask->iTask )
	{
	case TASK_MEDIC_HEAL:
		if (Heal())
			EmitSoundScript(healSoundScript);
		m_IdealActivity = ACT_MELEE_ATTACK2;
		break;
	case TASK_MEDIC_SAY_HEAL:
		if (m_hTargetEnt == 0)
			TaskFail("no target ent");
		else if (!m_hTargetEnt->IsFullyAlive())
			TaskFail("target ent is dead");
		else
		{
			if (!m_fSaidHeal && !InScriptedSentence())
			{
				m_hTalkTarget = m_hTargetEnt;
				PlaySentence( SentenceGroup(TLK_HEAL), 2, VOL_NORM, ATTN_IDLE );
				m_fSaidHeal = true;
			}
			TaskComplete();
		}
		break;
	case TASK_MEDIC_RESTORE_TARGET_ENT:
		TaskComplete();
		RestoreTargetEnt();
		break;
	case TASK_MEDIC_DRAW_NEEDLE:
		if (GetBodygroup(gunGroup) == MEDIC_GUN_NEEDLE)
		{
			TaskComplete();
		}
		else
		{
			m_IdealActivity = ACT_ARM;
		}
		break;
	case TASK_MEDIC_DRAW_GUN:
		if (ShouldDrawGun())
		{
			m_IdealActivity = ACT_DISARM;
		}
		else
		{
			TaskComplete();
		}
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
			Heal();
			if (HasHealTarget() && HasHealCharge())
			{
				m_IdealActivity = ACT_MELEE_ATTACK2;
				ALERT(at_aiconsole, "Medic continuing healing\n");
			}
			else
			{
				TaskComplete();
				StopHealing();
			}
		}
		else
		{
			if ( TargetDistance() > 90 ) {
				TaskFail("target ent is too far");
				StopHealing(false);
			}
			if (m_hTargetEnt != 0)
			{
				pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
				ChangeYaw( pev->yaw_speed );
			}
		}
		break;
	case TASK_MEDIC_DRAW_NEEDLE:
	case TASK_MEDIC_DRAW_GUN:
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
		CHFGrunt::RunTask(pTask);
		break;
	}
}

Schedule_t *CMedic::GetSchedule()
{
	Schedule_t* prioritizedSchedule = PrioritizedSchedule();
	if (prioritizedSchedule)
		return prioritizedSchedule;

	if ( ShouldDrawGun() ) {
		return slMedicDrawGun;
	}
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
		if ( m_hEnemy == 0 || !m_hEnemy->IsAlive() )
		{
			if (m_hTargetEnt != 0 && FollowedPlayer() == m_hTargetEnt)
			{
				m_fSaidHeal = false;
				if ( TargetDistance() <= 128 )
				{
					if ( m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health * 0.75 && CheckHealCharge() ) {
						ALERT(at_aiconsole, "Medic is going to heal a player\n");
						return GetScheduleOfType(SCHED_MEDIC_HEAL);
					}
				}
			}
			// was called by other ally
			else if (HasHealCharge() && HasHealTarget()) {
				return GetScheduleOfType(SCHED_MEDIC_HEAL);
			}
		}
	default:
		break;
	}
	return CHFGrunt::GetSchedule();
}

Schedule_t *CMedic::GetScheduleOfType(int Type)
{
	switch (Type) {
	case SCHED_MEDIC_HEAL:
		if (HasHealTarget())
			return slMedicHeal;
		else
			return slMedicRestoreTarget;
	case SCHED_MEDIC_RESTORE_TARGET:
		return slMedicRestoreTarget;
	case SCHED_FAIL:
		if (ShouldDrawGun())
		{
			return slMedicDrawGun;
		}
		else
		{
			return CHFGrunt::GetScheduleOfType(Type);
		}
	default:
		return CHFGrunt::GetScheduleOfType(Type);
	}
}
void CMedic::OnChangeSchedule( Schedule_t *pNewSchedule )
{
	if (m_fHealing) {
		StopHealing();
	}
	if (m_hLeadingPlayer != 0 && (m_hTargetEnt == 0 || !m_hTargetEnt->IsFullyAlive()))
	{
		// Restore target ent if medic follows a player but did not play RestoreTargetEnt task, e.g. due to save-load.
		m_hTargetEnt = m_hLeadingPlayer;
		m_hLeadingPlayer = 0;
	}
	CHFGrunt::OnChangeSchedule( pNewSchedule );
}

int CMedic::LookupActivity(int activity)
{
	switch ( activity )
	{
	case ACT_RANGE_ATTACK1:
		if ( m_fStanding )
		{
			return LookupSequence( "standing_mp5" );
		}
		else
		{
			return LookupSequence( "crouching_mp5" );
		}
	case ACT_RANGE_ATTACK2:
		return LookupSequence( "throwgrenade" );
	default:
		return CHFGrunt::LookupActivity(activity);
	}
}

CBaseEntity* CMedic::FollowedPlayer()
{
	if (m_hLeadingPlayer != 0 && m_hLeadingPlayer->IsPlayer())
		return m_hLeadingPlayer;
	return CHFGrunt::FollowedPlayer();
}

void CMedic::StopFollowing(bool clearSchedule, bool saySentence )
{
	if (InHealSchedule() && (m_hTargetEnt != 0 && !m_hTargetEnt->IsPlayer()))
		clearSchedule = false;
	CHFGrunt::StopFollowing(clearSchedule, saySentence);
}

void CMedic::ClearFollowedPlayer()
{
	if (m_hLeadingPlayer)
		m_hLeadingPlayer = 0;
	else
		CHFGrunt::ClearFollowedPlayer();
}

bool CMedic::SetAnswerQuestion(CTalkMonster *pSpeaker)
{
	if (InHealSchedule()) {
		return false;
	}
	return CTalkMonster::SetAnswerQuestion(pSpeaker);
}

int CMedic::GetDefaultVoicePitch()
{
	if (m_iHead == MEDIC_HEAD_BLACK)
		return 100;
	else
		return 105;
}

void CMedic::Spawn()
{
	Precache();

	SpawnHelper("models/hgrunt_medic.mdl", GetSkillValue("medic_ally_health"));
	SetBodyGroupNumbers();

	if (!pev->weapons)
	{
		pev->weapons |= MEDIC_EAGLE; // some medics on existing maps don't have a weapon selected
	}
	m_cClipSize = MEDIC_CLIP_SIZE_EAGLE;
	if ( FBitSet( pev->weapons, MEDIC_EAGLE ) )
	{
		SetBodygroup( gunGroup, MEDIC_GUN_EAGLE );
	}
	else if ( FBitSet( pev->weapons, MEDIC_HANDGUN ) )
	{
		SetBodygroup( gunGroup, MEDIC_GUN_PISTOL );
		m_cClipSize = MEDIC_CLIP_SIZE;
	}
	else if ( FBitSet( pev->weapons, MEDIC_NEEDLE ) )
	{
		SetBodygroup( gunGroup, MEDIC_GUN_NEEDLE );
	}
	m_cAmmoLoaded		= m_cClipSize;

	if (m_iHead < 0 || m_iHead >= MEDIC_HEAD_COUNT) {
		m_iHead = RANDOM_LONG(MEDIC_HEAD_WHITE, MEDIC_HEAD_BLACK);
	}

	SetBodygroup(headGroup, m_iHead);

	m_flHealCharge = GetSkillValue("medic_ally_heal");
	TalkMonsterInit();
}

void CMedic::Precache()
{
	PrecacheMyModel("models/hgrunt_medic.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript, CHFGrunt::painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, CHFGrunt::dieSoundScript);
	RegisterAndPrecacheSoundScript(callMedicSoundScript, CHFGrunt::callMedicSoundScript);

	RegisterAndPrecacheSoundScript(handgunSoundScript, NPC::handgunSoundScript);
	RegisterAndPrecacheSoundScript(reloadSoundScript, NPC::reloadSoundScript);
	RegisterAndPrecacheSoundScript(desertEagleSoundScript, NPC::desertEagleSoundScript);
	RegisterAndPrecacheSoundScript(desertEagleReloadSoundScript, NPC::desertEagleReloadSoundScript);
	RegisterAndPrecacheSoundScript(healSoundScript);

	PRECACHE_SOUND("fgrunt/medical.wav");
	PrecacheCommon();

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
	RegisterMedic();

	if (pev->modelindex)
		SetBodyGroupNumbers();
}

const char* CMedic::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_HEAL: return "MG_HEAL";
	case TLK_NOTHEAL: return "MG_NOTHEAL";
	default: return CHFGrunt::DefaultSentenceGroup(group);
	}
}

void CMedic::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch ( pEvent->event )
	{
	case MEDIC_AE_SHOWNEEDLE:
		SetBodygroup( gunGroup, MEDIC_GUN_NEEDLE );
		break;

	case MEDIC_AE_SHOWGUN:
		if ( FBitSet( pev->weapons, MEDIC_EAGLE ) )
			SetBodygroup( gunGroup, MEDIC_GUN_EAGLE );
		else if ( FBitSet( pev->weapons, MEDIC_HANDGUN ) )
			SetBodygroup( gunGroup, MEDIC_GUN_PISTOL );
		else
			SetBodygroup( gunGroup, MEDIC_GUN_NEEDLE );
		break;

	case MEDIC_AE_HIDENEEDLE:
		SetBodygroup( gunGroup, MEDIC_GUN_NONE );
		break;
	case MEDIC_AE_HIDEGUN:
		SetBodygroup( gunGroup, MEDIC_GUN_NONE );
		break;

	case HGRUNT_ALLY_AE_DROP_GUN:
		if ( FBitSet( pev->weapons, MEDIC_EAGLE | MEDIC_HANDGUN ) && GetBodygroup(gunGroup) != MEDIC_GUN_NONE )
		{
			DropMyItems(false);
		}
		break;
	case HGRUNT_ALLY_AE_RELOAD:
		if ( FBitSet( pev->weapons, MEDIC_EAGLE ) )
			EmitSoundScript(desertEagleReloadSoundScript);
		else
			EmitSoundScript(reloadSoundScript);
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;
	case HGRUNT_ALLY_AE_BURST1:
	{
		ReportFireAnimEvent(pEvent->event);
		if (FBitSet(pev->weapons, MEDIC_EAGLE)) {
			FirePistol(desertEagleSoundScript, GetSkillValue("357_bullet"));
		} else if (FBitSet(pev->weapons, MEDIC_HANDGUN)) {
			FirePistol(handgunSoundScript, GetSkillValue("9mm_bullet"));
		}
	}
		break;
	case HGRUNT_ALLY_AE_BURST2:
	case HGRUNT_ALLY_AE_BURST3:
		break;
	case HGRUNT_ALLY_AE_KICK:
	{
		PerformKick(pEvent->event, GetSkillValue("medic_ally_kick"));
	}
	break;
	default:
		CHFGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

bool CMedic::HasWeaponEquiped()
{
	return FBitSet( pev->weapons, MEDIC_EAGLE | MEDIC_HANDGUN );
}

bool CMedic::CheckRangeAttack1(float flDot, float flDist)
{
	return CHFGrunt::CheckRangeAttack1(flDot, flDist);
}

bool CMedic::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, MEDIC_HANDGRENADE))
		return false;
	return CheckRangeAttack2Impl(GetSkillValue("medic_ally_gspeed"), flDot, flDist);
}

void CMedic::GibMonster()
{
	if ( FBitSet( pev->weapons, MEDIC_EAGLE | MEDIC_HANDGUN ) && GetBodygroup(gunGroup) != MEDIC_GUN_NONE )
	{// throw a gun if the grunt has one
		DropMyItems(true);
	}
	CTalkMonster::GibMonster();
}

void CMedic::DropMyItems(bool isGibbed)
{
	if (g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		if (!isGibbed) {
			SetBodygroup( gunGroup, MEDIC_GUN_NONE );
		}
		Vector	vecGunPos;
		Vector	vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		FixupDropItemPosition(vecGunPos);
		if (FBitSet(pev->weapons, MEDIC_EAGLE))
			DropMyItem(g_modFeatures.DesertEagleDropName(), vecGunPos, vecGunAngles, isGibbed);
		else if (FBitSet(pev->weapons, MEDIC_HANDGUN)) {
			DropMyItem("weapon_9mmhandgun", vecGunPos, vecGunAngles, isGibbed);
		}
		if (g_modFeatures.medic_drop_healthkit && m_flHealCharge >= GetSkillValue("healthkit"))
			DropMyItem("item_healthkit", BodyTarget( pev->origin ), vecGunAngles, isGibbed);
	}
}

void CMedic::FirePistol(const char *shotSoundScript, float damage)
{
	UTIL_MakeVectors( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, damage );

	// Only shift about half the time
	SoundScriptParamOverride soundParams;
	if (RANDOM_LONG(0,1) == 1)
	{
		soundParams.OverridePitchShifted(RANDOM_LONG(0,15));
	}
	EmitSoundScript(shotSoundScript, soundParams);
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );

	m_cAmmoLoaded--;// take away a bullet!
}

void CMedic::StartFollowingHealTarget(CBaseEntity *pTarget)
{
	if (m_hTargetEnt != 0 && m_hTargetEnt->IsPlayer())
		m_hLeadingPlayer = m_hTargetEnt;

	m_fSaidHeal = false;

	StopScript();

	m_hTargetEnt = pTarget;
	ClearConditions( bits_COND_CLIENT_PUSH );
	ClearSchedule();
	//ChangeSchedule(GetScheduleOfType(SCHED_MEDIC_HEAL));
	ALERT(at_aiconsole, "Medic started to follow injured %s\n", STRING(pTarget->pev->classname));
}

void CMedic::RestoreTargetEnt()
{
	m_fSaidHeal = false;
	if (m_hLeadingPlayer != 0)
	{
		ALERT(at_aiconsole, "Medic restoring old target\n");
		m_hTargetEnt = m_hLeadingPlayer;
		m_hLeadingPlayer = 0;

		ClearConditions( bits_COND_CLIENT_PUSH );
	}
}

void CMedic::StopHealing(bool clearTargetEnt)
{
	m_fHealing = false;

	const SoundScript* soundScript = GetSoundScript(healSoundScript);
	if (soundScript)
	{
		EmitSound(soundScript->channel, "common/null.wav", 1.0f, ATTN_NORM);
	}

	if (m_hTargetEnt != 0 && !m_hTargetEnt->IsPlayer()) {
		if(m_movementGoal & MOVEGOAL_TARGETENT)
			RouteClear(); // Stop him from walking toward the target
		if (clearTargetEnt)
		{
			m_hTargetEnt = 0;
			if( m_hEnemy != 0 )
				m_IdealMonsterState = MONSTERSTATE_COMBAT;
		}
	}
}

CBaseEntity* CMedic::HealTarget()
{
	CBaseEntity* pTargetEnt = m_hTargetEnt;
	if (pTargetEnt != 0 && pTargetEnt->IsFullyAlive() && (pTargetEnt->pev->health < pTargetEnt->pev->max_health) &&
			pTargetEnt->MyMonsterPointer() && IRelationship(pTargetEnt) < R_DL) {
		return pTargetEnt;
	}
	return 0;
}

bool CMedic::CheckHealCharge()
{
	if ( !HasHealCharge() )
	{
		if ( !m_fDepleteLine && !IsTalking() )
		{
			m_hTalkTarget = m_hTargetEnt;
			PlaySentence( SentenceGroup(TLK_NOTHEAL), 2, VOL_NORM, ATTN_IDLE );
			m_fDepleteLine = true;
		}
		return false;
	}
	return true;
}

bool CMedic::ShouldDrawGun()
{
	return FBitSet( pev->weapons, MEDIC_EAGLE|MEDIC_HANDGUN ) &&
		 (GetBodygroup(gunGroup) == MEDIC_GUN_NEEDLE || GetBodygroup(gunGroup) == MEDIC_GUN_NONE);
}

bool CMedic::ReadyToHeal()
{
	return AbleToFollow() && HasHealCharge() && !InHealSchedule();
}

bool CMedic::InHealSchedule()
{
	return m_pSchedule == slMedicHeal;
}

void CMedic::ReportAIState(ALERT_TYPE level)
{
	CHFGrunt::ReportAIState(level);
	ALERT(level, "Heal charge: %3.1f. ", m_flHealCharge);
}

static void SetMedicBodyGroupNumbers(entvars_t *pev, int& headGroup, int& gunGroup)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	if (pmodel)
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		if (pstudiohdr->numbodyparts == 3)
		{
			headGroup = 1;
			gunGroup = 2;
		}
		else if (pstudiohdr->numbodyparts == 2)
		{
			headGroup = 0;
			gunGroup = 1;
		}
		else
		{
			headGroup = MEDIC_HEAD_GROUP;
			gunGroup = MEDIC_GUN_GROUP;
		}
	}
}

void CMedic::SetBodyGroupNumbers()
{
	SetMedicBodyGroupNumbers(pev, headGroup, gunGroup);
}

//=========================================================
// Medic Dead PROP
//=========================================================

class CDeadMedic : public CDeadFGrunt
{
public:
	void Spawn() override;
	void Precache() override;
	const char* DefaultModel() override { return "models/hgrunt_medic.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("human_grunt_medic"); }
	const char* getPos(int pos) const override;
	static const char *m_szPoses[3];

	void SetBodyGroupNumbers() {
		SetMedicBodyGroupNumbers(pev, headGroup, gunGroup);
	}
	int gunGroup;
	int headGroup;
};

const char *CDeadMedic::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

const char* CDeadMedic::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_human_medic_ally_dead, CDeadMedic )

void CDeadMedic::Spawn()
{
	SpawnHelper();
	SetBodyGroupNumbers();

	if ( pev->weapons <= 0 )
	{
		SetBodygroup( FG_GUN_GROUP, FG_GUN_NONE );
	}
	if ( FBitSet( pev->weapons, MEDIC_EAGLE ) )
	{
		SetBodygroup( gunGroup, MEDIC_GUN_EAGLE );
	}
	else if ( FBitSet( pev->weapons, MEDIC_HANDGUN ) )
	{
		SetBodygroup( gunGroup, MEDIC_GUN_PISTOL );
	}
	else if ( FBitSet( pev->weapons, MEDIC_NEEDLE ) )
	{
		SetBodygroup( gunGroup, MEDIC_GUN_NEEDLE );
	}

	if (m_iHead < 0 || m_iHead >= MEDIC_HEAD_COUNT) {
		m_iHead = RANDOM_LONG(MEDIC_HEAD_WHITE, MEDIC_HEAD_BLACK);
	}

	SetBodygroup(headGroup, m_iHead);

	MonsterInitDead();
}

void CDeadMedic::Precache()
{
	CDeadFGrunt::Precache();
	SetBodyGroupNumbers();
}
