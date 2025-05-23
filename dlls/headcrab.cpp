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
// headcrab.cpp - tiny, jumpy alien parasite
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"mod_features.h"
#if FEATURE_SHOCKRIFLE
#include "player.h"
#include "weapon_ids.h"
#endif

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HC_AE_JUMPATTACK	( 2 )

Task_t tlHCRangeAttack1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WAIT_RANDOM, (float)0.5 },
};

Schedule_t slHCRangeAttack1[] =
{
	{
		tlHCRangeAttack1,
		ARRAYSIZE( tlHCRangeAttack1 ),
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRangeAttack1"
	},
};

Task_t tlHCRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slHCRangeAttack1Fast[] =
{
	{
		tlHCRangeAttack1Fast,
		ARRAYSIZE( tlHCRangeAttack1Fast ),
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRAFast"
	},
};

class CHeadCrab : public CBaseMonster
{
public:
	void Spawn( void );
	void SpawnHelper(const char* modelName, float health);
	void Precache( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	Vector Center( void );
	Vector BodyTarget( const Vector &posSrc );
	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );
	void PrescheduleThink( void );
	int  DefaultClassify ( void );
	const char* DefaultDisplayName() { return "Headcrab"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	bool CheckRangeAttack1 ( float flDot, float flDist ) override;
	bool CheckRangeAttack2 ( float flDot, float flDist ) override;
	DamageInfo DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo) override;
	virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite; }

	Schedule_t* GetScheduleOfType ( int Type );

	CUSTOM_SCHEDULES

	virtual int DefaultSizeForGrapple() { return GRAPPLE_SMALL; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return Vector( -12.0f, -12.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 12.0f, 12.0f, 24.0f ); }

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript leapSoundScript;
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript biteSoundScript;

protected:
	virtual void AttackSound() {
		if( RANDOM_LONG(0,2) != 0 )
			EmitSoundScript(attackSoundScript);
	}
	virtual void LeapSound() {
		EmitSoundScript(leapSoundScript);
	}
	virtual void BiteSound() {
		EmitSoundScript(biteSoundScript);
	}
};

LINK_ENTITY_TO_CLASS( monster_headcrab, CHeadCrab )

DEFINE_CUSTOM_SCHEDULES( CHeadCrab )
{
	slHCRangeAttack1,
	slHCRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHeadCrab, CBaseMonster )

const NamedSoundScript CHeadCrab::idleSoundScript = {
	CHAN_VOICE,
	{ "headcrab/hc_idle1.wav", "headcrab/hc_idle2.wav", "headcrab/hc_idle3.wav" },
	1.0f,
	ATTN_IDLE,
	"Headcrab.Idle"
};

const NamedSoundScript CHeadCrab::alertSoundScript = {
	CHAN_VOICE,
	{ "headcrab/hc_alert1.wav" },
	1.0f,
	ATTN_IDLE,
	"Headcrab.Alert"
};

const NamedSoundScript CHeadCrab::painSoundScript = {
	CHAN_VOICE,
	{ "headcrab/hc_pain1.wav", "headcrab/hc_pain2.wav", "headcrab/hc_pain3.wav" },
	1.0f,
	ATTN_IDLE,
	"Headcrab.Pain"
};

const NamedSoundScript CHeadCrab::leapSoundScript = {
	CHAN_WEAPON,
	{ "headcrab/hc_attack1.wav" },
	1.0f,
	ATTN_IDLE,
	"Headcrab.Leap"
};

const NamedSoundScript CHeadCrab::attackSoundScript = {
	CHAN_VOICE,
	{ "headcrab/hc_attack2.wav", "headcrab/hc_attack3.wav" },
	1.0f,
	ATTN_IDLE,
	"Headcrab.Attack"
};

const NamedSoundScript CHeadCrab::dieSoundScript = {
	CHAN_VOICE,
	{ "headcrab/hc_die1.wav", "headcrab/hc_die2.wav" },
	1.0f,
	ATTN_IDLE,
	"Headcrab.Die"
};

const NamedSoundScript CHeadCrab::biteSoundScript = {
	CHAN_WEAPON,
	{ "headcrab/hc_headbite.wav" },
	1.0f,
	ATTN_IDLE,
	"Headcrab.Bite"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHeadCrab::DefaultClassify( void )
{
	return CLASS_ALIEN_PREY;
}

//=========================================================
// Center - returns the real center of the headcrab.  The 
// bounding box is much larger than the actual creature so 
// this is needed for targeting
//=========================================================
Vector CHeadCrab::Center( void )
{
	return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 6.0f );
}

Vector CHeadCrab::BodyTarget( const Vector &posSrc ) 
{ 
	return Center();
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHeadCrab::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:	
		ys = 30;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 20;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 60;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 30;
		break;
	default:
		ys = 30;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHeadCrab::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HC_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors( pev->angles );

			Vector vecJumpDir;
			if( m_hEnemy != 0 )
			{
				float gravity = g_psv_gravity->value;
				if( gravity <= 1 )
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z;
				if( height < 16 )
					height = 16;
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin;
				vecJumpDir = vecJumpDir * ( 1.0f / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();

				if( distance > 650.0f )
				{
					vecJumpDir = vecJumpDir * ( 650.0f / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350.0f;
			}

			AttackSound();

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2.0f;
		}
		break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHeadCrab::Spawn()
{
	Precache();
	SpawnHelper("models/headcrab.mdl", gSkillData.headcrabHealth);
	MonsterInit();
}

void CHeadCrab::SpawnHelper(const char *modelName, float health)
{
	SetMyModel( modelName );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects		= 0;
	SetMyHealth( health );
	pev->view_ofs		= Vector( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	SetMyCanOpenDoors(false);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHeadCrab::Precache()
{
	PrecacheMyModel( "models/headcrab.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(leapSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(biteSoundScript);
}

//=========================================================
// RunTask 
//=========================================================
void CHeadCrab::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
		{
			if( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( NULL );
				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	default:
		{
			CBaseMonster::RunTask( pTask );
		}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CHeadCrab::LeapTouch( CBaseEntity *pOther )
{
	if( !pOther->pev->takedamage )
	{
		return;
	}

	if( pOther->Classify() == Classify() )
	{
		return;
	}

	// Don't hit if back on ground
	if( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		BiteSound();
		pOther->TakeDamage( pev, pev, DamageInfo(GetDamageAmount(), DMG_SLASH) );
	}

	SetTouch( NULL );
}

//=========================================================
// PrescheduleThink
//=========================================================
void CHeadCrab::PrescheduleThink( void )
{
	// make the crab coo a little bit in combat state
	if( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 5 ) < 0.1f )
	{
		IdleSound();
	}
}

void CHeadCrab::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			LeapSound();
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch( &CHeadCrab::LeapTouch );
			break;
		}
	default:
		{
			CBaseMonster::StartTask( pTask );
		}
	}
}

//=========================================================
// CheckRangeAttack1
//=========================================================
bool CHeadCrab::CheckRangeAttack1( float flDot, float flDist )
{
	if( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65f )
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
bool CHeadCrab::CheckRangeAttack2( float flDot, float flDist )
{
	return false;
}

DamageInfo CHeadCrab::DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo)
{
	// Don't take ally acid damage -- BigMomma's mortar is acid
	if( ( inputDamageInfo.type & DMG_ACID ) && pevAttacker)
	{
		CBaseEntity* pAttacker = Instance( pevAttacker );
		if (pAttacker)
		{
			const int rel = IRelationship( pAttacker );
			if (rel < R_DL && rel != R_FR)
			{
				DamageInfo damageInfo = inputDamageInfo;
				damageInfo.mustSkip = true;
				return damageInfo;
			}
		}
	}
	return inputDamageInfo;
}

#define CRAB_ATTN_IDLE (float)1.5

//=========================================================
// IdleSound
//=========================================================
void CHeadCrab::IdleSound( void )
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// AlertSound 
//=========================================================
void CHeadCrab::AlertSound( void )
{
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// AlertSound 
//=========================================================
void CHeadCrab::PainSound( void )
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// DeathSound 
//=========================================================
void CHeadCrab::DeathSound( void )
{
	EmitSoundScript(dieSoundScript);
}

Schedule_t *CHeadCrab::GetScheduleOfType( int Type )
{
	switch( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slHCRangeAttack1[0];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

class CDeadHeadCrab : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/headcrab.mdl"; }
	int	DefaultClassify() { return	CLASS_ALIEN_PREY; }

	const char* getPos(int pos) const {
		return "dieback";
	}
};

LINK_ENTITY_TO_CLASS( monster_headcrab_dead, CDeadHeadCrab )

void CDeadHeadCrab::Spawn( )
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
	pev->frame = 255;
}

class CBabyCrab : public CHeadCrab
{
public:
	void ApplyDefaultRenderProps(int overridenRenderProps);
	void Spawn( void );
	void Precache( void );
	const char* DefaultDisplayName() { return "Baby Headcrab"; }
	void SetYawSpeed( void );
	float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite * 0.3f; }
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	Schedule_t *GetScheduleOfType ( int Type );

	static constexpr const char* idleSoundScript = "Babycrab.Idle";
	static constexpr const char* alertSoundScript = "Babycrab.Alert";
	static constexpr const char* painSoundScript = "Babycrab.Pain";
	static constexpr const char* leapSoundScript = "Babycrab.Leap";
	static constexpr const char* attackSoundScript = "Babycrab.Attack";
	static constexpr const char* dieSoundScript = "Babycrab.Die";
	static constexpr const char* biteSoundScript  ="Babycrab.Bite";

	void IdleSound() {
		EmitSoundScript(idleSoundScript);
	}
	void AlertSound() {
		EmitSoundScript(alertSoundScript);
	}
	void PainSound() {
		EmitSoundScript(painSoundScript);
	}
	void DeathSound( void ) {
		EmitSoundScript(dieSoundScript);
	}
protected:
	void AttackSound()
	{
		if( RANDOM_LONG(0,2) != 0 )
			EmitSoundScript(attackSoundScript);
	}
	void LeapSound() {
		EmitSoundScript(leapSoundScript);
	}
	void BiteSound() {
		EmitSoundScript(biteSoundScript);
	}
};

LINK_ENTITY_TO_CLASS( monster_babycrab, CBabyCrab )

void CBabyCrab::ApplyDefaultRenderProps(int overridenRenderProps)
{
	if ((overridenRenderProps & Visual::RENDERMODE_DEFINED) == 0)
		pev->rendermode = kRenderTransTexture;
	if ((overridenRenderProps & Visual::ALPHA_DEFINED) == 0)
		pev->renderamt = 192;
}

void CBabyCrab::Spawn( void )
{
	Precache();
	SpawnHelper("models/baby_headcrab.mdl", gSkillData.headcrabHealth * 0.25f); // less health than full grown
	MonsterInit();
}

void CBabyCrab::Precache( void )
{
	PrecacheMyModel( "models/baby_headcrab.mdl" );
	PrecacheMyGibModel();

	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchRelative(IntRange(140, 150));
	paramOverride.OverrideVolumeRelative(0.8f);

	RegisterAndPrecacheSoundScript(idleSoundScript, CHeadCrab::idleSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(alertSoundScript, CHeadCrab::alertSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(painSoundScript, CHeadCrab::painSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(leapSoundScript, CHeadCrab::leapSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(attackSoundScript, CHeadCrab::attackSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(dieSoundScript, CHeadCrab::dieSoundScript, paramOverride);
	RegisterAndPrecacheSoundScript(biteSoundScript, CHeadCrab::biteSoundScript, paramOverride);
}

void CBabyCrab::SetYawSpeed( void )
{
	pev->yaw_speed = 120;
}

bool CBabyCrab::CheckRangeAttack1( float flDot, float flDist )
{
	if( pev->flags & FL_ONGROUND )
	{
		if( pev->groundentity && ( pev->groundentity->v.flags & ( FL_CLIENT | FL_MONSTER ) ) )
			return true;

		// A little less accurate, but jump from closer
		if( flDist <= 180.0f && flDot >= 0.55f )
			return true;
	}

	return false;
}

Schedule_t *CBabyCrab::GetScheduleOfType( int Type )
{
	switch( Type )
	{
		case SCHED_FAIL:	// If you fail, try to jump!
			if( m_hEnemy != 0 )
				return slHCRangeAttack1Fast;
		break;
		case SCHED_RANGE_ATTACK1:
		{
			return slHCRangeAttack1Fast;
		}
		break;
	}

	return CHeadCrab::GetScheduleOfType( Type );
}

#if FEATURE_SHOCKTROOPER
#define bits_MEMORY_SHOCKTROOPER_IS_OWNER bits_MEMORY_CUSTOM1

class CShockRoach : public CHeadCrab
{
public:
	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("shockroach"); }
	const char* DefaultDisplayName() { return "Shock Roach"; }
	virtual float GetDamageAmount( void ) { return gSkillData.sroachDmgBite; }
	void EXPORT LeapTouch(CBaseEntity *pOther);
	bool TryGiveAsWeapon(CBaseEntity* pOther);
	void EXPORT RoachUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps() {
		if (IsFullyAlive())
			return CBaseMonster::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE;
		else
			return CBaseMonster::ObjectCaps();
	}
	void PainSound(void);
	void DeathSound(void);
	void IdleSound(void);
	void AlertSound(void);
	void MonsterThink(void);
	void StartTask(Task_t* pTask);
	bool ShouldFadeOnDeath() override;
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;
	void OnDying();

	Vector DefaultMinHullSize() { return Vector( -12.0f, -12.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 12.0f, 12.0f, 4.0f ); }

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript leapSoundScript;
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript biteSoundScript;

	float m_flBirthTime;
	bool m_fRoachSolid;

protected:
	void AttackSound();
};

LINK_ENTITY_TO_CLASS(monster_shockroach, CShockRoach)

TYPEDESCRIPTION	CShockRoach::m_SaveData[] =
{
	DEFINE_FIELD(CShockRoach, m_flBirthTime, FIELD_TIME),
	DEFINE_FIELD(CShockRoach, m_fRoachSolid, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CShockRoach, CHeadCrab)

const NamedSoundScript CShockRoach::idleSoundScript = {
	CHAN_VOICE,
	{ "shockroach/shock_idle1.wav", "shockroach/shock_idle2.wav", "shockroach/shock_idle3.wav" },
	1.0f,
	ATTN_IDLE,
	"Shockroach.Idle"
};

const NamedSoundScript CShockRoach::alertSoundScript = {
	CHAN_VOICE,
	{ "shockroach/shock_angry.wav" },
	1.0f,
	ATTN_IDLE,
	"Shockroach.Alert"
};

const NamedSoundScript CShockRoach::painSoundScript = {
	CHAN_VOICE,
	{ "shockroach/shock_flinch.wav" },
	1.0f,
	ATTN_IDLE,
	"Shockroach.Pain"
};

const NamedSoundScript CShockRoach::leapSoundScript = {
	CHAN_WEAPON,
	{ "shockroach/shock_jump1.wav" },
	1.0f,
	ATTN_IDLE,
	"Shockroach.Leap"
};

const NamedSoundScript CShockRoach::attackSoundScript = {
	CHAN_VOICE,
	{ "shockroach/shock_jump2.wav" },
	1.0f,
	ATTN_IDLE,
	"Shockroach.Attack"
};

const NamedSoundScript CShockRoach::dieSoundScript = {
	CHAN_VOICE,
	{ "shockroach/shock_die.wav" },
	1.0f,
	ATTN_IDLE,
	"Shockroach.Die"
};

const NamedSoundScript CShockRoach::biteSoundScript = {
	CHAN_WEAPON,
	{ "shockroach/shock_bite.wav" },
	1.0f,
	ATTN_IDLE,
	"Shockroach.Bite"
};


//=========================================================
// Spawn
//=========================================================
void CShockRoach::Spawn()
{
	Precache();

	SetMyModel("models/w_shock_rifle.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects = 0;
	SetMyHealth( gSkillData.sroachHealth );
	pev->view_ofs = Vector(0, 0, 20);// position of the eyes relative to monster's origin.
	pev->yaw_speed = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	SetMyCanOpenDoors(false);

	m_fRoachSolid = false;
	m_flBirthTime = gpGlobals->time;

	MonsterInit();

	if (pev->owner && FClassnameIs(pev->owner, "monster_shocktrooper")) {
		Remember(bits_MEMORY_SHOCKTROOPER_IS_OWNER);
	}

	SetUse(&CShockRoach::RoachUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShockRoach::Precache()
{
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(leapSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(biteSoundScript);

	PRECACHE_SOUND("shockroach/shock_walk.wav");

	PrecacheMyModel("models/w_shock_rifle.mdl");
	PrecacheMyGibModel();
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CShockRoach::LeapTouch(CBaseEntity *pOther)
{
	if (!pOther->pev->takedamage)
	{
		return;
	}

	if (pOther->Classify() == Classify())
	{
		return;
	}

	if (!TryGiveAsWeapon(pOther))
	{
		if (!FBitSet(pev->flags, FL_ONGROUND))
		{
			EmitSoundScript(biteSoundScript);
			pOther->TakeDamage(pev, pev, DamageInfo(GetDamageAmount(), DMG_SLASH));
		}
	}

	SetTouch(NULL);
}

bool CShockRoach::TryGiveAsWeapon(CBaseEntity *pOther)
{
#if FEATURE_SHOCKRIFLE
	// Give the shockrifle weapon to the player, if not already in possession.
	if (g_modFeatures.IsWeaponEnabled(WEAPON_SHOCKRIFLE) && pOther->IsPlayer() && pOther->IsAlive()) {
		CBasePlayer* pPlayer = (CBasePlayer*)(pOther);
		if (!pPlayer->HasWeaponBit(WEAPON_SHOCKRIFLE)) {
			pPlayer->GiveNamedItem("weapon_shockrifle");
			UTIL_Remove(this);
			return true;
		}
	}
#endif
	return false;
}

void CShockRoach::RoachUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (TryGiveAsWeapon(pCaller)) {
		SetTouch(NULL);
		SetUse(NULL);
	}
}
//=========================================================
// PrescheduleThink
//=========================================================
void CShockRoach::MonsterThink(void)
{
	float lifeTime = (gpGlobals->time - m_flBirthTime);
	if (lifeTime >= 0.2)
	{
		pev->movetype = MOVETYPE_STEP;
	}
	if (!m_fRoachSolid && lifeTime >= 2.0) {
		m_fRoachSolid = true;
		SetMySize(DefaultMinHullSize(), DefaultMaxHullSize());
	}
	// die when ready
	if (lifeTime >= gSkillData.sroachLifespan)
	{
		TakeDamage(pev, pev, DamageInfo(pev->health, DMG_GENERIC).SetGibPolicy(GIB_NEVER));
	}

	CHeadCrab::MonsterThink();
}

//=========================================================
// IdleSound
//=========================================================
void CShockRoach::IdleSound(void)
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CShockRoach::AlertSound(void)
{
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CShockRoach::PainSound(void)
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// DeathSound
//=========================================================
void CShockRoach::DeathSound(void)
{
	EmitSoundScript(dieSoundScript);
}


void CShockRoach::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
		EmitSoundScript(leapSoundScript);
		m_IdealActivity = ACT_RANGE_ATTACK1;
		SetTouch(&CShockRoach::LeapTouch);
		break;
	}
	default:
		CHeadCrab::StartTask(pTask);
	}
}

bool CShockRoach::ShouldFadeOnDeath()
{
	if( ( pev->spawnflags & SF_MONSTER_FADECORPSE ) || (!FNullEnt( pev->owner ) && !HasMemory(bits_MEMORY_SHOCKTROOPER_IS_OWNER)) )
		return true;
	return false;
}

void CShockRoach::AttackSound()
{
	if ( RANDOM_LONG(0, 2) != 0 )
		EmitSoundScript(attackSoundScript);
}

TakeDamageResult CShockRoach::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	DamageInfo dmgInfo = damageInfo;
	if ( gpGlobals->time - m_flBirthTime < 2.0 )
		dmgInfo.damage = 0.0;
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, dmgInfo );
}

void CShockRoach::OnDying()
{
	SetUse(NULL);
}

class CDeadShockRoach : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/w_shock_rifle.mdl"; }
	int	DefaultClassify () { return	CLASS_ALIEN_PREY; }

	const char* getPos(int pos) const {
		return "dieback";
	}
};

LINK_ENTITY_TO_CLASS( monster_shockroach_dead, CDeadShockRoach )

void CDeadShockRoach::Spawn()
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
	pev->frame = 255;
}

#endif
