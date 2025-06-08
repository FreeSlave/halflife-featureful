
//---------------------------------------------------------
//-	based on code by JujU						-----------
//-		julien.lecorre@free.fr					-----------
//---------------------------------------------------------

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"schedule.h"
#include	"combat.h"
#include	"flyingmonster.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"game.h"
#include	"decals.h"
#include	"visuals_utils.h"
#include	"mod_features.h"

#if FEATURE_FLYBEE

#define FLYBEE_SPEED		150
#define PROBE_LENGTH		150

#define FEAR_LEVEL		10
#define HATE_LEVEL		-10

enum 
{
	TASK_FLYBEE_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_FLYBEE_SWIM,
	TASK_FLYBEE_STOP_MOVING,
	TASK_FLYBEE_RUN_ATTACK,
	TASK_FLYBEE_RANGE_ATTACK2,
};

#define FLYBEE_AE_HIT		1
#define FLYBEE_AE_ATTACK1	2
#define FLYBEE_AE_ATTACK2	3

class CFlybee : public CFlyingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("flybee"); }
	int DefaultClassify( void );
	const char* DefaultDisplayName() { return "Flybee"; }

	int		Save( CSave &save ); 
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;
	void	HandleAnimEvent( MonsterEvent_t *pEvent );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );

	void	StartTask( Task_t *pTask );
	void	RunTask( Task_t *pTask );

	bool	CheckMeleeAttack1( float flDot, float flDist ) override;
	bool	CheckRangeAttack1( float flDot, float flDist ) override;
	bool	CheckRangeAttack2( float flDot, float flDist ) override;

	Activity GetStoppedActivity( void );

	void	MonsterThink( void );

	void	Move( float flInterval );
	void	MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );

	void	Stop( void );
	void	Swim( void );

	void	SetYawSpeed( void );
	float	ChangeYaw( int speed );
	Vector	DoProbe(const Vector &Probe);

	float	VectorToPitch( const Vector &vec);
	float	FlPitchDiff( void );
	float	ChangePitch( int speed );

	Vector	m_SaveVelocity;
	float	m_idealDist;

	float	m_flMaxSpeed;
	float	m_flMinSpeed;
	float	m_flMaxDist;

	Vector	m_vecAttack2;

	int		m_iFear;
	float m_flNextAlert;
	float m_flNextAttack;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript biteSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript beamSoundScript;

	void IdleSound() override;
	void AlertSound() override;
	void AttackSound();
	void BiteSound();
	void DeathSound() override;
	void PainSound() override;

	virtual int DefaultSizeForGrapple() { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return Vector( -24.0f, -24.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 24.0f, 24.0f, 24.0f ); }

	static const NamedVisual zapBeamVisual;
	static const NamedVisual zapBeamAltVisual;
	static const NamedVisual zapVisual;
	static const NamedVisual zapWaveVisual;
};

LINK_ENTITY_TO_CLASS( monster_flybee, CFlybee );

TYPEDESCRIPTION	CFlybee::m_SaveData[] = 
{
	DEFINE_FIELD( CFlybee, m_SaveVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CFlybee, m_idealDist, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flMaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flMinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flNextAlert, FIELD_TIME ),
	DEFINE_FIELD( CFlybee, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFlybee, m_iFear, FIELD_INTEGER ),
	DEFINE_FIELD( CFlybee, m_vecAttack2, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFlybee, CFlyingMonster );

class CFlyBall : public CBaseEntity
{
public :
	void Spawn( void );
	void Precache( void );
	void EXPORT AnimateThink( void );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );

	static CFlyBall *CreateFlyBall(Vector vecOrigin, Vector vecAngles, entvars_s *pevOwner , EntityOverrides entityOverrides);

	static const NamedSoundScript electroSoundScript;

	static const NamedVisual ballVisual;
	static const NamedVisual ballTrailVisual;
};
LINK_ENTITY_TO_CLASS( flyball, CFlyBall );

const NamedSoundScript CFlyBall::electroSoundScript = {
	CHAN_STATIC,
	{"weapons/electro4.wav"},
	0.3f,
	ATTN_NORM,
	IntRange(90, 99),
	"Flybee.Electro"
};

const NamedVisual CFlyBall::ballVisual = BuildVisual("Flybee.Ball")
		.Model("sprites/muz7.spr")
		.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 190)
		.Scale(0.2f);

const NamedVisual CFlyBall::ballTrailVisual = BuildVisual("Flybee.BallTrail")
		.Model("sprites/xspark3.spr")
		.Framerate(22.0f)
		.Scale(0.2f)
		.RenderProps(kRenderTransAdd, Color3(230, 255, 230), 150, kRenderFxNone);

constexpr float flybeeAttenuation = 0.6f;
constexpr IntRange flybeePitch(95, 105);

const NamedSoundScript CFlybee::idleSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_idle1.wav", "ichy/ichy_idle2.wav", "ichy/ichy_idle3.wav", "ichy/ichy_idle4.wav"},
	1.0f,
	flybeeAttenuation,
	flybeePitch,
	"Flybee.Idle"
};

const NamedSoundScript CFlybee::alertSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_alert2.wav", "ichy/ichy_alert3.wav"},
	1.0f,
	flybeeAttenuation,
	flybeePitch,
	"Flybee.Alert"
};

const NamedSoundScript CFlybee::attackSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_attack1.wav", "ichy/ichy_attack2.wav"},
	1.0f,
	flybeeAttenuation,
	flybeePitch,
	"Flybee.Attack"
};

const NamedSoundScript CFlybee::biteSoundScript = {
	CHAN_WEAPON,
	{"ichy/ichy_bite1.wav", "ichy/ichy_bite2.wav"},
	1.0f,
	flybeeAttenuation,
	flybeePitch,
	"Flybee.Bite"
};

const NamedSoundScript CFlybee::painSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_pain2.wav", "ichy/ichy_pain3.wav", "ichy/ichy_pain5.wav"},
	1.0f,
	flybeeAttenuation,
	flybeePitch,
	"Flybee.Pain"
};

const NamedSoundScript CFlybee::dieSoundScript = {
	CHAN_VOICE,
	{"ichy/ichy_die2.wav", "ichy/ichy_die4.wav"},
	1.0f,
	flybeeAttenuation,
	flybeePitch,
	"Flybee.Die"
};

const NamedSoundScript CFlybee::beamSoundScript = {
	CHAN_STATIC,
	{"debris/beamstart4.wav"},
	0.9f,
	ATTN_NORM,
	IntRange(90, 99),
	"Flybee.Beam"
};

const NamedVisual CFlybee::zapBeamVisual = BuildVisual("Flybee.ZapBeam")
		.Model("sprites/laserbeam.spr")
		.RenderColor(206, 118, 254)
		.Alpha(192)
		.BeamParams(8, 30)
		.Life(0.35f);

const NamedVisual CFlybee::zapBeamAltVisual = BuildVisual("Flybee.ZapBeamAlt")
		.RenderColor(223, 224, 255)
		.Mixin(&CFlybee::zapBeamVisual);

const NamedVisual CFlybee::zapVisual = BuildVisual("Flybee.Zap")
		.Model("sprites/nhth1.spr")
		.Framerate(10.0f)
		.Scale(0.8f)
		.RenderProps(kRenderTransAdd, Color3(230, 255, 230), 150, kRenderFxNone);

const NamedVisual CFlybee::zapWaveVisual = BuildVisual("Flybee.ZapWave")
		.Model("sprites/shockwave.spr")
		.Life(0.2f)
		.BeamWidth(16)
		.RenderColor(206, 118, 255)
		.Alpha(80);

void CFlybee::IdleSound()
{
	EmitSoundScript(idleSoundScript);
}

void CFlybee::AlertSound()
{
	EmitSoundScript(alertSoundScript);
}

void CFlybee::AttackSound()
{
	EmitSoundScript(attackSoundScript);
}

void CFlybee::BiteSound()
{
	EmitSoundScript(biteSoundScript);
}

void CFlybee::DeathSound()
{ 
	EmitSoundScript(dieSoundScript);
}

void CFlybee::PainSound()
{
	EmitSoundScript(painSoundScript);
}

void CFlybee::Spawn()
{
	Precache();

	SetMyModel("models/flybee.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	SetMyBloodColor(BLOOD_COLOR_GREEN);
	SetMyHealth(gSkillData.flybeeHealth);
	pev->view_ofs		= Vector ( 0, 0, 16 );
	SetMyFieldOfView(VIEW_FIELD_WIDE);
	m_MonsterState		= MONSTERSTATE_NONE;
	SetFlyingSpeed( FLYBEE_SPEED );
	SetFlyingMomentum( 2.5f );

	m_afCapability = bits_CAP_MELEE_ATTACK1 | bits_CAP_RANGE_ATTACK1 | bits_CAP_RANGE_ATTACK2 | bits_CAP_SWIM;

	MonsterInit();

	m_idealDist		= 384;
	m_flMinSpeed	= 80;
	m_flMaxSpeed	= 300;
	m_flMaxDist		= 384;

	m_iFear			= 0;
	m_flNextAttack	= 0;

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;
}

void CFlybee::Precache()
{
	PrecacheMyModel("models/flybee.mdl");
	PrecacheMyGibModel();

	PRECACHE_SOUND("zombie/claw_miss2.wav");

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(biteSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(beamSoundScript);

	RegisterVisual(zapBeamVisual);
	RegisterVisual(zapBeamAltVisual);
	RegisterVisual(zapVisual);
	RegisterVisual(zapWaveVisual);

	UTIL_PrecacheOther ( "flyball", GetProjectileOverrides() );
}

int	CFlybee::DefaultClassify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

bool CFlybee::CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 64  )
	{
		return true;
	}
	return false;
}

bool CFlybee::CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDot > -0.7 && m_iFear <= HATE_LEVEL )
	{
		return true;
	}
	return false;
}

bool CFlybee::CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && m_iFear >= FEAR_LEVEL && m_flNextAttack < gpGlobals->time )
	{
		return true;
	}
	return false;
}

void CFlybee::SetYawSpeed ( void )
{
	pev->yaw_speed = 100;
}

void CFlybee::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case FLYBEE_AE_HIT:
		{
			TraceHullAttackParams params;
			params.punchAngle.z = 25;
			params.damageInfo.damage = gSkillData.flybeeDmgKick;
			params.damageInfo.type = DMG_CLUB;
			SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

			PerformTraceHullAttack(params);

			BiteSound();
			break;
		}

	case FLYBEE_AE_ATTACK2:
		{
			UTIL_MakeVectors( pev->angles );
			TraceResult tr;

			Vector vecStart	= pev->origin + gpGlobals->v_up * 24 + gpGlobals->v_forward * 32;
			UTIL_TraceLine( vecStart, m_vecAttack2, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			Vector vecEnd = tr.vecEndPos;

			for ( int i = 0; i<4; i ++ )
			{
				const Visual* beamVisual = RANDOM_LONG(0,1) ? GetVisual(zapBeamVisual) : GetVisual(zapBeamAltVisual);
				CBeam *pBeam = CreateBeamFromVisual(beamVisual);
				if (pBeam)
				{
					pBeam->SetStartPos( vecStart );
					pBeam->SetEndPos( vecEnd );
					pBeam->RelinkBeam();

					pBeam->LiveForTime(RandomizeNumberFromRange(beamVisual->life));
				}
			}

			const Visual* visual = GetVisual(zapVisual);
			CSprite *pSprite = CreateSpriteFromVisual(visual, vecEnd);
			if (pSprite)
			{
				pSprite->AnimateAndDie(pSprite->pev->framerate);
				pSprite->Expand( pSprite->pev->scale, 120 );
			}

			const Visual* waveVisual = GetVisual(zapWaveVisual);
			if (waveVisual && waveVisual->modelIndex)
			{
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
					WRITE_BYTE( TE_BEAMCYLINDER );
					WRITE_CIRCLE( vecEnd, 1000 );
					WriteBeamVisual(waveVisual);
				MESSAGE_END();
			}
			RadiusDamage( vecEnd, pev, pev, DamageInfo{gSkillData.flybeeDmgBeam, DMG_SHOCK}, CLASS_ALIEN_MONSTER );

			EmitSoundScriptAmbient(vecEnd, beamSoundScript);
			break;
		}
	case FLYBEE_AE_ATTACK1:
		{
			UTIL_MakeVectors( pev->angles );

			Vector vecSrc	= pev->origin + gpGlobals->v_up * 24 + gpGlobals->v_forward * 32;
			Vector ang		= UTIL_VecToAngles( (m_hEnemy->Center() - vecSrc).Normalize() );

			EntityOverrides flyBallOverrides = GetProjectileOverrides();

			CFlyBall::CreateFlyBall( vecSrc + gpGlobals->v_right * 30, ang, pev, flyBallOverrides );
			CFlyBall::CreateFlyBall( vecSrc + gpGlobals->v_right * 10, ang, pev, flyBallOverrides );
			CFlyBall::CreateFlyBall( vecSrc - gpGlobals->v_right * 30, ang, pev, flyBallOverrides );
			CFlyBall::CreateFlyBall( vecSrc - gpGlobals->v_right * 10, ang, pev, flyBallOverrides );

			break;
		}
	default:
		CFlyingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

static Task_t	tlFlybeeSwimAround[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_FLYBEE_SWIM,				(float)0 },
};

static Schedule_t	slFlybeeSwimAround[] =
{
	{ 
		tlFlybeeSwimAround,
		ARRAYSIZE(tlFlybeeSwimAround), 
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY		|
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAR_SOUND,
		bits_SOUND_PLAYER |
		bits_SOUND_COMBAT,
		"SwimAround"
	},
};

static Task_t	tlFlybeeSwimAgitated[] =
{
	{ TASK_STOP_MOVING,				(float) 0 },
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_WAIT,					(float)2.0 },
};

static Schedule_t	slFlybeeSwimAgitated[] =
{
	{ 
		tlFlybeeSwimAgitated,
		ARRAYSIZE(tlFlybeeSwimAgitated), 
		0, 
		0, 
		"SwimAgitated"
	},
};


static Task_t	tlFlybeeCircleEnemy[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_FLYBEE_CIRCLE_ENEMY, 0.0 },
};

static Schedule_t	slFlybeeCircleEnemy[] =
{
	{ 
		tlFlybeeCircleEnemy,
		ARRAYSIZE(tlFlybeeCircleEnemy), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"CircleEnemy"
	},
};


Task_t tlFlybeeTwitchDie[] =
{
	{ TASK_STOP_MOVING,			0		 },
	{ TASK_SOUND_DIE,			(float)0 },
	{ TASK_DIE,					(float)0 },
};

Schedule_t slFlybeeTwitchDie[] =
{
	{
		tlFlybeeTwitchDie,
		ARRAYSIZE( tlFlybeeTwitchDie ),
		0,
		0,
		"Die"
	},
};

Task_t tlFlybeeRangeAttack1[] =
{
	{ TASK_FLYBEE_STOP_MOVING,	0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t slFlybeeRangeAttack1[] =
{
	{
		tlFlybeeRangeAttack1,
		ARRAYSIZE( tlFlybeeRangeAttack1 ),

		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_LOST		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Range attack 1"
	},
};

Task_t tlFlybeeRangeAttack2[] =
{
	{ TASK_FLYBEE_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,					(float)0		},
	{ TASK_FLYBEE_RANGE_ATTACK2,		(float)0		},
};

Schedule_t slFlybeeRangeAttack2[] =
{
	{
		tlFlybeeRangeAttack2,
		ARRAYSIZE( tlFlybeeRangeAttack2 ),

		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_LOST		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Range attack 2"
	},
};

Task_t tlFlybeeRunAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},
	{ TASK_FLYBEE_STOP_MOVING,	(float) 0					},
	{ TASK_FLYBEE_RUN_ATTACK,	(float)	0					},
};

Schedule_t slFlybeeRunAttack[] =
{
	{
		tlFlybeeRunAttack,
		ARRAYSIZE( tlFlybeeRunAttack ),

		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_LOST		|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Range attack 2"
	},
};

Task_t	tlFlybeeMeleeAttack[] =
{
	{ TASK_FLYBEE_STOP_MOVING,	(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slFlybeeMeleeAttack[] =
{
	{ 
		tlFlybeeMeleeAttack,
		ARRAYSIZE ( tlFlybeeMeleeAttack ), 
		bits_COND_NEW_ENEMY|
		bits_COND_ENEMY_DEAD|
		bits_COND_ENEMY_LOST,
		0,
		"Flybee Melee Attack"
	},
};

DEFINE_CUSTOM_SCHEDULES(CFlybee)
{
	slFlybeeSwimAround,
	slFlybeeSwimAgitated,
	slFlybeeCircleEnemy,
	slFlybeeTwitchDie,
	slFlybeeRangeAttack1,
	slFlybeeRangeAttack2,
	slFlybeeRunAttack,
	slFlybeeMeleeAttack,
};
IMPLEMENT_CUSTOM_SCHEDULES(CFlybee, CFlyingMonster);

Schedule_t* CFlybee::GetSchedule()
{
	switch(m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = 80;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_ALERT:
		m_flightSpeed = 150;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_COMBAT:
		m_flMaxSpeed = 400;

		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			m_iFear = Q_max ( HATE_LEVEL, m_iFear - 5 );
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}

		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			m_iFear = Q_min ( FEAR_LEVEL, m_iFear + 5 );
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}

		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
		{
			m_iFear -= 10;
			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT ( 3,5 );
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}

		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		{
			m_iFear = Q_min ( FEAR_LEVEL, m_iFear + 3 );

			m_flightSpeed = Q_min ( m_flMaxSpeed, m_flightSpeed + 40 );
		}
		return GetScheduleOfType( SCHED_STANDOFF );
	default:
		return CFlyingMonster::GetSchedule();
	}
}

Schedule_t* CFlybee::GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_IDLE_WALK:
		return slFlybeeSwimAround;

	case SCHED_STANDOFF:
		return slFlybeeCircleEnemy;

	case SCHED_FAIL:
		return slFlybeeSwimAgitated;

	case SCHED_DIE:
		return slFlybeeTwitchDie;

	case SCHED_MELEE_ATTACK1:
		return slFlybeeMeleeAttack;

	case SCHED_RANGE_ATTACK1:
		return slFlybeeRangeAttack1;

	case SCHED_RANGE_ATTACK2:

		if ( m_hEnemy->pev->velocity.IsLengthLessThanOrEqual(100) )
		{
			if ( (Center() - m_hEnemy->Center()).IsLengthLessThanOrEqual(300) && fabs(pev->origin.z - m_hEnemy->pev->origin.z) <= 128 )
				return slFlybeeRunAttack;
			else
			{
				if ( RANDOM_LONG ( 0,2 ) == 0 )
					return slFlybeeRangeAttack1;
				else
					return slFlybeeRangeAttack2;
			}
		}
		else
		{
			return slFlybeeRangeAttack1;
		}
	case SCHED_CHASE_ENEMY:
		AttackSound();
		// pssthrough
	default:
		return CFlyingMonster::GetScheduleOfType( Type );
	}
}

void CFlybee::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_FLYBEE_CIRCLE_ENEMY:
		break;

	case TASK_FLYBEE_SWIM:
		break;

	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_FLYBEE_STOP_MOVING:
		m_flightSpeed = 0;

		if ( m_IdealActivity == m_movementActivity )
		{
			m_IdealActivity = GetStoppedActivity();
		}

		RouteClear();
		TaskComplete();
		break;

	case TASK_FLYBEE_RUN_ATTACK:
		m_IdealActivity = ACT_CROUCH;
		m_flightSpeed = 128;
		RouteClear();
		break;

	case TASK_DIE:
		pev->movetype	= MOVETYPE_STEP;
		pev->angles.x = 0;
		UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8,8,8 ) );

		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_FLYBEE_RANGE_ATTACK2:
		m_vecAttack2 = m_hEnemy->Center();
		m_IdealActivity = ACT_RANGE_ATTACK2;
		break;

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CFlybee::RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FLYBEE_CIRCLE_ENEMY:
		if (m_hEnemy == 0)
		{
			TaskComplete( );
		}
		else if (FVisible( m_hEnemy ))
		{
			Vector vecFrom = m_hEnemy->EyePosition( );

			Vector vecDelta = (pev->origin - vecFrom).Normalize( );
			Vector vecSwim = CrossProduct( vecDelta, Vector( 0, 0, 1 ) ).Normalize( );
			
			if (DotProduct( vecSwim, m_SaveVelocity ) < 0)
				vecSwim *= -1.0f;

			Vector vecPos = vecFrom + vecDelta * m_idealDist + vecSwim * 32;

			TraceResult tr;
		
			UTIL_TraceHull( vecFrom, vecPos, ignore_monsters, large_hull, m_hEnemy->edict(), &tr );

			if (tr.flFraction > 0.5)
				vecPos = tr.vecEndPos;

			m_SaveVelocity = m_SaveVelocity * 0.8 + 0.2 * (vecPos - pev->origin).Normalize() * m_flightSpeed;

			if ( m_hEnemy->MyMonsterPointer()->FInViewCone ( this ) && m_hEnemy->FVisible( this ))
			{
				m_flNextAlert -= 0.1;

				if ( m_iFear < FEAR_LEVEL )
					m_iFear ++;

				if (m_idealDist < m_flMaxDist)
				{
					m_idealDist += 4;
				}
				if (m_flightSpeed < m_flMaxSpeed)
				{
					m_flightSpeed += 40;
				}
				else if (m_flightSpeed < m_flMinSpeed)
				{
					m_flightSpeed += 80;
				}
			}
			else 
			{
				m_flNextAlert += 0.1;

				if ( m_iFear > HATE_LEVEL )
					m_iFear --;


				if (m_idealDist > 128)
				{
					m_idealDist -= 4;
				}
				if (m_flightSpeed > m_flMinSpeed)
				{
					m_flightSpeed -= 3;
				}
			}

			if (m_flightSpeed < m_flMinSpeed)
			{
				m_flightSpeed = 128;
			}

			if (m_flightSpeed > m_flMaxSpeed)
			{
				m_flightSpeed = m_flMaxSpeed;
			}
		}
		else
		{
			m_flNextAlert = gpGlobals->time + 0.2;
		}

		if (m_flNextAlert < gpGlobals->time)
		{
			AlertSound( );
			m_flNextAlert = gpGlobals->time + RANDOM_FLOAT( 3, 5 );
		}
		break;

	case TASK_FLYBEE_RUN_ATTACK:
		{
			if (m_fSequenceFinished && m_IdealActivity == ACT_CROUCH )
			{
				m_IdealActivity = ACT_RUN;
			}
			else if (m_fSequenceFinished )
				TaskComplete( );

			TraceResult tr;
			UTIL_TraceHull( pev->origin, m_hEnemy->Center(), ignore_monsters, large_hull, m_hEnemy->edict(), &tr );

			if (tr.flFraction < 0.9)
			{
				TaskFail ();
				break;
			}

			m_SaveVelocity = (tr.vecEndPos - pev->origin).Normalize() * m_flightSpeed;

			m_flightSpeed = Q_min ( m_flMaxSpeed, m_flightSpeed * 1.2 );


			break;
		}

	case TASK_FLYBEE_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			break;
		}

	case TASK_FLYBEE_SWIM:
		if (m_fSequenceFinished )
		{
			TaskComplete( );
		}
		break;

	case TASK_DIE:

		if ( m_fSequenceFinished && m_IdealActivity == ACT_DIESIMPLE && (FBitSet( pev->flags, FL_ONGROUND) || pev->waterlevel == WL_Eyes) )
		{
			pev->deadflag = DEAD_DYING;
			m_IdealActivity = ACT_LAND;

			ResetSequenceInfo ();
		}
		else if ( m_fSequenceFinished && m_IdealActivity == ACT_DIESIMPLE )
		{
			pev->deadflag = DEAD_DYING;
		}
		else if ( m_fSequenceFinished && m_IdealActivity == ACT_LAND )
		{
			CFlyingMonster :: RunTask ( pTask );
			pev->deadflag = DEAD_DEAD;
		}
		break;

	default: 
		CFlyingMonster :: RunTask ( pTask );
		break;
	}
}

float CFlybee::VectorToPitch( const Vector &vec )
{
	float pitch;
	if (vec.z == 0 && vec.x == 0)
		pitch = 0;
	else
	{
		pitch = (int) (atan2(vec.z, sqrt(vec.x*vec.x+vec.y*vec.y)) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	return pitch;
}

void CFlybee::Move(float flInterval)
{
	CFlyingMonster::Move( flInterval );
}

float CFlybee::FlPitchDiff( void )
{
	float	flPitchDiff;
	float	flCurrentPitch;

	flCurrentPitch = UTIL_AngleMod( pev->angles.z );

	if ( flCurrentPitch == pev->idealpitch )
	{
		return 0;
	}

	flPitchDiff = pev->idealpitch - flCurrentPitch;

	if ( pev->idealpitch > flCurrentPitch )
	{
		if (flPitchDiff >= 180)
			flPitchDiff = flPitchDiff - 360;
	}
	else 
	{
		if (flPitchDiff <= -180)
			flPitchDiff = flPitchDiff + 360;
	}
	return flPitchDiff;
}

float CFlybee::ChangePitch( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlPitchDiff();
		float target = 0;
		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if (diff < -20)
				target = 45;
			else if (diff > 20)
				target = -45;
		}
		pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * 0.1 );
	}
	return 0;
}

float CFlybee::ChangeYaw( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlYawDiff();
		float target = 0;

		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if ( diff < -20 )
				target = 20;
			else if ( diff > 20 )
				target = -20;
		}
		pev->angles.z = UTIL_Approach( target, pev->angles.z, 220.0 * 0.1 );
	}
	return CFlyingMonster::ChangeYaw( speed );
}

Activity CFlybee::GetStoppedActivity( void )
{ 
	if ( pev->movetype != MOVETYPE_FLY )		// UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;
	return ACT_WALK;
}

void CFlybee::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	m_SaveVelocity = vecDir * m_flightSpeed;
}

void CFlybee::MonsterThink ( void )
{
	CFlyingMonster::MonsterThink( );

	if ( pev->deadflag == DEAD_NO && m_MonsterState != MONSTERSTATE_SCRIPT )
	{
		Swim( );
	}
}

void CFlybee::Stop( void )
{
	m_flightSpeed = 80.0;
}

void CFlybee::Swim()
{
	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

	if (FBitSet( pev->flags, FL_ONGROUND))
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT( -45, 45 );
		ClearBits( pev->flags, FL_ONGROUND );

		Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
		UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

		pev->velocity = Forward * 200 + Up * 200;

		return;
	}

	Angles = UTIL_VecToAngles( m_SaveVelocity );
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * Forward+Right);
	l = DoProbe(start + PROBE_LENGTH/3 * Forward-Right);
	u = DoProbe(start + PROBE_LENGTH/3 * Forward+Up);
	d = DoProbe(start + PROBE_LENGTH/3 * Forward-Up);

	Vector SteeringVector = f+r+l+u+d;
	m_SaveVelocity = (m_SaveVelocity + SteeringVector/2).Normalize();

	Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	float flDot = DotProduct( Forward, m_SaveVelocity );
	if (flDot > 0.5)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if (flDot > 0)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * (flDot + 0.5);
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 80;
	
	Angles = UTIL_VecToAngles( m_SaveVelocity );

	// Smooth Pitch
	//
	if (Angles.x > 180)
		Angles.x = Angles.x - 360;
	pev->angles.x = UTIL_Approach(Angles.x, pev->angles.x, 50 * 0.1 );
	if (pev->angles.x < -80) pev->angles.x = -80;
	if (pev->angles.x >  80) pev->angles.x =  80;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;

	if (fabs(Angles.y - pev->angles.y) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y;
	}
	if (fabs(Angles.y - pev->angles.y + 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	if (fabs(Angles.y - pev->angles.y - 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1;

	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
		{
			turn = -speed;
		}
		else
		{
			turn = speed;
		}
	}
	pev->angles.y += turn;
	pev->angles.z -= turn;
	pev->angles.y = fmod((pev->angles.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

	SetBoneController( 0, -yaw_adj / 4.0 );

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(Angles.z - pev->angles.z) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z;
	}
	if (fabs(Angles.z - pev->angles.z + 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	if (fabs(Angles.z - pev->angles.z - 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed/2 * 0.1;
	if (fabs(turn) < speed)
	{
		pev->angles.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			pev->angles.z -= speed;
		}
		else
		{
			pev->angles.z += speed;
		}
	}
	if (pev->angles.z < -20) pev->angles.z = -20;
	if (pev->angles.z >  20) pev->angles.z =  20;

	UTIL_MakeVectorsPrivate( Vector( -Angles.x, Angles.y, Angles.z ), Forward, Right, Up);

	// UTIL_MoveToOrigin ( ENT(pev), pev->origin + Forward * speed, speed, MOVE_STRAFE );
}


Vector CFlybee::DoProbe(const Vector &Probe)
{
	return CFlyingMonster::DoProbe(Probe, m_SaveVelocity);
}

CFlyBall *CFlyBall::CreateFlyBall( Vector vecOrigin, Vector vecAngles, entvars_s *pevOwner, EntityOverrides entityOverrides )
{
	CFlyBall *pBall = GetClassPtr( (CFlyBall *)NULL );

	UTIL_MakeAimVectors ( vecAngles );

	float x, y, z;
	// TODO: why such weird code?
	do
	{
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		z = x*x+y*y;
	}
	while (z > 1);

	Vector vecDir = gpGlobals->v_forward +
					x * VECTOR_CONE_6DEGREES.x * gpGlobals->v_right +
					y * VECTOR_CONE_6DEGREES.y * gpGlobals->v_up;

	pBall->pev->angles = UTIL_VecToAngles ( vecDir.Normalize() );

	pBall->pev->angles.x = -pBall->pev->angles.x;

	UTIL_SetOrigin( pBall->pev, vecOrigin );

	pBall->pev->owner = ENT ( pevOwner );

	pBall->AssignEntityOverrides(entityOverrides);

	pBall->Spawn();
	pBall->SetTouch( &CFlyBall::ExplodeTouch );
	
	return pBall;
}

void CFlyBall::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("flyball");

	ApplyVisual(GetVisual(ballVisual));

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CFlyBall::AnimateThink );
	SetTouch( &CFlyBall::ExplodeTouch );

	pev->dmgtime = gpGlobals->time; // keep track of when ball spawned
	pev->nextthink = gpGlobals->time + 0.1f;

	UTIL_MakeVectors ( pev->angles );
	pev->velocity = gpGlobals->v_forward * 1000;
}

void CFlyBall::Precache( void )
{
	RegisterAndPrecacheSoundScript(electroSoundScript);

	RegisterVisual(ballVisual);
	RegisterVisual(ballTrailVisual);
}

void CFlyBall::AnimateThink( void )
{
	pev->nextthink = gpGlobals->time + 0.05;

	float delta = gpGlobals->time - pev->dmgtime;

	if ( delta > 5 || pev->velocity.IsLengthLessThan(10) )
	{
		SetTouch( NULL );
		UTIL_Remove( this );
	}

	if ( delta > 0.0f && delta < 0.9f )
	{
		CSprite *pTrail = CreateSpriteFromVisual(GetVisual(ballTrailVisual), pev->origin);
		if (pTrail)
		{
			pTrail->AnimateAndDie ( pTrail->pev->framerate );
			pTrail->Expand ( pTrail->pev->scale, 120 );
		}
	}
}

void CFlyBall::ExplodeTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	TraceResult tr = UTIL_GetGlobalTrace();

	if (!pOther->pev->takedamage)
	{
		const int baseDecal = g_modFeatures.opfor_decals ? DECAL_OPFOR_SCORCH1 : DECAL_SMALLSCORCH1;
		UTIL_DecalTrace(&tr, baseDecal + RANDOM_LONG(0, 2));

		int iContents = UTIL_PointContents(pev->origin);

		if (iContents != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}
	}

	if (pOther->pev->takedamage)
	{
		if ( pOther->pev != VARS ( pev->owner ) )
		{
			CBaseEntity* pAttacker = !FNullEnt(pev->owner) ? CBaseEntity::Instance(pev->owner) : nullptr;
			pOther->ApplyTraceAttack(pev, pAttacker ? pAttacker->pev : pev, DamageInfo{gSkillData.flybeeDmgFlyball, DMG_ENERGYBEAM}, pev->velocity.Normalize(), &tr);
		}
	}

	EmitSoundScriptAmbient(tr.vecEndPos, electroSoundScript);

	pev->modelindex = 0;
	pev->solid = SOLID_NOT;
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.01f; // let the sound play
}
#endif
