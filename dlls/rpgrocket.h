#pragma once
#ifndef RPGROCKET_H
#define RPGROCKET_H

#include "ggrenade.h"
#include "weapons.h"

class CRpg;

class CRpgRocket : public CGrenade
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	void Spawn( void );
	void Precache( void );
	void EXPORT FollowThink( void );
	void EXPORT IgniteThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );
	static CRpgRocket *CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher );
	void Explode( TraceResult *pTrace, int bitsDamageType );
	inline CRpg *GetLauncher( void );

	float m_flIgniteTime;
	EHANDLE m_hLauncher; // handle back to the launcher that fired me.

	static const NamedSoundScript rocketIgniteSoundScript;

	static const NamedVisual trailVisual;
};

#endif
