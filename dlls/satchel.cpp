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

#if !CLIENT_DLL
#include "game.h"
#include "gamerules.h"
#include "common_soundscripts.h"
#endif

enum satchel_state
{
	SATCHEL_IDLE = 0,
	SATCHEL_READY,
	SATCHEL_RELOAD
};

enum satchel_e
{
	SATCHEL_IDLE1 = 0,
	SATCHEL_FIDGET1,
	SATCHEL_DRAW,
	SATCHEL_DROP
};

enum satchel_radio_e
{
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

class CSatchel : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn() override;
	void Precache() override;
	int WeaponId() const override { return WEAPON_SATCHEL; }
	bool GetItemInfo(ItemInfo *p) override;
	WeaponParameters GetDefaultParameters() const override;
	bool AddToPlayer( CBasePlayer *pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	int AddDuplicate(CBasePlayerWeapon *pOriginal) override;
	bool CanDeploy() override;
	bool Deploy() override;
	bool IsUseable() override;
	bool CanBeDropped() override;

	void Holster() override;
	void ItemPreFrame() override;
	void WeaponIdle() override;
	void Throw();
	void Detonate(bool allowThrow);
	int ControlBehavior();
	void DrawSatchel();
	void DrawRadio();

	void GetWeaponData(weapon_data_t& data) override;
	void SetWeaponData(const weapon_data_t& data) override;
};

#if !CLIENT_DLL
class CSatchelCharge : public CGrenade
{
public:
	Vector m_lastBounceOrigin;	// Used to fix a bug in engine: when object isn't moving, but its speed isn't 0 and on ground isn't set
	void Spawn() override;
	void Precache() override;
	void BounceSound() override;
	void Explode( TraceResult *pTrace, int bitsDamageType ) override;

	void EXPORT SatchelSlide( CBaseEntity *pOther );
	void EXPORT SatchelThink();

	bool HandleDoorBlockage(CBaseEntity* pDoor) override;

	void Deactivate();
	int ObjectCaps() override;
	void EXPORT SatchelUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	static const NamedSoundScript bounceSoundScript;
};

LINK_ENTITY_TO_CLASS( monster_satchel, CSatchelCharge )

const NamedSoundScript CSatchelCharge::bounceSoundScript = {
	CHAN_VOICE,
	{"weapons/g_bounce1.wav", "weapons/g_bounce2.wav", "weapons/g_bounce3.wav"},
	"SatchelCharge.Bounce"
};

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CSatchelCharge::Deactivate()
{
	pev->solid = SOLID_NOT;
	UTIL_Remove( this );
}

void CSatchelCharge::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/w_satchel.mdl" );
	//UTIL_SetSize( pev, Vector( -16, -16, -4 ), Vector( 16, 16, 32 ) );	// Old box -- size of headcrab monsters/players get blocked by this
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );	// Uses point-sized, and can be stepped over
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CSatchelCharge::SatchelSlide );
	SetUse( &CSatchelCharge::SatchelUse );
	SetThink( &CSatchelCharge::SatchelThink );
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->gravity = 0.5f;
	pev->friction = 0.8f;

	pev->dmg = GetSkillValue("plr_satchel");
	// ResetSequenceInfo();
	pev->sequence = 1;
}

void CSatchelCharge::SatchelSlide( CBaseEntity *pOther )
{
	//entvars_t *pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector( 300, 300, 300 );
	pev->gravity = 1;// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 10 ), ignore_monsters, edict(), &tr );

	if( tr.flFraction < 1.0f )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if( !( pev->flags & FL_ONGROUND ) && pev->velocity.Length2D() > 10.0f )
	{
		// Fix for a bug in engine: when object isn't moving, but its speed isn't 0 and on ground isn't set
		if( pev->origin != m_lastBounceOrigin )
		BounceSound();
	}
	m_lastBounceOrigin = pev->origin;
	// There is no model animation so commented this out to prevent net traffic
	// StudioFrameAdvance();
}

void CSatchelCharge::SatchelThink()
{
	// There is no model animation so commented this out to prevent net traffic
	// StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if( pev->waterlevel == WL_Eyes )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8f;
		pev->avelocity = pev->avelocity * 0.9f;
		pev->velocity.z += 8;
	}
	else if( pev->waterlevel == WL_NotInWater )
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8.0f;
	}	
}

void CSatchelCharge::Precache()
{
	PRECACHE_MODEL( "models/w_satchel.mdl" );

	PrecacheBaseGrenadeSounds();
	RegisterAndPrecacheSoundScript(bounceSoundScript);
}

void CSatchelCharge::BounceSound()
{
	EmitSoundScript(bounceSoundScript);
}

void CSatchelCharge::Explode( TraceResult *pTrace, int bitsDamageType )
{
	edict_t* pOwner = pev->owner;
	CGrenade::Explode(pTrace, bitsDamageType);
	if (!FNullEnt(pOwner))
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(pOwner);
		if (pEntity->IsPlayer())
		{
			CBasePlayer* pPlayer = (CBasePlayer*)pEntity;
			pPlayer->m_needSatchelRecheck = true;
		}
	}
}

bool CSatchelCharge::HandleDoorBlockage(CBaseEntity *pDoor)
{
	if( satchelfix.value )
	{
		// Detonate satchels
		Use( pDoor, pDoor, USE_ON, 0 );
		return true;
	}
	return false;
}

static bool AnySatchelsLeft(edict_t* pOwner, CBaseEntity* satchelSelf = nullptr)
{
	bool anySatchelsLeft = false;
	CBaseEntity* pSatchel = nullptr;
	while ((pSatchel = UTIL_FindEntityByClassname(pSatchel, "monster_satchel")) != nullptr)
	{
		if (pSatchel != satchelSelf && pSatchel->pev->owner == pOwner)
		{
			anySatchelsLeft = true;
			break;
		}
	}
	return anySatchelsLeft;
}

void CSatchelCharge::SatchelUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (useType == USE_SET && pActivator && pActivator->edict() == pev->owner && g_modFeatures.satchels_pickable)
	{
		if (pActivator->IsPlayer())
		{
			CBasePlayer* pPlayer = (CBasePlayer*)pActivator;

			{
				if (pPlayer->GiveAmmo(1, "Satchel Charge") > 0)
				{
#if !CLIENT_DLL
					pPlayer->EmitSoundScript(Items::ammoPickupSoundScript);
#endif

					bool anySatchelsLeft = AnySatchelsLeft(pActivator->edict(), this);
					CSatchel* pSatchelWeapon = (CSatchel*)pPlayer->WeaponById(WEAPON_SATCHEL);
					if (pSatchelWeapon) {
						if (!anySatchelsLeft)
						{
							if (pPlayer->m_pActiveItem == pSatchelWeapon) {
								pSatchelWeapon->m_ForceSendAnimations = true;
								pSatchelWeapon->DrawSatchel();
								pSatchelWeapon->m_ForceSendAnimations = false;
							}
							else
								pSatchelWeapon->m_chargeReady = SATCHEL_RELOAD;
						}
					}

					SetThink(&CBaseEntity::SUB_Remove);
					pev->nextthink = gpGlobals->time;
				}
			}
		}
	}
	else
	{
		CGrenade::DetonateUse(pActivator, pCaller, useType, value);
	}
}

int CSatchelCharge::ObjectCaps()
{
	int caps = CBaseEntity::ObjectCaps();
	if (pev->owner && g_modFeatures.satchels_pickable)
	{
		caps |= FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE;
	}
	return caps;
}
#endif

LINK_WEAPON_TO_CLASS( weapon_satchel, CSatchel )

#if !CLIENT_DLL
TYPEDESCRIPTION	CSatchel::m_SaveData[] =
{
	DEFINE_FIELD( CSatchel, m_chargeReady, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CSatchel, CBasePlayerWeapon )
#endif

//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
int CSatchel::AddDuplicate( CBasePlayerWeapon *pOriginal )
{
#if !CLIENT_DLL
	CSatchel *pSatchel;
	int nNumSatchels, nSatchelsInPocket;
	CBaseEntity *ent;

	if( g_pGameRules->IsMultiplayer() )
	{
		if( satchelfix.value )
		{
			if( !pOriginal->m_pPlayer )
				return 1;

			nNumSatchels = 0;
			nSatchelsInPocket = pOriginal->m_pPlayer->m_rgAmmo[pOriginal->PrimaryAmmoIndex()];
			ent = NULL;

			while( ( ent = UTIL_FindEntityInSphere( ent, pOriginal->m_pPlayer->pev->origin, 4096 )) != NULL )
			{
				if( FClassnameIs( ent->pev, "monster_satchel" ))
					nNumSatchels += ent->pev->owner == pOriginal->m_pPlayer->edict();
			}
		}

		pSatchel = (CSatchel *)pOriginal;

		if( pSatchel->m_chargeReady != SATCHEL_IDLE
		    && ( satchelfix.value && nSatchelsInPocket + nNumSatchels > SATCHEL_MAX_CARRY - 1 ))
		{
			// player has some satchels deployed. Refuse to add more.
			return 0;
		}
	}
#endif
	return CBasePlayerWeapon::AddDuplicate( pOriginal );
}

//=========================================================
//=========================================================
bool CSatchel::AddToPlayer( CBasePlayer *pPlayer )
{
	bool bResult = CBasePlayerWeapon::AddToPlayer( pPlayer );
	m_chargeReady = SATCHEL_IDLE;// this satchel charge weapon now forgets that any satchels are deployed by it.
	return bResult;
}

void CSatchel::Spawn()
{
	const WeaponParameters& params = MyParameters();

	Precache();
	SetMyModel(params.worldModel.c_str());

	SetInitialAmmoAmount();
	InitMaxClip();

	FallInit();// get ready to fall down.

	SetWorldModelProps();
}

void CSatchel::Precache()
{
	PrecacheWeaponModels();
	PrecacheModelSounds();

	UTIL_PrecacheOther( "monster_satchel" );

	PrecacheDropAmmo();
}

bool CSatchel::GetItemInfo( ItemInfo *p )
{
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return true;
}

WeaponParameters CSatchel::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 1;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = "Satchel Charge";

	params.worldModel = "models/w_satchel.mdl";
	params.viewModel = "models/v_satchel.mdl";
	params.viewModelDetonator = "models/v_satchel_radio.mdl";
	params.playerModel = "models/p_satchel.mdl";
	params.playerModelDetonator = "models/p_satchel_radio.mdl";
	params.playerAnimExt = "trip";
	params.playerAnimExtDetonator = "hive";
	params.priority = -10;

	params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;

	params.dropAmmo.classname = "weapon_satchel";

	return params;
}

//=========================================================
//=========================================================
bool CSatchel::IsUseable()
{
	return CanDeploy();
}

bool CSatchel::CanBeDropped()
{
	// Disallow drop if the only thing left is radio
	return m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0;
}

bool CSatchel::CanDeploy()
{
	if( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 ) 
	{
		// player is carrying some satchels
		return true;
	}

	if( m_chargeReady )
	{
		// player isn't carrying any satchels, but has some out
		return true;
	}
	return false;
}

bool CSatchel::Deploy()
{
	const WeaponParameters& params = MyParameters();

	if (m_chargeReady == SATCHEL_RELOAD)
		m_chargeReady = SATCHEL_IDLE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;

	bool result;
	if( m_chargeReady )
		result = DefaultDeploy( DetonatorViewModelToDeploy(params.DetonatorViewModel()), params.DetonatorPlayerModel(), SATCHEL_RADIO_DRAW, params.playerAnimExtDetonator.c_str() );
	else
		result = DefaultDeploy( ViewModelToDeploy(params.ViewModel()), params.PlayerModel(), SATCHEL_DRAW, params.playerAnimExt.c_str() );

	if (result)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	}
	return result;
}

void CSatchel::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if( m_chargeReady )
	{
		SendWeaponAnim( SATCHEL_RADIO_HOLSTER );
	}
	else
	{
		SendWeaponAnim( SATCHEL_DROP );
	}
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM );

	if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] && m_chargeReady != SATCHEL_READY )
	{
		m_pPlayer->ClearWeaponBit(WeaponId());
		DestroyItem();
	}
}

void CSatchel::PrimaryAttack()
{
	if (ControlBehavior() == 0)
		Detonate(true);
	else if( m_chargeReady != SATCHEL_RELOAD )
	{
		Throw();
	}
}

void CSatchel::SecondaryAttack()
{
	if (ControlBehavior() == 0)
	{
		if( m_chargeReady != SATCHEL_RELOAD )
		{
			Throw();
		}
	}
	else
		Detonate(false);
}

void CSatchel::Throw()
{
	if( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 )
	{
		const WeaponParameters& params = MyParameters();
#if !CLIENT_DLL
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

		CBaseEntity *pSatchel = Create( "monster_satchel", vecSrc, Vector( 0, 0, 0 ), m_pPlayer->edict() );
		pSatchel->pev->velocity = vecThrow;
		pSatchel->pev->avelocity.y = 400;

		m_pPlayer->pev->viewmodel = MAKE_STRING(params.viewModelDetonator.c_str());
		if (g_modFeatures.weapon_p_models)
			m_pPlayer->pev->weaponmodel = MAKE_STRING(params.playerModelDetonator.c_str());
#else
		LoadVModel( params.viewModelDetonator.c_str(), m_pPlayer );
#endif

		SendWeaponAnim( SATCHEL_RADIO_DRAW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_chargeReady = SATCHEL_READY;

		SpendAmmo();

		m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
}

void CSatchel::Detonate(bool allowThrow)
{
	switch( m_chargeReady )
	{
	case SATCHEL_IDLE:
		if (allowThrow)
		{
			Throw();
		}
		break;
	case SATCHEL_READY:
		{
			SendWeaponAnim( SATCHEL_RADIO_FIRE );

			edict_t *pPlayer = m_pPlayer->edict();

			CBaseEntity *pSatchel = NULL;

			while( ( pSatchel = UTIL_FindEntityInSphere( pSatchel, m_pPlayer->pev->origin, 4096 )) != NULL )
			{
				if( pSatchel->pev->owner == pPlayer && FClassnameIs( pSatchel->pev, "monster_satchel" ) )
				{
					pSatchel->Use( m_pPlayer, m_pPlayer, USE_ON, 0 );
				}
			}

			m_chargeReady = SATCHEL_RELOAD;
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
			break;
		}
	case SATCHEL_RELOAD:
		// we're reloading, don't allow fire
		break;
	}
}

#if CLIENT_DLL
extern cvar_t *cl_satchelcontrol;
#endif

int CSatchel::ControlBehavior()
{
#if CLIENT_DLL
	if (cl_satchelcontrol)
		return (int)cl_satchelcontrol->value;
	return 0;
#else
	if (m_pPlayer)
		return m_pPlayer->m_iSatchelControl;
	return 0;
#endif
}

void CSatchel::WeaponIdle()
{
	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	switch( m_chargeReady )
	{
	case SATCHEL_IDLE:
		SendWeaponAnim( SATCHEL_FIDGET1 );
		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );
		break;
	case SATCHEL_READY:
		SendWeaponAnim( SATCHEL_RADIO_FIDGET1 );
		// use hivehand animations
		strcpy( m_pPlayer->m_szAnimExtention, "hive" );
		break;
	case SATCHEL_RELOAD:
		if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

		DrawSatchel();
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
}

void CSatchel::ItemPreFrame()
{
	CBasePlayerWeapon::ItemPreFrame();
	if (m_pPlayer->m_needSatchelRecheck)
	{
		if (m_chargeReady == SATCHEL_READY && m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0)
		{
#if !CLIENT_DLL
			bool anySatchelsLeft = AnySatchelsLeft(m_pPlayer->edict());
			if (!anySatchelsLeft)
			{
				m_chargeReady = SATCHEL_RELOAD;
				m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
			}
#endif
		}

		m_pPlayer->m_needSatchelRecheck = false;
	}
}

void CSatchel::DrawSatchel()
{
	const WeaponParameters& params = MyParameters();
#if !CLIENT_DLL
	m_pPlayer->pev->viewmodel = MAKE_STRING(ViewModelToDeploy(params.ViewModel()));
	if (g_modFeatures.weapon_p_models)
		m_pPlayer->pev->weaponmodel = MAKE_STRING(params.PlayerModel());
#else
	LoadVModel( params.ViewModel(), m_pPlayer );
#endif
	SendWeaponAnim( SATCHEL_DRAW );

	// use tripmine animations
	strcpy( m_pPlayer->m_szAnimExtention, params.PlayerAnimExt() );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_chargeReady = SATCHEL_IDLE;
}

void CSatchel::DrawRadio()
{
	const WeaponParameters& params = MyParameters();
#if !CLIENT_DLL
	m_pPlayer->pev->viewmodel = MAKE_STRING(DetonatorViewModelToDeploy(params.DetonatorViewModel()));
	if (g_modFeatures.weapon_p_models)
		m_pPlayer->pev->weaponmodel = MAKE_STRING(params.DetonatorPlayerModel());
#else
	LoadVModel( params.DetonatorViewModel(), m_pPlayer );
#endif

	SendWeaponAnim( SATCHEL_RADIO_DRAW );

	strcpy( m_pPlayer->m_szAnimExtention, params.DetonatorPlayerAnimExt() );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_chargeReady = SATCHEL_READY;
}

void CSatchel::GetWeaponData(weapon_data_t& data)
{
	data.iuser1 = m_chargeReady;
}

void CSatchel::SetWeaponData(const weapon_data_t& data)
{
	m_chargeReady = data.iuser1;
}

//=========================================================
// DeactivateSatchels - removes all satchels owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
#if !CLIENT_DLL
void DeactivateSatchels( CBasePlayer *pOwner )
{
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_satchel" );

	while( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CSatchelCharge *pSatchel = (CSatchelCharge *)pEnt;

		if( pSatchel )
		{
			if( pSatchel->pev->owner == pOwner->edict() )
			{
				pSatchel->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_satchel" );
	}
}
#endif
