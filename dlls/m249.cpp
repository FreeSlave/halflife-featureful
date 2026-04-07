/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "mod_features.h"

enum m249_e
{
	M249_SLOWIDLE = 0,
	M249_IDLE2,
	M249_RELOAD,
	M249_RELOAD_END,
	M249_HOLSTER,
	M249_DEPLOY,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_SHOOT3
};

class CM249 : public CConfigurableWeapon
{
public:
	void PrecacheDefaultModelSounds() override;
	int WeaponId() const override { return WEAPON_M249; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS(weapon_m249, CM249)

//=========================================================
//=========================================================

void CM249::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");
}

bool CM249::GetItemInfo(ItemInfo *p)
{
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 5;
	p->iPosition = 0;
#else
	p->iSlot = 2;
	p->iPosition = 3;
#endif

	return true;
}

WeaponParameters CM249::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 50;
	params.maxClip = 50;
	params.ammoName = "556";

	params.worldModel = "models/w_saw.mdl";
	params.viewModel = "models/v_saw.mdl";
	params.playerModel = "models/p_saw.mdl";
	params.playerAnimExt = "mp5";
	params.priority = 15;

	params.deploy.animIndex = M249_DEPLOY;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{M249_SLOWIDLE, 0.8f, 5.0f},
		WeaponParameters::IdleAnim{M249_IDLE2, 0.2f, 155.0f / 25.0f},
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damage = ::GetSkillValueRange("plr_556_bullet");
	params.fire.anims = {M249_SHOOT1, M249_SHOOT2, M249_SHOOT3};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/saw_fire1.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(94, 109)
	};

	if (bIsMultiplayer())
	{
		params.fire.spread.SetStaticSpread(false, {
			WeaponSpreadRule::Static(VECTOR_CONE_3DEGREES, PlayerMovementConditions::Ducking()),
			WeaponSpreadRule::Static(VECTOR_CONE_15DEGREES, PlayerMovementConditions::Moving()),
			WeaponSpreadRule::Static(VECTOR_CONE_6DEGREES, PlayerMovementConditions())
		});
	}
	else
	{
		params.fire.spread.SetStaticSpread(false, {
			WeaponSpreadRule::Static(VECTOR_CONE_2DEGREES, PlayerMovementConditions::Ducking()),
			WeaponSpreadRule::Static(VECTOR_CONE_10DEGREES, PlayerMovementConditions::Moving()),
			WeaponSpreadRule::Static(VECTOR_CONE_4DEGREES, PlayerMovementConditions())
		});
	}

	params.fire.cycleTime = 0.067f;
	params.fire.idleDelay = 0.2f;
	params.fire.allowUnderwater = false;
	params.fire.tracerFreq = 0;

	params.fire.autoAimDegree = AUTOAIM_5DEGREES;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;

	params.fire.clientPunchPitch = FloatRange(-2.0f, 2.0f);
	params.fire.clientPunchYaw = FloatRange(-1.0f, 1.0f);
	params.fire.shellOffsetForward = 28;
	params.fire.shellOffsetUp = -24;
	params.fire.shellOffsetSide = 4;
	params.fire.shellModel = "models/saw_shell.mdl";
	params.fire.shellModelAlternating = "models/saw_link.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;
	params.fire.pushbackForce = 35.0f;
	params.fire.pushbackVertical = bIsMultiplayer();
	//

	// Alt fire
	params.secondaryFireType = SecondaryFireType::DISABLED;
	//

	params.reload.animIndex = M249_RELOAD;
	params.reload.duration = 1.33f;
	params.reload.idleDelay = 3.78f;

	params.endReload.animIndex = M249_RELOAD_END;
	params.endReload.attackDelay = 2.4f;
	params.endReload.idleDelay = 2.4f;

	params.holster.animIndex = M249_HOLSTER;
	params.holster.attackDelay = 0.5f;
	params.holster.idleDelay = FloatRange(10.0f, 15.0f);

	params.ammoToBody = {
		{0, 8},
		{1, 8},
		{2, 7},
		{3, 6},
		{4, 5},
		{5, 4},
		{6, 3},
		{7, 2},
		{8, 1},
	};

	params.dropAmmo.classname = "ammo_556";

	return params;
}
