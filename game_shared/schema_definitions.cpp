#include "json_schemas.h"

const char* const json_schemas::definitions = R"(
{
	"alpha": {
		"type": "integer",
		"minimum": 0,
		"maximum": 255
	},
	"color": {
		"type": ["string", "array", "null"],
		"pattern": "^([0-9]{1,3}[ ]+[0-9]{1,3}[ ]+[0-9]{1,3})|((#|0x)[0-9a-fA-F]{6})$",
		"items": {
			"type": "integer",
			"minimum": 0,
			"maximum": 255
		},
		"minItems": 3,
		"maxItems": 3
	},
	"tracer_color": {
		"enum": [
			"None", "none",
			"White", "white",
			"Red", "red",
			"Green", "green",
			"Blue", "blue",
			"Default", "default",
			"Golden", "golden", "Yellow", "yellow",
			"Orange", "orange",
			"Purple", "purple"
		]
	},
	"range": {
		"type": ["string", "object", "number", "array"],
		"pattern": "^[0-9]+(\\.[0-9]+)?(,[0-9]+(\\.[0-9]+)?)?$",
		"properties": {
			"min": {
				"type": "number"
			},
			"max": {
				"type": "number"
			}
		},
		"required": ["min", "max"],
		"additionalProperties": false,
		"items": {
			"type": "number"
		},
		"minItems": 2,
		"maxItems": 2
	},
	"range_non_negative": {
		"type": ["string", "object", "number", "array"],
		"minimum": 0,
		"pattern": "^[0-9]+(\\.[0-9]+)?(,[0-9]+(\\.[0-9]+)?)?$",
		"properties": {
			"min": {
				"type": "number",
				"minimum": 0
			},
			"max": {
				"type": "number",
				"minimum": 0
			}
		},
		"required": ["min", "max"],
		"additionalProperties": false,
		"items": {
			"type": "number",
			"minimum": 0
		},
		"minItems": 2,
		"maxItems": 2
	},
	"range_positive": {
		"type": ["object", "number", "array"],
		"exclusiveMinimum": 0.0,
		"properties": {
			"min": {
				"type": "number",
				"exclusiveMinimum": 0.0
			},
			"max": {
				"type": "number",
				"exclusiveMinimum": 0.0
			}
		},
		"required": ["min", "max"],
		"additionalProperties": false,
		"items": {
			"type": "number",
			"exclusiveMinimum": 0.0
		},
		"minItems": 2,
		"maxItems": 2
	},
	"range_int": {
		"type": ["string", "object", "integer", "array"],
		"pattern": "^[0-9]+(,[0-9]+)?$",
		"properties": {
			"min": {
				"type": "integer"
			},
			"max": {
				"type": "integer"
			}
		},
		"required": ["min", "max"],
		"additionalProperties": false,
		"items": {
			"type": "integer"
		},
		"minItems": 2,
		"maxItems": 2
	},
	"range_int_non_negative": {
		"type": ["string", "object", "integer", "array"],
		"minimum": 0,
		"pattern": "^[0-9]+(,[0-9]+)?$",
		"properties": {
			"min": {
				"type": "integer",
				"minimum": 0
			},
			"max": {
				"type": "integer",
				"minimum": 0
			}
		},
		"required": ["min", "max"],
		"additionalProperties": false,
		"items": {
			"type": "integer",
			"minimum": 0
		},
		"minItems": 2,
		"maxItems": 2
	},
	"range_alpha": {
		"type": ["string", "object", "integer", "array"],
		"minimum": 0,
		"pattern": "^[0-9]{1,3}(,[0-9]{1,3})?$",
		"properties": {
			"min": {
				"type": "integer",
				"minimum": 0,
				"maximum": 255
			},
			"max": {
				"type": "integer",
				"minimum": 0,
				"maximum": 255
			}
		},
		"required": ["min", "max"],
		"additionalProperties": false,
		"items": {
			"type": "integer",
			"minimum": 0,
			"maximum": 255
		},
		"minItems": 2,
		"maxItems": 2
	},
	"absolute_or_factor": {
		"type": ["number", "string"],
		"pattern": "^\\*[0-9]+(\\.[0-9]+)?$",
		"minimum": 0.0
	},
	"vector": {
		"type": ["array"],
		"items": {
			"type": "number"
		},
		"minItems": 3,
		"maxItems": 3
	},
	"chance": {
		"type": "number",
		"minimum": 0.0,
		"maximum": 1.0
	},
	"string_set": {
		"type": ["string", "array"],
		"items": {
			"type": "string"
		},
		"uniqueItems": true
	},
	"skill_value_non_negative": {
		"oneOf": [
			{
				"$ref": "#/range_non_negative"
			},
			{
				"type": "string",
				"pattern": "^[^ ]+$"
			},
			{
				"type": "array",
				"minItems": 3,
				"maxItems": 3,
				"items": {
					"$ref": "#/range_non_negative"
				}
			}
		]
	},
	"skill_value_positive": {
		"oneOf": [
			{
				"$ref": "#/range_positive"
			},
			{
				"type": "string",
				"pattern": "^[^ ]+$"
			},
			{
				"type": "array",
				"minItems": 3,
				"maxItems": 3,
				"items": {
					"$ref": "#/range_positive"
				}
			}
		]
	},
	"object_size": {
		"type": ["object", "string"],
		"properties": {
			"mins": {
				"$ref": "#/vector"
			},
			"maxs": {
				"$ref": "#/vector"
			}
		},
		"additionalProperties": false,
		"required": ["mins", "maxs"]
	},
	"sound_channel": {
		"enum": [
			"Auto",
			"auto",
			"Weapon",
			"weapon",
			"Voice",
			"voice",
			"Item",
			"item",
			"Body",
			"body",
			"Static",
			"static"
		]
	},
	"attenuation": {
		"oneOf": [
			{
				"enum": [
					"Norm",
					"norm",
					"Idle",
					"idle",
					"Static",
					"static",
					"None",
					"none"
				]
			},
			{
				"type": "number",
				"minimum": 0
			}
		]
	},
	"shake": {
		"type": ["object", "null"],
		"properties": {
			"radius": {
				"type": "integer",
				"minimum": 0
			},
			"duration": {
				"type": "number",
				"minimum": 0.0
			},
			"frequency": {
				"type": "number",
				"exclusiveMinimum": 0,
				"maximum": 255.0
			},
			"amplitude": {
				"type": "integer",
				"minimum": 0,
				"maximum": 16
			}
		},
		"additionalProperties": false
	},
	"soundscript": {
		"type": ["object", "string"],
		"properties": {
			"waves": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"maxItems": 10
			},
			"channel": {
				"$ref": "#/sound_channel"
			},
			"volume": {
				"$ref": "#/range"
			},
			"attenuation": {
				"$ref": "#/attenuation"
			},
			"pitch": {
				"$ref": "#/range_int_non_negative"
			}
		},
		"additionalProperties": false
	},
	"visual_object": {
		"type": "object",
		"properties": {
			"model": {
				"type": "string"
			},
			"sprite": {
				"type": "string"
			},
			"rendermode": {
				"enum": [
					"Normal",
					"normal",
					"Color",
					"color",
					"Texture",
					"texture",
					"Glow",
					"glow",
					"Solid",
					"solid",
					"Additive",
					"additive"
				]
			},
			"color": {
				"$ref": "#/color"
			},
			"alpha": {
				"$ref": "#/range_alpha"
			},
			"renderfx": {
				"oneOf": [
					{
						"enum": [
							"Normal",
							"normal",
							"Constant Glow",
							"constant glow",
							"Constant glow",
							"Distort",
							"distort",
							"Hologram",
							"hologram",
							"Glow Shell",
							"glow shell",
							"Glow shell"
						]
					},
					{
						"type": "integer",
						"minimum": 0,
						"maximum": 20
					}
				]
			},
			"scale": {
				"$ref": "#/range_non_negative"
			},
			"framerate": {
				"$ref": "#/range_non_negative"
			},
			"width": {
				"type": "integer",
				"minimum": 1
			},
			"noise": {
				"type": "integer",
				"minimum": 0
			},
			"scrollrate": {
				"type": "integer",
				"minimum": 0
			},
			"life": {
				"$ref": "#/range_non_negative"
			},
			"radius": {
				"$ref": "#/range_int_non_negative"
			},
			"beamflags": {
				"type": "array",
				"items": {
					"enum": [
						"Sine",
						"sine",
						"Solid",
						"solid",
						"Shadein",
						"shadein",
						"Shadeout",
						"shadeout"
					]
				}
			},
			"decay": {
				"type": "number"
			},
			"wave": {
				"enum": [
					"Torus",
					"torus",
					"Disk",
					"disk",
					"Cylinder",
					"cylinder"
				]
			}
		},
		"additionalProperties": false,
		"dependencies": {
			"sprite": { "not": { "required": ["model"] } }
		}
	},
	"visual": {
		"oneOf": [
			{
				"type": "string",
				"minLength": 1
			},
			{
				"$ref": "#/visual_object"
			}
		]
	},
	"body_filter": {
		"type": ["integer", "object"],
		"minimum": 0,
		"properties": {
			"bodygroup": {
				"type": "integer",
				"minimum": "0"
			},
			"submodel": {
				"type": "integer",
				"minimum": "0"
			}
		},
		"required": ["bodygroup", "submodel"],
		"additionalProperties": false
	},
	"entity_filter": {
		"type": "object",
		"properties": {
			"classname": {
				"$ref": "#/string_set"
			},
			"ent_template": {
				"$ref": "#/string_set"
			},
			"classify": {
				"$ref": "#/string_set"
			},
			"is_combat_character": {
				"type": "boolean"
			},
			"life_state": {
				"oneOf": [
					{
						"type": "array",
						"items": {
							"enum": ["alive", "dead", "dying"]
						},
						"uniqueItems": true
					},
					{
						"enum": ["alive", "dead", "dying"]
					}
				]
			},
			"body": {
				"oneOf": [
					{
						"$ref": "#/body_filter"
					},
					{
						"type": "array",
						"items": {
							"$ref": "#/body_filter"
						}
					}
				]
			},
			"invert_body_check": {
				"type": "boolean"
			},
			"negate": {
				"type": "boolean"
			}
		},
		"additionalProperties": false
	},
	"damage_info": {
		"type": "object",
		"properties": {
			"damage": {
				"$ref": "#/skill_value_non_negative"
			},
			"type": {
				"$ref": "#/string_set"
			},
			"type_policy": {
				"enum": ["replace", "add"]
			},
			"nonlethal": {
				"type": "boolean"
			},
			"ignore_armor": {
				"type": "boolean"
			},
			"no_blood": {
				"type": "boolean"
			},
			"gib": {
				"enum": ["always", "never", "normal"]
			},
			"timed_nonlethal": {
				"type": "boolean"
			},
			"timed_ignore_armor": {
				"type": "boolean"
			},
			"ignore_powershield": {
				"type": "boolean"
			}
		},
		"additionalProperties": false
	},
	"hitgroup_set": {
		"type": ["array", "string", "integer"],
		"minimum": 0,
		"minItems": 1,
		"items": {
			"type": ["string", "integer"],
			"minimum": 0
		},
		"uniqueItems": true
	},
	"attack_affinity": {
		"oneOf": [
			{
				"type": "array",
				"items": {
					"enum": ["enemy", "friendly", "self", "neutral"]
				},
				"uniqueItems": true
			},
			{
				"enum": ["enemy", "friendly", "self", "neutral"]
			}
		]
	},
	"check_melee_attack": {
		"type": "object",
		"properties": {
			"distance": {
				"type": "number",
				"minimum": 0.0
			},
			"dot": {
				"type": "number",
				"minimum": 0.0,
				"maximum": 1.0
			}
		},
		"additionalProperties": false
	},
	"punchangle": {
		"type": "object",
		"properties": {
			"pitch": {
				"type": "number"
			},
			"yaw": {
				"type": "number"
			},
			"roll": {
				"type": "number"
			}
		},
		"additionalProperties": false
	},
	"trace_hull_attack": {
		"type": "object",
		"properties": {
			"distance": {
				"type": "number",
				"minimum": 0
			},
			"height": {
				"$ref": "#/absolute_or_factor"
			},
			"punchangle": {
				"$ref": "#/punchangle"
			},
			"knock": {
				"type": "object",
				"properties": {
					"forward": {
						"type": "number"
					},
					"right": {
						"type": "number"
					},
					"up": {
						"type": "number"
					},
					"player_only": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"damage_info": {
				"$ref": "#/damage_info"
			},
			"spawn_blood": {
				"type": "boolean"
			},
			"hit_soundscript": {
				"$ref": "#/soundscript"
			},
			"miss_soundscript": {
				"$ref": "#/soundscript"
			}
		},
		"additionalProperties": false
	},
	"damage_comparator": {
		"type": "string",
		"pattern": "^(<=|>=|<|>)([0-9]+)(\\.[0-9]+)?$"
	},
	"damage_modifier": {
		"type": "string",
		"pattern": "^(\\*|\\+|-|=)((([0-9]+)(\\.[0-9]+)?)|health)$"
	},
	"trace_attack_effects": {
		"type": "object",
		"properties": {
			"ricochet": {
				"type": "object",
				"properties": {
					"chance": {
						"$ref": "#/chance"
					},
					"certain_on_new_frame": {
						"type": "boolean"
					},
					"scale": {
						"$ref": "#/range"
					}
				},
				"additionalProperties": false
			},
			"tracer": {
				"type": "object",
				"properties": {
					"chance": {
						"$ref": "#/chance"
					},
					"certain_on_new_frame": {
						"type": "boolean"
					},
					"variance": {
						"type": "number"
					}
				},
				"additionalProperties": false
			}
		},
		"additionalProperties": false
	},
	"trace_attack_rule": {
		"type": "object",
		"properties": {
			"conditions": {
				"type": "object",
				"properties": {
					"dmg_type": {
						"$ref": "#/string_set"
					},
					"dmg_type_match": {
						"enum": ["one", "all", "none", "exact"]
					},
					"dmg": {
						"$ref": "#/damage_comparator"
					},
					"inflictor": {
						"$ref": "#/entity_filter"
					},
					"attacker": {
						"$ref": "#/entity_filter"
					},
					"self": {
						"$ref": "#/entity_filter"
					},
					"attack_affinity": {
						"$ref": "#/attack_affinity"
					},
					"gib": {
						"enum": ["always", "never", "normal"]
					},
					"hitgroup": {
						"$ref": "#/hitgroup_set"
					},
					"invert_hitgroup_check": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"modifier": {
				"type": "object",
				"properties": {
					"dmg": {
						"$ref": "#/damage_modifier"
					},
					"dmg_min_threshold": {
						"type": "number",
						"minimum": 0
					},
					"skip_damage": {
						"type": "boolean"
					},
					"no_blood": {
						"type": "boolean"
					},
					"gib": {
						"enum": ["always", "never", "normal"]
					},
					"hitgroup": {
						"type": ["string", "integer"],
						"minimum": 0
					}
				},
				"additionalProperties": false
			},
			"effects": {
				"$ref": "#/trace_attack_effects"
			},
			"threshold_effects": {
				"$ref": "#/trace_attack_effects"
			}
		},
		"additionalProperties": false
	},
	"take_damage_rule_conditions": {
		"type": "object",
		"properties": {
			"dmg_type": {
				"$ref": "#/string_set"
			},
			"dmg_type_match": {
				"enum": ["one", "all", "none", "exact"]
			},
			"dmg": {
				"$ref": "#/damage_comparator"
			},
			"inflictor": {
				"$ref": "#/entity_filter"
			},
			"attacker": {
				"$ref": "#/entity_filter"
			},
			"self": {
				"$ref": "#/entity_filter"
			},
			"attack_affinity": {
				"$ref": "#/attack_affinity"
			},
			"gib": {
				"enum": ["always", "never", "normal"]
			}
		},
		"additionalProperties": false
	},
	"take_damage_rule": {
		"type": "object",
		"properties": {
			"conditions": {
				"$ref": "#/take_damage_rule_conditions"
			},
			"modifier": {
				"type": "object",
				"properties": {
					"dmg": {
						"$ref": "#/damage_modifier"
					},
					"dmg_min_threshold": {
						"type": "number",
						"minimum": 0
					},
					"skip_damage": {
						"type": "boolean"
					},
					"no_blood": {
						"type": "boolean"
					},
					"gib": {
						"enum": ["always", "never", "normal"]
					}
				},
				"additionalProperties": false
			}
		},
		"additionalProperties": false
	},
)"
R"(
	"drop_item": {
		"type": ["object", "string"],
		"properties": {
			"classname": {
				"type": "string"
			},
			"ent_template": {
				"$ref": "#/entity_template"
			},
			"pickup_name": {
				"type": "string"
			},
			"chance": {
				"$ref": "#/chance"
			},
			"weight": {
				"type": "number",
				"minimum": 0
			}
		},
		"required": ["classname"],
		"additionalProperties": false
	},
	"drop_item_list": {
		"type": "array",
		"items": {
			"$ref": "#/drop_item"
		}
	},
	"child_variant": {
		"type": "object",
		"properties": {
			"classname": {
				"type": "string",
				"minLength": 1
			},
			"parameters": {
				"type": "object",
				"additionalProperties": {
					"type": ["string", "number"],
					"minLength": 1
				}
			},
			"chance": {
				"type": "number",
				"exclusiveMinimum": 0.0
			}
		},
		"additionalProperties": false
	},
	"child_variant_list": {
		"type": "array",
		"items": {
			"$ref": "#/child_variant"
		}
	},
	"ai_sound_type": {
		"enum": [
			"Danger",
			"danger",
			"Combat",
			"combat"
		]
	},
	"blood_color": {
		"type": "string",
		"minLength": 1
	},
	"regen_resource_type_set": {
		"oneOf": [
			{
				"type": "array",
				"items": {
					"enum": ["standard", "native"]
				},
				"uniqueItems": true
			},
			{
				"enum": ["standard", "native"]
			}
		]
	},
	"cloak_conditions": {
		"type": "array",
		"items": {
			"enum": [
				"moving", "standing",
				"attacking", "reloading",
				"not_on_ground", "on_ground",
				"has_no_enemy", "has_enemy",
				"taking_damage", "hopping"
			]
		}
	},
	"leap_attack": {
		"type": "object",
		"properties": {
			"damage_info": {
				"$ref": "#/damage_info"
			},
			"punchangle": {
				"$ref": "#/punchangle"
			},
			"spawn_blood": {
				"type": "boolean"
			},
			"max_jump_distance": {
				"type": "number",
				"minimum": 8
			},
			"max_jump_height": {
				"type": "number",
				"minimum": 8
			},
			"animation_event": {
				"type": "integer",
				"minimum": 0
			},
			"start_frame_fraction": {
				"type": "number",
				"minimum": 0,
				"exclusiveMaximum": 1
			},
			"set_touch_early": {
				"type": "boolean"
			},
			"allow_hit_on_ground": {
				"type": "boolean"
			}
		},
		"additionalProperties": false,
		"dependencies": {
			"start_frame_fraction": { "not": { "required": ["animation_event"] } }
		}
	},
)"
R"(
	"entity_template": {
		"type": ["object", "string"],
		"properties": {
			"inherits": {
				"type": "string"
			},
			"own_visual": {
				"$ref": "#/visual_object"
			},
			"gib_visual": {
				"$ref": "#/visual_object"
			},
			"size": {
				"$ref": "#/object_size"
			},
			"collision_box": {
				"$ref": "#/object_size"
			},
			"classify": {
				"type": "string"
			},
			"blood": {
				"oneOf": [
					{
						"$ref": "#/blood_color"
					},
					{
						"type": "object",
						"properties": {
							"color": {
								"$ref": "#/blood_color"
							},
							"by_hitgroup": {
								"type": "array",
								"items": {
									"type": "object",
									"properties": {
										"hitgroup": {
											"type": ["string", "integer"]
										},
										"color": {
											"$ref": "#/blood_color"
										}
									},
									"required": ["hitgroup", "color"],
									"additionalProperties": false
								}
							}
						},
						"additionalProperties": false
					}
				]
			},
			"field_of_view": {
				"type": ["number", "string"],
				"minimum": -1.0,
				"exclusiveMaximum": 1.0
			},
			"health": {
				"$ref": "#/skill_value_positive"
			},
			"soundscripts": {
				"type": "object",
				"additionalProperties": {
					"$ref": "#/soundscript"
				}
			},
			"visuals": {
				"type": "object",
				"additionalProperties": {
					"$ref": "#/visual"
				}
			},
			"sound_replacement": {
				"type": "object",
				"additionalProperties": {
					"type": "string"
				}
			},
			"precached_sounds": {
				"type": "array",
				"items": {
					"type": "string"
				}
			},
			"precached_soundscripts": {
				"type": "array",
				"items": {
					"type": "string"
				}
			},
			"autoprecache_sounds": {
				"type": "boolean"
			},
			"autoprecache_soundscripts": {
				"type": "boolean"
			},
			"size_for_grapple": {
				"type": "string"
			},
			"material": {
				"type": "string",
				"minLength": 1,
				"maxLength": 1
			},
			"speech_prefix": {
				"type": "string",
				"minLength": 1
			},
			"squad_capability": {
				"type": ["boolean", "object"],
				"properties": {
					"can_recruit": {
						"type": "boolean"
					},
					"deny_recruiting": {
						"type": "boolean"
					},
					"allow_different_classification": {
						"type": "boolean"
					},
					"require_same_classname": {
						"type": "boolean"
					},
					"require_same_ent_template": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"open_door_capability": {
				"type": "boolean"
			},
			"check_melee_attack1": {
				"$ref": "#/check_melee_attack"
			},
			"check_melee_attack2": {
				"$ref": "#/check_melee_attack"
			},
			"trace_hull_attacks": {
				"type": "object",
				"patternProperties": {
					"^[0-9]+$": { "$ref": "#/trace_hull_attack" }
				},
				"additionalProperties": false
			},
			"touch_attack": {
				"$ref": "#/leap_attack"
			},
			"leap_attack": {
				"$ref": "#/leap_attack"
			},
			"primary_weapon": {
				"type": "array",
				"items": {
					"type": "object",
					"properties": {
						"weapons": {
							"type": "integer",
							"minimum": 0
						},
						"max_clip": {
							"type": "integer",
							"minimum": 0
						}
					},
					"additionalProperties": false
				},
				"minItems": 1
			},
			"take_damage": {
				"type": "array",
				"items": {
					"$ref": "#/take_damage_rule"
				}
			},
			"trace_attack": {
				"type": "array",
				"items": {
					"$ref": "#/trace_attack_rule"
				}
			},
			"loot_drop": {
				"oneOf": [
					{
						"$ref": "#/drop_item_list"
					},
					{
						"type": "object",
						"properties": {
							"items": {
								"$ref": "#/drop_item_list"
							},
							"max_weight": {
								"type": "number",
								"exlusiveMinimum": 0
							}
						},
						"required": ["items"],
						"additionalProperties": false
					}
				]
			},
			"equipment_drop": {
				"type": "array",
				"items": {
					"type": "object",
					"properties": {
						"weapons": {
							"type": "integer",
							"minimum": 0
						},
						"weapons_match": {
							"enum": ["one", "all", "none", "exact"]
						},
						"classname": {
							"type": "string",
							"minLength": 1
						},
						"ent_template": {
							"$ref": "#/entity_template"
						},
						"at_position": {
							"enum": ["gun", "body"]
						}
					},
					"required": ["classname"],
					"additionalProperties": false
				}
			},
			"children": {
				"oneOf": [
					{
						"$ref": "#/child_variant"
					},
					{
						"$ref": "#/child_variant_list"
					}
				]
			},
			"pain": {
				"type": "object",
				"properties": {
					"delay": {
						"$ref": "#/range_non_negative"
					},
					"chance": {
						"$ref": "#/chance"
					},
					"lower_bound_dmg": {
						"type": "number"
					},
					"allow_when_dying": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"skill": {
				"type": "object",
				"patternProperties": {
					"^[^ ]+$": {
						"oneOf": [
							{
								"$ref": "#/range"
							},
							{
								"type": ["string", "array"],
								"pattern": "^[^ ]+$",
								"minItems": 3,
								"maxItems": 3,
								"items": {
									"$ref": "#/range"
								}
							}
						]
					}
				},
				"additionalProperties": false
			},
			"displayname": {
				"type": "string"
			},
			"projectile": {
				"type": "object",
				"properties": {
					"effect_flags": {
						"type": "array",
						"items": {
							"enum": ["rocketflare", "brightlight"]
						}
					},
					"detonate_on_touch": {
						"enum": ["damageable", "aimable", "any", true]
					},
					"tracer": {
						"type": "object",
						"properties": {
							"color": {
								"$ref": "#/tracer_color"
							},
							"scale": {
								"type": "number",
								"exclusiveMinimuim": 0
							}
						},
						"additionalProperties": false
					}
				},
				"additionalProperties": false
			},
			"pickup": {
				"type": "object",
				"properties": {
					"hud_sprite": {
						"type": "string"
					},
					"suit_sentence": {
						"type": "string"
					},
					"suit_sentence_interval": {
						"type": "number",
						"minimum": 0
					}
				},
				"additionalProperties": false
			},
			"passive_regeneration": {
				"type": "object",
				"properties": {
					"interval": {
						"type": "number",
						"minimum": 0.1
					},
					"health_per_update": {
						"type": "number",
						"minimum": 0
					},
					"delay_after_hurt": {
						"type": "number",
						"minimum": 0
					},
					"health_fraction_limit": {
						"type": "number",
						"exclusiveMinimum": 0,
						"maximum": 1
					},
					"resource_type": {
						"$ref": "#/regen_resource_type_set"
					},
					"play_sprite": {
						"type": "boolean"
					},
					"particles_per_update": {
						"type": "integer",
						"minimum": 0
					},
					"particles_fade_time": {
						"type": "number",
						"minimum": 0
					},
					"beams_per_update": {
						"type": "integer",
						"minimum": 0
					}
				},
				"additionalProperties": false
			},
			"active_regeneration": {
				"type": "object",
				"properties": {
					"interval": {
						"type": "number",
						"minimum": 0.1
					},
					"health_per_update": {
						"type": "number",
						"minimum": 0
					},
					"health_fraction_limit": {
						"type": "number",
						"exclusiveMinimum": 0,
						"maximum": 1
					},
					"resource_type": {
						"$ref": "#/regen_resource_type_set"
					},
					"play_sprite": {
						"type": "boolean"
					},
					"particles_per_update": {
						"type": "integer",
						"minimum": 0
					},
					"particles_fade_time": {
						"type": "number",
						"minimum": 0
					},
					"beams_per_update": {
						"type": "integer",
						"minimum": 0
					},
					"health_fraction_combat_trigger": {
						"type": "number",
						"exclusiveMinimum": 0,
						"maximum": 1
					},
					"health_fraction_noncombat_trigger": {
						"type": "number",
						"exclusiveMinimum": 0,
						"maximum": 1
					},
					"cooldown": {
						"type": "number",
						"minimum": 0
					},
					"sequence": {
						"type": "string"
					},
					"early_finish": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"regeneration_resource": {
				"type": "object",
				"properties": {
					"amount": {
						"$ref": "#/skill_value_positive"
					}
				},
				"additionalProperties": false
			},
			"power_shield": {
				"type": "object",
				"properties": {
					"render_shield": {
						"type": "boolean"
					},
					"render_impact_debris": {
						"type": "boolean"
					},
					"render_impact_particles": {
						"type": "boolean"
					},
					"absorption": {
						"$ref": "#/skill_value_positive"
					},
					"armor": {
						"$ref": "#/skill_value_non_negative"
					},
					"reserve": {
						"$ref" : "#/skill_value_non_negative"
					},
					"recharge": {
						"type": "object",
						"properties": {
							"delay_after_hurt": {
								"type": "number",
								"minimum": 0
							},
							"interval": {
								"type": "number",
								"minimum": 0.1
							},
							"armor_per_update": {
								"type": "number",
								"minimum": 1
							}
						},
						"additionalProperties": false
					},
					"take_damage": {
						"type": "array",
						"items": {
							"type": "object",
							"properties": {
								"conditions": {
									"$ref": "#/take_damage_rule_conditions"
								},
								"dmg_factor": {
									"type": "number",
									"minimum": 0
								}
							},
							"additionalProperties": false
						}
					}
				},
				"additionalProperties": false
			},
			"cloaking": {
				"type": "object",
				"properties": {
					"ability": {
						"$ref": "#/skill_value_non_negative"
					},
					"opacity": {
						"$ref": "#/skill_value_non_negative"
					},
					"cloak_speed": {
						"type": "integer",
						"minimum": 1
					},
					"uncloak_speed": {
						"type": "integer",
						"minimum": 1
					},
					"uncloak_when": {
						"$ref": "#/cloak_conditions"
					},
					"cloak_when": {
						"$ref": "#/cloak_conditions"
					}
				},
				"additionalProperties": false,
				"dependencies": {
					"uncloak_when": { "not": { "required": ["cloak_when"] } }
				}
			}
		},
		"additionalProperties": false,
		"dependencies": {
			"leap_attack": { "not": { "required": ["touch_attack"] } }
		}
	}
}
)";
