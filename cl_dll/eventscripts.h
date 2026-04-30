//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// eventscripts.h
#pragma once
#if !defined ( EVENTSCRIPTSH )
#define EVENTSCRIPTSH

#include "vector.h"
#include "template_property_types.h"

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	28
#define VEC_DUCK_VIEW 12

struct ShellInfoParams
{
	Vector origin;
	Vector velocity;
	Vector forward;
	Vector up;
	Vector right;
	float forwardScale;
	float upScale;
	float rightScale;
	short attachment;
	FloatRange upFactor;
	FloatRange sideFactor;
	FloatRange forwardFactor;
};

// Some of these are HL/TFC specific?
void EV_EjectBrass( float *origin, float *velocity, float rotation, int model, int soundtype );
Vector EV_GetGunPosition(struct event_args_s *args, const Vector& origin);
void EV_GetDefaultShellInfo( const struct event_args_s *args, const ShellInfoParams& infoParams, float *ShellVelocity, float *ShellOrigin );
bool EV_IsLocal( int idx );
bool EV_IsPlayer( int idx );
void EV_CreateTracer( float *start, float *end );

struct cl_entity_s *GetEntity( int idx );
struct cl_entity_s *GetViewEntity();
void EV_MuzzleFlash();
void EV_MuzzleLight(const Vector &vecForward);
#endif // EVENTSCRIPTSH
