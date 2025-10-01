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
#include "hornet.h"

#if !CLIENT_DLL
#include "gamerules.h"
#endif

enum hgun_e
{
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

class CHgun : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_HORNETGUN; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;
};

LINK_WEAPON_TO_CLASS( weapon_hornetgun, CHgun )

bool CHgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = g_AmmoRegistry.GetMaxAmmo(PrimaryAmmoIndex());
		}
#endif
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( WeaponId() );
		MESSAGE_END();
		return true;
	}
	return false;
}

bool CHgun::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 3;
	p->iPosition = 3;

	return true;
}

WeaponParameters CHgun::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 8;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "Hornets";

	params.worldModel = "models/w_hgun.mdl";
	params.viewModel = "models/v_hgun.mdl";
	params.playerModel = "models/p_hgun.mdl";
	params.playerAnimExt = "hive";
	params.priority = 15;

	params.deploy.animIndex = HGUN_UP;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{HGUN_IDLE1, 0.75f, 30.0f / 16.0f * 2.0f},
		WeaponParameters::IdleAnim{HGUN_FIDGETSWAY, 0.125f, 40.0f / 16.0f},
		WeaponParameters::IdleAnim{HGUN_FIDGETSHAKE, 0.125f, 35.0f / 16.0f}
	};

	// Primary fire
	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.projectileName = "hornet";
	params.fire.projectileRespectPunchangle = false;
	params.fire.projectileAdjustToCross = false;
	params.fire.projectileOffsetForward = 16.0f;
	params.fire.projectileOffsetSide = 8.0f;
	params.fire.projectileOffsetUp = -12.0f;

	params.fire.anims = {HGUN_SHOOT};
	params.fire.sound = {
		CHAN_WEAPON,
		{"agrunt/ag_fire1.wav", "agrunt/ag_fire2.wav", "agrunt/ag_fire3.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.useStandardEmptySound = false;

	params.fire.cycleTime = 0.25f;
	params.fire.allowUnderwater = true;

	params.fire.weaponVolume = QUIET_GUN_VOLUME;
	params.fire.weaponFlash = DIM_GUN_FLASH;

	params.fire.clientPunchPitch = FloatRange{0.0f, 2.0f};
	//

	// Alt fire
	params.fire.fireType.alt = WeaponParameters::Fire::PROJECTILE;
	params.fire.projectileName.alt = "hornet dart";
	params.fire.projectileAdjustToCross.alt = true;
	params.fire.projectileFirePhases.alt = WeaponParameters::FirePhaseArray{
		WeaponParameters::FirePhase{0.0f, 8.0f},
		WeaponParameters::FirePhase{8.0f, 8.0f},
		WeaponParameters::FirePhase{8.0f, 0.0f},
		WeaponParameters::FirePhase{8.0f, -8.0f},
		WeaponParameters::FirePhase{0.0f, -8.0f},
		WeaponParameters::FirePhase{-8.0f, -8.0f},
		WeaponParameters::FirePhase{-8.0f, 0.0f},
		WeaponParameters::FirePhase{-8.0f, 8.0f},
	};

	params.fire.cycleTime.alt = 0.1f;
	params.fire.weaponVolume.alt = NORMAL_GUN_VOLUME;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.recharge.interval = bIsMultiplayer() ? 0.3f : 0.5f;

	params.holster.animIndex = HGUN_DOWN;
	params.holster.attackDelay = 0.5f;

	return params;
}
