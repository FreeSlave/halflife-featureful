#pragma once
#ifndef RPGROCKET_H
#define RPGROCKET_H

#include "ggrenade.h"
#include "weapons.h"

class CRpg;

class CRpgRocket : public CGrenade
{
public:
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];
	void Spawn() override;
	void Precache() override;
	void EXPORT FollowThink();
	void EXPORT IgniteThink();
	void EXPORT RocketTouch( CBaseEntity *pOther );
	static CRpgRocket *CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher );
	void Explode( TraceResult *pTrace, int bitsDamageType ) override;
	inline CRpg *GetLauncher();

	float m_flIgniteTime;
	EHANDLE m_hLauncher; // handle back to the launcher that fired me.

	static const NamedSoundScript rocketIgniteSoundScript;

	static const NamedVisual trailVisual;
};

#endif
