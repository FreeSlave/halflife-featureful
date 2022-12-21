#pragma once
#ifndef DMG_TYPES_H
#define DMG_TYPES_H

// instant damage

#define DMG_GENERIC			0			// generic damage was done
#define DMG_CRUSH			(1 << 0)	// crushed by falling or moving object
#define DMG_BULLET			(1 << 1)	// shot
#define DMG_SLASH			(1 << 2)	// cut, clawed, stabbed
#define DMG_BURN			(1 << 3)	// heat burned
#define DMG_FREEZE			(1 << 4)	// frozen
#define DMG_FALL			(1 << 5)	// fell too far
#define DMG_BLAST			(1 << 6)	// explosive blast damage
#define DMG_CLUB			(1 << 7)	// crowbar, punch, headbutt
#define DMG_SHOCK			(1 << 8)	// electric shock
#define DMG_SONIC			(1 << 9)	// sound pulse shockwave
#define DMG_ENERGYBEAM		(1 << 10)	// laser or other high energy beam
#define DMG_NEVERGIB		(1 << 12)	// with this bit OR'd in, no damage type will be able to gib victims upon death
#define DMG_ALWAYSGIB		(1 << 13)	// with this bit OR'd in, any damage type can be made to gib victims upon death.
#define DMG_DROWN			(1 << 14)	// Drowning
// time-based damage

#define DMG_PARALYZE		(1 << 15)	// slows affected creature down
#define DMG_NERVEGAS		(1 << 16)	// nerve toxins, very bad
#define DMG_POISON			(1 << 17)	// blood poisioning
#define DMG_RADIATION		(1 << 18)	// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)	// drowning recovery
#define DMG_ACID			(1 << 20)	// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)	// in an oven
#define DMG_SLOWFREEZE		(1 << 22)	// in a subzero freezer

// it's not time-based!
#define DMG_MORTAR			(1 << 23)	// Hit by air raid (done to distinguish grenade from mortar)

// additional flags
// TODO: make into another set of flags?
#define DMG_NONLETHAL		(1 << 24) // this damage shouldn't kill player
#define DMG_TIMEDNONLETHAL	(1 << 25) // timed damage, e.g. poison, shouldn't kill player completely
#define DMG_DONTBLEED		(1 << 26) // used in TraceAttack. Force not to bleed.
#define DMG_IGNORE_ARMOR	(1 << 27)

// Modifiers to time-based damage, up to 8
#define DMG_TIMED_MOD_NONLETHAL ( 1 << 0 )
#define DMG_TIMED_MOD_IGNORE_ARMOR ( 1 << 1 )

#define HEAL_GENERIC 0
#define HEAL_CHARGE (1<<0) // Charge my portable medkit

#endif
