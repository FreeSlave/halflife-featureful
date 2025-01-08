#include <gtest/gtest.h>
#include "objecthint_spec.h"

const char objecthints[] = R"(
{
	"visuals": {
		"base": {
			"sprite": "sprites/hud_brackets.spr"
		},
		"interactable": {
			"color": [255, 200, 50],
			"sprite": "sprites/hud_brackets.spr"
		},
		"charged": {
			"color": [0, 255, 0],
			"sprite": "sprites/hud_charged.spr",
			"scale": 1.2
		},
		"depleted": {
			"color": [255, 0, 0],
			"sprite": "sprites/hud_depleted.spr"
		},
		"lock": {
			"color": [255, 50, 0],
			"sprite": "sprites/hud_brackets.spr"
		}
	},
	"templates": {
		"button": {
			"interaction": {
				"default": "interactable",
				"unusable": null,
				"locked": "lock"
			}
		},
		"charger": {
			"interaction": {
				"default": "charged",
				"unusable": "depleted"
			}
		},
		"pickup": {
			"scan": {
				"default": "base",
				"unusable": null
			},
			"interaction": {
				"default": "interactable",
				"unusable": null
			},
			"distance": 256
		},
		"hidden": {}
	},
	"entity_mapping": {
		"func_button": "button",
		"func_rot_button": "button",
		"func_healthcharger": "charger",
		"func_recharge": "charger",
		"item_pickup": "pickup"
	},
	"pickup_mapping": {
		"charger_item": "charger",
		"button_item": "button"
	}
}
)";

TEST(ObjectHint, Parse) {
	ObjectHintCatalog c;
	ASSERT_TRUE(c.ReadFromContents(objecthints, ""));

	{
		const ObjectHintSpec* buttonSpec = c.GetSpec("button");
		ASSERT_TRUE(buttonSpec != nullptr);
		ASSERT_TRUE(buttonSpec->interactionVisualSet.defaultVisual != nullptr);
		ASSERT_TRUE(buttonSpec->interactionVisualSet.unusableVisual == nullptr);
		ASSERT_TRUE(buttonSpec->interactionVisualSet.lockedVisual != nullptr);
		EXPECT_FALSE(buttonSpec->scanVisualSet.HasAnySpriteDefined());

		EXPECT_EQ(buttonSpec->interactionVisualSet.defaultVisual->color, Color3(255, 200, 50));
		EXPECT_EQ(buttonSpec->interactionVisualSet.defaultVisual->sprite, "sprites/hud_brackets.spr");
		EXPECT_EQ(buttonSpec->interactionVisualSet.defaultVisual->scale, 1.0f);

		EXPECT_EQ(buttonSpec->interactionVisualSet.lockedVisual->color, Color3(255, 50, 0));

		EXPECT_EQ(buttonSpec, c.GetSpecByEntityName("func_button"));
		EXPECT_EQ(buttonSpec, c.GetSpecByEntityName("func_rot_button"));
	}

	{
		const ObjectHintSpec* chargerSpec = c.GetSpec("charger");
		ASSERT_TRUE(chargerSpec != nullptr);
		ASSERT_TRUE(chargerSpec->interactionVisualSet.defaultVisual != nullptr);
		ASSERT_TRUE(chargerSpec->interactionVisualSet.unusableVisual != nullptr);
		EXPECT_EQ(chargerSpec->interactionVisualSet.lockedVisual, chargerSpec->interactionVisualSet.defaultVisual);

		EXPECT_EQ(chargerSpec->interactionVisualSet.defaultVisual->color, Color3(0, 255, 0));
		EXPECT_EQ(chargerSpec->interactionVisualSet.defaultVisual->sprite, "sprites/hud_charged.spr");
		EXPECT_EQ(chargerSpec->interactionVisualSet.defaultVisual->scale, 1.2f);

		EXPECT_EQ(chargerSpec->interactionVisualSet.unusableVisual->color, Color3(255, 0, 0));
		EXPECT_EQ(chargerSpec->interactionVisualSet.unusableVisual->sprite, "sprites/hud_depleted.spr");

		EXPECT_EQ(chargerSpec, c.GetSpecByEntityName("func_healthcharger"));
		EXPECT_EQ(chargerSpec, c.GetSpecByEntityName("func_recharge"));
	}

	{
		const ObjectHintSpec* pickupSpec = c.GetSpec("pickup");
		ASSERT_TRUE(pickupSpec != nullptr);
		ASSERT_TRUE(pickupSpec->interactionVisualSet.defaultVisual != nullptr);
		ASSERT_TRUE(pickupSpec->interactionVisualSet.unusableVisual == nullptr);
		EXPECT_EQ(pickupSpec->interactionVisualSet.lockedVisual, pickupSpec->interactionVisualSet.defaultVisual);

		ASSERT_TRUE(pickupSpec->scanVisualSet.defaultVisual != nullptr);
		ASSERT_TRUE(pickupSpec->scanVisualSet.unusableVisual == nullptr);
		EXPECT_EQ(pickupSpec->scanVisualSet.lockedVisual, pickupSpec->scanVisualSet.defaultVisual);

		EXPECT_EQ(pickupSpec->interactionVisualSet.defaultVisual->color, Color3(255, 200, 50));

		EXPECT_EQ(pickupSpec, c.GetSpecByEntityName("item_pickup"));

		const ObjectHintSpec* item1Spec = c.GetSpecByPickupName("charger_item");
		ASSERT_TRUE(item1Spec != nullptr);
		EXPECT_EQ(item1Spec, c.GetSpec("charger"));

		const ObjectHintSpec* item2Spec = c.GetSpecByPickupName("button_item");
		ASSERT_TRUE(item2Spec != nullptr);
		EXPECT_EQ(item2Spec, c.GetSpec("button"));
	}

	{
		const ObjectHintSpec* hiddenSpec = c.GetSpec("hidden");
		ASSERT_TRUE(hiddenSpec != nullptr);
		EXPECT_FALSE(hiddenSpec->interactionVisualSet.HasAnySpriteDefined());
		EXPECT_FALSE(hiddenSpec->scanVisualSet.HasAnySpriteDefined());
	}
}
