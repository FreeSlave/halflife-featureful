#include "random_utils.h"

#if CLIENT_DLL
#include "cl_dll.h"
#elif SERVER_DLL
#include "extdll.h"
#include "enginecallback.h"
#else
#include <cstdlib>
#endif

int RandomInt(int low, int high)
{
#if CLIENT_DLL
	return gEngfuncs.pfnRandomLong(low, high);
#elif SERVER_DLL
	return RANDOM_LONG(low, high);
#else
	return low + rand() % (high-low+1);
#endif
}

float RandomFloat(float low, float high)
{
#if CLIENT_DLL
	return gEngfuncs.pfnRandomFloat(low, high);
#elif SERVER_DLL
	return RANDOM_FLOAT(low, high);
#else
	return low + (rand()/(float)RAND_MAX) * (high - low);
#endif
}
