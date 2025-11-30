#include "extdll.h"
#include "util.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "weapons.h"
#include "customentity.h"
#include "defaultai.h"
#include "soundent.h"
#include "effects.h"
#include "game.h"
#include "followingmonster.h"
#include "common_soundscripts.h"
#include "visuals_utils.h"

// TODO:
// range attack if can't reach target
// increase fps of most animations

#define EVENT_SLAM 1
#define EVENT_STAFF_SWING 2
#define EVENT_SHOOT 3
#define EVENT_SUMMON_GRUNT 4
#define EVENT_STAFF_STAB 7
#define EVENT_STEP_RIGHT 10
#define EVENT_STEP_LEFT 11

#define MELEE_ATTACK_DISTANCE 100
#define MELEE_ATTACK_CHECK_DISTANCE 88
#define MELEE_CHASE_DISTANCE 300

#define SLAM_CHECK_DISTANCE 150 // how close enemies need to be for a slam to be considered
#define SLAM_ATTACK_RADIUS 300 // radius of the attack
#define SLAM_ATTACK_ENEMY_COUNT 2 // minimum enemies nearby needed to do a slam

#define MAX_BEAM_SHOTS  5
#define TOR_SHOOT_RANGE 4096

#define SUMMON_DISTANCE 256.0f
#define SUMMON_HEIGHT 80.0f
#define MAX_ALLOWED_CHILDREN 3

#define SUMMON_CLASSNAME "monster_alien_grunt"

extern CBaseEntity* MakerBlocker(const Vector& mins, const Vector& maxs);

class CTorSummonPoint;

class CTor : public CFollowingMonster
{
public:
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("tor"); }
	void SetYawSpeed() override;
	int DefaultClassify() override {
		return CLASS_ALIEN_MILITARY;
	}
	const char* DefaultDisplayName() override {
		return "Tor";
	}
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	void StartTask(Task_t *pTask) override;

	void MonsterThink() override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckMeleeAttack2(float flDot, float flDist) override;
	int LookupActivity(int activity) override;
	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	void DeathNotice(entvars_t* pevChild) override;

	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-24, -24, 0);
		pev->absmax = pev->origin + Vector(24, 24, 88);
	}

	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;
	void DeathSound() override;
	void AlertSound() override;
	void IdleSound() override;

	void PlayUseSentence() override;
	void PlayUnUseSentence() override;

	CUSTOM_SCHEDULES

private:
	void SlamAttack();
	bool GetSummonPos(Vector& pos);
	void StartSummon();

	float m_nextSlam;
	int m_shotsFired;
	float m_nextShoot; // next time allowed to begin shooting
	float m_nextBeamBurst; // next time a burst shot will be fired
	float m_nextBeam; // next time a single beam will be fired
	int m_burstShotsFired; // number of shots fired in the current burst
	int m_failedMelees; // don't keep meleeing if the player is moving back and forth to avoid it
	float m_nextSummon; // next time a grunt can be spawned
	float m_nextSummonCheck;
	int m_numChildren;
	Vector m_summonMinSize;
	Vector m_summonMaxSize;

	friend class CTorSummonPoint;

protected:
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript leftFootSoundScript;
	static const NamedSoundScript rightFootSoundScript;
	static const NamedSoundScript slamSoundScript;
	static const NamedSoundScript shootSoundScript;

	static constexpr const char* attackHitSoundScript = "Tor.AttackHit";
	static constexpr const char* attackMissSoundScript = "Tor.AttackMiss";
	static constexpr const char* useSoundScript = "Tor.Use";
	static constexpr const char* unuseSoundScript = "Tor.UnUse";

	static const NamedSoundScript summonSoundScript;
	static const NamedSoundScript summonPortalSoundScript;
	static const NamedSoundScript summonSpawnSoundScript;

	static const NamedVisual slamVisual;
	static const NamedVisual beamVisual;
	static const NamedVisual beam2Visual;
	static const NamedVisual summonBeamVisual;
	static const NamedVisual summonSpriteVisual;
};

LINK_ENTITY_TO_CLASS(monster_alien_tor, CTor)

TYPEDESCRIPTION	CTor::m_SaveData[] =
{
	DEFINE_FIELD( CTor, m_nextSlam, FIELD_TIME ),
	DEFINE_FIELD( CTor, m_shotsFired, FIELD_INTEGER ),
	DEFINE_FIELD( CTor, m_nextShoot, FIELD_TIME ),
	DEFINE_FIELD( CTor, m_nextBeamBurst, FIELD_TIME ),
	DEFINE_FIELD( CTor, m_nextBeam, FIELD_TIME ),
	DEFINE_FIELD( CTor, m_burstShotsFired, FIELD_INTEGER ),
	DEFINE_FIELD( CTor, m_failedMelees, FIELD_INTEGER ),
	DEFINE_FIELD( CTor, m_nextSummon, FIELD_TIME ),
	DEFINE_FIELD( CTor, m_nextSummonCheck, FIELD_TIME ),
	DEFINE_FIELD( CTor, m_numChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CTor, m_summonMinSize, FIELD_VECTOR ),
	DEFINE_FIELD( CTor, m_summonMaxSize, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CTor, CFollowingMonster )

class CTorSummonPoint : public CPointEntity
{
public:
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void EXPORT SummonThink();

	EHANDLE m_sprite;
	EHANDLE m_torHandle;
};

LINK_ENTITY_TO_CLASS(env_tor_summon_point, CTorSummonPoint)

TYPEDESCRIPTION	CTorSummonPoint::m_SaveData[] =
{
	DEFINE_FIELD( CTorSummonPoint, m_sprite, FIELD_EHANDLE ),
	DEFINE_FIELD( CTorSummonPoint, m_torHandle, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CTorSummonPoint, CPointEntity )

const NamedSoundScript CTor::attackSoundScript = {
	CHAN_VOICE,
	{"tor/tor-attack1.wav", "tor/tor-attack2.wav"},
	IntRange(95, 105),
	"Tor.Attack"
};

const NamedSoundScript CTor::idleSoundScript = {
	CHAN_VOICE,
	{"tor/tor-idle.wav", "tor/tor-idle2.wav", "tor/tor-idle3.wav"},
	IntRange(95, 105),
	"Tor.Idle"
};

const NamedSoundScript CTor::alertSoundScript = {
	CHAN_VOICE,
	{"tor/tor-alerted.wav"},
	IntRange(100, 109),
	"Tor.Alert"
};

const NamedSoundScript CTor::painSoundScript = {
	CHAN_VOICE,
	{"tor/tor-pain.wav", "tor/tor-pain2.wav"},
	IntRange(100, 109),
	"Tor.Pain"
};

const NamedSoundScript CTor::dieSoundScript = {
	CHAN_VOICE,
	{"tor/tor-die.wav", "tor/tor-die2.wav"},
	"Tor.Die"
};

const NamedSoundScript CTor::leftFootSoundScript = {
	CHAN_BODY,
	{"tor/tor-foot.wav"},
	IntRange(90, 110),
	"Tor.LeftFoot"
};

const NamedSoundScript CTor::rightFootSoundScript = {
	CHAN_BODY,
	{"tor/tor-foot.wav"},
	IntRange(90, 110),
	"Tor.RightFoot"
};

const NamedSoundScript CTor::slamSoundScript = {
	CHAN_WEAPON,
	{"houndeye/he_blast1.wav", "houndeye/he_blast2.wav", "houndeye/he_blast3.wav"},
	IntRange(95, 105),
	"Tor.Slam"
};

const NamedSoundScript CTor::shootSoundScript = {
	CHAN_WEAPON,
	{"tor/tor-staff-discharge.wav"},
	"Tor.Shoot"
};

const NamedSoundScript CTor::summonSoundScript = {
	CHAN_WEAPON,
	{"tor/tor-summon.wav"},
	"Tor.Summon"
};

const NamedSoundScript CTor::summonPortalSoundScript = {
	CHAN_ITEM,
	{"debris/beamstart8.wav"},
	"Tor.SummonPortal"
};

const NamedSoundScript CTor::summonSpawnSoundScript = {
	CHAN_ITEM,
	{"debris/beamstart7.wav"},
	"Tor.SummonSpawn"
};

const NamedVisual CTor::slamVisual = BuildVisual("Tor.Slam")
	.Model("sprites/shockwave.spr")
	.Life(0.2f)
	.BeamWidth(12)
	.RenderColor(255, 255, 255)
	.Alpha(255);

const NamedVisual CTor::beamVisual = BuildVisual("Tor.Beam")
	.Model("sprites/xenobeam.spr")
	.RenderColor(96, 128, 16)
	.Alpha(150)
	.BeamWidth(50)
	.BeamNoise(10)
	.BeamScrollRate(150)
	.Life(0.5f);

const NamedVisual CTor::beam2Visual = BuildVisual("Tor.Beam2")
	.Model("sprites/xenobeam.spr")
	.RenderColor(96, 255, 16)
	.Alpha(150)
	.BeamWidth(50)
	.BeamNoise(15)
	.BeamScrollRate(150)
	.BeamFlags(BEAM_FSINE)
	.Life(0.5f);

const NamedVisual CTor::summonBeamVisual = BuildVisual("Tor.SummonBeam")
	.Model("sprites/xenobeam.spr")
	.Life(2.0f)
	.BeamFlags(BEAM_FSHADEOUT)
	.RenderColor(96, 255, 32)
	.Alpha(80)
	.BeamNoise(80)
	.BeamWidth(30);

const NamedVisual CTor::summonSpriteVisual = BuildVisual("Tor.SummonSprite")
	.Model("sprites/exit1.spr")
	.Scale(2.0f)
	.RenderMode(kRenderTransAdd)
	.RenderColor(255, 255, 255)
	.Alpha(128)
	.Framerate(10.0f);

Task_t tlMeleeAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_MELEE_ATTACK2, (float)0 },
};

Schedule_t slMeleeAttack[] =
{
	{
		tlMeleeAttack,
		ARRAYSIZE( tlMeleeAttack ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Tor Melee Attack"
	},
};

Task_t tlShootAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slShootAttack[] =
{
	{
		tlShootAttack,
		ARRAYSIZE( tlShootAttack ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Tor Shoot Attack"
	},
};

Task_t	tlSummonAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_RANGE_ATTACK2, (float)0 },
};

Schedule_t	slSummonAttack[] =
{
	{
		tlSummonAttack,
		ARRAYSIZE(tlSummonAttack),
		0,
		0,
		"Tor Summon Attack"
	},
};

// uninterruptable melee attack
Task_t	tlSlamAttack[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slSlamAttack[] =
{
	{
		tlSlamAttack,
		ARRAYSIZE(tlSlamAttack),
		0,
		0,
		"Tor Slam Attack"
	},
};

DEFINE_CUSTOM_SCHEDULES(CTor)
{
	slMeleeAttack,
	slShootAttack,
	slSummonAttack,
	slSlamAttack
};

IMPLEMENT_CUSTOM_SCHEDULES(CTor, CFollowingMonster)

void CTor::SetYawSpeed()
{
	pev->yaw_speed = 180;
}

void CTor::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case EVENT_SLAM:
		SlamAttack();
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_VOLUME, 0.3f);
		break;
	case EVENT_SHOOT:
		m_nextBeam = m_nextBeamBurst = gpGlobals->time;

		// pause sequence to prevent triggering more of these events
		pev->framerate = 0.001f;
		m_fSequenceFinished = false;

		m_shotsFired++;
		if (m_shotsFired >= MAX_BEAM_SHOTS) {
			m_shotsFired = 0;
			m_nextShoot = gpGlobals->time + 3.0f;
		}
		m_failedMelees = 0;
		break;
	case EVENT_SUMMON_GRUNT:
		if (m_nextSummon < gpGlobals->time)
			StartSummon();
		break;
	case EVENT_STAFF_SWING:
	{
		TraceHullAttackParams params;
		params.damageInfo = DamageInfo{gSkillData.torDmgPunch, DMG_SLASH};
		params.distance = MELEE_ATTACK_DISTANCE;
		params.punchAngle.x = 5;
		params.punchAngle.z = 18;

		params.knockForward = 200.0f;
		params.knockRight = 200.0f;
		params.knockUp = 200.0f;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
		CBaseEntity* pHurt = PerformTraceHullAttack(params);
		if (pHurt)
			m_failedMelees = 0;
		else
			m_failedMelees++;
		break;
	}
	case EVENT_STAFF_STAB:
	{
		TraceHullAttackParams params;
		params.damageInfo = DamageInfo{gSkillData.torDmgPunch, DMG_SLASH};
		params.distance = MELEE_ATTACK_DISTANCE;
		params.punchAngle.x = 18;
		params.knockForward = 100;
		params.hitSoundScript = attackHitSoundScript;
		params.missSoundScript = attackMissSoundScript;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);
		CBaseEntity* pHurt = PerformTraceHullAttack(params);
		if (pHurt)
			m_failedMelees = 0;
		else
			m_failedMelees++;
		break;
	}
	case EVENT_STEP_LEFT:
		EmitSoundScript(leftFootSoundScript);
		break;
	case EVENT_STEP_RIGHT:
		EmitSoundScript(rightFootSoundScript);
		break;
	default:
		CFollowingMonster::HandleAnimEvent(pEvent);
		break;
	}
}

Schedule_t* CTor::GetSchedule()
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
		{
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
	{
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

Schedule_t* CTor::GetScheduleOfType(int Type)
{
	switch (Type) {
	case SCHED_MELEE_ATTACK1:
		return slSlamAttack;
	case SCHED_MELEE_ATTACK2:
		return slMeleeAttack;
	case SCHED_RANGE_ATTACK1:
		return slShootAttack;
	case SCHED_RANGE_ATTACK2:
		return slSummonAttack;
	default:
		return CFollowingMonster::GetScheduleOfType(Type);
	}
}

void CTor::StartTask(Task_t *pTask)
{
	if (pTask->iTask == TASK_MELEE_ATTACK2)
	{
		EmitSoundScript(attackSoundScript);
	}
	CFollowingMonster::StartTask(pTask);
}

void CTor::MonsterThink()
{
	if (m_nextBeamBurst && m_nextBeamBurst < gpGlobals->time) {
		pev->framerate = 0.001f;

		if (m_nextBeam < gpGlobals->time) {
			m_nextBeam = gpGlobals->time + 0.05;
			m_burstShotsFired++;
			EmitSoundScript(shootSoundScript);
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_VOLUME, 0.3f);

			Vector vecSrc, angles;
			GetAttachment(0, vecSrc, angles);
			CBaseEntity* target = m_hEnemy;

			if (target) {
				Vector vecDir1 = (target->BodyTarget(pev->origin) - vecSrc).Normalize();

				TraceResult	tr;
				UTIL_TraceLine(vecSrc, vecSrc + vecDir1 * TOR_SHOOT_RANGE, dont_ignore_monsters, edict(), &tr);

				const Visual* pBeamVisual = GetVisual(beamVisual);
				CBeam* beam = CreateBeamFromVisual(pBeamVisual);
				if (beam)
				{
					beam->PointsInit(vecSrc, tr.vecEndPos);
					beam->pev->spawnflags |= SF_BEAM_TEMPORARY;
					beam->LiveForTime(RandomizeNumberFromRange(pBeamVisual->life));
				}

				const Visual* pBeam2Visual = GetVisual(beam2Visual);
				CBeam* beam2 = CreateBeamFromVisual(pBeam2Visual);
				if (beam2)
				{
					beam2->PointsInit(vecSrc, tr.vecEndPos);
					beam2->pev->spawnflags |= SF_BEAM_TEMPORARY;
					beam2->LiveForTime(RandomizeNumberFromRange(pBeam2Visual->life));
				}

				CBaseEntity* phit = CBaseEntity::Instance(tr.pHit);
				if (phit) {
					phit->TakeDamage(pev, pev, DamageInfo{gSkillData.torDmgEnergyBeam, DMG_ENERGYBEAM});

					if (phit->MyMonsterPointer() && (phit->pev->movetype == MOVETYPE_STEP || phit->IsPlayer())) {
						phit->pev->velocity.z += (phit->pev->flags & FL_ONGROUND) ? 200 : 120;
					}
				}
			}

			if (m_burstShotsFired >= 3) {
				m_nextBeam = 0;
				m_nextBeamBurst = 0;
				m_burstShotsFired = 0;

				// finish the shoot task
				pev->framerate = 1.0f;
				m_fSequenceFinished = true;
			}
		}
	}
	CFollowingMonster::MonsterThink();
}

bool CTor::CheckRangeAttack1(float flDot, float flDist)
{
	bool shouldMelee = flDist < MELEE_CHASE_DISTANCE && m_failedMelees < 2;
	if (!shouldMelee && flDist < TOR_SHOOT_RANGE) {
		if (gpGlobals->time > m_nextShoot)
			return true;
	}

	return false;
}

bool CTor::CheckRangeAttack2(float flDot, float flDist)
{
	if (m_numChildren < MAX_ALLOWED_CHILDREN && m_nextSummon < gpGlobals->time && m_nextSummonCheck < gpGlobals->time)
	{
		m_nextSummonCheck = gpGlobals->time + (HasConditions(bits_COND_ENEMY_OCCLUDED) ? 0.5f : 1.0f);
		Vector dummy;
		return GetSummonPos(dummy);
	}
	return false;
}

bool CTor::CheckMeleeAttack1(float flDot, float flDist)
{
	if (m_nextSlam > gpGlobals->time)
		return false;

	int nearbyEnemies = 0;

	CBaseEntity* pEntity = NULL;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, SLAM_CHECK_DISTANCE)) != NULL)
	{
		if (pEntity->MyMonsterPointer() && pEntity->IsAlive() && IRelationship(pEntity) >= R_DL)
		{
			if (pEntity->IsPlayer())
				return true;
			nearbyEnemies++;
			if (nearbyEnemies >= SLAM_ATTACK_ENEMY_COUNT) {
				return true;
			}
		}
	}

	return false;
}

bool CTor::CheckMeleeAttack2(float flDot, float flDist)
{
	CheckMeleeAttackParams params;
	params.distance = MELEE_ATTACK_CHECK_DISTANCE;
	return CheckMeleeAttackImpl(flDot, flDist, params, true);
}

int CTor::LookupActivity(int activity)
{
	ASSERT(activity != 0);
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	/*switch (activity) {
	case ACT_RANGE_ATTACK1:
	{
		return ::LookupActivityWithOffset(pmodel, pev, activity, 1);
	}
	default:*/
		return ::LookupActivity(pmodel, pev, activity);
	//}
}

DamageInfo CTor::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	DamageInfo damageInfo = inputDamageInfo;

	if (ptr->iHitgroup == 10 && (damageInfo.type & (DMG_BULLET | DMG_SLASH | DMG_CLUB)))
	{
		// hit armor
		if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1.0f, 2.0f));
			pev->dmgtime = gpGlobals->time;
		}

		if ((damageInfo.type & DMG_BULLET) && RANDOM_LONG(0, 1) == 0)
		{
			Vector vecTracerDir = vecDir;

			vecTracerDir.x += RANDOM_FLOAT(-0.3f, 0.3f);
			vecTracerDir.y += RANDOM_FLOAT(-0.3f, 0.3f);
			vecTracerDir.z += RANDOM_FLOAT(-0.3f, 0.3f);

			vecTracerDir *= -512.0f;

			Vector vecTracerEnd = ptr->vecEndPos + vecTracerDir;

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos );
			WRITE_BYTE( TE_TRACER );
			WRITE_VECTOR( ptr->vecEndPos );
			WRITE_VECTOR( vecTracerEnd );
			MESSAGE_END();
		}

		damageInfo.damage -= 20.0f;
		if (damageInfo.damage <= 0.0f)
			damageInfo.damage = 0.1f;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated

		ptr->iHitgroup = HITGROUP_GENERIC;
		damageInfo.SetNoBlood();
	}

	return damageInfo;
}

void CTor::Spawn()
{
	Precache();

	SetMyModel("models/Tor.mdl");
	SetMySize(Vector(-24, -24, 0), Vector(24, 24, 72));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( gSkillData.torHealth );
	pev->view_ofs = Vector(0, 0, 0);// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE);
	m_MonsterState = MONSTERSTATE_NONE;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(true);

	FollowingMonsterInit();
}

void CTor::Precache()
{
	CFollowingMonster::Precache();

	PrecacheMyModel("models/Tor.mdl");

	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(leftFootSoundScript);
	RegisterAndPrecacheSoundScript(rightFootSoundScript);
	RegisterAndPrecacheSoundScript(slamSoundScript);
	RegisterAndPrecacheSoundScript(shootSoundScript);
	RegisterAndPrecacheSoundScript(summonSoundScript);
	RegisterAndPrecacheSoundScript(summonPortalSoundScript);
	RegisterAndPrecacheSoundScript(summonSpawnSoundScript);

	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
	RegisterAndPrecacheSoundScript(useSoundScript, idleSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript, alertSoundScript);

	RegisterVisual(slamVisual);
	RegisterVisual(beamVisual);
	RegisterVisual(beam2Visual);
	RegisterVisual(summonBeamVisual);
	RegisterVisual(summonSpriteVisual);

	PrecacheChildren(SUMMON_CLASSNAME, m_reverseRelationship, &m_summonMinSize, &m_summonMaxSize);

	m_shotsFired = m_burstShotsFired = 0;
}

PainSoundRule CTor::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = 0.6f;
	return rule;
}

void CTor::PainSound()
{
	EmitSoundScript(painSoundScript);
}

void CTor::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

void CTor::AlertSound()
{
	EmitSoundScript(alertSoundScript);
}

void CTor::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

void CTor::PlayUseSentence() {
	EmitSoundScript(useSoundScript);
}

void CTor::PlayUnUseSentence() {
	EmitSoundScript(unuseSoundScript);
}

void CTor::SlamAttack()
{
	m_nextSlam = gpGlobals->time + RANDOM_FLOAT(3.5f, 5.5f);

	CBaseEntity* pEntity = NULL;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, SLAM_ATTACK_RADIUS)) != NULL)
	{
		if (pEntity->MyMonsterPointer() && pEntity->IsAlive() && pEntity->entindex() != entindex() && IRelationship(pEntity) >= R_DL)
		{
			const Vector delta = pEntity->pev->origin - pev->origin;
			Vector pushDir = delta.Normalize();
			pushDir.z = 0;

			float launchPower = 1.0f - ((delta.Length() - 64) * 0.5f / SLAM_ATTACK_RADIUS);
			launchPower = Q_max(0.7f, launchPower);
			float pushPower = 1.0f - launchPower;

			const Vector launchForce = Vector(0, 0, 1) * 500 * launchPower;
			const Vector pushForce = pushDir * 700 * pushPower;

			pEntity->pev->velocity = pEntity->pev->velocity + launchForce + pushForce;
			pEntity->TakeDamage(pev, pev, DamageInfo{gSkillData.torDmgSonicBlast * launchPower, DMG_SONIC});

			if (pEntity->IsPlayer()) {
				pEntity->pev->punchangle.x = 10;
			}
		}
	}

	EmitSoundScript(slamSoundScript);

	float radius = (SLAM_ATTACK_RADIUS + 50) / 0.3f;

	const Visual* visual = GetVisual(slamVisual);

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_BEAMCYLINDER );
	WRITE_CIRCLE( pev->origin, radius );
	WriteBeamVisual(visual);
	MESSAGE_END();
}

bool CTor::GetSummonPos(Vector& pos)
{
	const int attempts = 8;

	const Vector startPos = pev->origin + Vector(0, 0, pev->maxs.z - pev->mins.z + 2.0f);

	for (int i = 0; i < attempts; i++) {
		// random point on circle
		float c = RANDOM_FLOAT(0, 2*M_PI);
		float x = cos(c) * SUMMON_DISTANCE;
		float y = sin(c) * SUMMON_DISTANCE;
		float z = startPos.z + (m_summonMaxSize.z - m_summonMinSize.z)*0.5f + RANDOM_FLOAT(0.0f, SUMMON_HEIGHT);

		const Vector checkPos = pev->origin + Vector(x, y, z);

		TraceResult tr;
		UTIL_TraceHull(startPos, checkPos, dont_ignore_monsters, large_hull, edict(), &tr);

		if (tr.fStartSolid || tr.flFraction < 0.5f) {
			continue;
		}

		// move a little bit inward to avoid spawning inside a wall/ceiling
		const Vector dir = (checkPos - pev->origin).Normalize();
		pos = tr.vecEndPos - dir*40;

		const Vector mins = pos + m_summonMinSize;
		const Vector maxs = pos + m_summonMaxSize;

		UTIL_TraceLine(mins, maxs, dont_ignore_monsters, nullptr, &tr);
		if (tr.flFraction == 1.0f && MakerBlocker(mins, maxs) == nullptr)
			return true;
	}

	return false;
}

void CTor::StartSummon()
{
	Vector summonPos;
	if (!GetSummonPos(summonPos)) {
		ALERT(at_aiconsole, "%s: failed to find a summon position\n", STRING(pev->classname));
		return;
	}

	m_nextSummon = gpGlobals->time + RANDOM_FLOAT(5.0f, 10.0f);

	EmitSoundScript(summonSoundScript);

	Vector startPos = pev->origin;
	startPos.z += (pev->maxs.z - pev->mins.z) * 0.75f;

	const Visual* beamVisual = GetVisual(summonBeamVisual);
	for (int i = 0; i < 3; i++)
	{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_VECTOR(startPos);
		WRITE_VECTOR(summonPos);
		WriteBeamVisual(beamVisual);
		MESSAGE_END();
	}

	CSprite* portalSprite = CreateSpriteFromVisual(GetVisual(summonSpriteVisual), summonPos);
	if (portalSprite)
		portalSprite->TurnOn();

	CTorSummonPoint* summonPoint = GetClassPtr((CTorSummonPoint*)nullptr);
	summonPoint->pev->classname = MAKE_STRING("env_tor_summon_point");
	UTIL_SetOrigin(summonPoint->pev, summonPos);
	summonPoint->m_sprite = portalSprite;
	summonPoint->m_torHandle = this;
	summonPoint->SetThink(&CTorSummonPoint::SummonThink);
	summonPoint->pev->nextthink = gpGlobals->time + 2.0f;
	summonPoint->pev->dmgtime = gpGlobals->time;

	summonPoint->EmitSoundScript(GetSoundScript(summonPortalSoundScript));
}

void CTorSummonPoint::SummonThink()
{
	auto removeSelf = [&]() {
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1f;

		CSprite* pSprite = m_sprite.Entity<CSprite>();
		if (pSprite)
		{
			pSprite->AnimateAndDie(Q_max(pSprite->pev->framerate, 10.0f));
		}
	};

	CBaseEntity* pOwner = m_torHandle;

	const char* removalReason = nullptr;

	if (!pOwner)
	{
		removalReason = "no owner";
	}
	else if (!FStrEq(STRING(pOwner->pev->classname), "monster_alien_tor"))
	{
		removalReason = "owner is not a Tor";
	}
	else if (pev->dmgtime + 3.0f < gpGlobals->time)
	{
		removalReason = "stuck for too long";
	}
	if (removalReason)
	{
		ALERT(at_aiconsole, "%s is going to be removed. Reason: %s\n", STRING(pev->classname), removalReason);
		removeSelf();
		return;
	}

	CTor* pTor = (CTor*)pOwner;
	const Vector mins = pev->origin + pTor->m_summonMinSize;
	const Vector maxs = pev->origin + pTor->m_summonMaxSize;

	if (MakerBlocker(mins, maxs) != nullptr)
	{
		pev->nextthink = gpGlobals->time + 0.5f;
		return;
	}

	ChildVariantHandle childVariant = pTor->SelectChildVariant(SUMMON_CLASSNAME);

	CBaseEntity* ent = CreateNoSpawn(childVariant.classname, pev->origin, pTor->pev->angles, pOwner->edict());
	if (!ent)
	{
		ALERT(at_console, "%s is going to be removed. Reason: can't spawn a child '%s'\n", childVariant.classname);
		removeSelf();
		return;
	}

	ent->FillKeyValues(childVariant.parameters);
	CBaseMonster* mon = ent->MyMonsterPointer();
	if (mon)
	{
		SetBits(ent->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND);
		pTor->FixChildClassify(mon);
	}

	if (DispatchSpawnAutoClean(ent))
	{
		if (mon)
		{
			mon->PushEnemy(pTor->m_hEnemy, pTor->m_vecEnemyLKP);
		}

		EmitSoundScript(pOwner->GetSoundScript(CTor::summonSpawnSoundScript));
		pTor->m_numChildren++;
	}

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1f;

	CSprite* pSprite = m_sprite.Entity<CSprite>();
	if (pSprite)
	{
		pSprite->SetThink(&CBaseEntity::SUB_Remove);
		pSprite->pev->nextthink = gpGlobals->time;
	}
}

void CTor::DeathNotice(entvars_t* pevChild) {
	m_numChildren--;
	if (m_numChildren < 0) {
		m_numChildren = 0;
	}
}
