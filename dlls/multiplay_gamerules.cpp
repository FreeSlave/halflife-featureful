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
#include	"monsters.h"
#include	"gamerules.h"
 
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#if !NO_VOICEGAMEMGR
#include	"voice_gamemgr.h"
#endif
#include	"hltv.h"
#include	"mapconfig.h"
#include	"trains.h"

extern DLL_GLOBAL CGameRules *g_pGameRules;
extern DLL_GLOBAL bool	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;

extern int g_teamplay;

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20
#define HEVCHARGER_RESPAWN_TIME 30
#define HEALTHCHARGER_RESPAWN_TIME 30

float g_flIntermissionStartTime = 0;

#if !NO_VOICEGAMEMGR
CVoiceGameMgr	g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
		if( g_teamplay )
		{
			if( g_pGameRules->PlayerRelationship( pListener, pTalker ) != GR_TEAMMATE )
			{
				return false;
			}
		}

		return true;
	}
};

static CMultiplayGameMgrHelper g_GameMgrHelper;
#endif

struct PlayerState
{
	float health;
	float armor;
	char weapons[MAX_WEAPONS][32];
	short clips[MAX_WEAPONS];
	int ammo[MAX_AMMO_TYPES];
	char currentWeapon[32];
	char nickname[32];
	char uid[33];
	bool hasSuit;
	bool hasFlashlight;
	bool hasNVG;
	bool hasLongjump;
};

static PlayerState g_playerStates[32];

const char *GetAuthID( CBaseEntity *pPlayer )
{
	static char uid[33];
	const char *authid = GETPLAYERAUTHID( pPlayer->edict() );

	if( !authid || strstr(authid, "PENDING") )
	{
		const char *ip = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "ip" );
		if( ip )
		{
			char *pUid;

			safe_snprintf( uid, 32, "IP_%s", ip );

			for( pUid = uid; *pUid; pUid++ )
				if( *pUid == '.' ) *pUid = '_';
		}
		else
			return "UNKNOWN";
	}
	else strncpy( uid, authid, 32 );

	return "UNKNOWN";
}

void SavePlayerStates()
{
	int j = 0;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEntity = UTIL_PlayerByIndex( i );
		if (pEntity && pEntity->IsPlayer() && pEntity->IsAlive())
		{
			CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pEntity);
			PlayerState* state = &g_playerStates[j];
			strncpy(state->uid, GetAuthID(pPlayer), sizeof(state->uid) - 1);
			strncpy(state->nickname, STRING(pPlayer->pev->netname), sizeof(state->nickname) - 1);

			state->hasSuit = pPlayer->HasSuit();
			state->hasFlashlight = pPlayer->HasFlashlight();
			state->hasNVG = pPlayer->HasNVG();
			state->health = pEntity->pev->health;
			state->armor = pEntity->pev->armorvalue;
			CBasePlayer* player = (CBasePlayer*)pEntity;
			state->hasLongjump = player->m_fLongJump;

			if (player->m_pActiveItem != 0)
			{
				strncpy(state->currentWeapon, STRING(player->m_pActiveItem->pev->classname), sizeof(state->currentWeapon) - 1);
			}
			int k;
			for( k = 0; k < MAX_WEAPONS; k++ )
			{
				CBasePlayerWeapon* pWeapon = player->m_rgpPlayerWeapons[k];
				if (pWeapon)
				{
					strncpy( state->weapons[k], STRING(pWeapon->pev->classname), sizeof(state->weapons[k]) - 1);
					state->clips[k] = pWeapon->m_iClip;
				}
			}
			for( k = 0; k < MAX_AMMO_TYPES; k++ )
				state->ammo[k] = player->m_rgAmmo[k];
			j++;
		}
	}
}

bool RestorePlayerState(CBasePlayer* player)
{
	const char* uid = GetAuthID(player);
	const char* nickname = STRING(player->pev->netname);
	for (size_t i=0; i<ARRAYSIZE(g_playerStates); ++i)
	{
		PlayerState* state = &g_playerStates[i];
		if (strcmp(uid, state->uid) == 0 && strcmp(nickname, state->nickname) == 0)
		{
			player->pev->health = state->health;
			player->pev->armorvalue = state->armor;
			if (state->hasSuit)
				player->SetJustSuit();
			if (state->hasFlashlight)
				player->SetFlashlight();
			if (state->hasNVG)
				player->SetNVG();
			if (state->hasLongjump)
				player->SetLongjump(true);

			int k;
			for( k = 0; k < MAX_WEAPONS; ++k)
			{
				if (*state->weapons[k])
				{
					CBaseEntity *pCreated = CBaseEntity::Create(state->weapons[k], player->pev->origin, player->pev->angles );
					if (pCreated)
					{
						CBasePlayerWeapon* pWeapon = pCreated->MyWeaponPointer();
						if (pWeapon)
						{
							pWeapon->pev->spawnflags |= SF_NORESPAWN;
							pWeapon->m_iDefaultAmmo = 0;
							pWeapon->m_iClip = state->clips[k];
							player->AddPlayerItem(pWeapon);
						}
						else
						{
							ALERT(at_console, "RestorePlayerState: expected weapon, but created entity is not a weapon\n");
							UTIL_Remove(pCreated);
						}
					}
				}
			}
			for( k = 0; k < MAX_AMMO_TYPES; ++k )
				player->m_rgAmmo[k] = state->ammo[k];
			if (*state->currentWeapon)
				player->SelectItem(state->currentWeapon);
			return true;
		}
	}
	return false;
}

void ClearPlayerStates()
{
	memset(g_playerStates, 0, sizeof(g_playerStates));
}

static char g_changelevelName[cchMapNameMost];

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************
CHalfLifeMultiplay::CHalfLifeMultiplay()
{
#if !NO_VOICEGAMEMGR
	g_VoiceGameMgr.Init( &g_GameMgrHelper, gpGlobals->maxClients );
#endif
	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	g_flIntermissionStartTime = 0;

	if (*g_changelevelName)
	{
		if (!FStrEq(STRING(gpGlobals->mapname), g_changelevelName))
		{
			memset(g_changelevelName,0,sizeof(g_changelevelName));
			ClearPlayerStates();
		}
	}
	
	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if( IS_DEDICATED_SERVER() )
	{
		// dedicated server
		/*const char *servercfgfile = CVAR_GET_STRING( "servercfgfile" );

		if( servercfgfile && servercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing dedicated server config file\n" );
			sprintf( szCommand, "exec %s\n", servercfgfile );
			SERVER_COMMAND( szCommand );
		}
		*/
		// this code has been moved into engine, to only run server.cfg once
	}
	else
	{
		// listen server
		const char *lservercfgfile = CVAR_GET_STRING( "lservercfgfile" );

		if( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing listen server config file\n" );
			sprintf( szCommand, "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}

	if (IsCoOp())
	{
		if (ReadMapConfigByMapName(mapConfig, STRING(gpGlobals->mapname)))
		{
			for (int k=0; k<mapConfig.cvarCount; ++k)
			{
				const char* name  = mapConfig.overrideCvars[k].name;
				const char* value = mapConfig.overrideCvars[k].value;
				ALERT(at_aiconsole, "Setting %s to %s\n", name, value);
				CVAR_SET_STRING(name, value);
			}
		}
	}
}

bool CHalfLifeMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
#if !NO_VOICEGAMEMGR
	if( g_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) )
		return true;
#endif
	return CGameRules::ClientCommand( pPlayer, pcmd );
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData()
{
	// load all default values
	CGameRules::RefreshSkillData();

	if (IsCoOp())
		return;

	// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// 357 Round
	gSkillData.plrDmg357 = 50;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20;// fewer pellets in deathmatch

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20;

	// RPG
	gSkillData.plrDmgRPG = 120;

	// Egon
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120;

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;

	// Desert Eagle
	gSkillData.plrDmgEagle = 34;

	// Pipe wrench
	gSkillData.plrDmgPWrench = 20;

	// 762 Round
	gSkillData.plrDmg762 = 100;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

extern cvar_t timeleft, fragsleft;

extern cvar_t mp_chattime;

//=========================================================
//=========================================================
void CHalfLifeMultiplay::Think()
{
#if !NO_VOICEGAMEMGR
	g_VoiceGameMgr.Update( gpGlobals->frametime );
#endif

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if( g_fGameOver )   // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
		if( time < 1 )
			CVAR_SET_STRING( "mp_chattime", "1" );
		else if( time > MAX_INTERMISSION_TIME )
			CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

		m_flIntermissionEndTime = g_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if( m_flIntermissionEndTime < gpGlobals->time )
		{
			if( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ( ( g_flIntermissionStartTime + MAX_INTERMISSION_TIME ) < gpGlobals->time ) ) 
				ChangeLevel(); // intermission is over
		}

		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)( flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);

	if( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if( pPlayer && pPlayer->pev->frags >= flFragLimit )
			{
				GoToIntermission();
				return;
			}

			if( pPlayer )
			{
				remain = (int)( flFragLimit - pPlayer->pev->frags );
				if( remain < bestfrags )
				{
					bestfrags = remain;
				}
			}
		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	// Updates once per second
	if( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsMultiplayer()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsDeathmatch()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsCoOp()
{
	return gpGlobals->coop ? true : false;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
	if( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return false;
	}

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
		// can't put away the active item.
		return false;
	}

	if( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon )
{
	return HLGetNextBestWeapon( pPlayer, pCurrentWeapon );
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
#if !NO_VOICEGAMEMGR
	g_VoiceGameMgr.ClientConnected( pEntity );
#endif
	return true;
}

extern int gmsgSayText;
extern int gmsgGameMode;

void CHalfLifeMultiplay::UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );  // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD( CBasePlayer *pl )
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has joined the game\n", 
		( pl->pev->netname && ( STRING( pl->pev->netname ) )[0] != 0 ) ? STRING( pl->pev->netname ) : "unconnected" ) );

	// team match?
	if( g_teamplay )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" entered the game\n",  
			STRING( pl->pev->netname ), 
			GETPLAYERUSERID( pl->edict() ),
			GETPLAYERAUTHID( pl->edict() ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pl->edict() ), "model" ) );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%i>\" entered the game\n",  
			STRING( pl->pev->netname ), 
			GETPLAYERUSERID( pl->edict() ),
			GETPLAYERAUTHID( pl->edict() ),
			GETPLAYERUSERID( pl->edict() ) );
	}

	UpdateGameMode( pl );

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	//fix a bug in the information about the player's score when he left the server, so that his score would not be transferred to another player(seems to work)
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( (int)pl->pev->frags );
		WRITE_SHORT( pl->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pl->m_szTeamName ) + 1 );
	MESSAGE_END();

	SendMOTDToClient( pl->edict() );

	// loop through all active players and send their score info to the new client
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if( plr )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
				WRITE_BYTE( i );	// client number
				WRITE_SHORT( (int)plr->pev->frags );
				WRITE_SHORT( plr->m_iDeaths );
				WRITE_SHORT( 0 );
				WRITE_SHORT( GetTeamIndex( plr->m_szTeamName ) + 1 );
			MESSAGE_END();
		}
	}

	if( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::ClientDisconnected( edict_t *pClient )
{
	if( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if( pPlayer )
		{
			FireTargets( "game_playerleave", pPlayer, pPlayer );

			// team match?
			if( g_teamplay )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" ) );
			}
			else
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%i>\" disconnected\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GETPLAYERUSERID( pPlayer->edict() ) );
			}

			pPlayer->RemoveAllItems( STRIP_ALL_ITEMS );// destroy all of the players weapons and items
		}
	}
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)falldamage.value;

	switch( iFallDamage )
	{
	case 1:
		//progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:
		// fixed
		return 10;
		break;
	}
} 

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		// my teammate hit me.
		if( ( friendlyfire.value == 0 ) && ( pAttacker != pPlayer ) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return false;
		}
	}
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerThink( CBasePlayer *pPlayer )
{
	if( g_fGameOver )
	{
		// check for button presses
		if( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = true;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

extern bool gEvilImpulse101;

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	bool		addDefault = true;
	CBaseEntity	*pWeaponEntity = NULL;
	int 		iOldAutoWepSwitch;

	iOldAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

	if (IsCoOp() && keepinventory.value && RestorePlayerState(pPlayer))
		return;

	if (IsCoOp() && mapConfig.valid)
	{
		EquipPlayerFromMapConfig(pPlayer, mapConfig);
		addDefault = false;
	}
	else
	{
		pPlayer->SetSuitAndDefaultLight();

		while( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) )
		{
			pWeaponEntity->Touch( pPlayer );
			addDefault = false;
		}
	}

	if( addDefault )
	{
		if (g_modFeatures.IsWeaponEnabled(WEAPON_MEDKIT) && IsCoOp())
		{
			pPlayer->GiveNamedItem( "weapon_medkit" );
		}
		pPlayer->GiveNamedItem( "weapon_crowbar" );
		pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
		pPlayer->GiveAmmo( 68, "9mm" );// 4 full reloads
	}

	pPlayer->m_iAutoWepSwitch = iOldAutoWepSwitch;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return true;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

bool CHalfLifeMultiplay::AllowAutoTargetCrosshair()
{
	return ( aimcrosshair.value != 0 );
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if (PlayerRelationship(pAttacker, pKilled) == GR_TEAMMATE) {
		return -10;
	} else {
		return 1;
	}
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if( ktmp && (ktmp->Classify() == CLASS_PLAYER ) )
		peKiller = (CBasePlayer*)ktmp;
	else if( ktmp && ktmp->Classify() == CLASS_VEHICLE )
	{
		CBasePlayer *pDriver = (CBasePlayer *)( (CFuncVehicle *)ktmp )->m_pDriver;

		if( pDriver != NULL )
		{
			pKiller = pDriver->pev;
			peKiller = (CBasePlayer *)pDriver;
		}
	}

	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;

	FireTargets( "game_playerdie", pVictim, pVictim );

	if( pVictim->pev == pKiller )
	{
		// killed self
		pKiller->frags -= 1;
	}
	else if( ktmp && ktmp->IsPlayer() )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill( peKiller, pVictim );

		FireTargets( "game_playerkill", ktmp, ktmp );
	}
	else
	{
		// killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( (int)pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pVictim->m_szTeamName ) + 1 );
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX( PK->edict() ) );
			WRITE_SHORT( (int)PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
			WRITE_SHORT( 0 );
			WRITE_SHORT( GetTeamIndex( PK->m_szTeamName ) + 1 );
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CHalfLifeMultiplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;

	// Hack to fix name change
	const char *tau = "tau_cannon";
	const char *gluon = "gluon gun";

	if( pevInflictor )
	{
		if( pKiller->flags & FL_CLIENT )
		{
			killer_index = ENTINDEX( ENT( pKiller ) );

			if( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );

				if( pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
		else
		{
			killer_weapon_name = STRING( pevInflictor->classname );
		}
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killer_index );						// the killer
		WRITE_BYTE( ENTINDEX( pVictim->edict() ) );		// the victim
		WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if( !strcmp( killer_weapon_name, "egon" ) )
		killer_weapon_name = gluon;
	else if( !strcmp( killer_weapon_name, "gauss" ) )
		killer_weapon_name = tau;

	if( pVictim->pev == pKiller )  
	{
		// killed self

		// team match?
		if( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",  
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );		
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" committed suicide with \"%s\"\n",  
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );		
		}
	}
	else if( pKiller->flags & FL_CLIENT )
	{
		// team match?
		if( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",  
				STRING( pKiller->netname ),
				GETPLAYERUSERID( ENT(pKiller) ),
				GETPLAYERAUTHID( ENT(pKiller) ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( ENT(pKiller) ), "model" ),
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" killed \"%s<%i><%s><%i>\" with \"%s\"\n",  
				STRING( pKiller->netname ),
				GETPLAYERUSERID( ENT(pKiller) ),
				GETPLAYERAUTHID( ENT(pKiller) ),
				GETPLAYERUSERID( ENT(pKiller) ),
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );
		}
	}
	else
	{ 
		// killed by the world

		// team match?
		if( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ), 
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );		
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ), 
				GETPLAYERAUTHID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );		
		}
	}

	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );	// command length in bytes
		WRITE_BYTE( DRC_CMD_EVENT );	// player killed
		WRITE_SHORT( ENTINDEX( pVictim->edict() ) );	// index number of primary entity
		if( pevInflictor )
			WRITE_SHORT( ENTINDEX( ENT( pevInflictor ) ) );	// index number of secondary entity
		else
			WRITE_SHORT( ENTINDEX( ENT( pKiller ) ) );	// index number of secondary entity
		WRITE_LONG( 7 | DRC_FLAG_DRAMATIC );   // eventflags (priority and flags)
	MESSAGE_END();

//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
/*
	char szText[128];

	if( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " was killed by a monster.\n" );
		return;
	}

	if( pKiller == pVictim->pev )
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " commited suicide.\n" );
	}
	else if( pKiller->flags & FL_CLIENT )
	{
		strcpy( szText, STRING( pKiller->netname ) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, "\n" );
	}
	else if( FClassnameIs( pKiller, "worldspawn" ) )
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " fell or drowned or something.\n" );
	}
	else if( pKiller->solid == SOLID_BSP )
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
}

//
bool CHalfLifeMultiplay::PlayerCanDropWeapon(CBasePlayer *pPlayer)
{
	return weaponstay.value == 0 && dropweapons.value > 0;
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay::FlWeaponRespawnTime( CBasePlayerWeapon *pWeapon )
{
	if( weaponstay.value > 0 )
	{
		// make sure it's only certain weapons
		if( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD ) )
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + (weapon_respawndelay.value == -2 ? WEAPON_RESPAWN_TIME : weapon_respawndelay.value);
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay::FlWeaponTryRespawn( CBasePlayerWeapon *pWeapon )
{
	if( pWeapon && pWeapon->WeaponId() && ( pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD ) )
	{
		if( NUMBER_OF_ENTITIES() < ( gpGlobals->maxEntities - ENTITY_INTOLERANCE ) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecWeaponRespawnSpot( CBasePlayerWeapon *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay::WeaponShouldRespawn( CBasePlayerWeapon *pWeapon )
{
	if( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	if ( weapon_respawndelay.value == -1 ) {
		if (weaponstay.value > 0 && !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD )) {
			return GR_WEAPON_RESPAWN_YES;
		}
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pItem )
{
	if( weaponstay.value > 0 )
	{
		if( (pItem->iFlags() & ITEM_FLAG_LIMITINWORLD) || (pItem->pev->spawnflags & SF_NORESPAWN) )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		if (pPlayer->WeaponById(pItem->WeaponId())) {
			return false;
		}
	}

	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn( CItem *pItem )
{
	if( item_respawndelay.value == -1 || pItem->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + (item_respawndelay.value == -2 ? ITEM_RESPAWN_TIME : item_respawndelay.value);
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
//	if( pEntity->pev->flags & FL_MONSTER )
//		return false;

	return true;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if( ammo_respawndelay.value == -1 || pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + (ammo_respawndelay.value == -2 ? AMMO_RESPAWN_TIME : ammo_respawndelay.value);
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime()
{
	return healthcharger_rechargetime.value == -2 ? HEALTHCHARGER_RESPAWN_TIME : healthcharger_rechargetime.value;
}

float CHalfLifeMultiplay::FlHEVChargerRechargeTime()
{
	return hevcharger_rechargetime.value == -2 ? HEVCHARGER_RESPAWN_TIME : hevcharger_rechargetime.value;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );	
	if( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING( pentSpawnSpot->v.target ), pPlayer, pPlayer );
	}

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if (IsCoOp() && pTarget->IsPlayer())
		return GR_TEAMMATE;
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

bool CHalfLifeMultiplay::PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if( g_footsteps && g_footsteps->value == 0 )
		return false;

	if( pl->IsOnLadder() || pl->pev->velocity.IsLength2DGreaterThan(220) )
		return true;  // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

bool CHalfLifeMultiplay::FAllowFlashlight()
{
	return flashlight.value != 0; 
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FAllowMonsters()
{
	return IsCoOp() || ( allowmonsters.value != 0 );
}

bool CHalfLifeMultiplay::FMonsterCanDropWeapons(CBaseMonster *pMonster)
{
	return npc_dropweapons.value != 0;
}

bool CHalfLifeMultiplay::FMonsterCanTakeDamage( CBaseMonster* pMonster, CBaseEntity* pAttacker )
{
	if (npckill.value == 1)
		return true;
	if (!pMonster->IsPlayer() && IsCoOp() && pMonster->IDefaultRelationship(CLASS_PLAYER) == R_AL)
	{
		if (npckill.value == 0)
		{
			return false;
		}
		else if (npckill.value == 2)
		{
			if (pAttacker)
			{
				if (pMonster->IDefaultRelationship(pAttacker) == R_AL)
				{
					return false;
				}
			}
		}
	}
	return true;
}

void CHalfLifeMultiplay::BeforeChangeLevel(const char *nextMap)
{
	if (IsCoOp() && keepinventory.value)
	{
		strncpy(g_changelevelName, nextMap, sizeof(g_changelevelName)-1);
		SavePlayerStates();
	}
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME		6

void CHalfLifeMultiplay::GoToIntermission()
{
	if( g_fGameOver )
		return;  // intermission has already been triggered, so ignore.

	MESSAGE_BEGIN( MSG_ALL, SVC_INTERMISSION );
	MESSAGE_END();

	// bounds check
	int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
	if( time < 1 )
		CVAR_SET_STRING( "mp_chattime", "1" );
	else if( time > MAX_INTERMISSION_TIME )
		CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

	m_flIntermissionEndTime = gpGlobals->time + ( (int)mp_chattime.value );
	g_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = true;
	m_iEndIntermissionButtonHit = false;
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[32];
	int minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if( p )
	{
		start = p;
		p = p->next;
		while( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[1500];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
const char *COM_Parse( const char *data )
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if( !data )
		return NULL;

// skip whitespace
skipwhite:
	while( ( c = *data ) <= ' ')
	{
		if( c == 0 )
			return NULL;                    // end of file;
		data++;
	}

	// skip // comments
	if( c=='/' && data[1] == '/' )
	{
		while( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if( c == '\"' )
	{
		data++;
		while( 1 )
		{
			c = *data++;
			if( c=='\"' || !c )
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse single characters
	if( c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if( c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while ( c > 32 );

	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( const char *buffer )
{
	const char *p;

	p = buffer;
	while( *p && *p!='\n')
	{
		if( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}

/*
==============
ReloadMapCycleFile

Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( const char *filename, mapcycle_t *cycle )
{
	char szMap[32];
	int length;
	const char *pFileList;
	const char *aFileList = pFileList = (const char *)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if( pFileList && length )
	{
		// the first map name in the file becomes the default
		while( 1 )
		{
			char szBuffer[MAX_RULE_BUFFER] = {0};
			hasbuffer = 0;

			pFileList = COM_Parse( pFileList );

			if( com_token[0] == '\0' )
				break;

			strncpyEnsureTermination( szMap, com_token);

			// Any more tokens on this line?
			if( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );

				if( com_token[0] != '\0' )
				{
					hasbuffer = 1;
					strncpyEnsureTermination( szBuffer, com_token );
				}
			}

			// Check map
			if( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = Q_max( item->minplayers, 0 );
						item->minplayers = Q_min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = Q_max( item->maxplayers, 0 );
						item->maxplayers = Q_min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}
		}

		FREE_FILE( (void*)aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if( !item )
	{
		return 0;
	}

	while( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers()
{
	int num = 0;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if( pEnt )
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString( char *s, char *szCommand )
{
	// Now make rules happen
	char pkey[512];
	char value[512];	// use two buffers so compares
				// work without stomping on each other
	char *o;
	
	if( *s == '\\' )
		s++;

	while( 1 )
	{
		o = pkey;
		while( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while( *s != '\\' && *s )
		{
			if( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat( szCommand, pkey );

		if( value[0] != '\0' )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if( !*s )
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay::ChangeLevel()
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	int curplayers;
	bool do_cycle = true;

	// find the map to change to
	const char *mapcfile = CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[0] = '\0';
	szRules[0] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		DestroyMapCycle( &mapcycle );

		if( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = false;
		}
	}

	if( do_cycle && mapcycle.items )
	{
		bool keeplooking = false;
		bool found = false;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING( gpGlobals->mapname ) );
		strcpy( szFirstMapInList, STRING( gpGlobals->mapname ) );

		// Traverse list
		for( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = false;

			ASSERT( item != NULL );

			if( item->minplayers != 0 )
			{
				if( curplayers >= item->minplayers )
				{
					found = true;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if( item->maxplayers != 0 )
			{
				if( curplayers <= item->maxplayers )
				{
					found = true;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if( keeplooking )
				continue;

			found = true;
			break;
		}

		if( !found )
		{
			item = mapcycle.next_item;
		}		

		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy( szNextMap, item->mapname );

		ExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if( !IS_MAP_VALID( szNextMap ) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	g_fGameOver = true;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}

	if( szRules[0] != '\0' )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}

	CHANGE_LEVEL( szNextMap, NULL );

	if( szCommands[0] != '\0' )
	{
		SERVER_COMMAND( szCommands );
	}
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay::SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( CVAR_GET_STRING( "motdfile" ), &length );

	// send the server name
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING( "hostname" ) );
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK + 1];
		strncpyEnsureTermination( chunk, pFileList );
		char_count += strlen( chunk );
		if( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? 0 : 1 );	// 0 means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( (void*)aFileList );
}

int CMultiplayBusters::WeaponShouldRespawn( CBasePlayerWeapon *pWeapon )
{
	if( pWeapon->WeaponId() == WEAPON_EGON )
		return GR_WEAPON_RESPAWN_NO;

	return CHalfLifeMultiplay::WeaponShouldRespawn( pWeapon );
}

bool CMultiplayBusters::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return BustingCanHaveItem( pPlayer, pItem );
}

bool CMultiplayBusters::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pItem )
{
	if( !BustingCanHaveItem( pPlayer, pItem ))
		return false;

	return CHalfLifeMultiplay::CanHavePlayerItem( pPlayer, pItem );
}

int CMultiplayBusters::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if( IsPlayerBusting( pAttacker ))
		return 1;

	if( IsPlayerBusting( pKilled ))
		return 2;

	return 0;
}
void CMultiplayBusters::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	if( IsPlayerBusting( pVictim )
	    || IsPlayerBusting( CBaseEntity::Instance( pKiller )))
		CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pevInflictor );
}

void CMultiplayBusters::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if( IsPlayerBusting( pVictim ))
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "The Buster is dead!!" );

		m_flEgonBustingCheckTime = -1.0f;

		CBasePlayer *peKiller = NULL;
		CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
		if( ktmp && ( ktmp->Classify() == CLASS_PLAYER ) )
			peKiller = (CBasePlayer*)ktmp;
		else if( ktmp && ktmp->Classify() == CLASS_VEHICLE )
		{
			CBasePlayer *pDriver = (CBasePlayer *)( (CFuncVehicle *)ktmp )->m_pDriver;

			if( pDriver != NULL )
			{
				pKiller = pDriver->pev;
				peKiller = (CBasePlayer *)pDriver;
			}
		}

		if( peKiller && peKiller->IsPlayer() )
		{
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s has killed the Buster!", STRING( peKiller->pev->netname )));
		}

		pVictim->pev->renderfx = 0;
		pVictim->pev->rendercolor = g_vecZero;
	}

	CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
}

void CMultiplayBusters::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	SetPlayerModel( pPlayer, false );
	CHalfLifeMultiplay::ClientUserInfoChanged( pPlayer, infobuffer );
}

void CMultiplayBusters::PlayerSpawn( CBasePlayer *pPlayer )
{
	CHalfLifeMultiplay::PlayerSpawn( pPlayer );
	SetPlayerModel( pPlayer, false );
}

bool IsPlayerBusting( CBaseEntity *pPlayer )
{
	if( g_pGameRules->IsBustingGame()
	    && pPlayer && pPlayer->IsPlayer()
		&& ((CBasePlayer*)pPlayer)->WeaponById( WEAPON_EGON ) != 0)
		return true;

	return false;
}

bool BustingCanHaveItem( CBasePlayer *pPlayer, CBaseEntity *pItem )
{
	if( IsPlayerBusting( pPlayer )
	    && !( strncmp( STRING( pItem->pev->classname ), "weapon_", 7 )
	    && strncmp( STRING( pItem->pev->classname ), "ammo_", 5 )))
		return false;

	return true;
}

CMultiplayBusters::CMultiplayBusters()
{
	CHalfLifeMultiplay();

	m_flEgonBustingCheckTime = -1.0;
}

void CMultiplayBusters::CheckForEgons()
{
	CBaseEntity *pPlayer;
	CWeaponBox *pWeaponBox = NULL;
	CBasePlayer *pNewBuster = NULL;
	int i, bestfrags = 9999;

	if( m_flEgonBustingCheckTime <= 0.0f )
	{
		m_flEgonBustingCheckTime = gpGlobals->time + 10.0f;
		return;
	}

	if( gpGlobals->time < m_flEgonBustingCheckTime )
		return;

	m_flEgonBustingCheckTime = -1.0f;

	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );
		if( IsPlayerBusting( pPlayer ))
			return;
	}

	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );

		if( pPlayer && pPlayer->pev->frags < bestfrags )
		{
			pNewBuster = (CBasePlayer*)pPlayer;
			bestfrags = pPlayer->pev->frags;
		}
	}

	if( !pNewBuster )
		return;

	pNewBuster->GiveNamedItem( "weapon_egon" );

	while( ( pWeaponBox = (CWeaponBox*)UTIL_FindEntityByClassname( pWeaponBox, "weaponbox" )))
	{
		// destroy weaponboxes with egons
		CBasePlayerWeapon *pWeapon = pWeaponBox->WeaponById( WEAPON_EGON );
		if( pWeapon )
		{
			pWeaponBox->Kill();
		}
	}
}

void CMultiplayBusters::Think()
{
	CheckForEgons();
	CHalfLifeMultiplay::Think();
}

void CMultiplayBusters::SetPlayerModel(CBasePlayer *pPlayer, bool bKnownBuster )
{
	const char *pszModel = NULL;

	if( bKnownBuster || IsPlayerBusting( pPlayer ))
	{
		pszModel = "ivan";
	}
	else
	{
		pszModel = "skeleton";
	}

	g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pPlayer->edict()), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict()), "model", pszModel );
}

void CMultiplayBusters::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
	if( pWeapon->WeaponId() != WEAPON_EGON )
		return;

	pPlayer->RemoveAllItems( STRIP_WEAPONS_ONLY );
	UTIL_ClientPrintAll( HUD_PRINTCENTER, "Long live the new Buster!" );
	UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s is busting!\n", STRING( pPlayer->pev->netname )));
	SetPlayerModel( pPlayer, true );
	pPlayer->pev->health = pPlayer->pev->max_health;
	pPlayer->pev->armorvalue = pPlayer->MaxArmor();
	pPlayer->pev->renderfx = kRenderFxGlowShell;
	pPlayer->pev->renderamt = 25;
	pPlayer->pev->rendercolor = Vector( 0, 75, 250 );
	pPlayer->m_rgAmmo[pWeapon->PrimaryAmmoIndex()] = pWeapon->iMaxAmmo1();
}
