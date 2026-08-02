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
#include "mod_features.h"
#include "soundent_bits.h"

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3
};

class CMP5 : public CConfigurableWeapon
{
public:
	void Spawn() override;
	void PrecacheDefaultModelSounds() override;
	int WeaponId() const override { return WEAPON_MP5; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 )
LINK_WEAPON_TO_CLASS( weapon_9mmAR, CMP5 )

//=========================================================
//=========================================================

void CMP5::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_9mmAR" ); // hack to allow for old names
	CConfigurableWeapon::Spawn();
}

void CMP5::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );
}

bool CMP5::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 2;
	p->iPosition = 0;

	return true;
}

WeaponParameters CMP5::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 25;
	params.maxClip = 50;
	params.ammoName = "9mm";
	params.secondaryAmmoName = "ARgrenades";

	params.worldModel = "models/w_9mmAR.mdl";
	params.viewModel = "models/v_9mmAR.mdl";
	params.playerModel = "models/p_9mmAR.mdl";
	params.playerAnimExt = "mp5";
	params.priority = 15;

	params.deploy.animIndex = MP5_DEPLOY;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{MP5_LONGIDLE, 0.5f, 41.0f / 8.0f},
		WeaponParameters::IdleAnim{MP5_IDLE1, 0.5f, 111.0f / 35.0f},
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damageInfo.main.damage = ::GetSkillValueRange("plr_9mmAR_bullet");
	params.fire.anims.main = {MP5_FIRE1, MP5_FIRE2, MP5_FIRE3};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/hks1.wav", "weapons/hks2.wav"},
		FloatRange(0.92f, 1.0f),
		ATTN_NORM,
		IntRange(94, 109)
	};
	params.fire.spread.SetStaticSpread(false, bIsMultiplayer() ? VECTOR_CONE_6DEGREES : VECTOR_CONE_3DEGREES);
	params.fire.cycleTime = 0.1f;
	params.fire.allowUnderwater = false;
	params.fire.tracerFreq = 2;

	params.fire.autoAimDegree = AUTOAIM_5DEGREES;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;

	params.fire.clientPunchPitch = FloatRange(-2.0f, 2.0f);
	params.fire.shellOffsetForward = 20;
	params.fire.shellOffsetUp = -12;
	params.fire.shellOffsetSide = 4;
	params.fire.shellModel = "models/shell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;

	params.fire.gunSmokeAttachment = 1;
	//

	// Alt fire
	params.fire.fireType.alt = WeaponParameters::Fire::PROJECTILE;
	params.fire.anims.alt = {MP5_LAUNCH};
	params.fire.spread.SetStaticSpread(true, g_vecZero);

	params.fire.projectileName.alt = "grenade";
	params.fire.projectileOffsetForward.alt = 16.0f;
	params.fire.projectileRespectPunchangle.alt = true;
	params.fire.projectileAdjustToCross = false;

	params.fire.sound.alt = {
		CHAN_WEAPON,
		{"weapons/glauncher.wav", "weapons/glauncher2.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(94, 109)
	};

	params.fire.spread.SetStaticSpread(true, g_vecZero);
	params.fire.cycleTime.alt = 1.0f;
	params.fire.idleDelay.alt = 5.0f;
	params.fire.ammoPerFire.alt = 1;
	params.fire.allowUnderwater.alt = false;
	params.fire.useSecondaryAmmo.alt = true;

	params.fire.autoAimDegree.alt = 0.0f;
	params.fire.muzzleFlash.alt = false;
	params.fire.weaponVolume.alt = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash.alt = BRIGHT_GUN_FLASH;
	//params.fire.extraSoundTypes = bits_SOUND_DANGER;
	//params.fire.extraSoundTime = 0.2f;

	params.fire.delayAfterEmpty.alt = 0.0f;

	params.fire.clientPunchPitch.alt = -10;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.reload.animIndex = {WeaponParameters::ReloadAnim(MP5_RELOAD)};
	params.reload.duration = 1.5f;

	params.dropAmmo.classname = "ammo_9mmAR";
	params.dropAmmoSecondary.classname = "ammo_ARgrenades";

	return std::move(params);
}
