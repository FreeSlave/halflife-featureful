#pragma once
#ifndef GGRENADE_H
#define GGRENADE_H

#include "effects.h"
#include "clamp.h"

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache();
	void PrecacheBaseGrenadeSounds();

	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;

	static CGrenade *ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade *ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static void UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code );

	void Explode( Vector vecSrc, Vector vecAim );
	virtual void Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke( void );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT SlideTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink( void );

	virtual void BounceSound( void );
	virtual int	BloodColor( void ) { return DONT_BLEED; }
	KilledResult Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib ) override;
	virtual float ExplosionRadius() { return 0.0f; } // if 0 the default radius is used (depending on amount of damage)
	virtual int ExplosionDeciScaleFromDamage(float dmg) {
		int result = (dmg - Q_min(50.0f, dmg/2)) * 0.6f;
		return clamp(result, 1, 255);
	}
	virtual int SmokeDeciScaleFromDamage(float dmg) {
		int result = (dmg - Q_min(50.0f, dmg/2)) * 0.8f;
		return clamp(result, 1, 255);
	}

	bool m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.

	static const NamedSoundScript debrisSoundScript;
	static const NamedSoundScript bounceSoundScript;
};

#endif
