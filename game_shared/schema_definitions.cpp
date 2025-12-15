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
		"additionalProperties": false,
		"items": {
			"type": "number"
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
		"additionalProperties": false,
		"items": {
			"type": "integer",
			"minimum": 0
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
				"type": "number",
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
				"$ref": "#/range_int"
			}
		},
		"additionalProperties": false
	},
	"visual": {
		"type": ["object", "string"],
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
				"$ref": "#/alpha"
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
				"$ref": "#/range",
				"minimum": 0.0
			},
			"framerate": {
				"type": "number",
				"minimum": 0.0
			},
			"width": {
				"type": "integer",
				"minimum": 1
			},
			"noise": {
				"type": "integer"
			},
			"scrollrate": {
				"type": "integer"
			},
			"life": {
				"$ref": "#/range"
			},
			"radius": {
				"$ref": "#/range_int"
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
			}
		},
		"additionalProperties": false
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
				"type": "number"
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
			"gib": {
				"enum": ["always", "never", "normal"]
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
				}
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
				}
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
				}
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
				}
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
		}
	},
	"take_damage_rule": {
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
					}
				},
				"additionalProperties": false
			}
		}
	},
	"drop_item": {
		"type": ["object", "string"],
		"properties": {
			"classname": {
				"type": "string"
			},
			"ent_template": {
				"type": "string"
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
)"
R"(
	"entity_template": {
		"type": ["object", "string"],
		"properties": {
			"inherits": {
				"type": "string"
			},
			"own_visual": {
				"$ref": "#/visual"
			},
			"gib_visual": {
				"$ref": "#/visual"
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
				"type": "string"
			},
			"field_of_view": {
				"type": ["number", "string"],
				"minimum": -1.0,
				"exclusiveMaximum": 1.0
			},
			"health": {
				"type": "number",
				"exclusiveMinimum": 0.0
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
				}
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
				"type": "object",
				"properties": {
					"damage_info": {
						"$ref": "#/damage_info"
					}
				},
				"additionalProperties": false
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
						"$ref": "#/range"
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
			}
		},
		"additionalProperties": false
	}
}
)";
