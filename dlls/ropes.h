/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#ifndef ROPES_H
#define ROPES_H

class CRopeSegment;
class CRopeSample;

struct RopeSampleData;

#include "cbase.h"

#define MAX_SEGMENTS 63
#define MAX_SAMPLES  64

/**
*	A rope with a number of segments.
*	Uses an RK4 integrator with dampened springs to simulate rope physics.
*/
class CRope : public CBaseDelay
{
public:
	CRope();
	void KeyValue( KeyValueData* pkvd );

	void Precache();

	void Spawn();
	void Activate();
	void UpdateOnRemove();

	int ObjectCaps();

	void InitRope();
	void EXPORT RopeThink();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void InitializeRopeSim();
	void RunSimOnSamples();
	void ComputeForces( RopeSampleData* pSystem );
	void ComputeForces( CRopeSample** ppSystem );
	void ComputeSampleForce( RopeSampleData& data );
	void ComputeSpringForce(RopeSampleData& first, RopeSampleData& second);
	void RK4Integrate(const float flDeltaTime);
	void TraceModels( CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs );
	void SetRopeSegments( const int uiNumSegments,
						  CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs );

	bool MoveUp( const float flDeltaTime );
	bool MoveDown( const float flDeltaTime );

	Vector GetAttachedObjectsVelocity() const;
	void ApplyForceFromPlayer( const Vector& vecForce );
	void ApplyForceToSegment( const Vector& vecForce, const int uiSegment );
	void AttachObjectToSegment( CRopeSegment* pSegment );
	void DetachObject(float delay = 2.0f);
	bool IsObjectAttached() const { return mObjectAttached; }
	bool IsAcceptingAttachment() const;

	int GetNumSegments() const { return m_iSegments; }
	CRopeSegment** GetSegments() { return seg; }
	CRopeSegment** GetAltSegments() { return altseg; }

	bool GetToggleValue() const { return m_bToggle; }

	bool IsSoundAllowed() const { return m_bMakeSound; }
	void SetSoundAllowed( const bool bAllowed )
	{
		m_bMakeSound = bAllowed;
	}

	bool ShouldCreak() const;
	void Creak();

	string_t GetBodyModel() const { return mBodyModel; }
	string_t GetEndingModel() const { return mEndingModel; }

	float GetSegmentLength( int uiSegmentIndex ) const;
	float GetRopeLength() const;
	Vector GetRopeOrigin() const;

	bool IsValidSegmentIndex( const int uiSegment ) const;
	Vector GetSegmentOrigin( const int uiSegment ) const;
	Vector GetSegmentAttachmentPoint( const int uiSegment ) const;
	void SetAttachedObjectsSegment( CRopeSegment* pSegment );
	Vector GetSegmentDirFromOrigin( const int uiSegmentIndex ) const;
	Vector GetAttachedObjectsPosition() const;

	static const NamedSoundScript grabSoundScript;
	static const NamedSoundScript creakSoundScript;

private:
	int m_iSegments;

	CRopeSegment* seg[ MAX_SEGMENTS ];
	CRopeSegment* altseg[ MAX_SEGMENTS ];

	bool m_bToggle;

	bool m_InitialDeltaTime;

	float mLastTime;

	Vector m_LastEndPos;
	Vector m_Gravity;

	CRopeSample* m_Samples[ MAX_SAMPLES ];

	int m_NumSamples;

	bool mObjectAttached;

	int mAttachedObjectsSegment;
	float mAttachedObjectsOffset;
	float detachTime;
	float detachDelay;

	string_t mBodyModel;
	string_t mEndingModel;

	int mDisallowPlayerAttachment;

	bool m_bMakeSound;

protected:
	bool m_activated;
};

#endif //ROPES_H
