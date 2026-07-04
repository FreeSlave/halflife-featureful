#include <gtest/gtest.h>
#include "inventory.h"
#include "inventory_hud.h"
#include "color_utils.h"

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

const char inventoryHudSpecs[] = R"(
{
	"default_sprite_alpha": 225,
	"text_alpha": 200,
	"items": {
		"battery_blue": {
			"sprite": "item_battery",
			"position": "bottom",
			"color": [0, 100, 255],
			"show_in_journal": false
		},
		"keycard": {
			"sprite": "item_keycard",
			"position": "topleft",
			"color": "204 204 255",
			"show_in_history": false
		},
		"armorvest": {
			"sprite": "item_armorvest",
			"position": "topright"
		}
	}
}
)";

TEST(InventoryHudSpec, Parse)
{
	InventoryHudSpec ihs;

	ASSERT_TRUE(ihs.ReadFromContents(inventoryHudSpecs, ""));

	EXPECT_EQ(ihs.DefaultSpriteAlpha(), 225);
	EXPECT_EQ(ihs.TextAlpha(), 200);

	const InventoryItemHudSpec* batterySpec = ihs.GetInventoryItemSpec("battery_blue");
	ASSERT_TRUE(batterySpec != nullptr);

	EXPECT_STREQ(batterySpec->spriteName.c_str(),  "item_battery");
	EXPECT_TRUE(batterySpec->colorDefined);
	EXPECT_EQ(batterySpec->packedColor, PackRGB(0, 100, 255));
	EXPECT_EQ(batterySpec->position, INVENTORY_PLACE_BOTTOM_CENTER);
	EXPECT_FALSE(batterySpec->showInJournal);

	const InventoryItemHudSpec* keycardSpec = ihs.GetInventoryItemSpec("keycard");
	ASSERT_TRUE(keycardSpec != nullptr);

	EXPECT_STREQ(keycardSpec->spriteName.c_str(),  "item_keycard");
	EXPECT_TRUE(keycardSpec->colorDefined);
	EXPECT_EQ(keycardSpec->packedColor, PackRGB(204, 204, 255));
	EXPECT_EQ(keycardSpec->position, INVENTORY_PLACE_TOP_LEFT);
	EXPECT_FALSE(keycardSpec->showInHistory);

	const InventoryItemHudSpec* armorVestSpec = ihs.GetInventoryItemSpec("armorvest");
	ASSERT_TRUE(armorVestSpec != nullptr);

	EXPECT_STREQ(armorVestSpec->spriteName.c_str(),  "item_armorvest");
	EXPECT_FALSE(armorVestSpec->colorDefined);
	EXPECT_EQ(armorVestSpec->position, INVENTORY_PLACE_TOP_RIGHT);
	EXPECT_TRUE(armorVestSpec->showInHistory);
	EXPECT_TRUE(armorVestSpec->showInJournal);
}
