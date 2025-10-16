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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"

#if !CLIENT_DLL
#include "combat.h"
#include "global_models.h"
#include "gamerules.h"

#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CCrossbowBolt : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	void EXPORT BubbleThink();
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink();

	static CCrossbowBolt *BoltCreate();

	static const NamedSoundScript boltHitBody;
	static const NamedSoundScript boltHitWorld;
};

LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt )

const NamedSoundScript CCrossbowBolt::boltHitBody = {
	CHAN_BODY,
	{"weapons/xbow_hitbod1.wav", "weapons/xbow_hitbod2.wav"},
	"Crossbow.BoltHitBody"
};

const NamedSoundScript CCrossbowBolt::boltHitWorld = {
	CHAN_BODY,
	{"weapons/xbow_hit1.wav"},
	FloatRange(0.95f, 1.0f),
	ATTN_NORM,
	IntRange(98, 105),
	"Crossbow.BoltHitWorld"
};

CCrossbowBolt *CCrossbowBolt::BoltCreate()
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr( (CCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING( "crossbow_bolt" );	// g-cont. enable save\restore
	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5f;

	SET_MODEL( ENT( pev ), "models/crossbow_bolt.mdl" );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( &CCrossbowBolt::BoltTouch );
	SetThink( &CCrossbowBolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}

void CCrossbowBolt::Precache()
{
	PRECACHE_MODEL( "models/crossbow_bolt.mdl" );
	RegisterAndPrecacheSoundScript(boltHitBody);
	RegisterAndPrecacheSoundScript(boltHitWorld);
}

int CCrossbowBolt::Classify()
{
	return CLASS_NONE;
}

void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if( pOther->pev->takedamage )
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t *pevOwner = VARS( pev->owner );

		DamageInfo damageInfo(gSkillData.plrDmgCrossbowMonster, DMG_BULLET);
		damageInfo.SetGibPolicy(GIB_NEVER);

		if( pOther->IsPlayer() )
		{
			damageInfo.damage = gSkillData.plrDmgCrossbowClient;
			damageInfo.type = DMG_GENERIC;
		}
		pOther->ApplyTraceAttack( pev, pevOwner, damageInfo, pev->velocity.Normalize(), &tr );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
		EmitSoundScript(boltHitBody);

		if( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, pev, GIB_NEVER );
		}
	}
	else
	{
		EmitSoundScript(boltHitWorld);

		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12.0f );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 360 );
			pev->nextthink = gpGlobals->time + 10.0f;
		}
		// TODO: make configurable?
		/*else if( g_fIsXash3D && (pOther->pev->movetype == MOVETYPE_PUSH || pOther->pev->movetype == MOVETYPE_PUSHSTEP) )
		{
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12.0f );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 360 );
			pev->nextthink = gpGlobals->time + 10.0f;			

			// g-cont. Setup movewith feature
			pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
			pev->aiment = ENT( pOther->pev );	// set parent
		}*/

		if( UTIL_PointContents( pev->origin ) != CONTENTS_WATER )
		{
			UTIL_Sparks( pev->origin );
		}
	}

	if( g_pGameRules->IsMultiplayer() )
	{
		SetThink( &CCrossbowBolt::ExplodeThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CCrossbowBolt::BubbleThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->waterlevel == WL_NotInWater )
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 1 );
}

void CCrossbowBolt::ExplodeThink()
{
	int iContents = UTIL_PointContents( pev->origin );
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_VECTOR( pev->origin );
		if( iContents != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( iScale ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;

	if( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, DamageInfo(pev->dmg, DMG_BLAST).SetGibPolicy(GIB_ALWAYS), 128, CLASS_NONE );

	UTIL_Remove( this );
}
#endif

enum crossbow_e
{
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// reload
	CROSSBOW_FIRE3,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2	// empty
};

class CCrossbow : public CConfigurableWeapon
{
public:
	void Precache() override;
	int WeaponId() const override { return WEAPON_CROSSBOW; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
	int GetPlaybackEvent(bool altModeFire) const override;

	void NativeAttack(bool altMode) override;
private:
	unsigned short m_usCrossbow2;
};

LINK_WEAPON_TO_CLASS( weapon_crossbow, CCrossbow )

void CCrossbow::Precache()
{
	CConfigurableWeapon::Precache();

	UTIL_PrecacheOther( "crossbow_bolt" );

	m_usCrossbow2 = PRECACHE_EVENT( 1, "events/crossbow2.sc" );
}

bool CCrossbow::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 2;
	p->iPosition = 2;
	p->pszAmmoEntity = "ammo_crossbow";
	p->iDropAmmo = AMMO_CROSSBOWCLIP_GIVE;
	return true;
}

WeaponParameters CCrossbow::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 5;
	params.maxClip = 5;
	params.ammoName = "bolts";

	params.worldModel = "models/w_crossbow.mdl";
	params.viewModel = "models/v_crossbow.mdl";
	params.playerModel = "models/p_crossbow.mdl";
	params.playerAnimExt = "bow";
	params.priority = 10;

	params.deploy.animIndex = CROSSBOW_DRAW1;
	params.deploy.animIndex.mainEmptied = CROSSBOW_DRAW2;

	params.idleAnims.main =  WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{CROSSBOW_IDLE1, 0.75f, 91.0f / 30.0f },
		WeaponParameters::IdleAnim{CROSSBOW_FIDGET1, 0.25f, 81.0f / 30.0f},
	};

	params.idleAnims.mainEmptied =  WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{CROSSBOW_IDLE2, 0.75f, 91.0f / 30.0f },
		WeaponParameters::IdleAnim{CROSSBOW_FIDGET2, 0.25f, 81.0f / 30.0f},
	};

	params.fire.fireType = WeaponParameters::Fire::NATIVE;
	params.fire.anims = {CROSSBOW_FIRE1};
	params.fire.anims.mainEmptied = {CROSSBOW_FIRE3};
	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/xbow_fire1.wav"},
		1.0f,
		ATTN_NORM,
		IntRange(93, 108)
	};
	params.fire.soundAdditional = {
		CHAN_ITEM,
		{"weapons/xbow_reload1.wav"},
		FloatRange(0.9f, 1.0f),
		ATTN_NORM,
		IntRange(93, 108)
	};
	params.fire.cycleTime = 0.75f;
	params.fire.idleDelay = 5.0f;
	params.fire.idleDelay.mainEmptied = 0.75f;
	params.fire.allowUnderwater = true;
	params.fire.autoAimDegree = AUTOAIM_2DEGREES;
	params.fire.weaponVolume = QUIET_GUN_VOLUME;
	params.fire.clientPunchPitch = -2.0f;
	if (bIsMultiplayer())
		params.fire.clientPunchPitch.alt = 0.0f;

	params.fire.idleDelay = 5.0f;
	params.fire.idleDelay.mainEmptied = 0.75f;

	params.altMode.zoomFOV = 20;
	params.altMode.attackDelay = 1.0f;
	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;

	params.reload.animIndex = CROSSBOW_RELOAD;
	params.reload.duration = 4.5f;
	params.reload.sound = {
		CHAN_ITEM,
		{"weapons/xbow_reload1.wav"},
		FloatRange(0.95f, 1.0f),
		ATTN_NORM,
		IntRange(93, 108)
	};

	params.holster.animIndex = CROSSBOW_HOLSTER1;
	params.holster.attackDelay = 0.5f;

	params.holster.animIndex.mainEmptied = CROSSBOW_HOLSTER2;

	return params;
}

int CCrossbow::GetPlaybackEvent(bool altModeFire) const
{
	return (altModeFire && bIsMultiplayer()) ? m_usCrossbow2 : CConfigurableWeapon::GetPlaybackEvent(altModeFire);
}

void CCrossbow::NativeAttack(bool altMode)
{
	if (altMode && bIsMultiplayer())
	{
		TraceResult tr;

		Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		UTIL_MakeVectors( anglesAim );
		Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2.0f;
		Vector vecDir = gpGlobals->v_forward;

		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, m_pPlayer->edict(), &tr );

#if !CLIENT_DLL
		if( tr.pHit->v.takedamage )
		{
			CBaseEntity::Instance( tr.pHit )->ApplyTraceAttack( m_pPlayer->pev, m_pPlayer->pev, DamageInfo(120, DMG_BULLET).SetGibPolicy(GIB_NEVER), vecDir, &tr );
		}
#endif
		return;
	}

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );

	anglesAim.x	= -anglesAim.x;

#if !CLIENT_DLL
	Vector vecSrc	= m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2.0f;
	Vector vecDir	= gpGlobals->v_forward;

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10.0f;
#endif
}
