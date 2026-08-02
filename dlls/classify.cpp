#include <cstring>
#include "arraysize.h"
#include "classify.h"

struct ClassifyAndName
{
	ClassifyAndName(int cl, const char* n, const char* an = nullptr): classify(cl), name(n), altName(an) {}
	int classify;
	const char* name;
	const char* altName = nullptr;
};

static ClassifyAndName classifyAndNames[] = {
	{ CLASS_NONE, "None" },
	{ CLASS_MACHINE, "Machine" },
	{ CLASS_PLAYER, "Player" },
	{ CLASS_HUMAN_PASSIVE, "Human Passive" },
	{ CLASS_HUMAN_MILITARY, "Human Military" },
	{ CLASS_ALIEN_MILITARY, "Alien Military" },
	{ CLASS_ALIEN_PASSIVE, "Alien Passive" },
	{ CLASS_ALIEN_MONSTER, "Alien Monster" },
	{ CLASS_ALIEN_PREY, "Alien Prey" },
	{ CLASS_ALIEN_PREDATOR, "Alien Predator" },
	{ CLASS_INSECT, "Insect" },
	{ CLASS_PLAYER_ALLY, "Player Ally" },
	{ CLASS_PLAYER_BIOWEAPON, "Player Bioweapon" },
	{ CLASS_ALIEN_BIOWEAPON, "Alien Bioweapon" },
	{ CLASS_RACEX_PREDATOR, "Race X Predator" },
	{ CLASS_RACEX_SHOCK, "Race X Shock", "Race X" },
	{ CLASS_PLAYER_ALLY_MILITARY, "Player Ally Military", "Ally Military" },
	{ CLASS_HUMAN_BLACKOPS, "Human Blackops", "Blackops" },
	{ CLASS_SNARK, "Snark" },
	{ CLASS_GARGANTUA, "Gargantua" },
};

static_assert(ARRAYSIZE(classifyAndNames) == CLASS_NUMBER_OF_CLASSES, "array size doesn't match the number of relationship classes");

const char* ClassifyDisplayName(int classify)
{
	if (classify >= 0 && classify < CLASS_NUMBER_OF_CLASSES)
	{
		return classifyAndNames[classify].name;
	}
	return "Unknown";
}

int ClassifyFromName(const char* name)
{
	if (!name)
		return -1;
	for (const ClassifyAndName& p : classifyAndNames)
	{
		if (stricmp(name, p.name) == 0)
		{
			return p.classify;
		}
	}
	for (const ClassifyAndName& p : classifyAndNames)
	{
		if (p.altName)
		{
			if (stricmp(name, p.altName) == 0)
			{
				return p.classify;
			}
		}
	}
	return -1;
}
