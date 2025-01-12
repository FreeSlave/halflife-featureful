#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include "followers.h"

const char followers[] = R"(
{
	"fast_recruit_monsters": ["monster_scientist", "monster_barney"],
	"fast_recruit_range": 666
}
)";

TEST(Followers, Parse) {
	FollowersDescription f;
	ASSERT_TRUE(f.ReadFromContents(followers, ""));

	EXPECT_EQ(f.FastRecruitRange(), 666);

	std::array<std::string, 2> a = {"monster_scientist", "monster_barney"};
	EXPECT_TRUE(std::equal(f.RecruitsBegin(), f.RecruitsEnd(), a.cbegin()));
}

const char followersNonunique[] = R"(
{
	"fast_recruit_monsters": ["monster_barney", "monster_scientist", "monster_barney"],
}
)";

TEST(Followers, NonUniqueFollowers) {
	FollowersDescription f;
	ASSERT_FALSE(f.ReadFromContents(followersNonunique, ""));
}
