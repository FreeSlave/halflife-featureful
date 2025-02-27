/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
/*

===== util.cpp ========================================================

  Utility code.  Really not optional after all.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "shake.h"
#include "decals.h"
#include "player.h"
#include "combat.h"
#include "global_models.h"
#include "game.h"
#include "gamerules.h"
#include "string_utils.h"

#include <map>
#include <set>
#include <string>

class StringPool
{
public:
	void AddString(const char* str, string_t s)
	{
		stringMap[str] = s;
	}
	string_t FindString(const char* str)
	{
		auto it = stringMap.find(str);
		if (it != stringMap.end())
		{
			return it->second;
		}
		else
		{
			return iStringNull;
		}
	}
	void Clear()
	{
		stringMap.clear();
	}
private:
	std::map<std::string, string_t> stringMap;
};

#define USE_STRINGPOOL 1

StringPool g_StringPool;

string_t ALLOC_STRING(const char* str)
{
#if USE_STRINGPOOL
	string_t s = g_StringPool.FindString(str);
	if (!FStringNull(s))
	{
		return s;
	}
	else
	{
		s = g_engfuncs.pfnAllocString(str);
		g_StringPool.AddString(str, s);
		return s;
	}
#else
	return g_engfuncs.pfnAllocString(str);
#endif
}

void ClearStringPool()
{
	g_StringPool.Clear();
}

std::set<std::string> g_precachedModels;
std::set<std::string> g_precachedSounds;

int PRECACHE_MODEL(const char* name)
{
	if (!name)
	{
		ALERT(at_warning, "Tried to precache model by the null string!\n");
		return -1;
	}
	if (IsDeveloperModeOn())
		g_precachedModels.insert(name);
	return g_engfuncs.pfnPrecacheModel(name);
}

int PRECACHE_SOUND(const char* name)
{
	if (!name)
	{
		ALERT(at_warning, "Tried to precache sound by the null string!\n");
		return -1;
	}
	if (name && *name == '!')
	{
		// no need to precache since it's a sentence
		return -1;
	}
	if (IsDeveloperModeOn())
		g_precachedSounds.insert(name);
	return g_engfuncs.pfnPrecacheSound(name);
}

void SET_MODEL(edict_t *e, const char *m)
{
	if (IsDeveloperModeOn())
		g_precachedModels.insert(m);
	g_engfuncs.pfnSetModel(e, m);
}

void ClearPrecachedModels()
{
	g_precachedModels.clear();
}

void ClearPrecachedSounds()
{
	g_precachedSounds.clear();
}

static void ReportPrecachedResources(const std::set<std::string>& precachedResources, const char* resourceName)
{
	if (precachedResources.empty())
	{
		ALERT(at_console, "No precached %s registered! You need to restart or reload the map\n", resourceName);
		return;
	}
	const int argc = CMD_ARGC();
	if (argc > 1)
		ALERT(at_console, "List of precached %s according to the list of prefixes:\n", resourceName);
	else
		ALERT(at_console, "List of precached %s:\n", resourceName);
	int i = 0;
	int countShown = 0;
	for (const auto& entry : precachedResources)
	{
		bool show = false;
		if (argc > 1)
		{
			for (int j=1; j<argc; ++j)
			{
				const char* prefix = CMD_ARGV(j);
				if (entry.compare(0, strlen(prefix), prefix) == 0)
				{
					show = true;
					break;
				}
			}
		}
		else
			show = true;

		if (show)
		{
			ALERT(at_console, "%s; ", entry.c_str());
			countShown++;
			i++;
			if (i == 4)
			{
				ALERT(at_console, "\n");
				i = 0;
			}
		}
	}
	const char* adj = argc > 1 ? "shown" : "precached";
	ALERT(at_console, "\nNumber of %s %s: %d\n", adj, resourceName, countShown);
}

void ReportPrecachedModels()
{
	ReportPrecachedResources(g_precachedModels, "models");
}

void ReportPrecachedSounds()
{
	ReportPrecachedResources(g_precachedSounds, "sounds");
}

void AddMapBSPAsPrecachedModel()
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "maps/%s.bsp", STRING(gpGlobals->mapname));
	g_precachedModels.insert(buf);
}

void WRITE_COLOR(const Color3& color)
{
	WRITE_BYTE( color.r );
	WRITE_BYTE( color.g );
	WRITE_BYTE( color.b );
}

void WRITE_COLOR(const Vector& color)
{
	WRITE_BYTE( color.x );
	WRITE_BYTE( color.y );
	WRITE_BYTE( color.z );
}

void WRITE_VECTOR(const Vector& vecSrc)
{
	WRITE_COORD( vecSrc.x );
	WRITE_COORD( vecSrc.y );
	WRITE_COORD( vecSrc.z );
}

void WRITE_CIRCLE(const Vector& vecSrc, float radius)
{
	WRITE_VECTOR( vecSrc );
	WRITE_COORD( vecSrc.x );
	WRITE_COORD( vecSrc.y );
	WRITE_COORD( vecSrc.z + radius );
}

#include <cstdint>

void UTIL_DynamicLight( const Vector &vecSrc, float flRadius, byte r, byte g, byte b, float flTime, float flDecay )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_VECTOR( vecSrc );
		WRITE_BYTE( flRadius * 0.1f );	// radius * 0.1
		WRITE_BYTE( r );		// r
		WRITE_BYTE( g );		// g
		WRITE_BYTE( b );		// b
		WRITE_BYTE( flTime * 10.0f );	// time * 10
		WRITE_BYTE( flDecay * 0.1f );	// decay * 0.1
	MESSAGE_END();
}

void UTIL_MuzzleLight( const Vector& vecSrc )
{
	extern int gmsgMuzzleLight;
	MESSAGE_BEGIN( MSG_PVS, gmsgMuzzleLight, vecSrc );
		WRITE_VECTOR( vecSrc );
	MESSAGE_END();
}

float UTIL_WeaponTimeBase( void )
{
#if CLIENT_WEAPONS
	return 0.0f;
#else
	return gpGlobals->time;
#endif
}

void UTIL_ParametricRocket( entvars_t *pev, Vector vecOrigin, Vector vecAngles, edict_t *owner )
{	
	pev->startpos = vecOrigin;
	// Trace out line to end pos
	TraceResult tr;
	UTIL_MakeVectors( vecAngles );
	UTIL_TraceLine( pev->startpos, pev->startpos + gpGlobals->v_forward * 8192, ignore_monsters, owner, &tr );
	pev->endpos = tr.vecEndPos;

	// Now compute how long it will take based on current velocity
	Vector vecTravel = pev->endpos - pev->startpos;
	float travelTime = 0.0f;
	if( pev->velocity.Length() > 0.0f )
	{
		travelTime = vecTravel.Length() / pev->velocity.Length();
	}
	pev->starttime = gpGlobals->time;
	pev->impacttime = gpGlobals->time + travelTime;
}

int g_groupmask = 0;
int g_groupop = 0;

// Normal overrides
void UTIL_SetGroupTrace( int groupmask, int op )
{
	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK( g_groupmask, g_groupop );
}

void UTIL_UnsetGroupTrace( void )
{
	g_groupmask = 0;
	g_groupop = 0;

	ENGINE_SETGROUPMASK( 0, 0 );
}

// Smart version, it'll clean itself up when it pops off stack
UTIL_GroupTrace::UTIL_GroupTrace( int groupmask, int op )
{
	m_oldgroupmask = g_groupmask;
	m_oldgroupop = g_groupop;

	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK( g_groupmask, g_groupop );
}

UTIL_GroupTrace::~UTIL_GroupTrace( void )
{
	g_groupmask = m_oldgroupmask;
	g_groupop = m_oldgroupop;

	ENGINE_SETGROUPMASK( g_groupmask, g_groupop );
}

TYPEDESCRIPTION	gEntvarsDescription[] =
{
	DEFINE_ENTITY_FIELD( classname, FIELD_STRING ),
	DEFINE_ENTITY_GLOBAL_FIELD( globalname, FIELD_STRING ),

	DEFINE_ENTITY_FIELD( origin, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_FIELD( oldorigin, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_FIELD( velocity, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( basevelocity, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( movedir, FIELD_VECTOR ),

	DEFINE_ENTITY_FIELD( angles, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( avelocity, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( punchangle, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( v_angle, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( fixangle, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( idealpitch, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( pitch_speed, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( ideal_yaw, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( yaw_speed, FIELD_FLOAT ),

	DEFINE_ENTITY_FIELD( modelindex, FIELD_INTEGER ),
	DEFINE_ENTITY_GLOBAL_FIELD( model, FIELD_MODELNAME ),

	DEFINE_ENTITY_FIELD( viewmodel, FIELD_MODELNAME ),
	DEFINE_ENTITY_FIELD( weaponmodel, FIELD_MODELNAME ),

	DEFINE_ENTITY_FIELD( absmin, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_FIELD( absmax, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_GLOBAL_FIELD( mins, FIELD_VECTOR ),
	DEFINE_ENTITY_GLOBAL_FIELD( maxs, FIELD_VECTOR ),
	DEFINE_ENTITY_GLOBAL_FIELD( size, FIELD_VECTOR ),

	DEFINE_ENTITY_FIELD( ltime, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( nextthink, FIELD_TIME ),

	DEFINE_ENTITY_FIELD( solid, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( movetype, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( skin, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( body, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( effects, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( gravity, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( friction, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( light_level, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( frame, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( scale, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( sequence, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( animtime, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( framerate, FIELD_FLOAT ),
	DEFINE_ENTITY_ARRAY( controller, FIELD_CHARACTER, 4 ),
	DEFINE_ENTITY_ARRAY( blending, FIELD_CHARACTER, 2 ),

	DEFINE_ENTITY_FIELD( rendermode, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( renderamt, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( rendercolor, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( renderfx, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( health, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( frags, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( weapons, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( takedamage, FIELD_FLOAT ),

	DEFINE_ENTITY_FIELD( deadflag, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( view_ofs, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( button, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( impulse, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( chain, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( dmg_inflictor, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( enemy, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( aiment, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( owner, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( groundentity, FIELD_EDICT ),

	DEFINE_ENTITY_FIELD( spawnflags, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( flags, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( colormap, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( team, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( max_health, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( teleport_time, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( armortype, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( armorvalue, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( waterlevel, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( watertype, FIELD_INTEGER ),

	// Having these fields be local to the individual levels makes it easier to test those levels individually.
	DEFINE_ENTITY_GLOBAL_FIELD( target, FIELD_STRING ),
	DEFINE_ENTITY_GLOBAL_FIELD( targetname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( netname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( message, FIELD_STRING ),

	DEFINE_ENTITY_FIELD( dmg_take, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( dmg_save, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( dmg, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( dmgtime, FIELD_TIME ),

	DEFINE_ENTITY_FIELD( noise, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise1, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise2, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise3, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( speed, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( air_finished, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( pain_finished, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( radsuit_finished, FIELD_TIME ),
};

#define ENTVARS_COUNT		( sizeof(gEntvarsDescription) / sizeof(gEntvarsDescription[0]) )

#if	DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
	if( pev->pContainingEntity != NULL )
		return pev->pContainingEntity;
	ALERT( at_console, "entvars_t pContainingEntity is NULL, calling into engine\n" );
	edict_t *pent = (*g_engfuncs.pfnFindEntityByVars)( (entvars_t*)pev );
	if( pent == NULL )
		ALERT( at_console, "DAMN!  Even the engine couldn't FindEntityByVars!\n" );
	( (entvars_t *)pev )->pContainingEntity = pent;
	return pent;
}

void DBG_AssertFunction( bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage )
{
	if( fExpr )
		return;
	char szOut[512];
	
	if( szMessage != NULL )
		sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage );
	else
		sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)", szExpr, szFile, szLine );

	ALERT( at_console, szOut );
}
#endif	// DEBUG

Vector UTIL_VecToAngles( const Vector &vec )
{
	float rgflVecOut[3];
	VEC_TO_ANGLES( vec, rgflVecOut );
	return Vector( rgflVecOut );
}

// float UTIL_MoveToOrigin( edict_t *pent, const Vector vecGoal, float flDist, int iMoveType )
void UTIL_MoveToOrigin( edict_t *pent, const Vector &vecGoal, float flDist, int iMoveType )
{
	float rgfl[3];
	vecGoal.CopyToArray( rgfl );
	//return MOVE_TO_ORIGIN( pent, rgfl, flDist, iMoveType ); 
	MOVE_TO_ORIGIN( pent, rgfl, flDist, iMoveType ); 
}

int UTIL_EntitiesInBox(CBaseEntity **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask, int deadFlagMask)
{
	edict_t *pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity *pEntity;
	int count;

	count = 0;

	if( !pEdict )
		return count;

	for( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		if( pEdict->free )	// Not in use
			continue;

		if( flagMask && !( pEdict->v.flags & flagMask ) )	// Does it meet the criteria?
			continue;

		if ( deadFlagMask && !( pEdict->v.deadflag & deadFlagMask ) )
			continue;

		if( mins.x > pEdict->v.absmax.x ||
			mins.y > pEdict->v.absmax.y ||
			mins.z > pEdict->v.absmax.z ||
			maxs.x < pEdict->v.absmin.x ||
			maxs.y < pEdict->v.absmin.y ||
			maxs.z < pEdict->v.absmin.z )
			continue;

		pEntity = CBaseEntity::Instance( pEdict );
		if( !pEntity )
			continue;

		pList[count] = pEntity;
		count++;

		if( count >= listMax )
			return count;
	}

	return count;
}

int UTIL_MonstersInSphere( CBaseEntity **pList, int listMax, const Vector &center, float radius )
{
	edict_t *pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity *pEntity;
	int		count;
	float		distance, delta;

	count = 0;
	float radiusSquared = radius * radius;

	if( !pEdict )
		return count;

	for( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		if( pEdict->free )	// Not in use
			continue;

		if( !( pEdict->v.flags & ( FL_CLIENT | FL_MONSTER ) ) )	// Not a client/monster ?
			continue;

		// Use origin for X & Y since they are centered for all monsters
		// Now X
		delta = center.x - pEdict->v.origin.x;//( pEdict->v.absmin.x + pEdict->v.absmax.x ) * 0.5f;
		delta *= delta;

		if( delta > radiusSquared )
			continue;
		distance = delta;

		// Now Y
		delta = center.y - pEdict->v.origin.y;//( pEdict->v.absmin.y + pEdict->v.absmax.y ) * 0.5f;
		delta *= delta;

		distance += delta;
		if( distance > radiusSquared )
			continue;

		// Now Z
		delta = center.z - ( pEdict->v.absmin.z + pEdict->v.absmax.z ) * 0.5f;
		delta *= delta;

		distance += delta;
		if( distance > radiusSquared )
			continue;

		pEntity = CBaseEntity::Instance( pEdict );
		if( !pEntity )
			continue;

		pList[count] = pEntity;
		count++;

		if( count >= listMax )
			return count;
	}

	return count;
}

CBaseEntity *UTIL_FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	edict_t	*pentEntity;

	if( pStartEntity )
		pentEntity = pStartEntity->edict();
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_IN_SPHERE( pentEntity, vecCenter, flRadius );

	if( !FNullEnt( pentEntity ) )
		return CBaseEntity::Instance( pentEntity );
	return NULL;
}

CBaseEntity *UTIL_FindEntityByString( CBaseEntity *pStartEntity, const char *szKeyword, const char *szValue )
{
	edict_t	*pentEntity;

	if( pStartEntity )
		pentEntity = pStartEntity->edict();
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_BY_STRING( pentEntity, szKeyword, szValue );

	if( !FNullEnt( pentEntity ) )
		return CBaseEntity::Instance( pentEntity );
	return NULL;
}

CBaseEntity *UTIL_FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName )
{
	return UTIL_FindEntityByString( pStartEntity, "classname", szName );
}

CBaseEntity *UTIL_FindEntityByTargetname( CBaseEntity *pStartEntity, const char *szName )
{
	return UTIL_FindEntityByString( pStartEntity, "targetname", szName );
}

CBaseEntity *UTIL_FindEntityByTargetname( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pActivator )
{
	if (UTIL_TargetnameIsActivator(szName))
	{
		if (pActivator && (pStartEntity == NULL || pActivator->eoffset() > pStartEntity->eoffset()))
			return pActivator;
		else
			return NULL;
	}
	else if (UTIL_IsPlayerReference(szName))
	{
		CBaseEntity* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (pPlayer && (pStartEntity == NULL || pPlayer->eoffset() > pStartEntity->eoffset()))
			return pPlayer;
		else
			return NULL;
	}
	else
		return UTIL_FindEntityByTargetname( pStartEntity, szName );
}

CBaseEntity *UTIL_FindEntityGeneric( const char *szWhatever, Vector &vecSrc, float flRadius )
{
	CBaseEntity *pEntity = NULL;

	pEntity = UTIL_FindEntityByTargetname( NULL, szWhatever );
	if( pEntity )
		return pEntity;

	CBaseEntity *pSearch = NULL;
	float flMaxDist2 = flRadius * flRadius;
	while( ( pSearch = UTIL_FindEntityByClassname( pSearch, szWhatever ) ) != NULL )
	{
		float flDist2 = ( pSearch->pev->origin - vecSrc ).Length();
		flDist2 = flDist2 * flDist2;
		if( flMaxDist2 > flDist2 )
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}
	return pEntity;
}

bool UTIL_HasClassnameOrTargetname(entvars_t *pevToucher, const char* name )
{
	return FClassnameIs(pevToucher, name) || (!FStringNull(pevToucher->targetname) && FStrEq(STRING(pevToucher->targetname), name));
}

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
CBaseEntity *UTIL_PlayerByIndex( int playerIndex )
{
	CBaseEntity *pPlayer = NULL;

	if( playerIndex > 0 && playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = INDEXENT( playerIndex );
		if( pPlayerEdict && !pPlayerEdict->free )
		{
			pPlayer = CBaseEntity::Instance( pPlayerEdict );
		}
	}
	
	return pPlayer;
}

void UTIL_MakeVectors( const Vector &vecAngles )
{
	MAKE_VECTORS( vecAngles );
}

void UTIL_MakeAimVectors( const Vector &vecAngles )
{
	float rgflVec[3];
	vecAngles.CopyToArray(rgflVec);
	rgflVec[0] = -rgflVec[0];
	MAKE_VECTORS( rgflVec );
}

#define SWAP( a, b, temp )	( ( temp ) = ( a ), ( a ) = ( b ), ( b ) = ( temp ) )

void UTIL_MakeInvVectors( const Vector &vec, globalvars_t *pgv )
{
	MAKE_VECTORS( vec );

	float tmp;
	pgv->v_right = pgv->v_right * -1;

	SWAP( pgv->v_forward.y, pgv->v_right.x, tmp );
	SWAP( pgv->v_forward.z, pgv->v_up.x, tmp );
	SWAP( pgv->v_right.z, pgv->v_up.y, tmp );
}

void UTIL_EmitAmbientSound( edict_t *entity, const Vector &vecOrigin, const char *samp, float vol, float attenuation, int fFlags, int pitch )
{
	float rgfl[3];
	vecOrigin.CopyToArray( rgfl );

	if( samp && *samp == '!' )
	{
		char name[32];
		if( SENTENCEG_Lookup( samp, name ) >= 0 )
			EMIT_AMBIENT_SOUND( entity, rgfl, name, vol, attenuation, fFlags, pitch );
	}
	else
		EMIT_AMBIENT_SOUND( entity, rgfl, samp, vol, attenuation, fFlags, pitch );
}

static unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = (int)( value * scale );
	if( output < 0 )
		output = 0;
	if( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}

static short FixedSigned16( float value, float scale )
{
	int output;

	output = (int)( value * scale );

	if( output > 32767 )
		output = 32767;

	if( output < -32768 )
		output = -32768;

	return (short)output;
}

// Shake the screen of all clients within radius
// radius == 0, shake all clients
// UNDONE: Allow caller to shake clients not ONGROUND?
// UNDONE: Fix falloff model (disabled)?
// UNDONE: Affect user controls?
void UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius )
{
	int		i;
	float		localAmplitude;
	ScreenShake	shake;

	shake.duration = FixedUnsigned16( duration, 1 << 12 );		// 4.12 fixed
	shake.frequency = FixedUnsigned16( frequency, 1 << 8 );	// 8.8 fixed

	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if( !pPlayer || !( pPlayer->pev->flags & FL_ONGROUND ) )	// Don't shake if not onground
			continue;

		localAmplitude = 0;

		if( radius <= 0 )
			localAmplitude = amplitude;
		else
		{
			Vector delta = center - pPlayer->pev->origin;
			float distance = delta.Length();

			// Had to get rid of this falloff - it didn't work well
			if( distance < radius )
				localAmplitude = amplitude;//radius - distance;
		}
		if( localAmplitude )
		{
			shake.amplitude = FixedUnsigned16( localAmplitude, 1 << 12 );		// 4.12 fixed

			MESSAGE_BEGIN( MSG_ONE, gmsgShake, NULL, pPlayer->edict() );		// use the magic #1 for "one client"
				WRITE_SHORT( shake.amplitude );				// shake amount
				WRITE_SHORT( shake.duration );				// shake lasts this long
				WRITE_SHORT( shake.frequency );				// shake noise frequency
			MESSAGE_END();
		}
	}
}

void UTIL_ScreenShakeAll( const Vector &center, float amplitude, float frequency, float duration )
{
	UTIL_ScreenShake( center, amplitude, frequency, duration, 0 );
}

void UTIL_ScreenFadeBuild( ScreenFade &fade, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	fade.duration = FixedUnsigned16( fadeTime, 1 << 12 );		// 4.12 fixed
	fade.holdTime = FixedUnsigned16( fadeHold, 1 << 12 );		// 4.12 fixed
	fade.r = (int)color.x;
	fade.g = (int)color.y;
	fade.b = (int)color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}

void UTIL_ScreenFadeWrite( const ScreenFade &fade, CBaseEntity *pEntity )
{
	if( !pEntity || !pEntity->IsNetClient() )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgFade, NULL, pEntity->edict() );		// use the magic #1 for "one client"
		WRITE_SHORT( fade.duration );		// fade lasts this long
		WRITE_SHORT( fade.holdTime );		// fade lasts this long
		WRITE_SHORT( fade.fadeFlags );		// fade type (in / out)
		WRITE_BYTE( fade.r );				// fade red
		WRITE_BYTE( fade.g );				// fade green
		WRITE_BYTE( fade.b );				// fade blue
		WRITE_BYTE( fade.a );				// fade blue
	MESSAGE_END();
}

static int CalculateFadeAlpha( const Vector& fadeSource, CBaseEntity* pEntity, int baseAlpha )
{
	UTIL_MakeVectors(pEntity->pev->v_angle);
	const Vector a = gpGlobals->v_forward;
	const Vector b = (fadeSource - (pEntity->pev->origin + pEntity->pev->view_ofs)).Normalize();
	const float dot = DotProduct(a,b);
	if (dot >= 0)
		return (int)(baseAlpha*Q_min(dot+0.134, 1.0));
	else
		return 0;
}

void UTIL_ScreenFadeAll( const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	int i;
	ScreenFade fade;

	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, alpha, flags );

	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		UTIL_ScreenFadeWrite( fade, pPlayer );
	}
}

void UTIL_ScreenFadeAll( const Vector& fadeSource, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	int i;

	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer)
		{
			UTIL_ScreenFade( fadeSource, pPlayer, color, fadeTime, fadeHold, alpha, flags );
		}
	}
}

void UTIL_ScreenFade( CBaseEntity *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	ScreenFade fade;

	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, alpha, flags );
	UTIL_ScreenFadeWrite( fade, pEntity );
}

void UTIL_ScreenFade( const Vector& fadeSource, CBaseEntity *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	alpha = CalculateFadeAlpha(fadeSource, pEntity, alpha);
	if (alpha > 0)
		UTIL_ScreenFade( pEntity, color, fadeTime, fadeHold, alpha, flags );
}

void UTIL_HudMessage( CBaseEntity *pEntity, const hudtextparms_t &textparms, const char *pMessage )
{
	if( !pEntity || !pEntity->IsNetClient() )
		return;

	MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pEntity->edict() );
		WRITE_BYTE( TE_TEXTMESSAGE );
		WRITE_BYTE( textparms.channel & 0xFF );

		WRITE_SHORT( FixedSigned16( textparms.x, 1 << 13 ) );
		WRITE_SHORT( FixedSigned16( textparms.y, 1 << 13 ) );
		WRITE_BYTE( textparms.effect );

		WRITE_BYTE( textparms.r1 );
		WRITE_BYTE( textparms.g1 );
		WRITE_BYTE( textparms.b1 );
		WRITE_BYTE( textparms.a1 );

		WRITE_BYTE( textparms.r2 );
		WRITE_BYTE( textparms.g2 );
		WRITE_BYTE( textparms.b2 );
		WRITE_BYTE( textparms.a2 );

		WRITE_SHORT( FixedUnsigned16( textparms.fadeinTime, 1 << 8 ) );
		WRITE_SHORT( FixedUnsigned16( textparms.fadeoutTime, 1 << 8 ) );
		WRITE_SHORT( FixedUnsigned16( textparms.holdTime, 1 << 8 ) );

		if( textparms.effect == 2 )
			WRITE_SHORT( FixedUnsigned16( textparms.fxTime, 1 << 8 ) );

		if( strlen( pMessage ) < 512 )
		{
			WRITE_STRING( pMessage );
		}
		else
		{
			char tmp[512];
			strncpyEnsureTermination( tmp, pMessage );
			WRITE_STRING( tmp );
		}
	MESSAGE_END();
}

void UTIL_HudMessageAll( const hudtextparms_t &textparms, const char *pMessage )
{
	int i;

	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if( pPlayer )
			UTIL_HudMessage( pPlayer, textparms, pMessage );
	}
}

extern int gmsgCaption;
void UTIL_ShowCaption(const char *messageId, int holdTime, bool radio)
{
	if (!messageId || !*messageId)
		return;
	if (*messageId == '!')
	{
		messageId = messageId+1;
	}

	if (holdTime > 255)
		holdTime = 255;

	MESSAGE_BEGIN( MSG_ALL, gmsgCaption );
		WRITE_BYTE(holdTime);
		WRITE_BYTE(radio ? 1 : 0);
		WRITE_STRING(messageId);
	MESSAGE_END();
}
				 
extern int gmsgTextMsg, gmsgSayText;
void UTIL_ClientPrintAll( int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgTextMsg );
		WRITE_BYTE( msg_dest );
		WRITE_STRING( msg_name );

		if( param1 )
			WRITE_STRING( param1 );
		if( param2 )
			WRITE_STRING( param2 );
		if( param3 )
			WRITE_STRING( param3 );
		if( param4 )
			WRITE_STRING( param4 );
	MESSAGE_END();
}

void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgTextMsg, NULL, client );
		WRITE_BYTE( msg_dest );
		WRITE_STRING( msg_name );

		if( param1 )
			WRITE_STRING( param1 );
		if( param2 )
			WRITE_STRING( param2 );
		if( param3 )
			WRITE_STRING( param3 );
		if( param4 )
			WRITE_STRING( param4 );
	MESSAGE_END();
}

void UTIL_SayText( const char *pText, CBaseEntity *pEntity )
{
	if( !pEntity->IsNetClient() )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pEntity->edict() );
		WRITE_BYTE( pEntity->entindex() );
		WRITE_STRING( pText );
	MESSAGE_END();
}

void UTIL_SayTextAll( const char *pText, CBaseEntity *pEntity )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
		WRITE_BYTE( pEntity->entindex() );
		WRITE_STRING( pText );
	MESSAGE_END();
}

char *UTIL_dtos1( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

char *UTIL_dtos2( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

char *UTIL_dtos3( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

char *UTIL_dtos4( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

void UTIL_ShowMessage( const char *pString, CBaseEntity *pEntity )
{
	if( !pEntity || !pEntity->IsNetClient() )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgHudText, NULL, pEntity->edict() );
	WRITE_STRING( pString );
	MESSAGE_END();
}

void UTIL_ShowMessageAll( const char *pString )
{
	int i;

	// loop through all players
	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if( pPlayer )
			UTIL_ShowMessage( pString, pPlayer );
	}
}

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_LINE( vecStart, vecEnd, ( igmon == ignore_monsters ? 1 : 0 ) | ( ignoreGlass ? 0x100 : 0 ), pentIgnore, ptr );
}

void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_LINE( vecStart, vecEnd, ( igmon == ignore_monsters ? 1 : 0 ), pentIgnore, ptr );
}

void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_HULL( vecStart, vecEnd, ( igmon == ignore_monsters ? 1 : 0 ), hullNumber, pentIgnore, ptr );
}

void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, int hullNumber, edict_t *pentModel, TraceResult *ptr )
{
	g_engfuncs.pfnTraceModel( vecStart, vecEnd, hullNumber, pentModel, ptr );
}

TraceResult UTIL_GetGlobalTrace( )
{
	TraceResult tr;

	tr.fAllSolid		= (int)gpGlobals->trace_allsolid;
	tr.fStartSolid		= (int)gpGlobals->trace_startsolid;
	tr.fInOpen		= (int)gpGlobals->trace_inopen;
	tr.fInWater		= (int)gpGlobals->trace_inwater;
	tr.flFraction		= gpGlobals->trace_fraction;
	tr.flPlaneDist		= gpGlobals->trace_plane_dist;
	tr.pHit			= gpGlobals->trace_ent;
	tr.vecEndPos		= gpGlobals->trace_endpos;
	tr.vecPlaneNormal	= gpGlobals->trace_plane_normal;
	tr.iHitgroup		= gpGlobals->trace_hitgroup;
	return tr;
}

void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax )
{
	SET_SIZE( ENT( pev ), vecMin, vecMax );
}
	
float UTIL_VecToYaw( const Vector &vec )
{
	return VEC_TO_YAW( vec );
}

void UTIL_SetOrigin( entvars_t *pev, const Vector &vecOrigin )
{
	edict_t *ent = ENT( pev );
	if( ent )
		SET_ORIGIN( ent, vecOrigin );
}

void UTIL_ParticleEffect( const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount )
{
	PARTICLE_EFFECT( vecOrigin, vecDirection, (float)ulColor, (float)ulCount );
}

float UTIL_SplineFraction( float value, float scale )
{
	value = scale * value;
	float valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}

char *UTIL_VarArgs( const char *format, ... )
{
	va_list	argptr;
	static char string[1024];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	return string;
}

Vector UTIL_GetAimVector( edict_t *pent, float flSpeed )
{
	Vector tmp;
	GET_AIM_VECTOR( pent, flSpeed, tmp );
	return tmp;
}

bool UTIL_IsMasterTriggered( string_t sMaster, CBaseEntity *pActivator )
{
	if( sMaster )
	{
		bool reverse = false;
		const char* szMaster = STRING( sMaster );
		if (szMaster[0] == '~')
		{
			szMaster++;
			reverse = true;
		}

		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, szMaster );

		if( !FNullEnt( pentTarget ) )
		{
			CBaseEntity *pMaster = CBaseEntity::Instance( pentTarget );
			if( pMaster && ( pMaster->ObjectCaps() & FCAP_MASTER ) )
			{
				if (reverse)
					return !pMaster->IsTriggered( pActivator );
				else
					return pMaster->IsTriggered( pActivator );
			}
		}

		ALERT( at_console, "Master %s was null or not a master!\n", STRING(sMaster) );
	}

	// if this isn't a master entity, just say yes.
	return true;
}

bool UTIL_IsPlayerReference(const char *name)
{
	return name != 0 && FStrEq(name, "*player");
}

bool UTIL_TargetnameIsActivator(const char *targetName)
{
	return targetName != 0 && (FStrEq(targetName, "*locus") || FStrEq(targetName, "!activator"));
}

bool UTIL_TargetnameIsActivator(string_t targetName)
{
	if (FStringNull(targetName))
		return false;
	return UTIL_TargetnameIsActivator(STRING(targetName));
}

bool UTIL_ShouldShowBlood( int color )
{
	extern cvar_t* violence_hblood;
	extern cvar_t* violence_ablood;
	if( color != DONT_BLEED )
	{
		if( color == BLOOD_COLOR_RED )
		{
			if( violence_hblood->value != 0 )
				return true;
		}
		else
		{
			if( violence_ablood->value != 0 )
				return true;
		}
	}
	return false;
}

int UTIL_PointContents(	const Vector &vec )
{
	return POINT_CONTENTS(vec);
}

void UTIL_BloodStream( const Vector &origin, const Vector &direction, int color, int amount )
{
	if( !UTIL_ShouldShowBlood( color ) )
		return;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_BLOODSTREAM );
		WRITE_VECTOR( origin );
		WRITE_VECTOR( direction );
		WRITE_BYTE( color );
		WRITE_BYTE( Q_min( amount, 255 ) );
	MESSAGE_END();
}				

void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount )
{
	if( !UTIL_ShouldShowBlood( color ) )
		return;

	if( color == DONT_BLEED || amount == 0 )
		return;

	if( g_pGameRules->IsMultiplayer() )
	{
		// scale up blood effect in multiplayer for better visibility
		amount *= 2;
	}

	if( amount > 255 )
		amount = 255;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_BLOODSPRITE );
		WRITE_VECTOR( origin );								// pos
		WRITE_SHORT( g_sModelIndexBloodSpray );				// initial sprite model
		WRITE_SHORT( g_sModelIndexBloodDrop );				// droplet sprite models
		WRITE_BYTE( color );								// color index into host_basepal
		WRITE_BYTE( Q_min( Q_max( 3, amount / 10 ), 16 ) );		// size
	MESSAGE_END();
}				

Vector UTIL_RandomBloodVector( void )
{
	Vector direction;

	direction.x = RANDOM_FLOAT( -1, 1 );
	direction.y = RANDOM_FLOAT( -1, 1 );
	direction.z = RANDOM_FLOAT( 0, 1 );

	return direction;
}

void UTIL_BloodDecalTrace( TraceResult *pTrace, int bloodColor )
{
	if( UTIL_ShouldShowBlood( bloodColor ) )
	{
		if( bloodColor == BLOOD_COLOR_RED )
			UTIL_DecalTrace( pTrace, DECAL_BLOOD1 + RANDOM_LONG( 0, 5 ) );
		else
			UTIL_DecalTrace( pTrace, DECAL_YBLOOD1 + RANDOM_LONG( 0, 5 ) );
	}
}

void UTIL_DecalTrace( TraceResult *pTrace, int decalNumber )
{
	short entityIndex;
	int index;
	int message;

	if( decalNumber < 0 )
		return;

	index = gDecals[decalNumber].index;

	if( index < 0 )
		return;

	if( pTrace->flFraction == 1.0f )
		return;

	// Only decal BSP models
	if( pTrace->pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( pTrace->pHit );
		if( pEntity && !pEntity->IsBSPModel() )
			return;
		entityIndex = ENTINDEX( pTrace->pHit );
	}
	else
		entityIndex = 0;

	message = TE_DECAL;
	if( entityIndex != 0 )
	{
		if( index > 255 )
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if( index > 255 )
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( message );
		WRITE_VECTOR( pTrace->vecEndPos );
		WRITE_BYTE( index );
		if( entityIndex )
			WRITE_SHORT( entityIndex );
	MESSAGE_END();
}

/*
==============
UTIL_PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void UTIL_PlayerDecalTrace( TraceResult *pTrace, int playernum, int decalNumber, bool bIsCustom )
{
	int index;

	if( !bIsCustom )
	{
		if( decalNumber < 0 )
			return;

		index = gDecals[decalNumber].index;
		if( index < 0 )
			return;
	}
	else
		index = decalNumber;

	if( pTrace->flFraction == 1.0f )
		return;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_PLAYERDECAL );
		WRITE_BYTE( playernum );
		WRITE_VECTOR( pTrace->vecEndPos );
		WRITE_SHORT( (short)ENTINDEX( pTrace->pHit ) );
		WRITE_BYTE( index );
	MESSAGE_END();
}

void UTIL_GunshotDecalTrace( TraceResult *pTrace, int decalNumber )
{
	if( decalNumber < 0 )
		return;

	int index = gDecals[decalNumber].index;
	if( index < 0 )
		return;

	if( pTrace->flFraction == 1.0f )
		return;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pTrace->vecEndPos );
		WRITE_BYTE( TE_GUNSHOTDECAL );
		WRITE_VECTOR( pTrace->vecEndPos );
		WRITE_SHORT( (short)ENTINDEX( pTrace->pHit ) );
		WRITE_BYTE( index );
	MESSAGE_END();
}

void UTIL_Sparks( const Vector &position )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_SPARKS );
		WRITE_VECTOR( position );
	MESSAGE_END();
}

void UTIL_SparkShower(const Vector &position, const SparkEffectParams& params)
{
	extern int gmsgSparkShower;
	MESSAGE_BEGIN( MSG_PVS, gmsgSparkShower, position );
		WRITE_VECTOR( position );
		WRITE_SHORT( params.sparkModelIndex );
		WRITE_SHORT( params.streakCount );
		WRITE_SHORT( params.streakVelocity );
		WRITE_SHORT( short(params.sparkDuration * 100) );
		WRITE_SHORT( short(params.sparkScaleMin * 100) );
		WRITE_SHORT( short(params.sparkScaleMax * 100) );
		WRITE_SHORT( params.flags );
	MESSAGE_END();
}

void UTIL_Ricochet( const Vector &position, float scale )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_ARMOR_RICOCHET );
		WRITE_VECTOR( position );
		WRITE_BYTE( (int)( scale * 10.0f ) );
	MESSAGE_END();
}

bool UTIL_TeamsMatch( const char *pTeamName1, const char *pTeamName2 )
{
	// Everyone matches unless it's teamplay
	if( !g_pGameRules->IsTeamplay() )
		return true;

	// Both on a team?
	if( *pTeamName1 != 0 && *pTeamName2 != 0 )
	{
		if( !stricmp( pTeamName1, pTeamName2 ) )	// Same Team?
			return true;
	}

	return false;
}

Vector UTIL_StringToVector(const char* str)
{
	float x, y, z;
	if (sscanf( str, "%f %f %f", &x, &y, &z) == 3) {
		return Vector(x, y, z);
	}
	return g_vecZero;
}

//LRC - randomized vectors of the form "0 0 0 .. 1 0 0"
void UTIL_StringToRandomVector( float *pVector, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;
	float pAltVec[3];

	strcpy( tempString, pString );
	pstr = pfront = tempString;

	for ( j = 0; j < 3; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atof( pfront );

		while ( *pstr && *pstr != ' ' ) pstr++;
		if (!*pstr) break;
		pstr++;
		pfront = pstr;
	}
	if (j < 2)
	{
		for (j = j+1;j < 3; j++)
			pVector[j] = 0;
	}
	else if (*pstr == '.')
	{
		pstr++;
		if (*pstr != '.') return;
		pstr++;
		if (*pstr != ' ') return;

		UTIL_StringToVector(pAltVec, pstr);

		pVector[0] = RANDOM_FLOAT( pVector[0], pAltVec[0] );
		pVector[1] = RANDOM_FLOAT( pVector[1], pAltVec[1] );
		pVector[2] = RANDOM_FLOAT( pVector[2], pAltVec[2] );
	}
}

template<typename T>
void UTIL_StringToIntegerArray( T *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int j;

	strncpyEnsureTermination( tempString, pString );
	pstr = pfront = tempString;

	for( j = 0; j < count; j++ )
	{
		pVector[j] = (T)atoi( pfront );

		while( *pstr && *pstr != ' ' )
			pstr++;
		if( !(*pstr) )
			break;
		pstr++;
		pfront = pstr;
	}

	for( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

void UTIL_StringToIntArray( int *pVector, int count, const char *pString )
{
	UTIL_StringToIntegerArray(pVector, count, pString);
}

void UTIL_StringToCharArray( char *pVector, int count, const char *pString )
{
	UTIL_StringToIntegerArray(pVector, count, pString);
}

Vector UTIL_ClampVectorToBox( const Vector &input, const Vector &clampSize )
{
	Vector sourceVector = input;

	if( sourceVector.x > clampSize.x )
		sourceVector.x -= clampSize.x;
	else if( sourceVector.x < -clampSize.x )
		sourceVector.x += clampSize.x;
	else
		sourceVector.x = 0;

	if( sourceVector.y > clampSize.y )
		sourceVector.y -= clampSize.y;
	else if( sourceVector.y < -clampSize.y )
		sourceVector.y += clampSize.y;
	else
		sourceVector.y = 0;

	if( sourceVector.z > clampSize.z )
		sourceVector.z -= clampSize.z;
	else if( sourceVector.z < -clampSize.z )
		sourceVector.z += clampSize.z;
	else
		sourceVector.z = 0;

	return sourceVector.Normalize();
}

float UTIL_WaterLevel( const Vector &position, float minz, float maxz )
{
	Vector midUp = position;
	midUp.z = minz;

	if( UTIL_PointContents( midUp ) != CONTENTS_WATER )
		return minz;

	midUp.z = maxz;
	if( UTIL_PointContents( midUp ) == CONTENTS_WATER )
		return maxz;

	float diff = maxz - minz;
	while( diff > 1.0f )
	{
		midUp.z = minz + diff / 2.0f;
		if( UTIL_PointContents( midUp ) == CONTENTS_WATER )
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}

extern DLL_GLOBAL short g_sModelIndexBubbles;// holds the index for the bubbles model

void UTIL_Bubbles( Vector mins, Vector maxs, int count )
{
	Vector mid = ( mins + maxs ) * 0.5f;

	float flHeight = UTIL_WaterLevel( mid, mid.z, mid.z + 1024 );
	flHeight = flHeight - mins.z;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, mid );
		WRITE_BYTE( TE_BUBBLES );
		WRITE_VECTOR( mins );	// mins
		WRITE_VECTOR( maxs );	// maxz
		WRITE_COORD( flHeight );			// height
		WRITE_SHORT( g_sModelIndexBubbles );
		WRITE_BYTE( count ); // count
		WRITE_COORD( 8 ); // speed
	MESSAGE_END();
}

void UTIL_BubbleTrail( Vector from, Vector to, int count )
{
	float flHeight = UTIL_WaterLevel( from, from.z, from.z + 256 );
	flHeight = flHeight - from.z;

	if( flHeight < 8 )
	{
		flHeight = UTIL_WaterLevel( to, to.z, to.z + 256 );
		flHeight = flHeight - to.z;
		if( flHeight < 8 )
			return;

		// UNDONE: do a ploink sound
		flHeight = flHeight + to.z - from.z;
	}

	if( count > 255 )
		count = 255;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BUBBLETRAIL );
		WRITE_VECTOR( from );	// mins
		WRITE_VECTOR( to );	// maxz
		WRITE_COORD( flHeight );			// height
		WRITE_SHORT( g_sModelIndexBubbles );
		WRITE_BYTE( count ); // count
		WRITE_COORD( 8 ); // speed
	MESSAGE_END();
}

void UTIL_Remove( CBaseEntity *pEntity )
{
	if( !pEntity )
		return;

	pEntity->UpdateOnRemove();
	pEntity->pev->flags |= FL_KILLME;
	pEntity->pev->targetname = 0;
}

bool UTIL_IsValidEntity( edict_t *pent )
{
	if( !pent || pent->free || ( pent->v.flags & FL_KILLME ) )
		return false;
	return true;
}

static void UTIL_PrecacheOtherWithOverride(CBaseEntity* pEntity, EntityOverrides entityOverrides)
{
	pEntity->AssignEntityOverrides(entityOverrides);
	pEntity->Precache();
}

void UTIL_PrecacheOther( const char *szClassname, EntityOverrides entityOverrides )
{
	edict_t	*pent;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in UTIL_PrecacheOther\n" );
		return;
	}
	
	CBaseEntity *pEntity = CBaseEntity::Instance( VARS( pent ) );
	if( pEntity && pEntity->IsEnabledInMod() )
	{
		UTIL_PrecacheOtherWithOverride(pEntity, entityOverrides);
	}
	REMOVE_ENTITY( pent );
}

bool UTIL_PrecacheMonster(const char *szClassname, bool reverseRelationship, Vector* vecMin, Vector* vecMax, EntityOverrides entityOverrides)
{
	edict_t	*pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in UTIL_PrecacheMonster (%s)\n", szClassname );
		return false;
	}

	bool enabled = true;
	CBaseEntity *pEntity = CBaseEntity::Instance( VARS( pent ) );
	if( pEntity )
	{
		CBaseMonster *pMonster = pEntity->MyMonsterPointer();
		if (pMonster)
		{
			pMonster->m_reverseRelationship = reverseRelationship;
			if (vecMin)
				*vecMin = pMonster->DefaultMinHullSize();
			if (vecMax)
				*vecMax = pMonster->DefaultMaxHullSize();
		}
		enabled = pEntity->IsEnabledInMod();
		if (enabled)
		{
			UTIL_PrecacheOtherWithOverride(pEntity, entityOverrides);
		}
	}
	REMOVE_ENTITY( pent );
	return enabled;
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( const char *fmt, ... )
{
	va_list		argptr;
	static char	string[1024];
	
	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	// Print to server console
	ALERT( at_logged, "%s", string );
}

//=========================================================
// UTIL_DotPoints - returns the dot product of a line from
// src to check and vecdir.
//=========================================================
float UTIL_DotPoints( const Vector &vecSrc, const Vector &vecCheck, const Vector &vecDir )
{
	Vector2D vec2LOS;

	vec2LOS = ( vecCheck - vecSrc ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	return DotProduct( vec2LOS, ( vecDir.Make2D() ) );
}

//=========================================================
// UTIL_StripToken - for redundant keynames
//=========================================================
void UTIL_StripToken( const char *pKey, char *pDest, int nLen )
{
	int i = 0;

	while( i < nLen - 1 && pKey[i] && pKey[i] != '#' )
	{
		pDest[i] = pKey[i];
		i++;
	}
	pDest[i] = 0;
}

// --------------------------------------------------------------
//
// CSave
//
// --------------------------------------------------------------
static int gSizes[FIELD_TYPECOUNT] =
{
	sizeof(float),		// FIELD_FLOAT
	sizeof(string_t),		// FIELD_STRING
	sizeof(void*),		// FIELD_ENTITY
	sizeof(void*),		// FIELD_CLASSPTR
	sizeof(EHANDLE),	// FIELD_EHANDLE
	sizeof(void*),		// FIELD_entvars_t
	sizeof(void*),		// FIELD_EDICT
	sizeof(float) * 3,	// FIELD_VECTOR
	sizeof(float) * 3,	// FIELD_POSITION_VECTOR
	sizeof(void *),		// FIELD_POINTER
	sizeof(int),		// FIELD_INTEGER
#if GNUC
	sizeof(void *) * 2,	// FIELD_FUNCTION
#else
	sizeof(void *),		// FIELD_FUNCTION	
#endif
	sizeof(byte),		// FIELD_BOOLEAN
	sizeof(short),		// FIELD_SHORT
	sizeof(char),		// FIELD_CHARACTER
	sizeof(float),		// FIELD_TIME
	sizeof(int),		// FIELD_MODELNAME
	sizeof(int),		// FIELD_SOUNDNAME
	sizeof(std::uint64_t), //FIELD_INT64
};

// entities has different store size
static int gInputSizes[FIELD_TYPECOUNT] =
{
	sizeof(float),		// FIELD_FLOAT
	sizeof(string_t),		// FIELD_STRING
	sizeof(int),		// FIELD_ENTITY
	sizeof(int),		// FIELD_CLASSPTR
	sizeof(int),		// FIELD_EHANDLE
	sizeof(int),		// FIELD_entvars_t
	sizeof(int),		// FIELD_EDICT
	sizeof(float) * 3,	// FIELD_VECTOR
	sizeof(float) * 3,	// FIELD_POSITION_VECTOR
	sizeof(void *),		// FIELD_POINTER
	sizeof(int),		// FIELD_INTEGER
#if GNUC
	sizeof(void *) * 2,	// FIELD_FUNCTION
#else
	sizeof(void *),		// FIELD_FUNCTION
#endif
	sizeof(byte),		// FIELD_BOOLEAN
	sizeof(short),		// FIELD_SHORT
	sizeof(char),		// FIELD_CHARACTER
	sizeof(float),		// FIELD_TIME
	sizeof(int),		// FIELD_MODELNAME
	sizeof(int),		// FIELD_SOUNDNAME
};

// Base class includes common SAVERESTOREDATA pointer, and manages the entity table
CSaveRestoreBuffer::CSaveRestoreBuffer( void )
{
	m_pdata = NULL;
}

CSaveRestoreBuffer::CSaveRestoreBuffer( SAVERESTOREDATA *pdata )
{
	m_pdata = pdata;
}

CSaveRestoreBuffer::~CSaveRestoreBuffer( void )
{
}

int CSaveRestoreBuffer::EntityIndex( CBaseEntity *pEntity )
{
	if( pEntity == NULL )
		return -1;
	return EntityIndex( pEntity->pev );
}

int CSaveRestoreBuffer::EntityIndex( entvars_t *pevLookup )
{
	if( pevLookup == NULL )
		return -1;
	return EntityIndex( ENT( pevLookup ) );
}

int CSaveRestoreBuffer::EntityIndex( EOFFSET eoLookup )
{
	return EntityIndex( ENT( eoLookup ) );
}


int CSaveRestoreBuffer::EntityIndex( edict_t *pentLookup )
{
	if( !m_pdata || pentLookup == NULL )
		return -1;

	int i;
	ENTITYTABLE *pTable;

	for( i = 0; i < m_pdata->tableCount; i++ )
	{
		pTable = m_pdata->pTable + i;
		if( pTable->pent == pentLookup )
			return i;
	}
	return -1;
}

edict_t *CSaveRestoreBuffer::EntityFromIndex( int entityIndex )
{
	if( !m_pdata || entityIndex < 0 )
		return NULL;

	int i;
	ENTITYTABLE *pTable;

	for( i = 0; i < m_pdata->tableCount; i++ )
	{
		pTable = m_pdata->pTable + i;
		if( pTable->id == entityIndex )
			return pTable->pent;
	}
	return NULL;
}

int CSaveRestoreBuffer::EntityFlagsSet( int entityIndex, int flags )
{
	if( !m_pdata || entityIndex < 0 )
		return 0;
	if( entityIndex > m_pdata->tableCount )
		return 0;

	m_pdata->pTable[entityIndex].flags |= flags;

	return m_pdata->pTable[entityIndex].flags;
}

void CSaveRestoreBuffer::BufferRewind( int size )
{
	if( !m_pdata )
		return;

	if( m_pdata->size < size )
		size = m_pdata->size;

	m_pdata->pCurrentData -= size;
	m_pdata->size -= size;
}

#if !XASH_WIN32 && !__WATCOMC__
static unsigned _rotr( unsigned val, int shift )
{
	// Any modern compiler will generate one single ror instruction for x86, arm and mips here.
	return ( val >> shift ) | ( val << ( 32 - shift ));
}
#endif

unsigned int CSaveRestoreBuffer::HashString( const char *pszToken )
{
	unsigned int hash = 0;

	while( *pszToken )
		hash = _rotr( hash, 4 ) ^ *pszToken++;

	return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash( const char *pszToken )
{
	unsigned short hash = (unsigned short)( HashString( pszToken ) % (unsigned)m_pdata->tokenCount );
#if _DEBUG
	static int tokensparsed = 0;
	tokensparsed++;
	if( !m_pdata->tokenCount || !m_pdata->pTokens )
		ALERT( at_error, "No token table array in TokenHash()!\n" );
#endif
	for( int i = 0; i < m_pdata->tokenCount; i++ )
	{
#if _DEBUG
		static qboolean beentheredonethat = 0;
		if( i > 50 && !beentheredonethat )
		{
			beentheredonethat = 1;
			ALERT( at_error, "CSaveRestoreBuffer :: TokenHash() is getting too full!\n" );
		}
#endif
		int index = hash + i;
		if( index >= m_pdata->tokenCount )
			index -= m_pdata->tokenCount;

		if( !m_pdata->pTokens[index] || strcmp( pszToken, m_pdata->pTokens[index] ) == 0 )
		{
			m_pdata->pTokens[index] = (char *)pszToken;
			return index;
		}
	}

	// Token hash table full!!! 
	// [Consider doing overflow table(s) after the main table & limiting linear hash table search]
	ALERT( at_error, "CSaveRestoreBuffer :: TokenHash() is COMPLETELY FULL!\n" );
	return 0;
}

void CSave::WriteData( const char *pname, int size, const char *pdata )
{
	BufferField( pname, size, pdata );
}

void CSave::WriteShort( const char *pname, const short *data, int count )
{
	BufferField( pname, sizeof(short) * count, (const char *)data );
}

void CSave::WriteInt( const char *pname, const int *data, int count )
{
	BufferField( pname, sizeof(int) * count, (const char *)data );
}

void CSave::WriteFloat( const char *pname, const float *data, int count )
{
	BufferField( pname, sizeof(float) * count, (const char *)data );
}

void CSave::WriteTime( const char *pname, const float *data, int count )
{
	int i;
	//Vector tmp, input;

	BufferHeader( pname, sizeof(float) * count );
	for( i = 0; i < count; i++ )
	{
		float tmp = data[0];

		// Always encode time as a delta from the current time so it can be re-based if loaded in a new level
		// Times of 0 are never written to the file, so they will be restored as 0, not a relative time
		if( m_pdata )
			tmp -= m_pdata->time;

		BufferData( (const char *)&tmp, sizeof(float) );
		data ++;
	}
}

void CSave::WriteString( const char *pname, const char *pdata )
{
#if TOKENIZE
	short token = (short)TokenHash( pdata );
	WriteShort( pname, &token, 1 );
#else
	BufferField( pname, strlen( pdata ) + 1, pdata );
#endif
}

void CSave::WriteString( const char *pname, const int *stringId, int count )
{
	int i, size;
#if TOKENIZE
	short token = (short)TokenHash( STRING( *stringId ) );
	WriteShort( pname, &token, 1 );
#else
#if 0
	if( count != 1 )
		ALERT( at_error, "No string arrays!\n" );
	WriteString( pname, STRING( *stringId ) );
#endif
	size = 0;
	for( i = 0; i < count; i++ )
		size += strlen( STRING( stringId[i] ) ) + 1;

	BufferHeader( pname, size );
	for( i = 0; i < count; i++ )
	{
		const char *pString = STRING( stringId[i] );
		BufferData( pString, strlen( pString ) + 1 );
	}
#endif
}

void CSave::WriteVector( const char *pname, const Vector &value )
{
	WriteVector( pname, &value.x, 1 );
}

void CSave::WriteVector( const char *pname, const float *value, int count )
{
	BufferHeader( pname, sizeof(float) * 3 * count );
	BufferData( (const char *)value, sizeof(float) * 3 * count );
}

void CSave::WritePositionVector( const char *pname, const Vector &value )
{
	if( m_pdata && m_pdata->fUseLandmark )
	{
		Vector tmp = value - m_pdata->vecLandmarkOffset;
		WriteVector( pname, tmp );
	}

	WriteVector( pname, value );
}

void CSave::WritePositionVector( const char *pname, const float *value, int count )
{
	int i;
	//Vector tmp, input;

	BufferHeader( pname, sizeof(float) * 3 * count );
	for( i = 0; i < count; i++ )
	{
		Vector tmp( value[0], value[1], value[2] );

		if( m_pdata && m_pdata->fUseLandmark )
			tmp = tmp - m_pdata->vecLandmarkOffset;

		BufferData( (const char *)&tmp.x, sizeof(float) * 3 );
		value += 3;
	}
}

void CSave::WriteFunction( const char *pname, void **data, int count )
{
	const char *functionName;

	functionName = NAME_FOR_FUNCTION( *data );
	if( functionName )
		BufferField( pname, strlen( functionName ) + 1, functionName );
	else
		ALERT( at_error, "Invalid function pointer in entity!\n" );
}

void EntvarsKeyvalue( entvars_t *pev, KeyValueData *pkvd )
{
	int i;
	TYPEDESCRIPTION *pField;

	for( i = 0; i < (int)ENTVARS_COUNT; i++ )
	{
		pField = &gEntvarsDescription[i];

		if( !stricmp( pField->fieldName, pkvd->szKeyName ) )
		{
			switch( pField->fieldType )
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				( *(string_t *)( (char *)pev + pField->fieldOffset ) ) = ALLOC_STRING( pkvd->szValue );
				break;
			case FIELD_TIME:
			case FIELD_FLOAT:
				( *(float *)( (char *)pev + pField->fieldOffset ) ) = atof( pkvd->szValue );
				break;
			case FIELD_INTEGER:
				( *(int *)( (char *)pev + pField->fieldOffset ) ) = atoi( pkvd->szValue );
				break;
			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
			{
				int componentsRead = 0;
				UTIL_StringToVector( (float *)( (char *)pev + pField->fieldOffset ), pkvd->szValue, &componentsRead );
				if (componentsRead != 3 && DeveloperModeLevel() >= 4)
				{
					ALERT( at_warning, "Incorrect number of components for vector. %s:%s == \"%s\"\n", pkvd->szClassName, pField->fieldName, pkvd->szValue );
				}
			}
				break;
			case FIELD_CHARACTER:
				UTIL_StringToCharArray((char *)pev + pField->fieldOffset, pField->fieldSize, pkvd->szValue);
				break;
			default:
			case FIELD_EVARS:
			case FIELD_CLASSPTR:
			case FIELD_EDICT:
			case FIELD_ENTITY:
			case FIELD_POINTER:
				ALERT( at_error, "Bad field in entity!! %s:%s == \"%s\"\n", pkvd->szClassName, pField->fieldName, pkvd->szValue );
				break;
			}
			pkvd->fHandled = true;
			return;
		}
	}
}

CKeyValue ReadEntvarKeyvalue(entvars_t* pev, const char* keyName)
{
	CKeyValue keyValue;
	keyValue.keyName = keyName;

	for( int i = 0; i < (int)ENTVARS_COUNT; i++ )
	{
		TYPEDESCRIPTION *pField = &gEntvarsDescription[i];
		if( stricmp( pField->fieldName, keyName ) == 0 )
		{
			keyValue.fieldType = pField->fieldType;
			keyValue.offset = pField->fieldOffset;
			switch( pField->fieldType )
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				keyValue.sVal = ( *(string_t *)( (char *)pev + pField->fieldOffset ) );
				keyValue.keyType = KEY_TYPE_STRING;
				break;
			case FIELD_TIME:
			case FIELD_FLOAT:
				keyValue.fVal = ( *(float *)( (char *)pev + pField->fieldOffset ) );
				keyValue.keyType = KEY_TYPE_FLOAT;
				break;
			case FIELD_INTEGER:
				keyValue.iVal = ( *(int *)( (char *)pev + pField->fieldOffset ) );
				keyValue.keyType = KEY_TYPE_INT;
				break;
			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				keyValue.vVal = Vector((float *)( (char *)pev + pField->fieldOffset ));
				keyValue.keyType = KEY_TYPE_VECTOR;
				break;
			case FIELD_EDICT:
				keyValue.eVal = ( *(edict_t **)( (char *)pev + pField->fieldOffset ) );
				keyValue.keyType = KEY_TYPE_EDICT;
			}
			return keyValue;
		}
	}
	return keyValue;
}

int CSave::WriteEntVars( const char *pname, entvars_t *pev )
{
	return WriteFields( pname, pev, gEntvarsDescription, ENTVARS_COUNT );
}

int CSave::WriteFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	int i, j, actualCount, emptyCount;
	TYPEDESCRIPTION	*pTest;
	int entityArray[MAX_ENTITYARRAY];
	byte boolArray[MAX_ENTITYARRAY];

	// Precalculate the number of empty fields
	emptyCount = 0;
	for( i = 0; i < fieldCount; i++ )
	{
		void *pOutputData;
		pOutputData = ( (char *)pBaseData + pFields[i].fieldOffset );
		if( DataEmpty( (const char *)pOutputData, pFields[i].fieldSize * gSizes[pFields[i].fieldType] ) )
			emptyCount++;
	}

	// Empty fields will not be written, write out the actual number of fields to be written
	actualCount = fieldCount - emptyCount;
	WriteInt( pname, &actualCount, 1 );

	for( i = 0; i < fieldCount; i++ )
	{
		void *pOutputData;
		pTest = &pFields[i];
		pOutputData = ( (char *)pBaseData + pTest->fieldOffset );

		// UNDONE: Must we do this twice?
		if( DataEmpty( (const char *)pOutputData, pTest->fieldSize * gSizes[pTest->fieldType] ) )
			continue;

		switch( pTest->fieldType )
		{
		case FIELD_FLOAT:
			WriteFloat( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
			break;
		case FIELD_TIME:
			WriteTime( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
			break;
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			WriteString( pTest->fieldName, (string_t *)pOutputData, pTest->fieldSize );
			break;
		case FIELD_CLASSPTR:
		case FIELD_EVARS:
		case FIELD_EDICT:
		case FIELD_ENTITY:
		case FIELD_EHANDLE:
			if( pTest->fieldSize > MAX_ENTITYARRAY )
				ALERT( at_error, "Can't save more than %d entities in an array!!!\n", MAX_ENTITYARRAY );
			for( j = 0; j < pTest->fieldSize; j++ )
			{
				switch( pTest->fieldType )
				{
					case FIELD_EVARS:
						entityArray[j] = EntityIndex( ( (entvars_t **)pOutputData )[j] );
						break;
					case FIELD_CLASSPTR:
						entityArray[j] = EntityIndex( ( (CBaseEntity **)pOutputData )[j] );
						break;
					case FIELD_EDICT:
						entityArray[j] = EntityIndex( ( (edict_t **)pOutputData )[j] );
						break;
					case FIELD_ENTITY:
						entityArray[j] = EntityIndex( ( (EOFFSET *)pOutputData )[j] );
						break;
					case FIELD_EHANDLE:
						entityArray[j] = EntityIndex( (CBaseEntity *)( ( (EHANDLE *)pOutputData)[j] ) );
						break;
					default:
						break;
				}
			}
			WriteInt( pTest->fieldName, entityArray, pTest->fieldSize );
			break;
		case FIELD_POSITION_VECTOR:
			WritePositionVector( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
			break;
		case FIELD_VECTOR:
			WriteVector( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
			break;
		case FIELD_BOOLEAN:
		{
			for (j = 0; j < pTest->fieldSize; j++)
			{
				boolArray[j] = ((bool*)pOutputData)[j] ? 1 : 0;
			}
			WriteData(pTest->fieldName, pTest->fieldSize, (char*)boolArray);
		}
			break;
		case FIELD_INTEGER:
			WriteInt( pTest->fieldName, (int *)pOutputData, pTest->fieldSize );
			break;
		case FIELD_SHORT:
			WriteData( pTest->fieldName, 2 * pTest->fieldSize, ( (char *)pOutputData ) );
			break;
		case FIELD_CHARACTER:
			WriteData( pTest->fieldName, pTest->fieldSize, ( (char *)pOutputData ) );
			break;
		// For now, just write the address out, we're not going to change memory while doing this yet!
		case FIELD_POINTER:
			WriteInt( pTest->fieldName, (int *)(char *)pOutputData, pTest->fieldSize );
			break;
		case FIELD_FUNCTION:
			WriteFunction( pTest->fieldName, (void **)pOutputData, pTest->fieldSize );
			break;
		case FIELD_INT64:
			WriteData( pTest->fieldName, sizeof(std::uint64_t) * pTest->fieldSize, ((char*)pOutputData) );
			break;
		default:
			ALERT( at_error, "Bad field type\n" );
		}
	}

	return 1;
}

void CSave::BufferString( char *pdata, int len )
{
	char c = 0;

	BufferData( pdata, len );		// Write the string
	BufferData( &c, 1 );			// Write a null terminator
}

int CSave::DataEmpty( const char *pdata, int size )
{
	for( int i = 0; i < size; i++ )
	{
		if( pdata[i] )
			return 0;
	}
	return 1;
}

void CSave::BufferField( const char *pname, int size, const char *pdata )
{
	BufferHeader( pname, size );
	BufferData( pdata, size );
}

void CSave::BufferHeader( const char *pname, int size )
{
	short hashvalue = TokenHash( pname );
	if( size > 1 << ( sizeof(short) * 8 ) )
		ALERT( at_error, "CSave :: BufferHeader() size parameter exceeds 'short'!\n" );
	BufferData( (const char *)&size, sizeof(short) );
	BufferData( (const char *)&hashvalue, sizeof(short) );
}

void CSave::BufferData( const char *pdata, int size )
{
	if( !m_pdata )
		return;

	if( m_pdata->size + size > m_pdata->bufferSize )
	{
		ALERT( at_error, "Save/Restore overflow!\n" );
		m_pdata->size = m_pdata->bufferSize;
		return;
	}

	memcpy( m_pdata->pCurrentData, pdata, size );
	m_pdata->pCurrentData += size;
	m_pdata->size += size;
}

// --------------------------------------------------------------
//
// CRestore
//
// --------------------------------------------------------------
int CRestore::ReadField( void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount, int startField, int size, char *pName, void *pData )
{
	int i, j, stringCount, fieldNumber, entityIndex;
	TYPEDESCRIPTION *pTest;
	float time, timeData;
	Vector position;
	edict_t	*pent;
	char *pString;

	time = 0;
	position = Vector( 0, 0, 0 );

	if( m_pdata )
	{
		time = m_pdata->time;
		if( m_pdata->fUseLandmark )
			position = m_pdata->vecLandmarkOffset;
	}

	for( i = 0; i < fieldCount; i++ )
	{
		fieldNumber = ( i + startField ) % fieldCount;
		pTest = &pFields[fieldNumber];
		if( pTest->fieldName && !stricmp( pTest->fieldName, pName ) )
		{
			if( !m_global || !(pTest->flags & FTYPEDESC_GLOBAL ) )
			{
				for( j = 0; j < pTest->fieldSize; j++ )
				{
					void *pOutputData = ( (char *)pBaseData + pTest->fieldOffset + ( j * gSizes[pTest->fieldType] ) );
					void *pInputData = (char *)pData + j * gInputSizes[pTest->fieldType];

					switch( pTest->fieldType )
					{
					case FIELD_TIME:
					#if __VFP_FP__
						memcpy( &timeData, pInputData, 4 );
						// Re-base time variables
						timeData += time;
						memcpy( pOutputData, &timeData, 4 );
					#else
						timeData = *(float *)pInputData;
						// Re-base time variables
						timeData += time;
						*( (float *)pOutputData ) = timeData;
					#endif
						break;
					case FIELD_FLOAT:
						memcpy( pOutputData, pInputData, 4 );
						break;
					case FIELD_MODELNAME:
					case FIELD_SOUNDNAME:
					case FIELD_STRING:
						// Skip over j strings
						pString = (char *)pData;
						for( stringCount = 0; stringCount < j; stringCount++ )
						{
							while( *pString )
								pString++;
							pString++;
						}
						pInputData = pString;
						if( ( (char *)pInputData )[0] == '\0' )
							*( (string_t *)pOutputData ) = 0;
						else
						{
							string_t string;

							string = ALLOC_STRING( (char *)pInputData );

							*( (string_t *)pOutputData ) = string;

							if( !FStringNull( string ) && m_precache )
							{
								if( pTest->fieldType == FIELD_MODELNAME )
									PRECACHE_MODEL( STRING( string ) );
								else if( pTest->fieldType == FIELD_SOUNDNAME )
									PRECACHE_SOUND( STRING( string ) );
							}
						}
						break;
					case FIELD_EVARS:
						entityIndex = *( int *)pInputData;
						pent = EntityFromIndex( entityIndex );
						if( pent )
							*( (entvars_t **)pOutputData ) = VARS( pent );
						else
							*( (entvars_t **)pOutputData ) = NULL;
						break;
					case FIELD_CLASSPTR:
						entityIndex = *( int *)pInputData;
						pent = EntityFromIndex( entityIndex );
						if( pent )
							*( (CBaseEntity **)pOutputData ) = CBaseEntity::Instance( pent );
						else
							*( (CBaseEntity **)pOutputData ) = NULL;
						break;
					case FIELD_EDICT:
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex( entityIndex );
						*( (edict_t **)pOutputData ) = pent;
						break;
					case FIELD_EHANDLE:
						// Input and Output sizes are different!
						pInputData = (char*)pData + j * gInputSizes[pTest->fieldType];
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex( entityIndex );
						if( pent )
							*( (EHANDLE *)pOutputData ) = CBaseEntity::Instance( pent );
						else
							*( (EHANDLE *)pOutputData ) = NULL;
						break;
					case FIELD_ENTITY:
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex( entityIndex );
						if( pent )
							*( (EOFFSET *)pOutputData ) = OFFSET( pent );
						else
							*( (EOFFSET *)pOutputData ) = 0;
						break;
					case FIELD_VECTOR:
						#if __VFP_FP__
						memcpy( pOutputData, pInputData, sizeof( Vector ) );
						#else
						( (float *)pOutputData )[0] = ( (float *)pInputData )[0];
						( (float *)pOutputData )[1] = ( (float *)pInputData )[1];
						( (float *)pOutputData )[2] = ( (float *)pInputData )[2];
						#endif
						break;
					case FIELD_POSITION_VECTOR:
						#if  __VFP_FP__
						{
							Vector tmp;
							memcpy( &tmp, pInputData, sizeof( Vector ) );
							tmp = tmp + position;
							memcpy( pOutputData, &tmp, sizeof( Vector ) );
						}
						#else
						( (float *)pOutputData )[0] = ( (float *)pInputData )[0] + position.x;
						( (float *)pOutputData )[1] = ( (float *)pInputData )[1] + position.y;
						( (float *)pOutputData )[2] = ( (float *)pInputData )[2] + position.z;
						#endif
						break;
					case FIELD_BOOLEAN:
					{
						pOutputData = (char*)pOutputData + j * (sizeof(bool) - gSizes[pTest->fieldType]);
						const bool value = *((byte*)pInputData) != 0;
						*((bool*)pOutputData) = value;
					}
						break;
					case FIELD_INTEGER:
						*( (int *)pOutputData ) = *(int *)pInputData;
						break;
					case FIELD_SHORT:
						*( (short *)pOutputData ) = *(short *)pInputData;
						break;
					case FIELD_CHARACTER:
						*( (char *)pOutputData ) = *(char *)pInputData;
						break;
					case FIELD_POINTER:
						*( (void**)pOutputData ) = *(void **)pInputData;
						break;
					case FIELD_FUNCTION:
						if( ( (char *)pInputData )[0] == '\0' )
							*( (void**)pOutputData ) = 0;
						else
							*( (void**)pOutputData ) = (void*)FUNCTION_FROM_NAME( (char *)pInputData );
						break;
					case FIELD_INT64:
						*((std::uint64_t*)pOutputData) = *(std::uint64_t*)pInputData;
						break;
					default:
						ALERT( at_error, "Bad field type\n" );
					}
				}
			}
#if 0
			else
			{
				ALERT( at_console, "Skipping global field %s\n", pName );
			}
#endif
			return fieldNumber;
		}
	}
	return -1;
}

int CRestore::ReadEntVars( const char *pname, entvars_t *pev )
{
	return ReadFields( pname, pev, gEntvarsDescription, ENTVARS_COUNT );
}

int CRestore::ReadFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	unsigned short i, token;
	int lastField, fileCount;
	HEADER header;

	i = ReadShort();
	ASSERT( i == sizeof(int) );			// First entry should be an int

	token = ReadShort();

	// Check the struct name
	if( token != TokenHash(pname) )			// Field Set marker
	{
		//ALERT( at_error, "Expected %s found %s!\n", pname, BufferPointer() );
		BufferRewind( 2 * sizeof( short ) );
		return 0;
	}

	// Skip over the struct name
	fileCount = ReadInt();						// Read field count

	lastField = 0;								// Make searches faster, most data is read/written in the same order

	// Clear out base data
	for( i = 0; i < fieldCount; i++ )
	{
		// Don't clear global fields
		if( !m_global || !( pFields[i].flags & FTYPEDESC_GLOBAL ) )
			memset( ( (char *)pBaseData + pFields[i].fieldOffset ), 0, pFields[i].fieldSize * gSizes[pFields[i].fieldType] );
	}

	for( i = 0; i < fileCount; i++ )
	{
		BufferReadHeader( &header );
		lastField = ReadField( pBaseData, pFields, fieldCount, lastField, header.size, m_pdata->pTokens[header.token], header.pData );
		lastField++;
	}
	
	return 1;
}

void CRestore::BufferReadHeader( HEADER *pheader )
{
	ASSERT( pheader!=NULL );
	pheader->size = ReadShort();				// Read field size
	pheader->token = ReadShort();				// Read field name token
	pheader->pData = BufferPointer();			// Field Data is next
	BufferSkipBytes( pheader->size );			// Advance to next field
}


short CRestore::ReadShort( void )
{
	short tmp = 0;

	BufferReadBytes( (char *)&tmp, sizeof(short) );

	return tmp;
}

int CRestore::ReadInt( void )
{
	int tmp = 0;

	BufferReadBytes( (char *)&tmp, sizeof(int) );

	return tmp;
}

int CRestore::ReadNamedInt( const char *pName )
{
	HEADER header;

	BufferReadHeader( &header );
	return ( (int *)header.pData )[0];
}

char *CRestore::ReadNamedString( const char *pName )
{
	HEADER header;

	BufferReadHeader( &header );
#if TOKENIZE
	return (char *)( m_pdata->pTokens[*(short *)header.pData] );
#else
	return (char *)header.pData;
#endif
}

char *CRestore::BufferPointer( void )
{
	if( !m_pdata )
		return NULL;

	return m_pdata->pCurrentData;
}

void CRestore::BufferReadBytes( char *pOutput, int size )
{
	ASSERT( m_pdata !=NULL );

	if( !m_pdata || Empty() )
		return;

	if( ( m_pdata->size + size ) > m_pdata->bufferSize )
	{
		ALERT( at_error, "Restore overflow!\n" );
		m_pdata->size = m_pdata->bufferSize;
		return;
	}

	if( pOutput )
		memcpy( pOutput, m_pdata->pCurrentData, size );
	m_pdata->pCurrentData += size;
	m_pdata->size += size;
}

void CRestore::BufferSkipBytes( int bytes )
{
	BufferReadBytes( NULL, bytes );
}

int CRestore::BufferSkipZString( void )
{
	char *pszSearch;
	int len;

	if( !m_pdata )
		return 0;

	int maxLen = m_pdata->bufferSize - m_pdata->size;

	len = 0;
	pszSearch = m_pdata->pCurrentData;
	while( *pszSearch++ && len < maxLen )
		len++;

	len++;

	BufferSkipBytes( len );

	return len;
}

int CRestore::BufferCheckZString( const char *string )
{
	if( !m_pdata )
		return 0;

	int maxLen = m_pdata->bufferSize - m_pdata->size;
	int len = strlen( string );
	if( len <= maxLen )
	{
		if( !strncmp( string, m_pdata->pCurrentData, len ) )
			return 1;
	}
	return 0;
}

void UTIL_CleanSpawnPoint( Vector origin, float dist )
{
	CBaseEntity *ent = NULL;
	while( ( ent = UTIL_FindEntityInSphere( ent, origin, dist ) ) != NULL )
	{
		if( ent->IsPlayer() )
		{
			TraceResult tr;
			UTIL_TraceHull( ent->pev->origin + Vector( 0, 0, 36 ), ent->pev->origin + Vector( RANDOM_FLOAT( -150, 150 ), RANDOM_FLOAT( -150, 150 ), 0 ), dont_ignore_monsters, human_hull, ent->edict(), &tr );
			if( !tr.fAllSolid )
				UTIL_SetOrigin(ent->pev, tr.vecEndPos );
		}
	}
}

char *memfgets( byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize )
{
	// Bullet-proofing
	if( !pMemFile || !pBuffer )
		return NULL;

	if( filePos >= fileSize )
		return NULL;

	int i = filePos;
	int last = fileSize;

	// fgets always NULL terminates, so only read bufferSize-1 characters
	if( last - filePos > ( bufferSize - 1 ) )
		last = filePos + ( bufferSize - 1 );

	int stop = 0;

	// Stop at the next newline (inclusive) or end of buffer
	while( i < last && !stop )
	{
		if( pMemFile[i] == '\n' )
		{
			stop = 1;
		}
		if ( pMemFile[i] == '\r' )
		{
			if (i+1 < last && pMemFile[i+1] == '\n')
			{
				pMemFile[i] = '\n';
				pMemFile[i+1] = '\0';
				++i;
			}
			stop = 1;
		}
		i++;
	}

	// If we actually advanced the pointer, copy it over
	if( i != filePos )
	{
		// We read in size bytes
		int size = i - filePos;
		// copy it out
		memcpy( pBuffer, pMemFile + filePos, sizeof(byte) * size );

		// If the buffer isn't full, terminate (this is always true)
		if( size < bufferSize )
			pBuffer[size] = 0;

		// Update file pointer
		filePos = i;
		return pBuffer;
	}

	// No data read, bail
	return NULL;
}

float RandomizeNumberFromRange(const FloatRange& r)
{
	return RandomizeNumberFromRange(r.min, r.max);
}

float RandomizeNumberFromRange(float minF, float maxF)
{
	if (minF >= maxF) {
		return minF;
	}
	return RANDOM_FLOAT(minF, maxF);
}

int RandomizeNumberFromRange(const IntRange& r)
{
	return RandomizeNumberFromRange(r.min, r.max);
}

int RandomizeNumberFromRange(int minI, int maxI)
{
	if (minI >= maxI) {
		return minI;
	}
	return RANDOM_LONG(minI, maxI);
}

void ReportAIStateByClassname(const char* name)
{
	if (!name || !*name) {
		ALERT(at_console, "Must provide a classname!\n");
		return;
	}
	CBaseEntity* pEntity = 0;
	ALERT(at_console, "Listing all monsters of \"%s\" classname\n", name);
	while ( ( pEntity = UTIL_FindEntityByClassname(pEntity, name) ) != 0 ) {
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if (pMonster) {
			pMonster->ReportAIState(at_console);
			const bool clientInPVS = !FNullEnt(FIND_CLIENT_IN_PVS( pMonster->edict() ));
			ALERT(at_console, "Position in the world: (%3.1f, %3.1f, %3.1f). ", pMonster->pev->origin.x, pMonster->pev->origin.y, pMonster->pev->origin.z);
			ALERT(at_console, "Client in PVS: %s\n\n", clientInPVS ? "yes" : "no");
		}
	}
}

// LRC- change the origin to the given position, and bring any movewiths along too.
void UTIL_AssignOrigin( CBaseEntity *pEntity, const Vector vecOrigin )
{
	UTIL_AssignOrigin( pEntity, vecOrigin, true);
}

// LRC- bInitiator is true if this is being called directly, rather than because pEntity is moving with something else.
void UTIL_AssignOrigin( CBaseEntity *pEntity, const Vector vecOrigin, bool bInitiator)
{
//	ALERT(at_console, "AssignOrigin before %f, after %f\n", pEntity->pev->origin.x, vecOrigin.x);
#if 0
	Vector vecDiff = vecOrigin - pEntity->pev->origin;
	if (vecDiff.Length() > 0.01 && CVAR_GET_FLOAT("sohl_mwdebug"))
		ALERT(at_console,"AssignOrigin %s %s: (%f %f %f) goes to (%f %f %f)\n",STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->origin.z, vecOrigin.x, vecOrigin.y, vecOrigin.z);
#endif

//	UTIL_SetDesiredPos(pEntity, vecOrigin);
//	pEntity->pev->origin = vecOrigin;
	UTIL_SetOrigin(pEntity->pev, vecOrigin);

//	if (pEntity->m_vecDesiredVel != g_vecZero)
//	{
//		pEntity->pev->velocity = pEntity->m_vecDesiredVel;
//	}
#if 0
	if (bInitiator && pEntity->m_pMoveWith)
	{
//		UTIL_DesiredMWOffset( pEntity );
//		if (pEntity->m_vecMoveWithOffset != (pEntity->pev->origin - pEntity->m_pMoveWith->pev->origin))
//			ALERT(at_console, "Changing MWOffset for %s \"%s\"\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname));
		pEntity->m_vecMoveWithOffset = pEntity->pev->origin - pEntity->m_pMoveWith->pev->origin;
//		ALERT(at_console,"set m_vecMoveWithOffset = %f %f %f\n",pEntity->m_vecMoveWithOffset.x,pEntity->m_vecMoveWithOffset.y,pEntity->m_vecMoveWithOffset.z);
	}
	if (pEntity->m_pChildMoveWith) // now I've moved pEntity, does anything else have to move with it?
	{
		CBaseEntity* pChild = pEntity->m_pChildMoveWith;
//		if (vecDiff != g_vecZero)
//		{
			Vector vecTemp;
			while (pChild)
			{
				//ALERT(at_console,"  pre: parent origin is (%f %f %f), child origin is (%f %f %f)\n",
				//	pEntity->pev->origin.x,pEntity->pev->origin.y,pEntity->pev->origin.z,
				//	pChild->pev->origin.x,pChild->pev->origin.y,pChild->pev->origin.z
				//);
				if (pChild->pev->movetype != MOVETYPE_PUSH || pChild->pev->velocity == pEntity->pev->velocity) // if the child isn't moving under its own power
				{
					UTIL_AssignOrigin( pChild, vecOrigin + pChild->m_vecMoveWithOffset, false );
//					ALERT(at_console,"used m_vecMoveWithOffset based on %f %f %f to set %f %f %f\n",pEntity->pev->origin.x,pEntity->pev->origin.y,pEntity->pev->origin.z,pChild->pev->origin.x,pChild->pev->origin.y,pChild->pev->origin.z);
				}
				else
				{
					vecTemp = vecDiff + pChild->pev->origin;
					UTIL_AssignOrigin( pChild, vecTemp, false );
				}
				//ALERT(at_console,"  child origin becomes (%f %f %f)\n",pChild->pev->origin.x,pChild->pev->origin.y,pChild->pev->origin.z);
				//ALERT(at_console,"ent %p has sibling %p\n",pChild,pChild->m_pSiblingMoveWith);
				pChild = pChild->m_pSiblingMoveWith;
			}
//		}
	}
#endif
}

void UTIL_SetAngles( CBaseEntity *pEntity, const Vector vecAngles )
{
	UTIL_SetAngles( pEntity, vecAngles, true );
}

void UTIL_SetAngles( CBaseEntity *pEntity, const Vector vecAngles, bool bInitiator)
{
	Vector vecDiff = vecAngles - pEntity->pev->angles;
#if 0
	if (vecDiff.Length() > 0.01 && CVAR_GET_FLOAT("sohl_mwdebug"))
		ALERT(at_console,"SetAngles %s %s: (%f %f %f) goes to (%f %f %f)\n",STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), pEntity->pev->angles.x, pEntity->pev->angles.y, pEntity->pev->angles.z, vecAngles.x, vecAngles.y, vecAngles.z);
#endif

//	UTIL_SetDesiredAngles(pEntity, vecAngles);
	pEntity->pev->angles = vecAngles;
#if 0
	if (bInitiator && pEntity->m_pMoveWith)
	{
		pEntity->m_vecRotWithOffset = vecAngles - pEntity->m_pMoveWith->pev->angles;
	}
	if (pEntity->m_pChildMoveWith) // now I've moved pEntity, does anything else have to move with it?
	{
		CBaseEntity* pChild = pEntity->m_pChildMoveWith;
		Vector vecTemp;
		while (pChild)
		{
			if (pChild->pev->avelocity == pEntity->pev->avelocity) // if the child isn't turning under its own power
			{
				UTIL_SetAngles( pChild, vecAngles + pChild->m_vecRotWithOffset, false );
			}
			else
			{
				vecTemp = vecDiff + pChild->pev->angles;
				UTIL_SetAngles( pChild, vecTemp, false );
			}
			//ALERT(at_console,"  child origin becomes (%f %f %f)\n",pChild->pev->origin.x,pChild->pev->origin.y,pChild->pev->origin.z);
			//ALERT(at_console,"ent %p has sibling %p\n",pChild,pChild->m_pSiblingMoveWith);
			pChild = pChild->m_pSiblingMoveWith;
		}
	}
#endif
}

//LRC- an arbitrary limit. If this number is exceeded we assume there's an infinite loop, and abort.
#define MAX_MOVEWITH_DEPTH 100

//LRC
void UTIL_SetVelocity ( CBaseEntity *pEnt, const Vector vecSet )
{
	Vector vecNew;
#if 0
	if (pEnt->m_pMoveWith)
		vecNew = vecSet + pEnt->m_pMoveWith->pev->velocity;
	else
#endif
		vecNew = vecSet;

//	ALERT(at_console,"SetV: %s is sent (%f,%f,%f) - goes from (%f,%f,%f) to (%f,%f,%f)\n",
//	    STRING(pEnt->pev->targetname), vecSet.x, vecSet.y, vecSet.z,
//		pEnt->pev->velocity.x, pEnt->pev->velocity.y, pEnt->pev->velocity.z,
//		vecNew.x, vecNew.y, vecNew.z
//	);
#if 0
	if ( pEnt->m_pChildMoveWith )
	{
		CBaseEntity *pMoving = pEnt->m_pChildMoveWith;
		int sloopbreaker = MAX_MOVEWITH_DEPTH; // LRC - to save us from infinite loops
		while (pMoving)
		{
			UTIL_SetMoveWithVelocity(pMoving, vecNew, MAX_MOVEWITH_DEPTH );
			pMoving = pMoving->m_pSiblingMoveWith;
			sloopbreaker--;
			if (sloopbreaker <= 0)
			{
				ALERT(at_error, "SetVelocity: Infinite sibling list for MoveWith!\n");
				break;
			}
		}
	}
#endif
	pEnt->pev->velocity = vecNew;
}

void UTIL_SetAvelocity ( CBaseEntity *pEnt, const Vector vecSet )
{
	Vector vecNew;
#if 0
	if (pEnt->m_pMoveWith)
		vecNew = vecSet + pEnt->m_pMoveWith->pev->avelocity;
	else
#endif
		vecNew = vecSet;

//	ALERT(at_console, "Setting AVelocity %f %f %f\n", vecNew.x, vecNew.y, vecNew.z);
#if 0
	if ( pEnt->m_pChildMoveWith )
	{
		CBaseEntity *pMoving = pEnt->m_pChildMoveWith;
		int sloopbreaker = MAX_MOVEWITH_DEPTH; // LRC - to save us from infinite loops
		while (pMoving)
		{
			UTIL_SetMoveWithAvelocity(pMoving, vecNew, MAX_MOVEWITH_DEPTH );
			pMoving = pMoving->m_pSiblingMoveWith;
			sloopbreaker--;
			if (sloopbreaker <= 0)
			{
				ALERT(at_error, "SetAvelocity: Infinite sibling list for MoveWith!\n");
				break;
			}
		}
	}
#endif
	//UTIL_SetDesiredAvelocity(pEnt, vecNew);
	pEnt->pev->avelocity = vecNew;
}
