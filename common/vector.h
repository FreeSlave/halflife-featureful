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

//=========================================================
// 2DVector - used for many pathfinding and many other 
// operations that are treated as planar rather than 3d.
//=========================================================
class Vector2D
{
public:
	constexpr Vector2D(void): x( 0.0f ), y( 0.0f ) {}
	constexpr Vector2D(float X, float Y): x( X ), y( Y ) {}
	constexpr Vector2D operator+(const Vector2D& v)	const	{ return Vector2D( x + v.x, y + v.y );	}
	constexpr Vector2D operator-(const Vector2D& v)	const	{ return Vector2D( x - v.x, y - v.y );	}
	constexpr Vector2D operator*(float fl)		const	{ return Vector2D( x * fl, y * fl );	}
	constexpr Vector2D operator/(float fl)		const	{ return Vector2D( x / fl, y / fl );	}

	inline float Length(void)			const	{ return sqrt(x * x + y * y );		}

	inline Vector2D Normalize ( void ) const
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

	float	x, y;
};

inline float DotProduct( const Vector2D& a, const Vector2D& b ) { return( a.x * b.x + a.y * b.y ); }
inline Vector2D operator*( float fl, const Vector2D& v ) { return v * fl; }

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
	constexpr Vector operator-( void ) const			{ return Vector( -x, -y, -z );			}
	constexpr bool operator==( const Vector& v ) const		{ return x==v.x && y==v.y && z==v.z;		}
	constexpr bool operator!=( const Vector& v ) const		{ return !( *this==v );				}
	constexpr Vector operator+( const Vector& v ) const	{ return Vector( x + v.x, y + v.y, z + v.z );	}
	constexpr Vector operator-( const Vector& v ) const	{ return Vector( x - v.x, y - v.y, z - v.z );	}
	constexpr Vector operator*( float fl) const		{ return Vector( x * fl, y * fl, z * fl );	}
	constexpr Vector operator/( float fl) const		{ return Vector( x / fl, y / fl, z / fl );	}

	// Methods
	void CopyToArray( float* rgfl ) const		{ rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; }
	inline float Length( void ) const					{ return sqrt( x * x + y * y + z * z ); }
	operator float *()								{ return &x; } // Vectors will now automatically convert to float * when needed
	constexpr operator const float *() const					{ return &x; } // Vectors will now automatically convert to float * when needed
	inline Vector Normalize( void ) const
	{
		float flLen = Length();
		if( flLen == 0 ) return Vector( 0, 0, 1 ); // ????
		flLen = 1 / flLen;
		return Vector( x * flLen, y * flLen, z * flLen );
	}

	constexpr Vector2D Make2D( void ) const
	{
		return {x, y};
	}
	inline float Length2D( void ) const		{ return sqrt( x * x + y * y ); }

	// Members
	float x = 0, y = 0, z = 0;
};
constexpr Vector operator*( float fl, const Vector& v ) { return v * fl; }
constexpr float DotProduct( const Vector& a, const Vector& b ) { return( a.x * b.x + a.y * b.y + a.z * b.z); }
constexpr Vector CrossProduct( const Vector& a, const Vector& b ) { return Vector( a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x ); }
#endif
