#pragma once
#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H

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

// Keeps clutter down a bit, when using a float as a bit-vector
#define SetBits(flBitVector, bits)		((flBitVector) = (int)(flBitVector) | (bits))
#define ClearBits(flBitVector, bits)	((flBitVector) = (int)(flBitVector) & ~(bits))
#define FBitSet(flBitVector, bit)		((int)(flBitVector) & (bit))

#endif
