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
/*

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"
#include "game.h"
#include "visuals_utils.h"

//===================grenade


LINK_ENTITY_TO_CLASS( grenade, CGrenade )

TYPEDESCRIPTION	CGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CGrenade, m_isTimed, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CGrenade, CBaseMonster)

const NamedSoundScript CGrenade::debrisSoundScript = {
	CHAN_VOICE,
	{"weapons/debris1.wav", "weapons/debris2.wav", "weapons/debris3.wav"},
	0.55f,
	ATTN_NORM,
	"BaseGrenade.Debris"
};

const NamedSoundScript CGrenade::bounceSoundScript = {
	CHAN_VOICE,
	{"weapons/grenade_hit1.wav", "weapons/grenade_hit2.wav", "weapons/grenade_hit3.wav"},
	0.25f,
	ATTN_NORM,
	"HandGrenade.Bounce"
};

const NamedVisual CGrenade::handGrenadeVisual = BuildVisual("HandGrenade.Model")
		.Model("models/w_grenade.mdl");

const NamedVisual CGrenade::arGrenadeVisual = BuildVisual("ARGrenade.Model")
		.Model("models/grenade.mdl");

//
// Grenade Explode
//
void CGrenade::ExplodeDownwards()
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), & tr );

	Explode(&tr);
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode(const TraceResult *pTrace)
{
	// float flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	RadiusDamageInfo radiusDamageInfo = GetProjectileRadiusDamageInfo();

	// Pull out of the wall a bit
	if( pTrace->flFraction != 1.0f )
	{
		if (explosionfix.value)
			pev->origin = pTrace->vecEndPos + ( pTrace->vecPlaneNormal * 0.6f );
		else
			pev->origin = pTrace->vecEndPos + ( pTrace->vecPlaneNormal * ( radiusDamageInfo.damageInfo.damage - 24 ) * 0.6f );
	}

	int iContents = UTIL_PointContents( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_VECTOR( pev->origin );	// Send to PAS because of the sound
		if( iContents != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( FireballDeciScaleFromDamage( radiusDamageInfo.damageInfo.damage ) ); // scale * 10
		WRITE_BYTE( FireballFramerate() ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	InsertAISound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, radiusDamageInfo);

	if( RANDOM_FLOAT( 0, 1 ) < 0.5f )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	//flRndSound = RANDOM_FLOAT( 0, 1 );

	EmitSoundScript(debrisSoundScript);

	pev->effects |= EF_NODRAW;
	SetThink( &CGrenade::Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3f;

	if( iContents != CONTENTS_WATER )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );
		for( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}

void CGrenade::Smoke()
{
	if( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_VECTOR( pev->origin );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( SmokeDeciScaleFromDamage( GetProjectileRadiusDamageInfo().damageInfo.damage ) ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

KilledResult CGrenade::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	Detonate();
	return KilledResult();
}

// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate()
{
	InsertAISound( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate()
{
	TraceResult tr;
	Vector vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT(pev), &tr );

	Explode(&tr);
}


//
// Contact grenade, explode when it touches something
// 
void CGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT( pev ), &tr );

	Explode(&tr);
}

void CGrenade::DangerSoundThink()
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	InsertAISound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5f, GetProjectileRadiusDamageInfo().GetRadius(), 0.2 );
	pev->nextthink = gpGlobals->time + 0.2f;

	if( pev->waterlevel != WL_NotInWater )
	{
		pev->velocity = pev->velocity * 0.5f;
	}
}

void CGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// only do damage if we're moving fairly fast
	if( m_flNextAttack < gpGlobals->time && pev->velocity.IsLengthGreaterThan(100) )
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if( pevOwner && pOther->pev->takedamage )
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			pOther->ApplyTraceAttack( pev, pevOwner, GetProjectileDirectDamageInfo(), gpGlobals->v_forward, &tr );
		}
		m_flNextAttack = gpGlobals->time + 1.0f; // debounce
	}

	bool shouldDetonate = false;
	CheckDetonationOnTouch(shouldDetonate, pOther);

	if (shouldDetonate)
	{
		SetThink(nullptr);
		SetTouch(nullptr);
		Detonate();
		return;
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector( 300, 300, 300 );

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45f;

	if( !m_fRegisteredSound && vecTestVelocity.IsLengthLessThanOrEqual(60) )
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		InsertAISound( bits_SOUND_DANGER, pev->origin, (int)( GetProjectileRadiusDamageInfo().GetRadius() ), 0.3f );
		m_fRegisteredSound = true;
	}

	if( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8f;

		pev->sequence = RANDOM_LONG( 1, 1 );
		ResetSequenceInfo();
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0f;
	if( pev->framerate > 1.0f )
		pev->framerate = 1.0f;
	else if( pev->framerate < 0.5f )
	{
		pev->framerate = 0.0f;
		pev->frame = 0.0f;
	}
}

void CGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector( 300, 300, 300 );
	if( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95f;

		if( pev->velocity.x != 0 || pev->velocity.y != 0 )
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade::BounceSound()
{
	EmitSoundScript(bounceSoundScript);
}

void CGrenade::TumbleThink()
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->dmgtime - 1 < gpGlobals->time )
	{
		InsertAISound( bits_SOUND_DANGER, pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time ), 400, 0.1 );
	}

	if( pev->dmgtime <= gpGlobals->time )
	{
		SetThink( &CGrenade::Detonate );
	}
	if( pev->waterlevel != WL_NotInWater )
	{
		pev->velocity = pev->velocity * 0.5f;
		pev->framerate = 0.2f;
	}
}

void CGrenade::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenade" );

	pev->solid = SOLID_BBOX;

	if (m_isTimed)
		ApplyVisualWithOwn(GetVisual(handGrenadeVisual));
	else
		ApplyVisualWithOwn(GetVisual(arGrenadeVisual));
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_fRegisteredSound = false;
}

void CGrenade::Precache()
{
	PrecacheBaseGrenadeSounds();
	RegisterAndPrecacheSoundScript(bounceSoundScript);

	RegisterVisualAsMineOwn(handGrenadeVisual);
	RegisterVisualAsMineOwn(arGrenadeVisual);
}

void CGrenade::PrecacheBaseGrenadeSounds()
{
	RegisterAndPrecacheSoundScript(debrisSoundScript);
}

RadiusDamageInfo CGrenade::GetDefaultProjectileRadiusDamageInfo()
{
	DamageInfo damageInfo{0.0f, DMG_BLAST};
	if (m_isTimed)
	{
		damageInfo.damage = GetSkillValue("plr_9mmAR_grenade");
	}
	else
	{
		damageInfo.damage = GetSkillValue("plr_hand_grenade");
	}
	return RadiusDamageInfo(damageInfo);
}

DamageInfo CGrenade::GetDefaultProjectileDirectDamageInfo()
{
	return DamageInfo(GetSkillValue("plr_hand_grenade_hit"), DMG_CLUB);
}

void CGrenade::SetProjectileParamsBeforeSpawn(const ProjectileParameters& params)
{
	m_isTimed = params.variant == TIMED;
	SetProjectileParamsBeforeSpawnImpl(params);
}

void CGrenade::LaunchAsProjectile(const ProjectileParameters& params)
{
	if (params.variant == CONTACT)
	{
		pev->gravity = 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
		LaunchAsProjectileImpl(800.0f, params);
		SetMyProjectileEffectFlags();

		// make monsters afaid of it while in the air
		SetThink( &CGrenade::DangerSoundThink );
		pev->nextthink = gpGlobals->time;

		// Tumble in air
		pev->avelocity.x = RANDOM_FLOAT( -100, -500 );

		// Explode on contact
		SetTouch( &CGrenade::ExplodeTouch );
	}
	else
	{
		LaunchAsProjectileImpl(600.0f, params);
		SetMyProjectileEffectFlags();

		SetTouch( &CGrenade::BounceTouch );	// Bounce if touched

		pev->dmgtime = gpGlobals->time + params.time.value_or(0.0f);
		SetThink( &CGrenade::TumbleThink );
		pev->nextthink = gpGlobals->time + 0.1f;
		if( params.time.value_or(0.0f) < 0.1f )
		{
			pev->nextthink = gpGlobals->time;
			pev->velocity = Vector( 0, 0, 0 );
		}

		pev->sequence = RANDOM_LONG( 3, 6 );
		ResetSequenceInfo();
		pev->framerate = 1.0f;

		// Tumble through the air
		// pGrenade->pev->avelocity.x = -400;

		pev->gravity = 0.5f;
		pev->friction = 0.8f;
	}
}

CGrenade *CGrenade::ShootContact(CBaseEntity *pOwner, const Vector& vecStart, const Vector& vecVelocity, EntityOverrides entityOverrides )
{
	Vector vecDir = vecVelocity;
	const float speed = vecDir.NormalizeInPlace();
	const Vector vecAng = UTIL_VecToAngles(vecVelocity);

	ProjectileParameters parameters("grenade", vecStart, vecAng, vecDir, speed, pOwner, entityOverrides);
	return (CGrenade*)CreateAndLaunchAsProjectile(parameters);
}

CGrenade *CGrenade::ShootTimed( CBaseEntity *pOwner, const Vector& vecStart, const Vector& vecVelocity, float time, EntityOverrides entityOverrides )
{
	Vector vecDir{};
	float speed = 0.0f;

	if (vecVelocity != g_vecZero)
	{
		vecDir = vecVelocity;
		speed = vecDir.NormalizeInPlace();
	}

	const Vector vecAng = UTIL_VecToAngles(vecVelocity);

	ProjectileParameters parameters("grenade", vecStart, vecAng, vecDir, speed, pOwner, entityOverrides);
	parameters.variant = TIMED;
	parameters.time = time;
	return (CGrenade*)CreateAndLaunchAsProjectile(parameters);
}

//======================end grenade

class CGrenadeRound : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;

	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override;
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	RadiusDamageInfo GetDefaultProjectileRadiusDamageInfo() override;

	void EXPORT GrenadeTouch(CBaseEntity* pEntity);
	void EXPORT GrenadeThink();

	static const NamedSoundScript bounceSoundScript;

	static const NamedVisual trailVisual;
};

LINK_ENTITY_TO_CLASS( grenaderound, CGrenadeRound )

const NamedSoundScript CGrenadeRound::bounceSoundScript = {
	CHAN_VOICE,
	{"weapons/grenade_hit1.wav", "weapons/grenade_hit2.wav", "weapons/grenade_hit3.wav"},
	0.25f,
	ATTN_NORM,
	"Grenade.Bounce"
};

const NamedVisual CGrenadeRound::trailVisual = BuildVisual("Grenade.Trail")
		.Model("sprites/smoke.spr")
		.Life(2.0f)
		.BeamWidth(4)
		.RenderColor(224, 224, 255)
		.Alpha(200);

void CGrenadeRound::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetMyModel("models/pipebomb.mdl");
	pev->skin = 1;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_fRegisteredSound = false;
}

void CGrenadeRound::Precache()
{
	PrecacheMyModel("models/pipebomb.mdl");
	RegisterVisual(trailVisual);
	PrecacheBaseGrenadeSounds();
	RegisterAndPrecacheSoundScript(bounceSoundScript);
}

void CGrenadeRound::SetProjectileParamsBeforeSpawn(const ProjectileParameters& params)
{
	SetProjectileParamsBeforeSpawnImpl(params);
}

void CGrenadeRound::LaunchAsProjectile(const ProjectileParameters& params)
{
	const float baseSpeed = params.speedOverride ? params.speedOverride : 600.0f;
	pev->velocity = baseSpeed * params.direction;

	pev->velocity += params.up * baseSpeed / 3.0f;

	pev->avelocity = Vector(300, 300, 300);
	pev->friction = 0.5f;

	const float time = params.time.has_value() ? *params.time : 2.5f;
	pev->dmgtime = gpGlobals->time + time;

	SetMyProjectileEffectFlags();
	SetTouch(&CGrenadeRound::GrenadeTouch);
	SetThink(&CGrenadeRound::GrenadeThink);
	pev->nextthink = gpGlobals->time;

	SendBeamFollow(entindex(), GetVisual(trailVisual));
}

RadiusDamageInfo CGrenadeRound::GetDefaultProjectileRadiusDamageInfo()
{
	return RadiusDamageInfo(DamageInfo(GetSkillValue("plr_grenade"), DMG_BLAST));
}

void CGrenadeRound::GrenadeTouch(CBaseEntity* pOther)
{
	bool shouldDetonate = pOther->pev->takedamage == DAMAGE_AIM;
	CheckDetonationOnTouch(shouldDetonate, pOther);

	if (shouldDetonate)
	{
		SetThink(nullptr);
		SetTouch(nullptr);
		Detonate();
		return;
	}

	if (!m_fRegisteredSound)
	{
		Vector vecTestVelocity = pev->velocity;
		vecTestVelocity.z *= 0.45f;

		if (vecTestVelocity.IsLengthLessThanOrEqual(60))
		{
			InsertAISound(bits_SOUND_DANGER, pev->origin, (int)(GetProjectileRadiusDamageInfo().GetRadius()), 0.3f);
			m_fRegisteredSound = true;
		}
	}

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.75f;

		if (pev->velocity.Length() <= 20)
		{
			pev->avelocity = g_vecZero;
		}
	}
	else
	{
		EmitSoundScript(bounceSoundScript);
	}

	if (pev->velocity == g_vecZero)
		pev->avelocity = g_vecZero;
}

void CGrenadeRound::GrenadeThink()
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1f;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		InsertAISound(bits_SOUND_DANGER, pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time ), 400, 0.1f);
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
}
