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
//
// teamplay_gamerules.cpp
//

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"ammunition.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"items.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL bool	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;

//=========================================================
//=========================================================
CHalfLifeRules::CHalfLifeRules( void )
{
	SERVER_COMMAND( "exec spserver.cfg\n" );
	RefreshSkillData();
}

//=========================================================
//=========================================================
void CHalfLifeRules::Think( void )
{
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsMultiplayer( void )
{
	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsDeathmatch( void )
{
	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsCoOp( void )
{
	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
	if( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return true;
	}

	if( !pPlayer->m_iAutoWepSwitch )
	{
		return false;
	}

	if( pPlayer->m_iAutoWepSwitch == 2
	    && pPlayer->m_afButtonLast & ( IN_ATTACK | IN_ATTACK2 ) )
	{
		return false;
	}

	if( !pPlayer->m_pActiveItem->CanHolster() )
	{
		return false;
	}

	if( !pPlayer->m_settingsLoaded && pWeapon->iWeight() < pPlayer->m_pActiveItem->iWeight() )
	{
		return false;
	}

	return true;
}

//=========================================================
//=========================================================
bool HLGetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon )
{
	CBasePlayerWeapon *pBest = NULL;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	int i;

	if( pCurrentWeapon && !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return false;
	}

	const int currentWeight = pCurrentWeapon ? pCurrentWeapon->iWeight() : 0;

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pCheck = pPlayer->m_rgpPlayerWeapons[i];

		if( pCheck )
		{
			if( !FBitSet( pCheck->iFlags(), ITEM_FLAG_NOAUTOSWITCHTO ))
			{
				if( pCheck->iWeight() > -1 && pCheck->iWeight() == currentWeight && pCheck != pCurrentWeapon )
				{
					// this weapon is from the same category.
					if ( pCheck->CanDeploy() )
					{
						if ( pPlayer->SwitchWeapon( pCheck ) )
						{
							return true;
						}
					}
				}
				else if( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
				{
					//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
					// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
					// that the player was using. This will end up leaving the player with his heaviest-weighted
					// weapon.
					if( pCheck->CanDeploy() )
					{
						// if this weapon is useable, flag it as the best
						iBestWeight = pCheck->iWeight();
						pBest = pCheck;
					}
				}
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable
	// weapon in the same catagory as the current weapon.

	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.
	if( !pBest )
	{
		return false;
	}

	pPlayer->SwitchWeapon( pBest );

	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon )
{
	if( pCurrentWeapon && FBitSet( pCurrentWeapon->iFlags(), ITEM_FLAG_EXHAUSTIBLE ))
		return HLGetNextBestWeapon( pPlayer, pCurrentWeapon );
	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
	return true;
}

void CHalfLifeRules::InitHUD( CBasePlayer *pPlayer )
{
}

//=========================================================
//=========================================================
void CHalfLifeRules::ClientDisconnected( edict_t *pClient )
{
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerSpawn( CBasePlayer *pPlayer )
{
}

//=========================================================
//=========================================================
bool CHalfLifeRules::AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerThink( CBasePlayer *pPlayer )
{
	if ( !pPlayer->m_fInitHUD && !pPlayer->m_settingsLoaded)
	{
		MapConfig mapConfig;
		bool readConfig = ReadMapConfigByMapName(mapConfig, STRING(gpGlobals->mapname));

		if (readConfig)
		{
			EquipPlayerFromMapConfig(pPlayer, mapConfig);
		}
		CBaseEntity* pSettingEntity = NULL;
		while ( (pSettingEntity = UTIL_FindEntityByClassname( pSettingEntity, "game_player_settings" )) != NULL )
		{
			// If game_player_settings has a name, it means to be called by trigger, not run automatically.
			if (FStringNull(pSettingEntity->pev->targetname))
			{
				// If equiped from the config, just fire the game_player_settings target
				if (readConfig)
					pSettingEntity->SUB_UseTargets(pPlayer);
				else
					pSettingEntity->Touch( pPlayer );
				break;
			}
		}

		FireTargets( "game_playerspawn", pPlayer, pPlayer );
		pPlayer->m_settingsLoaded = true;
	}
}

//=========================================================
//=========================================================
bool CHalfLifeRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return true;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeRules::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	FireTargets( "game_playerdie", pVictim, pVictim );
}

//=========================================================
// Deathnotice
//=========================================================
void CHalfLifeRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeRules::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
}

//
bool CHalfLifeRules::PlayerCanDropWeapon(CBasePlayer *pPlayer)
{
	return false;
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeRules::FlWeaponRespawnTime( CBasePlayerWeapon *pWeapon )
{
	return -1;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeRules::FlWeaponTryRespawn( CBasePlayerWeapon *pWeapon )
{
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecWeaponRespawnSpot( CBasePlayerWeapon *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeRules::WeaponShouldRespawn( CBasePlayerWeapon *pWeapon )
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeRules::FlItemRespawnTime( CItem *pItem )
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return -1;
}

//=========================================================
//=========================================================
Vector CHalfLifeRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlHealthChargerRechargeTime( void )
{
	return 0;// don't recharge
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// why would a single player in half life need this? 
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::FAllowMonsters( void )
{
	return true;
}

bool CHalfLifeRules::FMonsterCanDropWeapons(CBaseMonster *pMonster )
{
	return true;
}

bool CHalfLifeRules::FMonsterCanTakeDamage( CBaseMonster* pMonster, CBaseEntity* pAttacker )
{
	return true;
}

CBasePlayer *CHalfLifeRules::EffectivePlayer(CBaseEntity *pActivator)
{
	return (CBasePlayer*)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
}
