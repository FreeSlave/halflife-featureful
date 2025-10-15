#include <cstring>
#include "blood_types.h"

int BloodTypeFromName(const char* name)
{
	if (stricmp(name, "red") == 0)
	{
		return BLOOD_COLOR_RED;
	}
	else if (stricmp(name, "yellow") == 0)
	{
		return BLOOD_COLOR_YELLOW;
	}
	else if (stricmp(name, "no") == 0)
	{
		return DONT_BLEED;
	}
	else
	{
		return BLOOD_COLOR_INVALID;
	}
}
