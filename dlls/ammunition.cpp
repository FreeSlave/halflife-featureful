#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ammunition.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"
#include "ammo_amounts.h"
#include "common_soundscripts.h"

void CBasePlayerAmmo::Spawn()
{
	Precache();
	SetMyModel(MyModel());

	if (pev->movetype < 0)
		pev->movetype = MOVETYPE_NONE;
	else if (pev->movetype == 0)
		pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	const bool comesFromBreakable = pev->owner != NULL;
	if (!comesFromBreakable && ItemsPhysicsFix() == 2)
	{
		pev->solid = SOLID_BBOX;
		SetThink( &CPickup::FallThink );
		pev->nextthink = gpGlobals->time + 0.1f;
		SetBits(pev->spawnflags, SF_ITEM_FIX_PHYSICS);
	}
	if (ItemsPhysicsFix() == 3)
	{
		SetBits(pev->spawnflags, SF_ITEM_FIX_PHYSICS);
	}

	if (FBitSet(pev->spawnflags, SF_ITEM_FIX_PHYSICS))
		UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	else
		UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouchAndUse();
}

void CBasePlayerAmmo::Precache()
{
	PrecacheMyModel(MyModel());
	RegisterAndPrecacheSoundScript(Items::ammoPickupSoundScript);
}

void CBasePlayerAmmo::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "ammo_amount"))
	{
		SetCustomAmount(atoi(pkvd->szValue));
		pkvd->fHandled = true;
	}
	else
		CPickup::KeyValue(pkvd);
}

Vector CBasePlayerAmmo::MyRespawnSpot()
{
	return g_pGameRules->VecAmmoRespawnSpot( this );
}

float CBasePlayerAmmo::MyRespawnTime()
{
	return g_pGameRules->FlAmmoRespawnTime( this );
}

void CBasePlayerAmmo::OnMaterialize()
{
	SetTouchAndUse();
	SetThink( NULL );
}

void CBasePlayerAmmo::DefaultTouch( CBaseEntity *pOther )
{
	if (IsPickableByTouch()) {
		//Prevent dropped ammo from touching at the same time
		if( pev->bInDuck && !( pev->flags & FL_ONGROUND ) )
		{
			return;
		}
		pev->bInDuck = 0;
		TouchOrUse(pOther);
	}
}

void CBasePlayerAmmo::DefaultUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsPickableByUse() && !(pev->effects & EF_NODRAW) ) {
		TouchOrUse(pCaller);
	}
}

extern bool gEvilImpulse101;

void CBasePlayerAmmo::TouchOrUse( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() || !pOther->IsAlive() || IsPlayerBusting( pOther ) )
	{
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;
	if (!pPlayer->CanHaveItem(this))
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if( AddAmmo( pOther ) )
	{
		SUB_UseTargets( pOther );

		if( g_pGameRules->AmmoShouldRespawn( this ) == GR_AMMO_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			ClearTouchAndUse();
			RemoveMyself();
		}
	}
	else if( gEvilImpulse101 )
	{
		// evil impulse 101 hack, kill always
		ClearTouchAndUse();
		RemoveMyself();
	}
}

bool CBasePlayerAmmo::AddAmmo(CBaseEntity *pOther)
{
	const char* ammoName = AmmoName();
	if (!ammoName)
		return false;

	const int amount = MyAmount();

	if ( pOther->GiveAmmo( amount, ammoName ) > 0 )
	{
		EmitSoundScript(Items::ammoPickupSoundScript);
		return true;
	}
	return false;
}

int CBasePlayerAmmo::MyAmount()
{
	if (pev->impulse > 0)
		return pev->impulse;
	const int amount = g_AmmoAmounts.AmountForAmmoEnt(STRING(pev->classname));
	if (amount >= 0)
		return amount;
	return DefaultAmount();
}

void CBasePlayerAmmo::SetCustomAmount(int amount)
{
	if (amount >= 0)
		pev->impulse = amount;
}

void CBasePlayerAmmo::SetTouchAndUse()
{
	SetTouch( &CBasePlayerAmmo::DefaultTouch );
	SetUse( &CBasePlayerAmmo::DefaultUse );
}

void CBasePlayerAmmo::ClearTouchAndUse()
{
	SetTouch( NULL );
	SetUse( NULL );
}

void CBasePlayerAmmo::RemoveMyself()
{
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

// Ammo entities


class CGlockAmmo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_9mmclip.mdl";
	}
	int DefaultAmount() override {
		return AMMO_GLOCKCLIP_GIVE;
	}
	const char* AmmoName() override {
		return "9mm";
	}
};

LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo )
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo )

class CPythonAmmo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_357ammobox.mdl";
	}
	int DefaultAmount() override {
		return AMMO_357BOX_GIVE;
	}
	const char* AmmoName() override {
		return "357";
	}
};

LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo )

class CMP5AmmoClip : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_9mmARclip.mdl";
	}
	int DefaultAmount() override {
		return AMMO_MP5CLIP_GIVE;
	}
	const char* AmmoName() override {
		return "9mm";
	}
};

LINK_ENTITY_TO_CLASS( ammo_mp5clip, CMP5AmmoClip )
LINK_ENTITY_TO_CLASS( ammo_9mmAR, CMP5AmmoClip )

class CMP5Chainammo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_chainammo.mdl";
	}
	int DefaultAmount() override {
		return AMMO_CHAINBOX_GIVE;
	}
	const char* AmmoName() override {
		return "9mm";
	}
};

LINK_ENTITY_TO_CLASS( ammo_9mmbox, CMP5Chainammo )

class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_ARgrenade.mdl";
	}
	int DefaultAmount() override {
		return AMMO_M203BOX_GIVE;
	}
	const char* AmmoName() override {
		return "ARgrenades";
	}
};

LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade )
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade )

class CShotgunAmmo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_shotbox.mdl";
	}
	int DefaultAmount() override {
		return AMMO_BUCKSHOTBOX_GIVE;
	}
	const char* AmmoName() override {
		return "buckshot";
	}
};

LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo )

class CCrossbowAmmo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_crossbow_clip.mdl";
	}
	int DefaultAmount() override {
		return AMMO_CROSSBOWCLIP_GIVE;
	}
	const char* AmmoName() override {
		return "bolts";
	}
};

LINK_ENTITY_TO_CLASS( ammo_crossbow, CCrossbowAmmo )

class CRpgAmmo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_rpgammo.mdl";
	}
	int DefaultAmount() override {
		int iGive;
	if( bIsMultiplayer() )
		{
			// hand out more ammo per rocket in multiplayer.
			iGive = AMMO_RPGCLIP_GIVE * 2;
		}
		else
		{
			iGive = AMMO_RPGCLIP_GIVE;
		}
		return iGive;
	}
	const char* AmmoName() override {
		return "rockets";
	}
};

LINK_ENTITY_TO_CLASS( ammo_rpgclip, CRpgAmmo )

class CGaussAmmo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_gaussammo.mdl";
	}
	int DefaultAmount() override {
		return AMMO_URANIUMBOX_GIVE;
	}
	const char* AmmoName() override {
		return "uranium";
	}
};

class CEgonAmmo : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_chainammo.mdl";
	}
	int DefaultAmount() override {
		return AMMO_URANIUMBOX_GIVE;
	}
	const char* AmmoName() override {
		return "uranium";
	}
};

LINK_ENTITY_TO_CLASS( ammo_egonclip, CEgonAmmo )

LINK_ENTITY_TO_CLASS( ammo_gaussclip, CGaussAmmo )

class CSniperrifleAmmo : public CBasePlayerAmmo
{
	bool IsEnabledInMod() override {
		return g_modFeatures.ammo762IsUsed;
	}
	const char* MyModel() override {
		return "models/w_m40a1clip.mdl";
	}
	int DefaultAmount() override {
		return AMMO_762BOX_GIVE;
	}
	const char* AmmoName() override {
		return "762";
	}
};
LINK_ENTITY_TO_CLASS( ammo_762, CSniperrifleAmmo )

class CM249AmmoClip : public CBasePlayerAmmo
{
	bool IsEnabledInMod() override {
		return g_modFeatures.ammo556IsUsed;
	}
	const char* MyModel() override {
		return "models/w_saw_clip.mdl";
	}
	int DefaultAmount() override {
		return AMMO_556CLIP_GIVE;
	}
	const char* AmmoName() override {
		return "556";
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CM249AmmoClip)

class C45ACPAmmoClip : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_9mmARclip.mdl";
	}
	int DefaultAmount() override {
		return 30;
	}
	const char* AmmoName() override {
		return "45acp";
	}
};

LINK_ENTITY_TO_CLASS( ammo_45acp, C45ACPAmmoClip )

class C57MMAmmoClip : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_9mmARclip.mdl";
	}
	int DefaultAmount() override {
		return 30;
	}
	const char* AmmoName() override {
		return "57mm";
	}
};

LINK_ENTITY_TO_CLASS( ammo_57mm, C57MMAmmoClip )

class CNailsAmmoClip : public CBasePlayerAmmo
{
	const char* MyModel() override {
		return "models/w_9mmARclip.mdl";
	}
	int DefaultAmount() override {
		return 30;
	}
	const char* AmmoName() override {
		return "nails";
	}
};

LINK_ENTITY_TO_CLASS( ammo_nails, CNailsAmmoClip )
