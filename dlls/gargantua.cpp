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
// Gargantua
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"nodes.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"customentity.h"
#include	"effects.h"
#include	"soundent.h"
#include	"decals.h"
#include	"explode.h"
#include	"func_break.h"
#include	"scripted.h"
#include	"followingmonster.h"
#include	"gamerules.h"
#include	"game.h"
#include	"mod_features.h"
#include	"fx_flags.h"
#include	"locus.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

//=========================================================
// Gargantua Monster
//=========================================================
const float GARG_ATTACKDIST = 80.0f;

// Garg animation events
#define GARG_AE_SLASH_LEFT			1
//#define GARG_AE_BEAM_ATTACK_RIGHT		2	// No longer used
#define GARG_AE_LEFT_FOOT			3
#define GARG_AE_RIGHT_FOOT			4
#define GARG_AE_STOMP				5
#define GARG_AE_BREATHE				6
#define BABYGARG_AE_KICK			7
#define STOMP_FRAMETIME				0.015f	// gpGlobals->frametime

// Gargantua is immune to any damage but this
#define GARG_DAMAGE					(DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST)
#define GARG_EYE_SPRITE_NAME		"sprites/gargeye1.spr"
#define GARG_BEAM_SPRITE_NAME		"sprites/xbeam3.spr"
#define GARG_BEAM_SPRITE2		"sprites/xbeam3.spr"
#define GARG_STOMP_SPRITE_NAME		"sprites/gargeye1.spr"
#define GARG_STOMP_BUZZ_SOUND		"weapons/mine_charge.wav"
#define GARG_FLAME_LENGTH		330
#define GARG_GIB_MODEL			"models/metalplategibs.mdl"

#define ATTN_GARG					(ATTN_NORM)

#define STOMP_SPRITE_COUNT			10

void SpawnExplosion( Vector center, float randomRange, float time, int magnitude );

class CSmoker;

// Spiral Effect
class CSpiral : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
	static CSpiral *Create( const Vector &origin, float height, float radius, float duration );
};

LINK_ENTITY_TO_CLASS( streak_spiral, CSpiral )

struct StompParams
{
	Vector origin;
	Vector end;
	float speed;
	float damage;
	edict_t* owner = nullptr;
	float soundAttenuation = 0.0f;
};

class CStomp : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache();
	void Think( void );
	static CStomp *StompCreate(const StompParams& stompParams, EntityOverrides entityOverrides = EntityOverrides());

	static const NamedSoundScript stompSoundScript;
protected:
	static void SetStompParams(CStomp* pStomp, const StompParams& stompParams);
	virtual const char* StompSoundScript() {
		return stompSoundScript;
	}
	virtual string_t MyClassname() {
		return MAKE_STRING("garg_stomp");
	}

	static const NamedVisual stompVisual;

	const Visual* m_stompVisual;

private:
// UNDONE: re-use this sprite list instead of creating new ones all the time
//	CSprite		*m_pSprites[STOMP_SPRITE_COUNT];
};

LINK_ENTITY_TO_CLASS( garg_stomp, CStomp )

const NamedSoundScript CStomp::stompSoundScript = {
	CHAN_BODY,
	{GARG_STOMP_BUZZ_SOUND},
	55,
	"Garg.Stomp",
};

const NamedVisual CStomp::stompVisual = BuildVisual("Garg.Stomp")
		.Model(GARG_STOMP_SPRITE_NAME)
		.Scale(1.0f)
		.RenderColor(255, 255, 255)
		.Alpha(255)
		.RenderMode(kRenderTransAdd)
		.RenderFx(kRenderFxFadeFast);

#define BABYGARG_STOMP_SPRITE_NAME "sprites/flare3.spr"

class CBabyStomp : public CStomp
{
public:
	void Precache();
	static CBabyStomp *StompCreate(const StompParams& stompParams, EntityOverrides entityOverrides = EntityOverrides());

	static constexpr const char* stompSoundScript = "BabyGarg.Stomp";
protected:
	const char* StompSoundScript() {
		return stompSoundScript;
	}
	virtual string_t MyClassname() {
		return MAKE_STRING("babygarg_stomp");
	}

	static const NamedVisual stompVisual;
};

LINK_ENTITY_TO_CLASS( babygarg_stomp, CBabyStomp )

const NamedVisual CBabyStomp::stompVisual = BuildVisual("BabyGarg.Stomp")
		.Model(BABYGARG_STOMP_SPRITE_NAME)
		.Scale(0.5f)
		.RenderColor(225, 170, 80)
		.Alpha(255)
		.RenderMode(kRenderTransAdd)
		.RenderFx(kRenderFxFadeFast);

#define SF_STOMPSHOOTER_FIRE_ONCE 1

class CStompShooter : public CPointEntity
{
public:
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData* pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS( env_stompshooter, CStompShooter )

void CStompShooter::Spawn()
{
	Precache();
}

void CStompShooter::Precache()
{
	UTIL_PrecacheOther("garg_stomp");
}

void CStompShooter::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "attenuation" ) )
	{
		pev->armortype = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CStompShooter::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Vector vecStart = pev->origin;
	Vector vecEnd;
	if (FStringNull(pev->target))
	{
		UTIL_MakeVectors( pev->angles );
		vecEnd = (gpGlobals->v_forward * 1024) + vecStart;
		TraceResult trace;
		UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &trace );
		vecEnd = trace.vecEndPos;
		vecEnd.z = vecStart.z;
	}
	else
	{
		CBaseEntity* pEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
		if (pEntity)
		{
			vecEnd = pEntity->pev->origin;
		}
		else
		{
			ALERT(at_error, "%s: can't find target '%s'", STRING(pev->classname), STRING(pev->target));
			return;
		}
	}

	CBaseEntity* pOwner = NULL;
	if (!FStringNull(pev->netname))
	{
		pOwner = UTIL_FindEntityByTargetname(NULL, STRING(pev->netname));
	}

	StompParams stompParams;
	stompParams.origin = vecStart;
	stompParams.end = vecEnd;
	stompParams.speed = pev->speed;
	stompParams.damage = gSkillData.gargantuaDmgStomp;
	stompParams.owner = pOwner ? pOwner->edict() : NULL;
	stompParams.soundAttenuation = pev->armortype;

	CStomp::StompCreate(stompParams);

	if (FBitSet(pev->spawnflags, SF_STOMPSHOOTER_FIRE_ONCE))
	{
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
}

CStomp *CStomp::StompCreate(const StompParams& stompParams, EntityOverrides entityOverrides)
{
	CStomp *pStomp = GetClassPtr( (CStomp *)NULL );
	SetStompParams(pStomp, stompParams);
	pStomp->AssignEntityOverrides(entityOverrides);
	pStomp->Spawn();
	return pStomp;
}

void CStomp::SetStompParams(CStomp* pStomp, const StompParams &stompParams)
{
	pStomp->pev->origin = stompParams.origin;
	const Vector dir = stompParams.end - stompParams.origin;
	pStomp->pev->scale = dir.Length();
	pStomp->pev->movedir = dir.Normalize();
	pStomp->pev->speed = stompParams.speed;
	pStomp->pev->dmg = stompParams.damage;
	pStomp->pev->owner = stompParams.owner;
	pStomp->pev->armortype = stompParams.soundAttenuation;
}

void CStomp::Spawn( void )
{
	Precache();
	pev->nextthink = gpGlobals->time;
	pev->classname = MyClassname();
	pev->dmgtime = gpGlobals->time;
	pev->effects |= EF_NODRAW;

	pev->framerate = 30;
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;

	SoundScriptParamOverride param;
	if (pev->armortype > 0)
	{
		param.OverrideAttenuationAbsolute(pev->armortype);
	}
	EmitSoundScript(StompSoundScript(), param);
}

void CStomp::Precache()
{
	m_stompVisual = RegisterVisual(stompVisual);
	RegisterAndPrecacheSoundScript(stompSoundScript);
}

#define	STOMP_INTERVAL		0.025f

void CStomp::Think( void )
{
	TraceResult tr;

	pev->nextthink = gpGlobals->time + 0.1f;

	// Do damage for this frame
	Vector vecStart = pev->origin;
	vecStart.z += 30;
	Vector vecEnd = vecStart + ( pev->movedir * pev->speed * STOMP_FRAMETIME );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if( tr.pHit && tr.pHit != pev->owner )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if (pEntity && pEntity->pev->takedamage)
		{
			entvars_t *pevOwner = pev;
			if( pev->owner )
				pevOwner = VARS( pev->owner );
			pEntity->TakeDamage( pev, pevOwner, DamageInfo(pev->dmg, DMG_SONIC) );
		}
	}

	// Accelerate the effect
	pev->speed = pev->speed + ( STOMP_FRAMETIME ) * pev->framerate;
	pev->framerate = pev->framerate + ( STOMP_FRAMETIME ) * 1500.0f;

	float stompInterval = STOMP_INTERVAL;
	int numOfSprites = 2;
	int maxNumOfSprites = 8;
	float spriteScale = RandomizeNumberFromRange(m_stompVisual->scale);
	if (g_pGameRules->IsMultiplayer())
	{
		stompInterval = STOMP_INTERVAL*2;
		spriteScale *= 1.8;
		maxNumOfSprites = 6;
	}
	const int freeEnts = gpGlobals->maxEntities - NUMBER_OF_ENTITIES();
	maxNumOfSprites = Q_min(maxNumOfSprites, freeEnts);

	// TODO: make it into clint side effects?
	// Move and spawn trails
	while( gpGlobals->time - pev->dmgtime > stompInterval )
	{
		pev->origin = pev->origin + pev->movedir * pev->speed * stompInterval;
		for( int i = 0; i < numOfSprites && maxNumOfSprites > 0; i++ )
		{
			maxNumOfSprites--;
			CSprite *pSprite = CreateSpriteFromVisual(m_stompVisual, pev->origin);
			if( pSprite )
			{
				UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 500 ), ignore_monsters, edict(), &tr );
				pSprite->pev->origin = tr.vecEndPos;
				pSprite->pev->velocity = Vector( RANDOM_FLOAT( -200.0f, 200.0f ), RANDOM_FLOAT( -200.0f, 200.0f ), 175 );
				// pSprite->AnimateAndDie( RANDOM_FLOAT( 8.0f, 12.0f ) );
				pSprite->pev->nextthink = gpGlobals->time + 0.3f;
				pSprite->SetThink( &CBaseEntity::SUB_Remove );
				pSprite->SetScale(spriteScale);
			}
		}
		pev->dmgtime += stompInterval;

		// Scale has the "life" of this effect
		pev->scale -= stompInterval * pev->speed;
		if( pev->scale <= 0 )
		{
			// Life has run out
			UTIL_Remove( this );
			StopSoundScript(StompSoundScript());
		}
	}
}

void CBabyStomp::Precache()
{
	m_stompVisual = RegisterVisual(stompVisual);

	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchAbsolute(65);
	RegisterAndPrecacheSoundScript(stompSoundScript, CStomp::stompSoundScript, paramOverride);
}

CBabyStomp* CBabyStomp::StompCreate(const StompParams& stompParams, EntityOverrides entityOverrides)
{
	CBabyStomp *pStomp = GetClassPtr( (CBabyStomp *)NULL );
	SetStompParams(pStomp, stompParams);
	pStomp->AssignEntityOverrides(entityOverrides);
	pStomp->Spawn();
	return pStomp;
}

void StreakSplash( const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_STREAK_SPLASH );
		WRITE_VECTOR( origin );		// origin
		WRITE_VECTOR( direction );	// direction
		WRITE_BYTE( color );	// Streak color 6
		WRITE_SHORT( count );	// count
		WRITE_SHORT( speed );
		WRITE_SHORT( velocityRange );	// Random velocity modifier
	MESSAGE_END();
}

class CGargantua : public CFollowingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void UpdateOnRemove();
	void SetYawSpeed( void );
	int DefaultClassify( void );
	const char* DefaultGibModel() { return GARG_GIB_MODEL; }
	const char* DefaultDisplayName() { return "Gargantua"; }
	DamageInfo DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo) override;
	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	bool CheckMeleeAttack1( float flDot, float flDist ) override;		// Swipe
	bool CheckMeleeAttack2( float flDot, float flDist ) override;		// Flames
	bool CheckRangeAttack1( float flDot, float flDist ) override;		// Stomp attack
	bool WantsToGetCloseToEnemy() override {
		return true;
	}
	void SetObjectCollisionBox( void )
	{
		SetMyObjectCollisionBox(Vector( -80, -80, 0 ), Vector( 80, 80, 214 ));
	}

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	void PlayUseSentence();
	void PlayUnUseSentence();

	void PrescheduleThink( void );

	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	void OnDying();
	void DeathEffect( void );

	void EyeOff( void );
	void EyeOn( int level );
	void EyeUpdate( void );
	void Leap( void );
	void StompAttack( void );
	void FlameCreate( void );
	void FlameUpdate( void );
	void FlameControls( float angleX, float angleY );
	void FlameDestroy( void );
	inline bool FlameIsOn( void ) { return m_pFlame[0] != NULL; }

	void FlameDamage( Vector vecStart, Vector vecEnd, entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, int iClassIgnore );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

	virtual int DefaultSizeForGrapple() { return GRAPPLE_LARGE; }
	Vector DefaultMinHullSize() {
		return Vector( -32.0f, -32.0f, 0.0f );
	}
	Vector DefaultMaxHullSize() {
		return Vector( 32.0f, 32.0f, 64.0f );
	}

	int m_GargGibModel;

protected:
	virtual float DefaultHealth();
	virtual float FireAttackDamage();
	virtual float StompAttackDamage();
	virtual const char* DefaultModel();
	virtual void FootEffect();
	virtual void MakeStomp(const StompParams& stompParams);
	virtual void StompEffect();
	virtual float FlameLength();
	virtual const Visual* GetWideFlameVisual();
	virtual const Visual* GetNarrowFlameVisual();
	virtual void BreatheSound();
	virtual void AttackSound();
	virtual float FlameTimeDivider();
	virtual Vector StompAttackStartVec();

	static constexpr const char* attackHitSoundScript = "Garg.AttackHit";
	static constexpr const char* attackMissSoundScript = "Garg.AttackMiss";
	static const NamedSoundScript flameOnSoundScript;
	static const NamedSoundScript flameRunSoundScript;
	static const NamedSoundScript flameOffSoundScript;
	static const NamedSoundScript footSoundScript;
	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript stompSoundScript;
	static const NamedSoundScript breathSoundScript;

	static const char *pRicSounds[];

	static const NamedVisual eyeVisual;
	static const NamedVisual flameVisual;
	static const NamedVisual bigFlameVisual;
	static const NamedVisual smallFlameVisual;
	static const NamedVisual flameLightVisual;

	virtual const char* AttackHitSound() {
		return attackHitSoundScript;
	}
	virtual const char* AttackMissSound() {
		return attackMissSoundScript;
	}
	virtual void FlameOnSound() {
		EmitSoundScript(flameOnSoundScript);
	}
	virtual void FlameRunSound() {
		EmitSoundScript(flameRunSoundScript);
	}
	virtual void FlameOffSound() {
		EmitSoundScript(flameOffSoundScript);
	}

	CSprite		*m_pEyeGlow;		// Glow around the eyes
	CBeam		*m_pFlame[4];		// Flame beams

	int		m_eyeBrightness;	// Brightness target
	float		m_seeTime;		// Time to attack (when I see the enemy, I set this)
	float		m_flameTime;		// Time of next flame attack
	float		m_painSoundTime;	// Time of next pain sound
	float		m_streakTime;		// streak timer (don't send too many)
	float		m_flameX;		// Flame thrower aim
	float		m_flameY;			
	float m_breatheTime;

	const Visual* m_eyeVisual; // this is accessed quite often so cache it
	const Visual* m_flameVisual; // this is accessed quite often so cache it
};

LINK_ENTITY_TO_CLASS( monster_gargantua, CGargantua )

TYPEDESCRIPTION	CGargantua::m_SaveData[] =
{
	DEFINE_FIELD( CGargantua, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CGargantua, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CGargantua, m_seeTime, FIELD_TIME ),
	DEFINE_FIELD( CGargantua, m_flameTime, FIELD_TIME ),
	DEFINE_FIELD( CGargantua, m_streakTime, FIELD_TIME ),
	DEFINE_FIELD( CGargantua, m_painSoundTime, FIELD_TIME ),
	DEFINE_ARRAY( CGargantua, m_pFlame, FIELD_CLASSPTR, 4 ),
	DEFINE_FIELD( CGargantua, m_flameX, FIELD_FLOAT ),
	DEFINE_FIELD( CGargantua, m_flameY, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGargantua, CFollowingMonster )

const NamedSoundScript CGargantua::flameOnSoundScript = {
	CHAN_BODY,
	{"garg/gar_flameon1.wav"},
	"Garg.BeamAttackOn"
};

const NamedSoundScript CGargantua::flameRunSoundScript = {
	CHAN_WEAPON,
	{"garg/gar_flamerun1.wav"},
	"Garg.BeamAttackRun"
};

const NamedSoundScript CGargantua::flameOffSoundScript = {
	CHAN_WEAPON,
	{"garg/gar_flameoff1.wav"},
	"Garg.BeamAttackOff"
};

const NamedSoundScript CGargantua::footSoundScript = {
	CHAN_BODY,
	{"garg/gar_step1.wav", "garg/gar_step2.wav"},
	1.0f,
	ATTN_GARG,
	IntRange(90, 110),
	"Garg.Footstep"
};

const NamedSoundScript CGargantua::idleSoundScript = {
	CHAN_VOICE,
	{"garg/gar_idle1.wav", "garg/gar_idle2.wav", "garg/gar_idle3.wav", "garg/gar_idle4.wav", "garg/gar_idle5.wav"},
	"Garg.Idle"
};

const NamedSoundScript CGargantua::alertSoundScript = {
	CHAN_VOICE,
	{"garg/gar_alert1.wav", "garg/gar_alert2.wav", "garg/gar_alert3.wav"},
	"Garg.Alert"
};

const NamedSoundScript CGargantua::painSoundScript = {
	CHAN_VOICE,
	{"garg/gar_pain1.wav", "garg/gar_pain2.wav", "garg/gar_pain3.wav"},
	1.0f,
	ATTN_GARG,
	"Garg.Pain"
};

const NamedSoundScript CGargantua::attackSoundScript = {
	CHAN_VOICE,
	{"garg/gar_attack1.wav", "garg/gar_attack2.wav", "garg/gar_attack3.wav"},
	1.0f,
	ATTN_GARG,
	"Garg.Attack"
};

const NamedSoundScript CGargantua::stompSoundScript = {
	CHAN_WEAPON,
	{"garg/gar_stomp1.wav"},
	1.0f,
	ATTN_GARG,
	IntRange(90, 110),
	"Garg.StompSound"
};

const NamedSoundScript CGargantua::breathSoundScript = {
	CHAN_VOICE,
	{"garg/gar_breathe1.wav", "garg/gar_breathe2.wav", "garg/gar_breathe3.wav"},
	1.0f,
	ATTN_GARG,
	IntRange(90, 110),
	"Garg.Breath"
};

const char *CGargantua::pRicSounds[] =
{
	"debris/metal4.wav",
	"debris/metal6.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
};

const NamedVisual CGargantua::eyeVisual = BuildVisual("Garg.Eye")
		.Model(GARG_EYE_SPRITE_NAME)
		.Scale(1.0f)
		.RenderColor(255, 255, 255)
		.Alpha(200)
		.RenderMode(kRenderGlow)
		.RenderFx(kRenderFxNoDissipation);

const NamedVisual CGargantua::flameVisual = BuildVisual("Garg.FlameBase")
		.Alpha(190)
		.BeamScrollRate(20);

const NamedVisual CGargantua::bigFlameVisual = BuildVisual("Garg.FlameWide")
		.Model(GARG_BEAM_SPRITE_NAME)
		.RenderColor(255, 130, 90)
		.BeamWidth(240)
		.BeamFlags(BEAM_FSHADEIN)
		.Mixin(&CGargantua::flameVisual);

const NamedVisual CGargantua::smallFlameVisual = BuildVisual("Garg.FlameNarrow")
		.Model(GARG_BEAM_SPRITE2)
		.RenderColor(0, 120, 255)
		.BeamWidth(140)
		.BeamFlags(BEAM_FSHADEIN)
		.Mixin(&CGargantua::flameVisual);

const NamedVisual CGargantua::flameLightVisual = BuildVisual("Garg.FlameLight")
		.RenderColor(255, 255, 255)
		.Life(0.2f)
		.Radius(IntRange(32, 48));

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

enum
{
	TASK_SOUND_ATTACK = LAST_FOLLOWINGMONSTER_TASK + 1,
	TASK_FLAME_SWEEP
};

Task_t tlGargFlame[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SOUND_ATTACK, (float)0 },
	// { TASK_PLAY_SEQUENCE, (float)ACT_SIGNAL1 },
	{ TASK_SET_ACTIVITY, (float)ACT_MELEE_ATTACK2 },
	{ TASK_FLAME_SWEEP, (float)4.5 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slGargFlame[] =
{
	{
		tlGargFlame,
		ARRAYSIZE( tlGargFlame ),
		0,
		0,
		"GargFlame"
	},
};

// primary melee attack
Task_t tlGargSwipe[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_MELEE_ATTACK1, (float)0 },
};

Schedule_t slGargSwipe[] =
{
	{
		tlGargSwipe,
		ARRAYSIZE( tlGargSwipe ),
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"GargSwipe"
	},
};

Task_t tlGargStomp[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slGargStomp[] =
{
	{
		tlGargStomp,
		ARRAYSIZE( tlGargStomp ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST,
		0,
		"GargStomp"
	},
};

DEFINE_CUSTOM_SCHEDULES( CGargantua )
{
	slGargFlame,
	slGargSwipe,
	slGargStomp,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGargantua, CFollowingMonster )

void CGargantua::EyeOn( int level )
{
	m_eyeBrightness = level;
}

void CGargantua::EyeOff( void )
{
	m_eyeBrightness = 0;
}

void CGargantua::EyeUpdate( void )
{
	if( m_pEyeGlow )
	{
		m_pEyeGlow->pev->renderamt = UTIL_Approach( m_eyeBrightness, m_pEyeGlow->pev->renderamt, m_eyeVisual->renderamt/8+1 );
		if( m_pEyeGlow->pev->renderamt == 0 )
			m_pEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_pEyeGlow->pev->effects &= ~EF_NODRAW;
		UTIL_SetOrigin( m_pEyeGlow->pev, pev->origin );
	}
}

void CGargantua::StompAttack( void )
{
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = StompAttackStartVec();
	Vector vecAim = ShootAtEnemy( vecStart );
	Vector vecEnd = (vecAim * 1024) + vecStart;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &trace );

	StompParams stompParams;
	stompParams.origin = vecStart;
	stompParams.end = trace.vecEndPos;
	stompParams.speed = 0;
	stompParams.damage = StompAttackDamage();
	stompParams.owner = edict();

	MakeStomp(stompParams);
	StompEffect();

	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,20), ignore_monsters, edict(), &trace );
	if( trace.flFraction < 1.0f )
		UTIL_DecalTrace( &trace, DECAL_GARGSTOMP1 );
}

void CGargantua::FlameCreate( void )
{
	int i;
	Vector posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );

	const Visual* bigFlameVisual = GetWideFlameVisual();
	const Visual* smallFlameVisual = GetNarrowFlameVisual();

	for( i = 0; i < 4; i++ )
	{
		if( i < 2 )
			m_pFlame[i] = CreateBeamFromVisual(bigFlameVisual);
		else
			m_pFlame[i] = CreateBeamFromVisual(smallFlameVisual);
		if( m_pFlame[i] )
		{
			int attach = i%2;
			// attachment is 0 based in GetAttachment
			GetAttachment( attach + 1, posGun, angleGun );

			Vector vecEnd = ( gpGlobals->v_forward * FlameLength() ) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->PointEntInit( trace.vecEndPos, entindex() );
			// attachment is 1 based in SetEndAttachment
			m_pFlame[i]->SetEndAttachment( attach + 2 );
			CSoundEnt::InsertSound( bits_SOUND_COMBAT, posGun, 384, 0.3 );
		}
	}
	FlameOnSound();
	FlameRunSound();
}

void CGargantua::FlameControls( float angleX, float angleY )
{
	if( angleY < -180 )
		angleY += 360;
	else if( angleY > 180 )
		angleY -= 360;

	if( angleY < -45 )
		angleY = -45;
	else if( angleY > 45 )
		angleY = 45;

	m_flameX = UTIL_ApproachAngle( angleX, m_flameX, 4 );
	m_flameY = UTIL_ApproachAngle( angleY, m_flameY, 8 );
	SetBoneController( 0, m_flameY );
	SetBoneController( 1, m_flameX );
}

void CGargantua::FlameUpdate( void )
{
	int		i;
	TraceResult	trace;
	Vector		vecStart, angleGun;
	bool		streaks = false;

	for( i = 0; i < 2; i++ )
	{
		if( m_pFlame[i] )
		{
			Vector vecAim = pev->angles;
			vecAim.x += m_flameX;
			vecAim.y += m_flameY;

			UTIL_MakeVectors( vecAim );

			GetAttachment( i + 1, vecStart, angleGun );
			Vector vecEnd = vecStart + ( gpGlobals->v_forward * FlameLength() ); //  - offset[i] * gpGlobals->v_right;

			UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->SetStartPos( trace.vecEndPos );
			m_pFlame[i+2]->SetStartPos( ( vecStart * 0.6f ) + ( trace.vecEndPos * 0.4f ) );

			if( trace.flFraction != 1.0f && gpGlobals->time > m_streakTime )
			{
				StreakSplash( trace.vecEndPos, trace.vecPlaneNormal, 6, 20, 50, 400 );
				streaks = true;
				UTIL_DecalTrace( &trace, DECAL_SMALLSCORCH1 + RANDOM_LONG( 0, 2 ) );
			}

			// RadiusDamage( trace.vecEndPos, pev, pev, gSkillData.gargantuaDmgFire, CLASS_ALIEN_MONSTER, DMG_BURN );
			FlameDamage( vecStart, trace.vecEndPos, pev, pev, DamageInfo{FireAttackDamage(), DMG_BURN}, Classify() );

			SendEntLight(entindex(), vecStart, m_flameVisual, i + 2);
		}
	}
	if( streaks )
		m_streakTime = gpGlobals->time;
}

void CGargantua::FlameDamage( Vector vecStart, Vector vecEnd, entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, int iClassIgnore )
{
	CBaseEntity	*pEntity = NULL;
	TraceResult	tr;
	Vector		vecSpot;

	Vector vecMid = ( vecStart + vecEnd ) * 0.5f;

	float searchRadius = ( vecStart - vecMid).Length();

	Vector vecAim = ( vecEnd - vecStart ).Normalize();

	// iterate on all entities in the vicinity.
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecMid, searchRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			vecSpot = pEntity->BodyTarget( vecMid );

			float dist = DotProduct( vecAim, vecSpot - vecMid );
			if( dist > searchRadius )
				dist = searchRadius;
			else if( dist < -searchRadius )
				dist = searchRadius;

			Vector vecSrc = vecMid + dist * vecAim;

			UTIL_TraceLine( vecSrc, vecSpot, dont_ignore_monsters, ENT( pev ), &tr );

			if( tr.flFraction == 1.0f || tr.pHit == pEntity->edict() )
			{
				// the explosion can 'see' this entity, so hurt them!
				// decrease damage for an ent that's farther from the flame.
				dist = ( vecSrc - tr.vecEndPos ).Length();

				DamageInfo dmgInfo = damageInfo;
				if( dist > 64.0f )
				{
					dmgInfo.damage = damageInfo.damage - ( dist - 64.0f ) * 0.4f;
					if( dmgInfo.damage <= 0 )
						continue;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if( tr.flFraction != 1.0f )
				{
					pEntity->ApplyTraceAttack( pevInflictor, pevAttacker, dmgInfo, ( tr.vecEndPos - vecSrc ).Normalize(), &tr );
				}
				else
				{
					pEntity->TakeDamage( pevInflictor, pevAttacker, dmgInfo );
				}
			}
		}
	}
}

void CGargantua::FlameDestroy( void )
{
	for( int i = 0; i < 4; i++ )
	{
		if( m_pFlame[i] )
		{
			UTIL_Remove( m_pFlame[i] );
			m_pFlame[i] = NULL;
		}
	}
}

void CGargantua::PrescheduleThink( void )
{
	if( !HasConditions( bits_COND_SEE_ENEMY ) )
	{
		m_seeTime = gpGlobals->time + 5.0f;
		EyeOff();
	}
	else
		EyeOn( m_eyeVisual->renderamt );

	EyeUpdate();
	CFollowingMonster::PrescheduleThink();
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CGargantua::DefaultClassify( void )
{
	return CLASS_GARGANTUA;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGargantua::SetYawSpeed( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 60;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_WALK:
	case ACT_RUN:
		ys = 60;
		break;
	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CGargantua::Spawn()
{
	Precache();

	SetMyModel( DefaultModel() );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( DefaultHealth() );
	//pev->view_ofs		= Vector ( 0, 0, 96 );// taken from mdl file
	SetMyFieldOfView(-0.2f);// width of forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	SetMySquadCapabilities();
	SetMyCanOpenDoors(false);

	FollowingMonsterInit();

	m_pEyeGlow = CreateSpriteFromVisual(m_eyeVisual, pev->origin);
	if (m_pEyeGlow)
	{
		m_pEyeGlow->SetAttachment( edict(), 1 );
		m_pEyeGlow->SetBrightness(0); // start with eye off
	}
	EyeOff();
	m_seeTime = gpGlobals->time + 5;
	m_flameTime = gpGlobals->time + 2;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGargantua::Precache()
{
	PrecacheMyModel( DefaultModel() );
	m_GargGibModel = PrecacheMyGibModel(DefaultGibModel());

	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchAbsolute(IntRange(50, 65));
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(flameOnSoundScript);
	RegisterAndPrecacheSoundScript(flameRunSoundScript);
	RegisterAndPrecacheSoundScript(flameOffSoundScript);
	RegisterAndPrecacheSoundScript(footSoundScript);
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(stompSoundScript);
	RegisterAndPrecacheSoundScript(breathSoundScript);

	PRECACHE_SOUND_ARRAY( pRicSounds );

	m_eyeVisual = RegisterVisual(eyeVisual);
	RegisterVisual(bigFlameVisual);
	RegisterVisual(smallFlameVisual);
	m_flameVisual = RegisterVisual(flameLightVisual);

	UTIL_PrecacheOther("garg_stomp", GetProjectileOverrides());
}

void CGargantua::UpdateOnRemove()
{
	if( m_pEyeGlow )
	{
		UTIL_Remove( m_pEyeGlow );
		m_pEyeGlow = 0;
	}

	FlameDestroy();
	CFollowingMonster::UpdateOnRemove();
}

DamageInfo CGargantua::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	DamageInfo damageInfo = inputDamageInfo;
	damageInfo.type &= GARG_DAMAGE;

	if( damageInfo.type == 0 )
	{
		if( pev->dmgtime != gpGlobals->time || (RANDOM_LONG( 0, 100 ) < 20 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 0.5 ,1.5 ) );
			pev->dmgtime = gpGlobals->time;
			//if ( RANDOM_LONG( 0, 100 ) < 25 )
			//	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, pRicSounds[RANDOM_LONG( 0, ARRAYSIZE( pRicSounds ) - 1 )], 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		damageInfo.damage = 0;
	}

	return damageInfo;
}

void CGargantua::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	if( !IsAlive() )
	{
		CFollowingMonster::TraceAttack( pevInflictor, pevAttacker, damageInfo, vecDir, ptr );
		return;
	}

	//TODO: adjust for custom trace attack rules
	// UNDONE: Hit group specific damage?
	if( damageInfo.type & GARG_DAMAGE )
	{
		if( m_painSoundTime < gpGlobals->time )
		{
			EmitSoundScript(painSoundScript);
			m_painSoundTime = gpGlobals->time + RANDOM_FLOAT( 2.5, 4 );
		}
	}

	CFollowingMonster::TraceAttack( pevInflictor, pevAttacker, damageInfo, vecDir, ptr );
}

DamageInfo CGargantua::DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo)
{
	DamageInfo damageInfo = inputDamageInfo;

	if( IsAlive() )
	{
		if( !( damageInfo.type & GARG_DAMAGE ) )
			damageInfo.damage *= 0.01f;
		if( damageInfo.type & DMG_BLAST )
			SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	return damageInfo;
}

void CGargantua::DeathEffect( void )
{
	int i;
	UTIL_MakeVectors( pev->angles );
	Vector deathPos = pev->origin + gpGlobals->v_forward * 100;

	// Create a spiral of streaks
	CSpiral::Create( deathPos, ( pev->absmax.z - pev->absmin.z ) * 0.6f, 125, 1.5 );

	Vector position = pev->origin;
	position.z += 32;
	for( i = 0; i < 7; i+=2 )
	{
		SpawnExplosion( position, 70, ( i * 0.3f ), 60 + ( i * 20 ) );
		position.z += 15;
	}

	CBaseEntity *pSmoker = CBaseEntity::Create( "env_smoker", pev->origin, g_vecZero, NULL );
	pSmoker->pev->health = 1;	// 1 smoke balls
	pSmoker->pev->scale = 46;	// 4.6X normal size
	pSmoker->pev->dmg = 0;		// 0 radial distribution
	pSmoker->pev->nextthink = gpGlobals->time + 2.5f;	// Start in 2.5 seconds
}

KilledResult CGargantua::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	return CFollowingMonster::Killed( pevInflictor, pevAttacker, GIB_NEVER );
}

void CGargantua::OnDying()
{
	EyeOff();
	UTIL_Remove( m_pEyeGlow );
	m_pEyeGlow = NULL;
	CFollowingMonster::OnDying();
}

//=========================================================
// CheckMeleeAttack1
// Garg swipe attack
// 
//=========================================================
bool CGargantua::CheckMeleeAttack1( float flDot, float flDist )
{
	//ALERT( at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist );
	CheckMeleeAttackParams params;
	params.distance = GARG_ATTACKDIST;
	return CheckMeleeAttackImpl(flDot, flDist, params, false);
}

// Flame thrower madness!
bool CGargantua::CheckMeleeAttack2( float flDot, float flDist )
{
	//ALERT( at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist );

	if( gpGlobals->time > m_flameTime )
	{
		float meleeAttack1Distance = GARG_ATTACKDIST;
		const EntTemplate* entTemplate = GetMyEntTemplate();
		if (entTemplate)
		{
			EntTemplate::CheckMeleeAttack check = entTemplate->GetCheckMeleeAttack1();
			if (check.distance)
				meleeAttack1Distance = *check.distance;
		}

		if( flDist > meleeAttack1Distance )
		{
			CheckMeleeAttackParams params;
			params.distance = FlameLength();
			params.dot = 0.8f;
			return CheckMeleeAttackImpl(flDot, flDist, params, true);
		}
	}
	return false;
}

//=========================================================
// CheckRangeAttack1
// flDot is the cos of the angle of the cone within which
// the attack can occur.
//=========================================================
//
// Stomp attack
//
//=========================================================
bool CGargantua::CheckRangeAttack1( float flDot, float flDist )
{
	if( gpGlobals->time > m_seeTime )
	{
		if( flDot >= 0.7f && flDist > GARG_ATTACKDIST )
		{
				return true;
		}
	}
	return false;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGargantua::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case GARG_AE_SLASH_LEFT:
	{
		TraceHullAttackParams params;
		params.punchAngle = Vector(-30, -30, 30);
		params.knockRight = -100.0f;
		params.distance = GARG_ATTACKDIST + 10.0f;
		params.verticalDistance = params.distance * -0.3f;
		params.height = 64;
		params.damageInfo.damage = gSkillData.gargantuaDmgSlash;
		params.useAimVectors = false;
		params.hitSoundScript = AttackHitSound();
		params.missSoundScript = AttackMissSound();
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
		break;
	case GARG_AE_RIGHT_FOOT:
	case GARG_AE_LEFT_FOOT:
		FootEffect();
		break;
	case GARG_AE_STOMP:
		StompAttack();
		m_seeTime = gpGlobals->time + 12.0f;
		break;
	case GARG_AE_BREATHE:
		BreatheSound();
		break;
	default:
		CFollowingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

Schedule_t *CGargantua::GetSchedule()
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
	{
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
	}
		break;
	default:
		break;
	}
	return CFollowingMonster::GetSchedule();
}

Schedule_t *CGargantua::GetScheduleOfType( int Type )
{
	// HACKHACK - turn off the flames if they are on and garg goes scripted / dead
	if( FlameIsOn() ) {
		FlameOffSound();
		FlameDestroy();
	}

	switch( Type )
	{
		case SCHED_MELEE_ATTACK2:
			return slGargFlame;
		case SCHED_MELEE_ATTACK1:
			return slGargSwipe;
		case SCHED_RANGE_ATTACK1:
			return slGargStomp;
		default:
			break;
	}

	return CFollowingMonster::GetScheduleOfType( Type );
}

void CGargantua::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FLAME_SWEEP:
		FlameCreate();
		m_flWaitFinished = gpGlobals->time + pTask->flData / FlameTimeDivider();
		m_flameTime = gpGlobals->time + 6.0f;
		m_flameX = 0;
		m_flameY = 0;
		break;
	case TASK_SOUND_ATTACK:
		if( RANDOM_LONG( 0, 100 ) < 30 )
			AttackSound();
		TaskComplete();
		break;
	// allow a scripted_action to make gargantua shoot flames.
	case TASK_PLAY_SCRIPT:
		if ( m_pCine->IsAction() && m_pCine->m_fAction == SCRIPT_ACT_MELEE_ATTACK2)
		{
			FlameCreate();
			m_flWaitFinished = gpGlobals->time + 4.5f;
			m_flameTime = gpGlobals->time + 6.0f;
			m_flameX = 0;
			m_flameY = 0;
		}
		else
			CBaseMonster::StartTask( pTask );
		break;
	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + 1.6f;
		DeathEffect();
		// FALL THROUGH
	default: 
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CGargantua::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_DIE:
		if( gpGlobals->time > m_flWaitFinished )
		{
			pev->renderfx = kRenderFxExplode;
			pev->rendercolor.x = 255;
			pev->rendercolor.y = 0;
			pev->rendercolor.z = 0;
			StopAnimation();
			pev->nextthink = gpGlobals->time + 0.15f;
			SetThink( &CBaseEntity::SUB_Remove );
			int i;
			int parts = MODEL_FRAMES( m_GargGibModel );
			const Visual* gibVisual = MyGibVisual();
			const char* gibModel = GibModel();
			for( i = 0; i < 10; i++ )
			{
				CGib *pGib = GetClassPtr( (CGib *)NULL );

				pGib->Spawn( gibModel, gibVisual );

				int bodyPart = 0;
				if( parts > 1 )
					bodyPart = RANDOM_LONG( 0, parts - 1 );

				pGib->pev->body = bodyPart;
				pGib->m_bloodColor = m_bloodColor;
				pGib->m_material = matNone;
				pGib->pev->origin = pev->origin;
				pGib->pev->velocity = UTIL_RandomBloodVector() * RANDOM_FLOAT( 300, 500 );
				pGib->pev->nextthink = gpGlobals->time + 1.25f;
				pGib->SetThink( &CBaseEntity::SUB_FadeOut );
			}
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL );

				// position
				WRITE_VECTOR( pev->origin );

				// size
				WRITE_COORD( 200 );
				WRITE_COORD( 200 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_VECTOR( g_vecZero );

				// randomization
				WRITE_BYTE( 200 ); 

				// Model
				WRITE_SHORT( m_GargGibModel );	//model id#

				// # of shards
				WRITE_BYTE( 50 );

				// duration
				WRITE_BYTE( 20 );// 3.0 seconds

				// flags

				WRITE_BYTE( BREAK_FLESH );
			MESSAGE_END();

			return;
		}
		else
			CFollowingMonster::RunTask( pTask );
		break;
	case TASK_PLAY_SCRIPT:
		if (m_pCine->IsAction() && m_pCine->m_fAction == SCRIPT_ACT_MELEE_ATTACK2)
		{
			if (m_fSequenceFinished)
			{
				if (m_pCine->m_iRepeatsLeft > 0)
					CBaseMonster::RunTask( pTask );
				else
				{
					FlameOffSound();
					FlameDestroy();
					FlameControls( 0, 0 );
					SetBoneController( 0, 0 );
					SetBoneController( 1, 0 );
					m_pCine->SequenceDone( this );
				}
				break;
			}
			//if not finished, drop through into task_flame_sweep!
		}
		else
		{
			CBaseMonster::RunTask( pTask );
			break;
		}
	case TASK_FLAME_SWEEP:
		if( gpGlobals->time > m_flWaitFinished )
		{
			FlameOffSound();
			FlameDestroy();
			TaskComplete();
			FlameControls( 0, 0 );
			SetBoneController( 0, 0 );
			SetBoneController( 1, 0 );
		}
		else
		{
			bool cancel = false;

			Vector angles = g_vecZero;

			FlameUpdate();

			Vector org = pev->origin;
			org.z += 64;
			Vector dir = g_vecZero;

			if (m_pCine) // LRC- are we obeying a scripted_action?
			{
				if (m_hTargetEnt != 0 && m_hTargetEnt.Get() != m_hMoveGoalEnt.Get())
				{
					dir = m_hTargetEnt->BodyTarget( org ) - org;
				}
				else
				{
					UTIL_MakeVectors( pev->angles );
					dir = gpGlobals->v_forward;
				}
			}
			else
			{
				CBaseEntity *pEnemy = m_hEnemy;
				if (pEnemy)
				{
					dir = pEnemy->BodyTarget( org ) - org;
				}
			}
			if( dir != g_vecZero )
			{
				angles = UTIL_VecToAngles( dir );
				angles.x = -angles.x;
				angles.y -= pev->angles.y;
				if( dir.Length() > 400 )
					cancel = true;
			}
			if( fabs(angles.y) > 60 )
				cancel = true;

			if( cancel )
			{
				m_flWaitFinished -= 0.5f;
				m_flameTime -= 0.5f;
			}
			// FlameControls( angles.x + 2.0f * sin( gpGlobals->time * 8.0f ), angles.y + 28.0f * sin( gpGlobals->time * 8.5f ) );
			FlameControls( angles.x, angles.y );
		}
		break;
	default:
		CFollowingMonster::RunTask( pTask );
		break;
	}
}

float CGargantua::DefaultHealth()
{
	return gSkillData.gargantuaHealth;
}

float CGargantua::FireAttackDamage()
{
	return gSkillData.gargantuaDmgFire;
}

float CGargantua::StompAttackDamage()
{
	return gSkillData.gargantuaDmgStomp;
}

const char* CGargantua::DefaultModel()
{
	return "models/garg.mdl";
}

void CGargantua::FootEffect()
{
	UTIL_ScreenShake( pev->origin, 4.0, 3.0, 1.0, 750 );
	EmitSoundScript(footSoundScript);
}

void CGargantua::MakeStomp(const StompParams& stompParams)
{
	CStomp::StompCreate(stompParams, GetProjectileOverrides());
}

void CGargantua::StompEffect()
{
	UTIL_ScreenShake( pev->origin, 12.0, 100.0, 2.0, 1000 );
	EmitSoundScript(stompSoundScript);
}

float CGargantua::FlameLength()
{
	return GARG_FLAME_LENGTH;
}

const Visual* CGargantua::GetWideFlameVisual()
{
	return GetVisual(bigFlameVisual);
}

const Visual* CGargantua::GetNarrowFlameVisual()
{
	return GetVisual(smallFlameVisual);
}

void CGargantua::BreatheSound()
{
	if (m_breatheTime <= gpGlobals->time)
		EmitSoundScript(breathSoundScript);
}

void CGargantua::AttackSound()
{
	EmitSoundScript(attackSoundScript);
}

float CGargantua::FlameTimeDivider()
{
	return 1.0;
}

Vector CGargantua::StompAttackStartVec()
{
	return pev->origin + Vector(0,0,60) + 35 * gpGlobals->v_forward;
}

void CGargantua::PlayUseSentence()
{
	m_breatheTime = gpGlobals->time + 1.5;
	EmitSoundScript(idleSoundScript);
}

void CGargantua::PlayUnUseSentence()
{
	m_breatheTime = gpGlobals->time + 1.5;
	EmitSoundScript(alertSoundScript);
}

#define SF_SMOKER_ACTIVE 1
#define SF_SMOKER_REPEATABLE 4
#define SF_SMOKER_DIRECTIONAL 8
#define SF_SMOKER_FADE 16
#define SF_SMOKER_MAXHEALTH_SET (1 << 24)

enum
{
	SMOKER_SPRITE_SCALE_DECILE = 0,
	SMOKER_SPRITE_SCALE_NORMAL = 1
};

class CSmoker : public CBaseEntity
{
public:
	void Precache();
	void Spawn( void );
	void KeyValue(KeyValueData* pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Think( void );

	bool IsActive() {
		return FBitSet(pev->spawnflags, SF_SMOKER_ACTIVE);
	}
	void SetActive(bool active)
	{
		if (active)
			SetBits(pev->spawnflags, SF_SMOKER_ACTIVE);
		else
			ClearBits(pev->spawnflags, SF_SMOKER_ACTIVE);
	}

	string_t m_iszPosition;
	string_t m_iszDirection;
	EHANDLE m_hActivator;

	int smokeIndex;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( env_smoker, CSmoker )

TYPEDESCRIPTION	CSmoker::m_SaveData[] =
{
	DEFINE_FIELD( CSmoker, m_iszPosition, FIELD_STRING ),
	DEFINE_FIELD( CSmoker, m_iszDirection, FIELD_STRING ),
	DEFINE_FIELD( CSmoker, m_hActivator, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CSmoker, CBaseEntity )

void CSmoker::Precache()
{
	if (!FStringNull(pev->model))
		smokeIndex = PRECACHE_MODEL(STRING(pev->model));
}

void CSmoker::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time;
	pev->solid = SOLID_NOT;
	UTIL_SetSize(pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	SetMovedir(pev);

	if (pev->scale <= 0.0f)
		pev->scale = 10;
	if (pev->framerate <= 0.0f)
		pev->framerate = 11.0f;

	if (pev->dmg_take <= 0.0f)
		pev->dmg_take = 0.1f;
	if (pev->dmg_save <= 0.0f)
		pev->dmg_save = 0.2f;

	pev->max_health = pev->health;

	if (FStringNull(pev->targetname))
		SetActive(true);

	if (IsActive())
		pev->nextthink = gpGlobals->time + 0.1f;
}

void CSmoker::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "scale_speed"))
	{
		pev->armorvalue = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "scale_unit_type"))
	{
		pev->impulse = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "position"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "direction"))
	{
		m_iszDirection = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CSmoker::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	const bool active = IsActive();
	if (ShouldToggle(useType, active))
	{
		if (active)
		{
			pev->nextthink = -1;
			SetActive(false);
		}
		else
		{
			pev->nextthink = gpGlobals->time;
			SetActive(true);
		}
		m_hActivator = pActivator;
	}
}

void CSmoker::Think( void )
{
	extern int gmsgSmoke;

	// Support for CBaseEntity::Create followed by pev->health setting
	if (!FBitSet(pev->spawnflags, SF_SMOKER_MAXHEALTH_SET))
	{
		pev->max_health = pev->health;
		SetBits(pev->spawnflags, SF_SMOKER_MAXHEALTH_SET);
	}

	if (!IsActive())
		return;

	const int minFramerate = Q_max(pev->framerate - 1, 1);
	const int maxFramerate = pev->framerate + 1;

	bool isPosValid = true;
	Vector position = pev->origin;

	if (!FStringNull(m_iszPosition))
	{
		isPosValid = TryCalcLocus_Position(this, m_hActivator, STRING(m_iszPosition), position);
	}

	bool isDirValid = true;
	bool directed = FBitSet(pev->spawnflags, SF_SMOKER_DIRECTIONAL);
	Vector direction;

	if (!FStringNull(m_iszDirection))
	{
		directed = true;
		isDirValid = TryCalcLocus_Velocity(this, m_hActivator, STRING(m_iszDirection), direction);
		if (isDirValid)
			direction = direction.Normalize();
	}
	else if (!FStringNull(pev->target))
	{
		directed = true;
		Vector targetPos;
		isDirValid = TryCalcLocus_Position(this, m_hActivator, STRING(pev->target), targetPos);
		if (isDirValid)
			direction = (targetPos - position).Normalize();
	}
	else if (directed)
	{
		direction = pev->movedir;
	}

	if (isDirValid && isPosValid)
	{
		int flags = 0;
		if (directed)
			flags |= SMOKER_FLAG_DIRECTED;
		if (FBitSet(pev->spawnflags, SF_SMOKER_FADE))
			flags |= SMOKER_FLAG_FADE_SPRITE;
		if (pev->impulse == SMOKER_SPRITE_SCALE_NORMAL)
			flags |= SMOKER_FLAG_SCALE_VALUE_IS_NORMAL;

		MESSAGE_BEGIN( MSG_PVS, gmsgSmoke, position );
			WRITE_BYTE( flags );
			WRITE_COORD( position.x + RANDOM_FLOAT( -pev->dmg, pev->dmg ) );
			WRITE_COORD( position.y + RANDOM_FLOAT( -pev->dmg, pev->dmg ) );
			WRITE_COORD( position.z);
			WRITE_SHORT( smokeIndex ? smokeIndex : g_sModelIndexSmoke );
			WRITE_COORD( RANDOM_FLOAT(pev->scale, pev->scale * 1.1f ) );
			WRITE_BYTE( RANDOM_LONG( minFramerate, maxFramerate ) ); // framerate
			WRITE_SHORT( pev->speed );
			WRITE_SHORT( pev->frags );
			WRITE_BYTE( pev->rendermode );
			WRITE_BYTE( pev->renderamt );
			WRITE_COLOR( pev->rendercolor );
			WRITE_SHORT( pev->armorvalue * 10 );
		if (directed)
		{
			WRITE_VECTOR( direction );
		}
		MESSAGE_END();
	}

	if (pev->max_health > 0)
		pev->health--;
	if( pev->max_health <= 0 || pev->health > 0 )
	{
		const float minDelay = pev->dmg_take;
		const float maxDelay = Q_max(pev->dmg_take, pev->dmg_save);

		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( minDelay, maxDelay );
	}
	else
	{
		if (FBitSet(pev->spawnflags, SF_SMOKER_REPEATABLE))
		{
			pev->health = pev->max_health;
			SetActive(false);
		}
		else
			UTIL_Remove( this );
	}
}

void CSpiral::Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time;
	pev->solid = SOLID_NOT;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->angles = g_vecZero;
}

CSpiral *CSpiral::Create( const Vector &origin, float height, float radius, float duration )
{
	if( duration <= 0 )
		return NULL;

	CSpiral *pSpiral = GetClassPtr( (CSpiral *)NULL );
	pSpiral->Spawn();
	pSpiral->pev->dmgtime = pSpiral->pev->nextthink;
	pSpiral->pev->origin = origin;
	pSpiral->pev->scale = radius;
	pSpiral->pev->dmg = height;
	pSpiral->pev->speed = duration;
	pSpiral->pev->health = 0;
	pSpiral->pev->angles = g_vecZero;

	return pSpiral;
}

#define SPIRAL_INTERVAL		0.1f //025

void CSpiral::Think( void )
{
	float time = gpGlobals->time - pev->dmgtime;

	while( time > SPIRAL_INTERVAL )
	{
		Vector position = pev->origin;
		Vector direction = Vector(0,0,1);

		float fraction = 1.0f / pev->speed;

		float radius = ( pev->scale * pev->health ) * fraction;

		position.z += ( pev->health * pev->dmg ) * fraction;
		pev->angles.y = ( pev->health * 360 * 8 ) * fraction;
		UTIL_MakeVectors( pev->angles );
		position = position + gpGlobals->v_forward * radius;
		direction = ( direction + gpGlobals->v_forward ).Normalize();

		StreakSplash( position, Vector( 0, 0, 1 ), RANDOM_LONG( 8, 11 ), 20, RANDOM_LONG( 50, 150 ), 400 );

		// Jeez, how many counters should this take ? :)
		pev->dmgtime += SPIRAL_INTERVAL;
		pev->health += SPIRAL_INTERVAL;
		time -= SPIRAL_INTERVAL;
	}

	pev->nextthink = gpGlobals->time;

	if( pev->health >= pev->speed )
		UTIL_Remove( this );
}

// HACKHACK Cut and pasted from explode.cpp
void SpawnExplosion( Vector center, float randomRange, float time, int magnitude )
{
	KeyValueData	kvd;
	char		buf[128];

	center.x += RANDOM_FLOAT( -randomRange, randomRange );
	center.y += RANDOM_FLOAT( -randomRange, randomRange );

	CBaseEntity *pExplosion = CBaseEntity::Create( "env_explosion", center, g_vecZero, NULL );
	sprintf( buf, "%3d", magnitude );
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue( &kvd );
	pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->SetThink( &CBaseEntity::SUB_CallUseToggle );
	pExplosion->pev->nextthink = gpGlobals->time + time;
}

#if FEATURE_BABYGARG
class CBabyGargantua : public CGargantua
{
public:
	void Precache();
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("babygarg"); }
	void SetYawSpeed( void );
	const char* ReverseRelationshipModel() { return "models/babygargf.mdl"; }
	const char* DefaultDisplayName() { return "Baby Gargantua"; }
	const char* DefaultGibModel() { return CFollowingMonster::DefaultGibModel(); }
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void PlayUseSentence();
	void PlayUnUseSentence();
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void DeathSound();
	DamageInfo DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo) override {
		return inputDamageInfo;
	}
	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override {
		return inputDamageInfo;
	}
	void TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr) override;
	void SetObjectCollisionBox( void )
	{
		SetMyObjectCollisionBox(Vector( -32, -32, 0 ), Vector( 32, 32, 100 ));
	}
	Vector DefaultMinHullSize() { return Vector( -32.0f, -32.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 32.0f, 32.0f, 64.0f ); }

protected:
	float DefaultHealth();
	float FireAttackDamage();
	float StompAttackDamage();
	const char* DefaultModel();
	void FootEffect();
	void MakeStomp(const StompParams& stompParams);
	void StompEffect();
	float FlameLength();
	const Visual* GetWideFlameVisual();
	const Visual* GetNarrowFlameVisual();
	void BreatheSound();
	void AttackSound();
	float FlameTimeDivider();
	Vector StompAttackStartVec();

	static constexpr const char* attackHitSoundScript = "BabyGarg.AttackHit";
	static constexpr const char* attackMissSoundScript = "BabyGarg.AttackMiss";
	static const NamedSoundScript flameOnSoundScript;
	static const NamedSoundScript flameRunSoundScript;
	static const NamedSoundScript flameOffSoundScript;
	static const NamedSoundScript footSoundScript;
	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript stompSoundScript;
	static const NamedSoundScript breathSoundScript;

	static const NamedVisual eyeVisual;
	static const NamedVisual bigFlameVisual;
	static const NamedVisual smallFlameVisual;
	static const NamedVisual flameLightVisual;

	virtual const char* AttackHitSound() override {
		return attackHitSoundScript;
	}
	virtual const char* AttackMissSound() override {
		return attackMissSoundScript;
	}
	virtual void FlameOnSound() {
		EmitSoundScript(flameOnSoundScript);
	}
	virtual void FlameRunSound() {
		EmitSoundScript(flameRunSoundScript);
	}
	virtual void FlameOffSound() {
		EmitSoundScript(flameOffSoundScript);
	}
};

LINK_ENTITY_TO_CLASS( monster_babygarg, CBabyGargantua )

const NamedSoundScript CBabyGargantua::flameOnSoundScript = {
	CHAN_BODY,
	{"babygarg/gar_flameon1.wav"},
	"BabyGarg.BeamAttackOn"
};

const NamedSoundScript CBabyGargantua::flameRunSoundScript = {
	CHAN_WEAPON,
	{"babygarg/gar_flamerun1.wav"},
	"BabyGarg.BeamAttackRun"
};

const NamedSoundScript CBabyGargantua::flameOffSoundScript = {
	CHAN_WEAPON,
	{"babygarg/gar_flameoff1.wav"},
	"BabyGarg.BeamAttackOff"
};

const NamedSoundScript CBabyGargantua::footSoundScript = {
	CHAN_BODY,
	{"babygarg/gar_step1.wav", "babygarg/gar_step2.wav"},
	1.0f,
	ATTN_GARG,
	IntRange(90, 110),
	"BabyGarg.Footstep"
};

const NamedSoundScript CBabyGargantua::idleSoundScript = {
	CHAN_VOICE,
	{"babygarg/gar_idle1.wav", "babygarg/gar_idle2.wav", "babygarg/gar_idle3.wav", "babygarg/gar_idle4.wav", "babygarg/gar_idle5.wav"},
	"BabyGarg.Idle"
};

const NamedSoundScript CBabyGargantua::alertSoundScript = {
	CHAN_VOICE,
	{"babygarg/gar_alert1.wav", "babygarg/gar_alert2.wav", "babygarg/gar_alert3.wav"},
	"BabyGarg.Alert"
};

const NamedSoundScript CBabyGargantua::painSoundScript = {
	CHAN_VOICE,
	{"babygarg/gar_pain1.wav", "babygarg/gar_pain2.wav", "babygarg/gar_pain3.wav"},
	1.0f,
	ATTN_GARG,
	"BabyGarg.Pain"
};

const NamedSoundScript CBabyGargantua::dieSoundScript = {
	CHAN_VOICE,
	{"babygarg/gar_die1.wav", "babygarg/gar_die2.wav"},
	"BabyGarg.Die"
};

const NamedSoundScript CBabyGargantua::attackSoundScript = {
	CHAN_VOICE,
	{"babygarg/gar_attack1.wav", "babygarg/gar_attack2.wav", "babygarg/gar_attack3.wav"},
	1.0f,
	ATTN_GARG,
	"BabyGarg.Attack"
};

const NamedSoundScript CBabyGargantua::stompSoundScript = {
	CHAN_WEAPON,
	{"babygarg/gar_stomp1.wav"},
	1.0f,
	ATTN_GARG,
	IntRange(90, 110),
	"BabyGarg.StompSound"
};

const NamedSoundScript CBabyGargantua::breathSoundScript = {
	CHAN_VOICE,
	{"babygarg/gar_breathe1.wav", "babygarg/gar_breathe2.wav", "babygarg/gar_breathe3.wav"},
	1.0f,
	ATTN_GARG,
	IntRange(90, 110),
	"BabyGarg.Breath"
};

const NamedVisual CBabyGargantua::eyeVisual = BuildVisual("BabyGarg.Eye")
		.Model("sprites/flare3.spr")
		.Scale(0.5f)
		.RenderColor(225, 170, 80)
		.Alpha(255)
		.RenderMode(kRenderGlow)
		.RenderFx(kRenderFxNoDissipation);

const NamedVisual CBabyGargantua::bigFlameVisual = BuildVisual("BabyGarg.FlameWide")
		.BeamWidth(120)
		.Mixin(&CGargantua::bigFlameVisual);

const NamedVisual CBabyGargantua::smallFlameVisual = BuildVisual("BabyGarg.FlameNarrow")
		.BeamWidth(70)
		.Mixin(&CGargantua::smallFlameVisual);

const NamedVisual CBabyGargantua::flameLightVisual = BuildVisual("BabyGarg.FlameLight")
		.Mixin(&CGargantua::flameLightVisual);

void CBabyGargantua::Precache()
{
	PrecacheMyModel( DefaultModel() );
	PrecacheMyGibModel();

	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchAbsolute(IntRange(60, 75));
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(flameOnSoundScript);
	RegisterAndPrecacheSoundScript(flameRunSoundScript);
	RegisterAndPrecacheSoundScript(flameOffSoundScript);
	RegisterAndPrecacheSoundScript(footSoundScript);
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(stompSoundScript);
	RegisterAndPrecacheSoundScript(breathSoundScript);

	m_eyeVisual = RegisterVisual(eyeVisual);
	RegisterVisual(bigFlameVisual);
	RegisterVisual(smallFlameVisual);
	m_flameVisual = RegisterVisual(flameLightVisual);

	UTIL_PrecacheOther("babygarg_stomp", GetProjectileOverrides());
}

void CBabyGargantua::StartTask(Task_t *pTask)
{
	switch (pTask->iTask) {
	case TASK_DIE:
		CFollowingMonster::StartTask(pTask);
		break;
	default:
		CGargantua::StartTask(pTask);
		break;
	}
}

void CBabyGargantua::RunTask(Task_t *pTask)
{
	switch (pTask->iTask) {
	case TASK_DIE:
		CFollowingMonster::RunTask(pTask);
		break;
	default:
		CGargantua::RunTask(pTask);
		break;
	}
}

void CBabyGargantua::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch( pEvent->event )
	{
	case GARG_AE_SLASH_LEFT:
	{
		TraceHullAttackParams params;
		params.punchAngle = Vector(-20, -20, 20);
		params.knockRight = -80.0f;
		params.distance = GARG_ATTACKDIST + 10.0f;
		params.verticalDistance = params.distance * -0.3f;
		params.height = 64;
		params.damageInfo.damage = gSkillData.babygargantuaDmgSlash;
		params.useAimVectors = false;
		params.hitSoundScript = AttackHitSound();
		params.missSoundScript = AttackMissSound();
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
		break;
	case BABYGARG_AE_KICK:
	{
		TraceHullAttackParams params;
		params.punchAngle.x = 20;
		params.knockForward = 200.0f;
		params.knockUp = 100;
		params.distance = GARG_ATTACKDIST + 5.0f;
		params.verticalDistance = params.distance * -0.3f;
		params.height = 64;
		params.damageInfo.damage = gSkillData.babygargantuaDmgSlash;
		params.useAimVectors = false;
		params.hitSoundScript = AttackHitSound();
		params.missSoundScript = AttackMissSound();
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		PerformTraceHullAttack(params);
	}
		break;
	default:
		CGargantua::HandleAnimEvent( pEvent );
		break;
	}
}

void CBabyGargantua::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

float CBabyGargantua::DefaultHealth()
{
	return gSkillData.babygargantuaHealth;
}

float CBabyGargantua::FireAttackDamage()
{
	return gSkillData.babygargantuaDmgFire;
}

float CBabyGargantua::StompAttackDamage()
{
	return gSkillData.babygargantuaDmgStomp;
}

const char* CBabyGargantua::DefaultModel()
{
	return "models/babygarg.mdl";
}

void CBabyGargantua::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr)
{
	if( !IsFullyAlive() )
	{
		CFollowingMonster::TraceAttack( pevInflictor, pevAttacker, damageInfo, vecDir, ptr );
		return;
	}

	if( m_painSoundTime < gpGlobals->time )
	{
		EmitSoundScript(painSoundScript);
		m_painSoundTime = gpGlobals->time + RANDOM_FLOAT( 2.5, 4 );
	}

	CFollowingMonster::TraceAttack( pevInflictor, pevAttacker, damageInfo, vecDir, ptr );
}

void CBabyGargantua::FootEffect()
{
	UTIL_ScreenShake( pev->origin, 2.0, 3.0, 1.0, 400 );
	EmitSoundScript(footSoundScript);
}

void CBabyGargantua::MakeStomp(const StompParams& stompParams)
{
	CBabyStomp::StompCreate(stompParams, GetProjectileOverrides());
}

void CBabyGargantua::StompEffect()
{
	UTIL_ScreenShake( pev->origin, 6.0, 100.0, 1.5, 600 );
	EmitSoundScript(stompSoundScript);
}

float CBabyGargantua::FlameLength()
{
	return GARG_FLAME_LENGTH / 2;
}

const Visual* CBabyGargantua::GetWideFlameVisual()
{
	return GetVisual(bigFlameVisual);
}

const Visual* CBabyGargantua::GetNarrowFlameVisual()
{
	return GetVisual(smallFlameVisual);
}

void CBabyGargantua::BreatheSound()
{
	if (m_breatheTime <= gpGlobals->time)
		EmitSoundScript(breathSoundScript);
}

void CBabyGargantua::AttackSound()
{
	EmitSoundScript(attackSoundScript);
}

float CBabyGargantua::FlameTimeDivider()
{
	return 1.5;
}

Vector CBabyGargantua::StompAttackStartVec()
{
	return pev->origin + Vector(0,0,30) + 35 * gpGlobals->v_forward;
}

void CBabyGargantua::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 100;
		break;
	}

	pev->yaw_speed = ys;
}

void CBabyGargantua::PlayUseSentence()
{
	m_breatheTime = gpGlobals->time + 1.5;
	EmitSoundScript(idleSoundScript);
}

void CBabyGargantua::PlayUnUseSentence()
{
	m_breatheTime = gpGlobals->time + 1.5;
	EmitSoundScript(alertSoundScript);
}
#endif
