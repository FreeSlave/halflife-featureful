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

enum knife_e
{
	KNIFE_IDLE1 = 0,
	KNIFE_DRAW,
	KNIFE_HOLSTER,
	KNIFE_ATTACK1,
	KNIFE_ATTACK1MISS,
	KNIFE_ATTACK2,
	KNIFE_ATTACK2HIT,
	KNIFE_ATTACK3,
	KNIFE_ATTACK3HIT,
	KNIFE_IDLE2,
	KNIFE_IDLE3,
	KNIFE_CHARGE,
	KNIFE_STAB
};

class CKnife : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_KNIFE; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS(weapon_knife, CKnife)

bool CKnife::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 0;
	p->iPosition = 2;
	return true;
}

WeaponParameters CKnife::GetDefaultParameters() const
{
	WeaponParameters params;

	params.maxClip = WEAPON_NOCLIP;

	params.worldModel = "models/w_knife.mdl";
	params.viewModel = "models/v_knife.mdl";
	params.playerModel = "models/p_knife.mdl";
	params.playerAnimExt = "crowbar";
	params.priority = 0;

	params.deploy.animIndex = KNIFE_DRAW;

	params.idleAnims = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{KNIFE_IDLE1, 0.1f, 70.0f / 25.0f},
		WeaponParameters::IdleAnim{KNIFE_IDLE2, 0.4f, 160.0f / 30.0f},
		WeaponParameters::IdleAnim{KNIFE_IDLE3, 0.5f, 160.0f / 30.0f}
	};

	// Primary attack
	params.fire.fireType = WeaponParameters::Fire::MELEE;
	params.fire.damageInfo.main.damage = ::GetSkillValueRange("plr_knife");
	params.fire.subsequentSwingFactor = 1.0f;
	params.fire.anims = {KNIFE_ATTACK1MISS, KNIFE_ATTACK2, KNIFE_ATTACK3};
	params.fire.hitAnims = {KNIFE_ATTACK2HIT, KNIFE_ATTACK3HIT};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/knife1.wav", "weapons/knife2.wav", "weapons/knife3.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.cycleTime = 0.5f;
	params.fire.hitCycleTime = 0.25f;
	params.fire.idleDelay = 5.0f;
	params.fire.hitBodySound = {
		CHAN_ITEM,
		{"weapons/knife_hit_flesh1.wav", "weapons/knife_hit_flesh2.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.hitWallSound = {
		CHAN_ITEM,
		{"weapons/knife_hit_wall1.wav", "weapons/knife_hit_wall2.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(98, 101)
	};
	//

	// Alt attack
	params.fire.fireType.alt = WeaponParameters::Fire::MELEE;
	params.fire.damageInfo.Materialize(true).damage = ::GetSkillValueRange("plr_knife_stab_base");
	params.fire.damageChargedFactor.alt = ::GetSkillValueRange("plr_knife_stab_factor");
	params.fire.damageChargedMax.alt = ::GetSkillValueRange("plr_knife_stab_max");
	params.fire.chargedAttack.alt = true;
	params.fire.anims.alt = {KNIFE_STAB};
	params.fire.hitAnims.alt = WeaponParameters::FireAnimArray{};
	params.fire.chargeAnims.alt = {KNIFE_CHARGE};
	params.fire.chargeTime.alt = 0.8f;
	params.fire.cycleTime.alt = 0.9f;
	params.fire.smackDelay.alt = 0.13f;
	params.fire.hitDecal.alt = false;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.holster.animIndex = KNIFE_HOLSTER;
	params.holster.attackDelay = 0.5f;

	return params;
}
