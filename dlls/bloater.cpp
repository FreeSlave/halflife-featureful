/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Bloater
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"soundent.h"
#include	"scripted.h"
#include	"studio.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BLOATER_AE_ATTACK_MELEE1		0x01

class CBloater : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int DefaultClassify() override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;

	// No range attacks
	bool CheckRangeAttack1( float flDot, float flDist ) override { return false; }
	bool CheckRangeAttack2( float flDot, float flDist ) override { return false; }
	TakeDamageResult TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo ) override;
	int DefaultSizeForGrapple() override { return GRAPPLE_SMALL; }
	bool IsDisplaceable() override { return true; }
};

LINK_ENTITY_TO_CLASS( monster_bloater, CBloater )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBloater::DefaultClassify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBloater::SetYawSpeed()
{
	pev->yaw_speed = 120;
}

TakeDamageResult CBloater::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, damageInfo );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBloater::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BLOATER_AE_ATTACK_MELEE1:
			{
				// do stuff for this event.
			}
			break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CBloater::Spawn()
{
	Precache();

	SetMyModel( "models/floater.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->spawnflags |= FL_FLY;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( 40 );
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBloater::Precache()
{
	PrecacheMyModel( "models/floater.mdl" );
	PrecacheMyGibModel();
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

#define BLOATING_TIME 2.1
#define BASE_FLOATER_SPEED 100
#define FLOATER_GLOW_SPRITE "sprites/glow02.spr"

#define bits_MEMORY_FLOATER_PROVOKED bits_MEMORY_CUSTOM1

class CFloater : public CBaseMonster
{
public:
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("floater"); }
	void SetYawSpeed() override;
	int DefaultISoundMask() override;
	int DefaultClassify() override;
	const char* DefaultDisplayName() override { return "Floater"; }
	int IRelationship(CBaseEntity *pTarget) override;
	int SizeForGrapple() override { return GRAPPLE_SMALL; }
	Vector DefaultMinHullSize() override { return Vector( -16.0f, -16.0f, 0.0f ); }
	Vector DefaultMaxHullSize() override { return Vector( 16.0f, 16.0f, 36.0f ); }

	void EXPORT FloaterTouch(CBaseEntity* pOther);
	void EXPORT FloaterBloatUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

//	void SetObjectCollisionBox()
//	{
//		pev->absmin = pev->origin + Vector( -24, -24, 0 );
//		pev->absmax = pev->origin + Vector( 24, 24, 48 );
//	}

	void IdleSound() override;
	void AlertSound() override;
	PainSoundRule DefaultPainSoundRule() override;
	void PainSound() override;
	float HearingSensitivity() override;
	TakeDamageResult TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo) override;
	KilledResult Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib) override;
	void GibMonster() override;
	void UpdateOnRemove() override;

	bool CheckRangeAttack1( float flDot, float flDist ) override;
	bool CheckRangeAttack2( float flDot, float flDist ) override;
	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	Schedule_t *GetSchedule() override;
	Schedule_t *GetScheduleOfType( int Type ) override;
	void RunTask( Task_t *pTask ) override;
	void PrescheduleThink() override;
	CUSTOM_SCHEDULES

	void Move( float flInterval ) override;
	int CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist ) override;
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval ) override;
	void SetActivity( Activity NewActivity ) override;
	int LookupActivity(int activity) override;
	bool ShouldAdvanceRoute( float flWaypointDist ) override;

	void StartBloating();
	void ChangeGlowVisual(CSprite* pGlow, const Visual& newGlow);
	void ExplodeEffect();
	void MakeProvoked(bool alertOthers = true);
	void AlertOthers();

	CSprite* m_leftGlow;
	CSprite* m_rightGlow;
	float m_nextPainTime;
	float m_originalScale;
	float m_targetScale;

	float m_idleSoundTime;

	static float g_howlTime;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript howlSoundScript;
	static const NamedSoundScript bloatSoundScript;
	static const NamedSoundScript explodeSoundScript;

	static const NamedVisual blowVisual;
	static const NamedVisual blowAltVisual;
	static const NamedVisual blowSprayVisual;
	static const NamedVisual blowLightVisual;

	static const NamedVisual glowVisual;
	static const NamedVisual glowUnprovokedVisual;
	static const NamedVisual glowBloatingVisual;

protected:
	inline float OriginalScale() const { return m_originalScale; }
	inline void SetOriginalScale()
	{
		m_originalScale = pev->scale ? pev->scale : 1;
	}
	inline float TargetScale() const { return m_targetScale; }
	inline void SetTargetScale(float scale)
	{
		m_targetScale = scale;
	}

	inline float StartBloatingTime() { return pev->dmgtime; }
	inline void SetStartBloatingTime(float time)
	{
		pev->dmgtime = time;
	}
	inline bool Bloating()
	{
		return pev->dmgtime > 0;
	}
	inline bool IsProvoked()
	{
		return !FBitSet(pev->spawnflags, SF_MONSTER_WAIT_UNTIL_PROVOKED) || HasMemory(bits_MEMORY_FLOATER_PROVOKED);
	}
	void GlowUpdate();
	void GlowUpdate(CSprite* glow);

	Vector m_velocity;

	void CheckForAttachments()
	{
		if (pev->modelindex)
		{
			void *pmodel = GET_MODEL_PTR(edict());
			if (pmodel)
			{
				studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
				m_hasAttachments = pstudiohdr->numattachments >= 2;
			}
		}
	}

	bool m_hasAttachments;
};

LINK_ENTITY_TO_CLASS( monster_floater, CFloater )

TYPEDESCRIPTION	CFloater::m_SaveData[] =
{
	DEFINE_FIELD( CFloater, m_leftGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFloater, m_rightGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFloater, m_originalScale, FIELD_FLOAT ),
	DEFINE_FIELD( CFloater, m_targetScale, FIELD_FLOAT ),
	DEFINE_FIELD( CFloater, m_idleSoundTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFloater, CBaseMonster )

float CFloater::g_howlTime = 0;

const NamedSoundScript CFloater::idleSoundScript = {
	CHAN_VOICE,
	{},
	"Floater.Idle"
};

const NamedSoundScript CFloater::alertSoundScript = {
	CHAN_VOICE,
	{},
	"Floater.Alert"
};

const NamedSoundScript CFloater::painSoundScript = {
	CHAN_VOICE,
	{"floater/floater_pain1.wav", "floater/floater_pain2.wav", "floater/floater_pain3.wav"},
	IntRange(95, 105),
	"Floater.Pain"
};

const NamedSoundScript CFloater::howlSoundScript = {
	CHAN_VOICE,
	{"floater/floater_howl.wav"},
	1.0f,
	0.7f,
	IntRange(95, 105),
	"Floater.Howl"
};

const NamedSoundScript CFloater::bloatSoundScript = {
	CHAN_ITEM,
	{"floater/floater_spinup.wav"},
	"Floater.Bloat"
};

const NamedSoundScript CFloater::explodeSoundScript = {
	CHAN_BODY,
	{"weapons/splauncher_impact.wav"},
	"Floater.Explode"
};

const NamedVisual CFloater::blowVisual = BuildVisual::Animated("Floater.Blow")
		.Model("sprites/spore_exp_01.spr")
		.RenderMode(kRenderTransAdd)
		.Scale(2.0f)
		.Alpha(120);

const NamedVisual CFloater::blowAltVisual = BuildVisual::Animated("Floater.BlowAlt")
		.Model("sprites/spore_exp_c_01.spr")
		.RenderMode(kRenderTransAdd)
		.Scale(2.0f)
		.Alpha(120);

const NamedVisual CFloater::blowSprayVisual = BuildVisual::Spray("Floater.BlowSpray")
		.Model("sprites/tinyspit.spr");

const NamedVisual CFloater::blowLightVisual = BuildVisual("Floater.BlowLight")
		.Radius(120)
		.RenderColor(60, 180, 0)
		.Life(2.0f)
		.Decay(120.0f);

const NamedVisual CFloater::glowVisual = BuildVisual("Floater.Glow")
		.Model("sprites/glow02.spr")
		.RenderMode(kRenderGlow)
		.RenderFx(kRenderFxNoDissipation)
		.RenderColor(127, 0, 255)
		.Alpha(220)
		.Scale(0.2f);

const NamedVisual CFloater::glowUnprovokedVisual = BuildVisual("Floater.GlowUnprovoked")
		.RenderColor(0, 255, 255)
		.Mixin(&CFloater::glowVisual);

const NamedVisual CFloater::glowBloatingVisual = BuildVisual("Floater.GlowBloating")
		.RenderColor(255, 0, 0)
		.Mixin(&CFloater::glowVisual);

int CFloater::DefaultISoundMask()
{
	int bits = bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_DANGER;
	if (IsProvoked())
		bits |= bits_SOUND_PLAYER;
	return bits;
}

int CFloater::DefaultClassify()
{
	return	CLASS_ALIEN_MONSTER;
}

int CFloater::IRelationship( CBaseEntity *pTarget )
{
	if( !IsProvoked() && pTarget->IsPlayer() )
		return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}

void CFloater::SetYawSpeed()
{
	pev->yaw_speed = 120;
}

void CFloater::Spawn()
{
	Precache();

	SetMyModel( "models/floater.mdl" );
	SetMySize();

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags		|= FL_FLY;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( gSkillData.floaterHealth );
	pev->view_ofs		= Vector( 0, 0, -2 );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_FULL);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	SetOriginalScale();

	CheckForAttachments();

	if (m_hasAttachments)
	{
		const Visual* pGlowVisual = nullptr;
		if (FBitSet(pev->spawnflags, SF_MONSTER_WAIT_UNTIL_PROVOKED))
			pGlowVisual = GetVisual(glowUnprovokedVisual);
		else
			pGlowVisual = GetVisual(glowVisual);
		if (pGlowVisual)
		{
			m_leftGlow = CreateSpriteFromVisual(pGlowVisual, pev->origin);
			if (m_leftGlow)
				m_leftGlow->SetAttachment(edict(), 2);
			m_rightGlow = CreateSpriteFromVisual(pGlowVisual, pev->origin);
			if (m_rightGlow)
				m_rightGlow->SetAttachment(edict(), 1);
		}
	}
	else
	{
		m_leftGlow = NULL;
		m_rightGlow = NULL;
	}

	SetTouch(&CFloater::FloaterTouch);

	MonsterInit();

	SetUse( &CFloater::FloaterBloatUse );
}

void CFloater::Precache()
{
	PrecacheMyModel( "models/floater.mdl" );
	PrecacheMyGibModel();

	RegisterVisual(blowSprayVisual);
	RegisterVisual(blowVisual);
	RegisterVisual(blowAltVisual);
	RegisterVisual(blowLightVisual);

	RegisterVisual(glowVisual);
	RegisterVisual(glowUnprovokedVisual);
	RegisterVisual(glowBloatingVisual);

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(howlSoundScript);
	RegisterAndPrecacheSoundScript(bloatSoundScript);
	RegisterAndPrecacheSoundScript(explodeSoundScript);

	g_howlTime = 0;
	CheckForAttachments();
}

void CFloater::FloaterTouch(CBaseEntity *pOther)
{
	if (pOther->IsPlayer() && IRelationship(pOther) != R_AL)
	{
		CBaseEntity* groundEnt = Instance(pOther->pev->groundentity);
		if (groundEnt == this)
			TakeDamage(pev, pev, DamageInfo(pev->health, DMG_GENERIC));
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// Chase enemy schedule
Task_t tlFloaterChaseEnemy[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CHASE_ENEMY_FAILED },
	{ TASK_GET_PATH_TO_ENEMY, (float)64 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slFloaterChaseEnemy[] =
{
	{
		tlFloaterChaseEnemy,
		ARRAYSIZE( tlFloaterChaseEnemy ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_TASK_FAILED,
		0,
		"FloaterChaseEnemy"
	},
};

Task_t tlFloaterTakeCover[] =
{
	{ TASK_WAIT, (float)0.2 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_WAIT, (float)1 },
};

Schedule_t slFloaterTakeCover[] =
{
	{
		tlFloaterTakeCover,
		ARRAYSIZE( tlFloaterTakeCover ),
		bits_COND_NEW_ENEMY,
		0,
		"FloaterTakeCover"
	},
};

Task_t tlFloaterFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_WAIT, (float)1 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slFloaterFail[] =
{
	{
		tlFloaterFail,
		ARRAYSIZE( tlFloaterFail ),
		0,
		0,
		"FloaterFail"
	},
};

DEFINE_CUSTOM_SCHEDULES( CFloater )
{
	slFloaterChaseEnemy,
	slFloaterTakeCover,
	slFloaterFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CFloater, CBaseMonster )

void CFloater::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		if (m_hEnemy != 0)
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );
			if (((m_hEnemy->Center() - pev->origin)).IsLengthLessThan(128) && !Bloating())
			{
				StartBloating();
			}
		}
		CBaseMonster::RunTask( pTask );
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

void CFloater::PrescheduleThink()
{
	if (g_howlTime <= gpGlobals->time && IsMoving() && RANDOM_LONG(0,10) == 0)
	{
		if (gpGlobals->time > m_flNextPainTime && !FBitSet(pev->spawnflags, SF_MONSTER_GAG))
		{
			if (EmitSoundScript(howlSoundScript))
			{
				m_idleSoundTime = g_howlTime = gpGlobals->time + 5.0f;
			}
		}
	}
	GlowUpdate();
	if (Bloating())
	{
		float fraction = (gpGlobals->time - StartBloatingTime()) / BLOATING_TIME;
		pev->scale = OriginalScale() + (TargetScale() - OriginalScale()) * fraction;
		if (gpGlobals->time >= StartBloatingTime() + BLOATING_TIME)
		{
			TakeDamage(pev, pev, DamageInfo(pev->health, DMG_GENERIC));
		}
		m_flGroundSpeed = BASE_FLOATER_SPEED + 400 * fraction;
	}
	CBaseMonster::PrescheduleThink();
}

Schedule_t *CFloater::GetSchedule()
{
	if( HasConditions( bits_COND_HEAR_SOUND ) && !IsProvoked() )
	{
		CSound *pSound = PBestSound();
		ASSERT( pSound != NULL );

		if( pSound && ( pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT) ) )
			MakeProvoked();
	}
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
			{
				return CBaseMonster::GetSchedule();
			}
			if (HasConditions(bits_COND_NEW_ENEMY))
			{
				AlertSound();
			}
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
		break;
	default:
		break;
	}

	return CBaseMonster::GetSchedule();
}

Schedule_t *CFloater::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slFloaterChaseEnemy;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slFloaterTakeCover;
	case SCHED_FAIL:
		return slFloaterFail;
	case SCHED_CHASE_ENEMY_FAILED:
		if (HasMemory(bits_MEMORY_BLOCKER_IS_ENEMY))
			return CBaseMonster::GetScheduleOfType(SCHED_CHASE_ENEMY);
		else
			return slFloaterFail;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

bool CFloater::CheckRangeAttack1( float flDot, float flDist )
{
	return false;
}

bool CFloater::CheckRangeAttack2( float flDot, float flDist )
{
	return false;
}

bool CFloater::CheckMeleeAttack1( float flDot, float flDist )
{
	return false;
}

void CFloater::SetActivity( Activity NewActivity )
{
	CBaseMonster::SetActivity(NewActivity);
	if (m_flGroundSpeed == 0)
	{
		m_flGroundSpeed = BASE_FLOATER_SPEED;
	}
}

int CFloater::LookupActivity(int activity)
{
	return LookupSequence("idle1");
}

#define DIST_TO_CHECK	200

void CFloater::Move( float flInterval )
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	float		flMoveDist;
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if( FRouteClear() )
	{
		TaskFail("route is empty");
		return;
	}

	if( m_flMoveWaitFinished > gpGlobals->time )
		return;

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	if( m_flGroundSpeed == 0 )
	{
		m_flGroundSpeed = BASE_FLOATER_SPEED;
	}

	flMoveDist = m_flGroundSpeed * flInterval;

	do
	{
		// local move to waypoint.
		vecDir = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Normalize();
		flWaypointDist = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Length();

		MakeIdealYaw( m_Route[m_iRouteIndex].vecLocation );
		ChangeYaw( pev->yaw_speed );

		// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
		if( flWaypointDist < DIST_TO_CHECK )
		{
			flCheckDist = flWaypointDist;
		}
		else
		{
			flCheckDist = DIST_TO_CHECK;
		}

		if( ( m_Route[m_iRouteIndex].iType & ( ~bits_MF_NOT_TO_MASK ) ) == bits_MF_TO_ENEMY )
		{
			// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
			pTargetEnt = m_hEnemy;
		}
		else if( ( m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK ) == bits_MF_TO_TARGETENT )
		{
			pTargetEnt = m_hTargetEnt;
		}

		// !!!BUGBUG - CheckDist should be derived from ground speed.
		// If this fails, it should be because of some dynamic entity blocking this guy.
		// We've already checked this path, so we should wait and time out if the entity doesn't move
		flDist = 0;
		if( CheckLocalMove( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
		{
			CBaseEntity *pBlocker;

			// Can't move, stop
			Stop();
			// Blocking entity is in global trace_ent
			pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
			if( pBlocker )
			{
				DispatchBlocked( edict(), pBlocker->edict() );
			}
			if( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
			{
				// Can we still move toward our target?
				if( flDist < m_flGroundSpeed )
				{
					// Wait for a second
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
					//ALERT( at_aiconsole, "Move %s!!!\n", STRING( pBlocker->pev->classname ) );
					return;
				}
			}
			else
			{
				// try to triangulate around whatever is in the way.
				if( FTriangulate( pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist, pTargetEnt, &vecApex ) )
				{
					InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
					RouteSimplify( pTargetEnt );
				}
				else
				{
	 				ALERT ( at_aiconsole, "Couldn't Triangulate. Blocker: %s\n", pBlocker ? STRING(pBlocker->pev->classname) : "Unknown" );
					Stop();
					if( m_moveWaitTime > 0 )
					{
						FRefreshRoute();
						m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime * 0.5;
					}
					else
					{
						if (m_movementGoal == MOVEGOAL_ENEMY && pBlocker && pBlocker == m_hEnemy)
						{
							Remember(bits_MEMORY_BLOCKER_IS_ENEMY);
						}

						TaskFail("failed to move");
						//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, ( pev->origin + ( vecDir * flCheckDist ) ).z, m_Route[m_iRouteIndex].vecLocation.z );
					}
					return;
				}
			}
		}

		// UNDONE: this is a hack to quit moving farther than it has looked ahead.
		if( flCheckDist < flMoveDist )
		{
			MoveExecute( pTargetEnt, vecDir, flCheckDist / m_flGroundSpeed );

			// ALERT( at_console, "%.02f\n", flInterval );
			AdvanceRoute( flWaypointDist );
			flMoveDist -= flCheckDist;
		}
		else
		{
			MoveExecute( pTargetEnt, vecDir, flMoveDist / m_flGroundSpeed );

			if( ShouldAdvanceRoute( flWaypointDist - flMoveDist ) )
			{
				AdvanceRoute( flWaypointDist );
			}
			flMoveDist = 0;
		}

		if( MovementIsComplete() )
		{
			Stop();
			RouteClear();
		}
	} while( flMoveDist > 0 && flCheckDist > 0 );
}

bool CFloater::ShouldAdvanceRoute( float flWaypointDist )
{
	return flWaypointDist <= 32;
}

int CFloater::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 18 ), vecEnd + Vector( 0, 0, 18 ), dont_ignore_monsters, head_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if( pflDist )
	{
		*pflDist = ( ( tr.vecEndPos - Vector( 0, 0, 18 ) ) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if( tr.fStartSolid || tr.flFraction < 1.0 )
	{
		if( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}

void CFloater::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	// ALERT( at_console, "move %.4f %.4f %.4f : %f\n", vecDir.x, vecDir.y, vecDir.z, flInterval );

	// float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	// UTIL_MoveToOrigin ( ENT( pev ), m_Route[m_iRouteIndex].vecLocation, flTotal, MOVE_STRAFE );

	m_velocity = m_velocity * 0.8 + m_flGroundSpeed * vecDir * 0.2;

	UTIL_MoveToOrigin( ENT( pev ), pev->origin + m_velocity, m_velocity.Length() * flInterval, MOVE_STRAFE );
}

void CFloater::IdleSound()
{
	if (gpGlobals->time > m_idleSoundTime)
		EmitSoundScript(idleSoundScript);
}

void CFloater::AlertSound()
{
	EmitSoundScript(alertSoundScript);
}

PainSoundRule CFloater::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = 1.5f;
	return rule;
}

void CFloater::PainSound()
{
	EmitSoundScript(painSoundScript);
}

float CFloater::HearingSensitivity()
{
	if (IsProvoked())
		return CBaseMonster::HearingSensitivity();
	else
		return 0.6f;
}

TakeDamageResult CFloater::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo)
{
	MakeProvoked();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, damageInfo);
}

KilledResult CFloater::Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib)
{
	KilledResult killedResult = CBaseMonster::Killed(pevInflictor, pevAttacker, GIB_ALWAYS);
	g_howlTime = gpGlobals->time;
	StopSoundScript(howlSoundScript);
	ExplodeEffect();
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 300, 0.3 );
	return killedResult;
}

void CFloater::GibMonster()
{
	const SoundScript* bloatSound = GetSoundScript(bloatSoundScript);
	if (bloatSound && !bloatSound->waves.empty())
	{
		EMIT_SOUND(edict(), bloatSound->channel, "common/null.wav", 1.0f, ATTN_NORM);
	}

	EmitSoundScript(NPC::bodySplatSoundScript);

	CGib::SpawnRandomClientGibs( pev, GibCount(), GibModel() );

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CFloater::UpdateOnRemove()
{
	UTIL_Remove(m_leftGlow);
	m_leftGlow = NULL;
	UTIL_Remove(m_rightGlow);
	m_rightGlow = NULL;
	CBaseMonster::UpdateOnRemove();
}

void CFloater::GlowUpdate()
{
	GlowUpdate(m_leftGlow);
	GlowUpdate(m_rightGlow);
}

void CFloater::GlowUpdate(CSprite* glow)
{
	if (glow)
	{
		if (Bloating())
		{
			const Visual* pGlowVisual = GetVisual(glowVisual);
			const Visual* pGlowBloatingVisual = GetVisual(glowBloatingVisual);

			if (pGlowVisual && pGlowBloatingVisual)
			{
				const float redSpeed = abs(pGlowVisual->rendercolor.r - pGlowBloatingVisual->rendercolor.r) / 8.0f;
				const float greenSpeed = abs(pGlowVisual->rendercolor.g - pGlowBloatingVisual->rendercolor.g) / 8.0f;
				const float blueSpeed = abs(pGlowVisual->rendercolor.b - pGlowBloatingVisual->rendercolor.b) / 8.0f;
				const float alphaSpeed = abs(pGlowVisual->renderamt - pGlowBloatingVisual->renderamt) / 8.0f;

				glow->pev->rendercolor.x = UTIL_Approach(pGlowBloatingVisual->rendercolor.r, glow->pev->rendercolor.x, redSpeed);
				glow->pev->rendercolor.y = UTIL_Approach(pGlowBloatingVisual->rendercolor.g, glow->pev->rendercolor.y, greenSpeed);
				glow->pev->rendercolor.z = UTIL_Approach(pGlowBloatingVisual->rendercolor.b, glow->pev->rendercolor.z, blueSpeed);
				glow->pev->renderamt = UTIL_Approach(pGlowBloatingVisual->renderamt, glow->pev->renderamt, alphaSpeed);
			}
		}
		UTIL_SetOrigin( glow->pev, pev->origin );
	}
}

void CFloater::StartBloating()
{
	EmitSoundScript(bloatSoundScript);
	SetTargetScale( OriginalScale() * 1.5f );
	SetStartBloatingTime( gpGlobals->time );

	if (m_leftGlow || m_rightGlow)
	{
		const Visual* pGlowBloatingVisual = GetVisual(glowBloatingVisual);
		if (pGlowBloatingVisual)
		{
			if (m_leftGlow)
				ChangeGlowVisual(m_leftGlow, *pGlowBloatingVisual);
			if (m_rightGlow)
				ChangeGlowVisual(m_rightGlow, *pGlowBloatingVisual);
		}
	}
}

void CFloater::ChangeGlowVisual(CSprite* pGlow, const Visual& newGlow)
{
	const char* model = newGlow.model;
	if (model && !FStrEq(STRING(pGlow->pev->model), model))
	{
		SET_MODEL(pGlow->edict(), model);
	}
	pGlow->pev->rendermode = newGlow.rendermode;
	pGlow->pev->renderfx = newGlow.renderfx;
	pGlow->SetScale(RandomizeNumberFromRange(newGlow.scale));
}

void CFloater::ExplodeEffect()
{
	Vector up(0,0,1);
	Vector exploOrigin = pev->origin;
	exploOrigin.z += 32.0f;

	SendSpray(exploOrigin, up, GetVisual(blowSprayVisual), 25, 15, 500);

	SendSprite(exploOrigin, GetVisual(RANDOM_LONG(0, 1) ? blowVisual : blowAltVisual));

	SendDynLight(exploOrigin, GetVisual(blowLightVisual));

	EmitSoundScript(explodeSoundScript);

	RadiusDamage(exploOrigin, pev, pev, DamageInfo{gSkillData.floaterExplode, DMG_BLAST|DMG_ACID}, Classify());
}

void CFloater::FloaterBloatUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	StartBloating();
	SetUse(NULL);
}

void CFloater::MakeProvoked(bool alertOthers)
{
	if (!IsProvoked())
	{
		if (m_leftGlow || m_rightGlow)
		{
			const Visual* pGlowVisual = GetVisual(glowVisual);
			if (pGlowVisual)
			{
				if (m_leftGlow)
				{
					ChangeGlowVisual(m_leftGlow, *pGlowVisual);
					m_leftGlow->SetColor(pGlowVisual->rendercolor.r, pGlowVisual->rendercolor.g, pGlowVisual->rendercolor.b);
				}
				if (m_rightGlow)
				{
					ChangeGlowVisual(m_rightGlow, *pGlowVisual);
					m_rightGlow->SetColor(pGlowVisual->rendercolor.r, pGlowVisual->rendercolor.g, pGlowVisual->rendercolor.b);
				}
			}
		}

		Remember(bits_MEMORY_FLOATER_PROVOKED);
		if (alertOthers)
			AlertOthers();
		if (m_pCine && m_pCine->CanInterrupt())
		{
			CineCleanup();
			ClearSchedule();
		}
	}
}

void CFloater::AlertOthers()
{
	if( FStringNull( pev->netname ) )
		return;

	CBaseEntity *pEntity = NULL;

	while( ( pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ) ) ) != NULL)
	{
		if ( pEntity == this )
			continue;
		if (!FClassnameIs(pEntity->pev, STRING(pev->classname)))
			continue;
		CBaseMonster *pMonster = pEntity->MyMonsterPointer();
		if( pMonster )
		{
			CFloater* pFloater = (CFloater*)pMonster;
			pFloater->MakeProvoked(false); // no recusrive alert
		}
	}
}
