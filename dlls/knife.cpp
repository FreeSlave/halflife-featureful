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

#if FEATURE_KNIFE

#define	KNIFE_WALLHIT_VOLUME 512
#define	KNIFE_BODYHIT_VOLUME 128

LINK_WEAPON_TO_CLASS(weapon_knife, CKnife)

void CKnife::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), MyWModel());

	m_iSwingMode = 0;
	m_iClip = -1;
	InitMaxClip(WEAPON_NOCLIP);
	FallInit();// get ready to fall down.
}


void CKnife::Precache(void)
{
	PRECACHE_MODEL("models/v_knife.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_knife.mdl");
	PRECACHE_SOUND("weapons/knife_hit_flesh1.wav");
	PRECACHE_SOUND("weapons/knife_hit_flesh2.wav");
	PRECACHE_SOUND("weapons/knife_hit_wall1.wav");
	PRECACHE_SOUND("weapons/knife_hit_wall2.wav");
	PRECACHE_SOUND("weapons/knife1.wav");
	PRECACHE_SOUND("weapons/knife2.wav");
	PRECACHE_SOUND("weapons/knife3.wav");

	m_usKnife = PRECACHE_EVENT(1, "events/knife.sc");
}

bool CKnife::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->pszAmmo2 = NULL;
	p->iSlot = 0;
	p->iPosition = 2;
	p->pszAmmoEntity = NULL;
	p->iDropAmmo = 0;
	return true;
}

bool CKnife::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

bool CKnife::Deploy()
{
	m_iSwingMode = 0;
	return DefaultDeploy("models/v_knife.mdl", "models/p_knife.mdl", KNIFE_DRAW, "crowbar");
}

void CKnife::Holster()
{
	m_iSwingMode = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(KNIFE_HOLSTER);
}

void CKnife::PrimaryAttack()
{
	if (!m_iSwingMode && !Swing(1))
	{
#if !CLIENT_DLL
		SetThink(&CKnife::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
#endif
	}
}

void CKnife::SecondaryAttack()
{
	if (m_iSwingMode != 1)
	{
		SendWeaponAnim(KNIFE_CHARGE);
		m_flStabStart = gpGlobals->time;
	}
	m_iSwingMode = 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
}

void CKnife::Smack()
{
#if !CLIENT_DLL
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
#endif
}


void CKnife::SwingAgain(void)
{
	Swing(0);
}


bool CKnife::Swing(bool fFirst)
{
	bool fDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32.0f;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#if !CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer);
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if ( fFirst )
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usKnife,
			0.0f, g_vecZero, g_vecZero, 0, 0, 1,
			0, 0, 0);
	}


	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(KNIFE_ATTACK1);
			break;
		case 1:
			SendWeaponAnim(KNIFE_ATTACK2HIT);
			break;
		case 2:
			SendWeaponAnim(KNIFE_ATTACK3HIT);
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

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
			// If building with the clientside weapon prediction system,
			// UTIL_WeaponTimeBase() is always 0 and m_flNextPrimaryAttack is >= -1.0f, thus making
			// m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() always evaluate to false.
#ifdef CLIENT_WEAPONS
			if( ( m_flNextPrimaryAttack + 1 == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
			if( ( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
			{
				// first swing does full damage
				pEntity->TraceAttack( m_pPlayer->pev, m_pPlayer->pev, gSkillData.plrDmgKnife, gpGlobals->v_forward, &tr, DMG_CLUB );
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack( m_pPlayer->pev, m_pPlayer->pev, gSkillData.plrDmgKnife * 0.5f, gpGlobals->v_forward, &tr, DMG_CLUB );
			}
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if( pEntity->HasFlesh() )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG( 0, 1 ) )
				{
				case 0:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/knife_hit_flesh1.wav", 1.0f, ATTN_NORM );
					break;
				case 1:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/knife_hit_flesh2.wav", 1.0f, ATTN_NORM );
					break;
				}
				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if( !pEntity->IsAlive() )
				{
					m_flNextPrimaryAttack = GetNextAttackDelay(0.25);
					return true;
				}
				else
					flVol = 0.1f;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2.0f, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1.0f;
			}

			// also play knife strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;

		SetThink(&CKnife::Smack);
		pev->nextthink = gpGlobals->time + 0.2f;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25f;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	return fDidHit;
}

void CKnife::Stab()
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

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usKnife,
		0.0,
		g_vecZero,
		g_vecZero,
		0, 0, 0, 0, 0, 0 );

	if ( tr.flFraction >= 1.0f )
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	else
	{
		//SendWeaponAnim( KNIFE_STAB );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL

		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if( pEntity )
		{
			ClearMultiDamage();
			float flDamage = (gpGlobals->time - m_flStabStart) * gSkillData.plrDmgKnife + gSkillData.plrDmgKnife*2.0f;
			if (flDamage > 100.0f) {
				flDamage = 100.0f;
			}
			pEntity->TraceAttack(m_pPlayer->pev, m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB|DMG_NEVERGIB);

			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

		// play thwack, smack, or dong sound
		float flVol = 1.0f;
		bool fHitWorld = true;

		if (pEntity)
		{
			if (pEntity->HasFlesh())
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,1) )
				{
				case 0:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_flesh1.wav", 1.0f, ATTN_NORM);
					break;
				case 1:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_flesh2.wav", 1.0f, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
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
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			case 1:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * KNIFE_WALLHIT_VOLUME );
#endif
	}
}

void CKnife::WeaponIdle( void )
{
	if ( m_iSwingMode == 1 )
	{
		if ( gpGlobals->time > m_flStabStart + 0.8f )
		{
			m_iSwingMode = 2;
		}
	}
	else if (m_iSwingMode == 2)
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.9f;
		m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		Stab();
		m_iSwingMode = 0;
		return;
	}
	else
	{
		m_iSwingMode = 0;
		if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9f )
		{
			iAnim = KNIFE_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
		}
		else
		{
			if( flRand > 0.5f )
			{
				iAnim = KNIFE_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0f / 25.0f;
			}
			else
			{
				iAnim = KNIFE_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
			}
		}
		SendWeaponAnim( iAnim );
	}
}

void CKnife::GetWeaponData(weapon_data_t& data)
{
	data.m_fInSpecialReload = m_iSwingMode;
}

void CKnife::SetWeaponData(const weapon_data_t& data)
{
	m_iSwingMode = data.m_fInSpecialReload;
}

#endif
