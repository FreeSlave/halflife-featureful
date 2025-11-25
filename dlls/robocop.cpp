#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"explode.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

class CFireTrail : public CBaseEntity
{
public:
	void Spawn() override;
	void Think() override;
	void Touch( CBaseEntity *pOther ) override;
	int ObjectCaps() override { return FCAP_DONT_SAVE; }

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
private:
	int m_spriteScale; // what's the exact fireball sprite scale?
};

LINK_ENTITY_TO_CLASS( fire_trail, CFireTrail )

TYPEDESCRIPTION CFireTrail::m_SaveData[] =
{
	DEFINE_FIELD( CFireTrail, m_spriteScale, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFireTrail, CBaseEntity )

void CFireTrail::Spawn()
{
	pev->velocity = RANDOM_FLOAT( 100.0f, 150.0f ) * pev->angles;
	if( RANDOM_LONG( 0, 1 ) )
		pev->velocity.x += RANDOM_FLOAT(-300.0f, -100.0f);
	else
		pev->velocity.x += RANDOM_FLOAT(100.0f, 300.0f);

	if( RANDOM_LONG( 0, 1 ) )
		pev->velocity.y += RANDOM_FLOAT(-300.0f, -100.0f);
	else
		pev->velocity.y += RANDOM_FLOAT(100.0f, 300.0f);

	if( pev->velocity.z >= 0 )
		pev->velocity.z += 200.0f;
	else
		pev->velocity.z -= 200.0f;

	m_spriteScale = RANDOM_LONG( 7, 13 );
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5f;
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "models/grenade.mdl" );	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT( 0.5f, 1.5f );
	pev->maxspeed = pev->speed;

	pev->angles = g_vecZero;
}

void CFireTrail::Think()
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION);
		WRITE_VECTOR( pev->origin );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( (BYTE)( m_spriteScale * pev->speed ) ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
	MESSAGE_END();

	pev->speed -= 0.1f;

	if( pev->speed > 0 )
		pev->nextthink = gpGlobals->time + 0.1f;
	else
		UTIL_Remove( this );

	pev->flags &= ~FL_ONGROUND;
}

void CFireTrail::Touch( CBaseEntity *pOther )
{
	if( pev->flags & FL_ONGROUND )
		pev->velocity = pev->velocity * 0.1f;
	else
		pev->velocity = pev->velocity * 0.6f;

	if( ( pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y ) < 10.0f )
		pev->speed = 0;
}

void SpawnExplosion( Vector center, float randomRange, float time, int magnitude );

#define ROBOCOP_DAMAGE				(DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST)

#define ROBOCOP_DEATH_DURATION			2.1f

#define LF_ROBOCOP_LASER		1
#define LF_ROBOCOP_BEAMSPOT		2
#define LF_ROBOCOP_BEAM			4
#define LF_ROBOCOP_LOWBRIGHTNESS	8
#define LF_ROBOCOP_HIGHBRIGHTNESS	16
#define LF_ROBOCOP_FULLBRIGHTNESS	32

#define ROBOCOP_AE_RIGHT_FOOT		0x03
#define ROBOCOP_AE_LEFT_FOOT		0x04
#define ROBOCOP_AE_FIST				0x05

class CRoboCop : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("robocop"); }
	void UpdateOnRemove() override;
	void RemoveSpriteEffects();
	void SetYawSpeed() override;
	int  DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Robocop"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void SetActivity( Activity NewActivity ) override;

	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	bool CheckMeleeAttack2( float flDot, float flDist ) override { return false; }
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	bool CheckRangeAttack2( float flDot, float flDist ) override { return false; }

	void SetObjectCollisionBox() override
	{
		SetMyObjectCollisionBox(Vector( -80, -80, 0 ), Vector( 80, 80, 214 ));
	}

	void PrescheduleThink() override;
	void OnChangeSchedule( Schedule_t* pNewSchedule ) override;

	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;

	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;

	Schedule_t *GetScheduleOfType( int Type ) override;
	void StartTask( Task_t *pTask ) override;
	void RunTask( Task_t *pTask ) override;

	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	DamageInfo DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo) override;

	void FistAttack();
	void CreateLaser();
	void ChangeLaserState();
	void HeadControls( float angleX, float angleY, bool zeropoint );

	int DefaultSizeForGrapple() override { return GRAPPLE_NOT_A_TARGET; }
	bool IsDisplaceable() override { return false; }

	Vector DefaultMinHullSize() override { return Vector( -40.0f, -40.0f, 0.0f ); }
	Vector DefaultMaxHullSize() override { return Vector( 40.0f, 40.0f, 200.0f ); }

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	CUSTOM_SCHEDULES

	CSprite	*m_pLaserPointer;
	CBeam	*m_pBeam;
	CSprite	*m_pBeamSpot;
	int	m_iLaserFlags;
	// int	m_iLaserAlpha;
	float	m_flHeadX;
	float	m_flHeadY;
	float	m_flLaserTime;
	float	m_flFistTime;
	float	m_flSparkTime;
	Vector	m_vecAimPos;
	bool m_vecAimPosSet;

	int m_renderAmtAlive;
	int m_renderFxAlive;
	Vector m_renderColorAlive;
	bool m_renderAliveSaved;

	int m_RobocopGibModel;

	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript chargeSoundScript;
	static const NamedSoundScript fistSoundScript;
	static const NamedSoundScript laserSoundScript;
	static const NamedSoundScript stepSoundScript;

	static constexpr const char* sparkSoundScript = "RoboCop.Spark";

	static const NamedVisual eyeVisual;
	static const NamedVisual spotVisual;
	static const NamedVisual beamVisual;
	static const NamedVisual shockWaveBaseVisual;
	static const NamedVisual shockWave1Visual;
	static const NamedVisual shockWave2Visual;
	static const NamedVisual shockWave3Visual;

	const Visual* m_eyeVisual;
	const Visual* m_spotVisual;
	const Visual* m_beamVisual;
};

LINK_ENTITY_TO_CLASS( monster_robocop, CRoboCop )

TYPEDESCRIPTION	CRoboCop::m_SaveData[] =
{
	DEFINE_FIELD( CRoboCop, m_pLaserPointer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboCop, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboCop, m_pBeamSpot, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboCop, m_iLaserFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CRoboCop, m_flLaserTime, FIELD_TIME ),
	DEFINE_FIELD( CRoboCop, m_flFistTime, FIELD_TIME ),
	DEFINE_FIELD( CRoboCop, m_flSparkTime, FIELD_TIME ),
	DEFINE_FIELD( CRoboCop, m_flHeadX, FIELD_FLOAT ),
	DEFINE_FIELD( CRoboCop, m_flHeadY, FIELD_FLOAT ),
	DEFINE_FIELD( CRoboCop, m_vecAimPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CRoboCop, m_vecAimPosSet, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRoboCop, m_renderAmtAlive, FIELD_INTEGER ),
	DEFINE_FIELD( CRoboCop, m_renderFxAlive, FIELD_INTEGER ),
	DEFINE_FIELD( CRoboCop, m_renderColorAlive, FIELD_VECTOR ),
	DEFINE_FIELD( CRoboCop, m_renderAliveSaved, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CRoboCop, CBaseMonster )

const NamedSoundScript CRoboCop::dieSoundScript = {
	CHAN_ITEM,
	{"ambience/sparks.wav"},
	"RoboCop.Die"
};

const NamedSoundScript CRoboCop::painSoundScript = {
	CHAN_VOICE,
	{},
	"RoboCop.Pain"
};

const NamedSoundScript CRoboCop::chargeSoundScript = {
	CHAN_BODY,
	{"robocop/rc_charge.wav"},
	"RoboCop.Charge"
};

const NamedSoundScript CRoboCop::fistSoundScript = {
	CHAN_WEAPON,
	{"robocop/rc_fist.wav"},
	IntRange(90, 110),
	"RoboCop.Fist"
};

const NamedSoundScript CRoboCop::laserSoundScript = {
	CHAN_BODY,
	{"robocop/rc_laser.wav"},
	"RoboCop.Laser"
};

const NamedSoundScript CRoboCop::stepSoundScript = {
	CHAN_BODY,
	{"robocop/rc_step1.wav", "robocop/rc_step2.wav"},
	IntRange(90, 110),
	"RoboCop.Step"
};

const NamedVisual CRoboCop::eyeVisual = BuildVisual("RoboCop.Eye")
		.Model("sprites/gargeye1.spr")
		.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 255, kRenderFxNone)
		.Scale(0.5f);

const NamedVisual CRoboCop::spotVisual = BuildVisual("RoboCop.BeamSpot")
		.Model("sprites/gargeye1.spr")
		.RenderProps(kRenderTransAdd, Color3(255, 255, 255), 255, kRenderFxNone)
		.Scale(0.3f);

const NamedVisual CRoboCop::beamVisual = BuildVisual("RoboCop.Beam")
		.Model("sprites/smoke.spr")
		.BeamParams(32, 0, 15)
		.RenderColor(255, 0, 0)
		.Alpha(255);

const NamedVisual CRoboCop::shockWaveBaseVisual = BuildVisual("RoboCop.ShockWaveBase")
		.Model("sprites/xbeam3.spr")
		.Framerate(10)
		.Life(0.2f)
		.BeamWidth(32)
		.Alpha(255);

const NamedVisual CRoboCop::shockWave1Visual = BuildVisual("RoboCop.ShockWave1")
		.RenderColor(101, 133, 221)
		.Life(0.2f)
		.Mixin(&CRoboCop::shockWaveBaseVisual);

const NamedVisual CRoboCop::shockWave2Visual = BuildVisual("RoboCop.ShockWave2")
		.RenderColor(67, 85, 255)
		.Life(0.3f)
		.Mixin(&CRoboCop::shockWaveBaseVisual);

const NamedVisual CRoboCop::shockWave3Visual = BuildVisual("RoboCop.ShockWave3")
		.RenderColor(62, 33, 211)
		.Life(0.4f)
		.Mixin(&CRoboCop::shockWaveBaseVisual);

enum
{
	SCHED_ROBOCOP_LASERFAIL = LAST_COMMON_SCHEDULE + 1,
};

enum
{
	TASK_ROBOCOP_LASER_SOUND = LAST_COMMON_TASK + 1,
	TASK_ROBOCOP_LASER_CHARGE,
	TASK_ROBOCOP_LASER_ON,
	TASK_ROBOCOP_MORTAR_SPAWN,
	TASK_ROBOCOP_LASER_OFF
};

Task_t	tlRoboCopLaser[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_ROBOCOP_LASERFAIL },
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_RANGE_ATTACK1 },
	{ TASK_ROBOCOP_LASER_CHARGE, 2.0f },
	{ TASK_ROBOCOP_LASER_SOUND, 0.0f },
	{ TASK_ROBOCOP_LASER_ON, 1.0f },
	{ TASK_ROBOCOP_MORTAR_SPAWN, 0.1f },
	{ TASK_ROBOCOP_LASER_OFF, 1.0f }
};

Schedule_t	slRoboCopLaser[] =
{
	{
		tlRoboCopLaser,
		ARRAYSIZE( tlRoboCopLaser ),
		bits_COND_TASK_FAILED |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE,
		0,
		"RoboCopLaser"
	}
};

Task_t	tlRoboCopLaserFail[] =
{
	{ TASK_ROBOCOP_LASER_OFF, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 1.0f },
	{ TASK_WAIT_PVS, 0.0f }
};

Schedule_t	slRoboCopLaserFail[] =
{
	{
		tlRoboCopLaserFail,
		ARRAYSIZE( tlRoboCopLaserFail ),
		bits_COND_CAN_ATTACK,
		0,
		"RoboCopLaserFail"
	}
};

DEFINE_CUSTOM_SCHEDULES( CRoboCop )
{
	slRoboCopLaser,
	slRoboCopLaserFail
};

IMPLEMENT_CUSTOM_SCHEDULES( CRoboCop, CBaseMonster )

void CRoboCop::FistAttack()
{
	Vector vecDist;
	float flDist, flAdjustedDamage;

	UTIL_MakeVectors( pev->angles );
	Vector vecSrc = pev->origin + 12 * gpGlobals->v_right + 95 * gpGlobals->v_forward;

	const Visual* waveVisuals[] = {GetVisual(shockWave1Visual), GetVisual(shockWave2Visual), GetVisual(shockWave3Visual)};

	for( size_t i = 0; i < ARRAYSIZE(waveVisuals); i++ )
	{
		if (waveVisuals[i] && waveVisuals[i]->modelIndex)
		{
			// blast circles
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMCYLINDER );
				WRITE_COORD( vecSrc.x );
				WRITE_COORD( vecSrc.y );
				WRITE_COORD( vecSrc.z + 16 );
				WRITE_COORD( vecSrc.x );
				WRITE_COORD( vecSrc.y );
				WRITE_COORD( vecSrc.z + gSkillData.robocopSWRadius / ( ( i + 1 ) * 0.2f ) ); // reach damage radius over .3 seconds
				WriteBeamVisual(waveVisuals[i]);
			MESSAGE_END();
		}
	}

	CBaseEntity *pEntity = NULL;

	// iterate on all entities in the vicinity.
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, gSkillData.robocopSWRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// Robocop does not take damage from it's own attacks.
			if( pEntity != this )
			{
				vecDist = pEntity->Center() - vecSrc;
				flDist = Q_max( 0, gSkillData.robocopSWRadius - vecDist.Length() );

				flDist = flDist / gSkillData.robocopSWRadius;

				if( !FVisible( pEntity ) )
				{
					if( pEntity->IsPlayer() )
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still 
						// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flDist *= 0.5f;
					}
					else if( !FClassnameIs( pEntity->pev, "func_breakable" ) && !FClassnameIs( pEntity->pev, "func_pushable" ) )
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flDist = 0;
					}
				}

				flAdjustedDamage = gSkillData.robocopDmgFist * flDist;

				if( flAdjustedDamage > 0 )
				{
					pEntity->TakeDamage( pev, pev, DamageInfo(flAdjustedDamage, DMG_SONIC) );
				}

				if( pEntity->IsPlayer() )
				{
					vecDist.NormalizeInPlace();
					vecDist.x = vecDist.x * flDist * 600.0f;
					vecDist.y = vecDist.y * flDist * 600.0f;
					vecDist.z = flDist * 450.0f;
					pEntity->pev->velocity = vecDist + pEntity->pev->velocity;
					pEntity->pev->punchangle.x = 5;
				}
			}
		}
	}

	UTIL_ScreenShake( pev->origin, 12.0f, 100.0f, 2.0f, 1000 );
	EmitSoundScript(fistSoundScript);
}

void CRoboCop::CreateLaser()
{
	m_pLaserPointer = CreateSpriteFromVisual(m_eyeVisual, pev->origin);
	if (m_pLaserPointer)
	{
		m_pLaserPointer->SetAttachment( edict(), 1 );
		m_pLaserPointer->SetBrightness(0);
	}

	m_pBeamSpot = CreateSpriteFromVisual(m_spotVisual, pev->origin);
	if (m_pBeamSpot)
	{
		m_pBeamSpot->SetBrightness(0);
	}

	m_pBeam = CreateBeamFromVisual(m_beamVisual);
	if (m_pBeam)
	{
		m_pBeam->PointEntInit( pev->origin, entindex() );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetBrightness( 0 );
	}

	ChangeLaserState();
}

void CRoboCop::ChangeLaserState()
{
	float time;
	float brightnessFraction;

	if( m_pBeam )
		m_pBeam->SetEndAttachment( 1 );

	if( !m_iLaserFlags )
	{
		if( m_pBeam )
			m_pBeam->SetBrightness( 0 );

		if( m_pLaserPointer )
			m_pLaserPointer->SetBrightness( 0 );

		if( m_pBeamSpot )
			m_pBeamSpot->SetBrightness( 0 );

		return;
	}

	if( m_iLaserFlags & LF_ROBOCOP_LOWBRIGHTNESS )
	{
		time = gpGlobals->time;
	}
	else
	{
		if( !( m_iLaserFlags & LF_ROBOCOP_HIGHBRIGHTNESS ) )
		{
			if( m_iLaserFlags & LF_ROBOCOP_FULLBRIGHTNESS )
			{
				brightnessFraction = 0.875f;
				goto end;
			}
		}

		time = gpGlobals->time * 5.0f;
	}

	brightnessFraction = fabs( sin( time ) );

end:
	if( m_iLaserFlags & LF_ROBOCOP_LASER )
	{
		if( m_pLaserPointer )
			m_pLaserPointer->SetBrightness( m_eyeVisual->renderamt * brightnessFraction );
	}

	if( m_iLaserFlags & LF_ROBOCOP_BEAM )
	{
		if( m_pBeam )
			m_pBeam->SetBrightness( m_beamVisual->renderamt * brightnessFraction );
	}

	if( m_iLaserFlags & LF_ROBOCOP_BEAMSPOT )
	{
		if( m_pBeamSpot )
			m_pBeamSpot->SetBrightness( m_spotVisual->renderamt * brightnessFraction );
	}
}

void CRoboCop::HeadControls( float angleX, float angleY, bool zeropoint )
{
	if( angleY < -180 )
		angleY += 360;
	else if( angleY > 180 )
		angleY -= 360;

	if( angleY < -45 )
		angleY = -45;
	else if( angleY > 45 )
		angleY = 45;

	if( zeropoint )
	{
		m_flHeadX = angleX;
		m_flHeadY = angleY;
	}
	else
	{
		m_flHeadX = UTIL_ApproachAngle( angleX, m_flHeadX, 4 );
		m_flHeadY = UTIL_ApproachAngle( angleY, m_flHeadY, 8 );
	}

	SetBoneController( 0, m_flHeadY );
	SetBoneController( 1, m_flHeadX );
}

void CRoboCop::PrescheduleThink()
{
	if( m_flLaserTime <= gpGlobals->time && m_iLaserFlags != ( LF_ROBOCOP_LASER | LF_ROBOCOP_LOWBRIGHTNESS ) )
	{
		m_iLaserFlags = 0;
		ChangeLaserState();
		m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_LOWBRIGHTNESS );
	}

	ChangeLaserState();
}

void CRoboCop::OnChangeSchedule(Schedule_t *pNewSchedule)
{
	if (m_iLaserFlags & LF_ROBOCOP_BEAM)
	{
		HeadControls( 0.0f, 0.0f, true );
		m_iLaserFlags = 0;
		ChangeLaserState();
	}
	CBaseMonster::OnChangeSchedule(pNewSchedule);
}

int CRoboCop::DefaultClassify()
{
	return	CLASS_MACHINE;
}

void CRoboCop::SetYawSpeed()
{
	int ys;

	switch( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CRoboCop::Spawn()
{
	Precache();

	SetMyModel( "models/robocop.mdl" );
	SetMySize();

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor(DONT_BLEED);
	SetMyHealth(gSkillData.robocopHealth);
	SetMyFieldOfView(VIEW_FIELD_WIDE);
	m_MonsterState		= MONSTERSTATE_NONE;
	SetMyCanOpenDoors(false);

	MonsterInit();

	CreateLaser();

	m_flFistTime = gpGlobals->time + 1.0f;
}

void CRoboCop::Precache()
{
	PrecacheMyModel( "models/robocop.mdl" );
	PrecacheMyGibModel();

	m_RobocopGibModel = PRECACHE_MODEL( "models/metalplategibs.mdl" );

	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(chargeSoundScript);
	RegisterAndPrecacheSoundScript(fistSoundScript);
	RegisterAndPrecacheSoundScript(laserSoundScript);
	RegisterAndPrecacheSoundScript(stepSoundScript);

	SoundScriptParamOverride param;
	param.OverrideChannel(CHAN_VOICE);
	param.OverrideVolumeRelative(0.6f);
	RegisterAndPrecacheSoundScript(sparkSoundScript, ::sparkBaseSoundScript, param);

	m_eyeVisual = RegisterVisual(eyeVisual);
	m_spotVisual = RegisterVisual(spotVisual);
	m_beamVisual = RegisterVisual(beamVisual);

	RegisterVisual(shockWave1Visual);
	RegisterVisual(shockWave2Visual);
	RegisterVisual(shockWave3Visual);

	UTIL_PrecacheOther( "monster_mortar" );
}

void CRoboCop::UpdateOnRemove()
{
	CBaseEntity::UpdateOnRemove();
	RemoveSpriteEffects();
}

void CRoboCop::RemoveSpriteEffects()
{
	UTIL_Remove( m_pLaserPointer );
	UTIL_Remove( m_pBeam );
	UTIL_Remove( m_pBeamSpot );

	m_pLaserPointer = NULL;
	m_pBeam = NULL;
	m_pBeamSpot = NULL;
}

DamageInfo CRoboCop::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	DamageInfo damageInfo = inputDamageInfo;

	damageInfo.type &= ROBOCOP_DAMAGE;

	if( IsAlive() && !FBitSet( damageInfo.type, ROBOCOP_DAMAGE ) )
	{
		if( pev->dmgtime != gpGlobals->time || (RANDOM_LONG( 0, 100 ) < 20 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 0.5f, 1.5f ) );
			pev->dmgtime = gpGlobals->time;
		}

		damageInfo.damage = 0.0f;
	}

	return damageInfo;
}

DamageInfo CRoboCop::DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo)
{
	DamageInfo damageInfo = inputDamageInfo;
	if( IsAlive() )
	{
		if( !( damageInfo.type & ROBOCOP_DAMAGE ) )
			damageInfo.damage *= 0.01f;

		if( damageInfo.type & DMG_BLAST )
			SetConditions( bits_COND_LIGHT_DAMAGE );
	}
	return damageInfo;
}

PainSoundRule CRoboCop::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = FloatRange{2.5f, 4.0f};
	return rule;
}

void CRoboCop::PainSound()
{
	EmitSoundScript(painSoundScript);
}

KilledResult CRoboCop::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	RemoveSpriteEffects();
	return CBaseMonster::Killed( pevInflictor, pevAttacker, GIB_NEVER );
}

bool CRoboCop::CheckMeleeAttack1( float flDot, float flDist )
{
	if( m_flFistTime <= gpGlobals->time )
	{
		CheckMeleeAttackParams params;
		params.dot = 0.8f;
		params.distance = gSkillData.robocopSWRadius;
		return CheckMeleeAttackImpl(flDot, flDist, params, false);
	}
	return false;
}

bool CRoboCop::CheckRangeAttack1( float flDot, float flDist )
{
	if( m_flLaserTime <= gpGlobals->time )
	{
		if( flDot >= 0.8f && flDist > gSkillData.robocopSWRadius )
		{
			if( flDist < 4096.0f )
				return true;
		}
	}

	return false;
}

void CRoboCop::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ROBOCOP_AE_RIGHT_FOOT:
	case ROBOCOP_AE_LEFT_FOOT:
		UTIL_ScreenShake( pev->origin, 4.0f, 3.0f, 1.0f, 250.0f );
		EmitSoundScript(stepSoundScript);
		break;
	case ROBOCOP_AE_FIST:
		FistAttack();
		m_flLaserTime = gpGlobals->time + 2.0f;
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

Schedule_t *CRoboCop::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return slRoboCopLaser;

	case SCHED_ROBOCOP_LASERFAIL:
		return slRoboCopLaserFail;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

void CRoboCop::StartTask( Task_t *pTask )
{
	TraceResult tr;

	switch( pTask->iTask )
	{
	case TASK_ROBOCOP_LASER_CHARGE:
		{
			EmitSoundScript(chargeSoundScript);
			m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_HIGHBRIGHTNESS );
			m_flWaitFinished = gpGlobals->time + pTask->flData;
			m_flFistTime = m_flLaserTime = gpGlobals->time + 9.0f;

			ChangeLaserState();
			m_vecAimPosSet = false;
		}
		break;

	case TASK_ROBOCOP_LASER_ON:
		{
			m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_BEAM | LF_ROBOCOP_BEAMSPOT | LF_ROBOCOP_FULLBRIGHTNESS );
			m_flWaitFinished = gpGlobals->time + pTask->flData;

			if( m_pBeam )
			{
				m_pBeam->pev->origin = m_vecAimPos;
				m_pBeam->SetEndAttachment( 1 );
				m_pBeam->RelinkBeam();
			}

			if( m_pBeamSpot )
				m_pBeamSpot->pev->origin = m_vecAimPos;

			m_failSchedule = 0;

			ChangeLaserState();
		}
		break;

	case TASK_ROBOCOP_MORTAR_SPAWN:
		{
			ExplosionCreate(m_vecAimPos, g_vecZero, edict(), gSkillData.robocopDmgMortar, true, pev);
			UTIL_ScreenShake( tr.vecEndPos, 25.0f, 150.0f, 1.0f, 750 );
			m_flWaitFinished = gpGlobals->time + pTask->flData; 
		}
		break;
	case TASK_ROBOCOP_LASER_SOUND:
		EmitSoundScript(laserSoundScript);
		TaskComplete();
		break;
	case TASK_ROBOCOP_LASER_OFF:
	{
		HeadControls( 0.0f, 0.0f, true );
		m_iLaserFlags = 0;
		ChangeLaserState();
		m_flFistTime = gpGlobals->time + 2.0f;
		m_flLaserTime = gpGlobals->time + 1.0f;

		const SoundScript *soundScript = GetSoundScript(chargeSoundScript);
		if (soundScript)
		{
			EMIT_SOUND(ENT( pev ), soundScript->channel, "common/null.wav", 1.0f, ATTN_NORM);
		}
		TaskComplete();
	}
		break;
	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + ROBOCOP_DEATH_DURATION;
		m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_LOWBRIGHTNESS );

		if (!m_renderAliveSaved)
		{
			m_renderAmtAlive = pev->renderamt;
			m_renderFxAlive = pev->renderfx;
			m_renderColorAlive = pev->rendercolor;
			m_renderAliveSaved = true;
		}

		pev->renderamt = 15;
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor = Vector( 67, 85, 255 );

		pev->health = 0;
		EmitSoundScript(dieSoundScript);
		m_flSparkTime = gpGlobals->time + 0.3f;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CRoboCop::RunTask( Task_t *pTask )
{
	TraceResult tr;

	switch( pTask->iTask )
	{
	case TASK_ROBOCOP_LASER_ON:
	case TASK_ROBOCOP_MORTAR_SPAWN:
		if( gpGlobals->time > m_flWaitFinished )
			TaskComplete();
		break;
	case TASK_ROBOCOP_LASER_CHARGE:
	{
		if( gpGlobals->time > m_flWaitFinished )
			TaskComplete();

		if( m_hEnemy == 0 )
		{
			TaskFail("no enemy");
			return;
		}

		const Vector enemyPos = m_hEnemy->Center();
		const bool enemyIsFlying = m_hEnemy->pev->movetype == MOVETYPE_FLY;
		Vector targetPos;

		if (!enemyIsFlying)
		{
			UTIL_TraceLine( enemyPos, enemyPos - Vector(0, 0, 4096), ignore_monsters, edict(), &tr );
			targetPos = tr.vecEndPos;
		}
		else
			targetPos = enemyPos;

		Vector vecSrc, vecAngle, vecDir;
		GetAttachment( 0, vecSrc, vecAngle );
		vecDir = ( targetPos - vecSrc ).Normalize();

		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 4096, dont_ignore_monsters, edict(), &tr );
		if (tr.pHit == m_hEnemy->edict())
		{
			m_vecAimPos = targetPos;
		}
		else if (!m_vecAimPosSet || tr.flFraction == 1.0f)
		{
			m_vecAimPos = tr.vecEndPos;
		}
		m_vecAimPosSet = true;

		vecAngle = UTIL_VecToAngles( vecDir );

		vecAngle.y = UTIL_AngleDiff( vecAngle.y, pev->angles.y );
		vecAngle.x = -vecAngle.x;

		if( fabs( vecAngle.y ) > 60.0f )
		{
			TaskFail("enemy is out of FOV");
			return;
		}

		HeadControls( vecAngle.x, vecAngle.y, false );
	}
		break;

	case TASK_DIE:
		if( m_flWaitFinished <= gpGlobals->time )
		{
			if( m_fSequenceFinished && pev->frame >= 255.0f )
			{
				const SoundScript *soundScript = GetSoundScript(dieSoundScript);
				if (soundScript)
				{
					EMIT_SOUND(ENT( pev ), soundScript->channel, "common/null.wav", 1.0, ATTN_NORM);
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
					WRITE_SHORT( m_RobocopGibModel );   //model id#

					// # of shards
					WRITE_BYTE( 20 );

					// duration
					WRITE_BYTE( 20 );// 3.0 seconds

					// flags
					WRITE_BYTE( BREAK_METAL );
				MESSAGE_END();

				SpawnExplosion( pev->origin, 70, 0, 150 );

				int trailCount = RANDOM_LONG( 2, 4 );

				for( int i = 0; i < trailCount; i++ )
					Create( "fire_trail", pev->origin, Vector( 0, 0, 1 ), NULL );

				// Restore original render values
				pev->renderamt = m_renderAmtAlive;
				pev->renderfx = m_renderFxAlive;
				pev->rendercolor = m_renderColorAlive;

				SetBodygroup( 0, 1 );

				CBaseMonster::RunTask( pTask );
				return;
			}
		}

		if( gpGlobals->time <= m_flSparkTime )
		{
			Vector headLocation;
			Vector headAngle;
			GetAttachment( 0, headLocation, headAngle );

			Vector sparkLocation = pev->origin;
			sparkLocation.z += RANDOM_FLOAT(0,4);
			sparkLocation.x = (pev->origin.x + headLocation.x) * 0.5f + RANDOM_FLOAT(-pev->size.x, pev->size.x) * 0.3;
			sparkLocation.y = (pev->origin.y + headLocation.y) * 0.5f + RANDOM_FLOAT(-pev->size.y, pev->size.y) * 0.3;

			Create( "spark_shower", sparkLocation, Vector( 0, 0, 1 ), NULL );
			EmitSoundScript(sparkSoundScript);
			m_flSparkTime = gpGlobals->time + 0.5f;
		}
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

void CRoboCop::SetActivity( Activity NewActivity )
{
	CBaseMonster::SetActivity( NewActivity );

	switch( m_Activity )
	{
	case ACT_WALK:
		m_flGroundSpeed = 220.0f;
		break;
	default:
		m_flGroundSpeed = 220.0f;
		break;
	}
}
