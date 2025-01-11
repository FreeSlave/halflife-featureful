#pragma once
#ifndef INVENTORY_H
#define INVENTORY_H

#include <string>
#include "json_config.h"

struct InventoryItemSpec
{
	InventoryItemSpec();
	std::string itemName;
	int maxCount;
};

class InventorySpec : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(rapidjson::Document& document, const char* fileName) override;
public:
	const InventoryItemSpec* GetInventoryItemSpec(const char* itemName);
private:
	std::vector<InventoryItemSpec> inventory;
};

extern InventorySpec g_InventorySpec;

#endif
