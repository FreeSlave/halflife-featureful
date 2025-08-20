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

#if FEATURE_MEDKIT

LINK_WEAPON_TO_CLASS(weapon_medkit, CMedkit)

CBaseEntity* CMedkit::FindHealTarget(bool increasedRadius)
{
#if !CLIENT_DLL
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * (increasedRadius ? 48.0f : 32.0f);

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
			{
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer );
			}
		}
	}

	if (!FNullEnt( tr.pHit )) {
		CBaseEntity* pEntity = CBaseEntity::Instance( tr.pHit );
		if( pEntity && pEntity->IsAlive() && pEntity->pev->health < pEntity->pev->max_health) {
			CBaseEntity* foundTarget = NULL;
			if (pEntity->IsPlayer()) {
				foundTarget = pEntity;
			}
			else
			{
				CBaseMonster* monster = pEntity->MyMonsterPointer();
				if (monster && monster->IDefaultRelationship(m_pPlayer) == R_AL) {
					foundTarget = pEntity;
				}
			}
			return foundTarget;
		}
	}

#endif
	return NULL;
}

void CMedkit::Precache()
{
	CConfigurableWeapon::Precache();

	PRECACHE_SOUND("items/medshot4.wav");
	PRECACHE_SOUND("items/medshot5.wav");
	PRECACHE_SOUND("items/medshotno1.wav");

	m_usMedkitFire = PRECACHE_EVENT(1, "events/medkit.sc");
}

bool CMedkit::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 0;
	p->iPosition = 4;

	return true;
}

WeaponParameters CMedkit::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 50;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "Medicine";

	params.worldModel = "models/w_medkit.mdl";
	params.viewModel = "models/v_medkit.mdl";
	params.playerModel = "models/p_medkit.mdl";
	params.playerAnimExt = "trip";
	params.priority = -1;

	params.deploy.animIndex = MEDKIT_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{MEDKIT_LONGIDLE, 0.75f, 72.0f / 30.0f},
		WeaponParameters::IdleAnim{MEDKIT_IDLE, 0.25f, 36.0f / 30.0f}
	};

	params.fire.emptySound.main = {
		CHAN_WEAPON,
		{"items/medshotno1.wav"},
		1.0f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.useStandardEmptySound.main = false;

	params.holster.animIndex = MEDKIT_HOLSTER;
	params.holster.attackDelay = 0.5f;

	return params;
}

bool CMedkit::Deploy()
{
	m_flSoundDelay = 0;
	return PerformDeploy();
}

void CMedkit::Holster()
{
	m_flSoundDelay = 0;

	//HACKHACK - can't select medkit if it's empty! no way to get ammo for it, either
	if( CanRecharge() && !HasAmmoToFire() ) {
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}

	CConfigurableWeapon::Holster();
}

void CMedkit::PrimaryAttack(void)
{
	Reload();

	CBaseEntity* healTarget;
	if (HasAmmoToFire() && (healTarget = FindHealTarget()) ) {
		//
	} else {
		PlayEmptySound(false);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8;
		return;
	}

	m_secondaryAttack = false;
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usMedkitFire, 0.0, g_vecZero, g_vecZero, 0, 0, 0, 0, 0, 0);
	float delay = GetNextAttackDelay(2);
	if (delay < UTIL_WeaponTimeBase())
		delay = UTIL_WeaponTimeBase() + 2;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = delay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5, 10);
	m_flSoundDelay = gpGlobals->time + 0.4;
}

void CMedkit::SecondaryAttack()
{
	Reload();

	if (!HasAmmoToFire() || m_pPlayer->pev->health >= m_pPlayer->pev->max_health) {
		PlayEmptySound(true);
		m_flNextSecondaryAttack = GetNextAttackDelay(0.8);
		return;
	}

	m_secondaryAttack = true;

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usMedkitFire, 0.0, g_vecZero, g_vecZero, 0, 0, 1, 0.0, 0, 0.0);
	float delay = GetNextAttackDelay(3);
	if (delay < UTIL_WeaponTimeBase())
		delay = UTIL_WeaponTimeBase() + 3;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = delay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5, 10);
	m_flSoundDelay = gpGlobals->time + 1;
}

void CMedkit::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] >= MEDKIT_MAX_CARRY )
		return;
	if( CanRecharge() && m_flRechargeTime < gpGlobals->time )
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]++;
		m_flRechargeTime = gpGlobals->time + gSkillData.plrMedkitTime;
	}
}

void CMedkit::WeaponIdle(void)
{
	Reload();
	ResetEmptySound();

	if (HasAmmoToFire() && m_flSoundDelay != 0 && m_flSoundDelay <= gpGlobals->time)
	{
		const int maxHeal = Q_min((int)gSkillData.plrDmgMedkit, m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]);
		if (m_secondaryAttack) {
			const int diff = (int)ceil(m_pPlayer->pev->max_health - m_pPlayer->pev->health);
			const int healResult = m_pPlayer->TakeHealth(m_pPlayer, Q_min(maxHeal, diff), DMG_GENERIC);
			SpendAmmo(healResult);
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/medshot5.wav", 1.0, ATTN_NORM, 0, 100);
		} else {
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

			CBaseEntity* healTarget = FindHealTarget(true);

			if (healTarget) {
				const int diff = (int)ceil(healTarget->pev->max_health - healTarget->pev->health);
				const int healResult = healTarget->TakeHealth(m_pPlayer, Q_min(maxHeal, diff), DMG_GENERIC);
				SpendAmmo(healResult);
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/medshot4.wav", 1.0, ATTN_NORM, 0, 100);
			}
		}
		m_flSoundDelay = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendIdleAnimation();
}

bool CMedkit::CanRecharge()
{
	if( bIsMultiplayer() )
	{
		return gSkillData.plrMedkitTime != 0;
	}
	else
	{
		return false;
	}
}
#endif
