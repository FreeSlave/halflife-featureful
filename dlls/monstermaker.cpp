/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// Monster Maker - this is an entity that creates monsters
// in the game.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"
#include "locus.h"

#define MONSTERMAKER_START_ON_FIX 1

// Monstermaker spawnflags
#define	SF_MONSTERMAKER_START_ON	1 // start active ( if has targetname )
#define	SF_MONSTERMAKER_CYCLIC		4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP	8 // Children are blocked by monsterclip
#define SF_MONSTERMAKER_PRISONER	16
#define SF_MONSTERMAKER_AUTOSIZEBBOX 32
#define SF_MONSTERMAKER_CYCLIC_BACKLOG 64
#define SF_MONSTERMAKER_WAIT_FOR_SCRIPT 128 // TODO: implement
#define SF_MONSTERMAKER_PREDISASTER 256
#define SF_MONSTERMAKER_DONT_DROP_GUN 1024 // Spawn monster won't drop gun upon death
#define SF_MONSTERMAKER_NO_GROUND_CHECK 2048 // don't check if something on ground prevents a monster to fall on spawn
#define SF_MONSTERMAKER_ALIGN_TO_PLAYER 4096 // Align to closest player on spawn
#define SF_MONSTERMAKER_WAIT_UNTIL_PROVOKED 8192
#define SF_MONSTERMAKER_PASS_MONSTER_AS_ACTIVATOR 16384 // DEPRECATED. Use the spawned monster as activator to fire target
#define SF_MONSTERMAKER_SPECIAL_FLAG		32768
#define SF_MONSTERMAKER_NONSOLID_CORPSE		65536

enum
{
	MONSTERMAKER_LIMIT = 0,
	MONSTERMAKER_BLOCKED,
	MONSTERMAKER_NULLENTITY,
	MONSTERMAKER_SPAWNED,
	MONSTERMAKER_BADPLACE,
};

typedef enum
{
	MMA_NO = -1,
	MMA_DEFAULT = 0,
	MMA_MAKER = 1,
	MMA_MONSTER = 2,
	MMA_FORWARD = 3,
} MONSTERMAKER_TARGET_ACTIVATOR;

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================
class CMonsterMaker : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MakerThink( void );
	void EXPORT CyclicBacklogThink( void );
	void DeathNotice( entvars_t *pevChild );// monster maker children use this to tell the monster maker that they have died.
	int MakeMonster( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMonsterClassname;// classname of the monster(s) that will be created.

	int m_cNumMonsters;// max number of monsters this ent can create

	int m_cLiveChildren;// how many monsters made by this monster maker that are currently alive
	int m_iMaxLiveChildren;// max number of monsters that this maker may have out at one time.

	float m_flGround; // z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child

	BOOL m_fActive;
	BOOL m_fFadeChildren;// should we make the children fadeout?
	string_t m_customModel;
	int m_iPose;
	BOOL m_notSolid;
	BOOL m_gag;
	int m_iHead;
	int m_cyclicBacklogSize;
	string_t m_iszPlacePosition;
	short m_targetActivator;
	short m_iMaxYawDeviation;
	Vector m_defaultMinHullSize;
	Vector m_defaultMaxHullSize;
};

LINK_ENTITY_TO_CLASS( monstermaker, CMonsterMaker )
LINK_ENTITY_TO_CLASS( squadmaker, CMonsterMaker )

TYPEDESCRIPTION	CMonsterMaker::m_SaveData[] =
{
	DEFINE_FIELD( CMonsterMaker, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_cNumMonsters, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fFadeChildren, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_customModel, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_iPose, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_notSolid, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_gag, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_iHead, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_minHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_maxHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_cyclicBacklogSize, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_iszPlacePosition, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_targetActivator, FIELD_SHORT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxYawDeviation, FIELD_SHORT ),
	DEFINE_FIELD( CMonsterMaker, m_defaultMinHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_defaultMaxHullSize, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CMonsterMaker, CBaseMonster )

void CMonsterMaker::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "monstercount" ) )
	{
		m_cNumMonsters = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_imaxlivechildren" ) )
	{
		m_iMaxLiveChildren = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "monstertype" ) )
	{
		m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "warpball" ) || FStrEq( pkvd->szKeyName, "xenmaker" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "new_model" ) )
	{
		m_customModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "notsolid" ) )
	{
		m_notSolid = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "gag" ) )
	{
		m_gag = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "head" ) )
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_target" ) )
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_condition" ) )
	{
		m_iTriggerCondition = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_alt_condition" ) )
	{
		m_iTriggerAltCondition = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "respawn_as_playerally" ) )
	{
		m_reverseRelationship = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "target_activator" ) )
	{
		m_targetActivator = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spawnorigin" ) )
	{
		m_iszPlacePosition = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "yawdeviation" ) )
	{
		m_iMaxYawDeviation = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CMonsterMaker::Spawn()
{
	if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_PASS_MONSTER_AS_ACTIVATOR))
	{
		ALERT(at_console, "%s: Usage of 'Pass monster as activator' flag is deprecated! Use Target's Activator instead\n", STRING(pev->classname));
	}

	pev->solid = SOLID_NOT;

	if (FStringNull(m_iszPlacePosition))
	{
		// Spirit compat
		m_iszPlacePosition = pev->noise;
		pev->noise = iStringNull;
	}

	m_cLiveChildren = 0;
	Precache();
	if( !FStringNull( pev->targetname ) )
	{
		if( pev->spawnflags & SF_MONSTERMAKER_CYCLIC )
		{
			SetUse( &CMonsterMaker::CyclicUse );// drop one monster each time we fire
		}
		else
		{
			SetUse( &CMonsterMaker::ToggleUse );// so can be turned on/off
		}

		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_START_ON ) )
		{
			// start making monsters as soon as monstermaker spawns
#if MONSTERMAKER_START_ON_FIX
			pev->nextthink = gpGlobals->time + m_flDelay;
#endif
			m_fActive = TRUE;
			SetThink( &CMonsterMaker::MakerThink );
		}
		else if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_CYCLIC ) && FBitSet( pev->spawnflags, SF_MONSTERMAKER_CYCLIC_BACKLOG ) )
		{
			SetThink( &CMonsterMaker::CyclicBacklogThink );
		}
		else
		{
			// wait to be activated.
			m_fActive = FALSE;
			SetThink( &CBaseEntity::SUB_DoNothing );
		}
	}
	else
	{
		// no targetname, just start.
		pev->nextthink = gpGlobals->time + m_flDelay;
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}

	if( m_cNumMonsters == 1 )
	{
		m_fFadeChildren = FALSE;
	}
	else
	{
		m_fFadeChildren = TRUE;
	}

	m_flGround = 0;
}

void CMonsterMaker::Precache( void )
{
	CBaseMonster::Precache();

	if (!FStringNull(m_customModel))
		PRECACHE_MODEL(STRING(m_customModel));
	if (!FStringNull(m_gibModel))
		PRECACHE_MODEL(STRING(m_gibModel));

	UTIL_PrecacheMonster( STRING(m_iszMonsterClassname), m_reverseRelationship, &m_defaultMinHullSize, &m_defaultMaxHullSize );
}

static CBaseEntity* MakerBlocker(const Vector& mins, const Vector& maxs)
{
	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
	if( count )
	{
		// don't build a stack of monsters!
		return pList[0];
	}
	return NULL;
}

static float MakerGroundLevel(const Vector& position, entvars_t* pev)
{
	TraceResult tr;
	UTIL_TraceLine( position, position - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
	return tr.vecEndPos.z;
}

//=========================================================
// MakeMonster-  this is the code that drops the monster
//=========================================================
int CMonsterMaker::MakeMonster( void )
{
	edict_t	*pent;
	entvars_t *pevCreate;
	edict_t *posEnt = NULL;

	if( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{
		// not allowed to make a new one yet. Too many live ones out right now.
		return MONSTERMAKER_LIMIT;
	}

	Vector minHullSize = Vector( -34, -34, 0 );
	Vector maxHullSize = Vector( 34, 34, 0 );

	if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_AUTOSIZEBBOX))
	{
		if (m_minHullSize != g_vecZero)
		{
			minHullSize = Vector( m_minHullSize.x, m_minHullSize.y, 0 );
		}
		else if (m_defaultMinHullSize != g_vecZero)
		{
			minHullSize = Vector( m_defaultMinHullSize.x, m_defaultMinHullSize.y, 0 );
		}

		if (m_maxHullSize != g_vecZero)
		{
			maxHullSize = Vector( m_maxHullSize.x, m_maxHullSize.y, 0 );
		}
		else if (m_defaultMaxHullSize != g_vecZero)
		{
			maxHullSize = Vector( m_defaultMaxHullSize.x, m_defaultMaxHullSize.y, 0 );
		}
	}

	Vector placePosition;
	Vector placeAngles;
	Vector mins, maxs;

	const char* placeIdentifier = STRING(m_iszPlacePosition);
	if (!FStringNull(m_iszPlacePosition) && *placeIdentifier == '@' && placeIdentifier[1] != '\0')
	{
		// Random spot out of several ones

		const char* candidateName = placeIdentifier+1;

		int total = 0;
		CBaseEntity* pCandidate = NULL;
		CBaseEntity* pLastValidCandidate = NULL;
		CBaseEntity* pChosenSpot = NULL;
		bool foundAnything = false;

		while( ( pCandidate = UTIL_FindEntityByTargetname( pCandidate, candidateName ) ) != NULL )
		{
			foundAnything = true;

			mins = pCandidate->pev->origin + minHullSize;
			maxs = pCandidate->pev->origin + maxHullSize;
			maxs.z = pCandidate->pev->origin.z;

			if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK))
			{
				mins.z = MakerGroundLevel(pCandidate->pev->origin, pev);
			}

			if (MakerBlocker(mins, maxs) == 0)
			{
				pLastValidCandidate = pCandidate;
				total++;
				if (RANDOM_LONG(0, total - 1) < 1)
					pChosenSpot = pLastValidCandidate;
			}
		}

		if (pChosenSpot)
		{
			placePosition = pChosenSpot->pev->origin;
			placeAngles = pChosenSpot->pev->angles;
			posEnt = pChosenSpot->edict();
		}
		else
		{
			if (foundAnything)
				return MONSTERMAKER_BLOCKED;
			else
				return MONSTERMAKER_BADPLACE;
		}
	}
	else
	{
		// Single spot
		placeAngles = pev->angles;

		if (FStringNull(m_iszPlacePosition))
		{
			placePosition = pev->origin;
			posEnt = edict();
		}
		else
		{
			bool evaluated;
			placePosition = CalcLocus_Position(this, m_hActivator, placeIdentifier, &evaluated);
			if (!evaluated)
				return MONSTERMAKER_BADPLACE;
			CBaseEntity* tempPosEnt = CBaseEntity::Create("info_target", placePosition, pev->angles);
			if (tempPosEnt)
			{
				tempPosEnt->SetThink(&CBaseEntity::SUB_Remove);
				tempPosEnt->pev->nextthink = gpGlobals->time + 0.1;
				posEnt = tempPosEnt->edict();
			}
		}

		if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK))
		{
			if( !m_flGround
					|| !FStringNull(m_iszPlacePosition) ) // The position could change, so we need to calculate the new ground level.
			{
				// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
				m_flGround = MakerGroundLevel(placePosition, pev);
			}
		}

		mins = placePosition + minHullSize;
		maxs = placePosition + maxHullSize;

		maxs.z = placePosition.z;
		if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK))
			mins.z = m_flGround;

		CBaseEntity *pBlocker = MakerBlocker(mins, maxs);
		if (pBlocker)
		{
			const char* blockerName = FStringNull(pBlocker->pev->classname) ? "" : STRING(pBlocker->pev->classname);
			ALERT( at_aiconsole, "Spawning of %s is blocked by %s\n", STRING(m_iszMonsterClassname), blockerName );
			return MONSTERMAKER_BLOCKED;
		}
	}

	if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_ALIGN_TO_PLAYER))
	{
		float minDist = 10000.0f;
		CBaseEntity* foundPlayer = NULL;
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			CBaseEntity* player = UTIL_PlayerByIndex(i);
			if (player && player->IsPlayer() && player->IsAlive()) {
				const float dist = (pev->origin - player->pev->origin).Length();
				if (dist < minDist) {
					minDist = dist;
					foundPlayer = player;
				}
			}
		}
		if (foundPlayer) {
			placeAngles = Vector(0, UTIL_VecToYaw(foundPlayer->pev->origin - placePosition), 0);
		}
	}

	if (m_iMaxYawDeviation)
	{
		const int deviation = RANDOM_LONG(0, m_iMaxYawDeviation);
		if (RANDOM_LONG(0,1)) {
			placeAngles.y += deviation;
		} else {
			placeAngles.y -= deviation;
		}
		placeAngles.y = UTIL_AngleMod(placeAngles.y);
	}

	pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );

	if( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in MonsterMaker!\n" );
		return MONSTERMAKER_NULLENTITY;
	}

	pevCreate = VARS( pent );
	pevCreate->origin = placePosition;
	pevCreate->angles = placeAngles;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );
	pevCreate->body = pev->body;
	pevCreate->skin = pev->skin;
	pevCreate->health = pev->health;
	pevCreate->scale = pev->scale;
	if (!FStringNull(m_customModel))
		pevCreate->model = m_customModel;

	CBaseMonster* createdMonster = GetMonsterPointer(pent);
	if (createdMonster)
	{
		// Children hit monsterclip brushes
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_MONSTERCLIP ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_HITMONSTERCLIP );

		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_PRISONER ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_PRISONER );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_PREDISASTER ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_PREDISASTER );
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_DONT_DROP_GUN))
			SetBits(pevCreate->spawnflags, SF_MONSTER_DONT_DROP_GUN);
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_WAIT_UNTIL_PROVOKED ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_WAIT_UNTIL_PROVOKED );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_SPECIAL_FLAG ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_SPECIAL_FLAG );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_NONSOLID_CORPSE ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_NONSOLID_CORPSE );
		if (m_gag > 0)
			SetBits(pevCreate->spawnflags, SF_MONSTER_GAG);
		pevCreate->weapons = pev->weapons;

		if (m_iClass)
			createdMonster->m_iClass = m_iClass;
		createdMonster->m_reverseRelationship = m_reverseRelationship;
		createdMonster->m_displayName = m_displayName;
		createdMonster->SetMyBloodColor(m_bloodColor);
		if (!FStringNull(m_gibModel))
			createdMonster->m_gibModel = m_gibModel;

		if (!FStringNull(m_iszTriggerTarget))
		{
			createdMonster->m_iszTriggerTarget = m_iszTriggerTarget;
			createdMonster->m_iTriggerCondition = m_iTriggerCondition;
			createdMonster->m_iTriggerAltCondition = m_iTriggerAltCondition;
		}

		createdMonster->m_minHullSize = m_minHullSize;
		createdMonster->m_maxHullSize = m_maxHullSize;

		createdMonster->m_customSoundMask = m_customSoundMask;
		createdMonster->m_prisonerTo = m_prisonerTo;

		createdMonster->SetHead(m_iHead);

		CDeadMonster* deadMonster = createdMonster->MyDeadMonsterPointer();
		if (deadMonster)
		{
			deadMonster->m_iPose = m_iPose;
		}
	}

	DispatchSpawn( ENT( pevCreate ) );
	pevCreate->owner = edict();
	// Disable until proper investigation
#if 0
	if (m_notSolid > 0)
	{
		pevCreate->solid = SOLID_NOT;
	}
#endif

	if ( !FStringNull( pev->message ) )
	{
		CBaseEntity* foundEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
		if ( foundEntity && (FClassnameIs(foundEntity->pev, "env_warpball")
							 || FClassnameIs(foundEntity->pev, "env_xenmaker")))
		{
			Vector vecPosition = pevCreate->origin;
			if (createdMonster)
			{
				Vector vecJunk;

				switch (pev->impulse) {
				case 1:
					vecPosition = createdMonster->EyePosition();
					break;
				case 3:
					vecPosition = createdMonster->Center();
					break;
				case 5:
					createdMonster->GetAttachment(0, vecPosition, vecJunk);
					break;
				case 6:
					createdMonster->GetAttachment(1, vecPosition, vecJunk);
					break;
				case 7:
					createdMonster->GetAttachment(2, vecPosition, vecJunk);
					break;
				case 8:
					createdMonster->GetAttachment(3, vecPosition, vecJunk);
					break;
				default:
					break;
				}
			}

			foundEntity->pev->vuser1 = vecPosition;
			foundEntity->pev->dmg_inflictor = posEnt;
			foundEntity->Use(this, this, USE_SET, 0.0f);
			foundEntity->pev->vuser1 = g_vecZero;
			foundEntity->pev->dmg_inflictor = NULL;
		}
		else
		{
			ALERT(at_error, "template %s for %s is not env_warpball or does not exist\n", STRING(pev->message), STRING(pev->classname));
		}
	}

	if( !FStringNull( pev->netname ) )
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++;// count this monster
	if (m_cNumMonsters > 0)
		m_cNumMonsters--;

	if( m_cNumMonsters == 0 )
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink( NULL );
		SetUse( NULL );
	}

	// If I have a target, fire!
	if( !FStringNull( pev->target ) )
	{
		// NOTE: in Spirit activator is monster.
		// We keep original Half-Life behavior by default, but it can be configured.
		CBaseEntity* pActivator = this;
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_PASS_MONSTER_AS_ACTIVATOR))
		{
			pActivator = createdMonster;
		}
		switch (m_targetActivator) {
		case MMA_NO:
			pActivator = NULL;
			break;
		case MMA_MAKER:
			pActivator = this;
			break;
		case MMA_MONSTER:
			pActivator = createdMonster;
			break;
		case MMA_FORWARD:
			pActivator = m_hActivator;
			break;
		case MMA_DEFAULT:
		default:
			break;
		}
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets( STRING( pev->target ), pActivator, this, USE_TOGGLE, 0 );
	}

	return MONSTERMAKER_SPAWNED;
}

//=========================================================
// CyclicUse - drops one monster from the monstermaker
// each time we call this.
//=========================================================
void CMonsterMaker::CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;

	if (MakeMonster() == MONSTERMAKER_BLOCKED)
	{
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_CYCLIC_BACKLOG))
		{
			m_cyclicBacklogSize++;
			pev->nextthink = gpGlobals->time + m_flDelay;
		}
	}
}

//=========================================================
// ToggleUse - activates/deactivates the monster maker
//=========================================================
void CMonsterMaker::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_fActive ) )
		return;

	m_hActivator = pActivator;

	if( m_fActive )
	{
		m_fActive = FALSE;
		SetThink( NULL );
	}
	else
	{
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}

	pev->nextthink = gpGlobals->time;
}

//=========================================================
// MakerThink - creates a new monster every so often
//=========================================================
void CMonsterMaker::MakerThink( void )
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	MakeMonster();
}

void CMonsterMaker::CyclicBacklogThink()
{
	if (MakeMonster() == MONSTERMAKER_SPAWNED)
	{
		m_cyclicBacklogSize--;
	}
	if (m_cyclicBacklogSize > 0)
		pev->nextthink = gpGlobals->time + m_flDelay;
}

//=========================================================
//=========================================================
void CMonsterMaker::DeathNotice( entvars_t *pevChild )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_cLiveChildren--;
	ALERT(at_aiconsole, "Monstermaker DeathNotice: %d live children left\n", m_cLiveChildren);

	if( !m_fFadeChildren )
	{
		pevChild->owner = NULL;
	}

	if (m_cNumMonsters == 0)
	{
		SetThink(&CMonsterMaker::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
}
