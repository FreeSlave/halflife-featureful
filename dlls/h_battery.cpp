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
/*

===== h_battery.cpp ========================================================

  battery-related code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "skill.h"
#include "gamerules.h"
#include "effects.h"
#include "customentity.h"
#include "wallcharger.h"
#include "player.h"
#include "visuals_utils.h"

class CRecharge : public CWallCharger
{
public:
	int RechargeTime() override { return (int)g_pGameRules->FlHEVChargerRechargeTime(); }
	int ChargerCapacity() override { return (int)(pev->health > 0 ? pev->health : GetSkillValue("suitcharger")); }
	bool GiveCharge(CBaseEntity* pActivator) override
	{
		return pActivator->TakeArmor(this, 1);
	}

	const NamedSoundScript& LoopingSoundScript() override {
		return loopingSoundScript;
	}
	const NamedSoundScript& DenySoundScript() override {
		return denySoundScript;
	}
	const NamedSoundScript& ChargeStartSoundScript() override {
		return startSoundScript;
	}
	const NamedSoundScript& RechargeSoundScript() override {
		return rechargeSoundScript;
	}

	static const NamedSoundScript denySoundScript;
	static const NamedSoundScript startSoundScript;
	static const NamedSoundScript loopingSoundScript;
	static const NamedSoundScript rechargeSoundScript;
};

LINK_ENTITY_TO_CLASS( func_recharge, CRecharge )

const NamedSoundScript CRecharge::denySoundScript = {
	CHAN_ITEM,
	{"items/suitchargeno1.wav"},
	0.85f,
	ATTN_NORM,
	"SuitRecharge.Deny"
};

const NamedSoundScript CRecharge::startSoundScript = {
	CHAN_ITEM,
	{"items/suitchargeok1.wav"},
	1.0f,
	ATTN_NORM,
	"SuitRecharge.Start"
};

const NamedSoundScript CRecharge::loopingSoundScript = {
	CHAN_STATIC,
	{"items/suitcharge1.wav"},
	0.85f,
	ATTN_NORM,
	"SuitRecharge.ChargingLoop"
};

const NamedSoundScript CRecharge::rechargeSoundScript = {
	CHAN_ITEM,
	{},
	"SuitRecharge.Recharge"
};

//-------------------------------------------------------------
// Wall mounted suit charger (PS2 && Decay)
//-------------------------------------------------------------

class CRechargeGlassDecay : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;

	static const NamedVisual rechargeGlass;
};

const NamedVisual CRechargeGlassDecay::rechargeGlass = BuildVisual("SuitRecharge.Glass")
		.Model("models/hev_glass.mdl")
		.RenderMode(kRenderTransTexture)
		.Alpha(150);

void CRechargeGlassDecay::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;

	ApplyVisual(GetVisual(rechargeGlass));
}

void CRechargeGlassDecay::Precache()
{
	RegisterVisual(rechargeGlass);
}

LINK_ENTITY_TO_CLASS(item_recharge_glass, CRechargeGlassDecay)

#define RECHARGER_COIL_CONTROLLER 1
#define RECHARGER_COIL_CONTROLLER2 2
#define RECHARGER_ARM_CONTROLLER 3
class CRechargeDecay : public CBaseAnimating
{
public:
	void KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void EXPORT AnimateAndWork();
	void SearchForPlayer();
	void Off();
	void EXPORT Recharge();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override { return ( CBaseAnimating::ObjectCaps() | FCAP_CONTINUOUS_USE | FCAP_ONLYDIRECT_USE ); }
	void TurnChargeToPlayer(const Vector &player);
	void SetChargeState(int state);
	void SetChargeController(float yaw);
	void UpdateOnRemove() override;
	void TurnBeamOn()
	{
		if (m_beam)
			ClearBits(m_beam->pev->effects, EF_NODRAW);
	}
	void TurnBeamOff()
	{
		if (m_beam)
			SetBits(m_beam->pev->effects, EF_NODRAW);
	}

	int ChargerCapacity() { return (int)(pev->health > 0 ? pev->health : GetSkillValue("suitcharger")); }

	bool IsUsefulToDisplayHint(CBaseEntity* pPlayer) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	enum {
		Still,
		Deploy,
		Idle,
		GiveShot,
		Healing,
		RetractShot,
		RetractArm,
		Inactive
	};

	float m_flNextCharge; 
	int m_iJuice;
	int m_iState;
	float m_flSoundTime;
	float m_goToOffTime;
	bool m_goingToOff;
	bool m_playingChargeSound;
	CRechargeGlassDecay* m_glass;
	CBeam* m_beam;
	float m_currentYaw;
	float m_goalYaw;
	string_t m_triggerOnFirstUse;
	string_t m_triggerOnEmpty;
	bool m_missingSequence;

	static constexpr const char* deploySoundScript = "SuitRecharge.Deploy";

protected:
	void SetMySequence(const char* sequence);
	void CreateBeam();

	static const NamedVisual beamVisual;
};

TYPEDESCRIPTION CRechargeDecay::m_SaveData[] =
{
	DEFINE_FIELD( CRechargeDecay, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CRechargeDecay, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CRechargeDecay, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CRechargeDecay, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CRechargeDecay, m_goToOffTime, FIELD_TIME ),
	DEFINE_FIELD( CRechargeDecay, m_goingToOff, FIELD_BOOLEAN),
	DEFINE_FIELD( CRechargeDecay, m_playingChargeSound, FIELD_BOOLEAN),
	DEFINE_FIELD( CRechargeDecay, m_triggerOnFirstUse, FIELD_STRING),
	DEFINE_FIELD( CRechargeDecay, m_triggerOnEmpty, FIELD_STRING),
	DEFINE_FIELD( CRechargeDecay, m_missingSequence, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE( CRechargeDecay, CBaseAnimating )

const NamedVisual CRechargeDecay::beamVisual = BuildVisual("SuitRecharge.Beam")
		.Model("sprites/lgtning.spr")
		.BeamParams(5, 10)
		.RenderColor(0, 225, 0)
		.Alpha(225);

void CRechargeDecay::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "capacity" ) || FStrEq( pkvd->szKeyName, "CustomJuice" ) )
	{
		pev->health = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnEmpty" ) )
	{
		m_triggerOnEmpty = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnFirstUse" ) )
	{
		m_triggerOnFirstUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseAnimating::KeyValue( pkvd );
}

void CRechargeDecay::Spawn()
{
	m_iJuice = ChargerCapacity();
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	SetMyModel("models/hev.mdl");
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(pev, pev->origin);
	pev->skin = 0;

	InitBoneControllers();
	SetBoneController(RECHARGER_COIL_CONTROLLER, 360);

	if (m_iJuice > 0)
	{
		SetChargeState(Still);
		SetThink(&CRechargeDecay::AnimateAndWork);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		SetChargeState(Inactive);
	}
}

LINK_ENTITY_TO_CLASS(item_recharge, CRechargeDecay)

void CRechargeDecay::Precache()
{
	PrecacheMyModel("models/hev.mdl");

	RegisterAndPrecacheSoundScript(CRecharge::startSoundScript);
	RegisterAndPrecacheSoundScript(CRecharge::denySoundScript);
	RegisterAndPrecacheSoundScript(CRecharge::loopingSoundScript);
	RegisterAndPrecacheSoundScript(CRecharge::rechargeSoundScript);
	RegisterAndPrecacheSoundScript(deploySoundScript, CRecharge::startSoundScript);

	RegisterVisual(beamVisual);

	UTIL_PrecacheOther("item_recharge_glass", GetProjectileOverrides());
}

void CRechargeDecay::Activate()
{
	CreateBeam();
	if (m_iState != Idle)
		TurnBeamOff();
	m_glass = GetClassPtr( (CRechargeGlassDecay *)NULL );
	if (m_glass)
	{
		m_glass->AssignEntityOverrides(GetProjectileOverrides());
		m_glass->Spawn();
		UTIL_SetOrigin(m_glass->pev, pev->origin);
		m_glass->pev->angles = pev->angles;
	}

	CBaseAnimating::Activate();
}

void CRechargeDecay::AnimateAndWork()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_goalYaw < 0)
		m_currentYaw = Q_max(m_currentYaw - 10, m_goalYaw);
	else
		m_currentYaw = Q_min(m_currentYaw + 10, m_goalYaw);
	SetBoneController(RECHARGER_ARM_CONTROLLER, m_currentYaw);

	if (m_goingToOff)
	{
		if (m_goToOffTime <= gpGlobals->time)
			Off();
	}
	else
	{
		SearchForPlayer();
	}
}

void CRechargeDecay::SearchForPlayer()
{
	CBaseEntity* pEntity = 0;
	UTIL_MakeVectors( pev->angles );
	while((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), 64)) != 0) // this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
	{
		if (pEntity->IsPlayer() && pEntity->IsAlive())
		{
			CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pEntity);
			if (pPlayer->HasSuit() && pPlayer->CanHaveItem(this))
			{
				if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) {
					continue;
				}
				TurnChargeToPlayer(pEntity->pev->origin);
				switch (m_iState) {
				case RetractShot:
					if (m_fSequenceFinished || m_missingSequence)
						SetChargeState(Idle);
					break;
				case RetractArm:
					SetChargeState(Deploy);
					break;
				case Still:
					SetChargeState(Deploy);
					break;
				case Deploy:
					if (m_fSequenceFinished || m_missingSequence)
					{
						TurnBeamOn();
						SetChargeState(Idle);
					}
					break;
				case Idle:
					break;
				default:
					break;
				}

				break;
			}
		}
	}
	if (!pEntity || !pEntity->IsPlayer()) {
		switch (m_iState) {
		case Deploy:
		case Idle:
		case RetractShot:
			SetChargeState(RetractArm);
			break;
		case RetractArm:
			if (m_fSequenceFinished || m_missingSequence)
			{
				SetChargeState(Still);
				SetChargeController(0);
			}
			else
			{
				SetChargeController(m_currentYaw*0.75);
			}
			break;
		case Still:
			break;
		default:
			break;
		}
	}
}

void CRechargeDecay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if( !pCaller )
		return;
	// if it's not a player, ignore
	if( !pCaller->IsPlayer() )
		return;

	CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pCaller);

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !pPlayer->HasSuit() ) || pPlayer->pev->armorvalue >= pPlayer->MaxArmor() )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EmitSoundScript(CRecharge::denySoundScript);
		}
		return;
	}

	if (m_iState != Idle && m_iState != GiveShot && m_iState != Healing && m_iState != Inactive)
		return;

	m_goingToOff = true;
	// if there is no juice left, turn it off
	if( (m_iState == Healing || m_iState == GiveShot) && m_iJuice <= 0 )
	{
		pev->skin = 1;
		pev->nextthink = m_goToOffTime = gpGlobals->time;
	}
	else
	{
		m_goToOffTime = gpGlobals->time + 0.25f;
	}

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	TurnChargeToPlayer(pPlayer->pev->origin);
	switch (m_iState) {
	case Idle:
		m_flSoundTime = 0.56f + gpGlobals->time;
		SetChargeState(GiveShot);
		EmitSoundScript(CRecharge::startSoundScript);
		break;
	case GiveShot:
		if (m_fSequenceFinished || m_missingSequence)
		{
			SetChargeState(Healing);
		}
		break;
	case Healing:
		if (!m_playingChargeSound && m_flSoundTime <= gpGlobals->time)
		{
			m_playingChargeSound = true;
			EmitSoundScript(CRecharge::loopingSoundScript);
		}
		// We need to keep playing animation even though it's 1 frame only for controllers smoothing
		SetChargeState(Healing);
		break;
	default:
		ALERT(at_console, "Unexpected recharger state on use: %d\n", m_iState);
		break;
	}

	// charge the player
	if( pPlayer->pev->armorvalue < pPlayer->MaxArmor() )
	{
		if (m_triggerOnFirstUse)
		{
			FireTargets( STRING( m_triggerOnFirstUse ), pPlayer, this );
			m_triggerOnFirstUse = iStringNull;
		}
		m_iJuice--;
		if (m_iJuice <= 0)
		{
			pev->skin = 1;
			if (m_triggerOnEmpty)
			{
				FireTargets( STRING( m_triggerOnEmpty ), pPlayer, this );
			}
		}
		const float boneControllerValue = (m_iJuice / (float)ChargerCapacity()) * 360;
		SetBoneController(RECHARGER_COIL_CONTROLLER, 360 - boneControllerValue);
		SetBoneController(RECHARGER_COIL_CONTROLLER2,  boneControllerValue);

		pPlayer->TakeArmor(this, 1);
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;
}

void CRechargeDecay::Recharge()
{
	EmitSoundScript(CRecharge::rechargeSoundScript);
	m_iJuice = ChargerCapacity();
	SetBoneController(RECHARGER_COIL_CONTROLLER, 360);
	SetBoneController(RECHARGER_COIL_CONTROLLER2, 0);
	pev->skin = 0;
	SetChargeState(Still);
	SetThink( &CRechargeDecay::AnimateAndWork );
	pev->nextthink = gpGlobals->time;
}

void CRechargeDecay::Off()
{
	switch (m_iState) {
	case GiveShot:
	case Healing:
		if (m_playingChargeSound) {
			StopSoundScript(CRecharge::loopingSoundScript);
			m_playingChargeSound = false;
		}
		SetChargeState(RetractShot);
		break;
	case RetractShot:
		if (m_fSequenceFinished || m_missingSequence)
		{
			if (m_iJuice > 0) {
				SetChargeState(Idle);
				m_goingToOff = false;
				pev->nextthink = gpGlobals->time;
			} else {
				SetChargeState(RetractArm);
			}
		}
		break;
	case RetractArm:
	{
		if (m_fSequenceFinished || m_missingSequence)
		{
			m_currentYaw = m_goalYaw = 0;
			SetBoneController(RECHARGER_ARM_CONTROLLER, m_currentYaw);
			if (m_iJuice <= 0)
			{
				SetChargeState(Inactive);
				const float rechargeTime = g_pGameRules->FlHEVChargerRechargeTime();
				if (rechargeTime > 0 ) {
					pev->nextthink = gpGlobals->time + rechargeTime;
					SetThink( &CRechargeDecay::Recharge );
				}
			}
		}
		else
		{
			SetChargeController(m_currentYaw*0.75);
		}
		break;
	}
	default:
		break;
	}
}

void CRechargeDecay::SetMySequence(const char *sequence)
{
	bool shouldReset = false;
	int newSequence = LookupSequence( sequence );
	if (newSequence == -1) {
		m_missingSequence = true;
		if (pev->sequence != 0)
		{
			pev->sequence = 0;
			shouldReset = true;
		}
	}
	else
	{
		m_missingSequence = false;
		pev->sequence = newSequence;
		shouldReset = true;
	}

	if (shouldReset)
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}
}

void CRechargeDecay::SetChargeState(int state)
{
	m_iState = state;
	switch (state) {
	case Still:
		SetMySequence("rest");
		break;
	case Deploy:
		EmitSoundScript(deploySoundScript);
		SetMySequence("deploy");
		break;
	case Idle:
		SetMySequence("prep_charge");
		break;
	case GiveShot:
		SetMySequence("give_charge");
		break;
	case Healing:
		SetMySequence("charge_idle");
		break;
	case RetractShot:
		SetMySequence("retract_charge");
		break;
	case RetractArm:
		TurnBeamOff();
		SetMySequence("retract_arm");
		break;
	case Inactive:
		SetMySequence("rest");
	default:
		break;
	}
}

void CRechargeDecay::TurnChargeToPlayer(const Vector& player)
{
	float yaw = UTIL_VecToYaw( player - pev->origin ) - pev->angles.y;

	if( yaw > 180 )
		yaw -= 360;
	if( yaw < -180 )
		yaw += 360;

	SetChargeController( yaw );
}

void CRechargeDecay::SetChargeController(float yaw)
{
	m_goalYaw = yaw;
}

void CRechargeDecay::CreateBeam()
{
	CBeam *beam = CreateBeamFromVisual(GetVisual(beamVisual));
	if (!beam)
		return;

	beam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	beam->SetType( BEAM_ENTS );
	beam->SetStartEntity( entindex() );
	beam->SetEndEntity( entindex() );
	beam->SetStartAttachment(3);
	beam->SetEndAttachment(4);
	beam->RelinkBeam();

	m_beam = beam;
}

void CRechargeDecay::UpdateOnRemove()
{
	UTIL_RemoveAndClean(m_beam);
	UTIL_RemoveAndClean(m_glass);
	CBaseAnimating::UpdateOnRemove();
}

bool CRechargeDecay::IsUsefulToDisplayHint(CBaseEntity* pPlayer)
{
	if(m_iJuice <= 0)
		return false;
	if (pPlayer->IsPlayer())
	{
		CBasePlayer* p = (CBasePlayer*)pPlayer;
		return p->CanHaveItem(this);
	}
	return false;
}
