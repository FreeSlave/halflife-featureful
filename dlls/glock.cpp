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

enum glock_e
{
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

class CGlock : public CConfigurableWeapon
{
public:
	void Spawn() override;
	void PrecacheDefaultModelSounds() override;
	int WeaponId() const override { return WEAPON_GLOCK; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock )
LINK_WEAPON_TO_CLASS( weapon_9mmhandgun, CGlock )

void CGlock::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_9mmhandgun" ); // hack to allow for old names
	CConfigurableWeapon::Spawn();
}

void CGlock::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );
}

bool CGlock::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 1;
	p->iPosition = 0;

	return true;
}

WeaponParameters CGlock::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 17;
	params.maxClip = 17;
	params.ammoName = "9mm";

	params.worldModel = "models/w_9mmhandgun.mdl";
	params.viewModel = "models/v_9mmhandgun.mdl";
	params.playerModel = "models/p_9mmhandgun.mdl";
	params.playerAnimExt = "onehanded";
	params.priority = 10;

	params.deploy.animIndex = GLOCK_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{GLOCK_IDLE3, 0.3f, 49.0f / 16.0f},
		WeaponParameters::IdleAnim{GLOCK_IDLE1, 0.3f, 60.0f / 16.0f},
		WeaponParameters::IdleAnim{GLOCK_IDLE2, 0.4f, 40.0f / 16.0f}
	};
	params.idleAnims.mainEmptied = WeaponParameters::IdleAnimArray{};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damageInfo.main.damage = ::GetSkillValueRange("plr_9mm_bullet");
	params.fire.anims.main = {GLOCK_SHOOT};
	params.fire.anims.mainEmptied = {GLOCK_SHOOT_EMPTY};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/pl_gun3.wav"},
		FloatRange(0.92f, 1.0f),
		ATTN_NORM,
		IntRange(98, 101)
	};

	params.fire.spread.SetStaticSpread(false, 0.01f);
	params.fire.cycleTime = 0.3f;
	params.fire.allowUnderwater = true;

	params.fire.autoAimDegree = AUTOAIM_10DEGREES;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;

	params.fire.delayAfterEmpty = 0.2f;

	params.fire.clientPunchPitch = -2.0f;
	params.fire.shellOffsetForward = 20;
	params.fire.shellOffsetUp = -12;
	params.fire.shellOffsetSide = 4;
	params.fire.shellModel = "models/shell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;

	params.fire.gunSmokeAttachment = 1;
	//

	// Alt fire
	params.fire.spread.SetStaticSpread(true, 0.1f);
	params.fire.cycleTime.alt = 0.2f;
	params.fire.autoAimDegree.alt = 0.0f;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.reload.animIndex = GLOCK_RELOAD_NOT_EMPTY;
	params.reload.duration = 1.5f;
	params.reload.idleDelay = FloatRange(10.0f, 15.0f);
	params.reload.animIndex.mainEmptied = GLOCK_RELOAD;

	params.dropAmmo.classname = "ammo_9mmclip";

	return std::move(params);
}
