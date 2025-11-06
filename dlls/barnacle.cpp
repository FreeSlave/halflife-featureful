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
// barnacle - stationary ceiling mounted 'fishing' monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

#define	BARNACLE_BODY_HEIGHT	44 // how 'tall' the barnacle's model is.
#define BARNACLE_PULL_SPEED		8
#define BARNACLE_KILL_VICTIM_DELAY	5 // how many seconds after pulling prey in to gib them. 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BARNACLE_AE_PUKEGIB	2

class CBarnacle : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void UpdateOnRemove() override;
	bool MustAddToFullPack(unsigned char *pSet) override;
	CBaseEntity *TongueTouchEnt( float *pflLength );
	int DefaultClassify() override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void EXPORT BarnacleThink();
	void EXPORT WaitTillDead();
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	DamageInfo DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& inputDamageInfo) override;
	void PainSound() override;
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int DefaultSizeForGrapple() override { return GRAPPLE_FIXED; }

	float m_flAltitude;
	float m_flCachedLength;	// tongue cached length
	float m_flKillVictimTime;
	int m_cGibs;		// barnacle loads up on gibs each time it kills something.
	bool m_fTongueExtended;
	bool m_fLiftingPrey;
	float m_flTongueAdj;
	CPointEntity* pTip;

	// FIXME: need a custom barnacle model with non-generic hitgroup
	// otherwise we can apply to damage to tongue instead of body
#if BARNACLE_FIX_VISIBILITY
	void SetObjectCollisionBox()
	{
		pev->absmin = pev->origin + Vector( -16.0f, -16.0f, -m_flCachedLength );
		pev->absmax = pev->origin + Vector( 16.0f, 16.0f, 0.0f );
	}
#endif

	static const NamedSoundScript biteSoundScript;
	static const NamedSoundScript chewSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript painSoundScript;
};

LINK_ENTITY_TO_CLASS( monster_barnacle, CBarnacle )

TYPEDESCRIPTION	CBarnacle::m_SaveData[] =
{
	DEFINE_FIELD( CBarnacle, m_flAltitude, FIELD_FLOAT ),
	DEFINE_FIELD( CBarnacle, m_flKillVictimTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacle, m_cGibs, FIELD_INTEGER ),// barnacle loads up on gibs each time it kills something.
	DEFINE_FIELD( CBarnacle, m_fTongueExtended, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle, m_fLiftingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle, m_flTongueAdj, FIELD_FLOAT ),
	DEFINE_FIELD( CBarnacle, m_flCachedLength, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBarnacle, CBaseMonster )

const NamedSoundScript CBarnacle::biteSoundScript = {
	CHAN_WEAPON,
	{"barnacle/bcl_bite3.wav"},
	"Barnacle.Bite"
};

const NamedSoundScript CBarnacle::chewSoundScript = {
	CHAN_WEAPON,
	{"barnacle/bcl_chew1.wav", "barnacle/bcl_chew2.wav", "barnacle/bcl_chew3.wav"},
	"Barnacle.Chew"
};

const NamedSoundScript CBarnacle::alertSoundScript = {
	CHAN_WEAPON,
	{"barnacle/bcl_alert2.wav"},
	"Barnacle.Alert"
};

const NamedSoundScript CBarnacle::dieSoundScript = {
	CHAN_WEAPON,
	{"barnacle/bcl_die1.wav", "barnacle/bcl_die3.wav"},
	"Barnacle.Die"
};

const NamedSoundScript CBarnacle::painSoundScript = {
	CHAN_WEAPON,
	{},
	"Barnacle.Pain"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBarnacle::DefaultClassify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarnacle::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNACLE_AE_PUKEGIB:
		CGib::SpawnHumanGibs( pev, 1 );
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarnacle::Spawn()
{
	Precache();

	SetMyModel( "models/barnacle.mdl" );
	SetMySize( Vector( -16.0f, -16.0f, -32.0f ), Vector( 16.0f, 16.0f, 0.0f ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_AIM;
	SetMyBloodColor( BLOOD_COLOR_RED );
	pev->effects = EF_INVLIGHT; // take light from the ceiling 
	SetMyHealth( gSkillData.barnacleHealth );
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flKillVictimTime = 0.0f;
	m_flCachedLength = 32.0f;	// mins.z
	m_cGibs = 0;
	m_fLiftingPrey = false;
	m_flTongueAdj = -100.0f;

	InitBoneControllers();

	SetActivity( ACT_IDLE );

	SetThink( &CBarnacle::BarnacleThink );
	pev->nextthink = gpGlobals->time + 0.5f;

	pev->max_health = pev->health;
	UTIL_SetOrigin( pev, pev->origin );
}

DamageInfo CBarnacle::DefaultTransformDamageInfo(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo)
{
	DamageInfo damageInfo = inputDamageInfo;
	if (damageInfo.type & DMG_CLUB)
	{
		damageInfo.damage = pev->health;
	}
	return damageInfo;
}

void CBarnacle::PainSound()
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
//=========================================================
void CBarnacle::BarnacleThink()
{
	CBaseEntity *pTouchEnt;
	CBaseMonster *pVictim;
	float flLength;
#if BARNACLE_FIX_VISIBILITY
	if( m_flCachedLength != ( m_flAltitude + m_flTongueAdj ) || ( pev->absmin.z != pev->origin.z + -m_flCachedLength ) )
	{
		// recalc collision box here to avoid barnacle disappears bug
		m_flCachedLength = m_flAltitude + m_flTongueAdj;
		UTIL_SetOrigin( pev, pev->origin );
	}
#endif
	pev->nextthink = gpGlobals->time + 0.1f;
	GlowShellUpdate();

	if( m_hEnemy != 0 )
	{
		// barnacle has prey.
		if( !m_hEnemy->IsAlive() )
		{
			// someone (maybe even the barnacle) killed the prey. Reset barnacle.
			m_fLiftingPrey = false;// indicate that we're not lifting prey.
			m_hEnemy = NULL;
			return;
		}

		if( m_fLiftingPrey )
		{
			if( m_hEnemy != 0 && m_hEnemy->pev->deadflag != DEAD_NO )
			{
				// crap, someone killed the prey on the way up.
				m_hEnemy = NULL;
				m_fLiftingPrey = false;
				return;
			}

			// still pulling prey.
			Vector vecNewEnemyOrigin = m_hEnemy->pev->origin;
			vecNewEnemyOrigin.x = pev->origin.x;
			vecNewEnemyOrigin.y = pev->origin.y;

			// guess as to where their neck is
			vecNewEnemyOrigin.x -= 6.0f * cos( m_hEnemy->pev->angles.y * M_PI_F / 180.0f );	
			vecNewEnemyOrigin.y -= 6.0f * sin( m_hEnemy->pev->angles.y * M_PI_F / 180.0f );

			m_flAltitude -= BARNACLE_PULL_SPEED;
			vecNewEnemyOrigin.z += BARNACLE_PULL_SPEED;

			if( fabs( pev->origin.z - ( vecNewEnemyOrigin.z + m_hEnemy->pev->view_ofs.z - 8 ) ) < BARNACLE_BODY_HEIGHT )
			{
				// prey has just been lifted into position ( if the victim origin + eye height + 8 is higher than the bottom of the barnacle, it is assumed that the head is within barnacle's body )
				m_fLiftingPrey = false;

				EmitSoundScript(biteSoundScript);

				pVictim = m_hEnemy->MyMonsterPointer();

				m_flKillVictimTime = gpGlobals->time + 10.0f;// now that the victim is in place, the killing bite will be administered in 10 seconds.

				if( pVictim )
				{
					pVictim->BarnacleVictimBitten( pev );
					SetActivity( ACT_EAT );
				}
			}

			UTIL_SetOrigin( m_hEnemy->pev, vecNewEnemyOrigin );
		}
		else
		{
			// prey is lifted fully into feeding position and is dangling there.
			pVictim = m_hEnemy->MyMonsterPointer();

			if( m_flKillVictimTime != -1.0f && gpGlobals->time > m_flKillVictimTime )
			{
				// kill!
				if( pVictim )
				{
					pVictim->TakeDamage( pev, pev, DamageInfo(pVictim->pev->health, DMG_SLASH).SetGibPolicy(GIB_ALWAYS) );
					m_cGibs = 3;
				}

				return;
			}

			// bite prey every once in a while
			if( pVictim && ( RANDOM_LONG( 0, 49 ) == 0 ) )
			{
				EmitSoundScript(chewSoundScript);
				pVictim->BarnacleVictimBitten( pev );
			}
		}
	}
	else
	{
		// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.
		// If idle and no nearby client, don't think so often
		if( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 1.0f, 1.5f );	// Stagger a bit to keep barnacles from thinking on the same frame

		if( m_fSequenceFinished )
		{
			// this is done so barnacle will fidget.
			SetActivity ( ACT_IDLE );
			m_flTongueAdj = -100;
		}

		if( m_cGibs && RANDOM_LONG( 0, 99 ) == 1 )
		{
			// cough up a gib.
			CGib::SpawnHumanGibs(pev, 1);
			m_cGibs--;

			EmitSoundScript(chewSoundScript);
		}

		pTouchEnt = TongueTouchEnt( &flLength );

		if( pTouchEnt != NULL && m_fTongueExtended )
		{
			// tongue is fully extended, and is touching someone.
			if( pTouchEnt->FBecomeProne() )
			{
				EmitSoundScript(alertSoundScript);

				SetSequenceByName( "attack1" );
				m_flTongueAdj = -20.0f;

				m_hEnemy = pTouchEnt;

				pTouchEnt->pev->movetype = MOVETYPE_FLY;
				pTouchEnt->pev->velocity = g_vecZero;
				pTouchEnt->pev->basevelocity = g_vecZero;
				pTouchEnt->pev->origin.x = pev->origin.x;
				pTouchEnt->pev->origin.y = pev->origin.y;

				m_fLiftingPrey = true;// indicate that we should be lifting prey.
				m_flKillVictimTime = -1;// set this to a bogus time while the victim is lifted.

				m_flAltitude = pev->origin.z - pTouchEnt->EyePosition().z;
			}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it. 
			if( m_flAltitude < flLength )
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += BARNACLE_PULL_SPEED;
				m_fTongueExtended = false;
			}
			else
			{
				m_flAltitude = flLength;
				m_fTongueExtended = true;
			}
		}
	}

	// ALERT( at_console, "tounge %f\n", m_flAltitude + m_flTongueAdj );
	SetBoneController( 0, -( m_flAltitude + m_flTongueAdj ) );
	StudioFrameAdvance( 0.1f );

	if (pTip)
		UTIL_SetOrigin(pTip->pev, pev->origin - Vector(0, 0, m_flAltitude));
}

//=========================================================
// Killed.
//=========================================================
KilledResult CBarnacle::Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	CBaseMonster *pVictim;

	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	if( m_hEnemy != 0 )
	{
		pVictim = m_hEnemy->MyMonsterPointer();

		if( pVictim )
		{
			pVictim->BarnacleVictimReleased();
		}
	}

	//CGib::SpawnRandomGibs( pev, 4, 1 );

	EmitSoundScript(dieSoundScript);

	SetActivity( ACT_DIESIMPLE );
	SetBoneController( 0, 0 );

	StudioFrameAdvance( 0.1f );

	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CBarnacle::WaitTillDead );
	return KilledResult();
}

//=========================================================
//=========================================================
void CBarnacle::WaitTillDead()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	GlowShellUpdate();

	float flInterval = StudioFrameAdvance( 0.1f );
	DispatchAnimEvents( flInterval );

	if( m_fSequenceFinished )
	{
		// death anim finished. 
		StopAnimation();
		SetThink( NULL );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarnacle::Precache()
{
	PrecacheMyModel( "models/barnacle.mdl" );
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(alertSoundScript);//happy, lifting food up
	RegisterAndPrecacheSoundScript(biteSoundScript);//just got food to mouth
	RegisterAndPrecacheSoundScript(chewSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);

	pTip = GetClassPtr((CPointEntity*)nullptr);
	pTip->pev->classname = MAKE_STRING("barnacle_tip");
	SET_MODEL(pTip->edict(), "sprites/iunknown.spr");
	pTip->pev->rendermode = kRenderTransAlpha;
	pTip->pev->renderamt = 0;
	UTIL_SetOrigin(pTip->pev, pev->origin - Vector(0, 0, m_flAltitude));
}

void CBarnacle::UpdateOnRemove()
{
	if (pTip)
	{
		UTIL_Remove(pTip);
		pTip = nullptr;
	}
}

bool CBarnacle::MustAddToFullPack(unsigned char *pSet)
{
	if (pTip)
		return ENGINE_CHECK_VISIBILITY(pTip->edict(), pSet) != 0;
	return CBaseMonster::MustAddToFullPack(pSet);
}

//=========================================================
// TongueTouchEnt - does a trace along the barnacle's tongue
// to see if any entity is touching it. Also stores the length
// of the trace in the int pointer provided.
//=========================================================
#define BARNACLE_CHECK_SPACING	8.0f
CBaseEntity *CBarnacle::TongueTouchEnt( float *pflLength )
{
	TraceResult tr;
	float length;

	// trace once to hit architecture and see if the tongue needs to change position.
	UTIL_TraceLine( pev->origin, pev->origin - Vector ( 0.0f, 0.0f, 2048.0f ), ignore_monsters, ENT( pev ), &tr );
	length = fabs( pev->origin.z - tr.vecEndPos.z );
	if( pflLength )
	{
		*pflLength = length;
	}

	Vector delta = Vector( BARNACLE_CHECK_SPACING, BARNACLE_CHECK_SPACING, 0.0f );
	Vector mins = pev->origin - delta;
	Vector maxs = pev->origin + delta;
	maxs.z = pev->origin.z;
	mins.z -= length;

	CBaseEntity *pList[10];
	int count = UTIL_EntitiesInBox( pList, 10, mins, maxs, ( FL_CLIENT | FL_MONSTER ) );
	if( count )
	{
		for( int i = 0; i < count; i++ )
		{
			// only clients and monsters
			if( pList[i] != this && IRelationship( pList[i] ) > R_NO && pList[ i ]->pev->deadflag == DEAD_NO )	// this ent is one of our enemies. Barnacle tries to eat it.
			{
				return pList[i];
			}
		}
	}

	return NULL;
}
