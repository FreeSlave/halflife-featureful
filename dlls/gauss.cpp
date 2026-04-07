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
#include "shake.h"
#if !CLIENT_DLL
#include "common_soundscripts.h"
#include "game.h"
#include "gamerules.h"
#include "soundent.h"
#endif

#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

class CGauss : public CConfigurableWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Precache() override;
	int WeaponId() const override { return WEAPON_GAUSS; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;

	bool Deploy() override;
	void Holster() override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponIdle() override;

	void StartFire();
	void Fire( Vector vecOrigSrc, Vector vecDirShooting, float flDamage );
	float GetFullChargeTime();
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;
	int m_iSoundState; // don't save this

	// was this weapon just fired primary or secondary?
	// we need to know so we can pick the right set of effects.
	bool m_fPrimaryFire;

	void GetWeaponData(weapon_data_t& data) override;
	void SetWeaponData(const weapon_data_t& data) override;

private:
	void SendStopEvent(bool sendToHost);

	unsigned short m_usGaussFire;
	unsigned short m_usGaussSpin;
};

LINK_WEAPON_TO_CLASS( weapon_gauss, CGauss )

#if !CLIENT_DLL
TYPEDESCRIPTION	CGauss::m_SaveData[] =
{
	DEFINE_FIELD( CGauss, m_fInAttack, FIELD_INTEGER ),
	//DEFINE_FIELD( CGauss, m_flStartCharge, FIELD_TIME ),
	//DEFINE_FIELD( CGauss, m_flPlayAftershock, FIELD_TIME ),
	//DEFINE_FIELD( CGauss, m_flNextAmmoBurn, FIELD_TIME ),
	DEFINE_FIELD( CGauss, m_fPrimaryFire, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CGauss, CConfigurableWeapon )
#endif

float CGauss::GetFullChargeTime()
{
	if( bIsMultiplayer() )
	{
		return 1.5f;
	}

	return 4.0f;
}

#if CLIENT_DLL
extern int g_irunninggausspred;
#endif

void CGauss::Precache()
{
	CConfigurableWeapon::Precache();

	PRECACHE_SOUND( "weapons/gauss2.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "weapons/electro5.wav" );
	PRECACHE_SOUND( "weapons/electro6.wav" );
	PRECACHE_SOUND( "ambience/pulsemachine.wav" );

	m_iGlow = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBalls = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBeam = PRECACHE_MODEL( "sprites/smoke.spr" );

	m_usGaussFire = PRECACHE_EVENT( 1, "events/gauss.sc" );
	m_usGaussSpin = PRECACHE_EVENT( 1, "events/gaussspin.sc" );
}

bool CGauss::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 3;
	p->iPosition = 1;

	return true;
}

WeaponParameters CGauss::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 20;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "uranium";

	params.worldModel = "models/w_gauss.mdl";
	params.viewModel = "models/v_gauss.mdl";
	params.playerModel = "models/p_gauss.mdl";
	params.playerAnimExt = "gauss";
	params.priority = 20;

	params.deploy.animIndex = GAUSS_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{GAUSS_IDLE, 0.66f, FloatRange(10.0f, 15.0f)},
		WeaponParameters::IdleAnim{GAUSS_IDLE2, 0.34f, FloatRange(10.0f, 15.0f)},
	};

	params.fire.anims = {GAUSS_FIRE2};
	params.fire.cycleTime = 0.2f;
	params.fire.allowUnderwater = false;
	params.fire.ammoPerFire = 2;

	params.fire.weaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	params.fire.clientPunchPitch = -2;

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.dropAmmo.classname = "ammo_gaussclip";

	return params;
}

bool CGauss::Deploy()
{
	m_pPlayer->m_flPlayAftershock = 0.0;
	return PerformDeploy();
}

void CGauss::Holster()
{
	SendStopEvent(true);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	SendWeaponAnim( GAUSS_HOLSTER );
	m_fInAttack = 0;
}

void CGauss::PrimaryAttack()
{
	const WeaponParameters& params = MyParameters();

	// don't fire underwater
	if (!params.fire.allowUnderwater.Get(false) && m_pPlayer->pev->waterlevel == WL_Eyes)
	{
		PlayEmptySound(false);
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.15f );
		return;
	}

	const int ammoPerFire = params.fire.ammoPerFire.Get(false);

	if( !HasAmmoToFire(ammoPerFire) )
	{
		PlayEmptySound(false);
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	m_fPrimaryFire = true;

	SpendAmmo(ammoPerFire);

	StartFire();
	m_fInAttack = 0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + params.fire.cycleTime.Get(false);
}

void CGauss::SecondaryAttack()
{
	const WeaponParameters& params = MyParameters();
	if (params.secondaryFireType == SecondaryFireType::DISABLED)
		return;

	if( m_pPlayer->m_flStartCharge > gpGlobals->time )
		m_pPlayer->m_flStartCharge = gpGlobals->time;
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		if( m_fInAttack != 0 )
		{
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 0x3f ) );
			SendStopEvent(true);
			SendWeaponAnim( GAUSS_IDLE );
			m_fInAttack = 0;
		}
		else
		{
			PlayEmptySound(true);
		}

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
		return;
	}

	if( m_fInAttack == 0 )
	{
		if( !HasAmmoToFire() )
		{
#if !CLIENT_DLL
			m_pPlayer->EmitSoundScript(Items::weaponEmptySoundScript);
#endif
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
			return;
		}

		m_fPrimaryFire = false;

		SpendAmmo();
		m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase();

		// spin up
		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;

		SendWeaponAnim( GAUSS_SPINUP );
		m_fInAttack = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
		m_pPlayer->m_flStartCharge = gpGlobals->time;
		m_pPlayer->m_flAmmoStartCharge = UTIL_WeaponTimeBase() + GetFullChargeTime();

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 110, 0, 0, 0 );

		m_iSoundState = SND_CHANGE_PITCH;
	}
	else if( m_fInAttack == 1 )
	{
		if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
		{
			SendWeaponAnim( GAUSS_SPIN );
			m_fInAttack = 2;
		}
	}
	else
	{
		// during the charging process, eat one bit of ammo every once in a while
		if( UTIL_WeaponTimeBase() >= m_pPlayer->m_flNextAmmoBurn && m_pPlayer->m_flNextAmmoBurn != 1000 )
		{
			if( bIsMultiplayer() )
			{
				SpendAmmo();
				m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.1f;
			}
			else
			{
				SpendAmmo();
				m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.3f;
			}
		}

		if( !HasAmmoToFire() )
		{
			// out of ammo! force the gun to fire
			StartFire();
			m_fInAttack = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
			return;
		}

		if( UTIL_WeaponTimeBase() >= m_pPlayer->m_flAmmoStartCharge )
		{
			// don't eat any more ammo after gun is fully charged.
			m_pPlayer->m_flNextAmmoBurn = 1000;
		}

		int pitch = (int)( ( gpGlobals->time - m_pPlayer->m_flStartCharge ) * ( 150 / GetFullChargeTime() ) + 100 );
		if( pitch > 250 ) 
			 pitch = 250;
		
		// ALERT( at_console, "%d %d %d\n", m_fInAttack, m_iSoundState, pitch );

		const bool overcharge = m_pPlayer->m_flStartCharge < gpGlobals->time - 10.0f;

		if( m_iSoundState == 0 )
			ALERT( at_console, "sound state %d\n", m_iSoundState );

		//if (!overcharge)
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, pitch, 0, ( m_iSoundState == SND_CHANGE_PITCH ) ? 1 : 0, 0 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions

		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;

		// m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		if( overcharge )
		{
			// Player charged up too long. Zap him.
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro4.wav", 1.0f, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 0x3f ) );
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/electro6.wav", 1.0f, ATTN_NORM, 0, 75 + RANDOM_LONG( 0, 0x3f ) );

			m_fInAttack = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;

			SendStopEvent(false);

#if !CLIENT_DLL
			m_pPlayer->TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), DamageInfo(GetSkillValue("plr_gauss_overcharge"), DMG_SHOCK) );
			UTIL_ScreenFade( m_pPlayer, Vector( 255, 128, 0 ), 2, 0.5f, 128, FFADE_IN );
#endif
			SendWeaponAnim( GAUSS_IDLE );

			// Player may have been killed and this weapon dropped, don't execute any more code after this!
			return;
		}
	}
}

//=========================================================
// StartFire- since all of this code has to run and then 
// call Fire(), it was easier at this point to rip it out 
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names 
//=========================================================
void CGauss::StartFire()
{
	float flDamage;

	if( m_pPlayer->m_flStartCharge > gpGlobals->time )
		m_pPlayer->m_flStartCharge = gpGlobals->time;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition(); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;



	if( m_fPrimaryFire )
	{
		// fixed damage on primary attack
#if CLIENT_DLL
		flDamage = 20.0f;
#else
		flDamage = GetSkillValue("plr_gauss");
#endif
	}
	else
	{
#if CLIENT_DLL
		const float maxDamage = 200.0f;
#else
		const float maxDamage = GetSkillValue("plr_gauss_maxspin");
#endif
		if( gpGlobals->time - m_pPlayer->m_flStartCharge > GetFullChargeTime() )
		{
			flDamage = maxDamage;
		}
		else
		{
			flDamage = maxDamage * ( ( gpGlobals->time - m_pPlayer->m_flStartCharge ) / GetFullChargeTime() );
		}
	}

	if( m_fInAttack != 3 )
	{
		//ALERT( at_console, "Time:%f Damage:%f\n", gpGlobals->time - m_pPlayer->m_flStartCharge, flDamage );
#if !CLIENT_DLL
		float flZVel = m_pPlayer->pev->velocity.z;

		if( !m_fPrimaryFire )
		{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * flDamage * 5.0f;
		}

		if( !g_pGameRules->IsMultiplayer() )
		{
			// in deathmatch, gauss can pop you up into the air. Not in single play.
			m_pPlayer->pev->velocity.z = flZVel;
		}
#endif
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}

	// time until aftershock 'static discharge' sound
	m_pPlayer->m_flPlayAftershock = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.3f, 0.8f );

	Fire( vecSrc, vecAiming, flDamage );
}

void CGauss::Fire( Vector vecOrigSrc, Vector vecDir, float flDamage )
{
	const WeaponParameters& params = MyParameters();

	m_pPlayer->m_iWeaponVolume = params.fire.weaponVolume.Get(false);
	TraceResult tr, beam_tr;
#if !CLIENT_DLL
	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192.0f;
	edict_t	*pentIgnore;
	float flMaxFrac = 1.0f;
	int nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int nMaxHits = 10;

	pentIgnore = ENT( m_pPlayer->pev );
#else
	if( m_fPrimaryFire == false )
		 g_irunninggausspred = true;
#endif	
	// The main firing event is sent unreliably so it won't be delayed.
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussFire, 0.0f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, flDamage, 0.0, 0, 0, m_fPrimaryFire ? 1 : 0, 0 );

	SendStopEvent(false);

	/*ALERT( at_console, "%f %f %f\n%f %f %f\n", 
		vecSrc.x, vecSrc.y, vecSrc.z, 
		vecDest.x, vecDest.y, vecDest.z );*/

	//ALERT( at_console, "%f %f\n", tr.flFraction, flMaxFrac );

#if !CLIENT_DLL
	while( flDamage > 10 && nMaxHits > 0 )
	{
		nMaxHits--;

		// ALERT( at_console, "." );
		UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

		if( tr.fAllSolid )
			break;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( pEntity == NULL )
			break;

		if( fFirstBeam )
		{
			m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
			fFirstBeam = 0;

			nTotal += 26;
		}

		if( pEntity->pev->takedamage )
		{
			if( pEntity->pev == m_pPlayer->pev )
				tr.iHitgroup = 0;

			pEntity->ApplyTraceAttack( m_pPlayer->pev, m_pPlayer->pev, DamageInfo{flDamage, DMG_BULLET}, vecDir, &tr );
		}

		if( pEntity->ReflectGauss() )
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct( tr.vecPlaneNormal, vecDir );

			if( n < 0.5f ) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;

				r = 2.0 * tr.vecPlaneNormal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.flFraction;
				vecDir = r;
				vecSrc = tr.vecEndPos + vecDir * 8.0f;
				vecDest = vecSrc + vecDir * 8192.0f;

				// explode a bit
				m_pPlayer->RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, DamageInfo{flDamage * n, DMG_BLAST}, CLASS_NONE );

				nTotal += 34;

				// lose energy
				if( n == 0.0f ) n = 0.1f;
				flDamage = flDamage * ( 1.0f - n );
			}
			else
			{
				nTotal += 13;

				// limit it to one hole punch
				if( fHasPunched )
					break;
				fHasPunched = 1;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if( !m_fPrimaryFire )
				{
					UTIL_TraceLine( tr.vecEndPos + vecDir * 8, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr );
					if( !beam_tr.fAllSolid )
					{
						// trace backwards to find exit point
						UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pentIgnore, &beam_tr );

						n = ( beam_tr.vecEndPos - tr.vecEndPos ).Length();

						if( n < flDamage )
						{
							if( n == 0.0f )
								n = 1.0f;
							flDamage -= n;

							// ALERT( at_console, "punch %f\n", n );
							nTotal += 21;

							// exit blast damage
							//m_pPlayer->RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, CLASS_NONE, DMG_BLAST );
							const float damage_radius = flDamage * GetSkillValue("plr_gauss_radius_factor");

							::RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, DamageInfo{flDamage, DMG_BLAST}, damage_radius, CLASS_NONE );

							InsertAISound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0f );

							nTotal += 53;

							vecSrc = beam_tr.vecEndPos + vecDir;
						}
						else if( !selfgauss.value )
						{
							flDamage = 0;
						}
					}
					else
					{
						 //ALERT( at_console, "blocked %f\n", n );
						flDamage = 0;
					}
				}
				else
				{
					//ALERT( at_console, "blocked solid\n" );

					flDamage = 0;
				}

			}
		}
		else
		{
			vecSrc = tr.vecEndPos + vecDir;
			pentIgnore = ENT( pEntity->pev );
		}
	}
#endif
	// ALERT( at_console, "%d bytes\n", nTotal );
}

void CGauss::WeaponIdle()
{
	ResetEmptySound();

	// play aftershock static discharge
	if( m_pPlayer->m_flPlayAftershock && m_pPlayer->m_flPlayAftershock < gpGlobals->time )
	{
		switch( RANDOM_LONG( 0, 3 ) )
		{
		case 0:
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro4.wav", RANDOM_FLOAT( 0.7f, 0.8f ), ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro5.wav", RANDOM_FLOAT( 0.7f, 0.8f ), ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro6.wav", RANDOM_FLOAT( 0.7f, 0.8f ), ATTN_NORM );
			break;
		case 3:
			break; // no sound
		}
		m_pPlayer->m_flPlayAftershock = 0.0f;
	}

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fInAttack != 0 )
	{
		StartFire();
		m_fInAttack = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	}
	else
	{
		SendIdleAnimation();
	}
}

void CGauss::GetWeaponData(weapon_data_t& data)
{
	data.iuser2 = m_fInAttack;
}

void CGauss::SetWeaponData(const weapon_data_t& data)
{
	m_fInAttack = data.iuser2;
}

void CGauss::SendStopEvent(bool sendToHost)
{
	// This reliable event is used to stop the spinning sound
	// It's delayed by a fraction of second to make sure it is delayed by 1 frame on the client
	// It's sent reliably anyway, which could lead to other delays

	int flags = FEV_RELIABLE | FEV_GLOBAL;

	if (!sendToHost)
	{
		flags |= FEV_NOTHOST;
	}

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usGaussFire, 0.01f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 1);
}
