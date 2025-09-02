#include <gtest/gtest.h>
#include "weapon_templates.h"

std::map<std::string, WeaponParameters> g_weaponParameters;

WeaponParameters* AccessWeaponParameters(const char* name)
{
	auto it = g_weaponParameters.find(name);
	if (it != g_weaponParameters.end())
		return &it->second;
	return nullptr;
}

const char weaponTemplates[] = R"(
{
	"weapon_9mmhandgun_silencer":{
		"fire": {
			"spread": 0.04,
			"cycle_time": 0.25,
			"autoaim": "4degrees"
		},
		"alt_fire": {
			"sound": {
				"waves": ["weapons/pl_gun1.wav", "weapons/pl_gun2.wav"],
				"volume": [0.92, 1.0],
				"pitch": [98, 101]
			},
			"weapon_volume": "quiet",
			"weapon_flash": "dim",
			"spread": 0.01,
			"cycle_time": 0.3
		},
		"switch_mode": {
			"anim": 9,
			"attack_delay": 3.3,
			"body_switch_delay": 0.8,
			"mode_switch_delay": 2.9
		},
		"switch_mode_back": {
			"anim": 8,
			"attack_delay": 2.0,
			"body_switch_delay": 1.0,
			"mode_switch_delay": 1.0,
			"end_anim": 7,
			"end_anim_duration": 0.3
		},
		"secondary_attack": "switch_mode",
		"viewmodel_body_alt": 1
	},
	"weapon_9mmhandgun_cs": {
		"ammo_amount": 40,
		"max_clip": 20,
		"deploy": {
			"anim": 8
		},
		"idle": [
			{
				"anim": 0,
				"duration": 3.75,
				"chance": 0.3
			},
			{
				"anim": 1,
				"duration": 2.5,
				"chance": 0.4
			},
			{
				"anim": 2,
				"duration": 3.0625,
				"chance": 0.3
			}
		],
		"fire": {
			"anims": [5],
			"anims_last_shot": [6],
			"sound": ["weapons/glock18-2.wav"],
			"spread": {
				"inaccuracy": 0.1,
				"recovery_time": 0.325,
				"factor": 0.275,
				"max_inaccuracy": 0.4,
				"spread": [
					{
						"conditions": {
							"in_air": true
						},
						"factor": 1.0
					},
					{
						"conditions": {
							"moving": true
						},
						"factor": 0.165
					},
					{
						"conditions": {
							"ducking": true
						},
						"factor": 0.075
					},
					{
						"factor": 0.1
					}
				]
			},
			"cycle_time": 0.195,
			"idle_delay": 2.5,
			"semiauto": true,
			"delay_after_empty": 0.2,
			"client_punch_pitch": 0,
			"shell": {
				"offset": {
					"forward": 36,
					"up": -14,
					"side": 14
				}
			}
		},
		"alt_fire": {
			"burst": 3,
			"burst_interval": 0.1,
			"spread": {
				"inaccuracy": 0.1,
				"recovery_time": 0.325,
				"factor": 0.275,
				"max_inaccuracy": 0.4,
				"spread": [
					{
						"conditions": {
							"in_air": true
						},
						"factor": 1.2
					},
					{
						"conditions": {
							"moving": true
						},
						"factor": 0.185
					},
					{
						"conditions": {
							"ducking": true
						},
						"factor": 0.095
					},
					{
						"factor": 0.3
					}
				]
			},
			"cycle_time": 0.5,
			"idle_delay": 2.5,
			"semiauto": false
		},
		"reload": {
			"anim": 7,
			"duration": 2.2
		},
		"reload_empty": {
			"anim": 12
		},
		"mirror_viewmodel": true,
		"model_sounds": [
			"weapons/clipout1.wav",
			"weapons/clipin1.wav",
			"weapons/sliderelease1.wav",
			"weapons/slideback1.wav",
			"weapons/de_clipin.wav",
			"weapons/de_clipout.wav"
		]
	},
	"weapon_9mmAR": {
		"fire": {
			"spread": {
				"inaccuracy": 0,
				"max_inaccuracy": 0.75,
				"inaccuracy_shift": 0.45,
				"type": "quadratic",
				"divisor": 200,
				"spread": [
					{
						"conditions": {
							"in_air": true
						},
						"factor": 0.2
					},
					{
						"conditions": {
							"moving": 140
						},
						"factor": 0.1
					},
					{
						"factor": 0.04
					}
				]
			},
			"weapon_flash": 200,
			"kickback": [
				{
					"conditions": {
						"in_air": true
					},
					"kickback": {
						"vertical": 0.9,
						"lateral": 0.475,
						"vertical_modifier": 0.35,
						"lateral_modifier": 0.0425,
						"vertical_max": 5,
						"lateral_max": 3,
						"lateral_persistance": 6
					}
				},
				{
					"conditions": {
						"moving": true
					},
					"kickback": {
						"vertical": 0.5,
						"lateral": 0.275,
						"vertical_modifier": 0.2,
						"lateral_modifier": 0.03,
						"vertical_max": 3,
						"lateral_max": 2,
						"lateral_persistance": 10
					}
				},
				{
					"conditions": {
						"ducking": true
					},
					"kickback": {
						"vertical": 0.225,
						"lateral": 0.15,
						"vertical_modifier": 0.1,
						"lateral_modifier": 0.015,
						"vertical_max": 2,
						"lateral_max": 1,
						"lateral_persistance": 10
					}
				},
				{
					"kickback": {
						"vertical": 0.25,
						"lateral": 0.175,
						"vertical_modifier": 0.112,
						"lateral_modifier": 0.02,
						"vertical_max": 2.25,
						"lateral_max": 1.25,
						"lateral_persistance": 10
					}
				}
			]
		}
	},
	"weapon_test": {
		"ammo_amount": [5,6],
		"world_model": "w_test.mdl",
		"view_model": "v_test.mdl",
		"player_model": "p_test.mdl",
		"player_anim_ext": "shotgun",
		"priority": 42,
		"fire": {
			"type": "bullet",
			"damage": 13,
			"spread": "5degree",
			"idle_delay": [1.0, 2.0],
			"ammo_per_fire": 2,
			"allow_underwater": false,
			"bullet_count": 5,
			"muzzleflash": true,
			"weapon_volume": "normal",
			"weapon_flash": "normal",
			"delay_underwater": 0.2,
			"pump_delay": 0.3,
			"pump_sound": ["weapons/pump.wav"],
			"bullet_distance": 3000,
			"client_punch_pitch": -2,
			"client_punch_yaw": -3,
			"shell": {
				"count": 2,
				"model": "models/custom_shell.mdl",
				"model_alternating": "models/alt_shell.mdl",
				"sound_type": "shotgun_shell",
				"left_side": true,
				"velocity": {
					"up": [0, 15],
					"side": [170, 200],
					"forward": [25, 250]
				}
			},
			"pushback_force": 100,
			"pushback_vertical": true
		},
		"alt_fire": {
			"damage": 7,
			"sound_additional": {
				"waves": ["weapons/test.wav"]
			},
			"spread": {
				"vertical": 0.04,
				"lateral": 0.03
			},
			"allow_underwater": true,
			"use_secondary_ammo": true,
			"weapon_volume": "loud",
			"weapon_flash": "bright"
		},
		"zoom": {
			"fov": 30,
			"sound": ["weapons/zoom.wav"],
			"unzoom_sound": ["weapons/unzoom.wav"],
			"reset_on_fire": true,
			"fade": {
				"color": [0, 100, 50],
				"alpha": 240,
				"fade_time": 0.2,
				"hold_time": 0.1
			}
		},
		"laser_spot": {
			"start_on": true,
			"attract_rockets": true,
			"scale": 0.6,
			"activation_sound": ["weapons/activate_laser.wav"],
			"deactivation_sound": ["weapons/deactivate_laser.wav"]
		},
		"switch_mode_common": {
			"toggle_laser_spot": true
		},
		"manual_reload": true,
		"reload_autostart": true,
		"start_in_alt_mode": true
	},
	"weapon_test2": {
		"fire": {
			"type": "melee",
			"hit_body_sound": ["hitbody.wav"],
			"hit_wall_sound": ["hitwall.wav"],
			"spread": {
				"vertical": "4degrees",
				"lateral": "3degrees"
			},
			"kickback": 2
		},
		"alt_fire": {
			"type": "bullets",
			"spread": [
				{
					"conditions": {
						"in_air": true
					},
					"spread": {
						"vertical": "6degrees",
						"lateral": "5degrees"
					}
				},
				{
					"conditions": {
						"moving": true
					},
					"spread": {
						"vertical": 0.04,
						"lateral": 0.03
					}
				},
				{
					"conditions": {
						"ducking": true
					},
					"spread": "1degree"
				},
				{
					"spread": "2degrees"
				}
			],
			"kickback": {
				"vertical": 3,
				"lateral": 1
			}
		},
		"manual_reload": {
			"restart_on_deploy": true
		},
		"recharge": {
			"interval": 0.5,
			"delay_after_fire": 2.0,
			"only_when_deployed": true,
			"sound": ["weapons/recharge.wav"]
		},
		"tool": {
			"icon": "test",
			"trigger_delay": 1.2
		}
	}
}
)";

TEST(Weapons, Parse) {
	g_weaponParameters["weapon_9mmhandgun_silencer"] = WeaponParameters();
	g_weaponParameters["weapon_9mmhandgun_cs"] = WeaponParameters();
	g_weaponParameters["weapon_9mmAR"] = WeaponParameters();
	g_weaponParameters["weapon_test"] = WeaponParameters();
	g_weaponParameters["weapon_test2"] = WeaponParameters();

	WeaponTemplateSystem s;

	ASSERT_TRUE(s.ReadFromContents(weaponTemplates, ""));

	{
		const WeaponParameters* pSilencerParams = AccessWeaponParameters("weapon_9mmhandgun_silencer");
		ASSERT_TRUE(pSilencerParams != nullptr);

		const WeaponParameters& silencerParams = *pSilencerParams;

		EXPECT_EQ(silencerParams.fire.spread.SpreadType(false), WeaponSpread::Type::STATIC);
		EXPECT_FALSE(silencerParams.fire.spread.UsesDynamicInaccuracy(false));
		{
			auto& ruleList = silencerParams.fire.spread.GetRuleList(false);
			EXPECT_EQ(ruleList.size(), 1);
			EXPECT_EQ(ruleList.back().GetStaticSpread().x, 0.04f);
			EXPECT_EQ(ruleList.back().GetStaticSpread().y, 0.04f);
		}

		EXPECT_EQ(silencerParams.fire.cycleTime.main, 0.25f);
		EXPECT_EQ(silencerParams.fire.autoAimDegree.main, AUTOAIM_4DEGREES);

		ASSERT_TRUE(silencerParams.fire.sound.alt.has_value());
		const WeaponSoundScript& fireSound = *silencerParams.fire.sound.alt;

		ASSERT_EQ(fireSound.waves.size(), 2);
		EXPECT_STREQ(fireSound.waves[0], "weapons/pl_gun1.wav");
		EXPECT_STREQ(fireSound.waves[1], "weapons/pl_gun2.wav");
		EXPECT_EQ(fireSound.volume, FloatRange(0.92f, 1.0f));
		EXPECT_EQ(fireSound.pitch, IntRange(98, 101));

		EXPECT_EQ(silencerParams.fire.weaponVolume.alt, QUIET_GUN_VOLUME);
		EXPECT_EQ(silencerParams.fire.weaponFlash.alt, DIM_GUN_FLASH);

		EXPECT_EQ(silencerParams.fire.spread.SpreadType(true), WeaponSpread::Type::STATIC);
		EXPECT_FALSE(silencerParams.fire.spread.UsesDynamicInaccuracy(true));
		{
			auto& ruleList = silencerParams.fire.spread.GetRuleList(true);
			EXPECT_EQ(ruleList.size(), 1);
			EXPECT_EQ(ruleList.back().GetStaticSpread().x, 0.01f);
			EXPECT_EQ(ruleList.back().GetStaticSpread().y, 0.01f);
		}

		EXPECT_EQ(silencerParams.fire.cycleTime.alt, 0.3f);

		EXPECT_EQ(silencerParams.altMode.animIndex.main, 9);
		EXPECT_EQ(silencerParams.altMode.attackDelay.main, 3.3f);
		EXPECT_EQ(silencerParams.altMode.bodyDelay.main, 0.8f);
		EXPECT_EQ(silencerParams.altMode.modeDelay.main, 2.9f);

		EXPECT_EQ(silencerParams.altMode.animIndex.alt, 8);
		EXPECT_EQ(silencerParams.altMode.attackDelay.alt, 2.0f);
		EXPECT_EQ(silencerParams.altMode.bodyDelay.alt, 1.0f);
		EXPECT_EQ(silencerParams.altMode.modeDelay.alt, 1.0f);
		EXPECT_EQ(silencerParams.altMode.endAnimIndex.alt, 7);
		EXPECT_EQ(silencerParams.altMode.endAnimDuration.alt, 0.3f);

		EXPECT_EQ(silencerParams.secondaryFireType, SecondaryFireType::SWITCH_MODE);
		EXPECT_EQ(silencerParams.viewModelBody.alt, 1);
	}

	{
		const WeaponParameters* pGlockParams = AccessWeaponParameters("weapon_9mmhandgun_cs");
		ASSERT_TRUE(pGlockParams != nullptr);

		const WeaponParameters& glockParams = *pGlockParams;

		EXPECT_EQ(glockParams.initialAmmoAmount, 40);
		EXPECT_EQ(glockParams.maxClip, 20);

		EXPECT_EQ(glockParams.deploy.animIndex.Get(false, false), 8);

		auto& idleAnims = glockParams.idleAnims.Get(false, false);
		ASSERT_EQ(idleAnims.size(), 3);

		EXPECT_EQ(idleAnims[0].animIndex, 0);
		EXPECT_EQ(idleAnims[0].duration, 3.75f);
		EXPECT_EQ(idleAnims[0].chance, 0.3f);

		EXPECT_EQ(idleAnims[1].animIndex, 1);
		EXPECT_EQ(idleAnims[2].animIndex, 2);

		auto& fire = glockParams.fire;

		ASSERT_EQ(fire.anims.Get(false, false).size(), 1);
		EXPECT_EQ(fire.anims.Get(false, false)[0], 5);

		ASSERT_EQ(fire.anims.Get(false, true).size(), 1);
		EXPECT_EQ(fire.anims.Get(false, true)[0], 6);

		auto& fireSound = fire.sound.Get(false);
		ASSERT_EQ(fireSound.waves.size(), 1);
		EXPECT_STREQ(fireSound.waves[0], "weapons/glock18-2.wav");

		EXPECT_EQ(fire.spread.SpreadType(false), WeaponSpread::RECOVERING);

		EXPECT_GT(fire.spread.GetNewInaccuracy(false, 0.1f, 1, 0.1f, 0.2f), 0.1f);
		EXPECT_EQ(fire.spread.GetNewInaccuracy(false, 0.1f, 1, 0.5f, 1.0f), 0.1f);

		auto& ruleList = fire.spread.GetRuleList(false);
		ASSERT_EQ(ruleList.size(), 4);

		EXPECT_EQ(ruleList[0].GetDynamicSpread(0.1f).x, 0.1f);
		EXPECT_FALSE(indeterminate(ruleList[0].Conditions().inAir));
		EXPECT_TRUE(ruleList[0].Conditions().inAir);
		EXPECT_EQ(ruleList[1].Conditions().moving, 0.0f);
		EXPECT_FALSE(indeterminate(ruleList[2].Conditions().ducking));
		EXPECT_TRUE(ruleList[2].Conditions().ducking);

		EXPECT_TRUE(indeterminate(ruleList[3].Conditions().inAir));
		EXPECT_TRUE(indeterminate(ruleList[3].Conditions().ducking));
		EXPECT_LE(ruleList[3].Conditions().moving, 0.0f);
		EXPECT_FLOAT_EQ(ruleList[3].GetDynamicSpread(0.1f).x, 0.01f);

		EXPECT_EQ(fire.cycleTime.Get(false), 0.195f);
		EXPECT_EQ(fire.idleDelay.Get(false, false).min, 2.5f);
		EXPECT_TRUE(fire.semiAuto.Get(false));
		EXPECT_EQ(fire.delayAfterEmpty.Get(false), 0.2f);
		EXPECT_EQ(fire.shellOffsetForward.Get(false), 36.0f);
		EXPECT_EQ(fire.shellOffsetUp.Get(false), -14.0f);
		EXPECT_EQ(fire.shellOffsetSide.Get(false), 14.0f);

		EXPECT_EQ(fire.burstShots.Get(true), 3);
		EXPECT_EQ(fire.burstInterval.Get(true), 0.1f);
		EXPECT_EQ(fire.cycleTime.Get(true), 0.5f);
		EXPECT_FALSE(fire.semiAuto.Get(true));

		auto& reload = glockParams.reload;
		EXPECT_EQ(reload.animIndex.Get(false, false), 7);
		EXPECT_EQ(reload.duration.Get(false, false), 2.2f);
		EXPECT_EQ(reload.animIndex.Get(false, true), 12);

		EXPECT_TRUE(glockParams.mirrorViewModel);

		EXPECT_EQ(glockParams.modelSounds.size(), 6);
		EXPECT_STREQ(glockParams.modelSounds[0].c_str(), "weapons/clipout1.wav");
	}

	{
		const WeaponParameters* pMP5Params = AccessWeaponParameters("weapon_9mmAR");
		ASSERT_TRUE(pMP5Params != nullptr);

		const WeaponParameters& mp5Params = *pMP5Params;

		const WeaponParameters::Fire& fire = mp5Params.fire;

		EXPECT_EQ(fire.weaponFlash.Get(false), 200);

		EXPECT_EQ(fire.spread.SpreadType(false), WeaponSpread::QUADRATIC);
		EXPECT_FLOAT_EQ(fire.spread.GetNewInaccuracy(false, 0.0f, 5, 0.0f, 1.0f), 0.575f);
		EXPECT_FLOAT_EQ(fire.spread.GetNewInaccuracy(false, 0.0f, 10, 0.0f, 1.0f), 0.75f);

		auto& ruleList = fire.spread.GetRuleList(false);
		ASSERT_EQ(ruleList.size(), 3);
		EXPECT_FLOAT_EQ(ruleList[0].GetDynamicSpread(0.1f).x, 0.02f);

		EXPECT_EQ(ruleList[1].Conditions().moving, 140.0f);

		auto& kickBackList = fire.kickBack.GetRuleList(false);
		ASSERT_EQ(kickBackList.size(), 4);

		EXPECT_TRUE(kickBackList[0].conditions.inAir);
		EXPECT_EQ(kickBackList[0].kickBack.verticalBase, 0.9f);
		EXPECT_EQ(kickBackList[0].kickBack.lateralBase, 0.475f);
		EXPECT_EQ(kickBackList[0].kickBack.verticalModifier, 0.35f);
		EXPECT_EQ(kickBackList[0].kickBack.lateralModifier, 0.0425f);
		EXPECT_EQ(kickBackList[0].kickBack.verticalMax, 5.0f);
		EXPECT_EQ(kickBackList[0].kickBack.lateralMax, 3.0f);
		EXPECT_EQ(kickBackList[0].kickBack.directionChangeLateral, 6);

		EXPECT_EQ(kickBackList[1].conditions.moving, 0.0f);
		EXPECT_TRUE(kickBackList[2].conditions.ducking);

		EXPECT_TRUE(indeterminate(kickBackList[3].conditions.inAir));
		EXPECT_TRUE(indeterminate(kickBackList[3].conditions.ducking));
		EXPECT_LE(kickBackList[3].conditions.moving, 0.0f);

		EXPECT_EQ(kickBackList[3].kickBack.verticalBase, 0.25f);
		EXPECT_EQ(kickBackList[3].kickBack.lateralBase, 0.175f);
		EXPECT_EQ(kickBackList[3].kickBack.verticalModifier, 0.112f);
		EXPECT_EQ(kickBackList[3].kickBack.lateralModifier, 0.02f);
		EXPECT_EQ(kickBackList[3].kickBack.verticalMax, 2.25f);
		EXPECT_EQ(kickBackList[3].kickBack.lateralMax, 1.25f);
		EXPECT_EQ(kickBackList[3].kickBack.directionChangeLateral, 10);
	}

	{
		const WeaponParameters* pTestParams = AccessWeaponParameters("weapon_test");
		ASSERT_TRUE(pTestParams != nullptr);

		const WeaponParameters& testParams = *pTestParams;

		EXPECT_EQ(testParams.initialAmmoAmount, IntRange(5, 6));

		EXPECT_EQ(testParams.worldModel, "w_test.mdl");
		EXPECT_EQ(testParams.viewModel, "v_test.mdl");
		EXPECT_EQ(testParams.playerModel, "p_test.mdl");
		EXPECT_EQ(testParams.playerAnimExt, "shotgun");
		EXPECT_EQ(testParams.priority, 42);

		const WeaponParameters::Fire& fire = testParams.fire;

		EXPECT_EQ(fire.fireType.Get(false), WeaponParameters::Fire::BULLETS);
		EXPECT_EQ(fire.damage.Get(false), 13.0f);
		ASSERT_EQ(fire.spread.GetRuleList(false).size(), 1);
		EXPECT_EQ(fire.spread.GetRuleList(false).front().GetStaticSpread().x, VECTOR_CONE_5DEGREES.x);
		EXPECT_EQ(fire.spread.GetRuleList(false).front().GetStaticSpread().y, VECTOR_CONE_5DEGREES.y);
		EXPECT_EQ(fire.idleDelay.Get(false, false), FloatRange(1.0f, 2.0f));
		EXPECT_EQ(fire.ammoPerFire.Get(false), 2);
		EXPECT_FALSE(fire.allowUnderwater.Get(false));
		EXPECT_TRUE(fire.muzzleFlash.Get(false));
		EXPECT_EQ(fire.weaponVolume.Get(false), NORMAL_GUN_VOLUME);
		EXPECT_EQ(fire.weaponFlash.Get(false), NORMAL_GUN_FLASH);
		EXPECT_EQ(fire.delayUnderwater.Get(false), 0.2f);
		EXPECT_EQ(fire.pumpDelay.Get(false), 0.3f);
		ASSERT_EQ(fire.pumpSound.Get(false).waves.size(), 1);
		EXPECT_STREQ(fire.pumpSound.Get(false).waves[0], "weapons/pump.wav");
		EXPECT_EQ(fire.bulletDistance.Get(false), 3000);
		EXPECT_EQ(fire.clientPunchPitch.Get(false), -2.0f);
		EXPECT_EQ(fire.clientPunchYaw.Get(false), -3.0f);
		EXPECT_EQ(fire.shellCount.Get(false), 2);
		EXPECT_STREQ(fire.shellModel.Get(false), "models/custom_shell.mdl");
		EXPECT_STREQ(fire.shellModelAlternating.Get(false), "models/alt_shell.mdl");
		EXPECT_EQ(fire.shellSound.Get(false), TE_BOUNCE_SHOTSHELL);
		EXPECT_TRUE(fire.shellLeftSide.Get(false));
		EXPECT_EQ(fire.shellVelocityUp.Get(false), FloatRange(0.0f, 15.0f));
		EXPECT_EQ(fire.shellVelocitySide.Get(false), FloatRange(170.0f, 200.0f));
		EXPECT_EQ(fire.shellVelocityForward.Get(false), FloatRange(25.0f, 250.0f));
		EXPECT_EQ(fire.pushbackForce.Get(false), 100.0f);
		EXPECT_TRUE(fire.pushbackVertical.Get(false));

		EXPECT_EQ(fire.damage.Get(true), 7.0f);
		ASSERT_EQ(fire.soundAdditional.Get(true).waves.size(), 1);
		EXPECT_STREQ(fire.soundAdditional.Get(true).waves[0], "weapons/test.wav");
		EXPECT_EQ(fire.soundAdditional.Get(true).channel, CHAN_ITEM);
		ASSERT_EQ(fire.spread.GetRuleList(true).size(), 1);
		EXPECT_EQ(fire.spread.GetRuleList(true).front().GetStaticSpread().x, 0.04f);
		EXPECT_EQ(fire.spread.GetRuleList(true).front().GetStaticSpread().y, 0.03f);
		EXPECT_TRUE(fire.allowUnderwater.Get(true));
		EXPECT_TRUE(fire.useSecondaryAmmo.Get(true));
		EXPECT_EQ(fire.weaponVolume.Get(true), LOUD_GUN_VOLUME);
		EXPECT_EQ(fire.weaponFlash.Get(true), BRIGHT_GUN_FLASH);

		EXPECT_TRUE(testParams.manualReload);
		EXPECT_TRUE(testParams.reloadAutostart);
		EXPECT_TRUE(testParams.startInAltMode);

		EXPECT_EQ(testParams.altMode.zoomFOV, 30);
		ASSERT_EQ(testParams.altMode.zoomSound.waves.size(), 1);
		EXPECT_STREQ(testParams.altMode.zoomSound.waves[0], "weapons/zoom.wav");
		ASSERT_EQ(testParams.altMode.unzoomSound.waves.size(), 1);
		EXPECT_STREQ(testParams.altMode.unzoomSound.waves[0], "weapons/unzoom.wav");
		EXPECT_TRUE(testParams.altMode.resetZoomOnFire);
		EXPECT_EQ(testParams.altMode.zoomFade.color, Color3(0, 100, 50));
		EXPECT_EQ(testParams.altMode.zoomFade.alpha, 240);
		EXPECT_EQ(testParams.altMode.zoomFade.fadeTime, 0.2f);
		EXPECT_EQ(testParams.altMode.zoomFade.holdTime, 0.1f);

		EXPECT_TRUE(testParams.startLaserSpot);
		EXPECT_TRUE(testParams.laserSpotAttractRockets);
		EXPECT_EQ(testParams.laserSpotScale, 0.6f);
		ASSERT_EQ(testParams.activateLaserSpotSound.waves.size(), 1);
		EXPECT_STREQ(testParams.activateLaserSpotSound.waves[0], "weapons/activate_laser.wav");
		ASSERT_EQ(testParams.deactivateLaserSpotSound.waves.size(), 1);
		EXPECT_STREQ(testParams.deactivateLaserSpotSound.waves[0], "weapons/deactivate_laser.wav");

		EXPECT_TRUE(testParams.altMode.toggleLaserSpot);
	}

	{
		const WeaponParameters* pTestParams = AccessWeaponParameters("weapon_test2");
		ASSERT_TRUE(pTestParams != nullptr);

		const WeaponParameters& testParams = *pTestParams;

		const WeaponParameters::Fire& fire = testParams.fire;

		EXPECT_EQ(fire.fireType.Get(false), WeaponParameters::Fire::MELEE);
		ASSERT_EQ(fire.hitBodySound.Get(false).waves.size(), 1);
		EXPECT_STREQ(fire.hitBodySound.Get(false).waves[0], "hitbody.wav");
		ASSERT_EQ(fire.hitWallSound.Get(true).waves.size(), 1);
		EXPECT_STREQ(fire.hitWallSound.Get(false).waves[0], "hitwall.wav");
		ASSERT_EQ(fire.spread.GetRuleList(false).size(), 1);
		EXPECT_EQ(fire.spread.GetRuleList(false).front().GetStaticSpread().x, VECTOR_CONE_4DEGREES.x);
		EXPECT_EQ(fire.spread.GetRuleList(false).front().GetStaticSpread().y, VECTOR_CONE_3DEGREES.x);
		ASSERT_EQ(fire.kickBack.GetRuleList(false).size(), 1);
		EXPECT_EQ(fire.kickBack.GetRuleList(false)[0].kickBack.verticalBase, 2.0f);
		EXPECT_EQ(fire.kickBack.GetRuleList(false)[0].kickBack.lateralBase, 2.0f);

		EXPECT_EQ(fire.fireType.Get(true), WeaponParameters::Fire::BULLETS);
		ASSERT_EQ(fire.spread.GetRuleList(true).size(), 4);
		EXPECT_EQ(fire.spread.GetRuleList(true)[0].GetStaticSpread().x, VECTOR_CONE_6DEGREES.x);
		EXPECT_EQ(fire.spread.GetRuleList(true)[0].GetStaticSpread().y, VECTOR_CONE_5DEGREES.x);
		EXPECT_EQ(fire.spread.GetRuleList(true)[1].GetStaticSpread().x, 0.04f);
		EXPECT_EQ(fire.spread.GetRuleList(true)[1].GetStaticSpread().y, 0.03f);
		EXPECT_EQ(fire.spread.GetRuleList(true)[2].GetStaticSpread().x, VECTOR_CONE_1DEGREES.x);
		EXPECT_EQ(fire.spread.GetRuleList(true)[2].GetStaticSpread().y, VECTOR_CONE_1DEGREES.x);
		EXPECT_EQ(fire.spread.GetRuleList(true)[3].GetStaticSpread().x, VECTOR_CONE_2DEGREES.x);
		EXPECT_EQ(fire.spread.GetRuleList(true)[3].GetStaticSpread().y, VECTOR_CONE_2DEGREES.x);
		ASSERT_EQ(fire.kickBack.GetRuleList(true).size(), 1);
		EXPECT_EQ(fire.kickBack.GetRuleList(true)[0].kickBack.verticalBase, 3.0f);
		EXPECT_EQ(fire.kickBack.GetRuleList(true)[0].kickBack.lateralBase, 1.0f);

		EXPECT_TRUE(testParams.manualReload);
		EXPECT_TRUE(testParams.manualReloadRestartOnDeploy);

		EXPECT_EQ(testParams.recharge.interval.Get(false), 0.5f);
		EXPECT_EQ(testParams.recharge.delayAfterFire.Get(false), 2.0f);
		EXPECT_TRUE(testParams.recharge.onlyWhenDeployed.Get(false));
		ASSERT_EQ(testParams.recharge.sound.Get(false).waves.size(), 1);
		EXPECT_STREQ(testParams.recharge.sound.Get(false).waves[0], "weapons/recharge.wav");

		EXPECT_EQ(testParams.toolIcon, "test");
		EXPECT_EQ(testParams.toolTriggerDelay, 1.2f);
	}
}
