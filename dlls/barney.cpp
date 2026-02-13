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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"scripted.h"
#include	"animation.h"
#include	"combat.h"
#include	"soundent.h"
#include	"game.h"
#include	"gamerules.h"
#include	"common_soundscripts.h"
#include	"studio.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE		2

#define BARNEY_GUN_GROUP 1
#define BARNEY_HEAD_GROUP 2

void SetBarneyHead(CBaseEntity* pEntity, int head)
{
	void *pmodel = GET_MODEL_PTR(ENT(pEntity->pev));
	if (pmodel)
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		if (pstudiohdr->numbodyparts > BARNEY_HEAD_GROUP)
		{
			if (head < 0)
			{
				int headCount = GetBodygroupNumModels(pmodel, BARNEY_HEAD_GROUP);
				if (headCount > 1)
					head = RANDOM_LONG(0, headCount - 1);
				else
					head = 0;
			}
			SetBodygroup(pmodel, pEntity->pev, BARNEY_HEAD_GROUP, head);
		}
	}
}

class CBarney : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue(KeyValueData* pkvd) override;
	void SetYawSpeed() override;
	int DefaultISoundMask() override;
	void BarneyFirePistol( const char* shotSoundScript, float flDamage );
	void AlertSound() override;
	const char* DefaultDisplayName() override { return "Security Guard"; }
	int DefaultClassify() override;
	const char* ReverseRelationshipModel() override { return "models/barnabus.mdl"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;

	void RunTask( Task_t *pTask ) override;
	void StartTask( Task_t *pTask ) override;
	int DefaultToleranceLevel() override { return TOLERANCE_LOW; }
	bool CheckRangeAttack1( float flDot, float flDist ) override;

	// Override these to set behavior
	Schedule_t *GetScheduleOfType( int Type ) override;
	Schedule_t *GetSchedule() override;

	void DeathSound() override;
	void PainSound() override;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript firePistolSoundScript;
	static constexpr const char* firePythonSoundScript = "Barney.FirePython";

	const char* DefaultSentenceGroup(int group) override;

	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	void OnDying(bool gibbed) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool m_fGunDrawn;
	float m_checkAttackTime;
	bool m_lastAttackCheck;

	void SetHead(int head) override {
		m_iHead = head;
	}

	virtual void SetGunState(int gunState);
	bool HasGun();

	int bodystate;
	CUSTOM_SCHEDULES

protected:
	void SpawnImpl(const char* modelName, float health);
	DamageInfo DefaultHandleTraceAttackImpl(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr, bool hasHelmet);
	virtual bool PrioritizeMeleeAttack() { return false; }

	int m_iHead;
};

LINK_ENTITY_TO_CLASS( monster_barney, CBarney )

TYPEDESCRIPTION	CBarney::m_SaveData[] =
{
	DEFINE_FIELD( CBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CBarney, CTalkMonster )

const NamedSoundScript CBarney::painSoundScript = {
	CHAN_VOICE,
	{"barney/ba_pain1.wav", "barney/ba_pain2.wav", "barney/ba_pain3.wav"},
	"Barney.Pain"
};

const NamedSoundScript CBarney::dieSoundScript = {
	CHAN_VOICE,
	{"barney/ba_die1.wav", "barney/ba_die2.wav", "barney/ba_die3.wav"},
	"Barney.Die"
};

const NamedSoundScript CBarney::firePistolSoundScript = {
	CHAN_WEAPON,
	{"barney/ba_attack2.wav"},
	"Barney.FirePistol"
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// BarneyDraw - much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float) ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] =
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t tlBaRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slBaRangeAttack1[] =
{
	{
		tlBaRangeAttack1,
		ARRAYSIZE( tlBaRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBarney )
{
	slBarneyEnemyDraw,
	slBaRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBarney, CTalkMonster )

void CBarney::StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );
}

void CBarney::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if( m_hEnemy != 0 && ( m_hEnemy->IsPlayer() ) )
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBarney::DefaultISoundMask()
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBarney::DefaultClassify()
{
	return CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney::AlertSound()
{
	if( m_hEnemy != 0 )
	{
		if( FOkToSpeak(SPEAK_DISREGARD_ENEMY) && !m_hEnemy->IsPlayer() )
		{
			PlaySentence( SentenceGroup(TLK_ATTACK), RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE );
		}
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney::SetYawSpeed()
{
	int ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
bool CBarney::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= 1024.0f && flDot >= 0.5f )
	{
		if( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector( 0.0f, 0.0f, 55.0f );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( ( pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin ) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT( pev ), &tr );
			m_checkAttackTime = gpGlobals->time + 1.0f;
			if( tr.flFraction == 1.0f || ( tr.pHit != NULL && CBaseEntity::Instance( tr.pHit ) == pEnemy ) )
				m_lastAttackCheck = true;
			else
				m_lastAttackCheck = false;
		}
		return m_lastAttackCheck;
	}
	return false;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney::BarneyFirePistol( const char* shotSoundScript, float flDamage )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors( pev->angles );
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, flDamage );

	// Only shift about half the time
	SoundScriptParamOverride soundParams;
	if (RANDOM_LONG(0,1) == 1)
	{
		soundParams.OverridePitchShifted(RANDOM_LONG(0,15));
	}
	EmitSoundScript(shotSoundScript, soundParams);

	InsertAISound( bits_SOUND_COMBAT, 384, 0.3f );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarney::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		ReportFireAnimEvent(pEvent->event);
		if (pev->frags)
		{
			BarneyFirePistol(firePythonSoundScript, GetSkillValue("357_bullet"));
		}
		else
		{
			BarneyFirePistol(firePistolSoundScript, GetSkillValue("9mm_bullet"));
		}
		break;
	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		SetGunState(BARNEY_BODY_GUNDRAWN);
		break;
	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		SetGunState(BARNEY_BODY_GUNHOLSTERED);
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney::SpawnImpl(const char* modelName, float health)
{
	SetMyModel( modelName );
	SetMySize();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( health );
	pev->view_ofs = Vector ( 0.0f, 0.0f, 50.0f );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE); // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;
	SetMySquadCapabilities();
	SetMyCanOpenDoors(true);

	TalkMonsterInit();
}

void CBarney::Spawn()
{
	Precache();
	SpawnImpl("models/barney.mdl", GetSkillValue("barney_health"));
	if (bodystate == -1) {
		bodystate = RANDOM_LONG(BARNEY_BODY_GUNHOLSTERED, BARNEY_BODY_GUNDRAWN);
	}
	SetGunState(bodystate);
	SetBarneyHead(this, m_iHead);
}

void CBarney::SetGunState(int gunState)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	if (pmodel)
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		if (pstudiohdr->numbodyparts > BARNEY_GUN_GROUP)
		{
			::SetBodygroup(pmodel, pev, BARNEY_GUN_GROUP, gunState);
		}
		else
		{
			pev->body = gunState;
		}
	}
	m_fGunDrawn = gunState == BARNEY_BODY_GUNDRAWN;
}

bool CBarney::HasGun()
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	if (pmodel)
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		if (pstudiohdr->numbodyparts > BARNEY_GUN_GROUP)
		{
			return ::GetBodygroup(pmodel, pev, BARNEY_GUN_GROUP) < BARNEY_BODY_GUNGONE;
		}
		else
		{
			return pev->body < BARNEY_BODY_GUNGONE;
		}
	}
	return false;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================

void CBarney::Precache()
{
	PrecacheMyModel("models/barney.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(firePistolSoundScript);
	RegisterAndPrecacheSoundScript(firePythonSoundScript, NPC::pythonSoundScript);

	PRECACHE_SOUND( "barney/ba_attack1.wav" );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}	

const char* CBarney::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "BA_ANSWER";
	case TLK_QUESTION: return "BA_QUESTION";
	case TLK_IDLE: return "BA_IDLE";
	case TLK_STARE: return "BA_STARE";
	case TLK_USE: return "BA_OK";
	case TLK_UNUSE: return "BA_WAIT";
	case TLK_DECLINE: return "BA_POK";
	case TLK_STOP: return "BA_STOP";
	case TLK_NOSHOOT: return "BA_SCARED";
	case TLK_HELLO: return "BA_HELLO";
	case TLK_PLHURT1: return "!BA_CUREA";
	case TLK_PLHURT2: return "!BA_CUREB";
	case TLK_PLHURT3: return "!BA_CUREC";
	case TLK_PHELLO: return "BA_PHELLO";
	case TLK_PIDLE: return "BA_PIDLE";
	case TLK_PQUESTION: return "BA_PQUEST";
	case TLK_SMELL: return "BA_SMELL";
	case TLK_WOUND: return "BA_WOUND";
	case TLK_MORTAL: return "BA_MORTAL";
	case TLK_SHOT: return "BA_SHOT";
	case TLK_MAD: return "BA_MAD";
	case TLK_KILL: return "BA_KILL";
	case TLK_ATTACK: return "BA_ATTACK";
	default: return NULL;
	}
}

void CBarney::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "bodystate"))
	{
		bodystate = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CTalkMonster::KeyValue( pkvd );
}

//=========================================================
// PainSound
//=========================================================
void CBarney::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

//=========================================================
// DeathSound 
//=========================================================
void CBarney::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

DamageInfo CBarney::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	return DefaultHandleTraceAttackImpl(pevInflictor, pevAttacker, inputDamageInfo, vecDir, ptr, true);
}

DamageInfo CBarney::DefaultHandleTraceAttackImpl(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr, bool hasHelmet)
{
	DamageInfo damageInfo = inputDamageInfo;

	switch( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (damageInfo.type & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
		{
			damageInfo.damage = damageInfo.damage * 0.5f;
		}
		break;
	case 10:
		if( (damageInfo.type & ( DMG_BULLET | DMG_SLASH | DMG_CLUB )) && hasHelmet )
		{
			damageInfo.damage -= 20.0f;
			if( damageInfo.damage <= 0.0f )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0f );
				damageInfo.damage = 0.01f;
			}
		}

		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	return damageInfo;
}

void CBarney::OnDying(bool gibbed)
{
	if( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && HasGun() )
	{
		// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetGunState(BARNEY_BODY_GUNGONE);
		GetAttachment( 0, vecGunPos, vecGunAngles );

		if (pev->frags)
			DropItem( "weapon_357", vecGunPos, vecGunAngles );
		else
			DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying(gibbed);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Schedule_t *CBarney::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if( m_hEnemy != 0 )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slBaRangeAttack1;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBarney::GetSchedule()
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak(SPEAK_DISREGARD_ENEMY) )
	{
		PlaySentence( SentenceGroup(TLK_KILL), 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			// always act surprized with a new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if( !m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) && PrioritizeMeleeAttack() )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
		break;
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
	{
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && CanIdleFlinch() )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;

		// try to say something about smells
		TrySmellTalk();
	}
		break;
	default:
		break;
	}

	return CTalkMonster::GetSchedule();
}

//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadBarney : public CDeadMonster
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/barney.mdl"; }
	void KeyValue( KeyValueData *pkvd ) override;
	int	DefaultClassify() override { return	CLASS_PLAYER_ALLY; }

	const char* getPos(int pos) const override;
	static const char *m_szPoses[3];

	int head;
};

const char *CDeadBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

const char* CDeadBarney::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_barney_dead, CDeadBarney )

void CDeadBarney::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CDeadMonster::KeyValue( pkvd );
}

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney::Spawn()
{
	SpawnHelper();
	SetBarneyHead(this, head);
	MonsterInitDead();
}

#define OTIS_GUN_GROUP 1
#define OTIS_HEAD_GROUP 2

#define	OTIS_BODY_GUNHOLSTERED	0
#define	OTIS_BODY_GUNDRAWN		1
#define OTIS_BODY_DONUT			2
#define OTIS_BODY_GUNGONE		3 // custom model

#define bits_OTIS_DROPPED_GUN (bits_MEMORY_CUSTOM5)

class COtis : public CBarney
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("otis"); }
	const char* DefaultSentenceGroup(int group) override;
	const char* ReverseRelationshipModel() override { return "models/otisf.mdl"; }

	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	void OnDying(bool gibbed) override;

	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;

	void SetGunState(int gunState) override;

	static constexpr const char* fireDesertEagleSoundScript = "Otis.FireDesertEagle";
	static constexpr const char* painSoundScript = "Otis.Pain";
	static constexpr const char* dieSoundScript = "Otis.Die";

	void DeathSound() override {
		EmitSoundScriptTalk(dieSoundScript);
	}
	void PainSound() override {
		EmitSoundScriptTalk(painSoundScript);
	}

protected:
	void CacheGunGroupModels();
	int m_gunGroupModels;
};

LINK_ENTITY_TO_CLASS( monster_otis, COtis )

void COtis::Spawn()
{
	Precache();
	SpawnImpl("models/otis.mdl", GetSkillValue("otis_health"));
	CacheGunGroupModels();
	if ( m_iHead == -1 )
		SetBodygroup(OTIS_HEAD_GROUP, RANDOM_LONG(0, 1));
	else
		SetBodygroup(OTIS_HEAD_GROUP, m_iHead);
	if (bodystate == -1) {
		bodystate = RANDOM_LONG(OTIS_BODY_GUNHOLSTERED, OTIS_BODY_GUNDRAWN);
	}
	SetGunState(bodystate);
}

void COtis::SetGunState(int gunState)
{
	SetBodygroup(OTIS_GUN_GROUP, gunState);
	m_fGunDrawn = gunState == OTIS_BODY_GUNDRAWN;
}

void COtis::Precache()
{
	PrecacheMyModel("models/otis.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(fireDesertEagleSoundScript, NPC::desertEagleSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript, CBarney::painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, CBarney::dieSoundScript);

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();

	if (pev->modelindex)
		CacheGunGroupModels();
}

const char* COtis::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "OT_ANSWER";
	case TLK_QUESTION: return "OT_QUESTION";
	case TLK_IDLE: return "OT_IDLE";
	case TLK_STARE: return "OT_STARE";
	case TLK_USE: return "OT_OK";
	case TLK_UNUSE: return "OT_WAIT";
	case TLK_DECLINE: return "OT_POK";
	case TLK_STOP: return "OT_STOP";
	case TLK_NOSHOOT: return "OT_SCARED";
	case TLK_HELLO: return "OT_HELLO";
	case TLK_PLHURT1: return "!OT_CUREA";
	case TLK_PLHURT2: return "!OT_CUREB";
	case TLK_PLHURT3: return "!OT_CUREC";
	case TLK_PHELLO: return "OT_PHELLO";
	case TLK_PIDLE: return "OT_PIDLE";
	case TLK_PQUESTION: return "OT_PQUEST";
	case TLK_SMELL: return "OT_SMELL";
	case TLK_WOUND: return "OT_WOUND";
	case TLK_MORTAL: return "OT_MORTAL";
	case TLK_SHOT: return "OT_SHOT";
	case TLK_MAD: return "OT_MAD";
	case TLK_KILL: return "OT_KILL";
	case TLK_ATTACK: return "OT_ATTACK";
	default: return NULL;
	}
}

DamageInfo COtis::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	return DefaultHandleTraceAttackImpl(pevInflictor, pevAttacker, inputDamageInfo, vecDir, ptr, false);
}

void COtis::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BARNEY_AE_SHOOT:
			ReportFireAnimEvent(pEvent->event);
			BarneyFirePistol(fireDesertEagleSoundScript, GetSkillValue("357_bullet"));
			break;
			
		case BARNEY_AE_DRAW:
			// barney's bodygroup switches here so he can pull gun from holster
			SetGunState(OTIS_BODY_GUNDRAWN);
			break;
			
		case BARNEY_AE_HOLSTER:
			// change bodygroup to replace gun in holster
			SetGunState(OTIS_BODY_GUNHOLSTERED);
			break;
			
		default:
			CTalkMonster::HandleAnimEvent( pEvent );
	}
}

void COtis::OnDying(bool gibbed)
{
	if ( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && !HasMemory(bits_OTIS_DROPPED_GUN) )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		if (m_gunGroupModels >= 4)
			SetBodygroup(OTIS_GUN_GROUP, OTIS_BODY_GUNGONE);
		else
			SetBodygroup(OTIS_GUN_GROUP, OTIS_BODY_GUNHOLSTERED);
		Remember(bits_OTIS_DROPPED_GUN);

		GetAttachment( 0, vecGunPos, vecGunAngles );

		DropItem( g_modFeatures.DesertEagleDropName(), vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying(gibbed);
}

void COtis::CacheGunGroupModels()
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	if (pmodel)
	{
		m_gunGroupModels = GetBodygroupNumModels(pmodel, OTIS_GUN_GROUP);
	}
}

class CDeadOtis : public CDeadBarney
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/otis.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("otis"); }
	const char* getPos(int pos) const override;
	static const char *m_szPoses[5];
};

const char *CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "dead_sitting" };

const char* CDeadOtis::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_otis_dead, CDeadOtis )

void CDeadOtis::Spawn()
{
	SpawnHelper();
	if (head == -1)
		SetBodygroup(OTIS_HEAD_GROUP, RANDOM_LONG(0, 1));
	else
		SetBodygroup(OTIS_HEAD_GROUP, head);
	MonsterInitDead();
}

class CBarniel : public CBarney
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("barniel"); }
	const char* DefaultSentenceGroup(int group) override;
	const char* ReverseRelationshipModel() override { return NULL; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void DeathSound() override;
	void PainSound() override;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript firePistolSoundScript;

	void OnDying(bool gibbed) override;
};

LINK_ENTITY_TO_CLASS( monster_barniel, CBarniel )

const NamedSoundScript CBarniel::painSoundScript = {
	CHAN_VOICE,
	{ "barniel/bn_pain1.wav" },
	"Barniel.Pain"
};

const NamedSoundScript CBarniel::dieSoundScript = {
	CHAN_VOICE,
	{ "barniel/bn_die1.wav" },
	"Barniel.Die"
};

const NamedSoundScript CBarniel::firePistolSoundScript = {
	CHAN_WEAPON,
	{"barniel/bn_attack2.wav"},
	"Barniel.FirePistol"
};

void CBarniel::Spawn()
{
	Precache();
	SpawnImpl("models/barniel.mdl", GetSkillValue("barney_health"));

	if (bodystate == -1) {
		bodystate = RANDOM_LONG(BARNEY_BODY_GUNHOLSTERED, BARNEY_BODY_GUNDRAWN);
	}
	SetGunState(bodystate);
	SetBarneyHead(this, m_iHead);
}

void CBarniel::Precache()
{
	PrecacheMyModel("models/barniel.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(firePistolSoundScript);

	PRECACHE_SOUND( "barniel/bn_attack1.wav" );

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

const char* CBarniel::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "BN_ANSWER";
	case TLK_QUESTION: return "BN_QUESTION";
	case TLK_IDLE: return "BN_IDLE";
	case TLK_STARE: return "BN_STARE";
	case TLK_USE: return "BN_OK";
	case TLK_UNUSE: return "BN_WAIT";
	case TLK_DECLINE: return "BN_POK";
	case TLK_STOP: return "BN_STOP";
	case TLK_NOSHOOT: return "BN_SCARED";
	case TLK_HELLO: return "BN_HELLO";
	case TLK_PLHURT1: return "!BN_CUREA";
	case TLK_PLHURT2: return "!BN_CUREB";
	case TLK_PLHURT3: return "!BN_CUREC";
	case TLK_PHELLO: return "BN_PHELLO";
	case TLK_PIDLE: return "BN_PIDLE";
	case TLK_PQUESTION: return "BN_PQUEST";
	case TLK_SMELL: return "BN_SMELL";
	case TLK_WOUND: return "BN_WOUND";
	case TLK_MORTAL: return "BN_MORTAL";
	case TLK_SHOT: return "BN_SHOT";
	case TLK_MAD: return "BN_MAD";
	case TLK_KILL: return "BN_KILL";
	case TLK_ATTACK: return "BN_ATTACK";
	default: return NULL;
	}
}

void CBarniel::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		ReportFireAnimEvent(pEvent->event);
		BarneyFirePistol(firePistolSoundScript, GetSkillValue("9mm_bullet"));
		break;
	default:
		CBarney::HandleAnimEvent( pEvent );
	}
}

void CBarniel::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

void CBarniel::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

void CBarniel::OnDying(bool gibbed)
{
	if( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && HasGun() )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		SetGunState(BARNEY_BODY_GUNGONE);
		GetAttachment( 0, vecGunPos, vecGunAngles );

		DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying(gibbed);
}

class CDeadBarniel : public CDeadBarney
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/barniel.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("barniel"); }
	const char* getPos(int pos) const override;
	static const char *m_szPoses[3];
};

const char *CDeadBarniel::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

const char* CDeadBarniel::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_barniel_dead, CDeadBarniel )

void CDeadBarniel::Spawn()
{
	SpawnHelper();
	SetBarneyHead(this, head);
	MonsterInitDead();
}

#define		KATE_AE_KICK		( 6 )
#define KATE_LIMP_HEALTH 40

class CKate : public CBarney
{
public:
	void Spawn() override;
	void Precache() override;
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("kate"); }
	const char* DefaultSentenceGroup(int group) override;
	const char* DefaultDisplayName() override { return "Kate"; }
	const char* ReverseRelationshipModel() override { return nullptr; }
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int LookupActivity(int activity) override;
	int DefaultToleranceLevel() override { return TOLERANCE_AVERAGE; }
	bool CheckMeleeAttack1( float flDot, float flDist ) override;
	void DeathSound() override;
	void PainSound() override;

	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	void OnDying(bool gibbed) override;

	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript firePistolSoundScript;
	static const NamedSoundScript kickSoundScript;
	static const NamedSoundScript punchSoundScript;

protected:
	bool PrioritizeMeleeAttack() override { return true; }
	float LimpHealth();

	int m_iCombatState;
};

LINK_ENTITY_TO_CLASS( monster_kate, CKate )

const NamedSoundScript CKate::painSoundScript = {
	CHAN_VOICE,
	{ "kate/ka_pain1.wav", "kate/ka_pain2.wav" },
	"Kate.Pain"
};

const NamedSoundScript CKate::dieSoundScript = {
	CHAN_VOICE,
	{ "kate/ka_die1.wav" },
	"Kate.Die"
};

const NamedSoundScript CKate::firePistolSoundScript = {
	CHAN_WEAPON,
	{"kate/ka_attack2.wav"},
	"Kate.FirePistol"
};

const NamedSoundScript CKate::kickSoundScript = {
	CHAN_ITEM,
	{"common/kick.wav"},
	"Kate.Kick"
};

const NamedSoundScript CKate::punchSoundScript = {
	CHAN_ITEM,
	{"common/punch.wav"},
	"Kate.Punch"
};

void CKate::Spawn()
{
	Precache();
	SpawnImpl("models/kate.mdl", GetSkillValue("kate_health"));
	m_iCombatState = -1;

	if (bodystate == -1) {
		bodystate = RANDOM_LONG(BARNEY_BODY_GUNHOLSTERED, BARNEY_BODY_GUNDRAWN);
	}
	SetGunState(bodystate);
	SetBarneyHead(this, m_iHead);
}

void CKate::Precache()
{
	PrecacheMyModel("models/kate.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(firePistolSoundScript);
	RegisterAndPrecacheSoundScript(NPC::swishSoundScript);

	PRECACHE_SOUND( "kate/ka_attack1.wav" );

	if (!ShouldAutoPrecacheSounds())
	{
		PRECACHE_SOUND( "zombie/claw_miss1.wav" );
		PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	}
	RegisterAndPrecacheSoundScript(kickSoundScript);
	RegisterAndPrecacheSoundScript(punchSoundScript);

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

const char* CKate::DefaultSentenceGroup(int group)
{
	switch (group) {
	case TLK_ANSWER: return "KA_ANSWER";
	case TLK_QUESTION: return "KA_QUESTION";
	case TLK_IDLE: return "KA_IDLE";
	case TLK_STARE: return "KA_STARE";
	case TLK_USE: return "KA_OK";
	case TLK_UNUSE: return "KA_WAIT";
	case TLK_DECLINE: return "KA_POK";
	case TLK_STOP: return "KA_STOP";
	case TLK_NOSHOOT: return "KA_SCARED";
	case TLK_HELLO: return "KA_HELLO";
	case TLK_PLHURT1: return "!KA_CUREA";
	case TLK_PLHURT2: return "!KA_CUREB";
	case TLK_PLHURT3: return "!KA_CUREC";
	case TLK_PHELLO: return "KA_PHELLO";
	case TLK_PIDLE: return "KA_PIDLE";
	case TLK_PQUESTION: return "KA_PQUEST";
	case TLK_SMELL: return "KA_SMELL";
	case TLK_WOUND: return "KA_WOUND";
	case TLK_MORTAL: return "KA_MORTAL";
	case TLK_SHOT: return "KA_SHOT";
	case TLK_MAD: return "KA_MAD";
	case TLK_KILL: return "KA_KILL";
	case TLK_ATTACK: return "KA_ATTACK";
	default: return NULL;
	}
}

void CKate::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		ReportFireAnimEvent(pEvent->event);
		BarneyFirePistol(firePistolSoundScript, GetSkillValue("9mm_bullet"));
		break;
	case KATE_AE_KICK:
	{
		TraceHullAttackParams params;
		params.punchAngle.x = 5;
		params.knockForward = -100;
		params.knockUp = 50;
		params.damageInfo.damage = GetSkillValue("hgrunt_kick");
		params.damageInfo.type = DMG_CLUB;
		params.skipAllies = true;
		SetTraceHullAttackParamsFromTemplate(pEvent->event, params);

		if (PerformTraceHullAttack(params))
		{
			const SoundScript* hitSoundScript = nullptr;
			if( m_iCombatState == -1 )
			{
				hitSoundScript = GetSoundScript(kickSoundScript);
			}
			else
			{
				++m_iCombatState;
				if( m_iCombatState == 3 )
					hitSoundScript = GetSoundScript(kickSoundScript);
				else
					hitSoundScript = GetSoundScript(punchSoundScript);
			}
			EmitSoundScript(hitSoundScript);
		}
	}
		break;
	default:
		CBarney::HandleAnimEvent( pEvent );
	}
}

int CKate::LookupActivity(int activity)
{
	switch (activity)
	{
	case ACT_RUN:
		if( pev->health <= LimpHealth() )
		{
			int seq = CBarney::LookupActivity( ACT_RUN_HURT );
			if (seq != ACTIVITY_NOT_AVAILABLE)
				return seq;
		}
		return CBarney::LookupActivity( activity );
	case ACT_WALK:
		if( pev->health <= LimpHealth() )
		{
			int seq = CBarney::LookupActivity( ACT_WALK_HURT );
			if (seq != ACTIVITY_NOT_AVAILABLE)
				return seq;
		}
		return CBarney::LookupActivity( activity );
	case ACT_MELEE_ATTACK1:
		if( RANDOM_LONG( 0, 2 ) )
		{
			m_iCombatState = -1;
			return LookupSequence( "karate_hit" );
		}
		else
		{
			m_iCombatState = 0;
			return LookupSequence( "karate_bighit" );
		}
		break;
	default:
		return CBarney::LookupActivity( activity );
	}
}

bool CKate::CheckMeleeAttack1(float flDot, float flDist)
{
	return CTalkMonster::CheckMeleeAttack1(flDot, flDist);
}

void CKate::DeathSound()
{
	EmitSoundScriptTalk(dieSoundScript);
}

void CKate::PainSound()
{
	EmitSoundScriptTalk(painSoundScript);
}

void CKate::OnDying(bool gibbed)
{
	if( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && HasGun() )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		SetGunState(BARNEY_BODY_GUNGONE);
		GetAttachment( 0, vecGunPos, vecGunAngles );

		DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying(gibbed);
}

DamageInfo CKate::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	return DefaultHandleTraceAttackImpl(pevInflictor, pevAttacker, inputDamageInfo, vecDir, ptr, false);
}

float CKate::LimpHealth()
{
	return Q_min(pev->max_health/2.0f, KATE_LIMP_HEALTH);
}

class CDeadKate : public CDeadBarney
{
public:
	void Spawn() override;
	const char* DefaultModel() override { return "models/kate.mdl"; }
	bool IsEnabledInMod() override { return g_modFeatures.IsMonsterEnabled("kate"); }
	const char* getPos(int pos) const override;
	static const char *m_szPoses[3];
};

const char *CDeadKate::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

const char* CDeadKate::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_kate_dead, CDeadKate )

void CDeadKate::Spawn()
{
	SpawnHelper();
	SetBarneyHead(this, head);
	MonsterInitDead();
}
