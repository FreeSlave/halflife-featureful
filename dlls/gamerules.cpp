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
//=========================================================
// GameRules.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"ammunition.h"
#include	"gamerules.h"
#include	"teamplay_gamerules.h"
#include	"skill.h"
#include	"game.h"
#include	"mod_features.h"

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

DLL_GLOBAL CGameRules *g_pGameRules = NULL;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgMOTD;

int g_teamplay = 0;

//=========================================================
//=========================================================
bool CGameRules::CanHaveAmmo(CBasePlayer *pPlayer, const char *pszAmmoName )
{
	const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(pszAmmoName);
	if( ammoType )
	{
		if (pPlayer->AmmoInventory(ammoType->id) < pPlayer->GetMaxAmmo(ammoType->id))
		{
			// player has room for more of this type of ammo
			return true;
		}
	}

	return false;
}

//=========================================================
//=========================================================
edict_t *CGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );

	pPlayer->pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
	pPlayer->pev->v_angle  = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS( pentSpawnSpot )->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = 1;

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
bool CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
	// only living players can have items
	if( pPlayer->pev->deadflag != DEAD_NO )
		return false;

	if( pWeapon->pszAmmo1() )
	{
		if( !CanHaveAmmo( pPlayer, pWeapon->pszAmmo1() ) )
		{
			// we can't carry anymore ammo for this gun. We can only 
			// have the gun if we aren't already carrying one of this type
			if( pPlayer->HasPlayerItem( pWeapon ) )
			{
				return false;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if( pPlayer->HasPlayerItem( pWeapon ) )
		{
			return false;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return true;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData ()
{
	const int iSkill = clamp((int)CVAR_GET_FLOAT("skill"), 1, 3);

	g_iSkillLevel = iSkill;

	ALERT(at_console, "\nGAME SKILL LEVEL:%d\n", iSkill);
}

void CGameRules::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	pPlayer->SetPrefsFromUserinfo( infobuffer );
}

CBasePlayer *CGameRules::EffectivePlayer(CBaseEntity *pActivator)
{
	if (pActivator && pActivator->IsPlayer()) {
		return (CBasePlayer*)pActivator;
	}
	return NULL;
}

CBasePlayer *CGameRules::EffectiveAlivePlayer(CBaseEntity *pActivator)
{
	CBasePlayer* pPlayer = EffectivePlayer(pActivator);
	if (pPlayer && pPlayer->IsAlive())
		return pPlayer;
	return NULL;
}

bool CGameRules::EquipPlayerFromMapConfig(CBasePlayer *pPlayer, const MapConfig &mapConfig)
{
	extern bool gEvilImpulse101;

	if (mapConfig.valid)
	{
		gEvilImpulse101 = true;

		bool giveSuit = !mapConfig.nosuit;
		if (giveSuit)
		{
			int suitSpawnFlags = 0;
			switch (mapConfig.suitLogon) {
			case SuitNoLogon:
				suitSpawnFlags |= SF_SUIT_NOLOGON;
				break;
			case SuitShortLogon:
				suitSpawnFlags |= SF_SUIT_SHORTLOGON;
				break;
			case SuitLongLogon:
				break;
			}
			pPlayer->GiveNamedItem("item_suit", suitSpawnFlags);
		}

		if (mapConfig.suit_light == MapConfig::SUIT_LIGHT_NOTHING)
		{
			pPlayer->RemoveSuitLight();
		}
		else if (mapConfig.suit_light == MapConfig::SUIT_LIGHT_FLASHLIGHT)
		{
			pPlayer->SetFlashlightOnly();
		}
		else if (mapConfig.suit_light == MapConfig::SUIT_LIGHT_NVG)
		{
			pPlayer->SetNVGOnly();
		}

		for (const auto& pickupEnt : mapConfig.pickupEnts)
		{
			for (int j=0; j<pickupEnt.count; ++j)
			{
				const char* entName = STRING(pickupEnt.entName);
				pPlayer->GiveNamedItem(entName);
			}
		}
		gEvilImpulse101 = false;

		for (const auto& ammo : mapConfig.ammo)
		{
			const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(ammo.name.c_str());
			if (ammoType && ammo.count > 0)
			{
				pPlayer->GiveAmmo(ammo.count, ammoType->name);
			}
		}

		for (const auto& inventory : mapConfig.inventory)
		{
			pPlayer->GiveInventoryItem(inventory.entName, inventory.count);
		}

		if (IsCoOp() && g_modFeatures.IsWeaponEnabled(WEAPON_MEDKIT) && !mapConfig.nomedkit && !pPlayer->WeaponById(WEAPON_MEDKIT))
		{
			pPlayer->GiveNamedItem("weapon_medkit");
		}
		if (mapConfig.maxhealth > 0)
		{
			pPlayer->SetMaxHealth(mapConfig.maxhealth);
			pPlayer->pev->health = pPlayer->pev->max_health;
		}
		if (mapConfig.maxarmor.has_value())
		{
			pPlayer->SetMaxArmor(*mapConfig.maxarmor);
		}
		if (mapConfig.startarmor > 0)
			pPlayer->SetArmor(mapConfig.startarmor);
		if (mapConfig.starthealth > 0)
			pPlayer->SetHealth(mapConfig.starthealth);

		pPlayer->m_antidotes = mapConfig.antidotes;
		pPlayer->m_radcans = mapConfig.radcans;
		pPlayer->m_adrenalines = mapConfig.adrenalines;

		if (mapConfig.longjump)
		{
			pPlayer->SetLongjump(true);
		}

		CBasePlayerWeapon* pWeaponToDeploy = *mapConfig.deployWeapon ? pPlayer->GetWeaponByName(mapConfig.deployWeapon) : nullptr;
		if (pWeaponToDeploy)
		{
			if (pWeaponToDeploy != pPlayer->m_pActiveItem)
			{
				pPlayer->SwitchWeapon(pWeaponToDeploy);
			}
		}
		else
			pPlayer->SwitchToBestWeapon();

		return true;
	}
	return false;
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules *InstallGameRules()
{
	SERVER_COMMAND( "exec game.cfg\n" );
	SERVER_EXECUTE();

	if( !gpGlobals->deathmatch && !(gpGlobals->coop && gpGlobals->maxClients > 1) )
	{
		// generic half-life
		g_teamplay = 0;
		return new CHalfLifeRules;
	}
	else
	{
		if( teamplay.value > 0 )
		{
			// teamplay
			g_teamplay = 1;
			return new CHalfLifeTeamplay;
		}
		if( sv_busters.value > 0 )
		{
			g_teamplay = 0;
			return new CMultiplayBusters;
		}
		if( (int)gpGlobals->deathmatch == 1 )
		{
			// vanilla deathmatch
			g_teamplay = 0;
			return new CHalfLifeMultiplay;
		}
		else
		{
			// vanilla deathmatch??
			g_teamplay = 0;
			return new CHalfLifeMultiplay;
		}
	}
}

int TridepthValue()
{
	extern cvar_t npc_tridepth;
	return (int)npc_tridepth.value;
}

bool TridepthForAll()
{
	extern cvar_t npc_tridepth_all;
	return npc_tridepth_all.value > 0;
}

bool TridepthVertical()
{
	extern cvar_t npc_tridepth_vertical;
	return npc_tridepth_vertical.value > 0;
}

bool AllowUseThroughWalls()
{
#if FEATURE_USE_THROUGH_WALLS_CVAR
	extern cvar_t use_through_walls;
	return use_through_walls.value != 0;
#else
	return true;
#endif
}

bool NpcFollowNearest()
{
	extern cvar_t npc_follow_nearest;
	return npc_follow_nearest.value != 0;
}

bool NpcGetToEnemyNearest()
{
	extern cvar_t npc_get_to_enemy_nearest;
	return npc_get_to_enemy_nearest.value != 0;
}

float NpcForgetEnemyTime()
{
	extern cvar_t npc_forget_enemy_time;
	return npc_forget_enemy_time.value;
}

bool NpcActiveAfterCombat()
{
	extern cvar_t npc_active_after_combat;
	return npc_active_after_combat.value != 0;
}

bool NpcFollowOutOfPvs()
{
	extern cvar_t npc_follow_out_of_pvs;
	return npc_follow_out_of_pvs.value != 0;
}

bool NpcFixMeleeDistance()
{
#if FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR
	extern cvar_t npc_fix_melee_distance;
	return npc_fix_melee_distance.value != 0;
#else
	return false;
#endif
}

float GrenadeJumpFactor()
{
	extern cvar_t grenade_jump;
	return grenade_jump.value;
}
