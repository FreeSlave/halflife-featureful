#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"mod_features.h"

#if FEATURE_PANTHEREYE

#define PANTHEREYE_AE_STRIKE_LEFT			( 1 )
#define PANTHEREYE_AE_STRIKE_RIGHT_LOW				( 2 )
#define PANTHEREYE_AE_STRIKE_RIGHT_HIGH				( 3 )

#define PANTHEREYE_MELEE_DISTANCE 84

struct PantherStrikeParams
{
	float forwardDistance = PANTHEREYE_MELEE_DISTANCE;
	float punchz = 0.0f;
	float velRightScalar = 0.0f;
	float velForwardScalar = 0.0f;
	float velUpScalar = 0.0f;
};

class CPantherEye : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("panthereye"); }
	void SetYawSpeed();
	int  DefaultClassify();
	const char* DefaultDisplayName() { return "Panther Eye"; }

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	Schedule_t* GetScheduleOfType(int Type);

	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	bool CheckMeleeAttack2( float flDot, float flDist ) override {return false;}
	bool CheckRangeAttack1( float flDot, float flDist ) override {return false;}
	bool CheckRangeAttack2( float flDot, float flDist ) override {return false;}

	void PerformStrike(const PantherStrikeParams& params);

	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int DefaultSizeForGrapple() { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() { return true; }

	Vector DefaultMinHullSize() { return Vector( -24.0f, -24.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 24.0f, 24.0f, 64.0f ); }

	static constexpr const char* attackHitSoundScript = "PantherEye.AttackHit";
	static constexpr const char* attackMissSoundScript = "PantherEye.AttackMiss";

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript attackSoundScript;

	void IdleSound() {
		EmitSoundScript(idleSoundScript);
	}
	void AlertSound() {
		EmitSoundScript(alertSoundScript);
	}
	void PainSound() {
		if(RANDOM_LONG(0, 5) < 2)
			EmitSoundScript(painSoundScript);
	}
	void DeathSound() {
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

void CPantherEye::SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

void CPantherEye::Spawn()
{
	Precache();

	SetMyModel("models/panthereye.mdl");
	SetMySize(DefaultMinHullSize(), DefaultMaxHullSize());

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor(BLOOD_COLOR_YELLOW);
	SetMyHealth(gSkillData.panthereyeHealth);
	SetMyFieldOfView(0.5f);
	m_MonsterState = MONSTERSTATE_NONE;

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
	if ( HasConditions(bits_COND_SEE_ENEMY) && flDist <= PANTHEREYE_MELEE_DISTANCE && flDot >= 0.7f && m_hEnemy != 0 )
	{
		return true;
	}
	return false;
}

void CPantherEye::PerformStrike(const PantherStrikeParams& params)
{
	CBaseEntity *pHurt = CheckTraceHullAttack( params.forwardDistance, gSkillData.panthereyeDmgClaw, DMG_SLASH );
	if ( pHurt )
	{
		if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
		{
			if (params.punchz) {
				pHurt->pev->punchangle.z = params.punchz;
			}
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity +
					gpGlobals->v_right * params.velRightScalar +
					gpGlobals->v_forward * params.velForwardScalar +
					gpGlobals->v_up * params.velUpScalar;
		}
		EmitSoundScript(attackHitSoundScript);
	}
	else
		EmitSoundScript(attackMissSoundScript);

	if (RANDOM_LONG(0,1))
		EmitSoundScript(attackSoundScript);
}

void CPantherEye::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevInflictor, pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CPantherEye::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch (pEvent->event)
	{
	case PANTHEREYE_AE_STRIKE_LEFT:
		{
			PantherStrikeParams params;
			params.punchz = 18.0f;
			params.velRightScalar = 100.0f;
			params.velForwardScalar = -50.0f;
			params.velUpScalar = 50.0f;

			PerformStrike(params);
			break;
		}

	case PANTHEREYE_AE_STRIKE_RIGHT_LOW:
		{
			PantherStrikeParams params;
			params.punchz = -9.0f;
			params.velRightScalar = -25.0f;
			params.velForwardScalar = -25.0f;
			params.velUpScalar = 25.0f;

			PerformStrike(params);
			break;
		}

	case PANTHEREYE_AE_STRIKE_RIGHT_HIGH:
		{
			PantherStrikeParams params;
			params.punchz = -18.0f;
			params.velRightScalar = -100.0f;
			params.velForwardScalar = -50.0f;
			params.velUpScalar = 50.0f;

			PerformStrike(params);
			break;
		}
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

Schedule_t* CPantherEye::GetScheduleOfType(int Type)
{
	if (Type == SCHED_CHASE_ENEMY_FAILED && HasMemory(bits_MEMORY_BLOCKER_IS_ENEMY))
	{
		return CBaseMonster::GetScheduleOfType(SCHED_CHASE_ENEMY);
	}
	return CBaseMonster::GetScheduleOfType(Type);
}
#endif
