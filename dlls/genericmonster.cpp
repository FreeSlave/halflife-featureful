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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 
#define SF_HEAD_CONTROLLER					8

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int DefaultClassify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int DefaultISoundMask( void );
	void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity *pListener ) override;
	void IdleHeadTurn( Vector &vecFriend );
	void MonsterThink();
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	Vector DefaultMinHullSize() { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() { return VEC_HUMAN_HULL_MAX; }
private:
	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;

};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster )

TYPEDESCRIPTION CGenericMonster::m_SaveData[] =
{
	DEFINE_FIELD( CGenericMonster, m_talkTime, FIELD_FLOAT ),
	DEFINE_FIELD( CGenericMonster, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CGenericMonster, m_flIdealYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CGenericMonster, m_flCurrentYaw, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGenericMonster, CBaseMonster )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CGenericMonster::DefaultClassify( void )
{
	return CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::DefaultISoundMask( void )
{
	return 0;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();

	SetMyModel(nullptr);
	if (FStringNull(pev->model))
	{
		ALERT(at_console, "Spawning monster_generic without model!\n");
	}
/*
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) )
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) || FStrEq( STRING( pev->model ), "models/holo.mdl" ) )
		SetMySize( VEC_HULL_MIN, VEC_HULL_MAX );
	else
		SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( 8 );
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();

	if( pev->spawnflags & SF_HEAD_CONTROLLER )
	{
		m_afCapability = bits_CAP_TURN_HEAD;
	}
	SetMyCanOpenDoors(false);

	m_flIdealYaw = m_flCurrentYaw = 0;

	if( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	PrecacheMyModel(nullptr);
	PrecacheMyGibModel();
}

void CGenericMonster::PlayScriptedSentence(const char *pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity *pListener )
{
	m_talkTime = gpGlobals->time + duration;
	PlaySentence( pszSentence, duration, volume, attenuation, true );

	m_hTalkTarget = pListener;
}

void CGenericMonster::IdleHeadTurn( Vector &vecFriend )
{
	// turn head in desired direction only if ent has a turnable head
	if( m_afCapability & bits_CAP_TURN_HEAD )
	{
		float yaw = VecToYaw( vecFriend - pev->origin ) - pev->angles.y;

		if( yaw > 180 )
			yaw -= 360;
		if( yaw < -180 )
			yaw += 360;

		m_flIdealYaw = yaw;
	}
}

void CGenericMonster::MonsterThink()
{
	if( m_afCapability & bits_CAP_TURN_HEAD )
		{
		if( m_hTalkTarget != 0 )
		{
			if( gpGlobals->time > m_talkTime )
			{
				m_flIdealYaw = 0;
				m_hTalkTarget = 0;
			}
			else
			{
				IdleHeadTurn( m_hTalkTarget->pev->origin );
			}
		}

		if( m_flCurrentYaw != m_flIdealYaw )
		{
			if( m_flCurrentYaw <= m_flIdealYaw )
			{
				m_flCurrentYaw += Q_min( m_flIdealYaw - m_flCurrentYaw, 20.0f );
			}
			else
			{
				m_flCurrentYaw -= Q_min( m_flCurrentYaw - m_flIdealYaw, 20.0f );
			}
			SetBoneController( 0, m_flCurrentYaw );
		}
	}

	CBaseMonster::MonsterThink();
}

KilledResult CGenericMonster::Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib)
{
	SentenceStop();
	return CBaseMonster::Killed(pevInflictor, pevAttacker, iGib);
}

class CDeadGenericMonster : public CBaseMonster
{
public:
	void Precache();
	void Spawn();
	void KeyValue( KeyValueData *pkvd );
	int DefaultClassify() { return CLASS_HUMAN_PASSIVE; }
	bool ShouldCollide(CBaseEntity* pOther) override;
};

LINK_ENTITY_TO_CLASS( monster_generic_dead, CDeadGenericMonster )

void CDeadGenericMonster::Precache()
{
	PrecacheMyModel(nullptr);
	PrecacheMyGibModel();
}

void CDeadGenericMonster::Spawn()
{
	Precache();
	SetMyModel(nullptr);
	if (FStringNull(pev->model))
	{
		ALERT(at_console, "Spawning monster_generic_dead without model!\n");
	}

	pev->effects &= EF_INVLIGHT;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	SetMyBloodColor( BLOOD_COLOR_RED );

	const int desiredActivity = (int)pev->frags;

	if (FStringNull(pev->netname))
	{
		if (desiredActivity <= 0)
			ALERT(at_console, "Spawning %s without pose!\n", STRING(pev->classname));
		else
		{
			pev->sequence = LookupActivity( desiredActivity );
			if (pev->sequence == -1) {
				ALERT(at_console, "%s with model %s has no activity %d\n", STRING(pev->classname), STRING(pev->model), desiredActivity);
				if (desiredActivity != ACT_DIESIMPLE) {
					pev->sequence = LookupActivity( ACT_DIESIMPLE ); // try simple death animation
				}
				if (pev->sequence == -1) {
					pev->sequence = 0;
				}
			}
		}
	}
	else
	{
		pev->sequence = LookupSequence( STRING(pev->netname) );
		if (pev->sequence == -1)
		{
			ALERT ( at_console, "%s with bad pose (no %s animation in %s)\n", STRING(pev->classname), STRING(pev->netname), STRING(pev->model) );
		}
	}

	SetMyHealth( 8 );
	MonsterInitDead();
	if (pev->spawnflags & SF_GENERICMONSTER_NOTSOLID)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	pev->frame = 255;
}

void CDeadGenericMonster::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

bool CDeadGenericMonster::ShouldCollide(CBaseEntity* pOther)
{
	return pOther->ShouldCollideWithCorpses();
}

//=========================================================
// Op4 Loader
//=========================================================

class CLoader : public CGenericMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int DefaultClassify() {return CLASS_NONE;}
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;
};

LINK_ENTITY_TO_CLASS(monster_op4loader, CLoader)

void CLoader::Spawn()
{
	Precache();

	SetMyModel("models/loader.mdl");

	SetMySize( VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor(DONT_BLEED);
	SetMyHealth(100);
	SetMyFieldOfView(0.5f);
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
	pev->takedamage = DAMAGE_NO;

	if (pev->spawnflags & SF_GENERICMONSTER_NOTSOLID)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
}

void CLoader::Precache()
{
	PrecacheMyModel("models/loader.mdl");
	PrecacheMyGibModel();
}

void CLoader::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr)
{
	UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(1.0,2.0) );
}
