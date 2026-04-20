#include <gtest/gtest.h>
#include "hl_palette.h"

TEST(HLPalette, Closest) {
	EXPECT_EQ(ClosestPaletteColorIndex(Color3(0,0,0)), 0);
	EXPECT_EQ(ClosestPaletteColorIndex(Color3(255,243,27)), 192);
	EXPECT_EQ(ClosestPaletteColorIndex(Color3(55,0,0)), 70);
	EXPECT_EQ(ClosestPaletteColorIndex(Color3(0,0,255)), 208);
	EXPECT_EQ(ClosestPaletteColorIndex(Color3(255,255,255)), 254);
}

TEST(HLPalette, Range) {
	// no variance
	EXPECT_EQ(GetRangeForColorIndex(0, 0), IntRange(0, 0));
	EXPECT_EQ(GetRangeForColorIndex(70, 0), IntRange(70, 70));
	EXPECT_EQ(GetRangeForColorIndex(242, 0), IntRange(242, 242));
	EXPECT_EQ(GetRangeForColorIndex(250, 0), IntRange(250, 250));

	// max variance
	EXPECT_EQ(GetRangeForColorIndex(0, 15), IntRange(0, 15));
	EXPECT_EQ(GetRangeForColorIndex(32, 15), IntRange(32, 47));
	EXPECT_EQ(GetRangeForColorIndex(192, 15), IntRange(192, 207));
	EXPECT_EQ(GetRangeForColorIndex(224, 15), IntRange(224, 239));

	// max variance at boundary
	EXPECT_EQ(GetRangeForColorIndex(41, 15), IntRange(32, 47));
	EXPECT_EQ(GetRangeForColorIndex(31, 15), IntRange(16, 31));

	// not max variance
	EXPECT_EQ(GetRangeForColorIndex(10, 5), IntRange(10, 15));
	EXPECT_EQ(GetRangeForColorIndex(58, 5), IntRange(58, 63));

	// not max variance at boundary
	EXPECT_EQ(GetRangeForColorIndex(10, 10), IntRange(5, 15));
	EXPECT_EQ(GetRangeForColorIndex(30, 10), IntRange(21, 31));

	// over max variance (should be clamped)
	EXPECT_EQ(GetRangeForColorIndex(16, 16), IntRange(16, 31));

	// bright red
	EXPECT_EQ(GetRangeForColorIndex(247, 10), IntRange(247, 251));
	EXPECT_EQ(GetRangeForColorIndex(250, 10), IntRange(247, 251));

	// brights
	EXPECT_EQ(GetRangeForColorIndex(240, 4), IntRange(240, 240));
	EXPECT_EQ(GetRangeForColorIndex(252, 4), IntRange(252, 252));
}
