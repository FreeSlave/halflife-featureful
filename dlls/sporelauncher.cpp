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
#include "fx_flags.h"
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
	int WeaponId() const override { return WEAPON_SPORELAUNCHER; }

	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS(weapon_sporelauncher, CSporelauncher)

void CSporelauncher::Spawn()
{
	CConfigurableWeapon::Spawn();
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;
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

	WeaponSoundScript fidgetSoundScript(CHAN_ITEM, {"weapons/splauncher_pet.wav"}, 0.7f, ATTN_NORM, 100);

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{SPLAUNCHER_IDLE, 0.75f, 2.0f},
		WeaponParameters::IdleAnim{SPLAUNCHER_IDLE2, 0.20f, 4.0f},
		WeaponParameters::IdleAnim{SPLAUNCHER_FIDGET, 0.05f, 4.0f, fidgetSoundScript}
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.anims = {SPLAUNCHER_FIRE};

	params.fire.projectileName = "spore rocket";
	params.fire.projectileOffsetForward = 16.0f;
	params.fire.projectileOffsetSide = 8.0f;
	params.fire.projectileOffsetUp = -8.0f;
	params.fire.projectileRespectPunchangle = false;
	params.fire.projectileAdjustToCross = true;

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

	params.fire.sprayOffsetUp = -20.0f;
	params.fire.sprayOffsetSide = 8.0f;
	params.fire.sprayOffsetForward = 16.0f;

	Visual sprayVisual;
	sprayVisual.SetModel("sprites/tinyspit.spr");
	sprayVisual.SetAlpha(255);
	sprayVisual.SetRenderMode(kRenderTransAlpha);
	sprayVisual.SetFramerate(0.5f);
	sprayVisual.SetRenderFx(kRenderFxNoDissipation);

	params.fire.sprayVisual = sprayVisual;
	params.fire.sprayCount = 8;
	params.fire.spraySpeed = 210;
	params.fire.spraySpread = 0.25f;
	params.fire.sprayFlags = SPRAY_FLAG_FADEOUT;
	//

	// Alt fire
	params.fire.projectileName.alt = "spore bouncy";
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
