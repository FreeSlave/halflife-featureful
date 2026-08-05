#pragma once
#ifndef RPGROCKET_H
#define RPGROCKET_H

#include "ggrenade.h"
#include "weapons.h"

class CRpg;

class CRpgRocket : public CGrenade
{
public:
	enum
	{
		STANDARD = 0,
		STRAIGHT = 1
	};

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
	void Spawn() override;
	void Precache() override;
	RadiusDamageInfo GetDefaultProjectileRadiusDamageInfo() override;
	void SetProjectileParamsBeforeSpawn(const ProjectileParameters& params) override;
	void LaunchAsProjectile(const ProjectileParameters& params) override;
	void EXPORT FollowThink();
	void EXPORT IgniteThink();
	void Explode(const TraceResult *pTrace) override;
	inline CConfigurableWeapon *GetLauncher();

	float m_flIgniteTime;
	EHANDLE m_hLauncher; // handle back to the launcher that fired me.
	bool m_straight;
	bool m_soundStarted;

	static const NamedSoundScript rocketIgniteSoundScript;

	static const NamedVisual trailVisual;
};

#endif
