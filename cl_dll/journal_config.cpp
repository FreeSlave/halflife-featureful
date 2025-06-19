#include "journal_config.h"
#include "json_utils.h"

using namespace rapidjson;

const char* journalConfiggSchema = R"(
{
	"type": "object",
	"properties": {
		"sections": {
			"type": "object",
			"additionalProperties": {
				"type": "object",
				"properties": {
					"header": {
						"type": "string"
					},
					"notification": {
						"type": "string"
					},
					"notification_right": {
						"type": "string"
					},
					"sound": {
						"type": ["string", "boolean"]
					},
					"show_inventory": {
						"type": "boolean"
					},
					"always_show": {
						"type": "boolean"
					}
				}
			}
		}
	},
	"additionalProperties": false
}
)";

const char* JournalConfig::Schema() const
{
	return journalConfiggSchema;
}

bool JournalConfig::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	HandleJSONMember(document, "sections", [this](const Value& value) {
		for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
		{
			Section section;
			section.name = it->name.GetString();
			UpdatePropertyFromJson(section.header, it->value, "header");
			UpdatePropertyFromJson(section.notification, it->value, "notification");
			UpdatePropertyFromJson(section.notificationRight, it->value, "notification_right");

			HandleJSONMember(it->value, "sound", [&section](const Value& value){
				if (value.IsBool())
				{
					if (value.GetBool())
						section.notificationSound = "misc/talk.wav";
				}
				else if (value.IsString())
				{
					section.notificationSound = value.GetString();
				}
			});

			UpdatePropertyFromJson(section.showInventory, it->value, "show_inventory");
			UpdatePropertyFromJson(section.alwaysShow, it->value, "always_show");
			sections.push_back(section);
		}
	});
	return true;
}
