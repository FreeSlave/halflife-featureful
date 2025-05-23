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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include        "game.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"flyingmonster.h"

#define FLYING_AE_FLAP		(8)
#define FLYING_AE_FLAPSOUND	(9)

extern DLL_GLOBAL edict_t *g_pBodyQueueHead;

int CFlyingMonster::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	// UNDONE: need to check more than the endpoint
	if( FBitSet( pev->flags, FL_SWIM ) && ( UTIL_PointContents( vecEnd ) != CONTENTS_WATER ) )
	{
		// ALERT( at_aiconsole, "can't swim out of water\n" );
		return LOCALMOVE_INVALID;
	}

	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0.0f, 0.0f, 32.0f ), vecEnd + Vector( 0.0f, 0.0f, 32.0f ), dont_ignore_monsters, large_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if( pflDist )
	{
		*pflDist = ( ( tr.vecEndPos - Vector( 0.0f, 0.0f, 32.0f ) ) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if( tr.fStartSolid || tr.flFraction < 1.0f )
	{
		if( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}

Activity CFlyingMonster::GetStoppedActivity( void )
{ 
	if( pev->movetype != MOVETYPE_FLY )		// UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;

	return ACT_HOVER; 
}

void CFlyingMonster::Stop( void ) 
{
	Activity stopped = GetStoppedActivity();
	if( m_IdealActivity != stopped )
	{
		m_flightSpeed = 0;
		m_IdealActivity = stopped;
	}
	pev->angles.z = 0.0f;
	pev->angles.x = 0.0f;
	m_vecTravel = g_vecZero;
}

float CFlyingMonster::ChangeYaw( int yawSpeed )
{
	if( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlYawDiff();
		float target = 0.0f;

		if( m_IdealActivity != GetStoppedActivity() )
		{
			if( diff < -20.0f )
				target = 90.0f;
			else if( diff > 20.0f )
				target = -90.0f;
		}

		float speed = 220.f;

		if( monsteryawspeedfix.value )
		{
			if( m_flLastZYawTime == 0.f )
				m_flLastZYawTime = gpGlobals->time - gpGlobals->frametime;

			float delta = Q_min( gpGlobals->time - m_flLastZYawTime, 0.25f );
			m_flLastZYawTime = gpGlobals->time;

			speed *= delta;
		}
		else
			speed *= gpGlobals->frametime;

		pev->angles.z = UTIL_Approach( target, pev->angles.z, speed );
	}
	return CBaseMonster::ChangeYaw( yawSpeed );
}

KilledResult CFlyingMonster::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_STEP;
	ClearBits( pev->flags, FL_ONGROUND );
	pev->angles.z = 0;
	pev->angles.x = 0;
	return CBaseMonster::Killed( pevInflictor, pevAttacker, iGib );
}

void CFlyingMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case FLYING_AE_FLAP:
		m_flightSpeed = 400;
		break;
	case FLYING_AE_FLAPSOUND:
		if( m_pFlapSound )
			EMIT_SOUND( edict(), CHAN_BODY, m_pFlapSound, 1, ATTN_NORM );	
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CFlyingMonster::Move( float flInterval )
{
	if( pev->movetype == MOVETYPE_FLY )
		m_flGroundSpeed = m_flightSpeed;
	CBaseMonster::Move( flInterval );
}

bool CFlyingMonster::ShouldAdvanceRoute( float flWaypointDist )
{
	// Get true 3D distance to the goal so we actually reach the correct height
	if( m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL )
		flWaypointDist = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Length();

	if( flWaypointDist <= 64.0f + ( m_flGroundSpeed * gpGlobals->frametime ) )
		return true;

	return false;
}

void CFlyingMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if( pev->movetype == MOVETYPE_FLY )
	{
		if( gpGlobals->time - m_stopTime > 1.0f )
		{
			if( m_IdealActivity != m_movementActivity )
			{
				m_IdealActivity = m_movementActivity;
				m_flGroundSpeed = m_flightSpeed = 200;
			}
		}
		Vector vecMove = pev->origin + ( ( vecDir + ( m_vecTravel * m_momentum ) ).Normalize() * (m_flGroundSpeed * flInterval ) );

		if( m_IdealActivity != m_movementActivity )
		{
			m_flightSpeed = UTIL_Approach( 100, m_flightSpeed, 75.0f * gpGlobals->frametime );
			if( m_flightSpeed < 100 )
				m_stopTime = gpGlobals->time;
		}
		else
			m_flightSpeed = UTIL_Approach( 20, m_flightSpeed, 300.0f * gpGlobals->frametime );

		if( CheckLocalMove( pev->origin, vecMove, pTargetEnt, NULL ) )
		{
			m_vecTravel = vecMove - pev->origin;
			m_vecTravel = m_vecTravel.Normalize();
			UTIL_MoveToOrigin( ENT( pev ), vecMove, ( m_flGroundSpeed * flInterval ), MOVE_STRAFE );
		}
		else
		{
			m_IdealActivity = GetStoppedActivity();
			m_stopTime = gpGlobals->time;
			m_vecTravel = g_vecZero;
		}
	}
	else
		CBaseMonster::MoveExecute( pTargetEnt, vecDir, flInterval );
}

float CFlyingMonster::CeilingZ( const Vector &position )
{
	TraceResult tr;

	Vector minUp = position;
	Vector maxUp = position;
	maxUp.z += 4096.0f;

	UTIL_TraceLine( position, maxUp, ignore_monsters, NULL, &tr );
	if( tr.flFraction != 1.0f )
		maxUp.z = tr.vecEndPos.z;

	if( ( pev->flags ) & FL_SWIM )
	{
		return UTIL_WaterLevel( position, minUp.z, maxUp.z );
	}
	return maxUp.z;
}

bool CFlyingMonster::ProbeZ( const Vector &position, const Vector &probe, float *pFraction )
{
	int conPosition = UTIL_PointContents( position );
	if( ( ( ( pev->flags ) & FL_SWIM ) == FL_SWIM ) ^ ( conPosition == CONTENTS_WATER ) )
	{
		//    SWIMING & !WATER
		// or FLYING  & WATER
		//
		*pFraction = 0.0f;
		return true; // We hit a water boundary because we are where we don't belong.
	}
	int conProbe = UTIL_PointContents( probe );
	if( conProbe == conPosition )
	{
		// The probe is either entirely inside the water (for fish) or entirely
		// outside the water (for birds).
		//
		*pFraction = 1.0f;
		return false;
	}

	Vector ProbeUnit = ( probe - position ).Normalize();
	float ProbeLength = ( probe - position ).Length();
	float maxProbeLength = ProbeLength;
	float minProbeLength = 0;

	float diff = maxProbeLength - minProbeLength;
	while( diff > 1.0f )
	{
		float midProbeLength = minProbeLength + diff / 2.0f;
		Vector midProbeVec = midProbeLength * ProbeUnit;
		if( UTIL_PointContents( position + midProbeVec ) == conPosition )
		{
			minProbeLength = midProbeLength;
		}
		else
		{
			maxProbeLength = midProbeLength;
		}
		diff = maxProbeLength - minProbeLength;
	}
	*pFraction = minProbeLength/ProbeLength;

	return true;
}

Vector CFlyingMonster::DoProbe(const Vector &Probe, const Vector& myVelocity)
{
	Vector WallNormal = Vector(0,0,-1); // WATER normal is Straight Down for fish.
	float frac;
	bool bBumpedSomething = ProbeZ(pev->origin, Probe, &frac);

	TraceResult tr;
	TRACE_MONSTER_HULL(edict(), pev->origin, Probe, dont_ignore_monsters, edict(), &tr);
	if ( tr.fAllSolid || tr.flFraction < 0.99 )
	{
		if (tr.flFraction < 0.0)
			tr.flFraction = 0.0;
		if (tr.flFraction > 1.0)
			tr.flFraction = 1.0;
		if (tr.flFraction < frac)
		{
			frac = tr.flFraction;
			bBumpedSomething = true;
			WallNormal = tr.vecPlaneNormal;
		}
	}

	if (bBumpedSomething && (m_hEnemy == 0 || tr.pHit != m_hEnemy->edict()))
	{
		Vector ProbeDir = Probe - pev->origin;

		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct( NormalToProbeAndWallNormal, ProbeDir);

		float SteeringForce = m_flightSpeed * (1-frac) * (DotProduct(WallNormal.Normalize(), myVelocity.Normalize()));
		if (SteeringForce < 0.0)
		{
			SteeringForce = -SteeringForce;
		}
		SteeringVector = SteeringForce * SteeringVector.Normalize();

		return SteeringVector;
	}
	return Vector(0, 0, 0);
}

float CFlyingMonster::FloorZ( const Vector &position )
{
	TraceResult tr;

	Vector down = position;
	down.z -= 2048.0f;

	UTIL_TraceLine( position, down, ignore_monsters, NULL, &tr );

	if( tr.flFraction != 1.0f )
		return tr.vecEndPos.z;

	return down.z;
}
