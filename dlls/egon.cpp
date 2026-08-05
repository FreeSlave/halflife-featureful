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
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "customentity.h"

#if !CLIENT_DLL
#include "combat.h"
#include "gamerules.h"
#endif

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

#define EGON_SWITCH_NARROW_TIME			0.75f			// Time it takes to switch fire modes
#define EGON_SWITCH_WIDE_TIME			1.5f

class CEgon : public CConfigurableWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Precache() override;
	int WeaponId() const override { return WEAPON_EGON; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;

	bool Deploy() override;
	bool CanHolster() override;
	void Holster() override;

	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );

	void CreateEffect ();
	void DestroyEffect ();
	void EndAttack();
	void Attack();
	void PrimaryAttack() override;
	void WeaponIdle() override;

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval();
	float GetDischargeInterval();

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );

	enum EGON_FIREMODE { FIRE_NARROW, FIRE_WIDE};

#if !CLIENT_DLL
	CBeam				*m_pBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;
#endif

	void GetWeaponData(weapon_data_t& data) override;
	void SetWeaponData(const weapon_data_t& data) override;

	unsigned short m_usEgonStop;

private:
#if !CLIENT_DLL
	float				m_shootTime;
#endif
	EGON_FIREMODE		m_fireMode;
	float				m_shakeTime;

	unsigned short m_usEgonFire;
};

LINK_WEAPON_TO_CLASS( weapon_egon, CEgon )

#if !CLIENT_DLL
TYPEDESCRIPTION	CEgon::m_SaveData[] =
{
	//DEFINE_FIELD( CEgon, m_pBeam, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CEgon, m_pNoise, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CEgon, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CEgon, m_shootTime, FIELD_TIME ),
	DEFINE_FIELD( CEgon, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CEgon, m_fireMode, FIELD_INTEGER ),
	DEFINE_FIELD( CEgon, m_shakeTime, FIELD_TIME ),
	DEFINE_FIELD( CEgon, m_flAmmoUseTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CEgon, CConfigurableWeapon )
#endif

void CEgon::Precache()
{
	CConfigurableWeapon::Precache();
	PRECACHE_SOUND( EGON_SOUND_OFF );
	PRECACHE_SOUND( EGON_SOUND_RUN );
	PRECACHE_SOUND( EGON_SOUND_STARTUP );

	PRECACHE_MODEL( EGON_BEAM_SPRITE );
	PRECACHE_MODEL( EGON_FLARE_SPRITE );

	m_usEgonFire = PRECACHE_EVENT( 1, "events/egon_fire.sc" );
	m_usEgonStop = PRECACHE_EVENT( 1, "events/egon_stop.sc" );
}

bool CEgon::Deploy()
{
	m_fireState = FIRE_OFF;
	return PerformDeploy();
}

void CEgon::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( EGON_HOLSTER );

	EndAttack();
}

bool CEgon::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 3;
	p->iPosition = 2;

	return true;
}

WeaponParameters CEgon::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 20;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "uranium";

	params.worldModel = "models/w_egon.mdl";
	params.viewModel = "models/v_egon.mdl";
	params.playerModel = "models/p_egon.mdl";
	params.playerAnimExt = "egon";
	params.priority = 20;

	params.deploy.animIndex = EGON_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{EGON_IDLE1, 0.5f, FloatRange(10.0f, 15.0f)},
		WeaponParameters::IdleAnim{EGON_FIDGET1, 0.5f, 3.0f},
	};

	params.dropAmmo.classname = "ammo_gaussclip";

	return std::move(params);
}

#define EGON_PULSE_INTERVAL		0.1
#define EGON_DISCHARGE_INTERVAL		0.1

float CEgon::GetPulseInterval()
{
	return EGON_PULSE_INTERVAL;
}

float CEgon::GetDischargeInterval()
{
	return EGON_DISCHARGE_INTERVAL;
}

void CEgon::Attack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		if( m_fireState != FIRE_OFF
#if !CLIENT_DLL
				|| m_pBeam
#endif
				)
		{
			EndAttack();
		}
		else
		{
			PlayEmptySound(false);
		}
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition();

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if( !HasAmmoToFire() )
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25f;
				PlayEmptySound(false);
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, m_fireMode, 1, 0 );
		
			m_shakeTime = 0;

			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
			pev->fuser1 = UTIL_WeaponTimeBase() + 2.0f;

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
			m_fireState = FIRE_CHARGE;
			break;
		}
		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );
			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;

			if( pev->fuser1 <= UTIL_WeaponTimeBase() )
			{
				PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, m_fireMode, 0, 0 );
				pev->fuser1 = 1000;
			}

			if( !HasAmmoToFire() )
			{
				EndAttack();
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
			}
			break;
		}
	}
}

void CEgon::PrimaryAttack()
{
	m_fireMode = FIRE_WIDE;
	Attack();
}

void CEgon::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
{
	Vector vecDest = vecOrigSrc + vecDir * 2048.0f;
	edict_t *pentIgnore;
	TraceResult tr;

	pentIgnore = m_pPlayer->edict();
	Vector tmpSrc = vecOrigSrc + gpGlobals->v_up * -8.0f + gpGlobals->v_right * 3.0f;

	// ALERT( at_console, "." );
	
	UTIL_TraceLine( vecOrigSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

	if( tr.fAllSolid )
		return;

#if !CLIENT_DLL
	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

	if( pEntity == NULL )
		return;

	if( g_pGameRules->IsMultiplayer() )
	{
		if( m_pSprite && pEntity->pev->takedamage )
		{
			m_pSprite->pev->effects &= ~EF_NODRAW;
		}
		else if( m_pSprite )
		{
			m_pSprite->pev->effects |= EF_NODRAW;
		}
	}
#endif
	float timedist = 0.0f;

	switch( m_fireMode )
	{
	case FIRE_NARROW:
#if !CLIENT_DLL
		if( pev->dmgtime < gpGlobals->time )
		{
			// Narrow mode only does damage to the entity it hits
			if( pEntity->pev->takedamage )
			{
				pEntity->ApplyTraceAttack( m_pPlayer->pev, m_pPlayer->pev, DamageInfo{GetSkillValue("plr_egon_narrow"), DMG_ENERGYBEAM}, vecDir, &tr );
			}

			if( g_pGameRules->IsMultiplayer() )
			{
				// multiplayer uses 1 ammo every 1/10th second
				if( gpGlobals->time >= m_flAmmoUseTime )
				{
					if( !g_pGameRules->IsBustingGame())
						SpendAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.1f;
				}
			}
			else
			{
				// single player, use 3 ammo/second
				if( gpGlobals->time >= m_flAmmoUseTime )
				{
					SpendAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.166f;
				}
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
		}
#endif
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetPulseInterval();
		break;
	case FIRE_WIDE:
#if !CLIENT_DLL
		if( pev->dmgtime < gpGlobals->time )
		{
			// wide mode does damage to the ent, and radius damage
			if( pEntity->pev->takedamage )
			{
				pEntity->ApplyTraceAttack( m_pPlayer->pev, m_pPlayer->pev, DamageInfo(GetSkillValue("plr_egon_wide"), DMG_ENERGYBEAM).SetGibPolicy(GIB_ALWAYS), vecDir, &tr);
			}

			if( g_pGameRules->IsMultiplayer() )
			{
				// radius damage a little more potent in multiplayer.
				::RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, RadiusDamageInfo(DamageInfo(GetSkillValue("plr_egon_wide") * 0.25f, DMG_ENERGYBEAM | DMG_BLAST).SetGibPolicy(GIB_ALWAYS), 128) );
			}

			if( !m_pPlayer->IsAlive() )
				return;

			if( g_pGameRules->IsMultiplayer() )
			{
				//multiplayer uses 5 ammo/second
				if( gpGlobals->time >= m_flAmmoUseTime )
				{
					if( !g_pGameRules->IsBustingGame())
						SpendAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.2f;
				}
			}
			else
			{
				// Wide mode uses 10 charges per second in single player
				if( gpGlobals->time >= m_flAmmoUseTime )
				{
					SpendAmmo( 1 );
					m_flAmmoUseTime = gpGlobals->time + 0.1f;
				}
			}

			pev->dmgtime = gpGlobals->time + GetDischargeInterval();
			if( m_shakeTime < gpGlobals->time )
			{
				UTIL_ScreenShake( tr.vecEndPos, 5.0f, 150.0f, 0.75f, 250.0f );
				m_shakeTime = gpGlobals->time + 1.5f;
			}
		}
#endif
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetDischargeInterval();
		break;
	}

	if( timedist < 0 )
		timedist = 0;
	else if( timedist > 1 )
		timedist = 1;
	timedist = 1 - timedist;

	UpdateEffect( tmpSrc, tr.vecEndPos, timedist );
}

void CEgon::UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
#if !CLIENT_DLL
	if( !m_pBeam )
	{
		CreateEffect();
	}

	m_pBeam->SetStartPos( endPoint );
	m_pBeam->SetBrightness( (int)( 255 - ( timeBlend * 180 )) );
	m_pBeam->SetWidth( (int)( 40 - ( timeBlend * 20 ) ) );

	if( m_fireMode == FIRE_WIDE )
		m_pBeam->SetColor( (int)( 30 + ( 25 * timeBlend ) ), (int)( 30 + ( 30 * timeBlend ) ), (int)( 64 + 80 * fabs( sin( gpGlobals->time * 10.0f ) ) ) );
	else
		m_pBeam->SetColor( (int)( 60 + ( 25 * timeBlend ) ), (int)( 120 + ( 30 * timeBlend ) ), (int)( 64 + 80 * fabs( sin( gpGlobals->time * 10.0f ) ) ) );

	UTIL_SetOrigin( m_pSprite->pev, endPoint );
	m_pSprite->pev->frame += 8.0f * gpGlobals->frametime;
	if( m_pSprite->pev->frame > m_pSprite->Frames() )
		m_pSprite->pev->frame = 0;

	m_pNoise->SetStartPos( endPoint );
#endif
}

void CEgon::CreateEffect()
{
#if !CLIENT_DLL
	DestroyEffect();

	m_pBeam = CBeam::BeamCreate( EGON_BEAM_SPRITE, 40 );
	m_pBeam->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pBeam->SetFlags( BEAM_FSINE );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition
	m_pBeam->pev->flags |= FL_SKIPLOCALHOST;
	m_pBeam->pev->owner = m_pPlayer->edict();

	m_pNoise = CBeam::BeamCreate( EGON_BEAM_SPRITE, 55 );
	m_pNoise->PointEntInit( pev->origin, m_pPlayer->entindex() );
	m_pNoise->SetScrollRate( 25 );
	m_pNoise->SetBrightness( 100 );
	m_pNoise->SetEndAttachment( 1 );
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_pNoise->pev->flags |= FL_SKIPLOCALHOST;
	m_pNoise->pev->owner = m_pPlayer->edict();

	m_pSprite = CSprite::SpriteCreate( EGON_FLARE_SPRITE, pev->origin );
	m_pSprite->pev->scale = 1.0;
	m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
	m_pSprite->pev->owner = m_pPlayer->edict();

	if( m_fireMode == FIRE_WIDE )
	{
		m_pBeam->SetScrollRate( 50 );
		m_pBeam->SetNoise( 20 );
		m_pNoise->SetColor( 50, 50, 255 );
		m_pNoise->SetNoise( 8 );
	}
	else
	{
		m_pBeam->SetScrollRate( 110 );
		m_pBeam->SetNoise( 5 );
		m_pNoise->SetColor( 80, 120, 255 );
		m_pNoise->SetNoise( 2 );
	}
#endif
}

void CEgon::DestroyEffect()
{
#if !CLIENT_DLL
	UTIL_RemoveAndClean(m_pBeam);
	UTIL_RemoveAndClean(m_pNoise);

	if( m_pSprite )
	{
		if( m_fireMode == FIRE_WIDE )
			m_pSprite->Expand( 10, 500 );
		else
			UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
#endif
}

void CEgon::WeaponIdle()
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fireState != FIRE_OFF )
		 EndAttack();

	SendIdleAnimation();
}

bool CEgon::CanHolster()
{
#if CLIENT_DLL
	return true;
#else
	return !g_pGameRules->IsBustingGame();
#endif
}

void CEgon::EndAttack()
{
	bool bMakeNoise = false;

	if( m_fireState != FIRE_OFF ) //Checking the button just in case!.
		 bMakeNoise = true;

	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, m_pPlayer->edict(), m_usEgonStop, 0.0f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0.0f, 0.0f, bMakeNoise, 0, 0, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;

	m_fireState = FIRE_OFF;

	DestroyEffect();
}

void CEgon::GetWeaponData(weapon_data_t& data)
{
	data.iuser3 = m_fireState;
}

void CEgon::SetWeaponData(const weapon_data_t& data)
{
	m_fireState = data.iuser3;
}
