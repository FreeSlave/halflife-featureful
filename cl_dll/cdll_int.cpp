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
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "windows_lean.h"
#include "gl_dynamic.h"

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "parsemsg.h"

#include "r_efx.h"
#include "r_studioint.h"
#include "event_api.h"
#include "com_model.h"
#include "studio.h"
#include "pmtrace.h"

#include "cl_msg.h"
#include "tex_materials.h"

#include "vcs_info.h"

#if USE_VGUI
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#endif

#include "cl_fx.h"

#include "IParticleMan_Active.h"
#include "CBaseParticle.h"

#include "environment.h"

#include "cmdkeys.h"
#include "keydefs.h"
#include "logger.h"
#include "parsetext.h"

IParticleMan *g_pParticleMan = NULL;

void CL_LoadParticleMan();
void CL_UnloadParticleMan();

#if GOLDSOURCE_SUPPORT && (XASH_WIN32 || XASH_LINUX || XASH_APPLE) && XASH_X86
#define USE_FAKE_VGUI	!USE_VGUI
#if USE_FAKE_VGUI
#include "VGUI_Panel.h"
#include "VGUI_App.h"
#endif
#endif

#include "pm_shared.h"

#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

#include "hud_renderer.h"

#if OPENGL_AVAILABLE
GLAPI_glFogi GL_glFogi = nullptr;

GLAPI_glPixelStorei GL_glPixelStorei = nullptr;
GLAPI_glGenTextures GL_glGenTextures = nullptr;
GLAPI_glBindTexture GL_glBindTexture = nullptr;
GLAPI_glTexImage2D GL_glTexImage2D = nullptr;
GLAPI_glTexParameteri GL_glTexParameteri = nullptr;

#ifdef _WIN32
HMODULE libOpenGL = NULL;

HMODULE LoadOpenGL()
{
	return GetModuleHandleA("opengl32.dll");
}

void UnloadOpenGL()
{
	//  Don't actually unload library on windows as it was loaded via GetModuleHandle
	libOpenGL = NULL;

	GL_glFogi = NULL;
}

FARPROC LoadLibFunc(HMODULE lib, const char *name)
{
	return GetProcAddress(lib, name);
}
#else
#include <dlfcn.h>
void* libOpenGL = NULL;

void* LoadOpenGL()
{
#ifdef __APPLE__
	return dlopen("libGL.dylib", RTLD_LAZY);
#else
	return dlopen("libGL.so.1", RTLD_LAZY);
#endif
}

void UnloadOpenGL()
{
	if (libOpenGL)
	{
		dlclose(libOpenGL);
		libOpenGL = NULL;
	}
	GL_glFogi = NULL;
}

void* LoadLibFunc(void* lib, const char *name)
{
	return dlsym(lib, name);
}
#endif

#endif

cl_enginefunc_t gEngfuncs;
CHud gHUD;
CmdKeys g_DefaultCmdKeys;
#if USE_VGUI
TeamFortressViewport *gViewPort = NULL;
#endif
mobile_engfuncs_t *gMobileEngfuncs = NULL;

void InitInput();
void EV_HookEvents();
void IN_Commands();

typedef void (*xcommand_t)(void);
typedef struct cmd_function_s
{
	struct cmd_function_s* next;
	const char* name;
	xcommand_t function;
	int flags;
} cmd_function_t;

xcommand_t originalSaveFunction = nullptr;
xcommand_t originalAutoSaveFunction = nullptr;
xcommand_t originalSetVideoModeFunction = nullptr;
bool manualSaveIsDisabled = false;

static void CallSaveCommand()
{
	if (manualSaveIsDisabled)
	{
		gEngfuncs.Con_DPrintf("Refusing to save: manual saves are disabled\n");
		gHUD.m_Message.MessageAdd("SAVE_DISABLED", gHUD.m_flTime, true);
	}
	else
	{
		if (originalSaveFunction)
			originalSaveFunction();
	}
}

static void CallAutoSaveCommand()
{
	if (originalAutoSaveFunction)
	{
		gHUD.m_Message.MessageAdd("AUTOSAVE", gHUD.m_flTime, true);
		originalAutoSaveFunction();
	}
}

static void SetVideoModeCommand()
{
	manualSaveIsDisabled = false;
	if (originalSetVideoModeFunction)
		originalSetVideoModeFunction();
}

static cmd_function_t* GetClientCommand(const char* name)
{
	auto pCmd = reinterpret_cast<cmd_function_t*>(gEngfuncs.pfnGetFirstCmdFunctionHandle());
	while(pCmd)
	{
		if (stricmp(pCmd->name, name) == 0)
		{
			return pCmd;
		}
		pCmd = pCmd->next;
	}
	return nullptr;
}

void HookClientCommands()
{
	auto hookCommand = [](const char* name, xcommand_t replacementFunc){
		cmd_function_t* cmd = GetClientCommand(name);
		if (cmd)
		{
			auto ret = cmd->function;
			cmd->function = replacementFunc;
			return ret;
		}
		return xcommand_t{};
	};

	originalSaveFunction = hookCommand("save", &CallSaveCommand);
	originalAutoSaveFunction = hookCommand("autosave", &CallAutoSaveCommand);
	originalSetVideoModeFunction = hookCommand("_setvideomode", &SetVideoModeCommand);
}

int __MsgFunc_UseSound( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int soundType = READ_BYTE();

	if (soundType)
		PlaySound( "common/wpn_select.wav", 0.4f );
	else
		PlaySound( "common/wpn_denyselect.wav", 0.4f );

	return 1;
}

int __MsgFunc_SaveDisable( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	const bool prevManualSaveIsDisabled = manualSaveIsDisabled;
	manualSaveIsDisabled = READ_BYTE() == 0 ? false : true;

	if (prevManualSaveIsDisabled != manualSaveIsDisabled)
	{
		if (manualSaveIsDisabled)
		{
			gEngfuncs.Con_DPrintf("Saves have been disabled\n");
		}
		else
		{
			gEngfuncs.Con_DPrintf("Saves have been enabled\n");
		}
	}
	return 1;
}

extern bool g_checkingBindings;
extern std::set<int> g_boundKeynums;
bool g_bindingsChecked = false;

static CmdKeys::CommandToKeysMap ReadConfigBindings(const char* pfile, int fileSize)
{
	CmdKeys cmdKeys;

	int i = 0;
	while (i < fileSize)
	{
		if (IsSpaceCharacter(pfile[i]))
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
			int tokenStart = i;
			ConsumeNonSpaceCharacters(pfile, i, fileSize);
			int tokenLength = i - tokenStart;

			if (tokenLength > 0 && strncmp(pfile + tokenStart, "bind", tokenLength) == 0)
			{
				SkipSpacesAndTabs(pfile, i, fileSize);
				int keyStart, keyEnd;
				if (ConsumePossiblyQuotedString(pfile, i, fileSize, keyStart, keyEnd))
				{
					SkipSpacesAndTabs(pfile, i, fileSize);
					int commandStart, commandEnd;
					if (ConsumePossiblyQuotedString(pfile, i, fileSize, commandStart, commandEnd))
					{
						std::string key{pfile + keyStart, pfile + keyEnd};
						std::string command{pfile + commandStart, pfile + commandEnd};

						cmdKeys.AddDefaultKeyNumForCommand(command, key.c_str());
					}
					else
					{
						ConsumeLine(pfile, i, fileSize);
					}
				}
				else
				{
					ConsumeLine(pfile, i, fileSize);
				}
			}
			else
			{
				ConsumeLine(pfile, i, fileSize);
			}
		}
	}

	return cmdKeys.MoveMap();
}

static CmdKeys::CommandToKeysMap ReadConfigBindings()
{
	int fileSize = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile("config.cfg", 5, &fileSize);
	if (!pfile)
		return CmdKeys::CommandToKeysMap();

	CmdKeys::CommandToKeysMap result = ReadConfigBindings(pfile, fileSize);
	gEngfuncs.COM_FreeFile(pfile);
	return result;
}

static void ApplyDefaultKeyBindings()
{
	std::set<int> keynumsToCheck;
	CmdKeys::CommandToKeysMap commandsMissingKeys;

	if (!g_DefaultCmdKeys.empty())
	{
		CmdKeys::CommandToKeysMap configBindings = ReadConfigBindings();

		for (const auto& ck : g_DefaultCmdKeys)
		{
			//gEngfuncs.Con_DPrintf("Checking %s\n", ck.first.c_str());

			auto it = configBindings.find(ck.first);
			if (it == configBindings.end())
			{
				gEngfuncs.Con_DPrintf("The command %s is not bound to anything. Going to search a free keynum\n", ck.first.c_str());
				commandsMissingKeys.insert(ck);

				for (int k : ck.second.keynums)
				{
					if (k > 0)
						keynumsToCheck.insert(k);
					else
						break;
				}
			}
		}
	}

	if (!commandsMissingKeys.empty())
	{
		g_checkingBindings = true;
		for (int k : keynumsToCheck)
		{
			gEngfuncs.Key_Event(k, 1);
			gEngfuncs.Key_Event(k, 0);
		}
		g_checkingBindings = false;

		for (const auto& ck : commandsMissingKeys)
		{
			for (int k : ck.second.keynums)
			{
				if (k <= 0)
					break;

				if (g_boundKeynums.find(k) == g_boundKeynums.end())
				{
					char commandBuf[128] = {0};

					const char* keyString = SpecialKeynumToString(k);
					if (keyString)
					{
						safe_snprintf(commandBuf, sizeof(commandBuf), "bind \"%s\" \"%s\"\n", keyString, ck.first.c_str());
					}
					else if (isprint(k))
					{
						safe_snprintf(commandBuf, sizeof(commandBuf), "bind \"%c\" \"%s\"\n", k, ck.first.c_str());
					}

					if (*commandBuf)
					{
						gEngfuncs.Con_DPrintf("Running %s", commandBuf);
						gEngfuncs.pfnClientCmd(commandBuf);
						break;
					}
				}
			}
		}
	}

	// No need for these anymore
	g_boundKeynums.clear();
	g_DefaultCmdKeys.Clear();
}

/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
extern "C" 
{
int		DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int		DLLEXPORT HUD_VidInit();
void	DLLEXPORT HUD_Init();
int		DLLEXPORT HUD_Redraw( float flTime, int intermission );
int		DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void	DLLEXPORT HUD_Reset ();
void	DLLEXPORT HUD_Shutdown();
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int		DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int		DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );
int DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *gpMobileEngfuncs );
}

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch( hullnumber )
	{
	case 0:				// Normal player
		Vector( -16, -16, -36 ).CopyToArray(mins);
		Vector( 16, 16, 36 ).CopyToArray(maxs);
		iret = 1;
		break;
	case 1:				// Crouched player
		Vector( -16, -16, -18 ).CopyToArray(mins);
		Vector( 16, 16, 18 ).CopyToArray(maxs);
		iret = 1;
		break;
	case 2:				// Point based hull
		Vector( 0, 0, 0 ).CopyToArray(mins);
		Vector( 0, 0, 0 ).CopyToArray(maxs);
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	// int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	PM_Init( ppmove );
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	return PM_FindTextureType( name );
}

void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	PM_Move( ppmove, server );
}

int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	gEngfuncs = *pEnginefuncs;

	if( iVersion != CLDLL_INTERFACE_VERSION )
		return 0;

	// for now filterstuffcmd is last in the engine interface
	memcpy( &gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t) - sizeof( void * ) );

	if( gEngfuncs.pfnGetCvarPointer( "cl_filterstuffcmd" ) == 0 )
	{
		gEngfuncs.pfnFilteredClientCmd = gEngfuncs.pfnClientCmd;
	}
	else
	{
		gEngfuncs.pfnFilteredClientCmd = pEnginefuncs->pfnFilteredClientCmd;
	}

	EV_HookEvents();

	CL_LoadParticleMan();

	gEngfuncs.pfnRegisterVariable( "cl_game_build_commit", g_VCSInfo_Commit, 0 );
	gEngfuncs.pfnRegisterVariable( "cl_game_build_branch", g_VCSInfo_Branch, 0 );

	return 1;
}

/*
=================
HUD_GetRect

VGui stub
=================
*/
int *HUD_GetRect()
{
	static int extent[4];

	extent[0] = gEngfuncs.GetWindowCenterX() - ScreenWidth / 2;
	extent[1] = gEngfuncs.GetWindowCenterY() - ScreenHeight / 2;
	extent[2] = gEngfuncs.GetWindowCenterX() + ScreenWidth / 2;
	extent[3] = gEngfuncs.GetWindowCenterY() + ScreenHeight / 2;

	return extent;
}

#if USE_FAKE_VGUI
class TeamFortressViewport : public vgui::Panel
{
public:
	TeamFortressViewport(int x,int y,int wide,int tall);
	void Initialize();

	void paintBackground() override;
	void *operator new( size_t stAllocateBlock );
};

static TeamFortressViewport* gViewPort = NULL;

TeamFortressViewport::TeamFortressViewport(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	gViewPort = this;
	Initialize();
}

void TeamFortressViewport::Initialize()
{
	//vgui::App::getInstance()->setCursorOveride( vgui::App::getInstance()->getScheme()->getCursor(vgui::Scheme::scu_none) );
}

void TeamFortressViewport::paintBackground()
{
//	int wide, tall;
//	getParent()->getSize( wide, tall );
//	setSize( wide, tall );
	int extents[4];
	getParent()->getAbsExtents(extents[0],extents[1],extents[2],extents[3]);
	gEngfuncs.VGui_ViewportPaintBackground(extents);
}

void *TeamFortressViewport::operator new( size_t stAllocateBlock )
{
	void *mem = ::operator new( stAllocateBlock );
	memset( mem, 0, stAllocateBlock );
	return mem;
}
#endif

/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/
extern void HUD_ResetClientWeaponData();

int DLLEXPORT HUD_VidInit()
{
	if (!g_bindingsChecked && IsAnyXash())
	{
		g_bindingsChecked = true;
		ApplyDefaultKeyBindings();
	}

	manualSaveIsDisabled = false;
	gHUD.m_iHardwareMode = IEngineStudio.IsHardware() != 0;
	HUD_ResetClientWeaponData();
	gHUD.VidInit();
	LoadDefaultSprites();
#if USE_FAKE_VGUI
	vgui::Panel* root=(vgui::Panel*)gEngfuncs.VGui_GetPanel();
	if (root) {
		gEngfuncs.Con_Printf( "Root VGUI panel exists\n" );
		root->setBgColor(128,128,0,0);

		if (gViewPort != NULL)
		{
			gViewPort->Initialize();
		}
		else
		{
			gViewPort = new TeamFortressViewport(0,0,root->getWide(),root->getTall());
			gViewPort->setParent(root);
		}
	} else {
		gEngfuncs.Con_Printf( "Root VGUI panel does not exist\n" );
	}
#elif USE_VGUI
	VGui_Startup();
#endif

#if OPENGL_AVAILABLE
	gEngfuncs.Con_DPrintf("Hardware Mode: %d\n", gHUD.m_iHardwareMode);
	if (gHUD.m_iHardwareMode == 1)
	{
		if (!GL_glFogi)
		{
			libOpenGL = LoadOpenGL();
#ifdef _WIN32
			if (libOpenGL)
#else
			if (!libOpenGL)
				gEngfuncs.Con_DPrintf("Failed to load OpenGL: %s. Trying to use OpenGL from engine anyway\n", dlerror());
#endif
			{
				GL_glFogi = (GLAPI_glFogi)LoadLibFunc(libOpenGL, "glFogi");

				GL_glPixelStorei = (GLAPI_glPixelStorei)LoadLibFunc(libOpenGL, "glPixelStorei");
				GL_glGenTextures = (GLAPI_glGenTextures)LoadLibFunc(libOpenGL, "glGenTextures");
				GL_glBindTexture = (GLAPI_glBindTexture)LoadLibFunc(libOpenGL, "glBindTexture");
				GL_glTexImage2D = (GLAPI_glTexImage2D)LoadLibFunc(libOpenGL, "glTexImage2D");
				GL_glTexParameteri = (GLAPI_glTexParameteri)LoadLibFunc(libOpenGL, "glTexParameteri");
			}

			if (GL_glFogi)
			{
				gEngfuncs.Con_DPrintf("OpenGL functions loaded\n");
			}
			else
			{
#ifdef _WIN32
				gEngfuncs.Con_Printf("Failed to load OpenGL functions!\n");
#else
				gEngfuncs.Con_Printf("Failed to load OpenGL functions! %s\n", dlerror());
#endif
			}
		}
	}
#endif

	if (g_pParticleMan)
	{
		g_pParticleMan->ResetParticles();
		g_Environment.Reset();
	}

	return 1;
}

bool ParseDefaultShortcuts(CmdKeys& cmdKeys, const char* pfile, int fileSize, const char* fileName)
{
	int i = 0;
	while (i < fileSize)
	{
		if (IsSpaceCharacter(pfile[i]))
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
			int commandNameStart, commandNameEnd;
			if (!ConsumePossiblyQuotedString(pfile, i, fileSize, commandNameStart, commandNameEnd))
			{
				LOG_ERROR("%s: incomplete quoting or empty quoted string\n", fileName);
				ConsumeLine(pfile, i, fileSize);
				continue;
			}
			const int commandNameLength = commandNameEnd - commandNameStart;
			if (commandNameLength == 0)
			{
				ConsumeLine(pfile, i, fileSize);
				continue;
			}

			SkipSpacesAndTabs(pfile, i, fileSize);

			int keyNameStart, keyNameEnd;
			if (!ConsumePossiblyQuotedString(pfile, i, fileSize, keyNameStart, keyNameEnd))
			{
				LOG_ERROR("%s: incomplete quoting or empty quoted string\n", fileName);
				ConsumeLine(pfile, i, fileSize);
				continue;
			}
			const int keyNameLength = keyNameEnd - keyNameStart;
			if (keyNameLength == 0)
			{
				ConsumeLine(pfile, i, fileSize);
				continue;
			}

			SkipSpacesAndTabs(pfile, i, fileSize);
			if (i == fileSize || (pfile[i] != '\n' && pfile[i] != '\r' && pfile[i] != '/'))
			{
				LOG_ERROR("%s: wrong format: more than two strings on the same line\n", fileName);
				ConsumeLine(pfile, i, fileSize);
				continue;
			}

			std::string commandName(pfile + commandNameStart, pfile + commandNameEnd);
			std::string keyName(pfile + keyNameStart, pfile + keyNameEnd);
			int result = cmdKeys.AddDefaultKeyNumForCommand(commandName, keyName.c_str());
			switch(result)
			{
			case COMMANDKEY_SUCCESS:
				break;
			case COMMANDKEY_UNKNOWNKEY:
				LOG_ERROR("%s: unknown key name %s\n", fileName, keyName.c_str());
				break;
			case COMMANDKEY_OUTOFBOUNDS:
				LOG_ERROR("%s: keynum is out of bounds for key name %s\n", fileName, keyName.c_str());
				break;
			case COMMANDKEY_TOOMANYKEYS:
				LOG_ERROR("%s: too many keys for the command %s\n", fileName, commandName.c_str());
				break;
			}
		}
	}

	return true;
}

bool ParseDefaultShortcuts(CmdKeys& cmdKeys, const char* fileName)
{
	int fileSize = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile(fileName, 5, &fileSize);
	if (!pfile)
		return false;

	bool result = ParseDefaultShortcuts(cmdKeys, pfile, fileSize, fileName);
	gEngfuncs.COM_FreeFile(pfile);
	return result;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

void DLLEXPORT HUD_Init()
{
	HookClientCommands();
	InitInput();
	ParseDefaultShortcuts(g_DefaultCmdKeys, "default_keys.cfg");
	gHUD.Init();
#if USE_VGUI
	Scheme_Init();
#endif

	HOOK_MESSAGE( UseSound );
	HOOK_MESSAGE( SaveDisable );

	HookFXMessages();
}

/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/
extern void DrawFlashlight();

int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	if (gHUD.m_bFlashlight)
		DrawFlashlight();

	gHUD.Redraw( time, intermission );

	return 1;
}

/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/

int DLLEXPORT HUD_UpdateClientData( client_data_t *pcldata, float flTime )
{
	IN_Commands();

	return gHUD.UpdateClientData( pcldata, flTime );
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/

void DLLEXPORT HUD_Reset()
{
	gHUD.VidInit();
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void DLLEXPORT HUD_Frame( double time )
{
	if (!g_bindingsChecked && !IsAnyXash())
	{
		g_bindingsChecked = true;
		ApplyDefaultKeyBindings();
	}

#if USE_VGUI
	GetClientVoiceMgr()->Frame(time);
#elif USE_FAKE_VGUI
	if (!gViewPort)
		gEngfuncs.VGui_ViewportPaintBackground(HUD_GetRect());
#else
	gEngfuncs.VGui_ViewportPaintBackground(HUD_GetRect());
#endif

	CHud::Renderer().HUD_Frame(time);
}

/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void DLLEXPORT HUD_VoiceStatus( int entindex, qboolean bTalking )
{
#if USE_VGUI
	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, bTalking);
#endif
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/

void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
	 gHUD.m_Spectator.DirectorMessage( iSize, pbuf );
}

void TestParticlesCmd()
{
	static model_t* texture = 0;

	if ( g_pParticleMan )
	{
		const float clTime = gEngfuncs.GetClientTime();

		if (texture == 0)
		{
			texture = (model_t*)gEngfuncs.GetSpritePointer(SPR_Load("sprites/steam1.spr"));
		}

		if (!texture)
			return;

		cl_entity_t* player = gEngfuncs.GetLocalPlayer();
		Vector origin = player->origin;
		Vector forward;
		AngleVectors(player->angles, forward, NULL, NULL);

		for (int i = 0; i < 10; ++i)
		{
			Vector shift = forward * 64.0f + forward * 8.0f * i + Vector( 0.0f, 0.0f, i * 8.0f );

			CBaseParticle *particle = g_pParticleMan->CreateParticle(origin + shift, Vector(0.0f, 0.0f, 0.0f), texture, 32.0f, 255.0f, "particle");

			particle->SetLightFlag(LIGHT_NONE);
			particle->SetCullFlag(CULL_PVS);
			particle->SetRenderFlag(RENDER_FACEPLAYER);
			particle->SetCollisionFlags(TRI_COLLIDEWORLD);
			particle->m_iRendermode = kRenderTransAlpha;
			particle->m_vColor = Vector(255, 255, 255);
			particle->m_iFramerate = 10;
			particle->m_iNumFrames = texture->numframes;
			particle->m_flGravity = 0.01f;
			particle->m_vVelocity = shift.Normalize() * 2;

			particle->m_flDieTime = clTime + 5 + i;
		}
	}
}

void CL_UnloadParticleMan()
{
	if (g_pParticleMan)
	{
		delete g_pParticleMan;
		g_pParticleMan = NULL;
	}
}

void CL_LoadParticleMan()
{
	//Now implemented in the client library.
	g_pParticleMan = new IParticleMan_Active();

	if (g_pParticleMan)
	{
		g_pParticleMan->SetUp(&gEngfuncs);

		gEngfuncs.pfnAddCommand("test_particles", &TestParticlesCmd);
	}
}

int DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *gpMobileEngfuncs )
{
	if( gpMobileEngfuncs->version != MOBILITY_API_VERSION )
		return 1;
	gMobileEngfuncs = gpMobileEngfuncs;
	return 0;
}

bool HUD_MessageBox( const char *msg )
{
	gEngfuncs.Con_Printf( msg ); // just in case

	if( IsXashFWGS() )
	{
		gMobileEngfuncs->pfnSys_Warn( msg );
		return true;
	}

	// TODO: Load SDL2 and call ShowSimpleMessageBox

	return false;
}

void DLLEXPORT HUD_Shutdown()
{
	ShutdownInput();
#if OPENGL_AVAILABLE
	UnloadOpenGL();
#endif
	g_Environment.Clear();
	auto miniMem = CMiniMem::Instance();
	if (miniMem)
	{
		miniMem->Reset();
		miniMem->Shutdown();
	}
	CL_UnloadParticleMan();
}

static bool isSomeXash = false;

extern "C" int DLLEXPORT HUD_GetRenderInterface( int version, void *renderfuncs, void *callback )
{
	isSomeXash = true;
	return 0;
}

bool IsAnyXash()
{
	return isSomeXash;
}

bool IsXashFWGS()
{
	return gMobileEngfuncs != NULL;
}

bool LibrarySideFullbrightSupportIsOn()
{
	return gHUD.clientFeatures.fullbright_textures;
}
