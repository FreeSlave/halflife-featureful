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

enum python_e
{
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

class CPython : public CConfigurableWeapon
{
public:
	void Spawn() override;
	void PrecacheDefaultModelSounds() override;
	int WeaponId() const override { return WEAPON_PYTHON; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
	void Reload() override;
	void WeaponIdle() override;
	float m_flSoundDelay;
};

LINK_ENTITY_TO_CLASS( weapon_python, CPython )
LINK_WEAPON_TO_CLASS( weapon_357, CPython )

bool CPython::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 1;
	p->iPosition = 1;

	return true;
}

WeaponParameters CPython::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 6;
	params.maxClip = 6;
	params.ammoName = "357";

	params.worldModel = "models/w_357.mdl";
	params.viewModel = "models/v_357.mdl";
	params.playerModel = "models/p_357.mdl";
	params.playerAnimExt = "python";
	params.priority = 15;

	params.deploy.animIndex = PYTHON_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{PYTHON_IDLE1, 0.5f, 70.0f / 30.0f},
		WeaponParameters::IdleAnim{PYTHON_IDLE2, 0.2f, 60.0f / 30.0f},
		WeaponParameters::IdleAnim{PYTHON_IDLE3, 0.2f, 88.0f / 30.0f},
		WeaponParameters::IdleAnim{PYTHON_FIDGET, 0.1f, 170.0f / 30.0f}
	};

	params.viewModelBody = bIsMultiplayer() ? 1 : 0;

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damageInfo.main.damage = ::GetSkillValueRange("plr_357_bullet");
	params.fire.anims.main = {PYTHON_FIRE1};

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/357_shot1.wav", "weapons/357_shot2.wav"},
		FloatRange(0.8f, 0.9f),
		ATTN_NORM,
		PITCH_NORM
	};

	params.fire.spread.SetStaticSpread(false, VECTOR_CONE_1DEGREES);
	params.fire.cycleTime = 0.75f;
	params.fire.allowUnderwater = false;

	params.fire.autoAimDegree = AUTOAIM_10DEGREES;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = BRIGHT_GUN_FLASH;

	params.fire.clientPunchPitch = -10.0f;

	params.fire.gunSmokeAttachment = 1;
	//

	// Alt fire
	params.secondaryFireType = bIsMultiplayer() ? SecondaryFireType::SWITCH_MODE : SecondaryFireType::DISABLED;
	params.altMode.zoomFOV = 40;
	params.altMode.attackDelay = 0.5f;
	//

	params.reload.animIndex = PYTHON_RELOAD;
	params.reload.duration = 2.0f;

	params.holster.animIndex = PYTHON_HOLSTER;
	params.holster.attackDelay = 1.0f;
	params.holster.idleDelay = FloatRange(10, 15);

	params.dropAmmo.classname = "ammo_357";

	return params;
}

void CPython::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_357" ); // hack to allow for old names
	CConfigurableWeapon::Spawn();
}

void CPython::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/357_reload1.wav" );
}

void CPython::Reload()
{
	bool result = PerformReload();
	if (result)
	{
		m_flSoundDelay = 1.5f;
	}
}

void CPython::WeaponIdle()
{
	if( m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT( 0.8f, 0.9f ), ATTN_NORM );
		m_flSoundDelay = 0.0f;
	}
	CConfigurableWeapon::WeaponIdle();
}
