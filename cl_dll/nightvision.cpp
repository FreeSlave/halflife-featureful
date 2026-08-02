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
#include "clamp.h"
#include "lerp.h"

extern cvar_t *cl_nvgradius_cs;
extern cvar_t *cl_nvgradius_of;
extern cvar_t *cl_nvgfadetime;

DECLARE_MESSAGE( m_Nightvision, Nightvision )

#define NIGHTVISION_SPRITE_NAME "sprites/of_nv_b.spr"

int CHudNightvision::Init()
{
	m_fOn = false;
	m_progress = 0.0f;

	HOOK_MESSAGE(Nightvision);

	m_iFlags |= HUD_ACTIVE;

	m_pLightCS = nullptr;
	m_pLightOF = nullptr;

	//gHUD.AddHudElem(this);

	return 1;
}

void CHudNightvision::Reset()
{
	m_fOn = false;
}

int CHudNightvision::VidInit()
{
	if (gHUD.clientFeatures.nvgstyle.configurable || gHUD.clientFeatures.nvgstyle.defaultValue == 0)
	{
		m_hSprite = SPR_Load(NIGHTVISION_SPRITE_NAME);
		m_nFrameCount = SPR_Frames(m_hSprite);
	}

	m_iFrame = 0;
	return 1;
}


int CHudNightvision::MsgFunc_Nightvision(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE() != 0;

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

	const int nvgStyle = gHUD.NVGStyle();
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
	return 1;
}

void CHudNightvision::DrawCSNVG(float flTime)
{
	const float fadeTime = NvgFadeTime();
	if (fadeTime <= 0.0f && !IsOn())
	{
		RemoveCSdlight();
		return;
	}

	const NVGFeatures& nvg_cs = gHUD.clientFeatures.nvg_cs;
	int nvgAlpha = nvg_cs.layer_alpha;
	float nvgRadius = CSNvgRadius();

	if (fadeTime > 0.0f)
	{
		const float delta = gHUD.m_flTimeDelta / fadeTime;

		if (IsOn())
			m_progress += delta;
		else
			m_progress -= delta;
		m_progress = clamp(m_progress, 0.0f, 1.0f);

		if (m_progress == 0.0f)
		{
			RemoveCSdlight();
			return;
		}

		nvgAlpha = lerp(0.0f, (float)nvgAlpha, m_progress);
		nvgRadius = lerp(0.0f, nvgRadius, m_progress);
	}
	else
		m_progress = 1.0f;

	int r, g, b;
	UnpackRGB(r, g, b, nvg_cs.layer_color);

	gEngfuncs.pfnFillRGBABlend(0, 0, ScreenWidth, ScreenHeight, r, g, b, nvgAlpha);

	if (!m_pLightCS || m_pLightCS->die < flTime)
	{
		UnpackRGB(r, g, b, gHUD.clientFeatures.nvg_cs.light_color);
		m_pLightCS = MakeDynLight(flTime, r, g, b);
	}
	UpdateDynLight(m_pLightCS, nvgRadius, gHUD.m_vecOrigin);
}

void CHudNightvision::DrawOpforNVG(float flTime)
{
	const float fadeTime = NvgFadeTime();
	if (fadeTime <= 0.0f && !IsOn())
	{
		RemoveOFdlight();
		return;
	}

	const NVGFeatures& nvg_opfor = gHUD.clientFeatures.nvg_opfor;
	int nvgAlpha = nvg_opfor.layer_alpha;
	float nvgRadius = OpforNvgRadius();

	if (fadeTime > 0.0f)
	{
		const float delta = gHUD.m_flTimeDelta / fadeTime;

		if (IsOn())
			m_progress += delta;
		else
			m_progress -= delta;
		m_progress = clamp(m_progress, 0.0f, 1.0f);

		if (m_progress == 0.0f)
		{
			RemoveOFdlight();
			return;
		}

		nvgAlpha = lerp(0.0f, (float)nvgAlpha, m_progress);
		nvgRadius = lerp(0.0f, nvgRadius, m_progress);
	}
	else
		m_progress = 1.0f;

	int r, g, b, x, y;

	UnpackRGB(r, g, b, nvg_opfor.layer_color);
	ScaleColors(r, g, b, nvgAlpha);

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

	if (!m_pLightOF || m_pLightOF->die < flTime)
	{
		UnpackRGB(r, g, b, gHUD.clientFeatures.nvg_opfor.light_color);
		m_pLightOF = MakeDynLight(flTime, r, g, b);
	}
	UpdateDynLight(m_pLightOF, nvgRadius, gHUD.m_vecOrigin + Vector(0.0f, 0.0f, 32.0f));
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
	if (dynLight)
	{
		dynLight->origin = origin;
		dynLight->radius = radius;
	}
}

void CHudNightvision::RemoveCSdlight()
{
	if (m_pLightCS)
	{
		m_pLightCS->die = 0;
		m_pLightCS = nullptr;
	}
}

void CHudNightvision::RemoveOFdlight()
{
	if (m_pLightOF)
	{
		m_pLightOF->die = 0;
		m_pLightOF = nullptr;
	}
}

float CHudNightvision::CSNvgRadius()
{
	const NVGFeatures& nvg = gHUD.clientFeatures.nvg_cs;
	const float radius = cl_nvgradius_cs && cl_nvgradius_cs->value > 0.0f ? cl_nvgradius_cs->value : nvg.radius.defaultValue;
	return clamp(radius, (float)nvg.radius.minValue, (float)nvg.radius.maxValue);
}

float CHudNightvision::OpforNvgRadius()
{
	const NVGFeatures& nvg = gHUD.clientFeatures.nvg_opfor;
	const float radius = cl_nvgradius_of && cl_nvgradius_of->value > 0.0f ? cl_nvgradius_of->value : nvg.radius.defaultValue;
	return clamp(radius, (float)nvg.radius.minValue, (float)nvg.radius.maxValue);
}

float CHudNightvision::NvgFadeTime()
{
	const float fadeTime = cl_nvgfadetime ? cl_nvgfadetime->value : gHUD.clientFeatures.nvg_fade_time.defaultValue;
	return clamp(fadeTime, 0.0f, 2.0f);
}

bool CHudNightvision::IsOn() const
{
	return m_fOn;
}
