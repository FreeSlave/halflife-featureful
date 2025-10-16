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

class CRpg : public CConfigurableWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Precache() override;
	int WeaponId() const override { return WEAPON_RPG; }
	void Reload() override;
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;

	bool CanHolster() override;

	void NativeAttack(bool altMode) override;

	bool ShouldWeaponIdle() override { return true; }

	int m_cActiveRockets;// how many missiles in flight from this launcher right now?

	void GetWeaponData(weapon_data_t& data) override;
	void SetWeaponData(const weapon_data_t& data) override;
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
CRpgRocket *CRpgRocket::CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher )
{
	CRpgRocket *pRocket = GetClassPtr( (CRpgRocket *)NULL );

	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch( &CRpgRocket::RocketTouch );
	pRocket->m_hLauncher = pLauncher;// remember what RPG fired me. 
	pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

void CRpgRocket::Explode( TraceResult *pTrace, int bitsDamageType )
{
	if( CRpg *pLauncher = GetLauncher())
	{
		// my launcher is still around, tell it I'm dead.
		pLauncher->m_cActiveRockets--;
		m_hLauncher = 0;
	}

	StopSoundScript(rocketIgniteSoundScript);

	CGrenade::Explode( pTrace, bitsDamageType );
}

CRpg *CRpgRocket::GetLauncher()
{
	return m_hLauncher.Entity<CRpg>();
}

//=========================================================
//=========================================================
void CRpgRocket::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING( "rpg_rocket" );

	SetThink( &CRpgRocket::IgniteThink );
	SetTouch( &CGrenade::ExplodeTouch );

	pev->angles.x -= 30.0f;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( pev->angles.x + 30.0f );

	pev->velocity = gpGlobals->v_forward * 250.0f;
	pev->gravity = 0.5f;

	pev->nextthink = gpGlobals->time + 0.4f;

	pev->dmg = gSkillData.plrDmgRPG;
}

//=========================================================
//=========================================================
void CRpgRocket::RocketTouch( CBaseEntity *pOther )
{
	if( CRpg *pLauncher = GetLauncher())
	{
		// my launcher is still around, tell it I'm dead.
		pLauncher->m_cActiveRockets--;
		m_hLauncher = 0;
	}

	StopSoundScript(rocketIgniteSoundScript);
	ExplodeTouch( pOther );
}

//=========================================================
//=========================================================
void CRpgRocket::Precache()
{
	PrecacheBaseGrenadeSounds();
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	RegisterVisual(trailVisual);
	RegisterAndPrecacheSoundScript(rocketIgniteSoundScript);
}

void CRpgRocket::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EmitSoundScript(rocketIgniteSoundScript);

	// rocket trail
	const Visual* visual = GetVisual(trailVisual);
	if (visual->modelIndex)
	{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() );	// entity
			WriteBeamFollowVisual( visual );
		MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	}

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
		if( pev->effects & EF_LIGHT )
		{
			pev->effects = 0;
			StopSoundScript(rocketIgniteSoundScript);
		}
		pev->velocity = pev->velocity * 0.2f + vecTarget * flSpeed * 0.798f;
		if( pev->waterlevel == WL_NotInWater && pev->velocity.IsLengthLessThan(1500.0f) )
			Detonate();
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	if( CRpg *pLauncher = GetLauncher())
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

#if !CLIENT_DLL
TYPEDESCRIPTION	CRpg::m_SaveData[] =
{
	DEFINE_FIELD( CRpg, m_cActiveRockets, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CRpg, CConfigurableWeapon )
#endif

void CRpg::Reload()
{
	if( m_cActiveRockets && m_bLaserActive )
	{
		// no reloading when there are active missiles tracking the designator.
		return;
	}

	PerformReload();
}

void CRpg::Precache()
{
	CConfigurableWeapon::Precache();
	UTIL_PrecacheOther( "rpg_rocket" );
}

bool CRpg::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 3;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHTO;
	p->pszAmmoEntity = "ammo_rpgclip";
	if( bIsMultiplayer() )
	{
		// hand out more ammo per rocket in multiplayer.
		p->iDropAmmo = AMMO_RPGCLIP_GIVE * 2;
	}
	else
	{
		p->iDropAmmo = AMMO_RPGCLIP_GIVE;
	}

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

	params.holster.animIndex = RPG_HOLSTER1;
	params.holster.attackDelay = 0.5f;

	return params;
}

bool CRpg::CanHolster()
{
	if( m_bLaserActive && m_cActiveRockets )
	{
		// can't put away while guiding a missile.
		return false;
	}
	return true;
}

void CRpg::NativeAttack(bool altMode)
{
#if !CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -8.0f;

	CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );// RpgRocket::Create stomps on globals, so remake.
	pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
#endif
}

void CRpg::GetWeaponData(weapon_data_t& data)
{
	CConfigurableWeapon::GetWeaponData(data);
	data.iuser3 |= (m_cActiveRockets & 0xF) << 12;
}

void CRpg::SetWeaponData(const weapon_data_t& data)
{
	CConfigurableWeapon::SetWeaponData(data);
	m_cActiveRockets = (data.iuser3 >> 12) & 0xF;
}
