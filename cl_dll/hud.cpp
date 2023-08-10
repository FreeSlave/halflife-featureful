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
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <cstring>
#include <cstdio>
#include "parsemsg.h"
#include "parsetext.h"
#if USE_VGUI
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#endif

#include "demo.h"
#include "demo_api.h"

hud_player_info_t	 g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
team_info_t		g_TeamInfo[MAX_TEAMS + 1];
int		g_IsSpectator[MAX_PLAYERS+1];
int g_iPlayerClass;
int g_iTeamNumber;
int g_iUser1 = 0;
int g_iUser2 = 0;
int g_iUser3 = 0;

ConfigurableBooleanValue::ConfigurableBooleanValue() : enabled_by_default(false), configurable(true) {}

ConfigurableBoundedValue::ConfigurableBoundedValue() :
	defaultValue(0),
	minValue(0),
	maxValue(0),
	configurable(true)
{}

ConfigurableBoundedValue::ConfigurableBoundedValue(int defValue, int minimumValue, int maximumValue, bool config) :
	defaultValue(defValue), minValue(minimumValue), maxValue(maximumValue), configurable(config)
{}

ConfigurableIntegerValue::ConfigurableIntegerValue() : defaultValue(0), configurable(true) {}

ConfigurableFloatValue::ConfigurableFloatValue() : defaultValue(0.0f), configurable(true) {}

FlashlightFeatures::FlashlightFeatures() : color(0xFFFFFF), distance(2048)
{
	fade_distance.defaultValue = 600;
	fade_distance.minValue = 500;
	fade_distance.maxValue = distance;
	radius.defaultValue = 100;
	radius.minValue = 80;
	radius.maxValue = 200;
}

#define OF_NVG_DLIGHT_RADIUS 400
#define CS_NVG_DLIGHT_RADIUS 775
#define NVG_DLIGHT_MIN_RADIUS 400
#define NVG_DLIGHT_MAX_RADIUS 1000

ClientFeatures::ClientFeatures()
{
	hud_color = RGB_HUD_DEFAULT;
	hud_color_critical = RGB_REDISH;
	hud_min_alpha = MIN_ALPHA;

	hud_draw_nosuit = false;
	hud_color_nosuit = RGB_HUD_NOSUIT;

	hud_color_nvg = 0x00FFFFFF;
	hud_min_alpha_nvg = 192;

	opfor_title = FEATURE_OPFOR_SPECIFIC ? true : false;

	movemode.configurable = false;

	nvgstyle.configurable = false;
	nvgstyle.defaultValue = 1;

	nvg_cs.radius.configurable = false;
	nvg_cs.radius.defaultValue = CS_NVG_DLIGHT_RADIUS;
	nvg_cs.radius.minValue = NVG_DLIGHT_MIN_RADIUS;
	nvg_cs.radius.maxValue = NVG_DLIGHT_MAX_RADIUS;
	nvg_cs.layer_color = 0x32E132;
	nvg_cs.layer_alpha = 110;
	nvg_cs.light_color = 0x32FF32;

	nvg_opfor.radius.configurable = false;
	nvg_opfor.radius.defaultValue = OF_NVG_DLIGHT_RADIUS;
	nvg_opfor.radius.minValue = NVG_DLIGHT_MIN_RADIUS;
	nvg_opfor.radius.maxValue = NVG_DLIGHT_MAX_RADIUS;
	nvg_opfor.layer_color = RGB_GREENISH;
	nvg_opfor.layer_alpha = 255;
	nvg_opfor.light_color = 0xFAFAFA;

	memset(nvg_empty_sprite, 0, sizeof (nvg_empty_sprite));
	memset(nvg_full_sprite, 0, sizeof (nvg_full_sprite));
}

static cvar_t* CVAR_CREATE_INTVALUE(const char* name, int value, int flags)
{
	char valueStr[12];
	sprintf(valueStr, "%d", value);
	return CVAR_CREATE(name, valueStr, flags);
}

static cvar_t* CVAR_CREATE_BOOLVALUE(const char* name, bool value, int flags)
{
	return CVAR_CREATE(name, value ? "1" : "0", flags);
}

static void CreateIntegerCvarConditionally(cvar_t*& cvarPtr, const char* name, const ConfigurableIntegerValue& integerValue)
{
	if (integerValue.configurable)
		cvarPtr = CVAR_CREATE_INTVALUE( name, integerValue.defaultValue, FCVAR_ARCHIVE );
	else
		cvarPtr = 0;
}

static void CreateFloatCvarConditionally(cvar_t*& cvarPtr, const char* name, const ConfigurableFloatValue& floatValue)
{
	if (floatValue.configurable)
	{
		char valueStr[32];
		sprintf(valueStr, "%g", floatValue.defaultValue);
		cvarPtr = CVAR_CREATE(name, valueStr, FCVAR_ARCHIVE);
	}
	else
		cvarPtr = 0;
}

static void CreateIntegerCvarConditionally(cvar_t*& cvarPtr, const char* name, const ConfigurableBoundedValue& boundedValue)
{
	if (boundedValue.configurable)
		cvarPtr = CVAR_CREATE_INTVALUE( name, boundedValue.defaultValue, FCVAR_ARCHIVE );
	else
		cvarPtr = 0;
}

static void CreateBooleanCvarConditionally(cvar_t*& cvarPtr, const char* name, const ConfigurableBooleanValue& booleanValue)
{
	if (booleanValue.configurable)
		cvarPtr = CVAR_CREATE_BOOLVALUE( name, booleanValue.enabled_by_default, FCVAR_ARCHIVE );
	else
		cvarPtr = 0;
}

#if USE_VGUI
#include "vgui_ScorePanel.h"

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		color[0] = color[1] = color[2] = 255;

		if( entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo)/sizeof(g_PlayerExtraInfo[0]) )
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if ( iTeam < 0 )
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	virtual void UpdateCursorState()
	{
		gViewPort->UpdateCursorState();
	}

	virtual int	GetAckIconHeight()
	{
		return ScreenHeight - gHUD.m_iFontHeight*3 - 6;
	}

	virtual bool			CanShowSpeakerLabels()
	{
		if( gViewPort && gViewPort->m_pScoreBoard )
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;
#endif

cvar_t *hud_textmode;
float g_hud_text_color[3];

extern client_sprite_t *GetSpriteList( client_sprite_t *pList, const char *psz, int iRes, int iCount );

extern cvar_t *sensitivity;
cvar_t *cl_lw = NULL;
cvar_t *cl_viewbob = NULL;
cvar_t *cl_rollspeed = NULL;
cvar_t *cl_rollangle = NULL;

cvar_t* cl_weapon_sparks = NULL;
cvar_t* cl_weapon_wallpuff = NULL;

cvar_t* cl_muzzlelight = NULL;
cvar_t* cl_muzzlelight_monsters = NULL;

#if FEATURE_NIGHTVISION_STYLES
cvar_t *cl_nvgstyle = NULL;
#endif

#if FEATURE_CS_NIGHTVISION
cvar_t *cl_nvgradius_cs = NULL;
#endif

#if FEATURE_OPFOR_NIGHTVISION
cvar_t *cl_nvgradius_of = NULL;
#endif

cvar_t* cl_flashlight_custom = NULL;
cvar_t* cl_flashlight_radius = NULL;
cvar_t* cl_flashlight_fade_distance = NULL;

cvar_t *cl_subtitles = NULL;

void ShutdownInput( void );

//DECLARE_MESSAGE( m_Logo, Logo )
int __MsgFunc_Logo( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_Logo( pszName, iSize, pbuf );
}

int __MsgFunc_Items(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_Items(pszName, iSize, pbuf);
}

int __MsgFunc_HUDColor(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_HUDColor(pszName, iSize, pbuf );
}

//LRC
int __MsgFunc_SetFog(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SetFog( pszName, iSize, pbuf );
}

//LRC
int __MsgFunc_KeyedDLight(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_KeyedDLight( pszName, iSize, pbuf );
}

//DECLARE_MESSAGE( m_Logo, Logo )
int __MsgFunc_ResetHUD( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_ResetHUD( pszName, iSize, pbuf );
}

int __MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	gHUD.MsgFunc_InitHUD( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	gHUD.MsgFunc_ViewMode( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_SetFOV( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_SetFOV( pszName, iSize, pbuf );
}

int __MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_Concuss( pszName, iSize, pbuf );
}

int __MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GameMode( pszName, iSize, pbuf );
}

void PlayMP3( const char* pszMp3, bool loop = false )
{
	if( !IsXashFWGS() && gEngfuncs.pfnGetCvarPointer( "gl_overbright" ) )
	{
		char cmd[256];

		if (loop)
			sprintf( cmd, "mp3 loop \"%s\"\n", pszMp3 );
		else
			sprintf( cmd, "mp3 play \"%s\"\n", pszMp3 );
		gEngfuncs.pfnClientCmd( cmd );
	}
	else
		gEngfuncs.pfnPrimeMusicStream( pszMp3, loop );
}

void StopMp3()
{
	if( !IsXashFWGS() && gEngfuncs.pfnGetCvarPointer( "gl_overbright" ) )
		gEngfuncs.pfnClientCmd( "mp3 stop\n" );
	else
		gEngfuncs.pfnPrimeMusicStream( 0, 0 );
}

int __MsgFunc_PlayMP3( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *pszSound = READ_STRING();
	const int loop = READ_BYTE();

	if (*pszSound == '\0')
		StopMp3();
	else
		PlayMP3(pszSound, loop != 0);

	return 1;
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu( void )
{
#if USE_VGUI
	if ( gViewPort )
	{
		gViewPort->ShowCommandMenu( gViewPort->m_StandardMenu );
	}
#endif
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial( void )
{
#if USE_VGUI
	if ( gViewPort )
	{
		gViewPort->InputPlayerSpecial();
	}
#endif
}

void __CmdFunc_CloseCommandMenu( void )
{
#if USE_VGUI
	if ( gViewPort )
	{
		gViewPort->InputSignalHideCommandMenu();
	}
#endif
}

void __CmdFunc_ForceCloseCommandMenu( void )
{
#if USE_VGUI
	if ( gViewPort )
	{
		gViewPort->HideCommandMenu();
	}
#endif
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
			return gViewPort->MsgFunc_ValClass( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_TeamNames( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_Feign( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_Feign( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_Detpack( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_Detpack( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_VGUIMenu( pszName, iSize, pbuf );
#endif
	return 0;
}

#if USE_VGUI && !USE_NOVGUI_MOTD
int __MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_MOTD( pszName, iSize, pbuf );
	return 0;
}
#endif

int __MsgFunc_BuildSt( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_BuildSt( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_RandomPC( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_RandomPC( pszName, iSize, pbuf );
#endif
	return 0;
}
 
int __MsgFunc_ServerName( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_ServerName( pszName, iSize, pbuf );
#endif
	return 0;
}

#if USE_VGUI && !USE_NOVGUI_SCOREBOARD
int __MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ScoreInfo( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamScore(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamScore( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamInfo( pszName, iSize, pbuf );
	return 0;
}
#endif

int __MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_Spectator( pszName, iSize, pbuf );
#endif
	return 0;
}

#if USE_VGUI
int __MsgFunc_SpecFade(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_SpecFade( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_ResetFade(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ResetFade( pszName, iSize, pbuf );
	return 0;

}
#endif

int __MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf )
{
#if USE_VGUI
	if (gViewPort)
		return gViewPort->MsgFunc_AllowSpec( pszName, iSize, pbuf );
#endif
	return 0;
}
 
// This is called every time the DLL is loaded
void CHud::Init( void )
{
	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( Concuss );
	HOOK_MESSAGE( Items );
	HOOK_MESSAGE( HUDColor );
	HOOK_MESSAGE( SetFog );
	HOOK_MESSAGE( KeyedDLight );

	// TFFree CommandMenu
	HOOK_COMMAND( "+commandmenu", OpenCommandMenu );
	HOOK_COMMAND( "-commandmenu", CloseCommandMenu );
	HOOK_COMMAND( "ForceCloseCommandMenu", ForceCloseCommandMenu );
	HOOK_COMMAND( "special", InputPlayerSpecial );

	HOOK_MESSAGE( ValClass );
	HOOK_MESSAGE( TeamNames );
	HOOK_MESSAGE( Feign );
	HOOK_MESSAGE( Detpack );
	HOOK_MESSAGE( BuildSt );
	HOOK_MESSAGE( RandomPC );
	HOOK_MESSAGE( ServerName );

#if USE_VGUI && !USE_NOVGUI_MOTD
	HOOK_MESSAGE( MOTD );
#endif

#if USE_VGUI && !USE_NOVGUI_SCOREBOARD
	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );
#endif

	HOOK_MESSAGE( Spectator );
	HOOK_MESSAGE( AllowSpec );

#if USE_VGUI
	HOOK_MESSAGE( SpecFade );
	HOOK_MESSAGE( ResetFade );
#endif

	// VGUI Menus
	HOOK_MESSAGE( VGUIMenu );

	HOOK_MESSAGE( PlayMP3 );

	CVAR_CREATE( "hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );		// controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE( "hud_takesshots", "0", FCVAR_ARCHIVE );		// controls whether or not to automatically take screenshots at the end of a round
	hud_textmode = CVAR_CREATE ( "hud_textmode", "0", FCVAR_ARCHIVE );

	m_iLogo = 0;
	m_iFOV = 0;

	ParseClientFeatures();

	CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	default_fov = CVAR_CREATE( "default_fov", "90", FCVAR_ARCHIVE );
	m_pCvarStealMouse = CVAR_CREATE( "hud_capturemouse", "1", FCVAR_ARCHIVE );
	m_pCvarDraw = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	CreateBooleanCvarConditionally(m_pCvarDrawMoveMode, "hud_draw_movemode", clientFeatures.movemode);
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );
	m_pCvarCrosshair = gEngfuncs.pfnGetCvarPointer( "crosshair" );

	CreateBooleanCvarConditionally(cl_viewbob, "cl_viewbob", clientFeatures.view_bob);
	CreateFloatCvarConditionally(cl_rollangle, "cl_rollangle", clientFeatures.rollangle);
	cl_rollspeed = gEngfuncs.pfnRegisterVariable ( "cl_rollspeed", "200", FCVAR_CLIENTDLL|FCVAR_ARCHIVE );

	CreateBooleanCvarConditionally(cl_weapon_sparks, "cl_weapon_sparks", clientFeatures.weapon_sparks);
	CreateBooleanCvarConditionally(cl_weapon_wallpuff, "cl_weapon_wallpuff", clientFeatures.weapon_wallpuff);

	CreateBooleanCvarConditionally(cl_muzzlelight, "cl_muzzlelight", clientFeatures.muzzlelight);
	cl_muzzlelight_monsters = CVAR_CREATE( "cl_muzzlelight_monsters", "0", FCVAR_ARCHIVE );

#if FEATURE_NIGHTVISION_STYLES
	CreateIntegerCvarConditionally(cl_nvgstyle, "cl_nvgstyle", clientFeatures.nvgstyle);
#endif

#if FEATURE_CS_NIGHTVISION
	CreateIntegerCvarConditionally(cl_nvgradius_cs, "cl_nvgradius_cs", clientFeatures.nvg_cs.radius );
#endif

#if FEATURE_OPFOR_NIGHTVISION
	CreateIntegerCvarConditionally(cl_nvgradius_of, "cl_nvgradius_of", clientFeatures.nvg_opfor.radius );
#endif

	CreateBooleanCvarConditionally(cl_flashlight_custom, "cl_flashlight_custom", clientFeatures.flashlight.custom);

	if (clientFeatures.flashlight.radius.configurable)
	{
		cl_flashlight_radius = CVAR_CREATE_INTVALUE( "cl_flashlight_radius", clientFeatures.flashlight.radius.defaultValue, FCVAR_CLIENTDLL|FCVAR_ARCHIVE );
	}
	if (clientFeatures.flashlight.fade_distance.configurable)
	{
		cl_flashlight_fade_distance = CVAR_CREATE_INTVALUE( "cl_flashlight_fade_distance", clientFeatures.flashlight.radius.defaultValue, FCVAR_CLIENTDLL|FCVAR_ARCHIVE );
	}

	cl_subtitles = CVAR_CREATE( "cl_subtitles", "1", FCVAR_ARCHIVE );

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
#if FEATURE_MOVE_MODE
	m_MoveMode.Init();
#endif
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	m_Nightvision.Init();
#if USE_VGUI
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);
#endif

#if !USE_VGUI || USE_NOVGUI_MOTD
	m_MOTD.Init();
#endif
#if !USE_VGUI || USE_NOVGUI_SCOREBOARD
	m_Scoreboard.Init();
#endif

	m_Menu.Init();

	m_Caption.Init();
	m_Achievements.Init();

	MsgFunc_ResetHUD( 0, 0, NULL );
}

const char* strStartsWith(const char* str, const char* start)
{
	while (*start && *str == *start)
	{
		start++;
		str++;
	}
	if (*start == '\0')
		return str;
	return 0;
}

bool UpdateBooleanValue(ConfigurableBooleanValue& value, const char* key, const char* valueStr)
{
	if (strcmp("enabled_by_default", key) == 0)
	{
		return ParseBoolean(valueStr, value.enabled_by_default);
	}
	else if (strcmp("configurable", key) == 0)
	{
		return ParseBoolean(valueStr, value.configurable);
	}
	return false;
}

bool UpdateBoundedValue(ConfigurableBoundedValue& value, const char* key, const char* valueStr)
{
	if (strcmp("default", key) == 0)
	{
		return ParseInteger(valueStr, value.defaultValue);
	}
	else if (strcmp("min", key) == 0)
	{
		return ParseInteger(valueStr, value.minValue);
	}
	else if (strcmp("max", key) == 0)
	{
		return ParseInteger(valueStr, value.maxValue);
	}
	else if (strcmp("configurable", key) == 0)
	{
		return ParseBoolean(valueStr, value.configurable);
	}
	return false;
}

bool UpdateIntegerValue(ConfigurableIntegerValue& value, const char* key, const char* valueStr)
{
	if (strcmp("default", key) == 0)
	{
		return ParseInteger(valueStr, value.defaultValue);
	}
	else if (strcmp("configurable", key) == 0)
	{
		return ParseBoolean(valueStr, value.configurable);
	}
	return false;
}

bool UpdateFloatValue(ConfigurableFloatValue& value, const char* key, const char* valueStr)
{
	if (strcmp("default", key) == 0)
	{
		return ParseFloat(valueStr, value.defaultValue);
	}
	else if (strcmp("configurable", key) == 0)
	{
		return ParseBoolean(valueStr, value.configurable);
	}
	return false;
}

bool UpdateNVGValue(NVGFeatures& nvg, const char* key, const char* valueStr)
{
	const char* subKey = 0;

	if (strcmp("layer_color", key) == 0)
	{
		return ParseColor(valueStr, nvg.layer_color);
	}
	else if (strcmp("layer_alpha", key) == 0)
	{
		return ParseInteger(valueStr, nvg.layer_alpha);
	}
	else if (strcmp("light_color", key) == 0)
	{
		return ParseColor(valueStr, nvg.light_color);
	}
	else if ((subKey = strStartsWith(key, "radius.")))
	{
		UpdateBoundedValue(nvg.radius, subKey, valueStr);
	}
	return false;
}

template <typename T>
struct KeyValueDefinition
{
	const char* name;
	T& value;
};

void CHud::ParseClientFeatures()
{
	const char* fileName = "features/featureful_client.cfg";
	int fileSize = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &fileSize );
	if ( !pfile )
	{
		fileName = "featureful_client.cfg";
		pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &fileSize );
	}

	if( !pfile )
		return;

	gEngfuncs.Con_DPrintf("Parsing client features from %s\n", fileName);

	KeyValueDefinition<int> colors[] = {
		{ "hud_color", clientFeatures.hud_color},
		{ "hud_color_critical", clientFeatures.hud_color_critical},
		{ "hud_color_nosuit", clientFeatures.hud_color_nosuit},
		{ "hud_color_nvg", clientFeatures.hud_color_nvg},
		{ "flashlight.color", clientFeatures.flashlight.color},
	};
	KeyValueDefinition<int> integers[] = {
		{ "hud_min_alpha", clientFeatures.hud_min_alpha },
		{ "hud_min_alpha_nvg", clientFeatures.hud_min_alpha_nvg },
		{ "flashlight.distance", clientFeatures.flashlight.distance },
	};
	KeyValueDefinition<ConfigurableBooleanValue> configurableBooleans[] = {
		{ "flashlight.custom.", clientFeatures.flashlight.custom},
		{ "view_bob.", clientFeatures.view_bob},
		{ "weapon_wallpuff.", clientFeatures.weapon_wallpuff},
		{ "weapon_sparks.", clientFeatures.weapon_sparks},
		{ "muzzlelight.", clientFeatures.muzzlelight},
		{ "movemode.", clientFeatures.movemode},
	};
	KeyValueDefinition<ConfigurableBoundedValue> configurableBounds[] = {
		{ "flashlight.radius.", clientFeatures.flashlight.radius },
		{ "flashlight.fade_distance.", clientFeatures.flashlight.fade_distance },
	};
	KeyValueDefinition<ConfigurableFloatValue> configurableFloats[] = {
		{ "rollangle.", clientFeatures.rollangle },
	};
	KeyValueDefinition<bool> booleans[] = {
		{ "hud_draw_nosuit", clientFeatures.hud_draw_nosuit},
		{ "opfor_title", clientFeatures.opfor_title},
	};

	char valueBuf[CLIENT_FEATURE_VALUE_LENGTH+1];
	int i = 0;
	while ( i<fileSize )
	{
		if (pfile[i] == ' ' || pfile[i] == '\r' || pfile[i] == '\n')
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, fileSize);
		}
		else
		{
			const int keyStart = i;
			ConsumeNonSpaceCharacters(pfile, i, fileSize);

			const int keyLength = i - keyStart;
			SkipSpaces(pfile, i, fileSize);
			const int valueStart = i;
			ConsumeLineSignificantOnly(pfile, i, fileSize);
			const int valueLength = i - valueStart;
			if (valueLength == 0)
			{
				continue;
			}
			if (valueLength > CLIENT_FEATURE_VALUE_LENGTH)
			{
				continue;
			}

			strncpy(valueBuf, pfile + valueStart, valueLength);
			valueBuf[valueLength] = '\0';

			char* keyName = pfile + keyStart;
			keyName[keyLength] = '\0';

			const char* subKey = 0;

			unsigned int i = 0;
			bool shouldContinue = true;
			for (i = 0; shouldContinue && i<sizeof(colors)/sizeof(colors[0]); ++i)
			{
				if (strcmp(keyName, colors[i].name) == 0)
				{
					ParseColor(valueBuf, colors[i].value);
					shouldContinue = false;
					break;
				}
			}
			for (i = 0; shouldContinue && i<sizeof(integers)/sizeof(integers[0]); ++i)
			{
				if (strcmp(keyName, integers[i].name) == 0)
				{
					ParseInteger(valueBuf, integers[i].value);
					shouldContinue = false;
					break;
				}
			}
			for (i = 0; shouldContinue && i<sizeof(configurableBooleans)/sizeof(configurableBooleans[0]); ++i)
			{
				if ((subKey = strStartsWith(keyName, configurableBooleans[i].name)))
				{
					UpdateBooleanValue(configurableBooleans[i].value, subKey, valueBuf);
					shouldContinue = false;
					break;
				}
			}
			for (i = 0; shouldContinue && i<sizeof(configurableFloats)/sizeof(configurableFloats[0]); ++i)
			{
				if ((subKey = strStartsWith(keyName, configurableFloats[i].name)))
				{
					UpdateFloatValue(configurableFloats[i].value, subKey, valueBuf);
					shouldContinue = false;
					break;
				}
			}
			for (i = 0; shouldContinue && i<sizeof(configurableBounds)/sizeof(configurableBounds[0]); ++i)
			{
				if ((subKey = strStartsWith(keyName, configurableBounds[i].name)))
				{
					UpdateBoundedValue(configurableBounds[i].value, subKey, valueBuf);
					shouldContinue = false;
					break;
				}
			}
			for (i = 0; shouldContinue && i<sizeof(booleans)/sizeof(booleans[0]); ++i)
			{
				if (strcmp(keyName, booleans[i].name) == 0)
				{
					ParseBoolean(valueBuf, booleans[i].value);
					shouldContinue = false;
					break;
				}
			}
			if (shouldContinue)
			{
				if ((subKey = strStartsWith(keyName, "nvgstyle.")))
				{
					UpdateIntegerValue(clientFeatures.nvgstyle, subKey, valueBuf);
				}
				else if ((subKey = strStartsWith(keyName, "nvg_cs.")))
				{
					UpdateNVGValue(clientFeatures.nvg_cs, subKey, valueBuf);
				}
				else if ((subKey = strStartsWith(keyName, "nvg_opfor.")))
				{
					UpdateNVGValue(clientFeatures.nvg_opfor, subKey, valueBuf);
				}
				else if (strcmp(keyName, "nvg_empty_sprite") == 0)
				{
					strncpy(clientFeatures.nvg_empty_sprite, valueBuf, MAX_SPRITE_NAME_LENGTH);
					clientFeatures.nvg_empty_sprite[MAX_SPRITE_NAME_LENGTH-1] = '\0';
				}
				else if (strcmp(keyName, "nvg_full_sprite") == 0)
				{
					strncpy(clientFeatures.nvg_full_sprite, valueBuf, MAX_SPRITE_NAME_LENGTH);
					clientFeatures.nvg_full_sprite[MAX_SPRITE_NAME_LENGTH-1] = '\0';
				}
			}
		}
	}

	gEngfuncs.COM_FreeFile(pfile);
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;

	if( m_pHudList )
	{
		HUDLIST *pList;
		while( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud::GetSpriteIndex( const char *SpriteName )
{
	// look through the loaded sprite name list for SpriteName
	for( int i = 0; i < m_iSpriteCount; i++ )
	{
		if( strncmp( SpriteName, m_rgszSpriteNames + ( i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
			return i;
	}

	return -1; // invalid sprite
}

void CHud::VidInit( void )
{
	int j;
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo( &m_scrinfo );

	// ----------
	// Load Sprites
	// ---------
	//m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;	
	m_hsprCursor = 0;

	if( ScreenWidth < 640 )
		m_iRes = 320;
	else
		m_iRes = 640;

	// Only load this once
	if( !m_pSpriteList )
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList( "sprites/hud.txt", &m_iSpriteCountAllRes );

		if( m_pSpriteList )
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			for( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if( p->iRes == m_iRes )
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
 			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if( p->iRes == m_iRes )
				{
					char sz[256];
					sprintf( sz, "sprites/%s.spr", p->szSprite );
					m_rghSprites[index] = SPR_Load( sz );
					m_rgrcRects[index] = p->rc;
					strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH - 1 );
					m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH + ( MAX_SPRITE_NAME_LENGTH - 1 )] = '\0';
					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;

		// count the number of sprites of the appropriate res
		m_iSpriteCount = 0;
		for( j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if( p->iRes == m_iRes )
				m_iSpriteCount++;
			p++;
		}

		delete[] m_rghSprites;
		delete[] m_rgrcRects;
		delete[] m_rgszSpriteNames;

		// allocated memory for sprite handle arrays
 		m_rghSprites = new HSPRITE[m_iSpriteCount];
		m_rgrcRects = new wrect_t[m_iSpriteCount];
		m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

		p = m_pSpriteList;
		int index = 0;
		for( j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if( p->iRes == m_iRes )
			{
				char sz[256];
				sprintf( sz, "sprites/%s.spr", p->szSprite );
				m_rghSprites[index] = SPR_Load( sz );
				m_rgrcRects[index] = p->rc;
				strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH - 1 );
				m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH + ( MAX_SPRITE_NAME_LENGTH - 1 )] = '\0';
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex( "number_0" );

	if( m_HUD_number_0 == -1 )
	{
		const char *msg = "There is something wrong with your game data! Please, reinstall\n";

		if( HUD_MessageBox( msg ) )
		{
			gEngfuncs.pfnClientCmd( "quit\n" );
		}

		return;
	}

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
#if FEATURE_MOVE_MODE
	m_MoveMode.VidInit();
#endif
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
#if USE_VGUI
	GetClientVoiceMgr()->VidInit();
#endif
#if !USE_VGUI || USE_NOVGUI_MOTD
	m_MOTD.VidInit();
#endif
#if !USE_VGUI || USE_NOVGUI_SCOREBOARD
	m_Scoreboard.VidInit();
#endif
	m_Nightvision.VidInit();

	m_Caption.VidInit();
	m_Achievements.VidInit();

	memset(&fog, 0, sizeof(fog));
}

int CHud::MsgFunc_Logo( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	// update Train data
	m_iLogo = READ_BYTE();

	return 1;
}

int CHud::MsgFunc_Items(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_iItemBits = READ_LONG();
	return 1;
}

int CHud::MsgFunc_HUDColor(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	clientFeatures.hud_color = READ_LONG();

	return 1;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase ( const char *in, char *out )
{
	int len, start, end;

	len = strlen( in );

	// scan backward for '.'
	end = len - 1;
	while( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;

	if( in[end] != '.' )		// no '.', copy to end
		end = len - 1;
	else 
		end--;					// Found ',', copy to left of '.'

	// Scan backward for '/'
	start = len - 1;
	while( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if( in[start] != '/' && in[start] != '\\' )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy( out, &in[start], len );

	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame( const char *game )
{
	const char *gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if( gamedir && gamedir[0] )
	{
		COM_FileBase( gamedir, gd );
		if( !stricmp( gd, game ) )
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV( void )
{
	if( gEngfuncs.pDemoAPI->IsRecording() )
	{
		// Write it
		int i = 0;
		unsigned char buf[100];

		// Active
		*(float *)&buf[i] = g_lastFOV;
		i += sizeof(float);

		Demo_WriteBuffer( TYPE_ZOOM, i, buf );
	}

	if( gEngfuncs.pDemoAPI->IsPlayingback() )
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

int CHud::MsgFunc_SetFOV( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT( "default_fov" );

	g_lastFOV = newfov;

	if( newfov == 0 )
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if( m_iFOV == def_fov )
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	return 1;
}

void CHud::AddHudElem( CHudBase *phudelem )
{
	HUDLIST *pdl, *ptemp;

	//phudelem->Think();

	if( !phudelem )
		return;

	pdl = (HUDLIST *)malloc( sizeof(HUDLIST) );
	if( !pdl )
		return;

	memset( pdl, 0, sizeof(HUDLIST) );
	pdl->p = phudelem;

	if( !m_pHudList )
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while( ptemp->pNext )
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}

void CHud::GetAllPlayersInfo()
{
	for( int i = 1; i < MAX_PLAYERS; i++ )
	{
		GetPlayerInfo( i, &g_PlayerInfoList[i] );

		if( g_PlayerInfoList[i].thisplayer )
		{
#if USE_VGUI
			if(gViewPort)
				gViewPort->m_pScoreBoard->m_iPlayerNum = i;
#endif
#if !USE_VGUI || USE_NOVGUI_SCOREBOARD
			m_Scoreboard.m_iPlayerNum = i;  // !!!HACK: this should be initialized elsewhere... maybe gotten from the engine
#endif
		}
	}
}

bool CHud::ClientFeatureEnabled(cvar_t* cVariable, bool defaultValue)
{
	if (cVariable)
		return cVariable->value != 0;
	return defaultValue;
}

bool CHud::ViewBobEnabled()
{
	return ClientFeatureEnabled(cl_viewbob, clientFeatures.view_bob.enabled_by_default);
}

bool CHud::WeaponWallpuffEnabled()
{
	return ClientFeatureEnabled(cl_weapon_wallpuff, clientFeatures.weapon_wallpuff.enabled_by_default);
}

bool CHud::WeaponSparksEnabled()
{
	return ClientFeatureEnabled(cl_weapon_sparks, clientFeatures.weapon_sparks.enabled_by_default);
}

bool CHud::MuzzleLightEnabled()
{
	return ClientFeatureEnabled(cl_muzzlelight, clientFeatures.muzzlelight.enabled_by_default);
}

bool CHud::CustomFlashlightEnabled()
{
	return ClientFeatureEnabled(cl_flashlight_custom, clientFeatures.flashlight.custom.enabled_by_default);
}

int CHud::NVGStyle()
{
#if FEATURE_NIGHTVISION_STYLES
	if (cl_nvgstyle)
		return (int)cl_nvgstyle->value;
#endif
	return clientFeatures.nvgstyle.defaultValue;
}

bool CHud::MoveModeEnabled()
{
	return ClientFeatureEnabled(m_pCvarDrawMoveMode, clientFeatures.movemode.enabled_by_default);
}

#if FEATURE_MOVE_MODE
DECLARE_MESSAGE( m_MoveMode, MoveMode )

int CHudMoveMode::Init()
{
	m_movementState = MovementStand;
	HOOK_MESSAGE(MoveMode);
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
}

void CHudMoveMode::Reset()
{
	m_movementState = MovementStand;
}

int CHudMoveMode::VidInit()
{
	int HUD_mode_stand = gHUD.GetSpriteIndex("mode_stand");
	int HUD_mode_run = gHUD.GetSpriteIndex("mode_run");
	int HUD_mode_crouch = gHUD.GetSpriteIndex("mode_crouch");
	int HUD_mode_jump = gHUD.GetSpriteIndex("mode_jump");

	m_hSpriteStand = gHUD.GetSprite(HUD_mode_stand);
	m_hSpriteRun = gHUD.GetSprite(HUD_mode_run);
	m_hSpriteCrouch = gHUD.GetSprite(HUD_mode_crouch);
	m_hSpriteJump = gHUD.GetSprite(HUD_mode_jump);

	m_prcStand = gHUD.GetSpriteRectPointer(HUD_mode_stand);
	m_prcRun = gHUD.GetSpriteRectPointer(HUD_mode_run);
	m_prcCrouch = gHUD.GetSpriteRectPointer(HUD_mode_crouch);
	m_prcJump = gHUD.GetSpriteRectPointer(HUD_mode_jump);

	return 1;
}

int CHudMoveMode::Draw(float flTime)
{
	if (!gHUD.MoveModeEnabled())
		return 1;

	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) || !gHUD.HasSuit() )
	{
		return 1;
	}
	int r, g, b, x, y;
	wrect_t* prc;
	HSPRITE sprite;
	UnpackRGB( r,g,b, gHUD.HUDColor() );

	switch (m_movementState) {
	case MovementRun:
		sprite = m_hSpriteRun;
		prc = m_prcRun;
		break;
	case MovementCrouch:
		sprite = m_hSpriteCrouch;
		prc = m_prcCrouch;
		break;
	case MovementJump:
		sprite = m_hSpriteJump;
		prc = m_prcJump;
		break;
	default:
		sprite = m_hSpriteStand;
		prc = m_prcStand;
		break;
	}

	if (!sprite || !prc)
		return 1;

	const wrect_t rc = *prc;
	int width = rc.right - rc.left;
	y = ( rc.bottom - rc.top ) * 2;
	x = ScreenWidth - width - width / 2;

	SPR_Set( sprite, r, g, b );
	SPR_DrawAdditive( 0,  x, y, &rc );
	return 1;
}

int CHudMoveMode::MsgFunc_MoveMode(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_movementState = READ_SHORT();
	return 1;
}
#endif

bool CHud::ShouldUseConsoleFont()
{
	return !IsXashFWGS();
}

int CHud::GetMessageConsoleWidth(const char* message, unsigned int length)
{
	char buf[512] = {0};
	length = Q_min(length, sizeof(buf) - 1);
	strncpy(buf, message, length);
	buf[length] = '\0';
	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen(buf, &width, &height);
	return width;
}

int CHud::GetConsoleFontHeight()
{
	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen("YAW", &width, &height);
	return height;
}

int CHud::GetLineHeight()
{
	if (CHud::ShouldUseConsoleFont())
		return GetConsoleFontHeight();
	else
		return gHUD.m_scrinfo.iCharHeight + 1;
}

int CHud::GetLineWidth(const char *str)
{
	int width = 0;
	if (CHud::ShouldUseConsoleFont())
	{
		int dummy;
		gEngfuncs.pfnDrawConsoleStringLen(str, &width, &dummy);
	}
	else
	{
		while (*str != '\0')
		{
			width += gHUD.m_scrinfo.charWidths[(unsigned char)*str];
			str++;
		}
	}
	return width;
}

static bool IsSpaceCharacter(char c)
{
	return c == ' ' || c == '\n' || c == '\r';
}

unsigned int CHud::SplitIntoWordBoundaries(WordBoundary* boundaries, const char *message)
{
	unsigned int wordCount = 0;

	const unsigned int len = strlen(message);

	bool searchingForWordStart = true;
	for (unsigned int i = 0; i<len; ++i)
	{
		if (searchingForWordStart && !IsSpaceCharacter(message[i]))
		{
			boundaries[wordCount].wordStart = i;
			searchingForWordStart = false;
		}

		if (!searchingForWordStart && IsSpaceCharacter(message[i]))
		{
			boundaries[wordCount].wordEnd = i;
			wordCount++;
			searchingForWordStart = true;
		}
	}
	if (!searchingForWordStart) {
		boundaries[wordCount].wordEnd = len;
		wordCount++;
	}

	return wordCount;
}
