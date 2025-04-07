/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//=========================================================
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"followingmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"combat.h"
#include	"player.h"
#include	"soundent.h"
#include	"mod_features.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

#define bits_MEMORY_ISLAVE_PROVOKED bits_MEMORY_CUSTOM1
#define bits_MEMORY_ISLAVE_REVIVED bits_MEMORY_CUSTOM2
#define bits_MEMORY_ISLAVE_LAST_ATTACK_WAS_COIL bits_MEMORY_CUSTOM3
#define bits_MEMORY_ISLAVE_FAMILIAR_IS_ALIVE bits_MEMORY_CUSTOM4
#define bits_MEMORY_ISLAVE_HAS_LAUNCHED_TOKEN bits_MEMORY_CUSTOM5

// whether vortigaunts can spawn familiars (snarks and headcrabs)
#define FEATURE_ISLAVE_FAMILIAR 1
// whether vortigaunts get health and energy when attack enemies
#define FEATURE_ISLAVE_ENERGY 1
// whether vortigaunt can have glowing hands
#define FEATURE_ISLAVE_HANDGLOW 1
// whether vortigaunt marked as squadleader has a different beam color
#define FEATURE_ISLAVE_LEADER_COLOR 1

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR = LAST_FOLLOWINGMONSTER_SCHEDULE + 1,
	SCHED_ISLAVE_SUMMON_FAMILIAR,
	SCHED_ISLAVE_HEAL_OR_REVIVE,
	SCHED_ISLAVE_GIVE_CHARGE,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_ISLAVE_SUMMON_FAMILIAR = LAST_FOLLOWINGMONSTER_TASK + 1,
	TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK,
	TASK_ISLAVE_MAKE_CHARGE_TOKEN,
	TASK_ISLAVE_SEND_CHARGE_TOKEN,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ISLAVE_AE_CLAW			( 1 )
#define		ISLAVE_AE_CLAWRAKE		( 2 )
#define		ISLAVE_AE_ZAP_POWERUP		( 3 )
#define		ISLAVE_AE_ZAP_SHOOT		( 4 )
#define		ISLAVE_AE_ZAP_DONE		( 5 )

#define		ISLAVE_MAX_BEAMS		8

#define ISLAVE_HAND_SPRITE_NAME "sprites/glow02.spr"

constexpr Color3 VortigauntZapBeamColor = Color3(180, 255, 96);
constexpr Color3 VortigauntZapBeamLeaderColor = Color3(150, 255, 120);

constexpr Color3 VortigauntArmBeamColor = Color3(96, 128, 16);
constexpr Color3 VortigauntArmBeamLeaderColor = Color3(72, 180, 72);

constexpr Color3 VortigauntBeamLightColor = Color3(255, 180, 96);

#define ISLAVE_ELECTROONLY	(1 << 0)
#define ISLAVE_SNARKS		(1 << 1)
#define ISLAVE_HEADCRABS	(1 << 2)

#define ISLAVE_COIL_ATTACK_RADIUS 196

#define ISLAVE_SPAWNFAMILIAR_SPRITE "sprites/bexplo.spr"
#define ISLAVE_SPAWNFAMILIAR_DELAY 6

enum {
	ISLAVE_LEFT_ARM = -1,
	ISLAVE_RIGHT_ARM = 1
};

static bool IsVortWounded(CBaseEntity* pEntity)
{
	return pEntity->pev->health <= Q_min(pEntity->pev->max_health / 2, 20);
}

static bool CanBeRevived(CBaseEntity* pEntity)
{
	if ( pEntity != NULL && pEntity->pev->deadflag == DEAD_DEAD && !FBitSet(pEntity->pev->flags, FL_KILLME) ) {
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if (!pMonster || pMonster->HasMemory(bits_MEMORY_ISLAVE_REVIVED))
		{
			// Wrong target or was already revived once
			return false;
		}

		const Vector vecDest = pEntity->pev->origin + Vector( 0, 0, 38 );
		TraceResult tr;
		UTIL_TraceHull( vecDest, vecDest - Vector(0,0,2), dont_ignore_monsters, human_hull, pEntity->edict(), &tr );
	
		return !tr.fAllSolid && !tr.fStartSolid;
	}
	return false;
}

static bool CanSpawnAtPosition(const Vector& position, int hullType, edict_t* pentIgnore)
{
	TraceResult tr;
	UTIL_TraceHull( position, position - Vector(0,0,1), dont_ignore_monsters, hullType, pentIgnore, &tr );
	return !tr.fStartSolid && !tr.fAllSolid;
}

#define VTOKEN_MAX_SPEED 320
#define VTOKEN_ACCEL_SPEED 160
#define VTOKEN_LIFESPAN 8.0f

class CChargeToken : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	void UpdateOnRemove();
	void EXPORT ArmorPieceTouch(CBaseEntity* pOther);
	void EXPORT AnimateThink();
	void EXPORT HuntThink();
	void EXPORT AnimateAndFade();
	void Animate();
	void MovetoTarget( Vector vecTarget );
	void Launch(CBaseEntity *pTarget, const Vector &pos);
	void MakeEntLight(int timeDs = 2);
	inline void SetAttachment( edict_t *pEntity, int attachment )
	{
		if( pEntity )
		{
			pev->skin = ENTINDEX( pEntity );
			pev->body = attachment;
			pev->aiment = pEntity;
			pev->movetype = MOVETYPE_FOLLOW;
		}
	}

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flLastThink;
	EHANDLE m_hTarget;

	static const NamedSoundScript suitOnSoundScript;

	static const NamedVisual tokenVisual;
	static const NamedVisual tokenLightVisual;
};

LINK_ENTITY_TO_CLASS( charge_token, CChargeToken )

TYPEDESCRIPTION	CChargeToken::m_SaveData[] =
{
	DEFINE_FIELD( CChargeToken, m_flLastThink, FIELD_TIME ),
	DEFINE_FIELD( CChargeToken, m_hTarget, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CChargeToken, CBaseEntity )

const NamedSoundScript CChargeToken::suitOnSoundScript = {
	CHAN_ITEM,
	{"items/suitchargeok1.wav"},
	150,
	"Vortigaunt.SuitOn"
};

const NamedVisual CChargeToken::tokenVisual = BuildVisual::Animated("Vortigaunt.ChargeToken")
		.Model("sprites/xspark1.spr")
		.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 225);

const NamedVisual CChargeToken::tokenLightVisual = BuildVisual("Vortigaunt.ChargeTokenLight")
		.RenderColor(0, 255, 255)
		.Radius(30);

void CChargeToken::Spawn()
{
	pev->classname = MAKE_STRING("charge_token");
	Precache();

	ApplyVisual(GetVisual(tokenVisual));

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	UTIL_SetOrigin( pev, pev->origin );

	pev->health = gSkillData.vortigauntArmorCharge;

	pev->frags = MODEL_FRAMES( pev->modelindex ) - 1;

	pev->nextthink = gpGlobals->time;
	SetThink(&CChargeToken::AnimateThink);
}

void CChargeToken::Precache()
{
	RegisterVisual(tokenVisual);
	RegisterVisual(tokenLightVisual);
	RegisterAndPrecacheSoundScript(suitOnSoundScript);
}

void CChargeToken::UpdateOnRemove()
{
	if (!FNullEnt(pev->owner))
	{
		CBaseMonster* pOwner = GetMonsterPointer(pev->owner);
		if (pOwner)
		{
			pOwner->Forget(bits_MEMORY_ISLAVE_HAS_LAUNCHED_TOKEN);
		}
	}
	CBaseEntity::UpdateOnRemove();
}

void CChargeToken::ArmorPieceTouch(CBaseEntity *pOther)
{
	CBasePlayer* pPlayer = pOther->IsPlayer() ? static_cast<CBasePlayer*>(pOther) : nullptr;
	if (pPlayer && pPlayer->HasSuit())
	{
		pPlayer->TakeArmor(this, pev->health);
		EmitSoundScript(suitOnSoundScript);
		SetTouch(NULL);
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
	else if (pOther == m_hTarget)
	{
		SetTouch(NULL);
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
}

void CChargeToken::AnimateThink()
{
	Animate();
	pev->nextthink = gpGlobals->time + 0.1;
}

void CChargeToken::Animate()
{
	pev->frame = AnimateWithFramerate(pev->frame, pev->frags, pev->framerate);
}

void CChargeToken::HuntThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	Animate();

	//MakeEntLight();

	if( gpGlobals->time - pev->dmgtime > VTOKEN_LIFESPAN || m_hTarget == 0 || !IsInWorld())
	{
		SetTouch( NULL );
		SetThink( &CChargeToken::AnimateAndFade );
		pev->velocity = g_vecZero;
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
		return;
	}

	MovetoTarget( m_hTarget->Center() );
	m_flLastThink = gpGlobals->time;
}

void CChargeToken::AnimateAndFade()
{
	Animate();
	if( pev->renderamt > 25 )
	{
		pev->renderamt -= 25;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink( &CBaseEntity::SUB_Remove );
	}
}

void CChargeToken::MovetoTarget(Vector vecTarget)
{
	// accelerate
	float flSpeed = pev->velocity.Length();
	float flDelta = gpGlobals->time - m_flLastThink;

	if ( flSpeed < VTOKEN_MAX_SPEED )
	{
		flSpeed += ( VTOKEN_ACCEL_SPEED * flDelta );
		if ( flSpeed > VTOKEN_MAX_SPEED )
		{
			flSpeed = VTOKEN_MAX_SPEED;
		}
	}

	Vector vecDir = ( vecTarget - pev->origin ).Normalize();
	pev->velocity = vecDir * flSpeed;
}

void CChargeToken::Launch(CBaseEntity* pTarget, const Vector& pos)
{
	pev->skin = 0;
	pev->body = 0;
	pev->aiment = 0;
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector( -8, -8, 8 ), Vector( 8, 8, 8 ) );
	UTIL_SetOrigin( pev, pos );
	m_hTarget = pTarget;

	SetTouch(&CChargeToken::ArmorPieceTouch);
	SetThink(&CChargeToken::HuntThink);

	pev->nextthink = gpGlobals->time + 0.1;
	m_flLastThink = gpGlobals->time;
	pev->dmgtime = gpGlobals->time;
}

void CChargeToken::MakeEntLight(int timeDs)
{
	const Visual* pVisual = GetVisual(tokenLightVisual);
	if (pVisual)
	{
		Visual visual = *pVisual;
		visual.life = timeDs * 0.1f;
		visual.radius = RandomizeNumberFromRange(visual.radius) * pev->renderamt / 225;
		SendEntLight(entindex(), pev->origin, &visual);
	}
}

class CISlave : public CFollowingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue(KeyValueData* pkvd);
	void UpdateOnRemove();
	void SetYawSpeed( void );
	int DefaultISoundMask( void );
	int DefaultClassify( void );
	const char* DefaultDisplayName() { return "Alien Slave"; }
	const char* ReverseRelationshipModel() { return "models/islavef.mdl"; }
	int IRelationship( CBaseEntity *pTarget );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	bool CheckRangeAttack2( float flDot, float flDist ) override;
	bool CheckHealOrReviveTargets( float flDist = 784, bool mustSee = false );
	bool IsValidHealTarget( CBaseEntity* pEntity );
	void CallForHelp( float flDist, EHANDLE hEnemy, Vector &vecLocation );
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo ) override;

	void DeathSound( void );
	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void PlayUseSentence();
	void PlayUnUseSentence();
	bool EmitSoundScriptTalk(const char* name);

	void OnDying();
	void DeathNotice( entvars_t* pevChild )
	{
		Forget(bits_MEMORY_ISLAVE_FAMILIAR_IS_ALIVE);
	}

	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void PrescheduleThink();
	int LookupActivity(int activity);
	virtual int DefaultSizeForGrapple() { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() { return VEC_HUMAN_HULL_MAX; }

	void SpawnFamiliar(const char *entityName, const Vector& origin, int hullType);
	void OnChangeSchedule( Schedule_t* pNewSchedule );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	CUSTOM_SCHEDULES

	int Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ReportAIState( ALERT_TYPE level );

	void ClearBeams();
	void ArmBeam(int side );
	void ArmBeamMessage(int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	CBaseEntity* ZapBeam( int side );
	void BeamGlow( void );
	void HandsGlowOn(int brightness = 224);
	void HandGlowOn(CSprite* handGlow, int brightness = 224);
	void StartMeleeAttackGlow(int side);
	bool CanUseGlowArms();
	void HandGlowOff(CSprite* handGlow);
	void HandsGlowOff();
	void CreateSummonBeams();
	void RemoveSummonBeams();
	void RemoveHandGlows();
	void RemoveChargeToken();
	void CoilBeam();

	Vector HandPosition(int side);

	CSprite* CreateHandGlow(int attachment);
	CBeam* CreateSummonBeam(const Vector& vecEnd, int t);

	float HealPower();
	void SpendEnergy(float energy);
	bool HasFreeEnergy();
	bool AbleToHeal() {
		return g_modFeatures.vortigaunt_heal;
	}
	bool AbleToRevive() {
		return g_modFeatures.vortigaunt_revive && gSkillData.slaveRevival > 0;
	}
	bool CanRevive();
	int HealOther(CBaseEntity* pEntity);
	bool CanSpawnFamiliar();

	inline int AttachmentFromSide(int side) {
		return side < 0 ? 2 : 1;
	}

	void SetHealTargetAsTargetEnt()
	{
		if (m_hDead) {
			m_hTargetEnt = m_hDead;
		} else if (m_hWounded) {
			m_hTargetEnt = m_hWounded;
		}
	}

	bool CanGoToTargetEnt()
	{
		if (m_hTargetEnt)
			return BuildRoute(m_hTargetEnt->pev->origin, bits_MF_TO_TARGETENT, m_hTargetEnt);
		return false;
	}

	const char* FamiliarName()
	{
		if (pev->weapons & ISLAVE_SNARKS) {
			return "monster_snark";
		} else if (pev->weapons & ISLAVE_HEADCRABS) {
			return "monster_headcrab";
		}
		return NULL;
	}

	int FamiliarHull()
	{
		return head_hull;
	}

	Vector GetFamiliarSpawnPosition()
	{
		UTIL_MakeVectors( pev->angles );
		if (pev->weapons & ISLAVE_SNARKS) {
			return pev->origin + gpGlobals->v_forward * 36 + Vector(0,0,20);
		} else if (pev->weapons & ISLAVE_HEADCRABS) {
			return pev->origin + gpGlobals->v_forward * 48 + Vector(0,0,20);
		}
		return pev->origin + gpGlobals->v_forward * 36 + Vector(0,0,20);
	}

	int m_iBravery;

	CBeam *m_pBeam[ISLAVE_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	int m_voicePitch;

	float m_freeEnergy;

	EHANDLE m_hDead;
	EHANDLE m_hWounded;
	EHANDLE m_hWounded2;
	float m_nextHealTargetCheck;
	float m_originalMaxHealth;

	short m_clawStrikeNum;

	float m_flSpawnFamiliarTime;

	CSprite	*m_handGlow1;
	CSprite	*m_handGlow2;
	CBeam *m_handsBeam1;
	CBeam *m_handsBeam2;

	CChargeToken* m_chargeToken;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static constexpr const char* attackHitSoundScript = "Vortigaunt.AttackHit";
	static constexpr const char* attackMissSoundScript = "Vortigaunt.AttackMiss";
	static const NamedSoundScript zapPowerupSoundScript;
	static const NamedSoundScript zapShootSoundScript;
	static const NamedSoundScript electroSoundScript;
	static const NamedSoundScript glowAlarmSoundScript;
	static const NamedSoundScript idleZapSoundScript;
	static const NamedSoundScript summonStartSoundScript;
	static const NamedSoundScript summonEndSoundScript;

	static const NamedVisual zapBeamColorVisual;
	static const NamedVisual armBeamColorVisual;
	static const NamedVisual beamLightColorVisual;

	static const NamedVisual zapBeamVisual;
	static const NamedVisual powerupBeamVisual;
	static const NamedVisual revivalBeamVisual;
	static const NamedVisual summonBeamVisual;
	static const NamedVisual idleBeamVisual;
	static const NamedVisual coilBeamVisual;
	static const NamedVisual trailBeamVisual;

	static const NamedVisual summonSpriteVisual;
	static const NamedVisual handGlowVisual;

	static const NamedVisual powerupLightVisual;
	static const NamedVisual idleLightVisual;
	static const NamedVisual summonLightVisual;
};

LINK_ENTITY_TO_CLASS( monster_alien_slave, CISlave )
LINK_ENTITY_TO_CLASS( monster_vortigaunt, CISlave )

TYPEDESCRIPTION	CISlave::m_SaveData[] =
{
	DEFINE_FIELD( CISlave, m_iBravery, FIELD_INTEGER ),

	DEFINE_ARRAY( CISlave, m_pBeam, FIELD_CLASSPTR, ISLAVE_MAX_BEAMS ),
	DEFINE_FIELD( CISlave, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CISlave, m_flNextAttack, FIELD_TIME ),

	DEFINE_FIELD( CISlave, m_voicePitch, FIELD_INTEGER ),
#if FEATURE_ISLAVE_ENERGY
	DEFINE_FIELD( CISlave, m_freeEnergy, FIELD_FLOAT ),
	DEFINE_FIELD( CISlave, m_hDead, FIELD_EHANDLE ),
	DEFINE_FIELD( CISlave, m_hWounded, FIELD_EHANDLE ),
	DEFINE_FIELD( CISlave, m_hWounded2, FIELD_EHANDLE ),
	DEFINE_FIELD( CISlave, m_nextHealTargetCheck, FIELD_TIME ),
	DEFINE_FIELD( CISlave, m_originalMaxHealth, FIELD_FLOAT ),
#endif
	DEFINE_FIELD( CISlave, m_clawStrikeNum, FIELD_SHORT ),
#if FEATURE_ISLAVE_FAMILIAR
	DEFINE_FIELD( CISlave, m_flSpawnFamiliarTime, FIELD_FLOAT ),
#endif
#if FEATURE_ISLAVE_HANDGLOW
	DEFINE_FIELD( CISlave, m_handGlow1, FIELD_CLASSPTR ),
	DEFINE_FIELD( CISlave, m_handGlow2, FIELD_CLASSPTR ),
#endif
	DEFINE_FIELD( CISlave, m_minHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CISlave, m_maxHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CISlave, m_chargeToken, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CISlave, CFollowingMonster )

const NamedSoundScript CISlave::painSoundScript = {
	CHAN_WEAPON,
	{"aslave/slv_pain1.wav", "aslave/slv_pain2.wav"},
	"Vortigaunt.Pain"
};

const NamedSoundScript CISlave::dieSoundScript = {
	CHAN_WEAPON,
	{"aslave/slv_die1.wav", "aslave/slv_die2.wav"},
	"Vortigaunt.Die"
};

const NamedSoundScript CISlave::zapPowerupSoundScript = {
	CHAN_WEAPON,
	{"debris/zap4.wav"},
	"Vortigaunt.ZapPowerup"
};

const NamedSoundScript CISlave::zapShootSoundScript = {
	CHAN_WEAPON,
	{"hassault/hw_shoot1.wav"},
	IntRange(130, 160),
	"Vortigaunt.ZapShoot"
};

const NamedSoundScript CISlave::electroSoundScript = {
	CHAN_STATIC,
	{"weapons/electro4.wav"},
	0.5f,
	ATTN_NORM,
	IntRange(140, 160),
	"Vortigaunt.Electro"
};

const NamedSoundScript CISlave::glowAlarmSoundScript = {
	CHAN_BODY,
	{"debris/zap3.wav", "debris/zap8.wav"},
	0.7f,
	ATTN_NORM,
	IntRange(70, 90),
	"Vortigaunt.GlowArm"
};

const NamedSoundScript CISlave::idleZapSoundScript = {
	CHAN_WEAPON,
	{"debris/zap1.wav"},
	"Vortigaunt.IdleZap"
};

const NamedSoundScript CISlave::summonStartSoundScript = {
	CHAN_BODY,
	{"debris/beamstart1.wav"},
	"Vortigaunt.SummonStart"
};

const NamedSoundScript CISlave::summonEndSoundScript = {
	CHAN_BODY,
	{"debris/beamstart7.wav"},
	0.9f,
	ATTN_NORM,
	"Vortigaunt.SummonEnd",
};

const NamedVisual CISlave::zapBeamColorVisual = BuildVisual("Vortigaunt.ZapBeamColor")
		.RenderColor(VortigauntZapBeamColor);

const NamedVisual CISlave::armBeamColorVisual = BuildVisual("Vortigaunt.ArmBeamColor")
		.RenderColor(VortigauntArmBeamColor);

const NamedVisual CISlave::beamLightColorVisual = BuildVisual("Vortigaunt.BeamLightColor")
		.RenderColor(VortigauntBeamLightColor);

const NamedVisual CISlave::zapBeamVisual = BuildVisual("Vortigaunt.ZapBeam")
		.Model("sprites/lgtning.spr")
		.Alpha(255)
		.BeamParams(50, 20)
		.Mixin(&CISlave::zapBeamColorVisual);

const NamedVisual CISlave::powerupBeamVisual = BuildVisual("Vortigaunt.PowerupBeam")
		.Model("sprites/lgtning.spr")
		.Alpha(64)
		.BeamParams(30, 80)
		.Mixin(&CISlave::armBeamColorVisual);

const NamedVisual CISlave::revivalBeamVisual = BuildVisual("Vortigaunt.RevivalBeam")
		.Model("sprites/lgtning.spr")
		.Alpha(255)
		.BeamParams(30, 80)
		.Mixin(&CISlave::zapBeamColorVisual);

const NamedVisual CISlave::summonBeamVisual = BuildVisual("Vortigaunt.SummonBeam")
		.Model("sprites/lgtning.spr")
		.Alpha(192)
		.BeamParams(30, 80)
		.Mixin(&CISlave::zapBeamColorVisual);

const NamedVisual CISlave::idleBeamVisual = BuildVisual("Vortigaunt.IdleBeam")
		.Model("sprites/lgtning.spr")
		.Alpha(64)
		.Framerate(10.0f)
		.BeamParams(30, 80, 10)
		.Life(FloatRange(0.8f, 1.5f))
		.Mixin(&CISlave::armBeamColorVisual);

const NamedVisual CISlave::coilBeamVisual = BuildVisual("Vortigaunt.CoilBeam")
		.Model("sprites/lgtning.spr")
		.Alpha(255)
		.Framerate(10.0f)
		.BeamParams(128, 20)
		.Life(0.2f)
		.Mixin(&CISlave::zapBeamColorVisual);

const NamedVisual CISlave::trailBeamVisual = BuildVisual("Vortigaunt.MeleeTrailBeam")
		.Model("sprites/plasma.spr")
		.Alpha(128)
		.BeamParams(3, 0)
		.Life(0.5f)
		.Mixin(&CISlave::armBeamColorVisual);

const NamedVisual CISlave::summonSpriteVisual = BuildVisual("Vortigaunt.SummonSprite")
		.Model("sprites/bexplo.spr")
		.RenderProps(kRenderTransAdd, VortigauntArmBeamColor, 255, kRenderFxNoDissipation)
		.Framerate(20.0f);

const NamedVisual CISlave::handGlowVisual = BuildVisual("Vortigaunt.HandGlow")
		.Model("sprites/glow02.spr")
		.RenderMode(kRenderTransAdd)
		.Alpha(224)
		.RenderFx(kRenderFxNoDissipation)
		.Scale(0.25f)
		.Mixin(&CISlave::zapBeamColorVisual);

const NamedVisual CISlave::powerupLightVisual = BuildVisual("Vortigaunt.PowerupLight")
		.Radius(120)
		.Mixin(&CISlave::beamLightColorVisual);

const NamedVisual CISlave::idleLightVisual = BuildVisual("Vortigaunt.IdleLight")
		.Radius(80)
		.Life(1.0f)
		.Mixin(&CISlave::beamLightColorVisual);

const NamedVisual CISlave::summonLightVisual = BuildVisual("Vortigaunt.SummonLight")
		.Radius(100)
		.Life(1.5f)
		.Mixin(&CISlave::beamLightColorVisual);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CISlave::DefaultClassify( void )
{
	return CLASS_ALIEN_MILITARY;
}

int CISlave::IRelationship( CBaseEntity *pTarget )
{
	if( ( pTarget && pTarget->IsPlayer() ) )
		if( ( pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ! ( m_afMemory & bits_MEMORY_ISLAVE_PROVOKED ) )
			return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}

void CISlave::CallForHelp(float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	// ALERT( at_aiconsole, "help " );

	// skip ones not on my netname
	if( FStringNull( pev->netname ) )
		return;

	CBaseEntity *pEntity = NULL;

	while( ( pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ) ) ) != NULL)
	{
		float d = ( pev->origin - pEntity->pev->origin ).Length();
		if( d < flDist )
		{
			if (!FClassnameIs(pEntity->pev, STRING(pev->classname)))
				continue;
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if( pMonster )
			{
				pMonster->m_afMemory |= bits_MEMORY_ISLAVE_PROVOKED;
				pMonster->PushEnemy( hEnemy, vecLocation );
			}
		}
	}
}

//=========================================================
// ALertSound - scream
//=========================================================
void CISlave::AlertSound( void )
{
	if( m_hEnemy != 0 )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), "SLV_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch );

		CallForHelp( 512, m_hEnemy, m_vecEnemyLKP );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CISlave::IdleSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), "SLV_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch );
	}

	if (g_modFeatures.vortigaunt_idle_effects)
	{
		int side = RANDOM_LONG( 0, 1 ) * 2 - 1;

		ArmBeamMessage( side );

		UTIL_MakeAimVectors( pev->angles );
		const Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
		SendDynLight(vecSrc, GetVisual(idleLightVisual));

		EmitSoundScript(idleZapSoundScript);
	}
}

//=========================================================
// PainSound
//=========================================================
void CISlave::PainSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		EmitSoundScriptTalk(painSoundScript);
	}
}

//=========================================================
// DieSound
//=========================================================
void CISlave::DeathSound( void )
{
	EmitSoundScriptTalk(dieSoundScript);
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CISlave::DefaultISoundMask( void )
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

void CISlave::OnDying()
{
	ClearBeams();
	RemoveHandGlows();
	RemoveChargeToken();
	CFollowingMonster::OnDying();
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CISlave::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_WALK:		
		ys = 50;	
		break;
	case ACT_RUN:		
		ys = 70;
		break;
	case ACT_IDLE:		
		ys = 50;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CISlave::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case ISLAVE_AE_CLAW:
		{
			m_clawStrikeNum++;
			int damageType = DMG_SLASH;
			float damage = gSkillData.slaveDmgClaw;
			if (CanUseGlowArms()) {
				if ( m_clawStrikeNum == 1 ) {
					HandGlowOff(m_handGlow1);
					StartMeleeAttackGlow(ISLAVE_LEFT_ARM);
				}
				if ( m_clawStrikeNum == 2 ) {
					HandGlowOff(m_handGlow2);
					StartMeleeAttackGlow(ISLAVE_RIGHT_ARM);
				}
				if ( m_clawStrikeNum == 3 ) {
					HandGlowOff(m_handGlow1);
				}
				damageType |= DMG_SHOCK;
				damage *= 1.5;
			}
			// SOUND HERE!

			TraceHullAttackParams params;
			params.punchAngle.z = 18;
			params.punchAngle.x = 5;
			params.damageInfo.damage = damage;
			params.damageInfo.type = damageType;
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformTraceHullAttack(params);
		}
			break;
		case ISLAVE_AE_CLAWRAKE:
		{
			TraceHullAttackParams params;
			params.punchAngle.z = -18;
			params.punchAngle.x = 5;
			params.damageInfo.damage = gSkillData.slaveDmgClawrake;
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformTraceHullAttack(params);
		}
			break;
		case ISLAVE_AE_ZAP_POWERUP:
		{
			// Hack to prevent the event from playing again when the animation ends
			if (m_iTaskStatus == TASKSTATUS_COMPLETE)
				break;
			// speed up attack depending on difficulty level
			pev->framerate = gSkillData.slaveZapRate;

			UTIL_MakeAimVectors( pev->angles );

			if( m_iBeams == 0 )
			{
				const Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				const Visual* pPowerupLightVisual = GetVisual(powerupLightVisual);
				if (pPowerupLightVisual)
				{
					Visual powerupVis = *pPowerupLightVisual;
					powerupVis.life = (2.0f/pev->framerate);
					SendDynLight(vecSrc, &powerupVis);
				}
			}
			if( CanRevive() )
			{
				WackBeam( ISLAVE_LEFT_ARM, m_hDead );
				WackBeam( ISLAVE_RIGHT_ARM, m_hDead );
			}
			else
			{
				ArmBeam( ISLAVE_LEFT_ARM );
				ArmBeam( ISLAVE_RIGHT_ARM );
				BeamGlow();
			}
			SoundScriptParamOverride params;
			params.OverridePitchShifted(m_iBeams * 10);
			EmitSoundScript(zapPowerupSoundScript, params);
		}
			break;
		case ISLAVE_AE_ZAP_SHOOT:
		{
			ClearBeams();

			if( CanRevive() )
			{
				if( CanBeRevived(m_hDead) )
				{
					Forget(bits_MEMORY_ISLAVE_LAST_ATTACK_WAS_COIL);

					CBaseEntity *revived = m_hDead;
					if (revived) {
						CBaseMonster* monster = revived->MyMonsterPointer();
						if (monster)
						{
							CISlave* revivedVort = (CISlave*)monster;

							revivedVort->pev->health = revivedVort->m_originalMaxHealth;
							// TODO: should restore the actual values that the vort had before he died
							revivedVort->pev->rendermode = kRenderNormal;
							revivedVort->pev->renderamt = 255;
							if (revivedVort->ShouldFadeOnDeath())
							{
								// Force fading upon death if monster should have faded originally
								revivedVort->pev->spawnflags |= SF_MONSTER_FADECORPSE;
							}
							revivedVort->pev->owner = NULL; // nullify owner to avoid additional DeathNotice calls
							revivedVort->Spawn();
							revivedVort->Remember(bits_MEMORY_ISLAVE_REVIVED);

							if (m_hEnemy)
							{
								revivedVort->PushEnemy(m_hEnemy, m_vecEnemyLKP);
							}

							// revived vort starts with zero energy
							revivedVort->m_freeEnergy = 0;
						}

						WackBeam( ISLAVE_LEFT_ARM, revived );
						WackBeam( ISLAVE_RIGHT_ARM, revived );
						m_hDead = NULL;
						EmitSoundScript(zapShootSoundScript);

						SpendEnergy(pev->max_health);
					}

					/*
					CBaseEntity *pEffect = Create( "test_effect", pNew->Center(), pev->angles );
					pEffect->Use( this, this, USE_ON, 1 );
					*/
					break;
				}
				else {
					ALERT(at_aiconsole, "Trace failed on revive\n");
				}
			}
			ClearMultiDamage();

			bool coilAttack = false;
			if (g_modFeatures.vortigaunt_coil_attack)
			{
				if (m_Activity == ACT_SPECIAL_ATTACK1 && m_pCine)
				{
					coilAttack = true;
					ALERT(at_aiconsole, "Vort makes coil attack due to the script\n");
				}
				// make coil attack on purpose to heal only if two wounded friends around
				if ( HasFreeEnergy() && IsValidHealTarget(m_hWounded) && IsValidHealTarget(m_hWounded2) &&
						(pev->origin - m_hWounded->pev->origin).Length() <= ISLAVE_COIL_ATTACK_RADIUS &&
						(pev->origin - m_hWounded2->pev->origin).Length() <= ISLAVE_COIL_ATTACK_RADIUS) {
					if (m_hWounded.Get() == m_hWounded2.Get()) {
						ALERT(at_console, "m_hWounded && m_hWounded2 are the same!\n");
					}
					coilAttack = true;
					ALERT(at_aiconsole, "Vort makes coil attack to heal friends\n");
				} else if ( m_hEnemy != 0 && (pev->origin - m_hEnemy->pev->origin).Length() <= ISLAVE_COIL_ATTACK_RADIUS && !HasMemory(bits_MEMORY_ISLAVE_LAST_ATTACK_WAS_COIL) ) {
					coilAttack = true;
				}
			}

			if (coilAttack) {
				CoilBeam();
				Remember(bits_MEMORY_ISLAVE_LAST_ATTACK_WAS_COIL);

				UTIL_ScreenShake( pev->origin, 3.0, 40.0, 1.0, ISLAVE_COIL_ATTACK_RADIUS );

				::RadiusDamage(this, pev->origin, pev, pev, DamageInfo{gSkillData.slaveDmgZap*2.5f, DMG_SHOCK},
							   ISLAVE_COIL_ATTACK_RADIUS,
							   RADIUSDAMAGE_SPOT_IS_TARGET_CENTER,
							   [this](CBaseEntity* pEntity) {
					const int rel = IRelationship(pEntity);
					if (rel == R_AL)
					{
						if (FClassnameIs(pEntity->pev, STRING(pev->classname))) {
							if (AbleToHeal() && HealOther(pEntity)) {
								ALERT(at_aiconsole, "Vort healed friend with coil attack\n");
							}
						}
						return false;
					}
					return true;
				});
				EmitSoundScriptAmbient(pev->origin, electroSoundScript);

				m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 1.0, 4.0 );
			} else {
				Forget(bits_MEMORY_ISLAVE_LAST_ATTACK_WAS_COIL);
				UTIL_MakeAimVectors( pev->angles );

				ZapBeam( ISLAVE_LEFT_ARM );
				ZapBeam( ISLAVE_RIGHT_ARM );
				
				EmitSoundScript(zapShootSoundScript);
				// STOP_SOUND( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav" );
				ApplyMultiDamage( pev, pev );

				m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
			}
		}
			break;
		case ISLAVE_AE_ZAP_DONE:
		{
			ClearBeams();
		}
			break;
		default:
			CFollowingMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
bool CISlave::CheckRangeAttack1( float flDot, float flDist )
{
	if( m_flNextAttack > gpGlobals->time )
	{
		return false;
	}

	if( g_modFeatures.vortigaunt_coil_attack && flDist > 64 && flDist <= ISLAVE_COIL_ATTACK_RADIUS )
	{
		return true;
	}

	return CFollowingMonster::CheckRangeAttack1( flDot, flDist );
}

//=========================================================
// CheckRangeAttack2 - try to resurect dead comrades or heal wounded ones
//=========================================================
bool CISlave::CheckRangeAttack2( float flDot, float flDist )
{
	if( m_flNextAttack > gpGlobals->time )
	{
		return false;
	}

	return HasFreeEnergy() && CheckHealOrReviveTargets(flDist, true);
}

bool CISlave::CheckHealOrReviveTargets(float flDist, bool mustSee)
{
	if (m_nextHealTargetCheck >= gpGlobals->time)
	{
		return (m_hDead != 0 && gSkillData.slaveRevival > 0) || m_hWounded != 0;
	}

	m_nextHealTargetCheck = gpGlobals->time + 1;
	m_hDead = NULL;
	m_hWounded = NULL;
	m_hWounded2 = NULL;

	CBaseEntity *pEntity = NULL;
	while( ( pEntity = UTIL_FindEntityByClassname( pEntity, "monster_alien_slave" ) ) != NULL )
	{
		if (IRelationship(pEntity) >= R_DL)
			continue;
		TraceResult tr;

		UTIL_TraceLine( EyePosition(), pEntity->EyePosition(), ignore_monsters, ENT( pev ), &tr );
		if( (mustSee && (tr.flFraction == 1.0f || tr.pHit == pEntity->edict())) || (!mustSee && (pEntity->pev->origin -pev->origin).Length() < flDist ) )
		{
			if( AbleToRevive() && CanBeRevived(pEntity) )
			{
				float d = ( pev->origin - pEntity->pev->origin ).Length();
				if( d < flDist )
				{
					m_hDead = pEntity;
					flDist = d;
				}
			}
			if ( AbleToHeal() && IsValidHealTarget(pEntity) )
			{
				float d = ( pev->origin - pEntity->pev->origin ).Length();
				if( d < flDist )
				{
					m_hWounded2 = m_hWounded;
					m_hWounded = pEntity;
					flDist = d;
				}
			}
		}
	}
	return m_hDead != 0 || m_hWounded != 0;
}

bool CISlave::IsValidHealTarget(CBaseEntity *pEntity)
{
	return pEntity != NULL && pEntity != this && pEntity->IsFullyAlive() && IsVortWounded(pEntity);
}

//=========================================================
// StartTask
//=========================================================
void CISlave::StartTask( Task_t *pTask )
{
	ClearBeams();

	switch(pTask->iTask)
	{
	case TASK_ISLAVE_SUMMON_FAMILIAR:
	{
		if (CanSpawnAtPosition(GetFamiliarSpawnPosition(), FamiliarHull(), edict()))
		{
			m_IdealActivity = ACT_CROUCH;
			EmitSoundScript(summonStartSoundScript);
			UTIL_MakeAimVectors( pev->angles );
			const Vector vecSrc = pev->origin + gpGlobals->v_forward * 8;
			SendDynLight(vecSrc, GetVisual(summonLightVisual));
			HandsGlowOn();
			CreateSummonBeams();
		}
		else
		{
			m_flSpawnFamiliarTime = gpGlobals->time + ISLAVE_SPAWNFAMILIAR_DELAY; // still set delay to avoid constant trying
			TaskFail("no space to spawn a familiar");
		}
		break;
	}
		
	case TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK:
	{
		ALERT(at_aiconsole, "start TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK\n");
		m_IdealActivity = ACT_RANGE_ATTACK1;
		break;
	}

	case TASK_ISLAVE_MAKE_CHARGE_TOKEN:
	{
		if (g_modFeatures.vortigaunt_armor_charge)
		{
			m_chargeToken = GetClassPtr( (CChargeToken *)NULL );
			if (m_chargeToken)
			{
				m_chargeToken->AssignEntityOverrides(GetProjectileOverrides());
				m_chargeToken->pev->owner = edict();
				m_chargeToken->Spawn();
				Remember(bits_MEMORY_ISLAVE_HAS_LAUNCHED_TOKEN);
				m_chargeToken->SetAttachment(edict(), 1);
				UTIL_SetOrigin(m_chargeToken->pev, pev->origin);
				m_chargeToken->MakeEntLight(20);
				TaskComplete();
			}
			else
				TaskFail("failed to create charge token entity");
		}
		else
			TaskFail("charge tokens are disabled");
		break;
	}
	case TASK_ISLAVE_SEND_CHARGE_TOKEN:
	{
		if (g_modFeatures.vortigaunt_armor_charge && m_chargeToken)
		{
			CBaseEntity* pTarget = FollowedPlayer();
			if (pTarget)
			{
				Vector vecPos, vecAng;
				GetAttachment(0, vecPos, vecAng);
				SpendEnergy(m_chargeToken->pev->health);
				m_chargeToken->Launch(pTarget, vecPos);
				m_chargeToken = NULL;
				TaskComplete();
			}
			else
			{
				TaskFail("no target player to send charge token to");
			}
		}
		else
		{
			TaskFail("no charge token to send");
		}
		break;
	}
	case TASK_WAIT_FOR_MOVEMENT:
		// a hack to prevent vortigaunts running with beams caused by dangling events from the attack animation
		m_IdealActivity = ACT_IDLE;
		// fallthrough
	default:
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

void CISlave::RunTask(Task_t *pTask)
{
	switch(pTask->iTask)
	{
	case TASK_ISLAVE_SUMMON_FAMILIAR:
		if( m_fSequenceFinished )
		{
			SpawnFamiliar(FamiliarName(), GetFamiliarSpawnPosition(), FamiliarHull());
			HandsGlowOff();
			TaskComplete();
			RemoveSummonBeams();
		}
		break;
	case TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK:
		if( m_fSequenceFinished )
		{
			m_Activity = ACT_RESET;
			TaskComplete();
		}
		break;
	default:
		CFollowingMonster::RunTask( pTask );
		break;
	}
}

void CISlave::PrescheduleThink()
{
	CFollowingMonster::PrescheduleThink();
	if (m_Activity == ACT_MELEE_ATTACK1 && m_clawStrikeNum == 0) {
		if ( m_handGlow1 && (m_handGlow1->pev->effects & EF_NODRAW) && CanUseGlowArms() ) {
			StartMeleeAttackGlow(ISLAVE_RIGHT_ARM);
			EmitSoundScript(glowAlarmSoundScript);
		}
	}
}

int CISlave::LookupActivity(int activity)
{
	if (activity == ACT_ARM && IDefaultRelationship(CLASS_PLAYER) == R_AL)
	{
		return LookupSequence("updown");
	}
	else if (activity == ACT_SPECIAL_ATTACK1 && m_pCine)
	{
		return LookupSequence("zapattack1");
	}
	return CFollowingMonster::LookupActivity(activity);
}

void CISlave::SpawnFamiliar(const char *entityName, const Vector &origin, int hullType)
{
	if (!entityName) {
		ALERT(at_console, "Null familiar name in SpawnFamiliar!\n");
		return;
	}
	if (CanSpawnAtPosition(origin, hullType, edict())) {
		CBaseEntity *pNew = Create( entityName, origin, pev->angles, edict() );

		if(pNew) {
			CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );

			Remember(bits_MEMORY_ISLAVE_FAMILIAR_IS_ALIVE);
			CreateSpriteFromVisual(GetVisual(summonSpriteVisual), origin, SF_SPRITE_ONCE_AND_REMOVE);
			EmitSoundScript(summonEndSoundScript);

			SetBits( pNew->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
			if (pNewMonster) {
				pNewMonster->m_iClass = m_iClass;
				pNewMonster->m_reverseRelationship = m_reverseRelationship;
				if (m_hEnemy) {
					pNewMonster->PushEnemy(m_hEnemy, m_vecEnemyLKP);
				}
			}
		}
	} else {
		ALERT(at_aiconsole, "Not enough room to create %s\n", entityName);
	}
	m_flSpawnFamiliarTime = gpGlobals->time + ISLAVE_SPAWNFAMILIAR_DELAY;
}

CSprite* CISlave::CreateHandGlow(int attachment)
{
	CSprite* handSprite = CreateSpriteFromVisual(GetVisual(handGlowVisual), pev->origin);
	if (handSprite)
		handSprite->SetAttachment( edict(), attachment );
	return handSprite;
}

//=========================================================
// Spawn
//=========================================================
void CISlave::Spawn()
{
	Precache();

	SetMyModel( "models/islave.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects		= 0;
	SetMyHealth( gSkillData.slaveHealth );
	pev->view_ofs		= Vector( 0, 0, 64 );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE); // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2;
	SetMyCanOpenDoors(true);

	m_voicePitch		= RANDOM_LONG( 85, 110 );

#if FEATURE_ISLAVE_HANDGLOW
	m_handGlow1 = CreateHandGlow(1);
	m_handGlow2 = CreateHandGlow(2);
	HandsGlowOff();
#endif

	FollowingMonsterInit();

	m_originalMaxHealth = pev->max_health;

#if FEATURE_ISLAVE_ENERGY
	// leader starts with some energy pool
	if (!m_freeEnergy && (pev->spawnflags & SF_SQUADMONSTER_LEADER))
		m_freeEnergy = pev->max_health;
#endif
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CISlave::Precache()
{
	RegisterVisual(zapBeamVisual);
	RegisterVisual(powerupBeamVisual);
	RegisterVisual(revivalBeamVisual);
	RegisterVisual(summonBeamVisual);

	RegisterVisual(idleBeamVisual);
	RegisterVisual(coilBeamVisual);
	RegisterVisual(trailBeamVisual);

	RegisterVisual(powerupLightVisual);
	RegisterVisual(idleLightVisual);
	RegisterVisual(summonLightVisual);

	PrecacheMyModel( "models/islave.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
	RegisterAndPrecacheSoundScript(zapPowerupSoundScript);
	RegisterAndPrecacheSoundScript(zapShootSoundScript);
	RegisterAndPrecacheSoundScript(electroSoundScript);
	RegisterAndPrecacheSoundScript(idleZapSoundScript);
	RegisterAndPrecacheSoundScript(summonStartSoundScript);
	RegisterAndPrecacheSoundScript(summonEndSoundScript);

	UTIL_PrecacheOther( "test_effect" );

#if FEATURE_ISLAVE_HANDGLOW
	RegisterVisual(handGlowVisual);
	if (g_modFeatures.vortigaunt_arm_boost)
		RegisterAndPrecacheSoundScript(glowAlarmSoundScript);
#endif
#if FEATURE_ISLAVE_FAMILIAR
	RegisterVisual(summonSpriteVisual);
	UTIL_PrecacheOther( "monster_snark" );
	UTIL_PrecacheOther( "monster_headcrab" );
#endif
	if (g_modFeatures.vortigaunt_armor_charge)
	{
		UTIL_PrecacheOther( "charge_token", GetProjectileOverrides() );
	}
}

void CISlave::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "energy"))
	{
		m_freeEnergy = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CFollowingMonster::KeyValue(pkvd);
}

void CISlave::UpdateOnRemove()
{
	ClearBeams();
	RemoveHandGlows();
	RemoveChargeToken();

	CFollowingMonster::UpdateOnRemove();
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CISlave::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	// don't slash one of your own
	if( ( damageInfo.type & DMG_SLASH ) && pevAttacker ) {
		CBaseEntity* pAttacker = Instance( pevAttacker );
		if (pAttacker && IRelationship( pAttacker ) == R_AL)
			return 0;
	}

	m_afMemory |= bits_MEMORY_ISLAVE_PROVOKED;
	return CFollowingMonster::TakeDamage( pevInflictor, pevAttacker, damageInfo );
}

void CISlave::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	if( (damageInfo.type & DMG_SHOCK)) {
		if (!pevAttacker)
			return;
		CBaseEntity* pAttacker = Instance( pevAttacker );
		if (pAttacker && IRelationship( pAttacker ) == R_AL)
			return;
	}

	CFollowingMonster::TraceAttack( pevInflictor, pevAttacker, damageInfo, vecDir, ptr );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlSlaveAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSlaveAttack1[] =
{
	{ 
		tlSlaveAttack1,
		ARRAYSIZE ( tlSlaveAttack1 ), 
		//bits_COND_CAN_MELEE_ATTACK1 | // don't interrupt electro attack by melee attack if enemy is close enough for melee attack
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Slave Range Attack1"
	},
};

Task_t tlSlaveHealOrReviveAttack[] =
{
	{ TASK_STOP_MOVING,				0	},
	{ TASK_MOVE_TO_TARGET_RANGE,	128 },
	{ TASK_FACE_TARGET,				0	},
	{ TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK,	0	}
};

Schedule_t	slSlaveHealOrReviveAttack[] =
{
	{ 
		tlSlaveHealOrReviveAttack,
		ARRAYSIZE ( tlSlaveHealOrReviveAttack ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Slave Heal or Revive Range Attack"
	},
};

Task_t tlSlaveCoverAndSummon[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FIND_SPOT_AWAY_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_ISLAVE_SUMMON_FAMILIAR, (float)0 },
};

Schedule_t slSlaveCoverAndSummon[] =
{
	{
		tlSlaveCoverAndSummon,
		ARRAYSIZE( tlSlaveCoverAndSummon ),
		bits_COND_NEW_ENEMY,
		0,
		"Slave Run Away and Summon"
	},
};

Task_t tlSlaveSummon[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_ISLAVE_SUMMON_FAMILIAR, (float)0 }
};

Schedule_t slSlaveSummon[] =
{
	{
		tlSlaveSummon,
		ARRAYSIZE( tlSlaveSummon ),
		bits_COND_NEW_ENEMY,
		0,
		"Slave Summon"
	}
};

Task_t tlSlaveGiveArmor[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)100 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_ARM },
	{ TASK_WAIT, 0.5f },
	{ TASK_ISLAVE_MAKE_CHARGE_TOKEN, (float)0 },
	{ TASK_WAIT, 0.6f },
	{ TASK_ISLAVE_SEND_CHARGE_TOKEN, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_IDLE_STAND }
};

Schedule_t slSlaveGiveArmor[] =
{
	{
		tlSlaveGiveArmor,
		ARRAYSIZE( tlSlaveGiveArmor ),
		bits_COND_NEW_ENEMY|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Slave Give Armor"
	}
};

DEFINE_CUSTOM_SCHEDULES( CISlave )
{
	slSlaveAttack1,
	slSlaveHealOrReviveAttack,
	slSlaveCoverAndSummon,
	slSlaveSummon,
	slSlaveGiveArmor
};

IMPLEMENT_CUSTOM_SCHEDULES( CISlave, CFollowingMonster )

//=========================================================
//=========================================================

void CISlave::OnChangeSchedule(Schedule_t *pNewSchedule)
{
	ClearBeams();
	RemoveChargeToken();
	m_clawStrikeNum = 0;
	CFollowingMonster::OnChangeSchedule(pNewSchedule);
}

Schedule_t *CISlave::GetSchedule( void )
{
/*
	if( pev->spawnflags )
	{
		pev->spawnflags = 0;
		return GetScheduleOfType( SCHED_RELOAD );
	}
*/
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if( pSound )
		{
			if( pSound->m_iType & bits_SOUND_DANGER )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			if( pSound->m_iType & bits_SOUND_COMBAT )
				m_afMemory |= bits_MEMORY_ISLAVE_PROVOKED;
		}
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		// dead enemy
		if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if( IsVortWounded(this) )
		{
			if( !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				const int sched = CanSpawnFamiliar() ? (int)SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR : (int)SCHED_RETREAT_FROM_ENEMY;

				if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					return GetScheduleOfType( sched );
				}
				if( HasConditions( bits_COND_SEE_ENEMY ) && HasConditions( bits_COND_ENEMY_FACING_ME ) )
				{
					return GetScheduleOfType( sched );
				}
			}
		}
		break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
	{
		if( !HasConditions( bits_COND_NEW_ENEMY | bits_COND_SEE_ENEMY ) ) // ensure there's no enemy
		{
			if ( HasFreeEnergy() && CheckHealOrReviveTargets()) {
				SetHealTargetAsTargetEnt();
				if (CanGoToTargetEnt()) {
					ALERT(at_aiconsole, "Vort gonna heal or revive friend when idle. State is %s\n", m_MonsterState == MONSTERSTATE_ALERT ? "alert" : "idle");
					return GetScheduleOfType( SCHED_ISLAVE_HEAL_OR_REVIVE );
				}
			}
		}
		if (HasFreeEnergy() && g_modFeatures.vortigaunt_armor_charge && !HasMemory(bits_MEMORY_ISLAVE_HAS_LAUNCHED_TOKEN))
		{
			CBasePlayer* pPlayer = static_cast<CBasePlayer*>(FollowedPlayer());
			if (pPlayer && pPlayer->HasSuit() && pPlayer->IsAlive() && pPlayer->pev->armorvalue < pPlayer->MaxArmor()/4 &&
					FVisible(pPlayer) && (pPlayer->pev->origin - pev->origin).Length() < 128)
			{
				return GetScheduleOfType(SCHED_ISLAVE_GIVE_CHARGE);
			}
		}
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
		break;
	}
	default:
		break;
	}
	return CFollowingMonster::GetSchedule();
}

Schedule_t *CISlave::GetScheduleOfType( int Type ) 
{
	switch( Type )
	{
	case SCHED_FAIL:
		if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			return CFollowingMonster::GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
	case SCHED_CHASE_ENEMY_FAILED:
		if ( HasFreeEnergy() && CheckHealOrReviveTargets() )
		{
			SetHealTargetAsTargetEnt();
			if (CanGoToTargetEnt())
			{
				ALERT(at_aiconsole, "Vort gonna heal or revive friends after chase enemy sched fail\n");
				return GetScheduleOfType( SCHED_ISLAVE_HEAL_OR_REVIVE );
			}
		}
		else if ( m_MonsterState == MONSTERSTATE_COMBAT && !HasConditions(bits_COND_ENEMY_TOOFAR) && CanSpawnFamiliar() )
		{
			return GetScheduleOfType( SCHED_ISLAVE_SUMMON_FAMILIAR );
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slSlaveAttack1;
	case SCHED_RANGE_ATTACK2:
		return slSlaveAttack1;
	case SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR:
		return slSlaveCoverAndSummon;
	case SCHED_ISLAVE_SUMMON_FAMILIAR:
		if (m_failSchedule == Type) {
			ALERT(at_aiconsole, "Vort gonna spawn familiar because it was set to failschedule\n");
		}
		return slSlaveSummon;
	case SCHED_ISLAVE_HEAL_OR_REVIVE:
		return slSlaveHealOrReviveAttack;
	case SCHED_ISLAVE_GIVE_CHARGE:
		return slSlaveGiveArmor;
	case SCHED_RETREAT_FROM_ENEMY_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && HasConditions( bits_COND_SEE_ENEMY ) )
			{
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
		}
		break;
	}
	
	return CFollowingMonster::GetScheduleOfType( Type );
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CISlave::ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = HandPosition(side);

	for( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if( flDist == 1.0f )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CreateBeamFromVisual(GetVisual(powerupBeamVisual));
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( AttachmentFromSide(side) );
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;
}

void CISlave::ArmBeamMessage( int side )
{
	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = HandPosition(side);

	for( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	const Visual* visual = GetVisual(idleBeamVisual);
	if (visual->modelIndex)
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE( TE_BEAMENTPOINT );
			WRITE_SHORT( entindex() + 0x1000 * (AttachmentFromSide(side)) );
			WRITE_VECTOR( tr.vecEndPos );
			WriteBeamVisual(visual);
		MESSAGE_END();
	}
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CISlave::BeamGlow()
{
	int b = m_iBeams * 32;
	if( b > 255 )
		b = 255;
	
	HandsGlowOn(b);

	for( int i = 0; i < m_iBeams; i++ )
	{
		if( m_pBeam[i]->GetBrightness() != 255 )
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}

//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CISlave::WackBeam( int side, CBaseEntity *pEntity )
{
	//Vector vecDest;
	//float flDist = 1.0;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	if( pEntity == NULL )
		return;

	m_pBeam[m_iBeams] = CreateBeamFromVisual(GetVisual(revivalBeamVisual));
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( AttachmentFromSide(side) );
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
CBaseEntity *CISlave::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
	{
		ALERT(at_warning, "Vort didn't zap because too many beams!\n");
		return NULL;
	}

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	if (IsValidHealTarget(m_hWounded)) {
		vecAim = ( ( m_hWounded->BodyTarget( vecSrc ) ) - vecSrc ).Normalize();
		ALERT(at_aiconsole, "Vort shoot friend on purpose to heal\n");
	} else {
		vecAim = ShootAtEnemy( vecSrc );
	}

	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr );

	m_pBeam[m_iBeams] = CreateBeamFromVisual(GetVisual(zapBeamVisual));
	if( !m_pBeam[m_iBeams] )
		return NULL;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( AttachmentFromSide(side) );
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;

	CBaseEntity* pResult = NULL;
	pEntity = CBaseEntity::Instance( tr.pHit );
	if( pEntity != NULL && pEntity->pev->takedamage )
	{
		if (IRelationship(pEntity) < R_DL && FClassnameIs(pEntity->pev, STRING(pev->classname))) {
			if (AbleToHeal() && HealOther(pEntity)) {
				ALERT(at_aiconsole, "Vortigaunt healed friend with zap attack\n");
			}
		} else {
			pEntity->TraceAttack( pev, pev, DamageInfo{gSkillData.slaveDmgZap, DMG_SHOCK}, vecAim, &tr );
#if FEATURE_ISLAVE_ENERGY
			if (pEntity->pev->flags & (FL_CLIENT | FL_MONSTER)) {
				//TODO: check that target is actually a living creature, not machine
				const float toHeal = gSkillData.slaveDmgZap;
				int healed = 0;
				if (g_modFeatures.vortigaunt_selfheal)
					healed = TakeHealth(this, toHeal, DMG_GENERIC);
				if (healed) // give some health to vortigaunt like in Decay bonus mission
				{
					ALERT(at_aiconsole, "Vortigaunt gets health from enemy\n");
				}
				if (toHeal > healed)
				{
					m_freeEnergy += toHeal - healed;
					ALERT(at_aiconsole, "Vortigaunt gets energy from enemy. Energy level: %d\n", (int)m_freeEnergy);
				}
			}
#endif
		}
		pResult = pEntity;
	}
	EmitSoundScriptAmbient(tr.vecEndPos, electroSoundScript);
	return pResult;
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CISlave::ClearBeams()
{
	for( int i = 0; i < ISLAVE_MAX_BEAMS; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	
	HandsGlowOff();
	RemoveSummonBeams();

	StopSoundScript(zapPowerupSoundScript);
}

void CISlave::CoilBeam()
{
	const Visual* visual = GetVisual(coilBeamVisual);

	if (!visual->modelIndex)
		return;

	const Vector coilOrigin = pev->origin + Vector(0, 0, 16.0f);
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_CIRCLE( coilOrigin, ISLAVE_COIL_ATTACK_RADIUS*5 );
		WriteBeamVisual(visual);
	MESSAGE_END();

	const Vector coilOrigin2 = pev->origin + Vector(0, 0, 48.0f);
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_CIRCLE( coilOrigin2, ISLAVE_COIL_ATTACK_RADIUS*2 );
		WriteBeamVisual(visual);
	MESSAGE_END();
}

void CISlave::HandGlowOff(CSprite *handGlow)
{
	if (handGlow) {
		handGlow->pev->effects |= EF_NODRAW;
	}
}

void CISlave::HandsGlowOff()
{
	HandGlowOff(m_handGlow1);
	HandGlowOff(m_handGlow2);
}

void CISlave::HandsGlowOn(int brightness)
{
	HandGlowOn(m_handGlow1, brightness);
	HandGlowOn(m_handGlow2, brightness);
}

void CISlave::HandGlowOn(CSprite *handGlow, int brightness)
{
	if (handGlow) {
		handGlow->SetBrightness(brightness);
		UTIL_SetOrigin(handGlow->pev, pev->origin);
		handGlow->SetScale(brightness / (float)255 * 0.3);
		handGlow->pev->effects &= ~EF_NODRAW;
	}
}

void CISlave::StartMeleeAttackGlow(int side)
{
	CSprite* handGlow = side == ISLAVE_LEFT_ARM ? m_handGlow2 : m_handGlow1;
	HandGlowOn(handGlow);

	const Visual* visual = GetVisual(trailBeamVisual);
	if (!visual->modelIndex)
		return;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * (AttachmentFromSide(side)) );
		WriteBeamFollowVisual( visual );
	MESSAGE_END();
}

bool CISlave::CanUseGlowArms()
{
	return FEATURE_ISLAVE_HANDGLOW && g_modFeatures.vortigaunt_arm_boost && ((pev->spawnflags & SF_SQUADMONSTER_LEADER) || HasFreeEnergy());
}

Vector CISlave::HandPosition(int side)
{
	UTIL_MakeAimVectors( pev->angles );
	return pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
}

void CISlave::CreateSummonBeams()
{
	UTIL_MakeVectors(pev->angles);
	Vector vecEnd = pev->origin + gpGlobals->v_forward * 36;
	if (m_handGlow1) {
		m_handsBeam1 = CreateSummonBeam(vecEnd, 1);
	}
	if (m_handGlow2) {
		m_handsBeam2 = CreateSummonBeam(vecEnd, 2);
	}
}

CBeam* CISlave::CreateSummonBeam(const Vector& vecEnd, int attachment)
{
	CBeam* beam = CreateBeamFromVisual(GetVisual(summonBeamVisual));
	if( !beam )
		return beam;

	beam->PointEntInit(vecEnd, entindex());
	beam->SetEndAttachment(attachment);
	return beam;
}

void CISlave::RemoveSummonBeams()
{
	UTIL_Remove(m_handsBeam1);
	m_handsBeam1 = NULL;
	UTIL_Remove(m_handsBeam2);
	m_handsBeam2 = NULL;
}

void CISlave::RemoveHandGlows()
{
	UTIL_Remove(m_handGlow1);
	m_handGlow1 = NULL;
	UTIL_Remove(m_handGlow2);
	m_handGlow2 = NULL;
}

void CISlave::RemoveChargeToken()
{
	UTIL_Remove(m_chargeToken);
	m_chargeToken = NULL;
}

float CISlave::HealPower()
{
	return Q_min(gSkillData.slaveDmgZap, m_freeEnergy);
}

void CISlave::SpendEnergy(float energy)
{
	// It's ok to be negative. Vort must restore power to positive values to proceed with healing or reviving.

	if (pev->spawnflags & SF_SQUADMONSTER_LEADER) // leader spends less energy
		m_freeEnergy -= energy/2;
	else
		m_freeEnergy -= energy;
}

bool CISlave::HasFreeEnergy()
{
	return m_freeEnergy > 0;
}

bool CISlave::CanRevive()
{
	return AbleToRevive() && m_hDead != 0 && HasFreeEnergy();
}

int CISlave::HealOther(CBaseEntity *pEntity)
{
	int result = 0;
	if (pEntity->IsFullyAlive()) {
		result = pEntity->TakeHealth(this, HealPower(), DMG_GENERIC);
		SpendEnergy(result);
	}
	return result;
}

bool CISlave::CanSpawnFamiliar()
{
#if FEATURE_ISLAVE_FAMILIAR
	if ((pev->weapons & (ISLAVE_SNARKS | ISLAVE_HEADCRABS)) != 0) {
		if (!HasMemory(bits_MEMORY_ISLAVE_FAMILIAR_IS_ALIVE) && m_flSpawnFamiliarTime < gpGlobals->time) {
			return true;
		}
	}
#endif
	return false;
}

void CISlave::PlayUseSentence()
{
	SENTENCEG_PlayRndSz( ENT( pev ), "SLV_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch );
}

void CISlave::PlayUnUseSentence()
{
	SENTENCEG_PlayRndSz( ENT( pev ), "SLV_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch );
}

bool CISlave::EmitSoundScriptTalk(const char *name)
{
	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchRelative(m_voicePitch);
	return EmitSoundScript(name, paramOverride);
}

void CISlave::ReportAIState(ALERT_TYPE level )
{
	CFollowingMonster::ReportAIState(level);
#if FEATURE_ISLAVE_ENERGY
	ALERT(level, "Free energy: %3.1f. ", (double)m_freeEnergy);
#endif
	ALERT(level, "Current number of beams: %d. ", m_iBeams);
}

class CDeadISlave : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/islave.mdl"; }
	int	DefaultClassify() { return	CLASS_ALIEN_MILITARY; }

	const char* getPos(int pos) const;
	static const char *m_szPoses[5];
};

const char *CDeadISlave::m_szPoses[] = { "dead_on_stomach", "dieheadshot", "diesimple", "diebackward", "dieforward" };

const char* CDeadISlave::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_alien_slave_dead, CDeadISlave )

void CDeadISlave::Spawn()
{
	bool shouldForceLastFrame = m_iPose != 0;
	SpawnHelper(BLOOD_COLOR_YELLOW);
	if (pev->sequence == -1)
	{
		if (strcmp(getPos(m_iPose), "dead_on_stomach") == 0)
		{
			pev->sequence = LookupSequence( "dieheadshot" );
			shouldForceLastFrame = true;
		}
	}
	MonsterInitDead();
	if (shouldForceLastFrame)
		pev->frame = 255;
}
