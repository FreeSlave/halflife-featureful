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

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};

class CHgun : public CConfigurableWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Precache() override;
	int WeaponId() const override { return WEAPON_HORNETGUN; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;

	void NativeAttack(bool altMode) override;

	int m_iFirePhase;
};

LINK_WEAPON_TO_CLASS( weapon_hornetgun, CHgun )

#if !CLIENT_DLL
TYPEDESCRIPTION CHgun::m_SaveData[] =
{
	DEFINE_FIELD( CHgun, m_iFirePhase, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CHgun, CConfigurableWeapon )
#endif

void CHgun::Precache()
{
	CConfigurableWeapon::Precache();
	UTIL_PrecacheOther( "hornet" );
}

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
	params.fire.cycleTime.alt = 0.1f;
	params.fire.weaponVolume.alt = NORMAL_GUN_VOLUME;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.recharge.interval = bIsMultiplayer() ? 0.3f : 0.5f;

	params.holster.animIndex = HGUN_DOWN;
	params.holster.attackDelay = 0.5f;

	return params;
}

void CHgun::NativeAttack(bool altMode)
{
	if (altMode)
	{
		//Wouldn't be a bad idea to completely predict these, since they fly so fast...
#if !CLIENT_DLL
		CBaseEntity *pHornet;
		Vector vecSrc;

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -12.0f;

		m_iFirePhase++;
		switch( m_iFirePhase )
		{
		case 1:
			vecSrc += gpGlobals->v_up * 8.0f;
			break;
		case 2:
			vecSrc += gpGlobals->v_up * 8.0f;
			vecSrc += gpGlobals->v_right * 8.0f;
			break;
		case 3:
			vecSrc += gpGlobals->v_right * 8.0f;
			break;
		case 4:
			vecSrc += gpGlobals->v_up * -8.0f;
			vecSrc += gpGlobals->v_right * 8.0f;
			break;
		case 5:
			vecSrc += gpGlobals->v_up * -8.0f;
			break;
		case 6:
			vecSrc += gpGlobals->v_up * -8.0f;
			vecSrc += gpGlobals->v_right * -8.0f;
			break;
		case 7:
			vecSrc += gpGlobals->v_right * -8.0f;
			break;
		case 8:
			vecSrc += gpGlobals->v_up * 8.0f;
			vecSrc += gpGlobals->v_right * -8.0f;
			m_iFirePhase = 0;
			break;
		}

		pHornet = CBaseEntity::Create( "hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
		pHornet->pev->velocity = gpGlobals->v_forward * 1200.0f;
		pHornet->pev->angles = UTIL_VecToAngles( pHornet->pev->velocity );

		pHornet->SetThink( &CHornet::StartDart );
#endif
	}
	else
	{
#if !CLIENT_DLL
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		CBaseEntity *pHornet = CBaseEntity::Create( "hornet", m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -12.0f, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
		pHornet->pev->velocity = gpGlobals->v_forward * 300.0f;
#endif
	}
}
