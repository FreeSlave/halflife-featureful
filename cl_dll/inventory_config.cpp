#include <algorithm>

#include "inventory_config.h"

#include "parsetext.h"
#include "color_utils.h"
#include "string_utils.h"
#include "json_utils.h"

using namespace rapidjson;

const char hudInventorySchema[] = R"(
{
	"type": "object",
	"properties": {
		"default_sprite_alpha": "definitions.json#/alpha",
		"text_alpha": "definitions.json#/alpha",
		"items": {
			"additionalProperties": {
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
					}
				},
				"additionalProperties": false
			}
		}
	}
}
)";

InventoryItemHudSpec::InventoryItemHudSpec(): packedColor(0), alpha(0), position(INVENTORY_PLACE_DEFAULT), colorDefined(false), showInHistory(true)
{
	spriteName[0] = '\0';
}

InventoryHudSpec::InventoryHudSpec(): defaultSpriteAlpha(175), textAlpha(225) {}

const char* InventoryHudSpec::Schema() const
{
	return hudInventorySchema;
}

bool InventoryHudSpec::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	auto itemsIt = document.FindMember("items");
	if (itemsIt != document.MemberEnd())
	{
		const Value& items = itemsIt->value;

		for (auto itemIt = items.MemberBegin(); itemIt != items.MemberEnd(); ++itemIt)
		{
			InventoryItemHudSpec item;
			item.itemName = itemIt->name.GetString();
			const Value& value = itemIt->value;

			HandleJSONMember(value, "sprite", [&item](const Value& value) {
				if (value.IsNull())
				{
					strncpyEnsureTermination(item.spriteName, item.itemName.c_str());
				}
				else
				{
					strncpyEnsureTermination(item.spriteName, value.GetString());
				}
			});

			Color3 color;
			if (UpdatePropertyFromJson(color, value, "color"))
			{
				item.packedColor = PackRGB(color.r, color.g, color.b);
				item.colorDefined = true;
			}

			UpdatePropertyFromJson(item.alpha, value, "alpha");
			UpdatePropertyFromJson(item.showInHistory, value, "show_in_history");

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

			inventory.push_back(item);
		}

		std::sort(inventory.begin(), inventory.end(), [](const InventoryItemHudSpec& a, const InventoryItemHudSpec& b) {
			return strcmp(a.itemName.c_str(), b.itemName.c_str()) < 0;
		});
	}

	UpdatePropertyFromJson(defaultSpriteAlpha, document, "default_sprite_alpha");
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
