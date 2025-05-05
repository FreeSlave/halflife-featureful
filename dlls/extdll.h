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
#if !defined(EXTDLL_H)
#define EXTDLL_H
#include "build.h"
//
// Global header file for extension DLLs
//

// Allow "DEBUG" in addition to default "_DEBUG"
#if _DEBUG
#define DEBUG 1
#endif

// Silence certain warnings
#if _MSC_VER
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4514)		// unreferenced inline function removed
#pragma warning(disable : 4100)		// unreferenced formal parameter
#endif

// Prevent tons of unused windows definitions
#if XASH_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOMINMAX
#define HSPRITE HSPRITE_win32
#include <windows.h>
#undef HSPRITE
#else // _WIN32
#include <climits>
#include <cstdarg>
typedef unsigned int ULONG;
typedef unsigned char BYTE;
#define MAX_PATH PATH_MAX
#if !defined(PATH_MAX)
#define PATH_MAX 4096
#endif
#endif //_WIN32

// Misc C-runtime library headers
#include <cstdio>
#include <cstdlib>
#include <cstddef>

#include "safe_snprintf.h"

#include <cmath>
#include "pi_constant.h"

// Vector class
#include "mathlib.h"

// Shared engine/DLL constants
#include "const.h"
#include "progdefs.h"
#include "edict.h"

// Shared header describing protocol between engine and DLLs
#include "eiface.h"

// Shared header between the client DLL and the game DLLs
#include "cdll_dll.h"
#include "min_and_max.h"

#endif //EXTDLL_H
