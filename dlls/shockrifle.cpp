/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "mod_features.h"
#if !CLIENT_DLL
#include "shockbeam.h"
#include "game.h"
#include "gamerules.h"
#endif

#if FEATURE_SHOCKRIFLE

LINK_WEAPON_TO_CLASS(weapon_shockrifle, CShockrifle)

void CShockrifle::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), MyWModel());

	int defaultAmmoGive = g_AmmoRegistry.GetMaxAmmo("shocks");
	if (defaultAmmoGive <= 0)
		defaultAmmoGive = SHOCKRIFLE_DEFAULT_GIVE;
	InitDefaultAmmo(defaultAmmoGive);
	InitMaxClip(WEAPON_NOCLIP);
	m_iFirePhase = 0;

	FallInit();// get ready to fall down.

	pev->sequence = 0;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;
}


void CShockrifle::Precache(void)
{
	PRECACHE_MODEL("models/v_shock.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_shock.mdl");

	PRECACHE_SOUND("weapons/shock_discharge.wav");
	PRECACHE_SOUND("weapons/shock_draw.wav");
	PRECACHE_SOUND("weapons/shock_fire.wav");
	PRECACHE_SOUND("weapons/shock_impact.wav");
	PRECACHE_SOUND("weapons/shock_recharge.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/flare3.spr");

	m_usShockFire = PRECACHE_EVENT(1, "events/shock.sc");

	UTIL_PrecacheOther("shock_beam");
}

bool CShockrifle::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{

#if !CLIENT_DLL
		if (g_pGameRules->IsMultiplayer())
		{
			// in multiplayer, all hivehands come full.
			pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = g_AmmoRegistry.GetMaxAmmo(PrimaryAmmoIndex());
		}
#endif

		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(WeaponId());
		MESSAGE_END();
		return true;
	}
	return false;
}

bool CShockrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Shocks";
	p->pszAmmo2 = NULL;
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 6;
	p->iPosition = 1;
#else
	p->iSlot = 3;
	p->iPosition = 4;
#endif
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;
	p->pszAmmoEntity = NULL;
	p->iDropAmmo = 0;

	return true;
}

bool CShockrifle::Deploy()
{
	if( bIsMultiplayer() )
		m_flRechargeTime = gpGlobals->time + 0.25;
	else
		m_flRechargeTime = gpGlobals->time + 0.5;

	return DefaultDeploy("models/v_shock.mdl", "models/p_shock.mdl", SHOCK_DRAW, "bow");
}

void CShockrifle::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(SHOCK_HOLSTER);
	ClearBeams();

	//!!!HACKHACK - can't select shockrifle if it's empty! no way to get ammo for it, either.
	if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}

void CShockrifle::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	if (m_pPlayer->pev->waterlevel == WL_Eyes)
	{
#if !CLIENT_DLL
		int attenuation = 150 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		int dmg = 100 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_discharge.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
		RadiusDamage(m_pPlayer->pev->origin, m_pPlayer->pev, m_pPlayer->pev, dmg, attenuation, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
#endif
		return;
	}

	CreateChargeEffect();

#if !CLIENT_DLL
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;

	const Vector vecSrc =
		m_pPlayer->GetGunPosition() +
		gpGlobals->v_forward * 8 +
		gpGlobals->v_right * 9 +
		gpGlobals->v_up * -7;

	m_pPlayer->GetAutoaimVectorFromPoint(vecSrc, AUTOAIM_10DEGREES);

	CShock::Shoot(m_pPlayer->pev, anglesAim, vecSrc, gpGlobals->v_forward * CShock::ShockSpeed());

	m_flRechargeTime = gpGlobals->time + 1.0f;
#endif
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;


	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usShockFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	if( bIsMultiplayer() )
		m_flNextPrimaryAttack = GetNextAttackDelay(0.1);
	else
		m_flNextPrimaryAttack = GetNextAttackDelay(0.2);

	SetThink( &CShockrifle::ClearBeams );
	pev->nextthink = gpGlobals->time + 0.08;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.33;
}

void CShockrifle::SecondaryAttack( void )
{
	CBasePlayerWeapon::SecondaryAttack();
}

void CShockrifle::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= g_AmmoRegistry.GetMaxAmmo(m_iPrimaryAmmoType))
		return;

	while (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < g_AmmoRegistry.GetMaxAmmo(m_iPrimaryAmmoType) && m_flRechargeTime < gpGlobals->time)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/shock_recharge.wav", 1, ATTN_NORM);

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
			m_flRechargeTime += 0.25;
		else
			m_flRechargeTime += 0.5;
#endif
	}
}


void CShockrifle::WeaponIdle(void)
{
	Reload();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.8) {
		SendWeaponAnim(SHOCK_IDLE3);
	} else {
		SendWeaponAnim(SHOCK_IDLE1);
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.3f;
}

void CShockrifle::CreateChargeEffect( void )
{
#if !CLIENT_DLL
	if( g_pGameRules->IsMultiplayer())
		return;
	int iBeam = 0;

	for( int i = 2; i < 5; i++)
	{
		if( !m_pBeam[iBeam] )
			m_pBeam[iBeam] = CBeam::BeamCreate("sprites/lgtning.spr", 16);
		m_pBeam[iBeam]->EntsInit( m_pPlayer->entindex(), m_pPlayer->entindex() );
		m_pBeam[iBeam]->SetStartAttachment(1);
		m_pBeam[iBeam]->SetEndAttachment(i);
		m_pBeam[iBeam]->SetNoise( 75 );
		m_pBeam[iBeam]->pev->scale= 10;
		m_pBeam[iBeam]->SetColor( 0, 253, 253 );
		m_pBeam[iBeam]->SetScrollRate( 30 );
		m_pBeam[iBeam]->SetBrightness( 190 );
		iBeam++;
	}
#endif
}

void CShockrifle::ClearBeams( void )
{
#if !CLIENT_DLL
	if( g_pGameRules->IsMultiplayer())
		return;

	for( int i = 0; i < 3; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	SetThink( NULL );
#endif
}
#endif
