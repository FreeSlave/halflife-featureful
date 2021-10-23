//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "gl_dynamic.h"

extern int g_iWaterLevel;

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
}

//#define TEST_IT	1
#if TEST_IT

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
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
		char sz[256];
		sprintf( sz, "sprites/cursor.spr" );
		gHUD.m_hsprCursor = SPR_Load( sz );
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

static void RenderFogImpl(short r, short g, short b, float startDist, float endDist, bool skyboxFog)
{
	const bool isXash = IsXashFWGS();
	bool useTriApi = isXash;
#ifdef CLDLL_FOG
	useTriApi = useTriApi || (GL_glFogi != NULL);
#endif

	if (useTriApi)
	{
		float fogColor[] = {(float)r, (float)g, (float)b};
		gEngfuncs.pTriAPI->Fog ( fogColor, startDist, endDist, 1 );
	}

#ifdef CLDLL_FOG
	if (!isXash && GL_glFogi != NULL)
	{
		// Hack for GoldSource, default fog mode does not work for some reason
		GL_glFogi(GL_FOG_MODE, GL_LINEAR);
	}
#endif

	if (gEngfuncs.pTriAPI->FogParams != NULL)
	{
		gEngfuncs.pTriAPI->FogParams(0.0f,skyboxFog);
	}
}

void RenderFog ( void )
{
	const FogProperties& fog = gHUD.fog;
	bool bFog = g_iWaterLevel < 2 && fog.startDist > 0 && fog.endDist > 0;
	if (bFog)
	{
		RenderFogImpl(fog.r,fog.g,fog.b, fog.startDist, fog.endDist, fog.affectSkybox);
	}
	else
	{
		float fogColor[] = {(float)fog.r, (float)fog.g, (float)fog.b};
		gEngfuncs.pTriAPI->Fog ( fogColor, fog.startDist, fog.endDist, 0 );
	}
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	RenderFog();
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
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
#if TEST_IT
//	Draw_Triangles();
#endif
}
