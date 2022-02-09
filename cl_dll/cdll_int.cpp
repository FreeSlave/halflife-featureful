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

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "parsemsg.h"
#include "gl_dynamic.h"

#include "r_efx.h"
#include "r_studioint.h"
#include "event_api.h"
#include "com_model.h"
#include "studio.h"
#include "pmtrace.h"

#if USE_VGUI
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#endif

#if GOLDSOURCE_SUPPORT && (_WIN32 || __linux__ || __APPLE__) && (__i386 || _M_IX86)
#define USE_FAKE_VGUI	!USE_VGUI
#if USE_FAKE_VGUI
#include "VGUI_Panel.h"
#include "VGUI_App.h"
#endif
#endif

extern "C"
{
#include "pm_shared.h"
}

#include <string.h>
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

#include "hud_sprite.h"

#ifdef CLDLL_FOG
GLAPI_glEnable GL_glEnable = NULL;
GLAPI_glDisable GL_glDisable = NULL;
GLAPI_glFogi GL_glFogi = NULL;
GLAPI_glFogf GL_glFogf = NULL;
GLAPI_glFogfv GL_glFogfv = NULL;
GLAPI_glHint GL_glHint = NULL;
GLAPI_glGetIntegerv GL_glGetIntegerv = NULL;

#ifdef _WIN32
HMODULE libOpenGL = NULL;

HMODULE LoadOpenGL()
{
	return GetModuleHandle("opengl32.dll");
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
#if USE_VGUI
TeamFortressViewport *gViewPort = NULL;
#endif
mobile_engfuncs_t *gMobileEngfuncs = NULL;

extern "C" int g_bhopcap;
void InitInput( void );
void EV_HookEvents( void );
void IN_Commands( void );

int __MsgFunc_Bhopcap( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	g_bhopcap = READ_BYTE();

	return 1;
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

void GibHitCallback( TEMPENTITY* ent, pmtrace_t* pmtrace )
{
	static const char* redBloodDecals[] = {"{blood1", "{blood2", "{blood3", "{blood4", "{blood5", "{blood6"};
	static const char* yellowBloodDecals[] = {"{yblood1", "{yblood2", "{yblood3", "{yblood4", "{yblood5", "{yblood6"};

	const char* decalName = NULL;
	if (ent->entity.curstate.iuser1 == 1)
	{
		decalName = redBloodDecals[gEngfuncs.pfnRandomLong(0, 5)];
	}
	else if (ent->entity.curstate.iuser1 > 1)
	{
		decalName = yellowBloodDecals[gEngfuncs.pfnRandomLong(0, 5)];
	}

	if (ent->entity.curstate.onground)
	{
		ent->entity.baseline.origin = ent->entity.baseline.origin * 0.9;
		ent->entity.curstate.angles.x = 0;
		ent->entity.curstate.angles.z = 0;
		ent->entity.baseline.angles.x = 0;
		ent->entity.baseline.angles.z = 0;
	}
	else
	{
		ent->entity.curstate.origin = ent->entity.curstate.origin + Vector(0, 0, 8);

		if (ent->entity.curstate.iuser1 > 0 && ent->entity.curstate.iuser2 > 0 && decalName != NULL)
		{
			int decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( (char*)decalName );
			int textureIndex = gEngfuncs.pEfxAPI->Draw_DecalIndex( decalIndex );
			int traceEntIndex = gEngfuncs.pEventAPI->EV_IndexFromTrace( pmtrace );
			gEngfuncs.pEfxAPI->R_DecalShoot(textureIndex, traceEntIndex, 0, pmtrace->endpos, 0 );
			ent->entity.curstate.iuser2--;
		}
	}
}

int __MsgFunc_RandomGibs( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector absmin;
	Vector size;
	Vector direction;

	absmin[0] = READ_COORD();
	absmin[1] = READ_COORD();
	absmin[2] = READ_COORD();

	size[0] = READ_COORD();
	size[1] = READ_COORD();
	size[2] = READ_COORD();

	direction[0] = READ_COORD();
	direction[1] = READ_COORD();
	direction[2] = READ_COORD();

	float randomization = READ_BYTE() / 100.0f;
	int modelIndex = READ_SHORT();
	int gibCount = READ_BYTE();
	int lifeTime = READ_BYTE();
	int bloodType = READ_BYTE();
	int gibBodiesNum = READ_BYTE();
	int startGibIndex = READ_BYTE();

	float velocityMultiplier = READ_BYTE() / 10.0f;

	struct model_s* model = IEngineStudio.GetModelByIndex(modelIndex);

	if (gibBodiesNum == 0)
	{
		studiohdr_t* pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(model);
		if (pstudiohdr)
		{
			mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)( (byte *)pstudiohdr + pstudiohdr->bodypartindex );

			gibBodiesNum = 1;
			for (int j=0; j<pstudiohdr->numbodyparts; ++j)
			{
				gibBodiesNum = gibBodiesNum * pbodypart[j].nummodels;
			}
		}
	}

	if (gibBodiesNum == 0)
		gibBodiesNum = startGibIndex + 1;
	startGibIndex = startGibIndex > gibBodiesNum - 1 ? gibBodiesNum - 1 : startGibIndex;

	for (int i=0; i<gibCount; ++i)
	{
		Vector gibPos;
		gibPos.x = absmin.x + size.x * gEngfuncs.pfnRandomFloat(0, 1);
		gibPos.y = absmin.y + size.y * gEngfuncs.pfnRandomFloat(0, 1);
		gibPos.z = absmin.z + size.z * gEngfuncs.pfnRandomFloat(0, 1) + 1;

		Vector gibVelocity = direction;
		gibVelocity.x += gEngfuncs.pfnRandomFloat(-randomization, randomization);
		gibVelocity.y += gEngfuncs.pfnRandomFloat(-randomization, randomization);
		gibVelocity.z += gEngfuncs.pfnRandomFloat(-randomization, randomization);

		gibVelocity = gibVelocity * gEngfuncs.pfnRandomFloat( 300, 400 ) * velocityMultiplier;

		if (gibVelocity.Length() > 1500)
		{
			gibVelocity = gibVelocity.Normalize() * 1500;
		}

		TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(gibPos, model);
		if (!pTemp)
			break;

		pTemp->entity.curstate.body = gEngfuncs.pfnRandomLong(startGibIndex, gibBodiesNum - 1);
		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_GRAVITY | FTENT_ROTATE | FTENT_PERSIST;

		pTemp->entity.curstate.iuser1 = bloodType;
		pTemp->entity.curstate.iuser2 = 5;
		pTemp->entity.curstate.solid = SOLID_SLIDEBOX;
		pTemp->entity.curstate.movetype = MOVETYPE_BOUNCE;
		pTemp->entity.curstate.friction = 0.55;
		pTemp->entity.curstate.rendermode = kRenderNormal;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 255;
		pTemp->hitcallback = &GibHitCallback;

		pTemp->entity.baseline.angles.x = gEngfuncs.pfnRandomFloat(-256, 255);
		pTemp->entity.baseline.angles.z = gEngfuncs.pfnRandomFloat(-256, 255);
		pTemp->entity.baseline.origin = gibVelocity;
		pTemp->die = gHUD.m_flTime + lifeTime;
	}

	return 1;
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
int		DLLEXPORT HUD_VidInit( void );
void	DLLEXPORT HUD_Init( void );
int		DLLEXPORT HUD_Redraw( float flTime, int intermission );
int		DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void	DLLEXPORT HUD_Reset ( void );
void	DLLEXPORT HUD_Shutdown( void );
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int		DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int		DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );
void DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *gpMobileEngfuncs );
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

	return 1;
}

/*
=================
HUD_GetRect

VGui stub
=================
*/
int *HUD_GetRect( void )
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
	void Initialize( void );

	virtual void paintBackground();
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

int DLLEXPORT HUD_VidInit( void )
{
	gHUD.VidInit();
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

#ifdef CLDLL_FOG
	gHUD.m_iHardwareMode = IEngineStudio.IsHardware();
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

	ScaledRenderer::Instance().HUD_VidInit();

	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

void DLLEXPORT HUD_Init( void )
{
	InitInput();
	gHUD.Init();
#if USE_VGUI
	Scheme_Init();
#endif

	gEngfuncs.pfnHookUserMsg( "Bhopcap", __MsgFunc_Bhopcap );
	gEngfuncs.pfnHookUserMsg( "UseSound", __MsgFunc_UseSound );
	gEngfuncs.pfnHookUserMsg( "RandomGibs", __MsgFunc_RandomGibs );

	ScaledRenderer::Instance().HUD_Init();
}

/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int DLLEXPORT HUD_Redraw( float time, int intermission )
{
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

void DLLEXPORT HUD_Reset( void )
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
#if USE_VGUI
	GetClientVoiceMgr()->Frame(time);
#elif USE_FAKE_VGUI
	if (!gViewPort)
		gEngfuncs.VGui_ViewportPaintBackground(HUD_GetRect());
#else
	gEngfuncs.VGui_ViewportPaintBackground(HUD_GetRect());
#endif

	ScaledRenderer::Instance().HUD_Frame(time);
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

void DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *gpMobileEngfuncs )
{
	if( gpMobileEngfuncs->version != MOBILITY_API_VERSION )
		return;
	gMobileEngfuncs = gpMobileEngfuncs;
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

void DLLEXPORT HUD_Shutdown( void )
{
	ShutdownInput();
#ifdef CLDLL_FOG
	UnloadOpenGL();
#endif
}

bool IsXashFWGS()
{
	return gMobileEngfuncs != NULL;
}
