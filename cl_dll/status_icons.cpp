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
// status_icons.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "parsemsg.h"
#include "event_api.h"
#include "ammohistory.h"
#include "string_utils.h"
#include "arraysize.h"

DECLARE_MESSAGE( m_StatusIcons, StatusIcon )
DECLARE_MESSAGE( m_StatusIcons, Inventory )

int CHudStatusIcons::Init( void )
{
	HOOK_MESSAGE( StatusIcon );
	HOOK_MESSAGE( Inventory );

	gHUD.AddHudElem( this );

	Reset();

	return 1;
}

int CHudStatusIcons::VidInit( void )
{
	return 1;
}

void CHudStatusIcons::Reset( void )
{
	memset( m_IconList, 0, sizeof m_IconList );
	for (auto& item : m_InventoryList)
	{
		item = inventory_t();
	}
	m_iFlags &= ~HUD_ACTIVE;
}

#define BUF_FOR_SHORT_SIZE 24

static void FillCharBufWithNumberSuffix(char* buf, int size, int count)
{
	safe_snprintf(buf, size, "x%d", count);
}

static int ItemNumberSuffixWidth(int count)
{
	if (count <= 1)
		return 0;
	char buf[BUF_FOR_SHORT_SIZE];
	FillCharBufWithNumberSuffix(buf, sizeof(buf), count);
	return CHud::AdditiveText::LineWidth(buf);
}

// Draw status icons along the left-hand side of the screen
int CHudStatusIcons::Draw( float flTime )
{
	if( gEngfuncs.IsSpectateOnly() )
		return 1;

	if (gHUD.m_Journal.m_iShowscoresHeld && gHUD.m_Journal.HasInventorySection())
		return 1;

	bool drawStatusIcons = true;//gHUD.CanDrawStatusIcons();

	int i;
	int xIcons = 5;
	int yIcons = 135;
	const int statusIconGap = 5;
	const int bottomIconGap = 2;
	const int topRightIconGap = 2;

	HudSpriteRenderer& renderer = CHud::Renderer();

	int bottomItemCount = 0;
	int bottomWidth = 0;
	for (i=0; i<MAX_INVENTORY_ITEMS; ++i)
	{
		const inventory_t& item = m_InventoryList[i];
		if (item.spr && item.position == INVENTORY_PLACE_BOTTOM_CENTER)
		{
			bottomWidth += item.rc.right - item.rc.left + ItemNumberSuffixWidth(item.count);
			bottomItemCount++;
		}
	}

	int yTopRight = gHUD.TopRightInventoryCoordinate();
	if (yTopRight == 0)
	{
		yTopRight = yIcons;
	}
	int xCenterBottom = (renderer.PerceviedScreenWidth() - bottomWidth - ((bottomItemCount-1)*bottomIconGap)) / 2;

	int itemsDrawnAsStatusIcons = 0;
	const int textLineHeight = CHud::AdditiveText::LineHeight();

	for (i=0; i<MAX_INVENTORY_ITEMS; ++i)
	{
		const inventory_t& item = m_InventoryList[i];
		if (item.spr)
		{
			int r = item.r;
			int g = item.g;
			int b = item.b;
			if (r == 0 && g == 0 && b == 0)
				UnpackRGB(r, g, b, gHUD.HUDColor());

			int rText = r;
			int gText = g;
			int bText = b;
			ScaleColors(rText, gText, bText, gHUD.m_inventorySpec.TextAlpha());

			int alpha = item.a;
			if (alpha <= 0)
			{
				alpha = gHUD.m_inventorySpec.DefaultSpriteAlpha();
			}
			ScaleColors(r, g, b, alpha);

			int place = item.position == INVENTORY_PLACE_DEFAULT ? INVENTORY_PLACE_TOP_LEFT : item.position;

			const int height = item.rc.bottom - item.rc.top;
			const int width = item.rc.right - item.rc.left;
			const int heightInScreenSpace = renderer.ScaleScreen(height);
			const int textYShift = heightInScreenSpace - Q_min(textLineHeight, heightInScreenSpace);
			int yAdditionalShift = 0;
			if (textLineHeight > heightInScreenSpace)
			{
				yAdditionalShift = renderer.UnscaleScreen(textLineHeight - heightInScreenSpace);
			}

			if (place == INVENTORY_PLACE_TOP_RIGHT)
			{
				int xItem = gHUD.m_Flash.RightmostCoordinate() - width;
				if (item.count > 1)
				{
					char buf[BUF_FOR_SHORT_SIZE];
					FillCharBufWithNumberSuffix(buf, sizeof(buf), item.count);
					xItem -= renderer.UnscaleScreen(CHud::AdditiveText::LineWidth(buf));
					CHud::AdditiveText::DrawString(renderer.ScaleScreen(xItem + width), renderer.ScaleScreen(yTopRight) + textYShift, ScreenWidth, buf, rText, gText, bText);
				}

				renderer.SPR_DrawAdditive(item.spr, r, g, b, xItem, yTopRight, &item.rc);
				yTopRight += height + topRightIconGap + yAdditionalShift;
			}
			else if (place == INVENTORY_PLACE_TOP_LEFT)
			{
				if (!drawStatusIcons)
					continue;
				renderer.SPR_DrawAdditive(item.spr, r, g, b, xIcons, yIcons, &item.rc);
				if (item.count > 1)
				{
					char buf[BUF_FOR_SHORT_SIZE];
					FillCharBufWithNumberSuffix(buf, sizeof(buf), item.count);
					CHud::AdditiveText::DrawString(renderer.ScaleScreen(xIcons + width), renderer.ScaleScreen(yIcons) + textYShift, ScreenWidth, buf, rText, gText, bText);
				}
				yIcons += height + statusIconGap + yAdditionalShift;
				itemsDrawnAsStatusIcons++;
			}
			else if (place == INVENTORY_PLACE_BOTTOM_CENTER)
			{
				const int yCenterBottom = renderer.PerceviedScreenHeight() - gHUD.m_iFontHeight / 2 - height;
				renderer.SPR_DrawAdditive(item.spr, r, g, b, xCenterBottom, yCenterBottom, &item.rc);
				xCenterBottom += width;

				if (item.count > 1)
				{
					char buf[BUF_FOR_SHORT_SIZE];
					FillCharBufWithNumberSuffix(buf, sizeof(buf), item.count);
					CHud::AdditiveText::DrawString(renderer.ScaleScreen(xCenterBottom), renderer.ScaleScreen(yCenterBottom) + textYShift, ScreenWidth, buf, rText, gText, bText);
					xCenterBottom += renderer.UnscaleScreen(CHud::AdditiveText::LineWidth(buf));
				}

				xCenterBottom += bottomIconGap;
			}
		}
	}

	if (!drawStatusIcons)
		return 1;

	int statusIconsLeft = MAX_ICONSPRITES - itemsDrawnAsStatusIcons;
	// loop through icon list, and draw any valid icons drawing up from the middle of screen
	for( i = 0; i < statusIconsLeft; i++ )
	{
		if( m_IconList[i].spr )
		{
			int r = m_IconList[i].r;
			int g = m_IconList[i].g;
			int b = m_IconList[i].b;
			if (r == 0 && g == 0 && b == 0)
				UnpackRGB(r, g, b, gHUD.HUDColor());
			renderer.SPR_DrawAdditive(m_IconList[i].spr, r, g, b, xIcons, yIcons, &m_IconList[i].rc);
			yIcons += ( m_IconList[i].rc.bottom - m_IconList[i].rc.top ) + statusIconGap;
		}
	}

	return 1;
}

// Message handler for StatusIcon message
// accepts five values:
//		byte   : TRUE = ENABLE icon, FALSE = DISABLE icon
//		string : the sprite name to display
//		byte   : red
//		byte   : green
//		byte   : blue
int CHudStatusIcons::MsgFunc_StatusIcon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const int flags = READ_BYTE();
	const bool shouldEnable = (flags & PLAYER_STATUS_ICON_ENABLE) != 0;
	const bool allowDuplicate = (flags & PLAYER_STATUS_ICON_ALLOW_DUPLICATE) != 0;
	const char *pszIconName = READ_STRING();
	if( shouldEnable )
	{
		int r = READ_BYTE();
		int g = READ_BYTE();
		int b = READ_BYTE();
		EnableIcon( pszIconName, r, g, b, allowDuplicate );
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		DisableIcon( pszIconName );
	}

	return 1;
}

int CHudStatusIcons::MsgFunc_Inventory(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int count = READ_SHORT();
	const char* itemName = READ_STRING();
	int flags = READ_BYTE();
	if (!*itemName)
		return 1; // don't allow empty names

	int freeSlot = -1;
	int i;
	for (i=0; i<static_cast<int>(ARRAYSIZE(m_InventoryList)); ++i)
	{
		if (freeSlot < 0 && m_InventoryList[i].itemName.empty())
		{
			freeSlot = i;
		}
		if (m_InventoryList[i].itemName == itemName)
		{
			break;
		}
	}

	if (i == static_cast<int>(ARRAYSIZE(m_InventoryList)))
	{
		if (freeSlot >= 0)
		{
			i = freeSlot;
		}
		else
		{
			// no free slots
			return 1;
		}
	}

	const InventoryItemHudSpec* itemSpec = gHUD.m_inventorySpec.GetInventoryItemSpec(itemName);

	const char* spriteName = itemName;
	if (itemSpec)
	{
		if (*itemSpec->spriteName)
			spriteName = itemSpec->spriteName;
	}

	int spr_index = gHUD.GetSpriteIndex(spriteName);
	if (spr_index == -1)
	{
		// no sprite, can't show anything, so don't bother
		return 1;
	}

	inventory_t& item = m_InventoryList[i];
	const int countDiff = count - item.count;

	item.itemName = itemName;
	item.count = count;

	if (item.count == 0)
	{
		item = inventory_t();
		return 1;
	}

	if (itemSpec)
	{
		int r, g, b;
		UnpackRGB(r, g, b, itemSpec->packedColor);
		item.r = r;
		item.g = g;
		item.b = b;
		item.a = itemSpec->alpha;
		item.position = itemSpec->position;
		item.showInJournal = itemSpec->showInJournal;
	}
	else
	{
		item.r = item.g = item.b = 0;
		item.a = 0;
		item.position = INVENTORY_PLACE_DEFAULT;
		item.showInJournal = true;
	}

	item.spr = gHUD.GetSprite( spr_index );
	item.rc = gHUD.GetSpriteRect( spr_index );
	m_iFlags |= HUD_ACTIVE;

	if (!(flags & INVENTORY_DONT_SHOW_IN_HISTORY) && countDiff >= 1 && (!itemSpec || itemSpec->showInHistory))
		gHR.AddToHistory(HISTSLOT_ITEM, spriteName, countDiff, itemSpec ? itemSpec->packedColor : 0);

	return 1;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon(const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue , bool allowDuplicate)
{
	int i = 0;

	// check to see if the sprite is in the current list
	if (!allowDuplicate)
	{
		for( i = 0; i < MAX_ICONSPRITES; i++ )
		{
			if( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
				break;
		}
	}

	if( allowDuplicate || i == MAX_ICONSPRITES )
	{
		// icon not in list, so find an empty slot to add to
		for( i = 0; i < MAX_ICONSPRITES; i++ )
		{
			if( !m_IconList[i].spr )
				break;
		}
	}

	// if we've run out of space in the list, overwrite the first icon
	if( i == MAX_ICONSPRITES )
	{
		i = 0;
	}

	// Load the sprite and add it to the list
	// the sprite must be listed in hud.txt
	int spr_index = gHUD.GetSpriteIndex( pszIconName );
	if (spr_index != -1)
	{
		m_IconList[i].spr = gHUD.GetSprite( spr_index );
		m_IconList[i].rc = gHUD.GetSpriteRect( spr_index );
	}
	m_IconList[i].r = red;
	m_IconList[i].g = green;
	m_IconList[i].b = blue;
	strncpyEnsureTermination( m_IconList[i].szSpriteName, pszIconName );

	// Hack: Play Timer sound when a grenade icon is played (in 0.8 seconds)
	// This is for TFC
#if 0
	if( strstr(m_IconList[i].szSpriteName, "grenade") )
	{
		cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
		gEngfuncs.pEventAPI->EV_PlaySound( pthisplayer->index, pthisplayer->origin, CHAN_STATIC, "weapons/timer.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
#endif
}

void CHudStatusIcons::DisableIcon(const char *pszIconName)
{
	// find the sprite is in the current list
	for( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
		{
			// clear the item from the list
			memset( &m_IconList[i], 0, sizeof(icon_sprite_t) );
			return;
		}
	}
}
