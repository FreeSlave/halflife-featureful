#pragma once
#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H

#include <type_traits>

int UTIL_SharedRandomLong( unsigned int seed, int low, int high );
float UTIL_SharedRandomFloat( unsigned int seed, float low, float high );
int UTIL_LastRandomSeed();

extern float		UTIL_AngleMod			(float a);
extern float		UTIL_AngleDiff			( float destAngle, float srcAngle );

extern float		UTIL_Approach( float target, float value, float speed );
extern float		UTIL_ApproachAngle( float target, float value, float speed );
extern float		UTIL_AngleDistance( float next, float cur );

extern void			UTIL_StringToVector( float *pVector, const char *pString, int* componentsRead = nullptr );

const char* RenderModeToString(int rendermode);
const char* RenderFxToString(int renderfx);

template<typename T1, typename T2, typename = std::enable_if<std::is_integral<T1>::value>>
void SetBits(T1& bitVector, T2 bits)
{
	bitVector |= bits;
}

template<typename T1, typename T2, typename = std::enable_if<std::is_integral<T1>::value>>
void ClearBits(T1& bitVector, T2 bits)
{
	bitVector &= ~bits;
}

template<typename T1, typename T2, typename = std::enable_if<std::is_integral<T1>::value>>
bool FBitSet(T1 bitVector, T2 bit)
{
	return (bitVector & bit) != 0;
}

#endif
