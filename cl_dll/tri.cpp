//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "windows_lean.h"
#include "gl_dynamic.h"

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

int g_iWaterLevel;

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles();
	void DLLEXPORT HUD_DrawTransparentTriangles();
}

void CacheFullbrightModels();
bool m_bCacheFullbrightModels = true;

#include "com_model.h"
#include "particleman.h"
#include "environment.h"

//#define TEST_IT	1
#if TEST_IT

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles()
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if( gHUD.m_hsprCursor == 0 )
	{
		gHUD.m_hsprCursor = SPR_Load( "sprites/cursor.spr" );
	}

	if( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ) )
	{
		return;
	}

	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}
#endif

static void RenderFogImpl(short r, short g, short b, float startDist, float endDist, bool skyboxFog, float density, short type)
{
	// Some default values for different fog modes, in case they're not provided
	if (endDist == 0 && density > 0)
	{
		endDist = 1500;
	}
	else if (endDist > 0 && density == 0)
	{
		density = 0.001f;
	}

	if (gEngfuncs.pTriAPI->FogParams != NULL)
	{
		gEngfuncs.pTriAPI->FogParams(density, skyboxFog);
	}

	float fogColor[] = {(float)r, (float)g, (float)b};
	gEngfuncs.pTriAPI->Fog ( fogColor, startDist, endDist, 1 );

#if OPENGL_AVAILABLE
	int glFogType = 0;

	switch (type) {
	case 1:
		glFogType = GL_EXP;
		break;
	case 2:
		glFogType = GL_EXP2;
		break;
	case 3:
		glFogType = GL_LINEAR;
		break;
	default:
		break;
	}

	if (glFogType != 0 && GL_glFogi != NULL && gHUD.m_iHardwareMode == 1)
	{
		GL_glFogi(GL_FOG_MODE, glFogType);
	}
#endif
}

void RenderFog ()
{
	const FogProperties& fog = gHUD.fog;
	bool bFog = g_iWaterLevel < WL_Eyes && (fog.endDist > 0 || fog.density > 0);
	if (bFog)
	{
		RenderFogImpl(fog.r,fog.g,fog.b, fog.startDist, fog.endDist, fog.affectSkybox, fog.density, fog.type);
	}
	else
	{
		float fogColor[] = {127.0f, 127.0f, 127.0f};
		gEngfuncs.pTriAPI->Fog ( fogColor, 0.0f, 0.0f, 0 );
	}
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
	gHUD.m_Spectator.DrawOverview();
#if TEST_IT
//	Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
#if TEST_IT
//	Draw_Triangles();
#endif

	if (m_bCacheFullbrightModels)
	{
		CacheFullbrightModels();
		m_bCacheFullbrightModels = false;
	}

	if ( g_pParticleMan )
	{
		g_pParticleMan->Update();
		g_Environment.Update();
	}
}
