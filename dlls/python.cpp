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
#include "weapons.h"
#include "monsters.h"
#include "player.h"

#if !CLIENT_DLL
#include "gamerules.h"
#endif

LINK_ENTITY_TO_CLASS( weapon_python, CPython )
LINK_WEAPON_TO_CLASS( weapon_357, CPython )

bool CPython::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "357";
	p->pszAmmo2 = NULL;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->pszAmmoEntity = "ammo_357";
	p->iDropAmmo = AMMO_357BOX_GIVE;

	return true;
}

bool CPython::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

void CPython::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_357" ); // hack to allow for old names
	Precache();
	SET_MODEL( ENT( pev ), MyWModel() );

	InitDefaultAmmo(PYTHON_DEFAULT_GIVE);
	InitMaxClip(PYTHON_MAX_CLIP);

	FallInit();// get ready to fall down.
}

void CPython::Precache( void )
{
	PRECACHE_MODEL( "models/v_357.mdl" );
	PRECACHE_MODEL( MyWModel() );
	PrecachePModel( "models/p_357.mdl" );

	PRECACHE_MODEL( "models/w_357ammobox.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/357_reload1.wav" );
	PRECACHE_SOUND( "weapons/357_shot1.wav" );
	PRECACHE_SOUND( "weapons/357_shot2.wav" );

	m_usFirePython = PRECACHE_EVENT( 1, "events/python.sc" );
}

bool CPython::Deploy()
{
	pev->body = ViewModelBody();
	return DefaultDeploy( "models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", pev->body );
}

void CPython::Holster()
{
	m_fInReload = false;// cancel any reload in progress.

	if( InZoom() )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
	SendWeaponAnim( PYTHON_HOLSTER );
}

void CPython::SecondaryAttack( void )
{
	if( !bIsMultiplayer() )
	{
		return;
	}

	if( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else if( m_pPlayer->pev->fov != 40 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 40;
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CPython::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if( !HasAmmoToFire() )
	{
		if( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	SpendAmmo();

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFirePython, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	CheckOutOfAmmo();

	m_flNextPrimaryAttack = 0.75f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
}

void CPython::Reload( void )
{
	if( !CanReload() )
		return;

	if( InZoom() )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	if( DefaultClipReload( PYTHON_RELOAD, 2.0f, ViewModelBody() ) )
	{
		m_flSoundDelay = 1.5f;
	}
}

void CPython::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if( m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT( 0.8f, 0.9f ), ATTN_NORM );
		m_flSoundDelay = 0.0f;
	}

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	if( flRand <= 0.5f )
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 70.0f / 30.0f );
	}
	else if( flRand <= 0.7f )
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 60.0f / 30.0f );
	}
	else if( flRand <= 0.9f )
	{
		iAnim = PYTHON_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 88.0f / 30.0f );
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 170.0f / 30.0f );
	}

	SendWeaponAnim( iAnim, ViewModelBody() );
}

int CPython::ViewModelBody()
{
	return bIsMultiplayer() ? 1 : 0;
}
