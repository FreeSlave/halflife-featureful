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

#if FEATURE_PIPEWRENCH

LINK_WEAPON_TO_CLASS(weapon_pipewrench, CPipeWrench)

#define	MELEE_WALLHIT_VOLUME 512
#define MELEE_BODYHIT_VOLUME 128

void CPipeWrench::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), MyWModel());

	m_iSwingMode = 0;
	m_iClip = -1;
	InitMaxClip(WEAPON_NOCLIP);
	FallInit();// get ready to fall down.
}


void CPipeWrench::Precache(void)
{
	PRECACHE_MODEL("models/v_pipe_wrench.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_pipe_wrench.mdl");

	PRECACHE_SOUND("weapons/pwrench_hit1.wav");
	PRECACHE_SOUND("weapons/pwrench_hit2.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod1.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod2.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod3.wav");
	PRECACHE_SOUND("weapons/pwrench_miss1.wav");
	PRECACHE_SOUND("weapons/pwrench_miss2.wav");

//This sounds don't used
/*	PRECACHE_SOUND("weapons/pwrench_big_hit1.wav");
	PRECACHE_SOUND("weapons/pwrench_big_hit2.wav");*/

	PRECACHE_SOUND("weapons/pwrench_big_hitbod1.wav");
	PRECACHE_SOUND("weapons/pwrench_big_hitbod2.wav");
	PRECACHE_SOUND("weapons/pwrench_big_miss.wav");

	m_usPWrench = PRECACHE_EVENT(1, "events/pipewrench.sc");
}

bool CPipeWrench::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->pszAmmo2 = NULL;
	p->iSlot = 0;
	p->iPosition = 1;
	p->pszAmmoEntity = NULL;
	p->iDropAmmo = 0;
	return true;
}

bool CPipeWrench::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

bool CPipeWrench::Deploy()
{
	m_iSwingMode = 0;
	return DefaultDeploy("models/v_pipe_wrench.mdl", "models/p_pipe_wrench.mdl", PIPEWRENCH_DRAW, "crowbar");
}

void CPipeWrench::Holster()
{
	m_iSwingMode = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(PIPEWRENCH_HOLSTER);
}

void CPipeWrench::PrimaryAttack()
{
	if (!m_iSwingMode && !Swing(true))
	{
#if !CLIENT_DLL
		SetThink(&CPipeWrench::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1f;
#endif
	}
}

void CPipeWrench::SecondaryAttack(void)
{
	if (m_iSwingMode != 1)
	{
		SendWeaponAnim(PIPEWRENCH_ATTACKBIGWIND);
		m_flBigSwingStart = gpGlobals->time;
	}
	m_iSwingMode = 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
}

void CPipeWrench::Smack()
{
#if !CLIENT_DLL
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
#endif
}


void CPipeWrench::SwingAgain(void)
{
	Swing(false);
}

bool CPipeWrench::Swing(bool fFirst)
{
	bool fDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32.0f;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
	if ( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif
	if (fFirst)
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usPWrench,
			0.0f, g_vecZero, g_vecZero, 0, 0, 1,
			0, 0, 0 );
	}

	if ( tr.flFraction >= 1.0 )
	{
		// miss
		if ( fFirst ) {
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7f;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.7f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( PIPEWRENCH_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim( PIPEWRENCH_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim( PIPEWRENCH_ATTACK3HIT );
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL

		// hit
		fDidHit = true;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		// play thwack, smack, or dong sound
		float flVol = 1.0f;
		bool fHitWorld = true;

		if( pEntity )
		{
			ClearMultiDamage();
			float flDamage;
#ifdef CLIENT_WEAPONS
			if( ( m_flNextPrimaryAttack + 1.0f == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
			if( ( m_flNextPrimaryAttack + 1.0f < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
			{
				// first swing does full damage
				flDamage = gSkillData.plrDmgPWrench;
			}
			else
			{
				// subsequent swings do half
				flDamage = gSkillData.plrDmgPWrench * 0.5f;
			}
			// Send trace attack to player.
			pEntity->TraceAttack(m_pPlayer->pev, m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB);

			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( pEntity->HasFlesh() )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = MELEE_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
				{
					m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
					return true;
				}
				else
					  flVol = 0.1f;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR );

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1.0f;
			}

			// also play pipe wrench strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			case 1:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * MELEE_WALLHIT_VOLUME );

		SetThink( &CPipeWrench::Smack );
		pev->nextthink = gpGlobals->time + 0.2f;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	return fDidHit;
}

void CPipeWrench::BigSwing(void)
{
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32.0f;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
	if ( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usPWrench,
		0.0f,
		g_vecZero,
		g_vecZero,
		0, 0, 0, 0, 0, 0 );

	//EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pwrench_big_miss.wav", 0.8, ATTN_NORM);

	m_pPlayer->pev->punchangle.x -= 2;
	if ( tr.flFraction >= 1.0 )
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	else
	{
		SendWeaponAnim( PIPEWRENCH_ATTACKBIGHIT );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL

		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if( pEntity )
		{
			ClearMultiDamage();
			float flDamage = (gpGlobals->time - m_flBigSwingStart) * gSkillData.plrDmgPWrench + 25.0f;
			if (flDamage > 150.0f) {
				flDamage = 150.0f;
			}
			pEntity->TraceAttack(m_pPlayer->pev, m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB);

			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool fHitWorld = true;

		if (pEntity)
		{
			if (pEntity->HasFlesh())
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,1) )
				{
				case 0:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_big_hitbod1.wav", 1, ATTN_NORM);
					break;
				case 1:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_big_hitbod2.wav", 1, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = MELEE_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
					  return;
				else
					  flVol = 0.1f;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR );

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1.0f;
			}

			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			case 1:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * MELEE_WALLHIT_VOLUME );
#endif
	}
}

void CPipeWrench::WeaponIdle(void)
{
	if ( m_iSwingMode == 1 )
	{
		if ( gpGlobals->time > m_flBigSwingStart + 1.0f )
		{
			m_iSwingMode = 2;
		}
	}
	else if (m_iSwingMode == 2)
	{
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1f;
		BigSwing();
		m_iSwingMode = 0;
		return;
	}
	else
	{
		m_iSwingMode = 0;
		if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if ( flRand <= 0.3f )
		{
			iAnim = PIPEWRENCH_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
		}
		else if ( flRand <= 0.6f )
		{
			iAnim = PIPEWRENCH_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
		}
		else
		{
			iAnim = PIPEWRENCH_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
		}
		SendWeaponAnim( iAnim );
	}
}

void CPipeWrench::GetWeaponData(weapon_data_t& data)
{
	data.m_fInSpecialReload = m_iSwingMode;
}

void CPipeWrench::SetWeaponData(const weapon_data_t& data)
{
	m_iSwingMode = data.m_fInSpecialReload;
}

#endif
