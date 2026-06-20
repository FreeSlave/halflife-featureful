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
#include "mod_features.h"

class CRpg : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_RPG; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
};

#if !CLIENT_DLL
#include "gamerules.h"
#include "rpgrocket.h"
#include "visuals_utils.h"

LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot )
LINK_ENTITY_TO_CLASS( eagle_laser, CLaserSpot )

//=========================================================
//=========================================================
CLaserSpot *CLaserSpot::CreateSpot( edict_t* pOwner )
{
	CLaserSpot *pSpot = GetClassPtr( (CLaserSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING( "laser_spot" );

#if FEATURE_PREDICTABLE_LASER_SPOT
	if( pOwner )
	{
		// predictable laserspot
		pSpot->pev->flags |= FL_SKIPLOCALHOST;
		pOwner->v.flags |= FL_LASERDOT;
		pSpot->pev->owner = pOwner;
	}
#endif

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpot::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL( ENT( pev ), "sprites/laserdot.spr" );
	UTIL_SetOrigin( pev, pev->origin );
}

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
#if FEATURE_PREDICTABLE_LASER_SPOT
	if (!FNullEnt(pev->owner))
		pev->owner->v.flags &= ~FL_LASERDOT;
#endif
	SetThink( &CLaserSpot::Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot::Revive()
{
	pev->effects &= ~EF_NODRAW;
#if FEATURE_PREDICTABLE_LASER_SPOT
	if (!FNullEnt(pev->owner))
		pev->owner->v.flags |= FL_LASERDOT;
#endif
	SetThink( NULL );
}

KilledResult CLaserSpot::Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib)
{
#if FEATURE_PREDICTABLE_LASER_SPOT
	if (!FNullEnt(pev->owner))
		pev->owner->v.flags &= ~FL_LASERDOT;
#endif
	return CBaseEntity::Killed( pevInflictor, pevAttacker, iGib );
}

void CLaserSpot::Precache()
{
	PRECACHE_MODEL( "sprites/laserdot.spr" );
}

LINK_ENTITY_TO_CLASS( rpg_rocket, CRpgRocket )

TYPEDESCRIPTION	CRpgRocket::m_SaveData[] =
{
	DEFINE_FIELD( CRpgRocket, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CRpgRocket, m_hLauncher, FIELD_EHANDLE ),
	DEFINE_FIELD( CRpgRocket, m_straight, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRpgRocket, m_soundStarted, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CRpgRocket, CGrenade )

const NamedSoundScript CRpgRocket::rocketIgniteSoundScript = {
	CHAN_VOICE,
	{"weapons/rocket1.wav"},
	1.0f,
	0.5f,
	"RPG.RocketIgnite"
};

const NamedVisual CRpgRocket::trailVisual = BuildVisual("RPG.Trail")
		.Model("sprites/smoke.spr")
		.Life(4)
		.BeamWidth(5)
		.RenderColor(224, 224, 255)
		.Alpha(255);

//=========================================================
//=========================================================
void CRpgRocket::Explode( TraceResult *pTrace, int bitsDamageType )
{
	if( CConfigurableWeapon *pLauncher = GetLauncher())
	{
		// my launcher is still around, tell it I'm dead.
		pLauncher->m_cActiveRockets--;
		m_hLauncher = 0;
	}

	StopSoundScript(rocketIgniteSoundScript);

	CGrenade::Explode( pTrace, bitsDamageType );
}

CConfigurableWeapon *CRpgRocket::GetLauncher()
{
	return m_hLauncher.Entity<CConfigurableWeapon>();
}

//=========================================================
//=========================================================
void CRpgRocket::Spawn()
{
	Precache();
	// motor
	if (m_straight)
		pev->movetype = MOVETYPE_FLY;
	else
		pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetMyModel("models/rpgrocket.mdl");
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING( "rpg_rocket" );

	SetThink( &CRpgRocket::IgniteThink );
	SetTouch( &CGrenade::ExplodeTouch );

	if (!m_straight)
	{
		Vector angles = pev->angles;
		angles.x = -angles.x;
		angles.x -= 30.0f;
		UTIL_MakeVectors(angles);
		pev->gravity = 0.5f;
	}
	pev->velocity = gpGlobals->v_forward * 250.0f;

	pev->nextthink = gpGlobals->time + 0.4f;

	SetDefaultProjectileDamage(GetSkillValue("plr_rpg"));
}

//=========================================================
//=========================================================
void CRpgRocket::Precache()
{
	PrecacheBaseGrenadeSounds();
	PrecacheMyModel("models/rpgrocket.mdl");
	RegisterVisual(trailVisual);
	RegisterAndPrecacheSoundScript(rocketIgniteSoundScript);
}

void CRpgRocket::SetProjectileParamsBeforeSpawn(const ProjectileParameters& params)
{
	SetProjectileParamsBeforeSpawnImpl(params);
	m_straight = params.variant != 0;
	if (params.pLauncher)
	{
		CBasePlayerWeapon* pWeapon = params.pLauncher->MyWeaponPointer();
		if (pWeapon)
		{
			CConfigurableWeapon* pConfigurableWeapon = pWeapon->MyConfigurableWeaponPointer();
			if (pConfigurableWeapon)
			{
				m_hLauncher = pConfigurableWeapon;// remember what RPG fired me.
			}
		}
	}
}

void CRpgRocket::LaunchAsProjectile(const ProjectileParameters& params)
{
	if (params.speedOverride)
		pev->velocity = pev->velocity.Normalize() * params.speedOverride;
	CConfigurableWeapon* pLauncher = GetLauncher();
	if (pLauncher)
	{
		pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	}
}

void CRpgRocket::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	SetMyProjectileEffectFlags(EF_LIGHT);

	// make rocket sound
	m_soundStarted = true;
	EmitSoundScript(rocketIgniteSoundScript);

	// rocket trail
	SendBeamFollow(entindex(), GetVisual(trailVisual));

	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink( &CRpgRocket::FollowThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRpgRocket::FollowThink()
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors( pev->angles );

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;
	
	// Examine all entities within a reasonable radius
	while( ( pOther = UTIL_FindEntityByClassname( pOther, "laser_spot" ) ) != NULL )
	{
		UTIL_TraceLine( pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT( pev ), &tr );
		// ALERT( at_console, "%f\n", tr.flFraction );
		if( tr.flFraction >= 0.9f )
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length();
			vecDir = vecDir.Normalize();
			flDot = DotProduct( gpGlobals->v_forward, vecDir );
			if( ( flDot > 0 ) && ( flDist * ( 1 - flDot ) < flMax ) )
			{
				flMax = flDist * ( 1 - flDot );
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles( vecTarget );

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if( gpGlobals->time - m_flIgniteTime < 1.0f )
	{
		pev->velocity = pev->velocity * 0.2f + vecTarget * ( flSpeed * 0.8f + 400.0f );
		if( pev->waterlevel == WL_Eyes )
		{
			// go slow underwater
			pev->velocity.ClampToLengthInPlace(300.0f);
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 4 );
		} 
		else 
		{
			pev->velocity.ClampToLengthInPlace(2000.0f);
		}
	}
	else
	{
		pev->effects = 0;
		if (m_soundStarted)
		{
			m_soundStarted = false;
			StopSoundScript(rocketIgniteSoundScript);
		}
		pev->velocity = pev->velocity * 0.2f + vecTarget * flSpeed * 0.798f;
		if( pev->waterlevel == WL_NotInWater && pev->velocity.IsLengthLessThan(1500.0f) )
			Detonate();
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	if( CConfigurableWeapon *pLauncher = GetLauncher())
	{
		if( ( pev->origin - pLauncher->pev->origin ).IsLengthGreaterThan(8192) || gpGlobals->time - m_flIgniteTime > 6.0f )
		{
			// my launcher is still around, tell it I'm dead.
			pLauncher->m_cActiveRockets--;
			m_hLauncher = 0;
		}
	}

	if( UTIL_PointContents( pev->origin ) == CONTENTS_SKY )
		Detonate();

	pev->nextthink = gpGlobals->time + 0.1f;
}
#endif

enum rpg_e
{
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,		// to reload
	RPG_FIRE2,		// to empty
	RPG_HOLSTER1,	// loaded
	RPG_DRAW1,		// loaded
	RPG_HOLSTER2,	// unloaded
	RPG_DRAW_UL,	// unloaded
	RPG_IDLE_UL,	// unloaded idle
	RPG_FIDGET_UL	// unloaded fidget
};

LINK_WEAPON_TO_CLASS( weapon_rpg, CRpg )

bool CRpg::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 3;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHTO;

	return true;
}

WeaponParameters CRpg::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = bIsMultiplayer() ? 2 : 1;
	params.maxClip = 1;
	params.ammoName = "rockets";

	params.worldModel = "models/w_rpg.mdl";
	params.viewModel = "models/v_rpg.mdl";
	params.playerModel = "models/p_rpg.mdl";
	params.playerAnimExt = "rpg";
	params.priority = 20;

	params.deploy.animIndex = RPG_DRAW1;
	params.deploy.animIndex.mainEmptied = RPG_DRAW_UL;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{RPG_IDLE, 1.0f, 90.0f / 15.0f}
	};

	params.idleAnims.mainEmptied = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{RPG_IDLE_UL, 1.0f, 90.0f / 15.0f}
	};

	params.idleAnims.alt = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{RPG_IDLE, 0.75f, 90.0f / 15.0f},
		WeaponParameters::IdleAnim{RPG_FIDGET, 0.25f, 6.1f}
	};

	params.idleAnims.altEmptied = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{RPG_IDLE_UL, 0.75f, 90.0f / 15.0f},
		WeaponParameters::IdleAnim{RPG_FIDGET_UL, 0.25f, 6.1f}
	};

	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.projectileName = "rpg_rocket";
	params.fire.projectileOffsetForward = 16.0f;
	params.fire.projectileOffsetSide = 8.0f;
	params.fire.projectileOffsetUp = -8.0f;
	params.fire.projectileRespectPunchangle = false;
	params.fire.projectileAdjustToCross = false;
	params.fire.projectileAddCurrentVelocity = WeaponParameters::Fire::ADD_VELOCITY_PROJECTION;

	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = BRIGHT_GUN_FLASH;

	params.fire.delayAfterEmpty = 0.2f;
	params.fire.cycleTime = 1.5f;
	params.fire.idleDelay = 1.5f;
	params.fire.allowUnderwater = true;

	params.fire.anims = {RPG_FIRE2};
	params.fire.clientPunchPitch = -5.0f;

	params.fire.sound = {
		CHAN_WEAPON,
		{"weapons/rocketfire1.wav"},
		0.9f,
		ATTN_NORM,
		PITCH_NORM
	};

	params.fire.soundAdditional = {
		CHAN_ITEM,
		{"weapons/glauncher.wav"},
		0.7f,
		ATTN_NORM,
		PITCH_NORM
	};

	params.reload.animIndex = RPG_RELOAD;
	params.reload.duration = 2.0f;
	params.reload.idleDelay = FloatRange(10, 15);
	params.reload.suspendLaserSpotTime = 2.1f;

	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
	params.altMode.attackDelay = 0.2f;
	params.altMode.toggleLaserSpot = true;
	params.startLaserSpot = true;
	params.laserSpotAttractRockets = true;
	params.laserSpotCheckActiveRockets = true;

	params.holster.animIndex = RPG_HOLSTER1;
	params.holster.attackDelay = 0.5f;

	params.dropAmmo.classname = "ammo_rpgclip";

	return params;
}
