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

===== bmodels.cpp ========================================================

  spawn, think, and use functions for entities that use brush models

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "func_break.h"
#include "decals.h"
#include "explode.h"
#include "game.h"
#include "locus.h"
#include "common_soundscripts.h"
#include "error_collector.h"

extern DLL_GLOBAL Vector	g_vecAttackDir;

typedef enum
{
	FB_TA_NO = 0,
	FB_TA_BREAKABLE = 1,
	FB_TA_ACTIVATOR_OR_ATTACKER = 2,
} BREAKABLE_TARGET_ACTIVATOR;

// =================== FUNC_Breakable ==============================================

// Just add more items to the bottom of this array and they will automagically be supported
// This is done instead of just a classname in the FGD so we can control which entities can
// be spawned, and still remain fairly flexible
const char *CBreakable::pSpawnObjects[] =
{
	NULL,			// 0
	"item_battery",		// 1
	"item_healthkit",	// 2
	"weapon_9mmhandgun",	// 3
	"ammo_9mmclip",		// 4
	"weapon_9mmAR",		// 5
	"ammo_9mmAR",		// 6
	"ammo_ARgrenades",	// 7
	"weapon_shotgun",	// 8
	"ammo_buckshot",	// 9
	"weapon_crossbow",	// 10
	"ammo_crossbow",	// 11
	"weapon_357",		// 12
	"ammo_357",		// 13
	"weapon_rpg",		// 14
	"ammo_rpgclip",		// 15
	"ammo_gaussclip",	// 16
	"weapon_handgrenade",	// 17
	"weapon_tripmine",	// 18
	"weapon_satchel",	// 19
	"weapon_snark",		// 20
	"weapon_hornetgun",	// 21
	"weapon_crowbar",	// 22
	"weapon_pipewrench",	// 23
	"weapon_sniperrifle",	// 24
	"ammo_762",		// 25
	"weapon_knife",		// 26
	"weapon_m249",		// 27
	"weapon_penguin",	// 28
	"ammo_556",		// 29
	"weapon_sporelauncher",	// 30
	"weapon_displacer",		// 31
	"ammo_9mmbox",	// 32
	"weapon_uzi",		// 33
	"weapon_uziakimbo",		// 34
	"weapon_eagle",		// 35
	"weapon_grapple",		// 36
	"weapon_medkit",		// 37
	"item_suit",		// 38
};

void CBreakable::KeyValue( KeyValueData* pkvd )
{
	// UNDONE_WC: explicitly ignoring these fields, but they shouldn't be in the map file!
	if( FStrEq( pkvd->szKeyName, "explosion" ) )
	{
		if( !stricmp( pkvd->szValue, "directed" ) )
			m_Explosion = expDirected;
		else if( !stricmp( pkvd->szValue, "random" ) )
			m_Explosion = expRandom;
		else
			m_Explosion = expRandom;

		if (strcmp(pkvd->szValue, "1") == 0)
		{
			m_Explosion = expDirected;
		}

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "material" ) )
	{
		int i = atoi( pkvd->szValue );

		// 0:glass, 1:metal, 2:flesh, 3:wood

		if( ( i < 0 ) || ( i >= matLastMaterial ) )
			m_Material = matWood;
		else
			m_Material = (Materials)i;

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "deadmodel" ) )
	{
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "shards" ) )
	{
			//m_iShards = atof( pkvd->szValue );
			pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "gibmodel" ) )
	{
		m_iszGibModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spawnobject" ) )
	{
		int object = atoi( pkvd->szValue );
		if( object > 0 && object < (int)ARRAYSIZE( pSpawnObjects ) )
			m_iszSpawnObject = MAKE_STRING( pSpawnObjects[object] );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spawnobject_name" ) )
	{
		m_iszSpawnObject = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spawnobject_template" ) )
	{
		m_iszSpawnObjectTemplate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "randomitem_template" ) )
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "explodemagnitude" ) )
	{
		ExplosionSetMagnitude( atoi( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "target_activator" ) )
	{
		m_targetActivator = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iGibs") )
	{
		m_iGibs = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "lip" ) )
		pkvd->fHandled = true;
	else if( FStrEq( pkvd->szKeyName, "whenhit" ) )
	{
		m_iszWhenHit = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "switch_texture_when_damaged" ) )
	{
		m_switchTextureWhenDamaged = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "spark_when_hit" ) )
	{
		m_sparkWhenHit = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = true;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

//
// func_breakable - bmodel that breaks into pieces after taking damage
//
LINK_ENTITY_TO_CLASS( func_breakable, CBreakable )

TYPEDESCRIPTION CBreakable::m_SaveData[] =
{
	DEFINE_FIELD( CBreakable, m_Material, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakable, m_Explosion, FIELD_INTEGER ),

// Don't need to save/restore these because we precache after restore
//	DEFINE_FIELD( CBreakable, m_idShard, FIELD_INTEGER ),

	DEFINE_FIELD( CBreakable, m_angle, FIELD_FLOAT ),
	DEFINE_FIELD( CBreakable, m_iszGibModel, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_iszSpawnObject, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_iszSpawnObjectTemplate, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_targetActivator, FIELD_SHORT ),
	DEFINE_FIELD( CBreakable, m_iGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakable, m_iszWhenHit, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_pHitProxy, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBreakable, m_switchTextureWhenDamaged, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBreakable, m_sparkWhenHit, FIELD_BOOLEAN ),

	// Explosion magnitude is stored in pev->impulse
};

IMPLEMENT_SAVERESTORE( CBreakable, CBaseDelay )

void CBreakable::Spawn()
{
	Precache();

	if( FBitSet( pev->spawnflags, SF_BREAK_TRIGGER_ONLY | SF_BREAK_NOT_SOLID ) )
		pev->takedamage	= DAMAGE_NO;
	else
		pev->takedamage	= DAMAGE_YES;
  
	pev->solid = FBitSet(pev->spawnflags, SF_BREAK_NOT_SOLID) ? SOLID_NOT : SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	m_angle = pev->angles.y;
	pev->angles.y = 0;

	// HACK:  matGlass can receive decals, we need the client to know about this
	//  so use class to store the material flag
	if( m_Material == matGlass )
	{
		pev->playerclass = 1;
	}

	SET_MODEL( ENT( pev ), STRING( pev->model ) );//set size and link into world.

	SetTouch( &CBreakable::BreakTouch );
	if( FBitSet( pev->spawnflags, SF_BREAK_TRIGGER_ONLY ) )		// Only break on trigger
		SetTouch( NULL );

	SetMyHealth(0.0f);
	pev->max_health = pev->health;

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if( !IsBreakable() && pev->rendermode != kRenderNormal )
		pev->flags |= FL_WORLDBRUSH;

	InitLootRandomSeed();
}

void CBreakable::Activate()
{
	if (FBitSet(pev->spawnflags, SF_BREAK_OP4MORTAR_ONLY))
	{
		const Vector center = Center();
		g_errorCollector.AddFormattedDeprecation("%s (center: %g, %g, %g) has the spawnflag %d. This will be removed/replaced in future. Use entity template with custom take_damage property instead.",
												 STRING(pev->classname), center.x, center.y, center.z, SF_BREAK_OP4MORTAR_ONLY);
	}
	CBaseDelay::Activate();
}

const NamedSoundScript CBreakable::woodSoundScript = {
	CHAN_VOICE,
	{"debris/wood1.wav", "debris/wood2.wav", "debris/wood3.wav"},
	"Breakable.Wood"
};

const NamedSoundScript CBreakable::fleshSoundScript = {
	CHAN_VOICE,
	{
		"debris/flesh1.wav", "debris/flesh2.wav", "debris/flesh3.wav",
		"debris/flesh5.wav", "debris/flesh6.wav", "debris/flesh7.wav",
	},
	"Breakable.Flesh"
};

const NamedSoundScript CBreakable::metalSoundScript = {
	CHAN_VOICE,
	{"debris/metal1.wav", "debris/metal3.wav"},
	"Breakable.Metal"
};

const NamedSoundScript CBreakable::concreteSoundScript = {
	CHAN_VOICE,
	{"debris/concrete1.wav", "debris/concrete2.wav", "debris/concrete3.wav"},
	"Breakable.Concrete"
};

const NamedSoundScript CBreakable::glassSoundScript = {
	CHAN_VOICE,
	{"debris/glass1.wav", "debris/glass2.wav", "debris/glass3.wav"},
	"Breakable.Glass"
};

const NamedSoundScript CBreakable::computerSoundScript = {
	CHAN_VOICE,
	{"debris/glass1.wav", "debris/glass2.wav", "debris/glass3.wav", "debris/metal1.wav", "debris/metal3.wav"},
	"Breakable.Computer"
};

const NamedSoundScript CBreakable::bustWoodSoundScript = {
	CHAN_VOICE,
	{"debris/bustcrate1.wav", "debris/bustcrate2.wav"},
	"Breakable.BustWood"
};

const NamedSoundScript CBreakable::bustFleshSoundScript = {
	CHAN_VOICE,
	{"debris/bustflesh1.wav", "debris/bustflesh2.wav"},
	"Breakable.BustFlesh"
};

const NamedSoundScript CBreakable::bustComputerSoundScript = {
	CHAN_VOICE,
	{"debris/bustmetal1.wav", "debris/bustmetal2.wav"},
	"Breakable.BustComputer"
};

const NamedSoundScript CBreakable::bustGlassSoundScript = {
	CHAN_VOICE,
	{"debris/bustglass1.wav", "debris/bustglass2.wav"},
	"Breakable.BustGlass"
};

const NamedSoundScript CBreakable::bustMetalSoundScript = {
	CHAN_VOICE,
	{"debris/bustmetal1.wav", "debris/bustmetal2.wav"},
	"Breakable.BustMetal"
};

const NamedSoundScript CBreakable::bustConcreteSoundScript = {
	CHAN_VOICE,
	{"debris/bustconcrete1.wav", "debris/bustconcrete2.wav"},
	"Breakable.BustConcrete"
};

const NamedSoundScript CBreakable::bustRocksSoundScript = {
	CHAN_VOICE,
	{"debris/bustconcrete1.wav", "debris/bustconcrete2.wav"},
	"Breakable.BustRocks"
};

const NamedSoundScript CBreakable::bustCeilingSoundScript = {
	CHAN_VOICE,
	{"debris/bustceiling.wav"},
	"Breakable.BustCeiling"
};

const char* CBreakable::sparkSoundScript = "Breakable.Spark";

static const NamedSoundScript* HitSoundScriptForMaterial(Materials material)
{
	switch(material)
	{
	case matWood:
		return &CBreakable::woodSoundScript;
	case matFlesh:
		return &CBreakable::fleshSoundScript;
	case matUnbreakableGlass:
	case matGlass:
		return &CBreakable::glassSoundScript;
	case matMetal:
		return &CBreakable::metalSoundScript;
	case matCinderBlock:
	case matRocks:
		return &CBreakable::concreteSoundScript;
	case matComputer:
		return &CBreakable::computerSoundScript;
	default:
		return nullptr;
	}
}

static void PrecacheMaterialSound(CBaseEntity* pEntity, Materials material)
{
	const NamedSoundScript* soundScript = HitSoundScriptForMaterial(material);
	if (soundScript)
		pEntity->RegisterAndPrecacheSoundScript(*soundScript);
}

static const NamedSoundScript* BustSoundScriptForMateral(Materials material)
{
	switch( material )
	{
	case matWood:
		return &CBreakable::bustWoodSoundScript;
	case matFlesh:
		return &CBreakable::bustFleshSoundScript;
	case matComputer:
		return &CBreakable::bustComputerSoundScript;
	case matUnbreakableGlass:
	case matGlass:
		return &CBreakable::bustGlassSoundScript;
	case matMetal:
		return &CBreakable::bustMetalSoundScript;
	case matCinderBlock:
		return &CBreakable::bustConcreteSoundScript;
	case matRocks:
		return &CBreakable::bustRocksSoundScript;
	case matCeilingTile:
		return &CBreakable::bustCeilingSoundScript;
	default:
		return nullptr;
	}
}

static void PrecacheMaterialBustSounds(CBaseEntity* pEntity, Materials material)
{
	const NamedSoundScript* soundScript = BustSoundScriptForMateral(material);
	if (soundScript)
		pEntity->RegisterAndPrecacheSoundScript(*soundScript);
}

static char SoundFlagForMaterial(Materials material)
{
	switch(material)
	{
	case matGlass:
	case matUnbreakableGlass:
		return BREAK_GLASS;
	case matWood:
		return BREAK_WOOD;
	case matComputer:
		return BREAK_METAL;
	case matMetal:
		return BREAK_METAL;
	case matFlesh:
		return BREAK_FLESH;
	case matRocks:
	case matCinderBlock:
		return BREAK_CONCRETE;
	default:
		return 0;
	}
}

static const char* DefaultMaterialGibModel( Materials material )
{
	switch( material )
	{
	case matWood:
		return "models/woodgibs.mdl";
	case matFlesh:
		return "models/fleshgibs.mdl";
	case matComputer:
		return "models/computergibs.mdl";
	case matUnbreakableGlass:
	case matGlass:
		return "models/glassgibs.mdl";;
	case matMetal:
		return "models/metalplategibs.mdl";
	case matCinderBlock:
		return "models/cindergibs.mdl";
	case matRocks:
		return "models/rockgibs.mdl";
	case matCeilingTile:
		return "models/ceilinggibs.mdl";
	case matNone:
	case matLastMaterial:
		break;
	default:
		break;
	}
	return NULL;
}

void CBreakable::Precache()
{
	PrecacheMaterialBustSounds(this, m_Material);
	PrecacheMaterialSound(this, m_Material);

	if (ShouldSparkOnHit())
	{
		SoundScriptParamOverride paramOverride;
		paramOverride.OverrideChannel(CHAN_VOICE);
		RegisterAndPrecacheSoundScript(sparkSoundScript, ::materialSparkSoundScript);
	}

	const char *pGibName = NULL;
	if( m_iszGibModel )
		pGibName = STRING( m_iszGibModel );
	else
		pGibName = DefaultMaterialGibModel(m_Material);

	m_idShard = PRECACHE_MODEL( pGibName );

	// Precache the spawn item's data
	if (!FStringNull(m_iszSpawnObject))
	{
		EntityOverrides entityOverrides;
		entityOverrides.entTemplate = m_iszSpawnObjectTemplate;
		UTIL_PrecacheOther(STRING(m_iszSpawnObject), entityOverrides);
	}
}

// play shard sound when func_breakable takes damage.
// the more damage, the louder the shard sound.
void CBreakable::DamageSound()
{
	int pitch;
	if( RANDOM_LONG( 0, 2 ) )
		pitch = PITCH_NORM;
	else
		pitch = 95 + RANDOM_LONG( 0, 34 );

	float fvol = RANDOM_FLOAT( 0.75f, 1.0f );

	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchRelative(pitch);
	paramOverride.OverrideVolumeRelative(fvol);

	const NamedSoundScript* soundScript = HitSoundScriptForMaterial(m_Material);
	if (soundScript)
		EmitSoundScript(soundScript->name, paramOverride);
}

void CBreakable::BreakTouch( CBaseEntity *pOther )
{
	float flDamage;
	entvars_t* pevToucher = pOther->pev;

	// only players can break these right now
	if( !pOther->IsPlayer() || !IsBreakable() )
	{
		return;
	}

	if( FBitSet( pev->spawnflags, SF_BREAK_TOUCH ) )
	{
		// can be broken when run into 
		flDamage = pevToucher->velocity.Length() * 0.01f;

		if( flDamage >= pev->health )
		{
			SetTouch( NULL );
			TakeDamage( pevToucher, pevToucher, DamageInfo(flDamage, DMG_CRUSH) );

			// do a little damage to player if we broke glass or computer
			pOther->TakeDamage( pev, pev, DamageInfo(flDamage/4, DMG_SLASH) );
		}
	}

	if( FBitSet( pev->spawnflags, SF_BREAK_PRESSURE ) && pevToucher->absmin.z >= pev->maxs.z - 2 )
	{
		// can be broken when stood upon
		// play creaking sound here.
		DamageSound();

		SetThink( &CBreakable::Die );
		SetTouch( NULL );

		if( m_flDelay == 0.0f )
		{
			// !!!BUGBUG - why doesn't zero delay work?
			m_flDelay = 0.1f;
		}

		pev->nextthink = pev->ltime + m_flDelay;
	}
}

//
// Smash the our breakable object
//

// Break when triggered
void CBreakable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( IsBreakable() )
	{
		pev->angles.y = m_angle;
		UTIL_MakeVectors( pev->angles );
		g_vecAttackDir = gpGlobals->v_forward;

		DieToActivator(pActivator);
	}
}

NODE_LINKENT CBreakable::HandleLinkEnt(int afCapMask, bool nodeQueryStatic)
{
	if (nodeQueryStatic) {
		return NLE_ALLOW;
	}
	if (FBitSet(pev->spawnflags, SF_BREAK_NOT_SOLID)) {
		return NLE_ALLOW;
	}
	return NLE_PROHIBIT;
}

CBaseEntity* CBreakable::GetHitProxy()
{
	if (!m_pHitProxy)
	{
		m_pHitProxy = GetClassPtr((CPointEntity*)NULL);
		m_pHitProxy->pev->classname = MAKE_STRING("info_target");
	}
	return m_pHitProxy;
}

void CBreakable::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	// random spark if this is a 'computer' object
	if( RANDOM_LONG( 0, 1 ) )
	{
		if (ShouldSparkOnHit())
		{
			UTIL_Sparks( ptr->vecEndPos );
			EmitSoundScript(sparkSoundScript);
		}
		if (m_Material == matUnbreakableGlass)
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 0.5f, 1.5f ) );
	}

	//LRC
	if (!FStringNull(m_iszWhenHit))
	{
		CBaseEntity* pHitProxy = GetHitProxy();
		if (pHitProxy)
		{
			pHitProxy->pev->origin = ptr->vecEndPos;
			if (FBitSet(pev->spawnflags, SF_BREAKABLE_INVERT))
			{
				vecDir.y = -vecDir.y;
				vecDir.x = -vecDir.x;
			}
			pHitProxy->pev->velocity = vecDir;
			pHitProxy->pev->angles = UTIL_VecToAngles(vecDir); //AJH

			FireTargets(STRING(m_iszWhenHit), pHitProxy, this);
		}
	}

	CBaseDelay::TraceAttack( pevInflictor, pevAttacker, damageInfo, vecDir, ptr );
}

bool CBreakable::ShouldSparkOnHit()
{
	return m_sparkWhenHit || m_Material == matComputer;
}

//=========================================================
// Special takedamage for func_breakable. Allows us to make
// exceptions that are breakable-specific
// bitsDamageType indicates the type of damage sustained ie: DMG_CRUSH
//=========================================================
DamageInfo CBreakable::DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo)
{
	DamageInfo damageInfo = inputDamageInfo;

	// Breakables take double damage from the crowbar
	if( damageInfo.type & DMG_CLUB )
		damageInfo.damage *= 2.0f;

	// Boxes / glass / etc. don't take much poison damage, just the impact of the dart - consider that 10%
	if( damageInfo.type & DMG_POISON )
		damageInfo.damage *= 0.1f;

	return damageInfo;
}

TakeDamageResult CBreakable::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo )
{
	TakeDamageResult takeDamageResult;
	Vector vecTemp;

	if ((pev->spawnflags & SF_BREAK_EXPLOSIVES_ONLY) && !(inputDamageInfo.type & DMG_BLAST))
		return takeDamageResult;
	if ((pev->spawnflags & SF_BREAK_OP4MORTAR_ONLY) && !FClassnameIs(pevInflictor, "mortar_shell"))
		return takeDamageResult;

	DamageInfo damageInfo = TransformDamageInfo(pevInflictor, pevAttacker, inputDamageInfo);
	if (damageInfo.mustSkip)
		return takeDamageResult;

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if( pevAttacker == pevInflictor )	
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5f ) );
		
		// if a client hit the breakable with a crowbar, and breakable is crowbar-sensitive, break it now.
		if( FBitSet ( pevAttacker->flags, FL_CLIENT ) &&
				 FBitSet ( pev->spawnflags, SF_BREAK_CROWBAR ) && ( damageInfo.type & DMG_CLUB ) )
			damageInfo.damage = pev->health;
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5f ) );
	}
	
	if( !IsBreakable() )
		return takeDamageResult;

	// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();

	const float healthBeforeDamage = pev->health;

	if (pev->takedamage != DAMAGE_NO)
	{
		if (damageInfo.nonLethal)
			SetNonLethalHealthThreshold();
		if (ApplyDamageToHealth(damageInfo.damage))
			takeDamageResult.SetTookDamageToHealth();
	}

	if( pev->health <= 0 )
	{
		KilledResult killedResult = Killed( pevInflictor, pevAttacker, GIB_NORMAL );
		DieToActivator(CBaseEntity::Instance(pevAttacker));
		takeDamageResult.SetKilledResult(killedResult);
		return takeDamageResult;
	}
	else if ( m_switchTextureWhenDamaged && healthBeforeDamage > DamagedHealth() && pev->health <= DamagedHealth() )
	{
		pev->frame = 1;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.
	DamageSound();

	return takeDamageResult;
}

int CBreakable::TakeHealth(CBaseEntity *pHealer, float flHealth, int healType)
{
	const float healthBeforeHealed = pev->health;
	const float result = CBaseDelay::TakeHealth(pHealer, flHealth, healType);
	if (m_switchTextureWhenDamaged)
	{
		if (healthBeforeHealed <= DamagedHealth() && pev->health > DamagedHealth())
			pev->frame = 0;
	}
	return result;
}

static char PlayBreakableBustSound( CBaseEntity* pEntity, Materials material, float fvol, int pitch )
{
	SoundScriptParamOverride paramOverride;
	paramOverride.OverrideVolumeRelative(fvol);
	paramOverride.OverridePitchRelative(pitch);
	const NamedSoundScript* soundScript = BustSoundScriptForMateral(material);
	if (soundScript)
		pEntity->EmitSoundScript(*soundScript, paramOverride);
	return SoundFlagForMaterial(material);
}

static char ExtraBreakableFlags(int spawnflags)
{
	char cFlag = 0;
	if (FBitSet(spawnflags, SF_BREAK_SMOKE_TRAILS))
		cFlag |= BREAK_SMOKE;
	if (FBitSet(spawnflags, SF_BREAK_TRANSPARENT_GIBS))
		cFlag |= BREAK_TRANS;
	return cFlag;
}

void CBreakable::BreakModel(const Vector& vecSpot, const Vector& size, const Vector& vecVelocity, int shardModelIndex, int iGibs, char cFlag)
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL );

		// position
		WRITE_VECTOR( vecSpot );

		// size
		WRITE_VECTOR( size );

		// velocity
		WRITE_VECTOR( vecVelocity );

		// randomization
		WRITE_BYTE( 10 );

		// Model
		WRITE_SHORT( shardModelIndex );	//model id#

		// # of shards
		WRITE_BYTE( iGibs );	// if 0, let client decide

		// duration
		WRITE_BYTE( 25 );// 2.5 seconds

		// flags
		WRITE_BYTE( cFlag );
	MESSAGE_END();
}

void CBreakable::Die()
{
	DieToActivator(NULL);
}

void CBreakable::DieToActivator( CBaseEntity* pActivator )
{
	Vector vecSpot;// shard origin
	Vector vecVelocity;// shard velocity
	char cFlag = 0;
	int pitch;
	float fvol;

	pitch = 95 + RANDOM_LONG( 0, 29 );

	if( pitch > 97 && pitch < 103 )
		pitch = 100;

	// The more negative pev->health, the louder
	// the sound should be.

	fvol = RANDOM_FLOAT( 0.85f, 1.0 ) + ( fabs( pev->health ) / 100.0f );

	if( fvol > 1.0f )
		fvol = 1.0f;

	cFlag = PlayBreakableBustSound(this, m_Material, fvol, pitch);
	cFlag |= ExtraBreakableFlags(pev->spawnflags);

	if( m_Explosion == expDirected )
		vecVelocity = -g_vecAttackDir * 100.0f;
	else
	{
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
	}

	vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;

	if (m_iGibs >= 0)
	{
		BreakModel(vecSpot, pev->size, vecVelocity, m_idShard, m_iGibs, cFlag);
	}

	/*float size = pev->size.x;
	if( size < pev->size.y )
		size = pev->size.y;
	if( size < pev->size.z )
		size = pev->size.z;*/

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity *pList[256];
	int count = UTIL_EntitiesInBox( pList, 256, mins, maxs, FL_ONGROUND );
	if( count )
	{
		for( int i = 0; i < count; i++ )
		{
			ClearBits( pList[i]->pev->flags, FL_ONGROUND );
			pList[i]->pev->groundentity = NULL;
		}
	}

	// Don't fire something that could fire myself
	pev->targetname = 0;
	pev->effects |= EF_NODRAW;
	pev->takedamage = DAMAGE_NO;

	pev->solid = SOLID_NOT;

	// Fire targets on break
	CBaseEntity* pTargetActivator = 0;
	if (m_targetActivator == FB_TA_BREAKABLE)
	{
		pTargetActivator = this;
	}
	else if (m_targetActivator == FB_TA_ACTIVATOR_OR_ATTACKER)
	{
		pTargetActivator = pActivator;
	}
	SUB_UseTargets( pTargetActivator );

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = pev->ltime + 0.1f;
	if (pev->message)
	{
		CBaseEntity* foundEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
		if ( foundEntity && FClassnameIs(foundEntity->pev, "info_item_random"))
		{
			foundEntity->Use(this, this, USE_TOGGLE, 0.0f);
		}
		else
		{
			ALERT(at_error, "Random item template %s for %s not found or not info_item_random\n", STRING(pev->message), STRING(pev->classname));
		}
	}
	else if( !FStringNull(m_iszSpawnObject) )
	{
		const char* spawnObject = STRING(m_iszSpawnObject);
		const Vector bmodelOrigin = VecBModelOrigin( pev );
		bool shouldApplyPhysicsFix = false;
		if (ItemsPhysicsFix() > 0 && (strncmp(spawnObject, "item_", 5) == 0 || strncmp(spawnObject, "ammo_", 5) == 0))
		{
			TraceResult tr;
			UTIL_TraceLine(bmodelOrigin, Vector(bmodelOrigin.x, bmodelOrigin.y, pev->absmin.z - 1), ignore_monsters, edict(), &tr);
			if (tr.pHit)
			{
				const int contents = UTIL_PointContents( tr.vecEndPos );
				shouldApplyPhysicsFix = contents == 0;
			}
		}
		EntityOverrides entityOverrides;
		entityOverrides.entTemplate = m_iszSpawnObjectTemplate;
		CBaseEntity* pEntity = CBaseEntity::CreateNoSpawn( spawnObject, bmodelOrigin, pev->angles, edict(), entityOverrides );
		if (pEntity)
		{
			if (shouldApplyPhysicsFix && IsProbablyPickupClassname(spawnObject))
				pEntity->pev->spawnflags |= SF_ITEM_FIX_PHYSICS;
			DispatchSpawnAutoClean(pEntity);
		}
	}

	DropLoot(false);

	if( Explodable() )
	{
		ExplosionCreate( Center(), pev->angles, edict(), ExplosionMagnitude(), true );
	}
}

void CBreakable::UpdateOnRemove()
{
	CBaseDelay::UpdateOnRemove();
	if (m_pHitProxy) {
		m_pHitProxy->SetThink(&CBaseEntity::SUB_Remove);
		m_pHitProxy->pev->nextthink = gpGlobals->time + 0.1f;
		m_pHitProxy = NULL;
	}
}

bool CBreakable::IsBreakable()
{ 
	return m_Material != matUnbreakableGlass;
}

int CBreakable::DamageDecal( int bitsDamageType )
{
	if( m_Material == matGlass )
		return DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 );

	if( m_Material == matUnbreakableGlass )
		return DECAL_BPROOF1;

	return CBaseEntity::DamageDecal( bitsDamageType );
}

bool CBreakable::IsDestroyableObstacle()
{
	return pev->takedamage && IsBreakable();
}

#define SF_PUSHABLE_DISABLED (1<<24)

class CPushable : public CBreakable
{
public:
	void Spawn() override;
	void Precache() override;
	void Touch( CBaseEntity *pOther ) override;
	void Move( CBaseEntity *pMover, int push );
	void PreEntvarsKeyvalue( KeyValueData* pkvd ) override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	NODE_LINKENT HandleLinkEnt(int afCapMask, bool nodeQueryStatic) override;
	void EXPORT StopSound();
	//virtual void	SetActivator( CBaseEntity *pActivator ) { m_pPusher = pActivator; }

	int ObjectCaps() override {
		int caps = CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
		if (!FBitSet(pev->spawnflags, SF_PUSHABLE_DISABLED))
			caps |= FCAP_CONTINUOUS_USE;
		return caps;
	}
	bool PlaysItsOwnHitSounds() const override {
		return FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE);
	}
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	inline float MaxSpeed() { return m_maxSpeed; }

	// breakables use an overridden takedamage
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;

	int DamageDecal(int bitsDamageType) override;
	const char* DefaultDisplayName() override { return "Pushable"; }
	bool IsDestroyableObstacle() override;
	bool ShouldCollideWithCorpses() override { return !m_ignoreCorpses; }
	bool ShouldCollideWithTinyCreatures() override {
		return !g_modFeatures.ShouldIgnoreTinyCreatures(m_handleTinyCreatures);
	}

	int SizeForGrapple()
	{
		if (m_sizeForGrapple < 0)
			return GRAPPLE_NOT_A_TARGET;
		else if (m_sizeForGrapple > 0 && m_sizeForGrapple <= GRAPPLE_FIXED)
			return m_sizeForGrapple;
		else
		{
			const EntTemplate* entTemplate = GetMyEntTemplate();
			if (entTemplate && entTemplate->IsSizeForGrappleDefined())
				return entTemplate->SizeForGrapple();
		}
		return DefaultSizeForGrapple();
	}

	static TYPEDESCRIPTION m_SaveData[];

	int m_lastSound;	// no need to save/restore, just keeps the same sound from playing twice in a row
	float m_maxSpeed;
	float m_soundTime;
	bool m_ignoreCorpses;
	bool m_instantGibCorpses;
	short m_handleTinyCreatures;
	bool m_toggleable;
	short m_sizeForGrapple;

	static const NamedSoundScript moveSoundScript;
};

TYPEDESCRIPTION	CPushable::m_SaveData[] =
{
	DEFINE_FIELD( CPushable, m_maxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CPushable, m_soundTime, FIELD_TIME ),
	DEFINE_FIELD( CPushable, m_ignoreCorpses, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPushable, m_instantGibCorpses, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPushable, m_handleTinyCreatures, FIELD_SHORT ),
	DEFINE_FIELD( CPushable, m_toggleable, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPushable, m_sizeForGrapple, FIELD_SHORT ),
};

IMPLEMENT_SAVERESTORE( CPushable, CBreakable )

LINK_ENTITY_TO_CLASS( func_pushable, CPushable )

const NamedSoundScript CPushable::moveSoundScript = {
	CHAN_WEAPON,
	{"debris/pushbox1.wav", "debris/pushbox2.wav", "debris/pushbox3.wav"},
	0.5f, ATTN_NORM,
	"Pushable.Move"
};

void CPushable::Spawn()
{
	if( pev->spawnflags & SF_PUSH_BREAKABLE )
		CBreakable::Spawn();
	else
		Precache();

	pev->movetype = MOVETYPE_PUSHSTEP;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->friction > 399 )
		pev->friction = 399;

	m_maxSpeed = 400 - pev->friction;
	SetBits( pev->flags, FL_FLOAT );
	pev->friction = 0;
	
	pev->origin.z += 1;	// Pick up off of the floor
	UTIL_SetOrigin( pev, pev->origin );

	// Multiply by area of the box's cross-section (assume 1000 units^3 standard volume)
	pev->skin = (int)( ( pev->skin * ( pev->maxs.x - pev->mins.x ) * ( pev->maxs.y - pev->mins.y ) ) * 0.0005f );
	m_soundTime = 0;
}

void CPushable::Precache()
{
	RegisterAndPrecacheSoundScript(moveSoundScript);

	if( pev->spawnflags & SF_PUSH_BREAKABLE )
		CBreakable::Precache();
}

void CPushable::PreEntvarsKeyvalue( KeyValueData* pkvd )
{
	if (FStrEq(pkvd->szKeyName, "size"))
	{
		// just ignore the 'size' key-value. It used to be incorrectly set in fgd.
		pkvd->fHandled = true;
	}
	else
		CBreakable::PreEntvarsKeyvalue(pkvd);
}

void CPushable::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "buoyancy" ) )
	{
		pev->skin = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "ignore_corpses") )
	{
		m_ignoreCorpses = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "instant_gib_corpses") )
	{
		m_instantGibCorpses = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "handle_tiny_creatures") )
	{
		m_handleTinyCreatures = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if ( FStrEq(pkvd->szKeyName, "toggleable_push") )
	{
		m_toggleable = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "size_for_grapple" ) )
	{
		m_sizeForGrapple = (short)atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBreakable::KeyValue( pkvd );
}

// Pull the func_pushable
void CPushable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_toggleable && (!pCaller || !pCaller->IsPlayer()))
	{
		const bool state = !FBitSet(pev->spawnflags, SF_PUSHABLE_DISABLED);
		if (ShouldToggle(useType, state))
		{
			if (state)
			{
				SetBits(pev->spawnflags, SF_PUSHABLE_DISABLED);
				pev->solid = FBitSet(pev->spawnflags, SF_BREAK_NOT_SOLID) ? SOLID_NOT : SOLID_BSP;
			}
			else
			{
				ClearBits(pev->spawnflags, SF_PUSHABLE_DISABLED);
				pev->solid = SOLID_BBOX;
			}
			UTIL_SetOrigin(pev, pev->origin);
		}
		return;
	}

	if( !pActivator || !pActivator->IsPlayer() )
	{
		if( pev->spawnflags & SF_PUSH_BREAKABLE )
			this->CBreakable::Use( pActivator, pCaller, useType, value );
		return;
	}

	if (FBitSet(pev->spawnflags, SF_PUSHABLE_DISABLED))
		return;

	if( pActivator->pev->velocity != g_vecZero )
		Move( pActivator, 0 );
}

NODE_LINKENT CPushable::HandleLinkEnt(int afCapMask, bool nodeQueryStatic)
{
	if (nodeQueryStatic) {
		return NLE_ALLOW;
	} else {
		return NLE_PROHIBIT;
	}
}

void CPushable::Touch( CBaseEntity *pOther )
{
	if (FBitSet(pev->spawnflags, SF_PUSHABLE_DISABLED))
		return;

	if( FClassnameIs( pOther->pev, "worldspawn" ) )
		return;

	Move( pOther, 1 );
}

void CPushable::Move( CBaseEntity *pOther, int push )
{
	entvars_t* pevToucher = pOther->pev;
	int playerTouch = 0;

	// Is entity standing on this pushable ?
	if( FBitSet( pevToucher->flags,FL_ONGROUND ) && pevToucher->groundentity && VARS( pevToucher->groundentity ) == pev )
	{
		// Only push if floating
		if( pev->waterlevel > WL_NotInWater )
			pev->velocity.z += pevToucher->velocity.z * 0.1f;

		return;
	}

	const bool shouldInstaGib = (m_instantGibCorpses && pOther->IsCorpse()) || (g_modFeatures.ShouldCrushTinyCreatures(m_handleTinyCreatures) && pOther->IsTinyCreature());
	if (shouldInstaGib)
	{
		pOther->TakeDamage(pev, pev, DamageInfo(pOther->pev->health + 1, DMG_CRUSH).SetIgnoreTransform().SetGibPolicy(GIB_ALWAYS));
	}

	if( pOther->IsPlayer() )
	{
		if( pushablemode.value == -1 )
		{
			// Don't push unless the player is pushing forward and NOT use (pull)
			if( push && !( pevToucher->button & ( IN_FORWARD | IN_USE )))
				return;
		}
		// g-cont. fix pushable acceleration bug (now implemented as cvar)
		else if( pushablemode.value != 0 )
		{
			// Allow player push when moving right, left and back too
			if( push && !( pevToucher->button & ( IN_FORWARD | IN_MOVERIGHT | IN_MOVELEFT | IN_BACK )))
				return;
			// Require player walking back when applying '+use' on pushable
			if( !push && !( pevToucher->button & ( IN_BACK )))
				return;
		}
		// Don't push when +use pressed
		else if( push && ( pevToucher->button & ( IN_USE )))
			return;
		playerTouch = 1;
	}

	float factor;

	if( playerTouch )
	{
		if( !( pevToucher->flags & FL_ONGROUND ) )	// Don't push away from jumping/falling players unless in water
		{
			if( pev->waterlevel < WL_Feet )
				return;
			else 
				factor = 0.1f;
		}
		else
			factor = 1.0f;
	}
	else 
		factor = 0.25f;

	if( pushablemode.value != 0 )
	{
		pev->velocity.x += pevToucher->velocity.x * factor;
		pev->velocity.y += pevToucher->velocity.y * factor;
	}
	else
	{ 
		if( push )
		{
			pev->velocity.x += pevToucher->velocity.x * factor;
			pev->velocity.y += pevToucher->velocity.y * factor;
		}
		else
		{
			// fix for pushable acceleration
			if( sv_pushable_fixed_tick_fudge.value >= 0 )
				factor *= ( sv_pushable_fixed_tick_fudge.value * gpGlobals->frametime );

			if( fabs( pev->velocity.x ) < fabs( pevToucher->velocity.x - pevToucher->velocity.x * factor ))
				pev->velocity.x += pevToucher->velocity.x * factor;
			if( fabs( pev->velocity.y ) < fabs( pevToucher->velocity.y - pevToucher->velocity.y * factor ))
				pev->velocity.y += pevToucher->velocity.y * factor;
		}
	}

	float length = sqrt( pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y );
	if( ( push && pushablemode.value != 0 )
	    || pushablemode.value == 0 )
	{
		if( length > MaxSpeed())
		{
			pev->velocity.x = ( pev->velocity.x * MaxSpeed() / length );
			pev->velocity.y = ( pev->velocity.y * MaxSpeed() / length );
		}
	}

	if( playerTouch )
	{
		if( push || pushablemode.value != 0 )
		{
			pevToucher->velocity.x = pev->velocity.x;
			pevToucher->velocity.y = pev->velocity.y;
		}

		if( ( gpGlobals->time - m_soundTime ) > 0.7f )
		{
			m_soundTime = gpGlobals->time;
			const SoundScript* myMoveSoundScript = GetSoundScript(moveSoundScript.name);
			if (myMoveSoundScript && !myMoveSoundScript->waves.empty())
			{
				if( length > 0 && FBitSet( pev->flags, FL_ONGROUND ))
				{
					m_lastSound = RANDOM_LONG(0, myMoveSoundScript->waves.size()-1);
					EmitSoundScriptSelectedSample(myMoveSoundScript, m_lastSound);
				}
				else
					StopSoundScriptSelectedSample(myMoveSoundScript, m_lastSound);
			}
		}
	}
}

#if 0
void CPushable::StopSound()
{
	Vector dist = pev->oldorigin - pev->origin;
	if( dist.IsLengthLessThanOrEqual(0) )
		STOP_SOUND( ENT( pev ), CHAN_WEAPON, m_soundNames[m_lastSound] );
}
#endif

TakeDamageResult CPushable::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	if( pev->spawnflags & SF_PUSH_BREAKABLE )
		return CBreakable::TakeDamage( pevInflictor, pevAttacker, damageInfo );

	return TakeDamageResult();
}

int CPushable::DamageDecal(int bitsDamageType)
{
	if (FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE))
		return CBreakable::DamageDecal(bitsDamageType);

	return CBaseEntity::DamageDecal(bitsDamageType);
}

bool CPushable::IsDestroyableObstacle()
{
	return FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE) && CBreakable::IsDestroyableObstacle();
}

#define FUNC_BREAKABLE_REPEATABLE 1

class CFuncBreakableEffect : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* pkvd) override;
	int ObjectCaps() override { return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ); }

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	Materials m_Material;
	string_t m_iszGibModel;
	int m_iGibs;
	string_t m_position;
	int m_idShard;
};

void CFuncBreakableEffect::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "material" ) )
	{
		int i = atoi( pkvd->szValue );

		// 0:glass, 1:metal, 2:flesh, 3:wood

		if( ( i < 0 ) || ( i >= matLastMaterial ) )
			m_Material = matWood;
		else
			m_Material = (Materials)i;

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "gibmodel" ) )
	{
		m_iszGibModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iGibs") )
	{
		m_iGibs = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "position" ) )
	{
		m_position = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( func_breakable_effect, CFuncBreakableEffect )

TYPEDESCRIPTION CFuncBreakableEffect::m_SaveData[] =
{
	DEFINE_FIELD( CFuncBreakableEffect, m_Material, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncBreakableEffect, m_iszGibModel, FIELD_STRING ),
	DEFINE_FIELD( CFuncBreakableEffect, m_iGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncBreakableEffect, m_position, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncBreakableEffect, CBaseEntity )

void CFuncBreakableEffect::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );//set size and link into world.
	pev->effects |= EF_NODRAW;
}

void CFuncBreakableEffect::Precache()
{
	PrecacheMaterialBustSounds(this, m_Material);

	const char *pGibName = NULL;
	if( m_iszGibModel )
		pGibName = STRING( m_iszGibModel );
	else
		pGibName = DefaultMaterialGibModel(m_Material);

	m_idShard = PRECACHE_MODEL( pGibName );
}

void CFuncBreakableEffect::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int pitch = 95 + RANDOM_LONG( 0, 29 );

	if( pitch > 97 && pitch < 103 )
		pitch = 100;

	float fvol = RANDOM_FLOAT( 0.85f, 1.0 );

	if( fvol > 1.0f )
		fvol = 1.0f;

	char cFlag = PlayBreakableBustSound(this, m_Material, fvol, pitch);
	cFlag |= ExtraBreakableFlags(pev->spawnflags);

	Vector vecOrigin = pev->origin;
	if (!FStringNull(m_position))
	{
		if (!TryCalcLocus_Position(this, pActivator, STRING(m_position), vecOrigin))
			return;
	}
	Vector vecSpot = vecOrigin + ( pev->mins + pev->maxs ) * 0.5f;

	if (m_iGibs >= 0)
	{
		CBreakable::BreakModel(vecSpot, pev->size, g_vecZero, m_idShard, m_iGibs, cFlag);
	}

	if (!FBitSet(pev->spawnflags, FUNC_BREAKABLE_REPEATABLE))
		UTIL_Remove(this);
}
