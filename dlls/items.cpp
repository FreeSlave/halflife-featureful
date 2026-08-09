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
#include "player.h"
#include "game.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "animation.h"
#include "common_soundscripts.h"
#include "inventory.h"

class CWorldItem : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	int m_iType;
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem )

void CWorldItem::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn()
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
	void KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void EXPORT SpawnItemThink();
	void SpawnItem();
};

LINK_ENTITY_TO_CLASS( item_random_proxy, CItemRandomProxy )

void CItemRandomProxy::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "template"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CItemRandomProxy::Spawn()
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
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
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
	return IsNullItem(name) || IsProbablyPickupClassname(name);
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
		pkvd->fHandled = true;
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
	void Spawn() override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
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
	if (pActivator->IsBrushModel())
	{
		SpawnItem(VecBModelOrigin( pActivator->pev ), pActivator->pev->angles, iStringNull);
	}
	else
	{
		SpawnItem(pActivator->pev->origin, pActivator->pev->angles, pActivator->pev->target);
	}
}

//=========

void CPickup::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

int CPickup::ObjectCaps()
{
	if (IsPickableByUse() && !(pev->effects & EF_NODRAW)) {
		return CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE;
	} else {
		return CBaseEntity::ObjectCaps();
	}
}

void CPickup::SetObjectCollisionBox()
{
	if (FBitSet(pev->spawnflags, SF_ITEM_FIX_PHYSICS))
	{
		pev->absmin = pev->origin + Vector( -16, -16, 0 );
		pev->absmax = pev->origin + Vector( 16, 16, 16 );
	}
	else
	{
		CBaseDelay::SetObjectCollisionBox();
	}
}

bool CPickup::IsPickableByTouch()
{
	return !FBitSet(pev->spawnflags, SF_ITEM_USE_ONLY) &&
			(FBitSet(pev->spawnflags, SF_ITEM_TOUCH_ONLY) || ItemsPickableByTouch());
}

bool CPickup::IsPickableByUse()
{
	return !FBitSet(pev->spawnflags, SF_ITEM_TOUCH_ONLY) &&
			(FBitSet(pev->spawnflags, SF_ITEM_USE_ONLY) || ItemsPickableByUse());
}

void CPickup::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if( (pev->flags & FL_ONGROUND) || pev->groundentity != NULL )
	{
		pev->solid = SOLID_TRIGGER;
		UTIL_SetOrigin( pev, pev->origin );
		SetThink( NULL );
	}
}

CBaseEntity* CPickup::Respawn()
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, MyRespawnSpot() );// blip to whereever you should respawn.

	SetThink( &CPickup::Materialize );
	pev->nextthink = MyRespawnTime();
	return this;
}

void CPickup::Materialize()
{
	if( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EmitSoundScript(Items::materializeSoundScript);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	OnMaterialize();
}

bool CPickup::IsLockedByMaster()
{
	return m_sMaster && !UTIL_IsMasterTriggered(m_sMaster, nullptr);
}

bool CPickup::IsUsefulToDisplayHint(CBaseEntity* pPlayer)
{
	if (pPlayer->IsPlayer())
	{
		CBasePlayer* p = (CBasePlayer*)pPlayer;
		return p->CanHaveItem(this);
	}
	return false;
}

TYPEDESCRIPTION CPickup::m_SaveData[] =
{
	DEFINE_FIELD(CPickup, m_sMaster, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE(CPickup, CBaseDelay)

extern bool gEvilImpulse101;

void CItem::Spawn()
{
	if (FBitSet(pev->spawnflags, SF_ITEM_NOFALL))
		pev->movetype = MOVETYPE_NONE;
	else
	{
		if (pev->movetype < 0)
			pev->movetype = MOVETYPE_NONE;
		else if (pev->movetype == 0)
			pev->movetype = MOVETYPE_TOSS;
	}
	pev->solid = SOLID_TRIGGER;

	bool instantDrop = g_modFeatures.items_instant_drop;

	if (FBitSet(pev->spawnflags, SF_ITEM_NO_INSTANT_DROP))
		instantDrop = false;

	const bool comesFromBreakable = pev->owner != NULL;
	if (!comesFromBreakable && ItemsPhysicsFix() == 2)
	{
		pev->solid = SOLID_BBOX;
		SetThink( &CPickup::FallThink );
		pev->nextthink = gpGlobals->time + 0.1f;
		SetBits(pev->spawnflags, SF_ITEM_FIX_PHYSICS);

		instantDrop = false;
	}
	if (ItemsPhysicsFix() == 3)
	{
		SetBits(pev->spawnflags, SF_ITEM_FIX_PHYSICS);
	}

	UTIL_SetOrigin( pev, pev->origin );
	if (FBitSet(pev->spawnflags, SF_ITEM_FIX_PHYSICS))
		UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	else
		UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CItem::ItemTouch );

	if (pev->movetype == MOVETYPE_TOSS)
	{
		if (instantDrop)
		{
			if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
			{
				ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z);
				UTIL_Remove( this );
				return;
			}
		}
	}
}

void CItem::ItemTouch( CBaseEntity *pOther )
{
	if (IsPickableByTouch()) {
		if (FBitSet(pev->spawnflags, SF_ITEM_WAIT_FOR_FALL) && !FBitSet(pev->flags, FL_ONGROUND))
		{
			return;
		}
		ClearBits(pev->spawnflags, SF_ITEM_WAIT_FOR_FALL);
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

	if (pPlayer->pev->deadflag != DEAD_NO)
		return;

	// ok, a player is touching this item, but can he have it?
	if (!pPlayer->CanHaveItem(this) || !g_pGameRules->CanHaveItem( pPlayer, this ))
	{
		// no? Ignore the touch.
		return;
	}

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if (MyTouch(pPlayer))
	{
		SUB_UseTargets( pOther );
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

void CItem::NotifyPickup(CBasePlayer* pPlayer, string_t defaultPickup)
{
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate)
	{
		const char* hudSprite = entTemplate->GetPickupHudSprite();
		if (hudSprite)
		{
			pPlayer->NotifyPickup(hudSprite);
			return;
		}
	}
	if (!FStringNull(defaultPickup))
	{
		pPlayer->NotifyPickup(STRING(defaultPickup));
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

void CItem::PrepareAsAmmoEnt(int amount)
{
	pev->spawnflags |= SF_ITEM_NO_INSTANT_DROP;
}

void CItem::DropAsAmmoEnt(int amount)
{
	pev->spawnflags |= SF_ITEM_WAIT_FOR_FALL;
}

class CItemSuit : public CItem
{
public:
	void Spawn() override
	{
		Precache();
		SetMyModel( "models/w_suit.mdl" );
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel( "models/w_suit.mdl" );
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		if( pPlayer->HasSuit() )
			return false;

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

		pPlayer->SetSuitAndDefaultLight();
		if (FBitSet(pev->spawnflags, SF_SUIT_FLASHLIGHT))
			pPlayer->SetFlashlight();

		return true;
	}
};

LINK_ENTITY_TO_CLASS( item_suit, CItemSuit )

class CItemBattery : public CItem
{
public:
	static constexpr const char* pickupSoundScript = "Battery.Pickup";

	void Spawn() override
	{
		Precache();
		SetMyModel( DefaultModel() );
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel( DefaultModel() );
		RegisterAndPrecacheSoundScript(pickupSoundScript, Items::pickupSoundScript);
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		if( ( pPlayer->pev->armorvalue < pPlayer->MaxArmor() ) && pPlayer->HasSuit() )
		{
			pPlayer->TakeArmor(this, pev->health > 0 ? pev->health : DefaultCapacity());

			pPlayer->EmitSoundScript(GetSoundScript(pickupSoundScript));

			NotifyPickup(pPlayer, pev->classname);

			if (ShouldSetSuitUpdate())
			{
				int pct;
				char szcharge[64];
				// Suit reports new power level
				// For some reason this wasn't working in release build -- round it.
				pct = (int)( (float)( pPlayer->pev->armorvalue * 100.0f ) * ( 1.0f / 100 ) + 0.5f );
				pct = ( pct / 5 );
				if( pct > 0 )
					pct--;

				sprintf( szcharge,"!HEV_%1dP", pct );

				//EMIT_SOUND_SUIT( ENT( pev ), szcharge );
				pPlayer->SetSuitUpdate( szcharge, SUIT_NEXT_IN_30SEC);
			}

			return true;
		}
		return false;
	}
protected:
	virtual const char* DefaultModel() { return "models/w_battery.mdl"; }
	virtual bool ShouldSetSuitUpdate() { return true; }
	virtual int DefaultCapacity() { return GetSkillValue("battery"); }
};

LINK_ENTITY_TO_CLASS( item_battery, CItemBattery )

class CItemArmorVest : public CItemBattery
{
protected:
	const char* DefaultModel() override { return "models/barney_vest.mdl"; }
	bool ShouldSetSuitUpdate() override { return false; }
	int DefaultCapacity() override { return 60; }
};

LINK_ENTITY_TO_CLASS( item_armorvest, CItemArmorVest )

class CItemHelmet : public CItemBattery
{
protected:
	const char* DefaultModel() override { return "models/barney_helmet.mdl"; }
	bool ShouldSetSuitUpdate() override { return false; }
	int DefaultCapacity() override { return 40; }
};

LINK_ENTITY_TO_CLASS( item_helmet, CItemHelmet )

class CItemAntidote : public CItem
{
public:
	void Spawn() override
	{
		Precache();
		SetMyModel( "models/w_antidote.mdl" );
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel( "models/w_antidote.mdl" );
		if (!FStringNull(pev->noise))
			PRECACHE_SOUND( STRING(pev->noise) );
		else
			RegisterAndPrecacheSoundScript(pickupSoundScript);
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		const int maxCount = g_InventorySpec.GetAntidoteSpec().maxCount;
		if (maxCount > 0 && pPlayer->m_antidotes >= maxCount)
			return false;

		pPlayer->SetPickupSuitUpdate(this, "!HEV_DET4", SUIT_NEXT_IN_1MIN);

		pPlayer->m_antidotes += 1;

		EmitSoundScriptWithOptionalSampleOverride(pickupSoundScript, pev->noise);

		NotifyPickup(pPlayer, pev->classname);

		return true;
	}

	static const NamedSoundScript pickupSoundScript;
};

LINK_ENTITY_TO_CLASS( item_antidote, CItemAntidote )

const NamedSoundScript CItemAntidote::pickupSoundScript = {
	CHAN_ITEM,
	{"items/gunpickup4.wav"},
	"Antidote.Pickup"
};

class CItemRadiation : public CItem
{
public:
	void Spawn() override
	{
		Precache();
		SetMyModel( "models/w_rad.mdl" );
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel( "models/w_rad.mdl" );
		RegisterAndPrecacheSoundScript(pickupSoundScript);
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		const int maxCount = g_InventorySpec.GetRadcanSpec().maxCount;
		if (maxCount > 0 && pPlayer->m_radcans >= maxCount)
			return false;

		pPlayer->SetPickupSuitUpdate(this, nullptr, SUIT_NEXT_IN_1MIN);

		pPlayer->m_radcans += 1;

		EmitSoundScript(pickupSoundScript);

		NotifyPickup(pPlayer, pev->classname);

		return true;
	}

	static const NamedSoundScript pickupSoundScript;
};

LINK_ENTITY_TO_CLASS( item_radiation, CItemRadiation )

const NamedSoundScript CItemRadiation::pickupSoundScript = {
	CHAN_ITEM,
	{"items/gunpickup4.wav"},
	"Antirad.Pickup"
};

class CItemAdrenaline : public CItem
{
public:
	void Spawn() override
	{
		Precache();
		SetMyModel( "models/w_adrenaline.mdl" );
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel( "models/w_adrenaline.mdl" );
		RegisterAndPrecacheSoundScript(pickupSoundScript);
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		const int maxCount = g_InventorySpec.GetAdrenalineSpec().maxCount;
		if (maxCount > 0 && pPlayer->m_adrenalines >= maxCount)
			return false;

		pPlayer->SetPickupSuitUpdate(this, nullptr, SUIT_NEXT_IN_1MIN);

		pPlayer->m_adrenalines += 1;

		EmitSoundScript(pickupSoundScript);

		NotifyPickup(pPlayer, pev->classname);

		return true;
	}

	static const NamedSoundScript pickupSoundScript;
};

LINK_ENTITY_TO_CLASS( item_adrenaline, CItemAdrenaline )

const NamedSoundScript CItemAdrenaline::pickupSoundScript = {
	CHAN_ITEM,
	{"items/gunpickup4.wav"},
	"Adrenaline.Pickup"
};

class CItemSecurity : public CItem
{
	void Spawn() override
	{
		Precache();
		SetMyModel( "models/w_security.mdl" );
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel( "models/w_security.mdl" );
		if (!FStringNull(pev->noise))
			PRECACHE_SOUND( STRING(pev->noise) );
	}
	void KeyValue(KeyValueData* pkvd) override
	{
		if (FStrEq(pkvd->szKeyName, "hudname"))
		{
			pev->netname = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else
			CItem::KeyValue(pkvd);
	}

	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		pPlayer->SetPickupSuitUpdate(this, nullptr, SUIT_NEXT_IN_1MIN);

		if (!FStringNull(pev->noise))
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, STRING(pev->noise), 1, ATTN_NORM );
		NotifyPickup(pPlayer, pev->netname);
		if (!FStringNull(pev->message))
			UTIL_ShowMessage( STRING( pev->message ), pPlayer );

		return true;
	}
};

LINK_ENTITY_TO_CLASS( item_security, CItemSecurity );

class CItemPickup : public CItem
{
public:
	void Spawn() override
	{
		Precache();
		SetMyModel("models/w_security.mdl");
		CItem::Spawn();
	}
	void Precache() override
	{
		if (FStringNull(pev->netname))
		{
			ALERT(at_warning, "%s without an inventory item type!\n", STRING(pev->classname));
		}
		if (FStringNull(m_entTemplate))
		{
			if (!FStringNull(pev->netname))
			{
				const InventoryItemSpec* spec = g_InventorySpec.GetInventoryItemSpec(STRING(pev->netname));
				if (spec && !spec->pickupEntTemplateName.empty())
				{
					m_entTemplate = ALLOC_STRING(spec->pickupEntTemplateName.c_str());
				}
			}
		}

		if (!MyOwnModel(nullptr))
		{
			ALERT(at_console, "%s without model defined! Fallbacking to the security card model\n", STRING(pev->classname));
		}
		PrecacheMyModel("models/w_security.mdl");
		RegisterAndPrecacheSoundScript(Items::inventoryPickupSoundScript);
		if (!FStringNull(pev->noise))
			PRECACHE_SOUND( STRING(pev->noise) );
	}
	void KeyValue(KeyValueData* pkvd) override
	{
		if (FStrEq(pkvd->szKeyName, "item_name"))
		{
			pev->netname = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else if (FStrEq(pkvd->szKeyName, "count"))
		{
			pev->impulse = atoi(pkvd->szValue);
			pkvd->fHandled = true;
		}
		else
			CItem::KeyValue(pkvd);
	}

	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		if (!FStringNull(pev->netname))
		{
			const int result = pPlayer->GiveInventoryItem(pev->netname, pev->impulse > 0 ? pev->impulse : 1);
			if (result < INVENTORY_ITEM_GIVEN)
				return false;
		}

		const SoundScript* pickupSoundScript = GetSoundScript(Items::inventoryPickupSoundScript);
		if (pickupSoundScript)
		{
			if (!FStringNull(pev->noise))
			{
				SoundScript soundScript = *pickupSoundScript;
				soundScript.waves.clear();
				soundScript.waves.push_back(STRING(pev->noise));
				pPlayer->EmitSoundScript(&soundScript);
			}
			else
			{
				pPlayer->EmitSoundScript(pickupSoundScript);
			}
		}

		if (!FStringNull(pev->message))
			UTIL_ShowMessage( STRING( pev->message ), pPlayer );

		pPlayer->SetPickupSuitUpdate(this, nullptr, SUIT_REPEAT_OK);

		return true;
	}

	bool IsUsefulToDisplayHint(CBaseEntity* pPlayer) override
	{
		if (!CItem::IsUsefulToDisplayHint(pPlayer))
			return false;

		if (pPlayer->IsPlayer())
		{
			CBasePlayer* p = (CBasePlayer*)pPlayer;
			return p->CanHaveIntenvoryItem(pev->netname);
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS( item_pickup, CItemPickup )

class CItemLongJump : public CItem
{
	void Spawn() override
	{ 
		Precache();
		SetMyModel( "models/w_longjump.mdl" );
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel( "models/w_longjump.mdl" );
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		if( pPlayer->m_fLongJump )
		{
			return false;
		}

		if( pPlayer->HasSuit() )
		{
			pPlayer->SetLongjump(true);
			NotifyPickup(pPlayer, pev->classname);

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump )

#define FLASHLIGHT_MODEL "models/w_flashlight.mdl"

class CItemFlashlight : public CItem
{
	static bool g_hasFlashlightModel;
	static bool g_checkedFlashligthModel;
public:
	static constexpr const char* pickupSoundScript = "Flashlight.Pickup";

	void Spawn() override
	{
		Precache();
		SetMyModel(DefaultModel());
		CItem::Spawn();
	}
	void Precache() override
	{
		if (!g_checkedFlashligthModel)
		{
			int fileSize;
			byte* pMemFile = g_engfuncs.pfnLoadFileForMe( FLASHLIGHT_MODEL, &fileSize );
			if (pMemFile)
			{
				g_hasFlashlightModel = true;
				g_engfuncs.pfnFreeFile( pMemFile );
			}
			g_checkedFlashligthModel = true;
		}

		PrecacheMyModel (DefaultModel());
		RegisterAndPrecacheSoundScript(pickupSoundScript, Items::pickupSoundScript);
	}
	const char* DefaultModel()
	{
		if (g_hasFlashlightModel)
			return FLASHLIGHT_MODEL;
		else
			return "sprites/iunknown.spr";
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		if (g_modFeatures.suit_light_allow_both)
		{
			if (pPlayer->HasFlashlight())
				return false;
		}
		else if ( pPlayer->HasSuitLight() )
			return false;
		pPlayer->SetFlashlight();
		NotifyPickup(pPlayer, pev->classname);
		pPlayer->EmitSoundScript(GetSoundScript(pickupSoundScript));
		return true;
	}
};
LINK_ENTITY_TO_CLASS(item_flashlight, CItemFlashlight)

bool CItemFlashlight::g_hasFlashlightModel = false;
bool CItemFlashlight::g_checkedFlashligthModel = false;

class CItemNVG : public CItem
{
public:
	void Spawn() override
	{
		Precache();
		SetMyModel("sprites/iunknown.spr");
		CItem::Spawn();
	}
	void Precache() override
	{
		PrecacheMyModel("sprites/iunknown.spr");
	}
	bool MyTouch( CBasePlayer *pPlayer ) override
	{
		if (g_modFeatures.suit_light_allow_both)
		{
			if (pPlayer->HasNVG())
				return false;
		}
		else if ( pPlayer->HasSuitLight() )
			return false;
		pPlayer->SetNVG();
		NotifyPickup(pPlayer, pev->classname);
		return true;
	}
};
LINK_ENTITY_TO_CLASS(item_nvgs, CItemNVG)

//=========================================================
// Generic item
//=========================================================
#define SF_ITEM_GENERIC_DROP_TO_FLOOR 1
#define SF_ITEM_GENERIC_DONT_TRANSIT 2
#define SF_ITEM_GENERIC_APPLY_GRAVITY 4

class CItemGeneric : public CBaseAnimating
{
public:
	int		Save(CSave &save) override;
	int		Restore(CRestore &restore) override;

	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Precache() override;
	void KeyValue(KeyValueData* pkvd) override;
	int	ObjectCaps() override;

	void SetObjectCollisionBox() override;

	void EXPORT StartupThink();
	void EXPORT SequenceThink();

	string_t m_iszSequenceName;
};

LINK_ENTITY_TO_CLASS(item_generic, CItemGeneric)

TYPEDESCRIPTION CItemGeneric::m_SaveData[] =
{
	DEFINE_FIELD(CItemGeneric, m_iszSequenceName, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE(CItemGeneric, CBaseAnimating)

void CItemGeneric::Spawn()
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

void CItemGeneric::Precache()
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL(STRING(pev->model));
}

void CItemGeneric::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		m_iszSequenceName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
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

void CItemGeneric::StartupThink()
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

void CItemGeneric::SequenceThink()
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

#define SF_EYESCANNER_SOLID 4

class CEyeScanner : public CBaseAnimating
{
public:
	void KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	void Precache() override;
	void PlayBeep();
	void WaitForSequenceEnd();
	void Think() override;
	int ObjectCaps() override { return CBaseAnimating::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE; }
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	TakeDamageResult TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo) override;
	int LookupActivity(int activity) override;
	void SetActivity(Activity NewActivity);
	void HandleAnimEvent(MonsterEvent_t *pEvent) override {
		HandleBaseAnimEvent(pEvent);
	}

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	bool IsUsefulToDisplayHint(CBaseEntity* pPlayer) override;

	static const NamedSoundScript grantedSoundScript;
	static const NamedSoundScript deniedSoundScript;
	static const NamedSoundScript beepSoundScript;
	static const NamedSoundScript grantedSentenceSoundScript;
	static const NamedSoundScript deniedSentenceSoundScript;

	string_t m_unlockedTarget;
	string_t m_lockedTarget;
	string_t m_unlockerName;
	string_t m_grantedSentence;
	string_t m_deniedSentence;
	Activity m_Activity;
	float m_fireTime;
	float m_playSentenceTime;
	float m_sentenceDelay;
	bool m_willUnlock;
	bool m_wasUnlocked;
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

const NamedSoundScript CEyeScanner::grantedSoundScript = {
	CHAN_ITEM,
	{"buttons/blip2.wav"},
	"EyeScanner.Granted"
};

const NamedSoundScript CEyeScanner::deniedSoundScript = {
	CHAN_ITEM,
	{"buttons/button11.wav"},
	"EyeScanner.Denied"
};

const NamedSoundScript CEyeScanner::beepSoundScript = {
	CHAN_BODY,
	{"buttons/blip1.wav"},
	"EyeScanner.Beep"
};

const NamedSoundScript CEyeScanner::grantedSentenceSoundScript = {
	CHAN_VOICE,
	{},
	"EyeScanner.GrantedSentence"
};

const NamedSoundScript CEyeScanner::deniedSentenceSoundScript = {
	CHAN_VOICE,
	{},
	"EyeScanner.DeniedSentence"
};

int CEyeScanner::LookupActivity(int activity)
{
	int foundSequence = CBaseAnimating::LookupActivity(activity);

	if (foundSequence == ACTIVITY_NOT_AVAILABLE)
	{
		const char* animTry = nullptr;

		switch(activity)
		{
		case ACT_CROUCHIDLE:
			animTry = "idle_closed";
			break;
		case ACT_IDLE:
			animTry = "idle_open";
			break;
		case ACT_STAND:
			animTry = "activate";
			break;
		case ACT_CROUCH:
			animTry = "deactivate";
			break;
		}

		if (animTry)
		{
			foundSequence = LookupSequence(animTry);
		}
	}
	return foundSequence;
}

void CEyeScanner::SetActivity( Activity NewActivity )
{
	int iSequence = LookupActivity( NewActivity );

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
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_target"))
	{
		m_lockedTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "unlockersname"))
	{
		m_unlockerName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "granted_sentence"))
	{
		m_grantedSentence = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "denied_sentence"))
	{
		m_deniedSentence = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "sentence_delay"))
	{
		m_sentenceDelay = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "reset_delay")) // Dunno if it affects anything in PC version of Decay
	{
		//m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseAnimating::KeyValue( pkvd );
}

void CEyeScanner::Spawn()
{
	Precache();

	//SetBits(pev->spawnflags, SF_EYESCANNER_SOLID);
	if (FBitSet(pev->spawnflags, SF_EYESCANNER_SOLID))
	{
		pev->solid = SOLID_SLIDEBOX;
	}
	else
	{
		pev->solid = SOLID_NOT;
	}
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->health = 1;
	pev->weapons = 0;
	m_willUnlock = false;

	SetMyModel("models/EYE_SCANNER.mdl");
	if (FBitSet(pev->spawnflags, SF_EYESCANNER_SOLID))
	{
		SetSequenceSafeBox(1.0f, 0.0f);
	}
	else
	{
		SetSequenceSafeBox(10.0f);
	}
	UTIL_SetOrigin(pev, pev->origin);
	SetActivity(ACT_CROUCHIDLE);
	ResetSequenceInfo();

	/*ALERT(at_console, "%s: yaw %g; mins: %g, %g, %g; maxs: %g, %g, %g\n",
		  STRING(pev->classname), pev->angles.y, pev->mins.x, pev->mins.y, pev->mins.z, pev->maxs.x, pev->maxs.y, pev->maxs.z);*/
}

void CEyeScanner::Precache()
{
	PrecacheMyModel("models/EYE_SCANNER.mdl");

	RegisterAndPrecacheSoundScript(grantedSoundScript);
	RegisterAndPrecacheSoundScript(deniedSoundScript);
	RegisterAndPrecacheSoundScript(beepSoundScript);
	RegisterAndPrecacheSoundScript(grantedSentenceSoundScript);
	RegisterAndPrecacheSoundScript(deniedSentenceSoundScript);

	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(STRING(pev->noise));
	if (!FStringNull(pev->noise1))
		PRECACHE_SOUND(STRING(pev->noise1));
	if (!FStringNull(pev->noise2))
		PRECACHE_SOUND(STRING(pev->noise2));

	SetActivity( m_Activity );
}

void CEyeScanner::PlayBeep()
{
	pev->skin = pev->weapons % 3 + 1;
	pev->weapons++;
	EmitSoundScriptWithOptionalSampleOverride(beepSoundScript, pev->noise2);
}

void CEyeScanner::WaitForSequenceEnd()
{
	if (m_fSequenceFinished) {
		if (m_Activity == ACT_STAND) {
			SetActivity(ACT_IDLE);
		} else if (m_Activity == ACT_CROUCH) {
			SetActivity(ACT_CROUCHIDLE);
		}
	} else {
		StudioFrameAdvance();
		DispatchAnimEvents();
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
			EmitSoundScriptWithOptionalSampleOverride(grantedSoundScript, pev->noise);
			DelayedUse( m_flDelay, this, this, USE_TOGGLE, m_unlockedTarget );
		} else {
			EmitSoundScriptWithOptionalSampleOverride(deniedSoundScript, pev->noise1);
			DelayedUse( m_flDelay, this, this, USE_TOGGLE, m_lockedTarget );
		}

		float sentenceDelay;
		if (m_sentenceDelay > 0.0f)
			sentenceDelay = m_sentenceDelay;
		else
			sentenceDelay = GetSkillValue("eyescanner_sentence_delay");
		m_playSentenceTime = gpGlobals->time + sentenceDelay;
		m_willUnlock = false;
		m_fireTime = 0;
		pev->skin = 0;
		pev->weapons = 0;
		if (m_Activity == ACT_IDLE)
			SetActivity(ACT_CROUCH);
	}
	if (m_playSentenceTime != 0 && m_playSentenceTime <= gpGlobals->time) {
		if (m_wasUnlocked) {
			EmitSoundScriptWithOptionalSampleOverride(grantedSentenceSoundScript, m_grantedSentence);
		} else {
			EmitSoundScriptWithOptionalSampleOverride(deniedSentenceSoundScript, m_deniedSentence);
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

bool CEyeScanner::IsUsefulToDisplayHint(CBaseEntity* pPlayer)
{
	if (!FStringNull(m_unlockerName))
	{
		return FClassnameIs(pPlayer->pev, STRING(m_unlockerName));
	}
	return false;
}

TakeDamageResult CEyeScanner::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo)
{
	return TakeDamageResult();
}
