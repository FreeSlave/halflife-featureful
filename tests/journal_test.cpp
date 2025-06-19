#include <gtest/gtest.h>
#include "journal_config.h"

const char journalConfig[] = R"(
{
	"sections": {
		"primary_objective": {
			"header": "PRIMARY_OBJECTIVE_HEADER",
			"notification": "PRIMARY_OBJECTRIVE_UPDATED1",
			"notification_right": "PRIMARY_OBJECTIVE_UPDATED2",
			"sound": true,
			"always_show": true
		},
		"optional_objective": {
			"header": "OPTIONAL_OBJECTIVE_HEADER",
			"sound": "misc/custom.wav"
		},
		"inventory": {
			"header": "INVENTORY_HEADER",
			"show_inventory": true
		}
	}
}

)";

TEST(Journal, Parse)
{
	JournalConfig config;
	ASSERT_TRUE(config.ReadFromContents(journalConfig, ""));

	auto range = config.SectionsRange();

	auto it = range.first;
	ASSERT_NE(it, range.second);

	const auto& primary = *it;
	EXPECT_EQ(primary.name, "primary_objective");
	EXPECT_EQ(primary.header, "PRIMARY_OBJECTIVE_HEADER");
	EXPECT_EQ(primary.notification, "PRIMARY_OBJECTRIVE_UPDATED1");
	EXPECT_EQ(primary.notificationRight, "PRIMARY_OBJECTIVE_UPDATED2");
	EXPECT_FALSE(primary.notificationSound.empty());
	EXPECT_TRUE(primary.alwaysShow);

	++it;
	ASSERT_NE(it, range.second);
	const auto& secondary = *it;
	EXPECT_EQ(secondary.name, "optional_objective");
	EXPECT_EQ(secondary.header, "OPTIONAL_OBJECTIVE_HEADER");
	EXPECT_EQ(secondary.notificationSound, "misc/custom.wav");

	++it;
	ASSERT_NE(it, range.second);
	const auto& inventory = *it;
	EXPECT_EQ(inventory.name, "inventory");
	EXPECT_EQ(inventory.header, "INVENTORY_HEADER");
	EXPECT_TRUE(inventory.showInventory);

	++it;
	ASSERT_EQ(it, range.second);
}
