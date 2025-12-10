#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"effects.h"
#include	"customentity.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"shockbeam.h"
#include	"game.h"
#include	"visuals_utils.h"

#define SHOCK_BEAM_LENGTH		64
#define SHOCK_BEAM_LENGTH_HALF	SHOCK_BEAM_LENGTH * 0.5f

#define SHOCK_BEAM_WIDTH		50

LINK_ENTITY_TO_CLASS(shock_beam, CShock)

TYPEDESCRIPTION	CShock::m_SaveData[] =
{
	DEFINE_FIELD(CShock, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pNoise, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pSprite, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CShock, CBaseAnimating)

const NamedSoundScript CShock::impactSoundScript = {
	CHAN_WEAPON,
	{"weapons/shock_impact.wav"},
	FloatRange(0.8f, 0.9f),
	ATTN_NORM,
	"ShockBeam.Impact"
};

const NamedVisual CShock::spriteVisual = BuildVisual("ShockBeam.Sprite")
		.Model("sprites/flare3.spr")
		.Scale(0.35f)
		.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 255, kRenderFxDistort);

const NamedVisual CShock::beam1Visual = BuildVisual("ShockBeam.Beam1")
		.Model("sprites/lgtning.spr")
		.Alpha(180)
		.BeamParams(60, 0, 10)
		.RenderColor(0, 253, 253)
		.BeamFlags(BEAM_FSHADEOUT);

const NamedVisual CShock::beam2Visual = BuildVisual("ShockBeam.Beam2")
		.Model("sprites/lgtning.spr")
		.Alpha(180)
		.BeamParams(20, 30, 30)
		.RenderColor(255, 255, 157)
		.BeamFlags(BEAM_FSHADEOUT);

const NamedVisual CShock::lightVisual = BuildVisual("ShockBeam.Light")
		.Radius(80)
		.RenderColor(8, 253, 253)
		.Life(0.5f)
		.Decay(100.0f);

const NamedVisual CShock::shellVisual = BuildVisual("ShockBeam.Shell")
		.RenderColor(0, 220, 255)
		.Life(0.5f)
		.Alpha(5)
		.RenderFx(kRenderFxGlowShell);

void CShock::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("shock_beam");
	SET_MODEL(ENT(pev), "models/shock_effect.mdl");
	UTIL_SetOrigin(pev, pev->origin);

	if (!FNullEnt(pev->owner) && (pev->owner->v.flags & FL_CLIENT))
	{
		if (g_pGameRules->IsMultiplayer())
			SetDefaultProjectileDamage(gSkillData.plrDmgShockroachM);
		else
			SetDefaultProjectileDamage(gSkillData.plrDmgShockroach);
	}
	else
	{
		SetDefaultProjectileDamage(gSkillData.monDmgShockroach);
	}

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	CreateEffects();
	SetThink( &CShock::FlyThink );
	pev->nextthink = gpGlobals->time + 0.01;
}

void CShock::Precache()
{
	RegisterVisual(spriteVisual);
	RegisterVisual(beam1Visual);
	RegisterVisual(beam2Visual);
	RegisterVisual(lightVisual);
	RegisterVisual(shellVisual);
	PRECACHE_MODEL("models/shock_effect.mdl");
	RegisterAndPrecacheSoundScript(impactSoundScript);
}

void CShock::FlyThink()
{
	if (pev->waterlevel == WL_Eyes)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		EmitSoundScript(impactSoundScript);
		RadiusDamage(pev->origin, pev, pevOwner ? pevOwner : pev, DamageInfo(GetProjectileDamage() * 3, DMG_SHOCK).SetGibPolicy(GIB_ALWAYS), 144, CLASS_NONE );
		ClearEffects();
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.05;
	}
}

void CShock::Touch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner)
		return;

	TraceResult tr = UTIL_GetGlobalTrace();

	SendDynLight(pev->origin, GetVisual(lightVisual));

	CBaseMonster* pMonster = pOther->MyMonsterPointer();
	if (pMonster && pMonster->IsAlive())
	{
		pMonster->GlowShellOn(GetVisual(shellVisual));
	}

	ClearEffects();
	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		const int baseDecal = gDecals[DECAL_OPFOR_SCORCH1].index >= 0 ? DECAL_OPFOR_SCORCH1 : DECAL_SMALLSCORCH1;
		UTIL_DecalTrace(&tr, baseDecal + RANDOM_LONG(0, 2));

		int iContents = UTIL_PointContents(pev->origin);

		// Create sparks
		if (iContents != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}
	}
	else
	{
		int damageType = DMG_SHOCK;
		if (pMonster && !pMonster->IsAlive())
		{
			damageType |= DMG_CLUB;
		}
		entvars_t *pevOwner = VARS(pev->owner);
		entvars_t *pevAttacker = pevOwner ? pevOwner : pev;
		pOther->ApplyTraceAttack(pev, pevAttacker, DamageInfo{GetProjectileDamage(), damageType}, pev->velocity.Normalize(), &tr );
		if (pOther->IsPlayer() && (UTIL_PointContents(pev->origin) != CONTENTS_WATER))
		{
			const Vector position = tr.vecEndPos;
			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOther->pev );
				WRITE_BYTE( TE_SPARKS );
				WRITE_VECTOR( position );
			MESSAGE_END();
		}
	}

	// splat sound
	EmitSoundScript(impactSoundScript);

	pev->modelindex = 0;
	pev->solid = SOLID_NOT;
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.01; // let the sound play
}

void CShock::CreateEffects()
{
	m_pSprite = CreateSpriteFromVisual(GetVisual(spriteVisual), pev->origin);
	if (m_pSprite)
		m_pSprite->SetAttachment( edict(), 0 );

	m_pBeam = CreateBeamFromVisual(GetVisual(beam1Visual));
	if (m_pBeam)
	{
		UTIL_SetOrigin(m_pBeam->pev, pev->origin);
		m_pBeam->EntsInit( entindex(), entindex(), 1, 2 );
	}

	m_pNoise = CreateBeamFromVisual(GetVisual(beam2Visual));
	if (m_pNoise)
	{
		UTIL_SetOrigin(m_pNoise->pev, pev->origin);
		m_pNoise->EntsInit( entindex(), entindex(), 1, 2 );
	}
}

void CShock::ClearEffects()
{
	if (m_pBeam)
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}

	if (m_pNoise)
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}

	if (m_pSprite)
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
}

void CShock::UpdateOnRemove()
{
	ClearEffects();
	CBaseAnimating::UpdateOnRemove();
}

void CShock::LaunchAsProjectile(const ProjectileParameters& params)
{
	LaunchAsProjectileImpl(SHOCKBEAM_SPEED, params);
	pev->nextthink = gpGlobals->time;
}
