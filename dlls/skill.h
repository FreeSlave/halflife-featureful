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

#include "template_property_types.h"

struct EntTemplate;

#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3

extern int g_iSkillLevel;

#if CLIENT_DLL
inline FloatRange GetSkillValueRange(const char* name, const EntTemplate* entTemplate = nullptr, const char* entTemplateName = "", const EntTemplate* ownerEntTemplate = nullptr, const char* ownerEntTemplateName = "") { return 0.0f; }
inline float GetSkillValue(const char* name, const EntTemplate* entTemplate = nullptr, const char* entTemplateName = "", const EntTemplate* ownerEntTemplate = nullptr, const char* ownerEntTemplateName = "") { return 0.0f; }
#else
float RandomizeSkillValue(const FloatRange& range);

FloatRange GetSkillValueRange(const char* name, const EntTemplate* entTemplate = nullptr, const char* entTemplateName = "", const EntTemplate* ownerEntTemplate = nullptr, const char* ownerEntTemplateName = "");
float GetSkillValue(const char* name, const EntTemplate* entTemplate = nullptr, const char* entTemplateName = "", const EntTemplate* ownerEntTemplate = nullptr, const char* ownerEntTemplateName = "");
#endif

#endif // SKILL_H
