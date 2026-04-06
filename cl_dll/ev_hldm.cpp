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

#include "hud.h"
#include "cl_util.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"
#include "hl_events.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "cl_fx.h"

#include "r_studioint.h"
#include "com_model.h"
#include "tex_materials.h"

#include "weapon_ids.h"
#include "weapon_parameters.h"
#include "util_shared.h"

extern const WeaponParameters& GetWeaponParameters(int id);

extern engine_studio_api_t IEngineStudio;

static int g_tracerCount[MAX_PLAYERS];

#include "pm_shared.h"

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern cvar_t *cl_lw;
extern cvar_t *r_decals;

static bool DidHitSky(pmtrace_t *ptr, float *vecSrc, float *vecEnd)
{
	int entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );
	if( entity == 0 )
	{
		const char* pTextureName = gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		if( pTextureName && strcmp( pTextureName, "sky" ) == 0 )
		{
			return true;
		}
	}
	return false;
}

static int SelectFireAnimation(const WeaponParameters& params, bool altMode, bool empty)
{
	const WeaponParameters::FireAnimArray& anims = params.fire.anims.Get(altMode, empty);

	if (anims.size())
	{
		if (anims.size() == 1)
		{
			return anims.front();
		}
		else
		{
			return anims[gEngfuncs.pfnRandomLong(0, anims.size()-1)];
		}
	}
	return -1;
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker.
// returns volume of strike instrument (crowbar) to play
char EV_HLDM_GetTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, bool& isSky )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = g_MaterialRegistry.DefaultMaterial();

	int entity;
	cl_entity_t *ent;
	const char *pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//
	chTextureType = 0;
	isSky = false;

	// Player
	if( EV_IsPlayer(entity)
	    || ( ( ent = gEngfuncs.GetEntityByIndex( entity )) && ( ent->curstate.eflags & EFLAG_FLESH_SOUND )))
	{
		// hit body
		chTextureType = g_MaterialRegistry.FleshMaterial();
	}
	else
	{
		physent_t *pe = NULL;
		if (entity)
			pe = gEngfuncs.pEventAPI->EV_GetPhysent( ptr->ent );

		if (entity == 0 || (pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP )))
		{
			// get texture from entity or world (world is ent(0))
			pTextureName = gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );

			if ( pTextureName )
			{
				strcpy( texname, pTextureName );
				pTextureName = texname;

				if( strcmp( pTextureName, "sky" ) == 0 )
				{
					isSky = true;
				}

				GetStrippedTextureName(szbuffer, pTextureName);

				// get texture type
				chTextureType = PM_FindTextureType( szbuffer );
			}
		}
	}

	return chTextureType;
}

float EV_HLDM_PlayTextureSound( pmtrace_t *ptr, char chTextureType )
{
	const MaterialData* mData = g_MaterialRegistry.GetMaterialDataWithFallback(chTextureType);
	if (!mData || mData->hit.waves.empty())
		return 0.0f;

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, mData->hit.waves[gEngfuncs.pfnRandomLong(0, mData->hit.waves.size() - 1)].c_str(), mData->hit.volume, mData->hit.attn, 0, 96 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	return mData->hit.volume;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[32];
	int idx;

	if( pe->classnumber == 1 )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if( pe->rendermode != kRenderNormal )
	{
		strcpy( decalname, "{bproof1" );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );

	iRand = gEngfuncs.pfnRandomLong( 0, 0x7FFF );
	if( iRand < ( 0x7fff / 2 ) )// not every bullet makes a sound.
	{
		switch( iRand % 5 )
		{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 3:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 4:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		}
	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if(  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if( r_decals->value )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot(
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ),
				gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
		}
	}
}

void EV_WallPuff_Wind( struct tempent_s *te, float frametime, float currenttime )
{
	static bool xWindDirection = true;
	static bool yWindDirection = true;
	static float xWindMagnitude;
	static float yWindMagnitude;

	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin.x = 0.97 * te->entity.baseline.origin.x;
		te->entity.baseline.origin.y = 0.97 * te->entity.baseline.origin.y;
		te->entity.baseline.origin.z = 0.97 * te->entity.baseline.origin.z + 0.7;
		if ( te->entity.baseline.origin.z > 70.0 )
			te->entity.baseline.origin.z = 70.0;
	}

	if ( te->entity.curstate.frame > 6.0 )
	{
		xWindMagnitude += 0.075;
		if ( xWindMagnitude > 5.0 )
			xWindMagnitude = 5.0;

		yWindMagnitude += 0.075;
		if ( yWindMagnitude > 5.0 )
			yWindMagnitude = 5.0;

		if( xWindDirection )
			te->entity.baseline.origin.x += xWindMagnitude;
		else
			te->entity.baseline.origin.x -= xWindMagnitude;

		if( yWindDirection )
			te->entity.baseline.origin.y += yWindMagnitude;
		else
			te->entity.baseline.origin.y -= yWindMagnitude;

		if ( !Com_RandomLong(0, 10) && yWindMagnitude > 3.0 )
		{
			yWindMagnitude = 0;
			yWindDirection = !yWindDirection;
		}
		if ( !Com_RandomLong(0, 10) && xWindMagnitude > 3.0 )
		{
			xWindMagnitude = 0;
			xWindDirection = !xWindDirection;
		}
	}
}

void EV_SmokeRise( struct tempent_s *te, float frametime, float currenttime )
{
	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin = 0.97f * te->entity.baseline.origin;
		te->entity.baseline.origin.z += 0.7f;

		if( te->entity.baseline.origin.z > 70.0f )
			te->entity.baseline.origin.z = 70.0f;
	}
}

void EV_HugWalls(TEMPENTITY *te, pmtrace_s *ptr)
{
	Vector norm = te->entity.baseline.origin.Normalize();
	float len = te->entity.baseline.origin.Length();

	/*const Vector innerNormal = ptr->plane.normal;
	Vector v = CrossProduct(norm, innerNormal);
	Vector projection = CrossProduct(innerNormal, v);*/
	Vector projection = CrossProduct( CrossProduct(norm, ptr->plane.normal), ptr->plane.normal);

	/*if( len <= 2000.0f )
		len *= 1.5;
	else len = 3000.0f;*/

	te->entity.baseline.origin = projection * len;
}

static void EV_CreateShotSmoke(int type, Vector origin, Vector dir, int speed, float scale, int r, int g, int b, bool wind, const IntRange& wallpuffAlphaRange, Vector velocity = Vector(0,0,0), int framerate = 35)
{
	TEMPENTITY *te = NULL;
	void ( *callback )( struct tempent_s *ent, float frametime, float currenttime ) = NULL;
	model_t* wallPuffSprite = nullptr;

	switch( type )
	{
	case SMOKE_WALLPUFF:
		if (gHUD.wallPuffCount <= 0)
			return;
		wallPuffSprite = gHUD.wallPuffs[Com_RandomLong(0, gHUD.wallPuffCount-1)];
		break;
	default:
		gEngfuncs.Con_DPrintf("Unknown smoketype %d\n", type);
		return;
	}

	if( wind )
		callback = EV_WallPuff_Wind;
	else
		callback = EV_SmokeRise;


	te = gEngfuncs.pEfxAPI->CL_TempEntAlloc( origin, wallPuffSprite );

	if( te )
	{
		te->callback = callback;
		te->hitcallback = EV_HugWalls;
		te->flags |= FTENT_SPRANIMATE | FTENT_COLLIDEALL | FTENT_CLIENTCUSTOM;

		msprite_t* spriteDef = (msprite_t*)wallPuffSprite->cache.data;

		te->entity.curstate.rendermode = spriteDef->texFormat == SPR_INDEXALPHA ? kRenderTransAlpha : kRenderTransAdd;
		te->entity.curstate.rendercolor.r = r;
		te->entity.curstate.rendercolor.g = g;
		te->entity.curstate.rendercolor.b = b;
		te->entity.curstate.renderamt = RandomizeNumberFromRange(wallpuffAlphaRange);
		te->entity.curstate.scale = scale;
		te->entity.baseline.origin = speed * dir;
		te->entity.curstate.framerate = framerate;
		te->frameMax = wallPuffSprite->numframes;
		te->die = gEngfuncs.GetClientTime() + (float)te->frameMax / framerate;
		te->entity.curstate.frame = 0;

		if( velocity != Vector(0,0,0) )
		{
			velocity.x *= 0.9;
			velocity.y *= 0.9;
			velocity.z *= 0.5;
			te->entity.baseline.origin = te->entity.baseline.origin + velocity;
		}
	}
}

void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, char cTextureType = 0, bool isSky = false )
{
	physent_t *pe;

	if ( isSky )
		return;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if( pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );

		const MaterialData* mData = g_MaterialRegistry.GetMaterialDataWithFallback(cTextureType);

		if( mData && mData->hit.allowWeaponSparks && gHUD.WeaponSparksEnabled() )
		{
			Vector dir = pTrace->plane.normal;
			dir.x = dir.x * dir.x * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			dir.y = dir.y * dir.y * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			dir.z = dir.z * dir.z * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			gEngfuncs.pEfxAPI->R_StreakSplash( pTrace->endpos, dir, 4, Com_RandomLong( 5, 10 ), dir.z, -75.0f, 75.0f );
		}

		if (mData && mData->hit.allowWallpuff && gHUD.WeaponWallpuffEnabled())
		{
			const Color3 smoke = mData->hit.wallpuffColor;
			EV_CreateShotSmoke(SMOKE_WALLPUFF, pTrace->endpos + pTrace->plane.normal * 5, pTrace->plane.normal, 25, 0.5f, smoke.r, smoke.g, smoke.b, true, g_MaterialRegistry.GetWallpuffAlphaRange());
		}
	}
}

int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iTracerFreq, int *tracerCount )
{
	int tracer = 0;
	int i;
	bool player = EV_IsPlayer(idx);

	if( iTracerFreq != 0 && ( (*tracerCount)++ % iTracerFreq ) == 0 )
	{
		Vector vecTracerSrc;

		if( player )
		{
			Vector offset( 0, 0, -4 );

			// adjust tracer position for player
			for( i = 0; i < 3; i++ )
			{
				vecTracerSrc[i] = vecSrc[i] + offset[i] + right[i] * 2 + forward[i] * 16;
			}
		}
		else
		{
			VectorCopy( vecSrc, vecTracerSrc );
		}

		if( iTracerFreq != 1 )		// guns that always trace also always decal
			tracer = 1;

		EV_CreateTracer( vecTracerSrc, end );
	}

	return tracer;
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY )
{
	int i;
	pmtrace_t tr;
	int iShot;
	bool isSky;

	if( EV_IsLocal( idx ) )
	{
		EV_MuzzleLight(Vector(forward));
	}

	for( iShot = 1; iShot <= cShots; iShot++ )
	{
		Vector vecDir, vecEnd;
		float x, y, z;

		//We randomize for the Shotgun.
		if( cShots > 1 )
		{
			do{
				x = gEngfuncs.pfnRandomFloat( -0.5, 0.5 ) + gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
				y = gEngfuncs.pfnRandomFloat( -0.5, 0.5 ) + gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
				z = x * x + y * y;
			}while( z > 1 );

			for( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[i] + y * flSpreadY * up [i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}//But other guns already have their spread randomized in the synched spread.
		else
		{
			for( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[i] + flSpreadY * up [i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

		EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iTracerFreq, tracerCount );

		// do damage, paint decals
		if( tr.fraction != 1.0f )
		{
			bool shouldPlayTextureSound = iShot == 1;
			bool shouldPlayGunshotEffect= true;

			if ( shouldPlayTextureSound || shouldPlayGunshotEffect )
			{
				const char cTextureType = EV_HLDM_GetTextureSound( idx, &tr, vecSrc, vecEnd, isSky );
				if ( shouldPlayTextureSound )
				{
					EV_HLDM_PlayTextureSound(&tr, cTextureType);
				}
				if ( shouldPlayGunshotEffect )
				{
					EV_HLDM_DecalGunshot( &tr, cTextureType, isSky );
				}
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

static void EV_PlayWeaponSoundScript(int idx, Vector origin, const WeaponSoundScript& sound)
{
	const char* wave = sound.Wave();
	if (wave)
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, sound.channel, wave, RandomizeNumberFromRange(sound.volume), sound.attenuation, 0, RandomizeNumberFromRange(sound.pitch));
}

static void EV_StopWeaponSoundScript(int idx, const WeaponSoundScript& sound)
{
	for (auto& wave : sound.waves)
	{
		gEngfuncs.pEventAPI->EV_StopSound(idx, sound.channel, wave);
	}
}

int g_iSwing;
bool g_primaryLoopedPlaying = false;
bool g_secondaryLoopedPlaying = false;
bool g_primaryAdditionalLoopedPlaying = false;
bool g_secondaryAdditionalLoopedPlaying = false;
int g_lastFireWeaponId = 0;

static void ResetLoopedPlayingVars()
{
	g_primaryLoopedPlaying = false;
	g_secondaryLoopedPlaying = false;
	g_primaryAdditionalLoopedPlaying = false;
	g_secondaryAdditionalLoopedPlaying = false;
}

static float DecodePunchAngleComponent(int i)
{
	const int punchAngleCoded = (i >> 7) & 0xFF;
	return punchAngleCoded / 8.0f;
}

static void EV_PerformWeaponFire(event_args_t *args)
{
	int idx = args->entindex;
	Vector origin{args->origin};
	Vector velocity{args->velocity};

	int iparam1 = args->iparam1;
	int iparam2 = args->iparam2;

	float punchAngleX = 0.0f;
	float punchAngleY = 0.0f;

	if (iparam1 < 0)
	{
		iparam1 = -iparam1;

		punchAngleX = DecodePunchAngleComponent(iparam1);
		punchAngleX = -punchAngleX;
	}
	else
	{
		punchAngleX = DecodePunchAngleComponent(iparam1);
	}

	if (iparam2 < 0)
	{
		iparam2 = -iparam2;

		punchAngleY = DecodePunchAngleComponent(iparam2);
		punchAngleY = -punchAngleY;
	}
	else
	{
		punchAngleY = DecodePunchAngleComponent(iparam2);
	}

	const bool altMode = FBitSet(iparam1, (int)WeaponEventFlags::ALTMODE);
	const bool empty = FBitSet(iparam1, (int)WeaponEventFlags::EMPTIED);
	const bool bAlternatingEject = FBitSet(iparam1, (int)WeaponEventFlags::ALTERNATING_EJECT);
	const int body = (iparam1 >> 3) & 0xF;
	const int weaponId = iparam2 & 0x3F;

	if (g_lastFireWeaponId != weaponId)
	{
		ResetLoopedPlayingVars();
		g_lastFireWeaponId = weaponId;
	}

	const WeaponParameters& params = GetWeaponParameters(weaponId);

	const WeaponParameters::Fire& fire = params.fire;
	const WeaponParameters::Fire::Type fireType = fire.fireType.Get(altMode);

	if (args->bparam1)
	{
		const WeaponSoundScript* possibleLoopedSoundScripts[] = {
			&fire.sound.Get(false),
			&fire.sound.Get(true),
			&fire.soundAdditional.Get(false),
			&fire.soundAdditional.Get(true)
		};

		for (const auto& soundScriptPtr : possibleLoopedSoundScripts)
		{
			if (soundScriptPtr->looped)
			{
				EV_StopWeaponSoundScript(idx, *soundScriptPtr);
			}
		}

		ResetLoopedPlayingVars();

		return;
	}

	if (fireType == WeaponParameters::Fire::MELEE)
	{
		EV_PlayWeaponSoundScript(idx, origin, fire.sound.Get(altMode));

		if( EV_IsLocal( idx ) )
		{
			const WeaponParameters::FireAnimArray& arr = fire.anims.Get(altMode, empty);
			if (arr.size())
			{
				const int count = static_cast<int>(arr.size());
				gEngfuncs.pEventAPI->EV_WeaponAnimation(arr[(g_iSwing++) % count], body);
			}
		}
		return;
	}

	const float spreadX = args->fparam1;
	const float spreadY = args->fparam2;

	//gEngfuncs.Con_Printf("Punch in event: %g, %g\n", punchAngleX, punchAngleY);

	const Vector angles{
		args->angles[0] + punchAngleX,
		args->angles[1] + punchAngleY,
		args->angles[2]
	};

	Vector up, right, forward;

	AngleVectors( angles, forward, right, up );

	if( EV_IsLocal( idx ) )
	{
		if (fire.muzzleFlash.Get(altMode))
			EV_MuzzleFlash();

		const int fireAnim = SelectFireAnimation(params, altMode, empty);
		if (fireAnim >= 0)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(fireAnim, body);

		const float punchX = RandomizeNumberFromRange(fire.clientPunchPitch.Get(altMode));
		if (punchX)
			V_PunchAxis(0, punchX);
		const float punchY = RandomizeNumberFromRange(fire.clientPunchYaw.Get(altMode));
		if (punchY)
			V_PunchAxis(1, punchY);
	}

	if (fireType == WeaponParameters::Fire::BULLETS && fire.shellEjectDelay.Get(altMode) <= 0.0f)
	{
		const char* alternatingShellModel = fire.shellModelAlternating.Get(altMode);
		const char* shellModel = (bAlternatingEject && alternatingShellModel) ? alternatingShellModel : fire.shellModel.Get(altMode);
		if (shellModel)
		{
			Vector ShellVelocity;
			Vector ShellOrigin;

			int shell = gEngfuncs.pEventAPI->EV_FindModelIndex(shellModel);
			int shellCount = fire.shellCount.Get(altMode);
			bool shellLeftSide = fire.shellLeftSide.Get(altMode);

			ShellInfoParams shellInfoParams;
			shellInfoParams.origin = origin;
			shellInfoParams.velocity = velocity;
			shellInfoParams.forward = forward;
			shellInfoParams.up = up;
			shellInfoParams.right = shellLeftSide ? -right : right;
			shellInfoParams.forwardScale = fire.shellOffsetForward.Get(altMode);
			shellInfoParams.upScale = fire.shellOffsetUp.Get(altMode);
			shellInfoParams.rightScale = shellLeftSide ? -fire.shellOffsetSide.Get(altMode) : fire.shellOffsetSide.Get(altMode);
			shellInfoParams.attachment = fire.shellAttachment.Get(altMode);
			shellInfoParams.upFactor = fire.shellVelocityUp.Get(altMode);
			shellInfoParams.sideFactor = fire.shellVelocitySide.Get(altMode);
			shellInfoParams.forwardFactor = fire.shellVelocityForward.Get(altMode);

			for(int j = 0; j < shellCount; j++)
			{
				EV_GetDefaultShellInfo(args, shellInfoParams, ShellVelocity, ShellOrigin);
				EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, fire.shellSound.Get(altMode));
			}
		}
	}

	auto CheckLoopedSounds = [&fire](int channel, bool fireMode)
	{
		auto IsLoopingOnChannel = [](const WeaponSoundScript& sound, int channel)
		{
			return sound.looped && sound.channel == channel;
		};

		if (!fireMode)
		{
			if (IsLoopingOnChannel(fire.sound.main, channel))
			{
				g_primaryLoopedPlaying = false;
			}
			if (IsLoopingOnChannel(fire.soundAdditional.main, channel))
			{
				g_primaryAdditionalLoopedPlaying = false;
			}
		}
		else
		{
			if (fire.sound.alt.has_value() && IsLoopingOnChannel(*fire.sound.alt, channel))
			{
				g_secondaryLoopedPlaying = false;
			}
			if (fire.soundAdditional.alt.has_value() && IsLoopingOnChannel(*fire.soundAdditional.alt, channel))
			{
				g_secondaryAdditionalLoopedPlaying = false;
			}
		}
	};

	const WeaponSoundScript& fireSound = fire.sound.Get(altMode);
	bool shouldPlayFireSound = true;
	if (fireSound.looped)
	{
		if (altMode)
		{
			shouldPlayFireSound = !g_secondaryLoopedPlaying;
			g_secondaryLoopedPlaying = true;
		}
		else
		{
			shouldPlayFireSound = !g_primaryLoopedPlaying;
			g_primaryLoopedPlaying = true;
		}
	}
	if (shouldPlayFireSound)
	{
		EV_PlayWeaponSoundScript(idx, origin, fireSound);
		CheckLoopedSounds(fireSound.channel, !altMode);
	}

	const WeaponSoundScript& fireSoundAdditional = fire.soundAdditional.Get(altMode);
	bool shouldPlayFireAdditionalSound = true;
	if (fireSoundAdditional.looped)
	{
		if (altMode)
		{
			shouldPlayFireAdditionalSound = !g_secondaryAdditionalLoopedPlaying;
			g_secondaryAdditionalLoopedPlaying = true;
		}
		else
		{
			shouldPlayFireAdditionalSound = !g_primaryAdditionalLoopedPlaying;
			g_primaryAdditionalLoopedPlaying = true;
		}
	}
	if (shouldPlayFireAdditionalSound)
	{
		EV_PlayWeaponSoundScript(idx, origin, fireSoundAdditional);
		CheckLoopedSounds(fireSoundAdditional.channel, !altMode);
	}

	if (fireType == WeaponParameters::Fire::BULLETS)
	{
		Vector vecSrc = EV_GetGunPosition(args, origin);
		Vector vecAiming = forward;
		EV_HLDM_FireBullets(idx, forward, right, up, fire.bulletCount.Get(altMode), vecSrc, vecAiming, fire.bulletDistance.Get(altMode),
							fire.tracerFreq.Get(altMode), &g_tracerCount[idx - 1], spreadX, spreadY);
	}

	const Visual& sprayVisual = fire.sprayVisual.Get(altMode);
	const int sprayCount = fire.sprayCount.Get(altMode);

	if (sprayVisual.HasModel() && sprayCount > 0)
	{
		const Vector vecSrc = EV_GetGunPosition(args, origin);
		Vector vecSpitDir = forward;
		Vector vecSpitPos = vecSrc + forward * fire.sprayOffsetForward.Get(altMode) + right * fire.sprayOffsetSide.Get(altMode) + up * fire.sprayOffsetUp.Get(altMode);

		int sprayModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex(sprayVisual.model);
		if (sprayModelIndex)
		{
			FX_Spray(vecSpitPos, vecSpitDir, sprayModelIndex, sprayCount, fire.spraySpeed.Get(altMode), fire.spraySpread.Get(altMode), sprayVisual, fire.sprayFlags.Get(altMode));
		}
	}
}

void EV_FireConfigurableWeapon( event_args_t *args )
{
	EV_PerformWeaponFire(args);
}

//======================
//	   GAUSS START
//======================
#define SND_STOP			(1 << 5)
#define SND_CHANGE_PITCH	(1 << 7)		// duplicated in protocol.h change sound pitch

void EV_SpinGauss( event_args_t *args )
{
	Vector origin;
	Vector angles;
	Vector velocity;

	int idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	int pitch = args->iparam1;
	int iSoundState = args->bparam1 ? SND_CHANGE_PITCH : 0;
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "ambience/pulsemachine.wav", 1.0, ATTN_NORM, iSoundState, pitch );
}

/*
==============================
EV_StopPreviousGauss

==============================
*/
void EV_StopPreviousGauss( int idx )
{
	// Make sure we don't have a gauss spin event in the queue for this guy
	gEngfuncs.pEventAPI->EV_KillEvents( idx, "events/gaussspin.sc" );
	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_WEAPON, "ambience/pulsemachine.wav" );
}

extern float g_flApplyVel;

void EV_FireGauss( event_args_t *args )
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	float flDamage = args->fparam1;
	//int primaryfire = args->bparam1;

	int m_fPrimaryFire = args->bparam1;
	//int m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	Vector vecSrc;
	Vector vecDest;
	//edict_t		*pentIgnore;
	pmtrace_t tr, beam_tr;
	float flMaxFrac = 1.0;
	//int nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int nMaxHits = 10;
	physent_t *pEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	if( args->bparam2 )
	{
		EV_StopPreviousGauss( idx );
		return;
	}

	//Con_Printf( "Firing gauss with %f\n", flDamage );
	vecSrc = EV_GetGunPosition( args, origin );

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
	m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow.spr" );

	AngleVectors( angles, forward, right, up );

	VectorMA( vecSrc, 8192, forward, vecDest );

	const WeaponParameters& params = GetWeaponParameters(WEAPON_GAUSS);

	if( EV_IsLocal( idx ) )
	{
		const int fireAnim = SelectFireAnimation(params, false, false);
		if (fireAnim >= 0)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(fireAnim, 0);

		const float punchX = RandomizeNumberFromRange(params.fire.clientPunchPitch.Get(false));
		if (punchX)
			V_PunchAxis(0, punchX);
		const float punchY = RandomizeNumberFromRange(params.fire.clientPunchYaw.Get(false));
		if (punchY)
			V_PunchAxis(1, punchY);

		if( m_fPrimaryFire == false )
			 g_flApplyVel = flDamage;
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss2.wav", 0.5f + flDamage * ( 1.0f / 400.0f ), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	while( flDamage > 10 && nMaxHits > 0 )
	{
		nMaxHits--;

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_NORMAL, -1, &tr );

		gEngfuncs.pEventAPI->EV_PopPMStates();

		if( tr.allsolid )
			break;

		const float beamBrightness = (m_fPrimaryFire ? 128.0f : flDamage) / 255.0f;
		const float beamR = (m_fPrimaryFire ? 255 : 255) / 255.0f;
		const float beamG = (m_fPrimaryFire ? 128 : 255) / 255.0f;
		const float beamB = (m_fPrimaryFire ? 0 : 255) / 255.0f;
		const float beamWidth = m_fPrimaryFire ? 1.0f : 2.5f;

		if( fFirstBeam )
		{
			if( EV_IsLocal( idx ) )
			{
				// Add muzzle flash to current weapon model
				EV_MuzzleFlash();
			}
			fFirstBeam = 0;

			gEngfuncs.pEfxAPI->R_BeamEntPoint(
				idx | 0x1000,
				tr.endpos,
				m_iBeam,
				0.1f,
				beamWidth,
				0.0f,
				beamBrightness,
				0,
				0,
				0,
				beamR,
				beamG,
				beamB
			);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BeamPoints( vecSrc,
				tr.endpos,
				m_iBeam,
				0.1f,
				beamWidth,
				0.0f,
				beamBrightness,
				0,
				0,
				0,
				beamR,
				beamG,
				beamB
			);
		}

		pEntity = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
		if( pEntity == NULL )
			break;

		if( pEntity->solid == SOLID_BSP )
		{
			float n;

			//pentIgnore = NULL;

			n = -DotProduct( tr.plane.normal, forward );

			bool isSky = DidHitSky(&tr, vecSrc, vecDest);

			if( n < 0.5f ) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;

				VectorMA( forward, 2.0f * n, tr.plane.normal, r );

				flMaxFrac = flMaxFrac - tr.fraction;

				VectorCopy( r, forward );

				VectorMA( tr.endpos, 8.0, forward, vecSrc );
				VectorMA( vecSrc, 8192.0, forward, vecDest );

				if (!isSky)
				{
					gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0f, flDamage * n * 0.5f * 0.1f, FTENT_FADEOUT );

					Vector fwd;
					VectorAdd( tr.endpos, tr.plane.normal, fwd );

					gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10.0f, 20.0f ) / 100.0f, 100,
										255, 100 );
				}

				// lose energy
				if( n == 0.0f )
				{
					n = 0.1f;
				}

				flDamage = flDamage * ( 1 - n );
			}
			else
			{
				// tunnel
				if (!isSky)
				{
					EV_HLDM_DecalGunshot( &tr );

					gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0f, 6.0f, FTENT_FADEOUT );
				}

				// limit it to one hole punch
				if( fHasPunched )
				{
					break;
				}
				fHasPunched = 1;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if( !m_fPrimaryFire )
				{
					Vector start;

					VectorMA( tr.endpos, 8.0, forward, start );

					// Store off the old count
					gEngfuncs.pEventAPI->EV_PushPMStates();

					// Now add in all of the players.
					gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );

					gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
					gEngfuncs.pEventAPI->EV_PlayerTrace( start, vecDest, PM_NORMAL, -1, &beam_tr );

					if( !beam_tr.allsolid )
					{
						Vector delta;

						// trace backwards to find exit point
						gEngfuncs.pEventAPI->EV_PlayerTrace( beam_tr.endpos, tr.endpos, PM_NORMAL, -1, &beam_tr );

						VectorSubtract( beam_tr.endpos, tr.endpos, delta );

						n = Length( delta );

						if( n < flDamage )
						{
							if( n == 0 )
								n = 1;
							flDamage -= n;

							// absorption balls
							{
								Vector fwd;
								VectorSubtract( tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10.0f, 20.0f ) / 100.0f, 100,
									255, 100 );
							}

	//////////////////////////////////// WHAT TO DO HERE
							// CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

							isSky = DidHitSky(&beam_tr, beam_tr.endpos, tr.endpos);
							if (!isSky)
							{
								EV_HLDM_DecalGunshot( &beam_tr );

								gEngfuncs.pEfxAPI->R_TempSprite( beam_tr.endpos, vec3_origin, 0.1, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0f, 6.0f, FTENT_FADEOUT );

								// balls
								{
									Vector fwd;
									VectorSubtract( beam_tr.endpos, forward, fwd );
									gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, beam_tr.endpos, fwd, m_iBalls, (int)( flDamage * 0.3f ), 0.1, gEngfuncs.pfnRandomFloat( 10.0f, 20.0f ) / 100.0f, 200,
										255, 40 );
								}
							}

							VectorAdd( beam_tr.endpos, forward, vecSrc );
						}
					}
					else
					{
						flDamage = 0;
					}

					gEngfuncs.pEventAPI->EV_PopPMStates();
				}
				else
				{
					if( m_fPrimaryFire )
					{
						// slug doesn't punch through ever with primary
						// fire, so leave a little glowy bit and make some balls
						if (!isSky)
						{
							gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0f / 255.0f, 0.3, FTENT_FADEOUT );
							{
								Vector fwd;
								VectorAdd( tr.endpos, tr.plane.normal, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 8, 0.6, gEngfuncs.pfnRandomFloat( 10.0f, 20.0f ) / 100.0f, 100,
									255, 200 );
							}
						}
					}

					flDamage = 0;
				}
			}
		}
		else
		{
			VectorAdd( tr.endpos, forward, vecSrc );
		}
	}
}
//======================
//	   GAUSS END
//======================

//======================
//	  CROSSBOW START
//======================

//=====================
// EV_BoltCallback
// This function is used to correct the origin and angles
// of the bolt, so it looks like it's stuck on the wall.
//=====================
void EV_BoltCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	ent->entity.origin = ent->entity.baseline.vuser1;
	ent->entity.angles = ent->entity.baseline.vuser2;
}

void EV_FireCrossbow2( event_args_t *args )
{
	EV_PerformWeaponFire(args);

	const int weaponId = args->iparam2 & 0x3F;
	const WeaponParameters& params = GetWeaponParameters(weaponId);

	if (params.fire.fireType.Get(true) != WeaponParameters::Fire::PROJECTILE || strcmp(params.fire.projectileName.Get(true).c_str(), "crossbow_bolt") != 0)
	{
		return;
	}

	Vector vecSrc, vecEnd;
	Vector up, right, forward;
	pmtrace_t tr;

	int idx = args->entindex;
	Vector origin{args->origin};
	Vector angles{args->angles};

	AngleVectors( angles, forward, right, up );

	vecSrc = EV_GetGunPosition( args, origin );

	VectorMA( vecSrc, 8192, forward, vecEnd );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

	//We hit something
	if( tr.fraction < 1.0f )
	{
		physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

		//Not the world, let's assume we hit something organic ( dog, cat, uncle joe, etc ).
		if( pe->solid != SOLID_BSP )
		{
			switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
			{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				break;
			}
		}
		//Stick to world but don't stick to glass, it might break and leave the bolt floating. It can still stick to other non-transparent breakables though.
		else if( pe->rendermode == kRenderNormal )
		{
			gEngfuncs.pEventAPI->EV_PlaySound( 0, tr.endpos, CHAN_BODY, "weapons/xbow_hit1.wav", gEngfuncs.pfnRandomFloat( 0.95f, 1.0f ), ATTN_NORM, 0, PITCH_NORM );

			//Not underwater, do some sparks...
			if( gEngfuncs.PM_PointContents( tr.endpos, NULL ) != CONTENTS_WATER )
				 gEngfuncs.pEfxAPI->R_SparkShower( tr.endpos );

			Vector vBoltAngles;
			int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/crossbow_bolt.mdl" );

			VectorAngles( forward, vBoltAngles );

			TEMPENTITY *bolt = gEngfuncs.pEfxAPI->R_TempModel( tr.endpos - forward * 10, Vector( 0, 0, 0 ), vBoltAngles , 5, iModelIndex, TE_BOUNCE_NULL );

			if( bolt )
			{
				bolt->flags |= ( FTENT_CLIENTCUSTOM ); //So it calls the callback function.
				bolt->entity.baseline.vuser1 = tr.endpos - forward * 10; // Pull out a little bit
				bolt->entity.baseline.vuser2 = vBoltAngles; //Look forward!
				bolt->callback = EV_BoltCallback; //So we can set the angles and origin back. (Stick the bolt to the wall)
			}
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

//======================
//	   CROSSBOW END
//======================

//======================
//	    EGON START
//======================

int g_fireAnims1[] = { EGON_FIRE1, EGON_FIRE2, EGON_FIRE3, EGON_FIRE4 };
int g_fireAnims2[] = { EGON_ALTFIRECYCLE };

enum EGON_FIRESTATE
{
	FIRE_OFF,
	FIRE_CHARGE
};

enum EGON_FIREMODE
{
	FIRE_NARROW,
	FIRE_WIDE
};

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

BEAM *pBeam;
BEAM *pBeam2;
TEMPENTITY *pFlare;	// Vit_amiN: egon's beam flare

void EV_EgonFlareCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	float delta = currenttime - ent->tentOffset.z;	// time past since the last scale
	if( delta >= ent->tentOffset.y )
	{
		ent->entity.curstate.scale += ent->tentOffset.x * delta;
		ent->tentOffset.z = currenttime;
	}
}

void EV_EgonFire( event_args_t *args )
{
	int idx, /*iFireState,*/ iFireMode;
	Vector origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	//iFireState = args->iparam1;
	iFireMode = args->iparam2;
	int iStartup = args->bparam1;

	if( iStartup )
	{
		if( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125 );
		else
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 100 );
	}
	else
	{
		// If there is any sound playing already, kill it. - Solokiller
		// This is necessary because multiple sounds can play on the same channel at the same time.
		// In some cases, more than 1 run sound plays when the egon stops firing, in which case only the earliest entry in the list is stopped.
		// This ensures no more than 1 of those is ever active at the same time.
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EGON_SOUND_RUN );

		if( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
		else
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.9, ATTN_NORM, 0, 100 );
	}

	//Only play the weapon anims if I shot it.
	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( g_fireAnims1[gEngfuncs.pfnRandomLong( 0, 3 )], 0 );

	if( iStartup == 1 && EV_IsLocal( idx ) && !( pBeam || pBeam2 || pFlare ) && cl_lw->value ) //Adrian: Added the cl_lw check for those lital people that hate weapon prediction.
	{
		Vector vecSrc, vecEnd, angles, forward, right, up;
		pmtrace_t tr;

		cl_entity_t *pl = gEngfuncs.GetEntityByIndex( idx );

		if( pl )
		{
			VectorCopy( gHUD.m_vecAngles, angles );

			AngleVectors( angles, forward, right, up );

			vecSrc = EV_GetGunPosition( args, pl->origin );

			VectorMA( vecSrc, 2048, forward, vecEnd );

			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

			// Store off the old count
			gEngfuncs.pEventAPI->EV_PushPMStates();

			// Now add in all of the players.
			gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );

			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

			gEngfuncs.pEventAPI->EV_PopPMStates();

			int iBeamModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( EGON_BEAM_SPRITE );

			float r = 50.0f;
			float g = 50.0f;
			float b = 125.0f;

			// if( IEngineStudio.IsHardware() )
			{
				r /= 255.0f;
				g /= 255.0f;
				b /= 255.0f;
			}

			pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 3.5, 0.2, 0.7, 55, 0, 0, r, g, b );

			if( pBeam )
				 pBeam->flags |= ( FBEAM_SINENOISE );

			pBeam2 = gEngfuncs.pEfxAPI->R_BeamEntPoint( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b );

			// Vit_amiN: egon beam flare
			pFlare = gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, gEngfuncs.pEventAPI->EV_FindModelIndex( EGON_FLARE_SPRITE ), kRenderGlow, kRenderFxNoDissipation, 1.0, 99999, FTENT_SPRCYCLE | FTENT_PERSIST );
		}
	}

	if( pFlare )	// Vit_amiN: store the last mode for EV_EgonStop()
	{
		pFlare->tentOffset.x = ( iFireMode == FIRE_WIDE ) ? 1.0f : 0.0f;
	}
}

void EV_EgonStop( event_args_t *args )
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EGON_SOUND_RUN );

	if( args->iparam1 )
		 gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_OFF, 0.98, ATTN_NORM, 0, 100 );

	if( EV_IsLocal( idx ) )
	{
		if( pBeam )
		{
			pBeam->die = 0.0f;
			pBeam = NULL;
		}

		if( pBeam2 )
		{
			pBeam2->die = 0.0f;
			pBeam2 = NULL;
		}

		if( pFlare )	// Vit_amiN: egon beam flare
		{
			pFlare->die = gEngfuncs.GetClientTime();

			if( gEngfuncs.GetMaxClients() == 1 || !(pFlare->flags & FTENT_NOMODEL) )
			{
				if( pFlare->tentOffset.x != 0.0f )	// true for iFireMode == FIRE_WIDE
				{
					pFlare->callback = &EV_EgonFlareCallback;
					pFlare->fadeSpeed = 2.0;			// fade out will take 0.5 sec
					pFlare->tentOffset.x = 10.0;		// scaling speed per second
					pFlare->tentOffset.y = 0.1;			// min time between two scales
					pFlare->tentOffset.z = pFlare->die;	// the last callback run time
					pFlare->flags = FTENT_FADEOUT | FTENT_CLIENTCUSTOM;
				}
			}

			pFlare = NULL;
		}
	}
}
//======================
//	    EGON END
//======================

//======================
//	   TRIPMINE START
//======================

//We only check if it's possible to put a trip mine
//and if it is, then we play the animation. Server still places it.
void EV_TripmineFire( event_args_t *args )
{
	int idx;
	Vector vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	const bool last = args->bparam1 != 0;
	VectorCopy( args->origin, vecSrc );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, NULL, NULL );

	if( !EV_IsLocal ( idx ) )
		return;

	// Grab predicted result for local player
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );

	vecSrc = vecSrc + view_ofs;

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecSrc + forward * 128.0f, PM_NORMAL, -1, &tr );

	//Hit something solid
	if( tr.fraction < 1.0f && !last )
		 gEngfuncs.pEventAPI->EV_WeaponAnimation ( TRIPMINE_DRAW, 0 );

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   TRIPMINE END
//======================

//======================
//	   SQUEAK START
//======================

#define VEC_HULL_MIN		Vector( -16, -16, -36 )
#define VEC_DUCK_HULL_MIN	Vector( -16, -16, -18 )

void EV_SnarkFire( event_args_t *args )
{
	int idx;
	Vector vecSrc, angles, /*view_ofs,*/ forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy( args->origin, vecSrc );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, NULL, NULL );

	if( !EV_IsLocal ( idx ) )
		return;

	if( args->ducking )
		vecSrc = vecSrc - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr );

	//Find space to drop the thing.
	if( tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25f )
		 gEngfuncs.pEventAPI->EV_WeaponAnimation( SQUEAK_THROW, 0 );

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   SQUEAK END
//======================

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;

	const char *pszSound;

	idx = args->entindex;

	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop = args->bparam1;

	m_flVolume = (float)( us_params & 0x003f ) / 40.0f;
	noise = (int)( ( ( us_params ) >> 12 ) & 0x0007 );
	pitch = (int)( 10.0f * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch( noise )
	{
	case 1:
		pszSound = "plats/ttrain1.wav";
		break;
	case 2:
		pszSound = "plats/ttrain2.wav";
		break;
	case 3:
		pszSound = "plats/ttrain3.wav";
		break;
	case 4:
		pszSound = "plats/ttrain4.wav";
		break;
	case 5:
		pszSound = "plats/ttrain6.wav";
		break;
	case 6:
		pszSound = "plats/ttrain7.wav";
		break;
	default:
		// no sound
		return;
	}

	if( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, pszSound );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, pszSound, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

void EV_VehiclePitchAdjust( event_args_t *args )
{
	int idx = args->entindex;

	Vector origin{args->origin,};

	float m_flVolume = args->fparam1;
	int noise = args->iparam1;
	int pitch = args->iparam2;
	int stop = args->bparam1;

	const char *pszSound;
	switch( noise )
	{
	case 1:
		pszSound = "plats/vehicle1.wav";
		break;
	case 2:
		pszSound = "plats/vehicle2.wav";
		break;
	case 3:
		pszSound = "plats/vehicle3.wav";
		break;
	case 4:
		pszSound = "plats/vehicle4.wav";
		break;
	case 5:
		pszSound = "plats/vehicle6.wav";
		break;
	case 6:
		pszSound = "plats/vehicle7.wav";
		break;
	default:
		// no sound
		return;
	}

	if( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, pszSound );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, pszSound, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

//======================
//	   DISPLACER START
//======================

void EV_Displacer( event_args_t *args )
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	if( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( DISPLACER_FIRE, 0 );
		V_PunchAxis( 0, -2.0 );
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/displacer_fire.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}
//======================
//	    DISPLACER END
//======================

//======================
//	   MEDKIT START
//======================

void EV_MedkitFire( event_args_s *args )
{
	int idx = args->entindex;
	if( EV_IsLocal( idx ) )
	{
		if (args->iparam1)
			gEngfuncs.pEventAPI->EV_WeaponAnimation( MEDKIT_LONGUSE, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( MEDKIT_SHORTUSE, 0 );
	}
}

//======================
//	   MEDKIT END
//======================

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}
