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

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "animation.h"

extern int gmsgItemPickup;

class CWorldItem : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd ); 
	void Spawn( void );
	int m_iType;
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem )

void CWorldItem::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch( m_iType ) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if( !pEntity )
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY( edict() );
}

class CItemRandomProxy : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT SpawnItemThink();
	void SpawnItem();
};

LINK_ENTITY_TO_CLASS( item_random_proxy, CItemRandomProxy )

void CItemRandomProxy::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "template"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CItemRandomProxy::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	if (FStringNull(pev->message))
	{
		ALERT(at_error, "No template for item_random_proxy\n");
		REMOVE_ENTITY( edict() );
		return;
	}
	if (FStringNull(pev->targetname)) {
		SetThink(&CItemRandomProxy::SpawnItemThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CItemRandomProxy::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SpawnItem();
	UTIL_Remove(this);
}

void CItemRandomProxy::SpawnItemThink()
{
	SpawnItem();
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CItemRandomProxy::SpawnItem()
{
	CBaseEntity* foundEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
	if ( foundEntity && FClassnameIs(foundEntity->pev, "info_item_random"))
	{
		foundEntity->Use(this, this, USE_TOGGLE, 0.0f);
	}
	else
	{
		ALERT(at_error, "Random item template %s for item_random_proxy not found or not info_item_random\n", STRING(pev->message));
	}
}

#define ITEM_RANDOM_MAX_COUNT 16

#define SF_ITEM_RANDOM_PREDETERMINED 64

class CItemRandom : public CPointEntity
{
public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	void KeyValue( KeyValueData *pkvd );
	void Spawn();
	void Precache();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void SpawnItem(const Vector& origin, const Vector& angles, string_t target);

	string_t m_itemNames[ITEM_RANDOM_MAX_COUNT];
	float m_itemProbabilities[ITEM_RANDOM_MAX_COUNT];
	int m_itemCount;
	unsigned int m_randomSeed;

	static bool IsAppropriateItemName(const char* name);
	static bool IsNullItem(const char* name);

	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_probabilitySum;
};

LINK_ENTITY_TO_CLASS( item_random, CItemRandom )

TYPEDESCRIPTION CItemRandom::m_SaveData[] =
{
	DEFINE_ARRAY( CItemRandom, m_itemNames, FIELD_STRING, ITEM_RANDOM_MAX_COUNT ),
	DEFINE_ARRAY( CItemRandom, m_itemProbabilities, FIELD_FLOAT, ITEM_RANDOM_MAX_COUNT ),
	DEFINE_FIELD( CItemRandom, m_itemCount, FIELD_INTEGER ),
	DEFINE_FIELD( CItemRandom, m_randomSeed, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CItemRandom, CBaseEntity )

bool CItemRandom::IsAppropriateItemName(const char *name)
{
	return IsNullItem(name) || (strncmp(name, "ammo_", 5) == 0) || (strncmp(name, "item_", 5) == 0) || (strncmp(name, "weapon_", 7) == 0);
}

bool CItemRandom::IsNullItem(const char *name)
{
	return FStrEq(name, "info_null") || FStrEq(name, "null");
}

void CItemRandom::KeyValue(KeyValueData *pkvd)
{
	if (IsAppropriateItemName(pkvd->szKeyName))
	{
		const float probability = atof(pkvd->szValue);
		if (m_itemCount < ITEM_RANDOM_MAX_COUNT && probability > 0)
		{
			m_itemNames[m_itemCount] = ALLOC_STRING(pkvd->szKeyName);
			m_itemProbabilities[m_itemCount] = probability;
			m_itemCount++;
		}
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue(pkvd);
	}
}

void CItemRandom::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if (FBitSet(pev->spawnflags, SF_ITEM_RANDOM_PREDETERMINED))
		m_randomSeed = RANDOM_LONG(0, (1<<15));

	if (FStringNull(pev->targetname))
	{
		SpawnItem(pev->origin, pev->angles, pev->target);
		REMOVE_ENTITY( edict() );
	}
}

void CItemRandom::Precache()
{
	m_probabilitySum = 0;
	for (int i=0; i<m_itemCount; ++i)
	{
		m_probabilitySum += m_itemProbabilities[i];
	}
}

void CItemRandom::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SpawnItem(pev->origin, pev->angles, pev->target);
	UTIL_Remove(this);
}

void CItemRandom::SpawnItem(const Vector &origin, const Vector &angles, string_t target)
{
	float choice;
	if (FBitSet(pev->spawnflags, SF_ITEM_RANDOM_PREDETERMINED))
	{
		choice = UTIL_SharedRandomFloat(m_randomSeed, 0, m_probabilitySum);
		m_randomSeed = UTIL_SharedRandomLong(m_randomSeed, 0, 1<<15);
	}
	else
	{
		choice = RANDOM_FLOAT(0, m_probabilitySum);
	}
	float sum = 0;
	for (int i=0; i<m_itemCount; ++i)
	{
		sum += m_itemProbabilities[i];
		if (choice <= sum)
		{
			if (IsNullItem(STRING(m_itemNames[i])))
				break;

			CBaseEntity *pEntity = CBaseEntity::Create( STRING(m_itemNames[i]), origin, angles );
			if (!pEntity)
				ALERT(at_error, "Could not spawn random item %s\n", STRING(m_itemNames[i]));
			else
				pEntity->pev->target = target;
			break;
		}
	}
}

class CInfoItemRandom : public CItemRandom
{
public:
	void Spawn();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS( info_item_random, CInfoItemRandom )

void CInfoItemRandom::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

void CInfoItemRandom::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Was called by SOLID_BSP entity, e.g. func_breakable
	const char* model = FStringNull(pActivator->pev->model) ? NULL : STRING(pActivator->pev->model);
	if (model && *model == '*')
	{
		SpawnItem(VecBModelOrigin( pActivator->pev ), pActivator->pev->angles, iStringNull);
	}
	else
	{
		SpawnItem(pActivator->pev->origin, pActivator->pev->angles, pActivator->pev->target);
	}
}

//=========

int CPickup::ObjectCaps()
{
	if (IsPickableByUse() && !(pev->effects & EF_NODRAW)) {
		return CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE;
	} else {
		return CBaseEntity::ObjectCaps();
	}
}

void CPickup::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -16, -16, 0 );
	pev->absmax = pev->origin + Vector( 16, 16, 16 );
}

bool CPickup::IsPickableByTouch()
{
	return !FBitSet(pev->spawnflags, SF_ITEM_USE_ONLY) &&
			(FBitSet(pev->spawnflags, SF_ITEM_TOUCH_ONLY) || !NeedUseToTake());
}

bool CPickup::IsPickableByUse()
{
	return !FBitSet(pev->spawnflags, SF_ITEM_TOUCH_ONLY) &&
			(FBitSet(pev->spawnflags, SF_ITEM_USE_ONLY) || NeedUseToTake());
}

void CPickup::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if( pev->flags & FL_ONGROUND )
	{
		pev->solid = SOLID_TRIGGER;
		UTIL_SetOrigin( pev, pev->origin );
		ResetThink();
	}
}

CBaseEntity* CPickup::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, MyRespawnSpot() );// blip to whereever you should respawn.

	SetThink( &CPickup::Materialize );
	pev->nextthink = MyRespawnTime();
	return this;
}

void CPickup::Materialize( void )
{
	if( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	OnMaterialize();
}

extern int gEvilImpulse101;

void CItem::Spawn( void )
{
	if (FBitSet(pev->spawnflags, SF_ITEM_NOFALL))
		pev->movetype = MOVETYPE_NONE;
	else
		pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	SetTouch( &CItem::ItemTouch );

	if (pev->movetype != MOVETYPE_NONE)
	{
#if FEATURE_ITEM_INSTANT_DROP
		if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
		{
			ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z);
			UTIL_Remove( this );
			return;
		}
#endif
	}
}

void CItem::ItemTouch( CBaseEntity *pOther )
{
	if (IsPickableByTouch()) {
		TouchOrUse(pOther);
	}
}

void CItem::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsPickableByUse() && !(pev->effects & EF_NODRAW)) {
		TouchOrUse(pCaller);
	}
}

void CItem::TouchOrUse(CBaseEntity *pOther)
{
	// if it's not a player, ignore
	if( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if( MyTouch( pPlayer ) )
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );

		// player grabbed the item.
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if( gEvilImpulse101 )
	{
		UTIL_Remove( this );
	}
}

Vector CItem::MyRespawnSpot()
{
	return g_pGameRules->VecItemRespawnSpot( this );
}

float CItem::MyRespawnTime()
{
	return g_pGameRules->FlItemRespawnTime( this );
}

void CItem::OnMaterialize()
{
	SetTouch( &CItem::ItemTouch );
	SetThink( NULL );
}

void CItem::SetMyModel(const char *model)
{
	if (FStringNull(pev->model)) {
		SET_MODEL( ENT( pev ), model );
	} else {
		SET_MODEL( ENT( pev ), STRING(pev->model) );
	}
}

void CItem::PrecacheMyModel(const char *model)
{
	if (FStringNull(pev->model)) {
		PRECACHE_MODEL( model );
	} else {
		PRECACHE_MODEL( STRING( pev->model ) );
	}
}

#define SF_SUIT_SHORTLOGON		0x0001
#define SF_SUIT_NOLOGON		0x0002
#if FEATURE_FLASHLIGHT_ITEM && !FEATURE_SUIT_FLASHLIGHT
#define SF_SUIT_FLASHLIGHT 0x0004
#endif

class CItemSuit : public CItem
{
public:
	void Spawn( void )
	{
		Precache();
		SetMyModel( "models/w_suit.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PrecacheMyModel( "models/w_suit.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->HasSuit() )
			return FALSE;

		if ( pev->spawnflags & SF_SUIT_NOLOGON )
		{
			// pass
		}
		else if( pev->spawnflags & SF_SUIT_SHORTLOGON )
		{
			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A0" );		// short version of suit logon,
		}
		else
		{
			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_AAx" );	// long version of suit logon
		}

		pPlayer->m_iItemsBits |= PLAYER_ITEM_SUIT;
#if FEATURE_FLASHLIGHT_ITEM && !FEATURE_SUIT_FLASHLIGHT
		if (FBitSet(pev->spawnflags, SF_SUIT_FLASHLIGHT))
		{
			pPlayer->m_iItemsBits |= PLAYER_ITEM_FLASHLIGHT;
		}
#endif
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_suit, CItemSuit )

class CItemBattery : public CItem
{
public:
	void Spawn( void )
	{
		Precache();
		SetMyModel( DefaultModel() );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PrecacheMyModel( DefaultModel() );
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if( ( pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY ) && pPlayer->HasSuit() )
		{
			pPlayer->pev->armorvalue += pev->health > 0 ? pev->health : DefaultCapacity();
			pPlayer->pev->armorvalue = Q_min( pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY );

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			if (ShouldSetSuitUpdate())
			{
				int pct;
				char szcharge[64];
				// Suit reports new power level
				// For some reason this wasn't working in release build -- round it.
				pct = (int)( (float)( pPlayer->pev->armorvalue * 100.0f ) * ( 1.0f / MAX_NORMAL_BATTERY ) + 0.5f );
				pct = ( pct / 5 );
				if( pct > 0 )
					pct--;

				sprintf( szcharge,"!HEV_%1dP", pct );

				//EMIT_SOUND_SUIT( ENT( pev ), szcharge );
				pPlayer->SetSuitUpdate( szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			}

			return TRUE;
		}
		return FALSE;
	}
protected:
	virtual const char* DefaultModel() { return "models/w_battery.mdl"; }
	virtual bool ShouldSetSuitUpdate() { return true; }
	virtual int DefaultCapacity() { return gSkillData.batteryCapacity; }
};

LINK_ENTITY_TO_CLASS( item_battery, CItemBattery )

class CItemArmorVest : public CItemBattery
{
protected:
	const char* DefaultModel() { return "models/barney_vest.mdl"; }
	bool ShouldSetSuitUpdate() { return false; }
	int DefaultCapacity() { return 60; }
};

LINK_ENTITY_TO_CLASS( item_armorvest, CItemArmorVest )

class CItemHelmet : public CItemBattery
{
protected:
	const char* DefaultModel() { return "models/barney_helmet.mdl"; }
	bool ShouldSetSuitUpdate() { return false; }
	int DefaultCapacity() { return 40; }
};

LINK_ENTITY_TO_CLASS( item_helmet, CItemHelmet )

class CItemAntidote : public CItem
{
	void Spawn( void )
	{
		Precache();
		SetMyModel( "models/w_antidote.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PrecacheMyModel( "models/w_antidote.mdl" );
		if (!FStringNull(pev->noise))
			PRECACHE_SOUND( STRING(pev->noise) );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}
		pPlayer->SetSuitUpdate( "!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN );

		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;

		if (!FStringNull(pev->noise))
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, STRING(pev->noise), 1, ATTN_NORM );
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING( pev->classname ) );
		MESSAGE_END();

		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_antidote, CItemAntidote )

class CItemSecurity : public CItem
{
	void Spawn( void )
	{
		Precache();
		SetMyModel( "models/w_security.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PrecacheMyModel( "models/w_security.mdl" );
		if (!FStringNull(pev->noise))
			PRECACHE_SOUND( STRING(pev->noise) );
	}
	void KeyValue(KeyValueData* pkvd)
	{
		if (FStrEq(pkvd->szKeyName, "hudname"))
		{
			pev->netname = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CItem::KeyValue(pkvd);
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;

		if (!FStringNull(pev->noise))
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, STRING(pev->noise), 1, ATTN_NORM );
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( FStringNull(pev->netname) ? STRING( pev->classname ) : STRING(pev->netname) );
		MESSAGE_END();
		if (!FStringNull(pev->message))
			UTIL_ShowMessage( STRING( pev->message ), pPlayer );

		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_security, CItemSecurity )

class CItemLongJump : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SetMyModel( "models/w_longjump.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PrecacheMyModel( "models/w_longjump.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if( pPlayer->HasSuit() )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump )

#if FEATURE_FLASHLIGHT_ITEM
class CItemFlashlight : public CItem
{
	void Spawn( void )
	{
		Precache( );
		SetMyModel("models/w_flashlight.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PrecacheMyModel ("models/w_flashlight.mdl");
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->HasFlashlight() )
			return FALSE;
		pPlayer->m_iItemsBits |= PLAYER_ITEM_FLASHLIGHT;
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
		MESSAGE_END();
		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
		return TRUE;
	}
};
LINK_ENTITY_TO_CLASS(item_flashlight, CItemFlashlight)
#endif

//=========================================================
// Generic item
//=========================================================
#define SF_ITEM_GENERIC_DROP_TO_FLOOR 1
#define SF_ITEM_GENERIC_DONT_TRANSIT 2
#define SF_ITEM_GENERIC_APPLY_GRAVITY 4

class CItemGeneric : public CBaseAnimating
{
public:
	int		Save(CSave &save);
	int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData* pkvd);
	int	ObjectCaps(void);

	void SetObjectCollisionBox( void );

	void EXPORT StartupThink(void);
	void EXPORT SequenceThink(void);

	string_t m_iszSequenceName;
};

LINK_ENTITY_TO_CLASS(item_generic, CItemGeneric)

TYPEDESCRIPTION CItemGeneric::m_SaveData[] =
{
	DEFINE_FIELD(CItemGeneric, m_iszSequenceName, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE(CItemGeneric, CBaseAnimating)

void CItemGeneric::Spawn(void)
{
	Precache();
	if (FStringNull(pev->model))
	{
		ALERT(at_console, "Spawning item_generic without model!\n");
	}
	else
	{
		SET_MODEL(ENT(pev), STRING(pev->model));
	}

	if (FBitSet(pev->spawnflags, SF_ITEM_GENERIC_APPLY_GRAVITY))
	{
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_TOSS;
	}
	else
	{
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->takedamage	 = DAMAGE_NO;

	// Call startup sequence to look for a sequence to play.
	if (!FStringNull(m_iszSequenceName))
	{
		SetThink(&CItemGeneric::StartupThink);
	}

	pev->nextthink = gpGlobals->time + 0.1f;

	if (FBitSet(pev->spawnflags, SF_ITEM_GENERIC_DROP_TO_FLOOR))
	{
		if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
		{
			ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
			UTIL_Remove( this );
		}
	}
}

void CItemGeneric::Precache(void)
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL(STRING(pev->model));
}

void CItemGeneric::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		m_iszSequenceName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

int CItemGeneric::ObjectCaps()
{
	if (FBitSet(pev->spawnflags, SF_ITEM_GENERIC_DONT_TRANSIT))
		return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	else
		return CBaseEntity::ObjectCaps();
}

void CItemGeneric::SetObjectCollisionBox()
{
	if (FBitSet(pev->spawnflags, SF_ITEM_GENERIC_APPLY_GRAVITY))
	{
		pev->absmin = pev->origin + Vector( -16, -16, 0 );
		pev->absmax = pev->origin + Vector( 16, 16, 16 );
	}
	else
	{
		CBaseAnimating::SetObjectCollisionBox();
	}
}

void CItemGeneric::StartupThink(void)
{
	pev->sequence = LookupSequence(STRING(m_iszSequenceName));
	if (pev->sequence == ACTIVITY_NOT_AVAILABLE)
	{
		ALERT(at_console, "Can't find a sequence \"%s\" in model \"%s\"\n", STRING(m_iszSequenceName), STRING(pev->model));
		pev->sequence = 0;
	}
	pev->frame = 0;
	ResetSequenceInfo();
	SetThink(&CItemGeneric::SequenceThink);
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CItemGeneric::SequenceThink(void)
{
	// Set next think time.
	pev->nextthink = gpGlobals->time + 0.1f;

	// Advance frames and dispatch events.
	StudioFrameAdvance();
	DispatchAnimEvents();

	// Restart sequence
	if (m_fSequenceFinished)
	{
		pev->frame = 0;
		ResetSequenceInfo();

		if (!m_fSequenceLoops)
		{
			SetThink(NULL);
		}
	}
}

class CEyeScanner : public CBaseAnimating
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn();
	void Precache(void);
	void PlayBeep();
	void WaitForSequenceEnd();
	void Think();
	int ObjectCaps( void ) { return CBaseAnimating::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE; }
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void SetActivity(Activity NewActivity);

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	const char* GrantedSound() {
		return pev->noise ? STRING(pev->noise) : "buttons/blip2.wav";
	}
	const char* DeniedSound() {
		return pev->noise1 ? STRING(pev->noise1) : "buttons/button11.wav";
	}
	const char* BeepSound() {
		return pev->noise2 ? STRING(pev->noise2) : "buttons/blip1.wav";
	}

	string_t m_unlockedTarget;
	string_t m_lockedTarget;
	string_t m_unlockerName;
	string_t m_grantedSentence;
	string_t m_deniedSentence;
	Activity m_Activity;
	float m_fireTime;
	float m_playSentenceTime;
	float m_sentenceDelay;
	BOOL m_willUnlock;
	BOOL m_wasUnlocked;
};

TYPEDESCRIPTION CEyeScanner::m_SaveData[] =
{
	DEFINE_FIELD( CEyeScanner, m_unlockedTarget, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_lockedTarget, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_unlockerName, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_grantedSentence, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_deniedSentence, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CEyeScanner, m_fireTime, FIELD_TIME ),
	DEFINE_FIELD( CEyeScanner, m_playSentenceTime, FIELD_TIME ),
	DEFINE_FIELD( CEyeScanner, m_sentenceDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CEyeScanner, m_willUnlock, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEyeScanner, m_wasUnlocked, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CEyeScanner, CBaseAnimating )

LINK_ENTITY_TO_CLASS( item_eyescanner, CEyeScanner )

void CEyeScanner::SetActivity( Activity NewActivity )
{
	int iSequence;

	iSequence = LookupActivity( NewActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if( !( m_Activity == ACT_WALK || m_Activity == ACT_RUN ) || !( NewActivity == ACT_WALK || NewActivity == ACT_RUN ) )
				pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
	}
	else
	{
		ALERT( at_aiconsole, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
		pev->sequence = 0;
	}

	m_Activity = NewActivity;
}

void CEyeScanner::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "unlocked_target"))
	{
		m_unlockedTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_target"))
	{
		m_lockedTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlockersname"))
	{
		m_unlockerName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "granted_sentence"))
	{
		m_grantedSentence = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "denied_sentence"))
	{
		m_deniedSentence = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sentence_delay"))
	{
		m_sentenceDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "reset_delay")) // Dunno if it affects anything in PC version of Decay
	{
		//m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue( pkvd );
}

void CEyeScanner::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->health = 1;
	pev->weapons = 0;
	m_willUnlock = false;

	SET_MODEL(ENT(pev), "models/EYE_SCANNER.mdl");
	const float yCos = fabs(cos(pev->angles.y * M_PI_F / 180.0f));
	const float ySin = fabs(sin(pev->angles.y * M_PI_F / 180.0f));
	UTIL_SetSize(pev, Vector(-10-ySin*6, -10-yCos*6, 32), Vector(10+ySin*6, 10+yCos*6, 72));
	UTIL_SetOrigin(pev, pev->origin);
	SetActivity(ACT_CROUCHIDLE);
	ResetSequenceInfo();
}

void CEyeScanner::Precache()
{
	PRECACHE_MODEL("models/EYE_SCANNER.mdl");
	PRECACHE_SOUND(GrantedSound());
	PRECACHE_SOUND(DeniedSound());
	PRECACHE_SOUND(BeepSound());

	SetActivity( m_Activity );
}

void CEyeScanner::PlayBeep()
{
	pev->skin = pev->weapons % 3 + 1;
	pev->weapons++;
	EMIT_SOUND( ENT(pev), CHAN_BODY, BeepSound(), 1, ATTN_NORM );
}

void CEyeScanner::WaitForSequenceEnd()
{
	if (m_fSequenceFinished) {
		if (m_Activity == ACT_STAND) {
			SetActivity(ACT_IDLE);
		} else if (m_Activity == ACT_CROUCH) {
			SetActivity(ACT_CROUCHIDLE);
		}
	} else if (m_Activity != ACT_IDLE && m_Activity != ACT_CROUCHIDLE) {
		StudioFrameAdvance();
	}
}

void CEyeScanner::Think()
{
	WaitForSequenceEnd();
	if (m_Activity == ACT_IDLE)
	{
		PlayBeep();
	}
	if (m_fireTime != 0 && m_fireTime <= gpGlobals->time)
	{
		m_wasUnlocked = m_willUnlock;
		if (m_willUnlock) {
			EMIT_SOUND( ENT(pev), CHAN_ITEM, GrantedSound(), 1.0f, ATTN_NORM );
			DelayedUse( m_flDelay, this, this, USE_TOGGLE, m_unlockedTarget );
		} else {
			EMIT_SOUND( ENT(pev), CHAN_ITEM, DeniedSound(), 1.0f, ATTN_NORM );
			DelayedUse( m_flDelay, this, this, USE_TOGGLE, m_lockedTarget );
		}
		m_playSentenceTime = gpGlobals->time + m_sentenceDelay;
		m_willUnlock = false;
		m_fireTime = 0;
		pev->skin = 0;
		pev->weapons = 0;
		if (m_Activity == ACT_IDLE)
			SetActivity(ACT_CROUCH);
	}
	if (m_playSentenceTime != 0 && m_playSentenceTime <= gpGlobals->time) {
		if (m_wasUnlocked) {
			if (!FStringNull(m_grantedSentence)) {
				EMIT_SOUND( ENT(pev), CHAN_VOICE, STRING(m_grantedSentence), 1.0f, ATTN_NORM );
			}
		} else {
			if (!FStringNull(m_deniedSentence)) {
				EMIT_SOUND( ENT(pev), CHAN_VOICE, STRING(m_deniedSentence), 1.0f, ATTN_NORM );
			}
		}
		m_playSentenceTime = 0;
	}
	pev->nextthink = gpGlobals->time + 0.11;
}

#define EYESCANNER_BASE_FIRE_DELAY 3.0f

void CEyeScanner::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// No repeated use by player
	if (pCaller->IsPlayer() && m_Activity != ACT_CROUCHIDLE)
	{
		return;
	}

	pActivator = pActivator ? pActivator : pCaller;

	if (!m_willUnlock)
	{
		if (FStringNull(m_unlockerName))
		{
			if (!pActivator->IsPlayer())
			{
				m_willUnlock = true;
				m_fireTime = gpGlobals->time + EYESCANNER_BASE_FIRE_DELAY;
			}
		}
		else if ((!FStringNull(pActivator->pev->targetname) && FStrEq(STRING(m_unlockerName), STRING(pActivator->pev->targetname)))
				 || FClassnameIs(pActivator->pev, STRING(m_unlockerName)))
		{
			m_willUnlock = true;
			m_fireTime = gpGlobals->time + EYESCANNER_BASE_FIRE_DELAY;
		}
	}

	if (m_Activity == ACT_CROUCHIDLE || m_Activity == ACT_CROUCH) {
		m_fireTime = gpGlobals->time + EYESCANNER_BASE_FIRE_DELAY;
		SetActivity( ACT_STAND );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

int CEyeScanner::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	return 0;
}
