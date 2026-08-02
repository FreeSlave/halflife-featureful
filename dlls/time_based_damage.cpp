#include "time_based_damage.h"
#include "json_utils.h"

using namespace rapidjson;

const char timeBasedDamageSchema[] = R"(
{
	"type": "object",
	"definitions": {
		"damage_type": {
			"type": "object",
			"properties": {
				"tick_count": {
					"type": "integer",
					"minimum": 0,
					"maximum": 255
				},
				"damage_per_tick": {
					"type": "number",
					"minimum": 0
				},
				"interval": {
					"type": "number",
					"minimum": 0
				},
				"first_delay": {
					"type": "number",
					"minimum": 0
				}
			},
			"additionalProperties": false
		}
	},
	"properties": {
		"types": {
			"type": "object",
			"properties": {
				"paralyze": {
					"$ref": "#/definitions/damage_type"
				},
				"nervegas": {
					"$ref": "#/definitions/damage_type"
				},
				"poison": {
					"$ref": "#/definitions/damage_type"
				},
				"radiation": {
					"$ref": "#/definitions/damage_type"
				},
				"acid": {
					"$ref": "#/definitions/damage_type"
				},
				"slowburn": {
					"$ref": "#/definitions/damage_type"
				},
				"slowfreeze": {
					"$ref": "#/definitions/damage_type"
				}
			},
			"additionalProperties": false
		}
	},
	"additionalProperties": false
}
)";

TimeBasedDamageDescription::TimeBasedDamageDescription()
{
	paralyze.tickCount = 3;
	nerveGas.tickCount = 3;

	poison.tickCount = 6;
	poison.damagePerTick = 2.0f;

	radiation.tickCount = 3;
	acid.tickCount = 3;
	slowBurn.tickCount = 3;
	slowFreeze.tickCount = 3;
}

const char* TimeBasedDamageDescription::Schema() const
{
	return timeBasedDamageSchema;
}

bool TimeBasedDamageDescription::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	HandleJSONMember(document, "types", [this](const Value& value) {
		auto handleDamageType = [&](TimeBasedDamageInfo& info, const char* name)
		{
			HandleJSONMember(value, name, [&info](const Value& value) {
				UpdatePropertyFromJson(info.tickCount, value, "tick_count");
				UpdatePropertyFromJson(info.damagePerTick, value, "damage_per_tick");
				UpdatePropertyFromJson(info.interval, value, "interval");
				UpdatePropertyFromJson(info.firstDelay, value, "first_delay");
			});
		};

		handleDamageType(paralyze, "paralyze");
		handleDamageType(nerveGas, "nervegas");
		handleDamageType(poison, "poison");
		handleDamageType(radiation, "radiation");
		handleDamageType(acid, "acid");
		handleDamageType(slowBurn, "slowburn");
		handleDamageType(slowFreeze, "slowfreeze");
	});

	return true;
}

TimeBasedDamageDescription g_timeBasedDamageDescription;
