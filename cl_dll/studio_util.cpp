//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cl_util.h"
#include "com_model.h"
#include "studio_util.h"
#include "build.h"
#if XASH_ARMv8
#define XASH_SIMD_NEON 1
#include <arm_neon.h>
#include "neon_mathfun.h"
#endif

// this conflicts with definitions from pm_math.cpp
#if XASH_SIMD_NEON
/*
====================
AngleMatrix

====================
*/
void AngleMatrix( const float *angles, float (*matrix)[4] )
{
#if XASH_SIMD_NEON
	static const uint32x4_t AngleMatrix_sign0 = vsetq_lane_u32(0x80000000, vdupq_n_u32(0), 0);
	static const uint32x4_t AngleMatrix_sign1 = vsetq_lane_u32(0x80000000, vdupq_n_u32(0), 1);
	static const uint32x4_t AngleMatrix_sign2 = vsetq_lane_u32(0x80000000, vdupq_n_u32(0), 2);
	float32x4x3_t out_reg;
	float32x4_t angles_reg = {};
	memcpy(&angles_reg, angles, sizeof(float) * 3);

	float32x4x2_t sp_sy_sr_0_cp_cy_cr_1;
	sincos_ps(vmulq_n_f32(angles_reg, (M_PI * 2 / 360)), &sp_sy_sr_0_cp_cy_cr_1.val[0], &sp_sy_sr_0_cp_cy_cr_1.val[1]);

	float32x4x2_t sp_sr_cp_cr_sy_0_cy_1 = vuzpq_f32(sp_sy_sr_0_cp_cy_cr_1.val[0], sp_sy_sr_0_cp_cy_cr_1.val[1]);
	float32x4x2_t sp_cp_sy_cy_sr_cr_0_1 = vzipq_f32(sp_sy_sr_0_cp_cy_cr_1.val[0], sp_sy_sr_0_cp_cy_cr_1.val[1]);

	float32x4_t _0_sr_cr_0 = vextq_f32(sp_sy_sr_0_cp_cy_cr_1.val[0], sp_cp_sy_cy_sr_cr_0_1.val[1], 3);
	float32x4_t cp_cr_sr_0 = vcombine_f32(vget_high_f32(sp_sr_cp_cr_sy_0_cy_1.val[0]), vget_high_f32(sp_sy_sr_0_cp_cy_cr_1.val[0]));
	float32x4_t cy_sy_sy_0 = vcombine_f32(vrev64_f32(vget_high_f32(sp_cp_sy_cy_sr_cr_0_1.val[0])), vget_low_f32(sp_sr_cp_cr_sy_0_cy_1.val[1]));
	float32x4_t sy_cy_cy_1 = vcombine_f32(vget_high_f32(sp_cp_sy_cy_sr_cr_0_1.val[0]), vget_high_f32(sp_sr_cp_cr_sy_0_cy_1.val[1]));

	float32x4_t _0_srsp_crsp_0 = vmulq_laneq_f32(_0_sr_cr_0, sp_sy_sr_0_cp_cy_cr_1.val[0], 0); // *sp
	out_reg.val[0] = vmulq_laneq_f32(_0_srsp_crsp_0, sp_sy_sr_0_cp_cy_cr_1.val[1], 1); // *cy
	out_reg.val[1] = vmulq_laneq_f32(_0_srsp_crsp_0, sp_sy_sr_0_cp_cy_cr_1.val[0], 1); // *sy

	cy_sy_sy_0 = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(cy_sy_sy_0), AngleMatrix_sign1));
	sy_cy_cy_1 = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(sy_cy_cy_1), AngleMatrix_sign2));
	out_reg.val[0] = vfmaq_f32(out_reg.val[0], cp_cr_sr_0, cy_sy_sy_0);
	out_reg.val[1] = vfmaq_f32(out_reg.val[1], cp_cr_sr_0, sy_cy_cy_1);

	float32x4_t cp_cr_0_1 = vcombine_f32(vget_high_f32(sp_sr_cp_cr_sy_0_cy_1.val[0]), vget_high_f32(sp_cp_sy_cy_sr_cr_0_1.val[1]));
	float32x4_t _1_cp_cr_0 = vextq_f32(cp_cr_0_1, cp_cr_0_1, 3);
	out_reg.val[2] = vmulq_f32(sp_sr_cp_cr_sy_0_cy_1.val[0], _1_cp_cr_0);
	out_reg.val[2] = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(out_reg.val[2]), AngleMatrix_sign0));

	memcpy(matrix, &out_reg, sizeof(float) * 3 * 4);
#else
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * ( M_PI_F * 2.0f / 360.0f );
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[PITCH] * ( M_PI_F * 2.0f / 360.0f );
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[ROLL] * ( M_PI_F * 2.0f / 360.0f );
	sr = sin( angle );
	cr = cos( angle );

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr * sp * cy + cr * -sy;
	matrix[1][1] = sr * sp * sy + cr * cy;
	matrix[2][1] = sr * cp;
	matrix[0][2] = (cr * sp * cy + -sr * -sy);
	matrix[1][2] = (cr * sp * sy + -sr* cy);
	matrix[2][2] = cr * cp;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
#endif
}

/*
====================
VectorCompare

====================
*/
int VectorCompare( const float *v1, const float *v2 )
{
#if XASH_SIMD_NEON
	// is this really works? 
	float32x4_t v1_reg = {}, v2_reg = {};
	memcpy(&v1_reg, v1, sizeof(float) * 3);
	memcpy(&v2_reg, v2, sizeof(float) * 3);
	return !vaddvq_u32(vceqq_f32(v1_reg, v2_reg));
#else
	int i;

	for( i = 0; i < 3; i++ )
		if( v1[i] != v2[i] )
			return 0;

	return 1;
#endif
}

/*
====================
CrossProduct

====================
*/
void CrossProduct( const float *v1, const float *v2, float *cross )
{
#if XASH_SIMD_NEON
	float32x4_t v1_reg = {}, v2_reg = {};
	memcpy(&v1_reg, v1, sizeof(float) * 3);
	memcpy(&v2_reg, v2, sizeof(float) * 3);

	float32x2_t xy_a = vget_low_f32(v1_reg);
	float32x2_t xy_b = vget_low_f32(v2_reg);
	float32x4_t yzxy_a = vcombine_f32(vext_f32(xy_a, vget_high_f32(v1_reg), 1), xy_a); // [aj, ak, ai, aj]
	float32x4_t yzxy_b = vcombine_f32(vext_f32(xy_b, vget_high_f32(v2_reg), 1), xy_b); // [bj, bk, bi, bj]
	float32x4_t zxyy_a = vextq_f32(yzxy_a, yzxy_a, 1); // [ak, ai, aj, aj]
	float32x4_t zxyy_b = vextq_f32(yzxy_b, yzxy_b, 1); // [bk, ai, bj, bj]
	float32x4_t cross_reg = vfmsq_f32(vmulq_f32(yzxy_a, zxyy_b), zxyy_a, yzxy_b); // [ajbk-akbj, akbi-aibk, aibj-ajbi, 0]

	memcpy(cross, &cross_reg, sizeof(float) * 3);
#else
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
#endif
}

/*
====================
VectorTransform

====================
*/
void VectorTransform( const float *in1, float in2[3][4], float *out )
{
#if XASH_SIMD_NEON
	float32x4_t in1_reg = {};
	memcpy(&in1_reg, in1, sizeof(float) * 3);

	float32x4x4_t in_t;
	memcpy(&in_t, in2, sizeof(float) * 3 * 4);
	//memset(&in_t.val[3], 0, sizeof(in_t.val[3]));
	in_t = vld4q_f32((const float*)&in_t);

	float32x4_t out_reg = in_t.val[3];
	out_reg = vfmaq_laneq_f32(out_reg, in_t.val[0], in1_reg, 0);
	out_reg = vfmaq_laneq_f32(out_reg, in_t.val[1], in1_reg, 1);
	out_reg = vfmaq_laneq_f32(out_reg, in_t.val[2], in1_reg, 2);

	memcpy(out, &out_reg, sizeof(float) * 3);
#else
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
#endif
}

/*
================
ConcatTransforms

================
*/
void ConcatTransforms( float in1[3][4], float in2[3][4], float out[3][4] )
{
#if XASH_SIMD_NEON
	float32x4x3_t in1_reg, in2_reg;
	memcpy(&in1_reg, in1, sizeof(float) * 3 * 4);
	memcpy(&in2_reg, in2, sizeof(float) * 3 * 4);
	float32x4x3_t out_reg = {};

	out_reg.val[0] = vcopyq_laneq_f32(out_reg.val[0], 3, in1_reg.val[0], 3); // out[0][3] = in[0][3]
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], in2_reg.val[0], in1_reg.val[0], 0); // out[0][n] += in2[0][n] * in1[0][0]
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], in2_reg.val[1], in1_reg.val[0], 1); // out[0][n] += in2[1][n] * in1[0][1]
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], in2_reg.val[2], in1_reg.val[0], 2); // out[0][n] += in2[2][n] * in1[0][2]

	out_reg.val[1] = vcopyq_laneq_f32(out_reg.val[1], 3, in1_reg.val[1], 3);
	out_reg.val[1] = vfmaq_laneq_f32(out_reg.val[1], in2_reg.val[0], in1_reg.val[1], 0);
	out_reg.val[1] = vfmaq_laneq_f32(out_reg.val[1], in2_reg.val[1], in1_reg.val[1], 1);
	out_reg.val[1] = vfmaq_laneq_f32(out_reg.val[1], in2_reg.val[2], in1_reg.val[1], 2);

	out_reg.val[2] = vcopyq_laneq_f32(out_reg.val[2], 3, in1_reg.val[2], 3);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], in2_reg.val[0], in1_reg.val[2], 0);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], in2_reg.val[1], in1_reg.val[2], 1);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], in2_reg.val[2], in1_reg.val[2], 2);

	memcpy(out, &out_reg, sizeof(float) * 3 * 4);
#else
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
				in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
				in1[2][2] * in2[2][3] + in1[2][3];
#endif
}
#endif

// angles index are not the same as ROLL, PITCH, YAW

/*
====================
AngleQuaternion

====================
*/
void AngleQuaternion( float *angles, vec4_t quaternion )
{
#if XASH_SIMD_NEON
	static const uint32x4_t AngleQuaternion_sign2 = vzipq_u32(vdupq_n_u32(0x80000000), vdupq_n_u32(0x00000000)).val[0]; // { 0x80000000, 0x00000000, 0x80000000, 0x00000000 };
	float32x4_t angles_reg = {};
	memcpy(&angles_reg, angles, sizeof(float) * 3);
	float32x4x2_t sr_sp_sy_0_cr_cp_cy_1;
	sincos_ps(vmulq_n_f32(angles_reg, 0.5), &sr_sp_sy_0_cr_cp_cy_1.val[0], &sr_sp_sy_0_cr_cp_cy_1.val[1]);

	float32x4x2_t sr_sy_cr_cy_sp_0_cp_1 = vuzpq_f32(sr_sp_sy_0_cr_cp_cy_1.val[0], sr_sp_sy_0_cr_cp_cy_1.val[1]);

	float32x4_t sr_sy_cr_cy = sr_sy_cr_cy_sp_0_cp_1.val[0];
	float32x4_t sy_cr_cy_sr = vextq_f32(sr_sy_cr_cy_sp_0_cp_1.val[0], sr_sy_cr_cy_sp_0_cp_1.val[0], 1);
	float32x4_t srsy_sycr_crcy_cysr = vmulq_f32(sr_sy_cr_cy, sy_cr_cy_sr);
	float32x4_t sycr_crcy_cysr_srsy = vextq_f32(srsy_sycr_crcy_cysr, srsy_sycr_crcy_cysr, 1);
	float32x4_t cysr_srsy_sycr_crcy = vextq_f32(srsy_sycr_crcy_cysr, srsy_sycr_crcy_cysr, 3);
	float32x4_t sycr_crcy_cysr_srsy_signed = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(sycr_crcy_cysr_srsy), AngleQuaternion_sign2));

	float32x4_t left = vmulq_laneq_f32(cysr_srsy_sycr_crcy, sr_sp_sy_0_cr_cp_cy_1.val[1], 1);

	float32x4_t out_reg = vfmaq_laneq_f32(left, sycr_crcy_cysr_srsy_signed, sr_sp_sy_0_cr_cp_cy_1.val[0], 1);
	memcpy(quaternion, &out_reg, sizeof(float) * 4);
	// A = sr * sy, B = sy * cr, C = cr * cy, D = cy * sr
	//quaternion[0] =   D * cp - B * sp; // X
	//quaternion[1] =   A * cp + C * sp; // Y
	//quaternion[2] =   B * cp - D * sp; // Z
	//quaternion[3] =   C * cp + A * sp; // W
#else
	float angle;
	float sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5f;
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[1] * 0.5f;
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[0] * 0.5f;
	sr = sin( angle );
	cr = cos( angle );

	quaternion[0] = sr * cp * cy - cr * sp * sy; // X
	quaternion[1] = cr * sp * cy + sr * cp * sy; // Y
	quaternion[2] = cr * cp * sy - sr * sp * cy; // Z
	quaternion[3] = cr * cp * cy + sr * sp * sy; // W
#endif
}

#if XASH_SIMD_NEON
static inline float32x4_t dot_dup_fast(float32x4_t a, float32x4_t b)
{
	float32x4_t m = vmulq_f32(a, b);
#if __aarch64__
	float s = vaddvq_f32(m);
	return vdupq_n_f32(s);
#else
	float32x2_t lo = vget_low_f32(m);
	float32x2_t hi = vget_high_f32(m);
	float32x2_t sum2 = vadd_f32(lo, hi);       // [m0+m2, m1+m3]
	float32x2_t final = vpadd_f32(sum2, sum2); // [dot, dot]
	return vcombine_f32(final, final);
#endif
}
#endif

/*
====================
QuaternionSlerp

====================
*/
void QuaternionSlerp( vec4_t p, vec4_t q, float t, vec4_t qt )
{
#if XASH_SIMD_NEON
	float32x4_t p_reg = vld1q_f32(p);
	float32x4_t q_reg = vld1q_f32(q);

	const uint32x4_t signmask = vdupq_n_u32(0x80000000);
	const float32x4_t one_minus_eps = vdupq_n_f32(1.0f - 0.000001f);

	float32x4_t vcosom = dot_dup_fast(p_reg, q_reg);

	uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(vcosom), signmask);
	q_reg = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q_reg), sign));
	vcosom = vabsq_f32(vcosom);

	uint32x4_t use_slerp = vcltq_f32(vcosom, one_minus_eps);

	float32x4_t omega = acos_ps(vcosom);
	float32x4_t sinom = sin_ps(omega);

	float32x4_t sinom_recip = vrecpeq_f32(sinom);
	sinom_recip = vmulq_f32(sinom_recip, vrecpsq_f32(sinom, sinom_recip));

	float32x4_t sclp_s = vmulq_f32(sin_ps(vmulq_n_f32(omega, (1.0f - t))), sinom_recip);
	float32x4_t sclq_s = vmulq_f32(sin_ps(vmulq_n_f32(omega, t)), sinom_recip);

	float32x4_t sclp_l = vdupq_n_f32(1.0f - t);
	float32x4_t sclq_l = vdupq_n_f32(t);

	float32x4_t sclp = vbslq_f32(use_slerp, sclp_s, sclp_l);
	float32x4_t sclq = vbslq_f32(use_slerp, sclq_s, sclq_l);

	float32x4_t qt_reg = vfmaq_f32(vmulq_f32(p_reg, sclp), q_reg, sclq);
	vst1q_f32(qt, qt_reg);
#else
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;

	for( i = 0; i < 4; i++ )
	{
		a += ( p[i] - q[i] ) * ( p[i] - q[i] );
		b += ( p[i] + q[i] ) * ( p[i] + q[i] );
	}
	if(a > b)
	{
		for( i = 0; i < 4; i++ )
		{
			q[i] = -q[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if( ( 1.0f + cosom ) > 0.000001f )
	{
		if( ( 1.0f - cosom ) > 0.000001f )
		{
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( ( 1.0f - t ) * omega ) / sinom;
			sclq = sin( t * omega ) / sinom;
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}
		for( i = 0; i < 4; i++ )
		{
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -q[1];
		qt[1] = q[0];
		qt[2] = -q[3];
		qt[3] = q[2];
		sclp = sin( ( 1.0f - t ) * ( 0.5f * M_PI_F ) );
		sclq = sin( t * ( 0.5f * M_PI_F ) );
		for( i = 0; i < 3; i++ )
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
#endif
}

/*
====================
QuaternionSlerpX4

====================
*/
void QuaternionSlerpX4( vec4_t p[4], vec4_t q[4], float t, vec4_t qt[4] )
{
#if XASH_SIMD_NEON
	float32x4x4_t p_reg = vld4q_f32((const float*)p);
	float32x4x4_t q_reg = vld4q_f32((const float*)q);

	const uint32x4_t signmask = vdupq_n_u32(0x80000000);
	const float32x4_t one_minus_eps = vdupq_n_f32(1.0f - 0.000001f);

	float32x4_t cosom = vmulq_f32(p_reg.val[0], q_reg.val[0]);
	cosom = vfmaq_f32(cosom, p_reg.val[1], q_reg.val[1]);
	cosom = vfmaq_f32(cosom, p_reg.val[2], q_reg.val[2]);
	cosom = vfmaq_f32(cosom, p_reg.val[3], q_reg.val[3]);

	uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(cosom), signmask);
	for( int i = 0; i < 4; i++ ) q_reg.val[i] = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q_reg.val[i]), sign));
	cosom = vabsq_f32(cosom);

	float32x4_t sclp_l = vdupq_n_f32(1.0f - t);
	float32x4_t sclq_l = vdupq_n_f32(t);
	uint32x4_t use_slerp = vcltq_f32(cosom, one_minus_eps);

	float32x4_t omega = acos_ps(cosom);
	float32x4_t sinom = sin_ps(omega);

	float32x4_t sinom_recip = vrecpeq_f32(sinom);
	sinom_recip = vmulq_f32(sinom_recip, vrecpsq_f32(sinom, sinom_recip));

	float32x4_t sinp = sin_ps(vmulq_n_f32(omega, (1.0f - t)));
	float32x4_t sinq = sin_ps(vmulq_n_f32(omega, t));

	float32x4_t sclp_s = vmulq_f32(sinp, sinom_recip);
	float32x4_t sclq_s = vmulq_f32(sinq, sinom_recip);

	float32x4_t sclp = vbslq_f32(use_slerp, sclp_s, sclp_l);
	float32x4_t sclq = vbslq_f32(use_slerp, sclq_s, sclq_l);

	float32x4x4_t qt_reg;
	for( int i = 0; i < 4; i++ ) qt_reg.val[i] = vfmaq_f32(vmulq_f32(p_reg.val[i], sclp), q_reg.val[i], sclq);
	vst4q_f32((float*)qt, qt_reg);
#else
	QuaternionSlerp(p[0], q[0], t, qt[0]);
	QuaternionSlerp(p[1], q[1], t, qt[1]);
	QuaternionSlerp(p[2], q[2], t, qt[2]);
	QuaternionSlerp(p[3], q[3], t, qt[3]);
#endif
}

/*
====================
QuaternionMatrix

====================
*/
void QuaternionMatrix( vec4_t quaternion, float (*matrix)[4] )
{
#if XASH_SIMD_NEON
	static const uint32x4_t QuaternionMatrix_sign1 = vsetq_lane_u32(0x80000000, vdupq_n_u32(0x00000000), 0); // { 0x80000000, 0x00000000, 0x00000000, 0x00000000 };
	static const uint32x4_t QuaternionMatrix_sign2 = vsetq_lane_u32(0x80000000, vdupq_n_u32(0x00000000), 1); // { 0x00000000, 0x80000000, 0x00000000, 0x00000000 };
	static const uint32x4_t QuaternionMatrix_sign3 = vsetq_lane_u32(0x00000000, vdupq_n_u32(0x80000000), 2); // { 0x80000000, 0x80000000, 0x00000000, 0x80000000 };
	static const float32x4_t matrix3x4_identity_0 = vsetq_lane_f32(1, vdupq_n_f32(0), 0); // { 1, 0, 0, 0 }
	static const float32x4_t matrix3x4_identity_1 = vsetq_lane_f32(1, vdupq_n_f32(0), 1); // { 0, 1, 0, 0 }
	static const float32x4_t matrix3x4_identity_2 = vsetq_lane_f32(1, vdupq_n_f32(0), 2); // { 0, 0, 1, 0 }
	float32x4_t quaternion_reg;
	memcpy(&quaternion_reg, quaternion, sizeof(float) * 4);

	float32x4_t q1032 = vrev64q_f32(quaternion_reg);
	float32x4_t q1032_signed = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q1032), QuaternionMatrix_sign1));
	float32x4_t q2301 = vextq_f32(quaternion_reg, quaternion_reg, 2);
	float32x4_t q2301_signed = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q2301), QuaternionMatrix_sign3));
	float32x4_t q3210 = vrev64q_f32(q2301);
	float32x4_t q3210_signed = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q3210), QuaternionMatrix_sign2));

	float32x4x3_t out_reg;

	out_reg.val[0] = vmulq_laneq_f32(q2301_signed, quaternion_reg, 2);
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], q1032_signed, quaternion_reg, 1);
	out_reg.val[0] = vfmaq_n_f32(matrix3x4_identity_0, out_reg.val[0], 2.0f);

	out_reg.val[1] = vmulq_laneq_f32(q3210_signed, quaternion_reg, 2);
	out_reg.val[1] = vfmsq_laneq_f32(out_reg.val[1], q1032_signed, quaternion_reg, 0);
	out_reg.val[1] = vfmaq_n_f32(matrix3x4_identity_1, out_reg.val[1], 2.0f);

	out_reg.val[2] = vmulq_laneq_f32(q3210_signed, quaternion_reg, 1);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], q2301_signed, quaternion_reg, 0);
	out_reg.val[2] = vfmsq_n_f32(matrix3x4_identity_2, out_reg.val[2], 2.0f);

	memcpy(matrix, &out_reg, sizeof(float) * 3 * 4);
/*
	matrix[0][0] = 1.0 + 2.0 * (  quaternion[1] * -quaternion[1] + -quaternion[2] * quaternion[2] );
	matrix[0][1] = 0.0 + 2.0 * (  quaternion[1] *  quaternion[0] + -quaternion[3] * quaternion[2] );
	matrix[0][2] = 0.0 + 2.0 * (  quaternion[1] *  quaternion[3] +  quaternion[0] * quaternion[2] );
	matrix[0][3] = 0.0 + 2.0 * (  quaternion[1] *  quaternion[2] + -quaternion[1] * quaternion[2] );

	matrix[1][0] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[1] +  quaternion[3] * quaternion[2] );
	matrix[1][1] = 1.0 + 2.0 * ( -quaternion[0] *  quaternion[0] + -quaternion[2] * quaternion[2] );
	matrix[1][2] = 0.0 + 2.0 * ( -quaternion[0] *  quaternion[3] +  quaternion[1] * quaternion[2] );
	matrix[1][3] = 0.0 + 2.0 * ( -quaternion[0] *  quaternion[2] +  quaternion[0] * quaternion[2] );
	
	matrix[2][0] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[2] + -quaternion[3] * quaternion[1] );
	matrix[2][1] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[3] +  quaternion[2] * quaternion[1] );
	matrix[2][2] = 1.0 + 2.0 * ( -quaternion[0] *  quaternion[0] + -quaternion[1] * quaternion[1] );
	matrix[2][3] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[1] + -quaternion[0] * quaternion[1] );
*/
#else
	matrix[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
#endif
}

/*
====================
MatrixCopy

====================
*/
void MatrixCopy( float in[3][4], float out[3][4] )
{
	memcpy( out, in, sizeof( float ) * 3 * 4 );
}
