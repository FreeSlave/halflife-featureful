#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"game.h"
#include	"common_soundscripts.h"

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

class CPantherEye : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("panthereye"); }
	void SetYawSpeed() override;
	int  DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Panther Eye"; }

	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	Schedule_t* GetScheduleOfType(int Type) override;
	void OnChangeSchedule(Schedule_t *pNewSchedule) override;
	void RunTask(Task_t *pTask) override;

	void EXPORT LeapTouch ( CBaseEntity *pOther );

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

	bool m_leaping;
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

	MonsterInit();
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
}

bool CPantherEye::CheckMeleeAttack1 ( float flDot, float flDist )
{
	CheckMeleeAttackParams params;
	params.distance = PANTHEREYE_MELEE_DISTANCE;
	return HasConditions(bits_COND_SEE_ENEMY) && CheckMeleeAttackImpl(flDot, flDist, params, false) && m_hEnemy != 0;
}

bool CPantherEye::CheckRangeAttack1( float flDot, float flDist )
{
	return FBitSet(pev->flags, FL_ONGROUND) && flDist > 80.0f && flDist <= 256 && flDot >= 0.65f && m_flNextAttack <= gpGlobals->time;
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
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

Schedule_t* CPantherEye::GetScheduleOfType(int Type)
{
	if (Type == SCHED_CHASE_ENEMY_FAILED)
	{
		if (HasMemory(bits_MEMORY_BLOCKER_IS_ENEMY))
			return CSquadMonster::GetScheduleOfType(SCHED_CHASE_ENEMY);
		else if (m_flNextAttack <= gpGlobals->time && !HasConditions(bits_COND_ENEMY_OCCLUDED))
			return slPantherRangeAttack1;

	}
	if (Type == SCHED_RANGE_ATTACK1)
	{
		return slPantherRangeAttack1;
	}
	return CSquadMonster::GetScheduleOfType(Type);
}

void CPantherEye::OnChangeSchedule(Schedule_t *pNewSchedule)
{
	CSquadMonster::OnChangeSchedule(pNewSchedule);
	m_leaping = false;
}

void CPantherEye::RunTask(Task_t *pTask)
{
	CSquadMonster::RunTask(pTask);

	if (pTask->iTask == TASK_RANGE_ATTACK1)
	{
		if (m_fSequenceFinished)
		{
			SetTouch(nullptr);
			m_leaping = false;
		}
		else if (!m_leaping && pev->frame >= 120.0f)
		{
			SetTouch(&CPantherEye::LeapTouch);
			m_leaping = true;

			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );// take him off ground so engine doesn't instantly reset onground
			UTIL_MakeVectors( pev->angles );

			Vector vecJumpDir;
			if( m_hEnemy != 0 )
			{
				float gravity = g_psv_gravity->value;
				if( gravity <= 1 )
					gravity = 1;

				// How fast does the panther need to travel to reach that height given gravity?
				float height = m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z;
				if( height < 16 )
					height = 16;
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin;
				vecJumpDir *= ( 1.0f / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();

				if( distance > 650.0f )
				{
					vecJumpDir *= ( 650.0f / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350.0f;
			}

			pev->velocity = vecJumpDir;

			EmitSoundScript(attackSoundScript);
			m_flNextAttack = gpGlobals->time + 3.5f;
		}
	}
}

void CPantherEye::LeapTouch( CBaseEntity *pOther )
{
	if (!pOther->pev->takedamage)
	{
		return;
	}

	if (pOther->Classify() == Classify())
	{
		return;
	}

	EmitSoundScript(attackHitSoundScript);

	TouchAttackParams params;
	params.damageInfo = DamageInfo(GetSkillValue("panthereye_dmg_claw"), DMG_SLASH);
	SetTouchAttackFromTemplate(params);
	PerformTouchAttack(params, pOther);

	if (pOther->IsPlayer())
	{
		pOther->pev->punchangle.x = 5.0f;
		pOther->pev->punchangle.z = RANDOM_LONG(0, 1) ? 15.0f : -15.0f;
	}

	pev->velocity *= 0.5f;

	SetTouch(nullptr);
}
