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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"scripted.h"
#include	"game.h"
#include	"combat.h"
#include	"followingmonster.h"
#include	"mod_features.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

#if FEATURE_PITDRONE

/*
 * In Opposing Force pitdrone spawned via monstermaker did not have spikes
 * That's probably a bug, because number of spikes is set in level editor,
 * so spawned pitdrones always had 0 spikes.
 * Having no spikes after spawn also prevented spike reloading.
 * Those who want to keep original Opposing Force behavior can set this constant to zero.
 */
#define FEATURE_PITDRONE_SPAWN_WITH_SPIKES 1

//=========================================================
// CPitDrone's spit projectile
//=========================================================
class CPitdroneSpike : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void EXPORT SpikeTouch(CBaseEntity *pOther);
	void EXPORT StartTrail();
	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles, EntityOverrides entityOverrides);

	static const NamedSoundScript hitWorldSoundScript;
	static const NamedSoundScript hitBodySoundScript;

	static const NamedVisual spikeVisual;
	static const NamedVisual trailVisual;
};

LINK_ENTITY_TO_CLASS(pitdronespike, CPitdroneSpike)

const NamedVisual CPitdroneSpike::spikeVisual = BuildVisual("Pitdrone.Spike")
		.Model("models/pit_drone_spike.mdl");

const NamedVisual CPitdroneSpike::trailVisual = BuildVisual("Pitdrone.SpikeTrail")
		.Model("sprites/spike_trail.spr")
		.BeamWidth(1)
		.Life(0.2f)
		.RenderColor(197, 194, 11)
		.Alpha(192);

const NamedSoundScript CPitdroneSpike::hitWorldSoundScript = {
	CHAN_WEAPON,
	{"weapons/xbow_hit1.wav"},
	VOL_NORM,
	ATTN_NORM,
	IntRange(120, 140),
	"PitDrone.SpikeHitWorld"
};

const NamedSoundScript CPitdroneSpike::hitBodySoundScript = {
	CHAN_WEAPON,
	{"weapons/xbow_hitbod1.wav", "weapons/xbow_hitbod2.wav"},
	VOL_NORM,
	ATTN_NORM,
	IntRange(120, 140),
	"PitDrone.SpikeHitBody"
};

void CPitdroneSpike::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("pitdronespike");

	pev->solid = SOLID_BBOX;

	ApplyVisual(GetVisual(spikeVisual));
	pev->frame = 0;

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
}

void CPitdroneSpike::Precache(void)
{
	RegisterVisual(spikeVisual);
	RegisterAndPrecacheSoundScript(hitWorldSoundScript);
	RegisterAndPrecacheSoundScript(hitBodySoundScript);
	RegisterVisual(trailVisual);
}

void CPitdroneSpike::SpikeTouch(CBaseEntity *pOther)
{
	SetTouch(NULL);
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;

	if (!pOther->pev->takedamage)
	{
		EmitSoundScript(hitWorldSoundScript);
		// make a horn in the wall

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 6.0f);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
			SetThink(&CBaseEntity::SUB_FadeOut);
		}
	}
	else
	{
		entvars_t	*pevOwner = VARS(pev->owner);
		pOther->TakeDamage(pev, pevOwner, DamageInfo(gSkillData.pitdroneDmgSpit, DMG_GENERIC).SetGibPolicy(GIB_NEVER));
		EmitSoundScript(hitBodySoundScript);
	}
}

void CPitdroneSpike::StartTrail()
{
	SendBeamFollow(entindex(), GetVisual(trailVisual));
	SetTouch(&CPitdroneSpike::SpikeTouch);
}

void CPitdroneSpike::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles, EntityOverrides entityOverrides)
{
	CPitdroneSpike *pSpit = GetClassPtr( (CPitdroneSpike *)NULL );
	pSpit->AssignEntityOverrides(entityOverrides);
	pSpit->Spawn();

	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->angles = vecAngles;
	pSpit->pev->owner = ENT( pevOwner );

	pSpit->SetThink(&CPitdroneSpike::StartTrail);
	pSpit->pev->nextthink = gpGlobals->time;
}

//
// PitDrone, main part.
//
#define HORNGROUP			1
#define PITDRONE_HORNS0		0
#define PITDRONE_HORNS1		1
#define PITDRONE_HORNS2		2
#define PITDRONE_HORNS3		3
#define PITDRONE_HORNS4		4
#define PITDRONE_HORNS5		5
#define PITDRONE_HORNS6		6
#define	PITDRONE_SPRINT_DIST			255
#define PITDRONE_MAX_HORNS	6
#define PITDRONE_GIB_COUNT	5

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_PDRONE_HURTHOP = LAST_FOLLOWINGMONSTER_SCHEDULE + 1,
	SCHED_PDRONE_SMELLFOOD,
	SCHED_PDRONE_EAT,
	SCHED_PDRONE_SNIFF_AND_EAT,
	SCHED_PDRONE_COVER_AND_RELOAD
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_PDRONE_HOPTURN = LAST_FOLLOWINGMONSTER_TASK + 1
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PIT_DRONE_AE_SPIT			( 1 )
// not sure what it is. It happens twice when pitdrone uses two claws at the same time
// once before 'throw' event and once after
#define PIT_DRONE_AE_ATTACK			( 2 )
#define PIT_DRONE_AE_SLASH			( 4 )
#define PIT_DRONE_AE_HOP			( 5 )
#define PIT_DRONE_AE_THROW			( 6 )
#define PIT_DRONE_AE_RELOAD			( 7 )

class CPitdrone : public CFollowingMonster
{
public:
	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("pitdrone"); }
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void SetYawSpeed(void);
	int DefaultISoundMask();
	void KeyValue(KeyValueData *pkvd);

	int DefaultClassify(void);
	const char* DefaultDisplayName() { return "Pit Drone"; }

	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	void IdleSound(void);
	void PainSound(void);
	void AlertSound(void);
	void DeathSound(void);
	void PlayUseSentence();
	void PlayUnUseSentence();
	void BodyChange(int horns);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& info) override;
	int IgnoreConditions(void);
	Schedule_t* GetSchedule(void);
	Schedule_t* GetScheduleOfType(int Type);
	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);
	void RunAI(void);
	void CheckAmmo();
	const char* DefaultGibModel() {
		return "models/pit_drone_gibs.mdl";
	}
	int DefaultGibCount() {
		return PITDRONE_GIB_COUNT;
	}

	CUSTOM_SCHEDULES

	virtual int DefaultSizeForGrapple() { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return Vector( -16.0f, -16.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 16.0f, 16.0f, 48.0f ); }

	float	m_flLastHurtTime;
	float	m_flNextSpitTime;// last time the PitDrone used the spit attack.
	float	m_flNextHopTime;
	int m_iInitialAmmo;
	bool shouldAttackWithLeftClaw;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static constexpr const char* attackMissSoundScript = "PitDrone.AttackMiss";
	static const NamedSoundScript attackHitSoundScript;

	static const NamedVisual tinySpitVisual;

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS(monster_pitdrone, CPitdrone)

TYPEDESCRIPTION	CPitdrone::m_SaveData[] =
{
	DEFINE_FIELD(CPitdrone, m_iInitialAmmo, FIELD_INTEGER),
	DEFINE_FIELD(CPitdrone, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CPitdrone, m_flNextSpitTime, FIELD_TIME),
	DEFINE_FIELD(CPitdrone, m_flNextHopTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CPitdrone, CFollowingMonster)

void CPitdrone::KeyValue(KeyValueData *pkvd)
{

	if (FStrEq(pkvd->szKeyName, "initammo"))
	{
		m_iInitialAmmo = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CFollowingMonster::KeyValue(pkvd);
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CPitdrone::IgnoreConditions(void)
{
	int iIgnore = CFollowingMonster::IgnoreConditions();

	if( gpGlobals->time - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the pitdrone care about stink.
		iIgnore |= bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	return iIgnore;
}

#define PITDRONE_ATTN_IDLE	1.5f

const NamedSoundScript CPitdrone::idleSoundScript = {
	CHAN_VOICE,
	{"pitdrone/pit_drone_idle1.wav", "pitdrone/pit_drone_idle2.wav", "pitdrone/pit_drone_idle3.wav"},
	1.0f,
	PITDRONE_ATTN_IDLE,
	"PitDrone.Idle"
};

const NamedSoundScript CPitdrone::alertSoundScript = {
	CHAN_VOICE,
	{"pitdrone/pit_drone_alert1.wav", "pitdrone/pit_drone_alert2.wav", "pitdrone/pit_drone_alert3.wav"},
	IntRange(140, 160),
	"PitDrone.Alert"
};

const NamedSoundScript CPitdrone::painSoundScript = {
	CHAN_VOICE,
	{"pitdrone/pit_drone_pain1.wav", "pitdrone/pit_drone_pain2.wav", "pitdrone/pit_drone_pain3.wav", "pitdrone/pit_drone_pain4.wav"},
	IntRange(85, 120),
	"PitDrone.Pain"
};

const NamedSoundScript CPitdrone::dieSoundScript = {
	CHAN_VOICE,
	{"pitdrone/pit_drone_die1.wav", "pitdrone/pit_drone_die2.wav", "pitdrone/pit_drone_die3.wav"},
	"PitDrone.Die"
};

const NamedSoundScript CPitdrone::attackHitSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_bite2.wav", "bullchicken/bc_bite3.wav"},
	0.7f,
	ATTN_NORM,
	IntRange(110, 120),
	"PitDrone.AttackHit"
};

const NamedVisual CPitdrone::tinySpitVisual = BuildVisual::Spray("Pitdrone.TinySpit")
		.Model("sprites/tinyspit.spr");

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CPitdrone::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& info)
{
	float flDist;
	Vector vecApex;

	// if the pitdrone is running, has an enemy, was hurt by the enemy, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != 0 && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > PITDRONE_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	m_flLastHurtTime = gpGlobals->time;

	return CFollowingMonster::TakeDamage(pevInflictor, pevAttacker, info);
}

//=========================================================
// CheckMeleeAttack1 - attack with both claws at the same time
//=========================================================
bool CPitdrone::CheckMeleeAttack1(float flDot, float flDist)
{
	// Give a better chance for MeleeAttack2
	if (RANDOM_LONG(0,2) == 0) {
		return CFollowingMonster::CheckMeleeAttack1(flDot, flDist);
	}
	return false;
}

//=========================================================
// CheckRangeAttack1 - spike attack
//=========================================================
bool CPitdrone::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_cAmmoLoaded <= 0)
	{
		return false;
	}
	if (IsMoving() && flDist >= 512)
	{
		// pitdone will far too far behind if he stops running to spit at this distance from the enemy.
		return false;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 1;
		}

		return true;
	}

	return false;

}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPitdrone::SetYawSpeed(void)
{
	int ys = 90;
	pev->yaw_speed = ys;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CPitdrone::DefaultISoundMask( void )
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_PLAYER;
}

void CPitdrone::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case PIT_DRONE_AE_THROW:
	{
		// SOUND HERE (in the pitdrone model)
		TraceHullAttackParams params;
		params.distance = 70.0f;
		params.knockForward = 100.0f;
		params.knockUp = 200.0f;
		params.damageInfo.damage = gSkillData.pitdroneDmgWhip;
		params.hitSoundScript = attackHitSoundScript; // croonchy bite sound
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		CBaseEntity *pHurt = PerformTraceHullAttack(params);

		if( pHurt )
		{
			// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
			UTIL_ScreenShake( pHurt->pev->origin, 25.0, 1.5, 0.7, 2 );
		}
	}
	break;

	case PIT_DRONE_AE_SLASH:
	{
		/* The same event is reused for both right and left claw attacks.
		 * Pitdrone always starts the attack with the right claw so we use shouldAttackWithLeftClaw to check which claw is used now.
		 */
		// SOUND HERE (in the pitdrone model)
		TraceHullAttackParams params;
		params.distance = 70.0f;
		params.punchAngle = Vector(5.0f, 0.0f, -18);
		params.knockRight =  -100;
		params.damageInfo.damage = gSkillData.pitdroneDmgWhip;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
		if (shouldAttackWithLeftClaw)
		{
			params.punchAngle.z = params.punchAngle.z * -1.0f;
			params.knockRight = params.knockRight * -1.0f;
		}

		PerformTraceHullAttack(params);

		shouldAttackWithLeftClaw = !shouldAttackWithLeftClaw;
	}
	break;

	case PIT_DRONE_AE_RELOAD:
	{
		if (m_iInitialAmmo >= 0)
			m_cAmmoLoaded = PITDRONE_MAX_HORNS;
		else
			m_cAmmoLoaded = 0;
		BodyChange(m_cAmmoLoaded);
		ClearConditions(bits_COND_NO_AMMO_LOADED);
	}
	break;

	case PIT_DRONE_AE_HOP:
	{
		float flGravity = g_psv_gravity->value;

		// throw the pitdrone up into the air on this frame.
		if( FBitSet( pev->flags, FL_ONGROUND ) )
		{
			pev->flags -= FL_ONGROUND;
		}

		// jump into air for 0.8 (24/30) seconds
		pev->velocity.z += ( 0.625 * flGravity ) * 0.5;
	}
	break;

	case PIT_DRONE_AE_SPIT:
	{
		m_cAmmoLoaded--;
		BodyChange(m_cAmmoLoaded);

		UTIL_MakeAimVectors(pev->angles);

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.
		const Vector vecSpitOffset = (gpGlobals->v_forward * 15 + gpGlobals->v_up * 36);
		const Vector vecSpitOrigin = (pev->origin + vecSpitOffset);

		const Vector vecSpitDir = SpitAtEnemy(vecSpitOrigin, 0.01f);

		// SOUND HERE! (in the pitdrone model)

		CPitdroneSpike::Shoot(pev, vecSpitOrigin, vecSpitDir * 900, UTIL_VecToAngles(vecSpitDir), GetProjectileOverrides());

		SendSpray(vecSpitOrigin, vecSpitDir, GetVisual(tinySpitVisual), 15, 210, 25);
	}
	break;
	case PIT_DRONE_AE_ATTACK:
		break;

	default:
		CFollowingMonster::HandleAnimEvent(pEvent);
	}
}

int	CPitdrone::DefaultClassify(void)
{
	return	CLASS_RACEX_PREDATOR;
}

void CPitdrone::BodyChange(int horns)
{
	if (horns <= 0)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS0);

	if (horns == 1)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS6);

	if (horns == 2)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS5);

	if (horns == 3)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS4);

	if (horns == 4)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS3);

	if (horns == 5)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS2);

	if (horns >= 6)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS1);

	return;
}
//=========================================================
// Spawn
//=========================================================
void CPitdrone::Spawn()
{
	Precache();

	SetMyModel("models/pit_drone.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects = 0;
	SetMyHealth( gSkillData.pitdroneHealth );
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(false);

	m_flNextSpitTime = gpGlobals->time;

	if (m_iInitialAmmo >= 0)
	{
		m_cAmmoLoaded = Q_min(m_iInitialAmmo, PITDRONE_MAX_HORNS);
#if FEATURE_PITDRONE_SPAWN_WITH_SPIKES
		if (!m_cAmmoLoaded) {
			m_cAmmoLoaded = PITDRONE_MAX_HORNS;
		}
#endif
	}
	BodyChange(m_cAmmoLoaded);
	FollowingMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitdrone::Precache()
{
	PrecacheMyModel("models/pit_drone.mdl");
	PrecacheMyGibModel(DefaultGibModel());
	RegisterVisual(tinySpitVisual);// client side spittle.

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript);

	if (!ShouldAutoPrecacheSounds())
	{
		// Used in model from Opposing Force
		PRECACHE_SOUND("pitdrone/pit_drone_melee_attack1.wav");
		PRECACHE_SOUND("pitdrone/pit_drone_melee_attack2.wav");

		PRECACHE_SOUND("pitdrone/pit_drone_attack_spike1.wav");
		//PRECACHE_SOUND("pitdrone/pit_drone_attack_spike2.wav");

		PRECACHE_SOUND("pitdrone/pit_drone_die1.wav");
		PRECACHE_SOUND("pitdrone/pit_drone_die2.wav");
		PRECACHE_SOUND("pitdrone/pit_drone_die3.wav");

		//PRECACHE_SOUND("pitdrone/pit_drone_communicate1.wav");
		//PRECACHE_SOUND("pitdrone/pit_drone_communicate2.wav");
		//PRECACHE_SOUND("pitdrone/pit_drone_communicate3.wav");
		//PRECACHE_SOUND("pitdrone/pit_drone_communicate4.wav");

		PRECACHE_SOUND("pitdrone/pit_drone_eat.wav");
		//PRECACHE_SOUND("pitdrone/pit_drone_hunt1.wav");
		//PRECACHE_SOUND("pitdrone/pit_drone_hunt2.wav");
		PRECACHE_SOUND("pitdrone/pit_drone_hunt3.wav");
	}

	UTIL_PrecacheOther("pitdronespike", GetProjectileOverrides());
}


//=========================================================
// IdleSound
//=========================================================

void CPitdrone::IdleSound(void)
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// PainSound
//=========================================================
void CPitdrone::PainSound(void)
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CPitdrone::AlertSound(void)
{
	EmitSoundScript(alertSoundScript);
}
//=========================================================
// DeathSound
//=========================================================
void CPitdrone::DeathSound(void)
{
	EmitSoundScript(dieSoundScript);
}

void CPitdrone::RunAI(void)
{
	// first, do base class stuff
	CFollowingMonster::RunAI();

	if (m_hEnemy != 0 && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < PITDRONE_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}
}

void CPitdrone::CheckAmmo( void )
{
	if( m_cAmmoLoaded <= 0 && m_iInitialAmmo >= 0 )
	{
		SetConditions( bits_COND_NO_AMMO_LOADED );
	}
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlPDroneRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slPDroneRangeAttack1[] =
{
	{
		tlPDroneRangeAttack1,
		ARRAYSIZE(tlPDroneRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"PDrone Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlPDroneChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CHASE_ENEMY_FAILED },
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slPDroneChaseEnemy[] =
{
	{
		tlPDroneChaseEnemy1,
		ARRAYSIZE(tlPDroneChaseEnemy1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
		bits_SOUND_MEAT,
		"PDrone Chase Enemy"
	},
};

Task_t tlPDroneHurtHop[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SOUND_WAKE, (float)0 },
	{ TASK_PDRONE_HOPTURN, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },// in case pitdrone didn't turn all the way in the air.
};

Schedule_t slPDroneHurtHop[] =
{
	{
		tlPDroneHurtHop,
		ARRAYSIZE( tlPDroneHurtHop ),
		0,
		0,
		"PDroneHurtHop"
	}
};


// PitDrone walks to something tasty and eats it.
Task_t tlPDroneEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the PitDrone can't get to the food
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneEat[] =
{
	{
		tlPDroneEat,
		ARRAYSIZE(tlPDroneEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"PDroneEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the PitDrone. This schedule plays a sniff animation before going to the source of food.
Task_t tlPDroneSniffAndEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the PitDrone can't get to the food
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.5f },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneSniffAndEat[] =
{
	{
		tlPDroneSniffAndEat,
		ARRAYSIZE(tlPDroneSniffAndEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"PDroneSniffAndEat"
	}
};

Task_t	tlPDroneHideReload[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RELOAD },
};

Schedule_t slPDroneHideReload[] =
{
	{
		tlPDroneHideReload,
		ARRAYSIZE( tlPDroneHideReload ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"PDroneHideReload"
	}
};

Task_t tlPDroneVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, 0.2f },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 40.0f },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneVictoryDance[] =
{
	{
		tlPDroneVictoryDance,
		ARRAYSIZE( tlPDroneVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"PDroneVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES(CPitdrone)
{
	slPDroneRangeAttack1,
	slPDroneChaseEnemy,
	slPDroneHurtHop,
	slPDroneEat,
	slPDroneSniffAndEat,
	slPDroneHideReload,
	slPDroneVictoryDance
};

IMPLEMENT_CUSTOM_SCHEDULES(CPitdrone, CFollowingMonster)

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CPitdrone::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	{
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
		break;
	}
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
	{
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && gpGlobals->time >= m_flNextHopTime )
		{
			return GetScheduleOfType( SCHED_PDRONE_HURTHOP );
		}

		if( HasConditions( bits_COND_ENEMY_DEAD ) && pev->health < pev->max_health )
		{
			return GetScheduleOfType( SCHED_VICTORY_DANCE );
		}

		if (HasConditions(bits_COND_SMELL_FOOD))
		{
			CSound		*pSound;

			pSound = PBestScent();

			if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
			{
				// scent is behind or occluded
				return GetScheduleOfType(SCHED_PDRONE_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_PDRONE_EAT);
		}

		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;

		break;
	}
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CFollowingMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if( HasConditions( bits_COND_NO_AMMO_LOADED ) && (m_iInitialAmmo >= 0) )
		{
			return GetScheduleOfType( SCHED_PDRONE_COVER_AND_RELOAD );
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK2);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	default:
		break;
	}

	return CFollowingMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CPitdrone::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slPDroneRangeAttack1[0];
		break;
	case SCHED_PDRONE_HURTHOP:
		return &slPDroneHurtHop[0];
		break;
	case SCHED_PDRONE_EAT:
		return &slPDroneEat[0];
		break;
	case SCHED_PDRONE_SNIFF_AND_EAT:
		return &slPDroneSniffAndEat[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slPDroneChaseEnemy[0];
		break;
	case SCHED_PDRONE_COVER_AND_RELOAD:
		return &slPDroneHideReload[0];
		break;
	case SCHED_VICTORY_DANCE:
		return slPDroneVictoryDance;
		break;
	}

	return CFollowingMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for PitDrone because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CPitdrone::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_PDRONE_HOPTURN:
		{
			m_flNextHopTime = gpGlobals->time + 5.0f;
			SetActivity( ACT_HOP );
			MakeIdealYaw( m_vecEnemyLKP );
			break;
		}
	default:
		CFollowingMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CPitdrone::RunTask(Task_t *pTask)
{
	switch( pTask->iTask )
	{
	case TASK_PDRONE_HOPTURN:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if( m_fSequenceFinished )
			{
				TaskComplete();
			}
			break;
		}
	default:
		{
			CFollowingMonster::RunTask( pTask );
			break;
		}
	}
}

void CPitdrone::PlayUseSentence()
{
	EmitSoundScript(idleSoundScript);
}

void CPitdrone::PlayUnUseSentence()
{
	EmitSoundScript(alertSoundScript);
}

class CDeadPitdrone : public CDeadMonster
{
public:
	void Spawn();
	void Precache();
	const char* DefaultModel() { return "models/pit_drone.mdl"; }
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("pitdrone"); }
	int	DefaultClassify ( void ) { return	CLASS_RACEX_PREDATOR; }
	const char* DefaultGibModel() {
		return "models/pit_drone_gibs.mdl";
	}
	int DefaultGibCount() {
		return PITDRONE_GIB_COUNT;
	}

	const char* getPos(int pos) const;
};

const char* CDeadPitdrone::getPos(int pos) const
{
	return "die2";
}

LINK_ENTITY_TO_CLASS( monster_pitdrone_dead, CDeadPitdrone )

void CDeadPitdrone::Precache()
{
	PrecacheMyModel(DefaultModel());
	PrecacheMyGibModel(DefaultGibModel());
}

void CDeadPitdrone::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW, gSkillData.pitdroneHealth/2);
	MonsterInitDead();
	pev->frame = 255;
}
#endif
