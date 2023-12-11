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
// hud_redraw.cpp
//
#include <cmath>

#include "hud.h"
#include "cl_util.h"
//#include "triangleapi.h"

#if USE_VGUI
#include "vgui_TeamFortressViewport.h"
#endif

#define MAX_LOGO_FRAMES 56

int grgLogoFrame[MAX_LOGO_FRAMES] =
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
	16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
	29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31 
};

extern int g_iVisibleMouse;

float HUD_GetFOV( void );

extern cvar_t *sensitivity;

// Think
void CHud::Think( void )
{
#if USE_VGUI
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);
#endif

	int newfov;
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p->m_iFlags & HUD_ACTIVE )
			pList->p->Think();
		pList = pList->pNext;
	}

	newfov = HUD_GetFOV();
	if( newfov == 0 )
	{
		m_iFOV = default_fov->value;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud
	// Set a new sensitivity
	if( m_iFOV == default_fov->value )
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)default_fov->value) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	// think about default fov
	if( m_iFOV == 0 )
	{
		// only let players adjust up in fov,  and only if they are not overriden by something else
		m_iFOV = Q_max( default_fov->value, 90 );  
	}
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud::Redraw( float flTime, int intermission )
{
	m_cachedMinAlpha = CalcMinHUDAlpha();
	int hudR = m_pCvarHudRed->value;
	int hudG = m_pCvarHudGreen->value;
	int hudB = m_pCvarHudBlue->value;
	m_cachedHudColor = ((hudR & 0xFF) << 16) | ((hudG & 0xFF) << 8) | (hudB & 0xFF);

	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)( m_flTime - m_fOldTime );
	static float m_flShotTime = 0;

	if (fog.fadeDuration)
	{
		double fFraction = m_flTimeDelta/fog.fadeDuration;
		fog.endDist -= (FOG_LIMIT - fog.finalEndDist)*fFraction;

		if (fog.endDist > FOG_LIMIT)
			fog.endDist = FOG_LIMIT;
		if (fog.endDist < fog.finalEndDist)
			fog.endDist = fog.finalEndDist;
	}

	// Clock was reset, reset delta
	if( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;

#if USE_VGUI
	// Bring up the scoreboard during intermission
	if (gViewPort)
	{
		if( m_iIntermission && !intermission )
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideScoreBoard();
			gViewPort->UpdateSpectatorPanel();
		}
		else if( !m_iIntermission && intermission )
		{
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideVGUIMenu();
#if !USE_NOVGUI_SCOREBOARD
			gViewPort->ShowScoreBoard();
#endif
			gViewPort->UpdateSpectatorPanel();
			// Take a screenshot if the client's got the cvar set
			if( CVAR_GET_FLOAT( "hud_takesshots" ) != 0 )
				m_flShotTime = flTime + 1.0;	// Take a screenshot in a second
		}
	}
#else
	if( !m_iIntermission && intermission )
	{
		// Take a screenshot if the client's got the cvar set
		if( CVAR_GET_FLOAT( "hud_takesshots" ) != 0 )
			m_flShotTime = flTime + 1.0f;	// Take a screenshot in a second
	}
#endif
	if( m_flShotTime && m_flShotTime < flTime )
	{
		gEngfuncs.pfnClientCmd( "snapshot\n" );
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;

	if( m_pCvarDraw->value )
	{
		HUDLIST *pList = m_pHudList;

		while( pList )
		{
			if( !intermission )
			{
				if ( ( pList->p->m_iFlags & HUD_ACTIVE ) && !( m_iHideHUDDisplay & HIDEHUD_ALL ) )
					pList->p->Draw( flTime );
			}
			else
			{
				// it's an intermission,  so only draw hud elements that are set to draw during intermissions
				if( pList->p->m_iFlags & HUD_INTERMISSION )
					pList->p->Draw( flTime );
			}

			pList = pList->pNext;
		}
	}
	m_Nightvision.Draw( flTime );

	// are we in demo mode? do we need to draw the logo in the top corner?
	if( m_iLogo )
	{
		int x, y, i;

		if( m_hsprLogo == 0 )
			m_hsprLogo = LoadSprite( "sprites/%d_logo.spr" );

		SPR_Set( m_hsprLogo, 250, 250, 250 );

		x = SPR_Width( m_hsprLogo, 0 );
		x = ScreenWidth - x;
		y = SPR_Height( m_hsprLogo, 0 ) / 2;

		// Draw the logo at 20 fps
		int iFrame = (int)( flTime * 20 ) % MAX_LOGO_FRAMES;
		i = grgLogoFrame[iFrame] - 1;

		SPR_DrawAdditive( i, x, y, NULL );
	}

	/*
	if( g_iVisibleMouse )
	{
		void IN_GetMousePos( int *mx, int *my );
		int mx, my;

		IN_GetMousePos( &mx, &my );

		if( m_hsprCursor == 0 )
		{
			m_hsprCursor = SPR_Load( "sprites/cursor.spr" );
		}

		SPR_Set( m_hsprCursor, 250, 250, 250 );

		// Draw the logo at 20 fps
		SPR_DrawAdditive( 0, mx, my, NULL );
	}
	*/

	return 1;
}

void ScaleColors( int &r, int &g, int &b, int a )
{
	a = Q_min(a, 255);

	float x = (float)a / 255;
	r = (int)( r * x );
	g = (int)( g * x );
	b = (int)( b * x );
}

const unsigned char colors[8][3] =
{
{127, 127, 127}, // additive cannot be black
{255,   0,   0},
{  0, 255,   0},
{255, 255,   0},
{  0,   0, 255},
{  0, 255, 255},
{255,   0, 255},
{240, 180,  24}
};

int CHud::DrawHudString( int xpos, int ypos, int iMaxX, const char *szIt, int r, int g, int b )
{
	// xash3d: reset unicode state
	TextMessageDrawChar( 0, 0, 0, 0, 0, 0 );

	// draw the string until we hit the null character or a newline character
	for( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		int w = gHUD.m_scrinfo.charWidths['M'];
		if( xpos + w  > iMaxX )
			return xpos;
		if( ( *szIt == '^' ) && ( *( szIt + 1 ) >= '0') && ( *( szIt + 1 ) <= '7') )
		{
			szIt++;
			r = colors[*szIt - '0'][0];
			g = colors[*szIt - '0'][1];
			b = colors[*szIt - '0'][2];
			if( !*(++szIt) )
				return xpos;
		}
		int c = (unsigned int)(unsigned char)*szIt;

		xpos += TextMessageDrawChar( xpos, ypos, c, r, g, b );
	}

	return xpos;
}

int CHud::DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b )
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b );
}

// draws a string from right to left (right-aligned)
int CHud::DrawHudStringReverse( int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b )
{
	// find the end of the string
	for( const char *szIt = szString; *szIt != 0; szIt++ )
		xpos -= gHUD.m_scrinfo.charWidths[(unsigned char)*szIt];
	if( xpos < iMinX )
		xpos = iMinX;
	DrawHudString( xpos, ypos, gHUD.m_scrinfo.iWidth, szString, r, g, b );
	return xpos;
}

int CHud::DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b )
{
	int iWidth = GetSpriteRect( m_HUD_number_0 ).right - GetSpriteRect( m_HUD_number_0 ).left;
	int k;
	
	if( iNumber > 0 )
	{
		// SPR_Draw 100's
		if( iNumber >= 100 )
		{
			k = iNumber / 100;
			SPR_Set( GetSprite( m_HUD_number_0 + k ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if( iFlags & ( DHN_3DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if( iNumber >= 10 )
		{
			k = ( iNumber % 100 ) / 10;
			SPR_Set( GetSprite( m_HUD_number_0 + k ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if( iFlags & ( DHN_3DIGITS | DHN_2DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set( GetSprite( m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0,  x, y, &GetSpriteRect( m_HUD_number_0 + k ) );
		x += iWidth;
	}
	else if( iFlags & DHN_DRAWZERO )
	{
		SPR_Set( GetSprite( m_HUD_number_0 ), r, g, b );

		// SPR_Draw 100's
		if( iFlags & ( DHN_3DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if( iFlags & ( DHN_3DIGITS | DHN_2DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		SPR_DrawAdditive( 0,  x, y, &GetSpriteRect( m_HUD_number_0 ) );
		x += iWidth;
	}

	return x;
}

int CHud::ConsoleText::DrawString(int xpos, int ypos, int iMaxX, const char *szString, int r, int g, int b, int length)
{
	char buf[512] = {0};
	const char* str = buf;

	if (length < 0) {
		str = szString;
	} else {
		length = Q_min(length, sizeof(buf) - 1);
		strncpy(buf, szString, length);
		buf[length] = '\0';
	}

	gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
	return gEngfuncs.pfnDrawConsoleString(xpos, ypos, str);
}

int CHud::ConsoleText::DrawNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b)
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawStringReverse( xpos, ypos, iMinX, szString, r, g, b );
}

int CHud::ConsoleText::DrawStringReverse(int x, int ypos, int iMinX, const char *szString, int r, int g, int b, int length)
{
	x -= LineWidth(szString, length);
	if (x < iMinX)
		x = iMinX;
	return DrawString(x, ypos, gHUD.m_scrinfo.iWidth, szString, r, g, b, length);
}

int CHud::ConsoleText::LineWidth(const char *szString, int length)
{
	char buf[512] = {0};
	const char* str = buf;

	if (length < 0) {
		str = szString;
	} else {
		length = Q_min(length, sizeof(buf) - 1);
		strncpy(buf, szString, length);
		buf[length] = '\0';
	}

	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen(str, &width, &height);
	return width;
}

int CHud::ConsoleText::WidestCharacterWidth()
{
	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen("M", &width, &height);
	return height;
}

int CHud::ConsoleText::LineHeight()
{
	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen("YAW", &width, &height);
	return height;
}

int CHud::AdditiveText::DrawString(int xpos, int ypos, int iMaxX, const char *szString, int r, int g, int b, int length)
{
	TextMessageDrawChar( 0, 0, 0, 0, 0, 0 );

	const char* szStringStart = szString;
	for( ; *szString != 0 && (length < 0 || szString < szStringStart + length); szString++ )
	{
		int w = gHUD.m_scrinfo.charWidths['M'];
		if( xpos + w  > iMaxX )
			return xpos;
		int c = (unsigned int)(unsigned char)*szString;

		xpos += TextMessageDrawChar( xpos, ypos, c, r, g, b );
	}

	return xpos;
}

int CHud::AdditiveText::DrawNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b)
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawStringReverse( xpos, ypos, iMinX, szString, r, g, b );
}

int CHud::AdditiveText::DrawStringReverse(int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b, int length)
{
	// find the end of the string
	xpos -= LineWidth(szString, length);
	if( xpos < iMinX )
		xpos = iMinX;
	DrawString( xpos, ypos, gHUD.m_scrinfo.iWidth, szString, r, g, b );
	return xpos;
}

int CHud::AdditiveText::LineWidth(const char *szString, int length)
{
	int width = 0;
	const char* szStringStart = szString;
	while ((length < 0 || szString < szStringStart + length) && *szString != '\0')
	{
		width += gHUD.m_scrinfo.charWidths[(unsigned char)*szString];
		szString++;
	}
	return width;
}

int CHud::AdditiveText::WidestCharacterWidth()
{
	return gHUD.m_scrinfo.charWidths['M'];
}

int CHud::AdditiveText::LineHeight()
{
	return gHUD.m_scrinfo.iCharHeight + 1;
}

int CHud::GetNumWidth( int iNumber, int iFlags )
{
	if( iFlags & ( DHN_3DIGITS ) )
		return 3;

	if( iFlags & ( DHN_2DIGITS ) )
		return 2;

	if( iNumber <= 0 )
	{
		if( iFlags & ( DHN_DRAWZERO ) )
			return 1;
		else
			return 0;
	}

	if( iNumber < 10 )
		return 1;

	if( iNumber < 100 )
		return 2;

	return 3;
}	

void CHud::DrawDarkRectangle( int x, int y, int wide, int tall )
{
	//gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
	gEngfuncs.pfnFillRGBABlend( x, y, wide, tall, 0, 0, 0, 255 * 0.6 );
	FillRGBA( x + 1, y, wide - 1, 1, 255, 140, 0, 255 );
	FillRGBA( x, y, 1, tall - 1, 255, 140, 0, 255 );
	FillRGBA( x + wide - 1, y + 1, 1, tall - 1, 255, 140, 0, 255 );
	FillRGBA( x, y + tall - 1, wide - 1, 1, 255, 140, 0, 255 );
}

int CHud::HUDColor()
{
	int result = HasSuit() ? m_cachedHudColor : clientFeatures.hud_color_nosuit;
#if FEATURE_NIGHTVISION
	if (this == &gHUD && gHUD.m_Nightvision.IsOn()) {
		result = clientFeatures.hud_color_nvg;
	}
#endif
	return result;
}

int CHud::HUDColorCritical()
{
	return clientFeatures.hud_color_critical;
}

int CHud::MinHUDAlpha() const
{
	return m_cachedMinAlpha;
}
