#pragma once
#ifndef QUAKE_PALETTE_H
#define QUAKE_PALETTE_H

#include "template_property_types.h"

extern const unsigned char hlPalette[256 * 3];

int ClosestPaletteColorIndex(const Color3 color);

IntRange GetRangeForColorIndex(int colorIndex, int variance);

#endif
