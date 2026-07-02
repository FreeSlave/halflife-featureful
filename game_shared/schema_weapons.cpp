#include "json_schemas.h"

const char* const json_schemas::weapons = R"(
{
	"model_path": {
		"type": "string",
		"pattern": "^.+\\.mdl$"
	},
	"anim_index": {
		"type": "integer",
		"minimum": 0
	},
	"anim_index_optional": {
		"type": "integer",
		"minimum": -1
	},
	"anim_array": {
		"type": "array",
		"items": {
			"$ref": "#/anim_index"
		},
		"minItems": 0,
		"maxItems": 4
	},
	"waves_array": {
		"type": "array",
		"items": {
			"type": "string"
		},
		"maxItems": 4
	},
	"w_soundscript": {
		"oneOf": [
			{
				"type": "object",
				"properties": {
					"waves": {
						"$ref": "#/waves_array"
					},
					"channel": {
						"$ref": "definitions.json#/sound_channel"
					},
					"volume": {
						"$ref": "definitions.json#/range"
					},
					"attenuation": {
						"$ref": "definitions.json#/attenuation"
					},
					"pitch": {
						"$ref": "definitions.json#/range_int_non_negative"
					},
					"looped": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			{
				"$ref": "#/waves_array"
			},
			{
				"type": "string"
			}
		]
	},
	"deploy": {
		"type": ["object", "null"],
		"properties": {
			"anim": {
				"$ref": "#/anim_index"
			},
			"duration": {
				"type": "number",
				"minimum": 0.0
			},
			"idle_delay": {
				"type": "number",
				"minimum": 0.0
			},
			"sound": {
				"$ref": "#/w_soundscript"
			}
		},
		"additionalProperties": false
	},
	"idle": {
		"type": ["array", "object", "null"],
		"properties": {
			"anim": {
				"$ref": "#/anim_index"
			},
			"duration": {
				"$ref": "definitions.json#/range"
			},
			"sound": {
				"$ref": "#/w_soundscript"
			}
		},
		"additionalProperties": false,
		"items": {
			"type": "object",
			"properties": {
				"anim": {
					"$ref": "#/anim_index"
				},
				"duration": {
					"$ref": "definitions.json#/range"
				},
				"chance": {
					"type": "number",
					"exclusiveMinimum": 0
				},
				"sound": {
					"$ref": "#/w_soundscript"
				}
			},
			"required": ["anim", "duration", "chance"],
			"additionalProperties": false
		},
		"maxItems": 5
	},
	"spread_value": {
		"type": ["number", "string"],
		"minimum": 0,
		"maximum": 1,
		"pattern": "^[1-9][0-9]?degrees?$"
	},
	"spread_value_pair": {
		"type": "object",
		"properties": {
			"vertical": {
				"$ref": "#/spread_value"
			},
			"lateral": {
				"$ref": "#/spread_value"
			}
		},
		"required": ["lateral", "vertical"],
		"additionalProperties": false
	},
	"kickback": {
		"type": ["object", "number"],
		"minimum": 0,
		"properties": {
			"vertical": {
				"type": "number",
				"minimum": 0
			},
			"lateral": {
				"type": "number",
				"minimum": 0
			},
			"vertical_max": {
				"type": "number",
				"minimum": 0
			},
			"lateral_max": {
				"type": "number",
				"minimum": 0
			},
			"vertical_modifier": {
				"type": "number",
				"minimum": 0
			},
			"lateral_modifier": {
				"type": "number",
				"minimum": 0
			},
			"vertical_persistance": {
				"type": "integer",
				"minimum": 0
			},
			"lateral_persistance": {
				"type": "integer",
				"minimum": 0
			}
		},
		"additionalProperties": false
	},
	"player_movement_conditions": {
		"type": "object",
		"properties": {
			"moving": {
				"type": ["boolean", "number"],
				"minimum": 0
			},
			"ducking": {
				"type": "boolean"
			},
			"in_air": {
				"type": "boolean"
			}
		},
		"additionalProperties": false
	},
	"weapon_damage": {
		"oneOf": [
			{
				"type": "string",
				"minLength": 1
			},
			{
				"$ref": "definitions.json#/range"
			},
			{
				"$ref": "definitions.json#/damage_info"
			}
		]
	},
	"dropammo": {
		"type": "object",
		"properties": {
			"classname": {
				"type": "string",
				"minLength": 1
			},
			"ent_template": {
				"type": "string"
			},
			"amount": {
				"type": "integer",
				"minimum": 0
			}
		},
		"additionalProperties": false
	},
	"attachment": {
		"type": "integer",
		"minimum": "1",
		"maximum": "4"
	},
)"
R"(
	"fire": {
		"type": "object",
		"properties": {
			"type": {
				"enum": ["bullet", "bullets", "melee", "projectile"]
			},
			"damage": {
				"$ref": "#/weapon_damage"
			},
			"damage_charged_factor": {
				"$ref": "#/weapon_damage"
			},
			"damage_charged_max": {
				"$ref": "#/weapon_damage"
			},
			"subsequent_swing_dmg_factor": {
				"type": "number",
				"minimum": 0
			},
			"anims": {
				"$ref": "#/anim_array"
			},
			"anims_last_shot": {
				"type": ["array", "null"],
				"items": {
					"$ref": "#/anim_index"
				},
				"minItems": 1,
				"maxItems": 4
			},
			"hit_anims": {
				"$ref": "#/anim_array"
			},
			"charge_anims": {
				"$ref": "anim_array"
			},
			"charge_time": {
				"type": "number",
				"minimum": 0
			},
			"charge_sound": {
				"$ref": "#/w_soundscript"
			},
			"charge_allow_holster": {
				"type": "boolean"
			},
			"charge_ammo_check": {
				"type": "boolean"
			},
			"charge_underwater_check": {
				"type": "boolean"
			},
			"charge_each_fire": {
				"type": "boolean"
			},
			"charge_damage": {
				"type": "boolean"
			},
			"charged_attack": {
				"type": "boolean"
			},
			"laser_spot_on_charge": {
				"type": "boolean"
			},
			"cooldown_anims": {
				"$ref": "anim_array"
			},
			"cooldown_time": {
				"type": "number",
				"minimum": 0
			},
			"cooldown_sound": {
				"$ref": "#/w_soundscript"
			},
			"sound": {
				"$ref": "#/w_soundscript"
			},
			"sound_additional": {
				"$ref": "#/w_soundscript"
			},
			"hit_body_sound": {
				"$ref": "#/w_soundscript"
			},
			"hit_wall_sound": {
				"$ref": "#/w_soundscript"
			},
			"empty_sound": {
				"$ref": "#/w_soundscript"
			},
			"use_standard_empty_sound": {
				"type": "boolean"
			},
			"spread": {
				"oneOf": [
					{
						"$ref": "#/spread_value"
					},
					{
						"$ref": "#/spread_value_pair"
					},
					{
						"type": "array",
						"items": {
							"type": "object",
							"properties": {
								"conditions": {
									"$ref": "#/player_movement_conditions"
								},
								"spread": {
									"oneOf": [
										{
											"$ref": "#/spread_value"
										},
										{
											"$ref": "#/spread_value_pair"
										}
									]
								}
							},
							"required": ["spread"],
							"additionalProperties": false
						},
						"minItems": 1,
						"maxItems": 5
					},
					{
						"type": "object",
						"properties": {
							"inaccuracy": {
								"type": "number",
								"minimum": 0
							},
							"inaccuracy_shift": {
								"type": "number",
								"minimum": 0
							},
							"recovery_time": {
								"type": "number",
								"minimum": 0
							},
							"factor": {
								"type": "number",
								"minimum": 0
							},
							"divisor": {
								"type": "number",
								"exclusiveMinimum": 0
							},
							"max_inaccuracy": {
								"type": "number",
								"minimum": 0
							},
							"type": {
								"enum": ["recovering", "quadratic", "qubic"]
							},
							"spread": {
								"oneOf": [
									{
										"type": "object",
										"properties": {
											"base": {
												"type": "number",
												"minimum": 0
											},
											"factor": {
												"type": "number",
												"minimum": 0
											}
										},
										"required": ["factor"],
										"additionalProperties": false
									},
									{
										"type": "array",
										"items": {
											"type": "object",
											"properties": {
												"conditions": {
													"$ref": "#/player_movement_conditions"
												},
												"base": {
													"type": "number",
													"minimum": 0
												},
												"factor": {
													"type": "number",
													"minimum": 0
												}
											},
											"required": ["factor"],
											"additionalProperties": false
										},
										"minItems": 1,
										"maxItems": 5
									}
								]
							}
						},
						"required": ["inaccuracy", "max_inaccuracy"],
						"additionalProperties": false
					}
				]
			},
			"cycle_time": {
				"type": "number",
				"exclusiveMinimum": 0
			},
			"cycle_time_last_shot": {
				"type": "number",
				"minimum": 0
			},
			"hit_cycle_time": {
				"type": "number",
				"minimum": 0
			},
			"idle_delay": {
				"$ref": "definitions.json#/range"
			},
			"idle_delay_empty": {
				"$ref": "definitions.json#/range"
			},
			"ammo_per_fire": {
				"type": "integer",
				"minimum": 0
			},
			"allow_underwater": {
				"type": "boolean"
			},
			"pump_delay": {
				"type": "number",
				"minimum": 0
			},
			"pump_sound": {
				"$ref": "#/w_soundscript"
			},
			"bullet_count": {
				"type": "integer",
				"minimum": 1
			},
			"tracer_freq": {
				"type": "integer",
				"minimum": 0
			},
			"burst": {
				"type": "integer",
				"minimum": "0"
			},
			"burst_interval": {
				"type": "number",
				"exclusiveMinimum": 0.0
			},
			"semiauto": {
				"type": "boolean"
			},
			"use_secondary_ammo": {
				"type": "boolean"
			},
			"autoaim": {
				"$ref": "#/spread_value"
			},
			"muzzleflash": {
				"type": "boolean"
			},
			"weapon_volume": {
				"oneOf": [
					{
						"enum": ["loud", "normal", "quiet"]
					},
					{
						"type": "integer",
						"minimum": 0
					}
				]
			},
			"weapon_flash": {
				"oneOf": [
					{
						"enum": ["bright", "normal", "dim"]
					},
					{
						"type": "integer",
						"minimum": 0
					}
				]
			},
			"delay_after_empty": {
				"type": "number",
				"exclusiveMinimum": 0
			},
			"delay_underwater": {
				"type": "number",
				"exclusiveMinimum": 0
			},
			"bullet_distance": {
				"type": "integer",
				"minimum": 0
			},
			"range_modifier": {
				"type": "number",
				"minimum": 0.0,
				"maximum": 1.0
			},
			"melee_distance": {
				"type": "integer",
				"minimum": 0
			},
			"client_punch_pitch": {
				"$ref": "definitions.json#/range"
			},
			"client_punch_yaw": {
				"$ref": "definitions.json#/range"
			},
			"shell": {
				"type": "object",
				"properties": {
					"count": {
						"type": "integer",
						"minimum": 0
					},
					"offset": {
						"oneOf": [
							{
								"type": "object",
								"properties": {
									"up": {
										"$ref": "definitions.json#/range"
									},
									"side": {
										"$ref": "definitions.json#/range"
									},
									"forward": {
										"$ref": "definitions.json#/range"
									}
								},
								"additionalProperties": false
							},
							{
								"type": "object",
								"properties": {
									"attachment": {
										"$ref": "#/attachment"
									}
								},
								"required": ["attachment"],
								"additionalProperties": false
							}
						]
					},
					"sound_type": {
						"enum": [null, "null", "shell", "shotgun_shell"]
					},
					"model": {
						"type": ["string", "null"]
					},
					"model_alternating": {
						"type": ["string", "null"]
					},
					"left_side": {
						"type": "boolean"
					},
					"velocity": {
						"type": "object",
						"properties": {
							"up": {
								"$ref": "definitions.json#/range"
							},
							"side": {
								"$ref": "definitions.json#/range"
							},
							"forward": {
								"$ref": "definitions.json#/range"
							}
						},
						"additionalProperties": false
					},
					"eject_delay": {
						"type": "number"
					}
				},
				"additionalProperties": false
			},
			"laser_suspend_time": {
				"type": "number",
				"minimum": 0
			},
			"kickback": {
				"oneOf": [
					{
						"$ref": "#/kickback"
					},
					{
						"type": "array",
						"items": {
							"type": "object",
							"properties": {
								"conditions": {
									"$ref": "#/player_movement_conditions"
								},
								"kickback": {
									"$ref": "#/kickback"
								}
							},
							"required": ["kickback"],
							"additionalProperties": false
						},
						"maxItems": 4
					}
				]
			},
			"kickback_on_hit_only": {
				"type": "boolean"
			},
			"pushback_force": {
				"type": "number",
				"minimum": 0
			},
			"pushback_vertical": {
				"type": "boolean"
			},
			"shake": {
				"$ref": "definitions.json#/shake"
			},
			"hit_shake": {
				"$ref": "definitions.json#/shake"
			},
			"smack_delay": {
				"type": "number",
				"minimum": 0
			},
			"hit_decal": {
				"type": "boolean"
			},
			"prevent_movement": {
				"type": "boolean"
			},
			"player_maxspeed": {
				"$ref": "definitions.json#/absolute_or_factor"
			},
			"player_maxspeed_on_charge": {
				"$ref": "definitions.json#/absolute_or_factor"
			},
			"projectile": {
				"type": "object",
				"properties": {
					"name": {
						"type": "string",
						"minLength": 1
					},
					"ent_template": {
						"$ref": "definitions.json#/ent_template"
					},
					"offset": {
						"type": "object",
						"properties": {
							"up": {
								"type": "number"
							},
							"side": {
								"type": "number"
							},
							"forward": {
								"type": "number"
							}
						},
						"additionalProperties": false
					},
					"respect_punchangle": {
						"type": "boolean"
					},
					"adjust_to_cross": {
						"type": "boolean"
					},
					"speed": {
						"type": "number",
						"exclusiveMinimum": 0
					},
					"add_player_velocity": {
						"enum": [false, true, "projection", "absolute"]
					},
					"time": {
						"type": "number",
						"minimum": 0
					},
					"cooked": {
						"type": "boolean"
					},
					"time_min": {
						"type": "number",
						"minimum": 0
					},
					"fire_phase_offsets": {
						"oneOf": [
							{
								"type": "array",
								"items": {
									"type": "object",
									"properties": {
										"up": {
											"type": "number"
										},
										"side": {
											"type": "number"
										}
									},
									"additionalProperties": false
								}
							},
							{
								"type": "object",
								"properties": {
									"start_angle": {
										"type": "number"
									},
									"count": {
										"type": "integer",
										"minimum": 1
									},
									"distance": {
										"type": "number",
										"exclusiveMinimum": 0
									},
									"type": {
										"enum": ["circle", "square"]
									},
									"orientation": {
										"enum": ["clockwise", "counter-clockwise"]
									}
								},
								"required": ["start_angle", "count"],
								"additionalProperties": false
							}
						]
					},
					"grenade_physics": {
						"enum": [false, true, "auto", "classic", "anniversary"]
					},
					"far_throw_anims": {
						"$ref": "#/anim_array"
					},
					"farthest_throw_anims": {
						"$ref": "#/anim_array"
					}
				},
				"additionalProperties": false
			},
			"extra_ai_sound": {
				"type": "object",
				"properties": {
					"type": {
						"oneOf": [
							{
								"$ref": "definitions.json#/ai_sound_type"
							},
							{
								"type": "array",
								"items": {
									"$ref": "definitions.json#/ai_sound_type"
								}
							}
						]
					},
					"time": {
						"type": "number",
						"exclusiveMinimum": 0
					}
				}
			},
			"spray": {
				"type": "object",
				"properties": {
					"offset": {
						"type": "object",
						"properties": {
							"up": {
								"type": "number"
							},
							"side": {
								"type": "number"
							},
							"forward": {
								"type": "number"
							}
						},
						"additionalProperties": false
					},
					"visual": {
						"$ref": "definitions.json#/visual_object"
					},
					"count": {
						"type": "integer",
						"minimum": 0
					},
					"speed": {
						"type": "integer",
						"minimum": 0
					},
					"spread": {
						"type": "number",
						"minimum": 0
					},
					"flags": {
						"type": "array",
						"items": {
							"enum": ["collideworld", "animate", "animated", "fadeout", "randombody", "nogravity"]
						}
					}
				},
				"additionalProperties": false
			},
			"viewmodel_beams": {
				"type": "array",
				"items": {
					"type": "object",
					"properties": {
						"start_attachment": {
							"$ref": "#/attachment"
						},
						"end_attachment": {
							"$ref": "#/attachment"
						},
						"visual": {
							"$ref": "definitions.json#/visual_object"
						}
					},
					"required": ["start_attachment", "end_attachment"],
					"additionalProperties": false
				}
			},
			"gunsmoke": {
				"type": "object",
				"properties": {
					"visual": {
						"oneOf": [
							{
								"type": "array",
								"items": {
									"$ref": "definitions.json#/visual_object"
								}
							},
							{
								"$ref": "definitions.json#/visual_object"
							}
						]
					},
					"policy": {
						"enum": ["auto", "allowed", "forced", "disallowed"]
					},
					"attachment": {
						"$ref": "#/attachment"
					},
					"forward_speed": {
						"type": "number"
					},
					"rising_acceleration": {
						"type": "number"
					},
					"start_rising_frame": {
						"type": "number",
						"minimum": 0,
						"maximum": 1
					},
					"interval": {
						"type": "number",
						"minimum": 0.0
					}
				},
				"additionalProperties": false
			},
			"switch_mode_delay": {
				"type": "number",
				"minimum": 0
			},
			"switch_mode_delay_empty": {
				"type": "number",
				"minimum": 0
			}
		},
		"additionalProperties": false
	},
	"switch_mode": {
		"type": "object",
		"properties": {
			"attack_delay": {
				"type": "number",
				"minimum": 0
			},
			"anim": {
				"$ref": "#/anim_index_optional"
			},
			"body_switch_delay": {
				"type": "number",
				"minimum": 0
			},
			"mode_switch_delay": {
				"type": "number",
				"minimum": 0
			},
			"end_anim": {
				"$ref": "#/anim_index_optional"
			},
			"end_anim_duration": {
				"type": "number",
				"minimum": 0
			}
		},
		"additionalProperties": false
	},
	"start_reload": {
		"type": "object",
		"properties": {
			"anim": {
				"$ref": "#/anim_index_optional"
			},
			"duration": {
				"type": "number",
				"minimum": 0
			}
		},
		"additionalProperties": false
	},
	"reload": {
		"type": ["object", "null"],
		"properties": {
			"anim": {
				"$ref": "#/anim_index_optional"
			},
			"duration": {
				"type": "number",
				"minimum": 0
			},
			"attack_delay": {
				"type": "number",
				"minimum": 0
			},
			"idle_delay": {
				"$ref": "definitions.json#/range_non_negative"
			},
			"sound": {
				"$ref": "#/w_soundscript"
			},
			"wait_for_recoil": {
				"type": "boolean"
			},
			"laser_suspend_time": {
				"type": "number",
				"minimum": 0
			},
			"ammo_count": {
				"type": "integer",
				"minimum": 0
			},
			"ammo_count_min": {
				"type": "integer",
				"minimum": 1
			}
		},
		"additionalProperties": false
	},
	"end_reload": {
		"type": "object",
		"properties": {
			"anim": {
				"$ref": "#/anim_index_optional"
			},
			"attack_delay": {
				"type": "number",
				"minimum": 0
			},
			"idle_delay": {
				"type": "number",
				"minimum": 0
			},
			"sound": {
				"$ref": "#/w_soundscript"
			}
		},
		"additionalProperties": false
	},
	"recharge": {
		"type": "object",
		"properties": {
			"interval": {
				"type": "number",
				"minimum": 0
			},
			"delay_after_fire": {
				"type": "number",
				"minimum": 0
			},
			"only_when_deployed": {
				"type": "boolean"
			},
			"sound": {
				"$ref": "#/w_soundscript"
			}
		},
		"additionalProperties": false
	},
)"
R"(
	"weapon_template": {
		"type": "object",
		"properties": {
			"from_scratch": {
				"type": "boolean"
			},
			"ammo_amount": {
				"$ref": "definitions.json#/range_int_non_negative"
			},
			"max_clip": {
				"type": "integer",
				"minimum": 0
			},
			"ammo_name": {
				"type": ["string", "null"],
				"minLength": 1
			},
			"secondary_ammo_name": {
				"type": ["string", "null"],
				"minLength": 1
			},
			"world_model": {
				"$ref": "#/model_path"
			},
			"view_model": {
				"$ref": "#/model_path"
			},
			"player_model": {
				"oneOf": [
					{
						"$ref": "#/model_path"
					},
					{
						"type": "null"
					}
				]
			},
			"player_anim_ext": {
				"type": "string",
				"minLength": 1
			},
			"priority": {
				"type": "integer"
			},
			"world_model_animated": {
				"type": "boolean"
			},
			"world_model_sequence": {
				"type": "integer",
				"minimum": 0
			},
			"deploy": {
				"$ref": "#/deploy"
			},
			"deploy_empty": {
				"$ref": "#/deploy"
			},
			"alt_deploy": {
				"$ref": "#/deploy"
			},
			"alt_deploy_empty": {
				"$ref": "#/deploy"
			},
			"idle": {
				"$ref": "#/idle"
			},
			"idle_empty": {
				"$ref": "#/idle"
			},
			"alt_idle": {
				"$ref": "#/idle"
			},
			"alt_idle_empty": {
				"$ref": "#/idle"
			},
			"fire": {
				"$ref": "#/fire"
			},
			"alt_fire": {
				"$ref": "#/fire"
			},
			"switch_mode": {
				"$ref": "#/switch_mode"
			},
			"switch_mode_back": {
				"$ref": "#/switch_mode"
			},
			"switch_mode_common": {
				"type": "object",
				"properties": {
					"toggle_laser_spot": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"zoom": {
				"type": "object",
				"properties": {
					"fov": {
						"type": "integer",
						"minimum": 1
					},
					"fov2": {
						"type": "integer",
						"minimum": 1
					},
					"sound": {
						"$ref": "#/w_soundscript"
					},
					"sound2": {
						"$ref": "#/w_soundscript"
					},
					"unzoom_sound": {
						"$ref": "#/w_soundscript"
					},
					"fade": {
						"type": "object",
						"properties": {
							"color": {
								"$ref": "definitions.json#/color"
							},
							"fade_time": {
								"type": "number",
								"minimum": 0
							},
							"hold_time": {
								"type": "number",
								"minimum": 0
							},
							"alpha": {
								"$ref": "definitions.json#/alpha"
							}
						}
					},
					"reset_on_fire": {
						"type": "boolean"
					},
					"resume_after_reset": {
						"type": "boolean"
					},
					"hide_viewmodel": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"secondary_attack": {
				"enum": ["alt_fire", "switch_mode", "disabled"]
			},
			"prioritize_primary_attack": {
				"type": "boolean"
			},
			"player_maxspeed": {
				"$ref": "definitions.json#/absolute_or_factor"
			},
			"player_maxspeed_alt": {
				"$ref": "definitions.json#/absolute_or_factor"
			},
			"prevent_jump": {
				"type": "boolean"
			},
			"exhaustible": {
				"type": "boolean"
			},
			"shared_charge_and_cooldown": {
				"type": "boolean"
			},
			"viewmodel_body": {
				"type": "integer",
				"minimum": 0
			},
			"viewmodel_body_alt": {
				"type": "integer",
				"minimum": 0
			},
			"ammo_to_viewmodel_body": {
				"type": "object",
				"patternProperties": {
					"^[0-9]+$": {
						"type": "integer",
						"minimum": 0
					}
				},
				"additionalProperties": false
			},
			"reload": {
				"$ref": "#/reload"
			},
			"reload_empty": {
				"$ref": "#/reload"
			},
			"alt_reload": {
				"$ref": "#/reload"
			},
			"alt_reload_empty": {
				"$ref": "#/reload"
			},
			"start_reload": {
				"$ref": "#/start_reload"
			},
			"start_reload_empty": {
				"$ref": "#/start_reload"
			},
			"alt_start_reload": {
				"$ref": "#/start_reload"
			},
			"alt_start_reload_empty": {
				"$ref": "#/start_reload"
			},
			"end_reload": {
				"$ref": "#/end_reload"
			},
			"end_reload_empty": {
				"$ref": "#/end_reload"
			},
			"alt_end_reload": {
				"$ref": "#/end_reload"
			},
			"alt_end_reload_empty": {
				"$ref": "#/end_reload"
			},
			"manual_reload": {
				"type": ["boolean", "object"],
				"properties": {
					"continue_on_deploy": {
						"type": "boolean"
					},
					"restart_on_deploy": {
						"type": "boolean"
					}
				},
				"additionalProperties": false
			},
			"reload_autostart": {
				"type": "boolean"
			},
			"start_in_alt_mode": {
				"type": "boolean"
			},
			"mirror_viewmodel": {
				"type": "boolean"
			},
			"recharge": {
				"$ref": "#/recharge"
			},
			"recharge_alt": {
				"$ref": "#/recharge"
			},
			"laser_spot": {
				"type": "object",
				"properties": {
					"start_on": {
						"type": "boolean"
					},
					"attract_rockets": {
						"type": "boolean"
					},
					"check_active_rockets": {
						"type": "boolean"
					},
					"scale": {
						"type": "number",
						"exclusiveMinimum": 0
					},
					"activation_sound": {
						"$ref": "#/w_soundscript"
					},
					"deactivation_sound": {
						"$ref": "#/w_soundscript"
					}
				},
				"additionalProperties": false
			},
			"model_sounds": {
				"type": "array",
				"items": {
					"type": "string"
				}
			},
			"tool": {
				"type": "object",
				"properties": {
					"icon": {
						"type": "string"
					},
					"trigger_delay": {
						"type": "number",
						"minimum": 0
					},
					"deny_sound": {
						"$ref": "#/w_soundscript"
					},
					"delay_after_deny": {
						"type": "number",
						"exclusiveMinimum": 0
					}
				},
				"additionalProperties": false
			},
			"dropammo": {
				"$ref": "#/dropammo"
			},
			"dropammo_secondary": {
				"$ref": "#/dropammo"
			}
		},
		"additionalProperties": false
	}
}
)";
