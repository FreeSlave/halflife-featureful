#include <cassert>
#include <cstdio>
#include <algorithm>
#include <utility>

#include "error_collector.h"
#include "parsetext.h"
#include "color_utils.h"
#include "json_utils.h"
#include "logger.h"

#include "warpball.h"

#include "soundent_bits.h"
#if SERVER_DLL
#include "effects.h"
#include "game.h"
#include "soundent.h"
#endif

static bool AlienTeleportSoundEnabled()
{
#if SERVER_DLL
	return g_modFeatures.alien_teleport_sound;
#else
	return false;
#endif
}

using namespace rapidjson;

const char warpballCatalogSchema[] = R"(
{
	"type": "object",
	"definitions": {
		"sprite_name": {
			"type": "string",
			"pattern": ".+\\.spr"
		},
		"sprite": {
			"type": ["object", "null"],
			"properties": {
				"sprite": {
					"$ref": "#/definitions/sprite_name"
				},
				"framerate": {
					"type": "number",
					"minimum": 0
				},
				"scale": {
					"type": "number",
					"exclusiveMinimum": 0
				},
				"alpha": {
					"type": "integer",
					"minimum": 0,
					"maximum": 255
				},
				"color": {
					"$ref": "#/definitions/color"
				}
			},
			"additionalProperties": false
		},
		"sound": {
			"type": ["object", "null"],
			"properties": {
				"sound": {
					"type": "string",
					"pattern": ".+\\.wav"
				},
				"volume": {
					"type": "number",
					"exclusiveMinimum": 0,
					"maximum": 1.0
				},
				"pitch": {
					"$ref": "definitions.json#/range_int"
				},
				"attenuation": {
					"$ref": "definitions.json#/attenuation"
				}
			},
			"additionalProperties": false
		}
	},
	"properties": {
		"entity_mappings": {
			"type": "object",
			"additionalProperties": {
				"type": "object",
				"additionalProperties": {
					"type": "string",
					"minLength": 1
				}
			}
		},
		"templates": {
			"type": "object",
			"additionalProperties": {
				"type": "object",
				"properties": {
					"inherits": {
						"type": "string"
					},
					"sound1": {
						"$ref": "#/definitions/sound"
					},
					"sound2": {
						"$ref": "#/definitions/sound"
					},
					"sprite1": {
						"$ref": "#/definitions/sprite"
					},
					"sprite2": {
						"$ref": "#/definitions/sprite"
					},
					"beam": {
						"type": "object",
						"properties": {
							"sprite": {
								"$ref": "#/definitions/sprite_name"
							},
							"color": {
								"$ref": "definitions.json#/color"
							},
							"alpha": {
								"$ref": "definitions.json#/alpha"
							},
							"width": {
								"type": "integer",
								"minimum": 1
							},
							"noise": {
								"type": "integer"
							},
							"life": {
								"$ref": "definitions.json#/range"
							}
						},
						"additionalProperties": false
					},
					"beam_radius": {
						"type": "integer",
						"minumum": 1
					},
					"beam_count": {
						"$ref": "definitions.json#/range_int"
					},
					"light": {
						"type": ["object", "null"],
						"properties": {
							"color": {
								"$ref": "definitions.json#/color"
							},
							"radius": {
								"type": "integer"
							},
							"life": {
								"type": "number",
								"minimum": 0
							}
						},
						"additionalProperties": false
					},
					"shake": {
						"$ref": "definitions.json#/shake"
					},
					"ai_sound": {
						"type": ["object", "null"],
						"properties": {
							"type": {
								"type": "string",
								"pattern": "^combat|danger$"
							},
							"duration": {
								"type": "number",
								"minimum": 0
							},
							"radius": {
								"type": "integer",
								"minimum": 0
							}
						},
						"additionalProperties": false
					},
					"spawn_delay": {
						"type": "number",
						"minimum": 0
					},
					"position": {
						"type": ["object", "null"],
						"properties": {
							"vertical_shift": {
								"type": "number"
							}
						},
						"additionalProperties": false
					}
				},
				"additionalProperties": false
			}
		}
	},
	"additionalProperties": false,
	"required": ["templates"]
}
)";

static Color3 DefaultWarpballColor()
{
	return Color3(WARPBALL_RED_DEFAULT, WARPBALL_GREEN_DEFAULT, WARPBALL_BLUE_DEFAULT);
}

static WarpballSprite DefaultWarpballSprite1()
{
	WarpballSprite sprite;
	sprite.sprite = WARPBALL_SPRITE;
	sprite.color = DefaultWarpballColor();
	return sprite;
}

static WarpballSprite DefaultWarpballSprite2()
{
	WarpballSprite sprite;
	sprite.sprite = WARPBALL_SPRITE2;
	sprite.color = DefaultWarpballColor();
	return sprite;
}

static WarpballSound DefaultWarpballSound1()
{
	WarpballSound sound;
	if (AlienTeleportSoundEnabled())
		sound.sound = ALIEN_TELEPORT_SOUND;
	else
		sound.sound = WARPBALL_SOUND1;
	return sound;
}

static WarpballSound DefaultWarpballSound2()
{
	WarpballSound sound;
	if (!AlienTeleportSoundEnabled())
		sound.sound = WARPBALL_SOUND2;
	return sound;
}

static WarpballBeam DefaultWarpballBeam()
{
	WarpballBeam beam;
	beam.sprite = WARPBALL_BEAM;
	beam.color = Color3(WARPBALL_BEAM_RED_DEFAULT, WARPBALL_BEAM_GREEN_DEFAULT, WARPBALL_BEAM_BLUE_DEFAULT);
	return beam;
}

static WarpballTemplate DefaultWarpballTemplate()
{
	WarpballTemplate w;
	w.sound1 = DefaultWarpballSound1();
	w.sound2 = DefaultWarpballSound2();
	w.sprite1 = DefaultWarpballSprite1();
	w.sprite2 = DefaultWarpballSprite2();
	w.beam = DefaultWarpballBeam();
	return w;
}

const char* WarpballTemplateCatalog::Schema() const {
	return warpballCatalogSchema;
}

bool WarpballTemplateCatalog::ReadFromDocument(const Document& document, const char* fileName)
{
	bool fullSuccess = true;

	auto templatesIt = document.FindMember("templates");
	if (templatesIt != document.MemberEnd())
	{
		auto& templates = templatesIt->value;
		for (auto templateIt = templates.MemberBegin(); templateIt != templates.MemberEnd(); ++templateIt)
		{
			if (!AddWarpballTemplate(templates, templateIt->name.GetString(), templateIt->value, fileName))
				fullSuccess = false;
		}
	}

	auto mappingsIt = document.FindMember("entity_mappings");
	if (mappingsIt != document.MemberEnd())
	{
		auto& entityMappings = mappingsIt->value;
		for (auto mappingIt = entityMappings.MemberBegin(); mappingIt != entityMappings.MemberEnd(); mappingIt++)
		{
			const char* mappingName = mappingIt->name.GetString();
			std::map<std::string, std::string> mapping;
			auto& mappingJson = mappingIt->value;
			for (auto pairIt = mappingJson.MemberBegin(); pairIt != mappingJson.MemberEnd(); ++pairIt)
			{
				auto entityName = pairIt->name.GetString();
				auto warpballName = pairIt->value.GetString();
				if (_templates.find(warpballName) == _templates.end())
				{
					g_errorCollector.AddFormattedError("%s: entity mapping '%s' refers to nonexistent template '%s'", fileName, mappingName, warpballName);
				}
				mapping[entityName] = warpballName;
			}
			if (mapping.find("default") == mapping.end())
			{
				g_errorCollector.AddFormattedError("%s: entity mapping '%s' doesn't define 'default' template", fileName, mappingName);
			}
			else
			{
				_entityMappings[mappingName] = mapping;
			}
		}
	}

	return fullSuccess;
}

const WarpballTemplate* WarpballTemplateCatalog::FindWarpballTemplate(const char* warpballName, const char* entityClassname)
{
	return GetWarpballTemplateMutable(warpballName, entityClassname);
}

WarpballTemplate* WarpballTemplateCatalog::GetWarpballTemplateMutable(const char* warpballName, const char* entityClassname)
{
	auto warpballTemplate = GetWarpballTemplateByName(warpballName);
	if (warpballTemplate)
		return warpballTemplate;
	if (entityClassname && *entityClassname)
	{
		auto mappingIt = _entityMappings.find(warpballName);
		if (mappingIt != _entityMappings.end())
		{
			auto& mapping = mappingIt->second;
			auto entityIt = mapping.find(entityClassname);
			if (entityIt != mapping.end())
			{
				warpballTemplate = GetWarpballTemplateByName(entityIt->second.c_str());
				if (warpballTemplate)
					return warpballTemplate;
			}
			auto defaultIt = mapping.find("default");
			if (defaultIt != mapping.end())
			{
				return GetWarpballTemplateByName(defaultIt->second.c_str());
			}
		}
	}
	return nullptr;
}

WarpballTemplate* WarpballTemplateCatalog::GetWarpballTemplateByName(const char* warpballName)
{
	auto it = _templates.find(warpballName);
	if (it != _templates.end())
	{
		return &it->second;
	}
	return nullptr;
}

static void AssignWarpballLight(WarpballLight& light, const Value& lightJson)
{
	if (lightJson.IsNull())
	{
		light = WarpballLight();
	}
	else
	{
		UpdatePropertyFromJson(light.color, lightJson, "color");
		UpdatePropertyFromJson(light.radius, lightJson, "radius");
		UpdatePropertyFromJson(light.life, lightJson, "life");
	}
}

static void AssignWarpballAiSound(WarpballAiSound& aiSound, const Value& aiSoundJson)
{
	if (aiSoundJson.IsNull())
	{
		aiSound = WarpballAiSound();
	}
	else
	{
		UpdatePropertyFromJson(aiSound.radius, aiSoundJson, "radius");
		UpdatePropertyFromJson(aiSound.duration, aiSoundJson, "duration");
		HandleJSONMember(aiSoundJson, "type", [&aiSound](const Value& value) {
			const char* valStr = value.GetString();
			if (stricmp(valStr, "combat") == 0)
			{
				aiSound.type = bits_SOUND_COMBAT;
			}
			else if (stricmp(valStr, "danger") == 0)
			{
				aiSound.type = bits_SOUND_DANGER;
			}
		});
	}
}

static void AssignWarpballPosition(WarpballPosition& pos, const Value& posJson)
{
	if (posJson.IsNull())
	{
		pos = WarpballPosition();
	}
	else
	{
		pos.defined |= UpdatePropertyFromJson(pos.verticalShift, posJson, "vertical_shift");
	}
}

bool WarpballTemplateCatalog::AddWarpballTemplate(const Value& allTemplatesJsonValue, const char* templateName, const Value& templateJsonValue, const char* fileName, std::vector<std::string> inheritanceChain)
{
	if (std::find(inheritanceChain.begin(), inheritanceChain.end(), templateName) != inheritanceChain.end())
	{
		std::string chainString;
		for (auto it = inheritanceChain.begin(); it != inheritanceChain.end(); it++)
		{
			chainString += "'" + *it + "' -> ";
		}
		chainString += "'";
		chainString += templateName;
		chainString += "'";
		g_errorCollector.AddFormattedError("%s: cycle in warpball inheritance detected: %s", fileName, chainString.c_str());
		return false;
	}

	auto existingTemplateIt = _templates.find(templateName);
	if (existingTemplateIt != _templates.end())
	{
		// Already added, has been used as parent for another template
		return true;
	}

	WarpballTemplate warpballTemplate;
	bool inherited = false;
	auto inheritsIt = templateJsonValue.FindMember("inherits");
	if (inheritsIt != templateJsonValue.MemberEnd())
	{
		const char* parentName = inheritsIt->value.GetString();
		existingTemplateIt = _templates.find(parentName);
		if (existingTemplateIt != _templates.end())
		{
			warpballTemplate = existingTemplateIt->second;
			inherited = true;
		}
		else
		{
			auto parentIt = allTemplatesJsonValue.FindMember(parentName);
			if (parentIt != allTemplatesJsonValue.MemberEnd())
			{
				inheritanceChain.push_back(templateName);
				if (AddWarpballTemplate(allTemplatesJsonValue, parentName, parentIt->value, fileName, inheritanceChain))
				{
					existingTemplateIt = _templates.find(parentName);
					if (existingTemplateIt != _templates.end())
					{
						warpballTemplate = existingTemplateIt->second;
						inherited = true;
					}
				}
				else
					return false;
			}
			else
			{
				g_errorCollector.AddFormattedError("%s: couldn't find a parent template '%s' for '%s'", fileName, parentName, templateName);
				return false;
			}
		}
	}
	if (!inherited)
		warpballTemplate = DefaultWarpballTemplate();

	HandleJSONMember(templateJsonValue, "sound1", [&warpballTemplate, this](const Value& value) {
		AssignWarpballSound(warpballTemplate.sound1, value);
	});

	HandleJSONMember(templateJsonValue, "sound2", [&warpballTemplate, this](const Value& value) {
		AssignWarpballSound(warpballTemplate.sound2, value);
	});

	HandleJSONMember(templateJsonValue, "sprite1", [&warpballTemplate, this](const Value& value) {
		AssignWarpballSprite(warpballTemplate.sprite1, value);
	});

	HandleJSONMember(templateJsonValue, "sprite2", [&warpballTemplate, this](const Value& value) {
		AssignWarpballSprite(warpballTemplate.sprite2, value);
	});

	HandleJSONMember(templateJsonValue, "beam", [&warpballTemplate, this](const Value& value) {
		AssignWarpballBeam(warpballTemplate.beam, value);
	});

	UpdatePropertyFromJson(warpballTemplate.beamRadius, templateJsonValue, "beam_radius");
	UpdatePropertyFromJson(warpballTemplate.beamCount, templateJsonValue, "beam_count");

	HandleJSONMember(templateJsonValue, "light", [&warpballTemplate](const Value& value) {
		AssignWarpballLight(warpballTemplate.light, value);
	});

	HandleJSONMember(templateJsonValue, "shake", [&warpballTemplate](const Value& value) {
		UpdatePlayerShake(warpballTemplate.shake, value);
	});

	HandleJSONMember(templateJsonValue, "ai_sound", [&warpballTemplate](const Value& value) {
		AssignWarpballAiSound(warpballTemplate.aiSound, value);
	});

	HandleJSONMember(templateJsonValue, "position", [&warpballTemplate](const Value& value) {
		AssignWarpballPosition(warpballTemplate.position, value);
	});

	UpdatePropertyFromJson(warpballTemplate.spawnDelay, templateJsonValue, "spawn_delay");
	_templates[templateName] = warpballTemplate;
	return true;
}

void WarpballTemplateCatalog::AssignWarpballSound(WarpballSound& sound, const Value &soundJson)
{
	if (soundJson.IsNull())
	{
		sound = WarpballSound();
	}
	else
	{
		UpdateStringFromJson(sound.sound, soundJson, "sound");
		UpdatePropertyFromJson(sound.volume, soundJson, "volume");
		UpdatePropertyFromJson(sound.pitch, soundJson, "pitch");

		HandleJSONMember(soundJson, "attenuation", [&sound](const Value& value) {
			UpdateAttenuationFromJson(sound.attenuation, value);
		});
	}
}

void WarpballTemplateCatalog::AssignWarpballSprite(WarpballSprite& sprite, const rapidjson::Value& spriteJson)
{
	if (spriteJson.IsNull())
	{
		sprite = WarpballSprite();
	}
	else
	{
		UpdateStringFromJson(sprite.sprite, spriteJson, "sprite");
		UpdatePropertyFromJson(sprite.color, spriteJson, "color");
		UpdatePropertyFromJson(sprite.alpha, spriteJson, "alpha");
		UpdatePropertyFromJson(sprite.scale, spriteJson, "scale");
		UpdatePropertyFromJson(sprite.framerate, spriteJson, "framerate");
	}
}

void WarpballTemplateCatalog::AssignWarpballBeam(WarpballBeam& beam, const rapidjson::Value& beamJson)
{
	if (beamJson.IsNull())
	{
		beam = WarpballBeam();
	}
	else
	{
		UpdateStringFromJson(beam.sprite, beamJson, "sprite");
		UpdatePropertyFromJson(beam.color, beamJson, "color");
		UpdatePropertyFromJson(beam.alpha, beamJson, "alpha");
		UpdatePropertyFromJson(beam.width, beamJson, "width");
		UpdatePropertyFromJson(beam.noise, beamJson, "noise");
		UpdatePropertyFromJson(beam.life, beamJson, "life");
	}
}

bool WarpballTemplateCatalog::UpdateStringFromJson(const char*& str, const rapidjson::Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		str = MakeConstantString(it->value.GetString());
		return true;
	}
	return false;
}

const char* WarpballTemplateCatalog::MakeConstantString(const char* str)
{
	auto strIt = _stringSet.find(str);
	if (strIt == _stringSet.end())
	{
		auto p = _stringSet.insert(str);
		strIt = p.first;
	}
	return strIt->c_str();
}

WarpballTemplateCatalog g_WarpballCatalog;

#if SERVER_DLL
static void PrecaheWarpballSprite(WarpballSprite& sprite)
{
	if (sprite.sprite)
	{
		PRECACHE_MODEL(sprite.sprite);
	}
}

static void PrecacheWarpballSound(WarpballSound& sound)
{
	if (sound.sound)
	{
		PRECACHE_SOUND(sound.sound);
	}
}

static void PrecacheWarpballBeam(WarpballBeam& beam)
{
	if (beam.sprite)
	{
		beam.texture = PRECACHE_MODEL(beam.sprite);
	}
}

void WarpballTemplateCatalog::PrecacheWarpballTemplate(const char* name, const char* entityClassname)
{
	WarpballTemplate* w = g_WarpballCatalog.GetWarpballTemplateMutable(name, entityClassname);
	if (w)
	{
		PrecaheWarpballSprite(w->sprite1);
		PrecaheWarpballSprite(w->sprite2);
		PrecacheWarpballSound(w->sound1);
		PrecacheWarpballSound(w->sound2);
		PrecacheWarpballBeam(w->beam);
	}
}

static void PlayWarpballSprite(const WarpballSprite& sprite, const Vector& vecOrigin)
{
	if (sprite.sprite != nullptr)
	{
		CSprite *pSpr = CSprite::SpriteCreate( sprite.sprite, vecOrigin, true );
		pSpr->AnimateAndDie(sprite.framerate);
		pSpr->SetTransparency(sprite.rendermode, sprite.color.r, sprite.color.g, sprite.color.b, sprite.alpha, sprite.renderfx);
		pSpr->SetScale(sprite.scale > 0 ? sprite.scale : 1.0f);
	}
}

static void PlayWarpballSound(const WarpballSound& sound, const Vector& vecOrigin, edict_t* playSoundEnt)
{
	if (sound.sound != nullptr)
	{
		UTIL_EmitAmbientSound(playSoundEnt, vecOrigin, sound.sound, sound.volume, sound.attenuation, 0, RandomizeNumberFromRange(sound.pitch));
	}
}

void PlayWarpballEffect(CBaseEntity* pInitiator, const WarpballTemplate& warpball, const Vector &vecOrigin, edict_t *playSoundEnt)
{
	PlayWarpballSound(warpball.sound1, vecOrigin, playSoundEnt);
	PlayWarpballSound(warpball.sound2, vecOrigin, playSoundEnt);

	if (warpball.shake.IsDefined())
	{
		auto& shake = warpball.shake;
		UTIL_ScreenShake( vecOrigin, shake.amplitude, shake.frequency, shake.duration, shake.radius );
	}

	PlayWarpballSprite(warpball.sprite1, vecOrigin);
	PlayWarpballSprite(warpball.sprite2, vecOrigin);

	if (warpball.light.IsDefined())
	{
		auto& light = warpball.light;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_VECTOR( vecOrigin );
			WRITE_BYTE( (int)(light.radius * 0.1f) );		// radius * 0.1
			WRITE_COLOR( light.color );
			WRITE_BYTE( (int)(light.life * 10) );		// time * 10
			WRITE_BYTE( (int)(light.life * 10 / 2) );		// decay * 0.1
		MESSAGE_END();
	}

	auto& beam = warpball.beam;
	if (beam.texture)
	{
		const int iBeams = RandomizeNumberFromRange(warpball.beamCount);
		BeamParams beamParams;
		beamParams.texture = beam.texture;
		beamParams.lifeMin = beam.life.min * 10;
		beamParams.lifeMax = beam.life.max * 10;
		beamParams.width = beam.width;
		beamParams.noise = beam.noise;
		beamParams.red = beam.color.r;
		beamParams.green = beam.color.g;
		beamParams.blue = beam.color.b;
		beamParams.alpha = beam.alpha;
		DrawChaoticBeams(vecOrigin, nullptr, warpball.beamRadius, beamParams, iBeams);
	}

	if (warpball.aiSound.IsDefined())
	{
		auto& aiSound = warpball.aiSound;
		CSoundEnt::InsertSound(pInitiator, aiSound.type, vecOrigin, aiSound.radius, aiSound.duration);
	}
}

void PlayWarpballEffect(const WarpballTemplate& warpballTemplate, const Vector& vecOrigin, edict_t* playSoundEnt) {
	PlayWarpballEffect(nullptr, warpballTemplate, vecOrigin, playSoundEnt);
}
#endif

static void ReportWarpballSprite(const WarpballSprite& sprite)
{
	if (sprite.sprite == nullptr) {
		LOG("undefined\n");
	} else {
		LOG("'%s'. Color: (%d, %d, %d). Alpha: %d. Scale: %g. Framerate: %g\n",
			sprite.sprite,
			sprite.color.r, sprite.color.g, sprite.color.b, sprite.alpha,
			sprite.scale, sprite.framerate);
	}
}

static void ReportWarpballSound(const WarpballSound& sound)
{
	if (sound.sound == nullptr) {
		LOG("undefined\n");
	} else {
		LOG("'%s'. Volume: %g. Attenuation: %g. Pitch: %d-%d\n", sound.sound, sound.volume, sound.attenuation, sound.pitch.min, sound.pitch.max);
	}
}

static void ReportWarpballBeam(const WarpballBeam& beam)
{
	if (beam.sprite == nullptr) {
		LOG("undefined\n");
	} else {
		LOG("'%s. Color: (%d, %d, %d). Alpha: %d. Width: %d. Noise: %d. Life: %g-%g\n",
			beam.sprite, beam.color.r, beam.color.g, beam.color.b, beam.alpha,
			beam.width, beam.noise, beam.life.min, beam.life.max);
	}
}

static void ReportWarpballLight(const WarpballLight& light)
{
	if (!light.IsDefined()) {
		LOG("undefined\n");
	} else {
		LOG("Color: (%d, %d, %d). Radius: %d. Life: %g\n", light.color.r, light.color.g, light.color.b, light.radius, light.life);
	}
}

static void ReportWarpballShake(const WarpballShake& shake)
{
	if (!shake.IsDefined()) {
		LOG("undefined\n");
	} else {
		LOG("Amplitude: %d. Frequency: %g. Duration: %g. Radius: %d\n", shake.amplitude, shake.frequency, shake.duration, shake.radius);
	}
}

void WarpballTemplateCatalog::DumpWarpballTemplates() const
{
	for (auto it = _templates.begin(); it != _templates.end(); ++it)
	{
		const WarpballTemplate& w = it->second;
		LOG("Warpball '%s'\n", it->first.c_str());

		LOG("Sprite 1: ");
		ReportWarpballSprite(w.sprite1);
		LOG("Sprite 2: ");
		ReportWarpballSprite(w.sprite2);

		LOG("Sound 1: ");
		ReportWarpballSound(w.sound1);
		LOG("Sound 2: ");
		ReportWarpballSound(w.sound2);

		LOG("Beam: ");
		ReportWarpballBeam(w.beam);

		LOG("Beam radius: %d\n", w.beamRadius);
		LOG("Beam count: %d-%d\n", w.beamCount.min, w.beamCount.max);

		LOG("Light: ");
		ReportWarpballLight(w.light);

		LOG("Shake: ");
		ReportWarpballShake(w.shake);

		LOG("Delay before monster spawn: %g\n", w.spawnDelay);
		if (!w.position.IsDefined())
			LOG("Position: default\n");
		else
			LOG("Position: Vertical shift: %g\n", w.position.verticalShift);

		if (w.aiSound.IsDefined())
			LOG("AI sound: Type: %d. Radius: %d. Duration: %g\n", w.aiSound.type, w.aiSound.radius, w.aiSound.duration);

		LOG("\n");
	}
}
