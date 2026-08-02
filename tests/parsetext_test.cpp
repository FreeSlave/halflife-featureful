#include <gtest/gtest.h>
#include "parsetext.h"
#include "color_utils.h"

TEST(ParseText, Color) {
	int packedColor;
	int packedColor0x;
	int packedColorHtml;

	EXPECT_TRUE(ParseColor("255 8 32", packedColor));
	EXPECT_TRUE(ParseColor("0xFF0820", packedColor0x));
	EXPECT_TRUE(ParseColor("#FF0820", packedColorHtml));

	EXPECT_EQ(packedColor, 0xFF0820);
	EXPECT_EQ(packedColor0x, 0xFF0820);
	EXPECT_EQ(packedColorHtml, 0xFF0820);

	int temp;
	EXPECT_FALSE(ParseColor("string", temp));
	EXPECT_FALSE(ParseColor("0xstring", temp));
	EXPECT_FALSE(ParseColor("#string", temp));
}

TEST(ParseText, Boolean) {
	bool zero, falseBool, FalseBool, noBool, NoBool;
	bool one, trueBool, TrueBool, yesBool, YesBool;

	EXPECT_TRUE(ParseBoolean("0", zero));
	EXPECT_TRUE(ParseBoolean("false", falseBool));
	EXPECT_TRUE(ParseBoolean("False", FalseBool));
	EXPECT_TRUE(ParseBoolean("no", noBool));
	EXPECT_TRUE(ParseBoolean("No", NoBool));

	EXPECT_FALSE(zero);
	EXPECT_FALSE(falseBool);
	EXPECT_FALSE(FalseBool);
	EXPECT_FALSE(noBool);
	EXPECT_FALSE(NoBool);

	EXPECT_TRUE(ParseBoolean("1", one));
	EXPECT_TRUE(ParseBoolean("true", trueBool));
	EXPECT_TRUE(ParseBoolean("True", TrueBool));
	EXPECT_TRUE(ParseBoolean("yes", yesBool));
	EXPECT_TRUE(ParseBoolean("Yes", YesBool));

	EXPECT_TRUE(one);
	EXPECT_TRUE(trueBool);
	EXPECT_TRUE(TrueBool);
	EXPECT_TRUE(yesBool);
	EXPECT_TRUE(YesBool);

	bool temp;
	EXPECT_FALSE(ParseBoolean("arbitrary string", temp));
	EXPECT_FALSE(ParseBoolean("10", temp));
	EXPECT_FALSE(ParseBoolean("2", temp));
}

TEST(ParseText, FloatRange) {
	FloatRange floatRange;
	EXPECT_TRUE(ParseFloatRange("0.5", floatRange));
	EXPECT_EQ(floatRange.min, 0.5f);
	EXPECT_LE(floatRange.max, floatRange.min);

	EXPECT_TRUE(ParseFloatRange("0.25,1.75", floatRange));
	EXPECT_EQ(floatRange.min, 0.25f);
	EXPECT_EQ(floatRange.max, 1.75f);

	EXPECT_TRUE(ParseFloatRange("0.5, 1.5", floatRange));
	EXPECT_EQ(floatRange.min, 0.5f);
	EXPECT_EQ(floatRange.max, 1.5f);

	EXPECT_FALSE(ParseFloatRange("ab", floatRange));
	EXPECT_FALSE(ParseFloatRange("0.5,ab", floatRange));
	EXPECT_FALSE(ParseFloatRange("no comma", floatRange));
	EXPECT_FALSE(ParseFloatRange("nocomma", floatRange));
}

TEST(ParseText, IntRange) {
	IntRange intRange;
	EXPECT_TRUE(ParseIntRange("69", intRange));
	EXPECT_EQ(intRange.min, 69);
	EXPECT_LE(intRange.max, intRange.min);

	EXPECT_TRUE(ParseIntRange("13,72", intRange));
	EXPECT_EQ(intRange.min, 13);
	EXPECT_EQ(intRange.max, 72);

	EXPECT_TRUE(ParseIntRange("27, 50", intRange));
	EXPECT_EQ(intRange.min, 27);
	EXPECT_EQ(intRange.max, 50);

	EXPECT_FALSE(ParseIntRange("ab", intRange));
	EXPECT_FALSE(ParseIntRange("27,ab", intRange));
	EXPECT_FALSE(ParseIntRange("no comma", intRange));
	EXPECT_FALSE(ParseIntRange("nocomma", intRange));
}
