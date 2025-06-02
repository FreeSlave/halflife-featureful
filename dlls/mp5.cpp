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
#include "soundent.h"

#if !CLIENT_DLL
#include "gamerules.h"
#endif

LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 )
LINK_WEAPON_TO_CLASS( weapon_9mmAR, CMP5 )

//=========================================================
//=========================================================

void CMP5::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_9mmAR" ); // hack to allow for old names
	Precache();
	SET_MODEL( ENT( pev ), MyWModel() );

	InitDefaultAmmo(MP5_DEFAULT_GIVE);
	InitMaxClip(MP5_MAX_CLIP);

	if( bIsMultiplayer() )
		m_iDefaultAmmo = MP5_DEFAULT_GIVE_MP;

	FallInit();// get ready to fall down.
}

void CMP5::Precache( void )
{
	PRECACHE_MODEL( "models/v_9mmAR.mdl" );
	PRECACHE_MODEL( MyWModel() );
	PrecachePModel( "models/p_9mmAR.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shellTE_MODEL

	PRECACHE_MODEL( "models/grenade.mdl" );	// grenade

	PRECACHE_MODEL( "models/w_9mmARclip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );

	PRECACHE_SOUND( "weapons/hks1.wav" );// H to the K
	PRECACHE_SOUND( "weapons/hks2.wav" );// H to the K
	PRECACHE_SOUND( "weapons/hks3.wav" );// H to the K

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );

	m_usMP5 = PRECACHE_EVENT( 1, "events/mp5.sc" );
	m_usMP52 = PRECACHE_EVENT( 1, "events/mp52.sc" );
}

bool CMP5::GetItemInfo( ItemInfo *p )
{
	p->pszAmmo1 = AmmoName("9mm");
	p->pszAmmo2 = "ARgrenades";
	p->iSlot = 2;
	p->iPosition = 0;
	p->pszAmmoEntity = "ammo_9mmAR";
	p->iDropAmmo = AMMO_MP5CLIP_GIVE;

	return true;
}

bool CMP5::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

bool CMP5::Deploy()
{
	return DefaultDeploy( "models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DEPLOY, "mp5" );
}

void CMP5::PrimaryAttack()
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
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	SpendAmmo();

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	// optimized multiplayer. Widened to make it easier to hit a moving player
	const Vector vecSpread = bIsMultiplayer() ? VECTOR_CONE_6DEGREES : VECTOR_CONE_3DEGREES;
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( PlaybackFlags(), m_pPlayer->edict(), m_usMP5, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	CheckOutOfAmmo();

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.1f );

	if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CMP5::SecondaryAttack( void )
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15f;
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2f;

	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
#if !CLIENT_DLL
	CGrenade::ShootContact( m_pPlayer->pev,
					m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16.0f,
					gpGlobals->v_forward * 800.0f );
#endif

	PLAYBACK_EVENT( PlaybackFlags(), m_pPlayer->edict(), m_usMP52 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;// idle pretty soon after shooting.

	if( !m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", false, 0 );
}

void CMP5::Reload( void )
{
	if( !CanReload() )
		return;

	DefaultClipReload( MP5_RELOAD, 1.5f );
}

void CMP5::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = MP5_LONGIDLE;	
		break;
	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
