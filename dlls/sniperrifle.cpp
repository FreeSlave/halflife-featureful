/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
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
#endif

#if FEATURE_SNIPERRIFLE

LINK_WEAPON_TO_CLASS( weapon_sniperrifle, CSniperrifle )

void CSniperrifle::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), MyWModel());

	InitDefaultAmmo(SNIPERRIFLE_DEFAULT_GIVE);
	InitMaxClip(SNIPERRIFLE_MAX_CLIP);

	FallInit();// get ready to fall down.
}


void CSniperrifle::Precache( void )
{
	PRECACHE_MODEL("models/v_m40a1.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_m40a1.mdl");

	PRECACHE_SOUND ("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND ("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND ("weapons/sniper_fire.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_second_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload3.wav");
	PRECACHE_SOUND ("weapons/sniper_zoom.wav");

	m_usSniper = PRECACHE_EVENT( 1, "events/sniper.sc" );
}

bool CSniperrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762";
	p->pszAmmo2 = NULL;
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 5;
	p->iPosition = 2;
#else
	p->iSlot = 2;
	p->iPosition = 4;
#endif
	p->iFlags = 0;
	p->iWeight = 10;
	p->pszAmmoEntity = "ammo_762";
	p->iDropAmmo = AMMO_762BOX_GIVE;

	return true;
}

bool CSniperrifle::Deploy( )
{
	return DefaultDeploy( "models/v_m40a1.mdl", "models/p_m40a1.mdl", SNIPER_DRAW, "bow", 0 );
}

bool CSniperrifle::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

void CSniperrifle::Holster()
{
	m_fInReload = false;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	SendWeaponAnim( SNIPER_HOLSTER );

	if ( InZoom() )
	{
		SecondaryAttack( );
	}
}

void CSniperrifle::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}
	else if ( m_pPlayer->pev->fov != 15 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 15;
	}

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}
void CSniperrifle::PrimaryAttack()
{
	if ( m_fInSpecialReload )
		return;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		}

		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == WL_Eyes)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15f;
		return;
	}

	float flSpread = 0.001f;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;

	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_762, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	m_flNextPrimaryAttack = 1.75f;
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSniper, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, ( m_iClip == 0 ) ? 1 : 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	// HEV suit - indicate out of ammo condition
	m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 68.0f / 38.0f;
}


void CSniperrifle::Reload( void )
{
	if (!CanReload())
		return;

	int iResult;

	if ( InZoom() )
	{
		SecondaryAttack();
	}

	if (m_iClip == 0)
	{
		iResult = DefaultClipReload( SNIPER_RELOAD1, 80.0f / 34.0f );
		m_fInSpecialReload = 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.25;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.25;
	}
	else
	{
		iResult = DefaultClipReload( SNIPER_RELOAD3, 2.25f );
	}
}
void CSniperrifle::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_fInSpecialReload )
	{
		m_fInSpecialReload = 0;
		SendWeaponAnim( SNIPER_RELOAD2 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 49.0f / 27.0f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 27.0f;
	}
	else
	{
		int iAnim;
		if (m_iClip <= 0)
		{
			iAnim = SNIPER_SLOWIDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0f / 16.0f;
		}
		else
		{
			iAnim = SNIPER_SLOWIDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 67.5f / 16.0f;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}

#endif
