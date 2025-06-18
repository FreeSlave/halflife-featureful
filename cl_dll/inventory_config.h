#pragma once
#ifndef HUD_INVENTORY_H
#define HUD_INVENTORY_H

#include <string>
#include <vector>
#include "json_config.h"

#define INVENTORY_PLACE_HIDE -1
#define INVENTORY_PLACE_DEFAULT 0
#define INVENTORY_PLACE_TOP_LEFT 1
#define INVENTORY_PLACE_TOP_RIGHT 2
#define INVENTORY_PLACE_BOTTOM_CENTER 3

struct InventoryItemHudSpec
{
	InventoryItemHudSpec();
	std::string itemName;
	char spriteName[24];
	int packedColor;
	int alpha;
	int position;
	bool colorDefined;
	bool showInHistory;
};

class InventoryHudSpec : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	InventoryHudSpec();
	const InventoryItemHudSpec* GetInventoryItemSpec(const char* itemName);

	int DefaultSpriteAlpha() const { return defaultSpriteAlpha; }
	int TextAlpha() const { return textAlpha; }
private:
	std::vector<InventoryItemHudSpec> inventory;
	int defaultSpriteAlpha;
	int textAlpha;
};

#endif
