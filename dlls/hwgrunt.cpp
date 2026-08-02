#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"hgrunt.h"
#include	"animation.h"
#include	"game.h"
#include	"combat.h"
#include	"gamerules.h"

#define GUN_GROUP 1

#define GUN_MINIGUN 0
#define GUN_NONE 1

#define HWGRUNT_AE_SHOOT_PISTOL 1
#define HWGRUNT_AE_DROP_GUN 11

enum
{
	SCHED_HWGRUNT_SHOOT = LAST_FOLLOWINGMONSTER_SCHEDULE + 1,
	SCHED_HWGRUNT_SPINDOWN,
	SCHED_HWGRUNT_SUPPRESSING_FIRE,
	SCHED_HWGRUNT_REPEL,
	SCHED_HWGRUNT_REPEL_ATTACK,
};

enum
{
	TASK_HWGRUNT_PLAY_SPINDOWN = LAST_FOLLOWINGMONSTER_TASK + 1,
	TASK_HWGRUNT_RELOAD,
};

#define HWGRUNT_CLIP 100

class CHWGrunt : public CFollowingMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("hwgrunt"); }
	void SetYawSpeed() override;
	int DefaultClassify() override { return CLASS_HUMAN_MILITARY; }
	const char* DefaultDisplayName() override { return "Heavy Weapons Grunt"; }
	const char* ReverseRelationshipModel() override { return "models/hwgruntf.mdl"; }
	int DefaultISoundMask() override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int LookupActivity(int activity) override;
	void SetActivity( Activity NewActivity ) override;
	bool CheckMeleeAttack1( float flDot, float flDist ) override {
		return false;
	}
	bool CheckMeleeAttack2( float flDot, float flDist ) override {
		return false;
	}
	bool CheckRangeAttack1( float flDot, float flDist ) override;
	bool CheckRangeAttack2( float flDot, float flDist ) override {
		return false;
	}
	bool PerceiveEnemyAsOccluded(CBaseEntity *pEnemy, CBaseEntity *pOccluder) override;
	void StartTask( Task_t *pTask ) override;
	void RunTask( Task_t *pTask ) override;

	void PlayUseSentence() override;
	void PlayUnUseSentence() override;

	void DeathSound() override;
	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;
	void Shoot();
	void FinishReload();

	void PrescheduleThink() override;
	bool ShouldAnticipateLanding();

	Schedule_t *GetSchedule() override;
	Schedule_t *GetScheduleOfType( int Type ) override;
	void OnChangeSchedule( Schedule_t *pNewSchedule ) override;

	int SizeForGrapple() override { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() override { return true; }
	Vector DefaultMinHullSize() override { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() override { return VEC_HUMAN_HULL_MAX; }

	void GibMonster() override;
	void DropMyItems(bool isGibbed);

	void DetectModelType();

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

	EHANDLE m_lastOccluder;
	bool m_firing;
	bool m_sc5Model;

	int		m_iM249Shell;
	int		m_iM249Link;

	static constexpr const char* painSoundScript = "HWGrunt.Pain";
	static constexpr const char* dieSoundScript = "HWGrunt.Die";

	static const NamedSoundScript shootSoundScript;
	static const NamedSoundScript spinDownSoundScript;
	static const NamedSoundScript spinUpSoundScript;

	static constexpr const char* useSoundScript = "HWGrunt.Use";
	static constexpr const char* unuseSoundScript = "HWGrunt.UnUse";
};

LINK_ENTITY_TO_CLASS( monster_hwgrunt, CHWGrunt )

TYPEDESCRIPTION	CHWGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CHWGrunt, m_lastOccluder, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CHWGrunt, CFollowingMonster )

const NamedSoundScript CHWGrunt::shootSoundScript = {
	CHAN_WEAPON,
	{"hassault/hw_shoot2.wav", "hassault/hw_shoot3.wav"},
	"HWGrunt.Shoot"
};

const NamedSoundScript CHWGrunt::spinDownSoundScript = {
	CHAN_WEAPON,
	{"hassault/hw_spindown.wav"},
	"HWGrunt.Spindown"
};

const NamedSoundScript CHWGrunt::spinUpSoundScript = {
	CHAN_WEAPON,
	{"hassault/hw_spinup.wav"},
	"HWGrunt.Spinup"
};

void CHWGrunt::Spawn()
{
	Precache();

	SetMyModel( "models/hwgrunt.mdl" );
	DetectModelType();
	SetMySize();

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	pev->effects		= 0;
	SetMyHealth( GetSkillValue("hwgrunt_health") );
	SetMyFieldOfView(0.2);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;

	m_afCapability		= bits_CAP_TURN_HEAD;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(true);

	m_fEnemyEluded		= false;

	m_HackedGunPos = Vector( 0, 0, 55 );
	m_cClipSize = HWGRUNT_CLIP;
	UpdateClipSizeForWeapon(m_cClipSize);
	m_cAmmoLoaded = m_cClipSize;

	FollowingMonsterInit();
}

void CHWGrunt::Precache()
{
	PrecacheMyModel("models/hwgrunt.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript, CHGrunt::painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, CHGrunt::dieSoundScript);

	RegisterAndPrecacheSoundScript(shootSoundScript);
	RegisterAndPrecacheSoundScript(spinDownSoundScript);
	RegisterAndPrecacheSoundScript(spinUpSoundScript);

	RegisterAndPrecacheSoundScript(useSoundScript, CHGrunt::useSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript, CHGrunt::unuseSoundScript);

	if (pev->modelindex)
		DetectModelType();

	m_iM249Shell = PRECACHE_MODEL ("models/saw_shell.mdl");// saw shell
	m_iM249Link = PRECACHE_MODEL ("models/saw_link.mdl");// saw link
}

void CHWGrunt::SetYawSpeed()
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

int CHWGrunt::DefaultISoundMask()
{
	return	bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_PLAYER |
			bits_SOUND_DANGER;
}

void CHWGrunt::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch(pEvent->event)
	{
	case HWGRUNT_AE_SHOOT_PISTOL:
		// TODO: Not supported yet
		break;
	case HWGRUNT_AE_DROP_GUN:
		if (GetBodygroup(GUN_GROUP) != GUN_NONE)
			DropMyItems(false);
		break;
	default:
		CFollowingMonster::HandleAnimEvent(pEvent);
		break;
	}
}

int CHWGrunt::LookupActivity(int activity)
{
	if (m_sc5Model)
	{
		int iSequence = -1;
		switch(activity)
		{
		case ACT_WALK:
			iSequence = LookupSequence("creeping_walk");
			break;
		case ACT_RUN:
			iSequence = LookupSequence("run");
			break;
		case ACT_RANGE_ATTACK1:
			iSequence = LookupSequence("attack");
			break;
		default:
			break;
		}
		if (iSequence != -1)
			return iSequence;
	}
	return CFollowingMonster::LookupActivity(activity);
}

void CHWGrunt::SetActivity( Activity NewActivity )
{
	if (NewActivity == ACT_THREAT_DISPLAY)
	{
		EmitSoundScript(spinUpSoundScript);
	}
	CFollowingMonster::SetActivity(NewActivity);
}

bool CHWGrunt::CheckRangeAttack1( float flDot, float flDist )
{
	if( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult tr;
		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), ignore_monsters, ignore_glass, ENT( pev ), &tr );

		if( tr.flFraction == 1.0 )
		{
			return true;
		}
	}

	return false;
}

bool CHWGrunt::PerceiveEnemyAsOccluded(CBaseEntity *pEnemy, CBaseEntity *pOccluder)
{
	/*if (pOccluder->IsDestroyableObstacle())
	{
		if (pEnemy && m_lastOccluder != pOccluder)
		{
			m_lastOccluder = pOccluder;
			m_vecEnemyLKP = pEnemy->pev->origin;
		}
		return false;
	}*/
	return true;
}

void CHWGrunt::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CFollowingMonster::StartTask( pTask );
		if( pev->movetype == MOVETYPE_FLY )
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;
	case TASK_HWGRUNT_PLAY_SPINDOWN:
	{
		EmitSoundScript(spinDownSoundScript);
		int iSequence = LookupSequence("spindown");
		if( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			pev->frame = 0;
			pev->sequence = iSequence;	// Set to the reset anim (if it's there)
			ResetSequenceInfo();
		}
		else
		{
			ALERT(at_aiconsole, "%s: couldn't find the \"spindown\" animation\n", STRING(pev->classname));
			TaskComplete();
		}
	}
		break;
	case TASK_HWGRUNT_RELOAD:
	{
		if (m_cAmmoLoaded >= m_cClipSize)
			TaskComplete();
		else
		{
			float delay = (m_cClipSize - m_cAmmoLoaded) / static_cast<float>(m_cClipSize);
			if (delay >= 0.1f)
			{
				m_flWaitFinished = gpGlobals->time + delay;
			}
			else
			{
				FinishReload();
				TaskComplete();
			}
		}
	}
		break;
	default:
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

void CHWGrunt::RunTask( Task_t *pTask )
{
	switch(pTask->iTask)
	{
	case TASK_HWGRUNT_PLAY_SPINDOWN:
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
		break;
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	{
		if (m_Activity == ACT_THREAT_DISPLAY)
		{
			Vector vecShootOrigin = GetGunPosition();
			Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

			Vector angDir = UTIL_VecToAngles( vecShootDir );
			SetBlending( 0, angDir.x );
		}
		CFollowingMonster::RunTask(pTask);
	}
		break;
	case TASK_RANGE_ATTACK1:
		// The fire rate depends on Think rate which is 0.1 for monsters.
		Shoot();
		CFollowingMonster::RunTask(pTask);
		break;
	case TASK_HWGRUNT_RELOAD:
		if( gpGlobals->time >= m_flWaitFinished )
		{
			FinishReload();
			TaskComplete();
		}
		break;
	default:
		CFollowingMonster::RunTask(pTask);
		break;
	}
}

#define HWGRUNT_VOLUME 0.4

void CHWGrunt::PlayUseSentence()
{
	EmitSoundScript(useSoundScript);
}

void CHWGrunt::PlayUnUseSentence()
{
	EmitSoundScript(unuseSoundScript);
}

void CHWGrunt::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

PainSoundRule CHWGrunt::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = 1.0f;
	return rule;
}

void CHWGrunt::PainSound()
{
	EmitSoundScript(painSoundScript);
}

void CHWGrunt::Shoot()
{
	if( m_hEnemy == 0 )
	{
		return;
	}

	EmitSoundScript(shootSoundScript);

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);

	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iM249Link, TE_BOUNCE_SHELL);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, 2048, GetSkillValue("556_bullet") ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	if (m_cClipSize > 0)
		m_cAmmoLoaded--;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

void CHWGrunt::FinishReload()
{
	m_cAmmoLoaded = m_cClipSize;
	ClearConditions( bits_COND_NO_AMMO_LOADED );
}

Task_t tlHWGruntStartRangeAttack[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_HWGRUNT_RELOAD, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_THREAT_DISPLAY },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_HWGRUNT_SPINDOWN },
	{ TASK_SET_SCHEDULE, (float)SCHED_HWGRUNT_SHOOT }
};

Schedule_t slHWGruntStartRangeAttack[] =
{
	{
		tlHWGruntStartRangeAttack,
		ARRAYSIZE( tlHWGruntStartRangeAttack ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"HWGrunt Start Range Attack"
	},
};

Task_t tlHWGruntContinueRangeAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_HWGRUNT_SPINDOWN },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_HWGRUNT_SHOOT }
};

Schedule_t slHWGruntContinueRangeAttack[] =
{
	{
		tlHWGruntContinueRangeAttack,
		ARRAYSIZE( tlHWGruntContinueRangeAttack ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"HWGrunt Continue Range Attack"
	},
};

Task_t tlHWGruntSuppressingRangeAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_HWGRUNT_SPINDOWN },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slHWGruntSuppressingRangeAttack[] =
{
	{
		tlHWGruntSuppressingRangeAttack,
		ARRAYSIZE( tlHWGruntSuppressingRangeAttack ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"HWGrunt Suppressing Range Attack"
	},
};

Task_t tlHWGruntSpindown[] =
{
	{ TASK_HWGRUNT_PLAY_SPINDOWN, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_HWGRUNT_RELOAD, (float)0 },
};

Schedule_t slHWGruntSpindown[] =
{
	{
		tlHWGruntSpindown,
		ARRAYSIZE( tlHWGruntSpindown ),
		0,
		0,
		"HWGrunt Spindown"
	},
};

Task_t tlHWGruntRepel[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_GLIDE },
};

Schedule_t	slHWGruntRepel[] =
{
	{
		tlHWGruntRepel,
		ARRAYSIZE( tlHWGruntRepel ),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER_IF_NOT_ALLY,
		"Repel"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHWGrunt )
{
	slHWGruntStartRangeAttack,
	slHWGruntContinueRangeAttack,
	slHWGruntSpindown,
	slHWGruntRepel,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHWGrunt, CFollowingMonster )

//=========================================================
// ShouldAnticipateLanding - traces down to the ground to
// determine whether impact is imminent, so the landing anim
// can be triggered before FL_ONGROUND is actually set by
// the physics.
//=========================================================
bool CHWGrunt::ShouldAnticipateLanding()
{
	if( m_MonsterState == MONSTERSTATE_PRONE )
		return false;

	if( pev->velocity.z >= 0 )
		return false; // still ascending/stationary, not falling

	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, RAPPEL_LANDING_TRACE_DIST ),
	                dont_ignore_monsters, ignore_glass, ENT( pev ), &tr );

	float flDistToGround = pev->origin.z - tr.vecEndPos.z;

	return flDistToGround <= 64.0f; // bigger margin for hwgrunt
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHWGrunt::PrescheduleThink()
{
	// Force an immediate reschedule as soon as landing is imminent or has
	// occurred, instead of waiting for a break condition of the repel
	// schedule to trigger by chance (ACT_GLIDE loops and never finishes
	// TASK_PLAY_SEQUENCE on its own).
	if( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE
	    && ( FBitSet( pev->flags, FL_ONGROUND ) || ShouldAnticipateLanding() ) )
	{
		ClearSchedule();
	}

	CFollowingMonster::PrescheduleThink();
}

Schedule_t *CHWGrunt::GetSchedule()
{
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if( FBitSet( pev->flags, FL_ONGROUND ) || ShouldAnticipateLanding() )
		{
			// just landed (or about to)
			pev->movetype = MOVETYPE_STEP;
			pev->velocity.z = 0; // avoid a slight residual slide if we cut before the actual impact
			return GetScheduleOfType( SCHED_REPEL_LAND );
		}
		else
		{
			return GetScheduleOfType( SCHED_HWGRUNT_REPEL );
		}
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
	{
		Schedule_t* utilitySchedule = GetUtilitySchedule();
		if (utilitySchedule)
			return utilitySchedule;
		break;
	}
	case MONSTERSTATE_COMBAT:
		if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
			return CBaseMonster::GetSchedule();
		if (HasConditions(bits_COND_ENEMY_OCCLUDED) && m_firing)
		{
			return GetScheduleOfType(SCHED_HWGRUNT_SUPPRESSING_FIRE);
		}
		break;
	default:
		break;
	}
	if (m_firing)
		return GetScheduleOfType(SCHED_HWGRUNT_SPINDOWN);
	return CFollowingMonster::GetSchedule();
}

Schedule_t* CHWGrunt::GetScheduleOfType(int Type)
{
	switch(Type)
	{
	case SCHED_RANGE_ATTACK1:
		{
			return slHWGruntStartRangeAttack;
		}
	case SCHED_HWGRUNT_SHOOT:
		{
			return slHWGruntContinueRangeAttack;
		}
	case SCHED_HWGRUNT_SUPPRESSING_FIRE:
		{
			return slHWGruntSuppressingRangeAttack;
		}
	case SCHED_HWGRUNT_SPINDOWN:
		{
			return slHWGruntSpindown;
		}
	case SCHED_HWGRUNT_REPEL:
		{
			if( pev->velocity.z > -128 )
				pev->velocity.z -= 32;
			return &slHWGruntRepel[0];
		}
	default:
		{
			return CFollowingMonster::GetScheduleOfType(Type);
		}
	}
}

void CHWGrunt::OnChangeSchedule(Schedule_t *pNewSchedule)
{
	CFollowingMonster::OnChangeSchedule(pNewSchedule);
	m_firing = pNewSchedule == slHWGruntContinueRangeAttack;
}

void CHWGrunt::GibMonster()
{
	if (GetBodygroup(GUN_GROUP) != GUN_NONE)
	{
		DropMyItems(true);
	}
	CFollowingMonster::GibMonster();
}

void CHWGrunt::DropMyItems(bool isGibbed)
{
	if (g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment(0, vecGunPos, vecGunAngles);

		FixupDropItemPosition(vecGunPos);

		CBaseEntity* pGun = DropItem("weapon_minigun", vecGunPos, vecGunAngles);
		if (pGun)
		{
			if (isGibbed)
			{
				pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
				pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			else
			{
				SetBodygroup(GUN_GROUP, GUN_NONE);
			}
		}
	}
}

void CHWGrunt::DetectModelType()
{
	m_sc5Model = LookupSequence("pistol_shoot") != -1;
}

class CHWGruntRepel : public CHGruntRepel
{
public:
	const char* TrooperName() override {
		return "monster_hwgrunt";
	}
};

LINK_ENTITY_TO_CLASS(monster_hwgrunt_repel, CHWGruntRepel)
