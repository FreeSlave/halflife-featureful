#include <algorithm>
#include <vector>

#include "inventory.h"
#include "string_utils.h"
#include "json_utils.h"

using namespace rapidjson;

const char inventorySpecSchema[] = R"(
{
	"type": "object",
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

InventoryItemSpec::InventoryItemSpec(): maxCount(0) {}

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
