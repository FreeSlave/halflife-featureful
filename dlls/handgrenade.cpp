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
#include "monsters.h"
#include "weapons.h"
#include "player.h"

#define	HANDGRENADE_PRIMARY_VOLUME		450

enum handgrenade_e
{
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
	HANDGRENADE_THROW2,	// medium
	HANDGRENADE_THROW3,	// hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};

class CHandGrenade : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_HANDGRENADE; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

LINK_WEAPON_TO_CLASS( weapon_handgrenade, CHandGrenade )

bool CHandGrenade::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 4;
	p->iPosition = 0;

	return true;
}

WeaponParameters CHandGrenade::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 5;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "Hand Grenade";

	params.worldModel = "models/w_grenade.mdl";
	params.viewModel = "models/v_grenade.mdl";
	params.playerModel = "models/p_grenade.mdl";
	params.playerAnimExt = "crowbar";
	params.priority = 5;

	params.deploy.animIndex = HANDGRENADE_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{HANDGRENADE_IDLE, 0.75f, FloatRange(10.0f, 15.0f)},
		WeaponParameters::IdleAnim{HANDGRENADE_FIDGET, 0.25f, FloatRange(75.0f / 30.0f)},
	};

	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.anims = {HANDGRENADE_THROW1};
	params.fire.cycleTime = 0.5f;
	params.fire.idleDelay = 0.5f;
	params.fire.chargeAnims = {HANDGRENADE_PINPULL};
	params.fire.chargeTime = 0.5f;
	params.fire.allowHolsterDuringCharge = false;
	params.fire.chargeEachFire = true;
	params.fire.cooldownAnims = {HANDGRENADE_DRAW};
	params.fire.cooldownTime = 0.5f;
	params.fire.projectileName = "hand grenade";
	params.fire.projectileOffsetForward = 16.0f;
	params.fire.projectileAddCurrentVelocity = WeaponParameters::Fire::ADD_VELOCITY_ABSOLUTE;
	params.fire.projectileRespectPunchangle = true;
	params.fire.projectileDetonationTime = 3.0f;
	params.fire.projectileDetonationCooked = true;
	params.fire.projectileGrenadePhysics = WeaponParameters::Fire::GRENADEPHYS_AUTO;
	params.fire.projectileFarThrowAnims = {HANDGRENADE_THROW2};
	params.fire.projectileFarthestThrowAnims = {HANDGRENADE_THROW3};

	params.holster.animIndex = HANDGRENADE_HOLSTER;
	params.holster.attackDelay = 0.5f;

	params.dropAmmo.classname = "weapon_handgrenade";
	params.exhausitble = true;

	return std::move(params);
}
