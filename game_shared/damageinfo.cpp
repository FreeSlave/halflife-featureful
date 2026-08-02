#include <cstring>
#include "damageinfo.h"
#include "util_shared.h"

int ParseDamageType(const char *type)
{
	if (stricmp(type, "generic") == 0)
	{
		return DMG_GENERIC;
	}
	else if (stricmp(type, "crush") == 0)
	{
		return DMG_CRUSH;
	}
	else if (stricmp(type, "bullet") == 0)
	{
		return DMG_BULLET;
	}
	else if (stricmp(type, "slash") == 0)
	{
		return DMG_SLASH;
	}
	else if (stricmp(type, "burn") == 0)
	{
		return DMG_BURN;
	}
	else if (stricmp(type, "freeze") == 0)
	{
		return DMG_FREEZE;
	}
	else if (stricmp(type, "blast") == 0)
	{
		return DMG_BLAST;
	}
	else if (stricmp(type, "club") == 0)
	{
		return DMG_CLUB;
	}
	else if (stricmp(type, "shock") == 0)
	{
		return DMG_SHOCK;
	}
	else if (stricmp(type, "sonic") == 0)
	{
		return DMG_SONIC;
	}
	else if (stricmp(type, "energybeam") == 0)
	{
		return DMG_ENERGYBEAM;
	}
	else if (stricmp(type, "paralyze") == 0)
	{
		return DMG_PARALYZE;
	}
	else if (stricmp(type, "nervegas") == 0)
	{
		return DMG_NERVEGAS;
	}
	else if (stricmp(type, "poison") == 0)
	{
		return DMG_POISON;
	}
	else if (stricmp(type, "radiation") == 0)
	{
		return DMG_RADIATION;
	}
	else if (stricmp(type, "acid") == 0)
	{
		return DMG_ACID;
	}
	else if (stricmp(type, "slowburn") == 0)
	{
		return DMG_SLOWBURN;
	}
	else if (stricmp(type, "slowfreeze") == 0)
	{
		return DMG_SLOWFREEZE;
	}
	return -1;
}
