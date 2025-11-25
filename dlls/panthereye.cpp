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

	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	bool CheckMeleeAttack2( float flDot, float flDist ) override {return false;}
	bool CheckRangeAttack1( float flDot, float flDist ) override {return false;}
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
	SetMyHealth(gSkillData.panthereyeHealth);
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
			params.damageInfo.damage = gSkillData.panthereyeDmgClaw;
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
			params.damageInfo.damage = gSkillData.panthereyeDmgClaw;
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
			params.damageInfo.damage = gSkillData.panthereyeDmgClaw;
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
	if (Type == SCHED_CHASE_ENEMY_FAILED && HasMemory(bits_MEMORY_BLOCKER_IS_ENEMY))
	{
		return CSquadMonster::GetScheduleOfType(SCHED_CHASE_ENEMY);
	}
	return CSquadMonster::GetScheduleOfType(Type);
}
