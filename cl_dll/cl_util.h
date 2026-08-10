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
// cl_util.h
//
#pragma once
#if !defined(CL_UTIL_H)
#define CL_UTIL_H
#include <cassert>
#include "cl_dll.h"
#include "exportdef.h"
#include "cvardef.h"
#include "color_utils.h"

// Macros to hook function calls into the HUD object

#define HOOK_MESSAGE(x) gEngfuncs.pfnHookUserMsg( #x, __MsgFunc_##x );

#define DECLARE_MESSAGE(y, x) int __MsgFunc_##x( const char *pszName, int iSize, void *pbuf ) \
						{ \
							return gHUD.y.MsgFunc_##x(pszName, iSize, pbuf ); \
						}

#define HOOK_COMMAND(x, y) gEngfuncs.pfnAddCommand( x, __CmdFunc_##y );
#define DECLARE_COMMAND(y, x) void __CmdFunc_##x() \
							{ \
								gHUD.y.UserCmd_##x(); \
							}

inline float CVAR_GET_FLOAT( const char *x ) {	return gEngfuncs.pfnGetCvarFloat( (char*)x ); }
inline char* CVAR_GET_STRING( const char *x ) {	return gEngfuncs.pfnGetCvarString( (char*)x ); }
inline struct cvar_s *CVAR_CREATE( const char *cv, const char *val, const int flags ) {	return gEngfuncs.pfnRegisterVariable( (char*)cv, (char*)val, flags ); }

#define SPR_Load ( *gEngfuncs.pfnSPR_Load )

inline void SPR_Set(HSPRITE hPic, int r, int g, int b) {
	gEngfuncs.pfnSPR_Set(hPic, r, g, b);
}

#define SPR_Frames ( *gEngfuncs.pfnSPR_Frames )
#define SPR_GetList ( *gEngfuncs.pfnSPR_GetList )

// SPR_Draw  draws a the current sprite as solid
inline void SPR_Draw(int frame, int x, int y, const wrect_t *prc) {
	gEngfuncs.pfnSPR_Draw(frame, x, y, prc);
}
// SPR_DrawHoles  draws the current sprites, with color index255 not drawn (transparent)
inline void SPR_DrawHoles(int frame, int x, int y, const wrect_t *prc) {
	gEngfuncs.pfnSPR_DrawHoles(frame, x, y, prc);
}

// SPR_DrawAdditive  adds the sprites RGB values to the background  (additive transulency)
inline void SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc) {
	gEngfuncs.pfnSPR_DrawAdditive(frame, x, y, prc);
}

// SPR_EnableScissor  sets a clipping rect for HUD sprites. (0,0) is the top-left hand corner of the screen.
#define SPR_EnableScissor ( *gEngfuncs.pfnSPR_EnableScissor )
// SPR_DisableScissor  disables the clipping rect
#define SPR_DisableScissor ( *gEngfuncs.pfnSPR_DisableScissor )
//
inline void FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a) {
	gEngfuncs.pfnFillRGBA(x, y, width, height, r, g, b, a);
}

// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight (gHUD.m_scrinfo.iHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth (gHUD.m_scrinfo.iWidth)

// Use this to set any co-ords in 640x480 space
#define XRES(x)		( (int)( float(x) * ( (float)ScreenWidth / 640.0f ) + 0.5f ) )
#define YRES(y)		( (int)( float(y) * ( (float)ScreenHeight / 480.0f ) + 0.5f ) )
#define XRES_HD(x)      ( (int)( float(x) * Q_max(1.f, (float)ScreenWidth / 1280.f )))
#define YRES_HD(y)	( (int)( float(y) * Q_max(1.f, (float)ScreenHeight / 720.f )))

// use this to project world coordinates to screen coordinates
#define XPROJECT(x)	( ( 1.0f + (x) ) * ScreenWidth * 0.5f )
#define YPROJECT(y)	( ( 1.0f - (y) ) * ScreenHeight * 0.5f )

#define GetScreenInfo ( *gEngfuncs.pfnGetScreenInfo )
#define ServerCmd ( *gEngfuncs.pfnServerCmd )
#define ClientCmd ( *gEngfuncs.pfnClientCmd )

inline void SetCrosshair(HSPRITE hspr, wrect_t rc, int r, int g, int b) {
	gEngfuncs.pfnSetCrosshair(hspr, rc, r, g, b);
}

#define AngleVectors ( *gEngfuncs.pfnAngleVectors )
#define Com_RandomLong (*gEngfuncs.pfnRandomLong)
#define Com_RandomFloat (*gEngfuncs.pfnRandomFloat)

inline void DrawSetTextColor( float r, float g, float b )
{
	gEngfuncs.pfnDrawSetTextColor( r, g, b );
}

// Gets the height & width of a sprite,  at the specified frame
inline int SPR_Height( HSPRITE x, int f )	{ return gEngfuncs.pfnSPR_Height(x, f); }
inline int SPR_Width( HSPRITE x, int f )	{ return gEngfuncs.pfnSPR_Width(x, f); }

inline client_textmessage_t *TextMessageGet( const char *pName )
{
	return gEngfuncs.pfnTextMessageGet( pName );
}

inline int TextMessageDrawChar( int x, int y, int number, int r, int g, int b ) 
{
	return gEngfuncs.pfnDrawCharacter( x, y, number, r, g, b ); 
}

inline int DrawConsoleString( int x, int y, const char *string )
{
	return gEngfuncs.pfnDrawConsoleString( x, y, (char*) string );
}

inline void GetConsoleStringSize( const char *string, int *width, int *height )
{
	gEngfuncs.pfnDrawConsoleStringLen( (char*)string, width, height );
}

inline int ConsoleStringLen( const char *string )
{
	int _width = 0, _height = 0;
	GetConsoleStringSize( string, &_width, &_height );
	return _width;
}

inline void ConsolePrint( const char *string )
{
	gEngfuncs.pfnConsolePrint( string );
}

inline void CenterPrint( const char *string )
{
	gEngfuncs.pfnCenterPrint( string );
}

// returns the players name of entity no.
#define GetPlayerInfo ( *gEngfuncs.pfnGetPlayerInfo )

// sound functions
inline void PlaySound( const char *szSound, float vol ) { gEngfuncs.pfnPlaySoundByName( szSound, vol ); }
inline void PlaySound( int iSound, float vol ) { gEngfuncs.pfnPlaySoundByIndex( iSound, vol ); }

#include "min_and_max.h"

int GetSpriteRes( int width, int height );

void ScaleColors( int &r, int &g, int &b, int a );

float Length( const float *v );
void VectorMA( const float *veca, float scale, const float *vecb, float *vecc );
void VectorScale( const float *in, float scale, float *out );
float VectorNormalize( float *v );
void VectorInverse( float *v );

float UTIL_ApproachAngle( float target, float value, float speed );

// disable 'possible loss of data converting float to int' warning message
#pragma warning( disable: 4244 )
// disable 'truncation from 'const double' to 'float' warning message
#pragma warning( disable: 4305 )

HSPRITE LoadSprite( const char *pszName );

bool HUD_MessageBox( const char *msg );
bool IsAnyXash();
bool IsXashFWGS();
bool LibrarySideFullbrightSupportIsOn();
void ShutdownInput();
#endif
