#pragma once
#ifndef GGRENADE_H
#define GGRENADE_H

#include "effects.h"
#include "clamp.h"

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	enum GrenadeType
	{
		CONTACT,
		TIMED
	};

	void Spawn() override;
	void Precache() override;
	void PrecacheBaseGrenadeSounds();

	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;

	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override;
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	static CGrenade *ShootTimed( CBaseEntity *pOwner, const Vector& vecStart, const Vector& vecVelocity, float time, EntityOverrides entityOverrides = EntityOverrides() );
	static CGrenade *ShootContact( CBaseEntity *pOwner, const Vector& vecStart, const Vector& vecVelocity, EntityOverrides entityOverrides = EntityOverrides() );
	static CGrenade *ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static void UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code );

	void Explode( Vector vecSrc, Vector vecAim );
	virtual void Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke();

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT SlideTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink();
	void EXPORT PreDetonate();
	void EXPORT Detonate();
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink();

	virtual void BounceSound();
	int	BloodColor() override { return DONT_BLEED; }
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	virtual float ExplosionRadius() { return 0.0f; } // if 0 the default radius is used (depending on amount of damage)
	virtual int FireballDeciScaleFromDamage(float dmg) {
		int result = (dmg - Q_min(50.0f, dmg/2)) * 0.6f;
		return clamp(result, 1, 255);
	}
	virtual int FireballFramerate() {
		return 15;
	}
	virtual int SmokeDeciScaleFromDamage(float dmg) {
		int result = (dmg - Q_min(50.0f, dmg/2)) * 0.8f;
		return clamp(result, 1, 255);
	}

	bool m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.
	bool m_isTimed;

	static const NamedSoundScript debrisSoundScript;
	static const NamedSoundScript bounceSoundScript;

	static const NamedVisual handGrenadeVisual;
	static const NamedVisual arGrenadeVisual;
};

#endif
