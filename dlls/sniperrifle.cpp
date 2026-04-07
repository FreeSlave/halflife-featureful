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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"
#include "mod_features.h"

enum sniper_e
{
	SNIPER_DRAW = 0,
	SNIPER_SLOWIDLE1,
	SNIPER_FIRE,
	SNIPER_FIRELASTROUND,
	SNIPER_RELOAD1,
	SNIPER_RELOAD2,
	SNIPER_RELOAD3,
	SNIPER_SLOWIDLE2,
	SNIPER_HOLSTER
};

class CSniperrifle : public CConfigurableWeapon
{
public:
	void PrecacheDefaultModelSounds() override;
	int WeaponId() const override { return WEAPON_SNIPERRIFLE; }

	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_sniperrifle, CSniperrifle )

void CSniperrifle::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND ("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND ("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_second_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload3.wav");
}

bool CSniperrifle::GetItemInfo(ItemInfo *p)
{
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 5;
	p->iPosition = 2;
#else
	p->iSlot = 2;
	p->iPosition = 4;
#endif

	return true;
}

WeaponParameters CSniperrifle::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 5;
	params.maxClip = 5;
	params.ammoName = "762";

	params.worldModel = "models/w_m40a1.mdl";
	params.viewModel = "models/v_m40a1.mdl";
	params.playerModel = "models/p_m40a1.mdl";
	params.playerAnimExt = "bow";
	params.priority = 10;

	params.deploy.animIndex = SNIPER_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{SNIPER_SLOWIDLE1, 1.0f, 67.5f / 16.0f}
	};
	params.idleAnims.mainEmptied = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{SNIPER_SLOWIDLE2, 1.0f, 80.0f / 16.0f}
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damage = ::GetSkillValueRange("plr_762_bullet");
	params.fire.anims.main = {SNIPER_FIRE};
	params.fire.anims.mainEmptied = {SNIPER_FIRELASTROUND};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/sniper_fire.wav"},
		FloatRange(0.9f, 1.0f),
		ATTN_NORM,
		IntRange(98, 101)
	};

	params.fire.spread.SetStaticSpread(false, 0.001f);
	params.fire.cycleTime = 1.75f;
	params.fire.idleDelay = 68.0f / 38.0f;
	params.fire.allowUnderwater = false;

	params.fire.autoAimDegree = AUTOAIM_2DEGREES;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = BRIGHT_GUN_FLASH;

	params.fire.delayAfterEmpty = 0.2f;

	params.fire.clientPunchPitch = -5.0f;
	//

	// Alt fire
	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
	params.altMode.zoomFOV = 18;
	params.altMode.attackDelay = 0.5f;
	params.altMode.zoomSound.waves = {"weapons/sniper_zoom.wav"};
	//

	params.reload.animIndex = SNIPER_RELOAD3;
	params.reload.duration = 80.0f / 34.0f;

	params.reload.animIndex.mainEmptied = SNIPER_RELOAD1;
	params.reload.duration.mainEmptied = 80.0f / 34.0f;
	params.reload.idleDelay.mainEmptied = 80.0f / 34.0f;

	params.endReload.animIndex.mainEmptied = SNIPER_RELOAD2;
	params.endReload.attackDelay.mainEmptied = 49.0f / 27.0f;
	params.endReload.idleDelay.mainEmptied = 49.0f / 27.0f;

	params.holster.animIndex = SNIPER_HOLSTER;
	params.holster.attackDelay = 0.25f;

	params.dropAmmo.classname = "ammo_762";

	return params;
}
