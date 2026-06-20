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

enum pwrench_e
{
	PIPEWRENCH_IDLE1 = 0,
	PIPEWRENCH_IDLE2,
	PIPEWRENCH_IDLE3,
	PIPEWRENCH_DRAW,
	PIPEWRENCH_HOLSTER,
	PIPEWRENCH_ATTACK1HIT,
	PIPEWRENCH_ATTACK1MISS,
	PIPEWRENCH_ATTACK2HIT,
	PIPEWRENCH_ATTACK2MISS,
	PIPEWRENCH_ATTACK3HIT,
	PIPEWRENCH_ATTACK3MISS,
	PIPEWRENCH_ATTACKBIGWIND,
	PIPEWRENCH_ATTACKBIGHIT,
	PIPEWRENCH_ATTACKBIGMISS,
	PIPEWRENCH_ATTACKBIGLOOP
};

class CPipeWrench : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_PIPEWRENCH; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS(weapon_pipewrench, CPipeWrench)

bool CPipeWrench::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 0;
	p->iPosition = 1;
	return true;
}

WeaponParameters CPipeWrench::GetDefaultParameters() const
{
	WeaponParameters params;

	params.maxClip = WEAPON_NOCLIP;

	params.worldModel = "models/w_pipe_wrench.mdl";
	params.viewModel = "models/v_pipe_wrench.mdl";
	params.playerModel = "models/p_pipe_wrench.mdl";
	params.playerAnimExt = "crowbar";
	params.priority = 0;

	params.deploy.animIndex = PIPEWRENCH_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{PIPEWRENCH_IDLE1, 0.3f, 2.0f},
		WeaponParameters::IdleAnim{PIPEWRENCH_IDLE2, 0.6f, 3.0f},
		WeaponParameters::IdleAnim{PIPEWRENCH_IDLE3, 0.1f, 3.0f}
	};

	// Primary attack
	params.fire.fireType = WeaponParameters::Fire::MELEE;
	params.fire.damageInfo.main.damage = ::GetSkillValueRange("plr_pipewrench");
	params.fire.subsequentSwingFactor = 0.5f;
	params.fire.anims = {PIPEWRENCH_ATTACK1MISS, PIPEWRENCH_ATTACK2MISS, PIPEWRENCH_ATTACK3MISS};
	params.fire.hitAnims = {PIPEWRENCH_ATTACK1HIT, PIPEWRENCH_ATTACK2HIT, PIPEWRENCH_ATTACK3HIT};
	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/pwrench_miss1.wav", "weapons/pwrench_miss2.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.cycleTime = 0.75f;
	params.fire.hitCycleTime = 0.5f;
	params.fire.idleDelay = 5.0f;
	params.fire.hitBodySound = {
		CHAN_ITEM,
		{"weapons/pwrench_hitbod1.wav", "weapons/pwrench_hitbod2.wav", "weapons/pwrench_hitbod3.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.hitWallSound = {
		CHAN_ITEM,
		{"weapons/pwrench_hit1.wav", "weapons/pwrench_hit2.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(98, 101)
	};
	//

	// Alt attack
	params.fire.fireType.alt = WeaponParameters::Fire::MELEE;
	params.fire.damageInfo.Materialize(true).damage = ::GetSkillValueRange("plr_pipewrench_wind_base");
	params.fire.damageChargedFactor.alt = ::GetSkillValueRange("plr_pipewrench_wind_factor");
	params.fire.damageChargedMax.alt = ::GetSkillValueRange("plr_pipewrench_wind_max");
	params.fire.chargedAttack.alt = true;
	params.fire.anims.alt = {PIPEWRENCH_ATTACKBIGMISS};
	params.fire.chargeAnims.alt = {PIPEWRENCH_ATTACKBIGWIND};
	params.fire.chargeTime.alt = 1.0f;
	params.fire.cycleTime.alt = 1.1f;
	params.fire.hitAnims.alt = {PIPEWRENCH_ATTACKBIGHIT};
	params.fire.sound.alt = {
		CHAN_WEAPON,
		{"weapons/pwrench_big_miss.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.hitBodySound.alt = {
		CHAN_ITEM,
		{"weapons/pwrench_big_hitbod1.wav", "weapons/pwrench_big_hitbod2.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	WeaponKickBack kickBack;
	kickBack.verticalBase = 2.0f;
	kickBack.verticalMax = 4.0f;
	params.fire.kickBack.SetKickBack(true, kickBack);
	params.fire.kickBackOnHitOnly.alt = true;

	params.fire.smackDelay.alt = 0.13f;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.holster.animIndex = PIPEWRENCH_HOLSTER;
	params.holster.attackDelay = 0.5f;

	return std::move(params);
}
