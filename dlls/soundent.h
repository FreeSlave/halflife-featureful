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
//=========================================================
// Soundent.h - the entity that spawns when the world 
// spawns, and handles the world's active and free sound
// lists.
//=========================================================
#pragma once
#if !defined(SOUNDENT_H)
#define SOUNDENT_H

#include "cbase.h"
#include "soundent_bits.h"
#include <utility>

#define	MAX_WORLD_SOUNDS	128 // maximum number of sounds handled by the world at one time.

#define SOUNDLIST_EMPTY	-1

#define SOUNDLISTTYPE_FREE	1// identifiers passed to functions that can operate on either list, to indicate which list to operate on.
#define SOUNDLISTTYPE_ACTIVE 2

#define	SOUND_NEVER_EXPIRE	-1 // with this set as a sound's ExpireTime, the sound will never expire.

extern const std::pair<int, const char*> g_SoundNames[7];

//=========================================================
// CSound - an instance of a sound in the world.
//=========================================================
class CSound
{
public:
	void	Clear ();
	void	Reset ();

	Vector	m_vecOrigin;	// sound's location in space
	int		m_iType;		// what type of sound this is
	int		m_iVolume;		// how loud the sound is
	float	m_flExpireTime;	// when the sound should be purged from the list
	int		m_iNext;		// index of next sound in this list ( Active or Free )
	int		m_iNextAudible;	// temporary link that monsters use to build a list of audible sounds

	bool FIsSound();
	bool FIsScent();
};

//=========================================================
// CSoundEnt - a single instance of this entity spawns when
// the world spawns. The SoundEnt's job is to update the 
// world's Free and Active sound lists.
//=========================================================
class CSoundEnt : public CBaseEntity 
{
public:
	void Precache() override;
	void Spawn() override;
	void Think() override;
	void Initialize ();

	static void		InsertSound ( CBaseEntity* pInitiator, int iType, const Vector &vecOrigin, int iVolume, float flDuration );
	static void		InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration ) {
		InsertSound(nullptr, iType, vecOrigin, iVolume, flDuration);
	}
	static void		FreeSound ( int iSound, int iPrevious );
	static int		ActiveList();// return the head of the active list
	static int		FreeList();// return the head of the free list
	static CSound*	SoundPointerForIndex( int iIndex );// return a pointer for this index in the sound list
	static int		ClientSoundIndex ( edict_t *pClient );
	static void		ReportUpdate();

	bool	IsEmpty() { return m_iActiveSound == SOUNDLIST_EMPTY; }
	int		ISoundsInList ( int iListType );
	int		IAllocSound ();
	int		ObjectCaps() override { return FCAP_DONT_SAVE; }

	int		m_iFreeSound;	// index of the first sound in the free sound list
	int		m_iActiveSound; // indes of the first sound in the active sound list
	int		m_cLastActiveSounds; // keeps track of the number of active sounds at the last update. (for diagnostic work)

private:
	CSound		m_SoundPool[ MAX_WORLD_SOUNDS ];
};
#endif // SOUNDENT_H
