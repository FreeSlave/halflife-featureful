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
#include	"mod_features.h"
 
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#ifndef NO_VOICEGAMEMGR
#include	"voice_gamemgr.h"
#endif
#include	"hltv.h"

extern DLL_GLOBAL CGameRules *g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
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

#ifndef NO_VOICEGAMEMGR
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

void UTIL_BecomeSpectator( CBasePlayer *pPlayer )
{
	pPlayer->pev->flags &= FL_PROXY;	// keep proxy flag sey by engine
	pPlayer->pev->flags |= FL_SPECTATOR;
	pPlayer->pev->effects |= EF_NODRAW;
	pPlayer->pev->maxspeed = 0;
	pPlayer->StartObserver(pPlayer->pev->origin, pPlayer->pev->v_angle);
}

void UTIL_RescuePlayer( CBasePlayer *pPlayer, entvars_t* where )
{
	pPlayer->m_iRespawnFrames = 0;
	pPlayer->pev->effects &= ~EF_NODRAW;
	pPlayer->pev->flags &= ~FL_SPECTATOR;

	pPlayer->m_iRespawnPoint = 1;
	pPlayer->StopObserver();

	pPlayer->pev->origin = where->origin + Vector( 0, 0, 1 );
	pPlayer->pev->v_angle  = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = where->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;

	pPlayer->SayRescued();
}

class CInfoPlayerRescue : public CPointEntity
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS( info_player_rescue, CInfoPlayerRescue )

void CInfoPlayerRescue::Spawn()
{
	CPointEntity::Spawn();
}

class CTriggerRescue : public CPointEntity
{
public:
	void Spawn( void );
	void Think( void );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void UseOff();
	inline int RescueState() {
		return pev->iuser1;
	}
	inline void SetResqueState(int state) {
		pev->iuser1 = state;
	}

	enum {
		InActive,
		Active,
		Disabled
	};
};

LINK_ENTITY_TO_CLASS( trigger_rescue, CTriggerRescue )

void CTriggerRescue::UseOff()
{
	if (RescueState() == Active) {
		SetResqueState( InActive );
		SUB_UseTargets(this, USE_OFF, 0.0f);
		EMIT_SOUND( edict(), CHAN_VOICE, "common/null.wav", 1.0, ATTN_IDLE );
	}
}

void CTriggerRescue::Spawn()
{
	CPointEntity::Spawn();
	pev->nextthink = gpGlobals->time + 5;
	SetResqueState( InActive );
	pev->frags = gpGlobals->time;
}

void CTriggerRescue::Think()
{
	if (survival.value) {
		BOOL rescuablePlayersExist = CHalfLifeMultiplay::IsAnyPlayerRescuable();

		if (RescueState() == InActive && rescuablePlayersExist) {
			SetResqueState( Active );
			SUB_UseTargets(this, USE_ON, 0.0f);
			pev->frags = gpGlobals->time + RANDOM_LONG(1,4);
		} else if (RescueState() == Active && !rescuablePlayersExist) {
			UseOff();
		}

		if (RescueState() == Active && rescuablePlayersExist && pev->frags < gpGlobals->time) {
			CBasePlayer* players[4] = {NULL, NULL, NULL, NULL};
			int j = 0;

			for( int i = 1; i <= gpGlobals->maxClients && j < sizeof(players)/sizeof(players[0]); i++ )
			{
				CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
				if (pPlayer && (pPlayer->pev->flags & FL_SPECTATOR)) {
					players[j] = (CBasePlayer*)pPlayer;
					j++;
				}
			}
			if (j != 0 && players[RANDOM_LONG(0, j-1)]->CallForRescue(edict())) {
				pev->frags = gpGlobals->time + 15 + RANDOM_LONG(0,5);
			}
		}
	}
	pev->nextthink = gpGlobals->time + 5;
}

void CTriggerRescue::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* triggerRescue = NULL;
	while((triggerRescue = UTIL_FindEntityByClassname(triggerRescue, "trigger_rescue")) != NULL) {
		((CTriggerRescue*)triggerRescue)->UseOff();
	}

	SetResqueState( Disabled );
	if (survival.value) {
		CBaseEntity *pEntity = NULL;
		while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 128 ) ) != NULL ) {
			if (FClassnameIs(pEntity->pev, "info_player_rescue")) {
				CBasePlayer* pPlayer = CHalfLifeMultiplay::FindRescuablePlayer();
				if (pPlayer) {
					UTIL_RescuePlayer(pPlayer, pEntity->pev);
					UTIL_Remove(pEntity);
				} else {
					break; // no more players to rescue
				}
			}
		}
	}

	UTIL_Remove(this);
}

class CTriggerSurvival : public CPointEntity
{
public:
	void Spawn();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS( trigger_survival, CTriggerSurvival )

void CTriggerSurvival::Spawn()
{
	CPointEntity::Spawn();
	if (!g_pGameRules->IsMultiplayer()) {
		UTIL_Remove(this);
	}
}

void CTriggerSurvival::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (g_pGameRules->IsMultiplayer()) {
		CHalfLifeMultiplay* rules = (CHalfLifeMultiplay*)g_pGameRules;
		rules->EnableSurvival();
	}
	UTIL_Remove(this);
}

struct PlayerState
{
	float health;
	float armor;
	char weapons[MAX_WEAPONS][32];
	short clips[MAX_WEAPONS];
	int ammo[MAX_AMMO_SLOTS];
	char currentWeapon[32];
	char nickname[32];
	char uid[33];
	bool hasSuit;
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

			snprintf( uid, 32, "IP_%s", ip );

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
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->IsPlayer() && pPlayer->IsAlive())
		{
			PlayerState* state = &g_playerStates[j];
			strncpy(state->uid, GetAuthID(pPlayer), sizeof(state->uid) - 1);
			strncpy(state->nickname, STRING(pPlayer->pev->netname), sizeof(state->nickname) - 1);

			state->hasSuit = (pPlayer->pev->weapons & ( 1 << WEAPON_SUIT )) != 0;
			state->health = pPlayer->pev->health;
			state->armor = pPlayer->pev->armorvalue;
			CBasePlayer* player = (CBasePlayer*)pPlayer;
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
			for( k = 0; k < MAX_AMMO_SLOTS; k++ )
				state->ammo[k] = player->m_rgAmmo[k];
			j++;
		}
	}
}

bool RestorePlayerState(CBasePlayer* player)
{
	const char* uid = GetAuthID(player);
	const char* nickname = STRING(player->pev->netname);
	for (int i=0; i<ARRAYSIZE(g_playerStates); ++i)
	{
		PlayerState* state = &g_playerStates[i];
		if (strcmp(uid, state->uid) == 0 && strcmp(nickname, state->nickname) == 0)
		{
			player->pev->health = state->health;
			player->pev->armorvalue = state->armor;
			if (state->hasSuit)
				player->pev->weapons |= ( 1 << WEAPON_SUIT );
			if (state->hasLongjump)
			{
				player->m_fLongJump = TRUE;
				g_engfuncs.pfnSetPhysicsKeyValue( player->edict(), "slj", "1" );
			}

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
			for( k = 0; k < MAX_AMMO_SLOTS; ++k )
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

struct AmmoEnt
{
	char entName[32];
	short count;
};

struct WeaponEnt
{
	char entName[32];
	short count;
};

struct OverrideCvar
{
	char name[32];
	char value[32];
};

struct MapConfig
{
	WeaponEnt weapons[MAX_WEAPONS];
	AmmoEnt ammo[MAX_AMMO_SLOTS];
	OverrideCvar overrideCvars[32];
	int cvarCount;

	int weaponsCount;
	int ammoCount;

	int starthealth;
	int startarmor;

	bool nomedkit;
	bool nosuit;
	bool longjump;

	bool valid;
};

MapConfig g_mapConfig;

bool ReadMapConfig(const char* mapName)
{
	memset(&g_mapConfig, 0, sizeof(g_mapConfig));
	char fileName[cchMapNameMost + 8];
	sprintf(fileName, "maps/%s.cfg", mapName);
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if( !pMemFile )
		return false;
	char buffer[512];
	memset( buffer, 0, sizeof(buffer) );
	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer) ) != NULL )
	{
		int i = 0;
		while( buffer[i] && buffer[i] == ' ' )
			i++;
		if( !buffer[i] || buffer[i] == '\n' )
			continue;
		if( buffer[i] == '/' || !isalpha( buffer[i] ) )
			continue;
		int j = i;
		while( buffer[j] && buffer[j] != ' ' && buffer[j] != '\n' )
			j++;
		char* key = buffer+i;
		char* value = buffer+j;
		if (buffer[j] && buffer[j] != '\n')
		{
			value = buffer+j+1;
			while(*value && *value == ' ')
				value++;
			int k = 0;
			while( value[k] && value[k] != ' ' && value[k] != '\n' )
				k++;
			value[k] = '\0';
		}

		key[j-i] = '\0';
		if (strncmp(key, "weapon_", 7) == 0)
		{
			if (g_mapConfig.weaponsCount < MAX_WEAPONS)
				strncpy(g_mapConfig.weapons[g_mapConfig.weaponsCount].entName, key, 31);
			g_mapConfig.weapons[g_mapConfig.weaponsCount].count = atoi(value);
			if (!g_mapConfig.weapons[g_mapConfig.weaponsCount].count)
				g_mapConfig.weapons[g_mapConfig.weaponsCount].count = 1;
			g_mapConfig.weaponsCount++;
		}
		else if (strncmp(key, "ammo_", 5) == 0)
		{
			if (g_mapConfig.ammoCount < MAX_AMMO_SLOTS)
			{
				strncpy(g_mapConfig.ammo[g_mapConfig.ammoCount].entName, key, 31);
				g_mapConfig.ammo[g_mapConfig.ammoCount].count = atoi(value);
				if (!g_mapConfig.ammo[g_mapConfig.ammoCount].count)
					g_mapConfig.ammo[g_mapConfig.ammoCount].count = 1;
				g_mapConfig.ammoCount++;
			}
		}
		else if (strncmp(key, "nomedkit", 8) == 0)
		{
			g_mapConfig.nomedkit = true;
		}
		else if (strncmp(key, "nosuit", 6) == 0)
		{
			g_mapConfig.nosuit = true;
		}
		else if (strncmp(key, "item_longjump", 6) == 0)
		{
			g_mapConfig.longjump = true;
		}
		else if (strncmp(key, "startarmor", 10) == 0)
		{
			g_mapConfig.startarmor = atoi(value);
		}
		else if (strncmp(key, "starthealth", 11) == 0)
		{
			g_mapConfig.starthealth = atoi(value);
		}
		else if (strncmp(key, "sv_", 3) == 0 || strncmp(key, "mp_", 3) == 0 || strncmp(key, "npc_", 4) == 0)
		{
			if (g_mapConfig.cvarCount < 32)
			{
				strncpy(g_mapConfig.overrideCvars[g_mapConfig.cvarCount].name, key, 31);
				strncpy(g_mapConfig.overrideCvars[g_mapConfig.cvarCount].value, value, 31);
				g_mapConfig.cvarCount++;
			}
		}
	}
	g_engfuncs.pfnFreeFile( pMemFile );
	g_mapConfig.valid = true;
	return true;
}

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************
CHalfLifeMultiplay::CHalfLifeMultiplay()
{
#ifndef NO_VOICEGAMEMGR
	g_VoiceGameMgr.Init( &g_GameMgrHelper, gpGlobals->maxClients );
#endif
	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	g_flIntermissionStartTime = 0;
	m_flSurvivalStartTime = gpGlobals->time;
	m_flNextSurvivalStartTime = gpGlobals->time;
	m_survivalState = SurvivalWaitForPlayers;
	m_restartTheSameMap = false;
	m_hasTriggerSurvival = -1;

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

	if (IsCoOp() && ReadMapConfig(STRING(gpGlobals->mapname)))
	{
		for (int k=0; k<g_mapConfig.cvarCount; ++k)
		{
			const char* name  = g_mapConfig.overrideCvars[k].name;
			const char* value = g_mapConfig.overrideCvars[k].value;
			ALERT(at_aiconsole, "Setting %s to %s\n", name, value);
			CVAR_SET_STRING(name, value);
		}
	}
}

BOOL CHalfLifeMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
#ifndef NO_VOICEGAMEMGR
	if( g_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) )
		return TRUE;
#endif
	return CGameRules::ClientCommand( pPlayer, pcmd );
}

void CHalfLifeMultiplay::ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer)
{
	static const char* allowedModels[] = {"barney", "scientist", "helmet", "gordon", "gina", "robo"};

	if (IsCoOp()) {
		const char *playerModel = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );

		bool isAllowed = false;
		for (int i=0; i<sizeof(allowedModels)/sizeof(const char*); ++i) {
			if (strcmp(playerModel, allowedModels[i]) == 0) {
				isAllowed = true;
				break;
			}
		}
		if (!isAllowed) {
			const char* randomModel = allowedModels[RANDOM_LONG(0, sizeof(allowedModels)/sizeof(const char*)-1)];
			g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), infobuffer, "model", randomModel );
		}
		pPlayer->RefreshCharacter();
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData( void )
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
	gSkillData.plrDmg357 = 40;

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

#if FEATURE_DESERT_EAGLE
	// Desert Eagle
	gSkillData.plrDmgEagle = 34;
#endif

#if FEATURE_PIPEWRENCH
	// Pipe wrench
	gSkillData.plrDmgPWrench = 20;
#endif

#if FEATURE_SNIPERRIFLE
	// 762 Round
	gSkillData.plrDmg762 = 100;
#endif
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

extern cvar_t timeleft, fragsleft;

extern cvar_t mp_chattime;

int CHalfLifeMultiplay::m_numberOfTries = 0;

//=========================================================
//=========================================================
void CHalfLifeMultiplay::Think( void )
{
#ifndef NO_VOICEGAMEMGR
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

	if (m_hasTriggerSurvival == -1) {
		m_hasTriggerSurvival = !FNullEnt(FIND_ENTITY_BY_CLASSNAME(NULL, "trigger_survival"));

		if (m_hasTriggerSurvival) {
			ALERT(at_console, "trigger_survival exists on the map\n");
		} else {
			ALERT(at_console, "trigger_survival does not exist on the map\n");
		}
	}

	if (!m_hasTriggerSurvival && IsSurvivalMode()) {
		if (m_survivalState == SurvivalWaitForPlayers) {
			if (IsAnyPlayerAlive()) {
				m_survivalState = SurvivalCountdown;
				m_flSurvivalStartTime = gpGlobals->time + survival_warmup_time.value;
			}
		} else if (m_survivalState == SurvivalCountdown) {
			if (!IsAnyPlayerConnected()) {
				m_survivalState = SurvivalWaitForPlayers;
			} else {
				if (m_flSurvivalStartTime > gpGlobals->time) {
					if (m_flNextSurvivalStartTime <= gpGlobals->time) {
						int secondsLeftForSurv = (int)ceil(m_flSurvivalStartTime - gpGlobals->time);
						UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "Survival starts in %d", secondsLeftForSurv ));
						m_flNextSurvivalStartTime = gpGlobals->time+1;
					}
				} else {
					ClearPlayerStates();
					EnableSurvival();
				}
			}
		} else if (m_survivalState == SurvivalEnabled) {
			if (!IsAnyPlayerAlive()) {
				m_numberOfTries++;
				int triesLeft = (int)survival_restart_number.value - m_numberOfTries;
				if (triesLeft <= 0) {
					UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "No alive players left.\n\nChanging the map." ));
					m_numberOfTries = 0;
					m_restartTheSameMap = false;
				} else if (triesLeft == 1) {
					UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "No alive players left.\n\nTry the last time." ));
					m_restartTheSameMap = true;
				} else {
					UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "No alive players left.\n\nYou have %d more attempts.", triesLeft ));
					m_restartTheSameMap = true;
				}
				GoToIntermission();
			}
		}
	}
}

bool CHalfLifeMultiplay::IsSurvivalMode()
{
	return IsCoOp() && survival.value;
}

void CHalfLifeMultiplay::EnableSurvival()
{
	m_survivalState = SurvivalEnabled;
	m_hasTriggerSurvival = 0;
	UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "Survival mode enabled!" ));
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsMultiplayer( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsDeathmatch( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsCoOp( void )
{
	return gpGlobals->coop ? TRUE : FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon )
{
	if( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if( !pPlayer->m_iAutoWepSwitch )
	{
		return FALSE;
	}

	if (l4m_weapon_system.value) {
		return TRUE;
	} else {
		if( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
		{
			return TRUE;
		}

		return FALSE;
	}
}

BOOL CHalfLifeMultiplay::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon )
{
	CBasePlayerWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -100;
	pBest = NULL;

	if( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pCheck = pPlayer->m_rgpPlayerWeapons[i];

		if ( pCheck )
		{
			if( pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category.
				if ( pCheck->CanDeploy() )
				{
					if ( pPlayer->SwitchWeapon( pCheck ) )
					{
						return TRUE;
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

	// if we make it here, we've checked all the weapons and found no useable
	// weapon in the same catagory as the current weapon.

	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.
	if( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest );

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
#ifndef NO_VOICEGAMEMGR
	g_VoiceGameMgr.ClientConnected( pEntity );
#endif
	return TRUE;
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
	MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
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
			pPlayer->SentenceStop();
			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

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

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items
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
BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		// my teammate hit me.
		if( ( friendlyfire.value == 0 ) && ( pAttacker != pPlayer ) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerThink( CBasePlayer *pPlayer )
{
	if( g_fGameOver )
	{
		// check for button presses
		if( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

extern int gEvilImpulse101;

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	pPlayer->m_flKilledTime = gpGlobals->time;

	BOOL		addDefault = TRUE;
	bool giveSuit = true;
	CBaseEntity	*pWeaponEntity = NULL;

	if (survival.value && m_survivalState == SurvivalEnabled && !pPlayer->m_iRespawnPoint) {
		UTIL_BecomeSpectator(pPlayer);
		return;
	}

	if (IsCoOp() && keepinventory.value && RestorePlayerState(pPlayer))
		return;

	if (IsCoOp() && g_mapConfig.valid)
	{
		int i, j;
		gEvilImpulse101 = TRUE;
		for (i=0; i<g_mapConfig.weaponsCount; ++i)
		{
			for (j=0; j<g_mapConfig.weapons[i].count; ++j)
			{
				pPlayer->GiveNamedItem(g_mapConfig.weapons[i].entName);
			}
		}
		for (i=0; i<g_mapConfig.ammoCount; ++i)
		{
			for (j=0; j<g_mapConfig.ammo[i].count; ++j)
			{
				pPlayer->GiveNamedItem(g_mapConfig.ammo[i].entName);
			}
		}
		gEvilImpulse101 = FALSE;
		if (g_mapConfig.nosuit)
			giveSuit = false;

#if FEATURE_MEDKIT
		if (!pPlayer->WeaponById(WEAPON_MEDKIT) && !g_mapConfig.nomedkit)
		{
			pPlayer->GiveNamedItem("weapon_medkit");
		}
#endif
		if (g_mapConfig.startarmor > 0)
			pPlayer->pev->armorvalue = Q_min(g_mapConfig.startarmor, MAX_NORMAL_BATTERY);
		if (g_mapConfig.starthealth > 0 && g_mapConfig.starthealth < pPlayer->pev->max_health)
			pPlayer->pev->health = g_mapConfig.starthealth;

		if (g_mapConfig.longjump)
		{
			pPlayer->m_fLongJump = TRUE;
			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );
		}

		pPlayer->SwitchToBestWeapon();

		addDefault = FALSE;
	}

	if (giveSuit)
		pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );

	while( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) )
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	if( addDefault )
	{
#if FEATURE_MEDKIT
		if (IsCoOp())
		{
			pPlayer->GiveNamedItem( "weapon_medkit" );
		}
#endif
		pPlayer->GiveNamedItem( "weapon_crowbar" );
		pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
		pPlayer->GiveAmmo( 68, "9mm" );// 4 full reloads
	}
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	if (survival.value) {
		BOOL result = m_survivalState != SurvivalEnabled;
		if (!result && (pPlayer->m_flKilledTime + 4 < gpGlobals->time) ) {
			pPlayer->pev->nextthink = -1;
			UTIL_BecomeSpectator(pPlayer);
		}
		return result;
	} else {
		return TRUE;
	}
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

BOOL CHalfLifeMultiplay::AllowAutoTargetCrosshair( void )
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
	pVictim->m_flKilledTime = gpGlobals->time;
	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;

	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if( ktmp && (ktmp->Classify() == CLASS_PLAYER ) )
		peKiller = (CBasePlayer*)ktmp;

	if( pVictim->pev == pKiller )
	{
		// killed self
		pKiller->frags -= 1;
	}
	else if( ktmp && ktmp->IsPlayer() )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill( peKiller, pVictim );

		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );
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

	if( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX( ENT( pKiller ) );

		if( pevInflictor )
		{
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
	}
	else
	{
		killer_weapon_name = STRING( pevInflictor->classname );
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
	if( pWeapon && pWeapon->m_iId && ( pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD ) )
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

	if (IsSurvivalMode())
		return GR_WEAPON_RESPAWN_NO;

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns FALSE if the player is not allowed
// to pick up this weapon
//=========================================================
BOOL CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerWeapon *pItem )
{
	if( weaponstay.value > 0 )
	{
		if( (pItem->iFlags() & ITEM_FLAG_LIMITINWORLD) || (pItem->pev->spawnflags & SF_NORESPAWN) )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		if (pPlayer->WeaponById(pItem->m_iId)) {
			return FALSE;
		}
	}

	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
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

	if (IsSurvivalMode())
		return GR_ITEM_RESPAWN_NO;

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
BOOL CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
//	if( pEntity->pev->flags & FL_MONSTER )
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if( ammo_respawndelay.value == -1 || pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	if (IsSurvivalMode())
		return GR_AMMO_RESPAWN_NO;

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
float CHalfLifeMultiplay::FlHealthChargerRechargeTime( void )
{
	if (IsSurvivalMode())
		return 0;
	return healthcharger_rechargetime.value == -2 ? HEALTHCHARGER_RESPAWN_TIME : healthcharger_rechargetime.value;
}

float CHalfLifeMultiplay::FlHEVChargerRechargeTime( void )
{
	if (IsSurvivalMode())
		return 0;
	return hevcharger_rechargetime.value == -2 ? HEVCHARGER_RESPAWN_TIME : hevcharger_rechargetime.value;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	if (survival.value) {
		if (m_survivalState != SurvivalEnabled) {
			return GR_PLR_DROP_GUN_NO;
		} else {
			return GR_PLR_DROP_GUN_ALL;
		}
	}
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	if (survival.value) {
		if (m_survivalState != SurvivalEnabled) {
			return GR_PLR_DROP_AMMO_NO;
		} else {
			return GR_PLR_DROP_AMMO_ALL;
		}
	}
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );	
	if( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING( pentSpawnSpot->v.target ), pPlayer, pPlayer, USE_TOGGLE, 0 );
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

BOOL CHalfLifeMultiplay::PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if( g_footsteps && g_footsteps->value == 0 )
		return FALSE;

	if( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220 )
		return TRUE;  // only make step sounds in multiplayer if the player is moving fast enough

	return FALSE;
}

BOOL CHalfLifeMultiplay::FAllowFlashlight( void )
{
	return flashlight.value != 0; 
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FAllowMonsters( void )
{
	return IsCoOp() || ( allowmonsters.value != 0 );
}

bool CHalfLifeMultiplay::FMonsterCanDropWeapons(CBaseMonster *pMonster)
{
	return npc_dropweapons.value != 0;
}

bool CHalfLifeMultiplay::IsTimeForPanic()
{
	return m_panicTime < gpGlobals->time;
}

void CHalfLifeMultiplay::DelayPanic(float delay)
{
	float toAdd = delay;

	if (!delay) {
		toAdd = defaultpanicdelay.value;
	} else if (delay < minpanicdelay.value) {
		ALERT(at_console, "Too short panic delay detected (%f). Resetting to minimum panic delay value (%f)\n", delay, minpanicdelay.value);
		toAdd = minpanicdelay.value;
	}
	toAdd *= gSkillData.panicDelayFactor;
	ALERT(at_console, "Next panic delay: %f\n", toAdd);
	m_panicTime = gpGlobals->time + toAdd;
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

void CHalfLifeMultiplay::GoToIntermission( void )
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

	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;
}

//=================
//Check if some alive players left
BOOL CHalfLifeMultiplay::IsAnyPlayerAlive()
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->IsAlive()) {
			return TRUE;
		}
	}
	return FALSE;
}

//=================
//Check if some connected players left
BOOL CHalfLifeMultiplay::IsAnyPlayerConnected()
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer) {
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CHalfLifeMultiplay::IsAnyPlayerRescuable()
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer && (pPlayer->pev->flags & FL_SPECTATOR)) {
			return TRUE;
		}
	}
	return FALSE;
}

CBasePlayer* CHalfLifeMultiplay::FindRescuablePlayer()
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer && (pPlayer->pev->flags & FL_SPECTATOR)) {
			return (CBasePlayer*)pPlayer;
		}
	}
	return NULL;
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

			strcpy( szMap, com_token );

			// Any more tokens on this line?
			if( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );

				if( com_token[0] != '\0' )
				{
					hasbuffer = 1;
					strcpy( szBuffer, com_token );
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
int CountPlayers( void )
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
void CHalfLifeMultiplay::ChangeLevel( void )
{
	if (m_restartTheSameMap) {
		CHANGE_LEVEL( STRING(gpGlobals->mapname), NULL );
		return;
	}
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	int curplayers;
	BOOL do_cycle = TRUE;

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
			do_cycle = FALSE;
		}
	}

	if( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING( gpGlobals->mapname ) );
		strcpy( szFirstMapInList, STRING( gpGlobals->mapname ) );

		// Traverse list
		for( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if( item->minplayers != 0 )
			{
				if( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if( item->maxplayers != 0 )
			{
				if( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if( keeplooking )
				continue;

			found = TRUE;
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

	g_fGameOver = TRUE;

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

		if( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			strcpy( chunk, pFileList );
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( (void*)aFileList );
}
