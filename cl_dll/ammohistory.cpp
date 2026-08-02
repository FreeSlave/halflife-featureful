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
//  ammohistory.cpp
//


#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include "ammohistory.h"

HistoryResource gHR;

#define AMMO_PICKUP_GAP ( gHR.iHistoryGap + 5 )
#define AMMO_PICKUP_PICK_HEIGHT		( 80 + ( gHR.iHistoryGap * 2 ) )
#define AMMO_PICKUP_HEIGHT_MAX		( ScreenHeight - 100 )

void HistoryResource::AddToHistory( int iType, int iId, int iCount )
{
	if( iType == HISTSLOT_AMMO && !iCount )
		return;  // no amount, so don't add

	if( ( ( ( AMMO_PICKUP_GAP * iCurrentHistorySlot ) + AMMO_PICKUP_PICK_HEIGHT ) > AMMO_PICKUP_HEIGHT_MAX ) || ( iCurrentHistorySlot >= MAX_HISTORY ) )
	{
		// the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}
	
	HIST_ITEM *freeslot = &rgAmmoHistory[iCurrentHistorySlot++];  // default to just writing to the first slot

	freeslot->type = iType;
	freeslot->iId = iId;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gHUD.m_flTime + gHUD.m_Ammo.DrawHistoryTime();
}

void HistoryResource::AddToHistory( int iType, const char *szName, int iCount, int packedColor )
{
	if( iType != HISTSLOT_ITEM )
		return;

	if( ( ( ( AMMO_PICKUP_GAP * iCurrentHistorySlot ) + AMMO_PICKUP_PICK_HEIGHT ) > AMMO_PICKUP_HEIGHT_MAX ) || ( iCurrentHistorySlot >= MAX_HISTORY ) )
	{
		// the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}

	HIST_ITEM *freeslot = &rgAmmoHistory[iCurrentHistorySlot++];  // default to just writing to the first slot

	// I am really unhappy with all the code in this file
	int i = gHUD.GetSpriteIndex( szName );
	if( i == -1 )
		return;  // unknown sprite name, don't add it to history

	freeslot->iId = i;
	freeslot->type = iType;
	freeslot->iCount = iCount;
	freeslot->packedColor = packedColor;

	freeslot->DisplayTime = gHUD.m_flTime + gHUD.m_Ammo.DrawHistoryTime();
}

void HistoryResource::CheckClearHistory()
{
	for( int i = 0; i < MAX_HISTORY; i++ )
	{
		if( rgAmmoHistory[i].type )
			return;
	}

	iCurrentHistorySlot = 0;
}

//
// Draw Ammo pickup history
//
void HistoryResource::ScaleColorsAccordingToDisplayTime(float displayTime, float flTime, int& r, int& g, int& b)
{
	float scale = (displayTime - flTime) * 80;
	ScaleColors( r, g, b, Q_min( scale, 255 ) );
}

int HistoryResource::DrawAmmoHistory( float flTime )
{
	for( int i = 0; i < MAX_HISTORY; i++ )
	{
		if( rgAmmoHistory[i].type )
		{
			rgAmmoHistory[i].DisplayTime = Q_min( rgAmmoHistory[i].DisplayTime, gHUD.m_flTime + gHUD.m_Ammo.DrawHistoryTime() );

			if( rgAmmoHistory[i].DisplayTime <= flTime )
			{
				// pic drawing time has expired
				memset( &rgAmmoHistory[i], 0, sizeof(HIST_ITEM) );
				CheckClearHistory();
			}
			else if( rgAmmoHistory[i].type == HISTSLOT_AMMO )
			{
				wrect_t rcPic;
				HSPRITE *spr = gWR.GetAmmoPicFromWeapon( rgAmmoHistory[i].iId, rcPic );
				bool hasSprite = spr && *spr;

				int r, g, b;
				UnpackRGB( r, g, b, gHUD.HUDColor() );
				ScaleColorsAccordingToDisplayTime(rgAmmoHistory[i].DisplayTime, flTime, r, g, b);

				// Draw the pic
				int ypos = ScreenHeight - ( CHud::Renderer().ScaleScreen(AMMO_PICKUP_PICK_HEIGHT) + (AMMO_PICKUP_GAP * i) );
				int width = 24;
				if (hasSprite)
				{
					width = Q_max((rcPic.right - rcPic.left), width);
				}
				int xpos = ScreenWidth - width;
				if( hasSprite )    // weapon isn't loaded yet so just don't draw the pic
				{
					// the dll has to make sure it has sent info the weapons you need
					SPR_Set( *spr, r, g, b );
					SPR_DrawAdditive( 0, xpos, ypos, &rcPic );
				}

				int numberXPos = xpos - 10;
				int numberXMin = xpos - 100;
				if (gHUD.UsingHighResSprites())
				{
					numberXPos -= 4;
					numberXMin -= 4;
				}
				CHud::AdditiveText::DrawNumberString( numberXPos, ypos, numberXMin, rgAmmoHistory[i].iCount, r, g, b );
			}
			else if( rgAmmoHistory[i].type == HISTSLOT_WEAP )
			{
				WEAPON *weap = gWR.GetWeapon( rgAmmoHistory[i].iId );

				if( !weap )
					return 1;  // we don't know about the weapon yet, so don't draw anything

				int r, g, b;
				UnpackRGB( r,g,b, gHUD.HUDColor() );

				if( !gWR.HasAmmo( weap ) )
					UnpackRGB( r, g, b, gHUD.HUDColorCritical() );	// if the weapon doesn't have ammo, display it as red

				ScaleColorsAccordingToDisplayTime(rgAmmoHistory[i].DisplayTime, flTime, r, g, b);

				int ypos = ScreenHeight - ( CHud::Renderer().ScaleScreen(AMMO_PICKUP_PICK_HEIGHT) + ( AMMO_PICKUP_GAP * i ) );
				int xpos = ScreenWidth - ( weap->rcInactive.right - weap->rcInactive.left );
				SPR_Set( weap->hInactive, r, g, b );
				SPR_DrawAdditive( 0, xpos, ypos, &weap->rcInactive );
			}
			else if( rgAmmoHistory[i].type == HISTSLOT_ITEM )
			{
				int r, g, b;

				if( !rgAmmoHistory[i].iId )
					continue;  // sprite not loaded

				wrect_t rect = gHUD.GetSpriteRect( rgAmmoHistory[i].iId );

				int spriteColor = rgAmmoHistory[i].packedColor != 0 ? rgAmmoHistory[i].packedColor : gHUD.HUDColor();
				UnpackRGB( r, g, b, spriteColor );
				ScaleColorsAccordingToDisplayTime(rgAmmoHistory[i].DisplayTime, flTime, r, g, b);

				int ypos = ScreenHeight - ( CHud::Renderer().ScaleScreen(AMMO_PICKUP_PICK_HEIGHT) + ( AMMO_PICKUP_GAP * i ) );
				int xpos = ScreenWidth - ( rect.right - rect.left ) - 10;

				SPR_Set( gHUD.GetSprite( rgAmmoHistory[i].iId ), r, g, b );
				SPR_DrawAdditive( 0, xpos, ypos, &rect );

				if (rgAmmoHistory[i].iCount > 1)
				{
					CHud::AdditiveText::DrawNumberString( xpos - 10, ypos, xpos - 100, rgAmmoHistory[i].iCount, r, g, b );
				}
			}
		}
	}

	return 1;
}
