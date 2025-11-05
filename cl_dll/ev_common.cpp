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
// shared event functions

#include "hud.h"
#include "cl_util.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "r_efx.h"

#include "eventscripts.h"
#include "event_api.h"
#include "pm_shared.h"
#include "util_shared.h"

#define IS_FIRSTPERSON_SPEC ( g_iUser1 == OBS_IN_EYE || ( g_iUser1 && ( gHUD.m_Spectator.m_pip->value == INSET_IN_EYE ) ) )
/*
=================
GetEntity

Return's the requested cl_entity_t
=================
*/
struct cl_entity_s *GetEntity( int idx )
{
	return gEngfuncs.GetEntityByIndex( idx );
}

/*
=================
GetViewEntity

Return's the current weapon/view model
=================
*/
struct cl_entity_s *GetViewEntity()
{
	return gEngfuncs.GetViewModel();
}

/*
=================
EV_CreateTracer

Creates a tracer effect
=================
*/
void EV_CreateTracer( float *start, float *end )
{
	gEngfuncs.pEfxAPI->R_TracerEffect( start, end );
}

/*
=================
EV_IsPlayer

Is the entity's index in the player range?
=================
*/
qboolean EV_IsPlayer( int idx )
{
	if( idx >= 1 && idx <= gEngfuncs.GetMaxClients() )
		return true;

	return false;
}

/*
=================
EV_IsLocal

Is the entity == the local player
=================
*/
qboolean EV_IsLocal( int idx )
{
	// check if we are in some way in first person spec mode
	if( IS_FIRSTPERSON_SPEC )
		return ( g_iUser2 == idx );
	else
		return gEngfuncs.pEventAPI->EV_IsLocal( idx - 1 ) ? true : false;
}

/*
=================
EV_GetGunPosition

Figure out the height of the gun
=================
*/
void EV_GetGunPosition( event_args_t *args, float *pos, float *origin )
{
	int idx;
	Vector view_ofs;

	idx = args->entindex;

	VectorClear( view_ofs );
	view_ofs[2] = DEFAULT_VIEWHEIGHT;

	if( EV_IsPlayer( idx ) )
	{
		// in spec mode use entity viewheigh, not own
		if( EV_IsLocal( idx ) && !IS_FIRSTPERSON_SPEC )
		{
			// Grab predicted result for local player
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	VectorAdd( origin, view_ofs, pos );
}

/*
=================
EV_EjectBrass

Bullet shell casings
=================
*/
void EV_EjectBrass( float *origin, float *velocity, float rotation, int model, int soundtype )
{
	Vector endpos;
	VectorClear( endpos );
	endpos[1] = rotation;
	gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, endpos, 2.5, model, soundtype );
}

/*
=================
EV_GetDefaultShellInfo

Determine where to eject shells from
=================
*/
void EV_GetDefaultShellInfo( const struct event_args_s *args, const ShellInfoParams& infoParams, float *ShellVelocity, float *ShellOrigin )
{
	Vector view_ofs;

	int idx = args->entindex;

	VectorClear( view_ofs );
	view_ofs[2] = DEFAULT_VIEWHEIGHT;

	if( EV_IsPlayer( idx ) )
	{
		if( EV_IsLocal( idx ) )
		{
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	const float fR = RandomizeNumberFromRange(infoParams.sideFactor);
	const float fU = RandomizeNumberFromRange(infoParams.upFactor);
	const float fF = RandomizeNumberFromRange(infoParams.forwardFactor);

	for( int i = 0; i < 3; i++ )
	{
		ShellVelocity[i] = infoParams.velocity[i] + infoParams.right[i] * fR + infoParams.up[i] * fU + infoParams.forward[i] * fF;

		if (infoParams.attachment > 0 && infoParams.attachment <= 4)
		{
			cl_entity_t *ent = GetViewEntity();
			if (ent)
				ShellOrigin[i] = ent->attachment[infoParams.attachment - 1][i];
		}
		else
		{
			ShellOrigin[i] = infoParams.origin[i] + view_ofs[i] + infoParams.up[i] * infoParams.upScale + infoParams.forward[i] * infoParams.forwardScale + infoParams.right[i] * infoParams.rightScale;
		}
	}
}

/*
=================
EV_MuzzleFlash

Flag weapon/view model for muzzle flash
=================
*/
void EV_MuzzleFlash()
{
	// Add muzzle flash to current weapon model
	cl_entity_t *ent = GetViewEntity();
	if( !ent )
	{
		return;
	}

	// Or in the muzzle flash
	ent->curstate.effects |= EF_MUZZLEFLASH;
}

void EV_MuzzleLight(const Vector &vecForward)
{
	cl_entity_t *ent = GetViewEntity();
	if( !ent )
	{
		return;
	}

	if (gHUD.MuzzleLightEnabled())
	{
		Vector vecSrc = ent->origin + vecForward * 36;
		vecSrc.z += (ent->curstate.maxs.z - ent->curstate.mins.z)*0.4f;
		dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
		dl->origin[0] = vecSrc.x;
		dl->origin[1] = vecSrc.y;
		dl->origin[2] = vecSrc.z;
		dl->radius = 144;
		dl->color.r = 255;
		dl->color.g = 208;
		dl->color.b = 128;
		dl->decay = 512;
		dl->die = gEngfuncs.GetClientTime() + 0.04f;
	}
}
