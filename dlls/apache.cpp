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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "combat.h"
#include "global_models.h"
#include "ggrenade.h"
#include "nodes.h"
#include "effects.h"
#include "game.h"
#include "common_soundscripts.h"
#include "visuals_utils.h"
#include "graphic_debug.h"

#define SF_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix!
#define SF_NOWRECKAGE		0x08

class CApache : public CBaseMonster
{
public:
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Precache() override;
	void KeyValue(KeyValueData* pkvd) override;
	int DefaultClassify() override { return CLASS_HUMAN_MILITARY; }
	bool HasFlesh() override { return false; }
	const char* DefaultDisplayName() override { return "Apache"; }
	const char* ReverseRelationshipModel() override { return "models/apachef.mdl"; }
	int BloodColor() override { return DONT_BLEED; }
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	void GibMonster() override;

	Vector DefaultMinHullSize() override { return Vector( -32.0f, -32.0f, -64.0f ); }
	Vector DefaultMaxHullSize() override { return Vector( 32.0f, 32.0f, 0.0f ); }
	void SetObjectCollisionBox() override
	{
		SetMyObjectCollisionBox(Vector( -300.0f, -300.0f, -172.0f ), Vector( 300.0f, 300.0f, 8.0f ));
	}

	void EXPORT HuntThink();
	void EXPORT FlyTouch( CBaseEntity *pOther );
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink();
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT StopUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink();

	void ShowDamage();
	void Flight();
	void FireRocket();
	bool FireGun();

	DamageInfo DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo) override;
	TakeDamageResult  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, const DamageInfo& damageInfo ) override;
	bool MustDoSmoke(const DamageInfo& damageInfo, const TraceResult* ptr);
	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	void TraceAttack( entvars_t *pevInflictor,  entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
	void ReportAIState(ALERT_TYPE level) override;

	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iSoundState; // don't save this

	int m_iBodyGibs;

	float m_flGoalSpeed;

	int m_iDoSmokePuff;
	CBeam *m_pBeam;

	bool m_iObeyTriggerMode;

	static const NamedSoundScript rotorSoundScript;
	static const NamedSoundScript fireGunSoundScript;
	static constexpr const char* crashSoundScript = "Apache.Crash";

	static const NamedVisual sharedSmokeVisual;
	static const NamedVisual fallingSmokeVisual;
	static const NamedVisual crashSmokeVisual;
	static const NamedVisual rocketSmokeVisual;
	static const NamedVisual damageSmokeVisual;
	static const NamedVisual fireBallVisual;
	static const NamedVisual blastCircleVisual;

protected:
	void SpawnImpl(const char* modelName);
	void PrecacheImpl(const char* modelName, const char* gibModel);
	void SetRotorVolumeOverride(SoundScriptParamOverride& param)
	{
		if (pev->armorvalue > 0.0f && pev->armorvalue <= 1.0f)
		{
			param.OverrideVolumeAbsolute(pev->armorvalue);
		}
	}
};

LINK_ENTITY_TO_CLASS( monster_apache, CApache )

TYPEDESCRIPTION	CApache::m_SaveData[] =
{
	DEFINE_FIELD( CApache, m_iRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_flNextRocket, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_flPrevSeen, FIELD_TIME ),
	//DEFINE_FIELD( CApache, m_iSoundState, FIELD_INTEGER ),		// Don't save, precached
	//DEFINE_FIELD( CApache, m_iSpriteTexture, FIELD_INTEGER ),
	//DEFINE_FIELD( CApache, m_iExplode, FIELD_INTEGER ),
	//DEFINE_FIELD( CApache, m_iBodyGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CApache, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_iDoSmokePuff, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_iObeyTriggerMode, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CApache, CBaseMonster )

const NamedSoundScript CApache::rotorSoundScript =  {
	CHAN_STATIC,
	{"apache/ap_rotor2.wav"},
	VOL_NORM,
	0.3f,
	"Apache.Rotor"
};

const NamedSoundScript CApache::fireGunSoundScript = {
	CHAN_WEAPON,
	{"turret/tu_fire1.wav"},
	1.0f,
	0.3f,
	"Apache.FireGun"
};

const NamedVisual CApache::sharedSmokeVisual = BuildVisual("Apache.SmokeBase")
		.Model(g_pModelNameSmoke)
		.Alpha(255)
		.RenderMode(kRenderTransAlpha);

const NamedVisual CApache::fallingSmokeVisual = BuildVisual("Apache.FallingSmoke")
		.Scale(10.0f)
		.Framerate(10.0f)
		.Mixin(&CApache::sharedSmokeVisual);

const NamedVisual CApache::crashSmokeVisual = BuildVisual("Apache.CrashSmoke")
		.Scale(25.0f)
		.Framerate(5.0f)
		.Mixin(&CApache::sharedSmokeVisual);

const NamedVisual CApache::rocketSmokeVisual = BuildVisual("Apache.RocketSmoke")
		.Scale(2.0f)
		.Framerate(12)
		.Mixin(&CApache::sharedSmokeVisual);

const NamedVisual CApache::damageSmokeVisual = BuildVisual("Apache.DamageSmoke")
		.Scale(FloatRange(0.9f, 2.9f))
		.Framerate(12)
		.Mixin(&CApache::sharedSmokeVisual);

const NamedVisual CApache::fireBallVisual = BuildVisual::Animated("Apache.Fireball")
		.Model("sprites/fexplo.spr")
		.RenderMode(kRenderTransAdd)
		.Scale(12.0f)
		.Alpha(255);

const NamedVisual CApache::blastCircleVisual = BuildVisual("Apache.BlastCircle")
		.Model("sprites/white.spr")
		.Life(0.4f)
		.BeamParams(32, 0)
		.RenderColor(255, 255, 192)
		.Alpha(128);

void CApache::Spawn()
{
	SpawnImpl("models/apache.mdl");
}

void CApache::SpawnImpl(const char *modelName)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetMyModel( modelName );
	SetMySize();
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	SetMyHealth( GetSkillValue("apache_health") );
	pev->max_health = pev->health;

	SetMyFieldOfView(-0.707f); // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG( 0, 0xFF );

	InitBoneControllers();

	if( pev->spawnflags & SF_WAITFORTRIGGER )
	{
		SetThink( &CApache::NullThink );
		SetUse( &CApache::StartupUse );
	}
	else
	{
		SetThink( &CApache::HuntThink );
		SetTouch( &CApache::FlyTouch );
		SetUse( &CApache::StopUse );
	}
	pev->nextthink = gpGlobals->time + 1.0f;

	m_iRockets = 10;

	InitLootRandomSeed();
}

void CApache::Precache()
{
	PrecacheImpl("models/apache.mdl", "models/metalplategibs_green.mdl");
}

void CApache::PrecacheImpl(const char* modelName, const char* gibModel)
{
	PrecacheMyModel( modelName );
	m_iBodyGibs = PrecacheMyGibModel(gibModel);

	PRECACHE_SOUND( "apache/ap_rotor1.wav" );
	RegisterAndPrecacheSoundScript(rotorSoundScript);
	PRECACHE_SOUND( "apache/ap_rotor3.wav" );
	PRECACHE_SOUND( "apache/ap_whine1.wav" );

	RegisterAndPrecacheSoundScript(crashSoundScript, NPC::crashSoundScript);

	RegisterVisual(blastCircleVisual);

	RegisterAndPrecacheSoundScript(fireGunSoundScript);

	RegisterVisual(fallingSmokeVisual);
	RegisterVisual(crashSmokeVisual);
	RegisterVisual(rocketSmokeVisual);
	RegisterVisual(damageSmokeVisual);

	PRECACHE_MODEL( "sprites/lgtning.spr" );

	RegisterVisual(fireBallVisual);

	UTIL_PrecacheOther( "hvr_rocket", GetProjectileOverrides() );
	UTIL_PrecacheOther( "cycler_wreckage", GetProjectileOverrides() );
}

void CApache::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "rotorvolume" ) )
	{
		pev->armorvalue = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq(pkvd->szKeyName, "m_iObeyTriggerMode" ) )
	{
		m_iObeyTriggerMode = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CApache::NullThink()
{
	StudioFrameAdvance();
	FCheckAITrigger();
	pev->nextthink = gpGlobals->time + 0.5f;
	GlowShellUpdate();
	ShowDamage();
}

void CApache::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!m_iObeyTriggerMode || useType != USE_OFF)
	{
		SetThink(&CApache::HuntThink);
		SetTouch(&CApache::FlyTouch);
		pev->nextthink = gpGlobals->time + 0.1f;
		SetUse(&CApache::StopUse);
	}
}

void CApache::StopUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iObeyTriggerMode && useType != USE_ON)
	{
		pev->velocity = g_vecZero;
		pev->avelocity = g_vecZero;
		m_iSoundState = 0;
		StopSoundScript(rotorSoundScript);

		SetThink(&CApache::NullThink);
		SetTouch(nullptr);
		pev->nextthink = gpGlobals->time + 0.1f;
		SetUse(&CApache::StartupUse);
	}
}

KilledResult CApache::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3f;

	StopSoundScript(rotorSoundScript);

	SetMySize();
	SetThink( &CApache::DyingThink );
	SetTouch( &CApache::CrashTouch );
	SetUse(nullptr);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;

	if( pev->spawnflags & SF_NOWRECKAGE )
	{
		m_flNextRocket = gpGlobals->time + 4.0f;
	}
	else
	{
		m_flNextRocket = gpGlobals->time + 15.0f;
	}
	OnDying(false);
	return KilledResult();
}

void CApache::DyingThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	GlowShellUpdate();

	pev->avelocity = pev->avelocity * 1.02f;

	// still falling?
	if( m_flNextRocket > gpGlobals->time )
	{
		FCheckAITrigger();

		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This just makes a dynamic light now
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150.0f, 150.0f ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150.0f, -50.0f ) );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG( 0, 29 ) + 30 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		// lots of smoke
		const Vector smokePosition(
					pev->origin.x + RANDOM_FLOAT(-150.0f, 150.0f),
					pev->origin.y + RANDOM_FLOAT(-150.0f, 150.0f),
					pev->origin.z + RANDOM_FLOAT(-150.0f, -50.0f)
		);
		SendSmoke(smokePosition, GetVisual(fallingSmokeVisual));

		Vector vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL );

			// position
			WRITE_VECTOR( vecSpot );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 132 );

			// velocity
			WRITE_VECTOR( pev->velocity );

			// randomization
			WRITE_BYTE( 50 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 4 );	// let client decide

			// duration
			WRITE_BYTE( 30 );// 3.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2f;
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 300.0f );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 8 ); // framerate
		MESSAGE_END();
		*/

		// fireball
		SendSprite(vecSpot + Vector(0, 0, 256.0f), GetVisual(fireBallVisual));

		// big smoke
		SendSmoke(vecSpot + Vector(0, 0, 512.0f), GetVisual(crashSmokeVisual));

		// blast circle
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_CIRCLE( pev->origin, 2000 ); // reach damage radius over .2 seconds
			WriteBeamVisual(GetVisual(blastCircleVisual));
		MESSAGE_END();

		EmitSoundScript(crashSoundScript);

		RadiusDamage( pev->origin, pev, pev, DamageInfo{GetSkillValue("apache_dmg_blast"), DMG_BLAST}, CLASS_NONE );

		if(/*!( pev->spawnflags & SF_NOWRECKAGE ) && */( pev->flags & FL_ONGROUND ) )
		{
			CBaseEntity *pWreckage = Create( "cycler_wreckage", pev->origin, pev->angles, nullptr, GetProjectileOverrides() );
			// SET_MODEL( ENT( pWreckage->pev ), STRING( pev->model ) );
			UTIL_SetSize( pWreckage->pev, Vector( -200.0f, -200.0f, -128.0f ), Vector( 200.0f, 200.0f, -32.0f ) );
			pWreckage->pev->frame = pev->frame;
			pWreckage->pev->sequence = pev->sequence;
			pWreckage->pev->framerate = 0;
			pWreckage->pev->dmgtime = gpGlobals->time + 5.0f;
		}

		// gibs
		vecSpot = pev->origin + ( pev->mins + pev->maxs ) * 0.5f;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_VECTOR( vecSpot + Vector(0, 0, 64.0f) );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 128 );

			// velocity
			WRITE_COORD( 0 );
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 30 );

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 200 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CApache::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if( pOther->pev->solid == SOLID_BSP )
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		// UNDONE, do a real bounce
		pev->velocity = pev->velocity + tr.vecPlaneNormal * ( pev->velocity.Length() + 200.0f );
	}
}

void CApache::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	switch(pOther->pev->solid)
	{
	case SOLID_BBOX:
	case SOLID_SLIDEBOX:
	case SOLID_BSP:
		SetTouch( NULL );
		m_flNextRocket = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
		break;
	default:
		break;
	}
}

void CApache::GibMonster()
{
	// EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "common/bodysplat.wav", 0.75f, ATTN_NORM, 0, 200 );
}

void CApache::HuntThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	GlowShellUpdate();

	ShowDamage();

	if( m_pGoalEnt == NULL && !FStringNull( pev->target ) )// this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if( m_pGoalEnt )
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if( m_hEnemy == NULL )
	{
		Look( 4092 );
		m_hEnemy = BestVisibleEnemy();

		//If i have an enemy i'm in combat, otherwise i'm patrolling.
		if (m_hEnemy != 0)
		{
			m_MonsterState = MONSTERSTATE_COMBAT;
		}
		else
		{
			m_MonsterState = MONSTERSTATE_ALERT;
		}
	}

	Listen();

	// generic speed up
	if( m_flGoalSpeed < 800.0f )
		m_flGoalSpeed += 5.0f;

	if( m_hEnemy != 0 )
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if( FVisible( m_hEnemy ) )
		{
			if( m_flLastSeen < gpGlobals->time - 5.0f )
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center();
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = ( m_posTarget - pev->origin ).Normalize();

	float flLength = ( pev->origin - m_posDesired ).Length();

	if( m_pGoalEnt )
	{
		// ALERT( at_console, "%.0f\n", flLength );

		if( flLength < 128.0f )
		{
			CBaseEntity* pPrevGoalEnt = m_pGoalEnt;

			m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
			if( m_pGoalEnt )
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				flLength = ( pev->origin - m_posDesired ).Length();
			}

			if (pPrevGoalEnt && !FStringNull(pPrevGoalEnt->pev->message))
			{
				FireTargets(STRING(pPrevGoalEnt->pev->message), this, this);
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if( flLength > 250.0f ) // 500
	{
		// float flLength2 = ( m_posTarget - pev->origin ).Length() * ( 1.5f - DotProduct( ( m_posTarget - pev->origin ).Normalize(), pev->velocity.Normalize() ) );
		// if( flLength2 < flLength )
		if( m_flLastSeen + 90.0f > gpGlobals->time && DotProduct( ( m_posTarget - pev->origin ).Normalize(), ( m_posDesired - pev->origin ).Normalize() ) > 0.25f )
		{
			m_vecDesired = ( m_posTarget - pev->origin ).Normalize();
		}
		else
		{
			m_vecDesired = ( m_posDesired - pev->origin ).Normalize();
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight();

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if( ( m_flLastSeen + 1.0f > gpGlobals->time ) && ( m_flPrevSeen + 2.0f < gpGlobals->time ) )
	{
		if( FireGun() )
		{
			// slow down if we're fireing
			if( m_flGoalSpeed > 400.0f )
				m_flGoalSpeed = 400.0f;
		}

		// don't fire rockets and gun on easy mode
		if (GetSkillValue("apache_rockets_and_gun") == 0.0f)
			m_flNextRocket = gpGlobals->time + 10.0f;
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = ( gpGlobals->v_forward * 800.0f + pev->velocity ).Normalize();
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0.0f, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100.0f, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	auto allyInRange = [this](const Vector& vecLocation, float flDist)
	{
		CBaseEntity* pEntity = nullptr;
		while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecLocation, flDist)) != nullptr)
		{
			CBaseMonster* monster = pEntity->MyMonsterPointer();
			if (monster != nullptr && FBitSet(monster->pev->flags, FL_MONSTER|FL_CLIENT) && monster->pev->deadflag != DEAD_DEAD && IRelationship(monster) == R_AL)
			{
				ALERT(at_aiconsole, "%s: Ally %s at search radius.\n", STRING(pev->classname), STRING(monster->pev->classname));
				return true;
			}
		}
		return false;
	};

	if( ( m_iRockets % 2 ) == 1 )
	{
		FireRocket();
		if (m_iRockets <= 0)
		{
			m_flNextRocket = gpGlobals->time + GetSkillValue("apache_rocket_reload_time");
			m_iRockets = 10;
		}
		else
		{
			m_flNextRocket = gpGlobals->time + GetSkillValue("apache_rocket_delay");
		}
	}
	else if( pev->angles.x < 0.0f && DotProduct( pev->velocity, gpGlobals->v_forward ) > -100.0f && m_flNextRocket < gpGlobals->time )
	{
		if( m_flLastSeen + 60.0f > gpGlobals->time )
		{
			if( m_hEnemy != 0 )
			{
				// make sure it's a good shot
				if( DotProduct( m_vecTarget, vecEst ) > .965f )
				{
					TraceResult tr;

					UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096.0f, ignore_monsters, edict(), &tr );
					if( (tr.vecEndPos - m_posTarget ).IsLengthLessThan(512.0f) )
					{
						if (!allyInRange(tr.vecEndPos + Vector(0, 0, 32), 256.0f))
						{
							FireRocket();
						}
						else
						{
							m_flNextRocket = gpGlobals->time + 1.0f;
						}
					}
				}
			}
			else if (m_flLastSeen + 15.0f > gpGlobals->time)
			{
				TraceResult tr;

				UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096.0f, dont_ignore_monsters, edict(), &tr );
				// just fire when close
				if( ( tr.vecEndPos - m_posTarget ).IsLengthLessThan(512.0f) )
				{
					if (!allyInRange(tr.vecEndPos + Vector(0, 0, 32), 384.0f))
					{
						FireRocket();
					}
					else
					{
						m_flNextRocket = gpGlobals->time + 1.0f;
					}
				}
			}
		}
	}

	FCheckAITrigger();
}

void CApache::Flight()
{
	// tilt model 5 degrees
	Vector vecAdj = Vector( 5.0f, 0.0f, 0.0f );

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2.0f + vecAdj );
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0.0f, 0.0f, 384.0f );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );
	
	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );

	if( flSide < 0.0f )
	{
		if( pev->avelocity.y < 60.0f )
		{
			pev->avelocity.y += 8.0f; // 9 * ( 3.0 / 2.0 );
		}
	}
	else
	{
		if( pev->avelocity.y > -60.0f )
		{
			pev->avelocity.y -= 8.0f; // 9 * ( 3.0 / 2.0 );
		}
	}
	pev->avelocity.y *= 0.98f;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 1.0f + vecAdj );
	Vector vecEst = pev->origin + pev->velocity * 2.0f + gpGlobals->v_up * m_flForce * 20.0f - Vector( 0.0f, 0.0f, 384.0f * 2.0f );

	// add immediate force
	UTIL_MakeAimVectors( pev->angles + vecAdj );
	pev->velocity.x += gpGlobals->v_up.x * m_flForce;
	pev->velocity.y += gpGlobals->v_up.y * m_flForce;
	pev->velocity.z += gpGlobals->v_up.z * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4f; // 32ft/sec


	float flSpeed = pev->velocity.Length();
	float flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0.0f ), Vector( pev->velocity.x, pev->velocity.y, 0.0f ) );
	if( flDir < 0 )
		flSpeed = -flSpeed;

	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct( m_posDesired - vecEst, gpGlobals->v_right );

	// fly sideways
	if( flSlip > 0.0f )
	{
		if( pev->angles.z > -30.0f && pev->avelocity.z > -15.0f )
			pev->avelocity.z -= 4.0f;
		else
			pev->avelocity.z += 2.0f;
	}
	else
	{
		if( pev->angles.z < 30 && pev->avelocity.z < 15.0f )
			pev->avelocity.z += 4.0f;
		else
			pev->avelocity.z -= 2.0f;
	}

	// sideways drag
	pev->velocity.x = pev->velocity.x * ( 1.0f - fabs( gpGlobals->v_right.x ) * 0.05f );
	pev->velocity.y = pev->velocity.y * ( 1.0f - fabs( gpGlobals->v_right.y ) * 0.05f );
	pev->velocity.z = pev->velocity.z * ( 1.0f - fabs( gpGlobals->v_right.z ) * 0.05f );

	// general drag
	pev->velocity = pev->velocity * 0.995f;

	// apply power to stay correct height
	if( m_flForce < 80.0f && vecEst.z < m_posDesired.z )
	{
		m_flForce += 12.0f;
	}
	else if( m_flForce > 30.0f )
	{
		if( vecEst.z > m_posDesired.z )
			m_flForce -= 8.0f;
	}

	// pitch forward or back to get to target
	if( flDist > 0.0f && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && pev->angles.x + pev->avelocity.x > -40.0f )
	{
		// ALERT( at_console, "F " );
		// lean forward
		pev->avelocity.x -= 12.0f;
	}
	else if( flDist < 0.0f && flSpeed > -50.0f && pev->angles.x + pev->avelocity.x < 20.0f )
	{
		// ALERT( at_console, "B " );
		// lean backward
		pev->avelocity.x += 12.0f;
	}
	else if( pev->angles.x + pev->avelocity.x > 0.0f )
	{
		// ALERT( at_console, "f " );
		pev->avelocity.x -= 4.0f;
	}
	else if( pev->angles.x + pev->avelocity.x < 0.0f )
	{
		// ALERT( at_console, "b " );
		pev->avelocity.x += 4.0f;
	}

	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce );
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce );

	// make rotor, engine sounds
	if( m_iSoundState == 0 )
	{
		SoundScriptParamOverride param;
		SetRotorVolumeOverride(param);
		param.OverridePitchRelative(110);
		EmitSoundScript(rotorSoundScript, param);
		// EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		// UNDONE: this needs to send different sounds to every player for multiplayer.	
		if( pPlayer )
		{
			float pitch = DotProduct( pev->velocity - pPlayer->pev->velocity, ( pPlayer->pev->origin - pev->origin ).Normalize() );

			pitch = (int)( 100.0f + pitch / 50.0f );

			if( pitch > 250.0f )
				pitch = 250.0f;
			if( pitch < 50.0f )
				pitch = 50.0f;
			if( pitch == 100.0f )
				pitch = 101.0f;

			/*float flVol = ( m_flForce / 100.0f ) + 0.1f;
			if( flVol > 1.0f ) 
				flVol = 1.0f;*/

			SoundScriptParamOverride param;
			SetRotorVolumeOverride(param);
			param.OverridePitchRelative(pitch);
			EmitSoundScript(rotorSoundScript, param, SND_CHANGE_PITCH | SND_CHANGE_VOL);
		}
		// EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2f, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch );
	
		// ALERT( at_console, "%.0f %.2f\n", pitch, flVol );
	}
}

void CApache::FireRocket()
{
	static float side = 1.0f;

	if( m_iRockets <= 0 )
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + 1.5f * ( gpGlobals->v_forward * 21.0f + gpGlobals->v_right * 70.0f * side + gpGlobals->v_up * -79.0f );

	switch( m_iRockets % 5 )
	{
	case 0:
		vecSrc += gpGlobals->v_right * 10.0f;
		break;
	case 1:
		vecSrc -= gpGlobals->v_right * 10.0f;
		break;
	case 2:
		vecSrc += gpGlobals->v_up * 10.0f;
		break;
	case 3:
		vecSrc -= gpGlobals->v_up * 10.0f;
		break;
	case 4:
		break;
	}

	ProjectileParameters params("hvr_rocket", vecSrc, pev->angles, gpGlobals->v_forward, this, GetProjectileOverrides());
	CBaseEntity *pRocket = CBaseEntity::CreateAndLaunchAsProjectile(params);
	if( pRocket )
	{
		SendSmoke(vecSrc, GetVisual(rocketSmokeVisual));

		pRocket->pev->velocity += pev->velocity;

		m_iRockets--;

		side = - side;
	}
}

bool CApache::FireGun()
{
	UTIL_MakeAimVectors( pev->angles );

	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = ( m_posTarget - posGun ).Normalize();

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles( vecOut );

	angles.x = -angles.x;
	if( angles.y > 180.0f )
		angles.y = angles.y - 360.0f;
	if( angles.y < -180.0f )
		angles.y = angles.y + 360.0f;
	if( angles.x > 180.0f )
		angles.x = angles.x - 360.0f;
	if( angles.x < -180.0f )
		angles.x = angles.x + 360.0f;

	if( angles.x > m_angGun.x )
		m_angGun.x = Q_min( angles.x, m_angGun.x + 12.0f );
	if( angles.x < m_angGun.x )
		m_angGun.x = Q_max( angles.x, m_angGun.x - 12.0f );
	if( angles.y > m_angGun.y )
		m_angGun.y = Q_min( angles.y, m_angGun.y + 12.0f );
	if( angles.y < m_angGun.y )
		m_angGun.y = Q_max( angles.y, m_angGun.y - 12.0f );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = ( posBarrel - posGun ).Normalize();

	if( DotProduct( vecGun, vecTarget ) > 0.98f )
	{
#if 1
		FireBullets( 1, posGun, vecGun, VECTOR_CONE_4DEGREES, 8192, GetSkillValue("12mm_bullet"), 1 );
		EmitSoundScript(fireGunSoundScript);
#else
		static float flNext;
		TraceResult tr;
		UTIL_TraceLine( posGun, posGun + vecGun * 8192, dont_ignore_monsters, ENT( pev ), &tr );

		if( !m_pBeam )
		{
			m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 80 );
			m_pBeam->PointEntInit( pev->origin, entindex() );
			m_pBeam->SetEndAttachment( 1 );
			m_pBeam->SetColor( 255, 180, 96 );
			m_pBeam->SetBrightness( 192 );
		}

		if( flNext < gpGlobals->time )
		{
			flNext = gpGlobals->time + 0.5f;
			m_pBeam->SetStartPos( tr.vecEndPos );
		}
#endif
		return true;
	}
	else
	{
		if( m_pBeam )
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
	}
	return false;
}

void CApache::ShowDamage()
{
	if( m_iDoSmokePuff > 0 || RANDOM_LONG( 0, 99 ) > pev->health )
	{
		SendSmoke(pev->origin + Vector(0, 0, -32), GetVisual(damageSmokeVisual));
	}
	if( m_iDoSmokePuff > 0 )
		m_iDoSmokePuff--;
}

DamageInfo CApache::DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo)
{
	DamageInfo damageInfo = inputDamageInfo;

	if (damageInfo.type & DMG_BLAST)
	{
		damageInfo.damage *= 2.0f;
	}

	return damageInfo;
}

TakeDamageResult CApache::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	if( pevInflictor->owner == edict() )
		return TakeDamageResult();

	/*
	if( ( bitsDamageType & DMG_BULLET ) && flDamage > 50.0f )
	{
		// clip bullet damage at 50
		flDamage = 50.0f;
	}
	*/

	// ALERT( at_console, "%.0f\n", flDamage );
	if (damageInfo.nonLethal)
		SetNonLethalHealthThreshold();
	TakeDamageResult result = CBaseEntity::TakeDamage( pevInflictor, pevAttacker, damageInfo );

	//Are we damaged at all?
	if (pev->health < pev->max_health)
	{
		//Took some damage.
		SetConditions(bits_COND_LIGHT_DAMAGE);
		result.SetGotLightDamage();

		if (pev->health < (pev->max_health / 2))
		{
			//Seriously damaged now.
			SetConditions(bits_COND_HEAVY_DAMAGE);
			result.SetGotHeavyDamage();
		}
		else
		{
			//Maybe somebody healed us somehow (trigger_hurt with negative damage?), clear this.
			ClearConditions(bits_COND_HEAVY_DAMAGE);
		}
	}
	else
	{
		//Maybe somebody healed us somehow (trigger_hurt with negative damage?), clear this.
		ClearConditions(bits_COND_LIGHT_DAMAGE);
	}
	return result;
}

bool CApache::MustDoSmoke(const DamageInfo &damageInfo, const TraceResult *ptr)
{
	// hit hard, hits cockpit, hits engines
	return damageInfo.damage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2;
}

DamageInfo CApache::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	// ignore blades
	if( ptr->iHitgroup == 6 && ( inputDamageInfo.type & ( DMG_ENERGYBEAM | DMG_BULLET | DMG_CLUB ) ) )
	{
		DamageInfo damageInfo = inputDamageInfo;
		damageInfo.mustSkip = true;
		return damageInfo;
	}

	if( !MustDoSmoke(inputDamageInfo, ptr) )
	{
		UTIL_Ricochet( ptr->vecEndPos, 2.0f );
		DamageInfo damageInfo = inputDamageInfo;
		damageInfo.mustSkip = true;
		return damageInfo;
	}

	return inputDamageInfo;
}

void CApache::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo, Vector vecDir, TraceResult *ptr )
{
	DamageInfo damageInfo = HandleTraceAttack(pevInflictor, pevAttacker, inputDamageInfo, vecDir, ptr);

	if (damageInfo.mustSkip)
		return;

	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );
	if (pev->takedamage)
	{
		AddMultiDamage( pevInflictor, pevAttacker, this, damageInfo );

		// TODO: Smoke currently can't be expressed via trace attack effects. Keep it as is.
		if( MustDoSmoke(damageInfo, ptr) )
		{
			m_iDoSmokePuff = 3.0f + ( inputDamageInfo.damage / 5.0f );
		}
	}
}

void CApache::ReportAIState(ALERT_TYPE level)
{
	CBaseMonster::ReportAIState(level);
	ALERT(level, "Rockets: %d; ", m_iRockets);
	ALERT(level, "Current time: %g; Next rocket time: %g; Last seen enemy time: %g; Prev seen enemy time: %g; ", gpGlobals->time, m_flNextRocket, m_flLastSeen, m_flPrevSeen);
}

class CApacheHVR : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT IgniteThink();
	void EXPORT AccelerateThink();
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_vecForward;

	static const NamedSoundScript rpgSoundScript;

	static const NamedVisual modelVisual;
	static const NamedVisual trailVisual;
};

LINK_ENTITY_TO_CLASS( hvr_rocket, CApacheHVR )

TYPEDESCRIPTION	CApacheHVR::m_SaveData[] =
{
	//DEFINE_FIELD( CApacheHVR, m_iTrail, FIELD_INTEGER ),	// Dont' save, precache
	DEFINE_FIELD( CApacheHVR, m_vecForward, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CApacheHVR, CGrenade )

const NamedSoundScript CApacheHVR::rpgSoundScript = {
	CHAN_VOICE,
	{"weapons/rocket1.wav"},
	1.0f,
	0.5f,
	"Apache.RPG"
};

const NamedVisual CApacheHVR::modelVisual = BuildVisual("Apache.RocketModel")
		.Model("models/HVR.mdl");

const NamedVisual CApacheHVR::trailVisual = BuildVisual("Apache.RocketTrail")
		.Model("sprites/smoke.spr")
		.Life(1.5f)
		.BeamWidth(5)
		.RenderColor(224, 224, 255)
		.Alpha(255);

void CApacheHVR::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	ApplyVisualWithOwn(GetVisual(modelVisual));
	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CApacheHVR::IgniteThink );
	SetTouch( &CGrenade::ExplodeTouch );

	UTIL_MakeAimVectors( pev->angles );
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5f;

	pev->nextthink = gpGlobals->time + 0.1f;

	SetDefaultProjectileDamage(150.0f);
}

void CApacheHVR::Precache()
{
	PrecacheBaseGrenadeSounds();
	RegisterVisualAsMineOwn(modelVisual);
	RegisterVisual(trailVisual);
	RegisterAndPrecacheSoundScript(rpgSoundScript);
}

void CApacheHVR::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	// pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EmitSoundScript(rpgSoundScript);

	// rocket trail
	SendBeamFollow(entindex(), GetVisual(trailVisual));

	// set to accelerate
	SetThink( &CApacheHVR::AccelerateThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CApacheHVR::AccelerateThink()
{
	// check world boundaries
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	// accelerate
	float flSpeed = pev->velocity.Length();
	if( flSpeed < 1800.0f )
	{
		pev->velocity = pev->velocity + m_vecForward * 200.0f;
	}

	// re-aim
	pev->angles = UTIL_VecToAngles( pev->velocity );

	pev->nextthink = gpGlobals->time + 0.1f;
}

void CApacheHVR::LaunchAsProjectile(const ProjectileParameters &params)
{
	LaunchAsProjectileImpl(100.0f, params);
}

class CBlkopApache : public CApache
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("blkop_apache"); }
	int	DefaultClassify() override
	{
		if (g_modFeatures.blackops_classify)
			return CLASS_HUMAN_BLACKOPS;
		return CApache::DefaultClassify();
	}
};

LINK_ENTITY_TO_CLASS( monster_blkop_apache, CBlkopApache )

void CBlkopApache::Spawn()
{
	SpawnImpl("models/blkop_apache.mdl");
}

void CBlkopApache::Precache()
{
	PrecacheImpl("models/blkop_apache.mdl", "models/metalplategibs_dark.mdl");
}
