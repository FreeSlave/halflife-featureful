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
/*

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "rpgrocket.h"
#include "soundent.h"
#include "decals.h"
#include "game.h"
#include "gamerules.h"
#include "ammoregistry.h"
#include "ammo_amounts.h"
#include "common_soundscripts.h"
#include "player_templates.h"
#include "weapon_templates.h"

extern bool gEvilImpulse101;

DLL_GLOBAL	short g_sModelIndexLaser;// holds the index for the laser beam
DLL_GLOBAL	const char *g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL	short g_sModelIndexLaserDot;// holds the index for the laser beam dot
DLL_GLOBAL	short g_sModelIndexFireball;// holds the index for the fireball
DLL_GLOBAL	short g_sModelIndexSmoke;// holds the index for the smoke cloud
DLL_GLOBAL	const char* g_pModelNameSmoke = "sprites/steam1.spr";
DLL_GLOBAL	short g_sModelIndexWExplosion;// holds the index for the underwater explosion
DLL_GLOBAL	short g_sModelIndexBubbles;// holds the index for the bubbles model
DLL_GLOBAL	short g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
DLL_GLOBAL	short g_sModelIndexBloodSpray;// holds the sprite index for splattered blood

ItemInfo CBasePlayerWeapon::ItemInfoArray[MAX_WEAPONS];

extern int gmsgCurWeapon;
extern int gmsgMaxClip;

MULTIDAMAGE gMultiDamage;

#define TRACER_FREQ		4			// Tracers fire every fourth bullet

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage()
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.damageInfo = DamageInfo{};
}

//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage
void ApplyMultiDamage( entvars_t *pevInflictor, entvars_t *pevAttacker )
{
	if( !gMultiDamage.pEntity )
		return;

	gMultiDamage.pEntity->TakeDamage( pevInflictor, pevAttacker, gMultiDamage.damageInfo );
}

// GLOBALS USED:
//		gMultiDamage
void AddMultiDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, CBaseEntity *pEntity, const DamageInfo& damageInfo )
{
	if( !pEntity )
		return;

	const int prevDamageType = gMultiDamage.damageInfo.type;
	const float prevDamage = gMultiDamage.damageInfo.damage;

	gMultiDamage.damageInfo = damageInfo;
	gMultiDamage.damageInfo.damage = prevDamage;
	gMultiDamage.damageInfo.type |= prevDamageType;

	if( pEntity != gMultiDamage.pEntity )
	{
		ApplyMultiDamage( pevInflictor, pevAttacker );
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.damageInfo.damage = 0;
	}

	gMultiDamage.damageInfo.damage += damageInfo.damage;
}

/*
================
SpawnBlood
================
*/
void SpawnBlood( Vector vecSpot, int bloodColor, float flDamage )
{
	UTIL_BloodDrips( vecSpot, g_vecAttackDir, bloodColor, (int)flDamage );
}

int DamageDecal( CBaseEntity *pEntity, int bitsDamageType )
{
	if( !pEntity )
		return ( DECAL_GUNSHOT1 + RANDOM_LONG( 0, 4 ) );
	
	return pEntity->DamageDecal( bitsDamageType );
}

void DecalGunshot( TraceResult *pTrace )
{
	// Is the entity valid
	if( !UTIL_IsValidEntity( pTrace->pHit ) )
		return;

	if( VARS( pTrace->pHit )->solid == SOLID_BSP || VARS( pTrace->pHit )->movetype == MOVETYPE_PUSHSTEP )
	{
		CBaseEntity *pEntity = NULL;
		// Decal the wall with a gunshot
		if( !FNullEnt( pTrace->pHit ) )
			pEntity = CBaseEntity::Instance( pTrace->pHit );

		UTIL_GunshotDecalTrace( pTrace, DamageDecal( pEntity, DMG_BULLET ) );
	}
}

void DecalSmack( TraceResult *pTrace )
{
	if (!UTIL_IsValidEntity( pTrace->pHit ))
		return;

	if (VARS( pTrace->pHit )->solid == SOLID_BSP || VARS( pTrace->pHit )->movetype == MOVETYPE_PUSHSTEP)
	{
		CBaseEntity *pEntity = nullptr;
		if( !FNullEnt( pTrace->pHit ) )
			pEntity = CBaseEntity::Instance( pTrace->pHit );

		UTIL_DecalTrace(pTrace, DamageDecal(pEntity, DMG_CLUB));
	}
}

//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype )
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_MODEL );
		WRITE_VECTOR( vecOrigin );
		WRITE_VECTOR( vecVelocity );
		WRITE_ANGLE( rotation );
		WRITE_SHORT( model );
		WRITE_BYTE( soundtype );
		WRITE_BYTE( 25 );// 2.5 seconds
	MESSAGE_END();
}

#if 0
// UNDONE: This is no longer used?
void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_EXPLODEMODEL );
		WRITE_VECTOR( vecOrigin );
		WRITE_COORD( speed );
		WRITE_SHORT( model );
		WRITE_SHORT( count );
		WRITE_BYTE( 15 );// 1.5 seconds
	MESSAGE_END();
}
#endif

bool bIsMultiplayer()
{
	return gpGlobals->maxClients > 1;
	//return g_pGameRules->IsMultiplayer();
}

void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, CBasePlayer *pPlayer )
{
	pPlayer->m_forceCollideWithCorpses = true;
	int		i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult	tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2.0f );
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pPlayer->edict(), &tmpTrace );
	if( tmpTrace.flFraction < 1.0f )
	{
		tr = tmpTrace;
		pPlayer->m_forceCollideWithCorpses = false;
		return;
	}

	for( i = 0; i < 2; i++ )
	{
		for( j = 0; j < 2; j++ )
		{
			for( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pPlayer->edict(), &tmpTrace );
				if( tmpTrace.flFraction < 1.0f )
				{
					float thisDistance = ( tmpTrace.vecEndPos - vecSrc ).Length();
					if( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
	pPlayer->m_forceCollideWithCorpses = false;
}

// Precaches the weapon and queues the weapon info for sending to clients
bool UTIL_PrecacheOtherWeapon( const char *szClassname )
{
	edict_t	*pent;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return false;
	}
	
	CBaseEntity *pEntity = CBaseEntity::Instance( VARS( pent ) );

	bool result = true;
	if( pEntity )
	{
		ItemInfo II{};
		CBasePlayerWeapon* pWeapon = pEntity->MyWeaponPointer();
		if( pWeapon != 0 )
		{
			if (pWeapon->IsEnabledInMod())
			{
				pEntity->Precache();

				if (pWeapon->GetItemInfo( &II ))
				{
					const WeaponParameters& params = pWeapon->MyParameters();

					II.pszName = szClassname;
					II.iId = pWeapon->WeaponId();
					II.pszAmmo1 = params.ammoName.empty() ? nullptr : params.ammoName.c_str();
					II.pszAmmo2 = params.secondaryAmmoName.empty() ? nullptr : params.secondaryAmmoName.c_str();

					if ((params.fire.useSecondaryAmmo.Get(false) || params.fire.useSecondaryAmmo.Get(true)) && params.secondaryAmmoName.empty())
						II.iFlags |= ITEM_FLAG_SELECTONEMPTY;

					CBasePlayerWeapon::ItemInfoArray[II.iId] = II;
				}
			}
			else
				result = false;
		}
		else
		{
			ALERT(at_console, "UTIL_PrecacheOtherWeapon: %s is not a weapon\n", szClassname);
			result = false;
		}
	}

	REMOVE_ENTITY( pent );
	return result;
}

void RegisterAmmoTypes()
{
	g_AmmoRegistry.Register("buckshot", BUCKSHOT_MAX_CARRY);
	g_AmmoRegistry.Register("9mm", _9MM_MAX_CARRY);
	g_AmmoRegistry.Register("ARgrenades", M203_GRENADE_MAX_CARRY);
	g_AmmoRegistry.Register("357", _357_MAX_CARRY);
	g_AmmoRegistry.Register("uranium", URANIUM_MAX_CARRY);
	g_AmmoRegistry.Register("rockets", ROCKET_MAX_CARRY);
	g_AmmoRegistry.Register("bolts", BOLT_MAX_CARRY);
	g_AmmoRegistry.Register("Trip Mine", TRIPMINE_MAX_CARRY, true);
	g_AmmoRegistry.Register("Satchel Charge", SATCHEL_MAX_CARRY, true);
	g_AmmoRegistry.Register("Hand Grenade", HANDGRENADE_MAX_CARRY, true);
	g_AmmoRegistry.Register("Snarks", SNARK_MAX_CARRY, true);
	g_AmmoRegistry.Register("Hornets", HORNET_MAX_CARRY);
	g_AmmoRegistry.Register("Medicine", MEDKIT_MAX_CARRY);
	g_AmmoRegistry.Register("Penguins", PENGUIN_MAX_CARRY, true);
	g_AmmoRegistry.Register("556", _556_MAX_CARRY);
	g_AmmoRegistry.Register("762", _762_MAX_CARRY);
	g_AmmoRegistry.Register("Shocks", SHOCK_MAX_CARRY);
	g_AmmoRegistry.Register("spores", SPORE_MAX_CARRY);
	g_AmmoRegistry.Register("45acp", 200);
	g_AmmoRegistry.Register("57mm", 200);
	g_AmmoRegistry.Register("nails", 200);
	g_AmmoRegistry.Register("grenades", 50);
	g_AmmoRegistry.Register("fuel", 100);
	g_AmmoRegistry.Register("cells", 100);
	g_AmmoRegistry.Register("charges", 10);
	g_AmmoRegistry.Register("rounds", 200);
	g_AmmoRegistry.Register("slugs", 100);

	for (unsigned int i = 0; i<g_modFeatures.maxAmmoCount; ++i)
	{
		g_AmmoRegistry.SetMaxAmmo(g_modFeatures.maxAmmos[i].name, g_modFeatures.maxAmmos[i].maxAmmo);
	}
}

struct AmmoEnabled
{
	AmmoEnabled(const char* name, const char* entity): ammoName(name), ammoEntity(entity) {}

	bool enabled{false};
	const char* ammoName;
	const char* ammoEntity;
};


// called by worldspawn
void W_Precache( CBaseEntity* pWorld )
{
	memset( CBasePlayerWeapon::ItemInfoArray, 0, sizeof(CBasePlayerWeapon::ItemInfoArray) );

	// custom items...

	// common world objects
	UTIL_PrecacheOther( "item_suit" );
	UTIL_PrecacheOther( "item_healthkit" );
	UTIL_PrecacheOther( "item_battery" );
	UTIL_PrecacheOther( "item_antidote" );
	UTIL_PrecacheOther( "item_security" );
	UTIL_PrecacheOther( "item_longjump" );

	UTIL_PrecacheOther( "item_flashlight" );
	UTIL_PrecacheOther( "item_nvgs" );

	UTIL_PrecacheOther( "ammo_buckshot" );
	UTIL_PrecacheOther( "ammo_9mmclip" );
	UTIL_PrecacheOther( "ammo_9mmAR" );
	UTIL_PrecacheOther( "ammo_ARgrenades" );
	UTIL_PrecacheOther( "ammo_9mmbox" );
	UTIL_PrecacheOther( "ammo_357" );
	UTIL_PrecacheOther( "ammo_gaussclip" );
	UTIL_PrecacheOther( "ammo_rpgclip" );
	UTIL_PrecacheOther( "ammo_crossbow" );

	if( g_pGameRules->IsDeathmatch() )
	{
		UTIL_PrecacheOther( "weaponbox" );// container for dropped deathmatch weapons
	}

	AmmoEnabled ammoEnabledList[] = {
		AmmoEnabled("556", "ammo_556"),
		AmmoEnabled("762", "ammo_762"),
		AmmoEnabled("45acp", "ammo_45acp"),
		AmmoEnabled("57mm", "ammo_57mm"),
		AmmoEnabled("nails", "ammo_nails"),
		AmmoEnabled("grenades", "ammo_grenadeclip"),
		AmmoEnabled("fuel", "ammo_fuel"),
		AmmoEnabled("cells", "ammo_cells"),
		AmmoEnabled("charges", "ammo_charges"),
		AmmoEnabled("rounds", "ammo_rounds"),
		AmmoEnabled("slugs", "ammo_slugs"),
	};

	ALERT(at_console, "Precaching weapons\n");

	int toolIndex = 0;

	for (int i=0; i<MAX_WEAPONS; ++i)
	{
		WeaponInfo& info = AccessWeaponInfo(i);
		if (info.classname && info.pWeapon->IsEnabledInMod())
		{
			UTIL_PrecacheOtherWeapon(info.classname);

			WeaponParameters& params = info.params;

			if (!params.toolIcon.empty())
			{
				params.toolIndex = toolIndex;
				toolIndex++;
			}

			for (auto& ammo : ammoEnabledList)
			{
				if (!ammo.enabled)
				{
					if (params.ammoName == ammo.ammoName)
					{
						ammo.enabled = true;
					}
					if (params.secondaryAmmoName == ammo.ammoName)
					{
						ammo.enabled = true;
					}
				}
			}
		}
	}

	g_modFeatures.ammo556IsUsed = ammoEnabledList[0].enabled;
	g_modFeatures.ammo762IsUsed = ammoEnabledList[1].enabled;

	for (auto& ammo : ammoEnabledList)
	{
		if (ammo.enabled)
		{
			UTIL_PrecacheOther(ammo.ammoEntity);
		}
	}

	g_sModelIndexFireball = PRECACHE_MODEL( "sprites/zerogxplode.spr" );// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL( "sprites/WXplo1.spr" );// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL( g_pModelNameSmoke );// smoke
	g_sModelIndexBubbles = PRECACHE_MODEL( "sprites/bubble.spr" );//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL( "sprites/bloodspray.spr" ); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL( "sprites/blood.spr" ); // splattered blood 

	g_sModelIndexLaser = PRECACHE_MODEL( g_pModelNameLaser );
	g_sModelIndexLaserDot = PRECACHE_MODEL( "sprites/laserdot.spr" );

	// used by explosions
	PRECACHE_MODEL( "models/grenade.mdl" );
	PRECACHE_MODEL( "sprites/explode1.spr" );

	PRECACHE_SOUND( "weapons/bullet_hit1.wav" );	// hit by bullet
	PRECACHE_SOUND( "weapons/bullet_hit2.wav" );	// hit by bullet

	pWorld->RegisterAndPrecacheSoundScript(Items::weaponDropSoundScript);// weapon falls to the ground
	pWorld->RegisterAndPrecacheSoundScript(Items::weaponEmptySoundScript);

	UTIL_PrecacheOther("grenade");

	for (auto it = g_PlayerTemplateSystem.PlayerTemplatesBegin(); it != g_PlayerTemplateSystem.PlayerTemplatesEnd(); ++it)
	{
		const PlayerTemplate& playerTemplate = it->second;

		if (!playerTemplate.HasAnyWeaponReplacaments())
			continue;

		for (int i=0; i<MAX_WEAPONS; ++i)
		{
			WeaponInfo& info = AccessWeaponInfo(i);
			if (info.classname && info.pWeapon->IsEnabledInMod())
			{
				auto wr = playerTemplate.GetWeaponReplacement(info.classname);
				if (wr)
				{
					if (!wr->viewModel.empty())
					{
						PRECACHE_MODEL(wr->viewModel.c_str());
					}
					if (!wr->viewModelDetonator.empty())
					{
						PRECACHE_MODEL(wr->viewModelDetonator.c_str());
					}
				}
			}
		}
	}
}

TYPEDESCRIPTION	CBasePlayerWeapon::m_SaveData[] =
{
	DEFINE_FIELD( CBasePlayerWeapon, m_pPlayer, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
	// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
#if CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_FLOAT ),
#else	// CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME ),
#endif	// CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_sMaster, FIELD_STRING ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iMaxClip, FIELD_INTEGER ),
	//DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
	//DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
	DEFINE_FIELD( CBasePlayerWeapon, m_packedTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayerWeapon, m_inAltMode, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CBasePlayerWeapon, CBaseAnimating )

void CBasePlayerWeapon::SetObjectCollisionBox()
{
	SetMyObjectCollisionBox(Vector( -24, -24, 0 ), Vector( 24, 24, 16 ));
}

void CBasePlayerWeapon::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "initammo" ) )
	{
		m_iDefaultAmmo = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

//=========================================================
// Sets up movetype, size, solidtype for a new weapon. 
//=========================================================
void CBasePlayerWeapon::FallInit()
{
	if (pev->movetype < 0)
		pev->movetype = MOVETYPE_NONE;
	else if (pev->movetype == 0)
		pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );//pointsize until it lands on the ground.

	SetTouch( &CBasePlayerWeapon::DefaultTouch );
	SetThink( &CBasePlayerWeapon::FallThink );

	pev->nextthink = gpGlobals->time + 0.1f;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerWeapon::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( (pev->flags & FL_ONGROUND) || pev->movetype != MOVETYPE_TOSS )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if( !FNullEnt( pev->owner ) )
		{
			EmitSoundScript(Items::weaponDropSoundScript);
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize(); 
	}
	else if( m_pPlayer )
	{
		SetThink( NULL );
	}

	if( g_pGameRules->IsBustingGame())
	{
		if( !FNullEnt( pev->owner ))
			return;

		if( FClassnameIs( pev, "weapon_egon" ))
			UTIL_Remove( this );
	}
}

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
void CBasePlayerWeapon::Materialize()
{
	if( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EmitSoundScript(Items::materializeSoundScript);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	//const int itemSize = 24;
	//UTIL_SetSize( pev, Vector( -itemSize, -itemSize, 0 ), Vector( itemSize, itemSize, itemSize ) );
	UTIL_SetOrigin( pev, pev->origin );// link into world.
	SetTouch( &CBasePlayerWeapon::DefaultTouch );
	SetThink( NULL );
}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerWeapon::AttemptToMaterialize()
{
	float time = g_pGameRules->FlWeaponTryRespawn( this );

	if( time == 0 )
	{
		SetThink( &CBasePlayerWeapon::FallThink );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	pev->nextthink = gpGlobals->time + time;
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should 
// it respawn?
//=========================================================
void CBasePlayerWeapon::CheckRespawn()
{
	switch( g_pGameRules->WeaponShouldRespawn( this ) )
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		return;
		break;
	}
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerWeapon::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create( STRING( pev->classname ), g_pGameRules->VecWeaponRespawnSpot( this ), pev->angles, pev->owner );

	if( pNewWeapon )
	{
		pNewWeapon->pev->effects |= EF_NODRAW;// invisible for now
		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( &CBasePlayerWeapon::AttemptToMaterialize );

		//DROP_TO_FLOOR( ENT( pev ) );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime( this );
	}
	else
	{
		ALERT( at_console, "Respawn failed to create %s!\n", STRING( pev->classname ) );
	}

	return pNewWeapon;
}

bool CBasePlayerWeapon::IsLockedByMaster()
{
	return m_sMaster && !UTIL_IsMasterTriggered(m_sMaster, nullptr);
}

bool CBasePlayerWeapon::IsUsefulToDisplayHint(CBaseEntity* pPlayer)
{
	if (pPlayer->IsPlayer())
	{
		CBasePlayer* p = (CBasePlayer*)pPlayer;
		return p->CanHaveItem(this);
	}
	return false;
}

static bool IsPickableByTouch(CBaseEntity* pEntity)
{
	return !FBitSet(pEntity->pev->spawnflags, SF_ITEM_USE_ONLY) &&
			(FBitSet(pEntity->pev->spawnflags, SF_ITEM_TOUCH_ONLY) || ItemsPickableByTouch());
}

static bool IsPickableByUse(CBaseEntity* pEntity)
{
	return !FBitSet(pEntity->pev->spawnflags, SF_ITEM_TOUCH_ONLY) &&
			(FBitSet(pEntity->pev->spawnflags, SF_ITEM_USE_ONLY) || ItemsPickableByUse());
}

void CBasePlayerWeapon::DefaultTouch( CBaseEntity *pOther )
{
	if (IsPickableByTouch(this)) {
		TouchOrUse(pOther);
	}
}

int CBasePlayerWeapon::ObjectCaps()
{
	int caps = CBaseAnimating::ObjectCaps();
	if (FBitSet(pev->spawnflags, SF_ITEM_DONT_TRANSIT_ACROSS_LEVELS))
	{
		ClearBits(caps, FCAP_ACROSS_TRANSITION);
	}
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW)) {
		return caps | FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE;
	} else {
		return caps;
	}
}

void CBasePlayerWeapon::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW) ) {
		TouchOrUse(pCaller);
	}
}

void CBasePlayerWeapon::TouchOrUse(CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// can I have this?
	if (!pPlayer->CanHaveItem(this) || !g_pGameRules->CanHavePlayerItem( pPlayer, this ))
	{
		if( gEvilImpulse101 )
		{
			UTIL_Remove( this );
		}
		return;
	}

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if( pOther->AddPlayerItem( this ) == GOT_NEW_ITEM )
	{
		pPlayer->EmitSoundScript(GetSoundScript(Items::weaponPickupSoundScript));
	}

	SUB_UseTargets( pOther );
}

void CBasePlayerWeapon::DestroyItem()
{
	if( m_pPlayer )
	{
		// if attached to a player, remove.
		m_pPlayer->ClearWeaponBit(WeaponId());
		m_pPlayer->RemovePlayerItem( this, false );
		//m_pPlayer = NULL;
	}

	Kill();
}

void CBasePlayerWeapon::Drop()
{
	SetTouch( NULL );
	SetUse( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CBasePlayerWeapon::Kill()
{
	SetTouch( NULL );
	SetUse( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CBasePlayerWeapon::AttachToPlayer( CBasePlayer *pPlayer )
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0;// server won't send down to clients if modelindex == 0
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();

	pev->nextthink = 0;// Remove think - prevents futher attempts to materialize

	SetTouch( NULL );
	SetThink( NULL );
}

const AmmoType* CBasePlayerWeapon::GetAmmoType(const char *name)
{
	return g_AmmoRegistry.GetByName(name);
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
int CBasePlayerWeapon::AddDuplicate( CBasePlayerWeapon *pOriginal )
{
	if( m_iDefaultAmmo )
	{
		return ExtractAmmo( pOriginal );
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo( pOriginal );
	}
}

bool CBasePlayerWeapon::IsEnabledInMod()
{
	return g_modFeatures.IsWeaponEnabled(WeaponId());
}

bool CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	pPlayer->SetWeaponBit(WeaponId());

	m_iPrimaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo1() );
	m_iSecondaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo2() );

	// Remove weapon's global name to avoid problems with carrying the weapon to other maps
	pev->globalname = iStringNull;
	m_iClientMaxClip = 0;

	return AddWeapon();
}

bool CBasePlayerWeapon::AddToPlayerDefault( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( WeaponId() );
		MESSAGE_END();
		return true;
	}
	return false;
}

int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer )
{
	bool bSend = false;
	int state = 0;
	if( pPlayer->m_pActiveItem == this )
	{
		if( pPlayer->m_fOnTarget )
			state = WEAPON_IS_ONTARGET;
		else
			state = 1;
	}

	// Forcing send of all data!
	if( !pPlayer->m_fWeapon )
	{
		bSend = true;
	}

	// This is the current or last weapon, so the state will need to be updated
	if( this == pPlayer->m_pActiveItem || this == pPlayer->m_pClientActiveItem )
	{
		if( pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem )
		{
			bSend = true;
		}
	}

	// If the ammo, state, or fov has changed, update the weapon
	if( m_iClip != m_iClientClip || state != m_iClientWeaponState || pPlayer->m_iFOV != pPlayer->m_iClientFOV )
	{
		bSend = true;
	}

	if( bSend )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev );
			WRITE_BYTE( state );
			WRITE_BYTE( WeaponId() );
			WRITE_SHORT( m_iClip );
			WRITE_SHORT( m_iMaxClip );
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = true;
	}

	if (m_iMaxClip != m_iClientMaxClip)
	{
		m_iClientMaxClip = m_iMaxClip;
		MESSAGE_BEGIN( MSG_ONE, gmsgMaxClip, NULL, pPlayer->pev );
			WRITE_BYTE( WeaponId() );
			WRITE_SHORT( m_iMaxClip );
		MESSAGE_END();
	}

	return 1;
}

void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body )
{
	const bool skiplocal = !m_ForceSendAnimations && UseDecrement();

	m_pPlayer->pev->weaponanim = iAnim;

#if CLIENT_WEAPONS
	if( skiplocal && ENGINE_CANSKIP( m_pPlayer->edict() ) )
		return;
#endif
	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev );
		WRITE_BYTE( iAnim );		// sequence number
		WRITE_BYTE( pev->body );	// weaponmodel bodygroup.
	MESSAGE_END();
}

bool CBasePlayerWeapon::AddPrimaryAmmo( int iCount )
{
	int iIdAmmo;
	const char* szName = pszAmmo1();

	if( !UsesClip() )
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName );
	}
	else if( m_iClip == 0 )
	{
		int i = Q_min( m_iClip + iCount, iMaxClip() ) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName );
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName );
	}

	if( iIdAmmo > 0 )
	{
		m_iPrimaryAmmoType = iIdAmmo;
		if( m_pPlayer->HasPlayerItem( this ) )
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EmitSoundScript(Items::ammoPickupSoundScript);
		}
	}

	return iIdAmmo > 0;
}

bool CBasePlayerWeapon::AddSecondaryAmmo(int iCount)
{
	int iIdAmmo = m_pPlayer->GiveAmmo( iCount, pszAmmo2() );

	if( iIdAmmo > 0 )
	{
		m_iSecondaryAmmoType = iIdAmmo;
		if (iCount > 0)
			EmitSoundScript(Items::ammoPickupSoundScript);
	}
	return iIdAmmo > 0;
}

//=========================================================
// IsUseable - this function determines whether or not a 
// weapon is useable by the player in its current state. 
// (does it have ammo loaded? do I have any ammo for the 
// weapon?, etc)
//=========================================================
bool CBasePlayerWeapon::IsUseable()
{
	// Player has unlimited ammo for this weapon or does not use magazines
	if (!UsesAmmo())
	{
		return true;
	}

	if (UsesClip() && m_iClip > 0)
	{
		return true;
	}

	if( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 )
	{
		return true;
	}

	if( UsesSecondaryAmmo() )
	{
		// Player has unlimited ammo for this weapon or does not use magazines
		if( iMaxAmmo2() == WEAPON_NOCLIP )
		{
			return true;
		}

		if( m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] > 0 )
		{
			return true;
		}
	}

	// clip is empty (or nonexistant) and the player has no more ammo of this type. 
	return false;
}

bool CBasePlayerWeapon::DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int body, float attackDelay, float idleDelay )
{
	if( !CanDeploy() )
		return false;

	m_pPlayer->pev->viewmodel = MAKE_STRING( szViewModel );
	if (g_modFeatures.weapon_p_models && szWeaponModel && *szWeaponModel)
		m_pPlayer->pev->weaponmodel = MAKE_STRING( szWeaponModel );
	else
		m_pPlayer->pev->weaponmodel = iStringNull;
	strcpy( m_pPlayer->m_szAnimExtention, szAnimExt );
	SendWeaponAnim( iAnim, body );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + attackDelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + idleDelay;
	m_flLastFireTime = 0.0f;

	m_pPlayer->m_bResumeZoom = false;

	return true;
}

const char* CBasePlayerWeapon::ViewModelToDeploy(const char *viewModel)
{
	if (m_pPlayer && m_pPlayer->m_playerTemplate)
	{
		auto wr = m_pPlayer->m_playerTemplate->GetWeaponReplacement(STRING(pev->classname));
		if (wr && !wr->viewModel.empty())
		{
			return wr->viewModel.c_str();
		}
	}
	return viewModel;
}

const char* CBasePlayerWeapon::DetonatorViewModelToDeploy(const char *viewModel)
{
	if (m_pPlayer && m_pPlayer->m_playerTemplate)
	{
		auto wr = m_pPlayer->m_playerTemplate->GetWeaponReplacement(STRING(pev->classname));
		if (wr && !wr->viewModelDetonator.empty())
		{
			return wr->viewModelDetonator.c_str();
		}
	}
	return viewModel;
}

const char* CBasePlayerWeapon::MyWorldModel()
{
	if (!FStringNull(pev->model))
		return STRING(pev->model);

	const Visual* ownVisual = MyOwnVisual();
	if (ownVisual && ownVisual->model)
		return ownVisual->model;

	const WeaponParameters& params = MyParameters();
	return params.worldModel.c_str();
}

void CBasePlayerWeapon::PrecacheWeaponModels()
{
	const WeaponParameters& params = MyParameters();

	PrecacheMyModel(params.worldModel.c_str());
	PRECACHE_MODEL(params.ViewModel());
	PrecachePModel(params.PlayerModel());

	const char* viewModelDetonator = params.DetonatorViewModel();
	if (viewModelDetonator)
		PRECACHE_MODEL(viewModelDetonator);
	PrecachePModel(params.DetonatorPlayerModel());
}

void CBasePlayerWeapon::PrecachePModel(const char *name)
{
	if (g_modFeatures.weapon_p_models && name)
		PRECACHE_MODEL(name);
}

bool CBasePlayerWeapon::PlayEmptySound(bool altMode)
{
	if (m_iPlayEmptySound)
	{
		const WeaponParameters& params = MyParameters();

		if (params.fire.useStandardEmptySound.Get(altMode))
		{
			m_pPlayer->EmitSoundScript(Items::weaponEmptySoundScript);
		}
		else
		{
			PlayWeaponSoundScript(params.fire.emptySound.Get(altMode));
		}
		m_iPlayEmptySound = false;
		return false;
	}
	return false;
}

void CBasePlayerWeapon::Holster()
{ 
	m_fInReload = false; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0; 
	m_pPlayer->pev->weaponmodel = 0;
	m_pPlayer->m_bResumeZoom = false;
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for 
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in 
// the weapon clip comes along. 
//=========================================================
bool CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon )
{
	bool iReturn = false;

	if( UsesAmmo() )
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want. 
		iReturn |= pWeapon->AddPrimaryAmmo( m_iDefaultAmmo );
		m_iDefaultAmmo = 0;
	}
	else if (UsesClip())
	{
		m_iClip = m_iDefaultAmmo;
		m_iDefaultAmmo = 0;
	}

	if( UsesSecondaryAmmo() )
	{
		iReturn |= pWeapon->AddSecondaryAmmo( 0 );
	}

	return iReturn;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
bool CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon )
{
	int iAmmo;

	if( m_iClip == WEAPON_NOCLIP )
	{
		iAmmo = 0;// guns with no clips always come empty if they are second-hand
	}
	else
	{
		iAmmo = m_iClip;
	}

	return pWeapon->m_pPlayer->GiveAmmo( iAmmo, pszAmmo1() ) > 0; // , &m_iPrimaryAmmoType
}
	
//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon()
{
	// first, no viewmodel at all.
	if (m_pPlayer->m_pActiveItem == this)
	{
		m_pPlayer->pev->viewmodel = iStringNull;
		m_pPlayer->pev->weaponmodel = iStringNull;
		//m_pPlayer->pev->viewmodelindex = NULL;
	}

	if (m_pPlayer->m_pActiveItem != this)
	{
		DestroyItem();
	}
	else if( !g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
	{
		// Another weapon wasn't selected. Get rid of current one
		if( m_pPlayer->m_pActiveItem == this )
		{
			m_pPlayer->ResetAutoaim();
			m_pPlayer->m_pActiveItem->Holster();
			m_pPlayer->m_pLastItem = NULL;
			m_pPlayer->m_pActiveItem = NULL;
		}
	}
}

//=========================================================================
// GetNextAttackDelay - An accurate way of calcualting the next attack time.
//=========================================================================
float CBasePlayerWeapon::GetNextAttackDelay( float delay )
{
	if( m_flLastFireTime == 0 || m_flNextPrimaryAttack == -1.0f )
	{
		// At this point, we are assuming that the client has stopped firing
		// and we are going to reset our book keeping variables.
		m_flLastFireTime = gpGlobals->time;
		m_flPrevPrimaryAttack = delay;
	}
	// calculate the time between this shot and the previous
	float flTimeBetweenFires = gpGlobals->time - m_flLastFireTime;
	float flCreep = 0.0f;
	if( flTimeBetweenFires > 0 )
		flCreep = flTimeBetweenFires - m_flPrevPrimaryAttack; // postive or negative

	// save the last fire time
	m_flLastFireTime = gpGlobals->time;

	float flNextAttack = UTIL_WeaponTimeBase() + delay - flCreep;
	// we need to remember what the m_flNextPrimaryAttack time is set to for each shot,
	// store it as m_flPrevPrimaryAttack.
	m_flPrevPrimaryAttack = flNextAttack - UTIL_WeaponTimeBase();
	//char szMsg[256];
	//safe_snprintf( szMsg, sizeof(szMsg), "next attack time: %0.4f\n", gpGlobals->time + flNextAttack );
	//OutputDebugString( szMsg );
	return flNextAttack;
}

TYPEDESCRIPTION	CConfigurableWeapon::m_SaveData[] =
{
	DEFINE_FIELD( CConfigurableWeapon, m_fInSpecialReload, FIELD_INTEGER ),
	DEFINE_FIELD( CConfigurableWeapon, m_wasEmptyReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_switchingBody, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_wasInAltModeBeforeSwitchingBody, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_wasInAltModeBeforeEjectLate, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_switchingMode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_playedFirstDeploy, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_shouldRestartReloading, FIELD_BOOLEAN ),

	DEFINE_FIELD( CConfigurableWeapon, m_kickBackDirectionVertical, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_kickBackDirectionLateral, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_lastShotWasInAltMode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_bDelayFire, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_iShotsFired, FIELD_INTEGER ),
	DEFINE_FIELD( CConfigurableWeapon, m_flInaccuracy, FIELD_FLOAT ),
	DEFINE_FIELD( CConfigurableWeapon, m_flLastFire, FIELD_TIME ),
	DEFINE_FIELD( CConfigurableWeapon, m_flDecreaseShotsFired, FIELD_TIME ),

	DEFINE_FIELD( CConfigurableWeapon, m_bLaserActive, FIELD_BOOLEAN ),

	DEFINE_FIELD( CConfigurableWeapon, m_burstFireIsAlt, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_burstShotsFired, FIELD_INTEGER ),
	DEFINE_FIELD( CConfigurableWeapon, m_burstTime, FIELD_TIME ),
	DEFINE_FIELD( CConfigurableWeapon, m_burstSpreadX, FIELD_FLOAT ),
	DEFINE_FIELD( CConfigurableWeapon, m_burstSpreadY, FIELD_FLOAT ),

	DEFINE_FIELD( CConfigurableWeapon, m_flPumpTime, FIELD_TIME ),

	DEFINE_FIELD( CConfigurableWeapon, m_iSwing, FIELD_INTEGER ),
	DEFINE_FIELD( CConfigurableWeapon, m_iSwingMode, FIELD_INTEGER ),
	DEFINE_FIELD( CConfigurableWeapon, m_swingIsAltAttack, FIELD_BOOLEAN ),

	DEFINE_FIELD( CConfigurableWeapon, m_flRechargeTime, FIELD_TIME ),

	DEFINE_FIELD( CConfigurableWeapon, m_chargingAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_chargingAltFire, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_shouldPlayCooldown, FIELD_BOOLEAN ),
	DEFINE_FIELD( CConfigurableWeapon, m_chargeStartTime, FIELD_TIME ),

	DEFINE_FIELD( CConfigurableWeapon, m_toolTriggerTime, FIELD_TIME ),

	DEFINE_FIELD( CConfigurableWeapon, m_primaryFireEndTime, FIELD_TIME ),
	DEFINE_FIELD( CConfigurableWeapon, m_secondaryFireEndTime, FIELD_TIME ),

	DEFINE_FIELD( CConfigurableWeapon, m_cActiveRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CConfigurableWeapon, m_iFirePhase, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CConfigurableWeapon, CBasePlayerWeapon )

bool CConfigurableWeapon::IsUseable()
{
	return CanRechargeAmmo() || CBasePlayerWeapon::IsUseable();
}

bool CConfigurableWeapon::AddToPlayer(CBasePlayer *pPlayer)
{
	const WeaponParameters& params = MyParameters();
	bool result;
	if (iFlags() & ITEM_FLAG_EXHAUSTIBLE)
	{
		result = CBasePlayerWeapon::AddToPlayer(pPlayer);
	}
	else
	{
		pev->body = params.viewModelBody.Get(m_inAltMode);
		result = AddToPlayerDefault(pPlayer);
	}
#if !CLIENT_DLL
	if (result && g_pGameRules->IsMultiplayer() && m_packedTime > 0.0f)
	{
		if (CanRechargeAmmo())
		{
			const float timeSpent = gpGlobals->time - m_packedTime;
			const float interval = params.recharge.interval.Get(InAltMode());
			const int ammo = (int)(timeSpent / interval);

			m_flRechargeTime = gpGlobals->time + interval;

			if (ammo > 0)
			{
				pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = Q_min(pPlayer->m_rgAmmo[PrimaryAmmoIndex()] + ammo, g_AmmoRegistry.GetMaxAmmo(PrimaryAmmoIndex()));
			}
		}
	}
	m_packedTime = 0.0f;
#endif
	return result;
}

extern int gmsgSetBody;

void CConfigurableWeapon::SetBody(int body)
{
	pev->body = body;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetBody, nullptr, m_pPlayer->pev);
	WRITE_SHORT(body);
	MESSAGE_END();
}

void CConfigurableWeapon::KickBack(const WeaponKickBack& kickBack)
{
	auto applyKickBack = [this](float currentPunchAngle, float base, float modifier, float maxValue, bool direction, int shotsFired) {
		if (maxValue == 0.0f)
		{
			currentPunchAngle = UTIL_SharedRandomFloat(m_pPlayer->random_seed + 1, -base, base);
		}
		else
		{
			const float kick = shotsFired == 1 ? base : shotsFired * modifier + base;
			if (direction)
			{
				currentPunchAngle += kick;
				if (currentPunchAngle > maxValue)
				{
					currentPunchAngle = maxValue;
				}
			}
			else
			{
				currentPunchAngle -= kick;
				if (currentPunchAngle < -maxValue)
				{
					currentPunchAngle = -maxValue;
				}
			}
		}
		return currentPunchAngle;
	};

	m_pPlayer->pev->punchangle.x = applyKickBack(m_pPlayer->pev->punchangle.x, kickBack.verticalBase, kickBack.verticalModifier, kickBack.verticalMax, m_kickBackDirectionVertical, m_iShotsFired);
	m_pPlayer->pev->punchangle.y = applyKickBack(m_pPlayer->pev->punchangle.y, kickBack.lateralBase, kickBack.lateralModifier, kickBack.lateralMax, m_kickBackDirectionLateral, m_iShotsFired);

	if (kickBack.directionChangeVertical > 0)
	{
		if (RANDOM_LONG(0, kickBack.directionChangeVertical) == 0)
			m_kickBackDirectionVertical = !m_kickBackDirectionVertical;
	}

	if (kickBack.directionChangeLateral > 0)
	{
		if (RANDOM_LONG(0, kickBack.directionChangeLateral) == 0)
			m_kickBackDirectionLateral = !m_kickBackDirectionLateral;
	}
}

//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS( weaponbox, CWeaponBox )

TYPEDESCRIPTION	CWeaponBox::m_SaveData[] =
{
	DEFINE_ARRAY( CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_TYPES ),
	DEFINE_ARRAY( CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_TYPES ),
	DEFINE_ARRAY( CWeaponBox, m_rgpPlayerWeapons, FIELD_CLASSPTR, MAX_WEAPONS ),
	DEFINE_FIELD( CWeaponBox, m_cAmmoTypes, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CWeaponBox, CBaseDelay )

//=========================================================
//
//=========================================================
void CWeaponBox::Precache()
{
	PRECACHE_MODEL( "models/w_weaponbox.mdl" );
}

//=========================================================
//=========================================================
void CWeaponBox::KeyValue( KeyValueData *pkvd )
{
	CBaseDelay::KeyValue(pkvd);
	if (!pkvd->fHandled)
	{
		if( m_cAmmoTypes < MAX_AMMO_TYPES )
		{
			PackAmmo( ALLOC_STRING( pkvd->szKeyName ), atoi( pkvd->szValue ) );
			m_cAmmoTypes++;// count this new ammo type.

			pkvd->fHandled = true;
		}
		else
		{
			ALERT( at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_TYPES );
		}
	}
}

//=========================================================
// CWeaponBox - Spawn 
//=========================================================
void CWeaponBox::Spawn()
{
	Precache();

	if (pev->movetype < 0)
		pev->movetype = MOVETYPE_NONE;
	else if (pev->movetype == 0)
		pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	//UTIL_SetSize( pev, g_vecZero, g_vecZero );

	const int itemSize = 24;
	UTIL_SetSize( pev, Vector( -itemSize, -itemSize, 0 ), Vector( itemSize, itemSize, itemSize ) );

	SET_MODEL( ENT( pev ), "models/w_weaponbox.mdl" );
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::Kill()
{
	CBasePlayerWeapon *pWeapon;
	int i;

	// destroy the weapons
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = m_rgpPlayerWeapons[i];

		if( pWeapon )
		{
			pWeapon->SetThink( &CBaseEntity::SUB_Remove );
			pWeapon->pev->nextthink = gpGlobals->time + 0.1f;
		}
	}

	// remove the box
	UTIL_Remove( this );
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================

void CWeaponBox::Touch( CBaseEntity *pOther )
{
	if (IsPickableByTouch(this)) {
		TouchOrUse(pOther);
	}
}

int CWeaponBox::ObjectCaps()
{
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW)) {
		return CBaseDelay::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE;
	} else {
		return CBaseDelay::ObjectCaps();
	}
}

void CWeaponBox::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW) ) {
		TouchOrUse(pCaller);
	}
}

void CWeaponBox::TouchOrUse( CBaseEntity *pOther )
{
	if( pev->movetype == MOVETYPE_TOSS && !( pev->flags & FL_ONGROUND ) )
	{
		return;
	}

	if( !pOther->IsPlayer() )
	{
		// only players may touch a weaponbox.
		return;
	}

	if( !pOther->IsAlive() )
	{
		// no dead guys.
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	int i;

	bool shouldRemove = false;

	// go through my weapons and try to give the usable ones to the player. 
	// it's important the the player be given ammo first, so the weapons code doesn't refuse 
	// to deploy a better weapon that the player may pick up because he has no ammo for it.

	// dole out ammo
	for( i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		if( !FStringNull( m_rgiszAmmo[i] ) )
		{
			// there's some ammo of this type.
			if (pPlayer->GiveAmmo( m_rgAmmo[i], STRING( m_rgiszAmmo[i] ) ) > 0) {
				//ALERT( at_console, "Gave %d rounds of %s\n", m_rgAmmo[i], STRING( m_rgiszAmmo[i] ) );
				shouldRemove = true;
				// now empty the ammo from the weaponbox since we just gave it to the player
				m_rgiszAmmo[i] = iStringNull;
				m_rgAmmo[i] = 0;
			}
		}
	}

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pItem = m_rgpPlayerWeapons[i];

		// have at least one weapon in this slot
		if( pItem )
		{
			//ALERT( at_console, "trying to give %s\n", STRING( m_rgpPlayerItems[i]->pev->classname ) );

			m_rgpPlayerWeapons[i] = NULL;// unlink this weapon from the box

			if( pPlayer->AddPlayerItem( pItem ) > DID_NOT_GET_ITEM )
			{
				shouldRemove = true;
			}
		}
	}

	if (shouldRemove) {
		pOther->EmitSoundScript(GetSoundScript(Items::weaponPickupSoundScript));
		SetTouch( NULL );
		SUB_UseTargets( pOther );
		UTIL_Remove(this);
	}
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
bool CWeaponBox::PackWeapon( CBasePlayerWeapon *pWeapon )
{
	// is one of these weapons already packed in this box?
	if( HasWeapon( pWeapon ) )
	{
		return false;// box can only hold one of each weapon type
	}

	if( pWeapon->m_pPlayer )
	{
		if( !pWeapon->m_pPlayer->RemovePlayerItem( pWeapon, true ) )
		{
			// failed to unhook the weapon from the player!
			return false;
		}
	}

	InsertWeaponById(pWeapon);

	pWeapon->pev->spawnflags |= SF_NORESPAWN;// never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->aiment = nullptr;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink( NULL );// crowbar may be trying to swing again, etc.
	pWeapon->SetTouch( NULL );
	pWeapon->m_pPlayer = NULL;
	UTIL_SetOrigin(pWeapon->pev, pev->origin);

	pWeapon->m_packedTime = gpGlobals->time;

	//ALERT( at_console, "packed %s\n", STRING( pWeapon->pev->classname ) );

	return true;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
bool CWeaponBox::PackAmmo( string_t iszName, int iCount )
{
	if( FStringNull( iszName ) )
	{
		// error here
		ALERT( at_console, "NULL String in PackAmmo!\n" );
		return false;
	}

	const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(STRING(iszName));
	if( ammoType && iCount > 0 )
	{
		//ALERT( at_console, "Packed %d rounds of %s\n", iCount, STRING( iszName ) );
		int i;

		for( i = 1; i < MAX_AMMO_TYPES && !FStringNull( m_rgiszAmmo[i] ); i++ )
		{
			if( stricmp( ammoType->name, STRING( m_rgiszAmmo[i] ) ) == 0 )
			{
				int iAdd = Q_min( iCount, ammoType->maxAmmo - m_rgAmmo[i] );
				if( iCount == 0 || iAdd > 0 )
				{
					m_rgAmmo[i] += iAdd;

					return true;
				}
				return false;
			}
		}
		if( i < MAX_AMMO_TYPES )
		{
			m_rgiszAmmo[i] = MAKE_STRING( ammoType->name );
			m_rgAmmo[i] = iCount;

			return true;
		}
		ALERT( at_console, "out of named ammo slots\n" );
		return false;
	}

	return false;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
bool CWeaponBox::HasWeapon( CBasePlayerWeapon *pCheckItem )
{
	return WeaponById(pCheckItem->WeaponId()) != nullptr;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
bool CWeaponBox::IsEmpty()
{
	int i;

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		if( m_rgpPlayerWeapons[i] )
		{
			return false;
		}
	}

	for( i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		if( !FStringNull( m_rgiszAmmo[i] ) )
		{
			// still have a bit of this type of ammo
			return false;
		}
	}

	return true;
}

void CWeaponBox::SetWeaponModel(CBasePlayerWeapon *pItem)
{
	if (pItem)
	{
		const char* worldModel = pItem->MyWorldModel();
		Vector weaponAngles = pev->angles;
		weaponAngles.y += 180 + RANDOM_LONG(-15,15);

		SET_MODEL( ENT( pev ), worldModel );
		pev->angles = weaponAngles;
		if (pItem->WeaponId() == WEAPON_TRIPMINE) {
			pev->body = 3;
			pev->sequence = 8;
		}
	}
}

//=========================================================
//=========================================================
void CWeaponBox::SetObjectCollisionBox()
{
	SetMyObjectCollisionBox(Vector( -16, -16, 0 ), Vector( 16, 16, 16 ));
}

void CWeaponBox::InsertWeaponById(CBasePlayerWeapon *pItem)
{
	if (pItem && pItem->WeaponId() && pItem->WeaponId() <= MAX_WEAPONS) {
		m_rgpPlayerWeapons[pItem->WeaponId()-1] = pItem;
	}
}

CBasePlayerWeapon* CWeaponBox::WeaponById(int id)
{
	if (id && id <= MAX_WEAPONS) {
		return m_rgpPlayerWeapons[id-1];
	}
	return NULL;
}

void CBasePlayerWeapon::PrintState()
{
	ALERT( at_console, "primary:  %f\n", (double)m_flNextPrimaryAttack );
	ALERT( at_console, "idle   :  %f\n", (double)m_flTimeWeaponIdle );

	//ALERT( at_console, "nextrl :  %f\n", m_flNextReload );
	//ALERT( at_console, "nextpum:  %f\n", m_flPumpTime );

	//ALERT( at_console, "m_frt  :  %f\n", m_fReloadTime );
	ALERT( at_console, "m_finre:  %i\n", m_fInReload );
	//ALERT( at_console, "m_finsr:  %i\n", m_fInSpecialReload );

	ALERT( at_console, "m_iclip:  %i\n", m_iClip );
}
