//=========================================================
// archer - funny fish from the outer world
// Implementation from Thrill
//=========================================================

#include	"float.h"
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"animation.h"
#include	"visuals_utils.h"
#include	"game.h"
#include	"studio.h"

// Animation events
#define ARCHER_AE_ATTACK		1
#define ARCHER_BEAM_ATTACK_ORIG	2
#define ARCHER_BEAM_ATTACK_THRILL	3

// Movement constants

#define		ARCHER_ACCELERATE		20
#define		ARCHER_SWIM_SPEED		100.0f
#define		ARCHER_SWIM_ACCEL		100.0f
#define		ARCHER_SWIM_DECEL		50.0f
#define		ARCHER_TURN_RATE		100.0f
#define		ARCHER_FRAMETIME		0.1f
#define		ARCHER_ATTACK_DISTANCE 260.0f
#define		ARCHER_BEAM_DISTANCE 360.0f

class CArcher : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void CheckForAttachments()
	{
		if (pev->modelindex)
		{
			void *pmodel = GET_MODEL_PTR(edict());
			if (pmodel)
			{
				studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
				m_hasAttachment = pstudiohdr->numattachments >= 1;
			}
		}
	}
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("archer"); }
	const char* DefaultDisplayName() override { return "Archer"; }

	void EXPORT SwimThink();
	void EXPORT DeadThink();

	/*void SetObjectCollisionBox() override
	{
		SetMyObjectCollisionBox(Vector(-32,-12,0), Vector(32,12,12));
	}*/
	int DefaultSizeForGrapple() override { return GRAPPLE_SMALL; }
	bool IsDisplaceable() override { return true; }
	Vector DefaultMinHullSize() override {
		return Vector(-12, -12, 0);
	}
	Vector DefaultMaxHullSize() override {
		return Vector(12, 12, 12);
	}

	void IdleSound() override {
		EmitSoundScript(idleSoundScript);
	}
	void AlertSound() override {
		EmitSoundScript(alertSoundScript);
		m_attackSoundTime = gpGlobals->time + RANDOM_FLOAT(1.5f, 3.0f);
	}
	void PainSound() override {
		EmitSoundScript(painSoundScript);
	}
	void DeathSound() override {
		EmitSoundScript(dieSoundScript);
	}
	void UpdateMotion(float turnSpeed, float obstacleFactor);
	float ForwardCheckDistance() {
		return pev->size.x * 3.5f;
	}
	float ObstacleDistance( CBaseEntity *pTarget );
	void MakeVectors();
	void RecalculateWaterlevel();
	void SwitchArcherState();

	int LookupActivity(int activity) override;
	void SetActivity(Activity NewActivity) override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib );
	void Activate() override;
	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;
	int	DefaultClassify() override { return CLASS_ALIEN_MONSTER; }

	void BeamAttack(const Vector& vecBeamPos);
	Vector GetBeamStartPos();

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	static const NamedSoundScript biteSoundScript;
	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript zapPowerupSoundScript;
	static const NamedSoundScript electroSoundScript;

	static const NamedVisual zapBeamVisual;
	static const NamedVisual powerupLightVisual;
	static const NamedVisual zapLightVisual;

private:
	float	m_flTurning;
	float	m_flAccelerate;
	float	m_top;
	float	m_bottom;
	float	m_height;
	float	m_waterTime;
	float	m_sideTime;
	float	m_zTime;
	float	m_stateTime;
	float	m_attackSoundTime;
	float	m_lastAttackTime;
	bool	m_onAttack;
	bool	m_hasAttachment;
};

LINK_ENTITY_TO_CLASS( monster_archer, CArcher );
LINK_ENTITY_TO_CLASS( monster_archerfish, CArcher );	// beta backward compatibility

TYPEDESCRIPTION	CArcher::m_SaveData[] =
{
	DEFINE_FIELD( CArcher, m_flTurning, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_flAccelerate, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_top, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_bottom, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_waterTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_sideTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_zTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_stateTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_attackSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_lastAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_onAttack, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CArcher, CBaseMonster );

const NamedSoundScript CArcher::biteSoundScript = {
	CHAN_VOICE,
	{"bullchicken/bc_bite1.wav"},
	"Archer.Bite"
};

const NamedSoundScript CArcher::idleSoundScript = {
	CHAN_VOICE,
	{},
	"Archer.Idle"
};

const NamedSoundScript CArcher::alertSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_alert1.wav"},
	IntRange{105, 110},
	"Archer.Alert"
};

const NamedSoundScript CArcher::painSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_pain1.wav"},
	IntRange{105, 110},
	"Archer.Pain"
};

const NamedSoundScript CArcher::dieSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_die1.wav", "ichy/ichy_die3.wav"},
	IntRange{105, 110},
	"Archer.Die"
};

const NamedSoundScript CArcher::zapPowerupSoundScript = {
	CHAN_WEAPON,
	{"debris/zap1.wav"},
	0.8f,
	ATTN_STATIC,
	IntRange{110, 120},
	"Archer.ZapPowerup"
};

const NamedSoundScript CArcher::electroSoundScript = {
	CHAN_STATIC,
	{"weapons/electro4.wav"},
	0.5f,
	ATTN_NORM,
	IntRange(140, 160),
	"Archer.Electro"
};

const NamedVisual CArcher::zapBeamVisual = BuildVisual("Archer.ZapBeam")
		.Model("sprites/lgtning.spr")
		.RenderColor(32, 96, 255)
		.Alpha(255)
		.BeamParams(50, 20)
		.Life(0.4f);

const NamedVisual CArcher::powerupLightVisual = BuildVisual("Archer.PowerupLight")
		.Radius(150)
		.RenderColor(32, 96, 255)
		.Life(0.5f)
		.Decay(80);

const NamedVisual CArcher::zapLightVisual = BuildVisual("Archer.ZapLight")
		.Radius(200)
		.RenderColor(32, 96, 255)
		.Life(1.0f)
		.Decay(80);

void CArcher::Spawn( void )
{
	Precache();
	SetMyModel("models/archer.mdl");
	SetMySize();
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	SetBits(pev->flags, FL_SWIM);
	SetMyHealth(GetSkillValue("archer_health"));
	SetMyBloodColor(BLOOD_COLOR_YELLOW);

	SetMyFieldOfView(VIEW_FIELD_FULL);
	m_flDistLook		= 750;
	MonsterInit();
	SetThink( &CArcher::SwimThink );
	pev->view_ofs = g_vecZero;

	m_flTurning = 0;
	SetActivity( ACT_WALK );
	SetState( MONSTERSTATE_IDLE );
	m_stateTime = gpGlobals->time + 0.5f;

	CheckForAttachments();
}

void CArcher::Activate()
{
	RecalculateWaterlevel();
	CBaseMonster::Activate();
}

void CArcher::RecalculateWaterlevel( void )
{
	// Calculate boundaries
	Vector vecTest = pev->origin - Vector(0,0,400);

	TraceResult tr;

	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
	if ( tr.flFraction != 1.0 )
		m_bottom = tr.vecEndPos.z + 1;
	else
		m_bottom = vecTest.z;

	m_top = UTIL_WaterLevel( pev->origin, pev->origin.z, pev->origin.z + 400 ) - 1;

	if (m_top <= m_bottom)
	{
		m_top = m_bottom + 1.0f;
	}

	const float myHeight = pev->size.z;
	float heightFraction = myHeight * 2.0f / (m_top - m_bottom);
	heightFraction = clamp(heightFraction, 0.1f, 0.2f);

	float newBottom = m_bottom * (1.0f - heightFraction) + m_top * heightFraction;
	m_top = m_bottom * heightFraction + m_top * (1.0f - heightFraction);
	m_bottom = newBottom;
	m_height = RANDOM_FLOAT( m_bottom, m_top );
	m_waterTime = gpGlobals->time + RANDOM_FLOAT( 5, 7 );
}

void CArcher::SwitchArcherState()
{
	m_stateTime = gpGlobals->time + RANDOM_FLOAT( 2, 4 );
	if ( m_MonsterState == MONSTERSTATE_COMBAT )
	{
		m_hEnemy = 0;
		SetState( MONSTERSTATE_IDLE );
		// We may be up against the player, so redo the side checks
		m_sideTime = 0;
	}
	else
	{
		CBaseEntity *pEnemy = BestVisibleEnemy();
		if ( pEnemy && pEnemy->IsAlive() )
		{
			m_hEnemy = pEnemy;
			m_vecEnemyLKP = pEnemy->pev->origin;
			SetState( MONSTERSTATE_COMBAT );
			m_stateTime = gpGlobals->time + RANDOM_FLOAT( 18, 25 );
			AlertSound();
		}
	}
}

void CArcher::Precache()
{
	PrecacheMyModel("models/archer.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(biteSoundScript);
	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(zapPowerupSoundScript);
	RegisterAndPrecacheSoundScript(electroSoundScript);

	RegisterVisual(zapBeamVisual);
	RegisterVisual(powerupLightVisual);
	RegisterVisual(zapLightVisual);

	CheckForAttachments();
}

DamageInfo CArcher::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	if (FBitSet(inputDamageInfo.type, DMG_SHOCK))
	{
		CBaseEntity* pAttacker = OwnInstance( pevAttacker );
		if (pAttacker)
		{
			const int relationship = IRelationship(pAttacker);
			if (relationship == R_AL || (relationship == R_NO && FClassnameIs(pevAttacker, STRING(pev->classname))))
			{
				DamageInfo damageInfo = inputDamageInfo;
				damageInfo.mustSkip = true;
				return damageInfo;
			}
		}
	}
	return inputDamageInfo;
}

TakeDamageResult CArcher::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo)
{
	pev->velocity = g_vecZero;

	if ( pevInflictor )
	{
		pev->velocity = (pev->origin - pevInflictor->origin).Normalize() * 25;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, damageInfo );
}

int CArcher::LookupActivity(int activity)
{
	int sequence = CBaseMonster::LookupActivity(activity);
	if (sequence == ACTIVITY_NOT_AVAILABLE)
	{
		if (activity == ACT_RANGE_ATTACK1)
		{
			return LookupSequence("shoot");
		}
	}
	return sequence;
}

void CArcher::SetActivity(Activity NewActivity)
{
	CBaseMonster::SetActivity(NewActivity);

	if (NewActivity == ACT_RANGE_ATTACK1)
	{
		m_onAttack = true;
		EmitSoundScript(zapPowerupSoundScript);
		SendEntLight(entindex(), GetBeamStartPos(), GetVisual(powerupLightVisual));
	}
	else if (NewActivity == ACT_MELEE_ATTACK1)
	{
		m_onAttack = true;
	}
}

void CArcher::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ARCHER_AE_ATTACK:
	{
		m_lastAttackTime = gpGlobals->time;

		EmitSoundScript(biteSoundScript);
		CBaseEntity *pEnemy = m_hEnemy;
		if ( pEnemy != NULL && (pEnemy->BodyTarget(pev->origin) - pev->origin).IsLengthLessThanOrEqual(pev->size.x * 3.0f) )
		{
			Vector dir, face;

			UTIL_MakeVectorsPrivate( pev->angles, face, NULL, NULL );
			face.z = 0;
			dir = (pEnemy->pev->origin - pev->origin);
			dir.z = 0;
			dir = dir.Normalize();
			face = face.Normalize();

			if ( DotProduct(dir, face) > 0.7f )
			{
				pEnemy->TakeDamage( pev, pev, DamageInfo(GetSkillValue("archer_dmg_bite"), DMG_SLASH) );

				Vector punchAngle{5, 0, -12};
				pEnemy->ApplyPunchAngle(punchAngle);
			}
		}
	}
	break;

	case ARCHER_BEAM_ATTACK_ORIG:
	case ARCHER_BEAM_ATTACK_THRILL:
	{
		m_lastAttackTime = gpGlobals->time;

		Vector vecBeamPos = GetBeamStartPos();
		BeamAttack(vecBeamPos);
		SendEntLight(entindex(), vecBeamPos, GetVisual(zapLightVisual));

		m_flNextAttack = gpGlobals->time + GetSkillValue("archer_delay_zap");
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CArcher::MakeVectors()
{
	Vector tmp = pev->angles;
	tmp.x = -tmp.x;
	UTIL_MakeVectors ( tmp );
}

float CArcher::ObstacleDistance( CBaseEntity *pTarget )
{
	TraceResult tr;

	MakeVectors();

	// check for obstacle ahead
	Vector vecTest = pev->origin + gpGlobals->v_forward * ForwardCheckDistance();
	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);

	if ( tr.fStartSolid )
	{
		pev->speed = -ARCHER_SWIM_SPEED * 0.5f;
	}

	if (tr.flFraction != 1.0f)
	{
		if ( (pTarget == NULL || tr.pHit != pTarget->edict()) )
		{
			return tr.flFraction;
		}
		else
		{
			if ( fabs(m_height - pev->origin.z) > 10 )
				return tr.flFraction;
		}
	}

	if ( m_sideTime < gpGlobals->time )
	{
		// extra wide checks
		vecTest = pev->origin + gpGlobals->v_right * pev->size.x + gpGlobals->v_forward * ForwardCheckDistance();
		UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
		if (tr.flFraction != 1.0f)
			return tr.flFraction;

		vecTest = pev->origin - gpGlobals->v_right * pev->size.x + gpGlobals->v_forward * ForwardCheckDistance();
		UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
		if (tr.flFraction != 1.0f)
			return tr.flFraction;

		m_sideTime = gpGlobals->time + 0.5f;
	}
	return 1.0f;
}

void CArcher::DeadThink()
{
	if ( m_fSequenceFinished )
	{
		if ( m_Activity == ACT_DIESIMPLE )
		{
			SetThink( NULL );
			StopAnimation();
			return;
		}
		else if ( pev->flags & FL_ONGROUND )
		{
			pev->solid = SOLID_NOT;
			SetActivity(ACT_DIESIMPLE);
		}
	}
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	// Apply damage velocity, but keep out of the walls
	if ( pev->velocity.x != 0 || pev->velocity.y != 0 )
	{
		TraceResult tr;

		// Look 0.5 seconds ahead
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 0.5f, missile, edict(), &tr);
		if (tr.flFraction != 1.0f)
		{
			pev->velocity.x = 0;
			pev->velocity.y = 0;
		}
	}

	HandleCloaking();
}

void CArcher::UpdateMotion(float turnSpeed, float obstacleFactor)
{
	float flapspeed = (pev->speed - m_flAccelerate) / ARCHER_ACCELERATE;
	m_flAccelerate = m_flAccelerate * 0.8 + pev->speed * 0.2;

	if (flapspeed < 0)
		flapspeed = -flapspeed;
	flapspeed += 1.0;
	if (flapspeed < 0.5)
		flapspeed = 0.5;
	if (flapspeed > 1.9)
		flapspeed = 1.9;

	pev->framerate = flapspeed;

	if (obstacleFactor == 0.0f)
		pev->avelocity.y = turnSpeed;
	else
		pev->avelocity.y = turnSpeed * obstacleFactor;

	if (!m_onAttack)
	{
		if ( pev->avelocity.y > 150 )
			m_IdealActivity = ACT_TURN_LEFT;
		else if ( pev->avelocity.y < -150 )
			m_IdealActivity = ACT_TURN_RIGHT;
		else if ( pev->speed > ARCHER_SWIM_SPEED )
		{
			m_IdealActivity = ACT_RUN;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
	}

	// lean
	const float maxPitch = 30;
	float targetPitch;
	float delta = m_height - pev->origin.z;

	if ( delta < -10 )
		targetPitch = -maxPitch;
	else if ( delta > 10 )
		targetPitch = maxPitch;
	else
		targetPitch = 0;

	pev->angles.x = UTIL_Approach( targetPitch, pev->angles.x, 60.0f * ARCHER_FRAMETIME );

	// bank
	pev->avelocity.z = -(pev->angles.z + (pev->avelocity.y * 0.25));

	if (m_MonsterState == MONSTERSTATE_COMBAT && !m_onAttack)
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
		}
		else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			m_IdealActivity = ACT_MELEE_ATTACK1;
		}
	}

	if ( m_Activity != m_IdealActivity )
	{
		SetActivity ( m_IdealActivity );
	}
	float flInterval = StudioFrameAdvance();
	DispatchAnimEvents ( flInterval );
	if (m_fSequenceFinished)
	{
		m_onAttack = false;

		if (m_Activity == ACT_RANGE_ATTACK1 || m_Activity == ACT_MELEE_ATTACK1)
		{
			m_IdealActivity = ACT_WALK;
			SetActivity(m_IdealActivity);
		}
	}
}

void CArcher::SwimThink()
{
	TraceResult		tr;
	float			flLeftSide;
	float			flRightSide;
	float			turnYawSpeed = 0;
	CBaseEntity		*pTarget;

	if ( FNullEnt(FIND_CLIENT_IN_PVS( edict() )) && m_hEnemy == 0 )
	{
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1.0f, 1.5f);
		pev->velocity = g_vecZero;
		return;
	}
	else
		pev->nextthink = gpGlobals->time + ARCHER_FRAMETIME;

	float targetSpeed = ARCHER_SWIM_SPEED;

	if ( m_waterTime < gpGlobals->time )
		RecalculateWaterlevel();

	if ( m_stateTime < gpGlobals->time )
		SwitchArcherState();

	ClearConditions( bits_COND_CAN_MELEE_ATTACK1 );
	ClearConditions( bits_COND_CAN_RANGE_ATTACK1 );

	Look( m_flDistLook );

	auto MaySelectNewHeight = [this]()
	{
		if (m_zTime < gpGlobals->time)
		{
			const float newHeight = RANDOM_FLOAT( m_bottom, m_top );
			m_height = 0.5 * m_height + 0.5 * newHeight;
			m_zTime = gpGlobals->time + RANDOM_FLOAT( 1, 4 );
		}
	};
	auto MayChooseRandomYawSpeed = [this, &turnYawSpeed]()
	{
		if ( RANDOM_LONG( 0, 100 ) < 10 )
			turnYawSpeed = RANDOM_LONG( -30, 30 );
	};
	auto MayUpdateSideTime = [this]()
	{
		if ( (pev->origin - pev->oldorigin).Length() < 1 )
		{
			m_sideTime = 0;
		}
	};

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		pTarget = m_hEnemy;
		if ( !pTarget || !(pTarget->IsAlive()) )
			SwitchArcherState();
		else
		{
			m_vecEnemyLKP = pTarget->pev->origin;

			if (m_lastAttackTime == 0.0f || m_lastAttackTime + 1.0f < gpGlobals->time)
			{
				// Chase the enemy's eyes
				m_height = pTarget->pev->origin.z + pTarget->pev->view_ofs.z - 5;
				// Clip to viable water area
				m_height = clamp(m_height, m_bottom, m_top);

				const Vector vecTargetPos = pTarget->Center();
				const Vector vecToTarget = vecTargetPos - Center();

				bool missedTarget = false;

				if (HasConditions(bits_COND_SEE_ENEMY))
				{
					UTIL_MakeVectors( pev->angles );
					Vector vec2LOS = vecToTarget;
					vec2LOS.NormalizeInPlace();

					const float flDot = DotProduct(vec2LOS, gpGlobals->v_forward);

					if ( vecToTarget.IsLengthLessThan(pev->size.x * 3.0f + 0.1f * pev->speed) && pTarget->pev->waterlevel != 0 && flDot >= 0.7f )
					{
						SetConditions( bits_COND_CAN_MELEE_ATTACK1 );
					}
					else if ( vecToTarget.IsLengthGreaterThanOrEqual(60) && vecToTarget.IsLength2DLessThanOrEqual(ARCHER_ATTACK_DISTANCE) && flDot >= 0.5f && m_flNextAttack < gpGlobals->time )
					{
						SetConditions( bits_COND_CAN_RANGE_ATTACK1 );
					}

					if (flDot < -0.1f && vecToTarget.IsLength2DLessThan(pev->size.x * 3.0f))
					{
						missedTarget = true;
					}
				}

				if (missedTarget)
				{
					targetSpeed *= 2.0f;
					MaySelectNewHeight();
					MayChooseRandomYawSpeed();
					MayUpdateSideTime();
				}
				else
				{
					const float targetYaw = UTIL_VecToYaw( vecToTarget );
					float yawDiff = UTIL_AngleDiff( targetYaw, UTIL_AngleMod( pev->angles.y ) );

					bool approxRightDirection = yawDiff > -45.0f && yawDiff < 45.0f;

					float heading = std::atan2(pev->velocity.y, pev->velocity.x) * 180.0f / M_PI_F;
					float desiredHeading = std::atan2(vecTargetPos.y - pev->origin.y, vecTargetPos.x - pev->origin.x) * 180.0f / M_PI_F;
					float angleError = UTIL_AngleDiff(desiredHeading, heading);
					turnYawSpeed = 2.0f * angleError;

					turnYawSpeed = clamp(turnYawSpeed, -120.0f, 120.0f);

					if (approxRightDirection)
					{
						if (gpGlobals->time > m_attackSoundTime)
						{
							AlertSound();
						}
						targetSpeed *= 3;
					}
				}
			}
			else
			{
				targetSpeed *= 2.5f;
				MaySelectNewHeight();
				MayChooseRandomYawSpeed();
				MayUpdateSideTime();
			}
		}
		break;

	default:
		MayPlayIdleSound();
		MaySelectNewHeight();
		MayChooseRandomYawSpeed();
		MayUpdateSideTime();
		pTarget = nullptr;
		break;
	}

	float obstacleFactor = 0.0f;

	float obstacleFraction = ObstacleDistance( pTarget );
	pev->oldorigin = pev->origin;

	// is the way ahead clear?
	if ( obstacleFraction == 1.0f )
	{
		// if the leech is turning, stop the trend.
		m_flTurning = 0;

		pev->speed = UTIL_Approach( targetSpeed, pev->speed, ARCHER_SWIM_ACCEL * ARCHER_FRAMETIME );
		pev->velocity = gpGlobals->v_forward * pev->speed;
	}
	else
	{
		obstacleFraction = Q_max(obstacleFraction, 0.1f);
		obstacleFactor = 1.0f / obstacleFraction;

		if ( m_flTurning == 0 )// something in the way and leech is not already turning to avoid
		{
			Vector vecTest;
			// measure clearance on left and right to pick the best dir to turn
			vecTest = pev->origin + (gpGlobals->v_right * pev->size.x * 0.5f) + (gpGlobals->v_forward * ForwardCheckDistance());
			UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
			flRightSide = tr.flFraction;

			vecTest = pev->origin + (gpGlobals->v_right * -pev->size.x * 0.5f) + (gpGlobals->v_forward * ForwardCheckDistance());
			UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
			flLeftSide = tr.flFraction;

			// turn left, right or random depending on clearance ratio
			float delta = (flRightSide - flLeftSide);
			if ( delta > 0.1 || (delta > -0.1 && RANDOM_LONG(0,100)<50) )
				m_flTurning = -ARCHER_TURN_RATE;
			else
				m_flTurning = ARCHER_TURN_RATE;
		}
		pev->speed = UTIL_Approach( -ARCHER_SWIM_SPEED * 0.5f, pev->speed, ARCHER_SWIM_DECEL * ARCHER_FRAMETIME * obstacleFactor );
		pev->velocity = gpGlobals->v_forward * pev->speed;
	}
	UpdateMotion(m_flTurning + turnYawSpeed, obstacleFactor);
	HandleCloaking();
}

void CArcher::BeamAttack(const Vector& vecBeamPos)
{
	TraceResult tr;

	const Vector vecAim = ShootAtEnemy(vecBeamPos);
	UTIL_TraceLine(vecBeamPos, vecBeamPos + vecAim * ARCHER_BEAM_DISTANCE, dont_ignore_monsters, edict(), &tr);

	const Visual* beamVisual = GetVisual(zapBeamVisual);
	if (beamVisual)
	{
		CBeam* zapBeam = CreateBeamFromVisual(beamVisual);
		if (zapBeam)
		{
			if (m_hasAttachment)
			{
				zapBeam->PointEntInit(tr.vecEndPos, entindex(), 1);
			}
			else
			{
				zapBeam->PointEntInit(tr.vecEndPos, entindex());
			}
			zapBeam->LiveForTime(RandomizeNumberFromRange(beamVisual->life));
			zapBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		}
	}

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
		pEntity->ApplyTraceAttack(pev, pev, DamageInfo(GetSkillValue("archer_dmg_zap"), DMG_SHOCK), vecAim, &tr);
	}
	EmitSoundScriptAmbient(tr.vecEndPos, electroSoundScript);
}

Vector CArcher::GetBeamStartPos()
{
	Vector vecSrc;
	if (m_hasAttachment)
	{
		Vector vecJunk;
		GetAttachment (0, vecSrc, vecJunk);
	}
	else
	{
		UTIL_MakeVectors(pev->angles);
		vecSrc = pev->origin + gpGlobals->v_forward * 16.0f;
		vecSrc.z += 16.0f;
	}
	return vecSrc;
}

KilledResult CArcher::Killed(entvars_t* pevInflictor, entvars_t *pevAttacker, int iGib)
{
	KilledResult killedResult;

	if (HasMemory(bits_MEMORY_KILLED))
	{
		if (ShouldGibMonster( iGib ))
			CallGibMonster();
		return killedResult.SetGibbed();
	}

	const bool shouldGib = ShouldGibMonster(iGib);
	OnDying(shouldGib, CBaseEntity::OwnInstance(pevAttacker));

	if (shouldGib)
	{
		CallGibMonster();
		return killedResult.SetGibbed();
	}

	// When we hit the ground, play the "death_end" activity
	if ( pev->waterlevel )
	{
		pev->angles.z = 0;
		pev->angles.x = 0;
		pev->origin.z += 1;
		pev->avelocity = g_vecZero;
		if ( RANDOM_LONG( 0, 99 ) < 70 )
			pev->avelocity.y = RANDOM_LONG( -720, 720 );

		pev->gravity = 0.02;
		ClearBits(pev->flags, FL_ONGROUND);
		SetActivity( ACT_DIESIMPLE );
	}
	else
		SetActivity( ACT_DIEFORWARD );

	pev->movetype = MOVETYPE_TOSS;
	SetThink( &CArcher::DeadThink );
	pev->deadflag = DEAD_DEAD;
	DeathSound();
	return killedResult;
}
