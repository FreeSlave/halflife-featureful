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
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "wallcharger.h"
#include "game.h"

extern int gmsgItemPickup;

class CHealthKit : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	bool MyTouch( CBasePlayer *pPlayer ) override;

	static const NamedSoundScript pickupSoundScript;
protected:
	virtual int DefaultCapacity() { return gSkillData.healthkitCapacity; }
};

LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit )

const NamedSoundScript CHealthKit::pickupSoundScript = {
	CHAN_ITEM,
	{"items/smallmedkit1.wav"},
	"HealthKit.Pickup"
};

void CHealthKit::Spawn()
{
	Precache();
	SetMyModel( "models/w_medkit.mdl" );

	CItem::Spawn();
}

void CHealthKit::Precache()
{
	PrecacheMyModel( "models/w_medkit.mdl" );
	RegisterAndPrecacheSoundScript(pickupSoundScript);
}

bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	if( pPlayer->pev->deadflag != DEAD_NO )
	{
		return false;
	}

	const bool healed = pPlayer->pev->health < pPlayer->pev->max_health;
	if( pPlayer->TakeHealth( this, pev->health > 0 ? pev->health : DefaultCapacity(), HEAL_CHARGE ) )
	{
		if (healed) {
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			pPlayer->EmitSoundScript(GetSoundScript(pickupSoundScript));
		}

		return true;
	}

	return false;
}

//-------------------------------------------------------------
// Base class for wall chargers
//-------------------------------------------------------------

#define SF_WALLCHARGER_STARTOFF 1
#define SF_WALLCHARGER_ONLYDIRECT 16

void CWallCharger::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	if (FBitSet(pev->spawnflags, SF_WALLCHARGER_STARTOFF))
	{
		m_iJuice = 0;
	}
	else
	{
		m_iJuice = ChargerCapacity();
	}
	pev->frame = 0;
}

void CWallCharger::Precache()
{
	RegisterAndPrecacheSoundScript(ChargeStartSoundScript());
	RegisterAndPrecacheSoundScript(DenySoundScript());
	RegisterAndPrecacheSoundScript(LoopingSoundScript());
	RegisterAndPrecacheSoundScript(RechargeSoundScript());

	const char* chargeStartSound = CustomChargeStartSound();
	if (chargeStartSound)
		PRECACHE_SOUND(chargeStartSound);

	const char* denySound = CustomDenySound();
	if (denySound)
		PRECACHE_SOUND(denySound);

	const char* loopingSound = CustomLoopingSound();
	if (loopingSound)
		PRECACHE_SOUND(loopingSound);

	const char* rechargeSound = CustomRechargeSound();
	if (rechargeSound)
		PRECACHE_SOUND(rechargeSound);
}

int CWallCharger::ObjectCaps()
{
	return ( CBaseEntity::ObjectCaps() | FCAP_CONTINUOUS_USE
			| (FBitSet(pev->spawnflags, SF_WALLCHARGER_ONLYDIRECT)?FCAP_ONLYDIRECT_USE:0) )
			& ~FCAP_ACROSS_TRANSITION;
}

void CWallCharger::Off()
{
	// Stop looping sound.
	if( m_iOn > 1 )
		StopChargerSound(LoopingSoundScript(), CustomLoopingSound());

	m_iOn = 0;

	SetThink( &CBaseEntity::SUB_DoNothing );
	if ( m_iJuice <= 0 )
	{
		if ( ( m_iReactivate = RechargeTime() ) > 0 )
		{
			pev->nextthink = pev->ltime + m_iReactivate;
			SetThink( &CWallCharger::Recharge );
		}
	}
}

void CWallCharger::Recharge()
{
	if (m_triggerOnRecharged)
	{
		FireTargets( STRING( m_triggerOnRecharged ), this, this );
	}
	PlayChargerSound(RechargeSoundScript(), CustomRechargeSound());
	m_iJuice = ChargerCapacity();
	pev->frame = OnStateFrame();
	SetThink( &CBaseEntity::SUB_DoNothing );
}

const char* CWallCharger::CustomLoopingSound()
{
	return pev->noise ? STRING(pev->noise) : nullptr;
}
const char* CWallCharger::CustomDenySound()
{
	return pev->noise1 ? STRING(pev->noise1) : nullptr;
}
const char* CWallCharger::CustomChargeStartSound()
{
	return pev->noise2 ? STRING(pev->noise2) : nullptr;
}
const char* CWallCharger::CustomRechargeSound()
{
	return pev->noise3 ? STRING(pev->noise3) : nullptr;
}

void CWallCharger::PlayChargerSound(const NamedSoundScript& soundScript, const char* customSample)
{
	if (customSample)
		EmitSoundScriptSelectedSample(soundScript, customSample);
	else
		EmitSoundScript(soundScript);
}

void CWallCharger::StopChargerSound(const NamedSoundScript& soundScript, const char* customSample)
{
	if (customSample)
		StopSoundScriptSelectedSample(soundScript, customSample);
	else
		StopSoundScript(soundScript);
}

TYPEDESCRIPTION CWallCharger::m_SaveData[] =
{
	DEFINE_FIELD( CWallCharger, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallCharger, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallCharger, m_triggerOnFirstUse, FIELD_STRING ),
	DEFINE_FIELD( CWallCharger, m_triggerOnEmpty, FIELD_STRING ),
	DEFINE_FIELD( CWallCharger, m_triggerOnRecharged, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CWallCharger, CBaseEntity )

void CWallCharger::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "style" ) ||
		FStrEq( pkvd->szKeyName, "height" ) ||
		FStrEq( pkvd->szKeyName, "value1" ) ||
		FStrEq( pkvd->szKeyName, "value2" ) ||
		FStrEq( pkvd->szKeyName, "value3" ) )
	{
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "dmdelay" ) )
	{
		m_iReactivate = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnEmpty" ) )
	{
		m_triggerOnEmpty = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnRecharged" ) )
	{
		m_triggerOnRecharged = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnFirstUse" ) )
	{
		m_triggerOnFirstUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "capacity" ) || FStrEq( pkvd->szKeyName, "CustomJuice" ) )
	{
		pev->health = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomLoopSound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomDeniedSound" ) )
	{
		pev->noise1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomStartSound" ) )
	{
		pev->noise2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomRechargeSound" ) )
	{
		pev->noise3 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWallCharger::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator || !pActivator->IsPlayer())
	{
		if (useType == USE_TOGGLE)
		{
			useType = m_iJuice > 0 ? USE_OFF : USE_ON;
		}
		switch (useType) {
		case USE_OFF:
			if (m_iJuice > 0)
			{
				PlayChargerSound(DenySoundScript(), CustomDenySound());
				m_iJuice = 0;
				pev->frame = OffStateFrame();
				Off();
			}
			return;
		case USE_ON:
			if (m_iJuice <= 0)
			{
				Recharge();
			}
			return;
		default:
			return;
		}
	}

	// if there is no juice left, turn it off
	if( m_iJuice <= 0 )
	{
		if (m_triggerOnEmpty && pev->frame == OnStateFrame())
		{
			FireTargets( STRING( m_triggerOnEmpty ), this, this );
		}
		pev->frame = OffStateFrame();
		Off();
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pActivator;
	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || !(pPlayer->HasSuit() || AllowNoSuit()) || !pPlayer->CanHaveItem(this) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			PlayChargerSound(DenySoundScript(), CustomDenySound());
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25f;
	SetThink( &CWallCharger::Off );

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;

	// charge the player
	if( GiveCharge(pActivator) )
	{
		if (m_triggerOnFirstUse)
		{
			FireTargets( STRING( m_triggerOnFirstUse ), this, this );
			m_triggerOnFirstUse = 0;
		}
		m_iJuice--;
	}
	else
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			PlayChargerSound(DenySoundScript(), CustomDenySound());
		}
		if( m_iOn > 1 )
			StopChargerSound(LoopingSoundScript(), CustomLoopingSound());
		m_iOn = 0;
		return;
	}

	// Play the on sound or the looping charging sound
	if( !m_iOn )
	{
		m_iOn++;
		PlayChargerSound(ChargeStartSoundScript(), CustomChargeStartSound());
		m_flSoundTime = 0.56f + gpGlobals->time;
	}
	if( ( m_iOn == 1 ) && ( m_flSoundTime <= gpGlobals->time ) )
	{
		m_iOn++;
		PlayChargerSound(LoopingSoundScript(), CustomLoopingSound());
	}
}

int CWallCharger::OnStateFrame()
{
	if (FBitSet(pev->spawnflags, SF_WALLCHARGER_STARTOFF))
		return 1;
	return 0;
}

int CWallCharger::OffStateFrame()
{
	if (FBitSet(pev->spawnflags, SF_WALLCHARGER_STARTOFF))
		return 0;
	return 1;
}

bool CWallCharger::CalcRatio( CBaseEntity *pLocus, float* outResult )
{
	*outResult = m_iJuice / static_cast<float>(ChargerCapacity());
	return true;
}

bool CWallCharger::IsUsefulToDisplayHint(CBaseEntity* pPlayer)
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

//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CWallCharger
{
public:
	int RechargeTime() override { return (int)g_pGameRules->FlHealthChargerRechargeTime(); }
	int ChargerCapacity() override { return (int)(pev->health > 0 ? pev->health : gSkillData.healthchargerCapacity); }
	bool GiveCharge(CBaseEntity* pActivator) override
	{
		return pActivator->TakeHealth( this, 1, HEAL_CHARGE ) > 0;
	}
	bool AllowNoSuit() override {
		return g_modFeatures.nosuit_allow_healthcharger;
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

LINK_ENTITY_TO_CLASS( func_healthcharger, CWallHealth )

const NamedSoundScript CWallHealth::denySoundScript = {
	CHAN_ITEM,
	{"items/medshotno1.wav"},
	1.0f,
	ATTN_NORM,
	"WallHealth.Deny"
};

const NamedSoundScript CWallHealth::startSoundScript = {
	CHAN_ITEM,
	{"items/medshot4.wav"},
	1.0f,
	ATTN_NORM,
	"WallHealth.Start"
};

const NamedSoundScript CWallHealth::loopingSoundScript = {
	CHAN_STATIC,
	{"items/medcharge4.wav"},
	1.0f,
	ATTN_NORM,
	"WallHealth.ChargingLoop"
};

const NamedSoundScript CWallHealth::rechargeSoundScript = {
	CHAN_ITEM,
	{"items/medshot4.wav"},
	1.0f,
	ATTN_NORM,
	"WallHealth.Recharge"
};

//-------------------------------------------------------------
// Wall mounted health kit (PS2 && Decay)
//-------------------------------------------------------------

class CWallHealthJarDecay : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	void Think() override;
	void Update(bool slosh, float value);
	void ToRest();

	static const NamedVisual wallHealthTank;
};

const NamedVisual CWallHealthJarDecay::wallHealthTank = BuildVisual("WallHealth.Tank")
		.Model("models/health_charger_both.mdl")
		.RenderMode(kRenderTransTexture)
		.Alpha(180);

void CWallHealthJarDecay::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;

	ApplyVisual(GetVisual(wallHealthTank));
	InitBoneControllers();
}

void CWallHealthJarDecay::Precache()
{
	RegisterVisual(wallHealthTank);
}

void CWallHealthJarDecay::Think()
{
	if (pev->sequence > 0)
	{
		StudioFrameAdvance();
		if (pev->sequence == 2 && m_fSequenceFinished)
		{
			pev->sequence = 0;
		}
		else
		{
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
}

void CWallHealthJarDecay::Update(bool slosh, float value)
{
	if (slosh && pev->sequence != 1)
	{
		pev->sequence = 1;
		ResetSequenceInfo();
		pev->frame = 0;
		m_fSequenceLoops = true;
		pev->nextthink = gpGlobals->time;
	}
	const float jarBoneControllerValue = value * 11 - 11;
	SetBoneController(0,  jarBoneControllerValue );
}

void CWallHealthJarDecay::ToRest()
{
	if (pev->sequence == 1)
	{
		pev->sequence = 2;
		ResetSequenceInfo();
		pev->frame = 0;
	}
}

LINK_ENTITY_TO_CLASS(item_healthcharger_jar, CWallHealthJarDecay)

class CWallHealthDecay : public CBaseAnimating
{
public:
	void KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	void Precache() override;
	void EXPORT AnimateAndWork();
	void SearchForPlayer();
	void Off();
	void EXPORT Recharge();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override { return ( CBaseAnimating::ObjectCaps() | FCAP_CONTINUOUS_USE | FCAP_ONLYDIRECT_USE ); }
	void TurnNeedleToPlayer(const Vector &player);
	void SetNeedleState(int state);
	void SetNeedleController(float yaw);
	void UpdateOnRemove() override;
	void UpdateJar();
	int ChargerCapacity() { return (int)(pev->health > 0 ? pev->health : gSkillData.healthchargerCapacity); }
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
	CWallHealthJarDecay* m_jar;
	float m_currentYaw;
	float m_goalYaw;
	string_t m_triggerOnFirstUse;
	string_t m_triggerOnEmpty;

protected:
	void SetMySequence(const char* sequence);
};

TYPEDESCRIPTION CWallHealthDecay::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealthDecay, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_goToOffTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_goingToOff, FIELD_BOOLEAN),
	DEFINE_FIELD( CWallHealthDecay, m_playingChargeSound, FIELD_BOOLEAN),
	DEFINE_FIELD( CWallHealthDecay, m_triggerOnFirstUse, FIELD_STRING),
	DEFINE_FIELD( CWallHealthDecay, m_triggerOnEmpty, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE( CWallHealthDecay, CBaseAnimating )

void CWallHealthDecay::KeyValue( KeyValueData *pkvd )
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

void CWallHealthDecay::Spawn()
{
	m_iJuice = ChargerCapacity();
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	SetMyModel("models/health_charger_body.mdl");
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(pev, pev->origin);
	pev->skin = 0;

	InitBoneControllers();

	if (m_iJuice > 0)
	{
		m_iState = Still;
		SetThink(&CWallHealthDecay::AnimateAndWork);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		m_iState = Inactive;
	}
}

LINK_ENTITY_TO_CLASS(item_healthcharger, CWallHealthDecay)

void CWallHealthDecay::Precache()
{
	PrecacheMyModel("models/health_charger_body.mdl");

	RegisterAndPrecacheSoundScript(CWallHealth::startSoundScript);
	RegisterAndPrecacheSoundScript(CWallHealth::denySoundScript);
	RegisterAndPrecacheSoundScript(CWallHealth::loopingSoundScript);
	RegisterAndPrecacheSoundScript(CWallHealth::rechargeSoundScript);

	m_jar = GetClassPtr( (CWallHealthJarDecay *)NULL );
	if (m_jar)
	{
		m_jar->m_ownerEntTemplate = m_entTemplate;
		m_jar->Spawn();
		UTIL_SetOrigin(m_jar->pev, pev->origin);
		m_jar->pev->angles = pev->angles;
		UpdateJar();
	}
}

void CWallHealthDecay::AnimateAndWork()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_goalYaw < 0)
		m_currentYaw = Q_max(m_currentYaw - 15, m_goalYaw);
	else
		m_currentYaw = Q_min(m_currentYaw + 15, m_goalYaw);
	SetBoneController(0, m_currentYaw);

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

void CWallHealthDecay::SearchForPlayer()
{
	CBaseEntity* pEntity = 0;
	UTIL_MakeVectors( pev->angles );
	while((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), 64)) != 0) // this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
	{
		if (pEntity->IsPlayer() && pEntity->IsAlive())
		{
			CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pEntity);
			if ((pPlayer->HasSuit() || g_modFeatures.nosuit_allow_healthcharger) && pPlayer->CanHaveItem(this))
			{
				if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) {
					continue;
				}
				TurnNeedleToPlayer(pEntity->pev->origin);
				switch (m_iState) {
				case RetractShot:
					if( m_fSequenceFinished )
						SetNeedleState(Idle);
					break;
				case RetractArm:
					SetNeedleState(Deploy);
					break;
				case Still:
					SetNeedleState(Deploy);
					break;
				case Deploy:
					if (m_fSequenceFinished)
					{
						SetNeedleState(Idle);
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
			SetNeedleState(RetractArm);
			break;
		case RetractArm:
			if (m_fSequenceFinished)
			{
				SetNeedleState(Still);
				SetNeedleController(0);
			}
			else
			{
				SetNeedleController(m_currentYaw*0.75);
			}
			break;
		case Still:
			break;
		default:
			break;
		}
	}
}

void CWallHealthDecay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if( !pCaller )
		return;
	// if it's not a player, ignore
	if( !pCaller->IsPlayer() )
		return;

	CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pCaller);

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || !(pPlayer->HasSuit() || g_modFeatures.nosuit_allow_healthcharger) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EmitSoundScript(CWallHealth::denySoundScript);
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

	int soundType = 0;
	TurnNeedleToPlayer(pPlayer->pev->origin);
	switch (m_iState) {
	case Idle:
		m_flSoundTime = 0.56 + gpGlobals->time;
		SetNeedleState(GiveShot);
		soundType = 1;
		break;
	case GiveShot:
		if (m_fSequenceFinished)
		{
			SetNeedleState(Healing);
		}
		break;
	case Healing:
		if (!m_playingChargeSound && m_flSoundTime <= gpGlobals->time)
		{
			soundType = 2;
			m_playingChargeSound = true;
		}
		break;
	default:
		ALERT(at_console, "Unexpected healthcharger state on use: %d\n", m_iState);
		break;
	}

	// charge the player
	if( pPlayer->TakeHealth( this, 1, HEAL_CHARGE ) )
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
		UpdateJar();

		if (soundType == 1)
		{
			EmitSoundScript(CWallHealth::startSoundScript);
		}
		else if (soundType == 2)
		{
			EmitSoundScript(CWallHealth::loopingSoundScript);
			m_playingChargeSound = true;
		}
	}
	else
	{
		if (m_jar)
		{
			m_jar->ToRest();
		}
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EmitSoundScript(CWallHealth::denySoundScript);
		}
		if (m_playingChargeSound) {
			StopSoundScript(CWallHealth::loopingSoundScript);
			m_playingChargeSound = false;
		}
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;
}

void CWallHealthDecay::Recharge()
{
	EmitSoundScript(CWallHealth::rechargeSoundScript);
	m_iJuice = ChargerCapacity();
	UpdateJar();
	pev->skin = 0;
	SetNeedleState(Still);
	SetThink( &CWallHealthDecay::AnimateAndWork );
	pev->nextthink = gpGlobals->time;
}

void CWallHealthDecay::Off()
{
	switch (m_iState) {
	case GiveShot:
	case Healing:
		if (m_playingChargeSound) {
			StopSoundScript(CWallHealth::loopingSoundScript);
			m_playingChargeSound = false;
		}
		if (m_jar)
		{
			m_jar->ToRest();
		}
		SetNeedleState(RetractShot);
		break;
	case RetractShot:
		if( m_fSequenceFinished )
		{
			if (m_iJuice > 0) {
				SetNeedleState(Idle);
				m_goingToOff = false;
				pev->nextthink = gpGlobals->time;
			} else {
				SetNeedleState(RetractArm);
			}
		}

		break;
	case RetractArm:
	{
		if( m_fSequenceFinished )
		{
			m_currentYaw = m_goalYaw = 0;
			SetBoneController(0, m_currentYaw);
			if( ( m_iJuice <= 0 ) )
			{
				SetNeedleState(Inactive);
				const float rechargeTime = g_pGameRules->FlHealthChargerRechargeTime();
				if (rechargeTime > 0 ) {
					pev->nextthink = gpGlobals->time + rechargeTime;
					SetThink( &CWallHealthDecay::Recharge );
				}
			}
		}
		else
		{
			SetNeedleController(m_currentYaw*0.75);
		}
		break;
	}
	default:
		break;
	}
}

void CWallHealthDecay::SetMySequence(const char *sequence)
{
	pev->sequence = LookupSequence( sequence );
	if (pev->sequence == -1) {
		ALERT(at_error, "unknown sequence in %s: %s\n", STRING(pev->model), sequence);
		pev->sequence = 0;
	}
	pev->frame = 0;
	ResetSequenceInfo();
}

void CWallHealthDecay::SetNeedleState(int state)
{
	m_iState = state;
	switch (state) {
	case Still:
		SetMySequence("still");
		break;
	case Deploy:
		EmitSoundScript(CWallHealth::startSoundScript);
		SetMySequence("deploy");
		break;
	case Idle:
		SetMySequence("prep_shot");
		break;
	case GiveShot:
		SetMySequence("give_shot");
		break;
	case Healing:
		SetMySequence("shot_idle");
		break;
	case RetractShot:
		SetMySequence("retract_shot");
		break;
	case RetractArm:
		SetMySequence("retract_arm");
		break;
	case Inactive:
		SetMySequence("inactive");
	default:
		break;
	}
}

void CWallHealthDecay::TurnNeedleToPlayer(const Vector& player)
{
	float yaw = UTIL_VecToYaw( player - pev->origin ) - pev->angles.y;

	if( yaw > 180 )
		yaw -= 360;
	if( yaw < -180 )
		yaw += 360;

	SetNeedleController( yaw );
}

void CWallHealthDecay::SetNeedleController(float yaw)
{
	m_goalYaw = yaw;
}

void CWallHealthDecay::UpdateOnRemove()
{
	UTIL_Remove(m_jar);
	m_jar = NULL;
	CBaseAnimating::UpdateOnRemove();
}

void CWallHealthDecay::UpdateJar()
{
	if (m_jar)
	{
		m_jar->Update(m_iState == Healing || m_iState == GiveShot, m_iJuice / (float)ChargerCapacity());
	}
}

bool CWallHealthDecay::IsUsefulToDisplayHint(CBaseEntity* pPlayer)
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
