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

enum shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00 )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

class CShotgun : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_SHOTGUN; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_shotgun, CShotgun )

bool CShotgun::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 2;
	p->iPosition = 1;

	return true;
}

WeaponParameters CShotgun::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 12;
	params.maxClip = 8;
	params.ammoName = "buckshot";

	params.worldModel = "models/w_shotgun.mdl";
	params.viewModel = "models/v_shotgun.mdl";
	params.playerModel = "models/p_shotgun.mdl";
	params.playerAnimExt = "shotgun";
	params.priority = 15;

	params.deploy.animIndex = SHOTGUN_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{SHOTGUN_IDLE_DEEP, 0.8f, 60.0f/12.0f},
		WeaponParameters::IdleAnim{SHOTGUN_IDLE, 0.15f, 20.0f/9.0f},
		WeaponParameters::IdleAnim{SHOTGUN_IDLE4, 0.05f, 20.0f/9.0f}
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damage = ::GetSkillValueRange("plr_buckshot");
	params.fire.anims.main = {SHOTGUN_FIRE};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/sbarrel1.wav"},
		FloatRange(0.95f, 1.0f),
		ATTN_NORM,
		IntRange(93, 124)
	};

	params.fire.spread.SetStaticSpread(false, bIsMultiplayer() ? VECTOR_CONE_DM_SHOTGUN : VECTOR_CONE_10DEGREES);
	params.fire.cycleTime = 0.75f;
	params.fire.idleDelay.main = 5.0f;
	params.fire.idleDelay.mainEmptied = 0.75f;
	params.fire.allowUnderwater = false;
	params.fire.bulletCount = bIsMultiplayer() ? 4 : 6;

	params.fire.autoAimDegree = AUTOAIM_5DEGREES;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;

	params.fire.pumpDelay = 0.5f;
	params.fire.pumpSound = WeaponSoundScript{
		CHAN_ITEM,
		{"weapons/scock1.wav"},
		1.0f,
		ATTN_NORM,
		IntRange{95, 126}
	};

	params.fire.bulletDistance = 2048;

	params.fire.clientPunchPitch = -5.0f;
	params.fire.shellOffsetForward = 32;
	params.fire.shellOffsetUp = -12;
	params.fire.shellOffsetSide = 6;
	params.fire.shellModel = "models/shotgunshell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHOTSHELL;
	//

	// Alt fire
	params.fire.anims.alt = {SHOTGUN_FIRE2};

	params.fire.sound.alt = {
		CHAN_WEAPON,
		{"weapons/dbarrel1.wav"},
		FloatRange(0.98f, 1.0f),
		ATTN_NORM,
		IntRange(85, 116)
	};

	if (bIsMultiplayer())
		params.fire.spread.SetStaticSpread(true, VECTOR_CONE_DM_DOUBLESHOTGUN);
	params.fire.cycleTime.alt = 1.5f;
	params.fire.idleDelay.alt = 6.0f;
	params.fire.idleDelay.altEmptied = 1.5f;
	params.fire.ammoPerFire.alt = 2;
	params.fire.bulletCount.alt = bIsMultiplayer() ? 8 : 12;

	params.fire.pumpDelay.alt = 0.95f;

	params.fire.shellCount.alt = 2;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.startReload.animIndex = SHOTGUN_START_RELOAD;
	params.startReload.duration = 0.7f;

	params.reloadAutostart = true;
	params.manualReload = true;

	params.reload.animIndex = SHOTGUN_RELOAD;
	params.reload.idleDelay = 0.5f;
	params.reload.duration = 0.0f;
	params.reload.sound = {
		CHAN_ITEM,
		{"weapons/reload1.wav", "weapons/reload3.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(85, 114)
	};
	params.reload.waitForRecoil = true;

	params.endReload.animIndex = SHOTGUN_PUMP;
	params.endReload.idleDelay = 1.5f;
	params.endReload.attackDelay = 0.0f;
	params.endReload.sound = {
		CHAN_ITEM,
		{"weapons/scock1.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(95, 124)
	};

	params.dropAmmo.classname = "ammo_buckshot";

	return params;
}
