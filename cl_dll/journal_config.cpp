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
				},
				"additionalProperties": false
			}
		},
		"geometry": {
			"type": "object",
			"properties": {
				"width": {
					"type": "number",
					"exclusiveMinimum": 0.0,
					"maximum": 1.0
				},
				"height": {
					"type": "number",
					"exclusiveMinimum": 0.0,
					"maximum": 1.0
				},
				"padding_horizontal": {
					"type": "number",
					"minimum": 0.0,
					"exclusiveMaximum": 0.5
				},
				"padding_vertical": {
					"type": "number",
					"minimum": 0.0,
					"exclusiveMaximum": 0.5
				}
			},
			"additionalProperties": false
		},
		"notification_position": {
			"type": "object",
			"properties": {
				"x": {
					"type": "number",
					"minimum": 0.0,
					"exclusiveMaximum": 1.0
				},
				"y": {
					"type": "number",
					"minimum": 0.0,
					"exclusiveMaximum": 1.0
				}
			},
			"additionalProperties": false
		},
		"render": {
			"type": "object",
			"properties": {
				"text_color": {
					"$ref": "definitions.json#/color"
				},
				"notification_text_color": {
					"$ref": "definitions.json#/color"
				},
				"background_color": {
					"$ref": "definitions.json#/color"
				},
				"background_alpha": {
					"$ref": "definitions.json#/alpha"
				},
				"background_additive": {
					"type": "boolean"
				},
				"frame_color": {
					"$ref": "definitions.json#/color"
				},
				"frame_alpha": {
					"$ref": "definitions.json#/alpha"
				},
				"frame_additive": {
					"type": "boolean"
				}
			},
			"additionalProperties": false
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

	HandleJSONMember(document, "geometry", [this](const Value& value) {
		UpdatePropertyFromJson(geometry.width, value, "width");
		UpdatePropertyFromJson(geometry.height, value, "height");
		UpdatePropertyFromJson(geometry.paddingHorizontal, value, "padding_horizontal");
		UpdatePropertyFromJson(geometry.paddingVertical, value, "padding_vertical");
	});

	HandleJSONMember(document, "notification_position", [this](const Value& value) {
		UpdatePropertyFromJson(notificationPosition.x, value, "x");
		UpdatePropertyFromJson(notificationPosition.y, value, "y");
	});

	HandleJSONMember(document, "render", [this](const Value& value) {
		UpdatePropertyFromJson(render.textColor, value, "text_color");
		UpdatePropertyFromJson(render.notificationTextColor, value, "notification_text_color");
		UpdatePropertyFromJson(render.backgroundColor, value, "background_color");
		UpdatePropertyFromJson(render.backgroundAlpha, value, "background_alpha");
		UpdatePropertyFromJson(render.backgroundBlend, value, "background_additive");
		UpdatePropertyFromJson(render.frameColor, value, "frame_color");
		UpdatePropertyFromJson(render.frameAlpha, value, "frame_alpha");
		UpdatePropertyFromJson(render.frameBlend, value, "frame_additive");
	});

	return true;
}
