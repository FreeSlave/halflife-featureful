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

#define	HANDGRENADE_PRIMARY_VOLUME		450

enum handgrenade_e
{
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
	HANDGRENADE_THROW2,	// medium
	HANDGRENADE_THROW3,	// hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};

LINK_WEAPON_TO_CLASS( weapon_handgrenade, CHandGrenade )

void CHandGrenade::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), MyWModel() );

#if !CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif
	InitDefaultAmmo(HANDGRENADE_DEFAULT_GIVE);
	InitMaxClip(WEAPON_NOCLIP);

	FallInit();// get ready to fall down.
}

void CHandGrenade::Precache( void )
{
	PRECACHE_MODEL( MyWModel() );
	PRECACHE_MODEL( "models/v_grenade.mdl" );
	PrecachePModel( "models/p_grenade.mdl" );
}

bool CHandGrenade::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hand Grenade";
	p->pszAmmo2 = NULL;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->pszAmmoEntity = STRING(pev->classname);
	p->iDropAmmo = HANDGRENADE_DEFAULT_GIVE;

	return true;
}

bool CHandGrenade::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_grenade.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "crowbar" );
}

bool CHandGrenade::CanHolster()
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenade::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		SendWeaponAnim( HANDGRENADE_HOLSTER );
	}
	else
	{
		// no more grenades!
		m_pPlayer->ClearWeaponBit(WEAPON_HANDGRENADE);
		DestroyItem();
	}

	if( m_flStartThrow )
	{
		m_flStartThrow = 0.0f;
		m_flReleaseThrow = 0.0f;
	}

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM );
}

void CHandGrenade::PrimaryAttack()
{
	if( !m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0.0f;

		SendWeaponAnim( HANDGRENADE_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
	}
}

bool CHandGrenade::PreferNewPhysics()
{
#if CLIENT_DLL
	extern cvar_t *cl_grenadephysics;
	if (cl_grenadephysics)
		return (int)cl_grenadephysics->value != 0;
	return false;
#else
	if (m_pPlayer)
		return m_pPlayer->m_iPreferNewGrenadePhysics != 0;
	return false;
#endif
}

void CHandGrenade::WeaponIdle( void )
{
	if( m_flReleaseThrow == 0.0f && m_flStartThrow )
		 m_flReleaseThrow = gpGlobals->time;

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_flStartThrow )
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if( angThrow.x < 0.0f )
			angThrow.x = -10.0f + angThrow.x * ( ( 90.0f - 10.0f ) / 90.0f );
		else
			angThrow.x = -10.0f + angThrow.x * ( ( 90.0f + 10.0f ) / 90.0f );

		float maxVel = 500.0f;
		float velVultiplier = 4.0f;
		if (PreferNewPhysics())
		{
			maxVel = 1000.0f;
			velVultiplier = 6.5f;
		}

		float flVel = ( 90.0f - angThrow.x ) * velVultiplier;
		if( flVel > maxVel )
			flVel = maxVel;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16.0f;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0f;
		if( time < 0.0f )
			time = 0.0f;

#if !CLIENT_DLL
		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );
#endif

		if( flVel < 500.0f )
		{
			SendWeaponAnim( HANDGRENADE_THROW1 );
		}
		else if( flVel < 1000.0f )
		{
			SendWeaponAnim( HANDGRENADE_THROW2 );
		}
		else
		{
			SendWeaponAnim( HANDGRENADE_THROW3 );
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		//m_flReleaseThrow = 0.0f;
		m_flStartThrow = 0.0f;
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );// ensure that the animation can finish playing
		}
		return;
	}
	else if( m_flReleaseThrow > 0.0f )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0.0f;

		if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			SendWeaponAnim( HANDGRENADE_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
		m_flReleaseThrow = -1.0f;
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
		if( flRand <= 0.75f )
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );// how long till we do this again.
		}
		else
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0f / 30.0f;
		}

		SendWeaponAnim( iAnim );
	}
}

void CHandGrenade::GetWeaponData(weapon_data_t& data)
{
	data.fuser2 = m_flStartThrow;
	data.fuser3 = m_flReleaseThrow;
}
void CHandGrenade::SetWeaponData(const weapon_data_t& data)
{
	m_flStartThrow = data.fuser2;
	m_flReleaseThrow = data.fuser3;
}
