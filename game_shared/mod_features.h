#pragma once
#ifndef MOD_FEATURES_H
#define MOD_FEATURES_H

/*
 * 0 to disable feature
 * 1 to enable feature
 */

// Enable opfor specific changes, like more weapon slots, green hud, etc.
// Follow the symbol to see what it actually changes
#define FEATURE_OPFOR_SPECIFIC 0

#define FEATURE_OPFOR_WEAPON_SLOTS (0 || FEATURE_OPFOR_SPECIFIC)

// enable reverse relationship models, like barnabus
#define FEATURE_REVERSE_RELATIONSHIP_MODELS 0

// monsters who carry hand grenades will drop one hand grenade upon death
#define FEATURE_MONSTERS_DROP_HANDGRENADES 0

// Weapon features
#define FEATURE_PREDICTABLE_LASER_SPOT 0

// Misc features
#define FEATURE_CLIENTSIDE_HUDSOUND 0
#define FEATURE_GEIGER_SOUNDS_FIX 1

// Experimental Cvars
#define FEATURE_EXPERIMENTAL_CVARS 1

#define FEATURE_USE_THROUGH_WALLS_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)
#define FEATURE_GRENADE_JUMP_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)
#define FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)

#endif
