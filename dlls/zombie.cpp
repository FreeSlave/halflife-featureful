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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"common_soundscripts.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

#define ZOMBIE_FLINCH_DELAY		2		// at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Zombie"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int IgnoreConditions() override;
	Schedule_t* GetScheduleOfType(int Type) override;

	float m_flNextFlinch;

	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;
	void DeathSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void AttackSound();

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript attackSoundScript;
	static constexpr const char* attackHitSoundScript = "Zombie.AttackHit";
	static constexpr const char* attackMissSoundScript = "Zombie.AttackMiss";

	// No range attacks
	bool CheckRangeAttack1( float flDot, float flDist ) override { return false; }
	bool CheckRangeAttack2( float flDot, float flDist ) override { return false; }
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;

	int DefaultSizeForGrapple() override { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() override { return true; }
	Vector DefaultMinHullSize() override { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() override { return VEC_HUMAN_HULL_MAX; }
	virtual float OneSlashDamage() { return GetSkillValue("zombie_dmg_one_slash"); }
	virtual float BothSlashDamage() { return GetSkillValue("zombie_dmg_both_slash"); }
protected:
	void SlashAttack(const TraceHullAttackParams& params);
	void ZombieSpawnHelper(const char* modelName, float health);
	void PrecacheSounds();
};

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie )

const NamedSoundScript CZombie::idleSoundScript = {
	CHAN_VOICE,
	{"zombie/zo_idle1.wav", "zombie/zo_idle2.wav", "zombie/zo_idle3.wav", "zombie/zo_idle4.wav"},
	IntRange(95, 104),
	"Zombie.Idle"
};

const NamedSoundScript CZombie::alertSoundScript = {
	CHAN_VOICE,
	{"zombie/zo_alert10.wav", "zombie/zo_alert20.wav", "zombie/zo_alert30.wav"},
	IntRange(95, 104),
	"Zombie.Alert"
};

const NamedSoundScript CZombie::painSoundScript = {
	CHAN_VOICE,
	{"zombie/zo_pain1.wav", "zombie/zo_pain2.wav"},
	IntRange(95, 104),
	"Zombie.Pain"
};

const NamedSoundScript CZombie::dieSoundScript = {
	CHAN_VOICE,
	{},
	"Zombie.Die"
};

const NamedSoundScript CZombie::attackSoundScript = {
	CHAN_VOICE,
	{"zombie/zo_attack1.wav", "zombie/zo_attack2.wav"},
	IntRange(95, 104),
	"Zombie.Attack"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CZombie::DefaultClassify()
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed()
{
	pev->yaw_speed = 120;
}

TakeDamageResult CZombie::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	if( damageInfo.type == DMG_BULLET && damageInfo.gibPolicy != GIB_NEVER )
	{
		const Vector vecDir = (pev->origin - ( pevInflictor->absmin + pevInflictor->absmax ) * 0.5f).Normalize();
		float flForce = DamageForce( damageInfo.damage );
		pev->velocity = pev->velocity + vecDir * flForce;
#if 0
		// Take 30% damage from bullets
		flDamage *= 0.3f;
#endif
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, damageInfo );
}

PainSoundRule CZombie::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.allowWhenDying = true;
	rule.chance = 1.0f / 3.0f;
	return rule;
}

void CZombie::PainSound()
{
	EmitSoundScript(painSoundScript);
}

void CZombie::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

void CZombie::AlertSound()
{
	EmitSoundScript(alertSoundScript);
}

void CZombie::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

void CZombie::AttackSound()
{
	EmitSoundScript(attackSoundScript);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::SlashAttack(const TraceHullAttackParams& params)
{
	PerformTraceHullAttack(params);

	if (RANDOM_LONG(0,1))
		AttackSound();
}

void CZombie::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			TraceHullAttackParams params;
			params.damageInfo.damage = OneSlashDamage();
			params.knockRight = -100;
			params.punchAngle.z = -18;
			params.punchAngle.x = 5;
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
			SlashAttack(params);
		}
		break;

		case ZOMBIE_AE_ATTACK_LEFT:
		{
			TraceHullAttackParams params;
			params.damageInfo.damage = OneSlashDamage();
			params.knockRight = 100;
			params.punchAngle.z = 18;
			params.punchAngle.x = 5;
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
			SlashAttack(params);
		}
		break;

		case ZOMBIE_AE_ATTACK_BOTH:
		{
			TraceHullAttackParams params;
			params.damageInfo.damage = BothSlashDamage();
			params.knockForward = -100;
			params.punchAngle.x = 5;
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
			SlashAttack(params);
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CZombie::ZombieSpawnHelper(const char* modelName, float health)
{
	SetMyModel( modelName );
	SetMySize();

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( health );
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	SetMyCanOpenDoors(true);

	MonsterInit();
}

void CZombie::Spawn()
{
	Precache();
	ZombieSpawnHelper("models/zombie.mdl", GetSkillValue("zombie_health"));
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache()
{
	PrecacheMyModel("models/zombie.mdl");
	PrecacheMyGibModel();
	PrecacheSounds();
}

void CZombie::PrecacheSounds()
{
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CZombie::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( ( m_Activity == ACT_MELEE_ATTACK1 ) || ( m_Activity == ACT_MELEE_ATTACK1 ) )
	{
#if 0
		if( pev->health < 20 )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE| bits_COND_HEAVY_DAMAGE );
		else
#endif
		if( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}

Schedule_t* CZombie::GetScheduleOfType(int Type)
{
	if (Type == SCHED_CHASE_ENEMY_FAILED && HasMemory(bits_MEMORY_BLOCKER_IS_ENEMY))
	{
		return CBaseMonster::GetScheduleOfType(SCHED_CHASE_ENEMY);
	}
	return CBaseMonster::GetScheduleOfType(Type);
}

class CDeadZombie : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/zombie.mdl"; }
	int	DefaultClassify() override { return	CLASS_ALIEN_MONSTER; }

	const char* getPos(int pos) const override;
	static const char *m_szPoses[2];
};

const char *CDeadZombie::m_szPoses[] = { "dieheadshot", "dieforward" };

const char* CDeadZombie::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_zombie_dead, CDeadZombie )

void CDeadZombie::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
	pev->frame = 255;
}

class CZombieBarney : public CZombie
{
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("zombie_barney"); }
	const char* DefaultDisplayName() override { return "Zombie Barney"; }
	float OneSlashDamage() override { return GetSkillValue("zombie_barney_dmg_one_slash"); }
	float BothSlashDamage() override { return GetSkillValue("zombie_barney_dmg_both_slash"); }
};

LINK_ENTITY_TO_CLASS( monster_zombie_barney, CZombieBarney )

void CZombieBarney::Spawn()
{
	Precache();
	ZombieSpawnHelper("models/zombie_barney.mdl", GetSkillValue("zombie_barney_health"));
}

void CZombieBarney::Precache()
{
	PrecacheMyModel("models/zombie_barney.mdl");
	PrecacheMyGibModel();
	PrecacheSounds();
}

class CDeadZombieBarney : public CDeadZombie
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/zombie_barney.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("zombie_barney"); }
};

LINK_ENTITY_TO_CLASS( monster_zombie_barney_dead, CDeadZombieBarney )

void CDeadZombieBarney::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
	pev->frame = 255;
}

class CZombieSoldier : public CZombie
{
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("zombie_soldier"); }
	const char* DefaultDisplayName() override { return "Zombie Soldier"; }
	float OneSlashDamage() override { return GetSkillValue("zombie_soldier_dmg_one_slash"); }
	float BothSlashDamage() override { return GetSkillValue("zombie_soldier_dmg_both_slash"); }
};

LINK_ENTITY_TO_CLASS( monster_zombie_soldier, CZombieSoldier )

void CZombieSoldier::Spawn()
{
	Precache();
	ZombieSpawnHelper("models/zombie_soldier.mdl", GetSkillValue("zombie_soldier_health"));
}

void CZombieSoldier::Precache()
{
	PrecacheMyModel("models/zombie_soldier.mdl");
	PrecacheMyGibModel();
	PrecacheSounds();
}

class CDeadZombieSoldier : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/zombie_soldier.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("zombie_soldier"); }
	int	DefaultClassify () override { return	CLASS_ALIEN_MONSTER; }

	const char* getPos(int pos) const override;
	static const char *m_szPoses[2];
};

const char *CDeadZombieSoldier::m_szPoses[] = { "dead_on_back", "dead_on_stomach" };

const char* CDeadZombieSoldier::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_zombie_soldier_dead, CDeadZombieSoldier )

void CDeadZombieSoldier::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
}
