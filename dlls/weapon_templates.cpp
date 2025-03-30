#include "weapon_templates.h"

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

bool WeaponTemplateSystem::ReadFromDocument(Document &document, const char *fileName)
{
	for (auto weaponIt = document.MemberBegin(); weaponIt != document.MemberEnd(); ++weaponIt)
	{
		Value& value = weaponIt->value;
		const char* name = weaponIt->name.GetString();
		WeaponTemplate weaponTemplate;

		{
			int maxClip;
			if (UpdatePropertyFromJson(maxClip, value, "max_clip"))
			{
				weaponTemplate.SetMaxClip(maxClip);
			}
		}

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
