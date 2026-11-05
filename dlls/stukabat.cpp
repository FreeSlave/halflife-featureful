#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"soundent.h"
#include	"decals.h"
#include	"combat.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"studio.h"
#include	"nodes.h"

class CStukaBomb : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	void Touch( CBaseEntity *pOther ) override;
	RadiusDamageInfo GetDefaultProjectileRadiusDamageInfo() override {
		return RadiusDamageInfo(DamageInfo(GetSkillValue("stuka_dmg_bomb"), DMG_POISON).SetTimedNonLethal().SetTimedIgnoreArmor(), 64.0f);
	}
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override {
		SetProjectileParamsBeforeSpawnImpl(params);
	}
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	void EXPORT Animate();

	static const NamedVisual bombVisual;

	static constexpr const char* spitTouchSoundScript = "Stukabat.BombTouch";
	static constexpr const char* spitHitSoundScript = "Stukabat.BombHit";
};

LINK_ENTITY_TO_CLASS( stukabomb, CStukaBomb );

const NamedVisual CStukaBomb::bombVisual = BuildVisual("Stukabat.Bomb")
	.Model("models/spit.mdl")
	.Framerate(1.0f);

void CStukaBomb::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	ApplyVisualWithOwn(GetVisual(bombVisual));

	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0));

	const float framerate = pev->framerate;
	ResetSequenceInfo();
	pev->framerate = framerate;

	pev->classname = MAKE_STRING("stukabomb");
}

void CStukaBomb::Precache()
{
	RegisterVisualAsMineOwn(bombVisual);
	RegisterAndPrecacheSoundScript(spitTouchSoundScript, NPC::spitTouchSoundScript);
	RegisterAndPrecacheSoundScript(spitHitSoundScript, NPC::spitHitSoundScript);
}

void CStukaBomb::Animate()
{
	pev->angles = UTIL_VecToAngles( pev->velocity );
	StudioFrameAdvance();
}

void CStukaBomb::Touch(CBaseEntity *pOther)
{
	EmitSoundScript(spitTouchSoundScript);
	EmitSoundScript(spitHitSoundScript);

	if (!pOther->pev->takedamage)
	{
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_YBLOOD2 + RANDOM_LONG(0,3));
	}

	entvars_t *pevOwner = !FNullEnt(pev->owner) ? VARS(pev->owner) : nullptr;
	::RadiusDamage(pev->origin, pev, pevOwner, GetProjectileRadiusDamageInfo());

	SetThink(&CStukaBomb::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

void CStukaBomb::LaunchAsProjectile(const ProjectileParameters& params)
{
	LaunchAsProjectileImpl(900.0f, params);
	SetMyProjectileEffectFlags();
	SetThink(&CStukaBomb::Animate);
	pev->nextthink = gpGlobals->time + 0.1f;
}

enum
{
	STUKABAT_MODE_FLYING = 0,
	STUKABAT_MODE_GOING_TO_PERCH = 1,
	STUKABAT_MODE_ON_CEILING = 2,
	STUKABAT_MODE_UNKNOWN = 3,
	STUKABAT_MODE_WAITING = 4
};

#define STUKABAT_AE_BOMB 1
#define STUKABAT_AE_CLAW 2

#define STUKABAT_AE_FLAP		8
#define STUKABAT_AE_FLAPSOUND	9

class CStukabat : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void CheckThrillModel();
	int DefaultClassify() override { return CLASS_ALIEN_MONSTER; }
	const char* DefaultModel() override { return "models/stukabat.mdl"; }
	float DefaultHealth() override { return GetSkillValue("stuka_health"); }
	const char* DefaultDisplayName() override { return "Stukabat"; }
	Vector DefaultMinHullSize() override { return Vector( -12, -12, 0 ); }
	Vector DefaultMaxHullSize() override { return Vector( 12, 12, 24 ); }
	void SetYawSpeed() override;
	int LookupActivity(int activity) override;
	void SetActivity( Activity activity ) override;
	void DoClawAttack(int event);
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	Activity GetStoppedActivity() override;
	void Stop() override;
	float CurrentLookDistance() override {
		if (m_iActiveMode == STUKABAT_MODE_ON_CEILING)
		{
			return CBaseMonster::CurrentLookDistance() / 8.0f;
		}
		return CBaseMonster::CurrentLookDistance();
	}
	float CurrentFieldOfView() override {
		if (m_iActiveMode == STUKABAT_MODE_ON_CEILING)
		{
			return VIEW_FIELD_FULL;
		}
		return CBaseMonster::CurrentFieldOfView();
	}
	void MonsterThink() override;
	Vector NextLocationInRoute();
	void Move( float interval = 0.1f ) override;
	void MoveExecute( CBaseEntity *target, const Vector &direction, float interval ) override;
	bool ShouldAdvanceRoute( float waypointDistance ) override;
	int CheckLocalMove( const Vector &start, const Vector &end, CBaseEntity *pTarget, float *pflDist ) override;
	void StartTask(Task_t *pTask ) override;
	void RunTask( Task_t *pTask ) override;
	Schedule_t *GetSchedule() override;
	Schedule_t *GetScheduleOfType( int type ) override;
	int DefaultChaseEnemyFailedPolicy() override {
		return CHASE_ENEMY_FAILED_RETREAT;
	}
	int DefaultSizeForGrapple() override { return GRAPPLE_SMALL; }
	bool IsDisplaceable() override { return true; }

	void IdleSound() override {
		if (m_iActiveMode == STUKABAT_MODE_ON_CEILING)
		{
			EmitSoundScript(idlePerchedSoundScript);
		}
		else
		{
			EmitSoundScript(idleSoundScript);
		}
	}
	void AlertSound() override {
		EmitSoundScript(alertSoundScript);
	}
	void PainSound() override {
		EmitSoundScript(painSoundScript);
	}
	void DeathSound() override {
		EmitSoundScript(dieSoundScript);
	}

	void SetActiveMode( int mode );
	std::pair<float, edict_t*> CeilingPos( const Vector &position );
	float FloorZ( const Vector &position );
	bool FindPerchPosition( Vector position, Vector &perch );
	bool FindPerchNode( float distance, Vector &perch );
	CUSTOM_SCHEDULES;

	int	Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	Vector	m_SaveVelocity;
	float	m_stopTime;
	int		m_iActiveMode;
	float	m_flightSpeed;

	float m_flNextBomb;
	bool m_didClawAttack;

	bool m_hasClawEvent;
	bool m_landCeilingIsActLand;
	bool m_hasStartCeiling;

	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript attackHitSoundScript;
	static const NamedSoundScript attackMissSoundScript;
	static const NamedSoundScript flapSoundScript;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript idlePerchedSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
};

LINK_ENTITY_TO_CLASS( monster_stukabat, CStukabat );

TYPEDESCRIPTION	CStukabat::m_SaveData[] =
{
	DEFINE_FIELD( CStukabat, m_SaveVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CStukabat, m_stopTime, FIELD_TIME ),
	DEFINE_FIELD( CStukabat, m_iActiveMode, FIELD_INTEGER ),
	DEFINE_FIELD( CStukabat, m_flightSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CStukabat, m_flNextBomb, FIELD_TIME ),
	DEFINE_FIELD( CStukabat, m_didClawAttack, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CStukabat, CBaseMonster );

const NamedSoundScript CStukabat::attackSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_attack1.wav"},
	"Stukabat.Attack"
};

const NamedSoundScript CStukabat::attackHitSoundScript = {
	CHAN_WEAPON,
	{"headcrab/hc_headbite.wav"},
	1.0f,
	ATTN_IDLE,
	IntRange(120, 140),
	"Stukabat.AttackHit"
};

const NamedSoundScript CStukabat::attackMissSoundScript = {
	CHAN_WEAPON,
	{"zombie/claw_miss2.wav"},
	"Stukabat.AttackMiss"
};

const NamedSoundScript CStukabat::flapSoundScript = {
	CHAN_BODY,
	{"zombie/claw_miss1.wav"},
	"Stukabat.Flap"
};

const NamedSoundScript CStukabat::idleSoundScript = {
	CHAN_VOICE,
	{},
	"Stukabat.Idle"
};

const NamedSoundScript CStukabat::idlePerchedSoundScript = {
	CHAN_VOICE,
	{},
	"Stukabat.IdlePerched"
};

const NamedSoundScript CStukabat::alertSoundScript = {
	CHAN_VOICE,
	{},
	"Stukabat.Alert"
};

const NamedSoundScript CStukabat::painSoundScript = {
	CHAN_VOICE,
	{},
	"Stukabat.Pain"
};

const NamedSoundScript CStukabat::dieSoundScript = {
	CHAN_VOICE,
	{},
	"Stukabat.Die"
};

enum
{
	SCHED_STUKA_REST_PERCH = LAST_COMMON_SCHEDULE + 1,
	SCHED_STUKA_FREEROAM_NO_WAIT
};

enum
{
	TASK_STUKA_FIND_PERCH = LAST_COMMON_TASK + 1,
	TASK_STUKA_FIND_GROUND,
	TASK_STUKA_MOVE,
	TASK_STUKA_TAKE_OFF,
	TASK_STUKA_FALL
};

Task_t tlStukaRestPerch[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_STUKA_FREEROAM_NO_WAIT },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_STUKA_FIND_PERCH, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_PLAY_SEQUENCE, (float) ACT_STAND },
	{ TASK_SET_ACTIVITY, ACT_IDLE }
};
Schedule_t slStukaRestPerch[] =
{
	{
		tlStukaRestPerch,
		ARRAYSIZE( tlStukaRestPerch ),
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY,
		0,
		"StukaRestPerch"
	}
};

Task_t tlStukaFreeroam[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT_RANDOM, 0.5f },
	{ TASK_GET_PATH_TO_FREEROAM_NODE, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slStukaFreeroam[] =
{
	{
		tlStukaFreeroam,
		ARRAYSIZE( tlStukaFreeroam ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Stuka Free Roaming"
	},
};

Task_t tlStukaDie[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SOUND_DIE, 0 },
	{ TASK_STUKA_FALL, 0 },
	{ TASK_DIE, 0 }
};
Schedule_t slStukaDie[] =
{
	{
		tlStukaDie,
		ARRAYSIZE( tlStukaDie ),
		0,
		0,
		"StukaDeath"
	}
};

Task_t tlStukaGroundTakeOff[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float) ACT_LEAP },
	{ TASK_PLAY_SEQUENCE, (float) ACT_HOVER }
};
Schedule_t slStukaGroundTakeOff[] =
{
	{
		tlStukaGroundTakeOff,
		ARRAYSIZE( tlStukaGroundTakeOff ),
		0,
		0,
		"StukaGroundTakeOff"
	}
};

Task_t tlStukaGroundTakeOffAndFly[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float) ACT_LEAP },
	{ TASK_PLAY_SEQUENCE, (float) ACT_HOVER },
	{ TASK_SET_SCHEDULE, (float)SCHED_STUKA_FREEROAM_NO_WAIT }
};
Schedule_t slStukaGroundTakeOffAndFly[] =
{
	{
		tlStukaGroundTakeOffAndFly,
		ARRAYSIZE( tlStukaGroundTakeOffAndFly ),
		0,
		0,
		"StukaGroundTakeOffAndFly"
	}
};

Task_t tlStukaVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float) ACT_HOVER },
	{ TASK_STUKA_FIND_GROUND, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_PLAY_SEQUENCE, (float) ACT_LAND },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0.0f },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_PLAY_SEQUENCE, (float) ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
};
Schedule_t slStukaVictoryDance[] =
{
	{
		tlStukaVictoryDance,
		ARRAYSIZE( tlStukaVictoryDance ),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"StukaVictoryDance"
	}
};

DEFINE_CUSTOM_SCHEDULES( CStukabat )
{
	slStukaRestPerch,
	slStukaFreeroam,
	slStukaGroundTakeOff,
	slStukaGroundTakeOffAndFly,
	slStukaVictoryDance,
	slStukaDie
};
IMPLEMENT_CUSTOM_SCHEDULES( CStukabat, CBaseMonster );

void CStukabat::Spawn()
{
	Precache();

	SetMyModel();
	SetMyHealth();
	SetMySize();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	SetBits( pev->flags, FL_FLY );
	pev->view_ofs = Vector( 0, 0, 22 );
	pev->yaw_speed = 150;
	SetMyFieldOfView(0.5f);
	m_MonsterState = MONSTERSTATE_NONE;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	m_stopTime = gpGlobals->time;

	MonsterInit();
	SetActivity( ACT_FLY );
	SetActiveMode( STUKABAT_MODE_FLYING );
	m_flightSpeed = 0;

	CheckThrillModel();
}

void CStukabat::Precache()
{
	PrecacheMyModel();

	UTIL_PrecacheOther("stukabomb", GetProjectileOverrides());

	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript);
	RegisterAndPrecacheSoundScript(flapSoundScript);

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(idlePerchedSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);

	if (pev->modelindex)
		CheckThrillModel();
}

void CStukabat::CheckThrillModel()
{
	m_hasClawEvent = false;
	m_landCeilingIsActLand = false;
	m_hasStartCeiling = false;
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	if (!pmodel)
		return;

	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;

	for (int i = 0; i<pstudiohdr->numseq; ++i)
	{
		mstudioseqdesc_t* pseqdesc = (mstudioseqdesc_t*)((byte *)pmodel + pstudiohdr->seqindex) + i;

		if (pseqdesc->activity == ACT_RANGE_ATTACK2)
		{
			mstudioevent_t *pevent = (mstudioevent_t *)( (byte *)pstudiohdr + pseqdesc->eventindex );

			for (int j = 0; j < pseqdesc->numevents; j++)
			{
				if (pevent[j].event == STUKABAT_AE_CLAW)
				{
					m_hasClawEvent = true;
					//ALERT(at_console, "%s: has claw event\n", STRING(pev->classname));
				}
			}
		}
		else if (pseqdesc->activity == ACT_LAND && stricmp(pseqdesc->label, "Land_ceiling") == 0)
		{
			m_landCeilingIsActLand = true;
			//ALERT(at_console, "%s: Land_ceiling is ACT_LAND\n", STRING(pev->classname));
		}
		else if (pseqdesc->activity == ACT_STAND && stricmp(pseqdesc->label, "Stand_ceiling") == 0)
		{
			m_hasStartCeiling = true;
			//ALERT(at_console, "%s: has Stand_ceiling\n", STRING(pev->classname));
		}
	}
}

void CStukabat::SetYawSpeed( void )
{
	pev->yaw_speed = 150;
}

bool CStukabat::CheckRangeAttack1( float flDot, float flDist )
{
	if (pev->movetype != MOVETYPE_FLY)
		return false;

	if (m_flNextBomb > gpGlobals->time)
		return false;

	if ( flDist > 96 && flDist <= 1024 && flDot > 0.6f )
		return true;

	return false;
}

bool CStukabat::CheckRangeAttack2( float flDot, float flDist )
{
	if (pev->movetype != MOVETYPE_FLY)
		return false;

	return flDist <= 64 && flDot >= 0.5f;
}

void CStukabat::DoClawAttack(int event)
{
	TraceHullAttackParams params;
	params.distance = 80.0f;
	params.damageInfo = DamageInfo(GetSkillValue("stuka_dmg_slash"), DMG_SLASH|DMG_POISON).SetTimedNonLethal().SetTimedIgnoreArmor();
	params.skipAllies = true;
	params.punchAngle.x = 5.0f;
	params.punchAngle.z = -18.0f;
	//params.knockRight = -100.0f;
	params.hitSoundScript = attackHitSoundScript;
	params.missSoundScript = attackMissSoundScript;
	SetTraceHullAttackParamsFromTemplate(event, params);
	PerformTraceHullAttack(params);
}

void CStukabat::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case STUKABAT_AE_BOMB:
	{
		EmitSoundScript(attackSoundScript);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3f);

		UTIL_MakeVectors(pev->angles);
		const Vector forward = gpGlobals->v_forward;
		const Vector up = gpGlobals->v_up;

		const Vector vecSpitOffset = pev->origin + forward * 32.0f;
		const Vector targetPos = m_hEnemy != 0 ? m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs : m_vecEnemyLKP;

		Vector vecSpitDir = targetPos - vecSpitOffset;
		const float gravity = g_psv_gravity->value;
		const float time = vecSpitDir.Length() / 900.0f;
		vecSpitDir = vecSpitDir * (1.0f / time);

		// adjust upward toss to compensate for gravity loss
		vecSpitDir.z += gravity * time * 0.5f;
		const float speed = vecSpitDir.NormalizeInPlace();

		ProjectileParameters params("stukabomb", vecSpitOffset, pev->angles, vecSpitDir, speed, this, GetProjectileOverrides());
		CreateAndLaunchAsProjectile(params);

		const Vector vecStreamDirBase = (forward + up).Normalize();
		for (int i = 0; i < 4; i++)
		{
			Vector vecStreamDir = vecStreamDirBase;
			vecStreamDir.x += RANDOM_FLOAT( -0.6f, 0.6f );
			vecStreamDir.y += RANDOM_FLOAT( -0.6f, 0.6f );
			vecStreamDir.z += RANDOM_FLOAT( -0.6f, 0.6f );

			UTIL_BloodStream(vecSpitOffset, vecStreamDir.Normalize(), BLOOD_COLOR_YELLOW, 100);
		}
		m_flNextBomb = gpGlobals->time + RANDOM_FLOAT(4.0f, 8.0f);
	}
	break;
	case STUKABAT_AE_CLAW:
		DoClawAttack(pEvent->event);
		break;
	case STUKABAT_AE_FLAP:
		break;
	case STUKABAT_AE_FLAPSOUND:
	{
		if (m_IdealActivity == ACT_HOVER)
		{
			SoundScriptParamOverride paramOverride;
			paramOverride.OverridePitchRelative(120);
			EmitSoundScript(flapSoundScript, paramOverride);
		}
		else
		{
			EmitSoundScript(flapSoundScript);
		}
	}
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

int CStukabat::LookupActivity(int activity)
{
	int result;
	if (activity == ACT_RANGE_ATTACK2)
	{
		result = LookupSequence("Attack_claw");
		if (result != ACTIVITY_NOT_AVAILABLE)
			return result;
	}
	if (activity == ACT_STAND && m_landCeilingIsActLand)
	{
		result = LookupSequence("Land_ceiling");
		if (result != ACTIVITY_NOT_AVAILABLE)
			return result;
	}
	if (activity == ACT_LAND && m_landCeilingIsActLand)
	{
		result = LookupSequence("Land_ground");
		if (result != ACTIVITY_NOT_AVAILABLE)
			return result;
	}
	if (activity == ACT_SMALL_FLINCH && pev->movetype != MOVETYPE_FLY)
	{
		return LookupSequence("Flinch_on_ground");
	}

	result = CBaseMonster::LookupActivity(activity);
	/*if (result == ACTIVITY_NOT_AVAILABLE && activity == ACT_GLIDE)
	{
		result = LookupSequence("Dive_cycler");
	}*/
	return result;
}

void CStukabat::SetActivity( Activity activity )
{
	if ( activity == ACT_IDLE && m_iActiveMode != STUKABAT_MODE_ON_CEILING )
		activity = pev->movetype == MOVETYPE_STEP && pev->groundentity ? ACT_CROUCHIDLE : ACT_HOVER;

	CBaseMonster::SetActivity( activity );
}

Activity CStukabat::GetStoppedActivity()
{
	return pev->movetype == MOVETYPE_FLY ? ACT_HOVER : ACT_CROUCHIDLE;
}

void CStukabat::Stop( void )
{
	m_IdealActivity = GetStoppedActivity();
	m_flightSpeed = 0;
	pev->velocity = g_vecZero;
	m_SaveVelocity = pev->velocity;
}

void CStukabat::SetActiveMode( int mode )
{
	if ( mode == STUKABAT_MODE_FLYING )
		m_stopTime = gpGlobals->time + 5;

	m_iActiveMode = mode;
}

void CStukabat::MonsterThink()
{
	CBaseMonster::MonsterThink();

	if ( HasMemory( bits_MEMORY_KILLED ) )
		return;

	Vector direction = ( NextLocationInRoute() - pev->origin ).Normalize();

	if ( m_flightSpeed <= 0 )
	{
		m_SaveVelocity = g_vecZero;
	}
	else if (m_IdealActivity == ACT_RANGE_ATTACK2 || m_IdealActivity == ACT_RANGE_ATTACK1)
	{
		m_SaveVelocity = direction * 60.0f;
	}
	else
	{
		m_SaveVelocity = direction * m_flightSpeed * 0.2f + m_SaveVelocity * 0.8f;
	}

	pev->velocity = m_SaveVelocity;

	if ( m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT || m_MonsterState == MONSTERSTATE_HUNT )
	{
		if ( gpGlobals->time > m_stopTime && m_iActiveMode != STUKABAT_MODE_ON_CEILING )
			SetActiveMode( STUKABAT_MODE_GOING_TO_PERCH );

		if ( m_iActiveMode == STUKABAT_MODE_FLYING )
			SetActiveMode( STUKABAT_MODE_WAITING );
	}
	else
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			m_stopTime = gpGlobals->time + 5;

		if ( m_iActiveMode != STUKABAT_MODE_WAITING )
			SetActiveMode( STUKABAT_MODE_FLYING );
	}
}

bool CStukabat::ShouldAdvanceRoute( float waypointDistance )
{
	Vector delta = m_Route[m_iRouteIndex].vecLocation - pev->origin;

	return FBitSet( pev->flags, FL_ONGROUND ) ? delta.Length2D() <= 8 : delta.Length() <= 8;
}

int CStukabat::CheckLocalMove( const Vector &start, const Vector &end, CBaseEntity *pTarget, float *pflDist )
{
	if ( pev->movetype == MOVETYPE_STEP )
		return CBaseMonster::CheckLocalMove( start, end, pTarget, pflDist );

	TraceResult tr;
	TRACE_MONSTER_HULL( edict(), start, end, dont_ignore_monsters, edict(), &tr );

	if ( pflDist )
		*pflDist = ( tr.vecEndPos - start ).Length();

	if( tr.fStartSolid || tr.flFraction < 1.0f )
	{
		if( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}

void CStukabat::MoveExecute( CBaseEntity *target, const Vector &direction, float interval )
{
	if ( pev->movetype != MOVETYPE_FLY )
	{
		CBaseMonster::MoveExecute( target, direction, interval );
		return;
	}

	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	MakeIdealYaw( pev->origin + m_SaveVelocity );
	ChangeYaw( pev->yaw_speed );
}

std::pair<float, edict_t*> CStukabat::CeilingPos( const Vector &position )
{
	Vector ceiling = position + Vector( 0, 0, 4096 );
	TraceResult tr;
	TRACE_MONSTER_HULL( edict(), position, ceiling, dont_ignore_monsters, edict(), &tr );

	if ( tr.flFraction != 1.0f )
		ceiling.z = tr.vecEndPos.z;

	return std::make_pair(ceiling.z, tr.pHit);
}

float CStukabat::FloorZ( const Vector &position )
{
	Vector floor = position - Vector( 0, 0, 2048 );
	TraceResult trace;
	UTIL_TraceLine( position, floor, ignore_monsters, NULL, &trace );

	return trace.flFraction == 1.0f ? floor.z : trace.vecEndPos.z;
}

bool CStukabat::FindPerchPosition( Vector position, Vector &perch )
{
	auto p = CeilingPos( position );
	position.z = p.first;

	if (p.second)
	{
		CBaseEntity* pEntity = CBaseEntity::OwnInstance(p.second);
		if (pEntity && pEntity->MyMonsterPointer())
		{
			// someone else is at this spot
			return false;
		}
	}

	if ( UTIL_PointContents( position ) < CONTENTS_SOLID )
		return false;

	TraceResult tr;
	Vector vecStart = position - Vector(0,0,1);
	Vector vecEnd = position + Vector(0,0,pev->size.z * 2);
	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &tr );
	CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);

	const char* pTexture = TRACE_TEXTURE(pHit?pHit->edict():ENT(0), vecStart, vecEnd);
	if (pTexture && stricmp(pTexture, "sky") == 0)
	{
		return false;
	}

	perch = position;
	return true;
}

bool CStukabat::FindPerchNode( float distance, Vector &perch )
{
	if ( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		//ALERT( at_aiconsole, "Graph not ready for FindPerchNode!\n" );
		return false;
	}

	if ( distance == 0 )
		distance = 784;

	int nearest = WorldGraph.FindNearestNode( pev->origin, this );
	if ( nearest == NO_NODE )
	{
		//ALERT( at_aiconsole, "FindPerchNode() - %s has no nearest node!\n", STRING( pev->classname ) );
		return false;
	}

	for ( int i = 0; i < WorldGraph.m_cNodes; i++ )
	{
		int index = ( i + WorldGraph.m_iLastActiveIdleSearch ) % WorldGraph.m_cNodes;
		CNode &node = WorldGraph.Node( index );

		if ( !( node.m_afNodeInfo & bits_NODE_AIR ) )
			continue;

		WorldGraph.m_iLastActiveIdleSearch = index + 1;

		if ( ( pev->origin - node.m_vecOrigin ).Length() < distance &&
			FindPerchPosition( node.m_vecOrigin, perch ) )
			return true;
	}

	return false;
}

void CStukabat::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH:
	case TASK_WALK_PATH:
	case TASK_STUKA_MOVE:
		m_movementActivity = pev->movetype == MOVETYPE_FLY ? ACT_FLY : ACT_WALK;
		TaskComplete();
		break;
	case TASK_PLAY_SEQUENCE:
		switch ( (int) pTask->flData )
		{
		case ACT_LEAP:
			pev->movetype = MOVETYPE_FLY;
			ClearBits( pev->flags, FL_ONGROUND );
			SetBits( pev->flags, FL_FLY );
			UTIL_SetOrigin( pev, pev->origin );
			break;
		case ACT_LAND:
			pev->movetype = MOVETYPE_STEP;
			ClearBits( pev->flags, FL_ONGROUND | FL_FLY );
			UTIL_SetOrigin( pev, pev->origin );
			break;
		case ACT_STAND:
			SetActiveMode( STUKABAT_MODE_ON_CEILING );
			break;
		}

		CBaseMonster::StartTask( pTask );
		break;
	case TASK_STUKA_FIND_PERCH:
	{
		Vector perch = pev->origin;

		if ( !FindPerchNode( 1024, perch ) && !FindPerchPosition( perch, perch ) )
		{
			TaskFail("couldn't find perch");
			break;
		}

		if ( MoveToLocation( m_movementActivity, 0, perch ) )
			TaskComplete();
		else
			TaskFail("couldn't move to perch");
		break;
	}
	case TASK_STUKA_FIND_GROUND:
	{
		Vector floor = pev->origin;
		floor.z = FloorZ( pev->origin );

		if ( MoveToLocation( m_movementActivity, 0, floor ) )
			TaskComplete();
		else
			TaskFail("couldn't move to floor");
		break;
	}
	case TASK_STUKA_TAKE_OFF:
		if ( pev->movetype == MOVETYPE_FLY )
			TaskComplete();
		else
			m_IdealActivity = ACT_LEAP;
		break;
	case TASK_STUKA_FALL:
		pev->movetype = MOVETYPE_TOSS;
		ClearBits( pev->flags, FL_ONGROUND | FL_FLY );
		pev->angles.z = 0;
		pev->angles.x = 0;
		m_IdealActivity = ACT_FALL;
		break;
	case TASK_RANGE_ATTACK2:
		m_didClawAttack = false;
		CBaseMonster::StartTask( pTask );
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CStukabat::RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_PLAY_SEQUENCE:
		CBaseMonster::RunTask( pTask );

		if ( m_iTaskStatus == TASKSTATUS_COMPLETE )
		{
			if ( (int) pTask->flData == ACT_LEAP )
			{
				pev->velocity = g_vecZero;
				m_flightSpeed = 0;
				pev->groundentity = NULL;
				pev->movetype = MOVETYPE_FLY;
				ClearBits( pev->flags, FL_ONGROUND );
				SetBits( pev->flags, FL_FLY );
				UTIL_SetOrigin( pev, pev->origin );
			}
			else if ( (int) pTask->flData == ACT_HOVER )
				SetActiveMode( STUKABAT_MODE_FLYING );
		}
		break;
	case TASK_WAIT_FOR_MOVEMENT:
		if ( pev->movetype == MOVETYPE_FLY )
		{
			if ( ( m_vecMoveGoal - pev->origin ).Length2D() < pev->size.x * 4 &&
				std::fabs( m_vecMoveGoal.z - pev->origin.z ) > pev->size.z )
				m_movementActivity = ACT_HOVER;
			else
				m_movementActivity = ACT_FLY;
		}

		CBaseMonster::RunTask( pTask );
		break;
	case TASK_STUKA_TAKE_OFF:
		if ( m_fSequenceFinished )
		{
			m_flNextBomb = gpGlobals->time + RANDOM_FLOAT (4.0f, 8.0f);
			TaskComplete();
		}
		break;
	case TASK_STUKA_FALL:
		if ( pev->groundentity )
			TaskComplete();
		break;
	case TASK_RANGE_ATTACK2:
	{
		if (!m_hasClawEvent && !m_didClawAttack && pev->frame > 160.0f)
		{
			DoClawAttack(STUKABAT_AE_CLAW);
			m_didClawAttack = true;
		}
		CBaseMonster::RunTask( pTask );
	}
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

Schedule_t *CStukabat::GetScheduleOfType( int type )
{
	switch ( type )
	{
	case SCHED_VICTORY_DANCE:
		return slStukaVictoryDance;
	case SCHED_DIE:
		return slStukaDie;
	case SCHED_STUKA_REST_PERCH:
		return slStukaRestPerch;
	case SCHED_STUKA_FREEROAM_NO_WAIT:
		return slStukaFreeroam;
	default:
		return CBaseMonster::GetScheduleOfType( type );
	}
}

Schedule_t *CStukabat::GetSchedule()
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		if ( pev->movetype == MOVETYPE_STEP )
			return slStukaGroundTakeOff;

		if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if ( HasAllConditions( bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE ) )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );

		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		break;
	case MONSTERSTATE_ALERT:
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			return GetScheduleOfType( SCHED_VICTORY_DANCE );

		// Fall through.
	case MONSTERSTATE_IDLE:
		if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			SetActiveMode( STUKABAT_MODE_FLYING );
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( pev->movetype == MOVETYPE_STEP || pev->groundentity )
			return slStukaGroundTakeOffAndFly;

		if ( m_iActiveMode == STUKABAT_MODE_GOING_TO_PERCH )
			return GetScheduleOfType( SCHED_STUKA_REST_PERCH );

		if ( m_iActiveMode == STUKABAT_MODE_ON_CEILING )
		{
			if (HasConditions(bits_COND_HEAR_SOUND))
			{
				SetActiveMode(STUKABAT_MODE_FLYING);
				return GetScheduleOfType(SCHED_ALERT_FACE);
			}
			return GetScheduleOfType( SCHED_IDLE_STAND );
		}
		break;
	}

	return CBaseMonster::GetSchedule();
}

Vector CStukabat::NextLocationInRoute()
{
	Vector location = m_Route[m_iRouteIndex].vecLocation;
	if (FBitSet(m_Route[m_iRouteIndex].iType, bits_MF_TO_ENEMY) && m_hEnemy != 0)
	{
		location += m_hEnemy->pev->view_ofs;
	}
	return location;
}

void CStukabat::Move( float interval )
{
	if ( pev->movetype == MOVETYPE_FLY && m_flightSpeed < 10 )
		m_flightSpeed = 370;

	if ( pev->movetype == MOVETYPE_STEP )
	{
		m_flightSpeed = 0;
		CBaseMonster::Move( interval );
		return;
	}

	if ( FRouteClear() )
	{
		TaskFail("tried to move with no route");
		return;
	}

	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

	Vector delta = m_Route[m_iRouteIndex].vecLocation - pev->origin;
	Vector direction = delta.Normalize();
	float distance = delta.Length();
	float checkDistance = Q_min( distance, 200.0f );
	int routeType = m_Route[m_iRouteIndex].iType & ~(bits_MF_NOT_TO_MASK|bits_MF_NEAREST_PATH);
	CBaseEntity *target = NULL;

	if ( routeType == bits_MF_TO_ENEMY )
		target = m_hEnemy;
	else if ( routeType == bits_MF_TO_TARGETENT )
		target = m_hTargetEnt;

	float travelled = 0;
	int localMove = CheckLocalMove( pev->origin, pev->origin + direction * checkDistance, target, &travelled );

	if ( localMove != LOCALMOVE_VALID )
	{
		Stop();

		CBaseEntity *blocker = CBaseEntity::Instance( gpGlobals->trace_ent );

		if ( blocker )
		{
			DispatchBlocked( edict(), blocker->edict() );

			if ( m_moveWaitTime > 0 && blocker->IsMoving() && !blocker->IsPlayer() &&
				gpGlobals->time - m_flMoveWaitFinished > 3 )
			{
				if ( m_flightSpeed > travelled )
				{
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
					return;
				}

				localMove = LOCALMOVE_VALID;
			}
		}

		if ( localMove != LOCALMOVE_VALID )
		{
			Vector apex;

			for ( int i = m_iRouteIndex; i < ROUTE_SIZE; i++ )
			{
				if ( !FTriangulate( pev->origin, m_Route[i].vecLocation, distance, target, &apex ) )
					continue;

				InsertWaypoint( apex, bits_MF_TO_DETOUR );
				RouteSimplify( target );
				localMove = LOCALMOVE_VALID;
				break;
			}
		}

		if ( localMove != LOCALMOVE_VALID && blocker && !blocker->IsBSPModel() &&
			gpGlobals->time - 1 > m_flMoveWaitFinished && !HasMemory( bits_MEMORY_MOVE_FAILED ) )
		{
			m_flMoveWaitFinished = gpGlobals->time;
			localMove = LOCALMOVE_VALID;
		}

		if ( localMove != LOCALMOVE_VALID )
		{
			if ( m_moveWaitTime > 0 && !HasMemory( bits_MEMORY_MOVE_FAILED ) )
			{
				if ( FRefreshRoute() )
				{
					if ( gpGlobals->time - m_flMoveWaitFinished < 0.2f )
						Remember( bits_MEMORY_MOVE_FAILED );

					m_flMoveWaitFinished = gpGlobals->time + 0.1f;
					Stop();
				}
				else
					TaskFail("route is empty");
			}
			else
			{
				Stop();
				TaskFail("failed to move");
				m_flMoveWaitFinished = gpGlobals->time + 0.5f;
			}

			return;
		}
	}

	m_flGroundSpeed = m_flightSpeed;

	if ( ShouldAdvanceRoute( distance ) )
		AdvanceRoute( distance );

	MoveExecute( target, direction, interval );

	if ( MovementIsComplete() )
	{
		Stop();
		RouteClear();
	}
	/*else if ( distance < 256 )
	{
		CBaseEntity *goal = m_movementGoal == MOVEGOAL_ENEMY ? (CBaseEntity *) m_hEnemy : (CBaseEntity *) m_hTargetEnt;

		RouteSimplify( goal );
	}*/
}
