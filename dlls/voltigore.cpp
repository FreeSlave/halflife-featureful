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
//=========================================================
// voltigore
//=========================================================

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
#include	"squadmonster.h"
#include	"combat.h"
#include	"mod_features.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

#if FEATURE_VOLTIFORE

#define VOLTIGORE_BEAM_COUNT 8
#define VOLTIGORE_GIB_COUNT 10

class CChargedBolt : public CBaseEntity
{
public:
	void Precache();
	void Spawn();

	void InitBeams();
	void ClearBeams();

	void EXPORT ShutdownChargedBolt();

	static CChargedBolt* ChargedBoltCreate(EntityOverrides entityOverrides = EntityOverrides());

	void LaunchChargedBolt(const Vector& vecAim, edict_t* pOwner, int nSpeed);

	void SetAttachment(CBaseAnimating* pAttachEnt, int iAttachIdx);

	void ArmBeam(int side);

	void EXPORT AttachThink();

	void EXPORT FlyThink();
	void EXPORT PreShutdownThink();
	void DoRadiusDamage(float dmg, float radius);

	void EXPORT ChargedBoltTouch(CBaseEntity* pOther);

	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_pBeam[VOLTIGORE_BEAM_COUNT];
	int m_iBeams;
	CBaseAnimating* m_pAttachEnt;
	int m_iAttachIdx;
	float m_shutdownTime;
	float m_radiusCheckTime;

	static const NamedVisual spriteVisual;
	static const NamedVisual beamVisual;
	static const NamedVisual hitBeamVisual;
};

LINK_ENTITY_TO_CLASS(charged_bolt, CChargedBolt);

TYPEDESCRIPTION CChargedBolt::m_SaveData[] =
{
	DEFINE_ARRAY(CChargedBolt, m_pBeam, FIELD_EHANDLE, VOLTIGORE_BEAM_COUNT),
	DEFINE_FIELD(CChargedBolt, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CChargedBolt, m_pAttachEnt, FIELD_CLASSPTR),
	DEFINE_FIELD(CChargedBolt, m_iAttachIdx, FIELD_INTEGER),
	DEFINE_FIELD(CChargedBolt, m_shutdownTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CChargedBolt, CBaseEntity);

const NamedVisual CChargedBolt::spriteVisual = BuildVisual("ChargedBolt.Sprite")
		.Model("sprites/blueflare2.spr")
		.RenderMode(kRenderTransAdd)
		.Alpha(255)
		.Scale(0.75f);

const NamedVisual CChargedBolt::beamVisual = BuildVisual("ChargedBolt.Beam")
		.Model("sprites/lgtning.spr")
		.RenderColor(180, 16, 255)
		.Alpha(255)
		.BeamWidth(30)
		.BeamNoise(80);

const NamedVisual CChargedBolt::hitBeamVisual = BuildVisual("ChargedBolt.HitBeam")
		.RenderColor(255, 16, 255)
		.BeamNoise(20)
		.Mixin(&beamVisual);

void CChargedBolt::Precache()
{
	RegisterVisual(spriteVisual);
	RegisterVisual(beamVisual);
	RegisterVisual(hitBeamVisual);
}

void CChargedBolt::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5f;

	ApplyVisual(GetVisual(spriteVisual));

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	InitBeams();
}

void CChargedBolt::InitBeams()
{
	memset(m_pBeam, 0, sizeof(m_pBeam));

	m_iBeams = 0;
	pev->skin = 0;
}

void CChargedBolt::ClearBeams()
{
	for (auto& pBeam : m_pBeam)
	{
		if (pBeam)
		{
			UTIL_Remove(pBeam);
			pBeam = nullptr;
		}
	}

	m_iBeams = 0;
	pev->skin = 0;
}

void CChargedBolt::ShutdownChargedBolt()
{
	ClearBeams();

	UTIL_Remove(this);
}

CChargedBolt* CChargedBolt::ChargedBoltCreate(EntityOverrides entityOverrides)
{
	auto pBolt = GetClassPtr<CChargedBolt>(nullptr);

	pBolt->pev->classname = MAKE_STRING("charged_bolt");
	pBolt->AssignEntityOverrides(entityOverrides);
	pBolt->Spawn();

	return pBolt;
}

void CChargedBolt::LaunchChargedBolt(const Vector& vecAim, edict_t* pOwner, int nSpeed)
{
	pev->angles = vecAim;
	pev->owner = pOwner;
	pev->velocity = vecAim * nSpeed;

	pev->speed = nSpeed;

	SetTouch(&CChargedBolt::ChargedBoltTouch);
	SetThink(&CChargedBolt::FlyThink);

	m_radiusCheckTime = pev->nextthink = gpGlobals->time + 0.15f;
}

void CChargedBolt::SetAttachment(CBaseAnimating* pAttachEnt, int iAttachIdx)
{
	Vector vecOrigin;
	Vector vecAngles;

	m_iAttachIdx = iAttachIdx;
	m_pAttachEnt = pAttachEnt;

	pAttachEnt->GetAttachment(iAttachIdx, vecOrigin, vecAngles);

	UTIL_SetOrigin(pev, vecOrigin);

	SetThink(&CChargedBolt::AttachThink);

	pev->nextthink = gpGlobals->time + 0.05;
}

void CChargedBolt::ArmBeam(int side)
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= VOLTIGORE_BEAM_COUNT)
		m_iBeams = 0;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	const Visual* pBeamVisual = GetVisual(beamVisual);
	if (!pBeamVisual)
		return;

	auto pBeam = m_pBeam[m_iBeams].Entity<CBeam>();
	if (!pBeam)
	{
		m_pBeam[m_iBeams] = pBeam = CreateBeamFromVisual(pBeamVisual);
	}

	if (!pBeam)
		return;

	auto pHit = Instance(tr.pHit);

	const Visual* pVisualToApply = pBeamVisual;
	if (pHit && pHit->pev->takedamage != DAMAGE_NO)
	{
		pBeam->EntsInit(entindex(), pHit->entindex());
		const Visual* pHitBeamVisual = GetVisual(hitBeamVisual);
		if (pHitBeamVisual)
		{
			pVisualToApply = pHitBeamVisual;
		}
	}
	else
	{
		pBeam->PointEntInit(tr.vecEndPos, entindex());
	}
	pBeam->SetColor(pVisualToApply->rendercolor.r, pVisualToApply->rendercolor.g, pVisualToApply->rendercolor.b);
	pBeam->SetBrightness(pVisualToApply->renderamt);
	pBeam->SetWidth(pVisualToApply->beamWidth);
	pBeam->SetNoise(pVisualToApply->beamNoise);
	pBeam->SetScrollRate(pVisualToApply->beamScrollRate);

	++m_iBeams;
}

void CChargedBolt::AttachThink()
{
	Vector vecOrigin;
	Vector vecAngles;

	m_pAttachEnt->GetAttachment(m_iAttachIdx, vecOrigin, vecAngles);
	UTIL_SetOrigin(pev, vecOrigin);

	pev->nextthink = gpGlobals->time + 0.05;
}

void CChargedBolt::FlyThink()
{
	ArmBeam(-1);
	ArmBeam(1);
	pev->nextthink = gpGlobals->time + 0.05f;

	if (m_radiusCheckTime <= gpGlobals->time)
	{
		DoRadiusDamage(gSkillData.voltigoreDmgBeam * 0.2f, 32);
		m_radiusCheckTime = gpGlobals->time + 0.1f;
	}
}

void CChargedBolt::PreShutdownThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	DoRadiusDamage(gSkillData.voltigoreDmgBeam * 0.2f, 32);

	if (m_shutdownTime <= gpGlobals->time)
	{
		pev->nextthink = gpGlobals->time + 0.05f;
		SetThink(&CChargedBolt::ShutdownChargedBolt);
	}
}

void CChargedBolt::DoRadiusDamage(float dmg, float radius)
{
	CBaseMonster* pOwner = GetMonsterPointer(pev->owner);
	::RadiusDamage(nullptr, pev->origin, pev, pOwner ? pOwner->pev : pev, DamageInfo{dmg, DMG_SHOCK}, radius, RADIUSDAMAGE_CHECK_ATTACKER_TRACE,
				   [pOwner](CBaseEntity* pEntity) {
		if (pOwner)
			return pOwner->IRelationship(pEntity) != R_AL;
		return true;
	});
}

void CChargedBolt::ChargedBoltTouch(CBaseEntity* pOther)
{
	SetTouch(nullptr);
	SetThink(nullptr);

	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		if (0 == strcmp("worldspawn", STRING(pOther->pev->classname)))
		{
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr);
			UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0, 1));
		}
	}
	else
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t* pAttacker = FNullEnt(pev->owner) ? pev : VARS(pev->owner);
		pOther->ApplyTraceAttack(pev, pAttacker, DamageInfo(gSkillData.voltigoreDmgBeam, DMG_SHOCK).SetGibPolicy(GIB_ALWAYS), pev->velocity, &tr);
	}

	pev->velocity = g_vecZero;

	SetThink(&CChargedBolt::PreShutdownThink);
	pev->nextthink = gpGlobals->time + 0.05f;
	m_shutdownTime = gpGlobals->time + 0.3f;
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		VOLTIGORE_AE_THROW			( 1 )
#define		VOLTIGORE_AE_PUNCH_BOTH		( 12 )
#define		VOLTIGORE_AE_PUNCH_SINGLE	( 13 )
#define		VOLTIGORE_AE_GIB			( 2002 )


//=========================================================
// CVoltigore
//=========================================================
class CVoltigore : public CSquadMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("voltigore"); }
	void SetYawSpeed(void);
	virtual int  DefaultClassify(void);
	const char* DefaultDisplayName() { return "Voltigore"; }
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void IdleSound();
	virtual void PainSound();
	virtual void PlayPainSound();
	virtual void DeathSound();
	virtual void AlertSound();
	virtual void StartTask(Task_t *pTask);
	virtual bool CheckMeleeAttack1(float flDot, float flDist) override;
	virtual bool CheckRangeAttack1(float flDot, float flDist) override;
	virtual void RunAI(void);
	virtual void GibMonster();
	void EXPORT CallDeathGibThink();
	virtual void DeathGibThink();
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	void OnDying();
	void UpdateOnRemove();
	const char* DefaultGibModel() {
		return "models/vgibs.mdl";
	}
	int DefaultGibCount() {
		return VOLTIGORE_GIB_COUNT;
	}
	
	int	Save(CSave &save);
	int Restore(CRestore &restore);

	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	virtual int DefaultSizeForGrapple() { return GRAPPLE_LARGE; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() {
		return Vector( -80.0f, -80.0f, 0.0f );
	}
	Vector DefaultMaxHullSize() {
		return Vector( 80.0f, 80.0f, 90.0f );
	}

	float m_flNextBeamAttackCheck; // next time the voltigore can use the spit attack.
	EHANDLE m_pBeam[3];
	float m_flNextPainTime;
	EHANDLE m_pChargedBolt;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static constexpr const char* attackHitSoundScript = "Voltigore.AttackHit";
	static constexpr const char* attackMissSoundScript = "Voltigore.AttackMiss";
	static const NamedSoundScript footstepSoundScript;
	static const NamedSoundScript beamAttackSoundScript;

	static const NamedVisual beamVisual;
	static const NamedVisual chargeBeamVisual;
	static const NamedVisual deathBeamVisual;

	void UpdateBeamAndBoltPositions();
	void ClearBeams();
	Vector BoltPosition();
};

LINK_ENTITY_TO_CLASS(monster_alien_voltigore, CVoltigore)


TYPEDESCRIPTION	CVoltigore::m_SaveData[] =
{
	DEFINE_FIELD(CVoltigore, m_flNextBeamAttackCheck, FIELD_TIME),
	DEFINE_ARRAY(CVoltigore, m_pBeam, FIELD_EHANDLE, 3),
	DEFINE_FIELD(CVoltigore, m_flNextPainTime, FIELD_TIME),
	DEFINE_FIELD(CVoltigore, m_pChargedBolt, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CVoltigore, CSquadMonster)

const NamedSoundScript CVoltigore::idleSoundScript = {
	CHAN_VOICE,
	{"voltigore/voltigore_idle1.wav", "voltigore/voltigore_idle2.wav", "voltigore/voltigore_idle3.wav"},
	"Voltigore.Idle"
};

const NamedSoundScript CVoltigore::alertSoundScript = {
	CHAN_VOICE,
	{"voltigore/voltigore_alert1.wav", "voltigore/voltigore_alert2.wav", "voltigore/voltigore_alert3.wav"},
	"Voltigore.Alert"
};

const NamedSoundScript CVoltigore::painSoundScript = {
	CHAN_VOICE,
	{"voltigore/voltigore_pain1.wav", "voltigore/voltigore_pain2.wav", "voltigore/voltigore_pain3.wav", "voltigore/voltigore_pain4.wav"},
	"Voltigore.Pain"
};

const NamedSoundScript CVoltigore::dieSoundScript = {
	CHAN_VOICE,
	{"voltigore/voltigore_die1.wav", "voltigore/voltigore_die2.wav", "voltigore/voltigore_die3.wav"},
	"Voltigore.Die"
};

const NamedSoundScript CVoltigore::footstepSoundScript = {
	CHAN_BODY,
	{"voltigore/voltigore_footstep1.wav", "voltigore/voltigore_footstep2.wav", "voltigore/voltigore_footstep3.wav"},
	"Voltigore.Footstep"
};

const NamedSoundScript CVoltigore::beamAttackSoundScript = {
	CHAN_STATIC,
	{"debris/beamstart2.wav"},
	0.5f,
	ATTN_NORM,
	IntRange(140, 160),
	"Voltigore.BeamAttack"
};

const NamedVisual CVoltigore::beamVisual = BuildVisual("Voltigore.Beam")
		.Model("sprites/lgtning.spr")
		.RenderColor(180, 16, 255)
		.Alpha(255);

const NamedVisual CVoltigore::chargeBeamVisual = BuildVisual("Voltigore.ChargeBeam")
		.BeamParams(50, 20)
		.Mixin(&beamVisual);

const NamedVisual CVoltigore::deathBeamVisual = BuildVisual("Voltigore.DeathBeam")
		.BeamParams(30, 128)
		.Mixin(&beamVisual);

bool CVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// voltigore will far too far behind if he stops running to spit at this distance from the enemy.
		return false;
	}

	if (flDist > 128.0f && flDist <= 1024.0f && flDot >= 0.5f && gpGlobals->time >= m_flNextBeamAttackCheck)
	{
		Vector vecStart, vecDir;
		TraceResult tr;

		UTIL_MakeVectors( pev->angles );
		GetAttachment( 3, vecStart, vecDir );
		UTIL_TraceLine( vecStart, m_hEnemy->BodyTarget( vecStart ), dont_ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction == 1.0f || tr.pHit == m_hEnemy->edict() )
		{
			m_flNextBeamAttackCheck = gpGlobals->time + RANDOM_FLOAT(1.5f, 5.0f);
			return true;
		}
		else
		{
			m_flNextBeamAttackCheck = gpGlobals->time + 0.2f;
		}
	}

	return false;
}

void CVoltigore::RunAI(void)
{
	UpdateBeamAndBoltPositions();
	CSquadMonster::RunAI();
}

void CVoltigore::GibMonster()
{
	pev->renderfx = kRenderFxExplode;

	pev->rendercolor.x = 255;
	pev->rendercolor.y = 0;
	pev->rendercolor.z = 0;
	pev->framerate = 0;

	CSquadMonster::GibMonster();

	pev->nextthink = gpGlobals->time + 0.15;
}

void CVoltigore::CallDeathGibThink()
{
	DeathGibThink();
}

void CVoltigore::DeathGibThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	DispatchAnimEvents(0.1f);
	StudioFrameAdvance(0.0f);

	if (m_fSequenceFinished)
	{
		GibMonster();
	}
	else
	{
		for (auto i = 0; i < 2; ++i)
		{
			const auto side = i == 0 ? -1 : 1;

			UTIL_MakeAimVectors(pev->angles);

			TraceResult tr;

			const auto vecSrc = pev->origin + gpGlobals->v_forward * 32 + side * gpGlobals->v_right * 16 + gpGlobals->v_up * 36;

			float closest = 1.0f;

			//Do 3 ray traces and use the closest one to make a beam
			for (auto ray = 0; ray < 3; ++ray)
			{
				TraceResult tr1;
				UTIL_TraceLine(vecSrc, vecSrc + (side * gpGlobals->v_right * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1)) * 512, dont_ignore_monsters, edict(), &tr1);

				if (tr1.flFraction < closest)
				{
					tr = tr1;
					closest = tr1.flFraction;
				}
			}

			//No nearby objects found
			if (closest == 1.0f)
			{
				return;
			}

			auto pBeam = CreateBeamFromVisual(GetVisual(deathBeamVisual));
			if (!pBeam)
				return;

			auto pHit = Instance(tr.pHit);

			if (pHit && pHit->pev->takedamage != DAMAGE_NO)
			{
				pBeam->PointEntInit(pev->origin + Vector(0, 0, 32), pHit->entindex());
			}
			else
			{
				pBeam->PointsInit(tr.vecEndPos, pev->origin + Vector(0, 0, 32));
				int noise = pBeam->GetNoise() * 1.5;
				if (noise > 255)
					noise = 255;
				pBeam->SetNoise(noise);
			}

			pBeam->SetThink(&CBaseEntity::SUB_Remove);
			pBeam->pev->nextthink = gpGlobals->time + 0.6;
		}

		const float attackRadius = Q_max(Q_min(gSkillData.voltigoreDmgExplode * 2.0f, 200.0f), 160.0f);
		::RadiusDamage(pev->origin, pev, pev, DamageInfo{gSkillData.voltigoreDmgExplode, DMG_SHOCK}, attackRadius, CLASS_NONE);
	}
}

bool CVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	CheckMeleeAttackParams params;
	params.distance = 128.0f;
	params.dot = 0.6f;
	if (HasConditions(bits_COND_SEE_ENEMY) && CheckMeleeAttackImpl(flDot, flDist, params, false) && m_hEnemy != 0)
	{
		return true;
	}
	return false;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CVoltigore::DefaultClassify(void)
{
	return	CLASS_RACEX_SHOCK;
}

//=========================================================
// IdleSound 
//=========================================================
void CVoltigore::IdleSound(void)
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// PainSound 
//=========================================================
void CVoltigore::PainSound(void)
{
	if (m_flNextPainTime > gpGlobals->time)
		return;

	PlayPainSound();
	m_flNextPainTime = gpGlobals->time + 0.6;
}

void CVoltigore::PlayPainSound()
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CVoltigore::AlertSound(void)
{
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CVoltigore::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case	ACT_TURN_LEFT:
	case	ACT_TURN_RIGHT:
		ys = 80;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CVoltigore::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case VOLTIGORE_AE_THROW:
	{
		if (m_pChargedBolt)
		{
			UTIL_MakeVectors(pev->angles);

			const auto shootPosition = pev->origin + gpGlobals->v_forward * 50 + gpGlobals->v_up * 32;

			const auto direction = ShootAtEnemy(shootPosition);

			TraceResult tr;
			UTIL_TraceLine(shootPosition, shootPosition + direction * 1024, dont_ignore_monsters, edict(), &tr);

			CChargedBolt* bolt = m_pChargedBolt.Entity<CChargedBolt>();

			bolt->LaunchChargedBolt(direction, edict(), 1000);

			//We no longer have to manage the bolt now
			m_pChargedBolt = nullptr;

			ClearBeams();
		}
	}
	break;


	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		// SOUND HERE!

		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		TraceHullAttackParams params;
		params.distance = 128.0f;
		params.punchAngle.z = -15;
		params.punchAngle.x = 15;
		params.knockRight = -150;
		params.knockUp = 100;
		params.damageInfo.damage = gSkillData.voltigoreDmgPunch;
		params.damageInfo.type = DMG_CLUB;
		params.spawnBlood = true;
		params.bloodOrigin = vecArmPos;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		// SOUND HERE!

		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		TraceHullAttackParams params;
		params.distance = 128.0f;
		params.punchAngle.x = 20;
		params.knockForward = 150;
		params.knockUp = 100;
		params.damageInfo.damage = gSkillData.voltigoreDmgPunch;
		params.damageInfo.type = DMG_CLUB;
		params.spawnBlood = true;
		params.bloodOrigin = vecArmPos;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
	break;

	case VOLTIGORE_AE_GIB:
	{
		pev->health = 0;
		GibMonster();
	}
	break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CVoltigore::Spawn()
{
	Precache();

	SetMyModel("models/voltigore.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor(BLOOD_COLOR_GREEN);
	pev->effects		= 0;
	SetMyHealth(gSkillData.voltigoreHealth);
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_TURN_HEAD;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(false);

	m_flNextBeamAttackCheck	= gpGlobals->time;

	MonsterInit();
	pev->view_ofs		= Vector(0, 0, 84);

	memset(m_pBeam, 0, sizeof(m_pBeam));
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CVoltigore::Precache()
{
	PrecacheMyModel("models/voltigore.mdl");
	PrecacheMyGibModel("models/vgibs.mdl");

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
	RegisterAndPrecacheSoundScript(footstepSoundScript);
	RegisterAndPrecacheSoundScript(beamAttackSoundScript);

	if (!ShouldAutoPrecacheSounds())
	{
		// Used in model from Opposing Force
		PRECACHE_SOUND("voltigore/voltigore_footstep1.wav");
		PRECACHE_SOUND("voltigore/voltigore_footstep2.wav");
		PRECACHE_SOUND("voltigore/voltigore_footstep3.wav");

		PRECACHE_SOUND("voltigore/voltigore_run_grunt1.wav");
		PRECACHE_SOUND("voltigore/voltigore_run_grunt2.wav");
		PRECACHE_SOUND("voltigore/voltigore_attack_melee1.wav");
		PRECACHE_SOUND("voltigore/voltigore_attack_melee2.wav");
		PRECACHE_SOUND("voltigore/voltigore_eat.wav");
		PRECACHE_SOUND("voltigore/voltigore_attack_shock.wav");
		PRECACHE_SOUND("voltigore/voltigore_communicate3.wav");

		PRECACHE_SOUND("voltigore/voltigore_pain1.wav");
		PRECACHE_SOUND("voltigore/voltigore_pain2.wav");
	}

	RegisterVisual(chargeBeamVisual);
	RegisterVisual(deathBeamVisual);

	UTIL_PrecacheOther("charged_bolt", GetProjectileOverrides());
}

//=========================================================
// DeathSound
//=========================================================
void CVoltigore::DeathSound(void)
{
	EmitSoundScript(dieSoundScript);
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slVoltigoreRangeAttack1[] =
{
	{
		tlVoltigoreRangeAttack1,
		ARRAYSIZE(tlVoltigoreRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Voltigore Range Attack1"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlVoltigoreVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, 0.2f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,	50.0f },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
};

Schedule_t slVoltigoreVictoryDance[] =
{
	{
		tlVoltigoreVictoryDance,
		ARRAYSIZE( tlVoltigoreVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"VoltigoreVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES(CVoltigore)
{
	slVoltigoreRangeAttack1,
	slVoltigoreVictoryDance
};

IMPLEMENT_CUSTOM_SCHEDULES(CVoltigore, CSquadMonster)

Schedule_t *CVoltigore::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CSquadMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	default:
		break;
	}

	return CSquadMonster::GetSchedule();
}

Schedule_t* CVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slVoltigoreRangeAttack1[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slVoltigoreVictoryDance[0];
		break;
	}

	return CSquadMonster::GetScheduleOfType(Type);
}

void CVoltigore::StartTask(Task_t *pTask)
{
	ClearBeams();

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		{
			UTIL_MakeVectors(pev->angles);

			const auto vecConverge = BoltPosition();

			for (auto i = 0; i < 3; ++i)
			{
				CBeam* pBeam = CreateBeamFromVisual(GetVisual(chargeBeamVisual));
				m_pBeam[i] = pBeam;

				if (!pBeam)
					return;

				pBeam->PointEntInit(vecConverge, entindex());
				pBeam->SetEndAttachment(i + 1);
			}

			m_pChargedBolt = CChargedBolt::ChargedBoltCreate(GetProjectileOverrides());

			UTIL_SetOrigin(m_pChargedBolt->pev, vecConverge);

			EmitSoundScriptAmbient(pev->origin, beamAttackSoundScript);
			CSquadMonster::StartTask(pTask);
		}
		break;
	case TASK_DIE:
	{
		SetThink(&CVoltigore::CallDeathGibThink);
		CSquadMonster::StartTask(pTask);
	}
	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}

void CVoltigore::OnDying()
{
	ClearBeams();
	CSquadMonster::OnDying();
}

void CVoltigore::UpdateOnRemove()
{
	if (m_pChargedBolt != 0)
		UTIL_Remove(m_pChargedBolt);
	ClearBeams();
	CSquadMonster::UpdateOnRemove();
}

void CVoltigore::UpdateBeamAndBoltPositions()
{
	if (m_pChargedBolt)
	{
		const Vector pos = BoltPosition();
		UTIL_SetOrigin(m_pChargedBolt->pev, pos);

		for (int i=0; i<ARRAYSIZE(m_pBeam); ++i)
		{
			CBeam* pBeam = m_pBeam[i].Entity<CBeam>();
			if (!pBeam)
				continue;
			pBeam->SetStartPos(pos);
			pBeam->RelinkBeam();
		}
	}
}

void CVoltigore::ClearBeams()
{
	for (auto& pBeam : m_pBeam)
	{
		if (pBeam)
		{
			UTIL_Remove(pBeam);
			pBeam = nullptr;
		}
	}

	if (m_pChargedBolt)
	{
		UTIL_Remove(m_pChargedBolt);
		m_pChargedBolt = nullptr;
	}
}

Vector CVoltigore::BoltPosition()
{
	UTIL_MakeVectors(pev->angles);
	return pev->origin + gpGlobals->v_forward * 50 + gpGlobals->v_up * 32;
}

//=========================================================
// CBabyAlienVoltigore
//=========================================================

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BABY_VOLTIGORE_AE_RUN			( 14 )

class CBabyVoltigore : public CVoltigore
{
public:
	void	Spawn(void);
	void	Precache(void);
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("babyvoltigore"); }
	const char* DefaultDisplayName() { return "Baby Voltigore"; }
	void	HandleAnimEvent(MonsterEvent_t* pEvent);
	bool	CheckMeleeAttack1(float flDot, float flDist) override;
	bool	CheckRangeAttack1(float flDot, float flDist) override;
	void	StartTask(Task_t *pTask);
	void	GibMonster();
	const char* DefaultGibModel() {
		return CSquadMonster::DefaultGibModel();
	}
	int DefaultGibCount() {
		return CSquadMonster::DefaultGibCount();
	}
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType(int Type);

	virtual int DefaultSizeForGrapple() { return GRAPPLE_SMALL; }
	Vector DefaultMinHullSize() { return Vector( -16.0f, -16.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 16.0f, 16.0f, 32.0f ); }

	void IdleSound();
	void AlertSound();
	void PlayPainSound();
	void DeathSound();
	void AttackSound();

	static constexpr const char* idleSoundScript = "BabyVoltigore.Idle";
	static constexpr const char* alertSoundScript = "BabyVoltigore.Alert";
	static constexpr const char* painSoundScript = "BabyVoltigore.Pain";
	static constexpr const char* dieSoundScript = "BabyVoltigore.Die";
	static constexpr const char* attackHitSoundScript = "BabyVoltigore.AttackHit";
	static constexpr const char* attackMissSoundScript = "BabyVoltigore.AttackMiss";
	static constexpr const char* footstepSoundScript = "BabyVoltigore.Footstep";
	static const NamedSoundScript attackSoundScript;
};

LINK_ENTITY_TO_CLASS(monster_alien_babyvoltigore, CBabyVoltigore)

const NamedSoundScript CBabyVoltigore::attackSoundScript = {
	CHAN_VOICE,
	{"voltigore/voltigore_attack_melee1.wav", "voltigore/voltigore_attack_melee2.wav"},
	130,
	"BabyVoltigore.Attack"
};

//=========================================================
// Spawn
//=========================================================
void CBabyVoltigore::Spawn()
{
	Precache();

	SetMyModel("models/baby_voltigore.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor(BLOOD_COLOR_GREEN);
	pev->effects		= 0;
	SetMyHealth(gSkillData.babyVoltigoreHealth);
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_TURN_HEAD;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(false);

	m_flNextBeamAttackCheck	= gpGlobals->time;

	MonsterInit();
	pev->view_ofs		= Vector(0, 0, 32);
}

//=========================================================
//=========================================================
void CBabyVoltigore::Precache(void)
{
	PrecacheMyModel("models/baby_voltigore.mdl");

	SoundScriptParamOverride voiceParamOverride;
	voiceParamOverride.OverridePitchRelative(180);

	SoundScriptParamOverride bodyParamOverride;
	bodyParamOverride.OverridePitchRelative(130);

	RegisterAndPrecacheSoundScript(idleSoundScript, CVoltigore::idleSoundScript, voiceParamOverride);
	RegisterAndPrecacheSoundScript(alertSoundScript, CVoltigore::alertSoundScript, voiceParamOverride);
	RegisterAndPrecacheSoundScript(painSoundScript, CVoltigore::painSoundScript, voiceParamOverride);
	RegisterAndPrecacheSoundScript(dieSoundScript, CVoltigore::dieSoundScript, voiceParamOverride);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript, bodyParamOverride);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript, bodyParamOverride);
	RegisterAndPrecacheSoundScript(footstepSoundScript, CVoltigore::footstepSoundScript, bodyParamOverride);
	RegisterAndPrecacheSoundScript(attackSoundScript);
}

void CBabyVoltigore::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BABY_VOLTIGORE_AE_RUN:
		EmitSoundScript(footstepSoundScript);
		break;

	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		TraceHullAttackParams params;
		params.distance = 64.0f;
		params.punchAngle.z = -10;
		params.punchAngle.x = 10;
		params.knockRight = -100;
		params.knockUp = 50;
		params.damageInfo.damage = gSkillData.babyVoltigoreDmgPunch;
		params.damageInfo.type = DMG_CLUB;
		params.spawnBlood = true;
		params.bloodOrigin = vecArmPos;
		params.height = pev->size.z;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		TraceHullAttackParams params;
		params.distance = 64.0f;
		params.punchAngle.x = 15;
		params.knockForward = 100;
		params.knockUp = 50;
		params.damageInfo.damage = gSkillData.babyVoltigoreDmgPunch;
		params.damageInfo.type = DMG_CLUB;
		params.spawnBlood = true;
		params.bloodOrigin = vecArmPos;
		params.height = pev->size.z;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
	break;
	default:
		CVoltigore::HandleAnimEvent(pEvent);
		break;
	}
}

bool CBabyVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	CheckMeleeAttackParams params;
	params.distance = 64.0f;
	params.dot = 0.6f;
	return HasConditions(bits_COND_SEE_ENEMY) && CheckMeleeAttackImpl(flDot, flDist, params, false) && m_hEnemy != 0;
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for voltigore because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CBabyVoltigore::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK1:
	{
		CSquadMonster::StartTask(pTask);
		break;
	}
	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}

void CBabyVoltigore::GibMonster()
{
	CSquadMonster::GibMonster();
}

bool CBabyVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	return false;
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CBabyVoltigore::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CSquadMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			AttackSound();
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	default:
		break;
	}

	return CSquadMonster::GetSchedule();
}

Schedule_t *CBabyVoltigore::GetScheduleOfType(int Type)
{
	switch (Type) {
	// TODO:
	// For some cryptic reason baby voltigore tries to start the range attack even though its model does not have sequence with range attack activity. 
	// This hack is for preventing baby voltigore to do this.
	case SCHED_RANGE_ATTACK1:
		return GetScheduleOfType(SCHED_CHASE_ENEMY);
		break;
	default:
		return CVoltigore::GetScheduleOfType(Type);
		break;
	}
}

void CBabyVoltigore::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

void CBabyVoltigore::AlertSound()
{
	EmitSoundScript(idleSoundScript);
}

void CBabyVoltigore::PlayPainSound()
{
	EmitSoundScript(painSoundScript);
}

void CBabyVoltigore::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

void CBabyVoltigore::AttackSound()
{
	EmitSoundScript(attackSoundScript);
}

#endif
