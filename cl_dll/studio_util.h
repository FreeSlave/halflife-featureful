//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once
#if !defined(STUDIO_UTIL_H)
#define STUDIO_UTIL_H

#include "pi_constant.h"
#include "in_defs.h"

#define FDotProduct( a, b ) ( fabs( ( a[0] ) * ( b[0] ) ) + fabs( ( a[1] ) * ( b[1] ) ) + fabs( ( a[2] ) * ( b[2] ) ) )

void	ConcatTransforms( float in1[3][4], float in2[3][4], float out[3][4] );
void	MatrixCopy( float in[3][4], float out[3][4] );
void	QuaternionMatrix( vec4_t quaternion, float (*matrix)[4] );
void	QuaternionSlerp( vec4_t p, vec4_t q, float t, vec4_t qt );
void	QuaternionSlerpX4( vec4_t p[4], vec4_t q[4], float t, vec4_t qt[4] );
void	AngleQuaternion( float *angles, vec4_t quaternion );
#endif // STUDIO_UTIL_H
