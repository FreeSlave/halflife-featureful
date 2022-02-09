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
#include "mod_features.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE( m_Flash, FlashBat )
DECLARE_MESSAGE( m_Flash, Flashlight )

#define BAT_NAME "sprites/%d_Flashlight.spr"

int CHudFlashlight::Init( void )
{
	m_fFade = 0;
	m_fOn = 0;

	HOOK_MESSAGE( Flashlight );
	HOOK_MESSAGE( FlashBat );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem( this );

	return 1;
}

void CHudFlashlight::Reset( void )
{
	m_fFade = 0;
	m_fOn = 0;
	m_iBat = 100;
	m_flBat = 1.0f;
}

int CHudFlashlight::VidInit( void )
{
	int HUD_flash_empty = gHUD.GetSpriteIndex( "flash_empty" );
	int HUD_flash_full = gHUD.GetSpriteIndex( "flash_full" );
	int HUD_flash_beam = gHUD.GetSpriteIndex( "flash_beam" );

	m_hSprite1 = gHUD.GetSprite( HUD_flash_empty );
	m_hSprite2 = gHUD.GetSprite( HUD_flash_full );
	m_hBeam = gHUD.GetSprite( HUD_flash_beam );
	m_prc1 = &gHUD.GetSpriteRect( HUD_flash_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_flash_full );
	m_prcBeam = &gHUD.GetSpriteRect(HUD_flash_beam);
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
	if( !show )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	if( gEngfuncs.IsSpectateOnly() )
		return 1;

	bool hasFlashlight = false;
#if FEATURE_FLASHLIGHT_ITEM
	hasFlashlight = hasFlashlight || ( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_FLASHLIGHT ) ) );
#endif
#if FEATURE_SUIT_FLASHLIGHT
	hasFlashlight = hasFlashlight || ( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) );
#endif
	if (!hasFlashlight)
		return 1;

	if( m_fOn )
		a = 225;
	else
		a = gHUD.MinHUDAlpha();

	if( m_flBat < 0.20f )
		UnpackRGB( r,g,b, RGB_REDISH );
	else
		UnpackRGB( r,g,b, gHUD.HUDColor() );

	ScaleColors( r, g, b, a );

	wrect_t* emptyFlash = m_prc1;
	wrect_t* fullFlash = m_prc2;

	y = ( emptyFlash->bottom - fullFlash->top ) / 2;
	x = ScaledRenderer::Instance().ScreenWidthScaled() - m_iWidth - m_iWidth / 2 ;

	// Draw the flashlight casing
	ScaledRenderer::Instance().SPR_Set( m_hSprite1, r, g, b );
	ScaledRenderer::Instance().SPR_DrawAdditive( 0,  x, y, emptyFlash );

	if( m_fOn )
	{
		// draw the flashlight beam
		x = ScaledRenderer::Instance().ScreenWidthScaled() - m_iWidth / 2;

		ScaledRenderer::Instance().SPR_Set( m_hBeam, r, g, b );
		ScaledRenderer::Instance().SPR_DrawAdditive( 0, x, y, m_prcBeam );
	}

	// draw the flashlight energy level
	x = ScaledRenderer::Instance().ScreenWidthScaled() - m_iWidth - m_iWidth / 2;
	int iOffset = m_iWidth * ( 1.0f - m_flBat );
	if( iOffset < m_iWidth )
	{
		rc = *m_prc2;
		rc.left += iOffset;

		ScaledRenderer::Instance().SPR_Set( m_hSprite2, r, g, b );
		ScaledRenderer::Instance().SPR_DrawAdditive( 0, x + iOffset, y, &rc );
	}

	return 1;
}
