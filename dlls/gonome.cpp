/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//=========================================================
// Gonome.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"
#include	"bullsquid.h"
#include	"decals.h"
#include	"scripted.h"
#include	"animation.h"
#include	"studio.h"
#include	"game.h"
#include	"common_soundscripts.h"

#define		GONOME_MELEE_ATTACK_RADIUS		70

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define GONOME_AE_SLASH_RIGHT	( 1 )
#define GONOME_AE_SLASH_LEFT	( 2 )
#define GONOME_AE_SPIT			( 3 )
#define GONOME_AE_THROW			( 4 )

#define GONOME_AE_BITE1			( 19 )
#define GONOME_AE_BITE2			( 20 )
#define GONOME_AE_BITE3			( 21 )
#define GONOME_AE_BITE4			( 22 )

//=========================================================
// Gonome's guts projectile
//=========================================================
class CGonomeGuts : public CSquidSpit
{
public:
	void Spawn() override;
	void Precache() override;
	void Touch(CBaseEntity *pOther) override;

	static constexpr const char* spitTouchSoundScript = "Gonome.SpitTouch";
	static constexpr const char* spitHitSoundScript = "Gonome.SpitHit";

	static const NamedVisual gutsVisual;
};

LINK_ENTITY_TO_CLASS( gonomeguts, CGonomeGuts )

const NamedVisual CGonomeGuts::gutsVisual = BuildVisual::Animated("Gonome.Guts")
		.Model("sprites/bigspit.spr")
		.RenderProps(kRenderTransAlpha, Color3(255, 0, 0), 255)
		.Scale(0.5f);

void CGonomeGuts::Spawn()
{
	SpawnHelper("gonomeguts", gutsVisual);
	SetDefaultProjectileDamage(GetSkillValue("gonome_dmg_guts"));
}

void CGonomeGuts::Precache()
{
	RegisterVisualAsMineOwn(gutsVisual);
	RegisterAndPrecacheSoundScript(spitTouchSoundScript, NPC::spitTouchSoundScript);
	RegisterAndPrecacheSoundScript(spitHitSoundScript, NPC::spitHitSoundScript);
}

void CGonomeGuts::Touch( CBaseEntity *pOther )
{
	TraceResult tr;

	EmitSoundScript(spitTouchSoundScript);
	EmitSoundScript(spitHitSoundScript);

	if( !pOther->pev->takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		UTIL_BloodDrips( tr.vecEndPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	else
	{
		CBaseMonster* owner = GetMonsterPointer( pev->owner );
		entvars_t* pevAttacker = owner ? owner->pev : pev;
		pOther->TakeDamage( pev, pevAttacker, DamageInfo(GetProjectileDamage(), DMG_GENERIC) );
	}

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// CGonome
//=========================================================
class CGonome : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("gonome"); }

	int  DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Gonome"; }
	void SetYawSpeed() override;
	void HandleAnimEvent(MonsterEvent_t *pEvent) override;
	int IgnoreConditions() override;
	void IdleSound() override;
	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;
	void DeathSound() override;
	void AlertSound() override;

	bool CheckMeleeAttack2(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	int LookupActivity(int activity) override;
	void SetActivity( Activity NewActivity ) override;

	Schedule_t *GetSchedule() override;
	Schedule_t *GetScheduleOfType( int Type ) override;
	void RunTask(Task_t* pTask) override;

	TakeDamageResult TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo) override;
	void OnDying(bool gibbed, CBaseEntity* pKiller) override;
	void UpdateOnRemove() override;

	void UnlockPlayer();
	CGonomeGuts* GetGonomeGuts(const Vector& pos);
	void ClearGuts();

	int	Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	
	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static constexpr const char* attackHitSoundScript = "Gonome.AttackHit";
	static constexpr const char* attackMissSoundScript = "Gonome.AttackMiss";
	static const NamedSoundScript biteSoundScript;
	static const NamedSoundScript melee1SoundScript;
	static const NamedSoundScript melee2SoundScript;

	int DefaultSizeForGrapple() override { return GRAPPLE_LARGE; }
	bool IsDisplaceable() override { return true; }
	Vector DefaultMinHullSize() override { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() override { return VEC_HUMAN_HULL_MAX; }
protected:
	float m_flNextFlinch;
	float m_flNextThrowTime;// last time the gonome used the guts attack.
	CGonomeGuts* m_pGonomeGuts;
	EHANDLE m_lockedPlayer;
	bool m_meleeAttack2;
	bool m_playedAttackSound;
};

LINK_ENTITY_TO_CLASS(monster_gonome, CGonome)

const NamedSoundScript CGonome::idleSoundScript = {
	CHAN_VOICE,
	{"gonome/gonome_idle1.wav", "gonome/gonome_idle2.wav", "gonome/gonome_idle3.wav"},
	IntRange(95, 105),
	"Gonome.Idle"
};

const NamedSoundScript CGonome::alertSoundScript = {
	CHAN_VOICE,
	{"zombie/zo_alert10.wav", "zombie/zo_alert20.wav", "zombie/zo_alert30.wav"},
	IntRange(95, 104),
	"Gonome.Alert"
};

const NamedSoundScript CGonome::painSoundScript = {
	CHAN_VOICE,
	{"gonome/gonome_pain1.wav", "gonome/gonome_pain2.wav", "gonome/gonome_pain3.wav", "gonome/gonome_pain4.wav"},
	IntRange(95, 104),
	"Gonome.Pain"
};

const NamedSoundScript CGonome::dieSoundScript = {
	CHAN_VOICE,
	{"gonome/gonome_death2.wav", "gonome/gonome_death3.wav", "gonome/gonome_death4.wav"},
	"Gonome.Die"
};

const NamedSoundScript CGonome::biteSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_bite2.wav", "bullchicken/bc_bite3.wav"},
	IntRange(90, 110),
	"Gonome.Bite"
};

const NamedSoundScript CGonome::melee1SoundScript = {
	CHAN_BODY,
	{"gonome/gonome_melee1.wav"},
	"Gonome.Melee1"
};

const NamedSoundScript CGonome::melee2SoundScript = {
	CHAN_BODY,
	{"gonome/gonome_melee2.wav"},
	"Gonome.Melee2"
};

TYPEDESCRIPTION	CGonome::m_SaveData[] =
{
	DEFINE_FIELD( CGonome, m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( CGonome, m_flNextThrowTime, FIELD_TIME ),
	DEFINE_FIELD( CGonome, m_lockedPlayer, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CGonome, CBaseMonster )

void CGonome::OnDying(bool gibbed, CBaseEntity* pKiller)
{
	ClearGuts();
	UnlockPlayer();
	CBaseMonster::OnDying(gibbed, pKiller);
}

void CGonome::UpdateOnRemove()
{
	ClearGuts();
	UnlockPlayer();
	CBaseMonster::UpdateOnRemove();
}

void CGonome::UnlockPlayer()
{
	if (m_lockedPlayer != 0)
	{
		CBasePlayer* player = nullptr;
		if (m_lockedPlayer->IsPlayer())
			player = m_lockedPlayer.Entity<CBasePlayer>();

		if (player)
			player->EnableControl(true);

		m_lockedPlayer = 0;
	}
}

CGonomeGuts* CGonome::GetGonomeGuts(const Vector &pos)
{
	if (m_pGonomeGuts)
		return m_pGonomeGuts;
	CGonomeGuts *pGuts = GetClassPtr( (CGonomeGuts *)NULL );
	pGuts->AssignEntityOverrides(GetProjectileOverrides());
	pGuts->Spawn();

	UTIL_SetOrigin( pGuts->pev, pos );

	m_pGonomeGuts = pGuts;
	return m_pGonomeGuts;
}

void CGonome::ClearGuts()
{
	UTIL_RemoveAndClean(m_pGonomeGuts);
}

int CGonome::LookupActivity(int activity)
{
	if (activity == ACT_MELEE_ATTACK1 && m_hEnemy != 0)
	{
		// special melee animations
		int sequence = ACTIVITY_NOT_AVAILABLE;
		if ((pev->origin - m_hEnemy->pev->origin).IsLength2DGreaterThanOrEqual(48) )
		{
			m_meleeAttack2 = false;
			sequence = LookupSequence("attack1");
		}
		else
		{
			m_meleeAttack2 = true;
			sequence = LookupSequence("attack2");
		}
		if (sequence != ACTIVITY_NOT_AVAILABLE)
		{
			return sequence;
		}
	}
	else if (activity == ACT_RUN && m_hEnemy != 0)
	{
		int sequence = ACTIVITY_NOT_AVAILABLE;
		// special run animations
		if ((pev->origin - m_hEnemy->pev->origin).IsLengthLessThanOrEqual(512) )
		{
			sequence = LookupSequence("runshort");
		}
		else
		{
			sequence = LookupSequence("runlong");
		}
		if (sequence != ACTIVITY_NOT_AVAILABLE)
		{
			return sequence;
		}
	}
	return CBaseMonster::LookupActivity(activity);
}

void CGonome::SetActivity( Activity NewActivity )
{
	if (NewActivity != ACT_RANGE_ATTACK1)
	{
		ClearGuts();
	}
	if (NewActivity != ACT_MELEE_ATTACK1 || m_hEnemy == 0)
	{
		UnlockPlayer();
	}
	CBaseMonster::SetActivity(NewActivity);
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGonome::DefaultClassify()
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
TakeDamageResult CGonome::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo)
{
	if( damageInfo.type == DMG_BULLET && damageInfo.gibPolicy != GIB_NEVER )
	{
		const Vector vecDir = (pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5f).Normalize();
		float flForce = DamageForce( damageInfo.damage );
		pev->velocity = pev->velocity + vecDir * flForce;
#if 0
		// Take 15% damage from bullets
		flDamage *= 0.15;
#endif
	}
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, damageInfo);
}


//=========================================================
// CheckRangeAttack1
//=========================================================
bool CGonome::CheckRangeAttack1(float flDot, float flDist)
{
	// gonome won't try to shoot in short range
	if (flDist < 256)
		return false;

	if (IsMoving() && flDist >= 512)
	{
		// gonome will far too far behind if he stops running to spit at this distance from the enemy.
		return false;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextThrowTime)
	{
		if (m_hEnemy != 0)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return false;
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextThrowTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextThrowTime = gpGlobals->time + 0.5;
		}

		return true;
	}

	return false;
}

//=========================================================
// CheckMeleeAttack2 - both gonome's melee attacks are ACT_MELEE_ATTACK1
//=========================================================
bool CGonome::CheckMeleeAttack2(float flDot, float flDist)
{
	return false;
}


//=========================================================
// IdleSound 
//=========================================================
void CGonome::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// PainSound 
//=========================================================
PainSoundRule CGonome::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.allowWhenDying = true;
	rule.chance = 1.0f / 3.0f;
	return rule;
}

void CGonome::PainSound()
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CGonome::AlertSound()
{
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGonome::SetYawSpeed()
{
	pev->yaw_speed = 120;
}
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGonome::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GONOME_AE_SPIT:
	{
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		if (GetGonomeGuts(vecArmPos))
		{
			m_pGonomeGuts->pev->skin = entindex();
			m_pGonomeGuts->pev->body = 1;
			m_pGonomeGuts->pev->aiment = edict();
			m_pGonomeGuts->pev->movetype = MOVETYPE_FOLLOW;
		}
		UTIL_BloodDrips( vecArmPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	break;
	case GONOME_AE_THROW:
	{
		UTIL_MakeVectors(pev->angles);
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		if (GetGonomeGuts(vecArmPos))
		{
			const Vector vecSpitDir = SpitAtEnemy(vecArmPos);

			m_pGonomeGuts->pev->body = 0;
			m_pGonomeGuts->pev->skin = 0;
			m_pGonomeGuts->pev->owner = ENT( pev );
			m_pGonomeGuts->pev->aiment = 0;
			m_pGonomeGuts->pev->movetype = MOVETYPE_FLY;
			m_pGonomeGuts->pev->velocity = vecSpitDir * 900;
			m_pGonomeGuts->SetThink( &CGonomeGuts::Animate );
			m_pGonomeGuts->pev->nextthink = gpGlobals->time + 0.1;
			UTIL_SetOrigin(m_pGonomeGuts->pev, vecArmPos);

			m_pGonomeGuts = 0;
		}
		UTIL_BloodDrips( vecArmPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	break;

	case GONOME_AE_SLASH_LEFT:
	{
		TraceHullAttackParams params;
		params.distance = GONOME_MELEE_ATTACK_RADIUS;
		params.punchAngle.x = 5;
		params.punchAngle.z = 9;
		params.knockRight = 25.0f;
		params.damageInfo.damage = GetSkillValue("gonome_dmg_one_slash");
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
	break;

	case GONOME_AE_SLASH_RIGHT:
	{
		TraceHullAttackParams params;
		params.distance = GONOME_MELEE_ATTACK_RADIUS;
		params.punchAngle.x = 5;
		params.punchAngle.z = -9;
		params.knockRight = -25.0f;
		params.damageInfo.damage = GetSkillValue("gonome_dmg_one_slash");
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
	break;

	case GONOME_AE_BITE1:
	case GONOME_AE_BITE2:
	case GONOME_AE_BITE3:
	case GONOME_AE_BITE4:
		{
			TraceHullAttackParams params;
			params.distance = GONOME_MELEE_ATTACK_RADIUS;
			params.punchAngle.x = 9;
			params.knockForward = -25.0f;
			params.damageInfo.damage = GetSkillValue("gonome_dmg_one_bite");
			if (pEvent->event == GONOME_AE_BITE4)
			{
				params.punchAngle.x = 15;
				params.knockForward = -75.0f;
			}
			params.hitSoundScript = biteSoundScript; // croonchy bite sound
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			CBaseEntity *pHurt = PerformTraceHullAttack(params);

			if (pHurt)
			{
				if (pEvent->event == GONOME_AE_BITE4)
				{
					UnlockPlayer();
				}
				else if (pHurt->IsPlayer() && pHurt->IsAlive() && GetSkillValue("gonome_lock_player"))
				{
					if (m_lockedPlayer == 0)
					{
						CBasePlayer* player = (CBasePlayer*)pHurt;
						player->EnableControl(false);
						m_lockedPlayer = player;
					}
				}
			}
		}
		break;



	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

#define GONOME_FLINCH_DELAY 2

int CGonome::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity == ACT_RANGE_ATTACK1)
	{
		iIgnore |= bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_TOOFAR | bits_COND_ENEMY_OCCLUDED;
	}
	else if( m_Activity == ACT_MELEE_ATTACK1 )
	{
		if( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + GONOME_FLINCH_DELAY;
	}

	return iIgnore;
}

//=========================================================
// Spawn
//=========================================================
void CGonome::Spawn()
{
	Precache();

	SetMyModel("models/gonome.mdl");
	SetMySize();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_YELLOW );
	pev->effects = 0;
	SetMyHealth( GetSkillValue("gonome_health") );
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	SetMyCanOpenDoors(true);

	m_flNextThrowTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGonome::Precache()
{
	PrecacheMyModel("models/gonome.mdl");
	PrecacheMyGibModel();

	UTIL_PrecacheOther("gonomeguts", GetProjectileOverrides());

	RegisterAndPrecacheSoundScript(NPC::swishSoundScript);// because we use the basemonster SWIPE animation event

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
	RegisterAndPrecacheSoundScript(biteSoundScript);
	RegisterAndPrecacheSoundScript(melee1SoundScript);
	RegisterAndPrecacheSoundScript(melee2SoundScript);

	if (!ShouldAutoPrecacheSounds())
	{
		// Used in model from Opposing Force
		PRECACHE_SOUND("gonome/gonome_idle1.wav");
		PRECACHE_SOUND("gonome/gonome_melee1.wav");
		PRECACHE_SOUND("gonome/gonome_melee2.wav");
		PRECACHE_SOUND("gonome/gonome_death2.wav");
		PRECACHE_SOUND("gonome/gonome_death3.wav");
		PRECACHE_SOUND("gonome/gonome_death4.wav");
		PRECACHE_SOUND("gonome/gonome_run.wav");
		PRECACHE_SOUND("gonome/gonome_eat.wav");
		PRECACHE_SOUND("gonome/gonome_jumpattack.wav");
	}
}

//=========================================================
// DeathSound
//=========================================================
void CGonome::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CGonome::GetSchedule()
{
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

			Schedule_t* regenSchedule = GetRegenerationSchedule();
			if (regenSchedule)
				return regenSchedule;

			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
			}

			return GetScheduleOfType( SCHED_CHASE_ENEMY );
			break;
		}
	default:
			break;
	}

	return CBaseMonster::GetSchedule();
}

// primary range attack
Task_t tlGonomeRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slGonomeRangeAttack1[] =
{
	{
		tlGonomeRangeAttack1,
		ARRAYSIZE( tlGonomeRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Gonome Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlGonomeChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slGonomeChaseEnemy[] =
{
	{
		tlGonomeChaseEnemy1,
		ARRAYSIZE( tlGonomeChaseEnemy1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED,
		0,
		"Gonome Chase Enemy"
	},
};

// victory dance (eating body)
Task_t tlGonomeVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, 0.1f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,	40.0f },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
};

Schedule_t slGonomeVictoryDance[] =
{
	{
		tlGonomeVictoryDance,
		ARRAYSIZE( tlGonomeVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"GonomeVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES( CGonome )
{
	slGonomeRangeAttack1,
	slGonomeChaseEnemy,
	slGonomeVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGonome, CBaseMonster )

Schedule_t* CGonome::GetScheduleOfType(int Type)
{
	switch ( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slGonomeRangeAttack1[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slGonomeChaseEnemy[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slGonomeVictoryDance[0];
		break;
	default:
		break;
	}
	return CBaseMonster::GetScheduleOfType(Type);
}

void CGonome::RunTask(Task_t *pTask)
{
	// HACK to stop Gonome from playing attack sound twice
	if (pTask->iTask == TASK_MELEE_ATTACK1)
	{
		if (!m_playedAttackSound)
		{
			if (m_meleeAttack2)
			{
				EmitSoundScript(melee2SoundScript);
			}
			else
			{
				EmitSoundScript(melee1SoundScript);
			}
			m_playedAttackSound = true;
		}
	}
	else
	{
		m_playedAttackSound = false;
	}
	CBaseMonster::RunTask(pTask);
}

//=========================================================
// DEAD GONOME PROP
//=========================================================
class CDeadGonome : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/gonome.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("gonome"); }
	int	DefaultClassify() override { return	CLASS_ALIEN_MONSTER; }
	const char* getPos(int pos) const override;
	static const char *m_szPoses[3];
};

const char *CDeadGonome::m_szPoses[] = { "dead_on_stomach1", "dead_on_back", "dead_on_side" };

const char* CDeadGonome::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS(monster_gonome_dead, CDeadGonome)

//=========================================================
// ********** DeadGonome SPAWN **********
//=========================================================
void CDeadGonome::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
}
