#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"
#include "soundent_bits.h"

enum nailgun_e
{
	NAILGUN_LONGIDLE = 0,
	NAILGUN_IDLE1,
	NAILGUN_LAUNCH,
	NAILGUN_RELOAD,
	NAILGUN_DEPLOY,
	NAILGUN_FIRE1,
	NAILGUN_FIRE2,
	NAILGUN_FIRE3
};

#if !CLIENT_DLL
class CNail : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;

	void EXPORT NailTouch(CBaseEntity* pOther);
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;

	static const NamedVisual modelVisual;
	static const NamedSoundScript hitBodySoundScript;
	static const NamedSoundScript hitWallSoundScript;
};

LINK_ENTITY_TO_CLASS(nail, CNail)

const NamedVisual CNail::modelVisual = BuildVisual("Nail.Model")
		.Model("models/nail.mdl");

const NamedSoundScript CNail::hitBodySoundScript = {
	CHAN_BODY,
	{"weapons/xbow_hitbod1.wav", "weapons/xbow_hitbod2.wav"},
	1.0f,
	ATTN_NORM,
	IntRange(105, 110),
	"Nail.HitBody"
};

const NamedSoundScript CNail::hitWallSoundScript = {
	CHAN_BODY,
	{"weapons/ric1.wav", "weapons/ric2.wav", "weapons/ric3.wav", "weapons/ric4.wav", "weapons/ric5.wav"},
	"Nail.HitWall"
};

void CNail::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;

	ApplyVisualWithOwn(GetVisual(modelVisual));

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CNail::NailTouch);

	if (!FNullEnt(pev->owner) && FBitSet(pev->owner->v.flags, FL_CLIENT))
	{
		SetDefaultProjectileDamage(GetSkillValue("plr_nail"));
	}
	else
	{
		SetDefaultProjectileDamage(GetSkillValue("nail"));
	}
}

void CNail::Precache()
{
	RegisterVisualAsMineOwn(modelVisual);
	RegisterAndPrecacheSoundScript(hitBodySoundScript);
	RegisterAndPrecacheSoundScript(hitWallSoundScript);
}

void CNail::NailTouch(CBaseEntity *pOther)
{
	SetTouch(nullptr);

	TraceResult tr = UTIL_GetGlobalTrace();
	DecalGunshot(&tr);

	if (pOther->pev->takedamage)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		DamageInfo damageInfo(GetProjectileDamage(), DMG_GENERIC);
		damageInfo.SetGibPolicy(GIB_NEVER);
		pOther->ApplyTraceAttack(pev, pevOwner, damageInfo, pev->velocity.Normalize(), &tr);

		if (FBitSet(pOther->pev->flags, FL_CLIENT) || RANDOM_LONG(0, 1))
			EmitSoundScript(hitBodySoundScript);
	}
	else
	{
		if (RANDOM_LONG(0, 1))
			EmitSoundScript(hitWallSoundScript);
	}

	SetThink(&CNail::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

void CNail::LaunchAsProjectile(const ProjectileParameters& params)
{
	LaunchAsProjectileImpl(1000.0f, params);
}

#endif

class CNailgun : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_NAILGUN; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_nailgun, CNailgun )

bool CNailgun::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 2;
	p->iPosition = 5;
	return true;
}

WeaponParameters CNailgun::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 50;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "nails";

	params.worldModel = "models/w_weaponbox.mdl";
	params.viewModel = "models/v_nailgun.mdl";
	params.playerModel = "models/p_nailgun.mdl";
	params.playerAnimExt = "mp5";
	params.priority = 15;

	params.deploy.animIndex = NAILGUN_DEPLOY;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{NAILGUN_LONGIDLE, 0.5f, 36.0f / 8.0f},
		WeaponParameters::IdleAnim{NAILGUN_IDLE1, 0.5f, 36.0f / 35.0f},
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.anims.main = {NAILGUN_FIRE1, NAILGUN_FIRE2, NAILGUN_FIRE3};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/spike2.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(94, 109)
	};
	params.fire.cycleTime = 0.1f;
	params.fire.allowUnderwater = true;

	params.fire.weaponVolume = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;

	params.fire.clientPunchPitch = -1.0f;

	params.fire.projectileName = "nail";
	params.fire.projectileOffsetSide = 2.0f;
	params.fire.projectileOffsetUp = -4.0f;
	params.fire.projectileRespectPunchangle = true;
	params.fire.projectileAdjustToCross = true;
	//

	params.secondaryFireType = SecondaryFireType::DISABLED;

	return params;
}
