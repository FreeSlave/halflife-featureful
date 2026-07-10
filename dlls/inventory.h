#pragma once
#ifndef INVENTORY_H
#define INVENTORY_H

#include <string>
#include "json_config.h"
#include "ent_templates.h"

struct InventoryItemSpec
{
	std::string itemName;
	int maxCount{0};
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
	const InventoryItemSpec& GetAntidoteSpec() const {
		return antidote;
	}
	const InventoryItemSpec& GetRadcanSpec() const {
		return radiation;
	}
	const InventoryItemSpec& GetAdrenalineSpec() const {
		return adrenaline;
	}
private:
	std::vector<InventoryItemSpec> inventory;
	InventoryItemSpec antidote;
	InventoryItemSpec radiation;
	InventoryItemSpec adrenaline;
	EntTemplateSystem* _entTemplateSystem = nullptr;
};

extern InventorySpec g_InventorySpec;

#endif
