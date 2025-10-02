#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"

enum minigun_e
{
	MINIGUN_IDLE = 0,
	MINIGUN_IDLE2,
	MINIGUN_GENTLEIDLE,
	MINIGUN_STILLIDLE,
	MINIGUN_DRAW,
	MINIGUN_HOLSTER,
	MINIGUN_SPINUP,
	MINIGUN_SPINDOWN,
	MINIGUN_SPINIDLE,
	MINIGUN_SPINFIRE,
	MINIGUN_SPINIDLEDOWN,
};

class CMinigun : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_MINIGUN; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS(weapon_minigun, CMinigun)

bool CMinigun::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 3;
	p->iPosition = 7;
	return true;
}

WeaponParameters CMinigun::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 200;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "556";

	params.worldModel = "models/w_minigun.mdl";
	params.viewModel = "models/v_minigun.mdl";
	params.playerModel = "models/p_minigunidle.mdl";
	params.playerAnimExt = "mp5";
	params.priority = 15;

	params.deploy.animIndex = MINIGUN_DRAW;
	params.deploy.duration = 2.0f;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{MINIGUN_GENTLEIDLE, 0.85f, 6.2f},
		WeaponParameters::IdleAnim{MINIGUN_IDLE, 0.1f, 6.2f},
		WeaponParameters::IdleAnim{MINIGUN_IDLE2, 0.05f, 6.2f},
	};

	// Primary fire
	params.fire.chargeAnims.main = {MINIGUN_SPINUP};
	params.fire.chargeTime = 1.0f;
	params.fire.chargeSound = {
		CHAN_WEAPON,
		{"hassault/hw_spinup.wav"},
		1.0f,
		ATTN_NORM,
		100
	};

	params.fire.cooldownAnims.main = {MINIGUN_SPINDOWN};
	params.fire.cooldownTime = 2.0f;
	params.fire.cooldownSound = {
		CHAN_WEAPON,
		{"hassault/hw_spindown.wav"},
		1.0f,
		ATTN_NORM,
		100
	};

	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damage = gSkillData.plrDmg556;
	params.fire.anims = {MINIGUN_SPINFIRE};

	params.fire.sound = {
		CHAN_WEAPON,
		{"hassault/hw_shoot2.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(94, 109)
	};

	params.fire.cycleTime = 0.04f;
	params.fire.idleDelay = 1.0f;
	params.fire.allowUnderwater = false;
	params.fire.tracerFreq = 2;
	params.fire.spread.SetStaticSpread(false, VECTOR_CONE_4DEGREES);
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;

	params.fire.shellOffsetForward = 14;
	params.fire.shellOffsetUp = -12;
	params.fire.shellOffsetSide = 4;
	params.fire.shellModel = "models/saw_shell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;

	WeaponKickBack duckKickBack;
	duckKickBack.verticalBase = 1.0f;
	duckKickBack.lateralBase = 0.5f;

	WeaponKickBack standKickBack;
	standKickBack.verticalBase = 1.3f;
	standKickBack.lateralBase = 0.65f;

	WeaponKickBackProfile::RuleList kickBackRules = {
		WeaponKickBackRule{
			PlayerMovementConditions().Ducking(),
			duckKickBack
		},
		WeaponKickBackRule{
			PlayerMovementConditions(),
			standKickBack
		}
	};

	params.fire.kickBack.SetKickBack(false, kickBackRules);

	params.fire.playerMaxSpeed.main.value = 0.1f;
	params.fire.playerMaxSpeed.main.isFactor = true;
	//

	// Alt fire
	params.fire.fireType.alt = WeaponParameters::Fire::NATIVE;
	params.fire.anims.alt = {MINIGUN_SPINIDLE};
	params.fire.cycleTime.alt = 0.205f;
	params.fire.ammoPerFire.alt = 0;
	params.fire.sound.alt = {
		CHAN_WEAPON,
		{"hassault/hw_spin.wav"},
		1.0f,
		ATTN_NORM,
		100
	};
	params.fire.sound.alt->looped = true;

	params.fire.kickBack.SetKickBack(true, 0.0f);
	params.fire.muzzleFlash.alt = false;
	params.fire.weaponVolume = QUIET_GUN_VOLUME;
	params.fire.weaponFlash = 0;

	params.fire.playerMaxSpeed.Materialize(true).value = 0.4f;
	params.fire.playerMaxSpeed.Materialize(true).isFactor = true;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;
	params.primaryFirePrioritized = true;
	params.playerMaxSpeed.main.value = 0.6f;
	params.playerMaxSpeed.main.isFactor = true;
	params.preventJump = true;

	params.holster.animIndex = MINIGUN_HOLSTER;
	params.holster.attackDelay = 0.5f;

	return params;
}
