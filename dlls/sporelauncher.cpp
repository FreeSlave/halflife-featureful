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
#ifndef CLIENT_DLL
#include "spore.h"
#include "game.h"
#include "gamerules.h"
#endif

#if FEATURE_SPORELAUNCHER

LINK_WEAPON_TO_CLASS(weapon_sporelauncher, CSporelauncher)

void CSporelauncher::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), MyWModel());

	InitDefaultAmmo(SPORELAUNCHER_DEFAULT_GIVE);
	InitMaxClip(SPORELAUNCHER_MAX_CLIP);

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;

	FallInit();// get ready to fall
}


void CSporelauncher::Precache(void)
{
	PRECACHE_MODEL("models/v_spore_launcher.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_spore_launcher.mdl");

	PRECACHE_SOUND("weapons/splauncher_altfire.wav");
	PRECACHE_SOUND("weapons/splauncher_bounce.wav");
	PRECACHE_SOUND("weapons/splauncher_fire.wav");
	PRECACHE_SOUND("weapons/splauncher_impact.wav");
	PRECACHE_SOUND("weapons/splauncher_pet.wav");
	PRECACHE_SOUND("weapons/splauncher_reload.wav");

	PRECACHE_MODEL("sprites/bigspit.spr");
	m_iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");
	UTIL_PrecacheOther("spore");

	m_usSporeFire = PRECACHE_EVENT(1, "events/spore.sc");
}

bool CSporelauncher::AddToPlayer(CBasePlayer *pPlayer)
{
	return AddToPlayerDefault(pPlayer);
}


bool CSporelauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "spores";
	p->pszAmmo2 = NULL;
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 6;
	p->iPosition = 0;
#else
	p->iSlot = 3;
	p->iPosition = 5;
#endif
	p->iFlags = 0;
	p->pszAmmoEntity = NULL;
	p->iDropAmmo = 0;

	return true;
}



bool CSporelauncher::Deploy()
{
	return DefaultDeploy("models/v_spore_launcher.mdl", "models/p_spore_launcher.mdl", SPLAUNCHER_DRAW1, "rpg");
}

void CSporelauncher::PrimaryAttack()
{
	if (m_iClip <= 0)
		return;

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif


	// m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

#ifndef CLIENT_DLL
		Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		CSpore::ShootContact( m_pPlayer, vecSrc, vecAngles, gpGlobals->v_forward * CSpore::SporeRocketSpeed() );
#endif

	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(),
		m_usSporeFire,
		0.0,
		g_vecZero,
		g_vecZero,
		vecSrc.x,
		vecSrc.y,
		(int)vecSrc.z,
		m_iSquidSpitSprite,
		0,
		1);


	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

#if 1
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
#endif

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	m_fInSpecialReload = 0;
}


void CSporelauncher::SecondaryAttack(void)
{
	if (m_iClip <= 0)
		return;

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	//m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

#ifndef CLIENT_DLL
	Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	CSpore::ShootTimed(m_pPlayer, vecSrc, vecAngles, m_pPlayer->pev->velocity + gpGlobals->v_forward * CSpore::SporeGrenadeSpeed());
#endif

	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(),
		m_usSporeFire,
		0.0,
		g_vecZero,
		g_vecZero,
		vecSrc.x,
		vecSrc.y,
		(int)vecSrc.z,
		m_iSquidSpitSprite,
		0,
		0);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

#if 1
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
#endif

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInSpecialReload = 0;

}


void CSporelauncher::Reload(void)
{
	if (!CanReload())
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(SPLAUNCHER_RELOAD_REACH);
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.7;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		// Play reload sound.
		EMIT_SOUND(m_pPlayer->edict(), CHAN_ITEM, "weapons/splauncher_reload.wav", 0.7f, ATTN_NORM);

		SendWeaponAnim(SPLAUNCHER_RELOAD_LOAD);

		m_flNextReload = UTIL_WeaponTimeBase() + 1.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CSporelauncher::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != SPORELAUNCHER_DEFAULT_GIVE && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim(SPLAUNCHER_RELOAD_AIM);

				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.8;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.75f)
			{
				iAnim = SPLAUNCHER_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
			}
			else if (flRand <= 0.95f)
			{
				iAnim = SPLAUNCHER_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
			} else {
				iAnim = SPLAUNCHER_FIDGET;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
				EMIT_SOUND(m_pPlayer->edict(), CHAN_ITEM, "weapons/splauncher_pet.wav", 0.7f, ATTN_NORM);
			}

			SendWeaponAnim(iAnim);
		}
	}
}
#endif
