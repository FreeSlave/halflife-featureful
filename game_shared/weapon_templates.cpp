#include "weapon_templates.h"
#include "ammoregistry.h"

#include "json_utils.h"
#include "logger.h"
#include "error_collector.h"
#include "weapon_parameters.h"
#include "sound_channel.h"
#include "soundent_bits.h"
#include "fx_flags.h"

#include "skill.h"

#include "rapidjson/writer.h"
#include "rapidjson/ostreamwrapper.h"
#include <sstream>

extern WeaponParameters* AccessWeaponParameters(const char* name);

using namespace rapidjson;

const char weaponTemplatesSchema[] = R"(
{
	"type": "object",
	"additionalProperties": {
		"oneOf": [
			{
				"$ref": "weapons.json#/weapon_template"
			},
			{
				"type": "string",
				"minLength": 1
			}
		]
	}
}
)";

const char weaponSingleTemplateSchema[] = R"(
{
	"$ref": "weapons.json#/weapon_template"
}
)";

class WeaponSingleTemplate : public JSONConfig
{
public:
	WeaponSingleTemplate(WeaponParameters* pParams, WeaponTemplateSystem* weaponTemplateSystem): _pParams(pParams), _weaponTemplateSystem(weaponTemplateSystem) {}
protected:
	const char* Schema() const override {
		return weaponSingleTemplateSchema;
	}
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override {
		_weaponTemplateSystem->ParseWeaponTemplate(*_pParams, document, fileName);
		return true;
	}

	WeaponParameters* _pParams;
	WeaponTemplateSystem* _weaponTemplateSystem;
};

static Vector ParseSpreadCone(const char* str)
{
	const int degree = atoi(str);
	switch (degree) {
	case 1: return VECTOR_CONE_1DEGREES;
	case 2: return VECTOR_CONE_2DEGREES;
	case 3: return VECTOR_CONE_3DEGREES;
	case 4: return VECTOR_CONE_4DEGREES;
	case 5: return VECTOR_CONE_5DEGREES;
	case 6: return VECTOR_CONE_6DEGREES;
	case 7: return VECTOR_CONE_7DEGREES;
	case 8: return VECTOR_CONE_8DEGREES;
	case 9: return VECTOR_CONE_9DEGREES;
	case 10: return VECTOR_CONE_10DEGREES;
	case 11: return VECTOR_CONE_11DEGREES;
	case 12: return VECTOR_CONE_12DEGREES;
	case 13: return VECTOR_CONE_13DEGREES;
	case 14: return VECTOR_CONE_14DEGREES;
	case 15: return VECTOR_CONE_15DEGREES;
	case 16: return VECTOR_CONE_16DEGREES;
	case 17: return VECTOR_CONE_17DEGREES;
	case 18: return VECTOR_CONE_18DEGREES;
	case 19: return VECTOR_CONE_19DEGREES;
	case 20: return VECTOR_CONE_20DEGREES;
	default: return Vector();
	}
}

static float ParseAutoAimDegree(const char* str)
{
	const int degree = atoi(str);
	switch (degree) {
	case 1: return AUTOAIM_1DEGREES;
	case 2: return AUTOAIM_2DEGREES;
	case 3: return AUTOAIM_3DEGREES;
	case 4: return AUTOAIM_4DEGREES;
	case 5: return AUTOAIM_5DEGREES;
	case 6: return AUTOAIM_6DEGREES;
	case 7: return AUTOAIM_7DEGREES;
	case 8: return AUTOAIM_8DEGREES;
	case 9: return AUTOAIM_9DEGREES;
	case 10: return AUTOAIM_10DEGREES;
	default: return 0.0f;
	}
}

const char* WeaponTemplateSystem::Schema() const
{
	return weaponTemplatesSchema;
}

template<typename T>
bool UpdatePropertyFromJson(WeaponModeValueNonNegative<T>& v, const Value& jsonValue, const char* key, bool altMode)
{
	T t;
	const bool result = ::UpdatePropertyFromJson(t, jsonValue, key);
	if (result)
		v.Materialize(altMode) = t;
	return result;
}

template<typename T>
bool UpdatePropertyFromJson(WeaponModeValue<T>& v, const Value& jsonValue, const char* key, bool altMode)
{
	T t;
	const bool result = ::UpdatePropertyFromJson(t, jsonValue, key);
	if (result)
		v.Materialize(altMode) = t;
	return result;
}

template<typename T>
bool UpdatePropertyFromJson(WeaponModeValueEmptyAwareNonNegative<T>& v, const Value& jsonValue, const char* key, bool altMode, bool emptied)
{
	T t;
	const bool result = ::UpdatePropertyFromJson(t, jsonValue, key);
	if (result)
		v.Materialize(altMode, emptied) = t;
	return result;
}

template<typename T>
bool UpdatePropertyFromJson(WeaponModeValueEmptyAware<T>& v, const Value& jsonValue, const char* key, bool altMode, bool emptied)
{
	T t;
	const bool result = ::UpdatePropertyFromJson(t, jsonValue, key);
	if (result)
		v.Materialize(altMode, emptied) = t;
	return result;
}

void WeaponTemplateSystem::ParseWeaponSoundScript(WeaponSoundScript& soundScript, const Value& value)
{
	auto assignWaves = [this](WeaponSoundScript& soundScript, const Value::ConstArray& waves) {
		soundScript.waves.clear();
		for (auto& wave : waves)
		{
			soundScript.waves.push_back(MakeConstantString(wave.GetString()));
		}
	};

	if (value.IsArray())
	{
		assignWaves(soundScript, value.GetArray());
	}
	else if (value.IsString())
	{
		soundScript.waves.clear();
		soundScript.waves.push_back(MakeConstantString(value.GetString()));
	}
	else
	{
		HandleJSONMember(value, "waves", [&](const Value& value) {
			assignWaves(soundScript, value.GetArray());
		});
		UpdatePropertyFromJson(soundScript.volume, value, "volume");
		HandleJSONMember(value, "channel", [&](const Value& value) {
			int channel;
			if (ParseSoundChannel(value.GetString(), channel))
				soundScript.channel = channel;
		});
		HandleJSONMember(value, "attenuation", [&](const Value& value) {
			UpdateAttenuationFromJson(soundScript.attenuation, value);
		});
		UpdatePropertyFromJson(soundScript.pitch, value, "pitch");
		UpdatePropertyFromJson(soundScript.looped, value, "looped");
	}
}

void WeaponTemplateSystem::ParseWeaponTemplate(WeaponParameters& params, const rapidjson::Value& value, const char* fileName)
{
	bool fromScratch = false;
	if (UpdatePropertyFromJson(fromScratch, value, "from_scratch"))
	{
		if (fromScratch)
		{
			WeaponParameters fromScratchParams;

			fromScratchParams.worldModel = std::move(params.worldModel);
			fromScratchParams.viewModel = std::move(params.viewModel);
			fromScratchParams.playerModel = std::move(params.playerModel);
			fromScratchParams.playerAnimExt = std::move(params.playerAnimExt);
			fromScratchParams.priority = params.priority;

			fromScratchParams.idleAnims.main = params.idleAnims.main;
			fromScratchParams.deploy.animIndex.main = params.deploy.animIndex.main;

			fromScratchParams.modelSoundsDefined = true;

			params = std::move(fromScratchParams);
		}
	}

	UpdatePropertyFromJson(params.initialAmmoAmount, value, "ammo_amount");
	UpdatePropertyFromJson(params.maxClip, value, "max_clip");

#if !CLIENT_DLL
	{
		decltype(WeaponParameters::ammoName) ammoName;
		if (UpdatePropertyFromJson(ammoName, value, "ammo_name"))
		{
			if (!ammoName.empty())
			{
				if (g_AmmoRegistry.GetByName(ammoName.c_str()) == nullptr)
				{
					g_errorCollector.AddFormattedError("%s: \"%s\" is not a registered ammo type", fileName, ammoName.c_str());
				}
				else
				{
					params.ammoName = ammoName;
				}
			}
			else
				params.ammoName.clear();
		}
	}

	{
		decltype(WeaponParameters::secondaryAmmoName) secondaryAmmoName;
		if (UpdatePropertyFromJson(secondaryAmmoName, value, "secondary_ammo_name"))
		{
			if (!secondaryAmmoName.empty())
			{
				if (g_AmmoRegistry.GetByName(secondaryAmmoName.c_str()) == nullptr)
				{
					g_errorCollector.AddFormattedError("%s: \"%s\" is not a registered ammo type", fileName, secondaryAmmoName.c_str());
				}
				else
				{
					params.secondaryAmmoName = secondaryAmmoName;
				}
			}
			else
				params.secondaryAmmoName.clear();
		}
	}
#endif

	UpdatePropertyFromJson(params.worldModel, value, "world_model");
	UpdatePropertyFromJson(params.viewModel, value, "view_model");
	UpdatePropertyFromJson(params.playerModel, value, "player_model");
	UpdatePropertyFromJson(params.playerAnimExt, value, "player_anim_ext");
	UpdatePropertyFromJson(params.priority, value, "priority");
	UpdatePropertyFromJson(params.worldModelAnimated, value, "world_model_animated");
	UpdatePropertyFromJson(params.worldModelSequence, value, "world_model_sequence");

	auto HandleDeploy = [&](const char* propName, bool altMode, bool emptied)
	{
		HandleJSONMember(value, propName, [&](const Value& value) {
			if (value.IsNull())
			{
				params.deploy.animIndex.Reset(altMode, emptied);
				params.deploy.duration.Reset(altMode, emptied);
				params.deploy.idleDelay.Reset(altMode, emptied);
				params.deploy.sound.Reset(altMode, emptied);
			}
			else
			{
				UpdatePropertyFromJson(params.deploy.animIndex, value, "anim", altMode, emptied);
				UpdatePropertyFromJson(params.deploy.duration, value, "duration", altMode, emptied);
				UpdatePropertyFromJson(params.deploy.idleDelay, value, "idle_delay", altMode, emptied);

				HandleJSONMember(value, "sound", [&](const Value& value) {
					WeaponSoundScript& soundScript = params.deploy.sound.Materialize(altMode, emptied);
					ParseWeaponSoundScript(soundScript, value);
				});
			}
		});
	};

	HandleDeploy("deploy", false, false);
	HandleDeploy("deploy_empty", false, true);
	HandleDeploy("alt_deploy", true, false);
	HandleDeploy("alt_deploy_empty", true, true);

	auto HandleIdle = [&](const char* propName, bool altMode, bool emptied)
	{
		HandleJSONMember(value, propName, [&](const Value& value) {
			if (value.IsNull())
			{
				params.idleAnims.Reset(altMode, emptied);
			}
			else if (value.IsArray())
			{
				Value::ConstArray animArr = value.GetArray();
				auto& v = params.idleAnims.Materialize(altMode, emptied);
				v.clear();
				for (auto& item : animArr)
				{
					WeaponParameters::IdleAnim anim;
					UpdatePropertyFromJson(anim.animIndex, item, "anim");
					UpdatePropertyFromJson(anim.chance, item, "chance");
					UpdatePropertyFromJson(anim.duration, item, "duration");

					HandleJSONMember(item, "sound", [&](const Value& value) {
						ParseWeaponSoundScript(anim.sound, value);
					});

					v.push_back(anim);
				}
			}
			else if (value.IsObject())
			{
				auto& v = params.idleAnims.Materialize(altMode, emptied);
				v.clear();

				WeaponParameters::IdleAnim anim;
				anim.chance = 1.0f;
				UpdatePropertyFromJson(anim.animIndex, value, "anim");
				UpdatePropertyFromJson(anim.duration, value, "duration");

				HandleJSONMember(value, "sound", [&](const Value& value) {
					ParseWeaponSoundScript(anim.sound, value);
				});

				v.push_back(anim);
			}
		});
	};

	HandleIdle("idle", false, false);
	HandleIdle("idle_empty", false, true);
	HandleIdle("alt_idle", true, false);
	HandleIdle("alt_idle_empty", true, true);

	auto ParsePlayerSpeed = [](const Value& value) {
		PlayerSpeed playerSpeed;
		if (value.IsString())
		{
			const char* heightStr = value.GetString();
			if (*heightStr == '*')
			{
				playerSpeed.value = static_cast<float>(atof(heightStr + 1));
				playerSpeed.isFactor = true;
			}
		}
		else if (value.IsNumber())
		{
			playerSpeed.value = value.GetFloat();
		}
		return playerSpeed;
	};

	auto HandleFire = [&](const char* propName, bool altMode) {
		HandleJSONMember(value, propName, [&](const Value& value) {
			auto& fire = params.fire;

			HandleJSONMember(value, "type", [&](const Value& value) {
				const char* str = value.GetString();
				if (strcmp(str, "bullet") == 0 || strcmp(str, "bullets") == 0)
				{
					fire.fireType.Materialize(altMode) = WeaponParameters::Fire::BULLETS;
				}
				else if (strcmp(str, "melee") == 0)
				{
					fire.fireType.Materialize(altMode) = WeaponParameters::Fire::MELEE;
				}
				else if (strcmp(str, "projectile") == 0)
				{
					fire.fireType.Materialize(altMode) = WeaponParameters::Fire::PROJECTILE;
				}
			});

			auto UpdateWeaponDamageInfo = [&](DamageInfoPatch& result, const Value& value)
			{
				if (value.IsObject())
				{
					auto minIt = value.FindMember("min");
					if (minIt != value.MemberEnd())
					{
						result.damage = FloatRangeFromJSON(value);
						return;
					}
					else
					{
						UpdateDamageInfoFromJson(result, value);
						return;
					}
				}
				if (value.IsNumber() || value.IsArray())
				{
					result.damage = FloatRangeFromJSON(value);
				}
				else if (value.IsString())
				{
					const char* str = value.GetString();
					if (strchr(str, ',') != nullptr)
					{
						result.damage = FloatRangeFromJSON(value);
					}
					else
					{
						result.damage = GetSkillValueRange(value.GetString());
					}
				}
			};

			auto ParseWeaponDamage = [&](const Value& value)
			{
				FloatRange result;
				if (value.IsNumber() || value.IsArray() || value.IsObject())
				{
					result = FloatRangeFromJSON(value);
				}
				else if (value.IsString())
				{
					const char* str = value.GetString();
					if (strchr(str, ',') != nullptr)
					{
						result = FloatRangeFromJSON(value);
					}
					else
					{
						result = GetSkillValueRange(value.GetString());
					}
				}
				return result;
			};

			HandleJSONMember(value, "damage", [&](const Value& value) {
				UpdateWeaponDamageInfo(fire.damageInfo.Materialize(altMode), value);
			});

			HandleJSONMember(value, "damage_charged_factor", [&](const Value& value) {
				fire.damageChargedFactor.Materialize(altMode) = ParseWeaponDamage(value);
			});

			HandleJSONMember(value, "damage_charged_max", [&](const Value& value) {
				fire.damageChargedMax.Materialize(altMode) = ParseWeaponDamage(value);
			});

			UpdatePropertyFromJson(fire.subsequentSwingFactor, value, "subsequent_swing_dmg_factor", altMode);

			auto HandleFireAnimArray = [](Value::ConstArray& animArr, WeaponParameters::FireAnimArray& v)
			{
				v.clear();
				for (auto& item : animArr)
				{
					v.push_back(item.GetInt());
				}
			};

			HandleJSONMember(value, "anims", [&](const Value& value) {
				Value::ConstArray animArr = value.GetArray();
				auto& v = fire.anims.Materialize(altMode, false);
				HandleFireAnimArray(animArr, v);
			});

			HandleJSONMember(value, "anims_last_shot", [&](const Value& value) {
				if (value.IsNull())
				{
					fire.anims.mainEmptied.reset();
				}
				else
				{
					Value::ConstArray animArr = value.GetArray();
					auto& v = fire.anims.Materialize(altMode, true);
					HandleFireAnimArray(animArr, v);
				}
			});

			HandleJSONMember(value, "hit_anims", [&](const Value& value) {
				Value::ConstArray animArr = value.GetArray();
				auto& v = fire.hitAnims.Materialize(altMode);
				HandleFireAnimArray(animArr, v);
			});

			HandleJSONMember(value, "charge_anims", [&](const Value& value) {
				Value::ConstArray animArr = value.GetArray();
				auto& v = fire.chargeAnims.Materialize(altMode);
				HandleFireAnimArray(animArr, v);
			});
			UpdatePropertyFromJson(fire.chargeTime, value, "charge_time", altMode);
			HandleJSONMember(value, "charge_sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.chargeSound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});
			UpdatePropertyFromJson(fire.chargedAttack, value, "charged_attack", altMode);
			UpdatePropertyFromJson(fire.laserSpotOnCharge, value, "laser_spot_on_charge", altMode);

			HandleJSONMember(value, "cooldown_anims", [&](const Value& value) {
				Value::ConstArray animArr = value.GetArray();
				auto& v = fire.cooldownAnims.Materialize(altMode);
				HandleFireAnimArray(animArr, v);
			});
			UpdatePropertyFromJson(fire.cooldownTime, value, "cooldown_time", altMode);
			HandleJSONMember(value, "cooldown_sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.cooldownSound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});

			HandleJSONMember(value, "sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.sound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});

			HandleJSONMember(value, "sound_additional", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.soundAdditional.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});

			HandleJSONMember(value, "hit_body_sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.hitBodySound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});

			HandleJSONMember(value, "hit_wall_sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.hitWallSound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});

			HandleJSONMember(value, "empty_sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.emptySound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
				fire.useStandardEmptySound.Materialize(altMode) = false;
			});

			UpdatePropertyFromJson(fire.useStandardEmptySound, value, "use_standard_empty_sound", altMode);

			auto ParsePlayerMovementConditions = [](const Value& value) {
				PlayerMovementConditions conditions;
				UpdatePropertyFromJson(conditions.inAir, value, "in_air");
				UpdatePropertyFromJson(conditions.ducking, value, "ducking");

				HandleJSONMember(value, "moving", [&conditions](const Value& value) {
					if (value.IsNumber())
					{
						conditions.moving = value.GetFloat();
					}
					else if (value.IsBool())
					{
						conditions.moving = value.GetBool() ? 0.0f : -1.0f;
					}
				});
				return conditions;
			};

			auto ParseSpreadValue = [](const Value& value)
			{
				if (value.IsNumber())
				{
					return value.GetFloat();
				}
				else if (value.IsString())
				{
					return ParseSpreadCone(value.GetString()).x;
				}
				return 0.0f;
			};

			HandleJSONMember(value, "spread", [&](const Value& value) {
				if (value.IsNumber() || value.IsString())
				{
					fire.spread.SetStaticSpread(altMode, ParseSpreadValue(value));
				}
				else if (value.IsObject())
				{
					float inaccuracy;
					if (UpdatePropertyFromJson(inaccuracy, value, "inaccuracy"))
					{
						WeaponSpread::Type type{};
						HandleJSONMember(value, "type", [&](const Value& value) {
							const char* str = value.GetString();
							if (strcmp(str, "recovering") == 0)
								type = WeaponSpread::Type::RECOVERING;
							else if (strcmp(str, "quadratic") == 0)
								type = WeaponSpread::Type::QUADRATIC;
							else if (strcmp(str, "qubic") == 0)
								type = WeaponSpread::Type::QUBIC;
						});

						float maxInaccuracy;
						UpdatePropertyFromJson(maxInaccuracy, value, "max_inaccuracy");
						float factor = 1.0f;
						UpdatePropertyFromJson(factor, value, "factor");
						float recoveryTime;
						if (UpdatePropertyFromJson(recoveryTime, value, "recovery_time"))
						{
							if (!type)
								type = WeaponSpread::Type::RECOVERING;
						}
						float inaccuracyShift;
						UpdatePropertyFromJson(inaccuracyShift, value, "inaccuracy_shift");
						float divisor;
						if (UpdatePropertyFromJson(divisor, value, "divisor"))
						{
							factor = 1.0f / divisor;
						}
						if (!type)
							type = WeaponSpread::Type::QUBIC;

						WeaponSpread::RuleList ruleList;
						HandleJSONMember(value, "spread", [&](const Value& value) {
							if (value.IsObject())
							{
								float base = 0.0f;
								float factor = 0.0f;
								UpdatePropertyFromJson(base, value, "base");
								UpdatePropertyFromJson(factor, value, "factor");
								ruleList.push_back(WeaponSpreadRule::Dynamic(base, factor, PlayerMovementConditions()));
							}
							else if (value.IsArray())
							{
								Value::ConstArray arr = value.GetArray();
								for (const Value& item : arr)
								{
									PlayerMovementConditions conditions;
									HandleJSONMember(item, "conditions", [&](const Value& value) {
										conditions = ParsePlayerMovementConditions(value);
									});
									float base = 0.0f;
									float factor = 0.0f;
									UpdatePropertyFromJson(base, item, "base");
									UpdatePropertyFromJson(factor, item, "factor");
									ruleList.push_back(WeaponSpreadRule::Dynamic(base, factor, conditions));
								}
							}
						});

						switch (type) {
						case WeaponSpread::Type::RECOVERING:
							fire.spread.SetInaccuracyRecovering(altMode, inaccuracy, recoveryTime, factor, maxInaccuracy, ruleList);
							break;
						case WeaponSpread::Type::QUBIC:
						case WeaponSpread::Type::QUADRATIC:
							fire.spread.SetInaccuracyAuto(altMode, inaccuracy, factor, inaccuracyShift, WeaponSpread::Type::QUADRATIC == type, maxInaccuracy, ruleList);
							break;
						default:
							break;
						}
					}
					else
					{
						float spreadX{};
						float spreadY{};
						HandleJSONMember(value, "vertical", [&](const Value& value) {
							spreadY = ParseSpreadValue(value);
						});
						HandleJSONMember(value, "lateral", [&](const Value& value) {
							spreadX = ParseSpreadValue(value);
						});
						fire.spread.SetStaticSpread(altMode, spreadX, spreadY);
					}
				}
				else if (value.IsArray())
				{
					WeaponSpread::RuleList ruleList;

					Value::ConstArray arr = value.GetArray();
					for (const Value& item : arr)
					{
						PlayerMovementConditions conditions;
						HandleJSONMember(item, "conditions", [&](const Value& value) {
							conditions = ParsePlayerMovementConditions(value);
						});

						HandleJSONMember(item, "spread", [&](const Value& value) {
							if (value.IsNumber() || value.IsString())
							{
								ruleList.push_back(WeaponSpreadRule::Static(ParseSpreadValue(value), conditions));
							}
							else if (value.IsObject())
							{
								float spreadX{};
								float spreadY{};
								HandleJSONMember(value, "vertical", [&](const Value& value) {
									spreadY = ParseSpreadValue(value);
								});
								HandleJSONMember(value, "lateral", [&](const Value& value) {
									spreadX = ParseSpreadValue(value);
								});

								ruleList.push_back(WeaponSpreadRule::Static(spreadX, spreadY, conditions));
							}
						});
					}
					fire.spread.SetStaticSpread(altMode, ruleList);
				}
			});

			UpdatePropertyFromJson(fire.cycleTime, value, "cycle_time", altMode);
			UpdatePropertyFromJson(fire.cycleTimeLastShot, value, "cycle_time_last_shot", altMode);
			UpdatePropertyFromJson(fire.hitCycleTime, value, "hit_cycle_time", altMode);
			UpdatePropertyFromJson(fire.idleDelay, value, "idle_delay", altMode, false);
			UpdatePropertyFromJson(fire.idleDelay, value, "idle_delay_empty", altMode, true);
			UpdatePropertyFromJson(fire.ammoPerFire, value, "ammo_per_fire", altMode);
			UpdatePropertyFromJson(fire.allowUnderwater, value, "allow_underwater", altMode);
			UpdatePropertyFromJson(fire.bulletCount, value, "bullet_count", altMode);
			UpdatePropertyFromJson(fire.tracerFreq, value, "tracer_freq", altMode);
			UpdatePropertyFromJson(fire.burstShots, value, "burst", altMode);
			UpdatePropertyFromJson(fire.burstInterval, value, "burst_interval", altMode);
			UpdatePropertyFromJson(fire.semiAuto, value, "semiauto", altMode);
			UpdatePropertyFromJson(fire.useSecondaryAmmo, value, "use_secondary_ammo", altMode);
			UpdatePropertyFromJson(fire.muzzleFlash, value, "muzzleflash", altMode);

			HandleJSONMember(value, "autoaim", [&](const Value& value) {
				if (value.IsNumber())
				{
					fire.autoAimDegree.Materialize(altMode) = value.GetFloat();
				}
				else if (value.IsString())
				{
					fire.autoAimDegree.Materialize(altMode) = ParseAutoAimDegree(value.GetString());
				}
			});

			HandleJSONMember(value, "weapon_volume", [&](const Value& value) {
				if (value.IsInt())
				{
					fire.weaponVolume.Materialize(altMode) = value.GetInt();
				}
				else if (value.IsString())
				{
					const char* str = value.GetString();
					if (strcmp(str, "loud") == 0)
					{
						fire.weaponVolume.Materialize(altMode) = LOUD_GUN_VOLUME;
					}
					else if (strcmp(str, "normal") == 0)
					{
						fire.weaponVolume.Materialize(altMode) = NORMAL_GUN_VOLUME;
					}
					else if (strcmp(str, "quiet") == 0)
					{
						fire.weaponVolume.Materialize(altMode) = QUIET_GUN_VOLUME;
					}
				}
			});

			HandleJSONMember(value, "weapon_flash", [&](const Value& value) {
				if (value.IsInt())
				{
					fire.weaponFlash.Materialize(altMode) = value.GetInt();
				}
				else if (value.IsString())
				{
					const char* str = value.GetString();
					if (strcmp(str, "bright") == 0)
					{
						fire.weaponFlash.Materialize(altMode) = BRIGHT_GUN_FLASH;
					}
					else if (strcmp(str, "normal") == 0)
					{
						fire.weaponFlash.Materialize(altMode) = NORMAL_GUN_FLASH;
					}
					else if (strcmp(str, "dim") == 0)
					{
						fire.weaponFlash.Materialize(altMode) = DIM_GUN_FLASH;
					}
				}
			});

			UpdatePropertyFromJson(fire.delayAfterEmpty, value, "delay_after_empty", altMode);
			UpdatePropertyFromJson(fire.delayUnderwater, value, "delay_underwater", altMode);

			UpdatePropertyFromJson(fire.pumpDelay, value, "pump_delay", altMode);
			HandleJSONMember(value, "pump_sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = fire.pumpSound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});

			UpdatePropertyFromJson(fire.bulletDistance, value, "bullet_distance", altMode);
			UpdatePropertyFromJson(fire.rangeModifier, value, "range_modifier", altMode);

			UpdatePropertyFromJson(fire.clientPunchPitch, value, "client_punch_pitch", altMode);
			UpdatePropertyFromJson(fire.clientPunchYaw, value, "client_punch_yaw", altMode);

			HandleJSONMember(value, "shell", [&](const Value& value) {
				UpdatePropertyFromJson(fire.shellCount, value, "count", altMode);

				HandleJSONMember(value, "offset", [&](const Value& value) {
					bool attachmentBased = UpdatePropertyFromJson(fire.shellAttachment, value, "attachment", altMode);
					bool anyOffset = false;
					if (UpdatePropertyFromJson(fire.shellOffsetUp, value, "up", altMode))
						anyOffset = true;
					if (UpdatePropertyFromJson(fire.shellOffsetSide, value, "side", altMode))
						anyOffset = true;
					if (UpdatePropertyFromJson(fire.shellOffsetForward, value, "forward", altMode))
						anyOffset = true;

					if (!attachmentBased && anyOffset)
					{
						fire.shellAttachment.Materialize(altMode) = 0;
					}
				});

				HandleJSONMember(value, "sound_type", [&](const Value& value) {
					if (value.IsNull())
					{
						fire.shellSound.Materialize(altMode) = TE_BOUNCE_NULL;
					}
					else if (value.IsString())
					{
						const char* str = value.GetString();
						if (strcmp(str, "null") == 0)
						{
							fire.shellSound.Materialize(altMode) = TE_BOUNCE_NULL;
						}
						else if (strcmp(str, "shell") == 0)
						{
							fire.shellSound.Materialize(altMode) = TE_BOUNCE_SHELL;
						}
						else if (strcmp(str, "shotgun_shell") == 0)
						{
							fire.shellSound.Materialize(altMode) = TE_BOUNCE_SHOTSHELL;
						}
					}
				});

				HandleJSONMember(value, "model", [&](const Value& value) {
					const char* str = value.IsNull() ? nullptr : value.GetString();
					fire.shellModel = str ? MakeConstantString(str) : nullptr;
				});

				HandleJSONMember(value, "model_alternating", [&](const Value& value) {
					const char* str = value.IsNull() ? nullptr : value.GetString();
					fire.shellModelAlternating = str ? MakeConstantString(str) : nullptr;
				});

				UpdatePropertyFromJson(fire.shellLeftSide, value, "left_side", altMode);

				HandleJSONMember(value, "velocity", [&](const Value& value) {
					UpdatePropertyFromJson(fire.shellVelocityUp, value, "up", altMode);
					UpdatePropertyFromJson(fire.shellVelocitySide, value, "side", altMode);
					UpdatePropertyFromJson(fire.shellVelocityForward, value, "forward", altMode);
				});

				UpdatePropertyFromJson(fire.shellEjectDelay, value, "eject_delay", altMode);
			});

			UpdatePropertyFromJson(fire.suspendLaserSpotTime, value, "laser_suspend_time", altMode);

			auto ParseKickBack = [](const Value& value) {
				WeaponKickBack kickBack;
				if (value.IsObject())
				{
					UpdatePropertyFromJson(kickBack.verticalBase, value, "vertical");
					UpdatePropertyFromJson(kickBack.lateralBase, value, "lateral");
					UpdatePropertyFromJson(kickBack.verticalMax, value, "vertical_max");
					UpdatePropertyFromJson(kickBack.lateralMax, value, "lateral_max");
					UpdatePropertyFromJson(kickBack.verticalModifier, value, "vertical_modifier");
					UpdatePropertyFromJson(kickBack.lateralModifier, value, "lateral_modifier");
					UpdatePropertyFromJson(kickBack.directionChangeVertical, value, "vertical_persistance");
					UpdatePropertyFromJson(kickBack.directionChangeLateral, value, "lateral_persistance");
				}
				else if (value.IsNumber())
				{
					const float val = value.GetFloat();
					kickBack.verticalBase = val;
					kickBack.lateralBase = val;
				}
				return kickBack;
			};

			HandleJSONMember(value, "kickback", [&](const Value& value) {
				if (value.IsArray())
				{
					Value::ConstArray arr = value.GetArray();
					WeaponKickBackProfile::RuleList ruleList;
					for (auto& item : arr)
					{
						PlayerMovementConditions conditions;
						HandleJSONMember(item, "conditions", [&](const Value& value) {
							conditions = ParsePlayerMovementConditions(value);
						});

						WeaponKickBack kickBack;
						HandleJSONMember(item, "kickback", [&](const Value& value) {
							kickBack = ParseKickBack(value);
						});

						ruleList.push_back(WeaponKickBackRule{conditions, kickBack});
					}
					fire.kickBack.SetKickBack(altMode, ruleList);
				}
				else
				{
					fire.kickBack.SetKickBack(altMode, ParseKickBack(value));
				}
			});

			UpdatePropertyFromJson(fire.kickBackOnHitOnly, value, "kickback_on_hit_only", altMode);

			UpdatePropertyFromJson(fire.pushbackForce, value, "pushback_force", altMode);
			UpdatePropertyFromJson(fire.pushbackVertical, value, "pushback_vertical", altMode);

			HandleJSONMember(value, "shake", [&](const Value& value) {
				UpdatePlayerShake(fire.shake.Materialize(altMode), value);
			});

			HandleJSONMember(value, "hit_shake", [&](const Value& value) {
				UpdatePlayerShake(fire.hitShake.Materialize(altMode), value);
			});

			UpdatePropertyFromJson(fire.smackDelay, value, "smack_delay", altMode);
			UpdatePropertyFromJson(fire.hitDecal, value, "hit_decal", altMode);

			UpdatePropertyFromJson(fire.preventMovement, value, "prevent_movement", altMode);

			HandleJSONMember(value, "player_maxspeed", [&](const Value& value) {
				fire.playerMaxSpeed.Materialize(altMode) = ParsePlayerSpeed(value);
			});

			HandleJSONMember(value, "player_maxspeed_on_charge", [&](const Value& value) {
				fire.playerMaxSpeedOnCharge.Materialize(altMode) = ParsePlayerSpeed(value);
			});

			HandleJSONMember(value, "projectile", [&](const Value& value) {
				UpdatePropertyFromJson(fire.projectileName, value, "name", altMode);
				UpdatePropertyFromJson(fire.projectileEntTemplate, value, "ent_template", altMode);

				HandleJSONMember(value, "offset", [&](const Value& value) {
					UpdatePropertyFromJson(fire.projectileOffsetUp, value, "up", altMode);
					UpdatePropertyFromJson(fire.projectileOffsetSide, value, "side", altMode);
					UpdatePropertyFromJson(fire.projectileOffsetForward, value, "forward", altMode);
				});

				UpdatePropertyFromJson(fire.projectileRespectPunchangle, value, "respect_punchangle", altMode);
				UpdatePropertyFromJson(fire.projectileAdjustToCross, value, "adjust_to_cross", altMode);
				UpdatePropertyFromJson(fire.projectileSpeed, value, "speed", altMode);
				UpdatePropertyFromJson(fire.projectileDetonationTime, value, "time", altMode);

				HandleJSONMember(value, "add_player_velocity", [&](const Value& value) {
					if (value.IsBool())
					{
						const bool b = value.GetBool();
						if (b)
						{
							fire.projectileAddCurrentVelocity.Materialize(altMode) = WeaponParameters::Fire::ADD_VELOCITY_ABSOLUTE;
						}
						else
						{
							fire.projectileAddCurrentVelocity.Materialize(altMode) = WeaponParameters::Fire::DONT_ADD_VELOCITY;
						}
					}
					else if (value.IsString())
					{
						const char* str = value.GetString();
						if (stricmp(str, "projection") == 0)
						{
							fire.projectileAddCurrentVelocity.Materialize(altMode) = WeaponParameters::Fire::ADD_VELOCITY_PROJECTION;
						}
						else if (stricmp(str, "absolute") == 0)
						{
							fire.projectileAddCurrentVelocity.Materialize(altMode) = WeaponParameters::Fire::ADD_VELOCITY_ABSOLUTE;
						}
					}
				});

				HandleJSONMember(value, "fire_phase_offsets", [&](const Value& value) {
					if (value.IsArray())
					{
						Value::ConstArray arr = value.GetArray();
						WeaponParameters::FirePhaseArray firePhases;
						for (auto& item : arr)
						{
							WeaponParameters::FirePhase phase;
							UpdatePropertyFromJson(phase.side, item, "side");
							UpdatePropertyFromJson(phase.up, item, "up");
							firePhases.push_back(phase);
						}
						fire.projectileFirePhases.Materialize(altMode) = std::move(firePhases);
					}
					else
					{
						float startAngle = 0.0f;
						int count = 2;
						float distance = 8.0f;

						UpdatePropertyFromJson(startAngle, value, "start_angle");
						UpdatePropertyFromJson(count, value, "count");
						UpdatePropertyFromJson(distance, value, "distance");

						enum {CIRCLE, SQUARE};
						int type = CIRCLE;

						HandleJSONMember(value, "type", [&type](const Value& value) {
							const char* typeStr = value.GetString();
							if (strcmp(typeStr, "circle") == 0)
							{
								type = CIRCLE;
							}
							else if (strcmp(typeStr, "square") == 0)
							{
								type = SQUARE;
							}
						});

						enum {CLOCKWISE, COUNTER_CLOCKWISE};
						int orientation = CLOCKWISE;

						HandleJSONMember(value, "orientation", [&orientation](const Value& value) {
							const char* orientationStr = value.GetString();
							if (strcmp(orientationStr, "clockwise") == 0)
							{
								orientation = CLOCKWISE;
							}
							else if (strcmp(orientationStr, "counter-clockwise") == 0)
							{
								orientation = COUNTER_CLOCKWISE;
							}
						});

						WeaponParameters::FirePhaseArray firePhases;

						float currentAngle = startAngle;
						float angleFraction = 360.0f / count;
						for (int i=0; i<count; ++i)
						{
							const float currentAngleDeg = currentAngle * M_PI_F / 180.0f;
							float x = std::cos(currentAngleDeg);
							float y = std::sin(currentAngleDeg);
							if (type == SQUARE)
							{
								const float denom = Q_max(std::fabs(x), std::fabs(y));
								x *= distance / denom;
								y *= distance / denom;
							}
							else
							{
								x *= distance;
								y *= distance;
							}
							firePhases.push_back({x, y});
							currentAngle += angleFraction * ((orientation == CLOCKWISE) ? -1.0f : 1.0f);
						}
						fire.projectileFirePhases.Materialize(altMode) = std::move(firePhases);
					}
				});
			});

			HandleJSONMember(value, "spray", [&](const Value& value) {
				HandleJSONMember(value, "offset", [&](const Value& value) {
					UpdatePropertyFromJson(fire.sprayOffsetUp, value, "up", altMode);
					UpdatePropertyFromJson(fire.sprayOffsetSide, value, "side", altMode);
					UpdatePropertyFromJson(fire.sprayOffsetForward, value, "forward", altMode);
				});
				HandleJSONMember(value, "visual", [&](const Value& value) {
					Visual visual = ParseVisualFromJSON(value, [this](const char* str){ return this->MakeConstantString(str); });
					visual.CompleteFrom(fire.sprayVisual.Materialize(altMode));
					fire.sprayVisual.Materialize(altMode) = visual;
				});
				UpdatePropertyFromJson(fire.sprayCount, value, "count", altMode);
				UpdatePropertyFromJson(fire.spraySpeed, value, "speed", altMode);
				UpdatePropertyFromJson(fire.spraySpread, value, "spread", altMode);
				HandleJSONMember(value, "flags", [&](const Value& value) {
					Value::ConstArray arr = value.GetArray();
					for (const auto& item : arr)
					{
						if (strcmp(item.GetString(), "collideworld") == 0)
						{
							fire.sprayFlags.Materialize(altMode) |= SPRAY_FLAG_COLLIDEWORLD;
						}
						else if (strcmp(item.GetString(), "animate") == 0 || strcmp(item.GetString(), "animated") == 0)
						{
							fire.sprayFlags.Materialize(altMode) |= SPRAY_FLAG_ANIMATE;
						}
						else if (strcmp(item.GetString(), "fadeout") == 0)
						{
							fire.sprayFlags.Materialize(altMode) |= SPRAY_FLAG_FADEOUT;
						}
					}
				});
			});

			HandleJSONMember(value, "extra_ai_sound", [&](const Value& value) {
				HandleJSONMember(value, "type", [&](const Value& value) {
					auto parseAISoundType = [](const char* str)
					{
						if (stricmp(str, "danger") == 0)
						{
							return bits_SOUND_DANGER;
						}
						else if (stricmp(str, "combat") == 0)
						{
							return bits_SOUND_COMBAT;
						}
						return 0;
					};
					int soundType = 0;
					if (value.IsArray())
					{
						Value::ConstArray arr = value.GetArray();
						for (auto& item : arr)
						{
							soundType |= parseAISoundType(item.GetString());
						}
					}
					else if (value.IsString())
					{
						soundType = parseAISoundType(value.GetString());
					}
					fire.extraSoundTypes.Materialize(altMode) = soundType;
				});
				UpdatePropertyFromJson(fire.extraSoundTime, value, "time", altMode);
			});
		});
	};

	HandleFire("fire", false);
	HandleFire("alt_fire", true);

	auto HandleSwitchMode = [&](const char* propName, bool switchBack) {
		HandleJSONMember(value, propName, [&](const Value& value) {
			auto& mode = params.altMode;

			UpdatePropertyFromJson(mode.attackDelay, value, "attack_delay", switchBack);
			UpdatePropertyFromJson(mode.animIndex, value, "anim", switchBack);
			UpdatePropertyFromJson(mode.bodyDelay, value, "body_switch_delay", switchBack);
			UpdatePropertyFromJson(mode.modeDelay, value, "mode_switch_delay", switchBack);
			UpdatePropertyFromJson(mode.endAnimIndex, value, "end_anim", switchBack);
			UpdatePropertyFromJson(mode.endAnimDuration, value, "end_anim_duration", switchBack);
		});
	};

	HandleSwitchMode("switch_mode", false);
	HandleSwitchMode("switch_mode_back", true);

	HandleJSONMember(value, "switch_mode_common", [&](const Value& value) {
		UpdatePropertyFromJson(params.altMode.toggleLaserSpot, value, "toggle_laser_spot");
	});

	HandleJSONMember(value, "zoom", [&](const Value& value) {
		UpdatePropertyFromJson(params.altMode.zoomFOV, value, "fov");
		UpdatePropertyFromJson(params.altMode.zoomFOV2, value, "fov2");

		HandleJSONMember(value, "sound", [&](const Value& value) {
			WeaponSoundScript& soundScript = params.altMode.zoomSound;
			ParseWeaponSoundScript(soundScript, value);
		});

		HandleJSONMember(value, "sound2", [&](const Value& value) {
			WeaponSoundScript& soundScript = params.altMode.zoomSound2;
			ParseWeaponSoundScript(soundScript, value);
		});

		HandleJSONMember(value, "unzoom_sound", [&](const Value& value) {
			WeaponSoundScript& soundScript = params.altMode.unzoomSound;
			ParseWeaponSoundScript(soundScript, value);
		});

		HandleJSONMember(value, "fade", [&](const Value& value) {
			UpdatePropertyFromJson(params.altMode.zoomFade.color, value, "color");
			UpdatePropertyFromJson(params.altMode.zoomFade.fadeTime, value, "fade_time");
			UpdatePropertyFromJson(params.altMode.zoomFade.holdTime, value, "hold_time");
			UpdatePropertyFromJson(params.altMode.zoomFade.alpha, value, "alpha");
		});

		UpdatePropertyFromJson(params.altMode.resetZoomOnFire, value, "reset_on_fire");
		UpdatePropertyFromJson(params.altMode.resumeZoomAfterReset, value, "resume_after_reset");
		UpdatePropertyFromJson(params.altMode.hideViewModelOnZoom, value, "hide_viewmodel");
	});

	HandleJSONMember(value, "secondary_attack", [&](const Value& value) {
		const char* str = value.GetString();
		if (strcmp(str, "disabled") == 0)
			params.secondaryFireType = SecondaryFireType::DISABLED;
		else if (strcmp(str, "alt_fire") == 0)
			params.secondaryFireType = SecondaryFireType::ALTERNATIVE_FIRE;
		else if (strcmp(str, "switch_mode") == 0)
			params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
	});

	UpdatePropertyFromJson(params.preventJump, value, "prevent_jump");
	UpdatePropertyFromJson(params.primaryFirePrioritized, value, "prioritize_primary_attack");
	UpdatePropertyFromJson(params.sharedChargeAndCooldown, value, "shared_charge_and_cooldown");
	UpdatePropertyFromJson(params.viewModelBody, value, "viewmodel_body", false);
	UpdatePropertyFromJson(params.viewModelBody, value, "viewmodel_body_alt", true);

	HandleJSONMember(value, "ammo_to_viewmodel_body", [&](const Value& value) {
		for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
		{
			const char* ammoAmountStr = it->name.GetString();
			const int ammoAmount = atoi(ammoAmountStr);
			const Value& bodyValue = it->value;
			if (bodyValue.IsInt())
			{
				params.ammoToBody.push_back(std::make_pair(ammoAmount, bodyValue.GetInt()));
			}
		}
	});

	HandleJSONMember(value, "player_maxspeed", [&](const Value& value) {
		params.playerMaxSpeed.Materialize(false) = ParsePlayerSpeed(value);
	});
	HandleJSONMember(value, "player_maxspeed_alt", [&](const Value& value) {
		params.playerMaxSpeed.Materialize(true) = ParsePlayerSpeed(value);
	});

	auto HandleReload = [&](const char* propName, bool altMode, bool emptied) {
		HandleJSONMember(value, propName, [&](const Value& value) {
			auto& reload = params.reload;

			if (value.IsNull())
			{
				reload.animIndex.Reset(altMode, emptied);
				reload.duration.Reset(altMode, emptied);
				reload.attackDelay.Reset(altMode, emptied);
				reload.idleDelay.Reset(altMode, emptied);
				reload.waitForRecoil.Reset(altMode, emptied);
				reload.suspendLaserSpotTime.Reset(altMode, emptied);
				reload.sound.Reset(altMode, emptied);
				reload.ammoCount.Reset(altMode, emptied);
				reload.ammoCountMin.Reset(altMode, emptied);
			}
			else
			{
				UpdatePropertyFromJson(reload.animIndex, value, "anim", altMode, emptied);
				UpdatePropertyFromJson(reload.duration, value, "duration", altMode, emptied);
				UpdatePropertyFromJson(reload.attackDelay, value, "attack_delay", altMode, emptied);
				UpdatePropertyFromJson(reload.idleDelay, value, "idle_delay", altMode, emptied);
				UpdatePropertyFromJson(reload.waitForRecoil, value, "wait_for_recoil", altMode, emptied);
				UpdatePropertyFromJson(reload.suspendLaserSpotTime, value, "laser_suspend_time", altMode, emptied);

				HandleJSONMember(value, "sound", [&](const Value& value) {
					WeaponSoundScript& soundScript = reload.sound.Materialize(altMode, emptied);
					ParseWeaponSoundScript(soundScript, value);
				});

				UpdatePropertyFromJson(reload.ammoCount, value, "ammo_count", altMode, emptied);
				UpdatePropertyFromJson(reload.ammoCountMin, value, "ammo_count_min", altMode, emptied);
			}
		});
	};

	HandleReload("reload", false, false);
	HandleReload("reload_empty", false, true);
	HandleReload("alt_reload", true, false);
	HandleReload("alt_reload_empty", true, true);

	auto HandleStartReload = [&](const char* propName, bool altMode, bool emptied) {
		HandleJSONMember(value, propName, [&](const Value& value) {
			auto& startReload = params.startReload;

			UpdatePropertyFromJson(startReload.animIndex, value, "anim", altMode, emptied);
			UpdatePropertyFromJson(startReload.duration, value, "duration", altMode, emptied);
		});
	};

	HandleStartReload("start_reload", false, false);
	HandleStartReload("start_reload_empty", false, true);
	HandleStartReload("alt_start_reload", true, false);
	HandleStartReload("alt_start_reload_empty", true, true);

	auto HandleEndReload = [&](const char* propName, bool altMode, bool emptied) {
		HandleJSONMember(value, propName, [&](const Value& value) {
			auto& endReload = params.endReload;

			UpdatePropertyFromJson(endReload.animIndex, value, "anim", altMode, emptied);
			UpdatePropertyFromJson(endReload.attackDelay, value, "attack_delay", altMode, emptied);
			UpdatePropertyFromJson(endReload.idleDelay, value, "idle_delay", altMode, emptied);

			HandleJSONMember(value, "sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = endReload.sound.Materialize(altMode, emptied);
				ParseWeaponSoundScript(soundScript, value);
			});
		});
	};

	HandleEndReload("end_reload", false, false);
	HandleEndReload("end_reload_empty", false, true);
	HandleEndReload("alt_end_reload", true, false);
	HandleEndReload("alt_end_reload_empty", true, true);

	HandleJSONMember(value, "manual_reload", [&](const Value& value) {
		if (value.IsBool())
		{
			params.manualReload = value.GetBool();
		}
		else if (value.IsObject())
		{
			params.manualReload = true;
			UpdatePropertyFromJson(params.manualReloadContinueOnDeploy, value, "continue_on_deploy");
			UpdatePropertyFromJson(params.manualReloadRestartOnDeploy, value, "restart_on_deploy");
		}
	});

	UpdatePropertyFromJson(params.reloadAutostart, value, "reload_autostart");
	UpdatePropertyFromJson(params.startInAltMode, value, "start_in_alt_mode");
	UpdatePropertyFromJson(params.mirrorViewModel, value, "mirror_viewmodel");

	auto HandleRecharge = [&](const char* propName, bool altMode) {
		HandleJSONMember(value, propName, [&](const Value& value) {
			auto& recharge = params.recharge;

			UpdatePropertyFromJson(recharge.interval, value, "interval", altMode);
			UpdatePropertyFromJson(recharge.delayAfterFire, value, "delay_after_fire", altMode);
			UpdatePropertyFromJson(recharge.onlyWhenDeployed, value, "only_when_deployed", altMode);

			HandleJSONMember(value, "sound", [&](const Value& value) {
				WeaponSoundScript& soundScript = recharge.sound.Materialize(altMode);
				ParseWeaponSoundScript(soundScript, value);
			});
		});
	};

	HandleRecharge("recharge", false);
	HandleRecharge("recharge_alt", true);

	HandleJSONMember(value, "laser_spot", [&](const Value& value) {
		UpdatePropertyFromJson(params.startLaserSpot, value, "start_on");
		UpdatePropertyFromJson(params.laserSpotAttractRockets, value, "attract_rockets");
		UpdatePropertyFromJson(params.laserSpotScale, value, "scale");

		HandleJSONMember(value, "activation_sound", [&](const Value& value) {
			ParseWeaponSoundScript(params.activateLaserSpotSound, value);
		});
		HandleJSONMember(value, "deactivation_sound", [&](const Value& value) {
			ParseWeaponSoundScript(params.deactivateLaserSpotSound, value);
		});
	});

	HandleJSONMember(value, "model_sounds", [&](const Value& value) {
		Value::ConstArray arr = value.GetArray();
		params.modelSoundsDefined = true;
		for (auto& item : arr)
		{
			params.modelSounds.push_back(item.GetString());
		}
	});

	HandleJSONMember(value, "tool", [&](const Value& value) {
		UpdatePropertyFromJson(params.toolIcon, value, "icon");
		UpdatePropertyFromJson(params.toolTriggerDelay, value, "trigger_delay");

		HandleJSONMember(value, "deny_sound", [&](const Value& value) {
			ParseWeaponSoundScript(params.toolDenySound, value);
		});
		UpdatePropertyFromJson(params.toolDelayAfterDeny, value, "delay_after_deny");
	});

	auto HandleDropAmmo = [&](const char* propName, WeaponParameters::DropAmmoEnt& dropAmmo) {
		HandleJSONMember(value, propName, [&](const Value& value) {
			UpdatePropertyFromJson(dropAmmo.classname, value, "classname");
			UpdatePropertyFromJson(dropAmmo.entTemplate, value, "ent_template");
			UpdatePropertyFromJson(dropAmmo.amount, value, "amount");
		});
	};

	HandleDropAmmo("dropammo", params.dropAmmo);
	HandleDropAmmo("dropammo_secondary", params.dropAmmoSecondary);
}

bool WeaponTemplateSystem::ReadFromDocument(const Document &document, const char *fileName)
{
	_stringSet.clear();

	for (auto weaponIt = document.MemberBegin(); weaponIt != document.MemberEnd(); ++weaponIt)
	{
		const Value& value = weaponIt->value;
		const char* name = weaponIt->name.GetString();

		WeaponParameters* pParams = AccessWeaponParameters(name);
		if (pParams)
		{
			if (value.IsString())
			{
				std::string weaponFileName = "templates/weapons/";
				weaponFileName += value.GetString();
				if (weaponFileName.compare(weaponFileName.size()-5, 5, ".json") != 0)
				{
					weaponFileName += ".json";
				}
				WeaponSingleTemplate singleWeapon(pParams, this);
				const bool success = singleWeapon.ReadFromFile(weaponFileName.c_str());
				if (!success)
					LOG_ERROR("Failed to load %s\n", weaponFileName.c_str());
			}
			else
			{
				ParseWeaponTemplate(*pParams, value, fileName);
			}
		}
		else
		{
			LOG_WARNING("Can't set parameters for unknown weapon: %s\n", name);
		}
	}

	return true;
}

const char* WeaponTemplateSystem::MakeConstantString(const char* str)
{
	auto strIt = _stringSet.find(str);
	if (strIt == _stringSet.end())
	{
		auto p = _stringSet.insert(str);
		strIt = p.first;
	}
	return strIt->c_str();
}

WeaponTemplateSystem g_WeaponTemplateSystem;
