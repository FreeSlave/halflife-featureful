#include <gtest/gtest.h>

#include "clamp.h"
#include "min_and_max.h"
#include "random_utils.h"
#include "string_utils.h"
#include "template_property_types.h"
#include "tribool.h"

TEST(Clamp, Cases) {
	EXPECT_EQ(clamp(4, 1, 5), 4);
	EXPECT_EQ(clamp(0, 1, 5), 1);
	EXPECT_EQ(clamp(6, 1, 5), 5);
	EXPECT_EQ(clamp(-6, -5, 0), -5);
	EXPECT_EQ(clamp(1, -5, 0), 0);
}

TEST(MinAndMax, Cases) {
	EXPECT_EQ(Q_min(4, 6), 4);
	EXPECT_EQ(Q_min(3.0f, 6), 3);
	EXPECT_EQ(Q_min(2, 2), 2);
	EXPECT_EQ(Q_max(4, 6), 6);
	EXPECT_EQ(Q_max(3.0f, 6), 6);
	EXPECT_EQ(Q_max(2, 2), 2);
}

TEST(StrncpyEnsureTermination, StringLongerThanBuffer) {
	char buf[12];
	strncpyEnsureTermination(buf, "Hello, World!");
	EXPECT_STREQ(buf, "Hello, Worl");
}

TEST(NumberRange, Equal) {
	IntRange r{13, 42};
	IntRange s = 7;

	EXPECT_EQ(r, IntRange(13, 42));
	EXPECT_EQ(r.min, 13);
	EXPECT_EQ(r.max, 42);

	EXPECT_EQ(s, 7);
	EXPECT_EQ(s.min, 7);
	EXPECT_EQ(s.max, 7);
}

TEST(Random, Int) {
	for (int i=0; i<10; ++i) {
		const int low = i;
		const int high = i + 20;

		const int val = RandomInt(low, high);
		EXPECT_TRUE(val >= low);
		EXPECT_TRUE(val <= high);
	}
}

TEST(Random, Float) {
	for (int i=0; i<10; ++i) {
		const float low = 0.0f;
		const float high = 1.0f;

		const float val = RandomFloat(low, high);
		EXPECT_TRUE(val >= low);
		EXPECT_TRUE(val <= high);
	}
}

TEST(Tribool, Test)
{
	tribool u;
	EXPECT_TRUE(indeterminate(u));
	EXPECT_FALSE((bool)u);

	u = true;
	EXPECT_TRUE(u);
	EXPECT_TRUE(u == true);
	u = false;
	EXPECT_FALSE(u);
	EXPECT_TRUE(u == false);

	u = indeterminate;
	EXPECT_TRUE(indeterminate(u));

	tribool t = true;
	tribool f = false;
	EXPECT_TRUE(t);
	EXPECT_FALSE(!t);
	EXPECT_FALSE(f);
	EXPECT_TRUE(!f);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
