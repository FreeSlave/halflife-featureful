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
#if !CLIENT_DLL
#include "game.h"
#include "gamerules.h"
#endif

#if FEATURE_UZI

LINK_ENTITY_TO_CLASS( weapon_uzi, CUzi )
LINK_ENTITY_TO_CLASS( weapon_uziakimbo, CUzi ) // Link to single uzi until akimbo is implemented

//=========================================================
//=========================================================
void CUzi::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), MyWModel() );

	InitDefaultAmmo(UZI_DEFAULT_GIVE);

	FallInit();// get ready to fall down.
}

void CUzi::Precache( void )
{
	PRECACHE_MODEL( "models/v_uzi.mdl" );
	PRECACHE_MODEL( MyWModel() );
	PrecachePModel( "models/p_uzi.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shellTE_MODEL

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/uzi/shoot1.wav" );
	PRECACHE_SOUND( "weapons/uzi/shoot2.wav" );
	PRECACHE_SOUND( "weapons/uzi/shoot3.wav" );

	PRECACHE_SOUND( "weapons/uzi/reload1.wav" );
	PRECACHE_SOUND( "weapons/uzi/reload2.wav" );
	PRECACHE_SOUND( "weapons/uzi/reload3.wav" );

	PRECACHE_SOUND( "weapons/uzi/deploy.wav" );
	PRECACHE_SOUND( "weapons/uzi/deploy1.wav" );
	PRECACHE_SOUND( "weapons/uzi/akimbo_pull2.wav" );

	m_usUzi = PRECACHE_EVENT( 1, "events/uzi.sc" );
}

bool CUzi::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->pszAmmo2 = NULL;
	p->iMaxClip = UZI_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iWeight = UZI_WEIGHT;
	p->pszAmmoEntity = "ammo_9mmclip";
	p->iDropAmmo = AMMO_GLOCKCLIP_GIVE;

	return true;
}

bool CUzi::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

bool CUzi::Deploy()
{
	int r = DefaultDeploy( "models/v_uzi.mdl", "models/p_uzi.mdl", UZI_DEPLOY, "mp5" );
	if (r)
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.2;
	}
	return r;
}

void CUzi::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	// single player spread
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_UZI, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usUzi, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", false, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.1 );

	if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CUzi::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == UZI_MAX_CLIP )
		return;

	DefaultReload( UZI_MAX_CLIP, UZI_RELOAD, 2.5 );
}

void CUzi::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if( flRand <= 0.6 )
	{
		iAnim = UZI_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
	}
	else if( flRand <= 0.7 )
	{
		iAnim = UZI_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 7;
	}
	else
	{
		iAnim = UZI_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6;
	}

	SendWeaponAnim( iAnim );
}
#endif
