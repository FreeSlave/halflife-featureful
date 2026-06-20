/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"

class CBarnacleGrappleTip;

#if !CLIENT_DLL
#include "game.h"
#include "gamerules.h"
#include "grapple_tonguetip.h"

LINK_ENTITY_TO_CLASS( grapple_tip, CBarnacleGrappleTip );

void CBarnacleGrappleTip::Precache()
{
	PRECACHE_MODEL( "models/shock_effect.mdl" );
}

void CBarnacleGrappleTip::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), "models/shock_effect.mdl" );

	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0) );

	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CBarnacleGrappleTip::FlyThink );
	SetTouch( &CBarnacleGrappleTip::TongueTouch );

	Vector vecAngles = pev->angles;

	vecAngles.x -= 30.0;

	pev->angles = vecAngles;

	UTIL_MakeVectors( pev->angles );

	vecAngles.x = -( 30.0 + vecAngles.x );

	pev->velocity = g_vecZero;

	pev->gravity = 1.0;

	pev->nextthink = gpGlobals->time + 0.02;

	m_bIsStuck = false;
	m_bMissed = false;
}

void CBarnacleGrappleTip::FlyThink()
{
	UTIL_MakeAimVectors( pev->angles );

	pev->angles = UTIL_VecToAngles( gpGlobals->v_forward );

	const float flNewVel = ( ( pev->velocity.Length() * 0.8 ) + 400.0 );

	pev->velocity = pev->velocity * 0.2 + ( flNewVel * gpGlobals->v_forward );

	if( !g_pGameRules->IsMultiplayer() )
	{
		//Note: the old grapple had a maximum velocity of 1600. - Solokiller
		pev->velocity.ClampToLengthInPlace(750.0f);
	}
	else
	{
		//TODO: should probably clamp at sv_maxvelocity to prevent the tip from going off course. - Solokiller
		pev->velocity.ClampToLengthInPlace(2000.0f);
	}

	pev->nextthink = gpGlobals->time + 0.02;
}

void CBarnacleGrappleTip::OffsetThink()
{
	//Nothing
}

void CBarnacleGrappleTip::TongueTouch( CBaseEntity* pOther )
{
	int targetClass;

	if( !pOther )
	{
		targetClass = GRAPPLE_NOT_A_TARGET;
		m_bMissed = true;
	}
	else
	{
		if( pOther->IsPlayer() )
		{
			targetClass = GRAPPLE_MEDIUM;

			m_hGrappleTarget = pOther;

			m_bIsStuck = true;
		}
		else
		{
			targetClass = CheckTarget( pOther );

			if( targetClass != GRAPPLE_NOT_A_TARGET )
			{
				m_bIsStuck = true;
			}
			else
			{
				m_bMissed = true;
			}
		}
	}

	pev->velocity = g_vecZero;

	m_GrappleType = targetClass;

	SetThink( &CBarnacleGrappleTip::OffsetThink );
	pev->nextthink = gpGlobals->time + 0.02;

	SetTouch( NULL );
}

int CBarnacleGrappleTip::CheckTarget( CBaseEntity* pTarget )
{
	if( !pTarget )
		return GRAPPLE_NOT_A_TARGET;

	if( pTarget->IsPlayer() )
	{
		m_hGrappleTarget = pTarget;

		return pTarget->SizeForGrapple();
	}

	Vector vecStart = pev->origin;
	Vector vecEnd = pev->origin + pev->velocity * 1024.0;

	TraceResult tr;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &tr );

	CBaseEntity* pHit = Instance( tr.pHit );

/*	if( !pHit )
		pHit = CWorld::GetInstance();*/

	float rgfl1[3];
	float rgfl2[3];
	const char *pTexture;

	vecStart.CopyToArray(rgfl1);
	vecEnd.CopyToArray(rgfl2);

	if (pHit)
		pTexture = TRACE_TEXTURE(ENT(pHit->pev), rgfl1, rgfl2);
	else
		pTexture = TRACE_TEXTURE(ENT(0), rgfl1, rgfl2);

	bool bIsFixed = false;

	if( pTexture && strnicmp( pTexture, "xeno_grapple", 12 ) == 0 )
	{
		bIsFixed = true;
	}
	else if (pTarget->SizeForGrapple() != GRAPPLE_NOT_A_TARGET)
	{
		if (pTarget->SizeForGrapple() == GRAPPLE_FIXED) {
			bIsFixed = true;
		} else {
			m_hGrappleTarget = pTarget;
			m_vecOriginOffset = pev->origin - pTarget->pev->origin;
			return pTarget->SizeForGrapple();
		}
	}

	if( bIsFixed )
	{
		m_hGrappleTarget = pTarget;
		m_vecOriginOffset = g_vecZero;

		return GRAPPLE_FIXED;
	}

	return GRAPPLE_NOT_A_TARGET;
}

void CBarnacleGrappleTip::SetPosition( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner )
{
	UTIL_SetOrigin( pev, vecOrigin );
	pev->angles = vecAngles;
	pev->owner = pOwner->edict();
}
#endif

enum BarnacleGrappleAnim
{
	BGRAPPLE_BREATHE = 0,
	BGRAPPLE_LONGIDLE,
	BGRAPPLE_SHORTIDLE,
	BGRAPPLE_COUGH,
	BGRAPPLE_DOWN,
	BGRAPPLE_UP,
	BGRAPPLE_FIRE,
	BGRAPPLE_FIREWAITING,
	BGRAPPLE_FIREREACHED,
	BGRAPPLE_FIRETRAVEL,
	BGRAPPLE_FIRERELEASE
};

class CBarnacleGrapple : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	enum FireState
	{
		OFF		= 0,
		CHARGE	= 1
	};

	void Precache() override;
	void PrecacheDefaultModelSounds() override;
	void Spawn() override;
	int WeaponId() const override { return WEAPON_GRAPPLE; }
	void EndAttack();

	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
	bool AddToPlayer( CBasePlayer* pPlayer ) override;
	bool Deploy() override;
	void Holster() override;
	void WeaponIdle() override;
	void PrimaryAttack() override;

	void Fire( Vector vecOrigin, Vector vecDir );

	void CreateEffect();
	void UpdateEffect();
	void DestroyEffect();
private:
	CBarnacleGrappleTip* m_pTip;
#if !CLIENT_DLL
	CBeam* m_pBeam;
#endif

	float m_flShootTime;
	float m_flDamageTime;

	bool m_bGrappling;
	bool m_bMissed;
	bool m_bMomentaryStuck;
};

LINK_WEAPON_TO_CLASS( weapon_grapple, CBarnacleGrapple )

#if !CLIENT_DLL
TYPEDESCRIPTION	CBarnacleGrapple::m_SaveData[] =
{
	DEFINE_FIELD( CBarnacleGrapple, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBarnacleGrapple, m_flShootTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleGrapple, m_fireState, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBarnacleGrapple, CBasePlayerWeapon )
#endif

void CBarnacleGrapple::Precache()
{
	PrecacheWeaponModels();
	PrecacheModelSounds();

	PRECACHE_SOUND( "weapons/bgrapple_release.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_impact.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_fire.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_cough.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_pull.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_wait.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew1.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew2.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew3.wav" );

	PRECACHE_MODEL( "sprites/tongue.spr" );

	UTIL_PrecacheOther( "grapple_tip" );
}

void CBarnacleGrapple::PrecacheDefaultModelSounds()
{
	PRECACHE_SOUND( "weapons/alienweap_draw.wav" );
}

void CBarnacleGrapple::Spawn()
{
	const WeaponParameters& params = MyParameters();
	Precache();
	SetMyModel(params.worldModel.c_str());
	m_pTip = NULL;
	m_bGrappling = false;
	SetInitialAmmoAmount();
	InitMaxClip();

	FallInit();
}

bool CBarnacleGrapple::GetItemInfo(ItemInfo *p)
{
	p->iSlot = 0;
	p->iPosition = 3;
	return true;
}

WeaponParameters CBarnacleGrapple::GetDefaultParameters() const
{
	WeaponParameters params;

	params.maxClip = WEAPON_NOCLIP;

	params.worldModel = "models/w_bgrap.mdl";
	params.viewModel = "models/v_bgrap.mdl";
	params.playerModel = "models/p_bgrap.mdl";
	params.playerAnimExt = "gauss";
	params.priority = 21;

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.fire.weaponVolume = 450;

	return std::move(params);
}

bool CBarnacleGrapple::AddToPlayer( CBasePlayer* pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

bool CBarnacleGrapple::Deploy()
{
	const WeaponParameters& params = MyParameters();

	bool r = DefaultDeploy(ViewModelToDeploy(params.ViewModel()), params.PlayerModel(), BGRAPPLE_UP, params.PlayerAnimExt() );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1;
	return r;
}

void CBarnacleGrapple::Holster()
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;

	if( m_fireState != OFF )
		EndAttack();

	SendWeaponAnim( BGRAPPLE_DOWN );
}

void CBarnacleGrapple::WeaponIdle()
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fireState != OFF )
	{
		EndAttack();
		return;
	}

	m_bMissed = false;

	const float flNextIdle = RANDOM_FLOAT( 0.0, 1.0 );

	int iAnim;

	if( flNextIdle <= 0.5 )
	{
		iAnim = BGRAPPLE_LONGIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0;
	}
	else if( flNextIdle > 0.95 )
	{
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/bgrapple_cough.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );

		iAnim = BGRAPPLE_COUGH;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.6;
	}
	else
	{
		iAnim = BGRAPPLE_BREATHE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.566;
	}

	SendWeaponAnim( iAnim );
}

void CBarnacleGrapple::PrimaryAttack()
{
	if( m_bMissed )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
#if !CLIENT_DLL
	if( m_pTip )
	{
		if( m_pTip->IsStuck() )
		{
			CBaseEntity* pTarget = m_pTip->GetGrappleTarget();

			if( !pTarget )
			{
				EndAttack();
				return;
			}

			if( m_pTip->GetGrappleType() > GRAPPLE_SMALL )
			{
				m_pPlayer->pev->movetype = MOVETYPE_FLY;
				m_pPlayer->pev->flags |= FL_IMMUNE_SLIME;
				//Tells the physics code that the player is not on a ladder - Solokiller
			}

			if( m_bMomentaryStuck )
			{
				SendWeaponAnim( BGRAPPLE_FIRETRAVEL );

				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );

				if( pTarget->IsPlayer() )
				{
					EMIT_SOUND_DYN( ENT(pTarget->pev), CHAN_STATIC,"weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );
				}

				m_bMomentaryStuck = false;
			}

			switch( m_pTip->GetGrappleType() )
			{
			case GRAPPLE_NOT_A_TARGET: break;

			case GRAPPLE_SMALL:
				//pTarget->BarnacleVictimGrabbed( this );
				m_pTip->pev->origin = pTarget->Center();

				pTarget->pev->velocity = pTarget->pev->velocity + ( m_pPlayer->pev->origin - pTarget->pev->origin );

				if( pTarget->pev->velocity.Length() > 450.0 )
				{
					pTarget->pev->velocity = pTarget->pev->velocity.Normalize() * 450.0;
				}

				break;

			case GRAPPLE_MEDIUM:
			case GRAPPLE_LARGE:
			case GRAPPLE_FIXED:
				//pTarget->BarnacleVictimGrabbed( this );

				if( m_pTip->GetGrappleType() != GRAPPLE_FIXED )
					UTIL_SetOrigin( m_pTip->pev, pTarget->Center() );

				m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + ( m_pTip->pev->origin - m_pPlayer->pev->origin );

				if( m_pPlayer->pev->velocity.Length() > 450.0 )
				{
					m_pPlayer->pev->velocity = m_pPlayer->pev->velocity.Normalize() * 450.0;

					Vector vecPitch = UTIL_VecToAngles( m_pPlayer->pev->velocity );

					if( (vecPitch.x > 55.0 && vecPitch.x < 205.0) || vecPitch.x < -55.0 )
					{
						m_bGrappling = false;
						m_pPlayer->SetAnimation( PLAYER_IDLE );
					}
					else
					{
						m_bGrappling = true;
						m_pPlayer->m_afPhysicsFlags |= PFLAG_LATCHING;
						m_pPlayer->SetAnimation(PLAYER_GRAPPLE);
					}
				}
				else
				{
					m_bGrappling = false;
					m_pPlayer->SetAnimation( PLAYER_IDLE );
				}

				break;
			}
		}

		if( m_pTip->HasMissed() )
		{
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_release.wav", 0.98, ATTN_NORM, 0, 125 );

			EndAttack();
			return;
		}
	}
#endif
	const WeaponParameters& params = MyParameters();

	if( m_fireState != OFF )
	{
		m_pPlayer->m_iWeaponVolume = params.fire.weaponVolume.Get(false);

		if( m_flShootTime != 0.0 && gpGlobals->time > m_flShootTime )
		{
			SendWeaponAnim( BGRAPPLE_FIREWAITING );

			m_pPlayer->pev->punchangle.x += 2.0;

			Fire( m_pPlayer->GetGunPosition(), gpGlobals->v_forward );
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/bgrapple_pull.wav", 0.98, ATTN_NORM, 0, 125 );
			m_flShootTime = 0;
		}
	}
	else
	{
		m_bMomentaryStuck = true;

		SendWeaponAnim( BGRAPPLE_FIRE );

		m_pPlayer->m_iWeaponVolume = params.fire.weaponVolume.Get(false);;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
		{
			m_flShootTime = gpGlobals->time;
		}
		else
		{
			m_flShootTime = gpGlobals->time + 0.35;
		}
#endif
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_fire.wav", 0.98, ATTN_NORM, 0, 125 );
		m_fireState = CHARGE;
	}

	if( !m_pTip )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
		return;
	}

#if !CLIENT_DLL
	if( m_pTip->GetGrappleType() != GRAPPLE_FIXED && m_pTip->IsStuck() )
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecEnd = vecSrc + gpGlobals->v_forward * 16.0;

		TraceResult tr;

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr );

		if( tr.flFraction >= 1.0 )
		{
			UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr );
			if( tr.flFraction < 1.0 )
			{
				if (!tr.pHit || FNullEnt(tr.pHit) || ((CBaseEntity*)GET_PRIVATE(tr.pHit))->IsBSPModel())
				{
					FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer );
				}
			}
		}

		if( tr.flFraction < 1.0 )
		{
			CBaseEntity* pHit = Instance( tr.pHit );

			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if( pHit )
			{
				if( m_pTip )
				{
					bool bValidTarget = false;
					if( pHit->IsPlayer() )
					{
						m_pTip->SetGrappleTarget( pHit );
						bValidTarget = true;
					}
					else if( m_pTip->CheckTarget( pHit ) != GRAPPLE_NOT_A_TARGET )
					{
						bValidTarget = true;
					}
					if( bValidTarget )
					{
						if( m_flDamageTime + 0.5 < gpGlobals->time )
						{
							float flDamage = GetSkillValue("plr_grapple");

							if( g_pGameRules->IsMultiplayer() )
							{
								flDamage *= 2;
							}

							pHit->ApplyTraceAttack( m_pPlayer->pev, m_pPlayer->pev, DamageInfo{flDamage, DMG_CLUB}, gpGlobals->v_forward, &tr );

							m_flDamageTime = gpGlobals->time;

							const char* pszSample;

							switch( RANDOM_LONG( 0, 2 ) )
							{
							default:
							case 0: pszSample = "barnacle/bcl_chew1.wav"; break;
							case 1: pszSample = "barnacle/bcl_chew2.wav"; break;
							case 2: pszSample = "barnacle/bcl_chew3.wav"; break;
							}
							EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, pszSample, VOL_NORM, ATTN_NORM, 0, 125 );
						}
					}
				}
			}
		}
	}
#endif

	//TODO: CTF support - Solokiller
	/*
	if( g_pGameRules->IsMultiplayer() && g_pGameRules->IsCTF() )
	{
		m_flNextPrimaryAttack = gpGlobals->time;
	}
	else
	*/
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
	}
}

void CBarnacleGrapple::Fire( Vector vecOrigin, Vector vecDir )
{
#if !CLIENT_DLL
	Vector vecSrc = vecOrigin;

	Vector vecEnd = vecSrc + vecDir * 2048.0;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr );

	if( !tr.fAllSolid )
	{
		CBaseEntity* pHit = Instance( tr.pHit );
/*
		if( !pHit )
			pHit = CWorld::GetInstance();
*/
		if( pHit )
		{
			UpdateEffect();

			m_flDamageTime = gpGlobals->time;
		}
	}
#endif
}

void CBarnacleGrapple::EndAttack()
{
	m_fireState = OFF;
	SendWeaponAnim( BGRAPPLE_FIRERELEASE );

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_STATIC, "weapons/bgrapple_pull.wav", 0.0, ATTN_NONE, SND_STOP, 100 );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/bgrapple_release.wav", 1, ATTN_NORM);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.9;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;

	DestroyEffect();

	if( m_bGrappling && m_pPlayer->IsAlive() )
	{
		m_pPlayer->SetAnimation( PLAYER_IDLE );
	}

	m_pPlayer->pev->movetype = MOVETYPE_WALK;
	m_pPlayer->pev->flags &= ~(FL_IMMUNE_SLIME);
	m_pPlayer->m_afPhysicsFlags &= ~PFLAG_LATCHING;
}

void CBarnacleGrapple::CreateEffect()
{
#if !CLIENT_DLL
	DestroyEffect();

	m_pTip = GetClassPtr((CBarnacleGrappleTip *)NULL);
	m_pTip->Spawn();

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	Vector vecOrigin =
		m_pPlayer->GetGunPosition() +
		gpGlobals->v_forward * 16.0 +
		gpGlobals->v_right * 8.0 +
		gpGlobals->v_up * -8.0;

	Vector vecAngles = m_pPlayer->pev->v_angle;

	vecAngles.x = -vecAngles.x;

	m_pTip->SetPosition( vecOrigin, vecAngles, m_pPlayer );

	if( !m_pBeam )
	{
		m_pBeam = CBeam::BeamCreate( "sprites/tongue.spr", 16 );

		m_pBeam->EntsInit( m_pTip->entindex(), m_pPlayer->entindex() );

		m_pBeam->SetFlags( BEAM_FSOLID );

		m_pBeam->SetBrightness( 100.0 );

		m_pBeam->SetEndAttachment( 1 );

		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}
#endif
}

void CBarnacleGrapple::UpdateEffect()
{
#if !CLIENT_DLL
	if( !m_pBeam || !m_pTip )
		CreateEffect();
#endif
}

void CBarnacleGrapple::DestroyEffect()
{
#if !CLIENT_DLL
	UTIL_RemoveAndClean(m_pBeam);

	if( m_pTip )
	{
		m_pTip->Killed( NULL, NULL, GIB_NEVER );
		m_pTip = NULL;
	}
#endif
}
