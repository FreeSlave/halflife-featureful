#include <gtest/gtest.h>
#include "savetitles.h"

#include <algorithm>
#include <vector>

const char saveTitlesFileContents[] = R"(
// Comment

featureful "#FEATUREFUL_DEMO"
// Inter comment
flying_demo	"#FLYERS_DEMO"
)";

TEST(SaveTitles, Defaults)
{
	EXPECT_STREQ(GetSaveTitleForMap("c2a1a"), "#C2A1TITLE");
	EXPECT_STREQ(GetSaveTitleForMap("c2a1"), "#C2A1TITLE");
	EXPECT_STREQ(GetSaveTitleForMap("C2A1A"), "#C2A1TITLE");
	EXPECT_STREQ(GetSaveTitleForMap("of1a4b"), "#OF1A3TITLE");
	EXPECT_STREQ(GetSaveTitleForMap("ba_tram1"), "#BA_TRAMTITLE");
}

TEST(SaveTitles, Parse)
{
	size_t fileSize = strlen(saveTitlesFileContents);
	std::vector<unsigned char> saveTitlesContents(fileSize);
	std::copy_n(saveTitlesFileContents, fileSize, saveTitlesContents.begin());

	ReadSaveTitles(saveTitlesContents.data(), (int)fileSize, "save_titles.txt");

	EXPECT_STREQ(GetSaveTitleForMap("featureful_demo"), "#FEATUREFUL_DEMO");
	EXPECT_STREQ(GetSaveTitleForMap("featureful"), "#FEATUREFUL_DEMO");
	EXPECT_STREQ(GetSaveTitleForMap("flying_demo"), "#FLYERS_DEMO");
}
