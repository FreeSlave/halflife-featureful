#pragma once
#ifndef INVENTORY_H
#define INVENTORY_H

#include <string>
#include "json_config.h"
#include "ent_templates.h"

struct InventoryItemSpec
{
	InventoryItemSpec();
	std::string itemName;
	int maxCount;
	std::string pickupEntTemplateName;
};

class InventorySpec : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	void SetEntTemplateSystem(EntTemplateSystem* entTemplateSystem) {
		_entTemplateSystem = entTemplateSystem;
	}
	const InventoryItemSpec* GetInventoryItemSpec(const char* itemName);
private:
	std::vector<InventoryItemSpec> inventory;
	EntTemplateSystem* _entTemplateSystem = nullptr;
};

extern InventorySpec g_InventorySpec;

#endif
