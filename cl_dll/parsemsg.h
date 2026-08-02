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
//  parsemsg.h
//
#pragma once
#if !defined(PARSEMSG_H)
#define PARSEMSG_H

#include "mathlib.h"
#include "common_types.h"

void BEGIN_READ( void *buf, int size );
int READ_CHAR();
int READ_BYTE();
int READ_SHORT();
int READ_WORD();
int READ_LONG();
float READ_FLOAT();
char* READ_STRING();
float READ_COORD();
Vector READ_VECTOR();
float READ_ANGLE();
float READ_HIRESANGLE();

color24 READ_COLOR();

#endif // PARSEMSG_H








