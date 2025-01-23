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
// GameRules
//=========================================================
#pragma once
#if !defined(GAMERULES_H)
#define GAMERULES_H

#include "cbase.h"
#include "mapconfig.h"
//#include "weapons.h"
//#include "items.h"
class CBasePlayerWeapon;
class CBasePlayer;
class CBaseMonster;
class CItem;
class CBasePlayerAmmo;

// weapon respawning return codes
enum
{	
	GR_NONE = 0,

	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,

	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,

	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO
};

// Player relationship return codes
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL
};

class CGameRules
{
public:
	virtual ~CGameRules(){}

	virtual void RefreshSkillData( void );// fill skill data struct with proper values
	virtual void Think( void ) = 0;// GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity ) = 0;  // Can this item spawn (eg monsters don't spawn in deathmatch).

	virtual bool FAllowFlashlight( void ) = 0;// Are players allowed to switch on their flashlight?
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon ) = 0;// should the player switch to this weapon?
	virtual bool GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon ) = 0;// I can't use this weapon anymore, get me the next best one.

	// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void ) = 0;// is this a multiplayer game? (either coop or deathmatch)
	virtual bool IsDeathmatch( void ) = 0;//is this a deathmatch game?
	virtual bool IsTeamplay( void ) { return false; }// is this deathmatch game being played with team rules?
	virtual bool IsCoOp( void ) = 0;// is this a coop game?
	virtual const char *GetGameDescription( void ) { return "Half-Life"; }  // this is the game name that gets seen in the server browser
	
	// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] ) = 0;// a client just connected to the server (player hasn't spawned yet)
	virtual void InitHUD( CBasePlayer *pl ) = 0;		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient ) = 0;// a client just disconnected from the server
	virtual void UpdateGameMode( CBasePlayer *pPlayer ) {}  // the client needs to be informed of the current game mode

	// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer ) = 0;// this client just hit the ground after a fall. How much damage?
	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker ) {return true;}// can this player take damage from this attacker?
	virtual bool ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return true; }

	// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer ) = 0;// called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink( CBasePlayer *pPlayer ) = 0; // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer ) = 0;// is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) = 0;// When in the future will this player be able to spawn?
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );// Place this player on their spawnspot and face them the proper direction.

	virtual bool AllowAutoTargetCrosshair( void ) { return true; }
	virtual bool ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) { return false; }  // handles the user commands;  returns true if command handled properly
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );		// the player has changed userinfo;  can change it now

	// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) = 0;// how many points do I award whoever kills this player?
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) = 0;// Called each time a player dies
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )=  0;// Call this from within a GameRules class to report an obituary.

	// Weapon retrieval
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon );// The player is touching an CBasePlayerWeapon, do I give it to him?
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon ) = 0;// Called each time a player picks up a weapon from the ground
	virtual bool PlayerCanDropWeapon( CBasePlayer* player ) = 0;

	// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerWeapon *pWeapon ) = 0;// should this weapon respawn?
	virtual float FlWeaponRespawnTime( CBasePlayerWeapon *pWeapon ) = 0;// when may this weapon respawn?
	virtual float FlWeaponTryRespawn( CBasePlayerWeapon *pWeapon ) = 0; // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot( CBasePlayerWeapon *pWeapon ) = 0;// where in the world should this weapon respawn?

	// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// is this player allowed to take this item?
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// call each time a player picks up an item (battery, healthkit, longjump)

	// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem ) = 0;// Should this item respawn?
	virtual float FlItemRespawnTime( CItem *pItem ) = 0;// when may this item respawn?
	virtual Vector VecItemRespawnSpot( CItem *pItem ) = 0;// where in the world should this item respawn?

	// Ammo retrieval
	virtual bool CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName );// can this player take more of this ammo?
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount ) = 0;// called each time a player picks up some ammo in the world

	// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo ) = 0;// should this ammo item respawn?
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo ) = 0;// when should this ammo item respawn?
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo ) = 0;// where in the world should this ammo item respawn?
									// by default, everything spawns

	// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void ) = 0;// how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime( void ) { return 0; }// how long until a depleted HealthCharger recharges itself?

	// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) = 0;// what do I do with a player's weapons when he's killed?

	// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) = 0;// Do I drop ammo when the player dies? How much?

	// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) = 0;// what team is this entity on?
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget ) = 0;// What is the player's relationship with this entity?
	virtual int GetTeamIndex( const char *pTeamName ) { return -1; }
	virtual const char *GetIndexedTeamName( int teamIndex ) { return ""; }
	virtual bool IsValidTeam( const char *pTeamName ) { return true; }
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib ) {}
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer ) { return ""; }

	// Sounds
	virtual bool PlayTextureSounds( void ) { return true; }
	virtual bool PlayFootstepSounds( CBasePlayer *pl, float fvol ) { return true; }

	// Monsters
	virtual bool FAllowMonsters( void ) = 0;//are monsters allowed
	virtual bool FMonsterCanDropWeapons( CBaseMonster* pMonster ) = 0;
	virtual bool FMonsterCanTakeDamage( CBaseMonster* pMonster, CBaseEntity* pAttacker ) = 0;

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) {}

	virtual void BeforeChangeLevel(const char* nextMap) {}
	virtual CBasePlayer* EffectivePlayer( CBaseEntity* pActivator );
	CBasePlayer* EffectiveAlivePlayer( CBaseEntity* pActivator );

	bool EquipPlayerFromMapConfig(CBasePlayer* pPlayer, const MapConfig& mapConfig);

	virtual bool IsBustingGame() { return false; }
};

extern CGameRules *InstallGameRules( void );
bool HLGetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon );

//=========================================================
// CHalfLifeRules - rules for the single player Half-Life 
// game.
//=========================================================
class CHalfLifeRules : public CGameRules
{
public:
	CHalfLifeRules ( void );

	// GR_Think
	virtual void Think( void );
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity ) override;
	virtual bool FAllowFlashlight( void ) override { return true; }

	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon ) override;
	virtual bool GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon ) override;

	// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void ) override;
	virtual bool IsDeathmatch( void ) override;
	virtual bool IsCoOp( void ) override;

	// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] ) override;
	virtual void InitHUD(CBasePlayer *pPlayer );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );

	// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );

	// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer ) override;
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );

	virtual bool AllowAutoTargetCrosshair( void ) override;

	// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

	// Weapon retrieval
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon );

	// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerWeapon *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerWeapon *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerWeapon *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerWeapon *pWeapon );

	// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) override;
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual bool PlayerCanDropWeapon( CBasePlayer* pPlayer );

	// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );

	// Ammo retrieval
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );

	// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );

	// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );

	// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

	// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

	// Monsters
	virtual bool FAllowMonsters( void ) override;
	virtual bool FMonsterCanDropWeapons( CBaseMonster* pMonster );
	virtual bool FMonsterCanTakeDamage( CBaseMonster* pMonster, CBaseEntity* pAttacker );

	// Teamplay stuff	
	virtual const char *GetTeamID( CBaseEntity *pEntity ) {return "";}
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	CBasePlayer* EffectivePlayer( CBaseEntity* pActivator );
};

//=========================================================
// CHalfLifeMultiplay - rules for the basic half life multiplayer
// competition
//=========================================================
class CHalfLifeMultiplay : public CGameRules
{
public:
	CHalfLifeMultiplay();

	// GR_Think
	virtual void Think( void );
	virtual void RefreshSkillData( void );
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity ) override;
	virtual bool FAllowFlashlight( void ) override;

	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon ) override;
	virtual bool GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon ) override;

	// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void ) override;
	virtual bool IsDeathmatch( void ) override;
	virtual bool IsCoOp( void ) override;

	// Client connection/disconnection
	// If ClientConnected returns false, the connection is rejected and the user is provided the reason specified in
	//  svRejectReason
	// Only the client's name and remote address are provided to the dll for verification.
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] ) override;
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode

	// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual bool  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker ) override;

	// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer ) override;
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	virtual bool AllowAutoTargetCrosshair( void ) override;
	virtual bool ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) override;

	// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

	// Weapon retrieval
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon );
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon ) override;// The player is touching an CBasePlayerWeapon, do I give it to him?
	virtual bool PlayerCanDropWeapon( CBasePlayer* pPlayer );

	// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerWeapon *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerWeapon *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerWeapon *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerWeapon *pWeapon );

	// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) override;
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

	// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );

	// Ammo retrieval
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );

	// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );

	// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );
	virtual float FlHEVChargerRechargeTime( void );

	// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

	// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

	// Teamplay stuff	
	virtual const char *GetTeamID( CBaseEntity *pEntity ) {return "";}
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	virtual bool PlayTextureSounds( void ) override { return false; }
	virtual bool PlayFootstepSounds( CBasePlayer *pl, float fvol );

	// Monsters
	virtual bool FAllowMonsters( void ) override;
	virtual bool FMonsterCanDropWeapons( CBaseMonster* pMonster );
	virtual bool FMonsterCanTakeDamage( CBaseMonster* pMonster, CBaseEntity* pAttacker );

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }

	virtual void BeforeChangeLevel(const char* nextMap);

protected:
	virtual void ChangeLevel( void );
	virtual void GoToIntermission( void );
	float m_flIntermissionEndTime;
	bool m_iEndIntermissionButtonHit;
	void SendMOTDToClient( edict_t *client );

	MapConfig mapConfig;
};

bool IsPlayerBusting( CBaseEntity *pPlayer );
bool BustingCanHaveItem( CBasePlayer *pPlayer, CBaseEntity *pItem );

class CMultiplayBusters : public CHalfLifeMultiplay
{
public:
	CMultiplayBusters();
	void Think();
	void PlayerSpawn( CBasePlayer *pPlayer );
	void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor );
	bool CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pItem ) override;
	void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon );
	int WeaponShouldRespawn( CBasePlayerWeapon *pWeapon );
	bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) override;
	void CheckForEgons();
	void SetPlayerModel( CBasePlayer *pPlayer, bool bKnownBuster );
	bool IsBustingGame() override { return true; }

protected:
	float m_flEgonBustingCheckTime;
};

extern DLL_GLOBAL CGameRules *g_pGameRules;

int TridepthValue();
bool TridepthForAll();
bool AllowUseThroughWalls();
bool NpcFollowNearest();
bool NpcGetToEnemyNearest();
float NpcForgetEnemyTime();
bool NpcActiveAfterCombat();
bool NpcFollowOutOfPvs();
bool NpcFixMeleeDistance();
bool AllowGrenadeJump();

#endif // GAMERULES_H
