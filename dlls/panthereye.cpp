#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"followingmonster.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"clamp.h"

#define PANTHEREYE_AE_STRIKE_LEFT			( 1 )
#define PANTHEREYE_AE_STRIKE_RIGHT_LOW				( 2 )
#define PANTHEREYE_AE_STRIKE_RIGHT_HIGH				( 3 )

#define PANTHEREYE_MELEE_DISTANCE 84

Task_t tlPantherRangeAttack1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WAIT_RANDOM, (float)0.5 },
};

Schedule_t slPantherRangeAttack1[] =
{
	{
		tlPantherRangeAttack1,
		ARRAYSIZE( tlPantherRangeAttack1 ),
		bits_COND_ENEMY_OCCLUDED,
		0,
		"PantherRangeAttack1"
	},
};

class CPantherEye : public CFollowingMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("panthereye"); }
	void SetYawSpeed() override;
	int  DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Panthereye"; }

	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	void OnChangeSchedule(Schedule_t *pNewSchedule) override;
	void StartTask(Task_t *pTask) override;
	void RunTask(Task_t *pTask) override;

	LeapAttackStartParams GetDefaultLeapAttackStart() override {
		LeapAttackStartParams params;
		params.startFrameFraction = 120.0f / 255.0f;
		return params;
	}
	LeapAttackJumpParams GetDefaultLeapAttackJump() override {
		LeapAttackJumpParams params;
		params.delay = 3.5f;
		return params;
	}
	LeapAttackImpactParams GetDefaultLeapAttackImpact() override {
		LeapAttackImpactParams params;
		params.damageInfo = DamageInfo(GetSkillValue("panthereye_dmg_claw"), DMG_SLASH);
		params.punchAngle = Vector(5.0f, 0.0f, 15.0f);
		params.slowAfter = true;
		return params;
	}
	void PlayLeapAttackSound() override {
		EmitSoundScript(leapAttackSoundScript);
	}
	void PlayLeapAttackHitSound() override {
		EmitSoundScript(attackHitSoundScript);
	}

	void PlayUseSentence() override {
		EmitSoundScript(useSoundScript);
	}
	void PlayUnUseSentence() override {
		EmitSoundScript(unuseSoundScript);
	}

	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	bool CheckMeleeAttack2( float flDot, float flDist ) override {return false;}
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	bool CheckRangeAttack2( float flDot, float flDist ) override {return false;}

	void PerformStrike(const TraceHullAttackParams& params);

	int DefaultSizeForGrapple() override { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() override { return true; }

	Vector DefaultMinHullSize() override { return Vector( -24.0f, -24.0f, 0.0f ); }
	Vector DefaultMaxHullSize() override { return Vector( 24.0f, 24.0f, 64.0f ); }

	static constexpr const char* attackHitSoundScript = "PantherEye.AttackHit";
	static constexpr const char* attackMissSoundScript = "PantherEye.AttackMiss";

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript attackSoundScript;
	static constexpr const char* leapAttackSoundScript = "PantherEye.LeapAttack";

	static constexpr const char* useSoundScript = "PantherEye.Use";
	static constexpr const char* unuseSoundScript = "PantherEye.UnUse";

	void IdleSound() override {
		EmitSoundScript(idleSoundScript);
	}
	void AlertSound() override {
		EmitSoundScript(alertSoundScript);
	}
	PainSoundRule DefaultPainSoundRule() override {
		PainSoundRule rule;
		rule.chance = 1.0f/ 3.0f;
		return rule;
	}
	void PainSound() override {
		EmitSoundScript(painSoundScript);
	}
	void DeathSound() override {
		EmitSoundScript(dieSoundScript);
	}
};

LINK_ENTITY_TO_CLASS( monster_panthereye, CPantherEye );
LINK_ENTITY_TO_CLASS( monster_panther, CPantherEye );

const NamedSoundScript CPantherEye::idleSoundScript = {
	CHAN_VOICE,
	{"panthereye/pa_idle1.wav", "panthereye/pa_idle2.wav","panthereye/pa_idle3.wav", "panthereye/pa_idle4.wav"},
	IntRange(95, 105),
	"PantherEye.Idle"
};

const NamedSoundScript CPantherEye::alertSoundScript = {
	CHAN_VOICE,
	{},
	"PantherEye.Alert"
};

const NamedSoundScript CPantherEye::painSoundScript = {
	CHAN_VOICE,
	{},
	"PantherEye.Pain"
};

const NamedSoundScript CPantherEye::dieSoundScript = {
	CHAN_VOICE,
	{"panthereye/pa_death1.wav"},
	"PantherEye.Die"
};

const NamedSoundScript CPantherEye::attackSoundScript = {
	CHAN_VOICE,
	{"panthereye/pa_attack1.wav"},
	"PantherEye.Attack"
};

int CPantherEye::DefaultClassify()
{
	return CLASS_ALIEN_MONSTER;
}

void CPantherEye::SetYawSpeed()
{
	pev->yaw_speed = 90;
}

void CPantherEye::Spawn()
{
	Precache();

	SetMyModel("models/panthereye.mdl");
	SetMySize();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor(BLOOD_COLOR_YELLOW);
	SetMyHealth(GetSkillValue("panthereye_health"));
	SetMyFieldOfView(0.5f);
	m_MonsterState = MONSTERSTATE_NONE;
	SetMySquadCapabilities(bits_CAP_SQUAD|bits_CAP_SQUAD_SAME_CLASSNAME);

	FollowingMonsterInit();
}

void CPantherEye::Precache()
{
	PrecacheMyModel("models/panthereye.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(leapAttackSoundScript, attackSoundScript);

	RegisterAndPrecacheSoundScript(useSoundScript, idleSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript, alertSoundScript);
}

bool CPantherEye::CheckMeleeAttack1 ( float flDot, float flDist )
{
	CheckMeleeAttackParams params;
	params.distance = PANTHEREYE_MELEE_DISTANCE;
	return HasConditions(bits_COND_SEE_ENEMY) && CheckMeleeAttackImpl(flDot, flDist, params, false) && m_hEnemy != 0;
}

bool CPantherEye::CheckRangeAttack1( float flDot, float flDist )
{
	return FBitSet(pev->flags, FL_ONGROUND) && flDist > 80.0f && flDist <= 256 && flDot >= 0.65f && m_flNextLeapAttack <= gpGlobals->time;
}

void CPantherEye::PerformStrike(const TraceHullAttackParams& params)
{
	PerformTraceHullAttack( params );

	if (RANDOM_LONG(0,1))
		EmitSoundScript(attackSoundScript);
}

void CPantherEye::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch (pEvent->event)
	{
	case PANTHEREYE_AE_STRIKE_LEFT:
		{
			TraceHullAttackParams params;
			params.distance = PANTHEREYE_MELEE_DISTANCE;
			params.punchAngle.x = 5;
			params.punchAngle.z = 18.0f;
			params.knockRight = 100.0f;
			params.knockForward = -50.0f;
			params.knockUp = 50.0f;
			params.damageInfo.damage = GetSkillValue("panthereye_dmg_claw");
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformStrike(params);
			break;
		}

	case PANTHEREYE_AE_STRIKE_RIGHT_LOW:
		{
			TraceHullAttackParams params;
			params.distance = PANTHEREYE_MELEE_DISTANCE;
			params.punchAngle.x = 5;
			params.punchAngle.z = -9.0f;
			params.knockRight = -25.0f;
			params.knockForward = -25.0f;
			params.knockUp = 25.0f;
			params.damageInfo.damage = GetSkillValue("panthereye_dmg_claw");
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformStrike(params);
			break;
		}

	case PANTHEREYE_AE_STRIKE_RIGHT_HIGH:
		{
			TraceHullAttackParams params;
			params.distance = PANTHEREYE_MELEE_DISTANCE;
			params.punchAngle.x = 5;
			params.punchAngle.z = -18.0f;
			params.knockRight = -100.0f;
			params.knockForward = -50.0f;
			params.knockUp = 50.0f;
			params.damageInfo.damage = GetSkillValue("panthereye_dmg_claw");
			params.hitSoundScript = attackHitSoundScript;
			params.missSoundScript = attackMissSoundScript;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformStrike(params);
			break;
		}
	default:
		CFollowingMonster::HandleAnimEvent(pEvent);
		break;
	}
}

Schedule_t* CPantherEye::GetSchedule()
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	{
		Schedule_t* utilitySchedule = GetUtilitySchedule();
		if (utilitySchedule)
			return utilitySchedule;
	}
	default:
		break;
	}
	return CFollowingMonster::GetSchedule();
}

Schedule_t* CPantherEye::GetScheduleOfType(int Type)
{
	if (Type == SCHED_CHASE_ENEMY_FAILED)
	{
		if (HasMemory(bits_MEMORY_BLOCKER_IS_ENEMY))
			return CFollowingMonster::GetScheduleOfType(SCHED_CHASE_ENEMY);
		else if (m_flNextAttack <= gpGlobals->time && !HasConditions(bits_COND_ENEMY_OCCLUDED))
			return slPantherRangeAttack1;

	}
	if (Type == SCHED_RANGE_ATTACK1)
	{
		return slPantherRangeAttack1;
	}
	return CFollowingMonster::GetScheduleOfType(Type);
}

void CPantherEye::OnChangeSchedule(Schedule_t *pNewSchedule)
{
	CFollowingMonster::OnChangeSchedule(pNewSchedule);
	m_leaping = false;
}

void CPantherEye::StartTask(Task_t *pTask)
{
	CFollowingMonster::StartTask(pTask);

	if (pTask->iTask == TASK_RANGE_ATTACK1)
	{
		if (GetLeapAttackStart().setTouchEarly)
			SetLeapAttackTouch();
	}
}

void CPantherEye::RunTask(Task_t *pTask)
{
	CFollowingMonster::RunTask(pTask);

	if (pTask->iTask == TASK_RANGE_ATTACK1)
	{
		if (m_fSequenceFinished)
		{
			SetTouch(nullptr);
			m_leaping = false;
		}
		else if (!m_leaping && ReadyToLaunchLeapAttackAtCurrentFrame())
		{
			SetLeapAttackTouch();
			m_leaping = true;
			LaunchLeapAttack();
		}
	}
}
