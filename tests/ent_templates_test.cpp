#include <gtest/gtest.h>
#include "ent_templates.h"
#include "blood_types.h"
#include "classify.h"
#include "dmg_types.h"
#include "gib.h"
#include "hitgroup.h"
#include "grapple_target.h"

#include <algorithm>

const char entities[] = R"(
{
	"female_civ": {
		"own_visual": {
			"model": "models/us_gal.mdl"
		},
		"soundscripts": {
			"Civilian.Pain": {
				"waves": ["colette/colette_pain0.wav", "colette/colette_pain1.wav"]
			},
			"Civilian.Die": "Female.Die"
		},
		"speech_prefix": "FEM",
		"pain": {
			"delay": [0.5, 1.5],
			"chance": 0.6,
			"lower_bound_dmg": 0.1,
			"allow_when_dying": true
		}
	},
	"vort_friendly": {
		"classify": "Player Ally",
		"field_of_view": "full",
		"size_for_grapple": "no",
		"visuals": {
			"Vortigaunt.ZapBeamColor": {
				"color": [255, 96, 180]
			},
			"Vortigaunt.ArmBeamColor": "Friendly.BeamColor"
		}
	},
	"monster_gonome": {
		"precached_sounds": ["gonome/gonome_step1.wav", "gonome/gonome_step2.wav"],
		"blood": "red",
		"health": 225,
		"field_of_view": 0.3
	},
	"monster_gargantua": {
		"gib_visual": {
			"model": "models/garg_gibs.mdl"
		},
		"blood": "no",
		"size": {
			"mins": [-40,-40,0],
			"maxs": [40,40,214]
		}
	},
	"small_bullsquid": {
		"size": "small",
		"size_for_grapple": "small"
	},
	"long_arms": {
		"check_melee_attack1": {
			"distance": 100,
			"dot": 0.6
		},
		"check_melee_attack2": {
			"distance": 110
		},
		"trace_hull_attacks": {
			"10": {
				"distance": 105,
				"height": "*0.8",
				"punchangle": {
					"pitch": -5,
					"roll": -36
				},
				"knock": {
					"right": 150,
					"forward": -200,
					"up": 100
				},
				"spawn_blood": true,
				"damage_info": {
					"damage": 42,
					"type": ["acid", "poison"],
					"nonlethal": true,
					"ignore_armor": true,
					"gib": "never"
				},
				"hit_soundscript": {
					"waves": ["long_arms/hit1.wav", "long_arms/hit2.wav"]
				},
				"miss_soundscript": {
					"waves": ["long_arms/miss1.wav", "long_arms/miss2.wav"]
				}
			},
			"11": {
				"height": 64,
				"damage_info": {
					"type": "burn",
					"type_policy": "replace",
					"gib": "always"
				}
			}
		}
	},
	"monster_zombie": {
		"loot_drop": {
			"items": [
				{
					"classname": "ammo_buckshot",
					"weight": 1
				},
				{
					"classname": "item_healthkit",
					"ent_template": "custom_healthkit",
					"weight": 0.5
				},
				{
					"classname": "item_pickup",
					"pickup_name": "battery_blue",
					"weight": 1.5
				},
				{
					"classname": "weapon_shotgun",
					"weight": 2
				}
			],
			"max_weight": 2
		}
	},
	"monster_zombie_barney": {
		"loot_drop": [
			{
				"classname": "ammo_9mmAR",
				"chance": 0.6
			},
			{
				"classname": "item_battery",
				"chance": 0.3
			},
			{
				"classname": "item_pickup",
				"pickup_name": "battery_red",
				"chance": 0.2
			}
		]
	}
}
)";

TEST(EntityTemplates, Parse)
{
	SoundScriptSystem ss;
	VisualSystem vs;
	EntTemplateSystem es;
	es.SetSoundScriptSystem(&ss);
	es.SetVisualSystem(&vs);

	ASSERT_TRUE(es.ReadFromContents(entities, ""));

	{
		const EntTemplate* femaleCiv = es.GetTemplate("female_civ");
		ASSERT_TRUE(femaleCiv != nullptr);

		EXPECT_STREQ(femaleCiv->OwnVisualName(), "female_civ##own_visual");
		const Visual* ownVisual = vs.GetVisual("female_civ##own_visual");
		ASSERT_TRUE(ownVisual != nullptr);
		EXPECT_STREQ(ownVisual->model, "models/us_gal.mdl");

		EXPECT_STREQ(femaleCiv->GetSoundScriptNameOverride("Civilian.Pain"), "female_civ#Civilian.Pain");
		const SoundScript* painSoundScript = ss.GetSoundScript("female_civ#Civilian.Pain");
		ASSERT_TRUE(painSoundScript != nullptr);
		EXPECT_EQ(painSoundScript->waves.size(), 2);
		EXPECT_STREQ(painSoundScript->waves[0], "colette/colette_pain0.wav");

		EXPECT_STREQ(femaleCiv->GetSoundScriptNameOverride("Civilian.Die"), "Female.Die");
		EXPECT_TRUE(femaleCiv->GetSoundScriptNameOverride("Nonexistent") == nullptr);

		EXPECT_EQ(femaleCiv->PrecachedSoundsBegin(), femaleCiv->PrecachedSoundsEnd());
		EXPECT_EQ(femaleCiv->PrecachedSoundScriptsBegin(), femaleCiv->PrecachedSoundScriptsEnd());

		EXPECT_FALSE(femaleCiv->IsClassifyDefined());
		EXPECT_FALSE(femaleCiv->IsBloodDefined());ASSERT_TRUE(femaleCiv != nullptr);
		EXPECT_FALSE(femaleCiv->IsHealthDefined());
		EXPECT_FALSE(femaleCiv->IsFielfOfViewDefined());
		EXPECT_FALSE(femaleCiv->IsSizeDefined());
		EXPECT_FALSE(femaleCiv->IsSizeForGrappleDefined());

		EXPECT_STREQ(femaleCiv->SpeechPrefix(), "FEM");

		EntTemplate::PainSoundRule rule = femaleCiv->GetPainSoundRule();
		ASSERT_TRUE(rule.delay.has_value());
		EXPECT_EQ(*rule.delay, FloatRange(0.5f, 1.5f));
		ASSERT_TRUE(rule.chance.has_value());
		EXPECT_EQ(*rule.chance, 0.6f);
		ASSERT_TRUE(rule.lowerBound.has_value());
		EXPECT_EQ(*rule.lowerBound, 0.1f);
		EXPECT_FALSE(indeterminate(rule.allowWhenDying));
		EXPECT_TRUE(rule.allowWhenDying);
	}

	{
		const EntTemplate* vortFriendly = es.GetTemplate("vort_friendly");
		ASSERT_TRUE(vortFriendly != nullptr);

		EXPECT_TRUE(vortFriendly->OwnVisualName() == nullptr);
		EXPECT_TRUE(vortFriendly->GibVisualName() == nullptr);

		EXPECT_TRUE(vortFriendly->IsClassifyDefined());
		EXPECT_EQ(vortFriendly->Classify(), CLASS_PLAYER_ALLY);

		EXPECT_TRUE(vortFriendly->IsFielfOfViewDefined());
		EXPECT_EQ(vortFriendly->FieldOfView(), -1.0f);

		EXPECT_TRUE(vortFriendly->IsSizeForGrappleDefined());
		EXPECT_EQ(vortFriendly->SizeForGrapple(), GRAPPLE_NOT_A_TARGET);

		EXPECT_STREQ(vortFriendly->GetVisualNameOverride("Vortigaunt.ZapBeamColor"), "vort_friendly#Vortigaunt.ZapBeamColor");
		EXPECT_STREQ(vortFriendly->GetVisualNameOverride("Vortigaunt.ArmBeamColor"), "Friendly.BeamColor");
	}

	{
		const EntTemplate* gonome = es.GetTemplate("monster_gonome");
		ASSERT_TRUE(gonome != nullptr);

		auto it = gonome->PrecachedSoundsBegin();
		EXPECT_EQ(*it, "gonome/gonome_step1.wav");
		++it;
		EXPECT_EQ(*it, "gonome/gonome_step2.wav");
		++it;
		EXPECT_EQ(it, gonome->PrecachedSoundsEnd());

		EXPECT_TRUE(gonome->IsBloodDefined());
		EXPECT_EQ(gonome->BloodColor(), BLOOD_COLOR_RED);

		EXPECT_TRUE(gonome->IsHealthDefined());
		EXPECT_EQ(gonome->Health(), 225.0f);

		EXPECT_TRUE(gonome->IsFielfOfViewDefined());
		EXPECT_EQ(gonome->FieldOfView(), 0.3f);
	}

	{
		const EntTemplate* garg = es.GetTemplate("monster_gargantua");
		ASSERT_TRUE(garg != nullptr);

		EXPECT_STREQ(garg->GibVisualName(), "monster_gargantua##gib_visual");
		const Visual* gibVisual = vs.GetVisual("monster_gargantua##gib_visual");
		ASSERT_TRUE(gibVisual != nullptr);
		EXPECT_STREQ(gibVisual->model, "models/garg_gibs.mdl");

		EXPECT_TRUE(garg->IsBloodDefined());
		EXPECT_EQ(garg->BloodColor(), DONT_BLEED);

		EXPECT_TRUE(garg->IsSizeDefined());
		EXPECT_EQ(garg->MinSize(), Vector(-40.0f, -40.0f, 0.0f));
		EXPECT_EQ(garg->MaxSize(), Vector(40.0f, 40.0f, 214.0f));
	}

	{
		const EntTemplate* smallSquid = es.GetTemplate("small_bullsquid");
		ASSERT_TRUE(smallSquid != nullptr);

		EXPECT_TRUE(smallSquid->IsSizeDefined());
		EXPECT_EQ(smallSquid->MinSize(), Vector(-16.0f, -16.0f, 0.0f));
		EXPECT_EQ(smallSquid->MaxSize(), Vector(16.0f, 16.0f, 36.0f));

		EXPECT_TRUE(smallSquid->IsSizeForGrappleDefined());
		EXPECT_EQ(smallSquid->SizeForGrapple(), GRAPPLE_SMALL);
	}

	{
		const EntTemplate* longArms = es.GetTemplate("long_arms");
		ASSERT_TRUE(longArms != nullptr);

		EntTemplate::CheckMeleeAttack checkMelee1 = longArms->GetCheckMeleeAttack1();
		EXPECT_TRUE(checkMelee1.distance.has_value());
		EXPECT_EQ(*checkMelee1.distance, 100.0f);
		EXPECT_TRUE(checkMelee1.dot.has_value());
		EXPECT_EQ(*checkMelee1.dot, 0.6f);

		EntTemplate::CheckMeleeAttack checkMelee2 = longArms->GetCheckMeleeAttack2();
		EXPECT_TRUE(checkMelee2.distance.has_value());
		EXPECT_EQ(*checkMelee2.distance, 110.0f);
		EXPECT_FALSE(checkMelee2.dot.has_value());

		const EntTemplate::TraceHullAttack* traceHullAttack = longArms->GetTraceHullAttackForEvent(10);
		ASSERT_TRUE(traceHullAttack != nullptr);

		EXPECT_TRUE(traceHullAttack->distance.has_value());
		EXPECT_EQ(*traceHullAttack->distance, 105.0f);

		EXPECT_TRUE(traceHullAttack->height.has_value());
		EXPECT_EQ(*traceHullAttack->height, 0.8f);
		EXPECT_TRUE(traceHullAttack->heightIsFactor);

		EXPECT_TRUE(traceHullAttack->punchAngle.pitch.has_value());
		EXPECT_EQ(*traceHullAttack->punchAngle.pitch, -5.0f);

		EXPECT_FALSE(traceHullAttack->punchAngle.yaw.has_value());

		EXPECT_TRUE(traceHullAttack->punchAngle.roll.has_value());
		EXPECT_EQ(*traceHullAttack->punchAngle.roll, -36.0f);

		EXPECT_TRUE(traceHullAttack->knock.right.has_value());
		EXPECT_EQ(*traceHullAttack->knock.right, 150.0f);

		EXPECT_TRUE(traceHullAttack->knock.forward.has_value());
		EXPECT_EQ(*traceHullAttack->knock.forward, -200.0f);

		EXPECT_TRUE(traceHullAttack->knock.up.has_value());
		EXPECT_EQ(*traceHullAttack->knock.up, 100.0f);

		EXPECT_TRUE(!indeterminate(traceHullAttack->spawnBlood));
		EXPECT_TRUE(traceHullAttack->spawnBlood);

		const EntTemplate::DamageInfo damageInfo = traceHullAttack->damageInfo;

		EXPECT_TRUE(damageInfo.damage.has_value());
		EXPECT_EQ(*damageInfo.damage, 42.0f);

		EXPECT_TRUE(damageInfo.type.has_value());
		EXPECT_EQ(*damageInfo.type, DMG_ACID|DMG_POISON);
		EXPECT_EQ(damageInfo.typePolicy, EntTemplate::DamageInfo::ADD_DAMAGE_TYPE);

		EXPECT_TRUE(!indeterminate(damageInfo.nonLethal));
		EXPECT_TRUE(damageInfo.nonLethal);

		EXPECT_TRUE(!indeterminate(damageInfo.ignoreArmor));
		EXPECT_TRUE(damageInfo.ignoreArmor);

		EXPECT_TRUE(damageInfo.gibPolicy.has_value());
		EXPECT_EQ(*damageInfo.gibPolicy, GIB_NEVER);

		EXPECT_EQ(traceHullAttack->hitSoundScript, "long_arms#trace_hull_attacks#10#hit_soundscript");
		EXPECT_EQ(traceHullAttack->missSoundScript, "long_arms#trace_hull_attacks#10#miss_soundscript");

		const EntTemplate::TraceHullAttack* traceHullAttack2 = longArms->GetTraceHullAttackForEvent(11);
		ASSERT_TRUE(traceHullAttack2 != nullptr);

		EXPECT_TRUE(traceHullAttack2->height.has_value());
		EXPECT_EQ(*traceHullAttack2->height, 64.0f);
		EXPECT_FALSE(traceHullAttack2->heightIsFactor);

		const EntTemplate::DamageInfo damageInfo2 = traceHullAttack2->damageInfo;
		EXPECT_TRUE(damageInfo2.type.has_value());
		EXPECT_EQ(*damageInfo2.type, DMG_BURN);
		EXPECT_EQ(damageInfo2.typePolicy, EntTemplate::DamageInfo::REPLACE_DAMAGE_TYPE);
		EXPECT_TRUE(damageInfo2.gibPolicy.has_value());
		EXPECT_EQ(*damageInfo2.gibPolicy, GIB_ALWAYS);
	}

	{
		const EntTemplate* zombie = es.GetTemplate("monster_zombie");
		ASSERT_TRUE(zombie != nullptr);

		const DropItemSet& dropItemSet = zombie->GetLootDrop();
		ASSERT_EQ(dropItemSet.items.size(), 4);
		EXPECT_EQ(dropItemSet.maxWeight, 2.0f);

		EXPECT_EQ(dropItemSet.items[0].classname, "ammo_buckshot");
		EXPECT_EQ(dropItemSet.items[0].weight, 1.0f);

		EXPECT_EQ(dropItemSet.items[1].classname, "item_healthkit");
		EXPECT_EQ(dropItemSet.items[1].entTemplate, "custom_healthkit");
		EXPECT_EQ(dropItemSet.items[1].weight, 0.5f);

		EXPECT_EQ(dropItemSet.items[2].classname, "item_pickup");
		EXPECT_EQ(dropItemSet.items[2].pickupName, "battery_blue");
		EXPECT_EQ(dropItemSet.items[2].weight, 1.5f);

		EXPECT_EQ(dropItemSet.items[3].classname, "weapon_shotgun");
		EXPECT_EQ(dropItemSet.items[3].weight, 2.0f);
	}

	{
		const EntTemplate* zombie = es.GetTemplate("monster_zombie_barney");
		ASSERT_TRUE(zombie != nullptr);

		const DropItemSet& dropItemSet = zombie->GetLootDrop();
		ASSERT_EQ(dropItemSet.items.size(), 3);
		EXPECT_EQ(dropItemSet.maxWeight, 0.0f);

		EXPECT_EQ(dropItemSet.items[0].classname, "ammo_9mmAR");
		EXPECT_EQ(dropItemSet.items[0].chance, 0.6f);

		EXPECT_EQ(dropItemSet.items[1].classname, "item_battery");
		EXPECT_EQ(dropItemSet.items[1].chance, 0.3f);

		EXPECT_EQ(dropItemSet.items[2].classname, "item_pickup");
		EXPECT_EQ(dropItemSet.items[2].pickupName, "battery_red");
		EXPECT_EQ(dropItemSet.items[2].chance, 0.2f);
	}

	{
		const EntTemplate* nonExistent = es.GetTemplate("nonexistent");
		EXPECT_TRUE(nonExistent == nullptr);
	}
}

const char entitiesTakeDamage[] = R"(
{
	"test": {
		"trace_attack": [
			{
				"conditions": {
					"dmg_type": "burn",
					"dmg": ">=10",
					"inflictor": {
						"classname": "test_classname",
						"ent_template": "test_template",
						"classify": "Alien Bioweapon"
					},
					"attacker": {
						"classname": ["classname1", "classname2"],
						"ent_template": ["template1", "template2"],
						"classify": ["Human Military", "Alien Military"],
						"is_combat_character": true
					},
					"self": {
						"body": [1, 3, {"bodygroup": 1, "submodel": 2}],
						"invert_body_check": true,
						"life_state": "alive"
					},
					"attack_affinity": ["enemy", "neutral", "self"],
					"hitgroup": ["left arm", "left leg"],
					"invert_hitgroup_check": true
				},
				"modifier": {
					"dmg": "*2.5",
					"dmg_min_threshold": 0.1,
					"no_blood": true,
					"hitgroup": "head"
				},
				"effects": {
					"tracer": {
						"chance": 0.6,
						"certain_on_new_frame": true,
						"variance": 0.2
					}
				},
				"threshold_effects": {
					"ricochet": {
						"chance": 0.6,
						"certain_on_new_frame": false,
						"scale": 1.5
					}
				}
			},
			{
				"conditions": {
					"dmg_type": ["bullet", "blast"],
					"dmg_type_match": "exact",
					"dmg": "<=2.5",
					"attacker": {
						"classname": ["same", "someone_else"],
						"ent_template": ["", "same"],
						"classify": ["same"]
					},
					"attack_affinity": "friendly"
				},
				"modifier": {
					"skip_damage": true
				}
			}
		],
		"take_damage": [
			{
				"conditions": {
					"dmg_type": ["acid", "poison"],
					"dmg_type_match": "all",
					"dmg": ">50",
					"attacker": {
						"classname": "same",
						"ent_template": "same",
						"classify": "same",
						"negate": true
					},
					"self": {
						"body": {
							"bodygroup": 0,
							"submodel": 1
						}
					},
					"gib": "normal"
				},
				"modifier": {
					"gib": "always",
					"dmg": "+10"
				}
			},
			{
				"conditions": {
					"dmg_type": ["freeze"],
					"dmg_type_match": "none",
					"dmg": "<1"
				},
				"modifier": {
					"gib": "never",
					"dmg": "=health"
				}
			}
		]
	}
}
)";

TEST(EntityTemplates, ParseTakeDamage)
{
	SoundScriptSystem ss;
	VisualSystem vs;
	EntTemplateSystem es;
	es.SetSoundScriptSystem(&ss);
	es.SetVisualSystem(&vs);

	ASSERT_TRUE(es.ReadFromContents(entitiesTakeDamage, ""));

	const EntTemplate* test = es.GetTemplate("test");
	ASSERT_TRUE(test != nullptr);

	ASSERT_TRUE(test->HasCustomTraceAttackRules());

	auto traceAttackRange = test->TraceAttackRulesRange();
	auto traceAttackIt = traceAttackRange.first;
	ASSERT_NE(traceAttackIt, traceAttackRange.second);

	{
		const EntTemplate::TraceAttackRule::Conditions& conditions = traceAttackIt->conditions;

		ASSERT_TRUE(conditions.dmgType.has_value());
		EXPECT_EQ(*conditions.dmgType, DMG_BURN);
		EXPECT_EQ(conditions.dmgTypeMatch, DamageTypeMatch::ONE);
		EXPECT_EQ(conditions.dmgComparison, ValueComparison::GREATER_OR_EQUAL);
		EXPECT_EQ(conditions.dmg, 10.0f);

		ASSERT_TRUE(conditions.inflictorFilter.has_value());
		const EntityFilter& inflictorFilter = *conditions.inflictorFilter;
		ASSERT_EQ(inflictorFilter.classnames.size(), 1);
		EXPECT_EQ(inflictorFilter.classnames[0], "test_classname");
		ASSERT_EQ(inflictorFilter.entTemplates.size(), 1);
		EXPECT_EQ(inflictorFilter.entTemplates[0], "test_template");
		ASSERT_EQ(inflictorFilter.classifications.size(), 1);
		EXPECT_EQ(inflictorFilter.classifications[0], CLASS_ALIEN_BIOWEAPON);

		ASSERT_TRUE(conditions.attackerFilter.has_value());
		const EntityFilter& attackerFilter = *conditions.attackerFilter;
		ASSERT_EQ(attackerFilter.classnames.size(), 2);
		EXPECT_EQ(attackerFilter.classnames[0], "classname1");
		EXPECT_EQ(attackerFilter.classnames[1], "classname2");
		ASSERT_EQ(attackerFilter.entTemplates.size(), 2);
		EXPECT_EQ(attackerFilter.entTemplates[0], "template1");
		EXPECT_EQ(attackerFilter.entTemplates[1], "template2");
		ASSERT_EQ(attackerFilter.classifications.size(), 2);
		EXPECT_EQ(attackerFilter.classifications[0], CLASS_HUMAN_MILITARY);
		EXPECT_EQ(attackerFilter.classifications[1], CLASS_ALIEN_MILITARY);
		EXPECT_TRUE(attackerFilter.isCombatCharacter);

		ASSERT_TRUE(conditions.selfFilter.has_value());

		const EntityFilter& selfFilter = *conditions.selfFilter;
		ASSERT_EQ(selfFilter.bodyFilter.size(), 3);
		EXPECT_EQ(selfFilter.bodyFilter[0], EntityFilter::BodyFilter(1));
		EXPECT_EQ(selfFilter.bodyFilter[1], EntityFilter::BodyFilter(3));
		EXPECT_EQ(selfFilter.bodyFilter[2], EntityFilter::BodyFilter(1, 2));
		EXPECT_TRUE(selfFilter.invertBodyCheck);

		EXPECT_EQ(selfFilter.lifeState, EntityFilter::ALIVE);
		EXPECT_EQ(conditions.attackAffinity, EntTemplate::DamageConditions::ENEMY | EntTemplate::DamageConditions::NEUTRAL | EntTemplate::DamageConditions::SELF);

		ASSERT_EQ(conditions.hitgroups.size(), 2);
		EXPECT_EQ(conditions.hitgroups[0], HITGROUP_LEFTARM);
		EXPECT_EQ(conditions.hitgroups[1], HITGROUP_LEFTLEG);
		EXPECT_TRUE(conditions.invertHitgroupCheck);

		const EntTemplate::TraceAttackRule::Modifier& modifier = traceAttackIt->modifier;
		EXPECT_EQ(modifier.dmgModifier, ValueModifier::FACTOR);
		EXPECT_EQ(modifier.dmg, 2.5f);
		EXPECT_EQ(modifier.dmgMinThreshold, 0.1f);
		EXPECT_TRUE(modifier.noBlood);
		EXPECT_EQ(modifier.hitgroup, HITGROUP_HEAD);

		const EntTemplate::TraceAttackRule::Effects& effects = traceAttackIt->effects;
		ASSERT_TRUE(effects.tracer.has_value());
		EXPECT_EQ(effects.tracer->chance, 0.6f);
		EXPECT_TRUE(effects.tracer->certainOnNewFrame);
		EXPECT_EQ(effects.tracer->variance, 0.2f);

		const EntTemplate::TraceAttackRule::Effects& thresholdEffects = traceAttackIt->thresholdEffects;
		ASSERT_TRUE(thresholdEffects.ricochet.has_value());
		EXPECT_EQ(thresholdEffects.ricochet->chance, 0.6f);
		EXPECT_FALSE(thresholdEffects.ricochet->certainOnNewFrame);
		EXPECT_EQ(thresholdEffects.ricochet->scale, 1.5f);
	}

	traceAttackIt++;
	ASSERT_NE(traceAttackIt, traceAttackRange.second);

	{
		const EntTemplate::TraceAttackRule::Conditions& conditions = traceAttackIt->conditions;

		ASSERT_TRUE(conditions.dmgType.has_value());
		EXPECT_EQ(*conditions.dmgType, DMG_BULLET | DMG_BLAST);
		EXPECT_EQ(conditions.dmgComparison, ValueComparison::LESS_OR_EQUAL);
		EXPECT_EQ(conditions.dmgTypeMatch, DamageTypeMatch::EXACT);
		EXPECT_EQ(conditions.dmg, 2.5f);

		ASSERT_TRUE(conditions.attackerFilter.has_value());
		const EntityFilter& attackerFilter = *conditions.attackerFilter;
		EXPECT_EQ(attackerFilter.classnames.size(), 1);
		EXPECT_EQ(attackerFilter.classnames[0], "someone_else");
		EXPECT_TRUE(attackerFilter.sameClassname);
		EXPECT_EQ(attackerFilter.entTemplates.size(), 1);
		EXPECT_TRUE(attackerFilter.entTemplates[0].empty());
		EXPECT_TRUE(attackerFilter.sameEntTemplate);
		EXPECT_EQ(attackerFilter.classifications.size(), 0);
		EXPECT_TRUE(attackerFilter.sameClassify);

		EXPECT_EQ(conditions.attackAffinity, EntTemplate::DamageConditions::FRIENDLY);

		const EntTemplate::TraceAttackRule::Modifier& modifier = traceAttackIt->modifier;
		EXPECT_TRUE(modifier.skip);
	}

	traceAttackIt++;
	EXPECT_EQ(traceAttackIt, traceAttackRange.second);

	ASSERT_TRUE(test->HasCustomTakeDamageRules());

	auto takeDamageRange = test->TakeDamageRulesRange();
	auto takeDamageIt = takeDamageRange.first;
	ASSERT_NE(takeDamageIt, takeDamageRange.second);

	{
		const EntTemplate::TakeDamageRule::Conditions& conditions = takeDamageIt->conditions;
		ASSERT_TRUE(conditions.dmgType.has_value());
		EXPECT_EQ(*conditions.dmgType, DMG_ACID | DMG_POISON);
		EXPECT_EQ(conditions.dmgTypeMatch, DamageTypeMatch::ALL);
		EXPECT_EQ(conditions.dmg, 50);
		EXPECT_EQ(conditions.dmgComparison, ValueComparison::GREATER);

		ASSERT_TRUE(conditions.attackerFilter.has_value());
		const EntityFilter& attackerFilter = *conditions.attackerFilter;
		EXPECT_TRUE(attackerFilter.sameClassname);
		EXPECT_TRUE(attackerFilter.sameEntTemplate);
		EXPECT_TRUE(attackerFilter.sameClassify);
		EXPECT_TRUE(attackerFilter.negate);

		ASSERT_TRUE(conditions.gibPolicy.has_value());
		EXPECT_EQ(*conditions.gibPolicy, GIB_NORMAL);

		const EntTemplate::TakeDamageRule::Modifier& modifier = takeDamageIt->modifier;
		ASSERT_TRUE(modifier.gibPolicy.has_value());
		EXPECT_EQ(*modifier.gibPolicy, GIB_ALWAYS);
		EXPECT_EQ(modifier.dmg, 10);
		EXPECT_EQ(modifier.dmgModifier, ValueModifier::ADD);
	}

	takeDamageIt++;
	ASSERT_NE(takeDamageIt, takeDamageRange.second);

	{
		const EntTemplate::TakeDamageRule::Conditions& conditions = takeDamageIt->conditions;
		ASSERT_TRUE(conditions.dmgType.has_value());
		EXPECT_EQ(*conditions.dmgType, DMG_FREEZE);
		EXPECT_EQ(conditions.dmgTypeMatch, DamageTypeMatch::NONE);
		EXPECT_EQ(conditions.dmg, 1);
		EXPECT_EQ(conditions.dmgComparison, ValueComparison::LESS);

		const EntTemplate::TakeDamageRule::Modifier& modifier = takeDamageIt->modifier;
		ASSERT_TRUE(modifier.gibPolicy.has_value());
		EXPECT_EQ(*modifier.gibPolicy, GIB_NEVER);
		EXPECT_TRUE(modifier.useHealthAsDmg);
		EXPECT_EQ(modifier.dmgModifier, ValueModifier::SET);
	}
}

const char entitiesInherited[] = R"(
{
	"vort_alt": {
		"inherits": "monster_alien_slave",
		"field_of_view": "full",
		"squad_capability": {
			"require_same_ent_template": true,
			"require_same_classname": false
		},
		"visuals": {
			"Vortigaunt.PowerupBeam": "VortAlt.PowerupBeam"
		},
		"soundscripts": {
			"Vortigaunt.ZapShoot": "VortAlt.ZapShoot"
		}
	},
	"monster_alien_slave": {
		"health": 100,
		"squad_capability": {
			"can_recruit": true,
			"require_same_classname": true
		},
		"visuals": {
			"Vortigaunt.ZapBeamColor": {
				"color": [0, 255, 0]
			},
			"Vortigaunt.ArmBeamColor": "MyVort.ArmBeamColor"
		},
		"soundscripts": {
			"Vortigaunt.Pain": {
				"waves": ["vort/pain1.wav", "vort/pain2.wav"]
			},
			"Vortigaunt.Die": "MyVort.Die"
		}
	},
	"monster_gargantua": {
		"size": {
			"mins": [-40,-40,0],
			"maxs": [40,40,214]
		}
	},
	"garg_alt": {
		"inherits": "monster_gargantua",
		"open_door_capability": true
	}
}
)";

TEST(EntityTemplates, Inheritance)
{
	SoundScriptSystem ss;
	VisualSystem vs;
	EntTemplateSystem es;
	es.SetSoundScriptSystem(&ss);
	es.SetVisualSystem(&vs);

	ASSERT_TRUE(es.ReadFromContents(entitiesInherited, ""));

	{
		const EntTemplate* alienSlave = es.GetTemplate("monster_alien_slave");
		ASSERT_TRUE(alienSlave != nullptr);

		EXPECT_EQ(alienSlave->Health(), 100.0f);

		SquadCapabilities squadCaps = alienSlave->GetSquadCapabilities();
		EXPECT_TRUE(squadCaps.canRecruit);
		EXPECT_TRUE(squadCaps.requireSameClassname);

		EXPECT_STREQ(alienSlave->GetVisualNameOverride("Vortigaunt.ZapBeamColor"), "monster_alien_slave#Vortigaunt.ZapBeamColor");
		EXPECT_STREQ(alienSlave->GetVisualNameOverride("Vortigaunt.ArmBeamColor"), "MyVort.ArmBeamColor");

		EXPECT_STREQ(alienSlave->GetSoundScriptNameOverride("Vortigaunt.Pain"), "monster_alien_slave#Vortigaunt.Pain");
		EXPECT_STREQ(alienSlave->GetSoundScriptNameOverride("Vortigaunt.Die"), "MyVort.Die");
	}

	{
		const EntTemplate* vortAlt = es.GetTemplate("vort_alt");
		ASSERT_TRUE(vortAlt != nullptr);

		EXPECT_EQ(vortAlt->Health(), 100.0f);
		EXPECT_EQ(vortAlt->FieldOfView(), -1.0f);

		SquadCapabilities squadCaps = vortAlt->GetSquadCapabilities();
		EXPECT_TRUE(squadCaps.canRecruit);
		EXPECT_TRUE(squadCaps.requireSameEntTemplate);
		EXPECT_FALSE(squadCaps.requireSameClassname);

		EXPECT_STREQ(vortAlt->GetVisualNameOverride("Vortigaunt.ZapBeamColor"), "monster_alien_slave#Vortigaunt.ZapBeamColor");
		EXPECT_STREQ(vortAlt->GetVisualNameOverride("Vortigaunt.ArmBeamColor"), "MyVort.ArmBeamColor");
		EXPECT_STREQ(vortAlt->GetVisualNameOverride("Vortigaunt.PowerupBeam"), "VortAlt.PowerupBeam");

		EXPECT_STREQ(vortAlt->GetSoundScriptNameOverride("Vortigaunt.Pain"), "monster_alien_slave#Vortigaunt.Pain");
		EXPECT_STREQ(vortAlt->GetSoundScriptNameOverride("Vortigaunt.Die"), "MyVort.Die");
		EXPECT_STREQ(vortAlt->GetSoundScriptNameOverride("Vortigaunt.ZapShoot"), "VortAlt.ZapShoot");
	}

	{
		const EntTemplate* garg = es.GetTemplate("monster_gargantua");
		ASSERT_TRUE(garg != nullptr);

		EXPECT_TRUE(garg->IsSizeDefined());
		EXPECT_EQ(garg->MinSize(), Vector(-40.0f, -40.0f, 0.0f));
		EXPECT_EQ(garg->MaxSize(), Vector(40.0f, 40.0f, 214.0f));
	}

	{
		const EntTemplate* garg = es.GetTemplate("garg_alt");
		ASSERT_TRUE(garg != nullptr);

		EXPECT_TRUE(garg->IsSizeDefined());
		EXPECT_EQ(garg->MinSize(), Vector(-40.0f, -40.0f, 0.0f));
		EXPECT_EQ(garg->MaxSize(), Vector(40.0f, 40.0f, 214.0f));

		EXPECT_TRUE(garg->IsOpenDoorCapabilityDefined());
		EXPECT_TRUE(garg->CanOpenDoors());
	}
}

const char looped[] = R"(
{
	"zombie1": {
		"inherits": "zombie3",
	},
	"zombie2": {
		"inherits": "zombie1"
	},
	"zombie3": {
		"inherits": "zombie2"
	}
}
)";

TEST(EntityTemplates, DetectLoop)
{
	SoundScriptSystem ss;
	VisualSystem vs;
	EntTemplateSystem es;
	es.SetSoundScriptSystem(&ss);
	es.SetVisualSystem(&vs);

	ASSERT_FALSE(es.ReadFromContents(looped, ""));
}
