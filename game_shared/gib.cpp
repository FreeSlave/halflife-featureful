#include <cstring>
#include "gib.h"

int ParseGibPolicy(const char *gibPolicyName)
{
	if (stricmp(gibPolicyName, "always") == 0)
	{
		return GIB_ALWAYS;
	}
	else if (stricmp(gibPolicyName, "never") == 0)
	{
		return GIB_NEVER;
	}
	return GIB_NORMAL;
}
