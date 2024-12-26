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
#if !CLIENT_DLL
#include "game.h"
#include "gamerules.h"
#endif

#if FEATURE_M249

LINK_WEAPON_TO_CLASS(weapon_m249, CM249)

//=========================================================
//=========================================================

void CM249::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), MyWModel());

	InitDefaultAmmo(M249_DEFAULT_GIVE);
	InitMaxClip(M249_MAX_CLIP);

	m_fInSpecialReload = 0;
	m_bAlternatingEject = false;

	FallInit();// get ready to fall down.
}


void CM249::Precache(void)
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");// brass shellTE_MODEL
	m_iLink = PRECACHE_MODEL("models/saw_link.mdl");

	PRECACHE_MODEL("models/w_saw_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/saw_fire1.wav");

	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");

	m_usM249 = PRECACHE_EVENT(1, "events/m249.sc");
}

bool CM249::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->pszAmmo2 = NULL;
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 5;
	p->iPosition = 0;
#else
	p->iSlot = 2;
	p->iPosition = 3;
#endif
	p->iFlags = 0;
	p->iWeight = M249_WEIGHT;
	p->pszAmmoEntity = "ammo_556";
	p->iDropAmmo = AMMO_762BOX_GIVE;

	return true;
}

bool CM249::AddToPlayer(CBasePlayer *pPlayer)
{
	return AddToPlayerDefault(pPlayer);
}

bool CM249::Deploy()
{
	m_fInSpecialReload = 0;
	UpdateTape();
	return DefaultDeploy("models/v_saw.mdl", "models/p_saw.mdl", M249_DEPLOY, "mp5", pev->body);
}

void CM249::Holster()
{
	m_fInSpecialReload = 0;
	CBasePlayerWeapon::Holster();
}

void CM249::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == WL_Eyes)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -1.0f, 1.0f );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -2.0f, 2.0f );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	UpdateTape();
	m_bAlternatingEject = !m_bAlternatingEject;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	// optimized multiplayer. Widened to make it easier to hit a moving player
	const Vector vecSpread = bIsMultiplayer() ? VECTOR_CONE_6DEGREES : VECTOR_CONE_3DEGREES;
	Vector vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usM249, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, pev->body, m_bAlternatingEject ? 1 : 0, 0);


#if !CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecVelocity = m_pPlayer->pev->velocity;
	Vector vecInvPushDir = gpGlobals->v_forward * 35.0;

	float flNewZVel;

	if( vecInvPushDir.z >= 10.0 )
		flNewZVel = vecInvPushDir.z;
	else
		flNewZVel = g_psv_maxspeed->value;

	Vector vecNewVel;

	if( g_pGameRules->IsDeathmatch() )
	{
		vecNewVel = vecVelocity - vecInvPushDir;
	}
	else
	{
		vecNewVel = vecVelocity;

		float flZTreshold = -( flNewZVel + 100.0 );

		if( vecVelocity.x > flZTreshold )
		{
			vecNewVel.x -= vecInvPushDir.x;
		}

		if( vecVelocity.y > flZTreshold )
		{
			vecNewVel.y -= vecInvPushDir.y;
		}
	}
	m_pPlayer->pev->velocity = vecNewVel;
#endif

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.067);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2f;
}


void CM249::Reload(void)
{
	if (!CanReload())
		return;

	if (DefaultClipReload( M249_LAUNCH, 1.33, pev->body)) {
		m_fInSpecialReload = 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.78;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.78;
	}
}

void CM249::ItemPostFrame()
{
	if (!m_fInReload)
	{
		m_iVisibleClip = m_iClip;
	}
	if ( m_fInSpecialReload )
	{
		if (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase())
		{
			int maxClip = iMaxClip();
			m_iVisibleClip = m_iClip + Q_min( maxClip - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] );

			UpdateTape(m_iVisibleClip);
			m_fInSpecialReload = 0;
			SendWeaponAnim( M249_RELOAD1, pev->body );
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 2.4;
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CM249::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	UpdateTape(m_iVisibleClip);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	int iAnim;
	if (flRand <= 0.8) {
		iAnim = M249_SLOWIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
	} else {
		iAnim = M249_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 155.0/25.0;
	}

	SendWeaponAnim(iAnim, pev->body);
}

void CM249::UpdateTape()
{
	UpdateTape(m_iClip);
	m_iVisibleClip = m_iClip;
}

void CM249::UpdateTape(int clip)
{
	pev->body = BodyFromClip(clip);
}

int CM249::BodyFromClip()
{
	return BodyFromClip(m_iVisibleClip);
}

int CM249::BodyFromClip(int clip)
{
	if (clip == 0) {
		return 8;
	} else if (clip > 0 && clip <= 8) {
		return 9 - clip;
	} else {
		return 0;
	}
}

void CM249::GetWeaponData(weapon_data_t &data)
{
	data.iuser1 = m_iVisibleClip;
}

void CM249::SetWeaponData(const weapon_data_t &data)
{
	UpdateTape(data.iuser1);
}

int CM249::ViewModelBody()
{
	return BodyFromClip();
}

#endif
