#include "util_shared.h"
#include "string_utils.h"
#include "const_render.h"
#include <cmath>
#include <cstdlib>

static unsigned int glSeed = 0;

unsigned int seed_table[256] =
{
	28985, 27138, 26457, 9451, 17764, 10909, 28790, 8716, 6361, 4853, 17798, 21977, 19643, 20662, 10834, 20103,
	27067, 28634, 18623, 25849, 8576, 26234, 23887, 18228, 32587, 4836, 3306, 1811, 3035, 24559, 18399, 315,
	26766, 907, 24102, 12370, 9674, 2972, 10472, 16492, 22683, 11529, 27968, 30406, 13213, 2319, 23620, 16823,
	10013, 23772, 21567, 1251, 19579, 20313, 18241, 30130, 8402, 20807, 27354, 7169, 21211, 17293, 5410, 19223,
	10255, 22480, 27388, 9946, 15628, 24389, 17308, 2370, 9530, 31683, 25927, 23567, 11694, 26397, 32602, 15031,
	18255, 17582, 1422, 28835, 23607, 12597, 20602, 10138, 5212, 1252, 10074, 23166, 19823, 31667, 5902, 24630,
	18948, 14330, 14950, 8939, 23540, 21311, 22428, 22391, 3583, 29004, 30498, 18714, 4278, 2437, 22430, 3439,
	28313, 23161, 25396, 13471, 19324, 15287, 2563, 18901, 13103, 16867, 9714, 14322, 15197, 26889, 19372, 26241,
	31925, 14640, 11497, 8941, 10056, 6451, 28656, 10737, 13874, 17356, 8281, 25937, 1661, 4850, 7448, 12744,
	21826, 5477, 10167, 16705, 26897, 8839, 30947, 27978, 27283, 24685, 32298, 3525, 12398, 28726, 9475, 10208,
	617, 13467, 22287, 2376, 6097, 26312, 2974, 9114, 21787, 28010, 4725, 15387, 3274, 10762, 31695, 17320,
	18324, 12441, 16801, 27376, 22464, 7500, 5666, 18144, 15314, 31914, 31627, 6495, 5226, 31203, 2331, 4668,
	12650, 18275, 351, 7268, 31319, 30119, 7600, 2905, 13826, 11343, 13053, 15583, 30055, 31093, 5067, 761,
	9685, 11070, 21369, 27155, 3663, 26542, 20169, 12161, 15411, 30401, 7580, 31784, 8985, 29367, 20989, 14203,
	29694, 21167, 10337, 1706, 28578, 887, 3373, 19477, 14382, 675, 7033, 15111, 26138, 12252, 30996, 21409,
	25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847
};

unsigned int U_Random( void )
{
	glSeed *= 69069;
	glSeed += seed_table[glSeed & 0xff];

	return ( ++glSeed & 0x0fffffff );
}

void U_Srand( unsigned int seed )
{
	glSeed = seed_table[seed & 0xff];
}

/*
=====================
UTIL_SharedRandomLong
=====================
*/
int UTIL_SharedRandomLong( unsigned int seed, int low, int high )
{
	unsigned int range;

	U_Srand( (int)seed + low + high );

	range = high - low + 1;
	if( !( range - 1 ) )
	{
		return low;
	}
	else
	{
		int offset;
		int rnum;

		rnum = U_Random();

		offset = rnum % range;

		return ( low + offset );
	}
}

/*
=====================
UTIL_SharedRandomFloat
=====================
*/
float UTIL_SharedRandomFloat( unsigned int seed, float low, float high )
{
	unsigned int range;

	U_Srand( (int)seed + *(int *)&low + *(int *)&high );

	U_Random();
	U_Random();

	range = (int)( high - low );
	if( !range )
	{
		return low;
	}
	else
	{
		int tensixrand;
		float offset;

		tensixrand = U_Random() & 65535;

		offset = (float)tensixrand / 65536.0f;

		return ( low + offset * range );
	}
}

int UTIL_LastRandomSeed()
{
	return glSeed;
}

// ripped this out of the engine
float UTIL_AngleMod( float a )
{
	/*if( a < 0 )
	{
		a = a + 360 * ( (int)( a / 360 ) + 1 );
	}
	else if( a >= 360 )
	{
		a = a - 360 * ( (int)( a / 360 ) );
	}*/
	// a = ( 360.0 / 65536 ) * ( (int)( a * ( 65536 / 360.0 ) ) & 65535 );
	a = fmod( a, 360.0f );
	if( a < 0 )
		a += 360;
	return a;
}

float UTIL_AngleDiff( float destAngle, float srcAngle )
{
	float delta;

	delta = destAngle - srcAngle;
	if( destAngle > srcAngle )
	{
		if( delta >= 180 )
			delta -= 360;
	}
	else
	{
		if( delta <= -180 )
			delta += 360;
	}
	return delta;
}

float UTIL_Approach( float target, float value, float speed )
{
	float delta = target - value;

	if( delta > speed )
		value += speed;
	else if( delta < -speed )
		value -= speed;
	else
		value = target;

	return value;
}

float UTIL_ApproachAngle( float target, float value, float speed )
{
	target = UTIL_AngleMod( target );
	value = UTIL_AngleMod( value );

	float delta = target - value;

	// Speed is assumed to be positive
	if( speed < 0 )
		speed = -speed;

	if( delta < -180 )
		delta += 360;
	else if( delta > 180 )
		delta -= 360;

	if( delta > speed )
		value += speed;
	else if( delta < -speed )
		value -= speed;
	else
		value = target;

	return value;
}

float UTIL_AngleDistance( float next, float cur )
{
	float delta = next - cur;

	if( delta < -180 )
		delta += 360;
	else if( delta > 180 )
		delta -= 360;

	return delta;
}

void UTIL_StringToVector( float *pVector, const char *pString, int* componentsRead )
{
	char *pstr, *pfront, tempString[128];
	int j;

	strncpyEnsureTermination( tempString, pString );
	pstr = pfront = tempString;

	int componentsParsed = 0;

	for( j = 0; j < 3; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atof( pfront );
		componentsParsed++;

		while( *pstr && *pstr != ' ' )
			pstr++;
		if( !( *pstr ) )
			break;
		pstr++;
		pfront = pstr;
	}
	if (componentsRead)
		*componentsRead = componentsParsed;
	if( j < 2 )
	{
		for( j = j + 1;j < 3; j++ )
			pVector[j] = 0;
	}
}

const char* RenderModeToString(int rendermode)
{
	switch (rendermode) {
	case kRenderNormal:
		return "Normal";
	case kRenderTransColor:
		return "Color";
	case kRenderTransTexture:
		return "Texture";
	case kRenderGlow:
		return "Glow";
	case kRenderTransAlpha:
		return "Solid";
	case kRenderTransAdd:
		return "Additive";
	default:
		return "Unknown";
	}
}

const char* RenderFxToString(int renderfx)
{
	switch (renderfx) {
	case kRenderFxNone:			return "Normal";
	case kRenderFxPulseSlow:	return "Slow Pulse";
	case kRenderFxPulseFast:	return "Fast Pulse";
	case kRenderFxPulseSlowWide:return "Slow Wide Pulse";
	case kRenderFxFadeSlow:		return "Slow Fade Away";
	case kRenderFxFadeFast:		return "Fast Fade Away";
	case kRenderFxSolidSlow:	return "Slow Become Solid";
	case kRenderFxSolidFast:	return "Fast Become Solid";
	case kRenderFxStrobeSlow:	return "Slow Strobe";
	case kRenderFxStrobeFast:	return "Fast Strobe";
	case kRenderFxStrobeFaster:	return "Faster Strobe";
	case kRenderFxFlickerSlow:	return "Slow Flicker";
	case kRenderFxFlickerFast:	return "Fast Flicker";
	case kRenderFxNoDissipation:return "Constant Glow";
	case kRenderFxDistort:		return "Distort";
	case kRenderFxHologram:		return "Hologram";
	case kRenderFxGlowShell:	return "Glow Shell";
	default: return "Unknown";
	}
}
