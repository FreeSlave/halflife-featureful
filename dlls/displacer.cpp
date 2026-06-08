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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "mod_features.h"
#include "shake.h"

#if !CLIENT_DLL
#include "game.h"
#include "gamerules.h"
#include "displacerball.h"
#endif

#if !CLIENT_DLL
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );
#endif // !defined ( CLIENT_DLL )

class CDisplacer : public CConfigurableWeapon
{
public:
#if !CLIENT_DLL
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif
	void Precache() override;
	int WeaponId() const override { return WEAPON_DISPLACER; }

	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Holster() override;

	enum DISPLACER_FIREMODE { FIREMODE_FORWARD = 1, FIREMODE_BACKWARD };

	void ClearSpin();
	void EXPORT SpinUp();
	void EXPORT Teleport();
	void EXPORT Displace();
private:
	int m_iFireMode;
	unsigned short m_usDisplacer;
};

LINK_WEAPON_TO_CLASS(weapon_displacer, CDisplacer)

#if !CLIENT_DLL
TYPEDESCRIPTION	CDisplacer::m_SaveData[] =
{
	DEFINE_FIELD( CDisplacer, m_iFireMode, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CDisplacer, CConfigurableWeapon )
#endif

bool CDisplacer::GetItemInfo(ItemInfo *p)
{
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 5;
	p->iPosition = 1;
#else
	p->iSlot = 3;
	p->iPosition = 6;
#endif

	return true;
}

WeaponParameters CDisplacer::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 40;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "uranium";

	params.worldModel = "models/w_displacer.mdl";
	params.viewModel = "models/v_displacer.mdl";
	params.playerModel = "models/p_displacer.mdl";
	params.playerAnimExt = "egon";
	params.priority = 20;

	params.deploy.animIndex = DISPLACER_DRAW;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{
		WeaponParameters::IdleAnim{DISPLACER_IDLE1, 0.5f, 3.0f},
		WeaponParameters::IdleAnim{DISPLACER_IDLE2, 0.5f, 3.0f}
	};

	// Primary fire
	params.fire.ammoPerFire = 20;
	params.fire.autoAimDegree = AUTOAIM_2DEGREES;
	params.fire.delayAfterEmpty = 0.5f;
	params.fire.cycleTime = 1.6f;

	params.fire.emptySound.main = {
		CHAN_WEAPON,
		{"buttons/button11.wav"},
		0.9f,
		ATTN_NORM,
		PITCH_NORM
	};
	params.fire.useStandardEmptySound.main = false;
	//

	// Alt fire
	params.fire.ammoPerFire.alt = 60;
	params.fire.cycleTime.alt = 4.0f;
	//

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.holster.animIndex = DISPLACER_HOLSTER;
	params.holster.attackDelay = 1.0f;
	params.holster.idleDelay = 1.0f;

	params.dropAmmo.classname = "ammo_gaussclip";

	return params;
}

void CDisplacer::Precache()
{
	PrecacheWeaponModels();
	PrecacheModelSounds();

	PRECACHE_SOUND("weapons/displacer_fire.wav");
	PRECACHE_SOUND("weapons/displacer_self.wav");
	PRECACHE_SOUND("weapons/displacer_spin.wav");
	PRECACHE_SOUND("weapons/displacer_spin2.wav");

	PRECACHE_SOUND("buttons/button11.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");

	UTIL_PrecacheOther("displacer_ball");

	m_usDisplacer = PRECACHE_EVENT(1, "events/displacer.sc");

	PrecacheDropAmmo();
}

void CDisplacer::Holster()
{
	m_fInReload = false;// cancel any reload in progress.

	ClearSpin();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	SendWeaponAnim(DISPLACER_HOLSTER);
}

void CDisplacer::SecondaryAttack()
{
	const WeaponParameters& params = MyParameters();
	if (params.secondaryFireType == SecondaryFireType::DISABLED)
		return;

	if (m_fFireOnEmpty || !HasAmmoToFire(params.fire.ammoPerFire.Get(true)))
	{
		PlayEmptySound(true);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iFireMode = FIREMODE_BACKWARD;

	SetThink (&CDisplacer::SpinUp);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 4.0;
	pev->nextthink = gpGlobals->time;
}

void CDisplacer::PrimaryAttack()
{
	const WeaponParameters& params = MyParameters();

	if ( m_fFireOnEmpty || !HasAmmoToFire(params.fire.ammoPerFire.Get(false)))
	{
		PlayEmptySound(false);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}
	m_iFireMode = FIREMODE_FORWARD;

	SetThink (&CDisplacer::SpinUp);
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.6;
	pev->nextthink = gpGlobals->time;
}

void CDisplacer::ClearSpin()
{
	switch (m_iFireMode)
	{
	case FIREMODE_FORWARD:
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav");
		break;
	case FIREMODE_BACKWARD:
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin2.wav");
		break;
	}
}

void CDisplacer::SpinUp()
{
	SendWeaponAnim( DISPLACER_SPINUP );

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usDisplacer, 0, g_vecZero, g_vecZero, 0, 0, 0, 0, 0, 1);

	if( m_iFireMode == FIREMODE_FORWARD )
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav", 1, ATTN_NORM );
		SetThink (&CDisplacer::Displace);
	}
	else
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin2.wav", 1, ATTN_NORM );
		SetThink (&CDisplacer::Teleport);
	}
	pev->nextthink = gpGlobals->time + 0.9;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.3;
}

void CDisplacer::Displace()
{
	const WeaponParameters& params = MyParameters();

	ClearSpin();

	SendWeaponAnim( DISPLACER_FIRE );
	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/displacer_fire.wav", 1, ATTN_NORM );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->punchangle.x -= 2;
#if !CLIENT_DLL
	Vector vecSrc;
	SpendAmmo(params.fire.ammoPerFire.Get(false));

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = m_pPlayer->GetGunPosition();
	vecSrc += gpGlobals->v_forward	* 22;
	vecSrc += gpGlobals->v_right	* 8;
	vecSrc += gpGlobals->v_up		* -12;

	ProjectileParameters projectileParams("displacer_ball", vecSrc, m_pPlayer->pev->v_angle, gpGlobals->v_forward, m_pPlayer);
	projectileParams.pLauncher = this;
	CBaseEntity::CreateAndLaunchAsProjectile(projectileParams);

	SetThink( NULL );
#endif
}

#if !CLIENT_DLL
extern CBaseEntity* GetDisplacerEarthTarget(CBaseEntity* pOther);
#endif

void CDisplacer::Teleport()
{
	const WeaponParameters& params = MyParameters();

	ClearSpin();
#if !CLIENT_DLL
	CBaseEntity *pDestination = nullptr;

	if( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() )
	{
		pDestination = GetClassPtr( (CBaseEntity *)VARS( EntSelectSpawnPoint( m_pPlayer ) ) );
	}
	else
	{
		if( !m_pPlayer->m_fInXen )
		{
			CBaseEntity *pDisplacerTarget = nullptr;
			while ((pDisplacerTarget = UTIL_FindEntityByClassname( pDisplacerTarget, "info_displacer_xen_target" )) != NULL)
			{
				if (!FBitSet(pDisplacerTarget->pev->spawnflags, SF_DISPLACER_TARGET_DISABLED))
				{
					pDestination = pDisplacerTarget;
					break;
				}
			}
		}
		else
			pDestination = GetDisplacerEarthTarget(m_pPlayer);
	}

	if( pDestination )
	{
		Vector newOrigin = pDestination->pev->origin;

		if( (m_pPlayer->m_afPhysicsFlags & PFLAG_ONROPE) )
			m_pPlayer->LetGoRope();

		// UTIL_ScreenFade( m_pPlayer, Vector( 0, 200, 0 ), 0.5, 0.5, 255, FFADE_IN );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();

		SpendAmmo(params.fire.ammoPerFire.Get(true));

		UTIL_CleanSpawnPoint( newOrigin, 50 );

		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_self.wav", 1, ATTN_NORM );
	 	CDisplacerBall::SelfCreate(m_pPlayer->pev, m_pPlayer->pev->origin);

		newOrigin.z += 37;

		m_pPlayer->pev->flags &= ~FL_ONGROUND;
		m_pPlayer->m_DisplacerReturn = m_pPlayer->pev->origin;
		m_pPlayer->m_DisplacerSndRoomtype = m_pPlayer->m_SndRoomtype;
		UTIL_SetOrigin(m_pPlayer->pev, newOrigin);

		m_pPlayer->pev->angles = pDestination->pev->angles;
		m_pPlayer->pev->v_angle = pDestination->pev->angles;
		m_pPlayer->pev->fixangle = 1;
		m_pPlayer->pev->velocity = m_pPlayer->pev->basevelocity = g_vecZero;

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.0f;

		if( !g_pGameRules->IsMultiplayer())
		{
			m_pPlayer->m_fInXen = !m_pPlayer->m_fInXen;
			if (m_pPlayer->m_fInXen)
				m_pPlayer->pev->gravity = 0.6f;
			else
				m_pPlayer->pev->gravity = 1.0f;
		}
	}
	else
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "buttons/button11.wav", 0.9f, ATTN_NORM );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.0f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.9;
	}

	SetThink( NULL );
#endif
}
