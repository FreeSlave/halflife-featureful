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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"
#include	"effects.h"
#include	"mod_features.h"
#include	"game.h"
#include	"combat.h"
#include	"clamp.h"
#include	"visuals_utils.h"

#if FEATURE_GENEWORM

#define GENEWORM_ATTN 0.1f

#define GENEWORM_SKIN_EYE_OPEN			0
#define GENEWORM_SKIN_EYE_LEFT			1
#define GENEWORM_SKIN_EYE_RIGHT			2
#define GENEWORM_SKIN_EYE_CLOSED		3

#define GENEWORM_ATTACHMENT_MOUTH 0
#define GENEWORM_ATTACHMENT_SPAWN 1
#define GENEWORM_ATTACHMENT_RIGHTEYE 2
#define GENEWORM_ATTACHMENT_LEFTEYE 3

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define GENEWORM_AE_BEAM			( 0 )		// Toxic beam attack (sprite trail)
#define GENEWORM_AE_PORTAL			( 2 )		// Create a portal that spawns an enemy.

#define GENEWORM_AE_MELEE_LEFT1		( 3 )		// Play hit sound
#define GENEWORM_AE_MELEE_LEFT2		( 4 )		// Activates the trigger_geneworm_hit

#define GENEWORM_AE_MELEE_RIGHT1	( 5 )		// Play hit sound
#define GENEWORM_AE_MELEE_RIGHT2	( 6 )		// Activates the trigger_geneworm_hit

#define GENEWORM_AE_MELEE_FORWARD1  ( 7 )		// Play hit sound
#define GENEWORM_AE_MELEE_FORWARD2  ( 8 )		// Activates the trigger_geneworm_hit

#define GENEWORM_AE_MAD				( 9 )		// Room starts shaking!

class CGeneWormCloud : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	void TurnOn();
	void RunGeneWormCloud(float frames);

	static CGeneWormCloud* GeneWormCloudCreate(const Vector& origin, EntityOverrides entityOverrides);
	void LaunchCloud(const Vector& origin, const Vector& aim, int nSpeed, edict_t* pOwner, float fadeTime);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	void EXPORT GeneWormCloudThink();
	void EXPORT CloudTouch( CBaseEntity *pOther );

	float m_maxFrame;

	float m_fadeScale;
	float m_fadeRender;
	float m_lastTime;

	bool m_bLaunched;
	float m_baseScale;

	static const NamedVisual cloudVisual;
};

LINK_ENTITY_TO_CLASS(env_genewormcloud, CGeneWormCloud)

TYPEDESCRIPTION CGeneWormCloud::m_SaveData[] =
{
	DEFINE_FIELD(CGeneWormCloud, m_maxFrame, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWormCloud, m_fadeScale, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWormCloud, m_fadeRender, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWormCloud, m_lastTime, FIELD_TIME),
	DEFINE_FIELD(CGeneWormCloud, m_bLaunched, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWormCloud, m_baseScale, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CGeneWormCloud, CBaseEntity)

const NamedVisual CGeneWormCloud::cloudVisual = BuildVisual("GeneWorm.Cloud")
	.Model("sprites/ballsmoke.spr")
	.RenderProps(kRenderGlow, Color3(0, 255, 0), 255, kRenderFxNoDissipation)
	.Framerate(10.0f)
	.Scale(0.5f);

void CGeneWormCloud::Precache()
{
	RegisterVisual(cloudVisual);
}

void CGeneWormCloud::Spawn()
{
	Precache();

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->effects = 0;
	pev->frame = 0;

	ApplyVisual(GetVisual(cloudVisual));
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	m_maxFrame = MODEL_FRAMES(pev->modelindex)-1;

	m_lastTime = gpGlobals->time;

	m_fadeScale = 0;
	m_bLaunched = false;

	m_baseScale = pev->scale ? pev->scale : 1.0f;
}

void CGeneWormCloud::CloudTouch(CBaseEntity *pOther)
{
	if ((!pev->owner || pOther->pev->modelindex != pev->owner->v.modelindex) && pev->modelindex != pOther->pev->modelindex)
	{
		if(pOther->pev->takedamage)
			pOther->TakeDamage(pev, pev, DamageInfo(gSkillData.gwormDmgSpit, DMG_ACID));

		pev->nextthink = gpGlobals->time;
		SetThink(NULL);
		UTIL_Remove(this);
	}
}


void CGeneWormCloud::TurnOn()
{
	pev->effects = 0;

	if (pev->framerate != 0 && m_maxFrame > 1.0f || (pev->spawnflags & 2) != 0)
	{
		SetThink(&CGeneWormCloud::GeneWormCloudThink);
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}

	pev->frame = 0;
}

void CGeneWormCloud::RunGeneWormCloud(float frames)
{
	if (m_bLaunched)
	{
		pev->scale += m_fadeScale;
		pev->renderamt -= m_fadeRender;

		if (pev->scale >= m_baseScale * 9.0f)
		{
			pev->scale = 0;
			UTIL_Remove(this);
			return;
		}
	}
	else if (pev->scale < m_baseScale * 4.0f)
	{
		pev->scale += m_baseScale * 0.1f;
	}

	pev->frame += frames;

	if (pev->frame > m_maxFrame && m_maxFrame > 0)
	{
		pev->frame = fmod(pev->frame, m_maxFrame);
	}
}

void CGeneWormCloud::GeneWormCloudThink()
{
	RunGeneWormCloud((gpGlobals->time - m_lastTime) * pev->framerate);

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

CGeneWormCloud* CGeneWormCloud::GeneWormCloudCreate(const Vector& origin, EntityOverrides entityOverrides)
{
	CGeneWormCloud* pCloud = GetClassPtr<CGeneWormCloud>(nullptr);

	pCloud->AssignEntityOverrides(entityOverrides);
	pCloud->Spawn();

	pCloud->pev->origin = origin;
	pCloud->pev->classname = MAKE_STRING("env_genewormcloud");
	pCloud->pev->solid = SOLID_BBOX;
	pCloud->pev->movetype = MOVETYPE_FLY;
	pCloud->pev->effects = 0;

	pCloud->TurnOn();

	pCloud->SetTouch(&CGeneWormCloud::CloudTouch);

	return pCloud;
}

void CGeneWormCloud::LaunchCloud(const Vector& origin, const Vector& aim, int nSpeed, edict_t* pOwner, float fadeTime)
{
	pev->angles = pOwner->v.angles;

	pev->owner = pOwner;

	pev->velocity = aim * nSpeed;

	m_fadeScale = 2.5 / fadeTime;
	m_fadeRender = (pev->renderamt - 7.0) / fadeTime;

	pev->skin = 0;
	pev->body = 0;
	pev->aiment = nullptr;
	pev->movetype = MOVETYPE_FLY;

	UTIL_SetOrigin(pev, origin);

	SetTouch(&CGeneWormCloud::CloudTouch);
	m_bLaunched = true;
}

constexpr int GENEWORM_SPAWN_BEAM_COUNT = 8;

class CGeneWormSpawn : public CBaseEntity
{
public:
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Precache() override;
	void Spawn() override;

	void EXPORT GeneWormSpawnThink();

	void EXPORT GeneWormSpawnTouch(CBaseEntity* pOther);

	void RunGeneWormSpawn(float frames);

	void TurnOn();

	static CGeneWormSpawn* GeneWormSpawnCreate(const Vector& origin, EntityOverrides entityOverrides);

	void LaunchSpawn(const Vector& origin, const Vector& aim, int nSpeed, edict_t* pOwner, float flBirthTime);

	void CreateWarpBeams(int side);

	float m_lastTime;
	float m_maxFrame;
	float m_flBirthTime;
	float m_flWarpTime;

	bool m_bLaunched;
	bool m_bWarping;
	bool m_bTrooperDropped;

	CBeam* m_pBeam[GENEWORM_SPAWN_BEAM_COUNT];

	int m_iBeams;

	static const NamedSoundScript spawnSoundScript;

	static const NamedVisual glowVisual;
	static const NamedVisual glowLightVisual;
	static const NamedVisual teleSpriteVisual;
	static const NamedVisual teleLightVisual;
	static const NamedVisual teleBeamVisual;
	static const NamedVisual teleBeamHitVisual;
};

LINK_ENTITY_TO_CLASS(env_genewormspawn, CGeneWormSpawn)

TYPEDESCRIPTION CGeneWormSpawn::m_SaveData[] =
{
	DEFINE_FIELD(CGeneWormSpawn, m_lastTime, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWormSpawn, m_maxFrame, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWormSpawn, m_flBirthTime, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWormSpawn, m_flWarpTime, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWormSpawn, m_bLaunched, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWormSpawn, m_bWarping, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWormSpawn, m_bTrooperDropped, FIELD_BOOLEAN),
	DEFINE_ARRAY(CGeneWormSpawn, m_pBeam, FIELD_CLASSPTR, GENEWORM_SPAWN_BEAM_COUNT),
	DEFINE_FIELD(CGeneWormSpawn, m_iBeams, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CGeneWormSpawn, CBaseEntity)

const NamedSoundScript CGeneWormSpawn::spawnSoundScript = {
	CHAN_WEAPON,
	{"debris/beamstart2.wav"},
	VOL_NORM,
	ATTN_NORM,
	"GeneWorm.Spawn"
};

const NamedVisual CGeneWormSpawn::glowVisual = BuildVisual("GeneWorm.SpawnGlow")
	.Model("sprites/boss_glow.spr")
	.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 255, kRenderFxNoDissipation)
	.Scale(0.95f).Framerate(10.0f);

const NamedVisual CGeneWormSpawn::glowLightVisual = BuildVisual("GeneWorm.SpawnGlowLight")
	.Radius(96.0f)
	.RenderColor(207, 0, 214)
	.Life(0.1f)
	.Decay(1.0f);

const NamedVisual CGeneWormSpawn::teleSpriteVisual = BuildVisual("GeneWorm.SpawnTeleSprite")
	.Model("sprites/tele1.spr");

const NamedVisual CGeneWormSpawn::teleLightVisual = BuildVisual("GeneWorm.SpawnTeleLight")
	.Radius(96.0f)
	.RenderColor(207, 214, 0)
	.Life(0.1f)
	.Decay(1.0f);

const NamedVisual CGeneWormSpawn::teleBeamVisual = BuildVisual("GeneWorm.SpawnTeleBeam")
	.Model("sprites/lgtning.spr")
	.RenderColor(255, 255, 0)
	.Alpha(192)
	.BeamWidth(30)
	.BeamNoise(80);

const NamedVisual CGeneWormSpawn::teleBeamHitVisual = BuildVisual("GeneWorm.SpawnTeleBeamHit")
	.Alpha(255)
	.BeamNoise(20)
	.Mixin(&CGeneWormSpawn::teleBeamVisual);

void CGeneWormSpawn::Precache()
{
	RegisterVisual(glowVisual);
	RegisterVisual(glowLightVisual);
	RegisterVisual(teleSpriteVisual);
	RegisterVisual(teleLightVisual);
	RegisterVisual(teleBeamVisual);
	RegisterVisual(teleBeamHitVisual);

	RegisterAndPrecacheSoundScript(spawnSoundScript);
}

void CGeneWormSpawn::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	Precache();

	ApplyVisual(GetVisual(glowVisual));

	UTIL_SetOrigin(pev, pev->origin);

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	m_maxFrame = MODEL_FRAMES(pev->modelindex) - 1;

	if (pev->angles.y != 0 && pev->angles.z == 0)
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}

	m_bLaunched = false;
	m_bWarping = false;
	m_bTrooperDropped = false;

	m_flBirthTime = gpGlobals->time;
	m_flWarpTime = gpGlobals->time;

	for (auto& pBeam : m_pBeam)
	{
		pBeam = nullptr;
	}

	m_iBeams = 0;
}

void CGeneWormSpawn::GeneWormSpawnThink()
{
	RunGeneWormSpawn((gpGlobals->time - m_lastTime) * pev->framerate);

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CGeneWormSpawn::GeneWormSpawnTouch(CBaseEntity* pOther)
{
	//Nothing
}

void CGeneWormSpawn::RunGeneWormSpawn(float frames)
{
	if (m_bLaunched)
	{
		if (gpGlobals->time > m_flBirthTime)
		{
			if (m_bWarping)
			{
				if (m_bTrooperDropped)
				{
					if (pev->scale < 0.1)
					{
						m_bLaunched = false;
						m_bWarping = false;
						m_bTrooperDropped = false;
						UTIL_Remove(this);
						return;
					}
					pev->scale = pev->scale - 0.075;
				}
				else
				{
					if (m_flWarpTime - 1.0 < gpGlobals->time)
					{
						UTIL_MakeVectors(pev->angles);

						const auto vecStart = pev->origin + Vector(0, 0, 1000);
						const auto vecEnd = pev->origin + Vector(0, 0, -1000);

						TraceResult tr;
						UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, dont_ignore_glass, edict(), &tr);

						m_bTrooperDropped = true;

						CBaseEntity *pEntity = CreateNoSpawn("monster_shocktrooper", tr.vecEndPos + Vector(0, 0, 48), pev->angles);
						if (pEntity)
						{
							CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
							if (pev->owner != 0 && pOwner)
							{
								CBaseMonster* geneworm = pOwner->MyMonsterPointer();
								if (geneworm)
								{
									CBaseMonster* strooper = pEntity->MyMonsterPointer();
									if (strooper)
									{
										strooper->m_iClass = geneworm->m_iClass;
										strooper->m_reverseRelationship = geneworm->m_reverseRelationship;
									}
								}
							}
							DispatchSpawn(pEntity->edict());
						}
					}
					else
					{
						::RadiusDamage(pev->origin, pev, pev, DamageInfo(1000.0, DMG_SHOCK).SetGibPolicy(GIB_ALWAYS), 128.0, CLASS_NONE);
						CreateWarpBeams(1);
						CreateWarpBeams(-1);
					}
				}
			}
			else
			{
				pev->velocity = g_vecZero;

				m_bWarping = true;
				m_flWarpTime = gpGlobals->time + 3.0;

				EmitSoundScript(spawnSoundScript);
			}
		}
		else
		{
			pev->scale = pev->scale + 0.1;
		}

		SendEntLight(entindex(), pev->origin, GetVisual(teleLightVisual));
	}
	else
	{
		SendEntLight(entindex(), pev->origin, GetVisual(glowLightVisual));
	}

	pev->frame += frames;

	if (pev->frame > m_maxFrame && m_maxFrame > 0)
	{
		pev->frame = fmod(pev->frame, m_maxFrame);
	}
}

void CGeneWormSpawn::TurnOn()
{
	pev->effects = 0;

	if (pev->framerate != 0 && m_maxFrame > 1.0 || (pev->spawnflags & 2) != 0)
	{
		SetThink(&CGeneWormSpawn::GeneWormSpawnThink);
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}

	pev->frame = 0;
}

CGeneWormSpawn* CGeneWormSpawn::GeneWormSpawnCreate(const Vector& origin, EntityOverrides entityOverrides)
{
	CGeneWormSpawn* pSpawn = GetClassPtr<CGeneWormSpawn>(nullptr);

	pSpawn->AssignEntityOverrides(entityOverrides);
	pSpawn->Spawn();

	pSpawn->pev->origin = origin;
	pSpawn->pev->classname = MAKE_STRING("env_genewormspawn");
	pSpawn->pev->solid = SOLID_NOT;
	pSpawn->pev->movetype = MOVETYPE_NONE;
	pSpawn->pev->effects = 0;

	pSpawn->TurnOn();

	return pSpawn;
}

void CGeneWormSpawn::LaunchSpawn(const Vector& origin, const Vector& aim, int nSpeed, edict_t* pOwner, float flBirthTime)
{
	pev->movetype = MOVETYPE_FLY;

	pev->angles = pOwner->v.angles;
	pev->owner = pOwner;
	pev->velocity = aim * nSpeed;

	pev->speed = nSpeed;
	pev->skin = 0;
	pev->body = 0;
	pev->aiment = nullptr;
	pev->movetype = MOVETYPE_FLY;

	ApplyVisual(GetVisual(teleSpriteVisual), nullptr, 0);

	m_maxFrame = MODEL_FRAMES(pev->modelindex) - 1;

	pev->frame = 0;

	UTIL_SetOrigin(pev, origin);

	SetTouch(&CGeneWormSpawn::GeneWormSpawnTouch);
	m_bLaunched = true;
	m_flBirthTime = flBirthTime + gpGlobals->time;
}

void CGeneWormSpawn::CreateWarpBeams(int side)
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= GENEWORM_SPAWN_BEAM_COUNT)
		return;

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

	auto pHit = Instance(tr.pHit);

	if (pHit && pHit->pev->takedamage != DAMAGE_NO)
	{
		m_pBeam[m_iBeams] = CreateBeamFromVisual(GetVisual(teleBeamHitVisual));
		if (!m_pBeam[m_iBeams])
			return;
		m_pBeam[m_iBeams]->EntsInit(pHit->entindex(), entindex());
	}
	else
	{
		m_pBeam[m_iBeams] = CreateBeamFromVisual(GetVisual(teleBeamVisual));
		if (!m_pBeam[m_iBeams])
			return;
		m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	}

	m_pBeam[m_iBeams]->SetThink(&CBeam::SUB_Remove);
	m_pBeam[m_iBeams]->pev->nextthink = gpGlobals->time + 1;

	++m_iBeams;
}

class CGeneWorm : public CBaseMonster
{
public:
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("geneworm"); }
	int  DefaultClassify(void) { return CLASS_RACEX_SHOCK; }
	void TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr) override;
	static bool FilterHurtTargets(CBaseEntity *pTarget, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void FireHurtTargets(const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType);

	void SetObjectCollisionBox(void)
	{
		SetMyObjectCollisionBox(Vector( -437, -720, -332 ), Vector( 425, 164, 355 ));
	}

	Vector LookerEyeOrigin() override;

	void HandleAnimEvent(MonsterEvent_t *pEvent);

	void EXPORT StartThink(void);
	void EXPORT HuntThink(void);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT NullThink(void);
	void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT HitTouch( CBaseEntity *pOther );

	void NextActivity(void);

	void TrackHead();

	int  TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo) override;

	bool ClawAttack();

	float m_flNextPainSound;

	Vector m_posTarget;
	float m_flLastSeen;

	int m_iWasHit;
	float m_flHitTime;
	float m_flNextMeleeTime;
	float m_flNextRangeTime;

	bool m_fRightEyeHit;
	bool m_fLeftEyeHit;
	bool m_fGetMad;

	bool m_fOrificeHit;
	float m_flOrificeOpenTime;
	CGeneWormSpawn* m_orificeGlow;

	bool m_fSpawningTrooper;

	int m_iHitTimes;
	int m_iMaxHitTimes;

	bool m_fSpitting;
	float m_flSpitStartTime;

	bool m_fActivated;
	float m_flDeathStart;
	bool m_fHasEntered;

	float m_flMadDelayTime;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript dieSoundScript;

	static const NamedSoundScript attack1SoundScript;
	static const NamedSoundScript attack2SoundScript;
	static const NamedSoundScript attack3SoundScript;
	static const NamedSoundScript beamAttackSoundScript;

	static const NamedSoundScript entrySoundScript;
	static const NamedSoundScript bigPain1SoundScript;
	static const NamedSoundScript bigPain2SoundScript;
	static const NamedSoundScript bigPain3SoundScript;
	static const NamedSoundScript bigPain4SoundScript;
	static const NamedSoundScript eyePainSoundScript;

	static const NamedSoundScript launchSpawnSoundScript;

	static const NamedVisual eyeLightVisual;
};

LINK_ENTITY_TO_CLASS(monster_geneworm, CGeneWorm)

TYPEDESCRIPTION CGeneWorm::m_SaveData[] =
{
	DEFINE_FIELD(CGeneWorm, m_flNextPainSound, FIELD_TIME),
	DEFINE_FIELD(CGeneWorm, m_posTarget, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CGeneWorm, m_flLastSeen, FIELD_TIME),
	DEFINE_FIELD(CGeneWorm, m_iWasHit, FIELD_INTEGER),
	DEFINE_FIELD(CGeneWorm, m_flHitTime, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWorm, m_flNextMeleeTime, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWorm, m_flNextRangeTime, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWorm, m_fRightEyeHit, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_fLeftEyeHit, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_fGetMad, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_fOrificeHit, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_flOrificeOpenTime, FIELD_FLOAT),
	DEFINE_FIELD(CGeneWorm, m_orificeGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(CGeneWorm, m_fSpawningTrooper, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_iHitTimes, FIELD_INTEGER),
	DEFINE_FIELD(CGeneWorm, m_iMaxHitTimes, FIELD_INTEGER),
	DEFINE_FIELD(CGeneWorm, m_fSpitting, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_flSpitStartTime, FIELD_TIME),
	DEFINE_FIELD(CGeneWorm, m_fActivated, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_flDeathStart, FIELD_TIME),
	DEFINE_FIELD(CGeneWorm, m_fHasEntered, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeneWorm, m_flMadDelayTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CGeneWorm, CBaseMonster)

const NamedSoundScript CGeneWorm::idleSoundScript = {
	CHAN_BODY,
	{"geneworm/geneworm_idle1.wav", "geneworm/geneworm_idle2.wav", "geneworm/geneworm_idle3.wav", "geneworm/geneworm_idle4.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	IntRange(95, 105),
	"GeneWorm.Idle"
};

const NamedSoundScript CGeneWorm::dieSoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_death.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.Die"
};

const NamedSoundScript CGeneWorm::attack1SoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_attack_mounted_rocket.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.Attack1"
};

const NamedSoundScript CGeneWorm::attack2SoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_attack_mounted_gun.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.Attack2"
};

const NamedSoundScript CGeneWorm::attack3SoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_big_attack_forward.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.Attack3"
};

const NamedSoundScript CGeneWorm::beamAttackSoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_beam_attack.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.BeamAttack"
};

const NamedSoundScript CGeneWorm::entrySoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_entry.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.Entry"
};

const NamedSoundScript CGeneWorm::bigPain1SoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_final_pain1.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.BigPain1"
};

const NamedSoundScript CGeneWorm::bigPain2SoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_final_pain2.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.BigPain2"
};

const NamedSoundScript CGeneWorm::bigPain3SoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_final_pain3.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.BigPain1"
};

const NamedSoundScript CGeneWorm::bigPain4SoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_final_pain4.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.BigPain4"
};

const NamedSoundScript CGeneWorm::eyePainSoundScript = {
	CHAN_VOICE,
	{"geneworm/geneworm_shot_in_eye.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	"GeneWorm.EyePain"
};

const NamedSoundScript CGeneWorm::launchSpawnSoundScript = {
	CHAN_WEAPON,
	{"debris/beamstart7.wav"},
	VOL_NORM,
	GENEWORM_ATTN,
	IntRange(95, 105),
	"GeneWorm.LaunchSpawn"
};

const NamedVisual CGeneWorm::eyeLightVisual = BuildVisual("GeneWorm.EyeLight")
	.Radius(48.0f)
	.RenderColor(128, 255, 128)
	.Life(0.1f)
	.Decay(2.0f);

void CGeneWorm::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SetMyModel("models/geneworm.mdl");

	UTIL_SetSize(pev, Vector( -437, -720, -332 ), Vector( 425, 164, 355 ));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;

	pev->effects = 0;

	SetMyHealth(gSkillData.gwormHealth);
	pev->max_health = pev->health;

	pev->view_ofs = Vector{0, 0, 300};

	SetMyBloodColor(BLOOD_COLOR_GREEN);
	SetMyFieldOfView(0.5f);

	SetThink(&CGeneWorm::StartThink);
	pev->nextthink = gpGlobals->time + 0.1;

	m_iWasHit = 0;
	m_fRightEyeHit = false;
	m_fLeftEyeHit = false;

	m_flHitTime = 0;

	m_fGetMad = false;
	m_fOrificeHit = false;

	m_flOrificeOpenTime = gpGlobals->time;

	m_iHitTimes = 0;

	m_MonsterState = MONSTERSTATE_IDLE;

	m_iMaxHitTimes = 4;

	pev->deadflag = DEAD_NO;

	m_fSpitting = false;
	m_fActivated = false;

	m_flSpitStartTime = gpGlobals->time;

	UTIL_SetOrigin(pev, pev->origin);

	pev->rendermode = kRenderTransTexture;

	m_fHasEntered = false;

	m_flMadDelayTime = gpGlobals->time;
}

void CGeneWorm::Precache()
{
	PrecacheMyModel("models/geneworm.mdl");

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);

	RegisterAndPrecacheSoundScript(attack1SoundScript);
	RegisterAndPrecacheSoundScript(attack2SoundScript);
	RegisterAndPrecacheSoundScript(attack3SoundScript);
	RegisterAndPrecacheSoundScript(beamAttackSoundScript);

	RegisterAndPrecacheSoundScript(entrySoundScript);
	RegisterAndPrecacheSoundScript(bigPain1SoundScript);
	RegisterAndPrecacheSoundScript(bigPain2SoundScript);
	RegisterAndPrecacheSoundScript(bigPain3SoundScript);
	RegisterAndPrecacheSoundScript(bigPain4SoundScript);
	RegisterAndPrecacheSoundScript(eyePainSoundScript);

	RegisterAndPrecacheSoundScript(launchSpawnSoundScript);

	RegisterVisual(eyeLightVisual);

	UTIL_PrecacheOther("monster_shocktrooper");
	UTIL_PrecacheOther("env_genewormcloud", GetProjectileOverrides());
	UTIL_PrecacheOther("env_genewormspawn", GetProjectileOverrides());
}

void CGeneWorm::StartThink(void)
{
	Vector vecEyePos, vecEyeAng;

	GetAttachment(GENEWORM_ATTACHMENT_MOUTH, vecEyePos, vecEyeAng);

	pev->view_ofs = vecEyePos - pev->origin;

	pev->frame = 0;
	pev->sequence = LookupSequence("entry");
	ResetSequenceInfo();
	m_flNextMeleeTime = gpGlobals->time;
	m_flNextRangeTime = gpGlobals->time;

	SetUse(&CGeneWorm::CommandUse);
	SetTouch(&CGeneWorm::HitTouch);
	SetThink(&CGeneWorm::HuntThink);

	pev->nextthink = gpGlobals->time + 0.1f;
}


void CGeneWorm::HitTouch( CBaseEntity *pOther )
{
	TraceResult tr = UTIL_GetGlobalTrace();

	if (pOther->pev->modelindex != pev->modelindex && m_flHitTime <= gpGlobals->time && tr.pHit && pev->modelindex == tr.pHit->v.modelindex)
	{
		m_flHitTime = gpGlobals->time + 0.5;

		//Apply damage to to the toucher based on what was hit
		switch (tr.iHitgroup)
		{
		case 1:
			pOther->TakeDamage(pev, pev, DamageInfo(10, DMG_CRUSH | DMG_SLASH));
			break;
		case 2:
			pOther->TakeDamage(pev, pev, DamageInfo(15, DMG_CRUSH | DMG_SLASH));
			break;
		case 3:
			pOther->TakeDamage(pev, pev, DamageInfo(20, DMG_CRUSH | DMG_SLASH));
			break;

		default:
			pOther->TakeDamage(pev, pev, DamageInfo(pOther->pev->health, DMG_CRUSH | DMG_SLASH));
			break;
		}

		pOther->pev->punchangle.z = 15;

		//TODO: maybe determine direction of velocity to apply?
		pOther->pev->velocity = pOther->pev->velocity + Vector{0, 0, 200};

		pOther->pev->flags &= ~FL_ONGROUND;
	}
}

void CGeneWorm::DyingThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	GlowShellUpdate();

	DispatchAnimEvents();
	StudioFrameAdvance();

	if (m_fSequenceFinished && pev->deadflag == DEAD_DYING)
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->deadflag == DEAD_NO)
	{
		pev->deadflag = DEAD_DYING;

		pev->frame = 0;

		//Note: bugged in vanilla, variable is not initialized and causes the ending sequence to break
		int iDir = 0;

		pev->sequence = FindTransition(pev->sequence, LookupSequence("death"), &iDir);

		if (iDir > 0)
			pev->frame = 255;
		else
			pev->frame = 0;

		pev->renderfx = kRenderFxNone;
		pev->rendermode = kRenderTransTexture;

		pev->renderamt = 255;

		ResetSequenceInfo();

		EmitSoundScript(dieSoundScript);

		m_flDeathStart = gpGlobals->time;

		FireTargets("GeneWormDead", this, this, USE_TOGGLE, 0);

		CBaseEntity* pTrooper = NULL;
		while((pTrooper = UTIL_FindEntityByClassname(pTrooper, "monster_shocktrooper")) != NULL)
			pTrooper->SUB_StartFadeOut();
	}

	if (gpGlobals->time - m_flDeathStart >= 15)
	{
		CBaseEntity* player = UTIL_FindEntityByClassname(0, "player");
		if (player)
		{
			CBaseEntity* teleport = UTIL_FindEntityByTargetname(0, "GeneWormTeleport");
			if (teleport)
				teleport->Touch(player);
			FireTargets("GeneWormTeleport", player, player, USE_ON);
		}

		m_flDeathStart = gpGlobals->time + 999;
	}

	if (pev->deadflag == DEAD_DYING)
	{
		pev->renderamt -= 1;
	}

	if (m_orificeGlow)
	{
		UTIL_Remove(m_orificeGlow);
		m_orificeGlow = nullptr;
	}
}

Vector CGeneWorm::LookerEyeOrigin()
{
	Vector vecLookerOrigin, vecLookerAngle;
	GetAttachment(GENEWORM_ATTACHMENT_MOUTH, vecLookerOrigin, vecLookerAngle);
	return vecLookerOrigin;
}

bool CGeneWorm::FilterHurtTargets(CBaseEntity *pTarget, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	return !(useType == USE_OFF && pTarget->pev->solid == SOLID_NOT) && !(useType == USE_ON && pTarget->pev->solid == SOLID_TRIGGER);
}

void CGeneWorm::FireHurtTargets(const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType)
{
	FireTargets(targetName, pActivator, pCaller, useType, 0.0f, FilterHurtTargets);
}

void CGeneWorm::NextActivity(void)
{
	UTIL_MakeAimVectors(pev->angles);

	if (m_hEnemy)
	{
		if (!m_hEnemy->IsAlive())
			m_hEnemy = nullptr;
	}

	if (gpGlobals->time > m_flLastSeen + 15.0 && nullptr != m_hEnemy && (pev->origin - m_hEnemy->pev->origin).Length2D() > 700.0)
	{
		m_hEnemy = nullptr;
	}

	if (!m_hEnemy)
	{
		Look(4096);
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_fGetMad)
	{
		pev->sequence = LookupSequence("mad");
		m_fGetMad = false;
		return;
	}

	if (m_fRightEyeHit && m_fLeftEyeHit)
	{
		if (gpGlobals->time <= m_flOrificeOpenTime && !m_fOrificeHit)
		{
			pev->sequence = LookupSequence("bigpain2");
			EmitSoundScript(bigPain2SoundScript);
			return;
		}

		m_flOrificeOpenTime = gpGlobals->time;

		if (!m_fSpawningTrooper)
		{
			pev->sequence = LookupSequence("bigpain4");
			EmitSoundScript(bigPain4SoundScript);
			m_fSpawningTrooper = true;
			return;
		}

		m_fLeftEyeHit = false;
		m_fRightEyeHit = false;
		m_fOrificeHit = false;
		pev->skin = GENEWORM_SKIN_EYE_OPEN;
		m_fSpawningTrooper = false;
	}

	if (m_hEnemy)
	{
		if (ClawAttack())
			return;

		if (m_iHitTimes > 1 && gpGlobals->time > m_flMadDelayTime && !RANDOM_LONG(0, m_iMaxHitTimes - m_iHitTimes) && m_hEnemy && FVisible(m_hEnemy))
		{
			pev->sequence = LookupSequence("mad");
			m_flMadDelayTime = gpGlobals->time + 15.0;
			return;
		}
	}

	switch (m_iHitTimes)
	{
	case 0:
		pev->sequence = LookupSequence("idlepain");
		break;
	case 1:
		pev->sequence = LookupSequence("idlepain2");
		break;
	case 2:
		pev->sequence = LookupSequence("idlepain3");
		break;
	default:
		break;
	}

	EmitSoundScript(idleSoundScript);
}

void CGeneWorm::TrackHead()
{
	Vector vecMouthPos, vecMouthAngle;
	GetAttachment(GENEWORM_ATTACHMENT_MOUTH, vecMouthPos, vecMouthAngle);
	Vector angles = UTIL_VecToAngles((m_posTarget - vecMouthPos).Normalize());
	float diff = UTIL_AngleDiff(angles.y, pev->angles.y);
	diff = clamp(diff, -30.0f, 30.0f);
	SetBoneController(0, diff);
}

bool CGeneWorm::ClawAttack()
{
	CBaseEntity* pEnemy = m_hEnemy.Entity<CBaseEntity>();

	if (pEnemy)
	{
		if (m_flNextMeleeTime <= gpGlobals->time)
		{
			m_posTarget = pEnemy->pev->origin;

			if (FVisible(m_posTarget))
			{
				const auto targetAngle = UTIL_VecToAngles((m_posTarget - pev->origin).Normalize());

				const auto yawDelta = UTIL_AngleDiff(targetAngle.y, pev->angles.y);

				if (gpGlobals->time > m_flNextRangeTime)
				{
					//TODO: never used?
					Vector vecMouthPos, vecMouthAngle;
					GetAttachment(GENEWORM_ATTACHMENT_MOUTH, vecMouthPos, vecMouthAngle);

					if (yawDelta >= 10.0)
						pev->sequence = LookupSequence("dattack2");
					else if (yawDelta > -10.0)
						pev->sequence = LookupSequence("dattack1");
					else
						pev->sequence = LookupSequence("dattack3");

					EmitSoundScript(beamAttackSoundScript);

					m_flNextRangeTime = RANDOM_FLOAT(10, 15) + gpGlobals->time;

					m_flNextMeleeTime = RANDOM_FLOAT(3, 5) + gpGlobals->time;
					return true;
				}

				if ((pev->origin - pEnemy->pev->origin).Length2D() < 1280)
				{
					if (m_posTarget.z <= pev->origin.z)
					{
						pev->sequence = LookupSequence("melee3");
					}
					else if (yawDelta >= 10.0)
					{
						pev->sequence = LookupSequence("melee1");
						EmitSoundScript(attack1SoundScript);
					}
					else if (yawDelta > -2.0)
					{
						pev->sequence = LookupSequence("melee3");
						EmitSoundScript(attack3SoundScript);
					}
					else
					{
						pev->sequence = LookupSequence("melee2");
						EmitSoundScript(attack2SoundScript);
					}

					m_flNextMeleeTime = RANDOM_FLOAT(3, 5) + gpGlobals->time;
					return true;
				}
			}
		}
	}

	return false;
}

void CGeneWorm::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr)
{
	const bool isLaser = 0 == strcmp("env_laser", STRING(pevInflictor->classname));

	if (ptr->iHitgroup != 4 && ptr->iHitgroup != 5 && ptr->iHitgroup != 6)
	{
		if (pev->dmgtime != gpGlobals->time || RANDOM_LONG(0, 10) <= 0)
		{
			if (isLaser)
			{
				UTIL_Sparks(ptr->vecEndPos);
			}
			else if ((damageInfo.type & DMG_BULLET) != 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
			}

			pev->dmgtime = gpGlobals->time;
		}

		return;
	}

	bool skipChecks = false;

	if (!isLaser)
	{
		if (ptr->iHitgroup != 6)
		{
			if (gpGlobals->time != pev->dmgtime || RANDOM_LONG(0, 10) <= 0)
			{
				if ((damageInfo.type & DMG_BULLET) != 0)
				{
					UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
				}
				pev->dmgtime = gpGlobals->time;
			}

			return;
		}

		skipChecks = true;
	}

	if (!skipChecks)
	{
		if (!m_fHasEntered)
		{
			UTIL_Sparks(ptr->vecEndPos);
			return;
		}

		if (m_fLeftEyeHit && m_fRightEyeHit || m_fGetMad)
			return;
	}

	switch (ptr->iHitgroup)
	{
	case 4:
	{
		if (!m_fLeftEyeHit)
		{
			if (0 == strcmp("left_eye_laser", STRING(pevInflictor->targetname)))
			{
				m_fLeftEyeHit = true;

				if (m_fRightEyeHit)
				{
					pev->skin = GENEWORM_SKIN_EYE_CLOSED;
					m_flOrificeOpenTime = gpGlobals->time + 20.0;
				}
				else
				{
					pev->skin = GENEWORM_SKIN_EYE_LEFT;
					m_fGetMad = true;
				}

				m_iWasHit = 1;

				if (m_bloodColor != DONT_BLEED)
				{
					SpawnBlood(ptr->vecEndPos - vecDir * 4, m_bloodColor, 300.0);
					TraceBleed(damageInfo.damage, vecDir, ptr, damageInfo.type);
				}
				break;
			}
		}

		UTIL_Sparks(ptr->vecEndPos);
		break;
	}

	case 5:
	{
		if (!m_fRightEyeHit)
		{
			if (0 == strcmp("right_eye_laser", STRING(pevInflictor->targetname)))
			{
				m_fRightEyeHit = true;

				if (m_fLeftEyeHit)
				{
					pev->skin = GENEWORM_SKIN_EYE_CLOSED;
					m_flOrificeOpenTime = gpGlobals->time + 20.0;
				}
				else
				{
					m_fGetMad = true;
					pev->skin = GENEWORM_SKIN_EYE_RIGHT;
				}

				m_iWasHit = 1;

				if (m_bloodColor != DONT_BLEED)
				{
					SpawnBlood(ptr->vecEndPos - vecDir * 4, m_bloodColor, 300.0);
					TraceBleed(damageInfo.damage, vecDir, ptr, damageInfo.type);
				}
				break;
			}
		}
		UTIL_Sparks(ptr->vecEndPos);
		break;
	}

	case 6:
	{
		if (m_flOrificeOpenTime > gpGlobals->time && !m_fOrificeHit)
		{
			pev->health -= damageInfo.damage;

			if (pev->health <= 0)
			{
				m_fOrificeHit = true;

				UTIL_BloodDecalTrace(ptr, m_bloodColor);

				++m_iHitTimes;

				if (m_iHitTimes >= m_iMaxHitTimes)
				{
					m_iWasHit = 2;
				}
				else
				{
					m_iWasHit = 1;
				}

				pev->health = pev->max_health;
			}
		}
		break;
	}

	default:
		break;
	}
}

void CGeneWorm::HuntThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (!m_fActivated)
		return;

	DispatchAnimEvents();
	StudioFrameAdvance();

	GlowShellUpdate();

	if (pev->rendermode == kRenderTransTexture)
	{
		if (pev->renderamt < 248.0)
		{
			pev->renderamt += 3.0;
		}
		else
		{
			pev->renderamt = 255;
			pev->rendermode = kRenderNormal;
			pev->renderfx = kRenderFxNone;
		}
	}

	if (m_iWasHit == 2)
	{
		SetThink(&CGeneWorm::DyingThink);
		m_fSequenceFinished = true;
		return;
	}

	if (m_iWasHit != 1)
	{
		if (m_fSequenceFinished)
		{
			const auto oldSequence = pev->sequence;

			if (!m_fHasEntered)
				m_fHasEntered = true;

			NextActivity();

			if (!m_fSequenceLoops || oldSequence != pev->sequence)
			{
				pev->frame = 0;
				ResetSequenceInfo();
			}
		}
	}
	else
	{
		int piDir = 1;

		if (m_fSpitting)
			m_fSpitting = false;

		if (!m_fRightEyeHit)
		{
			pev->sequence = FindTransition(pev->sequence, LookupSequence("eyepain1"), &piDir);
			EmitSoundScript(eyePainSoundScript);
		}
		else if (!m_fLeftEyeHit)
		{
			pev->sequence = FindTransition(pev->sequence, LookupSequence("eyepain2"), &piDir);
			EmitSoundScript(eyePainSoundScript);
		}
		else if (m_fOrificeHit)
		{
			pev->sequence = FindTransition(pev->sequence, LookupSequence("bigpain3"), &piDir);
			EmitSoundScript(bigPain3SoundScript);
		}
		else
		{
			pev->sequence = FindTransition(pev->sequence, LookupSequence("bigpain1"), &piDir);
			EmitSoundScript(bigPain1SoundScript);

			Vector vecOrigin, vecAngles;
			GetAttachment(GENEWORM_ATTACHMENT_SPAWN, vecOrigin, vecAngles);

			m_orificeGlow = CGeneWormSpawn::GeneWormSpawnCreate(vecOrigin, GetProjectileOverrides());
			m_orificeGlow->TurnOn();
		}

		if (piDir <= 0)
			pev->frame = 255;
		else
			pev->frame = 0;

		ResetSequenceInfo();

		m_iWasHit = 0;
	}

	if (!m_fRightEyeHit)
	{
		Vector vecOrigin, vecAngles;
		GetAttachment(GENEWORM_ATTACHMENT_RIGHTEYE, vecOrigin, vecAngles);

		SendEntLight(entindex(), vecOrigin, GetVisual(eyeLightVisual), 3);
	}

	if (!m_fLeftEyeHit)
	{
		Vector vecOrigin, vecAngles;
		GetAttachment(GENEWORM_ATTACHMENT_LEFTEYE, vecOrigin, vecAngles);

		SendEntLight(entindex(), vecOrigin, GetVisual(eyeLightVisual), 4);
	}

	if (m_orificeGlow)
	{
		//Keep the glow in place relative to the orifice
		Vector vecOrigin, vecAngles;
		GetAttachment(GENEWORM_ATTACHMENT_SPAWN, vecOrigin, vecAngles);
		UTIL_SetOrigin(m_orificeGlow->pev, vecOrigin);
	}

	if (m_hEnemy)
	{
		if (FVisible(m_hEnemy))
		{
			m_flLastSeen = gpGlobals->time;

			m_posTarget = m_hEnemy->pev->origin;
			m_posTarget.z += 24.0;
		}

		TrackHead();
	}
	else
	{
		//Look forward
		SetBoneController(0, 0);
	}

	if (m_fSpitting)
	{
		if (gpGlobals->time - m_flSpitStartTime <= 2.0)
		{
			if (m_hEnemy)
			{
				Vector vecOrigin, vecAngles;
				GetAttachment(GENEWORM_ATTACHMENT_MOUTH, vecOrigin, vecAngles);
				CGeneWormCloud* pCloud = CGeneWormCloud::GeneWormCloudCreate(vecOrigin, GetProjectileOverrides());

				if (edict())
				{
					pCloud->pev->skin = entindex();
					pCloud->pev->body = 1;
					pCloud->pev->aiment = edict();
					pCloud->pev->movetype = MOVETYPE_FOLLOW;
				}

				pCloud->TurnOn();
				pCloud->LaunchCloud(vecOrigin, (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecOrigin).Normalize() + Vector(0, 0, RANDOM_FLOAT(-0.01, 0.01)), 1000, edict(), 35);
			}
		}
		else
		{
			m_fSpitting = false;
		}
	}
}

void CGeneWorm::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GENEWORM_AE_BEAM:
		m_fSpitting = true;
		m_flSpitStartTime = gpGlobals->time;
		break;
	case GENEWORM_AE_PORTAL:
	{
		if (m_orificeGlow)
		{
			Vector vecPos, vecAng;
			GetAttachment(GENEWORM_ATTACHMENT_SPAWN, vecPos, vecAng);
			UTIL_MakeVectors(pev->angles);

			EmitSoundScript(launchSpawnSoundScript);

			m_orificeGlow->LaunchSpawn(vecPos, gpGlobals->v_forward, RANDOM_LONG(0, 50) + 300, edict(), 2);

			m_orificeGlow = nullptr;
		}
		break;
	}
	case GENEWORM_AE_MELEE_LEFT1:
		FireHurtTargets("GeneWormLeftSlash", this, this, USE_ON);
		break;
	case GENEWORM_AE_MELEE_LEFT2:
		FireHurtTargets("GeneWormLeftSlash", this, this, USE_OFF);
		break;
	case GENEWORM_AE_MELEE_RIGHT1:
		FireHurtTargets("GeneWormRightSlash", this, this, USE_ON);
		break;
	case GENEWORM_AE_MELEE_RIGHT2:
		FireHurtTargets("GeneWormRightSlash", this, this, USE_OFF);
		break;
	case GENEWORM_AE_MELEE_FORWARD1:
		FireHurtTargets("GeneWormCenterSlash", this, this, USE_ON);
		break;
	case GENEWORM_AE_MELEE_FORWARD2:
		FireHurtTargets("GeneWormCenterSlash", this, this, USE_OFF);
		break;
	case GENEWORM_AE_MAD:
		FireHurtTargets("GeneWormWallHit", this, this, USE_TOGGLE);
		UTIL_ScreenShake(pev->origin, 24, 3, 5, 2048);
		break;
	default:
		break;
	}
}

void CGeneWorm::CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if(useType == USE_TOGGLE && !m_fActivated)
	{
		pev->sequence = LookupSequence("entry");
		pev->frame = 0;
		ResetSequenceInfo();
		pev->rendermode = kRenderTransTexture;
		pev->renderfx = kRenderFxNone;
		pev->renderamt = 0;

		m_fActivated = true;

		pev->solid = SOLID_BBOX;

		UTIL_SetOrigin(pev, pev->origin);
		EmitSoundScript(entrySoundScript);
	}
}

int CGeneWorm::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo)
{
	if (damageInfo.damage >= pev->health)
	{
		pev->health = 1;
	}
	else
	{
		pev->health -= damageInfo.damage;
	}

	return 0;
}

#endif
