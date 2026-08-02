#pragma once
#ifndef CLASSIFY_H
#define CLASSIFY_H

// For CLASSIFY
enum
{
	CLASS_NONE,
	CLASS_MACHINE,
	CLASS_PLAYER,
	CLASS_HUMAN_PASSIVE,
	CLASS_HUMAN_MILITARY,
	CLASS_ALIEN_MILITARY,
	CLASS_ALIEN_PASSIVE,
	CLASS_ALIEN_MONSTER,
	CLASS_ALIEN_PREY,
	CLASS_ALIEN_PREDATOR,
	CLASS_INSECT,
	CLASS_PLAYER_ALLY,
	CLASS_PLAYER_BIOWEAPON, // hornets launched by players
	CLASS_ALIEN_BIOWEAPON, // hornets launched by the alien menace
	CLASS_RACEX_PREDATOR,
	CLASS_RACEX_SHOCK,
	CLASS_PLAYER_ALLY_MILITARY,
	CLASS_HUMAN_BLACKOPS,
	CLASS_SNARK,
	CLASS_GARGANTUA,
	CLASS_NUMBER_OF_CLASSES,
	CLASS_VEHICLE = 98,
	CLASS_BARNACLE = 99 // special because no one pays attention to it, and it eats a wide cross-section of creatures.
};

const char* ClassifyDisplayName(int classify);
int ClassifyFromName(const char* name);

#endif
