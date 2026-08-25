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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"
#include "arraysize.h"

#include "ammohistory.h"
#include "ammoregistry.h"
#include "string_utils.h"

#include "weapons.h"

#if USE_VGUI
#include "vgui_TeamFortressViewport.h"
#endif

#include "parsetext.h"

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON *gpLastSel;		// Last weapon menu selection 

client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = 0;

extern int GetClientMaxAmmo(int ammoIndex);

void WeaponsResource::Init()
{
	memset( rgWeapons, 0, sizeof rgWeapons );
	Reset();

	const char* fileName = "features/hud_weapon_layout.cfg";
	int fileSize = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile(fileName , 5, &fileSize );
	if (pfile)
	{
		gEngfuncs.Con_DPrintf("Parsing HUD weapon positions from %s\n", fileName);
		ParseBucketPreferences(bucketPreferences, pfile, fileSize, fileName);
		gEngfuncs.COM_FreeFile(pfile);
	}

	m_maxWeaponSlots = 5;
}

void WeaponsResource::Reset()
{
	iOldWeaponBits = 0;
	memset( rgSlots, 0, sizeof rgSlots );
	memset( riAmmo, 0, sizeof riAmmo );
	memset( weaponTable, 0, sizeof weaponTable );
}

void WeaponsResource::AddWeapon(WEAPON *wp)
{
	if (rgWeapons[wp->iId].iId == wp->iId)
	{
		wp->iSlot = rgWeapons[wp->iId].iSlot;
		wp->iSlotPos = rgWeapons[wp->iId].iSlotPos;
	}
	else
	{
		// Check user preferences
		bool foundUserPreference = false;
		for (const BucketPreference& pref : bucketPreferences.list)
		{
			if (pref.szName[0] == '\0')
				break;

			if (pref.iPreferredSlot > 0 && strcmp(pref.szName, wp->szName) == 0)
			{
				// The user has preferred slot for this weapon
				wp->iSlot = pref.iPreferredSlot - 1;
				if (pref.iPreferredSlotPos > 0)
					wp->iSlotPos = pref.iPreferredSlotPos - 1;
				else // is position is not specified, to to the end of the bucket
					wp->iSlotPos = MAX_WEAPON_POSITIONS-1;
				foundUserPreference = true;
				break;
			}
		}

		// Check if there's a registered weapon with such position
		WEAPON* registeredWeapon = weaponTable[wp->iSlot][wp->iSlotPos];
		if (registeredWeapon && registeredWeapon->iId != wp->iId)
		{
			const char* weaponName = foundUserPreference ? registeredWeapon->szName : wp->szName;
			gEngfuncs.Con_DPrintf("Searching unoccupied position for %s at slot %d\n", weaponName, wp->iSlot + 1);
			int j;
			for (j=0; j< MAX_WEAPON_POSITIONS; ++j)
			{
				if (weaponTable[wp->iSlot][j] == NULL)
				{
					// If it's user preference move the existing weapon to the unoccupied position
					if (foundUserPreference)
					{
						registeredWeapon->iSlotPos = j;
						weaponTable[wp->iSlot][j] = registeredWeapon;
					}
					// otherwise just find unoccupied position for this weapon
					else
					{
						wp->iSlotPos = j;
					}
					break;
				}
			}
			if (j >= MAX_WEAPON_POSITIONS)
			{
				gEngfuncs.Con_DPrintf("Coulnd't find unoccupied position for %s at slot %d\n", weaponName, wp->iSlot + 1);
			}
		}
	}

	rgWeapons[wp->iId] = *wp;
	WEAPON* newWeapon = &rgWeapons[wp->iId];
	weaponTable[newWeapon->iSlot][newWeapon->iSlotPos] = newWeapon;
	LoadWeaponSprites( newWeapon );
}

void WeaponsResource::LoadAllWeaponSprites()
{
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
}

int WeaponsResource::CountAmmo( int iId ) 
{ 
	if( iId < 0 )
		return 0;

	return riAmmo[iId];
}

bool WeaponsResource::HasAmmo( WEAPON *p )
{
	if( !p )
		return 0;

	// weapons with no max ammo can always be selected
	bool result = ( p->iAmmoType <= 0 ) || p->iClip > 0 || CountAmmo( p->iAmmoType )
		|| CountAmmo( p->iAmmo2Type ) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );

	if (!result)
	{
		const WeaponParameters& params = GetWeaponParameters(p->iId);
		return params.IsUsableWithoutAmmo();
	}
	return result;
}

void WeaponsResource::LoadWeaponSprites( WEAPON *pWeapon )
{
	int i, iRes;

	iRes = GetSpriteRes( ScreenWidth, ScreenHeight );

	char sz[256];

	if( !pWeapon )
		return;

	memset( &pWeapon->rcActive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcInactive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo2, 0, sizeof(wrect_t) );
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;

	pWeapon->hMode = 0;
	pWeapon->hAltMode = 0;

	sprintf( sz, "sprites/%s.txt", pWeapon->szName );
	client_sprite_t *pList = SPR_GetList( sz, &i );

	if( !pList )
		return;

	auto AssignSpriteFull = [&](const char* name, HSPRITE& sprite, wrect_t& rect, const HSPRITE& fallbackSprite, const wrect_t& fallbackRect, bool updateHistoryGap) {
		client_sprite_t *p = GetSpriteList(pList, name, iRes, i);

		if (p)
		{
			sprintf( sz, "sprites/%s.spr", p->szSprite );
			sprite = SPR_Load( sz );
			rect = p->rc;

			if (updateHistoryGap)
				gHR.iHistoryGap = Q_max(gHR.iHistoryGap, rect.bottom - rect.top);
		}
		else
		{
			sprite = fallbackSprite;
			rect = fallbackRect;
		}
	};

	auto AssignSpriteWithFallback = [&](const char* name, HSPRITE& sprite, wrect_t& rect, const HSPRITE& fallbackSprite, const wrect_t& fallbackRect) {
		AssignSpriteFull(name, sprite, rect, fallbackSprite, fallbackRect, false);
	};

	auto AssignSprite = [&](const char* name, HSPRITE& sprite, wrect_t& rect) {
		AssignSpriteWithFallback(name, sprite, rect, 0, wrect_t{});
	};

	auto AssignSpriteUpdateHistoryGap = [&](const char* name, HSPRITE& sprite, wrect_t& rect) {
		AssignSpriteFull(name, sprite, rect, 0, wrect_t{}, true);
	};

	AssignSprite("crosshair", pWeapon->hCrosshair, pWeapon->rcCrosshair);
	AssignSprite("autoaim", pWeapon->hAutoaim, pWeapon->rcAutoaim);
	AssignSpriteWithFallback("zoom", pWeapon->hZoomedCrosshair, pWeapon->rcZoomedCrosshair, pWeapon->hCrosshair, pWeapon->rcCrosshair);
	AssignSpriteWithFallback("zoom_autoaim", pWeapon->hZoomedAutoaim, pWeapon->rcZoomedAutoaim, pWeapon->hZoomedCrosshair, pWeapon->rcZoomedCrosshair);
	AssignSpriteUpdateHistoryGap("weapon", pWeapon->hInactive, pWeapon->rcInactive);
	AssignSprite("weapon_s", pWeapon->hActive, pWeapon->rcActive);
	AssignSpriteUpdateHistoryGap("ammo", pWeapon->hAmmo, pWeapon->rcAmmo);
	AssignSpriteUpdateHistoryGap("ammo2", pWeapon->hAmmo2, pWeapon->rcAmmo2);
	AssignSpriteWithFallback("crosshair2", pWeapon->hCrosshairAlt, pWeapon->rcCrosshairAlt, pWeapon->hCrosshair, pWeapon->rcCrosshair);
	AssignSpriteWithFallback("autoaim2", pWeapon->hAutoaimAlt, pWeapon->rcAutoaimAlt, pWeapon->hAutoaim, pWeapon->rcAutoaim);
	AssignSprite("mode", pWeapon->hMode, pWeapon->rcMode);
	AssignSprite("mode2", pWeapon->hAltMode, pWeapon->rcAltMode);
}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource::GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for( int i = 0; i < MAX_WEAPON_POSITIONS; i++ )
	{
		if ( rgSlots[iSlot][i] && HasAmmo( rgSlots[iSlot][i] ) )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}

WEAPON* WeaponsResource::GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= m_maxWeaponSlots )
		return NULL;

	WEAPON *p = gWR.rgSlots[iSlot][iSlotPos + 1];
	
	if ( !p || !gWR.HasAmmo( p ) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}

int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu

DECLARE_MESSAGE( m_Ammo, CurWeapon )	// Current weapon and clip
DECLARE_MESSAGE( m_Ammo, AmmoList )	// new ammo type
DECLARE_MESSAGE( m_Ammo, WeaponList )	// new weapon type
DECLARE_MESSAGE( m_Ammo, AmmoX )		// update known ammo type's count
DECLARE_MESSAGE( m_Ammo, AmmoPickup )	// flashes an ammo pickup record
DECLARE_MESSAGE( m_Ammo, WeapPickup )    // flashes a weapon pickup record
DECLARE_MESSAGE( m_Ammo, HideWeapon )	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE( m_Ammo, ItemPickup )

DECLARE_COMMAND( m_Ammo, Slot1 )
DECLARE_COMMAND( m_Ammo, Slot2 )
DECLARE_COMMAND( m_Ammo, Slot3 )
DECLARE_COMMAND( m_Ammo, Slot4 )
DECLARE_COMMAND( m_Ammo, Slot5 )
DECLARE_COMMAND( m_Ammo, Slot6 )
DECLARE_COMMAND( m_Ammo, Slot7 )
DECLARE_COMMAND( m_Ammo, Slot8 )
DECLARE_COMMAND( m_Ammo, Slot9 )
DECLARE_COMMAND( m_Ammo, Slot10 )
DECLARE_COMMAND( m_Ammo, Close )
DECLARE_COMMAND( m_Ammo, NextWeapon )
DECLARE_COMMAND( m_Ammo, PrevWeapon )

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME	"5"

int CHudAmmo::Init()
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( CurWeapon );
	HOOK_MESSAGE( AmmoList );
	HOOK_MESSAGE( WeaponList );
	HOOK_MESSAGE( AmmoPickup );
	HOOK_MESSAGE( WeapPickup );
	HOOK_MESSAGE( ItemPickup );
	HOOK_MESSAGE( HideWeapon );
	HOOK_MESSAGE( AmmoX );

	HOOK_COMMAND( "slot1", Slot1 );
	HOOK_COMMAND( "slot2", Slot2 );
	HOOK_COMMAND( "slot3", Slot3 );
	HOOK_COMMAND( "slot4", Slot4 );
	HOOK_COMMAND( "slot5", Slot5 );
	HOOK_COMMAND( "slot6", Slot6 );
	HOOK_COMMAND( "slot7", Slot7 );
	HOOK_COMMAND( "slot8", Slot8 );
	HOOK_COMMAND( "slot9", Slot9 );
	HOOK_COMMAND( "slot10", Slot10 );
	HOOK_COMMAND( "cancelselect", Close );
	HOOK_COMMAND( "invnext", NextWeapon );
	HOOK_COMMAND( "invprev", PrevWeapon );

	Reset();

	m_pCvarDrawHistoryTime = CVAR_CREATE( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
	m_pCvarHudFastSwitch = CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress

	m_iFlags |= HUD_ACTIVE; //!!!

	gWR.Init();
	gHR.Init();

	m_pWeapon = nullptr;

	return 1;
}

void CHudAmmo::Reset()
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();

	//VidInit();
	wrect_t nullrc = {0,};
	SetCrosshair( 0, nullrc, 0, 0, 0 ); // reset crosshair
	m_pWeapon = NULL; // reset last weapon
}

int CHudAmmo::VidInit()
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );

	char bucketName[8] = "bucket";
	for (unsigned int i=0; i<ARRAYSIZE(m_HUD_buckets); ++i)
	{
		bucketName[6] = '0' + i + 1;
		bucketName[7] = '\0';
		m_HUD_buckets[i] = gHUD.GetSpriteIndex(bucketName);
	}
	m_HUD_bucket0 = m_HUD_buckets[0];
	m_HUD_bucket_none = gHUD.GetSpriteIndex( "bucket0" );
	m_HUD_divider = gHUD.GetSpriteIndex( "divider" );

	if (*gHUD.clientFeatures.bucket_slot_sprite)
	{
		m_HUD_bucketSlot = gHUD.GetSpriteIndex(gHUD.clientFeatures.bucket_slot_sprite);
	}
	else
	{
		m_HUD_bucketSlot = -1;
	}

	ghsprBuckets = gHUD.GetSprite( m_HUD_bucket0 );
	giBucketWidth = gHUD.GetSpriteRect( m_HUD_bucket0 ).right - gHUD.GetSpriteRect( m_HUD_bucket0 ).left;
	giBucketHeight = gHUD.GetSpriteRect( m_HUD_bucket0 ).bottom - gHUD.GetSpriteRect( m_HUD_bucket0 ).top;

	gHR.iHistoryGap = gHUD.GetSpriteRect( m_HUD_bucket0 ).bottom - gHUD.GetSpriteRect( m_HUD_bucket0 ).top;

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	const int res = GetSpriteRes( ScreenWidth, ScreenHeight );
	int factor;
	if( res >= 2560 )
		factor = 4;
	else if( res >= 1280 )
		factor = 3;
	else if( res >= 640 )
		factor = 2;
	else
		factor = 1;

	giABWidth = 10 * factor;
	giABHeight = 2 * factor;

	m_pWeapon = nullptr;

	return 1;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think()
{
	if( gHUD.m_fPlayerDead )
		return;

	if( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for( int i = MAX_WEAPONS-1; i > 0; i-- )
		{
			WEAPON *p = gWR.GetWeapon( i );

			if( p && p->iId )
			{
				if( gHUD.HasWeapon(p->iId) )
					gWR.PickupWeapon( p );
				else
					gWR.DropWeapon( p );
			}
		}
	}

	if( !gpActiveSel )
		return;

	// has the player selected one?
	if( gHUD.m_iKeyBits & IN_ATTACK )
	{
		if( gpActiveSel != (WEAPON *) 1 )
		{
			ServerCmd( gpActiveSel->szName );
			g_weaponselect = gpActiveSel->iId;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound( "common/wpn_select.wav", 1 );
	}

}

//
// Helper function to return a Ammo pointer from id
//
HSPRITE* WeaponsResource::GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect )
{
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if( rgWeapons[i].iAmmoType == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if( rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}

	return NULL;
}

// Menu Selection Code
void WeaponsResource::SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if( gHUD.m_Menu.m_fMenuDisplayed && ( fAdvance == 0 ) && ( iDirection == 1 ) )
	{
		// menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if( iSlot > m_maxWeaponSlots )
		return;

	if( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	if ( !gHUD.HasSuit() && !gHUD.DrawHUDNoSuit() )
		return;

	if( !gHUD.HasAnyWeapons() )
		return;

	WEAPON *p = NULL;
	bool fastSwitch = gHUD.m_Ammo.FastSwitchEnabled();

	if ( ( gpActiveSel == NULL ) || ( gpActiveSel == (WEAPON *) 1 ) || ( iSlot != gpActiveSel->iSlot ) )
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		p = GetFirstPos( iSlot );

		if ( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );
			if ( !p2 )
			{
				// only one active item in bucket, so change directly to weapon
				ServerCmd( p->szName );
				g_weaponselect = p->iId;
				return;
			}
		}
	}
	else
	{
		PlaySound( "common/wpn_moveselect.wav", 1 );
		if ( gpActiveSel )
			p = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if ( !p )
			p = GetFirstPos( iSlot );
	}

	
	if ( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if ( !fastSwitch )
			gpActiveSel = (WEAPON *)1;
		else
			gpActiveSel = NULL;
	}
	else 
		gpActiveSel = p;
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iIndex = READ_BYTE();
	int iCount = READ_SHORT();

	gWR.SetAmmo( iIndex, abs( iCount ) );

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();
	int iCount = READ_SHORT();

	// Add ammo to the history
	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs( iCount ) );

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_ITEM, szName );

	return 1;
}

int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if( gEngfuncs.IsSpectateOnly() )
		return 1;

	if( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		wrect_t nullrc = {0,};
		gpActiveSel = NULL;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
	}
	else
	{
		if( m_pWeapon )
		{
			const int crosshairColor = gHUD.GetCrosshairColor();
			int r, g, b;
			UnpackRGB(r, g, b, crosshairColor);
			SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, r, g, b );
		}
	}

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon( const char *pszName, int iSize, void *pbuf )
{
	wrect_t nullrc = {0,};
	bool fOnTarget = false;

	BEGIN_READ( pbuf, iSize );

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_SHORT();
	int iMaxClip = READ_SHORT();
	bool inAltMode = READ_BYTE() != 0;

	// detect if we're also on target
	if( iState > 1 )
	{
		fOnTarget = true;
	}

	if( iId < 1 )
	{
		SetCrosshair( 0, nullrc, 0, 0, 0 );
		// Clear out the weapon so we don't keep drawing the last active weapon's ammo. - Solokiller
		m_pWeapon = 0;
		return 0;
	}

	if( g_iUser1 != OBS_IN_EYE )
	{
		// Is player dead???
		if( ( iId == -1 ) && ( iClip == -1 ) )
		{
			gHUD.m_fPlayerDead = true;
			gpActiveSel = NULL;
			return 1;
		}
		gHUD.m_fPlayerDead = false;
	}

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if( !pWeapon )
		return 0;

	bool updateFade = (pWeapon->iClip != abs(iClip)) || pWeapon->iMaxClip != iMaxClip;

	if( iClip < -1 )
		pWeapon->iClip = abs( iClip );
	else
		pWeapon->iClip = iClip;

	pWeapon->iMaxClip = iMaxClip;
	pWeapon->inAltMode = inAltMode;

	if( iState == 0 )	// we're not the current weapon, so update no more
		return 1;

	if (!updateFade)
		updateFade = m_pWeapon != pWeapon;

	m_pWeapon = pWeapon;

	if( !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
	{
		int crosshairColor = gHUD.GetCrosshairColor();
		int r,g,b;
		UnpackRGB(r,g,b,crosshairColor);

		auto SetCurrentCrosshair = [this, fOnTarget, r, g, b](HSPRITE hCrosshair, wrect_t rcCrosshair, HSPRITE hAutoaim, wrect_t rcAutoaim)
		{
			if (fOnTarget && hAutoaim)
				SetCrosshair(hAutoaim, rcAutoaim, r, g, b);
			else
				SetCrosshair(hCrosshair, rcCrosshair, r, g, b);
		};

		if (!gHUD.ShouldUseZoomedCrosshair())
		{
			if (inAltMode)
			{
				SetCurrentCrosshair(m_pWeapon->hCrosshairAlt, m_pWeapon->rcCrosshairAlt, m_pWeapon->hAutoaimAlt, m_pWeapon->rcAutoaimAlt);
			}
			else
			{
				SetCurrentCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, m_pWeapon->hAutoaim, m_pWeapon->rcAutoaim);
			}
		}
		else
		{
			SetCurrentCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, m_pWeapon->hZoomedAutoaim, m_pWeapon->rcZoomedAutoaim);
		}
	}

	if (updateFade || !m_fFade)
		m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
	
	return 1;
}

//
// WeaponList -- Tells the hud about a new weapon type.
//
int CHudAmmo::MsgFunc_AmmoList( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const char* ammoName = READ_STRING();
	int maxAmmo = READ_SHORT();
	int idAndExhaustibleByte = READ_CHAR();
	bool exhaustible = (idAndExhaustibleByte & AMMO_EXHAUSTIBLE_NETWORK_BIT) != 0;
	int id = idAndExhaustibleByte & ~AMMO_EXHAUSTIBLE_NETWORK_BIT;
	g_AmmoRegistry.RegisterOnClient(ammoName, maxAmmo, id, exhaustible);

	return 1;
}

int CHudAmmo::MsgFunc_WeaponList( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	WEAPON Weapon;

	strncpyEnsureTermination( Weapon.szName, READ_STRING(), sizeof(Weapon.szName) );

	Weapon.iAmmoType = (int)READ_CHAR();	
	Weapon.iAmmo2Type = READ_CHAR();
	Weapon.iSlot = READ_CHAR();
	Weapon.iSlotPos = READ_CHAR();
	Weapon.iId = READ_CHAR();
	Weapon.iFlags = READ_BYTE();
	Weapon.iClip = 0;
	Weapon.iMaxClip = 0;
	Weapon.inAltMode = false;

	if( Weapon.iId < 0 || Weapon.iId >= MAX_WEAPONS )
		return 0;
	if( Weapon.iSlot < 0 || Weapon.iSlot >= WEAPON_SLOTS_HARDLIMIT + 1 )
		return 0;
	if( Weapon.iSlotPos < 0 || Weapon.iSlotPos >= MAX_WEAPON_POSITIONS + 1 )
		return 0;
	if( Weapon.iAmmoType < 0 || Weapon.iAmmoType >= MAX_AMMO_TYPES )
		return 0;
	if( Weapon.iAmmo2Type < 0 || Weapon.iAmmo2Type >= MAX_AMMO_TYPES )
		return 0;

	gWR.AddWeapon( &Weapon );

	if (Weapon.iSlot >= gWR.m_maxWeaponSlots)
		gWR.m_maxWeaponSlots = Weapon.iSlot + 1;

	return 1;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
#if USE_VGUI
	// Let the Viewport use it first, for menus
	if( gViewPort && gViewPort->SlotInput( iSlot ) )
		return;
#endif
	gWR.SelectSlot(iSlot, 0, 1);
}

void CHudAmmo::UserCmd_Slot1()
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2()
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3()
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4()
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5()
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6()
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7()
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8()
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9()
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10()
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Close()
{
	if( gpActiveSel )
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound( "common/wpn_hudoff.wav", 1 );
	}
	else
		ClientCmd( "escape" );
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon()
{
	if( gHUD.m_fPlayerDead || ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		return;

	if( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos + 1;
		slot = gpActiveSel->iSlot;
	}

	for( int loop = 0; loop <= 1; loop++ )
	{
		for( ; slot < gWR.m_maxWeaponSlots; slot++ )
		{
			for( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if( wsp && gWR.HasAmmo( wsp ) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon()
{
	if( gHUD.m_fPlayerDead || ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		return;

	if( !gpActiveSel || gpActiveSel == (WEAPON*) 1 )
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS - 1;
	int slot = gWR.m_maxWeaponSlots - 1;
	if( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos - 1;
		slot = gpActiveSel->iSlot;
	}
	
	for( int loop = 0; loop <= 1; loop++ )
	{
		for( ; slot >= 0; slot-- )
		{
			for( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if( wsp && gWR.HasAmmo( wsp ) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS - 1;
		}
		
		slot = gWR.m_maxWeaponSlots - 1;
	}

	gpActiveSel = NULL;
}

//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------
int CHudAmmo::Draw( float flTime )
{
	if( !gHUD.HasSuit() && !gHUD.DrawHUDNoSuit() )
		return 1;

	if( ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		return 1;

	// Draw Weapon Menu
	DrawWList( flTime );

	// Draw ammo pickup history
	gHR.DrawAmmoHistory( flTime );

	if( !( m_iFlags & HUD_ACTIVE ) )
		return 0;

	if( !m_pWeapon )
		return 0;

	WEAPON *pw = m_pWeapon; // shorthand

	// SPR_Draw Ammo
	if ((pw->iAmmoType <= 0 && pw->iAmmo2Type <= 0 && pw->iMaxClip <= 0) && (pw->inAltMode == false && !pw->hMode) && (pw->inAltMode == true && !pw->hAltMode))
		return 0;

	int iFlags = DHN_DRAWZERO; // draw 0 values

	const bool useDividerSprite = gHUD.clientFeatures.use_divider_sprite && m_HUD_divider != -1;
	const int AmmoWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
	const int iBarWidth = useDividerSprite ? (gHUD.GetSpriteRect(m_HUD_divider).right - gHUD.GetSpriteRect(m_HUD_divider).left) : AmmoWidth / 10;

	int x, y, hudR, hudG, hudB;

	const int a = (int)Q_max(gHUD.MinHUDAlpha(), m_fFade);
	UnpackRGB(hudR, hudG, hudB, gHUD.HUDColor());

	int scaledR = hudR;
	int scaledG = hudG;
	int scaledB = hudB;
	ScaleColors(scaledR, scaledG, scaledB, a);

	if( m_fFade > 0 )
		m_fFade -= ( (float)gHUD.m_flTimeDelta * 20.0f );

	// Does this weapon have a clip?
	y = CHud::Renderer().PerceviedScreenHeight() - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	y += gHUD.m_iHudNumbersYOffset; // a1ba: fix HL25 HUD vertical inconsistensy

	auto DrawDividerBar = [&](int x, int y, int r, int g, int b, int a)
	{
		if (useDividerSprite)
		{
			const wrect_t& rect = gHUD.GetSpriteRect(m_HUD_divider);
			const int dividerY = y + gHUD.m_iFontHeight / 2 - (rect.bottom - rect.top) / 2;
			CHud::Renderer().SPR_DrawAdditiveWithAlphaScale(gHUD.GetSprite(m_HUD_divider), r, g, b, a, x, dividerY, &rect);
		}
		else
		{
			CHud::Renderer().FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a);
		}
	};

	HSPRITE modeSprite = pw->inAltMode ? m_pWeapon->hAltMode : m_pWeapon->hMode;
	wrect_t modeRect = pw->inAltMode ? m_pWeapon->rcAltMode : m_pWeapon->rcMode;

	auto DrawWeaponMode = [&](int x, int y)
	{
		int r, g, b;
		if (gHUD.clientFeatures.weaponmode_uses_hud_color)
		{
			r = hudR;
			g = hudG;
			b = hudB;
		}
		else
		{
			r = g = b = 255;
		}
		CHud::Renderer().SPR_DrawAuto(modeSprite, r, g, b, x, y, &modeRect);
	};

	if (pw->iAmmoType > 0)
	{
		// Does weapon have any ammo at all?
		const AmmoType* ammoType = g_AmmoRegistry.GetByIndex(m_pWeapon->iAmmoType);

		int rightSideValue = 0;
		int rightSideMaxValue = -1;
		if (ammoType)
		{
			rightSideValue = gWR.CountAmmo(pw->iAmmoType);
			rightSideMaxValue = GetClientMaxAmmo(ammoType->id);
		}
		else if (pw->iMaxClip > 0)
		{
			rightSideMaxValue = rightSideValue = pw->iMaxClip;
		}

		if (rightSideMaxValue >= 0)
		{
			int ammoWidths = 8;
			int drawNumberFlag = DHN_3DIGITS;

			if (rightSideMaxValue >= 1000)
			{
				ammoWidths++;
				drawNumberFlag |= DHN_4DIGITS;
			}

			const int iIconWidth = m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left;
			int startX = 0;

			if (pw->iClip >= 0)
			{
				int drawNumberClipFlag = DHN_3DIGITS;
				if (m_pWeapon->iClip >= 1000) {
					ammoWidths++;
					drawNumberClipFlag |= DHN_4DIGITS;
				}

				// room for the number and the '|' and the current ammo
				x = startX = CHud::Renderer().PerceviedScreenWidth() - ammoWidths * AmmoWidth - iIconWidth;
				x = gHUD.DrawHudNumber( x, y, iFlags | drawNumberClipFlag, pw->iClip, scaledR, scaledG, scaledB );

				x += AmmoWidth / 2;
				DrawDividerBar(x, y, hudR, hudG, hudB, a);
				x += iBarWidth + AmmoWidth / 2;

				x = gHUD.DrawHudNumber( x, y, iFlags | drawNumberFlag, rightSideValue, scaledR, scaledG, scaledB );
			}
			else
			{
				ammoWidths = 4;
				if (ammoType && rightSideMaxValue >= 1000) {
					ammoWidths++;
				}

				// SPR_Draw a bullets only line
				x = startX = CHud::Renderer().PerceviedScreenWidth() - ammoWidths * AmmoWidth - iIconWidth;
				x = gHUD.DrawHudNumber( x, y, iFlags | drawNumberFlag, gWR.CountAmmo( pw->iAmmoType ), scaledR, scaledG, scaledB );
			}

			// Draw the ammo Icon
			if (m_pWeapon->hAmmo)
			{
				int iOffset = ( m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top ) / 8;
				CHud::Renderer().SPR_DrawAdditive( m_pWeapon->hAmmo, scaledR, scaledG, scaledB, x, y - iOffset, &m_pWeapon->rcAmmo );
			}

			if (modeSprite)
			{
				int xMode = startX - iBarWidth;
				UnpackRGB(hudR,hudG,hudB, gHUD.HUDColor());
				DrawDividerBar(xMode, y, hudR, hudG, hudB, a);
				xMode -= AmmoWidth / 2 + (modeRect.right - modeRect.left);

				int yMode = CHud::Renderer().PerceviedScreenHeight() - (modeRect.bottom - modeRect.top) - gHUD.m_iFontHeight / 2;

				DrawWeaponMode(xMode, yMode);
			}
		}
	}
	else
	{
		if (modeSprite)
		{
			int xMode = CHud::Renderer().PerceviedScreenWidth() - (modeRect.right - modeRect.left) - AmmoWidth / 2;
			int yMode = CHud::Renderer().PerceviedScreenHeight() - (modeRect.bottom - modeRect.top) - gHUD.m_iFontHeight / 2;

			DrawWeaponMode(xMode, yMode);
		}
	}

	// Does weapon have seconday ammo?
	if (pw->iAmmo2Type > 0)
	{
		// Do we have secondary ammo?
		const AmmoType* ammo2Type = g_AmmoRegistry.GetByIndex(m_pWeapon->iAmmo2Type);
		if (ammo2Type && ( gWR.CountAmmo( pw->iAmmo2Type ) > 0 ))
		{
			const int iIconWidth = m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left;

			const int maxAmmo2 = GetClientMaxAmmo(ammo2Type->id);

			int ammoWidths = 4;
			int drawNumberFlag = DHN_3DIGITS;
			if (ammo2Type->maxAmmo >= maxAmmo2) {
				ammoWidths++;
				drawNumberFlag |= DHN_4DIGITS;
			}

			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight / 4;
			x = CHud::Renderer().PerceviedScreenWidth() - ammoWidths * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber( x, y, iFlags | drawNumberFlag, gWR.CountAmmo( pw->iAmmo2Type ), scaledR, scaledG, scaledB );

			// Draw the ammo Icon
			if (m_pWeapon->hAmmo2)
			{
				int iOffset = ( m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top) / 8;
				CHud::Renderer().SPR_DrawAdditive( m_pWeapon->hAmmo2, scaledR, scaledG, scaledB, x, y - iOffset, &m_pWeapon->rcAmmo2 );
			}
		}
	}
	return 1;
}

//
// Draws the ammo bar on the hud
//
int DrawBar( int x, int y, int width, int height, float f )
{
	int r, g, b;

	if( f < 0 )
		f = 0;
	if( f > 1 )
		f = 1;

	if( f )
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if( w <= 0 )
			w = 1;
		UnpackRGB( r, g, b, RGB_GREENISH );
		CHud::Renderer().FillRGBA( x, y, w, height, r, g, b, 255 );
		x += w;
		width -= w;
	}

	UnpackRGB( r, g, b, gHUD.HUDColor() );

	CHud::Renderer().FillRGBA( x, y, width, height, r, g, b, 128 );

	return ( x + width );
}

void DrawAmmoBar( WEAPON *p, int x, int y, int width, int height )
{
	if( !p )
		return;

	const AmmoType* ammoType = g_AmmoRegistry.GetByIndex(p->iAmmoType);
	if( ammoType )
	{
		if( !gWR.CountAmmo( p->iAmmoType ) )
			return;

		float f = (float)gWR.CountAmmo( p->iAmmoType ) / (float)GetClientMaxAmmo(ammoType->id);
		
		x = DrawBar( x, y, width, height, f );

		// Do we have secondary ammo too?
		const AmmoType* ammo2Type = g_AmmoRegistry.GetByIndex(p->iAmmo2Type);
		if( ammo2Type )
		{
			f = (float)gWR.CountAmmo( p->iAmmo2Type ) / (float)GetClientMaxAmmo(ammo2Type->id);

			x += 5; //!!!

			DrawBar( x, y, width, height, f );
		}
	}
}

//
// Draw Weapon Menu
//
int CHudAmmo::SpriteIndexForSlot(int iSlot)
{
	int result = -1;
	if (iSlot >=0 && iSlot < static_cast<int>(ARRAYSIZE(m_HUD_buckets)))
	{
		result = m_HUD_buckets[iSlot];
	}
	if (result == -1)
	{
		return m_HUD_bucket_none;
	}
	return result;
}

void CHudAmmo::DrawFillBucket(int x, int y, int r, int g, int b, int a)
{
	if (m_HUD_bucketSlot != -1)
	{
		CHud::Renderer().SPR_DrawAdditive(gHUD.GetSprite(m_HUD_bucketSlot), r, g, b, x, y, &gHUD.GetSpriteRect(m_HUD_bucketSlot));
	}
	else
	{
		CHud::Renderer().FillRGBA(x, y, giBucketWidth, giBucketHeight, r, g, b, a);
	}
}

int CHudAmmo::DrawWList( float flTime )
{
	int r, g, b, x, y, a, i;

	if( !gpActiveSel )
		return 0;

	int iActiveSlot;

	if( gpActiveSel == (WEAPON *) 1 )
		iActiveSlot = -1;	// current slot has no weapons
	else 
		iActiveSlot = gpActiveSel->iSlot;

	x = 10; //!!!
	y = 10; //!!!

	// Ensure that there are available choices in the active slot
	if( iActiveSlot > 0 )
	{
		if( !gWR.GetFirstPos( iActiveSlot ) )
		{
			gpActiveSel = (WEAPON *) 1;
			iActiveSlot = -1;
		}
	}

	// Draw top line
	for( i = 0; i < gWR.m_maxWeaponSlots; i++ )
	{
		int iWidth;

		UnpackRGB( r, g, b, gHUD.HUDColor() );

		if( iActiveSlot == i )
			a = 255;
		else
			a = 192;

		ScaleColors( r, g, b, 255 );

		const int HUD_bucket = SpriteIndexForSlot(i);

		// make active slot wide enough to accomodate gun pictures
		if( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( iActiveSlot );
			if( p )
				iWidth = p->rcActive.right - p->rcActive.left;
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		if ( HUD_bucket != -1 )
			CHud::Renderer().SPR_DrawAdditive( gHUD.GetSprite( HUD_bucket ), r, g, b, x, y, &gHUD.GetSpriteRect( HUD_bucket ) );
		else
			CHud::Renderer().FillRGBA( x, y, iWidth, giBucketHeight, r, g, b, 128 );
		
		x += iWidth + 5;
	}

	a = 128; //!!!
	x = 10;

	// Draw all of the buckets
	for( i = 0; i < gWR.m_maxWeaponSlots; i++ )
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );
			int iWidth = giBucketWidth;
			if( p )
				iWidth = p->rcActive.right - p->rcActive.left;

			for( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if( !p || !p->iId )
					continue;

				UnpackRGB( r, g, b, gHUD.HUDColor() );

				const auto& selectionRect = gHUD.GetSpriteRect(m_HUD_selection);
				bool hasDrawnSprite = false;

				// if active, then we must have ammo.
				if( gpActiveSel == p )
				{
					if (p->hActive)
					{
						CHud::Renderer().SPR_DrawAdditive( p->hActive, r, g, b, x, y, &p->rcActive );
						hasDrawnSprite = true;
					}
					else
						CHud::Renderer().FillRGBA(x, y, selectionRect.right - selectionRect.left, selectionRect.bottom - selectionRect.top, r, g, b, 80);
					CHud::Renderer().SPR_DrawAdditive( gHUD.GetSprite( m_HUD_selection ), r, g, b, x, y, &selectionRect );
				}
				else
				{
					// Draw Weapon if Red if no ammo
					if( gWR.HasAmmo( p ) )
						ScaleColors( r, g, b, 192 );
					else
					{
						UnpackRGB( r, g, b, gHUD.HUDColorCritical() );
						ScaleColors( r, g, b, 128 );
					}

					if (p->hInactive)
					{
						CHud::Renderer().SPR_DrawAdditive( p->hInactive, r, g, b, x, y, &p->rcInactive );
						hasDrawnSprite = true;
					}
					else
						CHud::Renderer().FillRGBA(x, y, selectionRect.right - selectionRect.left, selectionRect.bottom - selectionRect.top, r, g, b, 80);
				}

				// Draw Ammo Bar
				DrawAmmoBar( p, x + giABWidth / 2, y, giABWidth, giABHeight );
				
				int height = p->rcActive.bottom - p->rcActive.top;
				if (!hasDrawnSprite)
				{
					height = selectionRect.bottom - selectionRect.top;
				}
				y += height + 5;
			}

			x += iWidth + 5;
		}
		else
		{
			// Draw Row of weapons.
			UnpackRGB( r, g, b, gHUD.HUDColor() );

			for( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( i, iPos );

				if( !p || !p->iId )
					continue;

				if( gWR.HasAmmo( p ) )
				{
					UnpackRGB( r, g, b, gHUD.HUDColor() );
					a = 128;
				}
				else
				{
					UnpackRGB( r, g, b, gHUD.HUDColorCritical() );
					a = 96;
				}

				DrawFillBucket(x, y, r, g, b, a);

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}

	return 1;
}

float CHudAmmo::DrawHistoryTime()
{
	return m_pCvarDrawHistoryTime && m_pCvarDrawHistoryTime->value > 0 ? m_pCvarDrawHistoryTime->value : 5;
}

bool CHudAmmo::FastSwitchEnabled()
{
	return m_pCvarHudFastSwitch && m_pCvarHudFastSwitch->value ? true : false;
}

/* =================================
	GetSpriteList

Finds and returns the matching 
sprite name 'psz' and resolution 'iRes'
in the given sprite list 'pList'
iCount is the number of items in the pList
================================= */
client_sprite_t *GetSpriteList( client_sprite_t *pList, const char *psz, int iRes, int iCount )
{
	if( !pList )
		return NULL;

	int i = iCount;
	client_sprite_t *p = pList;

	while( i-- )
	{
		if( p->iRes == iRes && !strcmp( psz, p->szName ))
			return p;
		p++;
	}

	return NULL;
}
