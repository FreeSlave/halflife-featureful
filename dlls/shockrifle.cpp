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
#if !CLIENT_DLL
#include "shockbeam.h"
#include "gamerules.h"
#endif

enum shockrifle_e
{
	SHOCK_IDLE1 = 0,
	SHOCK_FIRE,
	SHOCK_DRAW,
	SHOCK_HOLSTER,
	SHOCK_IDLE3
};

class CShockrifle : public CConfigurableWeapon
{
public:
	void Precache() override;
	void PrecacheDefaultModelSounds() override;
	int WeaponId() const override { return WEAPON_SHOCKRIFLE; }

	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;

	bool HandleAttackSubstitution(bool altMode) override;
};

LINK_WEAPON_TO_CLASS(weapon_shockrifle, CShockrifle)

void CShockrifle::Precache()
{
	CConfigurableWeapon::Precache();

	PRECACHE_SOUND("weapons/shock_discharge.wav");
	PRECACHE_SOUND("weapons/shock_impact.wav");
}

void CShockrifle::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND("weapons/shock_draw.wav");
}

bool CShockrifle::GetItemInfo(ItemInfo *p)
{
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 6;
	p->iPosition = 1;
#else
	p->iSlot = 3;
	p->iPosition = 4;
#endif

	return true;
}

WeaponParameters CShockrifle::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 10;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "Shocks";

	params.worldModel = "models/w_shock_rifle.mdl";
	params.viewModel = "models/v_shock.mdl";
	params.playerModel = "models/p_shock.mdl";
	params.playerAnimExt = "bow";
	params.priority = 15;
	params.worldModelAnimated = true;

	params.deploy.animIndex = SHOCK_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{SHOCK_IDLE3, 0.8f, 3.3f},
		WeaponParameters::IdleAnim{SHOCK_IDLE1, 0.2f, 3.3f}
	};

	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.anims = {SHOCK_FIRE};
	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/shock_fire.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.useStandardEmptySound = false;

	params.fire.projectileName = "shock_beam";
	params.fire.projectileOffsetForward = 8.0f;
	params.fire.projectileOffsetSide = 9.0f;
	params.fire.projectileOffsetUp = -7.0f;
	params.fire.projectileRespectPunchangle = true;
	params.fire.projectileAdjustToCross = true;

	params.fire.cycleTime = bIsMultiplayer() ? 0.1f : 0.2f;
	params.fire.idleDelay = 0.33f;
	params.fire.allowUnderwater = true;
	params.fire.autoAimDegree = AUTOAIM_10DEGREES;

	params.fire.weaponVolume = QUIET_GUN_VOLUME;
	params.fire.weaponFlash = DIM_GUN_FLASH;

	WeaponParameters::ViewmodelBeam viewmodelBeam1, viewmodelBeam2, viewmodelBeam3;

	Visual beamVisual;
	beamVisual.SetModel("sprites/lgtning.spr");
	beamVisual.SetLife(0.08f);
	beamVisual.SetBeamWidth(1);
	beamVisual.SetBeamNoise(75);
	beamVisual.SetBeamScrollRate(30);
	beamVisual.SetFramerate(10);
	beamVisual.SetAlpha(190);
	beamVisual.SetColor(Color3(0, 253, 253));

	viewmodelBeam1.visual = beamVisual;
	viewmodelBeam1.startAttachment = 1;
	viewmodelBeam1.endAttachment = 2;

	viewmodelBeam2.startAttachment = 1;
	viewmodelBeam2.endAttachment = 3;
	viewmodelBeam3.startAttachment = 1;
	viewmodelBeam3.endAttachment = 4;

	params.fire.viewmodelBeams = {viewmodelBeam1, viewmodelBeam2, viewmodelBeam3};

	params.secondaryFireType = SecondaryFireType::DISABLED;

	params.recharge.interval = bIsMultiplayer() ? 0.25f : 0.5f;
	params.recharge.delayAfterFire = 1.0f;
	params.recharge.onlyWhenDeployed = true;
	params.recharge.sound = {
		CHAN_WEAPON,
		{"weapons/shock_recharge.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};

	params.holster.animIndex = SHOCK_HOLSTER;
	params.holster.attackDelay = 0.5f;

	return std::move(params);
}

bool CShockrifle::HandleAttackSubstitution(bool altMode)
{
	if (m_pPlayer->pev->waterlevel == WL_Eyes)
	{
#if !CLIENT_DLL
		const float dmg = GetSkillValue("plr_shockroach_discharge_factor") * m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()];
		const float radius = 1.5f * dmg;
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_discharge.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 0;
		RadiusDamage(m_pPlayer->pev->origin, m_pPlayer->pev, m_pPlayer->pev, DamageInfo(dmg, DMG_SHOCK).SetGibPolicy(GIB_ALWAYS), radius, CLASS_NONE );
#endif
		return true;
	}
	return false;
}
