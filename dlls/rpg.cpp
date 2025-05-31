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
#include "monsters.h"
#include "weapons.h"
#include "player.h"

LINK_WEAPON_TO_CLASS( weapon_rpg, CRpg )

#if !CLIENT_DLL
#include "gamerules.h"
#include "rpgrocket.h"
#include "visuals_utils.h"

LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot )

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
void CLaserSpot::Spawn( void )
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
void CLaserSpot::Revive( void )
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

void CLaserSpot::Precache( void )
{
	PRECACHE_MODEL( "sprites/laserdot.spr" );
}

LINK_ENTITY_TO_CLASS( rpg_rocket, CRpgRocket )

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

CRpg *CRpgRocket::GetLauncher( void )
{
	return m_hLauncher.Entity<CRpg>();
}

//=========================================================
//=========================================================
void CRpgRocket::Spawn( void )
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
void CRpgRocket::Precache( void )
{
	PrecacheBaseGrenadeSounds();
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	RegisterVisual(trailVisual);
	RegisterAndPrecacheSoundScript(rocketIgniteSoundScript);
}

void CRpgRocket::IgniteThink( void )
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

void CRpgRocket::FollowThink( void )
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
			if( pev->velocity.Length() > 300.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 300.0f;
			}
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 4 );
		} 
		else 
		{
			if( pev->velocity.Length() > 2000.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 2000.0f;
			}
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
		if( pev->waterlevel == WL_NotInWater && pev->velocity.Length() < 1500.0f )
			Detonate();
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	if( CRpg *pLauncher = GetLauncher())
	{
		if( ( pev->origin - pLauncher->pev->origin ).Length() > 8192 || gpGlobals->time - m_flIgniteTime > 6.0f )
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

void CRpg::Reload( void )
{
	// don't bother with any of this if don't need to reload.
	if( !CanReload() )
		return;

	// because the RPG waits to autoreload when no missiles are active while  the LTD is on, the
	// weapons code is constantly calling into this function, but is often denied because 
	// a) missiles are in flight, but the LTD is on
	// or
	// b) player is totally out of ammo and has nothing to switch to, and should be allowed to
	//    shine the designator around
	//
	// Set the next attack time into the future so that WeaponIdle will get called more often
	// than reload, allowing the RPG LTD to be updated
	
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );

	if( m_cActiveRockets && m_fSpotActive )
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit. 
		return;
	}

#if !CLIENT_DLL
	if( m_pSpot && m_fSpotActive )
	{
		m_pSpot->Suspend( 2.1f );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.1f;
	}
#endif

	bool iResult = false;
	if( m_iClip < iMaxClip() )
		iResult = DefaultClipReload( RPG_RELOAD, 2 );

	if( iResult )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CRpg::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), MyWModel() );
	m_fSpotActive = 1;

	if( bIsMultiplayer() )
	{
		// more default ammo in multiplay. 
		InitDefaultAmmo(RPG_DEFAULT_GIVE * 2);
	}
	else
	{
		InitDefaultAmmo(RPG_DEFAULT_GIVE);
	}
	InitMaxClip(RPG_MAX_CLIP);

	FallInit();// get ready to fall down.
}

void CRpg::Precache( void )
{
	PRECACHE_MODEL( MyWModel() );
	PRECACHE_MODEL( "models/v_rpg.mdl" );
	PrecachePModel( "models/p_rpg.mdl" );

	UTIL_PrecacheOther( "laser_spot" );
	UTIL_PrecacheOther( "rpg_rocket" );

	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" ); // alternative fire sound

	m_usRpg = PRECACHE_EVENT( 1, "events/rpg.sc" );
}

bool CRpg::GetItemInfo( ItemInfo *p )
{
	p->pszAmmo1 = AmmoName("rockets");
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

bool CRpg::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

bool CRpg::Deploy()
{
	if( Emptied() )
	{
		return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW_UL, "rpg" );
	}

	return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW1, "rpg" );
}

bool CRpg::CanHolster( void )
{
	if( m_fSpotActive && m_cActiveRockets )
	{
		// can't put away while guiding a missile.
		return false;
	}
	return true;
}

void CRpg::Holster()
{
	m_fInReload = false;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	SendWeaponAnim( RPG_HOLSTER1 );

#if !CLIENT_DLL
	if( m_pSpot )
	{
		m_pSpot->Killed( NULL, NULL, GIB_NEVER );
		m_pSpot = NULL;
	}
#endif
}

void CRpg::PrimaryAttack()
{
	if( HasAmmoToFire() )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#if !CLIENT_DLL
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -8.0f;

		CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );// RpgRocket::Create stomps on globals, so remake.
		pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
#endif

		// firing RPG no longer turns on the designator. ALT fire is a toggle switch for the LTD.
		// Ken signed up for this as a global change (sjb)

		int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usRpg );

		SpendAmmo();

		m_flNextPrimaryAttack = GetNextAttackDelay( 1.5f );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;

		ResetEmptySound();
	}
	else
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
	}
	UpdateSpot();
}

void CRpg::SecondaryAttack()
{
	m_fSpotActive = !m_fSpotActive;

#if !CLIENT_DLL
	if( !m_fSpotActive && m_pSpot )
	{
		m_pSpot->Killed( NULL, NULL, GIB_NORMAL );
		m_pSpot = NULL;
	}
#endif
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
}

void CRpg::WeaponIdle( void )
{
	UpdateSpot();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		ResetEmptySound();

		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
		if( flRand <= 0.75f || m_fSpotActive )
		{
			if( Emptied() )
				iAnim = RPG_IDLE_UL;
			else
				iAnim = RPG_IDLE;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0f / 15.0f;
		}
		else
		{
			if( Emptied() )
				iAnim = RPG_FIDGET_UL;
			else
				iAnim = RPG_FIDGET;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.1f;
		}

		SendWeaponAnim( iAnim );
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	}
}

void CRpg::UpdateSpot( void )
{
#if !CLIENT_DLL
	if( m_fSpotActive )
	{
		if (m_pPlayer->pev->viewmodel == 0)
			return;

		if( !m_pSpot )
		{
			m_pSpot = CLaserSpot::CreateSpot(m_pPlayer->edict());
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 8192.0f, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
#endif
}

void CRpg::GetWeaponData(weapon_data_t& data)
{
	data.fuser2 = static_cast<float>(m_fSpotActive);
	data.fuser3 = m_cActiveRockets;
}

void CRpg::SetWeaponData(const weapon_data_t& data)
{
	m_fSpotActive = data.fuser2 != 0;
	m_cActiveRockets = data.fuser3;
}
