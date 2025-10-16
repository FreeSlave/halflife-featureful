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
#ifndef CLIENT_DLL
#include "spore.h"
#endif

enum sporelauncher_e
{
	SPLAUNCHER_IDLE = 0,
	SPLAUNCHER_FIDGET,
	SPLAUNCHER_RELOAD_REACH,
	SPLAUNCHER_RELOAD_LOAD,
	SPLAUNCHER_RELOAD_AIM,
	SPLAUNCHER_FIRE,
	SPLAUNCHER_HOLSTER1,
	SPLAUNCHER_DRAW1,
	SPLAUNCHER_IDLE2
};

class CSporelauncher : public CConfigurableWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int WeaponId() const override { return WEAPON_SPORELAUNCHER; }

	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;

	void NativeAttack(bool altMode) override;
	void OnIdleAnimation(int anim) override;

	int m_iSquidSpitSprite;
};

LINK_WEAPON_TO_CLASS(weapon_sporelauncher, CSporelauncher)

void CSporelauncher::Spawn()
{
	CConfigurableWeapon::Spawn();
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;
}

void CSporelauncher::Precache()
{
	CConfigurableWeapon::Precache();

	PRECACHE_SOUND("weapons/splauncher_pet.wav");

	PRECACHE_MODEL("sprites/bigspit.spr");
	m_iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");
	UTIL_PrecacheOther("spore");
}

bool CSporelauncher::GetItemInfo(ItemInfo *p)
{
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 6;
	p->iPosition = 0;
#else
	p->iSlot = 3;
	p->iPosition = 5;
#endif

	return true;
}

WeaponParameters CSporelauncher::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 5;
	params.maxClip = 5;
	params.ammoName = "spores";

	params.worldModel = "models/w_spore_launcher.mdl";
	params.viewModel = "models/v_spore_launcher.mdl";
	params.playerModel = "models/p_spore_launcher.mdl";
	params.playerAnimExt = "rpg";
	params.priority = 20;

	params.deploy.animIndex = SPLAUNCHER_DRAW1;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{SPLAUNCHER_IDLE, 0.75f, 2.0f},
		WeaponParameters::IdleAnim{SPLAUNCHER_IDLE2, 0.20f, 4.0f},
		WeaponParameters::IdleAnim{SPLAUNCHER_FIDGET, 0.05f, 4.0f}
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::NATIVE;
	params.fire.anims = {SPLAUNCHER_FIRE};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/splauncher_fire.wav"},
		0.9f,
		ATTN_NORM,
		100
	};
	params.fire.useStandardEmptySound = false;

	params.fire.cycleTime = 0.5f;
	params.fire.idleDelay = 0.5f;
	params.fire.allowUnderwater = true;

	params.fire.autoAimDegree = AUTOAIM_10DEGREES;
	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = BRIGHT_GUN_FLASH;

	params.fire.clientPunchPitch = -3.0f;

	params.fire.spitSpray = true;
	//

	// Alt fire
	params.fire.sound.alt = {
		CHAN_WEAPON,
		{"weapons/splauncher_altfire.wav"},
		0.9f,
		ATTN_NORM,
		100
	};
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.startReload.animIndex = SPLAUNCHER_RELOAD_REACH;
	params.startReload.duration = 0.7f;

	params.reloadAutostart = true;
	params.manualReload = true;

	params.reload.animIndex = SPLAUNCHER_RELOAD_LOAD;
	params.reload.idleDelay = 1.0f;
	params.reload.duration = 0.0f;
	params.reload.sound = {
		CHAN_ITEM,
		{"weapons/splauncher_reload.wav"},
		0.7f,
		ATTN_NORM,
		100
	};
	params.reload.waitForRecoil = true;

	params.endReload.animIndex = SPLAUNCHER_RELOAD_AIM;
	params.endReload.idleDelay = 0.8f;
	params.endReload.attackDelay = 0.0f;

	return params;
}

void CSporelauncher::NativeAttack(bool altMode)
{
#if !CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

	if (altMode)
	{
		Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		CSpore::ShootTimed(m_pPlayer, vecSrc, vecAngles, m_pPlayer->pev->velocity + gpGlobals->v_forward * CSpore::SporeGrenadeSpeed());
	}
	else
	{
		Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		CSpore::ShootContact( m_pPlayer, vecSrc, vecAngles, gpGlobals->v_forward * CSpore::SporeRocketSpeed() );
	}
#endif
}

void CSporelauncher::OnIdleAnimation(int anim)
{
	if (anim == SPLAUNCHER_FIDGET)
	{
		EMIT_SOUND(m_pPlayer->edict(), CHAN_ITEM, "weapons/splauncher_pet.wav", 0.7f, ATTN_NORM);
	}
}
