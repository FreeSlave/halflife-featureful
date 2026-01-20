/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include	"soundent.h"
#include	"combat.h"
#include	"effects.h"
#include	"player.h"
#include	"decals.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

#define PITWORM_ATTN 0.1f
#define NUM_PITWORM_LEVELS		4

class CPitWorm : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("pitworm"); }
	int  DefaultClassify() override;
	int	ObjectCaps() override { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void SetObjectCollisionBox() override
	{
		SetMyObjectCollisionBox(Vector( -400, -400, 0 ), Vector( 400, 400, 850 ));
	}
	bool FVisible(CBaseEntity* pEntity, CBaseEntity** ppSightBlocker = nullptr) override;
	bool FVisible(const Vector& vecOrigin, CBaseEntity** ppSightBlocker = nullptr) override;

	void IdleSound() override;
	void AlertSound() override;
	void DeathSound() override;
	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;

	int		Save(CSave &save) override;
	int		Restore(CRestore &restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void HandleAnimEvent(MonsterEvent_t *pEvent) override;
	TakeDamageResult TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo) override;
	void TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr) override;

	void EXPORT StartupThink();
	void EXPORT DyingThink();
	void EXPORT StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT NullThink();
	void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT	HuntThink();
	void EXPORT HitTouch(CBaseEntity* pOther);

	void LockTopLevel();
	bool ClawAttack();
	void ShootBeam();
	void StrafeBeam();
	void ChangeLevel();
	void TrackEnemy();

	void NextActivity();

	void EyeLight(const Vector& vecEyePos);
	void BeamEffect(TraceResult& tr);

	int DefaultSizeForGrapple() override { return GRAPPLE_LARGE; }

	Vector m_vecTarget;
	Vector m_posTarget;
	Vector m_vecDesired;
	Vector m_posDesired;

	float m_offsetBeam;
	Vector m_posBeam;
	Vector m_vecBeam;
	Vector m_angleBeam;
	float m_flBeamExpireTime;
	float m_flBeamDir;

	float m_flTorsoYaw;
	float m_flHeadYaw;
	float m_flHeadPitch;
	float m_flIdealTorsoYaw;
	float m_flIdealHeadYaw;
	float m_flIdealHeadPitch;

	float m_flLevels[NUM_PITWORM_LEVELS];
	float m_flTargetLevels[NUM_PITWORM_LEVELS];

	float m_flLastSeen;

	int m_iLevel;
	float m_flLevelSpeed;

	CBeam* m_pBeam;
	CSprite* m_pSprite;

	bool m_fAttacking;
	bool m_fLockHeight;
	bool m_fLockYaw;

	int m_iWasHit;
	float m_flTakeHitTime;

	float m_flHitTime;
	float m_flNextMeleeTime;
	float m_flNextRangeTime;
	float m_flDeathStartTime;

	bool m_fFirstSighting;
	bool m_fTopLevelLocked;

	float m_flLastBlinkTime;
	float m_flLastBlinkInterval;
	float m_flLastEventTime;

	static const NamedSoundScript hitGroundSoundScript;
	static const NamedSoundScript angrySoundScript;
	static const NamedSoundScript blastSoundScript;
	static const NamedSoundScript swipeSoundScript;
	static const NamedSoundScript shootSoundScript;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript dieSoundScript;

	static constexpr const char* attackHitSoundScript = "PitWorm.AttackHit";

	static const NamedVisual beamVisual;
	static const NamedVisual eyeGlowVisual;
	static const NamedVisual eyeLightVisual;
};

LINK_ENTITY_TO_CLASS(monster_pitworm, CPitWorm)
LINK_ENTITY_TO_CLASS(monster_pitworm_up, CPitWorm)

#define PITWORM_EYE_OFFSET				Vector(0, 0, 300)

#define PITWORM_CONTROLLER_EYE_YAW		0
#define PITWORM_CONTROLLER_EYE_PITCH	1
#define PITWORM_CONTROLLER_BODY_YAW		2

#define PITWORM_EYE_PITCH_MIN	-45
#define PITWORM_EYE_PITCH_MAX	 45
#define PITWORM_EYE_YAW_MIN		-45
#define PITWORM_EYE_YAW_MAX		 45

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PITWORM_AE_SWIPE			( 1 )
#define PITWORM_AE_EYEBLAST_START	( 2 )
#define PITWORM_AE_EYEBLAST_END		( 4 )

//=========================================================
// Save & Restore
//=========================================================
TYPEDESCRIPTION	CPitWorm::m_SaveData[] =
{
	DEFINE_FIELD(CPitWorm, m_vecTarget, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_posTarget, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_vecDesired, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_posDesired, FIELD_POSITION_VECTOR),

	DEFINE_FIELD(CPitWorm, m_offsetBeam, FIELD_FLOAT),
	DEFINE_FIELD(CPitWorm, m_posBeam, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_vecBeam, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_angleBeam, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_flBeamExpireTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flBeamDir, FIELD_FLOAT),

	DEFINE_ARRAY(CPitWorm, m_flLevels, FIELD_FLOAT, NUM_PITWORM_LEVELS),
	DEFINE_ARRAY(CPitWorm, m_flTargetLevels, FIELD_FLOAT, NUM_PITWORM_LEVELS),

	DEFINE_FIELD(CPitWorm, m_flLastSeen, FIELD_TIME),

	DEFINE_FIELD(CPitWorm, m_iLevel, FIELD_INTEGER),
	DEFINE_FIELD(CPitWorm, m_flLevelSpeed, FIELD_FLOAT),

	DEFINE_FIELD(CPitWorm, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CPitWorm, m_pSprite, FIELD_CLASSPTR),

	DEFINE_FIELD(CPitWorm, m_fAttacking, FIELD_BOOLEAN),
	DEFINE_FIELD(CPitWorm, m_fLockHeight, FIELD_BOOLEAN),
	DEFINE_FIELD(CPitWorm, m_fLockYaw, FIELD_BOOLEAN),

	DEFINE_FIELD(CPitWorm, m_iWasHit, FIELD_INTEGER),
	DEFINE_FIELD(CPitWorm, m_flTakeHitTime, FIELD_TIME),

	DEFINE_FIELD(CPitWorm, m_flHitTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flNextMeleeTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flNextRangeTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flDeathStartTime, FIELD_TIME),

	DEFINE_FIELD(CPitWorm, m_fFirstSighting, FIELD_BOOLEAN),
	DEFINE_FIELD(CPitWorm, m_fTopLevelLocked, FIELD_BOOLEAN),

	DEFINE_FIELD(CPitWorm, m_flLastBlinkTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flLastBlinkInterval, FIELD_FLOAT),
	DEFINE_FIELD(CPitWorm, m_flLastEventTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CPitWorm, CBaseMonster)

const NamedSoundScript CPitWorm::hitGroundSoundScript = {
	CHAN_WEAPON,
	{"tentacle/te_strike1.wav", "tentacle/te_strike2.wav"},
	IntRange(95, 105),
	"PitWorm.HitGround"
};

const NamedSoundScript CPitWorm::angrySoundScript = {
	CHAN_VOICE,
	{"pitworm/pit_worm_angry1.wav", "pitworm/pit_worm_angry2.wav", "pitworm/pit_worm_angry3.wav"},
	VOL_NORM,
	PITWORM_ATTN,
	"PitWorm.Angry"
};

const NamedSoundScript CPitWorm::blastSoundScript = {
	CHAN_VOICE,
	{"pitworm/pit_worm_attack_eyeblast.wav"},
	VOL_NORM,
	PITWORM_ATTN,
	"PitWorm.Blast"
};

const NamedSoundScript CPitWorm::swipeSoundScript = {
	CHAN_VOICE,
	{"pitworm/pit_worm_attack_swipe1.wav", "pitworm/pit_worm_attack_swipe2.wav", "pitworm/pit_worm_attack_swipe3.wav"},
	VOL_NORM,
	PITWORM_ATTN,
	"PitWorm.Swipe"
};

const NamedSoundScript CPitWorm::shootSoundScript = {
	CHAN_WEAPON,
	{"debris/beamstart3.wav", "debris/beamstart8.wav"},
	IntRange(95, 105),
	"PitWorm.Shoot"
};

const NamedSoundScript CPitWorm::painSoundScript = {
	CHAN_VOICE,
	{"pitworm/pit_worm_flinch1.wav", "pitworm/pit_worm_flinch2.wav"},
	VOL_NORM,
	PITWORM_ATTN,
	"PitWorm.Pain"
};

const NamedSoundScript CPitWorm::alertSoundScript = {
	CHAN_VOICE,
	{"pitworm/pit_worm_alert.wav"},
	VOL_NORM,
	PITWORM_ATTN,
	"PitWorm.Alert"
};

const NamedSoundScript CPitWorm::idleSoundScript = {
	CHAN_VOICE,
	{"pitworm/pit_worm_idle1.wav", "pitworm/pit_worm_idle2.wav", "pitworm/pit_worm_idle3.wav"},
	VOL_NORM,
	PITWORM_ATTN,
	"PitWorm.Idle"
};

const NamedSoundScript CPitWorm::dieSoundScript = {
	CHAN_VOICE,
	{"pitworm/pit_worm_death.wav"},
	VOL_NORM,
	PITWORM_ATTN,
	"PitWorm.Die"
};

const NamedVisual CPitWorm::beamVisual = BuildVisual("PitWorm.Beam")
	.Model("sprites/laserbeam.spr")
	.BeamWidth(80)
	.RenderColor(0, 255, 32)
	.Alpha(128);

const NamedVisual CPitWorm::eyeGlowVisual = BuildVisual("PitWorm.EyeGlow")
	.Model("sprites/tele1.spr")
	.RenderProps(kRenderGlow, Color3(0, 255, 0), 255, kRenderFxNoDissipation)
	.Scale(0.75f)
	.Framerate(10.0f);

const NamedVisual CPitWorm::eyeLightVisual = BuildVisual("PitWorm.EyeLight")
	.RenderColor(128, 255, 128)
	.Radius(128)
	.Life(0.1f)
	.Decay(2);

//=========================================================
// Spawn
//=========================================================
void CPitWorm::Spawn()
{
	Precache();
	SetMyModel("models/pit_worm_up.mdl");

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_MONSTER|FL_FLY;
	pev->takedamage = DAMAGE_AIM;

	SetMyHealth( GetSkillValue("pitworm_health") );
	pev->max_health = pev->health;

	pev->view_ofs = PITWORM_EYE_OFFSET;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyFieldOfView( 0.5f );

	pev->sequence = 0;
	ResetSequenceInfo();

	m_flTorsoYaw = 0;
	m_flHeadYaw = 0;
	m_flHeadPitch = 0;
	m_flIdealTorsoYaw = 0;
	m_flIdealHeadYaw = 0;
	m_flIdealHeadPitch = 0;

	InitBoneControllers();

	SetThink(&CPitWorm::StartupThink);
	SetTouch(&CPitWorm::HitTouch);
	pev->nextthink = gpGlobals->time + 0.1;

	m_vecDesired = Vector(1,0,0);
	m_posDesired = pev->origin;

	m_fAttacking = false;
	m_fLockHeight = false;
	m_fFirstSighting = false;
	m_flBeamExpireTime = gpGlobals->time;
	m_iLevel = 0;
	m_fLockYaw = false;

	m_iWasHit = 0;
	m_flTakeHitTime = 0;
	m_flHitTime = 0;
	m_flLevelSpeed = 10;

	m_fTopLevelLocked = false;
	m_flLastBlinkTime = gpGlobals->time;
	m_flLastBlinkInterval = gpGlobals->time;
	m_flLastEventTime = gpGlobals->time;
	m_flTargetLevels[3] = pev->origin.z;
	m_flLevels[3] = pev->origin.z - 350.0;
	m_flTargetLevels[2] = pev->origin.z;
	m_flLevels[2] = pev->origin.z - 300.0;
	m_flTargetLevels[1] = pev->origin.z;
	m_flLevels[1] = pev->origin.z - 300.0;
	m_flTargetLevels[0] = pev->origin.z;
	m_flLevels[0] = pev->origin.z - 300.0;;
	m_pBeam = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitWorm::Precache()
{
	PrecacheMyModel("models/pit_worm_up.mdl");

	PRECACHE_SOUND("pitworm/pit_worm_attack_eyeblast_impact.wav");

	RegisterAndPrecacheSoundScript(hitGroundSoundScript);
	RegisterAndPrecacheSoundScript(angrySoundScript);
	RegisterAndPrecacheSoundScript(blastSoundScript);
	RegisterAndPrecacheSoundScript(swipeSoundScript);
	RegisterAndPrecacheSoundScript(shootSoundScript);

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);

	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);

	RegisterVisual(beamVisual);
	RegisterVisual(eyeGlowVisual);
	RegisterVisual(eyeLightVisual);
}

bool CPitWorm::FVisible(CBaseEntity *pEntity, CBaseEntity** ppSightBlocker)
{
	if( FBitSet( pEntity->pev->flags, FL_NOTARGET ) )
		return false;

	if( LineOfSightSeparatedByWaterSurface(pev->waterlevel, pEntity->pev->waterlevel) )
		return false;

	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecLookerAngle;

	GetAttachment(0, vecLookerOrigin, vecLookerAngle);
	UTIL_TraceLine( vecLookerOrigin, pEntity->EyePosition(), ignore_monsters, ignore_glass, ENT( pev ), &tr );

	return tr.flFraction == 1.0;
}

bool CPitWorm::FVisible(const Vector& vecOrigin, CBaseEntity** ppSightBlocker)
{
	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecLookerAngle;

	GetAttachment(0, vecLookerOrigin, vecLookerAngle);
	UTIL_TraceLine( vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT( pev ), &tr );
	return tr.flFraction == 1.0;
}

//=========================================================
// Classify
//=========================================================
int CPitWorm::DefaultClassify()
{
	return CLASS_RACEX_SHOCK;
}

//=========================================================
// IdleSound
//=========================================================
void CPitWorm::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CPitWorm::AlertSound()
{
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// DeathSound
//=========================================================
void CPitWorm::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

PainSoundRule CPitWorm::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = FloatRange{2, 5};
	return rule;
}

void CPitWorm::PainSound()
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// HandleAnimEvent
//=========================================================
void CPitWorm::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case PITWORM_AE_SWIPE:	// bang
	{
		EmitSoundScript(hitGroundSoundScript);

		if (pev->sequence == 2)
			UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 100);
		else
			UTIL_ScreenShake(pev->origin, 4.0, 3.0, 1.0, 750.0);
	}
		break;
	case PITWORM_AE_EYEBLAST_START: // start killing swing
	{
		if ( gpGlobals->time - m_flLastEventTime >= 1.1 )
		{
			if (m_hEnemy)
			{
				m_posBeam = m_hEnemy->pev->origin;
				m_posBeam.z += 24;

				Vector vecEyePos, vecEyeAng;
				GetAttachment(0, vecEyePos, vecEyeAng);

				m_vecBeam = (m_posBeam - vecEyePos).Normalize();
				m_angleBeam = UTIL_VecToAngles(m_vecBeam);
				UTIL_MakeVectors(m_angleBeam);
				ShootBeam();
				m_fLockYaw = true;
			}
		}
	}
		break;
	case PITWORM_AE_EYEBLAST_END: // end killing swing
	{
		m_fLockYaw = true;
	}
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);

	}
}

void CPitWorm::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	if ( ptr->iHitgroup == HITGROUP_HEAD )
	{
		if (gpGlobals->time > m_flTakeHitTime )
		{
			pev->health -= damageInfo.damage;
			if (pev->health <= 0)
			{
				pev->health = pev->max_health;
				m_iWasHit = 1;
				m_flTakeHitTime = m_flTakeHitTime + RANDOM_LONG(2,4);
			}
		}

		UTIL_BloodDrips(ptr->vecEndPos, vecDir, m_bloodColor, damageInfo.damage * 10);
		UTIL_BloodDecalTrace(ptr, m_bloodColor);

		if (m_hEnemy == 0)
		{
			m_hEnemy = Instance(pevAttacker);
		}
		if (pev->skin == 0)
		{
			pev->skin = 1;
			m_flLastBlinkInterval = gpGlobals->time;
			m_flLastBlinkTime = gpGlobals->time;
		}
	}
	else
	{
		if (pev->dmgtime != gpGlobals->time || RANDOM_LONG(0, 10) >= 0 )
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(0.5, 1.5));
			pev->dmgtime = gpGlobals->time;
		}
	}
}

void CPitWorm::CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	switch (useType)
	{
	case USE_ON:
		if (pActivator)
			InsertAISound(bits_SOUND_WORLD, pActivator->pev->origin, 1024, 1.0f);
		break;
	case USE_OFF:
	case USE_TOGGLE:
	{
		pev->takedamage = DAMAGE_NO;
		pev->health = 0;

		SetThink(&CPitWorm::DyingThink);
		pev->nextthink = gpGlobals->time;
	}
		break;
	default:
		break;
	}
}


//=========================================================
//
//=========================================================
TakeDamageResult CPitWorm::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	PainSound();
	return TakeDamageResult();
}


//=========================================================
// StartupThink
//=========================================================
void CPitWorm::StartupThink()
{
	CBaseEntity *pEntity = NULL;

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tleveldead");
	if (pEntity)
	{
		ALERT(at_console, "level dead node set\n");
		m_flTargetLevels[0] = pEntity->pev->origin.z;
		m_flLevels[0] = m_flTargetLevels[0] - 300;
	}

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel1");
	if (pEntity)
	{
		ALERT(at_console, "level 1 node set\n");
		m_flTargetLevels[1] = pEntity->pev->origin.z;
		m_flLevels[1] = m_flTargetLevels[1] - 300;
	}

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel2");
	if (pEntity)
	{
		ALERT(at_console, "level 2 node set\n");
		m_flTargetLevels[2] = pEntity->pev->origin.z;
		m_flLevels[2] = m_flTargetLevels[2] - 300;
	}

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel3");
	if (pEntity)
	{
		ALERT(at_console, "level 3 node set\n");
		m_flTargetLevels[3] = pEntity->pev->origin.z;
		m_flLevels[3] = m_flTargetLevels[3] - 350;
	}

	m_iLevel = 2;
	if (!FStringNull(pev->target))
	{
		if (FStrEq(STRING(pev->target), "pw_level1"))
			m_iLevel = 1;
		else if (FStrEq(STRING(pev->target), "pw_level2"))
			m_iLevel = 2;
		else if (FStrEq(STRING(pev->target), "pw_level3"))
			m_iLevel = 3;
		else if (FStrEq(STRING(pev->target), "pw_leveldead"))
			m_iLevel = 0;
	}

	m_posDesired.z = m_flLevels[m_iLevel];

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);
	pev->view_ofs = vecEyePos - pev->origin;

	SetThink(&CPitWorm::HuntThink);
	SetUse(&CPitWorm::CommandUse);
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
//
//=========================================================
void CPitWorm::StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CPitWorm::HuntThink);
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse(&CPitWorm::CommandUse);
}

//=========================================================
// NullThink
//=========================================================
void CPitWorm::NullThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5;
}

//=========================================================
// DyingThink
//=========================================================
void CPitWorm::DyingThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	GlowShellUpdate();
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (pev->deadflag == DEAD_DYING)
	{
		if ( gpGlobals->time - m_flDeathStartTime > 3.0 )
		{
			ChangeLevel();
		}
		if ( fabs(pev->origin.z - this->m_flLevels[0]) < 16.0 )
		{
			pev->velocity = Vector(0, 0, 0);
			pev->deadflag = DEAD_DEAD;
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
	else
	{
		pev->deadflag = DEAD_DYING;
		int deathAnim = LookupSequence("death");
		int iDir = 1;
		m_posDesired.z = m_flLevels[0];
		pev->sequence = FindTransition(pev->sequence, deathAnim, &iDir);
		if (iDir <= 0)
		{
			pev->frame = 255;
		}
		else
		{
			pev->frame = 0;
		}
		m_flLevelSpeed = 5;
		ResetSequenceInfo();
		DeathSound();

		if (m_pBeam)
		{
			UTIL_Remove(m_pBeam);
			m_pBeam = NULL;
		}
		if (m_pSprite)
		{
			UTIL_Remove(m_pSprite);
			m_pSprite = NULL;
		}

		SetUse(NULL);
		SetTouch(NULL);
	}
}

//=========================================================
// HuntThink
//=========================================================
void CPitWorm::HuntThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	GlowShellUpdate();
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (m_pBeam)
	{
		if (m_hEnemy != 0 && m_flBeamExpireTime > gpGlobals->time)
		{
			StrafeBeam();
		}
		else
		{
			UTIL_Remove(m_pBeam);
			m_pBeam = NULL;
			UTIL_Remove(m_pSprite);
			m_pSprite = NULL;
		}
	}

	if (pev->health <= 0)
	{
		SetThink(&CPitWorm::DyingThink);
		m_fSequenceFinished = true;
	}
	else
	{
		if ( (gpGlobals->time - m_flLastBlinkTime) >= 6.0 && !m_pBeam )
		{
			pev->skin = 1;
			m_flLastBlinkInterval = gpGlobals->time;
			m_flLastBlinkTime = gpGlobals->time;
		}

		if (pev->skin)
		{
			if ( gpGlobals->time - m_flLastBlinkInterval >= 0.0 )
			{
				if ( pev->skin == 5 )
					pev->skin = 0;
				else
					pev->skin++;
				m_flLastBlinkInterval = gpGlobals->time;
			}
		}
	}

	if (m_iWasHit)
	{
		int iDir = 1;
		const char* flinchAnim = RANDOM_LONG(0,1) ? "flinch1" : "flinch2";
		pev->sequence = FindTransition(pev->sequence, LookupSequence(flinchAnim), &iDir);
		if (iDir > 0)
		{
			pev->frame = 0;
		}
		else
		{
			pev->frame = 255;
		}
		ResetSequenceInfo();
		m_iWasHit = 0;
		PainSound();
	}
	else if (m_fSequenceFinished)
	{
		int oldSeq = pev->sequence;
		if ( m_fAttacking )
		{
			m_fLockHeight = false;
			m_fLockYaw = false;
			m_fAttacking = false;
			m_flNextMeleeTime = gpGlobals->time + 0.25;
		}
		NextActivity();
		if (pev->sequence != oldSeq || !m_fSequenceLoops)
		{
			pev->frame = 0;
			ResetSequenceInfo();
		}
	}

	if (m_hEnemy != 0)
	{
		if (FVisible(m_hEnemy))
		{
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->pev->origin;
			m_posTarget.z += 24;
			Vector vecEyePos, vecEyeAng;
			GetAttachment(0, vecEyePos, vecEyeAng);
			m_vecTarget = (m_posTarget - vecEyePos).Normalize();
			m_vecDesired = m_vecTarget;
		}
	}

	if (m_posDesired.z > m_flLevels[3])
	{
		m_posDesired.z = m_flLevels[3];
	}
	else if (m_posDesired.z < m_flLevels[0])
	{
		m_posDesired.z = m_flLevels[0];
	}
	ChangeLevel();
	if (m_hEnemy != 0 && !m_pBeam)
	{
		TrackEnemy();
	}
}

//=========================================================
//
//=========================================================
void CPitWorm::HitTouch(CBaseEntity* pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();

	if (pOther->pev->modelindex == pev->modelindex)
		return;

	if (m_flHitTime > gpGlobals->time)
		return;

	if( tr.pHit == NULL || tr.pHit->v.modelindex != pev->modelindex )
		return;

	if (pOther->pev->takedamage)
	{
		pOther->TakeDamage(pev, pev, DamageInfo(GetSkillValue("pitworm_dmg_swipe"), DMG_CRUSH|DMG_SLASH));
		pOther->pev->punchangle.z = 15;
		pOther->pev->velocity.z += 200;
		EmitSoundScript(attackHitSoundScript);
		m_flHitTime = gpGlobals->time + 1.0;
	}
}

//=========================================================
// NextActivity
//=========================================================
void CPitWorm::NextActivity()
{
	UTIL_MakeAimVectors(pev->angles);

	if (m_hEnemy)
	{
		if (!m_hEnemy->IsAlive())
		{
			m_hEnemy = 0;
			m_flIdealHeadYaw = 0;
		}
	}

	if (gpGlobals->time > m_flLastSeen + 15)
	{
		if (m_hEnemy != 0)
		{
			if ((pev->origin - m_hEnemy->pev->origin).IsLengthGreaterThan(700))
				m_hEnemy = NULL;
		}
	}

	if (m_hEnemy == 0)
	{
		Look(4096);
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_hEnemy != 0 || m_fFirstSighting)
	{
		if (m_iWasHit)
		{
			const char* flinchAnim = RANDOM_LONG(0,1) ? "flinch1" : "flinch2";
			pev->sequence = LookupSequence(flinchAnim);
			m_iWasHit = 0;
			PainSound();
			m_fLockHeight = 0;
			m_fLockYaw = 0;
			m_fAttacking = 0;
		}
		else if (pev->origin.z == m_posDesired.z)
		{
			if ( abs((int)floor(m_flIdealTorsoYaw - m_flTorsoYaw)) > 10 || !ClawAttack() )
			{
				if ( RANDOM_LONG(0, 2) == 0 )
				{
					IdleSound();
				}

				pev->sequence = LookupSequence("idle2");
				m_fLockHeight = false;
				m_fLockYaw = false;
				m_fAttacking = false;
			}
		}
		else
		{
			if ( RANDOM_LONG(0, 2) == 0 )
			{
				IdleSound();
			}
			pev->sequence = LookupSequence("idle");
			m_fLockHeight = false;
			m_fLockYaw = false;
			m_fAttacking = false;
		}
	}
	if (m_hEnemy != 0 && !m_fFirstSighting)
	{
		AlertSound();
		m_fFirstSighting = true;
		pev->sequence = LookupSequence("scream");
	}
}

void CPitWorm::EyeLight(const Vector &vecEyePos)
{
	SendEntLight(entindex(), vecEyePos, GetVisual(eyeLightVisual), 1);
}

void CPitWorm::BeamEffect(TraceResult &tr)
{
	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
	if( pEntity != NULL && pEntity->pev->takedamage )
	{
		pEntity->ApplyTraceAttack(pev, pev, DamageInfo{GetSkillValue("pitworm_dmg_beam"), DMG_ENERGYBEAM}, m_vecBeam, &tr);
	}
	else if ( tr.flFraction != 1.0f )
	{
		UTIL_DecalTrace(&tr, DECAL_GUNSHOT1 + RANDOM_LONG(0,4));
		m_pBeam->DoSparks(tr.vecEndPos, tr.vecEndPos);
	}
}

void CPitWorm::LockTopLevel()
{
	if (m_iLevel == 3)
	{
		pev->health = pev->max_health;
		m_iWasHit = 1;
		m_iLevel = 2;
		m_fTopLevelLocked = true;
		m_flTakeHitTime = gpGlobals->time + RANDOM_LONG(0,2);
		m_posDesired.z = m_flLevels[2];
	}
	else
	{
		m_fTopLevelLocked = true;
	}
}

bool CPitWorm::ClawAttack()
{
	if (m_hEnemy == 0)
		return false;
	if (pev->origin.z != m_posDesired.z)
		return false;
	if ( m_flNextMeleeTime > gpGlobals->time )
		return false;

	float flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();
	Vector targetAngle = UTIL_VecToAngles((m_posTarget - pev->origin).Normalize());
	float angleDiff = UTIL_AngleDiff(targetAngle.y, pev->angles.y);

	if (!FVisible(m_posTarget))
	{
		if (flDist < 600)
		{
			return 0;
		}

		bool shouldClaw = false;
		if (m_iLevel == 2)
		{
			if (angleDiff >= 30)
			{
				pev->sequence = LookupSequence("doorclaw1");
				m_flIdealHeadYaw = 0;
				shouldClaw = true;
			}
		}
		else if (m_iLevel == 1)
		{
			if ( angleDiff <= -30.0 )
			{
				pev->sequence = LookupSequence("doorclaw2");
				m_flIdealHeadYaw = 0;
				shouldClaw = true;
			}
			if ( angleDiff >= 30.0 )
			{
				pev->sequence = LookupSequence("doorclaw3");
				m_flIdealHeadYaw = 0;
				shouldClaw = true;
			}
		}

		if (shouldClaw)
		{
			EmitSoundScript(swipeSoundScript);
			m_fLockHeight = true;
			m_fLockYaw = true;
			m_fAttacking = true;
			return true;
		}

		return false;
	}

	m_fLockYaw = false;
	if (m_iLevel == 2)
	{
		if (angleDiff < 30)
		{
			if (flDist > 425 || angleDiff >= -50)
			{
				pev->sequence = LookupSequence("eyeblast");
				m_posBeam = m_posTarget;
				m_vecBeam = m_vecTarget;
				m_angleBeam = UTIL_VecToAngles(m_vecBeam);
			}
			else
			{
				pev->sequence = LookupSequence("attack");
			}
		}
		else
		{
			pev->sequence = LookupSequence("doorclaw1");
			m_flIdealHeadYaw = 0;
			m_fLockYaw = true;
		}
	}
	else if (m_iLevel == 3)
	{
		if (flDist <= 425)
		{
			pev->sequence = RANDOM_LONG(0,1) ? LookupSequence("platclaw1") : LookupSequence("platclaw2");
		}
		else
		{
			pev->sequence = LookupSequence("eyeblast");
			m_posBeam = m_posTarget;
			m_vecBeam = m_vecTarget;
			m_angleBeam = UTIL_VecToAngles(m_vecBeam);
		}
	}
	else if (m_iLevel == 1)
	{
		if (angleDiff < 50)
		{
			if (angleDiff > -30)
			{
				if (flDist > 425)
				{
					pev->sequence = LookupSequence("eyeblast");
					m_posBeam = m_posTarget;
					m_vecBeam = m_vecTarget;
					m_angleBeam = UTIL_VecToAngles(m_vecBeam);
				}
				else
				{
					pev->sequence = LookupSequence("attacklow");
				}
			}
			else
			{
				pev->sequence = LookupSequence("doorclaw2");
			}
		}
		else
		{
			pev->sequence = LookupSequence("doorclaw3");
			m_flIdealHeadYaw = 0;
			m_fLockYaw = true;
		}
	}
	if (pev->sequence == LookupSequence("eyeblast"))
	{
		EmitSoundScript(blastSoundScript);
	}
	else
	{
		EmitSoundScript(swipeSoundScript);
	}
	m_fAttacking = true;
	m_fLockHeight = true;
	return true;
}

void CPitWorm::ShootBeam()
{
	if ( m_hEnemy != 0 )
	{
		if ( m_flHeadYaw > 0.0 )
		{
			m_offsetBeam = 80;
			m_flBeamDir = -1;
		}
		else
		{
			m_offsetBeam = -80;
			m_flBeamDir = 1;
		}
		Vector vecEyePos, vecEyeAng;
		GetAttachment(0, vecEyePos, vecEyeAng);

		m_vecBeam = (m_posBeam - vecEyePos).Normalize();
		m_angleBeam = UTIL_VecToAngles(m_vecBeam);
		UTIL_MakeVectors(m_angleBeam);
		m_vecBeam = gpGlobals->v_forward;
		m_vecBeam.z = -m_vecBeam.z;
		Vector vecEnd = m_vecBeam * m_offsetBeam + m_vecBeam * 1280 + vecEyePos;

		TraceResult tr;
		UTIL_TraceLine(vecEyePos, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

		m_pBeam = CreateBeamFromVisual(GetVisual(beamVisual));
		if ( m_pBeam )
		{
			m_pBeam->PointEntInit(tr.vecEndPos, entindex());
			m_pBeam->SetEndAttachment(1);
			m_pBeam->pev->spawnflags |= SF_BEAM_SPARKSTART;

			BeamEffect(tr);

			m_pBeam->DoSparks(vecEyePos, vecEyePos);
			m_flBeamExpireTime = gpGlobals->time + 0.9;
			float beamYaw = m_flHeadYaw - m_flBeamDir * 25.0;
			if ( beamYaw < -45.0 || beamYaw > 45.0 )
			{
				m_flIdealHeadYaw += m_flBeamDir * 50;
			}

			EmitSoundScript(shootSoundScript);

			EyeLight(vecEyePos);

			m_pSprite = CreateSpriteFromVisual(GetVisual(eyeGlowVisual), vecEyePos);
			if ( m_pSprite )
			{
				m_pSprite->SetAttachment(edict(), 1);
				m_pSprite->TurnOn();
			}
		}
	}
}

void CPitWorm::StrafeBeam()
{
	m_offsetBeam += m_flBeamDir * 20;

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	m_vecBeam = (m_posBeam - vecEyePos).Normalize();
	m_angleBeam = UTIL_VecToAngles(m_vecBeam);
	UTIL_MakeVectors(m_angleBeam);
	m_vecBeam = gpGlobals->v_forward;
	m_vecBeam.z = -m_vecBeam.z;

	Vector vecEnd = vecEyePos + gpGlobals->v_right * m_offsetBeam + m_vecBeam * 1280;

	TraceResult tr;
	UTIL_TraceLine(vecEyePos, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

	m_pBeam->SetStartPos(tr.vecEndPos);
	BeamEffect(tr);

	EyeLight(vecEyePos);
}

void CPitWorm::ChangeLevel()
{
	float currentZ = pev->origin.z;
	float desiredZ = m_posDesired.z;
	if (currentZ > desiredZ)
	{
		pev->origin.z -= m_flLevelSpeed;

		if (pev->origin.z < desiredZ)
			pev->origin.z = desiredZ;
	}
	else if (currentZ < desiredZ)
	{
		pev->origin.z += m_flLevelSpeed;

		if (pev->origin.z > desiredZ)
			pev->origin.z = desiredZ;
	}

	if (m_flIdealTorsoYaw != m_flTorsoYaw)
	{
		if (m_flIdealTorsoYaw < m_flTorsoYaw)
		{
			m_flTorsoYaw -= 5.0;
			if (m_flTorsoYaw < m_flIdealTorsoYaw)
				m_flTorsoYaw = m_flIdealTorsoYaw;
		}
		else if (m_flIdealTorsoYaw > m_flTorsoYaw)
		{
			m_flTorsoYaw += 5;
			if (m_flTorsoYaw > m_flIdealTorsoYaw)
				m_flTorsoYaw = m_flIdealTorsoYaw;
		}
		SetBoneController(PITWORM_CONTROLLER_BODY_YAW, m_flTorsoYaw);
	}

	if (m_flIdealHeadYaw != m_flHeadYaw)
	{
		if (m_flIdealHeadYaw < m_flHeadYaw)
		{
			m_flHeadYaw -= 5.0;
			if (m_flHeadYaw < m_flIdealHeadYaw)
				m_flHeadYaw = m_flIdealHeadYaw;
		}
		else if (m_flIdealHeadYaw > m_flHeadYaw)
		{
			m_flHeadYaw += 5;
			if (m_flHeadYaw > m_flIdealHeadYaw)
				m_flHeadYaw = m_flIdealHeadYaw;
		}
		SetBoneController(PITWORM_CONTROLLER_EYE_YAW, m_flHeadYaw);
	}

	if (m_flIdealHeadPitch != m_flHeadPitch)
	{
		if (m_flIdealHeadPitch < m_flHeadPitch)
		{
			m_flHeadPitch -= 5.0;
			if (m_flHeadPitch < m_flIdealHeadPitch)
				m_flHeadPitch = m_flIdealHeadPitch;
		}
		else if (m_flIdealHeadPitch > m_flHeadPitch)
		{
			m_flHeadPitch += 5;
			if (m_flHeadPitch > m_flIdealHeadPitch)
				m_flHeadPitch = m_flIdealHeadPitch;
		}
		SetBoneController(PITWORM_CONTROLLER_EYE_PITCH, m_flHeadPitch);
	}
}

void CPitWorm::TrackEnemy()
{
	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	Vector vec = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - Vector(pev->origin.x, pev->origin.y, vecEyePos.z);
	Vector vecDir = UTIL_VecToAngles(vec);
	float angleDiff = UTIL_AngleDiff(vecDir.x, pev->angles.x);
	if (angleDiff < PITWORM_EYE_PITCH_MIN)
	{
		angleDiff = PITWORM_EYE_PITCH_MIN;
	}
	else if (angleDiff > PITWORM_EYE_PITCH_MAX)
	{
		angleDiff = PITWORM_EYE_PITCH_MAX;
	}
	m_flIdealHeadPitch = angleDiff;

	float targetYaw = UTIL_VecToYaw(m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecEyePos) - pev->angles.y;
	if (targetYaw > 180)
	{
		targetYaw -= 360;
	}
	else if (targetYaw < -180)
	{
		targetYaw += 360;
	}
	if (!m_fLockYaw)
	{
		if (targetYaw < 0)
		{
			const float minYaw = (m_iLevel == 1) ? -30 : -50;
			if (targetYaw <= minYaw)
				targetYaw = minYaw;
		}
		else
		{
			const float maxYaw = (m_iLevel == 2) ? 30 :50;
			if (targetYaw > maxYaw)
				targetYaw = maxYaw;
		}
		m_flIdealTorsoYaw = targetYaw;
	}
	float torsoYawDiff = m_flTorsoYaw - targetYaw;
	if (torsoYawDiff > 0)
	{
		if (torsoYawDiff > PITWORM_EYE_YAW_MAX)
		{
			torsoYawDiff = PITWORM_EYE_YAW_MAX;
		}
		else if (torsoYawDiff < PITWORM_EYE_YAW_MIN)
		{
			torsoYawDiff = PITWORM_EYE_YAW_MIN;
		}
	}
	if (!m_fAttacking || m_pBeam != 0)
	{
		m_flIdealHeadYaw = torsoYawDiff;
	}

	if (!m_fLockHeight)
	{
		if (m_hEnemy->pev->origin.z <= m_flTargetLevels[1])
		{
			m_iLevel = 0;
		}
		else if (m_hEnemy->pev->origin.z <= m_flTargetLevels[2])
		{
			m_iLevel = 1;
		}
		else if (m_fTopLevelLocked || m_hEnemy->pev->origin.z <= m_flTargetLevels[3])
		{
			m_iLevel = 2;
		}
		else
		{
			m_iLevel = 3;
		}
		m_posDesired.z = m_flLevels[m_iLevel];
	}
}

class CPitwormGib : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("pitworm"); }
	void EXPORT GibFloat();
};

LINK_ENTITY_TO_CLASS(pitworm_gib, CPitwormGib)

void CPitwormGib::Precache()
{
	PRECACHE_MODEL("models/pit_worm_gibs.mdl");
}

void CPitwormGib::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55; // deading the bounce a bit

	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_NOT;
	pev->classname = MAKE_STRING( "pitworm_gib" );

	SET_MODEL( ENT( pev ), "models/pit_worm_gibs.mdl" );
	UTIL_SetSize( pev, Vector( -8, -8, -4 ), Vector( 8, 8, 16 ) );

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CPitwormGib::GibFloat);
}

void CPitwormGib::GibFloat()
{
	if (pev->waterlevel == WL_Eyes)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel)
	{
		pev->velocity.z -= 8;
	}
	else
	{
		pev->movetype = MOVETYPE_BOUNCE;
		pev->velocity.z -= 8;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

class CPitwormGibShooter : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("pitworm"); }
	void KeyValue( KeyValueData *pkvd ) override;
	void EXPORT ShootThink();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	CPitwormGib *CreateGib();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iGibModelIndex;
	float m_flGibVelocity;
};

TYPEDESCRIPTION CPitwormGibShooter::m_SaveData[] =
{
	DEFINE_FIELD( CPitwormGibShooter, m_flGibVelocity, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CPitwormGibShooter, CBaseDelay )

LINK_ENTITY_TO_CLASS(pitworm_gibshooter, CPitwormGibShooter)

void CPitwormGibShooter::Precache()
{
	m_iGibModelIndex = PRECACHE_MODEL("models/pit_worm_gibs.mdl");
	UTIL_PrecacheOther("pitworm_gib");
}

void CPitwormGibShooter::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if( m_flDelay == 0 )
	{
		m_flDelay = 0.1;
	}

	SetMovedir( pev );
	pev->body = MODEL_FRAMES( m_iGibModelIndex );
}

void CPitwormGibShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_flGibVelocity = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CPitwormGibShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CPitwormGibShooter::ShootThink );
	pev->nextthink = gpGlobals->time;
}

CPitwormGib *CPitwormGibShooter::CreateGib()
{
	if (violence_hgibs->value == 0)
		return NULL;

	CPitwormGib *pGib = GetClassPtr((CPitwormGib *)NULL);
	pGib->Spawn();

	if (pev->body <= 1)
	{
		ALERT(at_aiconsole, "PitwormGibShooter Body is <= 1!\n");
	}

	pGib->pev->body = RANDOM_LONG(0, pev->body - 1);

	return pGib;
}

void CPitwormGibShooter::ShootThink()
{
	UTIL_MakeVectors(pev->angles);
	CPitwormGib *pGib = CreateGib();
	if (pGib)
	{
		pGib->pev->origin = pev->origin;
		pGib->pev->velocity = gpGlobals->v_forward * m_flGibVelocity;
	}
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

class CPitWormSteamTrigger : public CBaseEntity
{
public:
	void Spawn() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("pitworm"); }
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
};

LINK_ENTITY_TO_CLASS(info_pitworm_steam_lock, CPitWormSteamTrigger)

void CPitWormSteamTrigger::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin(pev, pev->origin);
}

void CPitWormSteamTrigger::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CPitWorm* pWorm = (CPitWorm*)UTIL_FindEntityByClassname(0, "monster_pitworm_up");
	if (pWorm)
	{
		pWorm->LockTopLevel();
	}
}
