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
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//
#pragma once
#if !defined(HUD_H)
#define HUD_H

#include <cstdint>
#include "mod_features.h"

#define FOG_LIMIT 30000

#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,16,16
#define RGB_GREENISH 0x0000A000 //0,160,0

#if FEATURE_OPFOR_SPECIFIC
#define RGB_HUD_DEFAULT RGB_GREENISH
#else
#define RGB_HUD_DEFAULT RGB_YELLOWISH
#endif

#define RGB_HUD_NOSUIT 0x00CCCCCC

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"
#include "dlight.h"
#include "template_property_types.h"

#include "hud_renderer.h"
#include "hud_inventory.h"
#include "hud_objecthint.h"
#include "message_strings.h"

#include <array>
#include <vector>
#include <string>

#include "cvardef.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4
#define DHN_4DIGITS  8
#define MIN_ALPHA	 100	

#define		HUDELEM_ACTIVE	1

enum 
{ 
	MAX_PLAYERS = 64,
	MAX_TEAMS = 64,
	MAX_TEAM_NAME = 16
};

typedef struct cvar_s cvar_t;

#define HUD_ACTIVE	1
#define HUD_INTERMISSION 2

#define MAX_PLAYER_NAME_LENGTH		32

#define	MAX_MOTD_LENGTH				1536

#define MAX_SERVERNAME_LENGTH	64
#define MAX_TEAMNAME_SIZE 32

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	int   m_type;
	int	  m_iFlags; // active, moving, 
	virtual		~CHudBase() {}
	virtual int Init( void ) { return 0; }
	virtual int VidInit( void ) { return 0; }
	virtual int Draw( float flTime ) { return 0; }
	virtual void Think( void ) { return; }
	virtual void Reset( void ) { return; }
	virtual void InitHUDData( void ) {}		// called every time a server is connected to
};

struct HUDLIST
{
	CHudBase	*p;
	HUDLIST		*pNext;
};

//
//-----------------------------------------------------
#if USE_VGUI
#include "voice_status.h" // base voice handling class
#endif
#include "hud_spectator.h"

//
//-----------------------------------------------------
//
class CHudAmmo : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Think( void );
	void Reset( void );
	int SpriteIndexForSlot(int iSlot);
	int DrawWList( float flTime );
	int MsgFunc_CurWeapon( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AmmoList( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeaponList( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AmmoX( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf );

	void SlotInput( int iSlot );
	void _cdecl UserCmd_Slot1( void );
	void _cdecl UserCmd_Slot2( void );
	void _cdecl UserCmd_Slot3( void );
	void _cdecl UserCmd_Slot4( void );
	void _cdecl UserCmd_Slot5( void );
	void _cdecl UserCmd_Slot6( void );
	void _cdecl UserCmd_Slot7( void );
	void _cdecl UserCmd_Slot8( void );
	void _cdecl UserCmd_Slot9( void );
	void _cdecl UserCmd_Slot10( void );
	void _cdecl UserCmd_Close( void );
	void _cdecl UserCmd_NextWeapon( void );
	void _cdecl UserCmd_PrevWeapon( void );

	WEAPON *GetWeapon() {
		return m_pWeapon;
	}

	float DrawHistoryTime();
	bool FastSwitchEnabled();

private:
	float m_fFade;
	WEAPON *m_pWeapon;
	int m_HUD_bucket0;
	int m_HUD_selection;
	int m_HUD_buckets[WEAPON_SLOTS_HARDLIMIT];
	int m_HUD_bucket_none;

	cvar_t* m_pCvarDrawHistoryTime;
	cvar_t* m_pCvarHudFastSwitch;
};

//
//-----------------------------------------------------
//
class CHudAmmoSecondary : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SecAmmoIcon( const char *pszName, int iSize, void *pbuf );

private:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};


#include "health.h"


#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Geiger( const char *pszName, int iSize, void *pbuf );
	
private:
	int m_iGeigerRange;
};

//
//-----------------------------------------------------
//
class CHudTrain : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Train( const char *pszName, int iSize, void *pbuf );

private:
	HSPRITE m_hSprite;
	int m_iPos;
};

class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	bool HandleMOTDMessage( const char *pszName, int iSize, void *pbuf );
	void Scroll( int dir );
	void Scroll( float amount );
	float scroll;
	bool m_bShow;

	char m_szMOTD[MAX_MOTD_LENGTH];
protected:
	static int MOTD_DISPLAY_TIME;

	int m_iLines;
	int m_iMaxLength;
};

class CHudErrorCollection : public CHudBase
{
public:
	int Init();
	int VidInit();
	void Reset();
	int Draw(float flTime);
	int MsgFunc_ParseErrors( const char *pszName, int iSize, void *pbuf );
	void SetClientErrors(const std::string& str);

private:
	int DrawMultiLineString(const char* str, int xpos, int ypos, int xmax, const int LineHeight);

	std::string m_clientErrorString;
	std::string m_serverErrorString;
};

struct CaptionProfile_t
{
	char firstLetter;
	char secondLetter;
	int r, g, b;
};

struct Caption_t
{
	Caption_t();
	Caption_t(const char* captionName);
	char name[32];
	const CaptionProfile_t* profile;
	std::string message;
	float delay;
	float duration;
};

#define SUB_MAX_LINES 5

struct Subtitle_t
{
	const Caption_t* caption;
	int lineOffsets[SUB_MAX_LINES];
	int lineEndOffsets[SUB_MAX_LINES];
	int r, g, b;
	float timeLeft;
	float timeBeforeStart;
	int lineCount;
	bool radio;
};

class CHudCaption : public CHudBase
{
public:
	int Init();
	int VidInit();
	void Update(float flTime, float flTimeDelta);
	int Draw(float flTime);
	void Reset();

	int MsgFunc_Caption( const char *pszName, int iSize, void *pbuf );
	void AddSubtitle(const Subtitle_t& sub);
	void CalculateLineOffsets(Subtitle_t& sub);
	void RecalculateLineOffsets();

	void UserCmd_DumpCaptions();

	bool ParseCaptionsProfilesFile();
	bool ParseCaptionsFile();
	const Caption_t* CaptionLookup(const char* name);

protected:
	bool ParseFloatParameter(char* pfile, int& currentTokenStart, unsigned int& tokenLength, Caption_t &caption);

	CaptionProfile_t *CaptionProfileLookup(char firstLetter, char secondLetter);

	CaptionProfile_t defaultProfile;
	std::vector<CaptionProfile_t> profiles;
	Caption_t defaultCaption;
	std::vector<Caption_t> captions;

	Subtitle_t subtitles[4];
	int sub_count;
	bool captionsInit;
	HSPRITE m_hVoiceIcon;
	int voiceIconWidth;
	int voiceIconHeight;
};

class CHudScoreboard : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, const char *team = NULL ); // returns the ypos where it finishes drawing
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScores( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );
	void RebuildTeams();
	void UpdateTeams();
	int BestTeam();

	int m_iNumTeams;

	int m_iLastKilledBy;
	int m_fLastKillTime;
	int m_iPlayerNum;
	bool m_iShowscoresHeld;
};

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	void ParseStatusString( int line_num );

	int MsgFunc_StatusText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_StatusValue( const char *pszName, int iSize, void *pbuf );

protected:
	enum
	{ 
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 2
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	// the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar

	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float *m_pflNameColors[MAX_STATUSBAR_LINES];
};

struct extra_player_info_t
{
	short frags;
	short deaths;
	short playerclass;
	short teamnumber;
	char teamname[MAX_TEAM_NAME];
};

struct team_info_t
{
	char name[MAX_TEAM_NAME];
	short frags;
	short deaths;
	short ping;
	short packetloss;
	bool ownteam;
	short players;
	bool already_drawn;
	bool scores_overriden;
	int teamnumber;
};

extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS + 1];	   // player info from the engine
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS + 1];   // additional player info sent directly to the client dll
extern team_info_t			g_TeamInfo[MAX_TEAMS + 1];
extern int					g_IsSpectator[MAX_PLAYERS + 1];

//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf );

private:
	int m_HUD_d_skull;  // sprite index of skull icon
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );

	void SelectMenuItem( int menu_item );

	int m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_SayText( const char *pszName, int iSize, void *pbuf );
	void SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
	friend class CHudSpectator;

private:
	struct cvar_s *	m_HUD_saytext;
	struct cvar_s *	m_HUD_saytext_time;
};

//
//-----------------------------------------------------
//
class CHudHealth : public CHudBase
{
public:
	virtual int Init( void );
	virtual int VidInit( void );
	virtual int Draw( float fTime );
	virtual void Reset( void );
	int MsgFunc_Health( const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_Damage( const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_Battery( const char *pszName,  int iSize, void *pbuf );
	int m_iHealth;
	int m_iMaxHealth;
	int m_HUD_dmg_bio;
	int m_HUD_cross;
	float m_fAttackFront, m_fAttackRear, m_fAttackLeft, m_fAttackRight;
	void GetHealthColor( int &r, int &g, int &b );
	void GetPainColor( int &r, int &g, int &b );
	float m_fFade;

private:
	HSPRITE m_hSprite;
	HSPRITE m_hDamage;

	DAMAGE_IMAGE m_dmg[NUM_DMG_TYPES];
	int m_bitsDamage;

	HSPRITE m_ArmorSprite1;
	HSPRITE m_ArmorSprite2;
	const wrect_t *m_prc1;
	const wrect_t *m_prc2;
	int m_iBat;
	int m_iMaxBat;
	float m_fArmorFade;
	int m_iHeight;		// width of the battery innards

	int DrawHealth();
	void DrawArmor(int startX);
	int DrawPain( float fTime );
	int DrawDamage( float fTime );
	void CalcDamageDirection( Vector vecFrom );
	void UpdateTiles( float fTime, long bits );
};

//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_Flashlight( const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_FlashBat( const char *pszName,  int iSize, void *pbuf );
	int RightmostCoordinate();

	int bottomCoordinate;
private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	HSPRITE m_hSprite3;
	HSPRITE m_hSprite4;
	HSPRITE m_hBeam;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	wrect_t *m_prcBeam;
	wrect_t *m_prc3;
	wrect_t *m_prc4;
	float m_flBat;	
	int m_iBat;	
	int m_fOn;
	float m_fFade;
	int m_iWidth;		// width of the battery innards
};

class CHudNightvision : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_Nightvision( const char *pszName, int iSize, void *pbuf );
	void DrawCSNVG(float flTime);
	void DrawOpforNVG(float flTime);
	dlight_t* MakeDynLight(float flTime, int r, int g, int b);
	void UpdateDynLight(dlight_t* dynLight, float radius, const Vector &origin);
	void RemoveCSdlight();
	void RemoveOFdlight();
	void UserCmd_NVGAdjustDown();
	void UserCmd_NVGAdjustUp();
#if FEATURE_CS_NIGHTVISION
	float CSNvgRadius();
#endif
#if FEATURE_OPFOR_NIGHTVISION
	float OpforNvgRadius();
#endif
	bool IsOn();
private:
	int m_fOn;
#if FEATURE_CS_NIGHTVISION
	dlight_t* m_pLightCS;
#endif
#if FEATURE_OPFOR_NIGHTVISION
	dlight_t* m_pLightOF;
#endif
#if FEATURE_OPFOR_NIGHTVISION
	HSPRITE m_hSprite;
	int m_iFrame, m_nFrameCount;
#endif
};
//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t	*pMessage;
	float	time;
	int x, y;
	int	totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	int Init( void );
	static char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size );
	static char *BufferedLocaliseTextString( const char *msg );
	const char *LookupString( const char *msg_name, int *msg_dest = NULL );
	int MsgFunc_TextMsg( const char *pszName, int iSize, void *pbuf );
};

//
//-----------------------------------------------------
//

class CHudMessage : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_HudText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_GameTitle( const char *pszName, int iSize, void *pbuf );

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName, float time );
	void MessageAdd(client_textmessage_t * newMessage );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );
	void SetColorParams( bool consoleFont );

private:
	client_textmessage_t		*m_pMessages[maxHUDMessages];
	float						m_startTime[maxHUDMessages];
	message_parms_t				m_parms;
	float						m_gameTitleTime;
	client_textmessage_t		*m_pGameTitle;

	int m_HUD_title_life;
	int m_HUD_title_half;

	int m_HUD_title_opposing;
	int m_HUD_title_force;
};

class CHudMonsterInfo : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw(float flTime) override;
	void Reset();
	int MsgFunc_MonsterInfo(const char *pszName, int iSize, void *pbuf);

private:
	cvar_t* m_pCvarShowMonsterInfo;

	char displayName[128];
	char healthDisplay[128];
	char armorDisplay[128];
	int health;
	int maxHealth;
	int armor;
	bool isMonster;
	bool isPlayer;
	bool isAlly;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH	24

struct inventory_t
{
	inventory_t(): itemName(), spr(0), count(0) {}
	std::string itemName;
	HSPRITE spr;
	wrect_t rc;
	unsigned char r, g, b, a;
	int position;
	int count;
};

class CHudStatusIcons : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_StatusIcon( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Inventory( const char *pszName, int iSize, void *pbuf );

	void EnableIcon(const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue, bool allowDuplicate = false);
	void DisableIcon(const char *pszIconName);
private:
	typedef struct
	{
		char szSpriteName[MAX_SPRITE_NAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES];
	inventory_t m_InventoryList[MAX_INVENTORY_ITEMS];
};

//
//-----------------------------------------------------
//
class CHudMoveMode: public CHudBase
{
	enum
	{
		MovementStand,
		MovementRun,
		MovementCrouch,
		MovementJump,
	};
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_MoveMode( const char *pszName,  int iSize, void *pbuf );

	int bottomCoordinate;
private:
	HSPRITE m_hSpriteStand;
	HSPRITE m_hSpriteRun;
	HSPRITE m_hSpriteCrouch;
	HSPRITE m_hSpriteJump;
	wrect_t *m_prcStand;
	wrect_t *m_prcRun;
	wrect_t *m_prcCrouch;
	wrect_t *m_prcJump;
	short m_movementState;
};

struct FogProperties
{
	short r,g,b;

	float startDist;
	float endDist;
	float finalEndDist;
	float fadeDuration;
	bool affectSkybox;

	float density;
	short type;
};

#define CLIENT_FEATURE_VALUE_LENGTH 127

struct ConfigurableBooleanValue
{
	ConfigurableBooleanValue();
	bool enabled_by_default;
	bool configurable;
};

struct ConfigurableBoundedValue
{
	ConfigurableBoundedValue();
	ConfigurableBoundedValue(int defValue, int minimumValue, int maximumValue, bool config = true);
	int defaultValue;
	int minValue;
	int maxValue;
	bool configurable;
};

struct ConfigurableIntegerValue
{
	ConfigurableIntegerValue();
	int defaultValue;
	bool configurable;
};

struct ConfigurableFloatValue
{
	ConfigurableFloatValue();
	float defaultValue;
	bool configurable;
};

struct FlashlightFeatures
{
	FlashlightFeatures();

	ConfigurableBooleanValue custom;
	int color;
	int distance;
	ConfigurableBoundedValue fade_distance;
	ConfigurableBoundedValue radius;
};

struct NVGFeatures
{
	ConfigurableBoundedValue radius;
	int light_color;
	int layer_color;
	int layer_alpha;
};

#define MAX_WALLPUFF_COUNT 4

struct ClientFeatures
{
	ClientFeatures();

	int hud_color;
	bool hud_color_configurable;
	ConfigurableBoundedValue hud_min_alpha;
	int hud_color_critical;

	ConfigurableFloatValue hud_scale;
	bool hud_draw_nosuit;
	int hud_color_nosuit;

	ConfigurableBooleanValue hud_armor_near_health;

	int hud_color_nvg;
	int hud_min_alpha_nvg;
	bool opfor_title;

	FlashlightFeatures flashlight;

	ConfigurableBooleanValue view_bob;
	ConfigurableFloatValue rollangle;
	ConfigurableBooleanValue weapon_wallpuff;
	ConfigurableBooleanValue weapon_sparks;
	ConfigurableBooleanValue muzzlelight;

	ConfigurableBooleanValue crosshair_colorable;
	ConfigurableBooleanValue movemode;

	ConfigurableIntegerValue nvgstyle;

	NVGFeatures nvg_cs;
	NVGFeatures nvg_opfor;

	char nvg_empty_sprite[MAX_SPRITE_NAME_LENGTH];
	char nvg_full_sprite[MAX_SPRITE_NAME_LENGTH];

	char wall_puffs[MAX_WALLPUFF_COUNT][64];

	bool fullbright_textures;
};

#define MAX_DLIGHTS 32

struct DlightExtraData
{
	int key;
	int entindex;
};

class KeyedDLightManager
{
public:
	void Reset();
	void AddDlight(dlight_t* dl, int entindex = 0);
	void RemoveDlight(int key);
	void Update();
	void SetPosition(int key, const Vector& pos);
private:
	struct DlightAndData
	{
		dlight_t* dl = nullptr;
		int entindex = 0;
	};
	void Reset(DlightAndData& data);
	std::array<DlightAndData, 32> _dlights;
};

struct RectangleRenderProperties
{
	Color3 frameColor{255, 140, 0};
	Color3 backgroundColor{0, 0, 0};
	int frameAlpha = 255;
	int backgroundAlpha = 160;
	bool frameBlend = false;
	bool backgroundBlend = true;
};

//
//-----------------------------------------------------
//
class CHud
{
private:
	HUDLIST						*m_pHudList;
	HSPRITE						m_hsprLogo;
	int							m_iLogo;
	client_sprite_t				*m_pSpriteList;
	int							m_iSpriteCount;
	int							m_iSpriteCountAllRes;
	float						m_flMouseSensitivity;
	int							m_iConcussionEffect;

	int m_cachedMinAlpha; // cache per frame
	int m_cachedHudColor;

	// this is solely to track whether we need to reset the crosshair
	bool m_colorableCrosshair;
	int m_lastCrosshairColor;

public:
	HSPRITE						m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	Vector	m_velocity;
	int		m_iKeyBits;
	int		m_iHideHUDDisplay;
	int		m_iFOV;
	int		m_Teamplay;
	int		m_iRes;
	int		m_iMaxRes;
	int		m_iHudNumbersYOffset;
	cvar_t  *m_pCvarDeveloper;
	cvar_t  *m_pCvarStealMouse;
	cvar_t	*m_pCvarDraw;
	cvar_t	*m_pCvarShowPos;
	cvar_t  *m_pAllowHD;
	cvar_t	*m_pCvarDrawMoveMode;
	cvar_t	*m_pCvarCrosshair;
	cvar_t	*m_pCvarCrosshairColorable;

	cvar_t	*m_pCvarMinAlpha;
	cvar_t	*m_pCvarHudRed;
	cvar_t	*m_pCvarHudGreen;
	cvar_t	*m_pCvarHudBlue;
	cvar_t	*m_pCvarArmorNearHealth;

	cvar_t	*m_pCvarObjectHint;

	cvar_t	*m_pCvarMOTDVGUI;
	cvar_t	*m_pCvarScoreboardVGUI;

	int m_iFontHeight;
	int DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b );
	int DrawHudString( int x, int y, int iMaxX, const char *szString, int r, int g, int b, int length = -1 );
	int DrawHudStringReverse( int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b );
	int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
	int GetNumWidth( int iNumber, int iFlags );
	void DrawDarkRectangle( int x, int y, int wide, int tall );
	void DrawDarkRectangle( int x, int y, int wide, int tall, const RectangleRenderProperties& rectProps );

	struct ConsoleText
	{
		static int DrawString( int xpos, int ypos, int iMaxX, const char *szString, int r, int g, int b, int length = -1 );
		static int DrawString( int xpos, int ypos, const char *szString, int r, int g, int b, int length = -1 );
		static int DrawNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
		static int DrawStringReverse( int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b, int length = -1 );
		static int LineWidth( const char *szString, int length = -1 );
		static int WidestCharacterWidth();
		static int LineHeight();
	};

	struct AdditiveText
	{
		static int DrawString( int xpos, int ypos, int iMaxX, const char *szString, int r, int g, int b, int length = -1 );
		static int DrawString( int xpos, int ypos, const char *szString, int r, int g, int b, int length = -1 );
		static int DrawNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
		static int DrawStringReverse( int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b, int length = -1 );
		static int LineWidth( const char *szString, int length = -1 );
		static int WidestCharacterWidth();
		static int LineHeight();
	};

	typedef ConsoleText UtfText;

	void HUDColorCmd();
	int HUDColor();
	int HUDColorCritical();
	int MinHUDAlpha() const;
	void RecacheValues();
	int GetCrosshairColor();
	void ResetCrosshair();
	ClientFeatures clientFeatures;

	bool HasSuit() const
	{
		return (m_iItemBits & PLAYER_ITEM_SUIT) != 0;
	}
	bool HasFlashlight() const
	{
		return (m_iItemBits & PLAYER_ITEM_FLASHLIGHT) != 0;
	}
	bool HasNVG() const
	{
		return (m_iItemBits & PLAYER_ITEM_NIGHTVISION) != 0;
	}
	bool HasWeapon(int id) const
	{
		return (m_iWeaponBits & (1ULL << id)) != 0;
	}
	bool HasAnyWeapons() const
	{
		return m_iWeaponBits != 0;
	}
	bool ViewBobEnabled();
	int CalcMinHUDAlpha();
	bool DrawArmorNearHealth();
	bool WeaponWallpuffEnabled();
	bool WeaponSparksEnabled();
	bool MuzzleLightEnabled();
	bool CustomFlashlightEnabled();
	float FlashlightRadius();
	float FlashlightDistance();
	float FlashlightFadeDistance();
	color24 FlashlightColor();
	int NVGStyle();
	bool MoveModeEnabled();
	inline bool ShouldUseZoomedCrosshair() { return m_iFOV < 90; }
	bool CrosshairColorable();
private:
	void ParseClientFeatures();
	static bool ClientFeatureEnabled(cvar_t *cVariable, bool defaultValue);

	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	struct cvar_s *default_fov;
public:
	HSPRITE GetSprite( int index ) 
	{
		return ( index < 0 ) ? 0 : m_rghSprites[index];
	}

	wrect_t& GetSpriteRect( int index )
	{
		return m_rgrcRects[index];
	}

	wrect_t* GetSpriteRectPointer( int index )
	{
		if (index < 0 || index >= m_iSpriteCount)
			return NULL;
		return &m_rgrcRects[index];
	}

	inline bool UsingHighResSprites( void )
	{
		// a1ba: only HL25 have higher resolution HUD spritesheets
		// and only accept HUD style changes if user has allowed HD sprites
		return m_iMaxRes > 640 && m_pAllowHD->value;
	}
	
	int GetSpriteIndex( const char *SpriteName );	// gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo		m_Ammo;
	CHudHealth		m_Health;
	CHudSpectator		m_Spectator;
	CHudGeiger		m_Geiger;
	CHudTrain		m_Train;
	CHudFlashlight	m_Flash;
	CHudMoveMode	m_MoveMode;
	CHudMessage		m_Message;
	CHudStatusBar   m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText		m_SayText;
	CHudMenu		m_Menu;
	CHudAmmoSecondary	m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudScoreboard	m_Scoreboard;
	CHudMOTD	m_MOTD;
	CHudErrorCollection	m_ErrorCollection;
	CHudNightvision m_Nightvision;
	CHudCaption		m_Caption;
	CHudMonsterInfo		m_MonsterInfo;

	void ParseModConfigs();
	bool IsDeveloperModeOn();
	void Init( void );
	void VidInit( void );
	void Think(void);
	int Redraw( float flTime, int intermission );
	int UpdateClientData( client_data_t *cdata, float time );

	CHud() : m_pHudList(NULL), m_iSpriteCount(0) {}
	~CHud();			// destructor, frees allocated memory

	static HudSpriteRenderer& Renderer();

	// user messages
	int _cdecl MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Logo( const char *pszName,  int iSize, void *pbuf );
	int _cdecl MsgFunc_ResetHUD( const char *pszName,  int iSize, void *pbuf );
	void _cdecl MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_SetFOV( const char *pszName,  int iSize, void *pbuf );
	int  _cdecl MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf );

	int _cdecl MsgFunc_Weapons( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Items(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_SetFog( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_KeyedDLight( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_ObjectHint( const char *pszName, int iSize, void *pbuf );

	// Screen information
	SCREENINFO	m_scrinfo;

	std::uint64_t m_iWeaponBits;
	int m_iItemBits;
	bool m_fPlayerDead;
	int m_iIntermission;

	// sprite indexes
	int m_HUD_number_0;

	void AddHudElem( CHudBase *p );

	float GetSensitivity();

	void GetAllPlayersInfo( void );

	bool m_iHardwareMode;
	FogProperties fog;

	void LoadWallPuffSprites();
	int wallPuffCount;
	model_t* wallPuffs[MAX_WALLPUFF_COUNT];

	bool m_bFlashlight;

	InventoryHudSpec m_inventorySpec;
	MessageStrings m_messageStrings;
	ObjectHintManager objectHintManager;
	KeyedDLightManager keyedDlightManager;

	HudSpriteRenderer hudRenderer;
	bool hasHudScaleInEngine;

	static bool ShouldUseConsoleFont();

	bool CanDrawStatusIcons();
	int TopRightInventoryCoordinate();
	bool UseVguiMOTD();
	bool UseVguiScoreBoard();
};

extern CHud gHUD;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;
#endif
