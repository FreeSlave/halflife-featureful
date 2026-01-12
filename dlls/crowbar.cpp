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

enum crowbar_e
{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT,
	CROWBAR_IDLE2,
	CROWBAR_IDLE3,
};

class CCrowbar : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_CROWBAR; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_crowbar, CCrowbar )

bool CCrowbar::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 0;
	p->iPosition = 0;
	return true;
}

WeaponParameters CCrowbar::GetDefaultParameters() const
{
	WeaponParameters params;

	params.maxClip = WEAPON_NOCLIP;

	params.worldModel = "models/w_crowbar.mdl";
	params.viewModel = "models/v_crowbar.mdl";
	params.playerModel = "models/p_crowbar.mdl";
	params.playerAnimExt = "crowbar";
	params.priority = 0;

	params.deploy.animIndex = CROWBAR_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{CROWBAR_IDLE, 0.4f, 70.0f / 25.0f},
		WeaponParameters::IdleAnim{CROWBAR_IDLE2, 0.1f, 160.0f / 30.0f},
		WeaponParameters::IdleAnim{CROWBAR_IDLE3, 0.5f, 160.0f / 30.0f}
	};

	params.fire.fireType = WeaponParameters::Fire::MELEE;
	params.fire.damage = ::GetSkillValueRange("plr_crowbar");
	params.fire.subsequentSwingFactor = 0.5f;
	params.fire.anims = {CROWBAR_ATTACK1MISS, CROWBAR_ATTACK2MISS, CROWBAR_ATTACK3MISS};
	params.fire.hitAnims = {CROWBAR_ATTACK2HIT, CROWBAR_ATTACK3HIT};
	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/cbar_miss1.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.cycleTime = 0.5f;
	params.fire.hitCycleTime = 0.25f;
	params.fire.idleDelay = FloatRange(6.0f, 10.0f);
	params.fire.hitBodySound = {
		CHAN_ITEM,
		{"weapons/cbar_hitbod1.wav", "weapons/cbar_hitbod2.wav", "weapons/cbar_hitbod3.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.hitWallSound = {
		CHAN_ITEM,
		{"weapons/cbar_hit1.wav", "weapons/cbar_hit2.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(98, 101)
	};

	params.secondaryFireType = SecondaryFireType::DISABLED;

	params.holster.animIndex = CROWBAR_HOLSTER;
	params.holster.attackDelay = 0.5f;

	return params;
}
