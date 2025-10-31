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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Flash, FlashBat )
DECLARE_MESSAGE( m_Flash, Flashlight )

#define BAT_NAME "sprites/%d_Flashlight.spr"

int CHudFlashlight::Init()
{
	m_fFade = 0;
	m_fOn = 0;

	HOOK_MESSAGE( Flashlight );
	HOOK_MESSAGE( FlashBat );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem( this );

	return 1;
}

void CHudFlashlight::Reset()
{
	m_fFade = 0;
	m_fOn = 0;
	m_iBat = 100;
	m_flBat = 1.0f;
}

int CHudFlashlight::VidInit()
{
	int HUD_flash_empty = gHUD.GetSpriteIndex( "flash_empty" );
	int HUD_flash_full = gHUD.GetSpriteIndex( "flash_full" );
	int HUD_flash_beam = gHUD.GetSpriteIndex( "flash_beam" );

	const char* nvgEmptySprite = *gHUD.clientFeatures.nvg_empty_sprite ? gHUD.clientFeatures.nvg_empty_sprite : "flash_empty";
	const char* nvgFullSprite = *gHUD.clientFeatures.nvg_full_sprite ? gHUD.clientFeatures.nvg_full_sprite : "flash_full";

	int HUD_night_empty = gHUD.GetSpriteIndex( nvgEmptySprite );
	int HUD_night_full = gHUD.GetSpriteIndex( nvgFullSprite );

	m_hSprite1 = gHUD.GetSprite( HUD_flash_empty );
	m_hSprite2 = gHUD.GetSprite( HUD_flash_full );
	m_hSprite3 = gHUD.GetSprite( HUD_night_empty );
	m_hSprite4 = gHUD.GetSprite( HUD_night_full );
	m_hBeam = gHUD.GetSprite( HUD_flash_beam );
	m_prc1 = &gHUD.GetSpriteRect( HUD_flash_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_flash_full );
	m_prcBeam = &gHUD.GetSpriteRect(HUD_flash_beam);
	m_prc3 = &gHUD.GetSpriteRect( HUD_night_empty );
	m_prc4 = &gHUD.GetSpriteRect( HUD_night_full );
	m_iWidth = m_prc2->right - m_prc2->left;

	return 1;
}

int CHudFlashlight::MsgFunc_FlashBat( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ( (float)x ) / 100.0f;

	return 1;
}

int CHudFlashlight::MsgFunc_Flashlight( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ( (float)x ) / 100.0f;

	return 1;
}

int CHudFlashlight::Draw( float flTime )
{
	static bool show = ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) );
	if( show != !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) ) )
	{
		show = !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) );
		if( gMobileEngfuncs )
		{
			gMobileEngfuncs->pfnTouchHideButtons( "flashlight", !show );
		}
	}
	bottomCoordinate = 0;
	if( !show )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	if( gEngfuncs.IsSpectateOnly() )
		return 1;

	bool hasFlashlight = gHUD.HasFlashlight();
	bool hasNightVision = gHUD.HasNVG();
	if (!hasFlashlight && !hasNightVision)
		return 1;

	const bool nvgIsOn = gHUD.m_Nightvision.IsOn();
	const bool shouldDrawNvg = nvgIsOn || (!hasFlashlight && hasNightVision);

	if( m_fOn )
		a = 225;
	else
		a = gHUD.MinHUDAlpha();

	if( m_flBat < 0.20f )
		UnpackRGB( r,g,b, gHUD.HUDColorCritical() );
	else
		UnpackRGB( r,g,b, gHUD.HUDColor() );

	ScaleColors( r, g, b, a );

	int emptySprite = shouldDrawNvg ? m_hSprite3 : m_hSprite1;
	int fullSprite = shouldDrawNvg ? m_hSprite4 : m_hSprite2;

	if (!emptySprite || !fullSprite)
		return 1;

	wrect_t* emptyFlash = shouldDrawNvg ? m_prc3 : m_prc1;
	wrect_t* fullFlash = shouldDrawNvg ? m_prc4 : m_prc2;

	y = ( emptyFlash->bottom - fullFlash->top ) / 2;
	x = CHud::Renderer().PerceviedScreenWidth() - m_iWidth - m_iWidth / 2 ;

	// Draw the flashlight casing
	CHud::Renderer().SPR_DrawAdditive( emptySprite, r, g, b,  x, y, emptyFlash );

	// Don't draw a beam for nvg
	if( m_fOn && m_hBeam && !nvgIsOn )
	{
		// draw the flashlight beam
		x = CHud::Renderer().PerceviedScreenWidth() - m_iWidth / 2;

		CHud::Renderer().SPR_DrawAdditive( m_hBeam, r, g, b, x, y, m_prcBeam );
	}

	// draw the flashlight energy level
	x = CHud::Renderer().PerceviedScreenWidth() - m_iWidth - m_iWidth / 2;
	int iOffset = m_iWidth * ( 1.0f - m_flBat );
	if( iOffset < m_iWidth )
	{
		rc = *fullFlash;
		rc.left += iOffset;

		CHud::Renderer().SPR_DrawAdditive( fullSprite, r, g, b, x + iOffset, y, &rc );
	}

	bottomCoordinate = y + (emptyFlash->bottom - emptyFlash->top);
	return 1;
}

int CHudFlashlight::RightmostCoordinate()
{
	return CHud::Renderer().PerceviedScreenWidth() - m_iWidth / 2;
}
