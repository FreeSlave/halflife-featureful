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
	},
	"geometry": {
		"width": 0.9,
		"height": 1.0,
		"padding_horizontal": 0.15,
		"padding_vertical": 0.2
	},
	"notification_position": {
		"x": 0.1,
		"y": 0.2
	},
	"render": {
		"text_color": [0, 100, 255],
		"notification_text_color": [100, 255, 0],
		"background_color": [50, 50, 50],
		"background_alpha": 200,
		"background_additive": false,
		"frame_color": [255, 255, 0],
		"frame_alpha": 220,
		"frame_additive": true
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

	auto geometry = config.GetWindowGeometry();
	EXPECT_EQ(geometry.width, 0.9f);
	EXPECT_EQ(geometry.height, 1.0f);
	EXPECT_EQ(geometry.paddingHorizontal, 0.15f);
	EXPECT_EQ(geometry.paddingVertical, 0.2f);

	auto notifiation = config.NotificationPosition();
	EXPECT_EQ(notifiation.x, 0.1f);
	EXPECT_EQ(notifiation.y, 0.2f);

	auto renderProps = config.RenderProps();
	ASSERT_TRUE(renderProps.textColor.has_value());
	EXPECT_EQ(*renderProps.textColor, Color3(0, 100, 255));

	ASSERT_TRUE(renderProps.notificationTextColor.has_value());
	EXPECT_EQ(*renderProps.notificationTextColor, Color3(100, 255, 0));

	EXPECT_EQ(renderProps.backgroundColor, Color3(50, 50, 50));
	EXPECT_EQ(renderProps.backgroundAlpha, 200);
	EXPECT_FALSE(renderProps.backgroundBlend);

	ASSERT_TRUE(renderProps.frameColor.has_value());
	EXPECT_EQ(*renderProps.frameColor, Color3(255, 255, 0));
	EXPECT_EQ(renderProps.frameAlpha, 220);
	EXPECT_TRUE(renderProps.frameBlend);
}
