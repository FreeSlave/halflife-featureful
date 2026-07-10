#include <algorithm>

#include "inventory_hud.h"

#include "color_utils.h"
#include "string_utils.h"
#include "json_utils.h"

using namespace rapidjson;

const char hudInventorySchema[] = R"(
{
	"type": "object",
	"definitions": {
		"item": {
			"type": "object",
			"properties": {
				"sprite": {
					"type": ["string", "null"]
				},
				"color": {
					"$ref": "definitions.json#/color"
				},
				"alpha": {
					"$ref": "definitions.json#/alpha"
				},
				"position": {
					"type": "string",
					"pattern": "^topleft|status|bottom|topright|hide$"
				},
				"show_in_history": {
					"type": "boolean"
				},
				"show_in_journal": {
					"type": "boolean"
				},
				"show_count_when_one": {
					"type": "boolean"
				}
			},
			"additionalProperties": false
		}
	},
	"properties": {
		"default_sprite_alpha": "definitions.json#/alpha",
		"sprite_alpha": "definitions.json#/alpha",
		"text_alpha": "definitions.json#/alpha",
		"items": {
			"type": "object",
			"additionalProperties": {
				"$ref" : "#/definitions/item"
			}
		},
		"special_items": {
			"type": "object",
			"properties": {
				"item_antidote": {
					"$ref" : "#/definitions/item"
				},
				"item_radiation": {
					"$ref" : "#/definitions/item"
				},
				"item_adrenaline": {
					"$ref" : "#/definitions/item"
				}
			},
			"additionalProperties": false
		}
	}
}
)";

const char* InventoryHudSpec::Schema() const
{
	return hudInventorySchema;
}

bool InventoryHudSpec::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	auto parseHudSpec = [](InventoryItemHudSpec& item, const Value& value)
	{
		HandleJSONMember(value, "sprite", [&item](const Value& value) {
			if (value.IsNull())
			{
				item.spriteName = item.itemName.c_str();
			}
			else
			{
				item.spriteName = value.GetString();
			}
		});

		Color3 color;
		if (UpdatePropertyFromJson(color, value, "color"))
		{
			item.packedColor = PackRGB(color.r, color.g, color.b);
		}

		UpdatePropertyFromJson(item.alpha, value, "alpha");
		UpdatePropertyFromJson(item.showInHistory, value, "show_in_history");
		UpdatePropertyFromJson(item.showInJournal, value, "show_in_journal");
		UpdatePropertyFromJson(item.showCountWhenOne, value, "show_count_when_one");

		HandleJSONMember(value, "position", [&item](const Value& value) {
			const char* positionStr = value.GetString();
			if (strcmp(positionStr, "topleft") == 0 || strcmp(positionStr, "status") == 0)
			{
				item.position = INVENTORY_PLACE_TOP_LEFT;
			}
			else if (strcmp(positionStr, "topright") == 0)
			{
				item.position = INVENTORY_PLACE_TOP_RIGHT;
			}
			else if (strcmp(positionStr, "bottom") == 0)
			{
				item.position = INVENTORY_PLACE_BOTTOM_CENTER;
			}
			else if (strcmp(positionStr, "hide") == 0)
			{
				item.position = INVENTORY_PLACE_HIDE;
			}
		});
	};

	auto itemsIt = document.FindMember("items");
	if (itemsIt != document.MemberEnd())
	{
		const Value& items = itemsIt->value;

		for (auto itemIt = items.MemberBegin(); itemIt != items.MemberEnd(); ++itemIt)
		{
			InventoryItemHudSpec item;
			item.itemName = itemIt->name.GetString();
			parseHudSpec(item, itemIt->value);
			inventory.push_back(item);
		}

		std::sort(inventory.begin(), inventory.end(), [](const InventoryItemHudSpec& a, const InventoryItemHudSpec& b) {
			return strcmp(a.itemName.c_str(), b.itemName.c_str()) < 0;
		});
	}

	HandleJSONMember(document, "special_items", [this, &parseHudSpec](const Value& value) {
		HandleJSONMember(value, "item_antidote", [this, &parseHudSpec](const Value& value) {
			parseHudSpec(antidote, value);
		});
		HandleJSONMember(value, "item_radiation", [this, &parseHudSpec](const Value& value) {
			parseHudSpec(radiation, value);
		});
		HandleJSONMember(value, "item_adrenaline", [this, &parseHudSpec](const Value& value) {
			parseHudSpec(adrenaline, value);
		});
	});

	UpdatePropertyFromJson(defaultSpriteAlpha, document, "default_sprite_alpha");
	UpdatePropertyFromJson(defaultSpriteAlpha, document, "sprite_alpha");
	UpdatePropertyFromJson(textAlpha, document, "text_alpha");

	return true;
}

struct InventoryItemCompare
{
	bool operator ()(const InventoryItemHudSpec& lhs, const char* rhs)
	{
		return strcmp(lhs.itemName.c_str(), rhs) < 0;
	}
	bool operator ()(const char* lhs, const InventoryItemHudSpec& rhs)
	{
		return strcmp(lhs, rhs.itemName.c_str()) < 0;
	}
};

const InventoryItemHudSpec* InventoryHudSpec::GetInventoryItemSpec(const char *itemName)
{
	auto result = std::equal_range(inventory.begin(), inventory.end(), itemName, InventoryItemCompare());
	if (result.first != result.second)
		return &(*result.first);
	return nullptr;
}
