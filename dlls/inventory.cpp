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
					}
				},
				"additionalProperties": false
			}
		}
	}
}
)";

InventoryItemSpec::InventoryItemSpec(): maxCount(0) {}

const char* InventorySpec::Schema() const
{
	return inventorySpecSchema;
}

bool InventorySpec::ReadFromDocument(rapidjson::Document& document, const char* fileName)
{
	auto itemsIt = document.FindMember("items");
	if (itemsIt != document.MemberEnd())
	{
		Value& items = itemsIt->value;
		for (auto itemIt = items.MemberBegin(); itemIt != items.MemberEnd(); ++itemIt)
		{
			InventoryItemSpec item;
			item.itemName = itemIt->name.GetString();
			Value& value = itemIt->value;
			UpdatePropertyFromJson(item.maxCount, value, "max_count");
			inventory.push_back(item);
		}
	}
	std::sort(inventory.begin(), inventory.end(), [](const InventoryItemSpec& a, const InventoryItemSpec& b) {
		return strcmp(a.itemName.c_str(), b.itemName.c_str()) < 0;
	});

	return true;
}

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
};

const InventoryItemSpec* InventorySpec::GetInventoryItemSpec(const char *itemName)
{
	auto result = std::equal_range(inventory.begin(), inventory.end(), itemName, InventoryItemCompare());
	if (result.first != result.second)
		return &(*result.first);
	return nullptr;
}

InventorySpec g_InventorySpec;
