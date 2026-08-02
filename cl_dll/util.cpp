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
// util.cpp
//
// implementation of class-less helper functions
//

#include <cstdio>
#include <cstdlib>

#include "hud.h"
#include "cl_util.h"

int GetSpriteRes( int width, int height )
{
	int i;

	if( width < 640 )
		i = 320;
	else if( width < 1280 || !gHUD.m_pAllowHD->value )
		i = 640;
	else
	{
		if( height <= 720 )
			i = 640;
		else if( width <= 2560 || height <= 1600 )
			i = 1280;
		else
			i = 2560;
	}

	return Q_min( i, gHUD.m_iMaxRes );
}

HSPRITE LoadSprite( const char *pszName )
{
	int i = GetSpriteRes( ScreenWidth, ScreenHeight );
	char sz[256];

	sprintf( sz, pszName, i );

	return SPR_Load( sz );
}
