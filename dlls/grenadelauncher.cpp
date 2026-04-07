#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"

enum gl_e
{
	GL_IDLE = 0,
	PL_IDLE = 1,
	GL_FIRE = 2,
	PL_FIRE = 3,
	GL_RELOAD_DN = 4,
	GL_RELOAD_UP = 5,
	PL_RELOAD_DN = 6,
	PL_RELOAD_UP = 7,
	GL_DRAW = 8,
	PL_DRAW = 9,
	GL_HOLSTER = 10,
	PL_HOLSTER = 11
};

class CGrenadeLauncher : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_GRENADE_LAUNCHER; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS(weapon_grenadelauncher, CGrenadeLauncher)

bool CGrenadeLauncher::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 3;
	p->iPosition = 8;
	return true;
}

WeaponParameters CGrenadeLauncher::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 6;
	params.maxClip = 6;
	params.ammoName = "grenades";

	params.worldModel = "models/w_weaponbox.mdl";
	params.viewModel = "models/v_glauncher.mdl";
	params.playerModel = "models/p_glauncher.mdl";
	params.playerAnimExt = "mp5";
	params.priority = 20;

	params.deploy.animIndex = GL_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{GL_IDLE, 1.0f, 3.6f}
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.anims = {GL_FIRE};

	params.fire.projectileName = "grenaderound";
	params.fire.projectileOffsetForward = 8.0f;
	params.fire.projectileOffsetSide = 8.0f;
	params.fire.projectileOffsetUp = -16.0f;
	params.fire.projectileRespectPunchangle = false;
	params.fire.projectileAdjustToCross = true;

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/glauncher.wav"},
		0.7f,
		ATTN_NORM,
		100
	};

	params.fire.cycleTime = 0.6f;
	params.fire.idleDelay = 0.6f;
	params.fire.allowUnderwater = true;

	params.fire.autoAimDegree = AUTOAIM_5DEGREES;
	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = BRIGHT_GUN_FLASH;
	//

	params.startReload.animIndex = GL_RELOAD_DN;
	params.startReload.duration = 0.1f;

	params.reloadAutostart = true;
	params.manualReload = true;
	params.manualReloadContinueOnDeploy = false;

	params.reload.idleDelay = 2.0f / 3.0f;
	params.reload.duration = 2.0f / 3.0f;
	params.reload.waitForRecoil = true;

	params.endReload.animIndex = GL_RELOAD_UP;
	params.endReload.idleDelay = 0.6f;

	params.dropAmmo.classname = "ammo_grenadeclip";

	return params;
}
