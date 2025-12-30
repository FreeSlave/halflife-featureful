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
//=========================================================
// skill.h - skill level concerns
//=========================================================
#pragma once
#if !defined(SKILL_H)
#define SKILL_H

struct EntTemplate;

#if CLIENT_DLL
inline float GetSkillValue(const char* name, const EntTemplate* entTemplate = nullptr, const char* entTemplateName = "", const EntTemplate* ownerEntTemplate = nullptr, const char* ownerEntTemplateName = "") { return 0.0f; }
#else
#include "util.h"

float GetSkillValue(const char* name, const EntTemplate* entTemplate = nullptr, const char* entTemplateName = "", const EntTemplate* ownerEntTemplate = nullptr, const char* ownerEntTemplateName = "");

extern DLL_GLOBAL int		g_iSkillLevel;

#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3
#endif

#endif // SKILL_H
