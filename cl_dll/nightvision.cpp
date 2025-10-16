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

#if FEATURE_CS_NIGHTVISION
extern cvar_t *cl_nvgradius_cs;
#endif

#if FEATURE_OPFOR_NIGHTVISION
extern cvar_t *cl_nvgradius_of;
#endif

DECLARE_MESSAGE( m_Nightvision, Nightvision )

#define NIGHTVISION_SPRITE_NAME "sprites/of_nv_b.spr"

int CHudNightvision::Init()
{
	m_fOn = false;

	HOOK_MESSAGE(Nightvision);

	m_iFlags |= HUD_ACTIVE;

#if FEATURE_CS_NIGHTVISION
	m_pLightCS = 0;
#endif

#if FEATURE_OPFOR_NIGHTVISION
	m_pLightOF = 0;
#endif

	//gHUD.AddHudElem(this);

	return 1;
}

void CHudNightvision::Reset()
{
	m_fOn = false;
}

int CHudNightvision::VidInit()
{
#if FEATURE_OPFOR_NIGHTVISION
	if (gHUD.clientFeatures.nvgstyle.configurable || gHUD.clientFeatures.nvgstyle.defaultValue == 0)
	{
		m_hSprite = LoadSprite(NIGHTVISION_SPRITE_NAME);

		// Get the number of frames available in this sprite.
		m_nFrameCount = SPR_Frames(m_hSprite);
	}

	// current frame.
	m_iFrame = 0;
#endif
	return 1;
}


int CHudNightvision::MsgFunc_Nightvision(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE() != 0;
	if (!m_fOn) {
		RemoveCSdlight();
		RemoveOFdlight();
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
	if (!gHUD.HasNVG())
		return 1;

	if (m_fOn) {
		int nvgStyle = gHUD.NVGStyle();
		if (nvgStyle == 0)
		{
			RemoveCSdlight();
			DrawOpforNVG(flTime);
		}
		else
		{
			RemoveOFdlight();
			DrawCSNVG(flTime);
		}
	}
	return 1;
}

void CHudNightvision::DrawCSNVG(float flTime)
{
#if FEATURE_CS_NIGHTVISION
	const NVGFeatures& nvg_cs = gHUD.clientFeatures.nvg_cs;
	int r, g, b;
	UnpackRGB(r, g, b, nvg_cs.layer_color);
	gEngfuncs.pfnFillRGBABlend(0, 0, ScreenWidth, ScreenHeight, r, g, b, nvg_cs.layer_alpha);

	if( !m_pLightCS || m_pLightCS->die < flTime )
	{
		UnpackRGB(r, g, b, gHUD.clientFeatures.nvg_cs.light_color);
		m_pLightCS = MakeDynLight(flTime, r, g, b);
	}
	UpdateDynLight( m_pLightCS, CSNvgRadius(), gHUD.m_vecOrigin );
#endif
}

void CHudNightvision::DrawOpforNVG(float flTime)
{
#if FEATURE_OPFOR_NIGHTVISION
	int r, g, b, x, y;

	const NVGFeatures& nvg_opfor = gHUD.clientFeatures.nvg_opfor;
	UnpackRGB(r, g, b, nvg_opfor.layer_color);
	int a = nvg_opfor.layer_alpha;

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
			SPR_DrawAdditive(m_iFrame, x + (j * 256), y + (i * 256), NULL);
		}
	}

	// Increase sprite frame.
	m_iFrame++;

	if( !m_pLightOF || m_pLightOF->die < flTime )
	{
		UnpackRGB(r, g, b, gHUD.clientFeatures.nvg_opfor.light_color);
		m_pLightOF = MakeDynLight(flTime, r, g, b);
	}
	UpdateDynLight( m_pLightOF, OpforNvgRadius(), gHUD.m_vecOrigin + Vector(0.0f, 0.0f, 32.0f ) );
#endif
}

dlight_t* CHudNightvision::MakeDynLight(float flTime, int r, int g, int b)
{
	dlight_t* dLight = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );

	// I hope no one is crazy so much to keep NVG for 9999 seconds
	dLight->die = flTime + 9999.0f;
	dLight->color.r = r;
	dLight->color.g = g;
	dLight->color.b = b;

	return dLight;
}

void CHudNightvision::UpdateDynLight(dlight_t *dynLight, float radius, const Vector &origin)
{
	if( dynLight )
	{
		dynLight->origin = origin;
		dynLight->radius = radius;
	}
}

void CHudNightvision::RemoveCSdlight()
{
#if FEATURE_CS_NIGHTVISION
	if( m_pLightCS )
	{
		m_pLightCS->die = 0;
		m_pLightCS = NULL;
	}
#endif
}

void CHudNightvision::RemoveOFdlight()
{
#if FEATURE_OPFOR_NIGHTVISION
	if( m_pLightOF )
	{
		m_pLightOF->die = 0;
		m_pLightOF = NULL;
	}
#endif
}

#if FEATURE_CS_NIGHTVISION
float CHudNightvision::CSNvgRadius()
{
	const NVGFeatures& nvg = gHUD.clientFeatures.nvg_cs;
	float radius = cl_nvgradius_cs && cl_nvgradius_cs->value > 0.0f ? cl_nvgradius_cs->value : nvg.radius.defaultValue;
	if (radius < nvg.radius.minValue)
		return nvg.radius.minValue;
	else if (radius > nvg.radius.maxValue)
		return nvg.radius.minValue;
	return radius;
}
#endif

#if FEATURE_OPFOR_NIGHTVISION
float CHudNightvision::OpforNvgRadius()
{
	const NVGFeatures& nvg = gHUD.clientFeatures.nvg_opfor;
	float radius = cl_nvgradius_of && cl_nvgradius_of->value > 0.0f ? cl_nvgradius_of->value : nvg.radius.defaultValue;
	if (radius < nvg.radius.minValue)
		return nvg.radius.minValue;
	else if (radius > nvg.radius.maxValue)
		return nvg.radius.minValue;
	return radius;
}
#endif

bool CHudNightvision::IsOn()
{
	return m_fOn;
}
