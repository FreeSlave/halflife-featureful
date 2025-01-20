#include <gtest/gtest.h>
#include "inventory.h"

const char inventorySpecs[] = R"(
{
	"items": {
		"healthkit": {
			"max_count": 4
		},
		"battery_blue": {
			"pickup_template": {
				"own_visual": {
					"model": "models/w_battery.mdl",
					"renderfx": "Glow Shell",
					"color": [0, 0, 255]
				}
			}
		},
		"battery_red": {
			"pickup_template": "red_battery"
		}
	}
}
)";

TEST(InventorySpec, Parse)
{
	SoundScriptSystem ss;
	VisualSystem vs;
	EntTemplateSystem es;
	es.SetSoundScriptSystem(&ss);
	es.SetVisualSystem(&vs);

	InventorySpec is;
	is.SetEntTemplateSystem(&es);

	ASSERT_TRUE(is.ReadFromContents(inventorySpecs, ""));

	{
		const InventoryItemSpec* spec = is.GetInventoryItemSpec("healthkit");
		ASSERT_TRUE(spec != nullptr);

		EXPECT_EQ(spec->maxCount, 4);
	}

	{
		const InventoryItemSpec* spec = is.GetInventoryItemSpec("battery_blue");
		ASSERT_TRUE(spec != nullptr);

		EXPECT_EQ(spec->maxCount, 0);
		EXPECT_EQ(spec->pickupEntTemplateName, "battery_blue##pickup_template");

		const EntTemplate* entTemplate = es.GetTemplate("battery_blue##pickup_template");
		ASSERT_TRUE(entTemplate != nullptr);
	}

	{
		const InventoryItemSpec* spec = is.GetInventoryItemSpec("battery_red");
		ASSERT_TRUE(spec != nullptr);

		EXPECT_EQ(spec->pickupEntTemplateName, "red_battery");
	}
}
