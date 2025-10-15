#include "player_templates.h"
#include "blood_types.h"
#include "json_utils.h"
#include "logger.h"

using namespace rapidjson;

const char* playerTemplatesSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"type": "object",
		"properties": {
			"ent_template": {
				"$ref": "definitions.json#/entity_template"
			},
			"hud_color": {
				"$ref": "definitions.json#/color"
			},
			"maxspeed": {
				"type": "number",
				"minimum": 0
			},
			"allowed_items": {
				"type": "array",
				"items": {
					"type": "string",
					"minLength": 1
				},
				"uniqueItems": true
			},
			"prohibited_items": {
				"type": "array",
				"items": {
					"type": "string",
					"minLength": 1
				},
				"uniqueItems": true
			},
			"suit_sentences": {
				"type": "boolean"
			},
			"weapons": {
				"type": "object",
				"additionalProperties": {
					"type": "object",
					"properties": {
						"viewmodel": {
							"type": "string",
							"minLength": 5
						},
						"viewmodel_detonator": {
							"type": "string",
							"minLength": 5
						}
					}
				}
			}
		},
		"additionalProperties": false
	}
}
)";

const PlayerTemplate::WeaponReplacement* PlayerTemplate::GetWeaponReplacement(const char* name) const
{
	if (weapons.empty())
		return nullptr;
	_temp = name;
	auto it = weapons.find(_temp);
	if (it != weapons.end())
		return &it->second;
	return nullptr;
}

bool PlayerTemplate::IsItemAllowed(const char *name) const
{
	if (!allowedItems.empty())
	{
		_temp = name;
		return allowedItems.find(_temp) != allowedItems.end();
	}

	if (prohibitedItems.empty())
		return true;
	_temp = name;
	return prohibitedItems.find(_temp) == prohibitedItems.end();
}

const char* PlayerTemplateSystem::Schema() const
{
	return playerTemplatesSchema;
}

bool PlayerTemplateSystem::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	_defaultPlayerTemplate = nullptr;

	for (auto templateIt = document.MemberBegin(); templateIt != document.MemberEnd(); ++templateIt)
	{
		const char* name = templateIt->name.GetString();
		const Value& templateValue = templateIt->value;

		if (!*name)
			name = "default";
		const bool isDefault = stricmp(name, "default") == 0;
		std::string nameStr = name;

		PlayerTemplate playerTemplate;

		HandleJSONMember(templateValue, "ent_template", [&](const Value& value) {
			if (value.IsString())
			{
				playerTemplate.entTemplateName = value.GetString();
			}
			else if (value.IsObject())
			{
				playerTemplate.entTemplateName = nameStr + "##player_template";
				_entTemplateSystem->AddTemplateFromJsonValue(playerTemplate.entTemplateName.c_str(), value, fileName);
			}
		});

		UpdatePropertyFromJson(playerTemplate.hudColor, templateValue, "hud_color");
		UpdatePropertyFromJson(playerTemplate.maxSpeed, templateValue, "maxspeed");
		UpdatePropertyFromJson(playerTemplate.suitSentences, templateValue, "suit_sentences");

		HandleJSONMember(templateValue, "weapons", [&playerTemplate](const Value& value) {
			for (auto weaponIt = value.MemberBegin(); weaponIt != value.MemberEnd(); ++weaponIt)
			{
				const char* weaponName = weaponIt->name.GetString();
				const Value& weaponValue = weaponIt->value;

				PlayerTemplate::WeaponReplacement weapon;
				UpdatePropertyFromJson(weapon.viewModel, weaponValue, "viewmodel");
				UpdatePropertyFromJson(weapon.viewModelDetonator, weaponValue, "viewmodel_detonator");

				playerTemplate.weapons[weaponName] = weapon;
			}
		});

		HandleJSONMember(templateValue, "allowed_items", [&playerTemplate](const Value& value) {
			Value::ConstArray arr = value.GetArray();
			for (auto& item : arr)
			{
				playerTemplate.allowedItems.insert(item.GetString());
			}
		});

		HandleJSONMember(templateValue, "prohibited_items", [&playerTemplate](const Value& value) {
			Value::ConstArray arr = value.GetArray();
			for (auto& item : arr)
			{
				playerTemplate.prohibitedItems.insert(item.GetString());
			}
		});

		_playerTemplates[nameStr] = std::move(playerTemplate);
		if (isDefault)
			_defaultPlayerTemplate = &_playerTemplates[nameStr];
	}

	return true;
}

const PlayerTemplate* PlayerTemplateSystem::GetTemplate(const char* name) const
{
	if (!name || !*name || stricmp(name, "default" ) == 0)
		return _defaultPlayerTemplate;

	auto it = _playerTemplates.find(name);
	if (it != _playerTemplates.end())
		return &it->second;
	return _defaultPlayerTemplate;
}

PlayerTemplateSystem g_PlayerTemplateSystem;
