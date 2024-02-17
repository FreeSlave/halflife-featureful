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
#pragma once
#if !defined(__AMMO_H__)
#define __AMMO_H__

#include "common_limits.h"

#define WEAPON_FLAGS_SELECTONEMPTY	1

#define WEAPON_IS_ONTARGET 0x40

#include "wrect.h"
#include "cdll_int.h"

struct SpriteRectPair
{
	SpriteRectPair();
	HSPRITE sprite;
	wrect_t rect;
};

struct CrosshairSpriteData
{
	SpriteRectPair crosshair;
	SpriteRectPair crosshair_1280;
	SpriteRectPair crosshair_2560;
};

struct WEAPON
{
	char	szName[MAX_WEAPON_NAME];
	int		iAmmoType;
	int		iAmmo2Type;
	int		iSlot;
	int		iSlotPos;
	int		iFlags;
	int		iId;
	int		iClip;

	int		iCount;		// # of itesm in plist

	HSPRITE hActive;
	wrect_t rcActive;
	HSPRITE hInactive;
	wrect_t rcInactive;
	HSPRITE	hAmmo;
	wrect_t rcAmmo;
	HSPRITE hAmmo2;
	wrect_t rcAmmo2;

	CrosshairSpriteData crosshair;
	CrosshairSpriteData autoaim;
	CrosshairSpriteData zoomed;
	CrosshairSpriteData zoomedAutoaim;

	bool HasAutoaimCrosshair() const;
	bool HasZoomedAutoaimCrosshair() const;
};

typedef int AMMO;
#endif
