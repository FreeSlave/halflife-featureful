/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#if !defined(VECTOR_H)
#define VECTOR_H

#include <cmath>

#if __cplusplus >= 201703L
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif

//=========================================================
// 2DVector - used for many pathfinding and many other 
// operations that are treated as planar rather than 3d.
//=========================================================
class Vector2D
{
public:
	constexpr Vector2D(void): x( 0.0f ), y( 0.0f ) {}
	constexpr Vector2D(float X, float Y): x( X ), y( Y ) {}
	NODISCARD constexpr Vector2D operator+(const Vector2D& v)	const	{ return Vector2D( x + v.x, y + v.y );	}
	NODISCARD constexpr Vector2D operator-(const Vector2D& v)	const	{ return Vector2D( x - v.x, y - v.y );	}
	NODISCARD constexpr Vector2D operator*(float fl)		const	{ return Vector2D( x * fl, y * fl );	}
	NODISCARD constexpr Vector2D operator/(float fl)		const	{ return Vector2D( x / fl, y / fl );	}

	NODISCARD inline float Length() const	{ return sqrt(x * x + y * y ); }
	NODISCARD inline constexpr float LengthSqr() const	{ return x * x + y * y; }

	NODISCARD inline Vector2D Normalize() const
	{
		//Vector2D vec2;

		float flLen = Length();
		if( flLen == 0 )
		{
			return Vector2D( 0, 0 );
		}
		else
		{
			flLen = 1 / flLen;
			return Vector2D( x * flLen, y * flLen );
		}
	}
	float NormalizeInPlace()
	{
		const float flLen = Length();
		if( flLen == 0 )
		{
			x = y = 0;
		}
		else
		{
			float den = 1 / flLen;
			x *= den;
			y *= den;
		}
		return flLen;
	}

	float	x, y;
};

NODISCARD inline float DotProduct( const Vector2D& a, const Vector2D& b ) { return( a.x * b.x + a.y * b.y ); }
NODISCARD inline Vector2D operator*( float fl, const Vector2D& v ) { return v * fl; }

//=========================================================
// 3D Vector
//=========================================================
class Vector						// same data-layout as engine's vec3_t,
{								//		which is a vec_t[3]
public:
	// Construction/destruction
	constexpr Vector( void ): x( 0.0f ), y( 0.0f ), z( 0.0f ) {}
	constexpr Vector( float X, float Y, float Z ): x( X ), y( Y), z( Z ) {}
	//inline Vector( double X, double Y, double Z )		{ x = (float)X; y = (float)Y; z = (float)Z;	}
	//inline Vector( int X, int Y, int Z )			{ x = (float)X; y = (float)Y; z = (float)Z;	}
	constexpr Vector( const Vector& v ) = default;
	constexpr Vector( float rgfl[3] ): x( rgfl[0] ), y( rgfl[1] ), z( rgfl[2] )	{}

	// Operators
	NODISCARD constexpr Vector operator-( void ) const			{ return Vector( -x, -y, -z );			}
	NODISCARD constexpr bool operator==( const Vector& v ) const		{ return x==v.x && y==v.y && z==v.z;		}
	NODISCARD constexpr bool operator!=( const Vector& v ) const		{ return !( *this==v );				}
	NODISCARD constexpr Vector operator+( const Vector& v ) const	{ return Vector( x + v.x, y + v.y, z + v.z );	}
	NODISCARD constexpr Vector operator-( const Vector& v ) const	{ return Vector( x - v.x, y - v.y, z - v.z );	}
	NODISCARD constexpr Vector operator*( float fl) const		{ return Vector( x * fl, y * fl, z * fl );	}
	NODISCARD constexpr Vector operator/( float fl) const		{ return Vector( x / fl, y / fl, z / fl );	}

	inline Vector& operator+=(const Vector& v) { x += v.x; y += v.y; z += v.z; return *this; }
	inline Vector& operator-=(const Vector& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	inline Vector& operator*=(float fl) { x *= fl; y *= fl; z *= fl; return *this; }
	inline Vector& operator/=(float fl) { x /= fl; y /= fl; z /= fl; return *this; }

	// Methods
	void CopyToArray( float* rgfl ) const		{ rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; }
	NODISCARD inline float Length() const		{ return sqrt( x * x + y * y + z * z ); }
	NODISCARD inline constexpr float LengthSqr() const { return x * x + y * y + z * z; }
	operator float *()								{ return &x; } // Vectors will now automatically convert to float * when needed
	constexpr operator const float *() const					{ return &x; } // Vectors will now automatically convert to float * when needed
	NODISCARD inline Vector Normalize( void ) const
	{
		float flLen = Length();
		if( flLen == 0 ) return Vector( 0, 0, 1 ); // ????
		flLen = 1 / flLen;
		return Vector( x * flLen, y * flLen, z * flLen );
	}
	float NormalizeInPlace()
	{
		const float flLen = Length();
		if (flLen == 0)
		{
			x = 0;
			y = 0;
			z = 1;
		}
		else
		{
			float den = 1 / flLen;
			x *= den;
			y *= den;
			z *= den;
		}
		return flLen;
	}

	NODISCARD constexpr Vector2D Make2D( void ) const
	{
		return {x, y};
	}
	NODISCARD inline float Length2D() const		{ return sqrt( x * x + y * y ); }
	NODISCARD inline constexpr float Length2DSqr() const		{ return x * x + y * y; }

	NODISCARD inline constexpr bool IsLengthGreaterThan(float val) const {
		return LengthSqr() > val*val;
	}
	NODISCARD inline constexpr bool IsLengthGreaterThanOrEqual(float val) const {
		return LengthSqr() >= val*val;
	}
	NODISCARD inline constexpr bool IsLengthLessThan(float val) const {
		return LengthSqr() < val*val;
	}
	NODISCARD inline constexpr bool IsLengthLessThanOrEqual(float val) const {
		return LengthSqr() <= val*val;
	}

	NODISCARD inline constexpr bool IsLength2DGreaterThan(float val) const {
		return Length2DSqr() > val*val;
	}
	NODISCARD inline constexpr bool IsLength2DGreaterThanOrEqual(float val) const {
		return Length2DSqr() >= val*val;
	}
	NODISCARD inline constexpr bool IsLength2DLessThan(float val) const {
		return Length2DSqr() < val*val;
	}
	NODISCARD inline constexpr bool IsLength2DLessThanOrEqual(float val) const {
		return Length2DSqr() <= val*val;
	}
	void ClampToLengthInPlace(float length)
	{
		if (IsLengthGreaterThan(length))
		{
			NormalizeInPlace();
			operator*=(length);
		}
	}

	// Members
	float x = 0, y = 0, z = 0;
};
NODISCARD constexpr Vector operator*( float fl, const Vector& v ) { return v * fl; }
NODISCARD constexpr float DotProduct( const Vector& a, const Vector& b ) { return( a.x * b.x + a.y * b.y + a.z * b.z); }
NODISCARD constexpr Vector CrossProduct( const Vector& a, const Vector& b ) { return Vector( a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x ); }
#endif
