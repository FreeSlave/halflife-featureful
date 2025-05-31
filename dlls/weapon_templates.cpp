#include "weapon_templates.h"
#include "ammoregistry.h"

#include "json_utils.h"
#include "logger.h"
#include "error_collector.h"

using namespace rapidjson;

const char* weaponTemplatesSchema = R"(
{
	"type": "object",
	"additionalProperties": {
		"type": "object",
		"properties": {
			"max_clip": {
				"type": "integer",
				"minimum": 0
			},
			"ammo_name": {
				"type": "string",
				"minLength": 1
			}
		},
		"additionalProperties": false
	}
}
)";

const char* WeaponTemplateSystem::Schema() const
{
	return weaponTemplatesSchema;
}

bool WeaponTemplateSystem::ReadFromDocument(const Document &document, const char *fileName)
{
	for (auto weaponIt = document.MemberBegin(); weaponIt != document.MemberEnd(); ++weaponIt)
	{
		const Value& value = weaponIt->value;
		const char* name = weaponIt->name.GetString();
		WeaponTemplate weaponTemplate;

		{
			int maxClip;
			if (UpdatePropertyFromJson(maxClip, value, "max_clip"))
			{
				weaponTemplate.SetMaxClip(maxClip);
			}
		}

		HandleJSONMember(value, "ammo_name", [&weaponTemplate, fileName](const Value& value) {
			const char* ammoName = value.GetString();
			if (g_AmmoRegistry.GetByName(ammoName) == nullptr)
			{
				char buf[512];
				safe_snprintf(buf, sizeof(buf), "%s: \"%s\" is not a registered ammo type", fileName, ammoName);
				g_errorCollector.AddError(buf);
			}
			else
			{
				weaponTemplate.SetAmmoName(ammoName);
			}
		});

		_templates[name] = weaponTemplate;
	}

	return true;
}

const WeaponTemplate* WeaponTemplateSystem::GetTemplate(const char *name)
{
	if (!name || *name == '\0')
		return nullptr;
	_temp = name;
	auto it = _templates.find(_temp);
	if (it != _templates.end())
		return &it->second;
	return nullptr;
}

WeaponTemplateSystem g_WeaponTemplateSystem;
