#pragma once
#ifndef OBJECTHINT_SPEC_H
#define OBJECTHINT_SPEC_H

#include <map>
#include <string>

#include "json_config.h"
#include "spritehint_flags.h"
#include "template_property_types.h"

struct ObjectHintVisual
{
	std::string sprite;
	Color3 color;
	float scale;
};

struct ObjectHintVisualSet
{
	const ObjectHintVisual* defaultVisual = nullptr;
	const ObjectHintVisual* unusableVisual = nullptr;
	const ObjectHintVisual* lockedVisual = nullptr;

	bool HasAnySpriteDefined() const {
		return defaultVisual || unusableVisual || lockedVisual;
	}
};

struct ObjectHintSpec
{
	ObjectHintVisualSet scanVisualSet;
	ObjectHintVisualSet interactionVisualSet;
	float distance = 0.0f;
	float verticalOffset = 0.0f;
};

struct ObjectHintCatalog : public JSONConfig
{
	const ObjectHintSpec* GetSpec(const char* name);
	const ObjectHintSpec* GetSpecByEntityName(const char* name);
	const ObjectHintSpec* GetSpecByPickupName(const char* name);
	float GetMaxDistance() const;
	bool HasAnyTemplates() const;

protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
private:
	const ObjectHintSpec* GetSpec(const std::string& name);

	std::map<std::string, ObjectHintVisual> _visuals;
	std::map<std::string, ObjectHintSpec> _templates;
	std::map<std::string, std::string> _entityMapping;
	std::map<std::string, std::string> _pickupMapping;
	float _maxDistance;
	std::string _temp;
};

extern ObjectHintCatalog g_objectHintCatalog;

#endif
