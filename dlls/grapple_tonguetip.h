/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#ifndef BARNACLEGRAPPLETIP_H
#define BARNACLEGRAPPLETIP_H

#include "cbase.h"

class CBarnacleGrappleTip : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override;

	void EXPORT FlyThink();
	void EXPORT OffsetThink();

	void EXPORT TongueTouch( CBaseEntity* pOther );

	int CheckTarget( CBaseEntity* pTarget );

	void SetPosition( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner );

	int GetGrappleType() const { return m_GrappleType; }

	bool IsStuck() const { return m_bIsStuck; }

	bool HasMissed() const { return m_bMissed; }
#ifndef CLIENT_DLL
	EHANDLE& GetGrappleTarget() { return m_hGrappleTarget; }
	void SetGrappleTarget( CBaseEntity* pTarget )
	{
		m_hGrappleTarget = pTarget;
	}
#endif
private:
	int m_GrappleType;
	bool m_bIsStuck;
	bool m_bMissed;
#ifndef CLIENT_DLL
	EHANDLE m_hGrappleTarget;
#endif
	Vector m_vecOriginOffset;
};

#endif
