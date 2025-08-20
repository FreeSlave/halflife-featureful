/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"

enum eagle_e
{
	EAGLE_IDLE1 = 0,
	EAGLE_IDLE2,
	EAGLE_IDLE3,
	EAGLE_IDLE4,
	EAGLE_IDLE5,
	EAGLE_SHOOT,
	EAGLE_SHOOT_EMPTY,
	EAGLE_RELOAD,
	EAGLE_RELOAD_NOT_EMPTY,
	EAGLE_DRAW,
	EAGLE_HOLSTER
};

#if FEATURE_DESERT_EAGLE

LINK_WEAPON_TO_CLASS( weapon_eagle, CEagle )

void CEagle::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND ("weapons/desert_eagle_reload.wav");
}

bool CEagle::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 1;
	p->iPosition = 2;
	p->pszAmmoEntity = "ammo_357";
	p->iDropAmmo = AMMO_357BOX_GIVE;

	return true;
}

WeaponParameters CEagle::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 7;
	params.maxClip = 7;
	params.ammoName = "357";

	params.worldModel = "models/w_desert_eagle.mdl";
	params.viewModel = "models/v_desert_eagle.mdl";
	params.playerModel = "models/p_desert_eagle.mdl";
	params.playerAnimExt = "onehanded";
	params.priority = 15;

	params.deploy.animIndex = EAGLE_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{EAGLE_IDLE1, 0.3f, 2.5f},
		WeaponParameters::IdleAnim{EAGLE_IDLE2, 0.3f, 2.5f},
		WeaponParameters::IdleAnim{EAGLE_IDLE3, 0.4f, 1.63f}
	};
	params.idleAnims.mainEmptied = WeaponParameters::IdleAnimArray{};
	params.idleAnims.alt = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{EAGLE_IDLE5, 0.5f, 2.0f},
		WeaponParameters::IdleAnim{EAGLE_IDLE4, 0.5f, 2.5f}
	};
	params.idleAnims.altEmptied = WeaponParameters::IdleAnimArray{};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damage = gSkillData.plrDmgEagle;
	params.fire.anims.main = {EAGLE_SHOOT};
	params.fire.anims.mainEmptied = {EAGLE_SHOOT_EMPTY};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/desert_eagle_fire.wav"},
		FloatRange(0.92f, 1.0f),
		ATTN_NORM,
		IntRange(98, 101)
	};

	params.fire.spread.SetStaticSpread(false, 0.1f);
	params.fire.cycleTime = 0.22f;
	params.fire.allowUnderwater = false;

	params.fire.autoAimDegree = AUTOAIM_10DEGREES;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;

	params.fire.delayAfterEmpty = 0.2f;

	params.fire.clientPunchPitch = -4.0f;
	params.fire.shellOffsetForward = 18;
	params.fire.shellOffsetUp = -9;
	params.fire.shellOffsetSide = 9;
	params.fire.shellModel = "models/shell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;
	//

	// Alt fire
	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;

	params.fire.spread.SetStaticSpread(true, 0.001f);
	params.fire.cycleTime.alt = 0.5f;
	//

	params.fire.suspendLaserSpotTime = 0.6f;

	params.altMode.attackDelay = 0.5f;
	params.altMode.toggleLaserSpot = true;

	params.reload.animIndex = EAGLE_RELOAD_NOT_EMPTY;
	params.reload.duration = 1.5f;
	params.reload.idleDelay = FloatRange(10.0f, 15.0f);
	params.reload.suspendLaserSpotTime = 1.6f;

	params.reload.animIndex.mainEmptied = EAGLE_RELOAD;

	params.holster.animIndex = EAGLE_HOLSTER;
	params.holster.attackDelay = 0.5f;
	params.holster.idleDelay = FloatRange(10, 15);

	params.laserSpotAttractRockets = false;
	params.laserSpotScale = 0.5f;
	params.activateLaserSpotSound.waves = {"weapons/desert_eagle_sight.wav"};
	params.deactivateLaserSpotSound.waves = {"weapons/desert_eagle_sight2.wav"};

	return params;
}

#endif
