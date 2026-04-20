#include "tex_materials.h"
#include "pm_materials.h"
#include "string_utils.h"
#include "error_collector.h"
#include "json_utils.h"
#include "logger.h"
#include "clamp.h"
#include <cstring>

#if CLIENT_DLL
#include "hl_palette.h"
#endif

const char materialsSchema[] = R"(
{
	"type": "object",
	"definitions": {
		"volume": {
			"type": "number",
			"maximum": 1.0,
			"minimum": 0.0
		},
		"step_move_sound_data": {
			"type": "object",
			"properties": {
				"volume": {
					"$ref": "#/definitions/volume"
				},
				"time": {
					"type": "integer",
					"exclusiveMinimum": 0
				}
			}
		},
		"step_sound_array": {
			"type": "array",
			"items": {
				"type": "string"
			},
			"maxItems": 5
		},
		"step": {
			"type": "object",
			"properties": {
				"right": {
					"$ref": "#/definitions/step_sound_array"
				},
				"left": {
					"$ref": "#/definitions/step_sound_array"
				},
				"walking": {
					"$ref": "#/definitions/step_move_sound_data"
				},
				"running": {
					"$ref": "#/definitions/step_move_sound_data"
				},
				"skip_some_steps": {
					"type": "boolean"
				}
			},
			"additionalProperties": false
		},
		"hit_sound_array": {
			"type": "array",
			"items": {
				"type": "string"
			},
			"maxItems": 5
		},
		"hit": {
			"type": "object",
			"properties": {
				"waves": {
					"$ref": "#/definitions/hit_sound_array"
				},
				"volume": {
					"$ref": "#/definitions/volume"
				},
				"volume_bar": {
					"$ref": "#/definitions/volume"
				},
				"attenuation": {
					"$ref": "definitions.json#/attenuation"
				},
				"allow_wallpuff": {
					"type": "boolean"
				},
				"allow_weapon_sparks": {
					"type": "boolean"
				},
				"play_sparks": {
					"type": "boolean"
				},
				"wallpuff_color": {
					"$ref": "definitions.json#/color"
				},
				"impact_particle_color": {
					"$ref": "definitions.json#/color"
				}
			},
			"additionalProperties": false
		},
		"material_name": {
			"type": "string",
			"minLength": 1,
			"maxLength": 1
		}
	},
	"properties": {
		"materials": {
			"type": "object",
			"additionalProperties": {
					"type": "object",
					"properties": {
						"step": {
							"$ref": "#/definitions/step"
						},
						"hit": {
							"$ref": "#/definitions/hit"
						}
					},
					"additionalProperties": false
			}
		},
		"ladder_step": {
			"$ref": "#/definitions/step"
		},
		"wade_step": {
			"$ref": "#/definitions/step"
		},
		"default_material": {
			"$ref": "#/definitions/material_name"
		},
		"default_step_material": {
			"$ref": "#/definitions/material_name"
		},
		"slosh_material": {
			"$ref": "#/definitions/material_name"
		},
		"flesh_material": {
			"$ref": "#/definitions/material_name"
		},
		"wallpuff_color": {
			"$ref": "definitions.json#/color"
		},
		"wallpuff_alpha": {
			"$ref": "definitions.json#/range_int_non_negative"
		},
		"impact_particle_color": {
			"$ref": "definitions.json#/color"
		}
	}
}
)";

void GetStrippedTextureName(char *szbuffer, const char *pTextureName)
{
	// strip leading '-0' or '+0~' or '{' or '!'
	if( *pTextureName == '-' || *pTextureName == '+' )
		pTextureName += 2;

	if( *pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ' )
		pTextureName++;
	// '}}'
	strcpy( szbuffer, pTextureName );
	szbuffer[CBTEXTURENAMEMAX - 1] = 0;
}

constexpr float WalkingStepVolume = 0.2f;
constexpr float RunningStepVolume = 0.5f;

constexpr int WalkingStepTime = 400;
constexpr int RunningStepTime = 300;

constexpr MaterialStepSoundData WalkingStepData = {WalkingStepVolume, WalkingStepTime};
constexpr MaterialStepSoundData RunningStepData = {RunningStepVolume, RunningStepTime};

MaterialStepData::MaterialStepData(): walking(WalkingStepData), running(RunningStepData) {}

void MaterialRegistry::FillDefaults()
{
	_defaultMaterial = CHAR_TEX_CONCRETE;
	_defaultStepMaterial = '\0';
	_sloshMaterial = CHAR_TEX_SLOSH;
	_fleshMaterial = CHAR_TEX_FLESH;

	{
		MaterialData data;
		data.step.SetRight({"player/pl_step1.wav", "player/pl_step3.wav"});
		data.step.SetLeft({"player/pl_step2.wav", "player/pl_step4.wav"});
		data.hit.volume = 0.9f;
		data.hit.volumebar = 0.6f;
		data.hit.SetWaves({"player/pl_step1.wav", "player/pl_step2.wav"});
		data.hit.wallpuffColor = Color3(65, 65, 65);
		SetMaterialData(CHAR_TEX_CONCRETE, data);
	}

	{
		MaterialData data;
		data.step.SetRight({"player/pl_metal1.wav", "player/pl_metal3.wav"});
		data.step.SetLeft({"player/pl_metal2.wav", "player/pl_metal4.wav"});
		data.hit.volume = 0.9f;
		data.hit.volumebar = 0.3f;
		data.hit.SetWaves({"player/pl_metal1.wav", "player/pl_metal2.wav"});
		SetMaterialData(CHAR_TEX_METAL, data);
	}

	{
		MaterialData data;
		data.step.SetRight({"player/pl_dirt1.wav", "player/pl_dirt3.wav"});
		data.step.SetLeft({"player/pl_dirt2.wav", "player/pl_dirt4.wav"});
		data.step.walking = {0.25f, WalkingStepTime};
		data.step.running = {0.55f, RunningStepTime};
		data.hit.volume = 0.9f;
		data.hit.volumebar = 0.1f;
		data.hit.SetWaves({"player/pl_dirt1.wav", "player/pl_dirt2.wav", "player/pl_dirt3.wav"});
		SetMaterialData(CHAR_TEX_DIRT, data);
	}

	{
		MaterialData data;
		data.step.SetRight({"player/pl_duct1.wav", "player/pl_duct3.wav"});
		data.step.SetLeft({"player/pl_duct2.wav", "player/pl_duct4.wav"});
		data.step.walking = {0.4f, WalkingStepTime};
		data.step.running = {0.7f, RunningStepTime};
		data.hit.volume = 0.5f;
		data.hit.volumebar = 0.3f;
		data.hit.SetWaves({"player/pl_duct1.wav"});
		SetMaterialData(CHAR_TEX_VENT, data);
	}

	{
		MaterialData data;
		data.step.SetRight({"player/pl_grate1.wav", "player/pl_grate3.wav"});
		data.step.SetLeft({"player/pl_grate2.wav", "player/pl_grate4.wav"});
		data.hit.volume = 0.9f;
		data.hit.volumebar = 0.5f;
		data.hit.SetWaves({"player/pl_grate1.wav", "player/pl_grate4.wav"});
		SetMaterialData(CHAR_TEX_GRATE, data);
	}

	{
		MaterialData data;
		// repeat sounds to preserve the original chances of pl_tile5.wav
		data.step.SetRight({"player/pl_tile1.wav", "player/pl_tile3.wav", "player/pl_tile1.wav", "player/pl_tile3.wav", "player/pl_tile5.wav"});
		data.step.SetLeft({"player/pl_tile2.wav", "player/pl_tile4.wav", "player/pl_tile2.wav", "player/pl_tile4.wav", "player/pl_tile5.wav"});
		data.hit.volume = 0.8f;
		data.hit.volumebar = 0.2f;
		data.hit.SetWaves({"player/pl_tile1.wav", "player/pl_tile3.wav", "player/pl_tile2.wav", "player/pl_tile4.wav"});
		SetMaterialData(CHAR_TEX_TILE, data);
	}

	{
		MaterialData data;
		data.step.SetRight({"player/pl_slosh1.wav", "player/pl_slosh3.wav"});
		data.step.SetLeft({"player/pl_slosh2.wav", "player/pl_slosh4.wav"});
		data.hit.volume = 0.9f;
		data.hit.volumebar = 0.0f;
		data.hit.SetWaves({"player/pl_slosh1.wav", "player/pl_slosh3.wav", "player/pl_slosh2.wav", "player/pl_slosh4.wav"});
		SetMaterialData(CHAR_TEX_SLOSH, data);
	}

	{
		MaterialData data;
		data.hit.volume = 0.9f;
		data.hit.volumebar = 0.2f;
		data.hit.SetWaves({"debris/wood1.wav", "debris/wood2.wav", "debris/wood3.wav"});
		data.hit.allowWeaponSparks = false;
		data.hit.wallpuffColor = Color3(75, 42, 15);
		SetMaterialData(CHAR_TEX_WOOD, data);
	}

	{
		MaterialData data;
		data.hit.volume = 0.8f;
		data.hit.volumebar = 0.2f;
		data.hit.SetWaves({"debris/glass1.wav", "debris/glass2.wav", "debris/glass3.wav"});
		SetMaterialData(CHAR_TEX_GLASS, data);
		data.hit.playSparks = true;
		SetMaterialData(CHAR_TEX_COMPUTER, data);
	}

	{
		MaterialData data;
		data.hit.volume = 1.0f;
		data.hit.volumebar = 0.2f;
		data.hit.attn = 1.0f;
		data.hit.SetWaves({"weapons/bullet_hit1.wav", "weapons/bullet_hit2.wav"});
		SetMaterialData(CHAR_TEX_FLESH, data);
	}

	{
		MaterialData data;
		data.step.SetRight({"player/pl_snow1.wav", "player/pl_snow3.wav"});
		data.step.SetLeft({"player/pl_snow2.wav", "player/pl_snow4.wav"});
		data.hit.volume = 0.9f;
		data.hit.volumebar = 0.1f;
		data.hit.SetWaves({"player/pl_snow1.wav", "player/pl_snow2.wav", "player/pl_snow3.wav"});
		SetMaterialData(CHAR_TEX_SNOW, data);
		SetMaterialData(CHAR_TEX_SNOW_OPFOR, data);
	}

	{
		MaterialStepData data;
		data.SetRight({"player/pl_ladder1.wav", "player/pl_ladder3.wav"});
		data.SetLeft({"player/pl_ladder2.wav", "player/pl_ladder4.wav"});
		data.walking.volume = 0.35f;
		data.walking.timeMsec = 350;
		data.running = data.walking;
		SetLadderStepData(data);
	}

	{
		MaterialStepData data;
		data.SetRight({"player/pl_wade1.wav", "player/pl_wade2.wav"});
		data.SetLeft({"player/pl_wade3.wav", "player/pl_wade4.wav"});
		data.walking.volume = 0.65f;
		data.running.timeMsec = 600;
		data.skipSomeSteps = true;
		SetWadeStepData(data);
	}
}

void MaterialRegistry::SetMaterialData(char c, const MaterialData &data)
{
	_materials[c] = data;
}

const MaterialData* MaterialRegistry::GetMaterialData(char c) const
{
	auto it = _materials.find(c);
	return it != _materials.end() ? &it->second : nullptr;
}

const MaterialData* MaterialRegistry::GetMaterialDataWithFallback(char c) const
{
	const MaterialData* mData = GetMaterialData(c);
	if (!mData && c != _defaultMaterial)
		mData = GetMaterialData(_defaultMaterial);
	return mData;
}

const MaterialStepData* MaterialRegistry::GetMaterialStepData(char c) const
{
	const MaterialData* mData = GetMaterialData(c);
	if (!mData)
	{
		mData = GetMaterialData(_defaultMaterial);
	}
	if (!mData || !mData->step.IsDefined())
	{
		const char stepMaterialType = _defaultStepMaterial != '\0' ? _defaultStepMaterial : _defaultMaterial;
		const MaterialData* defaultStepData = GetMaterialData(stepMaterialType);
		return defaultStepData ? &defaultStepData->step : nullptr;
	}
	return &mData->step;
}

const MaterialStepData* MaterialRegistry::GetSloshStepData() const
{
	const MaterialData* mData = GetMaterialData(_sloshMaterial);
	if (mData)
		return &mData->step;
	return nullptr;
}

const char* MaterialRegistry::Schema() const {
	return materialsSchema;
}

using namespace rapidjson;

template<typename S, size_t N>
void FillVectorFromJsonArray(fixed_vector<S, N>& vec, const Value& value)
{
	vec.clear();
	Value::ConstArray arr = value.GetArray();
	for (auto& item : arr)
	{
		vec.push_back(item.GetString());
	}
}

void AssignStepSoundData(MaterialStepSoundData& data, const Value& value)
{
	UpdatePropertyFromJson(data.volume, value, "volume");
	UpdatePropertyFromJson(data.timeMsec, value, "time");
}

void AssignMaterialStepData(MaterialStepData& data, const Value& stepJsonValue)
{
	auto rightIt = stepJsonValue.FindMember("right");
	if (rightIt != stepJsonValue.MemberEnd())
	{
		FillVectorFromJsonArray(data.right, rightIt->value);
	}
	auto leftIt = stepJsonValue.FindMember("left");
	if (leftIt != stepJsonValue.MemberEnd())
	{
		FillVectorFromJsonArray(data.left, leftIt->value);
	}

	auto walkingIt = stepJsonValue.FindMember("walking");
	if (walkingIt != stepJsonValue.MemberEnd())
	{
		AssignStepSoundData(data.walking, walkingIt->value);
	}
	auto runningIt = stepJsonValue.FindMember("running");
	if (runningIt != stepJsonValue.MemberEnd())
	{
		AssignStepSoundData(data.running, runningIt->value);
	}
	UpdatePropertyFromJson(data.skipSomeSteps, stepJsonValue, "skip_some_steps");
}

bool MaterialRegistry::ReadFromDocument(const rapidjson::Document& document, const char* fileName)
{
	Color3 defaultWallpuffColor;
	if (UpdatePropertyFromJson(defaultWallpuffColor, document, "wallpuff_color"))
	{
		for (auto& item : _materials)
		{
			Color3& wallpuffColor = item.second.hit.wallpuffColor;

			wallpuffColor.r = clamp(defaultWallpuffColor.r * wallpuffColor.r / DEFAULT_WALLPUFF_COLOR.r, 0, 255);
			wallpuffColor.g = clamp(defaultWallpuffColor.g * wallpuffColor.g / DEFAULT_WALLPUFF_COLOR.g, 0, 255);
			wallpuffColor.b = clamp(defaultWallpuffColor.b * wallpuffColor.b / DEFAULT_WALLPUFF_COLOR.b, 0, 255);
		}
	}

	if (UpdatePropertyFromJson(_wallpuffAlpha, document, "wallpuff_alpha"))
	{
		_wallpuffAlpha.min = clamp(_wallpuffAlpha.min, 0, 255);
		_wallpuffAlpha.max = clamp(_wallpuffAlpha.max, 0, 255);
	}

	Color3 defaultImpactParticleColor;
	if (UpdatePropertyFromJson(defaultImpactParticleColor, document, "impact_particle_color"))
	{
#if CLIENT_DLL
		_impactParticleColorIndex = ClosestPaletteColorIndex(defaultImpactParticleColor);
#endif
	}

	auto materialsIt = document.FindMember("materials");
	if (materialsIt != document.MemberEnd())
	{
		auto& materials = materialsIt->value;
		for (auto matIt = materials.MemberBegin(); matIt != materials.MemberEnd(); ++matIt)
		{
			if (matIt->name.GetStringLength() != 1)
			{
				g_errorCollector.AddFormattedError("Material name \"%s\" must be a single character\n", matIt->name.GetString());
				continue;
			}

			const char matName = *matIt->name.GetString();

			const MaterialData* existingMaterial = GetMaterialData(matName);
			MaterialData data;
			if (existingMaterial)
				data = *existingMaterial;

			const Value& materialJsonValue = matIt->value;

			{
				auto stepIt = materialJsonValue.FindMember("step");
				if (stepIt != materialJsonValue.MemberEnd())
				{
					AssignMaterialStepData(data.step, stepIt->value);
				}
			}

			{
				auto hitIt = materialJsonValue.FindMember("hit");
				if (hitIt != materialJsonValue.MemberEnd())
				{
					const Value& hitJsonValue = hitIt->value;
					auto wavesIt = hitJsonValue.FindMember("waves");
					if (wavesIt != hitJsonValue.MemberEnd())
					{
						FillVectorFromJsonArray(data.hit.waves, wavesIt->value);
					}

					UpdatePropertyFromJson(data.hit.volume, hitJsonValue, "volume");
					UpdatePropertyFromJson(data.hit.volumebar, hitJsonValue, "volume_bar");

					auto attnIt = hitJsonValue.FindMember("attenuation");
					if (attnIt != hitJsonValue.MemberEnd())
					{
						UpdateAttenuationFromJson(data.hit.attn, attnIt->value);
					}

					UpdatePropertyFromJson(data.hit.allowWallpuff, hitJsonValue, "allow_wallpuff");
					UpdatePropertyFromJson(data.hit.allowWeaponSparks, hitJsonValue, "allow_weapon_sparks");
					UpdatePropertyFromJson(data.hit.playSparks, hitJsonValue, "play_sparks");
					UpdatePropertyFromJson(data.hit.wallpuffColor, hitJsonValue, "wallpuff_color");

					Color3 impactParticleColor;
					if (UpdatePropertyFromJson(impactParticleColor, hitJsonValue, "impact_particle_color"))
					{
#if CLIENT_DLL
						data.hit.impactParticleColorIndex = ClosestPaletteColorIndex(impactParticleColor);
#endif
					}
				}
			}

			SetMaterialData(matName, data);
		}
	}

	auto ladderStepIt = document.FindMember("ladder_step");
	if (ladderStepIt != document.MemberEnd())
	{
		AssignMaterialStepData(_ladder, ladderStepIt->value);
	}

	auto wadeStepIt = document.FindMember("wade_step");
	if (wadeStepIt != document.MemberEnd())
	{
		AssignMaterialStepData(_wade, wadeStepIt->value);
	}

	UpdatePropertyFromJson(_defaultMaterial, document, "default_material");
	UpdatePropertyFromJson(_defaultStepMaterial, document, "default_step_material");
	UpdatePropertyFromJson(_sloshMaterial, document, "slosh_material");
	UpdatePropertyFromJson(_fleshMaterial, document, "flesh_material");

	return true;
}

template<typename T, size_t N>
static void DumpSoundVector(const fixed_vector<T, N>& v)
{
	if (!v.empty())
	{
		for (const auto& item : v)
		{
			LOG("\"%s\"; ", item.c_str());
		}
		LOG("\n");
	}
	else
		LOG("not defined\n");
}

static void DumpStepData(const MaterialStepData& data)
{
	LOG("Right foot step sounds: ");
	DumpSoundVector(data.right);

	LOG("Left foot step sounds: ");
	DumpSoundVector(data.left);

	LOG("Walking. Volume: %g. Delay between step sounds (msecs): %d\n", data.walking.volume, data.walking.timeMsec);
	LOG("Running. Volume: %g. Delay between step sounds (msecs): %d\n", data.running.volume, data.running.timeMsec);

	if (data.skipSomeSteps)
		LOG("Skipping some step sounds\n");
}

void MaterialRegistry::DumpMaterials() const
{
	LOG("Default material: '%c'\n", _defaultMaterial);
	if (_defaultStepMaterial != '\0')
		LOG("Default step material: '%c'\n", _defaultStepMaterial);
	LOG("Slosh material: '%c'\n", _sloshMaterial);
	LOG("Flesh material: '%c'\n\n", _fleshMaterial);

	for (auto it = _materials.begin(); it != _materials.end(); ++it)
	{
		DumpMaterial(it->first, it->second);
	}

	LOG("Ladder steps:\n");
	DumpStepData(_ladder);
	LOG("\n");

	LOG("Wade steps:\n");
	DumpStepData(_wade);
	LOG("\n");
}

void MaterialRegistry::DumpMaterial(char c, const MaterialData& data) const
{
	LOG("Material '%c':\n", c);

	if (data.step.IsDefined())
		DumpStepData(data.step);
	else
		LOG("Foot steps for this material are not defined (default ones will be used)\n");

	LOG("Hit sounds: ");
	DumpSoundVector(data.hit.waves);

	LOG("Volume: %g. Volume bar: %g. Attenuation: %g\n", data.hit.volume, data.hit.volumebar, data.hit.attn);

	if (data.hit.allowWallpuff)
		LOG("Wallpuffs allowed. ");
	if (data.hit.allowWeaponSparks)
		LOG("Sparks/streaks from bullet impact allowed. ");
	if (data.hit.playSparks)
		LOG("Spark effect from melee impact allowed. ");
	LOG("Wallpuff color: (%d, %d, %d)\n\n", data.hit.wallpuffColor.r, data.hit.wallpuffColor.g, data.hit.wallpuffColor.b);
}

void MaterialRegistry::DumpMaterial(char c) const
{
	const MaterialData* data = GetMaterialData(c);
	if (data)
		DumpMaterial(c, *data);
	else
		LOG("Couldn't find material data for '%c'\n", c);
}

MaterialRegistry g_MaterialRegistry;
