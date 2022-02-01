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
#include "dlight.h"
#include "r_efx.h"
#include "mod_features.h"

#include <string.h>
#include <stdio.h>

#if FEATURE_CS_NIGHTVISION && FEATURE_OPFOR_NIGHTVISION
extern cvar_t *cl_nvgstyle;
#endif

#if FEATURE_CS_NIGHTVISION
extern cvar_t *cl_nvgradius;
#endif

#define CS_NVG_MIN_DISTANCE 400
#define CS_NVG_MAX_DISTANCE 1000

DECLARE_MESSAGE( m_Nightvision, Nightvision )

#define NIGHTVISION_SPRITE_NAME "sprites/of_nv_b.spr"

int CHudNightvision::Init(void)
{
	m_fOn = 0;

	HOOK_MESSAGE(Nightvision);

	m_iFlags |= HUD_ACTIVE;

#if FEATURE_CS_NIGHTVISION
	m_pLight = 0;
#endif

	//gHUD.AddHudElem(this);

	return 1;
}

void CHudNightvision::Reset(void)
{
	m_fOn = 0;
}

int CHudNightvision::VidInit(void)
{
#if FEATURE_OPFOR_NIGHTVISION
	m_hSprite = LoadSprite(NIGHTVISION_SPRITE_NAME);

	// Get the number of frames available in this sprite.
	m_nFrameCount = SPR_Frames(m_hSprite);

	// current frame.
	m_iFrame = 0;
#endif
	return 1;
}


int CHudNightvision::MsgFunc_Nightvision(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	if (!m_fOn) {
		RemoveCSdlight();
	}

	return 1;
}

int CHudNightvision::Draw(float flTime)
{
	if( gEngfuncs.IsSpectateOnly() )
	{
		return 1;
	}

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_FLASHLIGHT | HIDEHUD_ALL))
		return 1;

	// Only display this if the player is equipped with the suit.
	if (!(gHUD.m_iWeaponBits & (1 << (WEAPON_SUIT))))
		return 1;

	if (m_fOn) {
#if FEATURE_CS_NIGHTVISION && FEATURE_OPFOR_NIGHTVISION
	if (!cl_nvgstyle || cl_nvgstyle->value == 0) {
		RemoveCSdlight();
		DrawOpforNVG(flTime);
	} else {
		DrawCSNVG(flTime);
	}
#elif FEATURE_CS_NIGHTVISION
	DrawCSNVG(flTime);
#elif FEATURE_OPFOR_NIGHTVISION
	DrawOpforNVG(flTime);
#endif
	}

	return 1;
}

void CHudNightvision::DrawCSNVG(float flTime)
{
#if FEATURE_CS_NIGHTVISION
	gEngfuncs.pfnFillRGBABlend(0, 0, ScreenWidth, ScreenHeight, 50, 225, 50, 110);
	if( !m_pLight || m_pLight->die < flTime )
	{
		m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );

		// I hope no one is crazy so much to keep NVG for 9999 seconds
		m_pLight->die = flTime + 9999.0f;
		m_pLight->color.r = 50;
		m_pLight->color.g = 255;
		m_pLight->color.b = 50;
	}
	// just update origin
	if( m_pLight )
	{
		m_pLight->origin = gHUD.m_vecOrigin;
		m_pLight->radius = CSNvgRadius();
	}
#endif
}

void CHudNightvision::DrawOpforNVG(float flTime)
{
#if FEATURE_OPFOR_NIGHTVISION
	int r, g, b, x, y;
	int a = 255;

	// Get each color component from the main
	// hud color.
	UnpackRGB(r, g, b, RGB_GREENISH);

	ScaleColors(r, g, b, a);

	// Top left of the screen.
	x = y = 0;

	// Reset the number of frame if we are at last frame.
	if (m_iFrame >= m_nFrameCount)
		m_iFrame = 0;

	const int nvgSpriteWidth = SPR_Width(m_hSprite, 0);
	const int nvgSpriteHeight = SPR_Height(m_hSprite, 0);

	const int colCount = (int)ceil(ScreenWidth / (float)nvgSpriteWidth);
	const int rowCount = (int)ceil(ScreenHeight / (float)nvgSpriteHeight);

	//
	// draw nightvision scanlines sprite.
	//
	SPR_Set(m_hSprite, r, g, b);

	int i, j;
	for (i = 0; i < rowCount; ++i) // height
	{
		for (j = 0; j < colCount; ++j) // width
		{
			// Nightvision sprites are 256 x 256. So draw 128 -> (8 * 16) instances to cover
			// the entire screen. It's cheap but does the work.
			//
			// Keep in mind this is used until we find a better solution.
			SPR_DrawAdditive(m_iFrame, x + (j * 256), y + (i * 256), NULL);
		}
	}

	// Increase sprite frame.
	m_iFrame++;
#endif
}

void CHudNightvision::RemoveCSdlight()
{
#if FEATURE_CS_NIGHTVISION
	if( m_pLight )
	{
		m_pLight->die = 0;
		m_pLight = NULL;
	}
#endif
}

#if FEATURE_CS_NIGHTVISION
float CHudNightvision::CSNvgRadius()
{
	float radius = cl_nvgradius && cl_nvgradius->value > 0.0f ? cl_nvgradius->value : 775;
	if (radius < CS_NVG_MIN_DISTANCE)
		return CS_NVG_MIN_DISTANCE;
	else if (radius > CS_NVG_MAX_DISTANCE)
		return CS_NVG_MAX_DISTANCE;
	return radius;
}
#endif

bool CHudNightvision::IsOn()
{
	return m_fOn;
}
