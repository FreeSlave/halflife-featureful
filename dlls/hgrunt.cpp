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
// hgrunt
//=========================================================

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"animation.h"
#include	"combat.h"
#include	"ggrenade.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"scripted.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"hgrunt.h"
#include	"mod_features.h"
#include	"common_soundscripts.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	GRUNT_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL						0.35		// volume of grunt sounds
#define GRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH				20
#define HGRUNT_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS				2 // how many grunt heads are there? 
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE			15 // must do at least this much damage in one shot to head to score a headshot kill
#define	HGRUNT_SENTENCE_VOLUME				(float)0.35 // volume of grunt sentences

#define HGRUNT_9MMAR				( 1 << 0)
#define HGRUNT_HANDGRENADE			( 1 << 1)
#define HGRUNT_GRENADELAUNCHER			( 1 << 2)
#define HGRUNT_SHOTGUN				( 1 << 3)

#define HEAD_GROUP					1
#define HEAD_GRUNT					0
#define HEAD_COMMANDER					1
#define HEAD_SHOTGUN					2
#define HEAD_M203					3
#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HGRUNT_AE_RELOAD		( 2 )
#define		HGRUNT_AE_KICK			( 3 )
#define		HGRUNT_AE_BURST1		( 4 )
#define		HGRUNT_AE_BURST2		( 5 ) 
#define		HGRUNT_AE_BURST3		( 6 ) 
#define		HGRUNT_AE_GREN_TOSS		( 7 )
#define		HGRUNT_AE_GREN_LAUNCH		( 8 )
#define		HGRUNT_AE_GREN_DROP		( 9 )
#define		HGRUNT_AE_CAUGHT_ENEMY		( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		HGRUNT_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

LINK_ENTITY_TO_CLASS( monster_human_grunt, CHGrunt )

TYPEDESCRIPTION	CHGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CHGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	//DEFINE_FIELD( CHGrunt, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CHGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_voicePitch, FIELD_INTEGER ),
	//DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
	//DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHGrunt, CFollowingMonster )

const char *CHGrunt::pGruntSentences[] =
{
	"HG_GREN", // grenade scared grunt
	"HG_ALERT", // sees player
	"HG_MONST", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
	"HG_CHECK",
	"HG_QUEST",
	"HG_IDLE",
	"HG_CLEAR",
	"HG_ANSWER",
	"HG_HOSTILE",
};

const NamedSoundScript CHGrunt::painSoundScript = {
	CHAN_VOICE,
	{"hgrunt/gr_pain1.wav", "hgrunt/gr_pain2.wav", "hgrunt/gr_pain3.wav", "hgrunt/gr_pain4.wav", "hgrunt/gr_pain5.wav"},
	"HGrunt.Pain"
};

const NamedSoundScript CHGrunt::dieSoundScript = {
	CHAN_VOICE,
	{"hgrunt/gr_die1.wav", "hgrunt/gr_die2.wav", "hgrunt/gr_die3.wav"},
	"HGrunt.Die"
};

const NamedSoundScript CHGrunt::useSoundScript = {
	CHAN_VOICE,
	{"!HG_ANSWER0", "!HG_ANSWER1", "!HG_ANSWER2"},
	HGRUNT_SENTENCE_VOLUME,
	GRUNT_ATTN,
	"HGrunt.Use"
};

const NamedSoundScript CHGrunt::unuseSoundScript = {
	CHAN_VOICE,
	{"!HG_ANSWER5", "!HG_QUEST4"},
	HGRUNT_SENTENCE_VOLUME,
	GRUNT_ATTN,
	"HGrunt.UnUse"
};

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CHGrunt::SpeakSentence( void )
{
	if( m_iSentence == HGRUNT_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if( FOkToSpeak() )
	{
		PlayGruntSentence( m_iSentence );
		m_iSentence = HGRUNT_SENT_NONE;
	}
}

bool CHGrunt::PlayGruntSentence(int sentence, int flags)
{
	return PlaySentenceGroup(SentenceByNumber(sentence), flags);
}

bool CHGrunt::PlaySentenceGroup(const char *group, int flags)
{
	if (SENTENCEG_PlayRndSz( ENT(pev), group, SentenceVolume(), SentenceAttn(), flags, m_voicePitch) >= 0)
	{
		JustSpoke();
		return true;
	}
	return false;
}

void CHGrunt::PlaySentenceSoundScript(const char *soundScript)
{
	if (EmitSoundScriptTalk(soundScript))
		JustSpoke();
}

bool CHGrunt::EmitSoundScriptTalk(const char* name)
{
	SoundScriptParamOverride paramOverride;
	paramOverride.OverridePitchRelative(m_voicePitch);
	return EmitSoundScript(name, paramOverride);
}

void CHGrunt::PlayUseSentence()
{
	PlaySentenceSoundScript(useSoundScript);
}

void CHGrunt::PlayUnUseSentence()
{
	PlaySentenceSoundScript(unuseSoundScript);
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CHGrunt::IRelationship( CBaseEntity *pTarget )
{
	if( IDefaultRelationship(pTarget) >= R_DL && (FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) )) )
	{
		return R_NM;
	}

	return CFollowingMonster::IRelationship( pTarget );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CHGrunt::GibMonster( void )
{
	if( GetBodygroup( GUN_GROUP ) != GUN_NONE )
	{
		DropMyItems(true);
	}

	CBaseMonster::GibMonster();
}

CBaseEntity *CHGrunt::DropMyItem(const char* entityName, const Vector& vecGunPos, const Vector& vecGunAngles, bool isGibbed)
{
	CBaseEntity* pGun = DropItem(entityName, vecGunPos, vecGunAngles);
	if (pGun && isGibbed) {
		pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
		pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
	}
	return pGun;
}

void CHGrunt::DropMyItems(bool isGibbed)
{
	if (g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );

		FixupDropItemPosition(vecGunPos);

		if (!isGibbed) {
			SetBodygroup( GUN_GROUP, GUN_NONE );
		}
		if( FBitSet( pev->weapons, HGRUNT_SHOTGUN ) ) {
			DropMyItem( "weapon_shotgun", vecGunPos, vecGunAngles, isGibbed );
		} else if ( FBitSet( pev->weapons, HGRUNT_9MMAR ) ) {
			DropMyItem( "weapon_9mmAR", vecGunPos, vecGunAngles, isGibbed );
		}
		if( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ) ) {
			DropMyItem( "ammo_ARgrenades", isGibbed ? vecGunPos : BodyTarget( pev->origin ), vecGunAngles, isGibbed );
		}
#if FEATURE_MONSTERS_DROP_HANDGRENADES
		if ( FBitSet (pev->weapons, HGRUNT_HANDGRENADE ) ) {
			CBaseEntity* pGrenadeEnt = DropMyItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles, isGibbed );
			if (pGrenadeEnt)
			{
				CBasePlayerWeapon* pGrenadeWeap = pGrenadeEnt->MyWeaponPointer();
				if (pGrenadeWeap)
					pGrenadeWeap->m_iDefaultAmmo = 1;
			}
		}
#endif
	}
	pev->weapons = 0;
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CHGrunt::DefaultISoundMask( void )
{
	return	bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_PLAYER |
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
bool CHGrunt::FOkToSpeak( void )
{
	// if someone else is talking, don't speak
	if( CTalkMonster::SomeoneIsTalking() )
		return false;

	if( pev->spawnflags & SF_MONSTER_GAG )
	{
		if( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return false;
		}
	}

	// if player is not in pvs, don't speak
	//if( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
	//		return false;

	return true;
}

//=========================================================
//=========================================================
void CHGrunt::JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.5f, 2.0f );
	m_iSentence = HGRUNT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHGrunt::PrescheduleThink( void )
{
	if( InSquad() && m_hEnemy != 0 )
	{
		if( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5.0f )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = true;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
bool CHGrunt::FCanCheckAttacks( void )
{
	if( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
bool CHGrunt::CheckMeleeAttack1( float flDot, float flDist )
{
	// Note: this code used to have a check for CLASS_ALIEN_BIOWEAPON and CLASS_PLAYER_BIOWEAPON enemy classes.
	// This code was probably outdated as human grunts don't see hornets as enemies anyway.
	// TODO: remove this override altogether?
	return CSquadMonster::CheckMeleeAttack1(flDot, flDist);
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
bool CHGrunt::CheckRangeAttack1( float flDot, float flDist )
{
	if( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048.0f && flDot >= 0.5f && NoFriendlyFire() )
	{
		TraceResult tr;

		if( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return false;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), ignore_monsters, ignore_glass, ENT( pev ), &tr );

		if( tr.flFraction == 1.0f )
		{
			return true;
		}
	}

	return false;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
bool CHGrunt::CheckRangeAttack2( float flDot, float flDist )
{
	if( !FBitSet( pev->weapons, ( HGRUNT_HANDGRENADE | HGRUNT_GRENADELAUNCHER ) ) )
	{
		return false;
	}
	return CheckRangeAttack2Impl(gSkillData.hgruntGrenadeSpeed, flDot, flDist, FBitSet(pev->weapons, HGRUNT_GRENADELAUNCHER));
}

bool CHGrunt::CheckRangeAttack2Impl( float grenadeSpeed, float flDot, float flDist, bool contact )
{
	// if the grunt isn't moving, it's ok to check.
	if( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if( gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == WL_NotInWater && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if( !contact )
	{
		// find feet
		if( RANDOM_LONG( 0, 1 ) )
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + ( m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin );
		// estimate position
		if( HasConditions( bits_COND_SEE_ENEMY ) )
			vecTarget += ( ( vecTarget - pev->origin).Length() / grenadeSpeed ) * m_hEnemy->pev->velocity;
	}

	// are any of my allies near the intended grenade impact area?
	if( AllyMonsterInRange( vecTarget, 256 ) )
	{
		// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	if( ( vecTarget - pev->origin ).IsLength2DLessThanOrEqual(256.0f) )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	if( !contact )
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = false;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1.0f; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, grenadeSpeed, 0.5 );

		if( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3f; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = false;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1.0f; // one full second.
		}
	}

	return m_fThrowGrenade;
}

int CHGrunt::GetRangeAttack1Sequence()
{
	// grunt is either shooting standing or shooting crouched
	if( FBitSet( pev->weapons, HGRUNT_9MMAR ) )
	{
		if( m_fStanding )
		{
			// get aimable sequence
			return LookupSequence( "standing_mp5" );
		}
		else
		{
			// get crouching shoot
			return LookupSequence( "crouching_mp5" );
		}
	}
	else
	{
		if( m_fStanding )
		{
			// get aimable sequence
			return LookupSequence( "standing_shotgun" );
		}
		else
		{
			// get crouching shoot
			return LookupSequence( "crouching_shotgun" );
		}
	}
}

int CHGrunt::GetRangeAttack2Sequence()
{
	// grunt is going to a secondary long range attack. This may be a thrown
	// grenade or fired grenade, we must determine which and pick proper sequence
	if( pev->weapons & HGRUNT_GRENADELAUNCHER )
	{
		// get launch anim
		return LookupSequence( "launchgrenade" );
	}
	else
	{
		// get toss anim
		return LookupSequence( "throwgrenade" );
	}
}

//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================

DamageInfo CHGrunt::DefaultHandleTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo &inputDamageInfo, Vector vecDir, TraceResult *ptr)
{
	DamageInfo damageInfo = inputDamageInfo;
	// check for helmet shot
	if( ptr->iHitgroup == 11 )
	{
		// make sure we're wearing one
		if( GetBodygroup( HEAD_GROUP ) == HEAD_GRUNT && ( damageInfo.type & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB ) ) )
		{
			// absorb damage
			damageInfo.damage -= 20;
			if( damageInfo.damage <= 0 )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				damageInfo.damage = 0.01f;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	return damageInfo;
}

//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
TakeDamageResult CHGrunt::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, const DamageInfo& damageInfo )
{
	Forget( bits_MEMORY_INCOVER );

	return CFollowingMonster::TakeDamage( pevInflictor, pevAttacker, damageInfo );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHGrunt::SetYawSpeed( void )
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
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
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

void CHGrunt::IdleSound()
{
	if( FOkToSpeak() && ( *GruntQuestionVar() || RANDOM_LONG( 0, 1 ) ) )
	{
		if( !*GruntQuestionVar() )
		{
			// ask question or make statement
			switch( RANDOM_LONG( 0, 2 ) )
			{
			case 0:
				// check in
				if (PlayGruntSentence(HGRUNT_SENT_CHECK))
					*GruntQuestionVar() = 1;
				break;
			case 1:
				// question
				if (PlayGruntSentence(HGRUNT_SENT_QUEST))
					*GruntQuestionVar() = 2;
				break;
			case 2:
				// statement
				PlayGruntSentence(HGRUNT_SENT_IDLE);
				break;
			}
		}
		else
		{
			switch( *GruntQuestionVar() )
			{
			case 1:
				// check in
				PlayGruntSentence(HGRUNT_SENT_CLEAR);
				break;
			case 2:
				// question 
				PlayGruntSentence(HGRUNT_SENT_ANSWER);
				break;
			}
			*GruntQuestionVar() = 0;
		}
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHGrunt::CheckAmmo( void )
{
	if( m_cAmmoLoaded <= 0 )
	{
		SetConditions( bits_COND_NO_AMMO_LOADED );
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHGrunt::DefaultClassify( void )
{
	return CLASS_HUMAN_MILITARY;
}

const char* CHGrunt::ReverseRelationshipModel()
{
	return "models/hgruntf.mdl";
}

//=========================================================
//=========================================================
void CHGrunt::PerformKick(int eventIndex, float damage, float zpunch)
{
	TraceHullAttackParams params;
	params.punchAngle.x = 15;
	params.punchAngle.z = zpunch;
	params.knockForward = 100;
	params.knockUp = 50;
	params.skipAllies = true;
	params.useAimVectors = false;
	params.damageInfo.damage = damage;
	params.damageInfo.type = DMG_CLUB;
	SetTraceHullAttackParamsFromTemplate(eventIndex, params);

	PerformTraceHullAttack(params);
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CHGrunt::GetGunPosition()
{
	if( m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CHGrunt::Shoot( void )
{
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors( pev->angles );

	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	EjectBrass( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5 ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CHGrunt::Shotgun( void )
{
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors( pev->angles );

	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	EjectBrass( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL ); 
	FireBullets( gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_MONSTER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHGrunt::PlayFirstBurstSounds()
{
	// the first round of the three round burst plays the sound and puts a sound in the world sound list.
	EmitSoundScript(burst9mmSoundScript);
}

void CHGrunt::PlayReloadSound()
{
	EmitSoundScript(reloadSoundScript);
}

void CHGrunt::PlayGrenadeLaunchSound()
{
	EmitSoundScript(grenadeLaunchSoundScript);
}

void CHGrunt::PlayShogtunSound()
{
	EmitSoundScript(shotgunSoundScript);
}

void CHGrunt::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
		{
			if( GetBodygroup( GUN_GROUP ) != GUN_NONE )
				DropMyItems(false);
		}
			break;
		case HGRUNT_AE_RELOAD:
			PlayReloadSound();
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions( bits_COND_NO_AMMO_LOADED );
			break;
		case HGRUNT_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector( 0, 0, 32 ), m_vecTossVelocity, 3.5 );
			//LRC - a bit of a hack. Ideally the grunts would work out in advance whether it's ok to throw.
			if (m_pCine)
			{
				Vector vecToss = g_vecZero;
				if (m_hTargetEnt != 0 && m_pCine->PreciseAttack())
				{
					vecToss = VecCheckToss( pev, GetGunPosition(), m_hTargetEnt->pev->origin, 0.5 );
				}
				if (vecToss == g_vecZero)
				{
					vecToss = (gpGlobals->v_forward*0.5+gpGlobals->v_up*0.5).Normalize()*gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootTimed( pev, GetGunPosition(), vecToss, 3.5 );
			}
			else
				CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = false;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
			break;
		case HGRUNT_AE_GREN_LAUNCH:
		{
			PlayGrenadeLaunchSound();
			//LRC: firing due to a script?
			if (m_pCine)
			{
				Vector vecToss;
				if (m_hTargetEnt != 0 && m_pCine->PreciseAttack())
					vecToss = VecCheckThrow( pev, GetGunPosition(), m_hTargetEnt->pev->origin, gSkillData.hgruntGrenadeSpeed, 0.5 );
				else
				{
					// just shoot diagonally up+forwards
					UTIL_MakeVectors(pev->angles);
					vecToss = (gpGlobals->v_forward*0.5 + gpGlobals->v_up*0.5).Normalize() * gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootContact( pev, GetGunPosition(), vecToss );
			}
			else
				CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = false;
			if( g_iSkillLevel == SKILL_HARD )
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2.0f, 5.0f );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6.0f;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
			break;
		case HGRUNT_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
			break;
		case HGRUNT_AE_BURST1:
		{
			if( FBitSet( pev->weapons, HGRUNT_9MMAR ) )
			{
				Shoot();
				PlayFirstBurstSounds();
			}
			else
			{
				Shotgun();
				PlayShogtunSound();
			}

			CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
			break;
		case HGRUNT_AE_BURST2:
		case HGRUNT_AE_BURST3:
			Shoot();
			break;
		case HGRUNT_AE_KICK:
		{
			PerformKick(pEvent->event, gSkillData.hgruntDmgKick);
		}
			break;
		case HGRUNT_AE_CAUGHT_ENEMY:
		{
			if( FOkToSpeak() )
			{
				SpeakCaughtEnemy();
			}
		}
			break;
		default:
			CFollowingMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHGrunt::SpawnHelper(const char* modelName, int health, int bloodColor)
{
	Precache();

	SetMyModel( modelName );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( bloodColor );
	pev->effects		= 0;
	SetMyHealth( health );
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck	= gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence		= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_TURN_HEAD;
	SetMySquadCapabilities(bits_CAP_SQUAD);
	SetMyCanOpenDoors(true);

	m_fEnemyEluded		= false;
	m_fFirstEncounter	= true;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector( 0, 0, 55 );
}

void CHGrunt::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "desired_skin" ) )
	{
		m_desiredSkin = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CFollowingMonster::KeyValue( pkvd );
}

void CHGrunt::Spawn()
{
	SpawnHelper("models/hgrunt.mdl", gSkillData.hgruntHealth);
	if( pev->weapons == 0 )
	{
		// initialize to original values
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;
		// pev->weapons = HGRUNT_SHOTGUN;
		// pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
	}

	if( FBitSet( pev->weapons, HGRUNT_SHOTGUN ) )
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize = 8;
	}
	else
	{
		m_cClipSize = GRUNT_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	if (m_desiredSkin == 1)
	{
		pev->skin = 0;
	}
	else if (m_desiredSkin == 2)
	{
		pev->skin = 1;
	}
	else
	{
		if( RANDOM_LONG( 0, 99 ) < 80 )
			pev->skin = 0;	// light skin
		else
			pev->skin = 1;	// dark skin
	}

	if( FBitSet( pev->weapons, HGRUNT_SHOTGUN ) )
	{
		SetBodygroup( HEAD_GROUP, HEAD_SHOTGUN );
	}
	else if( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ) )
	{
		SetBodygroup( HEAD_GROUP, HEAD_M203 );
		pev->skin = 1; // alway dark skin
	}

	CTalkMonster::g_talkWaitTime = 0;

	FollowingMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHGrunt::PrecacheHelper(const char *modelName)
{
	PrecacheMyModel( modelName );
	PrecacheMyGibModel();
	RegisterAndPrecacheSoundScript(NPC::swishSoundScript);// because we use the basemonster SWIPE animation event
}

void CHGrunt::Precache()
{
	PrecacheHelper("models/hgrunt.mdl");

	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);

	RegisterAndPrecacheSoundScript(reloadSoundScript, NPC::reloadSoundScript);
	RegisterAndPrecacheSoundScript(burst9mmSoundScript, NPC::burst9mmSoundScript);
	RegisterAndPrecacheSoundScript(grenadeLaunchSoundScript, NPC::grenadeLaunchSoundScript);
	RegisterAndPrecacheSoundScript(shotgunSoundScript, NPC::shotgunSoundScript);

	RegisterAndPrecacheSoundScript(useSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript);

	// get voice pitch
	if( RANDOM_LONG( 0, 1 ) )
		m_voicePitch = 109 + RANDOM_LONG( 0, 7 );
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell
	m_iShotgunShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );
}

//=========================================================
// start task
//=========================================================
void CHGrunt::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_GRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CFollowingMonster::StartTask( pTask );
		break;
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;
	case TASK_GRUNT_FACE_TOSS_DIR:
		break;
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CFollowingMonster::StartTask( pTask );
		if( pev->movetype == MOVETYPE_FLY )
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;
	default: 
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CHGrunt::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_GRUNT_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	default:
		{
			CFollowingMonster::RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
PainSoundRule CHGrunt::DefaultPainSoundRule()
{
	PainSoundRule rule;
	rule.delay = 1.0f;
	return rule;
}

void CHGrunt::PainSound()
{
#if 0
	if( RANDOM_LONG( 0, 99 ) < 5 )
	{
		// pain sentences are rare
		if( FOkToSpeak() )
		{
			SENTENCEG_PlayRndSz( ENT( pev ), "HG_PAIN", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM );
			JustSpoke();
			return;
		}
	}
#endif
	EmitSoundScript(painSoundScript);
}

//=========================================================
// DeathSound 
//=========================================================
void CHGrunt::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

float CHGrunt::SentenceVolume()
{
	return HGRUNT_SENTENCE_VOLUME;
}

float CHGrunt::SentenceAttn()
{
	return GRUNT_ATTN;
}

const char* CHGrunt::SentenceByNumber(int sentence)
{
	return pGruntSentences[sentence];
}

int* CHGrunt::GruntQuestionVar()
{
	static int g_fGruntQuestion = 0; // true if an idle grunt asked a question. Cleared when someone answers.
	return &g_fGruntQuestion;
}

void CHGrunt::SpeakCaughtEnemy()
{
	if ( m_hEnemy != 0 )
	{
		if ( m_hEnemy->IsPlayer() )
			PlayGruntSentence(HGRUNT_SENT_ALERT);
		else if( m_hEnemy->IsAlienMonster() )
			PlayGruntSentence(HGRUNT_SENT_MONSTER);
		else
		{
			// Try HOSTILE sentense on non-player non-alien enemy
			// Fallback to ALERT if allowed
			const bool result = PlayGruntSentence(HGRUNT_SENT_HOSTILE, SND_DONT_REPORT_MISSING);
			if (!result && !AlertSentenceIsForPlayerOnly())
			{
				PlayGruntSentence(HGRUNT_SENT_ALERT);
			}
		}
	}
}

bool CHGrunt::AlertSentenceIsForPlayerOnly()
{
	return true;
}

Schedule_t* CHGrunt::ScheduleOnRangeAttack1()
{
	if( InSquad() )
	{
		// if the enemy has eluded the squad and a squad member has just located the enemy
		// and the enemy does not see the squad member, issue a call to the squad to waste a
		// little time and give the player a chance to turn.
		if( MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
		{
			MySquadLeader()->m_fEnemyEluded = false;
			return GetScheduleOfType( SCHED_GRUNT_FOUND_ENEMY );
		}
	}

	if( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
	{
		// try to take an available ENGAGE slot
		return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
	}
	else if( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
	{
		// throw a grenade if can and no engage slots are available
		return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
	}
	else
	{
		// hide!
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	}
}

float CHGrunt::LimpHealth()
{
	return HGRUNT_LIMP_HEALTH;
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t tlGruntFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slGruntFail[] =
{
	{
		tlGruntFail,
		ARRAYSIZE( tlGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t tlGruntCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, (float)2 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slGruntCombatFail[] =
{
	{
		tlGruntCombatFail,
		ARRAYSIZE( tlGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, 1.5f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 64.0f },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
};

Schedule_t slGruntVictoryDance[] =
{
	{
		tlGruntVictoryDance,
		ARRAYSIZE( tlGruntVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAR_SOUND |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlGruntEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_GRUNT_ELOF_FAIL },
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_GRUNT_SPEAK_SENTENCE,(float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slGruntEstablishLineOfFire[] =
{
	{
		tlGruntEstablishLineOfFire,
		ARRAYSIZE( tlGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t tlGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL1 },
};

Schedule_t slGruntFoundEnemy[] =
{
	{
		tlGruntFoundEnemy,
		ARRAYSIZE( tlGruntFoundEnemy ),
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t tlGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, (float)1.5 },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_SWEEP },
};

Schedule_t slGruntCombatFace[] =
{
	{
		tlGruntCombatFace1,
		ARRAYSIZE( tlGruntCombatFace1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t tlGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL2 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0},
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

Schedule_t slGruntSignalSuppress[] =
{
	{
		tlGruntSignalSuppress,
		ARRAYSIZE( tlGruntSignalSuppress ),
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_NOFIRE |
		bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t tlGruntSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
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

Schedule_t slGruntSuppress[] =
{
	{
		tlGruntSuppress,
		ARRAYSIZE( tlGruntSuppress ),
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_NOFIRE |
		bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER,
		"Suppress"
	},
};

//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t tlGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, (float)1 },
};

Schedule_t slGruntWaitInCover[] =
{
	{
		tlGruntWaitInCover,
		ARRAYSIZE( tlGruntWaitInCover ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t tlGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_GRUNT_TAKECOVER_FAILED },
	{ TASK_WAIT, (float)0.1	 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_GRUNT_SPEAK_SENTENCE, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t slGruntTakeCover[] =
{
	{ 
		tlGruntTakeCover1,
		ARRAYSIZE ( tlGruntTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)99 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, (float)384 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_SPECIAL_ATTACK1 },
	{ TASK_CLEAR_MOVE_WAIT, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_WAIT_FACE_ENEMY },
};

Schedule_t slGruntGrenadeCover[] =
{
	{
		tlGruntGrenadeCover1,
		ARRAYSIZE( tlGruntGrenadeCover1 ),
		0,
		0,
		"GrenadeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK2, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY },
};

Schedule_t slGruntTossGrenadeCover[] =
{
	{
		tlGruntTossGrenadeCover1,
		ARRAYSIZE( tlGruntTossGrenadeCover1 ),
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t tlGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER },// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, (float)179 },
};

Schedule_t slGruntTakeCoverFromBestSound[] =
{
	{
		tlGruntTakeCoverFromBestSound,
		ARRAYSIZE( tlGruntTakeCoverFromBestSound ),
		0,
		0,
		"GruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlGruntHideReload[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RELOAD },
};

Schedule_t slGruntHideReload[] =
{
	{
		tlGruntHideReload,
		ARRAYSIZE( tlGruntHideReload ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t tlGruntSweep[] =
{
	{ TASK_TURN_LEFT, (float)179 },
	{ TASK_WAIT, (float)1 },
	{ TASK_TURN_LEFT, (float)179 },
	{ TASK_WAIT, (float)1 },
};

Schedule_t slGruntSweep[] =
{
	{
		tlGruntSweep,
		ARRAYSIZE( tlGruntSweep ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_WORLD |// sound flags
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER,
		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlGruntRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
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

Schedule_t slGruntRangeAttack1A[] =
{
	{
		tlGruntRangeAttack1A,
		ARRAYSIZE( tlGruntRangeAttack1A ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND |
		bits_COND_NOFIRE |
		bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_IDLE_ANGRY },
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

Schedule_t slGruntRangeAttack1B[] =
{
	{
		tlGruntRangeAttack1B,
		ARRAYSIZE( tlGruntRangeAttack1B ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlGruntRangeAttack2[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_GRUNT_FACE_TOSS_DIR, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2 },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_WAIT_FACE_ENEMY },// don't run immediately after throwing grenade.
};

Schedule_t slGruntRangeAttack2[] =
{
	{
		tlGruntRangeAttack2,
		ARRAYSIZE( tlGruntRangeAttack2 ),
		0,
		0,
		"RangeAttack2"
	},
};

//=========================================================
// repel 
//=========================================================
Task_t tlGruntRepel[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_GLIDE },
};

Schedule_t	slGruntRepel[] =
{
	{
		tlGruntRepel,
		ARRAYSIZE( tlGruntRepel ),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER, 
		"Repel"
	},
};

//=========================================================
// repel 
//=========================================================
Task_t tlGruntRepelAttack[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_FLY },
};

Schedule_t slGruntRepelAttack[] =
{
	{
		tlGruntRepelAttack,
		ARRAYSIZE( tlGruntRepelAttack ),
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t tlGruntRepelLand[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_LAND },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slGruntRepelLand[] =
{
	{
		tlGruntRepelLand,
		ARRAYSIZE( tlGruntRepelLand ),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};


DEFINE_CUSTOM_SCHEDULES( CHGrunt )
{
	slGruntFail,
	slGruntCombatFail,
	slGruntVictoryDance,
	slGruntEstablishLineOfFire,
	slGruntFoundEnemy,
	slGruntCombatFace,
	slGruntSignalSuppress,
	slGruntSuppress,
	slGruntWaitInCover,
	slGruntTakeCover,
	slGruntGrenadeCover,
	slGruntTossGrenadeCover,
	slGruntTakeCoverFromBestSound,
	slGruntHideReload,
	slGruntSweep,
	slGruntRangeAttack1A,
	slGruntRangeAttack1B,
	slGruntRangeAttack2,
	slGruntRepel,
	slGruntRepelAttack,
	slGruntRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHGrunt, CFollowingMonster )

//=========================================================
// SetActivity 
//=========================================================
int CHGrunt::LookupActivity(int activity)
{
	switch( activity )
	{
	case ACT_RANGE_ATTACK1:
		return GetRangeAttack1Sequence();
	case ACT_RANGE_ATTACK2:
		return GetRangeAttack2Sequence();
	case ACT_RUN:
		if( pev->health <= LimpHealth() )
		{
			// limp!
			return CFollowingMonster::LookupActivity( ACT_RUN_HURT );
		}
		else
		{
			return CFollowingMonster::LookupActivity( activity );
		}
	case ACT_WALK:
		if( pev->health <= LimpHealth() )
		{
			// limp!
			return CFollowingMonster::LookupActivity( ACT_WALK_HURT );
		}
		else
		{
			return CFollowingMonster::LookupActivity( activity );
		}
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			return CFollowingMonster::LookupActivity( ACT_IDLE_ANGRY );
		}
		// pass through
	default:
		return CFollowingMonster::LookupActivity( activity );
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CHGrunt::GetSchedule( void )
{

	// clear old sentence
	m_iSentence = HGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if( pev->flags & FL_ONGROUND )
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_GRUNT_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType( SCHED_GRUNT_REPEL_ATTACK );
			else
				return GetScheduleOfType( SCHED_GRUNT_REPEL );
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound )
		{
			if( pSound->m_iType & bits_SOUND_DANGER )
			{
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.
				if( FOkToSpeak() )
				{
					PlayGruntSentence(HGRUNT_SENT_GREN);
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if( !HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & ( bits_SOUND_PLAYER | bits_SOUND_COMBAT ) ) )
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
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

			// new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				if( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = false;

					if( !IsLeader() )
					{
						return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight"
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if( FOkToSpeak() )// && RANDOM_LONG( 0, 1 ) )
						{
							SpeakCaughtEnemy();
						}

						if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType( SCHED_GRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
			// no ammo
			else if( HasConditions( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType( SCHED_GRUNT_COVER_AND_RELOAD );
			}
			// damaged just a little
			else if( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG( 0, 99 );

				if( iPercent <= 90 && m_hEnemy != 0 )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if( FOkToSpeak() ) // && RANDOM_LONG( 0, 1 ) )
					{
						m_iSentence = HGRUNT_SENT_COVER;
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
			// can kick
			else if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}
			// can grenade launch
			else if( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER) && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			// can shoot
			else if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return ScheduleOnRangeAttack1();
			}
			// can't see enemy
			else if( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if( FOkToSpeak() )
					{
						PlayGruntSentence(HGRUNT_SENT_THROW);
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if( FOkToSpeak() )// && RANDOM_LONG( 0, 1 ) )
					{
						m_iSentence = HGRUNT_SENT_CHARGE;
					}

					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if( FOkToSpeak() && RANDOM_LONG( 0, 1 ) )
					{
						PlayGruntSentence(HGRUNT_SENT_TAUNT);
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			if( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
		break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
	{
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
		break;
	}
	default:
		break;
	}

	// no special cases here, call the base class
	return CFollowingMonster::GetSchedule();
}

bool CHGrunt::CanDropGrenade() const
{
	return FBitSet(pev->weapons, HGRUNT_HANDGRENADE);
}

//=========================================================
//=========================================================
Schedule_t *CHGrunt::GetScheduleOfType( int Type ) 
{
	switch( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if( InSquad() )
			{
				if( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					if( FOkToSpeak() )
					{
						PlayGruntSentence(HGRUNT_SENT_THROW);
					}
					return slGruntTossGrenadeCover;
				}
				else
				{
					return &slGruntTakeCover[0];
				}
			}
			else
			{
				if (CanDropGrenade())
				{
					if( RANDOM_LONG( 0, 1 ) )
					{
						return &slGruntTakeCover[0];
					}
					else
					{
						return &slGruntGrenadeCover[0];
					}
				}
				else
					return &slGruntTakeCover[0];
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slGruntTakeCoverFromBestSound[0];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType( SCHED_FAIL );
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slGruntEstablishLineOfFire[0];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if( RANDOM_LONG( 0, 9 ) == 0 )
				m_fStanding = RANDOM_LONG( 0, 1 ) ? true : false;

			if( m_fStanding )
				return &slGruntRangeAttack1B[0];
			else
				return &slGruntRangeAttack1A[0];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slGruntRangeAttack2[0];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slGruntCombatFace[0];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slGruntWaitInCover[0];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slGruntSweep[0];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slGruntHideReload[0];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slGruntFoundEnemy[0];
		}
	case SCHED_VICTORY_DANCE:
		{
			const bool inSquad = InSquad();
			if ( !inSquad || (inSquad && IsLeader()) )
			{
				return &slGruntVictoryDance[ 0 ];
			}
			return GetScheduleOfType(SCHED_IDLE_STAND);
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			if( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = false;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slGruntSignalSuppress[0];
			}
			else
			{
				return &slGruntSuppress[0];
			}
		}
	case SCHED_FAIL:
		{
			if( m_hEnemy != 0 )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slGruntCombatFail[0];
			}

			return &slGruntFail[0];
		}
	case SCHED_GRUNT_REPEL:
		{
			if( pev->velocity.z > -128 )
				pev->velocity.z -= 32;
			return &slGruntRepel[0];
		}
	case SCHED_GRUNT_REPEL_ATTACK:
		{
			if( pev->velocity.z > -128 )
				pev->velocity.z -= 32;
			return &slGruntRepelAttack[0];
		}
	case SCHED_GRUNT_REPEL_LAND:
		{
			return &slGruntRepelLand[0];
		}
	default:
		{
			return CFollowingMonster::GetScheduleOfType( Type );
		}
	}
}

void CHGrunt::ReportAIState(ALERT_TYPE level)
{
	CFollowingMonster::ReportAIState(level);
	ALERT(level, "Ammo loaded: %d / %d. ", m_cAmmoLoaded, m_cClipSize);
}

void CHGrunt::OnBecomingLeader()
{
	if (FClassnameIs( pev, "monster_human_grunt" ))
	{
		SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
		pev->skin = 0;
	}
}

//=========================================================
// CHGruntRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

LINK_ENTITY_TO_CLASS( monster_grunt_repel, CHGruntRepel )

void CHGruntRepel::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;

	SetUse( &CHGruntRepel::RepelUse );
}

const char* CHGruntRepel::TrooperName()
{
	return "monster_human_grunt";
}

void CHGruntRepel::Precache( void )
{
	EntityOverrides entityOverrides;
	entityOverrides.entTemplate = m_entTemplate;
	entityOverrides.model = pev->model;

	UTIL_PrecacheOther( TrooperName(), entityOverrides );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
	if (!FStringNull(m_gibModel))
		PRECACHE_MODEL(STRING(m_gibModel));
}

void CHGruntRepel::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "gruntname" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CFollowingMonster::KeyValue( pkvd );
}

void CHGruntRepel::PrepareBeforeSpawn(CBaseEntity *pEntity)
{

}

void CHGruntRepel::RepelUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0 ), dont_ignore_monsters, ENT( pev ), &tr );
	/*
	if( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP )
		return NULL;
	*/

	CBaseEntity *pEntity = CreateNoSpawn( TrooperName(), pev->origin, pev->angles );
	if (!pEntity) {
		UTIL_Remove( this );
		return;
	}
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer();
	if (!pGrunt) {
		UTIL_Remove( this );
		return;
	}

	const int knownFlags =
			SF_MONSTER_GAG | SF_MONSTER_HITMONSTERCLIP | SF_MONSTER_PRISONER |
			SF_MONSTER_DONT_DROP_GUN | SF_SQUADMONSTER_LEADER | SF_MONSTER_PREDISASTER |
			SF_MONSTER_FADECORPSE | SF_MONSTER_NONSOLID_CORPSE | SF_MONSTER_ACT_OUT_OF_PVS |
			SF_MONSTER_IGNORE_PLAYER_PUSH;
	const int flagsToSet = knownFlags & pev->spawnflags;
	SetBits(pEntity->pev->spawnflags, flagsToSet);

	pEntity->pev->targetname = pev->message;
	pEntity->pev->netname = pev->netname;
	pEntity->pev->weapons = pev->weapons;
	pEntity->pev->health = pev->health;
	pEntity->pev->model = pev->model;
	pEntity->m_entTemplate = m_entTemplate;
	pGrunt->m_iClass = m_iClass;
	pGrunt->m_reverseRelationship = m_reverseRelationship;
	pGrunt->SetMyBloodColor(m_bloodColor);
	pGrunt->SetMyFieldOfView(m_flFieldOfView);
	pGrunt->m_gibModel = m_gibModel;
	pGrunt->m_iszTriggerTarget = m_iszTriggerTarget;
	pGrunt->m_iTriggerCondition = m_iTriggerCondition;
	pGrunt->m_iTriggerAltCondition = m_iTriggerAltCondition;
	pGrunt->m_displayName = m_displayName;
	pGrunt->m_customSoundMask = m_customSoundMask;
	pGrunt->m_prisonerTo = m_prisonerTo;
	pGrunt->m_ignoredBy = m_ignoredBy;
	pGrunt->m_freeRoam = m_freeRoam;
	pGrunt->m_activeAfterCombat = m_activeAfterCombat;
	pGrunt->m_sizeForGrapple = m_sizeForGrapple;
	pGrunt->m_gibPolicy = m_gibPolicy;
	CFollowingMonster* pFollowingMonster = pGrunt->MyFollowingMonsterPointer();
	if (pFollowingMonster)
	{
		pFollowingMonster->m_followFailPolicy = m_followFailPolicy;
		pFollowingMonster->m_followagePolicy = m_followagePolicy;
	}
	PrepareBeforeSpawn(pEntity);
	DispatchSpawn(pEntity->edict());
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pGrunt->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector( 0, 0, 112 ), pGrunt->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( &CBaseEntity::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0f * tr.flFraction / pGrunt->pev->velocity.z + 0.5f;

	UTIL_Remove( this );
}

//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadHGrunt : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/hgrunt.mdl"; }
	int	DefaultClassify() { return	CLASS_HUMAN_MILITARY; }

	const char* getPos(int pos) const;
	static const char *m_szPoses[3];
};

const char *CDeadHGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

const char* CDeadHGrunt::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_hgrunt_dead, CDeadHGrunt )

//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadHGrunt::Spawn()
{
	SpawnHelper();

	// map old bodies onto new bodies
	switch( pev->body )
	{
	case 0: // Grunt with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
		SetBodygroup( GUN_GROUP, GUN_MP5 );
		break;
	case 1: // Commander with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
		SetBodygroup( GUN_GROUP, GUN_MP5 );
		break;
	case 2: // Grunt no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	case 3: // Commander no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	}

	MonsterInitDead();
}
