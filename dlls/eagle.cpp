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
#endif

#if FEATURE_DESERT_EAGLE

LINK_ENTITY_TO_CLASS( weapon_eagle, CEagle )

#if !CLIENT_DLL
LINK_ENTITY_TO_CLASS( eagle_laser, CLaserSpot )
#endif

void CEagle::Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), MyWModel());

	InitDefaultAmmo(EAGLE_DEFAULT_GIVE);
	m_fEagleLaserActive = 0;
	m_pEagleLaser = 0;

	FallInit();// get ready to fall down.
}


void CEagle::Precache( void )
{
	UTIL_PrecacheOther( "eagle_laser" );
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_desert_eagle.mdl");
	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND ("weapons/desert_eagle_reload.wav");
	PRECACHE_SOUND ("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND ("weapons/desert_eagle_sight.wav");
	PRECACHE_SOUND ("weapons/desert_eagle_sight2.wav");

	m_usEagle = PRECACHE_EVENT( 1, "events/eagle.sc" );
}

bool CEagle::AddToPlayer(CBasePlayer *pPlayer)
{
	return AddToPlayerDefault(pPlayer);
}

bool CEagle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->pszAmmo2 = NULL;
	p->iMaxClip = EAGLE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iWeight = EAGLE_WEIGHT;
	p->pszAmmoEntity = "ammo_357";
	p->iDropAmmo = AMMO_357BOX_GIVE;

	return true;
}

bool CEagle::Deploy( )
{
	return DefaultDeploy( "models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", EAGLE_DRAW, "onehanded" );
}

void CEagle::Holster()
{
	m_fInReload = false;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( EAGLE_HOLSTER );

#if !CLIENT_DLL
	if (m_pEagleLaser)
	{
		m_pEagleLaser->Killed( NULL, NULL, GIB_NEVER );
		m_pEagleLaser = NULL;
	}
#endif
}

void CEagle::SecondaryAttack()
{
	bool wasActive = m_fEagleLaserActive;
	m_fEagleLaserActive = !m_fEagleLaserActive;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	if (wasActive)
	{
#if !CLIENT_DLL
		if (m_pEagleLaser)
		{
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			m_pEagleLaser->Killed( NULL, NULL, GIB_NORMAL );
			m_pEagleLaser = NULL;
		}
#endif
	}
}

void CEagle::PrimaryAttack()
{
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
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	float flSpread = 0.001;

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
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	if (m_fEagleLaserActive)
	{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_EAGLE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
#if !CLIENT_DLL
		if (m_pEagleLaser)
			m_pEagleLaser->Suspend( 0.6f );
#endif
	}
	else
	{
		flSpread = 0.1;
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_EAGLE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.22f;
	}

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEagle, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CEagle::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == EAGLE_MAX_CLIP)
	{
		return;
	}

	if ( m_pEagleLaser && m_fEagleLaserActive )
	{
#if !CLIENT_DLL
		m_pEagleLaser->Suspend( 1.6 );
#endif
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5f;
	}

	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( EAGLE_MAX_CLIP, EAGLE_RELOAD, 1.5f );
	else
		iResult = DefaultReload( EAGLE_MAX_CLIP, EAGLE_RELOAD_NOT_EMPTY, 1.5f );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CEagle::UpdateSpot( void )
{
#if !CLIENT_DLL
	if (m_fEagleLaserActive)
	{
		if (m_pPlayer->pev->viewmodel == 0)
			return;

		if (!m_pEagleLaser)
		{
			m_pEagleLaser = CLaserSpot::CreateSpot(m_pPlayer->edict());
			m_pEagleLaser->pev->classname = MAKE_STRING("eagle_laser");
			m_pEagleLaser->pev->scale = 0.5;
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );

		UTIL_SetOrigin( m_pEagleLaser->pev, tr.vecEndPos );
	}
#endif
}

void CEagle::ItemPostFrame()
{
	UpdateSpot();
	CBasePlayerWeapon::ItemPostFrame();
}

void CEagle::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (m_pEagleLaser)
		{
			if (flRand > 0.5f )
			{
				iAnim = EAGLE_IDLE5;//Done
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
			}
			else
			{
				iAnim = EAGLE_IDLE4;//Done
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5f;
			}
		}
		else
		{
			if (flRand <= 0.3f )
			{
				iAnim = EAGLE_IDLE1;//Done
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5f;
			}
			else if (flRand <= 0.6 )
			{
				iAnim = EAGLE_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5f;
			}
			else
			{
				iAnim = EAGLE_IDLE3;//Done
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.63f;
			}
		}
		SendWeaponAnim( iAnim );
	}
}

void CEagle::GetWeaponData(weapon_data_t& data)
{
	data.iuser1 = m_fEagleLaserActive;
}

void CEagle::SetWeaponData(const weapon_data_t& data)
{
	m_fEagleLaserActive = data.iuser1;
}

#endif
