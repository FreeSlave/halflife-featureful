#ifndef SPORE_H
#define SPORE_H

#include "ggrenade.h"

#define SPORE_ROCKET_SPEED 1200.0f
#define SPORE_GRENADE_SPEED 800.0f

// Contact/Timed spore grenade
class CSpore : public CGrenade
{
public:
	enum SporeType
	{
		GRENADE_THROWN = 0,
		ROCKET = 1,
		GRENADE_LAUNCHED = 2,
		GRENADE_PUKED = 3
	};

public:
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	void Precache() override;
	void Spawn() override;
	void UpdateOnRemove() override;

	void EXPORT IgniteThink();
	void EXPORT FlyThink();
	void EXPORT RocketTouch(CBaseEntity* pOther);
	void EXPORT MyBounceTouch(CBaseEntity* pOther);

	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override;
	void LaunchAsProjectile(const ProjectileParameters& params) override;

	static const NamedSoundScript bounceSoundScript;
	static const NamedSoundScript impactSoundScript;

	static const NamedVisual modelVisual;
	static const NamedVisual spriteVisual;
	static const NamedVisual blowVisual;
	static const NamedVisual blowAltVisual;
	static const NamedVisual sprayVisual;
	static const NamedVisual trailVisual;
	static const NamedVisual lightVisual;

private:
	SporeType m_SporeType;

	float m_flIgniteTime;
	float m_flSoundDelay;
	float m_flExploDelay;

	EHANDLE m_hSprite;
};
#endif
