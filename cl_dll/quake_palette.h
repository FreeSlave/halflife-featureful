#pragma once
#ifndef QUAKE_PALETTE_H
#define QUAKE_PALETTE_H

#include "template_property_types.h"

extern const unsigned char quakePalette[256 * 3];

int ClosestPaletteColorIndex(const Color3 color);

#endif
