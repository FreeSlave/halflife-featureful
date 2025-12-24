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

enum uzi_e {
	UZI_IDLE1 = 0,
	UZI_IDLE2,
	UZI_IDLE3,
	UZI_RELOAD,
	UZI_DEPLOY,
	UZI_SHOOT,
};

class CUzi : public CConfigurableWeapon
{
public:
	void PrecacheDefaultModelSounds() override;
	int WeaponId() const override { return WEAPON_UZI; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_uzi, CUzi )
LINK_ENTITY_TO_CLASS( weapon_uziakimbo, CUzi ) // Link to single uzi until akimbo is implemented

//=========================================================
//=========================================================

void CUzi::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND( "weapons/uzi/reload1.wav" );
	PRECACHE_SOUND( "weapons/uzi/reload2.wav" );
	PRECACHE_SOUND( "weapons/uzi/reload3.wav" );

	PRECACHE_SOUND( "weapons/uzi/deploy.wav" );
	PRECACHE_SOUND( "weapons/uzi/deploy1.wav" );
	PRECACHE_SOUND( "weapons/uzi/akimbo_pull2.wav" );
}

bool CUzi::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 1;
	p->iPosition = 3;
	p->pszAmmoEntity = "ammo_9mmclip";
	p->iDropAmmo = AMMO_GLOCKCLIP_GIVE;

	return true;
}

WeaponParameters CUzi::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 32;
	params.maxClip = 32;
	params.ammoName = "9mm";

	params.worldModel = "models/w_uzi.mdl";
	params.viewModel = "models/v_uzi.mdl";
	params.playerModel = "models/p_uzi.mdl";
	params.playerAnimExt = "mp5";
	params.priority = 15;

	params.deploy.animIndex = UZI_DEPLOY;
	params.deploy.duration = 1.28f;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{UZI_IDLE1, 0.6f, 2.8f},
		WeaponParameters::IdleAnim{UZI_IDLE2, 0.1f, 6.73f},
		WeaponParameters::IdleAnim{UZI_IDLE3, 0.3f, 5.0f},
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damage = ::GetSkillValue("plr_uzi");
	params.fire.anims.main = {UZI_SHOOT};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/uzi/shoot1.wav", "weapons/uzi/shoot2.wav", "weapons/uzi/shoot3.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(94, 109)
	};

	params.fire.spread.SetStaticSpread(false, VECTOR_CONE_3DEGREES);
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
	//

	// Alt fire
	params.secondaryFireType = SecondaryFireType::DISABLED;
	//

	params.reload.animIndex = UZI_RELOAD;
	params.reload.duration = 2.5f;

	return params;
}
