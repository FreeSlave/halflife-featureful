#include <gtest/gtest.h>
#include "bucket_preferences.h"

#include <algorithm>
#include <vector>

const char layoutFileContents[] = R"(
// Comment

weapon_shotgun 2 3
weapon_handgrenade 1	2
// Inter comment
weapon_rpg	5 1
weapon_hornetgun 6
weapon_shockrifle	7
)";

TEST(BucketPreferences, Parse)
{
	BucketPreferenceSet layout;
	size_t fileSize = strlen(layoutFileContents);
	ParseBucketPreferences(layout, layoutFileContents, (int)fileSize, "hud_weapon_layout.txt");

	EXPECT_STREQ(layout.list[0].szName, "weapon_shotgun");
	EXPECT_EQ(layout.list[0].iPreferredSlot, 2);
	EXPECT_EQ(layout.list[0].iPreferredSlotPos, 3);

	EXPECT_STREQ(layout.list[1].szName, "weapon_handgrenade");
	EXPECT_EQ(layout.list[1].iPreferredSlot, 1);
	EXPECT_EQ(layout.list[1].iPreferredSlotPos, 2);

	EXPECT_STREQ(layout.list[2].szName, "weapon_rpg");
	EXPECT_EQ(layout.list[2].iPreferredSlot, 5);
	EXPECT_EQ(layout.list[2].iPreferredSlotPos, 1);

	EXPECT_STREQ(layout.list[3].szName, "weapon_hornetgun");
	EXPECT_EQ(layout.list[3].iPreferredSlot, 6);
	EXPECT_EQ(layout.list[3].iPreferredSlotPos, 0);

	EXPECT_STREQ(layout.list[4].szName, "weapon_shockrifle");
	EXPECT_EQ(layout.list[4].iPreferredSlot, 7);
	EXPECT_EQ(layout.list[4].iPreferredSlotPos, 0);
}
