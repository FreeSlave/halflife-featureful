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
// Agrunt - Dominant, warlike alien grunt monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"followingmonster.h"
#include	"combat.h"
#include	"soundent.h"
#include	"hornet.h"
#include	"scripted.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_AGRUNT_SUPPRESS = LAST_FOLLOWINGMONSTER_SCHEDULE + 1,
	SCHED_AGRUNT_THREAT_DISPLAY
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_AGRUNT_SETUP_HIDE_ATTACK = LAST_FOLLOWINGMONSTER_TASK + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		AGRUNT_AE_HORNET1	( 1 )
#define		AGRUNT_AE_HORNET2	( 2 )
#define		AGRUNT_AE_HORNET3	( 3 )
#define		AGRUNT_AE_HORNET4	( 4 )
#define		AGRUNT_AE_HORNET5	( 5 )
// some events are set up in the QC file that aren't recognized by the code yet.
#define		AGRUNT_AE_PUNCH		( 6 )
#define		AGRUNT_AE_BITE		( 7 )

#define		AGRUNT_AE_LEFT_FOOT	 ( 10 )
#define		AGRUNT_AE_RIGHT_FOOT ( 11 )

#define		AGRUNT_AE_LEFT_PUNCH ( 12 )
#define		AGRUNT_AE_RIGHT_PUNCH ( 13 )

#define		AGRUNT_MELEE_DIST	100.0f

class CAGrunt : public CFollowingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int DefaultClassify( void );
	const char* DefaultDisplayName() { return "Alien Grunt"; }
	const char* ReverseRelationshipModel() { return "models/agruntf.mdl"; }
	int DefaultISoundMask( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetObjectCollisionBox( void )
	{
		SetMyObjectCollisionBox(Vector( -32.0f, -32.0f, 0.0f ), Vector( 32.0f, 32.0f, 85.0f ));
	}

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	bool FCanCheckAttacks( void ) override;
	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	void StartTask( Task_t *pTask );
	void AlertSound( void );
	void DeathSound( void );
	void PainSound( void );
	void AttackSound( void );
	void PrescheduleThink( void );
	float HeadHitGroupDamageMultiplier();
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
	int IRelationship( CBaseEntity *pTarget );
	void StopTalking( void );
	bool ShouldSpeak( void );
	void PlayUseSentence();
	void PlayUnUseSentence();
	CUSTOM_SCHEDULES

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual int DefaultSizeForGrapple() { return GRAPPLE_LARGE; }
	bool IsDisplaceable() { return true; }

	Vector DefaultMinHullSize() { return Vector( -32.0f, -32.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 32.0f, 32.0f, 64.0f ); }

	static constexpr const char* attackHitSoundScript = "AlienGrunt.AttackHit";
	static constexpr const char* attackMissSoundScript = "AlienGrunt.AttackMiss";
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript leftFootSoundScript;
	static const NamedSoundScript rightFootSoundScript;
	static const NamedSoundScript fireSoundScript;
	static constexpr const char* useSoundScript = "AlienGrunt.Use";
	static constexpr const char* unuseSoundScript = "AlienGrunt.UnUse";

	bool m_fCanHornetAttack;
	float m_flNextHornetAttackCheck;

	float m_flNextPainTime;

	// three hacky fields for speech stuff. These don't really need to be saved.
	float m_flNextSpeakTime;
	float m_flNextWordTime;
	int m_iLastWord;

	static const NamedVisual muzzleFlashVisual;
};

LINK_ENTITY_TO_CLASS( monster_alien_grunt, CAGrunt )

TYPEDESCRIPTION	CAGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CAGrunt, m_fCanHornetAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAGrunt, m_flNextHornetAttackCheck, FIELD_TIME ),
	DEFINE_FIELD( CAGrunt, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CAGrunt, m_flNextSpeakTime, FIELD_TIME ),
	DEFINE_FIELD( CAGrunt, m_flNextWordTime, FIELD_TIME ),
	DEFINE_FIELD( CAGrunt, m_iLastWord, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAGrunt, CFollowingMonster )

const NamedSoundScript CAGrunt::attackSoundScript = {
	CHAN_VOICE,
	{"agrunt/ag_attack1.wav", "agrunt/ag_attack2.wav", "agrunt/ag_attack3.wav"},
	"AlienGrunt.Attack"
};

const NamedSoundScript CAGrunt::dieSoundScript = {
	CHAN_VOICE,
	{"agrunt/ag_die1.wav", "agrunt/ag_die4.wav", "agrunt/ag_die5.wav"},
	"AlienGrunt.Die"
};

const NamedSoundScript CAGrunt::painSoundScript = {
	CHAN_VOICE,
	{"agrunt/ag_pain1.wav", "agrunt/ag_pain2.wav", "agrunt/ag_pain3.wav", "agrunt/ag_pain4.wav", "agrunt/ag_pain5.wav"},
	"AlienGrunt.Pain"
};

const NamedSoundScript CAGrunt::idleSoundScript = {
	CHAN_VOICE,
	{"agrunt/ag_idle1.wav", "agrunt/ag_idle2.wav", "agrunt/ag_idle3.wav", "agrunt/ag_idle4.wav"},
	"AlienGrunt.Idle"
};

const NamedSoundScript CAGrunt::alertSoundScript = {
	CHAN_VOICE,
	{"agrunt/ag_alert1.wav", "agrunt/ag_alert3.wav", "agrunt/ag_alert4.wav", "agrunt/ag_alert5.wav"},
	"AlienGrunt.Alert"
};

const NamedSoundScript CAGrunt::leftFootSoundScript = {
	CHAN_BODY,
	{"player/pl_ladder2.wav", "player/pl_ladder4.wav"},
	IntRange(70),
	"AlienGrunt.LeftFoot"
};

const NamedSoundScript CAGrunt::rightFootSoundScript = {
	CHAN_BODY,
	{"player/pl_ladder1.wav", "player/pl_ladder3.wav"},
	IntRange(70),
	"AlienGrunt.RightFoot"
};

const NamedSoundScript CAGrunt::fireSoundScript = {
	CHAN_WEAPON,
	{"agrunt/ag_fire1.wav", "agrunt/ag_fire2.wav", "agrunt/ag_fire3.wav"},
	"AlienGrunt.Fire"
};

const NamedVisual CAGrunt::muzzleFlashVisual = BuildVisual("AlienGrunt.MuzzleFlash")
		.Model("sprites/muz4.spr")
		.RenderMode(kRenderTransAdd)
		.Scale(0.6f)
		.Alpha(128);

//=========================================================
// IRelationship - overridden because Human Grunts are
// Alien Grunt's nemesis.
//=========================================================
int CAGrunt::IRelationship( CBaseEntity *pTarget )
{
	if( IDefaultRelationship(pTarget) >= R_DL && FClassnameIs( pTarget->pev, "monster_human_grunt" ) )
	{
		return R_NM;
	}

	return CFollowingMonster::IRelationship( pTarget );
}

//=========================================================
// ISoundMask
//=========================================================
int CAGrunt::DefaultISoundMask( void )
{
	return ( bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER );
}

//=========================================================
// TraceAttack
//=========================================================
static void AgruntTraceAttack( CBaseMonster* self, entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	DamageInfo dmgInfo = damageInfo;
	if( ptr->iHitgroup == 10 && ( dmgInfo.type & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) ) )
	{
		// hit armor
		if( self->pev->dmgtime != gpGlobals->time || ( RANDOM_LONG( 0, 10 ) < 1 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1.0f, 2.0f ) );
			self->pev->dmgtime = gpGlobals->time;
		}

		if( RANDOM_LONG( 0, 1 ) == 0 )
		{
			Vector vecTracerDir = vecDir;

			vecTracerDir.x += RANDOM_FLOAT( -0.3f, 0.3f );
			vecTracerDir.y += RANDOM_FLOAT( -0.3f, 0.3f );
			vecTracerDir.z += RANDOM_FLOAT( -0.3f, 0.3f );

			vecTracerDir = vecTracerDir * -512.0f;

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos );
			WRITE_BYTE( TE_TRACER );
				WRITE_VECTOR( ptr->vecEndPos );
				WRITE_VECTOR( vecTracerDir );
			MESSAGE_END();
		}

		dmgInfo.damage -= 20.0f;
		if( dmgInfo.damage <= 0.0f )
			dmgInfo.damage = 0.1f;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated

		ptr->iHitgroup = HITGROUP_GENERIC;
		dmgInfo.SetNoBlood();
	}

	self->CBaseMonster::TraceAttack(pevInflictor, pevAttacker, dmgInfo, vecDir, ptr);
}

float CAGrunt::HeadHitGroupDamageMultiplier()
{
	return Q_min(gSkillData.monHead, 1.5f);
}

void CAGrunt::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	AgruntTraceAttack(this, pevInflictor, pevAttacker, damageInfo, vecDir, ptr);
}

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void CAGrunt::StopTalking( void )
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10.0f + RANDOM_LONG( 0, 10 );
}

//=========================================================
// ShouldSpeak - Should this agrunt be talking?
//=========================================================
bool CAGrunt::ShouldSpeak( void )
{
	if( m_flNextSpeakTime > gpGlobals->time )
	{
		// my time to talk is still in the future.
		return false;
	}

	if( pev->spawnflags & SF_MONSTER_GAG )
	{
		if( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time
			// into the future a bit, so we don't talk immediately after
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3.0f;
			return false;
		}
	}

	return true;
}

//=========================================================
// PrescheduleThink
//=========================================================
void CAGrunt::PrescheduleThink( void )
{
	if( ShouldSpeak() )
	{
		if( m_flNextWordTime < gpGlobals->time )
		{
			int num = -1;

			const SoundScript* myIdleSoundScript = GetSoundScript(idleSoundScript.name);
			if (myIdleSoundScript)
			{
				if (myIdleSoundScript->waves.size() == 1)
				{
					num = 0;
				}
				else if (myIdleSoundScript->waves.size() > 1)
				{
					do
					{
						num = RANDOM_LONG(0, myIdleSoundScript->waves.size()-1);
					}
					while( num == m_iLastWord );
				}
				else
				{
					num = -1;
				}

				if (num >= 0)
				{
					// play a new sound
					EmitSoundScriptSelectedSample(myIdleSoundScript, num);
				}
			}

			m_iLastWord = num;

			// is this word our last?
			if( RANDOM_LONG( 1, 10 ) <= 1 )
			{
				// stop talking.
				StopTalking();
			}
			else
			{
				m_flNextWordTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 1.0f );
			}
		}
	}
	CFollowingMonster::PrescheduleThink();
}

//=========================================================
// DieSound
//=========================================================
void CAGrunt::DeathSound( void )
{
	StopTalking();
	EmitSoundScript(dieSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CAGrunt::AlertSound( void )
{
	StopTalking();
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// AttackSound
//=========================================================
void CAGrunt::AttackSound( void )
{
	StopTalking();
	EmitSoundScript(attackSoundScript);
}

//=========================================================
// PainSound
//=========================================================
void CAGrunt::PainSound( void )
{
	if( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}
	m_flNextPainTime = gpGlobals->time + 0.6f;

	StopTalking();
	EmitSoundScript(painSoundScript);
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CAGrunt::DefaultClassify( void )
{
	return CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CAGrunt::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 110;
		break;
	default:
		ys = 100;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CAGrunt::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case AGRUNT_AE_HORNET1:
	case AGRUNT_AE_HORNET2:
	case AGRUNT_AE_HORNET3:
	case AGRUNT_AE_HORNET4:
	case AGRUNT_AE_HORNET5:
		{
			// m_vecEnemyLKP should be center of enemy body
			Vector vecArmPos, vecArmDir;
			Vector vecDirToEnemy;
			Vector angDir;

			if( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				vecDirToEnemy = ( ( m_vecEnemyLKP ) - pev->origin );
				angDir = UTIL_VecToAngles( vecDirToEnemy );
				vecDirToEnemy = vecDirToEnemy.Normalize();
			}
			else
			{
				angDir = pev->angles;
				UTIL_MakeAimVectors( angDir );
				vecDirToEnemy = gpGlobals->v_forward;
			}

			pev->effects |= EF_MUZZLEFLASH;

			// make angles +-180
			if( angDir.x > 180.0f )
			{
				angDir.x = angDir.x - 360.0f;
			}

			SetBlending( 0, angDir.x );
			GetAttachment( 0, vecArmPos, vecArmDir );

			vecArmPos = vecArmPos + vecDirToEnemy * 32.0f;

			SendSprite(vecArmPos, GetVisual(muzzleFlashVisual));

			CBaseEntity *pHornet = CBaseEntity::Create( "hornet", vecArmPos, UTIL_VecToAngles( vecDirToEnemy ), edict(), GetProjectileOverrides() );
			UTIL_MakeVectors( pHornet->pev->angles );
			pHornet->pev->velocity = gpGlobals->v_forward * 300.0f;

			EmitSoundScript(fireSoundScript);

			CBaseMonster *pHornetMonster = pHornet->MyMonsterPointer();

			if( pHornetMonster )
			{
				if (m_pCine && m_pCine->PreciseAttack()) //LRC- are we doing a scripted action?
					pHornetMonster->m_hEnemy = m_hTargetEnt;
				else
					pHornetMonster->m_hEnemy = m_hEnemy;
				pHornetMonster->m_iClass = m_iClass;
				pHornetMonster->m_reverseRelationship = m_reverseRelationship;
			}
		}
		break;
	case AGRUNT_AE_LEFT_FOOT:
		EmitSoundScript(leftFootSoundScript);
		break;
	case AGRUNT_AE_RIGHT_FOOT:
		// right foot
		EmitSoundScript(rightFootSoundScript);
		break;

	case AGRUNT_AE_LEFT_PUNCH:
		{
			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);

			TraceHullAttackParams params;
			params.distance = AGRUNT_MELEE_DIST;
			params.punchAngle.y = -25.0f;
			params.punchAngle.x = 8.0f;
			params.knockRight = 250.0f;
			params.knockPlayerOnly = true;
			params.damageInfo.damage = gSkillData.agruntDmgPunch;
			params.damageInfo.type = DMG_CLUB;
			params.spawnBlood = true;
			params.bloodOrigin = vecArmPos;
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformTraceHullAttack(params);
		}
		break;
	case AGRUNT_AE_RIGHT_PUNCH:
		{
			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);

			TraceHullAttackParams params;
			params.distance = AGRUNT_MELEE_DIST;
			params.punchAngle.y = 25.0f;
			params.punchAngle.x = 8.0f;
			params.knockRight = -250.0f;
			params.knockPlayerOnly = true;
			params.damageInfo.damage = gSkillData.agruntDmgPunch;
			params.damageInfo.type = DMG_CLUB;
			params.spawnBlood = true;
			params.bloodOrigin = vecArmPos;
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformTraceHullAttack(params);
		}
		break;
	default:
		CFollowingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CAGrunt::Spawn()
{
	Precache();

	SetMyModel( "models/agrunt.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects = 0;
	SetMyHealth( gSkillData.agruntHealth );
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(false);

	m_HackedGunPos = Vector( 24.0f, 64.0f, 48.0f );

	m_flNextSpeakTime = m_flNextWordTime = gpGlobals->time + 10.0f + RANDOM_LONG( 0, 10 );

	FollowingMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CAGrunt::Precache()
{
	PrecacheMyModel( "models/agrunt.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(leftFootSoundScript);
	RegisterAndPrecacheSoundScript(rightFootSoundScript);
	RegisterAndPrecacheSoundScript(fireSoundScript);
	RegisterAndPrecacheSoundScript(useSoundScript, idleSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript, alertSoundScript);

	RegisterVisual(muzzleFlashVisual);

	UTIL_PrecacheOther( "hornet", GetProjectileOverrides() );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t tlAGruntFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2.0f },
	{ TASK_WAIT_PVS, 0.0f },
};

Schedule_t slAGruntFail[] =
{
	{
		tlAGruntFail,
		ARRAYSIZE( tlAGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AGrunt Fail"
	},
};

//=========================================================
// Combat Fail Schedule
//=========================================================
Task_t tlAGruntCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2.0f },
	{ TASK_WAIT_PVS, 0.0f },
};

Schedule_t slAGruntCombatFail[] =
{
	{
		tlAGruntCombatFail,
		ARRAYSIZE( tlAGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AGrunt Combat Fail"
	},
};

//=========================================================
// Standoff schedule. Used in combat when a monster is
// hiding in cover or the enemy has moved out of sight.
// Should we look around in this schedule?
//=========================================================
Task_t tlAGruntStandoff[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2.0f },
};

Schedule_t slAGruntStandoff[] =
{
	{
		tlAGruntStandoff,
		ARRAYSIZE( tlAGruntStandoff ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Agrunt Standoff"
	}
};

//=========================================================
// Suppress
//=========================================================
Task_t tlAGruntSuppressHornet[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_RANGE_ATTACK1, 0.0f },
};

Schedule_t slAGruntSuppress[] =
{
	{
		tlAGruntSuppressHornet,
		ARRAYSIZE( tlAGruntSuppressHornet ),
		0,
		0,
		"AGrunt Suppress Hornet",
	},
};

//=========================================================
// primary range attacks
//=========================================================
Task_t tlAGruntRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_RANGE_ATTACK1, 0.0f },
};

Schedule_t slAGruntRangeAttack1[] =
{
	{
		tlAGruntRangeAttack1,
		ARRAYSIZE( tlAGruntRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE,
		0,
		"AGrunt Range Attack1"
	},
};

Task_t tlAGruntHiddenRangeAttack1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_STANDOFF },
	{ TASK_AGRUNT_SETUP_HIDE_ATTACK, 0 },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_RANGE_ATTACK1_NOTURN, (float)0 },
};

Schedule_t slAGruntHiddenRangeAttack[] =
{
	{
		tlAGruntHiddenRangeAttack1,
		ARRAYSIZE ( tlAGruntHiddenRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"AGrunt Hidden Range Attack1"
	},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node
// cover!
//=========================================================
Task_t tlAGruntTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_WAIT, 0.1f },
	{ TASK_FIND_COVER_FROM_ENEMY, 0.0f },
	{ TASK_RUN_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0.0f },
};

Schedule_t slAGruntTakeCoverFromEnemy[] =
{
	{
		tlAGruntTakeCoverFromEnemy,
		ARRAYSIZE( tlAGruntTakeCoverFromEnemy ),
		bits_COND_NEW_ENEMY,
		0,
		"AGruntTakeCoverFromEnemy"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlAGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_AGRUNT_THREAT_DISPLAY },
	{ TASK_WAIT, 0.2f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,	50.0f },
	{ TASK_WALK_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.1f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
	{ TASK_PLAY_SEQUENCE, (float)ACT_THREAT_DISPLAY },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.1f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.1f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.1f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.1f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
};

Schedule_t slAGruntVictoryDance[] =
{
	{
		tlAGruntVictoryDance,
		ARRAYSIZE( tlAGruntVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"AGruntVictoryDance"
	},
};

//=========================================================
//=========================================================
Task_t tlAGruntThreatDisplay[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_THREAT_DISPLAY },
};

Schedule_t slAGruntThreatDisplay[] =
{
	{
		tlAGruntThreatDisplay,
		ARRAYSIZE( tlAGruntThreatDisplay ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_PLAYER |
		bits_SOUND_COMBAT |
		bits_SOUND_WORLD,
		"AGruntThreatDisplay"
	},
};

DEFINE_CUSTOM_SCHEDULES( CAGrunt )
{
	slAGruntFail,
	slAGruntCombatFail,
	slAGruntStandoff,
	slAGruntSuppress,
	slAGruntRangeAttack1,
	slAGruntHiddenRangeAttack,
	slAGruntTakeCoverFromEnemy,
	slAGruntVictoryDance,
	slAGruntThreatDisplay,
};

IMPLEMENT_CUSTOM_SCHEDULES( CAGrunt, CFollowingMonster )

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
bool CAGrunt::FCanCheckAttacks( void )
{
	if( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

//=========================================================
// CheckMeleeAttack1 - alien grunts zap the crap out of
// any enemy that gets too close.
//=========================================================
bool CAGrunt::CheckMeleeAttack1( float flDot, float flDist )
{
	CheckMeleeAttackParams params;
	params.distance = AGRUNT_MELEE_DIST;
	params.dot = 0.6f;
	return HasConditions( bits_COND_SEE_ENEMY ) && CheckMeleeAttackImpl(flDot, flDist, params, false) && m_hEnemy != 0;
}

//=========================================================
// CheckRangeAttack1
//
// !!!LATER - we may want to load balance this. Several
// tracelines are done, so we may not want to do this every
// server frame. Definitely not while firing.
//=========================================================
bool CAGrunt::CheckRangeAttack1( float flDot, float flDist )
{
	if( gpGlobals->time < m_flNextHornetAttackCheck )
	{
		return m_fCanHornetAttack;
	}

	if( HasConditions( bits_COND_SEE_ENEMY ) && flDist >= AGRUNT_MELEE_DIST && flDist <= 1024.0f && flDot >= 0.5f && NoFriendlyFire() )
	{
		TraceResult tr;
		Vector	vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors( pev->angles );
		GetAttachment( 0, vecArmPos, vecArmDir );
		//UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256.0f, ignore_monsters, ENT( pev ), &tr );
		UTIL_TraceLine( vecArmPos, m_hEnemy->BodyTarget( vecArmPos ), dont_ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction == 1.0f || tr.pHit == m_hEnemy->edict() )
		{
			m_flNextHornetAttackCheck = gpGlobals->time + RANDOM_FLOAT( 2.0f, 5.0f );
			m_fCanHornetAttack = true;
			return m_fCanHornetAttack;
		}
	}

	m_flNextHornetAttackCheck = gpGlobals->time + 0.2f;// don't check for half second if this check wasn't successful
	m_fCanHornetAttack = false;
	return m_fCanHornetAttack;
}

//=========================================================
// StartTask
//=========================================================
void CAGrunt::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_AGRUNT_SETUP_HIDE_ATTACK:
		// alien grunt shoots hornets back out into the open from a concealed location.
		// try to find a spot to throw that gives the smart weapon a good chance of finding the enemy.
		// ideally, this spot is along a line that is perpendicular to a line drawn from the agrunt to the enemy.
		CBaseMonster	*pEnemyMonsterPtr;

		pEnemyMonsterPtr = m_hEnemy->MyMonsterPointer();

		if( pEnemyMonsterPtr )
		{
			Vector vecCenter;
			TraceResult tr;
			bool fSkip = false;
			vecCenter = Center();

			UTIL_VecToAngles( m_vecEnemyLKP - pev->origin );

			UTIL_TraceLine( Center() + gpGlobals->v_forward * 128.0f, m_vecEnemyLKP, ignore_monsters, ENT( pev ), &tr );
			if( tr.flFraction == 1.0f )
			{
				MakeIdealYaw( pev->origin + gpGlobals->v_right * 128.0f );
				fSkip = true;
				TaskComplete();
			}

			if( !fSkip )
			{
				UTIL_TraceLine( Center() - gpGlobals->v_forward * 128.0f, m_vecEnemyLKP, ignore_monsters, ENT( pev ), &tr );
				if( tr.flFraction == 1.0f )
				{
					MakeIdealYaw( pev->origin - gpGlobals->v_right * 128.0f );
					fSkip = true;
					TaskComplete();
				}
			}

			if( !fSkip )
			{
				UTIL_TraceLine( Center() + gpGlobals->v_forward * 256.0f, m_vecEnemyLKP, ignore_monsters, ENT( pev ), &tr );
				if( tr.flFraction == 1.0f )
				{
					MakeIdealYaw( pev->origin + gpGlobals->v_right * 256.0f );
					fSkip = true;
					TaskComplete();
				}
			}

			if( !fSkip )
			{
				UTIL_TraceLine( Center() - gpGlobals->v_forward * 256.0f, m_vecEnemyLKP, ignore_monsters, ENT( pev ), &tr );
				if( tr.flFraction == 1.0f )
				{
					MakeIdealYaw( pev->origin - gpGlobals->v_right * 256.0f );
					fSkip = true;
					TaskComplete();
				}
			}

			if( !fSkip )
			{
				TaskFail("failed to setup a hidden attack");
			}
		}
		else
		{
			TaskFail("no enemy");
		}
		break;
	default:
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CAGrunt::GetSchedule( void )
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
		{
			// dangerous sound nearby!
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			if( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

			// zap player!
			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				AttackSound();// this is a total hack. Should be parto f the schedule
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}

			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

			// can attack
			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot ( bits_SLOTS_AGRUNT_HORNET ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if( OccupySlot ( bits_SLOT_AGRUNT_CHASE ) )
			{
				return GetScheduleOfType( SCHED_CHASE_ENEMY );
			}

			return GetScheduleOfType( SCHED_STANDOFF );
		}
		break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
	{
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
		break;
	}
	default:
		break;
	}

	return CFollowingMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t *CAGrunt::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slAGruntTakeCoverFromEnemy[0];
		break;
	case SCHED_RANGE_ATTACK1:
		if( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			//normal attack
			return &slAGruntRangeAttack1[0];
		}
		else
		{
			// attack an unseen enemy
			// return &slAGruntHiddenRangeAttack[0];
			return &slAGruntRangeAttack1[0];
		}
		break;
	case SCHED_AGRUNT_THREAT_DISPLAY:
		return &slAGruntThreatDisplay[0];
		break;
	case SCHED_AGRUNT_SUPPRESS:
		return &slAGruntSuppress[0];
		break;
	case SCHED_STANDOFF:
		return &slAGruntStandoff[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slAGruntVictoryDance[0];
		break;
	case SCHED_FAIL:
		// no fail schedule specified, so pick a good generic one.
		{
			if( m_hEnemy != 0 )
			{
				// I have an enemy
				// !!!LATER - what if this enemy is really far away and i'm chasing him?
				// this schedule will make me stop, face his last known position for 2
				// seconds, and then try to move again
				return &slAGruntCombatFail[0];
			}

			return &slAGruntFail[0];
		}
		break;
	}

	return CFollowingMonster::GetScheduleOfType( Type );
}

void CAGrunt::PlayUseSentence()
{
	EmitSoundScript(useSoundScript);
	StopTalking();
}

void CAGrunt::PlayUnUseSentence()
{
	EmitSoundScript(unuseSoundScript);
	StopTalking();
}

class CDeadAgrunt : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/agrunt.mdl"; }
	int	DefaultClassify ( void ) { return	CLASS_ALIEN_MILITARY; }
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;

	const char* getPos(int pos) const;
	static const char *m_szPoses[2];
};

const char *CDeadAgrunt::m_szPoses[] = { "diesimple", "diebackward" };

const char* CDeadAgrunt::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_alien_grunt_dead, CDeadAgrunt )

void CDeadAgrunt::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW, gSkillData.agruntHealth/2);
	MonsterInitDead();
	pev->frame = 255;
}

void CDeadAgrunt::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	AgruntTraceAttack(this, pevInflictor, pevAttacker, damageInfo, vecDir, ptr);
}
