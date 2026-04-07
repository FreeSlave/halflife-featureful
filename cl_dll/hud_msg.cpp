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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "arraysize.h"
#include "string_utils.h"
#include "spritehint_flags.h"
#include "soundscripts.h"

#include "environment.h"

#define MAX_CLIENTS 32

extern BEAM *pBeam;
extern BEAM *pBeam2;
extern TEMPENTITY *pFlare;	// Vit_amiN

extern float g_lastFOV;			// Vit_amiN

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud::MsgFunc_ResetHUD( const char *pszName, int iSize, void *pbuf )
{
	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}
	m_Nightvision.Reset();

	m_iWeaponBits = 0ULL;
	m_iItemBits = 0;
	m_suppressedCapabilities = 0;
	m_onRope = false;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	// Vit_amiN: reset the FOV
	m_iFOV = 0;	// default_fov
	g_lastFOV = 0.0f;

	return 1;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	pFlare = NULL;	// Vit_amiN: clear egon's beam flare

	g_Environment.Initialize();
}

int CHud::MsgFunc_SetFog( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	fog.r = READ_BYTE();
	fog.g = READ_BYTE();
	fog.b = READ_BYTE();

	fog.fadeDuration = READ_SHORT();
	fog.startDist = READ_SHORT();

	if (fog.fadeDuration > 0)
	{
//		// fading in
		fog.finalEndDist = READ_SHORT();
		fog.endDist = FOG_LIMIT;
	}
	else if (fog.fadeDuration < 0)
	{
//		// fading out
		fog.finalEndDist = fog.endDist = READ_SHORT();
	}
	else
	{
		fog.endDist = READ_SHORT();
	}

	fog.density = READ_LONG() / 10000.0f;
	fog.type = READ_BYTE();
	fog.affectSkybox = READ_BYTE() != 0;

	return 1;
}

//LRC
int CHud::MsgFunc_KeyedDLight( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iKey = READ_SHORT();
	int bActive = READ_BYTE();
	if (!bActive)
	{
		// die instantly
		keyedDlightManager.RemoveDlight(iKey);
	}
	else if (bActive == 2)
	{
		keyedDlightManager.SetPosition(iKey, READ_VECTOR());
	}
	else
	{
		// never die
		dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( iKey );
		dl->die = gEngfuncs.GetClientTime() + (float)1E6;

		dl->origin = READ_VECTOR();
		dl->radius = READ_SHORT();
		dl->color = READ_COLOR();
		int entindex = READ_SHORT();
		keyedDlightManager.AddDlight(dl, entindex);
	}
	return 1;
}

int CHud::MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	if( m_Teamplay )
		ClientCmd( "richpresence_gamemode Teamplay\n" );
	else
		ClientCmd( "richpresence_gamemode\n" );
	ClientCmd( "richpresence_update\n" );
	return 1;
}

int CHud::MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	int r, g, b;
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if( m_iConcussionEffect )
	{
		UnpackRGB( r, g, b, HUDColor() );	// Vit_amiN: fixed
		this->m_StatusIcons.EnableIcon( "dmg_concuss", r, g, b );
	}
	else
		this->m_StatusIcons.DisableIcon( "dmg_concuss" );
	return 1;
}

int CHud::MsgFunc_ObjectHint(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	ObjectHint objectHint;

	int flags = READ_BYTE();
	if (flags & OBJECTHINT_FLAG_CLOSEST)
		objectHint.interactable = true;
	else
		objectHint.interactable = false;
	objectHint.entindex = READ_SHORT();

	if (objectHint.entindex > 0)
	{
		objectHint.color = READ_COLOR();
		objectHint.scaleFactor = READ_COORD();
		objectHint.center = READ_VECTOR();
		objectHint.size = READ_VECTOR();
		const char* spriteName = READ_STRING();
		strncpyEnsureTermination(objectHint.sprite, spriteName);

		if (m_pCvarObjectHint->value && m_pCvarDraw->value)
		{
			const bool shouldSet = m_pCvarObjectHint->value == 2 ? objectHint.interactable : true;
			if (shouldSet)
				objectHintManager.SetHint(objectHint);
		}
	}
	else if (objectHint.interactable)
	{
		objectHintManager.RemoveInteractable();
	}

	return 1;
}

int CHud::MsgFunc_PlTemplate(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_forcedHudColor = PackRGB(READ_COLOR());
	m_forcedHudColorNoSuit = PackRGB(READ_COLOR());
	m_forcedHudColorCritical = PackRGB(READ_COLOR());
	m_forcedHudDrawNoSuit = READ_BYTE();

	return 1;
}

const SoundScript* PM_GetPlayerSoundScript(int playerIndex, const char* name)
{
	return g_SoundScriptSystem.GetSoundScript(name);
}

int CHud::MsgFunc_SoundScript(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	std::string name = READ_STRING();

	SoundScript soundScript;

	int waveCount = READ_BYTE();
	for (int i=0; i<waveCount; ++i)
	{
		const char* wave = READ_STRING();
		soundScript.waves.push_back(g_SoundScriptSystem.RegisterWaveString(wave));
	}
	soundScript.channel = READ_BYTE();
	soundScript.volume.min = READ_BYTE() / 100.0f;
	soundScript.volume.max = READ_BYTE() / 100.0f;
	soundScript.attenuation = READ_BYTE() / 50.0f;
	soundScript.pitch.min = READ_BYTE();
	soundScript.pitch.max = READ_BYTE();

	g_SoundScriptSystem.ReplaceSoundScript(name.c_str(), soundScript);
	return 1;
}

int PM_GetSuppressedCapabilities(int playerIndex)
{
	return gHUD.m_suppressedCapabilities;
}

int CHud::MsgFunc_Capability(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_suppressedCapabilities = READ_LONG();
	return 1;
}

int CHud::MsgFunc_OnRope(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_onRope = READ_BYTE() != 0;
	return 1;
}

int CHud::MsgFunc_Mirror(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const bool enabled = READ_BYTE() != 0;
	const Vector mirrorCenter = READ_VECTOR();
	const float radius = READ_SHORT();
	const int type = READ_BYTE();

	bool bNew = true;

	for (FakeMirror& mirror : fakeMirrors)
	{
		if (mirror.origin == mirrorCenter)
		{
			mirror.enabled = enabled;
			mirror.origin = mirrorCenter;
			mirror.radius = radius;
			mirror.type = type;
			bNew = false;
		}
	}

	if (bNew)
	{
		gEngfuncs.Con_DPrintf("Registering a new mirror!\n");

		FakeMirror mirror;
		mirror.enabled = enabled;
		mirror.origin = mirrorCenter;
		mirror.radius = radius;
		mirror.type = type;
		fakeMirrors.push_back(mirror);
	}

	return 1;
}

int CHud::MsgFunc_Weapons( const char* pszName, int iSize, void* pbuf )
{
	BEGIN_READ(pbuf, iSize);

	const std::uint64_t lowerBits = READ_LONG();
	const std::uint64_t upperBits = READ_LONG();

	m_iWeaponBits = lowerBits | (upperBits << 32ULL);

	return 1;
}
