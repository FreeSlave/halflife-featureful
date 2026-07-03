#pragma once
#ifndef INVENTORY_HUD_H
#define INVENTORY_HUD_H

#include <string>
#include <vector>
#include "json_config.h"
#include "fixed_string.h"

#define INVENTORY_PLACE_HIDE -1
#define INVENTORY_PLACE_DEFAULT 0
#define INVENTORY_PLACE_TOP_LEFT 1
#define INVENTORY_PLACE_TOP_RIGHT 2
#define INVENTORY_PLACE_BOTTOM_CENTER 3

struct InventoryItemHudSpec
{
	std::string itemName;
	fixed_string<24> spriteName;
	int packedColor{0};
	int alpha{0};
	int position{INVENTORY_PLACE_DEFAULT};
	bool colorDefined{false};
	bool showInHistory{true};
	bool showInJournal{true};
	bool showCountWhenOne{false};
};

class InventoryHudSpec : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	const InventoryItemHudSpec* GetInventoryItemSpec(const char* itemName);

	int DefaultSpriteAlpha() const { return defaultSpriteAlpha; }
	int TextAlpha() const { return textAlpha; }
private:
	std::vector<InventoryItemHudSpec> inventory;
	int defaultSpriteAlpha{175};
	int textAlpha{225};
};

#endif
