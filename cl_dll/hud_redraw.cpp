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

float HUD_GetFOV();

extern cvar_t *sensitivity;

// Think
void CHud::Think()
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

	if( gEngfuncs.IsSpectateOnly() )
	{
		m_iFOV = gHUD.m_Spectator.GetFOV(); // default_fov->value;
	}
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud::Redraw( float flTime, int intermission )
{
	RecacheValues();

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
			if (gHUD.UseVguiScoreBoard())
				gViewPort->ShowScoreBoard();
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

	m_iHudNumbersYOffset = UsingHighResSprites() ? m_iFontHeight * 0.2 : 0;

	m_Caption.Update(flTime, m_flTimeDelta);
	m_Journal.Update(flTime, m_flTimeDelta);

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

	bool shouldResetCrosshair = false;

	if (m_colorableCrosshair != CrosshairColorable())
	{
		m_colorableCrosshair = !m_colorableCrosshair;
		gEngfuncs.Con_DPrintf("Resetting crosshair because colorable_crosshair changed\n");
		shouldResetCrosshair = true;
	}
	int crosshairColor = GetCrosshairColor();
	if (m_lastCrosshairColor != crosshairColor)
	{
		m_lastCrosshairColor = crosshairColor;
		if (!shouldResetCrosshair)
		{
			gEngfuncs.Con_DPrintf("Resetting crosshair because hud color changed\n");
			shouldResetCrosshair = true;
		}
	}
	if (shouldResetCrosshair) {
		ResetCrosshair();
	}

	if (m_pCvarCrosshair->value > 0.0f && !gHUD.m_Scoreboard.m_iShowscoresHeld && !(gHUD.m_Journal.m_iShowscoresHeld && gHUD.m_Journal.ShouldDraw())) {
		CHud::Renderer().DrawCrosshair();
	}

	if (m_pCvarShowPos && m_pCvarShowPos->value > 0)
	{
		extern Vector v_origin, v_angles;

		cl_entity_t* pl = gEngfuncs.GetLocalPlayer();

		const Vector pos = m_pCvarShowPos->value == 2 ? pl->origin : v_origin;
		const Vector ang = m_pCvarShowPos->value == 2 ? pl->angles : v_angles;
		const char* posType = m_pCvarShowPos->value == 2 ? "ent" : "view";

		const int x = ScreenWidth/2;
		int y = 4;
		const int textHeight = ConsoleText::LineHeight();
		char posBuf[256];

		safe_snprintf(posBuf, sizeof(posBuf), "pos (%s): %.2f %.2f %.2f", posType, pos.x, pos.y, pos.z);
		ConsoleText::DrawString(x, y, ScreenWidth, posBuf, 255, 255, 255);
		y += textHeight;

		safe_snprintf(posBuf, sizeof(posBuf), "ang (%s): %.2f %.2f %.2f", posType, ang.x, ang.y, ang.z);
		ConsoleText::DrawString(x, y, ScreenWidth, posBuf, 255, 255, 255);
		y += textHeight;

		safe_snprintf(posBuf, sizeof(posBuf), "velocity: %.2f", m_velocity.Length());
		ConsoleText::DrawString(x, y, ScreenWidth, posBuf, 255, 255, 255);
	}

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

int CHud::DrawHudString(int xpos, int ypos, int iMaxX, const char *szIt, int r, int g, int b, int length)
{
	// xash3d: reset unicode state
	TextMessageDrawChar( 0, 0, 0, 0, 0, 0 );

	const char* start = szIt;

	// draw the string until we hit the null character or a newline character
	for( ; (length == -1 || szIt - start < length) && *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		int w = gHUD.m_scrinfo.charWidths[(unsigned char)*szIt];
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
		if( iNumber >= 1000 )
		{
			k = iNumber / 1000;
			CHud::Renderer().SPR_DrawAdditive( GetSprite( m_HUD_number_0 + k ), r, g, b, x, y, &GetSpriteRect( m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if( iFlags & ( DHN_4DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 100's
		if( iNumber >= 100 )
		{
			k = (iNumber % 1000) / 100;
			CHud::Renderer().SPR_DrawAdditive( GetSprite( m_HUD_number_0 + k ), r, g, b, x, y, &GetSpriteRect( m_HUD_number_0 + k ) );
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
			CHud::Renderer().SPR_DrawAdditive( GetSprite( m_HUD_number_0 + k ), r, g, b, x, y, &GetSpriteRect( m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if( iFlags & ( DHN_3DIGITS | DHN_2DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		CHud::Renderer().SPR_DrawAdditive( GetSprite( m_HUD_number_0 + k ), r, g, b,  x, y, &GetSpriteRect( m_HUD_number_0 + k ) );
		x += iWidth;
	}
	else if( iFlags & DHN_DRAWZERO )
	{
		if( iFlags & ( DHN_4DIGITS ) )
		{
			x += iWidth;
		}

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
		CHud::Renderer().SPR_DrawAdditive( GetSprite( m_HUD_number_0 ), r, g, b,  x, y, &GetSpriteRect( m_HUD_number_0 ) );
		x += iWidth;
	}

	return x;
}

static int count_digits(int n) {
	int result = 0;

	do {
		++result;
	} while ((n /= 10) != 0);

	return result;
}

static constexpr int ten_powers[] = {
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000
};

int CHud::DrawHudNumber(int x, int y, int number, int r, int g, int b)
{
	auto digit_width = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	auto digit_count = count_digits(number);

	for (int i = digit_count; i > 0; --i) {
		int digit = number / ten_powers[i - 1];

		CHud::Renderer().SPR_DrawAdditive(GetSprite(m_HUD_number_0 + digit), r, g, b, x, y, &GetSpriteRect(m_HUD_number_0 + digit));
		x += digit_width;

		number -= digit * ten_powers[i - 1];
	}

	return x;
}

int CHud::DrawHudNumberCentered(int x, int y, int number, int r, int g, int b)
{
	auto digit_width = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	auto digit_count = count_digits(number);

	return DrawHudNumber(x - (digit_width * digit_count) / 2, y, number, r, g, b);
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

int CHud::ConsoleText::DrawString(int xpos, int ypos, const char *szString, int r, int g, int b, int length)
{
	return DrawString(xpos, ypos, ScreenWidth, szString, r, g, b, length);
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

int CHud::AdditiveText::DrawString(int xpos, int ypos, const char *szString, int r, int g, int b, int length)
{
	return DrawString(xpos, ypos, ScreenWidth, szString, r, g, b, length);
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
	if( iFlags & ( DHN_4DIGITS ) )
		return 4;

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

	if ( iNumber < 1000 )
		return 3;

	return 4;
}	

void CHud::DrawDarkRectangle( int x, int y, int wide, int tall )
{
	DrawDarkRectangle(x, y, wide, tall, RectangleRenderProperties{});
}

void CHud::DrawDarkRectangle(int x, int y, int wide, int tall, const RectangleRenderProperties& rectProps )
{
	const auto& background = rectProps.backgroundColor;
	const auto& frame = rectProps.frameColor;

	auto fillBackgroundFunc = rectProps.backgroundBlend ? gEngfuncs.pfnFillRGBABlend : gEngfuncs.pfnFillRGBA;
	auto fillFrameFunc = rectProps.frameBlend ? gEngfuncs.pfnFillRGBABlend : gEngfuncs.pfnFillRGBA;

	fillBackgroundFunc( x, y, wide, tall, background.r, background.g, background.b, rectProps.backgroundAlpha );
	fillFrameFunc( x + 1, y, wide - 1, 1, frame.r, frame.g, frame.b, rectProps.frameAlpha );
	fillFrameFunc( x, y, 1, tall - 1, frame.r, frame.g, frame.b, rectProps.frameAlpha );
	fillFrameFunc( x + wide - 1, y + 1, 1, tall - 1, frame.r, frame.g, frame.b, rectProps.frameAlpha );
	fillFrameFunc( x, y + tall - 1, wide - 1, 1, frame.r, frame.g, frame.b, rectProps.frameAlpha );
}

int CHud::HUDColor()
{
	int result = HasSuit() ? m_cachedHudColor : (m_forcedHudColorNoSuit ? m_forcedHudColorNoSuit : clientFeatures.hud_color_nosuit);
	if (this == &gHUD && gHUD.m_Nightvision.IsOn()) {
		result = clientFeatures.hud_color_nvg;
	}
	return result;
}

int CHud::HUDColorCritical()
{
	if (m_forcedHudColorCritical)
		return m_forcedHudColorCritical;
	return clientFeatures.hud_color_critical;
}

int CHud::MinHUDAlpha() const
{
	return m_cachedMinAlpha;
}

void CHud::RecacheValues()
{
	m_cachedMinAlpha = CalcMinHUDAlpha();
	if (m_forcedHudColor)
	{
		m_cachedHudColor = m_forcedHudColor;
	}
	else
	{
		if (clientFeatures.hud_color_configurable)
		{
			int hudR = m_pCvarHudRed->value;
			int hudG = m_pCvarHudGreen->value;
			int hudB = m_pCvarHudBlue->value;
			m_cachedHudColor = PackRGB(hudR, hudG, hudB);
		}
		else
		{
			m_cachedHudColor = clientFeatures.hud_color;
		}
	}

	m_cachedTextColor = m_cachedHudColor;
	int r, g, b;
	UnpackRGB(r, g, b, m_cachedTextColor);
	const int rgbSum  = r + g + b;
	if (rgbSum < 224 && rgbSum > 0)
	{
		const float multiplier = 224.0 / rgbSum;
		r = Q_min(r * multiplier, 255);
		g = Q_min(g * multiplier, 255);
		b = Q_min(b * multiplier, 255);
		m_cachedTextColor = PackRGB(r, g, b);
	}
}

int CHud::GetCrosshairColor()
{
	if (CrosshairColorable())
	{
		return gHUD.HUDColor();
	}
	else
	{
		return 0xFFFFFF;
	}
}

void CHud::ResetCrosshair()
{
	if( !( m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
	{
		WEAPON* pWeapon = m_Ammo.GetWeapon();
		if (pWeapon)
		{
			int crosshairColor = gHUD.GetCrosshairColor();
			int r,g,b;
			UnpackRGB(r,g,b,crosshairColor);
			if( !ShouldUseZoomedCrosshair() )
			{
				SetCrosshair( pWeapon->hCrosshair, pWeapon->rcCrosshair, r, g, b );
			}
			else
			{
				SetCrosshair( pWeapon->hZoomedCrosshair, pWeapon->rcZoomedCrosshair, r, g, b );
			}
		}
	}
}

int CHud::HUDTextColor()
{
	return m_cachedTextColor;
}
