//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma once
#ifndef EV_HLDMH
#define EV_HLDMH

#include "weapon_animations.h"

enum
{
	SMOKE_WALLPUFF = 0,
	SMOKE_RIFLE,
	SMOKE_PISTOL,
	SMOKE_BLACK
};

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
#endif // EV_HLDMH
