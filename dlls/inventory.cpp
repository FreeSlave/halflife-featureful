#include <algorithm>
#include <vector>

#include "inventory.h"
#include "string_utils.h"
#include "json_utils.h"

using namespace rapidjson;

const char inventorySpecSchema[] = R"(
{
	"type": "object",
	"definitions": {
		"special_item": {
			"type": "object",
			"properties": {
				"max_count": {
					"type": "integer",
					"minimum": 0
				}
			},
			"additionalProperties": false
		}
	},
	"properties": {
		"items": {
			"additionalProperties": {
				"type": "object",
				"properties": {
					"max_count": {
						"type": "integer",
						"minimum": 0
					},
					"pickup_template": {
						"$ref": "definitions.json#/entity_template"
					}
				},
				"additionalProperties": false
			}
		},
		"special_items": {
			"type": "object",
			"properties": {
				"item_antidote": {
					"$ref": "#/definitions/special_item"
				},
				"item_radiation": {
					"$ref": "#/definitions/special_item"
				},
				"item_adrenaline": {
					"$ref": "#/definitions/special_item"
				}
			},
			"additionalProperties": false
		}
	}
}
)";

struct InventoryItemCompare
{
	bool operator ()(const InventoryItemSpec& lhs, const char* rhs)
	{
		return strcmp(lhs.itemName.c_str(), rhs) < 0;
	}
	bool operator ()(const char* lhs, const InventoryItemSpec& rhs)
	{
		return strcmp(lhs, rhs.itemName.c_str()) < 0;
	}
	bool operator ()(const InventoryItemSpec& lhs, const InventoryItemSpec& rhs)
	{
		return strcmp(lhs.itemName.c_str(), rhs.itemName.c_str()) < 0;
	}
};

const char* InventorySpec::Schema() const
{
	return inventorySpecSchema;
}

bool InventorySpec::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	auto itemsIt = document.FindMember("items");
	if (itemsIt != document.MemberEnd())
	{
		const Value& items = itemsIt->value;
		for (auto itemIt = items.MemberBegin(); itemIt != items.MemberEnd(); ++itemIt)
		{
			InventoryItemSpec item;
			item.itemName = itemIt->name.GetString();
			const Value& value = itemIt->value;
			UpdatePropertyFromJson(item.maxCount, value, "max_count");

			{
				auto it = value.FindMember("pickup_template");
				if (it != value.MemberEnd())
				{
					if (it->value.IsString())
					{
						item.pickupEntTemplateName = it->value.GetString();
					}
					else if (it->value.IsObject())
					{
						item.pickupEntTemplateName = item.itemName + "##pickup_template";
						_entTemplateSystem->AddTemplateFromJsonValue(item.pickupEntTemplateName.c_str(), it->value, fileName);
					}
				}
			}

			inventory.push_back(item);
		}
	}
	HandleJSONMember(document, "special_items", [this](const Value& value) {
		auto parseSpecialItemSpec = [](InventoryItemSpec& item, const Value& value)
		{
			UpdatePropertyFromJson(item.maxCount, value, "max_count");
		};

		HandleJSONMember(value, "item_antidote", [this, &parseSpecialItemSpec](const Value& value) {
			parseSpecialItemSpec(antidote, value);
		});
		HandleJSONMember(value, "item_radiation", [this, &parseSpecialItemSpec](const Value& value) {
			parseSpecialItemSpec(radiation, value);
		});
		HandleJSONMember(value, "item_adrenaline", [this, &parseSpecialItemSpec](const Value& value) {
			parseSpecialItemSpec(adrenaline, value);
		});
	});
	std::sort(inventory.begin(), inventory.end(), InventoryItemCompare());

	return true;
}

const InventoryItemSpec* InventorySpec::GetInventoryItemSpec(const char *itemName)
{
	auto result = std::equal_range(inventory.begin(), inventory.end(), itemName, InventoryItemCompare());
	if (result.first != result.second)
		return &(*result.first);
	return nullptr;
}

InventorySpec g_InventorySpec;
