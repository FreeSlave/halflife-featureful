#pragma
#ifndef GRAPHIC_DEBUG_H
#define GRAPHIC_DEBUG_H

#include "vector.h"
#include "template_property_types.h"

void DrawBeamLine(const Vector& vecStart, const Vector& vecEnd, int r, int g, int b, int life = 10, int width = 16);
void DrawBeamLine(const Vector& vecStart, const Vector& vecEnd, const Color3& color, int life = 10, int width = 16);

#endif
