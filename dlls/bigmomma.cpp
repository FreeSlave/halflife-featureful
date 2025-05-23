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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"decals.h"
#include	"combat.h"
#include	"game.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

#define SF_INFOBM_RUN		0x0001
#define SF_INFOBM_WAIT		0x0002
#define SF_INFOBM_FORGET_ENEMIES 0x0004

#define SF_BIGMOM_NOBABYCRABS SF_MONSTER_DONT_DROP_GUN
#define SF_MONSTERCLIP_BABYCRABS SF_MONSTER_SPECIAL_FLAG

enum
{
	LAYCRABS_NOCHANGE = 0,
	LAYCRABS_ON,
	LAYCRABS_OFF,
	LAYCRABS_TOGGLE,
};

// AI Nodes for Big Momma
class CInfoBM : public CPointEntity
{
public:
	void KeyValue( KeyValueData* pkvd );

	// name in pev->targetname
	// next in pev->target
	// radius in pev->scale
	// health in pev->health
	// Reach target in pev->message
	// Reach delay in pev->speed
	// Reach sequence in pev->netname
	
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_preSequence;
};

LINK_ENTITY_TO_CLASS( info_bigmomma, CInfoBM )

TYPEDESCRIPTION	CInfoBM::m_SaveData[] =
{
	DEFINE_FIELD( CInfoBM, m_preSequence, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CInfoBM, CPointEntity )

void CInfoBM::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		pev->scale = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "reachdelay" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "reachtarget" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "reachsequence" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "presequence" ) )
	{
		m_preSequence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "laycrabs" ) )
	{
		pev->weapons = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

//=========================================================
// Mortar shot entity
//=========================================================
class CBMortar : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache();

	static CBMortar *Shoot( edict_t *pOwner, Vector vecStart, Vector vecVelocity, EntityOverrides entityOverrides );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxFrame;

	static constexpr const char* spitTouchSoundScript = "BigMomma.SpitTouch";
	static constexpr const char* spitHitSoundScript = "BigMomma.SpitHit";

	static const NamedVisual mortarVisual;
	static const NamedVisual mortarSprayVisual;
};

LINK_ENTITY_TO_CLASS( bmortar, CBMortar )

TYPEDESCRIPTION	CBMortar::m_SaveData[] =
{
	DEFINE_FIELD( CBMortar, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBMortar, CBaseEntity )

const NamedVisual CBMortar::mortarVisual = BuildVisual("BigMomma.Mortar")
		.Model("sprites/mommaspit.spr")
		.RenderMode(kRenderTransAlpha)
		.Alpha(255)
		.Scale(2.5f)
		.Framerate(10.0f);

const NamedVisual CBMortar::mortarSprayVisual = BuildVisual::Spray("BigMomma.MortarSpray")
		.Model("sprites/mommaspout.spr");

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BIG_AE_STEP1			1		// Footstep left
#define	BIG_AE_STEP2			2		// Footstep right
#define	BIG_AE_STEP3			3		// Footstep back left
#define	BIG_AE_STEP4			4		// Footstep back right
#define BIG_AE_SACK			5		// Sack slosh
#define BIG_AE_DEATHSOUND		6		// Death sound

#define	BIG_AE_MELEE_ATTACKBR		8		// Leg attack
#define	BIG_AE_MELEE_ATTACKBL		9		// Leg attack
#define	BIG_AE_MELEE_ATTACK1		10		// Leg attack
#define BIG_AE_MORTAR_ATTACK1		11		// Launch a mortar
#define BIG_AE_LAY_CRAB			12		// Lay a headcrab
#define BIG_AE_JUMP_FORWARD		13		// Jump up and forward
#define BIG_AE_SCREAM			14		// alert sound
#define BIG_AE_PAIN_SOUND		15		// pain sound
#define BIG_AE_ATTACK_SOUND		16		// attack sound
#define BIG_AE_BIRTH_SOUND		17		// birth sound
#define BIG_AE_EARLY_TARGET		50		// Fire target early

// User defined conditions
#define bits_COND_NODE_SEQUENCE			( bits_COND_SPECIAL1 )		// pev->netname contains the name of a sequence to play

// Attack distance constants
#define	BIG_ATTACKDIST			170.0f
#define BIG_MORTARDIST			800.0f
#define BIG_MAXCHILDREN			20			// Max # of live headcrab children

#define bits_MEMORY_CHILDPAIR		( bits_MEMORY_CUSTOM1 )
#define bits_MEMORY_ADVANCE_NODE	( bits_MEMORY_CUSTOM2 )
#define bits_MEMORY_COMPLETED_NODE	( bits_MEMORY_CUSTOM3 )
#define bits_MEMORY_FIRED_NODE		( bits_MEMORY_CUSTOM4 )

Vector VecCheckSplatToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float maxHeight );
void MortarSpray( const Vector &position, const Vector &direction, const Visual* visual, int count );

// UNDONE:
//
#define BIG_CHILDCLASS		"monster_babycrab"

class CBigMomma : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Activate( void );
	DamageInfo DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo) override;
	void BeforeApplyDamageToHealth(float flDamage) override;

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	DamageInfo DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr) override;
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr ) override;

	void NodeStart(string_t iszNextNode );
	void NodeReach( void );
	bool ShouldGoToNode( void );

	void SetYawSpeed( void );
	int DefaultClassify( void );
	const char* DefaultDisplayName() { return "Big Momma"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void LayHeadcrab( void );

	CInfoBM* GetTargetInfoBM()
	{
		if (m_hTargetEnt != 0 && FClassnameIs(m_hTargetEnt->pev, "info_bigmomma"))
		{
			return m_hTargetEnt.Entity<CInfoBM>();
		}
		return nullptr;
	}

	string_t GetNodeSequence( void )
	{
		CInfoBM *pTarget = GetTargetInfoBM();
		if( pTarget )
		{
			return pTarget->pev->netname;	// netname holds node sequence
		}
		return iStringNull;
	}

	string_t GetNodePresequence( void )
	{
		CInfoBM *pTarget = GetTargetInfoBM();
		if( pTarget )
		{
			return pTarget->m_preSequence;
		}
		return iStringNull;
	}

	float GetNodeDelay( void )
	{
		CInfoBM *pTarget = GetTargetInfoBM();
		if( pTarget )
		{
			return pTarget->pev->speed;	// Speed holds node delay
		}
		return 0;
	}

	float GetNodeRange( void )
	{
		CInfoBM *pTarget = GetTargetInfoBM();
		if( pTarget )
		{
			return pTarget->pev->scale;	// Scale holds node delay
		}
		return 1e6;
	}

	float GetNodeYaw( void )
	{
		CBaseEntity *pTarget = m_hTargetEnt;
		if( pTarget )
		{
			if( pTarget->pev->angles.y != 0 )
				return pTarget->pev->angles.y;
		}
		return pev->angles.y;
	}

	int GetNodeLayCrabs( void )
	{
		CInfoBM *pTarget = GetTargetInfoBM();
		if( pTarget )
		{
			return pTarget->pev->weapons;
		}
		return 0;
	}

	// Restart the crab count on each new level
	void OverrideReset( void )
	{
		m_crabCount = 0;
	}

	void DeathNotice( entvars_t *pevChild );

	bool CanLayCrab( void )
	{ 
		if (FBitSet(pev->spawnflags, SF_BIGMOM_NOBABYCRABS))
			return false;

		if( m_crabTime < gpGlobals->time && m_crabCount < BIG_MAXCHILDREN )
		{
			// Don't spawn crabs inside each other
			Vector mins = pev->origin - Vector( 32.0f, 32.0f, 0.0f );
			Vector maxs = pev->origin + Vector( 32.0f, 32.0f, 0.0f );

			CBaseEntity *pList[2];
			int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_MONSTER );
			for( int i = 0; i < count; i++ )
			{
				if( pList[i] != this )	// Don't hurt yourself!
					return false;
			}
			return true;
		}

		return false;
	}

	void LaunchMortar( void );

	void SetObjectCollisionBox( void )
	{
		SetMyObjectCollisionBox(Vector( -95.0f, -95.0f, 0.0f ), Vector( 95.0f, 95.0f, 190.0f ));
	}

	bool CheckMeleeAttack1( float flDot, float flDist ) override;	// Slash
	bool CheckMeleeAttack2( float flDot, float flDist ) override;	// Lay a crab
	bool CheckRangeAttack1( float flDot, float flDist ) override;	// Mortar launch

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ReportAIState(ALERT_TYPE level);

	virtual int DefaultSizeForGrapple() { return GRAPPLE_LARGE; }
	Vector DefaultMinHullSize() { return Vector( -32.0f, -32.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 32.0f, 32.0f, 64.0f ); }

	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript attackSoundScript;
	static constexpr const char* attackHitSoundScript = "BigMomma.AttackHit";
	static const NamedSoundScript footstepLeftSoundScript;
	static const NamedSoundScript footstepRightSoundScript;
	static const NamedSoundScript birthSoundScript;
	static const NamedSoundScript layHeadcrabSoundScript;
	static const NamedSoundScript childDieSoundScript;
	static const NamedSoundScript sackSoundScript;
	static const NamedSoundScript launchMortarSoundScript;

	CUSTOM_SCHEDULES

private:
	float m_nodeTime;
	float m_crabTime;
	float m_mortarTime;
	float m_painSoundTime;
	int m_crabCount;

	int m_SpitSprite;
};

LINK_ENTITY_TO_CLASS( monster_bigmomma, CBigMomma )

TYPEDESCRIPTION	CBigMomma::m_SaveData[] =
{
	DEFINE_FIELD( CBigMomma, m_nodeTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_crabTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_mortarTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_painSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_crabCount, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBigMomma, CBaseMonster )

constexpr IntRange bigMommaPitch(95, 105);

const NamedSoundScript CBigMomma::alertSoundScript = {
	CHAN_VOICE,
	{"gonarch/gon_alert1.wav", "gonarch/gon_alert2.wav", "gonarch/gon_alert3.wav"},
	bigMommaPitch,
	"BigMomma.Alert"
};

const NamedSoundScript CBigMomma::painSoundScript = {
	CHAN_VOICE,
	{"gonarch/gon_pain2.wav", "gonarch/gon_pain4.wav", "gonarch/gon_pain5.wav"},
	bigMommaPitch,
	"BigMomma.Pain"
};

const NamedSoundScript CBigMomma::dieSoundScript = {
	CHAN_VOICE,
	{"gonarch/gon_die1.wav"},
	bigMommaPitch,
	"BigMomma.Die"
};

const NamedSoundScript CBigMomma::attackSoundScript = {
	CHAN_VOICE,
	{"gonarch/gon_attack1.wav", "gonarch/gon_attack2.wav", "gonarch/gon_attack3.wav"},
	"BigMomma.Attack"
};

const NamedSoundScript CBigMomma::footstepLeftSoundScript = {
	CHAN_ITEM,
	{"gonarch/gon_step1.wav", "gonarch/gon_step2.wav", "gonarch/gon_step3.wav"},
	bigMommaPitch,
	"BigMomma.FootstepLeft"
};

const NamedSoundScript CBigMomma::footstepRightSoundScript = {
	CHAN_BODY,
	{"gonarch/gon_step1.wav", "gonarch/gon_step2.wav", "gonarch/gon_step3.wav"},
	bigMommaPitch,
	"BigMomma.FootstepRight"
};

const NamedSoundScript CBigMomma::birthSoundScript = {
	CHAN_BODY,
	{"gonarch/gon_birth1.wav", "gonarch/gon_birth2.wav", "gonarch/gon_birth3.wav"},
	bigMommaPitch,
	"BigMomma.Birth"
};

const NamedSoundScript CBigMomma::layHeadcrabSoundScript = {
	CHAN_WEAPON,
	{"gonarch/gon_birth1.wav", "gonarch/gon_birth2.wav", "gonarch/gon_birth3.wav"},
	bigMommaPitch,
	"BigMomma.LayHeadcrab"
};

const NamedSoundScript CBigMomma::childDieSoundScript = {
	CHAN_WEAPON,
	{"gonarch/gon_childdie1.wav", "gonarch/gon_childdie2.wav", "gonarch/gon_childdie3.wav"},
	bigMommaPitch,
	"BigMomma.ChildDie"
};

const NamedSoundScript CBigMomma::sackSoundScript = {
	CHAN_BODY,
	{"gonarch/gon_sack1.wav", "gonarch/gon_sack2.wav", "gonarch/gon_sack3.wav"},
	bigMommaPitch,
	"BigMomma.Sack"
};

const NamedSoundScript CBigMomma::launchMortarSoundScript = {
	CHAN_WEAPON,
	{"gonarch/gon_sack1.wav", "gonarch/gon_sack2.wav", "gonarch/gon_sack3.wav"},
	bigMommaPitch,
	"BigMomma.LaunchMortar"
};

void CBigMomma::KeyValue( KeyValueData *pkvd )
{
#if 0
	if( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		m_volume = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
#endif
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBigMomma::DefaultClassify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBigMomma::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
		ys = 100;
		break;
	default:
		ys = 90;
		break;
	}
	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBigMomma::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BIG_AE_MELEE_ATTACKBR:
		case BIG_AE_MELEE_ATTACKBL:
		case BIG_AE_MELEE_ATTACK1:
		{
			Vector forward, right;

			UTIL_MakeVectorsPrivate( pev->angles, forward, right, NULL );

			Vector center = pev->origin + forward * 128.0f;
			Vector mins = center - Vector( 64.0f, 64.0f, 0.0f );
			Vector maxs = center + Vector( 64.0f, 64.0f, 64.0f );

			CBaseEntity *pList[8];
			int count = UTIL_EntitiesInBox( pList, 8, mins, maxs, FL_MONSTER | FL_CLIENT );
			CBaseEntity *pHurt = NULL;

			for( int i = 0; i < count && !pHurt; i++ )
			{
				if( pList[i] != this )
				{
					if( pList[i]->pev->owner != edict() )
						pHurt = pList[i];
				}
			}

			if( pHurt )
			{
				pHurt->TakeDamage( pev, pev, DamageInfo(gSkillData.bigmommaDmgSlash, DMG_CRUSH | DMG_SLASH) );
				pHurt->pev->punchangle.x = 15.0f;
				switch( pEvent->event )
				{
					case BIG_AE_MELEE_ATTACKBR:
						pHurt->pev->velocity = pHurt->pev->velocity + ( forward * 150.0f ) + Vector( 0.0f, 0.0f, 250.0f ) - ( right * 200.0f );
						break;
					case BIG_AE_MELEE_ATTACKBL:
						pHurt->pev->velocity = pHurt->pev->velocity + ( forward * 150.0f ) + Vector( 0.0f, 0.0f, 250.0f ) + ( right * 200.0f );
						break;
					case BIG_AE_MELEE_ATTACK1:
						pHurt->pev->velocity = pHurt->pev->velocity + ( forward * 220.0f ) + Vector( 0.0f, 0.0f, 200.0f );
						break;
				}

				pHurt->pev->flags &= ~FL_ONGROUND;
				EmitSoundScript(attackHitSoundScript);
			}
		}
		break;
		case BIG_AE_SCREAM:
			EmitSoundScript(alertSoundScript);
			break;
		case BIG_AE_PAIN_SOUND:
			EmitSoundScript(painSoundScript);
			break;
		case BIG_AE_ATTACK_SOUND:
			EmitSoundScript(attackSoundScript);
			break;
		case BIG_AE_BIRTH_SOUND:
			EmitSoundScript(birthSoundScript);
			break;
		case BIG_AE_SACK:
			if( RANDOM_LONG( 0, 100 ) < 30 )
				EmitSoundScript(sackSoundScript);
			break;
		case BIG_AE_DEATHSOUND:
			EmitSoundScript(dieSoundScript);
			break;
		case BIG_AE_STEP1:		// Footstep left
		case BIG_AE_STEP3:		// Footstep back left
			EmitSoundScript(footstepLeftSoundScript);
			break;
		case BIG_AE_STEP4:		// Footstep back right
		case BIG_AE_STEP2:		// Footstep right
			EmitSoundScript(footstepRightSoundScript);
			break;
		case BIG_AE_MORTAR_ATTACK1:
			LaunchMortar();
			break;
		case BIG_AE_LAY_CRAB:
			LayHeadcrab();
			break;
		case BIG_AE_JUMP_FORWARD:
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin( pev, pev->origin + Vector( 0.0f, 0.0f, 1.0f ) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors( pev->angles );

			pev->velocity = gpGlobals->v_forward * 200.0f + gpGlobals->v_up * 500.0f;
			break;
		case BIG_AE_EARLY_TARGET:
			{
				CBaseEntity *pTarget = m_hTargetEnt;
				if( pTarget && pTarget->pev->message )
					FireTargets( STRING( pTarget->pev->message ), this, this );
				Remember( bits_MEMORY_FIRED_NODE );
			}
			break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

DamageInfo CBigMomma::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	DamageInfo damageInfo = inputDamageInfo;

	if( ptr->iHitgroup != 1 )
	{
		// didn't hit the sack?
		if( pev->dmgtime != gpGlobals->time || ( RANDOM_LONG( 0, 10 ) < 1 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1.0f, 2.0f ) );
			pev->dmgtime = gpGlobals->time;
		}

		damageInfo.damage = 0.1f;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}

	return damageInfo;
}

void CBigMomma::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo, Vector vecDir, TraceResult *ptr )
{
	// TODO: how to adjust this for custom trace attack rules?
	if (ptr->iHitgroup == 1 && !HasMemory(bits_MEMORY_KILLED) && gpGlobals->time > m_painSoundTime)
	{
		m_painSoundTime = gpGlobals->time + RANDOM_LONG( 1, 3 );
		EmitSoundScript(painSoundScript);
	}

	CBaseMonster::TraceAttack( pevInflictor, pevAttacker, damageInfo, vecDir, ptr );
}

DamageInfo CBigMomma::DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo)
{
	// Don't take ally acid damage -- BigMomma's mortar is acid
	DamageInfo damageInfo = inputDamageInfo;

	if( (damageInfo.type & DMG_ACID) && pevAttacker )
	{
		CBaseEntity* pAttacker = Instance( pevAttacker );
		if (pAttacker == this)
		{
			damageInfo.damage = 0;
		}
		else if (pAttacker)
		{
			const int rel = IRelationship( pAttacker );
			if (rel < R_DL && rel != R_FR)
				damageInfo.damage = 0.0f;
		}
	}

	return damageInfo;
}

void CBigMomma::BeforeApplyDamageToHealth(float flDamage)
{
	if( !HasMemory( bits_MEMORY_PATH_FINISHED ) )
	{
		if( pev->health <= flDamage )
		{
			pev->health = flDamage + 1;
			Remember( bits_MEMORY_ADVANCE_NODE | bits_MEMORY_COMPLETED_NODE );
			ALERT( at_aiconsole, "BM: Finished node health!!!\n" );
		}
	}
}

void CBigMomma::LayHeadcrab( void )
{
	CBaseEntity *pChild = CBaseEntity::Create( BIG_CHILDCLASS, pev->origin, pev->angles, edict() );
	CBaseMonster *pNewMonster = pChild->MyMonsterPointer();
	if (pNewMonster) {
		pNewMonster->m_iClass = m_iClass;
		pNewMonster->m_reverseRelationship = m_reverseRelationship;
	}

	pChild->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;

	if (FBitSet(pev->spawnflags, SF_MONSTERCLIP_BABYCRABS))
		pChild->pev->spawnflags |= SF_MONSTER_HITMONSTERCLIP;

	// Is this the second crab in a pair?
	if( HasMemory( bits_MEMORY_CHILDPAIR ) )
	{
		m_crabTime = gpGlobals->time + RANDOM_FLOAT( 5.0f, 10.0f );
		Forget( bits_MEMORY_CHILDPAIR );
	}
	else
	{
		m_crabTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 2.5f );
		Remember( bits_MEMORY_CHILDPAIR );
	}

	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector( 0.0f, 0.0f, 100.0f ), ignore_monsters, edict(), &tr );
	UTIL_DecalTrace( &tr, DECAL_MOMMABIRTH );

	EmitSoundScript(layHeadcrabSoundScript);
	m_crabCount++;
}

void CBigMomma::DeathNotice( entvars_t *pevChild )
{
	if( m_crabCount > 0 )		// Some babies may cross a transition, but we reset the count then
		m_crabCount--;
	if( IsFullyAlive() )
	{
		// Make the "my baby's dead" noise!
		EmitSoundScript(childDieSoundScript);
	}
}

void CBigMomma::LaunchMortar( void )
{
	m_mortarTime = gpGlobals->time + RANDOM_FLOAT( 2.0f, 15.0f );

	Vector startPos = pev->origin;
	startPos.z += 180.0f;

	Vector vecLaunch = g_vecZero;

	if (m_pCine) // is a scripted_action making me shoot?
	{
		if (m_hTargetEnt != 0) // don't check m_fTurnType- bigmomma can fire in any direction.
		{
			vecLaunch = VecCheckSplatToss( pev, startPos, m_hTargetEnt->pev->origin, RANDOM_FLOAT( 150, 500 ) );
		}
		if (vecLaunch == g_vecZero)
		{
			vecLaunch = pev->movedir;
		}
	}
	else
	{
		vecLaunch = pev->movedir;
	}

	EmitSoundScript(launchMortarSoundScript);
	CBMortar *pBomb = CBMortar::Shoot( edict(), startPos, vecLaunch, GetProjectileOverrides() );
	pBomb->pev->gravity = 1.0f;
	MortarSpray( startPos, Vector( 0.0f, 0.0f, 1.0f ), GetVisual(CBMortar::mortarSprayVisual), 24 );
}

//=========================================================
// Spawn
//=========================================================
void CBigMomma::Spawn()
{
	Precache();

	SetMyModel( "models/big_mom.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( 150.0f * gSkillData.bigmommaHealthFactor );
	pev->view_ofs = Vector( 0.0f, 0.0f, 128.0f );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(0.3f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	SetMyCanOpenDoors(false);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBigMomma::Precache()
{
	PrecacheMyModel( "models/big_mom.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(footstepLeftSoundScript);
	RegisterAndPrecacheSoundScript(footstepRightSoundScript);
	RegisterAndPrecacheSoundScript(birthSoundScript);
	RegisterAndPrecacheSoundScript(sackSoundScript);
	RegisterAndPrecacheSoundScript(layHeadcrabSoundScript);
	RegisterAndPrecacheSoundScript(childDieSoundScript);
	RegisterAndPrecacheSoundScript(launchMortarSoundScript);

	UTIL_PrecacheOther(BIG_CHILDCLASS);
	UTIL_PrecacheOther("bmortar", GetProjectileOverrides());

	RegisterVisual(CBMortar::mortarSprayVisual);// client side spittle.
}

void CBigMomma::Activate( void )
{
	if( m_hTargetEnt == 0 )
		Remember( bits_MEMORY_ADVANCE_NODE );	// Start 'er up
	CBaseMonster::Activate();
}

void CBigMomma::NodeStart( string_t iszNextNode )
{
	pev->netname = iszNextNode;

	CBaseEntity *pTarget = NULL;

	if( pev->netname )
	{
		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->netname ) );

		if( !FNullEnt( pentTarget ) )
			pTarget = Instance( pentTarget );
	}

	if( !pTarget )
	{
		ALERT( at_aiconsole, "BM: Finished the path!!\n" );
		Remember( bits_MEMORY_PATH_FINISHED );
		return;
	}
	Remember( bits_MEMORY_ON_PATH );
	m_hTargetEnt = pTarget;
}

void CBigMomma::NodeReach( void )
{
	CBaseEntity *pTarget = m_hTargetEnt;

	Forget( bits_MEMORY_ADVANCE_NODE );

	if( !pTarget )
		return;

	if( pTarget->pev->health > 0.0f )
		pev->max_health = pev->health = pTarget->pev->health * gSkillData.bigmommaHealthFactor;

	if( !HasMemory( bits_MEMORY_FIRED_NODE ) )
	{
		if( pTarget->pev->message )
			FireTargets( STRING( pTarget->pev->message ), this, this );
	}
	Forget( bits_MEMORY_FIRED_NODE );

	pev->netname = pTarget->pev->target;

	if (g_modFeatures.bigmomma_lastnode_fix && FStringNull(pev->netname))
	{
		ALERT( at_aiconsole, "BM: Finished the path!!\n" );
		Remember( bits_MEMORY_PATH_FINISHED );
		return;
	}

	if( pTarget->pev->health <= 0.0f )
		Remember( bits_MEMORY_ADVANCE_NODE );	// Move on if no health at this node
}

// Slash
bool CBigMomma::CheckMeleeAttack1( float flDot, float flDist )
{
	CheckMeleeAttackParams params;
	params.distance = BIG_ATTACKDIST;
	return CheckMeleeAttackImpl(flDot, flDist, params, false);
}

// Lay a crab
bool CBigMomma::CheckMeleeAttack2( float flDot, float flDist )
{
	return CanLayCrab();
}

// Mortar launch
bool CBigMomma::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= BIG_MORTARDIST && m_mortarTime < gpGlobals->time )
	{
		CBaseEntity *pEnemy = m_hEnemy;

		if( pEnemy )
		{
			Vector startPos = pev->origin;
			startPos.z += 180.0f;
			pev->movedir = VecCheckSplatToss( pev, startPos, pEnemy->BodyTarget( pev->origin ), RANDOM_FLOAT( 150.0f, 500.0f ) );
			if( pev->movedir != g_vecZero )
				return true;
		}
	}
	return false;
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
enum
{
	SCHED_BIG_NODE = LAST_COMMON_SCHEDULE + 1,
	SCHED_NODE_FAIL
};

enum
{
	TASK_MOVE_TO_NODE_RANGE = LAST_COMMON_TASK + 1,	// Move within node range
	TASK_FIND_NODE,									// Find my next node
	TASK_PLAY_NODE_PRESEQUENCE,						// Play node pre-script
	TASK_PLAY_NODE_SEQUENCE,						// Play node script
	TASK_PROCESS_NODE,								// Fire targets, etc.
	TASK_WAIT_NODE,									// Wait at the node
	TASK_NODE_DELAY,								// Delay walking toward node for a bit. You've failed to get there
	TASK_NODE_YAW									// Get the best facing direction for this node
};

Task_t tlBigNode[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_NODE_FAIL },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_NODE, (float)0 },	// Find my next node
	{ TASK_PLAY_NODE_PRESEQUENCE, (float)0 },	// Play the pre-approach sequence if any
	{ TASK_MOVE_TO_NODE_RANGE, (float)0 },	// Move within node range
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_NODE_YAW, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WAIT_NODE, (float)0 },	// Wait for node delay
	{ TASK_PLAY_NODE_SEQUENCE, (float)0 },	// Play the sequence if one exists
	{ TASK_PROCESS_NODE, (float)0 },	// Fire targets, etc.
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slBigNode[] =
{
	{
		tlBigNode,
		ARRAYSIZE( tlBigNode ),
		0,
		0,
		"Big Node"
	},
};

Task_t tlNodeFail[] =
{
	{ TASK_NODE_DELAY, (float)10 },	// Try to do something else for 10 seconds
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slNodeFail[] =
{
	{
		tlNodeFail,
		ARRAYSIZE( tlNodeFail ),
		0,
		0,
		"NodeFail"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBigMomma )
{
	slBigNode,
	slNodeFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBigMomma, CBaseMonster )

Schedule_t *CBigMomma::GetScheduleOfType( int Type )
{
	switch( Type )
	{
		case SCHED_BIG_NODE:
			return slBigNode;
			break;
		case SCHED_NODE_FAIL:
			return slNodeFail;
			break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

bool CBigMomma::ShouldGoToNode( void )
{
	bool shouldAdvance = HasMemory( bits_MEMORY_ADVANCE_NODE );
	if (g_modFeatures.bigmomma_lastnode_fix)
		shouldAdvance = shouldAdvance && !FStringNull(pev->netname);
	if( shouldAdvance )
	{
		if( m_nodeTime < gpGlobals->time )
			return true;
	}
	return false;
}

Schedule_t *CBigMomma::GetSchedule( void )
{
	if( ShouldGoToNode() )
	{
		return GetScheduleOfType( SCHED_BIG_NODE );
	}

	return CBaseMonster::GetSchedule();
}

void CBigMomma::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FIND_NODE:
		{
			CBaseEntity *pTarget = GetTargetInfoBM();
			if( !HasMemory( bits_MEMORY_ADVANCE_NODE ) )
			{
				if( pTarget )
					pev->netname = pTarget->pev->target;
			}
			NodeStart( pev->netname );
			TaskComplete();
			ALERT( at_aiconsole, "BM: Found node '%s'\n", STRING( pev->netname ) );
		}
		break;
	case TASK_NODE_DELAY:
		m_nodeTime = gpGlobals->time + pTask->flData;
		TaskComplete();
		ALERT( at_aiconsole, "BM: FAIL! Delay %.2f\n", (double)pTask->flData );
		break;
	case TASK_PROCESS_NODE:
		ALERT( at_aiconsole, "BM: Reached node '%s'\n", STRING( pev->netname ) );
		NodeReach();
		TaskComplete();
		break;
	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:
		{
			string_t sequenceName;
			if( pTask->iTask == TASK_PLAY_NODE_SEQUENCE )
				sequenceName = GetNodeSequence();
			else
				sequenceName = GetNodePresequence();

			if( sequenceName )
			{
				int sequence = LookupSequence( STRING( sequenceName ) );
				if( sequence != -1 )
				{
					pev->sequence = sequence;
					pev->frame = 0;
					ResetSequenceInfo();
					ALERT( at_aiconsole, "BM: Playing node %s '%s'\n", pTask->iTask == TASK_PLAY_NODE_PRESEQUENCE ? "presequence" : "sequence", STRING( sequenceName ) );
					return;
				}
				else
				{
					ALERT( at_aiconsole, "BM: Couldn't play node sequence '%s' - the sequence is missing from the model\n", STRING( sequenceName ) );
				}
			}
			TaskComplete();
		}
		break;
	case TASK_NODE_YAW:
		pev->ideal_yaw = GetNodeYaw();
		TaskComplete();
		break;
	case TASK_WAIT_NODE:
	{
		CInfoBM* pTargetNode = GetTargetInfoBM();
		if (pTargetNode && pTargetNode->pev->spawnflags & SF_INFOBM_FORGET_ENEMIES)
		{
			ALERT(at_aiconsole, "BM: Forgets about enemies\n", STRING(pev->classname));
			m_hEnemy = NULL;
			for (EHANDLE& oldEnemy : m_hOldEnemy)
			{
				oldEnemy = 0;
			}
			SetState( MONSTERSTATE_ALERT );
		}

		{
			const int layCrabs = GetNodeLayCrabs();
			switch (layCrabs) {
			case LAYCRABS_OFF:
				SetBits(pev->spawnflags, SF_BIGMOM_NOBABYCRABS);
				break;
			case LAYCRABS_ON:
				ClearBits(pev->spawnflags, SF_BIGMOM_NOBABYCRABS);
				break;
			case LAYCRABS_TOGGLE:
			{
				if (FBitSet(pev->spawnflags, SF_BIGMOM_NOBABYCRABS))
				{
					ClearBits(pev->spawnflags, SF_BIGMOM_NOBABYCRABS);
				}
				else
				{
					SetBits(pev->spawnflags, SF_BIGMOM_NOBABYCRABS);
				}
			}
				break;
			default:
				break;
			}
		}

		const float delay = GetNodeDelay();
		if (g_modFeatures.bigmomma_wait_fix)
		{
			m_flWaitFinished = gpGlobals->time + delay;
			if (delay > 0)
				Stop();
		}
		else
			m_flWait = gpGlobals->time + delay;
		if( pTargetNode && pTargetNode->pev->spawnflags & SF_INFOBM_WAIT )
			ALERT( at_aiconsole, "BM: Wait at node %s forever\n", STRING( pev->netname ) );
		else
			ALERT( at_aiconsole, "BM: Wait at node %s for %.2f\n", STRING( pev->netname ), delay );
	}
		break;


	case TASK_MOVE_TO_NODE_RANGE:
		{
			CInfoBM *pTarget = GetTargetInfoBM();
			if( !pTarget )
				TaskFail("no target bigmomma node");
			else
			{
				if( ( pTarget->pev->origin - pev->origin ).Length() < GetNodeRange() )
					TaskComplete();
				else
				{
					Activity act = ACT_WALK;
					if( pTarget->pev->spawnflags & SF_INFOBM_RUN )
						act = ACT_RUN;

					m_vecMoveGoal = pTarget->pev->origin;
					if( !MoveToTarget( act, 2 ) )
					{
						TaskFail("failed to reach target bigmomma node");
					}
				}
			}
		}
		ALERT( at_aiconsole, "BM: Moving to node '%s'\n", STRING( pev->netname ) );
		break;
	case TASK_MELEE_ATTACK1:
		// Play an attack sound here
		EmitSoundScript(attackSoundScript);
		CBaseMonster::StartTask( pTask );
		break;
	default: 
		CBaseMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CBigMomma::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_MOVE_TO_NODE_RANGE:
		{
			float distance;

			if( m_hTargetEnt == 0 )
				TaskFail("no target ent");
			else
			{
				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if( (distance < GetNodeRange() ) || MovementIsComplete() )
				{
					ALERT( at_aiconsole, "BM: Reached node!\n" );
					TaskComplete();
					RouteClear();		// Stop moving
				}
			}
		}
		break;
	case TASK_WAIT_NODE:
		{
			CInfoBM* pTargetNode = GetTargetInfoBM();
			if( pTargetNode && ( pTargetNode->pev->spawnflags & SF_INFOBM_WAIT ) )
				return;

			if( gpGlobals->time > m_flWaitFinished )
			{
				TaskComplete();
				ALERT( at_aiconsole, "BM: The WAIT is over!\n" );
			}
		}
		break;
	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:
		if( m_fSequenceFinished )
		{
			m_Activity = ACT_RESET;
			TaskComplete();
		}
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

void CBigMomma::ReportAIState(ALERT_TYPE level)
{
	CBaseMonster::ReportAIState(level);
	if (HasMemory(bits_MEMORY_PATH_FINISHED))
		ALERT(level, "Path finished. ");
	if (!FStringNull(pev->netname))
		ALERT(level, "Target node: %s. ", STRING(pev->netname));
	if (HasMemory(bits_MEMORY_ADVANCE_NODE))
		ALERT(level, "Advance node. ");
	if (HasMemory(bits_MEMORY_COMPLETED_NODE))
		ALERT(level, "Completed node. ");
	if (HasMemory(bits_MEMORY_FIRED_NODE))
		ALERT(level, "Fired node. ");
}

Vector VecCheckSplatToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float maxHeight )
{
	TraceResult tr;
	Vector vecMidPoint;// halfway point between Spot1 and Spot2
	Vector vecApex;// highest point 
	Vector vecScale;
	Vector vecGrenadeVel;
	Vector vecTemp;
	const float flGravity = Q_max( g_psv_gravity->value, 0.1f );

	// calculate the midpoint and apex of the 'triangle'
	vecMidPoint = vecSpot1 + ( vecSpot2 - vecSpot1 ) * 0.5f;
	UTIL_TraceLine( vecMidPoint, vecMidPoint + Vector( 0.0f, 0.0f, maxHeight ), ignore_monsters, ENT( pev ), &tr );
	vecApex = tr.vecEndPos;

	// apex is too low, try another mid point
	if (vecApex.z < vecSpot1.z)
	{
		vecMidPoint = vecSpot1 + ( Vector(vecSpot2.x, vecSpot2.y, 0.0f) - Vector(vecSpot1.x, vecSpot1.y, 0.0f) ) * 0.5f;
		UTIL_TraceLine( vecMidPoint, vecMidPoint + Vector( 0.0f, 0.0f, maxHeight ), ignore_monsters, ENT( pev ), &tr );
		vecApex = tr.vecEndPos;
	}

	UTIL_TraceLine( vecSpot1, vecApex, dont_ignore_monsters, ENT( pev ), &tr );
	if( tr.flFraction != 1.0f )
	{
		// fail!
		return g_vecZero;
	}

	// Don't worry about actually hitting the target, this won't hurt us!

	// How high should the grenade travel (subtract 15 so the grenade doesn't hit the ceiling)?
	const float height = vecApex.z - vecSpot1.z - 15.0f;
	if (height < 0)
	{
		ALERT(at_console, "Got negative height %f on big momma splat\n", height);
		return g_vecZero;
	}
	// How fast does the grenade need to travel to reach that height given gravity?
	const float speed = sqrt( 2.0f * flGravity * height );
	
	// How much time does it take to get to enemy position
	const float deltaY = vecSpot2.y - vecSpot1.y;
	const float d = speed*speed - 2*flGravity*deltaY; // discriminant, a = -flGravity/2, b = speed, c = -deltaY
	const float time = (-speed - sqrt(d)) / (-flGravity); // quadratic equation
	//ALERT(at_console, "speed is %f, d is %f, sqrt(d) is %f, time is %f, gravity is %f\n", speed, d, sqrt(d), time, flGravity);
	vecGrenadeVel = vecSpot2 - vecSpot1;
	vecGrenadeVel.z = 0.0f;
	
	// Travel half the distance to the target in that time (apex is at the midpoint)
	vecGrenadeVel = vecGrenadeVel / time;
	// Speed to offset gravity at the desired height
	vecGrenadeVel.z = speed;

	return vecGrenadeVel;
}

// ---------------------------------
//
// Mortar
//
// ---------------------------------
void MortarSpray( const Vector &position, const Vector &direction, const Visual* visual, int count )
{
	SendSpray(position, direction, visual, count, 130, 80);
}

// UNDONE: right now this is pretty much a copy of the squid spit with minor changes to the way it does damage
void CBMortar::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING( "bmortar" );
	
	pev->solid = SOLID_BBOX;

	const Visual* visual = GetVisual(mortarVisual);

	ApplyVisual(visual);

	pev->frame = 0;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_maxFrame = MODEL_FRAMES( pev->modelindex ) - 1;
	pev->dmgtime = gpGlobals->time + 0.4f;
}

void CBMortar::Precache()
{
	RegisterVisual( mortarVisual );// spit projectile.
	RegisterVisual( mortarSprayVisual );// client side spittle.

	RegisterAndPrecacheSoundScript(spitTouchSoundScript, NPC::spitTouchSoundScript);
	RegisterAndPrecacheSoundScript(spitHitSoundScript, NPC::spitHitSoundScript);
}

void CBMortar::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( gpGlobals->time > pev->dmgtime )
	{
		pev->dmgtime = gpGlobals->time + 0.2f;
		MortarSpray( pev->origin, -pev->velocity.Normalize(), GetVisual(mortarSprayVisual), 3 );
	}
	pev->frame = AnimateWithFramerate(pev->frame, m_maxFrame, pev->framerate);
}

CBMortar *CBMortar::Shoot(edict_t *pOwner, Vector vecStart, Vector vecVelocity, EntityOverrides entityOverrides)
{
	CBMortar *pSpit = GetClassPtr( (CBMortar *)NULL );
	pSpit->AssignEntityOverrides(entityOverrides);
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = pOwner;
	pSpit->SetThink( &CBMortar::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1f;

	return pSpit;
}

void CBMortar::Touch( CBaseEntity *pOther )
{
	TraceResult tr;

	EmitSoundScript(spitTouchSoundScript);
	EmitSoundScript(spitHitSoundScript);

	if( pOther->IsBSPModel() )
	{

		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10.0f, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace( &tr, DECAL_MOMMASPLAT );
	}
	else
	{
		tr.vecEndPos = pev->origin;
		tr.vecPlaneNormal = -1.0f * pev->velocity.Normalize();
	}

	// make some flecks
	MortarSpray( tr.vecEndPos, tr.vecPlaneNormal, GetVisual(mortarSprayVisual), 24 );

	entvars_t *pevOwner = NULL;
	if( pev->owner )
		pevOwner = VARS(pev->owner);

	RadiusDamage( pev->origin, pev, pevOwner, DamageInfo{gSkillData.bigmommaRadiusBlast, DMG_ACID}, gSkillData.bigmommaRadiusBlast, CLASS_NONE );
	UTIL_Remove( this );
}
