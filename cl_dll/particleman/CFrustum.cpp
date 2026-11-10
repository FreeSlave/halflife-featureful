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

//The triangle API uses OpenGL constants for certain functions.
#include "windows_lean.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#include <OpenGLES/ES1/gl.h>
#else
#include <OpenGL/gl.h>
#endif //TARGET_OS_IOS
#elif __ANDROID__
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif //__APPLE__

#include "cl_util.h"
#include "triangleapi.h"
#include "CFrustum.h"

//TODO: this function always operates on the frustum matrix that's part of this object, so there is no need to pass the address.
void CFrustum::NormalizeFrustumPlane(float frustum[6][4], int side)
{
	auto& elements = frustum[side];

	const float length = Vector{elements[0], elements[1], elements[2]}.Length();

	elements[0] /= length;
	elements[1] /= length;
	elements[2] /= length;
	elements[3] /= length;
}

void CFrustum::CalculateFrustum()
{
	float proj[16];
	gEngfuncs.pTriAPI->GetMatrix(GL_PROJECTION_MATRIX, proj);

	float modl[16];
	gEngfuncs.pTriAPI->GetMatrix(GL_MODELVIEW_MATRIX, modl);

	float frustum[16]{};

	for (int c = 0; c < 4; ++c)
	{
		for (int r = 0; r < 4; ++r)
		{
			float sum = 0;
			for (int k = 0; k < 4; ++k)
				sum += proj[k*4 + r] * modl[c*4 + k];
			frustum[c*4 + r] = sum;
		}
	}

	g_flFrustum[RIGHT][0] = frustum[3] - frustum[0];
	g_flFrustum[RIGHT][1] = frustum[7] - frustum[4];
	g_flFrustum[RIGHT][2] = frustum[11] - frustum[8];
	g_flFrustum[RIGHT][3] = frustum[15] - frustum[12];

	NormalizeFrustumPlane(g_flFrustum, RIGHT);

	g_flFrustum[LEFT][0] = frustum[0] + frustum[3];
	g_flFrustum[LEFT][1] = frustum[4] + frustum[7];
	g_flFrustum[LEFT][2] = frustum[8] + frustum[11];
	g_flFrustum[LEFT][3] = frustum[12] + frustum[15];
	NormalizeFrustumPlane(g_flFrustum, LEFT);

	g_flFrustum[BOTTOM][0] = frustum[1] + frustum[3];
	g_flFrustum[BOTTOM][1] = frustum[5] + frustum[7];
	g_flFrustum[BOTTOM][2] = frustum[9] + frustum[11];
	g_flFrustum[BOTTOM][3] = frustum[15] + frustum[13];
	NormalizeFrustumPlane(g_flFrustum, BOTTOM);

	g_flFrustum[TOP][0] = frustum[3] - frustum[1];
	g_flFrustum[TOP][1] = frustum[7] - frustum[5];
	g_flFrustum[TOP][2] = frustum[11] - frustum[9];
	g_flFrustum[TOP][3] = frustum[15] - frustum[13];
	NormalizeFrustumPlane(g_flFrustum, TOP);

	g_flFrustum[BACK][0] = frustum[3] - frustum[2];
	g_flFrustum[BACK][1] = frustum[7] - frustum[6];
	g_flFrustum[BACK][2] = frustum[11] - frustum[10];
	g_flFrustum[BACK][3] = frustum[15] - frustum[14];
	NormalizeFrustumPlane(g_flFrustum, BACK);

	g_flFrustum[FRONT][0] = frustum[2] + frustum[3];
	g_flFrustum[FRONT][1] = frustum[6] + frustum[7];
	g_flFrustum[FRONT][2] = frustum[10] + frustum[11];
	g_flFrustum[FRONT][3] = frustum[14] + frustum[15];
	NormalizeFrustumPlane(g_flFrustum, FRONT);
}

bool CFrustum::PointInsideFrustum(float x, float y, float z)
{
	for (int i = 0; i < 6; ++i)
	{
		if (((g_flFrustum[i][0] * x) + (g_flFrustum[i][1] * y) + (g_flFrustum[i][2] * z) + g_flFrustum[i][3]) <= 0)
		{
			return false;
		}
	}

	return true;
}

bool CFrustum::SphereInsideFrustum(float x, float y, float z, float radius)
{
	for (int i = 0; i < 6; ++i)
	{
		if (((g_flFrustum[i][0] * x) + (g_flFrustum[i][1] * y) + (g_flFrustum[i][2] * z) + g_flFrustum[i][3]) <= -radius)
		{
			return false;
		}
	}

	return true;
}

bool CFrustum::PlaneInsideFrustum(float x, float y, float z, float size)
{
	const float sizes[3][2] =
		{
			{x - size, x + size},
			{y - size, y + size},
			{z - size, z + size}};

	for (int i = 0; i < 6; ++i)
	{
		bool anyCornerInside = false;

		//Test every permutation of box corners against this plane.
		for (int zAxis = 0; zAxis < 2 && !anyCornerInside; ++zAxis)
		{
			for (int yAxis = 0; yAxis < 2 && !anyCornerInside; ++yAxis)
			{
				for (int xAxis = 0; xAxis < 2; ++xAxis)
				{
					if (((sizes[0][xAxis] * g_flFrustum[i][0]) + (sizes[1][yAxis] * g_flFrustum[i][1]) + (sizes[2][zAxis] * g_flFrustum[i][2]) + g_flFrustum[i][3]) > 0)
					{
						anyCornerInside = true;
						break;
					}
				}
			}
		}

		//All 8 corners are behind this plane, so the box cannot be visible.
		if (!anyCornerInside)
		{
			return false;
		}
	}

	return true;
}
