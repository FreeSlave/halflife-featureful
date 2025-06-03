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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"

#if !CLIENT_DLL
#include "game.h"
#include "gamerules.h"
#endif

enum w_squeak_e
{
	WSQUEAK_IDLE1 = 0,
	WSQUEAK_FIDGET,
	WSQUEAK_JUMP,
	WSQUEAK_RUN
};

#if !CLIENT_DLL
class CSqueakGrenade : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	int DefaultClassify( void );
	void EXPORT SuperBounceTouch( CBaseEntity *pOther );
	void EXPORT HuntThink( void );
	int	BloodColor( void ) { return CBaseMonster::BloodColor(); }
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	void GibMonster( void );

	virtual int Save( CSave &save ); 
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	virtual float AdditionalExplosionDamage();
	virtual float MaximumExplosionDamage();

	virtual int SizeForGrapple() { return GRAPPLE_SMALL; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return Vector( -4.0f, -4.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 4.0f, 4.0f, 8.0f ); }

	static float m_flNextBounceSoundTime;

	// CBaseEntity *m_pTarget;
	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int m_iMyClass;

protected:
	void SpawnImpl(const char* modelName, float damage);
	void PrecacheImpl(const char* modelName);

	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript gibbedSoundScript;
	static const NamedSoundScript squeakSoundScript;
	static const NamedSoundScript deploySoundScript;
	static const NamedSoundScript bounceSoundScript;
};

float CSqueakGrenade::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS( monster_snark, CSqueakGrenade )

TYPEDESCRIPTION	CSqueakGrenade::m_SaveData[] =
{
	DEFINE_FIELD( CSqueakGrenade, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CSqueakGrenade, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CSqueakGrenade, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CSqueakGrenade, m_flNextHit, FIELD_TIME ),
	DEFINE_FIELD( CSqueakGrenade, m_posPrev, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CSqueakGrenade, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CSqueakGrenade, CGrenade )

const NamedSoundScript CSqueakGrenade::dieSoundScript = {
	CHAN_ITEM,
	{"squeek/sqk_blast1.wav"},
	1.0f,
	0.5f,
	"Snark.Die"
};

const NamedSoundScript CSqueakGrenade::gibbedSoundScript = {
	CHAN_VOICE,
	{"common/bodysplat.wav"},
	0.75f,
	ATTN_NORM,
	200,
	"Snark.Gibbed"
};

const NamedSoundScript CSqueakGrenade::squeakSoundScript = {
	CHAN_VOICE,
	{"squeek/sqk_die1.wav"},
	IntRange(100, 163),
	"Snark.Squeak"
};

const NamedSoundScript CSqueakGrenade::deploySoundScript = {
	CHAN_WEAPON,
	{"squeek/sqk_deploy1.wav"},
	"Snark.Deploy"
};

const NamedSoundScript CSqueakGrenade::bounceSoundScript = {
	CHAN_VOICE,
	{"squeek/sqk_hunt1.wav", "squeek/sqk_hunt2.wav", "squeek/sqk_hunt3.wav"},
	"Snark.Bounce"
};

#define SQUEEK_DETONATE_DELAY	15.0f

int CSqueakGrenade::DefaultClassify( void )
{
	return CLASS_SNARK;
}

void CSqueakGrenade::Spawn()
{
	Precache();
	SpawnImpl("models/w_squeak.mdl", gSkillData.snarkDmgPop);
}

void CSqueakGrenade::SpawnImpl(const char* modelName , float damage)
{
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	SetMyBloodColor( BLOOD_COLOR_YELLOW );

	SET_MODEL( ENT( pev ), modelName );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CSqueakGrenade::SuperBounceTouch );
	SetThink( &CSqueakGrenade::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_flNextHunt = gpGlobals->time + (float)1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.snarkHealth;
	pev->gravity = 0.5f;
	pev->friction = 0.5f;

	pev->dmg = damage;

	m_flDie = gpGlobals->time + SQUEEK_DETONATE_DELAY;

	SetMyFieldOfView(0.0f); // 180 degrees

	if( pev->owner )
		m_hOwner = Instance( pev->owner );

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	pev->sequence = WSQUEAK_RUN;
	ResetSequenceInfo();
}

void CSqueakGrenade::Precache()
{
	PrecacheImpl("models/w_squeak.mdl");
}

void CSqueakGrenade::PrecacheImpl( const char* modelName )
{
	PRECACHE_MODEL( modelName );
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(gibbedSoundScript);
	RegisterAndPrecacheSoundScript(squeakSoundScript);
	RegisterAndPrecacheSoundScript(deploySoundScript);
	RegisterAndPrecacheSoundScript(bounceSoundScript);
}

KilledResult CSqueakGrenade::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetThink( &CBaseEntity::SUB_Remove );
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.1f;

	// since squeak grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	// play squeek blast
	EmitSoundScript(dieSoundScript);

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0f );

	UTIL_BloodDrips( pev->origin, g_vecZero, BloodColor(), 80 );

	entvars_t* pevExploAttacker = m_hOwner != 0 ? m_hOwner->pev : pev;
	RadiusDamage( pev, pevExploAttacker, DamageInfo{pev->dmg, DMG_BLAST}, CLASS_NONE );

	// reset owner so death message happens
	if( m_hOwner != 0 )
		pev->owner = m_hOwner->edict();

	return CBaseMonster::Killed( pevInflictor, pevAttacker, GIB_ALWAYS );
}

void CSqueakGrenade::GibMonster( void )
{
	EmitSoundScript(gibbedSoundScript);
}

float CSqueakGrenade::AdditionalExplosionDamage()
{
	return gSkillData.snarkDmgPop;
}

float CSqueakGrenade::MaximumExplosionDamage()
{
	return 0;
}

void CSqueakGrenade::HuntThink( void )
{
	// ALERT( at_console, "think\n" );

	if( !IsInWorld() )
	{
		SetTouch( NULL );
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	// explode when ready
	if( gpGlobals->time >= m_flDie )
	{
		g_vecAttackDir = pev->velocity.Normalize();
		pev->health = -1;
		Killed( pev, pev, 0 );
		return;
	}

	// float
	if( pev->waterlevel != WL_NotInWater )
	{
		if( pev->movetype == MOVETYPE_BOUNCE )
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9f;
		pev->velocity.z += 8.0f;
	}
	else if( pev->movetype == MOVETYPE_FLY )
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if( m_flNextHunt > gpGlobals->time )
		return;

	m_flNextHunt = gpGlobals->time + 2.0f;

	//CBaseEntity *pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat.NormalizeInPlace();

	UTIL_MakeVectors( pev->angles );

	if( m_hEnemy == 0 || !m_hEnemy->IsAlive() )
	{
		// find target, bounce a bit towards it.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy();
	}

	// squeek if it's about time blow up
	if( ( m_flDie - gpGlobals->time <= 0.5f ) && ( m_flDie - gpGlobals->time >= 0.3f ) )
	{
		EmitSoundScript(squeakSoundScript);
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 256, 0.25f );
	}

	// higher pitch as squeeker gets closer to detonation time
	/*float flpitch = 155.0f - 60.0f * ( ( m_flDie - gpGlobals->time ) / SQUEEK_DETONATE_DELAY );
	if( flpitch < 80.0f )
		flpitch = 80.0f;*/

	if( m_hEnemy != 0 )
	{
		if( FVisible( m_hEnemy ) )
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize();
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0f / ( flVel + 10.0f );

		if( flAdj > 1.2f )
			flAdj = 1.2f;
		
		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300.0f;
	}

	if( pev->flags & FL_ONGROUND )
	{
		pev->avelocity = Vector( 0, 0, 0 );
	}
	else
	{
		if( pev->avelocity == Vector( 0, 0, 0 ) )
		{
			pev->avelocity.x = RANDOM_FLOAT( -100, 100 );
			pev->avelocity.z = RANDOM_FLOAT( -100, 100 );
		}
	}

	if( ( pev->origin - m_posPrev ).IsLengthLessThan(1.0f) )
	{
		pev->velocity.x = RANDOM_FLOAT( -100, 100 );
		pev->velocity.y = RANDOM_FLOAT( -100, 100 );
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.z = 0;
	pev->angles.x = 0;
}

void CSqueakGrenade::SuperBounceTouch( CBaseEntity *pOther )
{
	float flpitch;

	TraceResult tr = UTIL_GetGlobalTrace();

	// don't hit the guy that launched this grenade
	if( pev->owner && pOther->edict() == pev->owner )
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0.0f;
	pev->angles.z = 0.0f;

	// avoid bouncing too much
	if( m_flNextHit > gpGlobals->time )
		return;

	// higher pitch as squeeker gets closer to detonation time
	flpitch = 155.0f - 60.0f * ( ( m_flDie - gpGlobals->time ) / SQUEEK_DETONATE_DELAY );

	if( !FBitSet( pOther->pev->flags, FL_WORLDBRUSH )
	    && pOther->pev->takedamage && m_flNextAttack < gpGlobals->time )
	{
		// attack!

		// make sure it's me who has touched them
		if( tr.pHit == pOther->edict() )
		{
			// and it's not another squeakgrenade
			if( tr.pHit->v.modelindex != pev->modelindex )
			{
				// ALERT( at_console, "hit enemy\n" );
				entvars_t* pevAttacker = m_hOwner != 0 ? m_hOwner->pev : pev;
				pOther->ApplyTraceAttack( pev, pevAttacker, DamageInfo{gSkillData.snarkDmgBite, DMG_SLASH}, gpGlobals->v_forward, &tr );

				pev->dmg += AdditionalExplosionDamage(); // add more explosion damage
				if (MaximumExplosionDamage()) {
					pev->dmg = Q_min(pev->dmg, MaximumExplosionDamage());
				}
				// m_flDie += 2.0f; // add more life

				// make bite sound
				SoundScriptParamOverride param;
				param.OverridePitchRelative((int)flpitch);
				EmitSoundScript(deploySoundScript, param);
				m_flNextAttack = gpGlobals->time + 0.5f;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n" );
		}
	}

	m_flNextHit = gpGlobals->time + 0.1f;
	m_flNextHunt = gpGlobals->time;

	if( g_pGameRules->IsMultiplayer() )
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if( gpGlobals->time < m_flNextBounceSoundTime )
		{
			// too soon!
			return;
		}
	}

	if( !( pev->flags & FL_ONGROUND ) )
	{
		// play bounce sound
		SoundScriptParamOverride param;
		param.OverridePitchRelative((int)flpitch);
		EmitSoundScript(bounceSoundScript, param);
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 256, 0.25f );
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 100, 0.1f );
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5f;// half second.
}

#if FEATURE_PENGUIN
class CPenguinGrenade : public CSqueakGrenade
{
	void Spawn( void );
	void Precache( void );
	KilledResult Killed(entvars_t *pevInflictor,entvars_t *pevAttacker, int iGib) override;
	float AdditionalExplosionDamage();
	float MaximumExplosionDamage();
	float ExplosionRadius()
	{
		return Q_min(gSkillData.plrDmgHandGrenade*5, pev->dmg * 2.5);
	}
};

void CPenguinGrenade::Spawn()
{
	Precache();
	SpawnImpl("models/w_penguin.mdl", gSkillData.plrDmgHandGrenade);
}

void CPenguinGrenade::Precache()
{
	PrecacheBaseGrenadeSounds();
	PrecacheImpl("models/w_penguin.mdl");
}

KilledResult CPenguinGrenade::Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib)
{
	if( m_hOwner != 0 )
		pev->owner = m_hOwner->edict();
	CGrenade::Detonate();
	UTIL_BloodDrips( pev->origin, g_vecZero, BloodColor(), 80 );
	if( m_hOwner != 0 )
		pev->owner = m_hOwner->edict();
	return KilledResult();
}

float CPenguinGrenade::AdditionalExplosionDamage()
{
	return gSkillData.plrDmgHandGrenade;
}

float CPenguinGrenade::MaximumExplosionDamage()
{
	return gSkillData.plrDmgHandGrenade*5;
}

LINK_ENTITY_TO_CLASS( monster_penguin, CPenguinGrenade )
#endif

#endif

LINK_WEAPON_TO_CLASS( weapon_snark, CSqueak )

void CSqueak::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), NestModel() );

	FallInit();//get ready to fall down.

	InitDefaultAmmo(DefaultGive());
	InitMaxClip(WEAPON_NOCLIP);

	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;
}

void CSqueak::Precache( void )
{
	PRECACHE_MODEL( NestModel() );
	PRECACHE_MODEL( VModel() );
	PrecachePModel( PModel() );
	PRECACHE_SOUND( "squeek/sqk_hunt2.wav" );
	PRECACHE_SOUND( "squeek/sqk_hunt3.wav" );
	UTIL_PrecacheOther( GrenadeName() );

	m_usSnarkFire = PRECACHE_EVENT( 1, EventsFile() );
}

bool CSqueak::GetItemInfo( ItemInfo *p )
{
	p->pszAmmo1 = AmmoName(DefaultAmmoName());
	p->iSlot = 4;
	p->iPosition = PositionInSlot();
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->pszAmmoEntity = STRING(pev->classname);
	p->iDropAmmo = DefaultGive();

	return true;
}

bool CSqueak::Deploy()
{
	// play hunt sound
	float flRndSound = RANDOM_FLOAT( 0.0f, 1.0f );

	if( flRndSound <= 0.5f )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1, ATTN_NORM, 0, 100 );
	else
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "squeek/sqk_hunt3.wav", 1, ATTN_NORM, 0, 100 );

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	const bool result = DefaultDeploy( VModel(), PModel(), SQUEAK_UP, "squeak" );
	if (result)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.7f;
	}
	return result;
}

void CSqueak::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->ClearWeaponBit(WeaponId());
		DestroyItem();
		return;
	}

	SendWeaponAnim( SQUEAK_DOWN );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM );
}

void CSqueak::PrimaryAttack()
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		TraceResult tr;
		Vector trace_origin, forward;
		float flVel;

		UTIL_MakeVectors( Vector( 0, m_pPlayer->pev->v_angle.y, m_pPlayer->pev->v_angle.z ));
		forward = gpGlobals->v_forward;
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		if( m_pPlayer->pev->v_angle.x <= 0 )
		{
			flVel = 1;
		}
		else
		{
			flVel = m_pPlayer->pev->v_angle.x / 90.0f;
		}

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if( m_pPlayer->pev->flags & FL_DUCKING )
		{
			trace_origin = trace_origin - Vector( 0, 0, 1 ) * ( flVel + 1.0f ) * -18;
		}

		forward = forward * flVel + gpGlobals->v_forward * ( 1 - flVel );

		// find place to toss monster
		UTIL_TraceLine( trace_origin + forward * 24.0f, trace_origin + gpGlobals->v_forward * 60.0f, dont_ignore_monsters, NULL, &tr );

		PLAYBACK_EVENT_FULL( PlaybackFlags(), m_pPlayer->edict(), m_usSnarkFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );

		if( tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0 )
		{
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
#if !CLIENT_DLL
			CBaseEntity *pSqueak = CBaseEntity::Create( GrenadeName(), tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
			pSqueak->pev->velocity = forward * 200.0f + m_pPlayer->pev->velocity;
#endif
			// play hunt sound
			float flRndSound = RANDOM_FLOAT( 0.0f, 1.0f );

			if( flRndSound <= 0.5f )
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1, ATTN_NORM, 0, 105 );
			else 
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "squeek/sqk_hunt3.wav", 1, ATTN_NORM, 0, 105 );

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = GetNextAttackDelay( 0.3f );
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		}
	}
}

void CSqueak::SecondaryAttack( void )
{

}

void CSqueak::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fJustThrown )
	{
		m_fJustThrown = 0;

		if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim( SQUEAK_UP );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if( flRand <= 0.75f )
	{
		iAnim = SQUEAK_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0f / 16.0f * 2.0f;
	}
	else if( flRand <= 0.875f )
	{
		iAnim = SQUEAK_FIDGETFIT;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0f / 16.0f;
	}
	else
	{
		iAnim = SQUEAK_FIDGETNIP;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0f / 16.0f;
	}
	SendWeaponAnim( iAnim );
}

const char* CSqueak::GrenadeName() const
{
	return "monster_snark";
}

const char* CSqueak::NestModel() const
{
	return "models/w_sqknest.mdl";
}

const char* CSqueak::PModel() const
{
	return "models/p_squeak.mdl";
}

const char* CSqueak::VModel() const
{
	return "models/v_squeak.mdl";
}

int CSqueak::PositionInSlot() const
{
	return 3;
}

int CSqueak::DefaultGive() const
{
	return SNARK_DEFAULT_GIVE;
}

const char* CSqueak::DefaultAmmoName() const
{
	return "Snarks";
}

const char* CSqueak::EventsFile() const
{
	return "events/snarkfire.sc";
}

#if FEATURE_PENGUIN
LINK_WEAPON_TO_CLASS( weapon_penguin, CPenguin )

const char* CPenguin::GrenadeName() const
{
	return "monster_penguin";
}

const char* CPenguin::NestModel() const
{
	return "models/w_penguinnest.mdl";
}

const char* CPenguin::PModel() const
{
	return "models/p_penguin.mdl";
}

const char* CPenguin::VModel() const
{
	return "models/v_penguin.mdl";
}

int CPenguin::PositionInSlot() const
{
	return 4;
}

int CPenguin::DefaultGive() const
{
	return PENGUIN_DEFAULT_GIVE;
}

const char* CPenguin::DefaultAmmoName() const
{
	return "Penguins";
}

const char* CPenguin::EventsFile() const
{
	return "events/penguinfire.sc";
}
#endif
