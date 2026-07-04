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
// hassassin - Human assassin, fast and stealthy
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"followingmonster.h"
#include	"combat.h"
#include	"ggrenade.h"
#include	"soundent.h"
#include	"scripted.h"
#include	"game.h"
#include	"gamerules.h"
#include	"graphic_debug.h"

#define FEATURE_HASSSASSIN_DROP_AMMO 0

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ASSASSIN_EXPOSED = LAST_FOLLOWINGMONSTER_SCHEDULE + 1,// cover was blown.
	SCHED_ASSASSIN_JUMP,	// fly through the air
	SCHED_ASSASSIN_JUMP_ATTACK,	// fly through the air and shoot
	SCHED_ASSASSIN_JUMP_LAND, // hit and run away
	SCHED_ASSASSIN_JUMP_FOLLOWING,
};

//=========================================================
// monster-specific tasks
//=========================================================

enum
{
	TASK_ASSASSIN_FALL_TO_GROUND = LAST_FOLLOWINGMONSTER_TASK + 1, // falling and waiting to hit ground
	TASK_ASSASSIN_FALL_TO_GROUND_FOLLOWING
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ASSASSIN_AE_SHOOT1	1
#define		ASSASSIN_AE_TOSS1	2
#define		ASSASSIN_AE_JUMP	3

#define bits_MEMORY_BADJUMP		( bits_MEMORY_CUSTOM1 )

#define HASSASSIN_JUMP_HEIGHT 160.0f

struct LaunchToHeightResult
{
	float height;
	float gravity;
	float time;
	float speed;
};

LaunchToHeightResult CalcLaunchToHeight(float height)
{
	LaunchToHeightResult result;
	result.height = height;
	result.gravity = g_psv_gravity->value;
	if (result.gravity <= 1)
		result.gravity = 1;
	result.time = sqrt(height * 2.0f / result.gravity);
	result.speed = result.gravity * result.time;
	return result;
}

class CHAssassin : public CFollowingMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Female Assassin"; }
	const char* ReverseRelationshipModel() override { return "models/hassassinf.mdl"; }
	int DefaultISoundMask() override;
	void Shoot();
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	Schedule_t *GetSchedule() override;
	Schedule_t *GetScheduleOfType( int Type ) override;
	bool CheckMeleeAttack1( float flDot, float flDist ) override;	// jump
	// BOOL CheckMeleeAttack2( float flDot, float flDist );
	bool CheckRangeAttack1( float flDot, float flDist ) override;	// shoot
	bool CheckRangeAttack2( float flDot, float flDist ) override;	// throw grenade
	void StartTask( Task_t *pTask ) override;
	void RunAI() override;
	void RunTask( Task_t *pTask ) override;
	void PlayUseSentence() override;
	void PlayUnUseSentence() override;
	void DeathSound() override;
	void IdleSound() override;
	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;
	void OnDying(bool gibbed) override;
	CUSTOM_SCHEDULES

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int DefaultSizeForGrapple() override { return GRAPPLE_MEDIUM; }
	Vector DefaultMinHullSize() override { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() override { return VEC_HUMAN_HULL_MAX; }

	bool CanCloakByDefault() override {
		return GetSkillValueRange("hassassin_cloaking").min != 0;
	}
	float JumpSpeedFactor(float jumpHeight = HASSASSIN_JUMP_HEIGHT) const
	{
		auto result = CalcLaunchToHeight(jumpHeight);
		return result.speed / jumpHeight;
	}
	std::pair<Vector, int> CalcMonsterJump(const Vector& vecTarget, float maximumHeight, float maximumDistance);
	bool FindFollowJump();

	float m_flLastShot;
	float m_flDiviation;

	float m_flNextJump;
	Vector m_vecJumpVelocity;

	float m_flNextGrenadeCheck;
	Vector	m_vecTossVelocity;
	bool m_fThrowGrenade;

	int m_iFrustration;
	float m_nextWalkFootstep;

	int m_iShell;

	static const NamedSoundScript shotSoundScript;
	static const NamedSoundScript cloakSoundScript;
	static const NamedSoundScript footstepSoundScript;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript useSoundScript;
	static const NamedSoundScript unuseSoundScript;
};

LINK_ENTITY_TO_CLASS( monster_human_assassin, CHAssassin )

TYPEDESCRIPTION	CHAssassin::m_SaveData[] =
{
	DEFINE_FIELD( CHAssassin, m_flLastShot, FIELD_TIME ),
	DEFINE_FIELD( CHAssassin, m_flDiviation, FIELD_FLOAT ),

	DEFINE_FIELD( CHAssassin, m_flNextJump, FIELD_TIME ),
	DEFINE_FIELD( CHAssassin, m_vecJumpVelocity, FIELD_VECTOR ),

	DEFINE_FIELD( CHAssassin, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHAssassin, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHAssassin, m_fThrowGrenade, FIELD_BOOLEAN ),

	DEFINE_FIELD( CHAssassin, m_iFrustration, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHAssassin, CFollowingMonster )

const NamedSoundScript CHAssassin::shotSoundScript = {
	CHAN_WEAPON,
	{"weapons/pl_gun1.wav", "weapons/pl_gun2.wav"},
	FloatRange(0.6f, 0.8f),
	ATTN_NORM,
	"HAssassin.Shot"
};

const NamedSoundScript CHAssassin::footstepSoundScript = {
	CHAN_BODY,
	{"player/pl_step1.wav", "player/pl_step2.wav", "player/pl_step3.wav", "player/pl_step4.wav"},
	0.5f,
	ATTN_NORM,
	"HAssassin.Footstep"
};

const NamedSoundScript CHAssassin::cloakSoundScript = {
	CHAN_BODY,
	{"debris/beamstart1.wav"},
	0.2f,
	ATTN_NORM,
	"HAssassin.Cloak"
};

const NamedSoundScript CHAssassin::painSoundScript = {
	CHAN_VOICE,
	{},
	"HAssassin.Pain"
};

const NamedSoundScript CHAssassin::dieSoundScript = {
	CHAN_VOICE,
	{},
	"HAssassin.Die"
};

const NamedSoundScript CHAssassin::useSoundScript = {
	CHAN_VOICE,
	{"HA_OK"},
	"HAssassin.Use"
};

const NamedSoundScript CHAssassin::unuseSoundScript = {
	CHAN_VOICE,
	{"HA_WAIT"},
	"HAssassin.UnUse"
};

void CHAssassin::PlayUseSentence()
{
	EmitSoundScript(useSoundScript);
	//SENTENCEG_PlayRndSz( ENT( pev ), "HA_OK", 0.6, ATTN_NORM, 0, 90 );
}

void CHAssassin::PlayUnUseSentence()
{
	EmitSoundScript(unuseSoundScript);
	//SENTENCEG_PlayRndSz( ENT( pev ), "HA_WAIT", 0.6, ATTN_NORM, 0, 90 );
}

//=========================================================
// DieSound
//=========================================================
void CHAssassin::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

//=========================================================
// IdleSound
//=========================================================
void CHAssassin::IdleSound()
{
}

PainSoundRule CHAssassin::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = 1.0f;
	return rule;
}

void CHAssassin::PainSound()
{
	EmitSoundScript(painSoundScript);
}

void CHAssassin::OnDying(bool gibbed)
{
#if FEATURE_HASSSASSIN_DROP_AMMO || FEATURE_MONSTERS_DROP_HANDGRENADES
	if( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) )
	{
		// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		GetAttachment( 0, vecGunPos, vecGunAngles );
#if FEATURE_HASSSASSIN_DROP_AMMO
		DropItem( "ammo_9mmclip", vecGunPos, vecGunAngles );
#endif
#if FEATURE_MONSTERS_DROP_HANDGRENADES
		CBaseEntity* pGrenadeEnt = DropItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles );
		if (pGrenadeEnt)
		{
			CBasePlayerWeapon* pGrenadeWeap = pGrenadeEnt->MyWeaponPointer();
			if (pGrenadeWeap)
				pGrenadeWeap->m_iDefaultAmmo = 1;
		}
#endif
	}
#endif
	CFollowingMonster::OnDying(gibbed);
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CHAssassin::DefaultISoundMask() 
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHAssassin::DefaultClassify()
{
	if (g_modFeatures.blackops_classify)
		return CLASS_HUMAN_BLACKOPS;
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHAssassin::SetYawSpeed()
{
	int ys;

	switch( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 360;
		break;
	default:	
		ys = 360;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Shoot
//=========================================================
void CHAssassin::Shoot()
{
	if( m_hEnemy == 0 && !m_pCine )
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if( m_flLastShot + 2.0f < gpGlobals->time )
	{
		m_flDiviation = 0.10f;
	}
	else
	{
		m_flDiviation -= 0.01f;
		if( m_flDiviation < 0.02f )
			m_flDiviation = 0.02f;
	}
	m_flLastShot = gpGlobals->time;

	UTIL_MakeVectors( pev->angles );

	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	EjectBrass( pev->origin + gpGlobals->v_up * 32 + gpGlobals->v_forward * 12, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL );
	FireBullets( 1, vecShootOrigin, vecShootDir, Vector( m_flDiviation, m_flDiviation, m_flDiviation ), 2048, GetSkillValue("9mm_bullet") ); // shoot +-8 degrees

	EmitSoundScript(shotSoundScript);

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

	if (m_cClipSize > 0)
		m_cAmmoLoaded--;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CHAssassin::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ASSASSIN_AE_SHOOT1:
		ReportFireAnimEvent(pEvent->event);
		Shoot();
		break;
	case ASSASSIN_AE_TOSS1:
		{
			UTIL_MakeVectors( pev->angles );
			Vector vecGunPosition = pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32);
			//LRC
			if (m_pCine && m_pCine->IsAction())
			{
				Vector vecToss;
				if (m_pCine->PreciseAttack() && m_hTargetEnt != 0)
				{
					vecToss = VecCheckToss( pev, vecGunPosition, m_hTargetEnt->pev->origin, 0.5f, 0.0f );
					//if (vecToss != g_vecZero)
					//	ALERT(at_console,"Assassin %s throws precise grenade\n",STRING(pev->targetname));
				}
				else
				{
					//ALERT(at_console,"Assassin %s throws nonprecise grenade\n",STRING(pev->targetname));
					// what speed would be best to use, here? Borrowing the hgrunt grenade speed seems silly...
					vecToss = ((gpGlobals->v_forward*0.5)+(gpGlobals->v_up*0.5)).Normalize()*GetSkillValue("hgrunt_gspeed");
				}
				CGrenade::ShootTimed( this, vecGunPosition, vecToss, 2.0f, GetProjectileOverrides() );
			}
			else
				CGrenade::ShootTimed( this, vecGunPosition, m_vecTossVelocity, 2.0f, GetProjectileOverrides() );

			m_flNextGrenadeCheck = gpGlobals->time + 6.0f;// wait six seconds before even looking again to see if a grenade can be thrown.
			m_fThrowGrenade = false;
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;
	case ASSASSIN_AE_JUMP:
		{
			// ALERT( at_console, "jumping");
			UTIL_MakeAimVectors( pev->angles );
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			if (m_pCine) //LRC...
			{
				pev->velocity = g_vecZero;
				if (m_pCine->PreciseAttack() && m_hTargetEnt != 0)
				{
					const Vector vecToReach = m_hTargetEnt->pev->origin + Vector(0, 0, 50);
					pev->velocity = VecCheckToss(pev, pev->origin, vecToReach);
					//if (pev->velocity != g_vecZero)
					//	ALERT(at_console,"Precise jump for assassin %s\n",STRING(pev->targetname));
					//else
					//	ALERT(at_console,"Precise jump failed. ");
				}
				if (pev->velocity == g_vecZero)
				{ // just jump, it doesn't matter where to.
					//ALERT(at_console,"Nonprecise jump for assassin %s\n",STRING(pev->targetname));
					UTIL_MakeVectors(pev->angles);
					Vector vecDest = pev->origin + (gpGlobals->v_forward * 32);
					vecDest.z += HASSASSIN_JUMP_HEIGHT; // don't forget to jump into the air, now...
					pev->velocity = (vecDest - pev->origin) * JumpSpeedFactor();
				}
			}
			else
				pev->velocity = m_vecJumpVelocity;
			m_flNextJump = gpGlobals->time + 3.0f;
		}
		return;
	default:
		CFollowingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHAssassin::Spawn()
{
	Precache();

	SetMyModel( "models/hassassin.mdl" );
	SetMySize();

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	pev->effects		= 0;
	SetMyHealth( GetSkillValue("hassassin_health") );
	SetMyFieldOfView(VIEW_FIELD_WIDE); // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_MELEE_ATTACK1;
	SetMySquadCapabilities();
	SetMyCanOpenDoors(true);
	pev->friction		= 1;

	m_HackedGunPos		= Vector( 0, 24, 48 );

	UpdateClipSizeForWeapon(m_cClipSize);
	m_cAmmoLoaded = m_cClipSize;

	FollowingMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHAssassin::Precache()
{
	PrecacheMyModel( "models/hassassin.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(shotSoundScript);
	RegisterAndPrecacheSoundScript(footstepSoundScript);
	RegisterAndPrecacheSoundScript(cloakSoundScript);

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(useSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript);

	UTIL_PrecacheOther("grenade", GetProjectileOverrides());

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t tlAssassinFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, (float)2 },
	// { TASK_WAIT_PVS, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_CHASE_ENEMY },
};

Schedule_t slAssassinFail[] =
{
	{
		tlAssassinFail,
		ARRAYSIZE( tlAssassinFail ),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"AssassinFail"
	},
};

//=========================================================
// Enemy exposed Agrunt's cover
//=========================================================
Task_t tlAssassinExposed[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_ASSASSIN_JUMP },
	{ TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t slAssassinExposed[] =
{
	{
		tlAssassinExposed,
		ARRAYSIZE( tlAssassinExposed ),
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AssassinExposed",
	},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlAssassinTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
};

Schedule_t slAssassinTakeCoverFromEnemy[] =
{
	{
		tlAssassinTakeCoverFromEnemy,
		ARRAYSIZE( tlAssassinTakeCoverFromEnemy ),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"AssassinTakeCoverFromEnemy"
	},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t tlAssassinTakeCoverFromEnemy2[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK2 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, (float)384 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
};

Schedule_t slAssassinTakeCoverFromEnemy2[] =
{
	{
		tlAssassinTakeCoverFromEnemy2,
		ARRAYSIZE( tlAssassinTakeCoverFromEnemy2 ),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"AssassinTakeCoverFromEnemy2"
	},
};

//=========================================================
// hide from the loudest sound source
//=========================================================
Task_t tlAssassinTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_MELEE_ATTACK1 },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, (float)179 },
};

Schedule_t slAssassinTakeCoverFromBestSound[] =
{
	{
		tlAssassinTakeCoverFromBestSound,
		ARRAYSIZE( tlAssassinTakeCoverFromBestSound ),
		bits_COND_NEW_ENEMY,
		0,
		"AssassinTakeCoverFromBestSound"
	},
};

//=========================================================
// AlertIdle Schedules
//=========================================================
Task_t tlAssassinHide[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 },
	{ TASK_SET_SCHEDULE, (float)SCHED_CHASE_ENEMY },
};

Schedule_t slAssassinHide[] =
{
	{
		tlAssassinHide,
		ARRAYSIZE( tlAssassinHide ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"AssassinHide"
	},
};

//=========================================================
// HUNT Schedules
//=========================================================
Task_t tlAssassinHunt[] =
{
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slAssassinHunt[] =
{
	{
		tlAssassinHunt,
		ARRAYSIZE( tlAssassinHunt ),
		bits_COND_NEW_ENEMY |
		// bits_COND_SEE_ENEMY |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"AssassinHunt"
	},
};

//=========================================================
// Jumping Schedules
//=========================================================
Task_t tlAssassinJump[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_HOP },
	{ TASK_SET_SCHEDULE, (float)SCHED_ASSASSIN_JUMP_ATTACK },
};

Schedule_t slAssassinJump[] =
{
	{
		tlAssassinJump,
		ARRAYSIZE( tlAssassinJump ),
		0, 
		0, 
		"AssassinJump"
	},
};

//=========================================================
// repel 
//=========================================================
Task_t tlAssassinJumpAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_ASSASSIN_JUMP_LAND },
	// { TASK_SET_ACTIVITY, (float)ACT_FLY },
	{ TASK_ASSASSIN_FALL_TO_GROUND, (float)0 },
};

Schedule_t slAssassinJumpAttack[] =
{
	{
		tlAssassinJumpAttack,
		ARRAYSIZE( tlAssassinJumpAttack ),
		0, 
		0,
		"AssassinJumpAttack"
	},
};

//=========================================================
// repel 
//=========================================================
Task_t	tlAssassinJumpLand[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_ASSASSIN_EXPOSED	},
	// { TASK_SET_FAIL_SCHEDULE, (float)SCHED_MELEE_ATTACK1	},
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_REMEMBER, (float)bits_MEMORY_BADJUMP },
	{ TASK_FIND_NODE_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_FORGET, (float)bits_MEMORY_BADJUMP },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },
};

Schedule_t slAssassinJumpLand[] =
{
	{
		tlAssassinJumpLand,
		ARRAYSIZE( tlAssassinJumpLand ),
		0,
		0,
		"AssassinJumpLand"
	},
};

Task_t tlAssassinJumpFollowing[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_HOP },
	{ TASK_ASSASSIN_FALL_TO_GROUND_FOLLOWING, (float)0 },
};

Schedule_t slAssassinJumpFollowing[] =
{
	{
		tlAssassinJumpFollowing,
		ARRAYSIZE( tlAssassinJumpFollowing ),
		0,
		0,
		"AssassinJumpFollowing"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHAssassin )
{
	slAssassinFail,
	slAssassinExposed,
	slAssassinTakeCoverFromEnemy,
	slAssassinTakeCoverFromEnemy2,
	slAssassinTakeCoverFromBestSound,
	slAssassinHide,
	slAssassinHunt,
	slAssassinJump,
	slAssassinJumpAttack,
	slAssassinJumpLand,
	slAssassinJumpFollowing,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHAssassin, CFollowingMonster )

//=========================================================
// CheckMeleeAttack1 - jump like crazy if the enemy gets too close. 
//=========================================================
bool CHAssassin::CheckMeleeAttack1( float flDot, float flDist )
{
	if( m_flNextJump < gpGlobals->time && ( flDist <= 128.0f || HasMemory( bits_MEMORY_BADJUMP ) ) && m_hEnemy != 0 )
	{
		TraceResult tr;

		Vector vecDest = pev->origin + Vector( RANDOM_FLOAT( -64, 64), RANDOM_FLOAT( -64, 64 ), HASSASSIN_JUMP_HEIGHT );

		UTIL_TraceHull( pev->origin + Vector( 0, 0, 36 ), vecDest + Vector( 0, 0, 36 ), dont_ignore_monsters, human_hull, ENT( pev ), &tr );

		if( tr.fStartSolid || tr.flFraction < 1.0f )
		{
			return false;
		}

		m_vecJumpVelocity = ( vecDest - pev->origin ) * JumpSpeedFactor();

		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack1  - drop a cap in their ass
//
//=========================================================
bool CHAssassin::CheckRangeAttack1( float flDot, float flDist )
{
	if( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist > 64 && flDist <= 2048 /* && flDot >= 0.5 */ && NoFriendlyFire() )
	{
		TraceResult tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), dont_ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
		{
			return true;
		}
	}
	return false;
}

//=========================================================
// CheckRangeAttack2 - toss grenade is enemy gets in the way and is too close. 
//=========================================================
bool CHAssassin::CheckRangeAttack2( float flDot, float flDist )
{
	m_fThrowGrenade = false;
	if( !FBitSet( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		// don't throw grenades at anything that isn't on the ground!
		return false;
	}

	// don't get grenade happy unless the player starts to piss you off
	if( m_iFrustration <= 2 )
		return false;

	if( m_flNextGrenadeCheck < gpGlobals->time && !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 512 /* && flDot >= 0.5 */ /* && NoFriendlyFire() */ )
	{
		Vector vecTarget = m_vecEnemyLKP;
		if (AllyMonsterInRange( vecTarget, 256 ))
		{
			m_flNextGrenadeCheck = gpGlobals->time + 1.0f;
			return m_fThrowGrenade;
		}

		// Originally assassins didn't have this check.
		if( ( vecTarget - pev->origin ).IsLength2DLessThanOrEqual(128.0f) )
		{
			m_flNextGrenadeCheck = gpGlobals->time + 0.5f;
			return m_fThrowGrenade;
		}

		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), m_hEnemy->Center(), flDist, 0.5 ); // use dist as speed to get there in 1 second

		if( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;

			return true;
		}
	}

	return false;
}

//=========================================================
// RunAI
//=========================================================
void CHAssassin::RunAI()
{
	CFollowingMonster::RunAI();

	if( m_Activity == ACT_RUN )
	{
		static int iStep = 0;
		iStep = !iStep;
		if( iStep )
		{
			EmitSoundScript(footstepSoundScript);
		}
	}
	else if (m_Activity == ACT_WALK)
	{
		if (m_nextWalkFootstep <= gpGlobals->time)
		{
			EmitSoundScript(footstepSoundScript);
			m_nextWalkFootstep = gpGlobals->time + 0.58f;
		}
	}
}

//=========================================================
// StartTask
//=========================================================
void CHAssassin::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK2:
		if( !m_fThrowGrenade )
		{
			TaskComplete();
		}
		else
		{
			CFollowingMonster::StartTask( pTask );
		}
		break;
	case TASK_ASSASSIN_FALL_TO_GROUND:
	case TASK_ASSASSIN_FALL_TO_GROUND_FOLLOWING:
		break;
	default:
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask 
//=========================================================
void CHAssassin::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ASSASSIN_FALL_TO_GROUND:
	case TASK_ASSASSIN_FALL_TO_GROUND_FOLLOWING:
	{
		if (pTask->iTask == TASK_ASSASSIN_FALL_TO_GROUND_FOLLOWING)
		{
			CBaseEntity* pLeader = FollowedPlayer();
			if (pLeader)
			{
				MakeIdealYaw(pLeader->pev->origin);
			}
		}
		else
		{
			MakeIdealYaw( m_vecEnemyLKP );
		}
		ChangeYaw( pev->yaw_speed );

		if( m_fSequenceFinished )
		{
			if( pev->velocity.z > 0 )
			{
				pev->sequence = LookupSequence( "fly_up" );
			}
			else if( HasConditions( bits_COND_SEE_ENEMY ) && pTask->iTask != TASK_ASSASSIN_FALL_TO_GROUND_FOLLOWING )
			{
				pev->sequence = LookupSequence( "fly_attack" );
				pev->frame = 0;
			}
			else
			{
				pev->sequence = LookupSequence( "fly_down" );
				pev->frame = 0;
			}

			ResetSequenceInfo();
			SetYawSpeed();
		}
		if( pev->flags & FL_ONGROUND )
		{
			// ALERT( at_console, "on ground\n" );
			TaskComplete();
		}
	}
		break;
	default: 
		CFollowingMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CHAssassin::GetSchedule()
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
		{
			if (pev->movetype == MOVETYPE_TOSS && FBitSet(pev->flags, FL_ONGROUND))
			{
				pev->movetype = MOVETYPE_STEP;
			}
			CSound *pSound = NULL;
			if( HasConditions( bits_COND_HEAR_SOUND ) )
			{
				pSound = PBestSound();

				ASSERT( pSound != NULL );
				if( pSound &&( pSound->m_iType & bits_SOUND_DANGER ) )
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
				}
			}

			Schedule_t* reloadSched = GetIdleReloadSchedule();
			if (reloadSched)
				return reloadSched;

			Schedule_t* utilitySchedule = GetUtilitySchedule();
			if (utilitySchedule)
				return utilitySchedule;

			if( pSound &&( pSound->m_iType & bits_SOUND_COMBAT ) )
			{
				return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
			}
		}
		break;
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CFollowingMonster::GetSchedule();
			}

			// flying?
			if( pev->movetype == MOVETYPE_TOSS )
			{
				if( pev->flags & FL_ONGROUND )
				{
					// ALERT( at_console, "landed\n" );
					// just landed
					pev->movetype = MOVETYPE_STEP;
					return GetScheduleOfType ( SCHED_ASSASSIN_JUMP_LAND );
				}
				else
				{
					// ALERT( at_console, "jump\n" );
					// jump or jump/shoot
					if( m_MonsterState == MONSTERSTATE_COMBAT )
						return GetScheduleOfType( SCHED_ASSASSIN_JUMP );
					else
						return GetScheduleOfType( SCHED_ASSASSIN_JUMP_ATTACK );
				}
			}

			if( HasConditions( bits_COND_HEAR_SOUND ) )
			{
				CSound *pSound = PBestSound();

				ASSERT( pSound != NULL );
				if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
				}
			}

			if( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				m_iFrustration++;
			}
			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			{
				m_iFrustration++;
			}

			if (HasConditions(bits_COND_NO_AMMO_LOADED))
				return GetScheduleOfType(SCHED_RELOAD);

			Schedule_t* regenSchedule = GetRegenerationSchedule();
			if (regenSchedule)
				return regenSchedule;

			// jump player!
			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				// ALERT( at_console, "melee attack 1\n" );
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}

			// throw grenade
			if( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				// ALERT( at_console, "range attack 2\n");
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}

			// spotted
			if( HasConditions( bits_COND_SEE_ENEMY ) && HasConditions( bits_COND_ENEMY_FACING_ME ) )
			{
				// ALERT( at_console, "exposed\n" );
				m_iFrustration++;
				return GetScheduleOfType( SCHED_ASSASSIN_EXPOSED );
			}

			// can attack
			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				// ALERT( at_console, "range attack 1\n" );
				m_iFrustration = 0;
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				// ALERT( at_console, "face\n" );
				return GetScheduleOfType( SCHED_COMBAT_FACE );
			}

			// new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				// ALERT( at_console, "take cover\n" );
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}

			// ALERT( at_console, "stand\n" );
			return GetScheduleOfType( SCHED_ALERT_STAND );
		}
		break;
	default:
		break;
	}

	return CFollowingMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t *CHAssassin::GetScheduleOfType( int Type ) 
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if( pev->health > 30 )
			return slAssassinTakeCoverFromEnemy;
		else
			return slAssassinTakeCoverFromEnemy2;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slAssassinTakeCoverFromBestSound;
	case SCHED_ASSASSIN_EXPOSED:
		return slAssassinExposed;
	case SCHED_FAIL:
		if( m_MonsterState == MONSTERSTATE_COMBAT )
			return slAssassinFail;
		break;
	case SCHED_ALERT_STAND:
		if( m_MonsterState == MONSTERSTATE_COMBAT )
			return slAssassinHide;
		break;
	case SCHED_CHASE_ENEMY:
		return slAssassinHunt;
	case SCHED_MELEE_ATTACK1:
		if( pev->flags & FL_ONGROUND )
		{
			if( m_flNextJump > gpGlobals->time )
			{
				// can't jump yet, go ahead and fail
				return slAssassinFail;
			}
			else
			{
				return slAssassinJump;
			}
		}
		else
		{
			return slAssassinJumpAttack;
		}
	case SCHED_ASSASSIN_JUMP:
	case SCHED_ASSASSIN_JUMP_ATTACK:
		return slAssassinJumpAttack;
	case SCHED_ASSASSIN_JUMP_LAND:
		return slAssassinJumpLand;
	case SCHED_ASSASSIN_JUMP_FOLLOWING:
		return slAssassinJumpFollowing;
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
			if (m_flNextJump <= gpGlobals->time && FindFollowJump())
			{
				m_flNextJump = gpGlobals->time + 0.5f;
				return GetScheduleOfType(SCHED_ASSASSIN_JUMP_FOLLOWING);
			}
			return GetScheduleOfType(SCHED_FOLLOW_NEAREST);
		}
		else
		{
			MakeMyBlockerMoveAway();
			if (m_flNextJump <= gpGlobals->time && FindFollowJump())
			{
				m_flNextJump = gpGlobals->time + 0.5f;
				return GetScheduleOfType(SCHED_ASSASSIN_JUMP_FOLLOWING);
			}
			return GetScheduleOfType(SCHED_FAIL_PVS_INDEPENDENT);
		}
	}
	case SCHED_FOLLOW_NEAREST_FAILED:
	{
		if (m_flNextJump <= gpGlobals->time && FindFollowJump())
		{
			m_flNextJump = gpGlobals->time + 0.5f;
			return GetScheduleOfType(SCHED_ASSASSIN_JUMP_FOLLOWING);
		}
		return CFollowingMonster::GetScheduleOfType(SCHED_FOLLOW_NEAREST_FAILED);
	}
	case SCHED_TARGET_FACE_CHECK_JUMP:
	{
		if (NpcFollowNearest() && m_hTargetEnt != 0 && (m_hTargetEnt->pev->origin - pev->origin).IsLength2DGreaterThanOrEqual(128.0f))
		{
			if (m_flNextJump <= gpGlobals->time && FindFollowJump())
			{
				m_flNextJump = gpGlobals->time + 0.5f;
				return GetScheduleOfType(SCHED_ASSASSIN_JUMP_FOLLOWING);
			}
		}
		return GetScheduleOfType(SCHED_TARGET_FACE);
	}
	}

	return CFollowingMonster::GetScheduleOfType( Type );
}

static Vector TraceBottom(const Vector& pos, edict_t* pIgnore, float deepness = 500)
{
	TraceResult tr;
	UTIL_TraceLine(pos, pos - Vector(0,0,deepness), ignore_monsters, pIgnore, &tr);
	return tr.vecEndPos;
}

enum
{
	MONSTERJUMP_OK = 0,
	MONSTERJUMP_CLOSE_IN_2D,
	MONSTERJUMP_TOOFAR_SOLID,
	MONSTERJUMP_TOOFAR_TOODEEP,
	MONSTERJUMP_TOOHIGH,
	MONSTERJUMP_NO_TRACE_TO_APEX
};

std::pair<Vector, int> CHAssassin::CalcMonsterJump(const Vector &vecTarget, float maximumHeight, float maximumDistance)
{
	const float myHeight = pev->size.z;
	const float halfMyHeight = myHeight * 0.5f;
	const Vector halfMyHeightVec(0.0f, 0.0f, halfMyHeight);
	Vector vecChosenDest = vecTarget;

	const Vector2D vec2DDist = (vecChosenDest - pev->origin).Make2D();
	const float dist2DSqr = vec2DDist.LengthSqr();
	const bool isTooFar = dist2DSqr > maximumDistance * maximumDistance;
	const bool isTooCloseIn2D = dist2DSqr < 48.0f * 48.0f;
	if (isTooCloseIn2D)
	{
		return std::make_pair(g_vecZero, MONSTERJUMP_CLOSE_IN_2D);
	}

	// Can't jump that far. Just jump to get closer
	if (isTooFar)
	{
		const Vector2D horizontalShift = vec2DDist.Normalize() * maximumDistance;
		vecChosenDest.x = pev->origin.x + horizontalShift.x;
		vecChosenDest.y = pev->origin.y + horizontalShift.y;

		if (UTIL_PointContents(vecChosenDest) == CONTENTS_SOLID)
		{
			return std::make_pair(g_vecZero, MONSTERJUMP_TOOFAR_SOLID);
		}

		const Vector bottom = TraceBottom(vecChosenDest, edict());
		const float bottomZ = bottom.z;
		if (bottomZ + 8.0f < Q_min(pev->origin.z, vecChosenDest.z))
		{
			//DrawBeamLine(vecChosenDest, bottom, Color3(255, 100, 50));
			return std::make_pair(g_vecZero, MONSTERJUMP_TOOFAR_TOODEEP);
		}
	}

	Vector midPoint = (vecChosenDest + pev->origin) * 0.5f;

	TraceResult mindpointTr;
	UTIL_TraceLine(midPoint, midPoint + Vector(0,0,Q_max(500.0f, maximumHeight + myHeight)), ignore_monsters, edict(), &mindpointTr);
	const float ceilingZ = mindpointTr.vecEndPos.z;

	const float jumpMinZ = Q_max(vecChosenDest.z, pev->origin.z) + halfMyHeight * 0.5f;
	const float jumpMaxZ = Q_max(ceilingZ - myHeight, jumpMinZ);

	if (jumpMinZ - pev->origin.z > maximumHeight)
	{
		return std::make_pair(g_vecZero, MONSTERJUMP_TOOHIGH);
	}

	Vector vecApex = midPoint;
	vecApex.z = Q_min(pev->origin.z + maximumHeight, jumpMaxZ);

	const Vector vecTraceHullStart = pev->origin + halfMyHeightVec;
	const Vector vectraceHullEnd = vecApex + halfMyHeightVec;

	TraceResult tr;
	UTIL_TraceHull(vecTraceHullStart, vectraceHullEnd, dont_ignore_monsters, human_hull, edict(), &tr );

	if (!tr.fStartSolid && tr.flFraction == 1.0f)
	{
		//DrawBeamLine(vecTraceHullStart, vectraceHullEnd, Color3(0, 255, 0));

		auto launchCalcResult = CalcLaunchToHeight(vecApex.z - pev->origin.z);

		Vector velocity;
		velocity.x = (vecApex.x - pev->origin.x) / launchCalcResult.time;
		velocity.y = (vecApex.y - pev->origin.y) / launchCalcResult.time;
		velocity.z = launchCalcResult.speed;
		return std::make_pair(velocity, MONSTERJUMP_OK);
	}
	else
	{
		//DrawBeamLine(vecTraceHullStart, vectraceHullEnd, Color3(255, 0, 0));
		return std::make_pair(g_vecZero, MONSTERJUMP_NO_TRACE_TO_APEX);
	}
}

bool CHAssassin::FindFollowJump()
{
	CBaseEntity* pLeader = FollowedPlayer();
	if (!pLeader)
		return false;

	const float horizontalJumpDistance = 360.0f;
	const Vector vecTarget(pLeader->pev->origin.x, pLeader->pev->origin.y, pLeader->pev->absmin.z + 1.0f);

	const Vector2D vec2DDist = (vecTarget - pev->origin).Make2D();
	const float dist2DSqr = vec2DDist.LengthSqr();
	const bool isTooCloseIn2D = dist2DSqr < 48.0f * 48.0f;

	if (isTooCloseIn2D)
	{
		if (std::fabs(pev->origin.z - vecTarget.z) <= pev->size.z)
		{
			// Refuse early
			//ALERT(at_console, "%s: already is close to the target\n", STRING(pev->classname));
			return false;
		}
		if (pev->origin.z < vecTarget.z)
		{
			// The target is on top. Can't handle it for now
			return false;
		}

		// The target is underneath. Try jump off to it
		const std::pair<float, float> directions[] = {
			{1, 0},
			{-1, 0},
			{0, 1},
			{0, -1}
		};

		const float spotOffset = Q_min(128.0f, horizontalJumpDistance);

		const size_t indexShift = RANDOM_LONG(0, ARRAYSIZE(directions)-1);

		for (size_t i=0; i<ARRAYSIZE(directions); ++i)
		{
			size_t j = (i + indexShift) % ARRAYSIZE(directions);
			const auto& randomDir = directions[j];

			Vector vecSpot = pev->origin + Vector(randomDir.first * spotOffset, randomDir.second * spotOffset, 0.0f);
			vecSpot.z = vecTarget.z;

			auto jumpResult = CalcMonsterJump(vecSpot, HASSASSIN_JUMP_HEIGHT, horizontalJumpDistance);
			if (jumpResult.first != g_vecZero)
			{
				m_vecJumpVelocity = jumpResult.first;
				return true;
			}
		}

		return false;
	}

	//TODO: calculate this depending on the monster and player geometry?
	const FloatRange baseOffsetRange(45.0f, 50.0f); // don't jump right onto the target

	const Vector2D vecTarget2D = vecTarget.Make2D();
	const Vector2D vecToTarget = vecTarget2D - pev->origin.Make2D();
	const Vector2D dirToTarget = vecToTarget.Normalize();
	const Vector2D inFrontOfTarget = vecTarget2D - dirToTarget * RandomizeNumberFromRange(baseOffsetRange);
	const Vector2D toRightFromTarget = vecTarget2D + Vector2D(dirToTarget.y, -dirToTarget.x) * RandomizeNumberFromRange(baseOffsetRange);
	const Vector2D toLeftFromTarget = vecTarget2D + Vector2D(-dirToTarget.y, dirToTarget.x) * RandomizeNumberFromRange(baseOffsetRange);

	const Vector possibleTargets[] = {
		Vector(inFrontOfTarget, vecTarget.z),
		Vector(toRightFromTarget, vecTarget.z),
		Vector(toLeftFromTarget, vecTarget.z)
	};

	const size_t indexShift = RANDOM_LONG(0, ARRAYSIZE(possibleTargets)-1);

	for (size_t i=0; i<ARRAYSIZE(possibleTargets); ++i)
	{
		size_t j = (i + indexShift) % ARRAYSIZE(possibleTargets);

		const Vector& vecPossibleTarget = possibleTargets[j];
		const Vector bottom = TraceBottom(vecPossibleTarget, edict());

		if (bottom.z + 8.0f < Q_min(pev->origin.z, vecPossibleTarget.z))
		{
			continue;
		}

		auto jumpResult = CalcMonsterJump(vecPossibleTarget, HASSASSIN_JUMP_HEIGHT, horizontalJumpDistance);
		if (jumpResult.first != g_vecZero)
		{
			m_vecJumpVelocity = jumpResult.first;
			//ALERT(at_console, "%s: vertical jump velocity: %g; horizontal jump velocity: %g\n", STRING(pev->classname), m_vecJumpVelocity.z, m_vecJumpVelocity.Make2D().Length());
			return true;
		}
		else
		{
			/*switch(jumpResult.second)
			{
			case MONSTERJUMP_CLOSE_IN_2D:
				ALERT(at_console, "%s can't make jump: is already too close\n", STRING(pev->classname));
				break;
			case MONSTERJUMP_TOOFAR_SOLID:
				ALERT(at_console, "%s can't make jump: the target is to far and the new selected point is in solid volume\n", STRING(pev->classname));
				break;
			case MONSTERJUMP_TOOFAR_TOODEEP:
				ALERT(at_console, "%s can't make jump: new selected point is too deep\n", STRING(pev->classname));
				break;
			case MONSTERJUMP_TOOHIGH:
				ALERT(at_console, "%s can't make jump: too high!\n", STRING(pev->classname));
				break;
			default:
				ALERT(at_console, "%s can't make jump: can't calculate jump\n", STRING(pev->classname));
				break;
			}*/
		}
	}

	return false;
}

class CDeadHAssassin : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/hassassin.mdl"; }
	int	DefaultClassify() override
	{
		if (g_modFeatures.blackops_classify)
			return CLASS_HUMAN_BLACKOPS;
		return CLASS_HUMAN_MILITARY;
	}
	const char* getPos(int pos) const override;
	static const char *m_szPoses[3];
};

const char *CDeadHAssassin::m_szPoses[] = { "death_during_run", "die_backwards", "die_simple" };

const char* CDeadHAssassin::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_human_assassin_dead, CDeadHAssassin )

void CDeadHAssassin::Spawn()
{
	SpawnHelper();
	MonsterInitDead();
	pev->frame = 255;
}
