#include "extdll.h"
#include "util.h"
#include "global_models.h"
#include "graphic_debug.h"

void DrawBeamLine(const Vector& vecStart, const Vector& vecEnd, int r, int g, int b, int life, int width)
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_VECTOR( vecStart );
		WRITE_VECTOR( vecEnd );

		WRITE_SHORT( g_sModelIndexLaser );
		WRITE_BYTE( 0 ); // frame start
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( life ); // life
		WRITE_BYTE( width );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( r );   // r, g, b
		WRITE_BYTE( g );   // r, g, b
		WRITE_BYTE( b );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 10 );		// speed
	MESSAGE_END();
}

void DrawBeamLine(const Vector& vecStart, const Vector& vecEnd, const Color3& color, int life, int width)
{
	DrawBeamLine(vecStart, vecEnd, color.r, color.g, color.b, life, width);
}
