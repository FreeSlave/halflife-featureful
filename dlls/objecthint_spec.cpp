#include "objecthint_spec.h"

#include "json_utils.h"
#include "error_collector.h"

using namespace rapidjson;

const char objectHintCatalogSchema[] = R"(
{
	"type": "object",
	"definitions": {
		"visual": {
			"type": "object",
			"properties": {
				"sprite": {
					"type": "string",
					"pattern": "^.+\\.spr$"
				},
				"color": {
					"$ref": "definitions.json#/color"
				},
				"scale": {
					"type": "number",
					"exclusiveMinimum": 0
				}
			},
			"additionalProperties": false
		},
		"visual_set": {
			"type": "object",
			"properties": {
				"default": {
					"type": ["string", "null"]
				},
				"unusable": {
					"type": ["string", "null"]
				},
				"locked": {
					"type": ["string", "null"]
				}
			},
			"additionalProperties": false
		},
		"objecthint_spec": {
			"type": "object",
			"properties": {
				"distance": {
					"type": "number",
					"exclusiveMinimum": 0
				},
				"scan": {
					"$ref": "#/definitions/visual_set"
				},
				"interaction": {
					"$ref": "#/definitions/visual_set"
				},
				"vertical_offset": {
					"type": "number"
				}
			},
			"additionalProperties": false
		}
	},
	"properties": {
		"visuals": {
			"type": "object",
			"additionalProperties": {
				"$ref": "#/definitions/visual"
			}
		},
		"templates": {
			"type": "object",
			"additionalProperties": {
				"$ref": "#/definitions/objecthint_spec"
			}
		},
		"entity_mapping": {
			"type": "object",
			"additionalProperties": {
				"type": "string"
			}
		},
		"pickup_mapping": {
			"type": "object",
			"additionalProperties": {
				"type": "string"
			}
		}
	},
	"additionalProperties": false
}
)";

const ObjectHintSpec* ObjectHintCatalog::GetSpec(const char* name)
{
	_temp = name;
	return GetSpec(_temp);
}

const ObjectHintSpec* ObjectHintCatalog::GetSpec(const std::string& name)
{
	auto it = _templates.find(name);
	return it != _templates.end() ? &it->second : nullptr;
}

const ObjectHintSpec* ObjectHintCatalog::GetSpecByEntityName(const char* name)
{
	_temp = name;
	auto it = _entityMapping.find(_temp);
	return it != _entityMapping.end() ? GetSpec(it->second) : nullptr;
}

const ObjectHintSpec* ObjectHintCatalog::GetSpecByPickupName(const char* name)
{
	_temp = name;
	auto it = _pickupMapping.find(_temp);
	return it != _pickupMapping.end() ? GetSpec(it->second) : nullptr;
}

float ObjectHintCatalog::GetMaxDistance() const
{
	return _maxDistance;
}

bool ObjectHintCatalog::HasAnyTemplates() const
{
	return !_templates.empty();
}

ObjectHintVisual DefaultObjectHintVisual()
{
	ObjectHintVisual visual;
	visual.color = Color3(0, 0, 0);
	visual.scale = 1.0f;
	return visual;
}

ObjectHintSpec DefaultObjectHintSpec()
{
	return ObjectHintSpec();
}

static void ReportUndefinedTemplate(const char* fileName, const char* subject, const char* templateName)
{
	g_errorCollector.AddFormattedError("%s: %s refers to the template '%s' which is not defined", fileName, subject, templateName);
}

const char* ObjectHintCatalog::Schema() const
{
	return objectHintCatalogSchema;
}

bool ObjectHintCatalog::ReadFromDocument(rapidjson::Document& document, const char* fileName)
{
	{
		auto it = document.FindMember("visuals");
		if (it != document.MemberEnd())
		{
			Value& visualsObj = it->value;
			for (auto visualIt = visualsObj.MemberBegin(); visualIt != visualsObj.MemberEnd(); ++visualIt)
			{
				Value& value = visualIt->value;

				ObjectHintVisual visual = DefaultObjectHintVisual();
				UpdatePropertyFromJson(visual.sprite, value, "sprite");
				UpdatePropertyFromJson(visual.color, value, "color");
				UpdatePropertyFromJson(visual.scale, value, "scale");
				_visuals[visualIt->name.GetString()] = visual;
			}
		}
	}

	_maxDistance = 0.0f;

	auto readHintVisualSet = [this, fileName](ObjectHintVisualSet& visualSet, Value& value, const char* templateName)
	{
		auto findHintVisual = [this, fileName, templateName](const std::string& name) -> const ObjectHintVisual*
		{
			auto visualIt = _visuals.find(name);
			if (visualIt != _visuals.end())
				return &visualIt->second;

			g_errorCollector.AddFormattedError("%s: template '%s' refers to the visual description '%s' which is not defined", fileName, templateName, name.c_str());
			return nullptr;
		};

		std::string defaultVisual, unusableVisual, lockedVisual;
		if (UpdatePropertyFromJson(defaultVisual, value, "default"))
		{
			if (!defaultVisual.empty())
				visualSet.defaultVisual = findHintVisual(defaultVisual);
		}

		if (UpdatePropertyFromJson(unusableVisual, value, "unusable"))
		{
			if (!unusableVisual.empty())
				visualSet.unusableVisual = findHintVisual(unusableVisual);
		}
		else
		{
			visualSet.unusableVisual = visualSet.defaultVisual;
		}

		if (UpdatePropertyFromJson(lockedVisual, value, "locked"))
		{
			if (!lockedVisual.empty())
				visualSet.lockedVisual = findHintVisual(lockedVisual);
		}
		else
		{
			visualSet.lockedVisual = visualSet.defaultVisual;
		}
	};

	{
		auto it = document.FindMember("templates");
		if (it != document.MemberEnd())
		{
			Value& templatesObj = it->value;
			for (auto templateIt = templatesObj.MemberBegin(); templateIt != templatesObj.MemberEnd(); ++templateIt)
			{
				Value& value = templateIt->value;
				const char* templateName = templateIt->name.GetString();

				ObjectHintSpec spec = DefaultObjectHintSpec();

				auto scanIt = value.FindMember("scan");
				if (scanIt != value.MemberEnd())
				{
					readHintVisualSet(spec.scanVisualSet, scanIt->value, templateName);
				}
				auto interactionIt = value.FindMember("interaction");
				if (interactionIt != value.MemberEnd())
				{
					readHintVisualSet(spec.interactionVisualSet, interactionIt->value, templateName);
				}

				UpdatePropertyFromJson(spec.distance, value, "distance");
				UpdatePropertyFromJson(spec.verticalOffset, value, "vertical_offset");

				_templates[templateIt->name.GetString()] = spec;

				if (spec.distance > _maxDistance)
					_maxDistance = spec.distance;
			}
		}
	}

	{
		auto it = document.FindMember("entity_mapping");
		if (it != document.MemberEnd())
		{
			Value& mappingsObj = it->value;
			for (auto mappingIt = mappingsObj.MemberBegin(); mappingIt != mappingsObj.MemberEnd(); ++mappingIt)
			{
				std::string entityName = mappingIt->name.GetString();
				std::string templateName = mappingIt->value.GetString();

				auto templateIt = _templates.find(templateName);
				if (templateIt != _templates.end())
					_entityMapping[entityName] = templateName;
				else
					ReportUndefinedTemplate(fileName, "entity_mapping", templateName.c_str());
			}
		}
	}

	{
		auto it = document.FindMember("pickup_mapping");
		if (it != document.MemberEnd())
		{
			Value& mappingsObj = it->value;
			for (auto mappingIt = mappingsObj.MemberBegin(); mappingIt != mappingsObj.MemberEnd(); ++mappingIt)
			{
				std::string pickupName = mappingIt->name.GetString();
				std::string templateName = mappingIt->value.GetString();

				auto templateIt = _templates.find(templateName);
				if (templateIt != _templates.end())
					_pickupMapping[pickupName] = templateName;
				else
					ReportUndefinedTemplate(fileName, "pickup_mapping", templateName.c_str());
			}
		}
	}

	return true;
}

ObjectHintCatalog g_objectHintCatalog;
