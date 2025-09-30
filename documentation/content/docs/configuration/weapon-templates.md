---
title: "Weapon templates"
toc_start_level: 1
---

# {{% param "title" %}}

Weapon templates allow to configure some aspects of weapon behavior via **templates/weapons.json** file. This includes fire rate, sounds, deploy and reload delays, animation indices and more.

# Format of templates/weapons.json

The document is an object where each property presents a weapon template. The keys must be weapon entity names, case sensitive (e.g. `"weapon_9mmhandgun"`). The value must either be a weapon template or a string referring to the .json file under **templates/weapons/** directory (**.json** extension can be omitted) and in its turn this file must contain a document representing a single weapon template.

Example:

```json
{
    "weapon_9mmhandgun": {
        "alt_fire": {
            "cycle_time": 0.5,
            "burst": 3,
            "burst_interval": 0.1,
            "spread": 0.03
        }
    },
    "weapon_eagle": "cs/deagle",
    "weapon_shotgun": "cs/m3"
}
```

This defines custom configuration for `"weapon_9mmhandgun"` right in the **templates/weapons.json** file.
Custom configurations for `"weapon_eagle"` and `"weapon_shotgun"` reside in **templates/weapons/cs/deagle.json** and **templates/weapons/cs/m3.json**.

{{% hint info %}}
See [Premade weapon templates]({{< ref "weapon-templates-presets" >}}) for ready-to-use weapon presets.
{{% /hint %}}

Currently not all weapons are fully configurable. Some may support only certain properties.

## Common configurable properties

These properties are available for all weapons:

* [world_model](#world_model)
* [view_model](#view_model)
* [player_model](#player_model)
* [player_anim_ext](#player_anim_ext)
* [priority](#priority)
* [model_sounds](#model_sounds)
* [max_clip](#max_clip)
* [ammo_name](#ammo_name)
* [secondary_ammo_name](#secondary_ammo_name)
* [empty_sound](#empty_sound)
* [use_standard_empty_sound](#use_standard_empty_sound)

## Fully configurable weapons

The following weapons can utilize the weapon configuration to the full potential:

* [weapon_9mmAR]({{< ref weapon_9mmAR >}})
* [weapon_9mmhandgun]({{< ref weapon_9mmhandgun >}})
* [weapon_357]({{< ref weapon_357 >}})
* [weapon_crowbar]({{< ref weapon_crowbar >}})
* [weapon_crossbow]({{< ref weapon_crossbow >}})
* [weapon_eagle]({{< ref weapon_eagle >}})
* [weapon_hornetgun]({{< ref weapon_hornetgun >}})
* [weapon_m249]({{< ref weapon_m249 >}})
* [weapon_pipewrench]({{< ref weapon_pipewrench >}})
* [weapon_rpg]({{< ref weapon_rpg >}})
* [weapon_shotgun]({{< ref weapon_shotgun >}})
* [weapon_shockrifle]({{< ref weapon_shockrifle >}})
* [weapon_sniperrifle]({{< ref weapon_sniperrifle >}})
* [weapon_sporelauncher]({{< ref weapon_sporelauncher >}})
* [weapon_uzi]({{< ref weapon_uzi >}})

### Additional weapons

Some weapon classnames have been added specifically to be configured (so the developer can configure their own guns). These weapons are as well fully configurable:

* [weapon_pistol]({{< ref weapon_pistol >}})
* [weapon_pistol2]({{< ref weapon_pistol2 >}})
* [weapon_smg]({{< ref weapon_smg >}})
* [weapon_smg2]({{< ref weapon_smg2 >}})
* [weapon_rifle]({{< ref weapon_rifle >}})
* [weapon_rifle2]({{< ref weapon_rifle2 >}})
* [weapon_shotgun2]({{< ref weapon_shotgun2 >}})
* [weapon_sniperrifle2]({{< ref weapon_sniperrifle2 >}})

{{% hint info %}}
A technical limitation in GoldSource engine prevents creation of new classnames dynamically (i.e. without code modifications). So we have to resort to this trick of providing the predefined classnames that can be utilized by the SDK user.

The names themselves don't limit the weapon configuration in any way. E.g. `weapon_pistol` could be configured to act as a shotgun or a sniper rifle. The names are provided solely for the user convenience.
{{% /hint %}}

## Partially configurable weapons

The following weapons have only partial support for configuration:

* [weapon_displacer]({{< ref weapon_displacer >}})
    * idle animations
    * deploy animation
    * `"ammo_per_fire"` (for both primary and secondary attack)
    * `"secondary_attack"` can be set to `"disabled"` to disable self-teleport.
* [weapon_egon]({{< ref weapon_egon >}})
    * idle animations
    * deploy animation
    * `"reload"` (in case the weapon is configured to have a clip)
* [weapon_gauss]({{< ref weapon_gauss >}})
    * idle animations
    * deploy animation
    * fire animations (for primary attack)
    * `"cycle_time"` (for primary attack)
    * `"ammo_per_fire"` (for primary attack)
    * `"allow_underwater"`
    * `"weapon_volume"`
    * `"client_punch_pitch"`
    * `"client_punch_yaw"`
    * `"secondary_attack"` can be set to `"disabled"` to disable spinning attack.
    * `"reload"` (in case the weapon is configured to have a clip)
* [weapon_handgrenade]({{< ref weapon_handgrenade >}})
    * idle animations
    * deploy animation
* [weapon_medkit]({{< ref weapon_medkit >}})
    * idle animations
    * deploy animation
* [weapon_tripmine]({{< ref weapon_tripmine >}})
    * idle animations
    * deploy animation

# Weapon template properties

Each weapon template can define following properties:

## max_clip

Allows to change the weapon default max clip.

{{% hint info %}}
When setting a max clip for a weapon that doesn't usually use clips (e.g. [weapon_egon]({{< ref weapon_gauss >}})) one also should provide the [reload](#reload) parameters.
{{% /hint %}}

Set `max_clip` to 0 to disable weapon clip. This means the weapon won't need to be reloaded and will use ammo right from the player's ammo pool.

```json
{
    "weapon_9mmAR": {
        "max_clip": 0
    },
    "weapon_shotgun": {
        "max_clip": 6
    }
}
```

## ammo_amount

Allows to change the ammo amount the weapon comes with by default. This might be different from [max_clip](#max_clip). E.g. HL shotgun comes with 12 shells while having a clip of 8 shells at max. This also can be [range_int]({{< ref "JSON/#range_int" >}}) for randomized starting ammo amount.

```json
{
    "weapon_357": {
        "ammo_amount": [3,6]
    },
    "weapon_shotgun": {
        "ammo_amount": 8
    }
}
```

## ammo_name

Allows to change the primary ammo the weapon is using. This only works for weapons that use ammo.

```json
{
    "weapon_shotgun": {
        "ammo_name": "9mm"
    },
    "weapon_crossbow": {
        "ammo_name": "uranium"
    }
}
```

Possible ammo names:

* `"buckshot"`
* `"9mm"`
* `"ARgrenades"`
* `"357"`
* `"uranium"`
* `"rockets"`
* `"bolts"`
* `"Trip Mine"`
* `"Satchel Charge"`
* `"Hand Grenade"`
* `"Snarks"`
* `"Hornets"`
* `"Medicine"`
* `"Penguins"`
* `"556"`
* `"762"`
* `"Shocks"`
* `"spores"`
* `"45acp"`
* `"57mm"`
* `"nails"`

{{% hint warning %}}
You still need to change the ammo sprite in the weapon hud .txt file to match the actual ammo type.
{{% /hint %}}

{{% hint info %}}
The max ammo carried by the player can be configured via **features/maxammo.cfg**.
{{% /hint %}}

## secondary_ammo_name

Allows to change the secondary ammo the weapon is using (for attacks that set [use_secondary_ammo](#use_secondary_ammo)).

```json
{
    "weapon_9mmAR": {
        "secondary_ammo_name": "Hand Grenade"
    }
}
```

## world_model

The path to the model used by a weapon when it's lying in the world. Usually they have "w_" prefix, e.g. `"models/w_shotgun.mdl"`.

## view_model

The path to the model used by a weapon when shown in the first person. Usually they have "v_" prefix, e.g. `"models/v_shotgun.mdl"`.

## player_model

The path to the model used by a weapon when shown on the player model, e.g. in third person or when looking at other players in multiplayer. Usually they have "p_" prefix, e.g. `"models/p_shotgun.mdl"`.

This also can be set to `null` to disable the weapon model in third person on this specific weapon.

## player_anim_ext

The player animation extension (suffix) that defines how the weapon is carried in player model hands. This is visible in third person or when looking at other players in multiplayer. Default extensions:

* `"bow"`
* `"crowbar"`
* `"egon"`
* `"gauss"`
* `"mp5"`
* `"onehanded"`
* `"python"`
* `"rpg"`
* `"shotgun"`
* `"squeak"`
* `"trip"`

## priority

The weapon deploy priority. The weapon with the highest priority will be deployed at the start if players gets weapons via the [map config]({{< ref "map-config" >}}) or via [game_player_settings]({{< ref game_player_settings >}}). This also defines whether player will switch to the newly picked weapon in multiplayer automatically.

## model_sounds

An array of sounds used in the view model animation events (event 5004). These sounds will be precached by the server.

```json
{
    "weapon_smg": {
        "model_sounds": [
            "weapons/ap9_clipout.wav",
            "weapons/ap9_clipin.wav",
            "weapons/ap9_bolt.wav",
            "items/cliprelease1.wav"
        ]
    }
}
```

## from_scratch

A boolean defining whether the weapon configuration should start from scratch, i.e. from the default weapon template. This preserves only the following properties:

* `"world_model"`
* `"view_model"`
* `"player_model"`
* `"player_anim_ext"`
* `"priority"`
* `"idle"`
* `"deploy"`

Everything else should be manually defined.

This option is useful if you want to ensure there're no lingering options left from the original weapon.

## deploy

An object defining properties for weapon deploy customization.

```json
{
    "weapon_9mmAR": {
        "deploy": {
            "anim": 2,
            "duration": 0.89
        }
    }
}
```

### anim

The animation index. Must be a non-negative integer number. Look up the animation indices in the .mdl file of the weapon view model.

### duration

Delay before the weapon is usable (i.e. can fire). This is 0.5 by default for most weapons.

### idle_delay

Delay before playing the idle animation. This is 1.0 by default. If this is less than `"duration"`, the latter will be used instead.

### sound

[Weapon soundscript](#weapon-soundscript) to play along the deploy animation. This is not needed if the sound is played by the event in the model.

## deploy_empty

Same as [deploy](#deploy), but when weapon clip is empty.

If some option is not defined the value from [deploy](#deploy) is used. Some weapons define this by default:

* [weapon_crossbow]({{< ref weapon_crossbow >}}) uses a different deploy animation when its clip is empty.

Set this property to `null` to remove the native weapon rules.

## alt_deploy

Same as [deploy](#deploy), but when weapon is in alternative mode.

If some option is not defined the value from [deploy](#deploy) is used.

## alt_deploy_empty

Same as [deploy](#deploy), but when weapon is in alternative mode and weapon clip is empty.

If some option is not defined the value from [alt_deploy](#alt_deploy) or [deploy_empty](#deploy_empty) options is used (depending on which one is defined).

## idle

Customize weapon idle animations.

Can be defined in two ways: either array (if multiple animations are used) or single object (for single idle animation).

```json
{
    "weapon_9mmhandgun": {
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
        ]
    },
    "weapon_eagle": {
        "idle": {
            "anim": 0,
            "duration": 20
        }
    }
}
```

### anim

The animation index. Must be a non-negative integer number. Look up the animation indices in the .mdl file of the weapon view model.

### duration

Delay before repicking the idle animation. This is usually duration of the animation as defined in the .mdl file (i.e. number of frames divided by animation fps). This can be [range]({{< ref "JSON/#range" >}}) for randomized delays.

### chance

Chance that this animation will be picked (in relation to the sum of chances of all idle animations). The chances are used only when there're multiple idle animations.

## idle_empty

Same as [idle](#idle), but when weapon clip is empty.

If not defined the [idle](#idle) options are used. Some weapons define this by default:

* [weapon_9mmhandgun]({{< ref weapon_9mmhandgun >}}) defines it as empty array to prevent playing idle animations after firing the last shot.
* [weapon_eagle]({{< ref weapon_eagle >}}) defines it as empty array to prevent playing idle animations after firing the last shot.
* [weapon_crossbow]({{< ref weapon_crossbow >}}) defines a different set of idle animations for empty clip.

Set `null` to remove the native weapon rules.

```json
{
    "weapon_9mmhandgun": {
        "idle_empty": null
    }
}
```

## alt_idle

Same as [idle](#idle), but when weapon is in alternative mode.

If not defined the [idle](#idle) options are used. Some weapons define this by default:

* [weapon_eagle]({{< ref weapon_eagle >}}) uses different set of animations when the laser spot is on.

Set `null` to remove the native weapon rules.

## alt_idle_empty

Same as [idle](#idle), but when weapon is in alternative mode and weapon clip is empty.

If not defined the [alt_idle](#alt_idle) or [idle_empty](#idle_empty) options are used (depending on which are defined). Some weapons define this by default:

* [weapon_eagle]({{< ref weapon_eagle >}}) defines it as empty array to prevent playing idle animations after firing the last shot.

Set `null` to remove the native weapon rules.

## fire

Customize weapon firing options like delay between shots, fire spread and recoil.

```json
{
    "weapon_9mmAR": {
        "fire": {
            "anims": [3,4,5],
            "sound": ["weapons/mp5-1.wav", "weapons/mp5-2.wav"],
            "spread": 0.04,
            "cycle_time": 0.075,
            "idle_delay": 2,
            "weapon_flash": "dim",
            "client_punch_pitch": 0
        }
    }
}
```

### type

The attack type. Current supported values are:

* `"bullet"` or `"bullets"` (these are synonyms) - used by most firearms.
* `"melee"` - used by melee weapons.
* `"projectile"` - the weapon fires projectiles. This requires [projectile]({#projectile}) to be defined.
* Note: configuration of laser attacks will come later.

### allow_underwater

A boolean defining whether the weapon can fire underwater.

### ammo_per_fire

Ammo spent per fire. E.g. [weapon_shotgun]({{< ref weapon_shotgun >}}) uses two shells on the secondary attack.

Note: this has nothing to do with the number of bullets fired. It's just how much ammo is used.

### anims

The array of animation indices. The random animation is picked when weapon is firing.

### anims_last_shot

Same as [anims](#anims), but used when weapon becomes out of ammo. Some weapons define this by default:

* [weapon_9mmhandgun]({{< ref weapon_9mmhandgun >}})
* [weapon_crossbow]({{< ref weapon_crossbow >}})
* [weapon_eagle]({{< ref weapon_eagle >}})
* [weapon_sniperrifle]({{< ref weapon_sniperrifle >}})

Set it to `null` to remove the defaults and let the weapon always use [anims](#anims).

### autoaim

An autoaim degree (used when autoaim is enabled). This must be a number between 0 and 1 (the `sin` of autoaim angle) where 0 means no autoaim or the string in the form `"5degrees"`, `"8degrees"`, etc. The number before `degrees` is the angle. It must be an integer between 1 and 10.

```json
{
    "weapon_9mmhandgun": {
        "fire": {
            "autoaim": "10degrees"
        }
    },
    "weapon_9mmAR": {
        "fire": {
            "autoaim": "5degrees"
        },
        "alt_fire": {
            "autoaim": 0
        }
    }
}
```

### bullet_count

The number of bullets (pellets) fired per shot (on the same frame). Each bullet/pellet has its own trajectory randomized based on the weapon [spread](#spread). Most weapons fire a single bullet, but [weapon_shotgun]({{< ref weapon_shotgun >}}) fires 6 pellets on primary attack and 12 pellets on secondary attack.

### bullet_distance

The distance of trace checks when detecting whether the bullet hit something. This is usually 8192, but set to 2048 for shotguns.

### burst

The maximum number of shots for the burst fire. I.e. player presses the attack button once, but the weapon fires several times in a row (if there's enough ammo). Setting this to 1 removes the burst fire.

Example of a weapon using the burst fire: Counter Strike FAMAS in the burst mode.

### burst_interval

The interval between shots in burst fire. Ideally the [cycle_time](#cycle_time) must be higher than this multiplied by the [burst](#burst) value.

### charge_anims

The array of animation indices. The random animation is picked when weapon is charging its fire. Used by [weapon_minigun]({{< ref weapon_minigun >}}), [weapon_pipewrench]({{< ref weapon_pipewrench >}}) and [weapon_knife]({{< ref weapon_knife >}}). This requires [charge_time](#charge_time) to be set.

### charge_sound

[Weapon soundscript](#weapon-soundscript) to play on charging the attack. Used by [weapon_minigun]({{< ref weapon_minigun >}}).

### charge_time

The time in seconds after the weapon starts firing after initial charge. Used by [weapon_minigun]({{< ref weapon_minigun >}}). This is also the minimum time before melee weapons do their swing attack (e.g. [weapon_pipewrench]({{< ref weapon_pipewrench >}})).

### cooldown_anims

The array of animation indices. The random animation is picked when weapon after the weapon has fired. Used by [weapon_minigun]({{< ref weapon_minigun >}}) for spindown animation.

### cooldown_sound

[Weapon soundscript](#weapon-soundscript) to play after firing. Used by [weapon_minigun]({{< ref weapon_minigun >}}).

### cooldown_time

Delay in seconds before weapon can switch to the idle animation after firing. Usually it's a length of the cooldown animation. Used by [weapon_minigun]({{< ref weapon_minigun >}}).

### client_punch_pitch

The vertical punch angle applied on each shot fired. This can be [range]({{< ref "JSON/#range" >}}) for randomized punch, e.g. `[-2, 2]`. Use negative numbers to punch upwards. Set to 0 to disable.

Note: this is a client-side punch that only affects the player's visual view, but doesn't change where the next bullet will hit.

### client_punch_yaw

Same as [client_punch_pitch](#client_punch_pitch) but for lateral punch. Use negative number to punch to the right. The only weapon that uses it by default is [weapon_m249]({{< ref weapon_m249 >}}).

### cycle_time

The delay between shots, in seconds. This defines the rate of fire: the less the value, the higher the rate.

### damage

The amount of damage the weapon deals per bullet.

This can be either a number (e.g. `8`) or the name of the skill cvar (e.g. `"sk_plr_357_bullet"`). Currently it's not possible to register new skill variables, so you can just use a constant number (player's damage values are usually not affected by difficulty anyway).

Note: this currently can't configure damage values for anything other than bullets.

### delay_after_empty

Delay before attempting to fire again after trying to shoot without enough ammo. This is basically a delay between "weapon empty" sounds.

### delay_underwater

Delay before attempting to fire again after trying to shoot underwater (if weapon can't fire underwater).

### empty_sound

Custom [weapon soundscript](#weapon-soundscript) to play when player is trying to fire a weapon without having enough ammo for it or underwater with [allow_underwater](#allow_underwater) set to `false`. Defining this automatically sets [use_standard_empty_sound](#use_standard_empty_sound) to `false`.

Example:

```json
{
    "weapon_gauss": {
        "fire": {
            "empty_sound": {
                "waves": ["buttons/button11.wav"]
            }
        }
    }
}
```

### use_standard_empty_sound

A boolean defining whether the default **Weapon.Empty** soundscript (see [common item soundscripts]({{< ref "soundscripts#common-item-soundscripts" >}})) should be played when player is trying to fire a weapon without having enough ammo for it or underwater with [allow_underwater](#allow_underwater) set to `false`.

This is `true` by default. You must define [empty_sound](#empty_sound) to change the empty sound or set this property to `false` to remove the empty sound altogether.

### idle_delay

Delay before starting playing idle animations after firing a shot. This can be [range]({{< ref "JSON/#range" >}}). By default it's `[10, 15]`.

### idle_delay_empty

Same as [idle_delay](#idle_delay), but when weapon fired the last bullet.

### kickback

Player view camera kickback angle. As opposite to [client_punch_pitch](#client_punch_pitch) and [client_punch_yaw](#client_punch_yaw) this kickback is server-side, thus it actually may affect the direction of the next shot.

The kickback can be defined in several ways.

#### Random kickback on both axes

```json
{
    "weapon_9mmAR": {
        "client_punch_pitch": 0,
        "kickback": 2
    }
}
```

In this example the kickback will be randomized in range between -2 and 2, on both axes.

#### Different values for vertical and lateral kickback

```json
{
    "weapon_9mmAR": {
        "client_punch_pitch": 0,
        "kickback": {
            "vertical": 2,
            "lateral": 1
        }
    }
}
```

In this example the kickback will be randomized in range between -2 and 2 on vertical axis and between -1 and 1 on horizontal axis.

#### Dynamically increasing kickback from Counter Strike

Some Counter Strike weapons (rifles and SMGs) dynamically increase their kickback with each shot fired. This helps to simulate the recoil.

New kickback angle is calculated as *current_angle* + *base_kickback* + *shots_fired* * *modifier*

Then the value is clamped to the specified maximum to prevent endlessly growing kickback.

To add the dynamically increasing kickback `"vertical_max"` or `"lateral_max"` should be defined.

```json
{
    "weapon_9mmAR": {
        "client_punch_pitch": 0,
        "kickback": {
            "vertical": 0.9,
            "lateral": 0.45,
            "vertical_modifier": 0.25,
            "lateral_modifier": 0.035,
            "vertical_max": 3.5,
            "lateral_max": 2.75,
            "lateral_persistance": 7
        }
    },
    "weapon_sniperrifle": {
        "client_punch_pitch": 0,
        "kickback": {
            "vertical": 2.0,
            "vertical_max": 4.0
        }
    }
}
```

* `"vertical"` - base vertical kickback angle.
* `"lateral"` - base lateral kickback angle.
* `"vertical_modifier"` - how much each shot impacts the vertical kickback.
* `"lateral_modifier"` - how much each shot impacts the lateral kickback.
* `"vertical_max"` - the maximum vertical kickback angle.
* `"lateral_max"` - the maximum lateral kickback angle.
* `"lateral_persistance"` - a positive integer value. The higher this value the less the chance the kickback starts going to the oppposite direction. Weapons start kickback to the right and with `"lateral_persistance"` may start going left during firing. This creates the famous 7-like or T-like spread pattern in Counter Strike.
* `"vertical_persistance"` - same as `"lateral_persistance"` but for changing the vertical direction. The weapons start kickback up. This wasn't used in Counter Strike (so recoil always goes up) and added just for completeness.

#### Kickback depending on the player movement

Kickback can depend on whether the player is in air, moving or ducking.

In this case the kickback is defined as an array of objects with [player movement](#Player-movement-conditions) `"conditions"` property and `"kickback"` property used when the condtions are met. Each `"kickback"` can be in any form mentioned before (single value or object).

```json
{
    "weapon_9mmAR": {
        "fire": {
            "client_punch_pitch": 0,
            "kickback": [
                {
                    "conditions": {
                        "in_air": true
                    },
                    "kickback": 4
                },
                {
                    "conditions": {
                        "moving": true
                    },
                    "kickback": 3
                },
                {
                    "conditions": {
                        "ducking": true
                    },
                    "kickback": 1
                },
                {
                    "kickback": 2
                }
            ]
        }
    }
}
```

Notes:

* Conditions are checked in the order of definition. The first matched entry is used, so the order matters.
* If no conditions are met, the last entry in the array is used even if it has conditions defined - they're ignored. So the last entry should be the one without conditions.
* The maximum number of rules is **4**.
* The conditions can be used together (e.g. if you want to check whether player is both ducking and moving).

### laser_suspend_time

Hide the laser spot (if there's any) for the specified amount of time (in seconds) upon firing.

### muzzleflash

Whether the entity light should be played on the weapon view model for a very short period of time when firing.

### player_maxspeed {#fire-player_maxspeed}

Set the player's maximum speed for the duration of the attack (currently it's the same as [cycle_time](#cycle_time)). This can be a positive numeric value or a string starting with `*` followed by the factor value which works as a multiplier for the base player's maxspeed (less than 1).

```json
{
    "weapon_shotgun": {
        "fire": {
            "player_maxspeed": "*0.8"
        },
        "alt_fire": {
            "player_maxspeed": 200
        }
    }
}
```

See also: [player's maximum speed]({{< ref "player#maximum-speed" >}}).

{{% hint info %}}
Use upper level [player_maxspeed](#player_maxspeed) parameter to change the player's maximum speed depending on the active equipped weapon. If both upper level `player_maxspeed` and fire-level `player_maxspeed` are defined, the latter is prioritized during the fire (so you can make a weapon that generally slows down the player when equipped but allows to move at higher speed when firing).
{{% /hint %}}

{{% hint info %}}
In order to disable player's movement for the attack duration it's better to use [prevent_movement](#prevent_movement).
{{% /hint %}}

### prevent_movement

A boolean defining whether player can't move during firing. This is used by [weapon_camera]({{< ref weapon_camera >}}) and [weapon_radio]({{< ref weapon_radio >}}).

### projectile

An object defining the properties of the fired projectile if the [fire type](#type) is `"projectile"`.

#### name

The name of the projectile type. Technically is can be any existing entity classname, but only certain entities support proper launching logic:

* [bmortar]({{< ref bmortar >}})
* [charged_bolt]({{< ref charged_bolt >}})
* [controller_energy_ball]({{< ref controller_energy_ball >}})
* [crossbow_bolt]({{< ref crossbow_bolt >}})
* [displacer_ball]({{< ref displacer_ball >}})
* [grenade]({{< ref grenade >}}) - AR grenade.
* [hornet]({{< ref hornet >}}) - tracking hornet.
* [hvr_rocket]({{< ref hvr_rocket >}})
* [mortar_shell]({{< ref mortar_shell >}})
* [pitdronespike]({{< ref pitdronespike >}})
* [rpg_rocket]({{< ref rpg_rocket >}})
* [shock_beam]({{< ref shock_beam >}})
* [spore]({{< ref spore >}}) - throwable spore (as thrown by [shock troopers]({{< ref monster_shocktrooper >}})).
* [squidspit]({{< ref squidspit >}})
* [squidtoxicspit]({{< ref squidtoxicspit >}})

There're also some predefined names to support alternative variants of existing projectiles:

* `"crossbow_bolt explosive"` - explosive crossbow bolt (Half-Life deathmatch).
* `"hornet dart"` - fast non-tracking hornets (hornetgun secondary attack).
* `"hand grenade"` - timed hand grenade.
* `"spore rocket"` - contact spore that flies forward (spore launcher primary attack).
* `"spore bouncy"` - spore that bounces of the surfaces (spore launcher secondary attack).

Example: make hornetgun fire squid spits instead of hornets on primary attack.

```json
{
    "weapon_hornetgun": {
        "fire": {
            "projectile": {
                "name": "squidspit"
            }
        }
    }
}
```

#### ent_template

The name of the [entity template]({{< ref entity-templates >}}) for the projectile. This allows to configure visuals and soundscripts for the fired projectile.

#### offset {#projectile-offset}

The offset that defines how far from the player's eyes the projectile should spawn. The object can define the following values:

* `"forward"` - forward offset (how far from the player's camera).
* `"side"` - offset to the right side. Make it negative to offset to the left side.
* `"up"` - vertical offset. It's usually a negative value since weapons are drawn at the bottom.

Example:

```json
{
    "weapon_hornetgun": {
        "fire": {
            "projectile": {
                "offset": {
                    "forward": 16,
                    "side": 8,
                    "up": -12,
                }
            }
        }
    }
}
```

#### adjust_to_cross

A boolean - whether the projectile must be adjusted to the screen's center. Depending on the `offset` the projectile might shoot at the point lower and to the side from the crosshair (this is especially noticeable in Opposing Force with shockrifle and sporelauncher). This is `true` by default, but `false` for the following weapons:

* [weapon_crossbow]({{< ref weapon_crossbow >}})
* [weapon_hornetgun]({{< ref weapon_hornetgun >}}) primary attack
* [weapon_9mmAR]({{< ref weapon_9mmAR >}}) secondary attack.
* [weapon_rpg]({{< ref weapon_rpg >}})

#### respect_punchangle

A boolean - whether the current player's punchangle should be accounted for when setting the projectile angle. This is `true` by default, but `false` for the following weapons:

* [weapon_hornetgun]({{< ref weapon_hornetgun >}})
* [weapon_rpg]({{< ref weapon_rpg >}})
* [weapon_sporelauncher]({{< ref weapon_sporelauncher >}})

#### speed

Custom speed for a projectile. This must be a number higher than 0. If not set the speed depends on the projectile.

#### time

Time before detonation, in seconds. This is used by `"hand grenade"`, `"spore"` and `"spore bouncy"`.

```json
{
    "weapon_hornetgun": {
        "fire": {
            "projectile": {
                "name": "hand grenade",
                "time": 1.5
            }
        }
    }
}
```

#### add_player_velocity

How (and if) the player's current velocity should be added to the one of the projectile. Possible values:

* `false` - don't add player's velocity (default).
* `true` - add player's velocity (like in hand grenades or satchels).
* `"absolute"` - same as `true`
* `"projection"` - add the player's velocity vector projected on the fire direction vector (via dot product). Used by RPG by default.

#### fire_phase_offsets

An array that defines the additional offsets for the projectile position - these offsets alternate with each shot. By default this is used by the secondary attack of [weapon_hornetgun]({{< ref weapon_hornetgun >}}).

Each item in the array if the object that can have the following properties:

* `"side"` - additional offset to the right side. Use negative values for the offset to the left side.
* `"up"` - additional vertical offset.

Example: hornetgun's fire phase offsets defined as an array:

```json
{
    "weapon_hornetgun": {
        "fire": {
            "projectile": {
                "fire_phase_offsets": [
                    {
                        "side": 0,
                        "up": 8
                    },
                    {
                        "side": 8,
                        "up": 8
                    },
                    {
                        "side": 8,
                        "up": 0
                    },
                    {
                        "side": 8,
                        "up": -8
                    },
                    {
                        "side": 0,
                        "up": -8
                    },
                    {
                        "side": -8,
                        "up": -8
                    },
                    {
                        "side": -8,
                        "up": 0
                    },
                    {
                        "side": -8,
                        "up": 8
                    },
                ]
            }
        }
    }
}
```

Alternatively if the offsets form a square or an equilateral polygon and come in the clockwise or counter-clockwise order, the fire phase offsets can be defined via the object with the following properties:

* `"start_angle"` - a starting angle in degrees where the phase starts. E.g. `90` means the first shot will have an offset going up. The value `0` means the direction to the right. This option is required.
* `"count"` - the number of phases. Used to calculate the angle offset per phase step. E.g. `8` means that each fraction will be 360/8 = 45 degrees. This option is required.
* `"distance"` - the offset distance.
* `"type"` - how to interpret the offset. The value `"circle"` means to interpret the offset as a circle radius. The value `"square"` means to interpret the distance as a half of the square side length. E.g. considering the `"distance"` is set to 1, and `"start_angle"` is set to 45 the initial offset would be 1, 1 for `"square"` type and approximately 0.707, 0.707 (cos and sin of 45 degree) for a `"circle"` type. The default value is `"circle"`.
* `"orientation"` - in which direction the angle fraction should be changed for the next fire phase. Possible values: `"clockwise"` and `"counter-clockwise"`. The default orientation is `"clockwise"`.

Example: hornetgun's fire phase offsets defined as an object:

```json
{
    "weapon_hornetgun": {
        "alt_fire": {
            "projectile": {
                "fire_phase_offsets": {
                    "start_angle": 90,
                    "count": 8,
                    "distance": 8,
                    "type": "square"
                }
            }
        }
    }
}
```

### pump_delay

Delay before playing the [pump_sound](#pump_sound).

### pump_sound

[Weapon soundscript](#weapon-soundscript) to play on pump action after firing. This is used by [weapon_shotgun]({{< ref weapon_shotgun >}}).

### pushback_force

The velocity applied to the player in the direction opposite to the fire direction. This is used by [weapon_m249]({{< ref weapon_m249 >}}) to push player backwards as they fire.

### pushback_vertical

A boolean defining whether the [pushback_force](#pushback_force) is applied vertically as well. [weapon_m249]({{< ref weapon_m249 >}}) uses it in multiplayer.

### range_modifier

The numeric value between 0 and 1 defining the bullet damage dropoff depending on the distance the target hit at.

This is a mechanic from Counter Strike. The damage is calculated by:

`base_damage` * (`range_modifier` ^ (`distance` / 500)).

The default value is 1 which means no damage dropoff. Counter Strike uses values around 0.8 for pistols and SMGs, and values close to 1 for rifles making rifles more effective at long range.

### semiauto

A boolean definding whether the player must release and press the attack button again in order to continue firing. Usually you want to provide a small [cycle_time](#cycle_time) to allow player to fire as fast as they click.

Examples: all pistols in Counter Strike, the pistol from Half-Life 2.

### shake

An object defining screen shake properties. The shake is played on each fire.

* `"duration"` - shake duration (in seconds). This is needed to be higher than 0 to enable the screen shake.
* `"frequency"` - shake frequency (between 0 and 255). Default value is 160.
* `"amplitude"` - shake amplitude (between 0 and 16). Default value is 6.

### shell

An object that defines properties related to shell ejection on bullet fire.

#### count

The number of shells dropped from a weapon per shot.

E.g. [weapon_shotgun]({{< ref weapon_shotgun >}}) ejects 2 shells on the secondary attack.

#### eject_delay

Delay between fire and shell ejection, in seconds. By default there's no delay.

{{% hint warning %}}
Currently this doesn't work propely if the shell position is determined by the weapon view model attachment.
{{% /hint %}}

#### left_side

Whether shells should be ejected to left side (by default they eject to the right side).

#### model

Path to the model (e.g. `"models/shell.mdl"`) to use as a shell model. Set to `null` to disable shell ejection.

#### model_alternating

Same as [shell_model](#shell_model) but it alternates with the base shell mode. [weapon_m249]({{< ref weapon_m249 >}}) uses it to alternate between shells and links.

#### offset

An object defining an offset relative to the player's view origin. The shell ejection position is calculated based on this offset.

* `"up"` - vertical offset. It's usually a negative value since weapons are drawn at the bottom.
* `"side"` - side offset (to the right side, unless [left_side](#left_side) is set).
* `"forward"` - forward offset (how far from the player's camera it should be).

Example:

```json
{
    "weapon_9mmhandgun": {
        "fire": {
            "shell": {
                "offset": {
                    "forward": 20,
                    "up": -12,
                    "side": 4
                }
            }
        }
    }
}
```

This also can specify the `"attachment"` property - in this case the specified attachment on the view model will be used to position the shell. This must be the value in `[1, 4]` range. The `"attachment"` can't be used together with other offset options.

Example:

```json
{
    "weapon_9mmhandgun": {
        "fire": {
            "shell": {
                "offset": {
                    "attachment": 2
                }
            }
        }
    }
}
```

#### sound_type

The sound type to play when shell drop hits something (usually the ground).

Possible values:

* `null` - no sound.
* `"shell"` - regular shell sound (this is default value).
* `"shotgun_shell"` - shotgun shell sound.

#### velocity

An object defining the shell velocity. Each property is a range to randomize the shell velocity in the certain direction:

* `"up"` - vertical velocity. The default value is `[50, 70]`.
* `"side"` - side velocity. The default value is `[100, 150]`.
* `"forward"` - forward velocity. The default value is `25` (just a number, not a range).

Example:

```json
{
    "weapon_9mmhandgun": {
        "fire": {
            "shell": {
                "left_side": true,
                "velocity": {
                    "side": [50, 70],
                    "up": [75, 175],
                    "forward": [25, 100]
                }
            }
        }
    }
}
```

### sound

[Weapon soundscript](#weapon-soundscript) to play on fire. This is played on the player's weapon channel by default.

For melee weapons it's the "miss" sound (when weapon doesn't hit anything).

### sound_additional

[Weapon soundscript](#weapon-soundscript) to play on fire. This is played on the player's item channel by default. This is used by [weapon_crossbow]({{< ref weapon_crossbow >}}) and [weapon_rpg]({{< ref weapon_rpg >}}).

### hit_body_sound

[Weapon soundscript](#weapon-soundscript) to play when melee attack hits the fleshy body.

### hit_wall_sound

[Weapon soundscript](#weapon-soundscript) to play when melee attack hits the wall.

### spread

Configure the bullet spread (this currently doesn't affect projectiles). There're several types of spreads and therefore it can be defined in different ways.

#### Universal spread with static inaccuracy

Spread is a number between 0 and 1 where 0 means no spread (perfect accuracy). In relation to the spread cone the spread value is `sin` of the apex angle of the cone (i.e. half of the "whole" cone angle). E.g. for the cone with apex angle 5 the spread is approximately 0.08716.

E.g. the default [weapon_9mmhandgun]({{< ref weapon_9mmhandgun >}}) spread is 0.01 which is around 0.6 degrees. And secondary attack has spread 0.1 which is around 6 degrees.

Spread can also be defined via string such as `"1degree"` or `"2degress"` where the number before `degree` (or `degrees`) is the *whole* angle of the cone (i.e. double apex angle). The number must be integer between 1 and 20. E.g. [weapon_9mmAR]({{< ref weapon_9mmAR >}}) has `"3degrees"` spread (in singleplayer) by default.

```json
{
    "weapon_9mmhandgun": {
        "fire": {
            "spread": 0.01
        }
    },
    "weapon_9mmAR": {
        "fire": {
            "spread": "3degrees"
        }
    }
}
```

#### Different values for vertical and lateral spread

It's possible to define different vertical and lateral spread e.g. if you want a weapon to have a smaller lateral spread. In this case the spread must be defined as object with `"vertical"` and `"lateral"` properties instead of single number.

```json
{
    "weapon_shotgun": {
        "fire": {
            "spread": {
                "vertical": 0.08716,
                "lateral": 0.04362
            }
        }
    }
}
```

#### Spread depending on the player movement

Spread can depend on whether the player is in air, moving or ducking. E.g. you may want a weapon has lower spread when player is ducking.

In this case the spread is defined as an array of objects with [player movement](#Player-movement-conditions) `"conditions"` property and `"spread"` property used when the condtions are met.

```json
{
    "weapon_m249": {
        "fire": {
            "spread": [
                {
                    "conditions": {
                        "ducking": true
                    },
                    "spread": "2degrees"
                },
                {
                    "conditions": {
                        "moving": true
                    },
                    "spread": "10degrees"
                },
                {
                    "conditions": {
                        "in_air": true
                    },
                    "spread": "15degrees"
                },
                {
                    "spread": "4degrees"
                }
            ]
        }
    }
}
```

Notes:

* Conditions are checked in the order of definition. The first matched entry is used, so the order matters.
* If no conditions are met, the last entry in the array is used even if it has conditions defined - they're ignored. So the last entry should be the one without conditions.
* The maximum number of rules is **5**.
* The conditions can be used together (e.g. if you want to check whether player is both ducking and moving).

#### Dynamic inaccuracy from Counter Strike

Featureful SDK supports Counter Strike-like dynamic inaccuracy.

The weapon starts with the base inaccuracy and then the inaccuracy changes as player fires the weapon. The spread is calculated from the current inaccuracy.

The new inaccuracy after firing a shot can be calculated in several different ways (depending on the chosen type):

* `"qubic"` (in CS it's used by rifles): (*shots_fired* * *shots_fired* * *shots_fired*) / *divisor* + *base_inaccuracy* + *inaccuracy_shift*
* `"quadratic"` (in CS it's used by some SMGs): (*shots_fired* * *shots_fired*) / *divisor* + *base_inaccuracy* + *inaccuracy_shift*
* `"recovering"` (in CS it's used by pistols): *current_inaccuracy* + (*recovery_time* - *time_elapsed_since_last_fire*) * *inaccuracy_factor*

The result is clamped between base inaccuracy and max inaccuracy.

The spread is calculated as *base* + *current_inaccuracy* * *factor*, where the *base* and *factor* should be specifically defined for a weapon.

{{% hint info %}}
While spread and inaccuracy are related, they're not the same.
{{% /hint %}}

The inaccuracy resets to its base value on weapon deploy and reload or after some not firing a weapon (currently it's not configurable).

```json
{
    "weapon_m249": {
        "fire": {
            "spread": {
                "inaccuracy": 0.2,
                "max_inaccuracy": 0.9,
                "inaccuracy_shift": 0.2,
                "type": "qubic",
                "divisor": 175,
                "spread": [
                    {
                        "conditions": {
                            "in_air": true
                        },
                        "factor": 0.5,
                        "base": 0.045
                    },
                    {
                        "conditions": {
                            "moving": 140
                        },
                        "factor": 0.095,
                        "base": 0.045
                    },
                    {
                        "factor": 0.03
                    }
                ]
            }
        }
    },
    "weapon_9mmhandgun": {
        "fire": {
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
                        "factor": 0.1,
                    }
                ]
            }
        }
    }
}
```

Properties:

* `"inaccuracy"` - base weapon inaccuracy. This is usually a value close to 0, where 0 means no inaccuracy (perfect accuracy on the first shot). This property is required.
* `"max_inaccuracy"` - as inaccuracy increases with every shot fired, it won't exceed this value. This property is required.
* `"type"` - the type of formula to use when calculating a new inaccuracy. Possible values: `"qubic"`, `"quadratic"`, `"recovering"`. If this is omitted, the `"qubic"` type is used by default, or if `"recovery_time"` is defined, the `"recovering"` type is used.
* `"inaccuracy_shift"` - inaccuracy amount added after the first shot has fired independent of the number of shots fired previously. Can be omitted or set to 0 if not needed. This is used only for `"qubic"` and `"quadratic"` inaccuracy types.
* `"divisor"` - the higher the divisor value, the less impact to inaccuracy each shot does. This is usually a value around 200. This is used only for `"qubic"` and `"quadratic"` inaccuracy types.
* `"recovery_time"` - the time in seconds to fully recover inaccuracy to the base value for `"recovering"` inaccuracy type.
* `"factor"` - how much of the time left before the full recovery (in seconds) is translated into the additional inaccuracy for `"recovering"` inaccuracy type.
* `"spread"` - the object with properties that define how the spread is calculated from the current inaccuracy. This also can be an array of objects which represent the spread rules depending on the player movement conditions.
    * `"base"` - base spread. 0 if omitted.
    * `"factor"` - how much of inaccuracy is translated to the extra spread. This property is required.
    * `"conditions"` (only if array) - an object that defines player movement conditions to check against when choosing the spread rule.

### use_secondary_ammo

Whether firing a weapon should use a secondary ammo. The only weapon that uses a secondary ammo by default is [weapon_9mmAR]({{< ref weapon_9mmAR >}}). For others you must explicitly provide `"secondary_ammo_name"`.

### weapon_volume

The volume of the fire sound as heard by AI. This is a radius in units. It also supports following string constants:

* `"quiet"` (200)
* `"normal"` (600)
* `"loud"` (1000)

Note: this has nothing to do with audio. It's used by monsters to detect player's sounds.

### weapon_flash

The amount of illumination the player gets upon firing. Higher values also mean longer illumination duration. This also supports following string constants:

* `"dim"` (128)
* `"normal"` (256)
* `"bright"` (512)

## alt_fire

Same as [fire](#fire), but it defines settings for the secondary attack or for the primary attack in the alternative mode (depending on how the weapon works).

If some setting for alternative fire is missing from default weapon configuration **and** custom configuration doesn't define it either, it reuses the value from primary attack configuration. This allows to easily change the properties for both primary and secondary attacks by editing the `"fire"` object only. On other hand this may cause the confusion if both attacks are drastically different, requiring the developer to ensure that the `"alt_fire"` doesn't inherit something it doesn't need from the `"fire"`.

```json
{
    "weapon_9mmhandgun": {
        "fire": {
            "anims": [3],
            "allow_underwater": true,
            "cycle_time": 0.3,
            "spread": 0.01
        },
        "alt_fire": {
            "cycle_time": 0.2,
            "spread": 0.1
        }
    }
}
```

In this example we define some `"fire"` properties for `"weapon_9mmhandgun"`. In `"alt_fire"` we define `"cycle_time"` and `"spread"`. The secondary attack will use `"anims"` and `"allow_underwater"` as they are defined in `"fire"` because `weapon_9mmhandgun` doesn't define these for the secondary attack.

## laser_spot

Laser spot related properties.

Note: it expects the `"secondary_attack"` to be set to `"switch_mode"`.

### start_on

A boolean defining whether the laser spot is turned on by default, in the primary mode.

### attract_rockets

A boolean defining whether laser spot attracts [rockets]({{< ref rpg_rocket >}}).

### scale

The laser spot sprite scale. 1 by default.

### activation_sound

[Weapon soundscript](#weapon-soundscript) to play on laser spot activation.

### deactivation_sound

[Weapon soundscript](#weapon-soundscript) to play on laser spot deactivation.

## manual_reload

A boolean defining whether the reload is performed by one ammo unit at a time, i.e. it plays the reload animation several times before the clip is full or no spare ammo left. This is true for [weapon_shotgun]({{< ref weapon_shotgun >}}) and [weapon_sporelauncher]({{< ref weapon_sporelauncher >}}).

This also can be an object (in which case manual reload is automatically considered to be enabled) with the following properties:

### continue_on_deploy

A boolean defining whether the weapon should continue reloading on deploy if it has been reloading before being holstered.

This is `true` by default, thus the shotgun continues its reload after re-deploy.

Example:
```json
{
    "weapon_shotgun": {
        "manual_reload": {
            "continue_on_deploy": false
        }
    }
}
```

### restart_on_deploy

A boolean defining whether the weapon reload should re-start with a start reload animation on deploy if it has been reloading before being holstered.

This is `false` by default, thus shotgun goes straight to the shell reloading after re-deploy instead of playing the start reload animation first.

This doesn't affect anything if `continue_on_deploy` is set to false. This is useful only for weapons that define the [start_reload](#start_reload) animation.

Example:
```json
{
    "weapon_shotgun": {
        "manual_reload": {
            "restart_on_deploy": true
        }
    }
}
```

## mirror_viewmodel

Whether the weapon view model must be mirrored. This is required for left-handed weapon models from Counter Strike to make them render on the right.

## player_maxspeed

Set the player's maximum speed when the weapon is deployed. This can be a positive numeric value or a string starting with `*` followed by the factor value which works as a multiplier for the base player's maxspeed (less than 1). This allows to make it feel like moving with a certain weapon equipped is heavier than with others (similar to Counter Strike).

```json
{
    "weapon_rpg": {
        "player_maxspeed": "*0.6"
    },
    "weapon_gauss": {
        "player_maxspeed": 300
    }
}
```

See also: [player's maximum speed]({{< ref "player#maximum-speed" >}}).

## player_maxspeed_alt

Same as [player_maxspeed](#player_maxspeed) but when the weapon is in alternative mode. For example, this can be used to make player slower while using a sniper scope.

```json
{
    "weapon_crossbow": {
        "player_maxspeed_alt": "*0.5"
    }
}
```

## prevent_jump

Whether the jump is disallowed while the weapon is deployed.

```json
{
    "weapon_rpg": {
        "prevent_jump": true
    }
}
```

## prioritize_primary_attack

Whether the primary attack button is prioritized when both primary and secondary fire are pressed. This is used in [weapon_minigun]({{< ref weapon_minigun >}}) to prioritize the firing over spinning.

## recharge

An object defining weapon ammo recharge options (like in [weapon_hornetgun]({{< ref weapon_hornetgun >}}) or [weapon_shockrifle]({{< ref weapon_shockrifle >}})).

{{% hint warning %}}
Currently recharging works only for weapons that don't use clips.
{{% /hint %}}

### interval

Interval in seconds between recharging.

### delay_after_fire

Delay before the weapon starts recharging after the fire has been performed. This is optional - use this in case you want a delay after fire to be longer than a general interval.

### only_when_deployed

If set to `true` it will recharge ammo only while the weapon is deployed (i.e. it's the current selected weapon). If set to `false` (default), it allows recharging in background.

### sound

[Weapon soundscript](#weapon-soundscript) to play on each ammo recharge. E.g. [weapon_shockrifle]({{< ref weapon_shockrifle >}}) uses it. This is played on the player's weapon channel by default.

## recharge_alt

Same as [recharge](#recharge), but defines options to use when weapon is in the alternative mode.

## reload

An object defining weapon reload options.

### anim

Index of reload animation.

### duration

Delay before the weapon is usable after the reload has started.

This is usually set to 0 for [manual_reload](#manual_reload) weapons so the reload can be interrupted by firing at any point.

### idle_delay

Delay before weapon can play idle animations after the reload has started.

For [manual_reload](#manual_reload) weapons it becomes a delay between ammo transfers from the inventory into the weapon clip.

### sound

[Weapon soundscript](#weapon-soundscript) to play along the reload animation. E.g. [weapon_crossbow]({{< ref weapon_crossbow >}}) defines it by default. This is not needed if the sound is played by the event in the model.

### wait_for_recoil

A boolean. Don't allow reloading until the weapon's [cycle_time](#cycle_time) has passed after the last fire. This is used by shotguns to ensure that reloading doesn't interrupt 'pump action' part of the fire animation.

### laser_suspend_time

A boolean. Hide the laser spot (if any) for this amount of time (in seconds).

## reload_empty

Same as [reload](#reload) but used when weapon clip is empty.

If some option is not defined the value from [reload](#reload) is used. Some weapons define this by default:

* [weapon_9mmhandgun]({{< ref weapon_9mmhandgun >}})
* [weapon_eagle]({{< ref weapon_eagle >}})
* [weapon_sniperrifle]({{< ref weapon_sniperrifle >}})

Set this property to `null` to remove the native weapon rules.

## alt_reload

Same as [reload](#reload) but used when weapon is in alternative mode.

If some option is not defined the value from [reload](#reload) is used.

## alt_reload_empty

Same as [reload](#reload) but used when weapon is in alternative mode and weapon clip is empty.

If some option is not defined the value from [alt_reload](#alt_reload) or [reload_empty](#reload_empty) is used (depending on which one is defined).

## start_reload

Parameters for animation to play before the actual reload animation. This is used by [weapon_shotgun]({{< ref weapon_shotgun >}}) and [weapon_sporelauncher]({{< ref weapon_sporelauncher >}}).

### anim

Animation index. Set to -1 to disable.

### duration

Delay before weapon becomes usable, i.e. can perform attacks.

## start_reload_empty

Same as [start_reload](#start_reload) but used when weapon clip is empty.

This is not used by any weapon by default.

## alt_start_reload

Same as [start_reload](#start_reload) but used when weapon is in alternative mode.

This is not used by any weapon by default.

## alt_start_reload_empty

Same as [start_reload](#start_reload) but used when weapon is in alternative mode and weapon clip is empty.

This is not used by any weapon by default.

## end_reload

Parameters for animation to play after the reload has been performed. This is used by [weapon_shotgun]({{< ref weapon_shotgun >}}), [weapon_sporelauncher]({{< ref weapon_sporelauncher >}}) and [weapon_m249]({{< ref weapon_m249 >}}).

### anim

Animation index. Set to -1 to disable.

### attack_delay

Delay before weapon can fire (in seconds). This is usually an animation duration.

It's set to 0 for [manual_reload](#manual_reload) weapons so player can fire a shotgun without waiting for the pump action to finish, but it can be to some value to enable the delay.

### idle_delay

Delay before picking an idle animation.

### sound

[Weapon soundscript](#weapon-soundscript) to play along the animation.

## end_reload_empty

Same as [end_reload](#end_reload) but used when weapon clip is empty.

This is used by [weapon_sniperrifle]({{< ref weapon_sniperrifle >}}) for bolt-action animation.

## alt_end_reload

Same as [end_reload](#end_reload) but used when weapon is in alternative mode.

This is not used by any weapon by default.

## alt_end_reload_empty

Same as [end_reload](#end_reload) but used when weapon is in alternative mode and weapon clip is empty.

This is not used by any weapon by default.

## reload_autostart

A boolean defining whether the weapon shot automatically start reloading instead of playing empty clip sound when it's out of ammo and player is trying to fire.

This is true for [weapon_shotgun]({{< ref weapon_shotgun >}}) and [weapon_sporelauncher]({{< ref weapon_sporelauncher >}}).

## secondary_attack

Defines how the secondary attack works. The available values are:

* `"alt_fire"` - when players presses the secondary attack button, the weapon performs an alternative fire . Examples:
    * [weapon_9mmhandgun]({{< ref weapon_9mmhandgun >}}) shoots faster but with large spread in alternative attack.
    * [weapon_shotgun]({{< ref weapon_shotgun >}}) shoots two shells at once in alternative attack.
* `"switch_mode"` - when player presses the secondary attack button, the weapon switches to the alternative mode. Firing is still performed via primary attack button. When player presses the secondary attack button again, the weapon switches back to the primary mode. Examples:
    * [weapon_crossbow]({{< ref weapon_crossbow >}}) zooms in/out.
    * [weapon_eagle]({{< ref weapon_eagle >}}) enables/disables the laser spot.
* `"disabled"` - the secondary attack is disabled. Examples:
    * [weapon_357]({{< ref weapon_357 >}}) doesn't have an alternative attack in singleplayer.
    * Disabling the secondary attack on [weapon_shotgun]({{< ref weapon_shotgun >}}) would remove double shot.
    * Disabling the secondary attack on [weapon_crossbow]({{< ref weapon_crossbow >}}) would disable its zooming ability.

## start_in_alt_mode

A boolean defining whether the weapon should be in the alternative mode when picked up.

This is ignored if weapon uses zoom (scope) to avoid deploying the weapon in the zoomed state.

## switch_mode

Defines properties for fire mode switching (if `"secondary_attack"` is `"switch_mode"`).

### attack_delay

Delay (in seconds) before the weapon mode can be switched again. E.g. [weapon_crossbow]({{< ref weapon_crossbow >}}) uses 1 second delay, so player can't zoom in and instantly zoom out.

### anim

Index of animation to play when starting switching the mode. If this is omitted or set to -1, no animation is performed.

### body_switch_delay

Delay before the view model switches its body from `"viewmodel_body"` to `"viewmodel_body_alt"` or back if the weapon is in alternative mode.

### mode_switch_delay

Delay before the fire mode is actually getting switched (should be used with an animation).

### end_anim

The index animation to play after the mode has been switched. This will play only if `"mode_switch_delay"` is higher than 0.

### end_anim_duration

If `"end_anim"` is defined, this is the delay before the weapon becomes usable (usually this should be the animation duration).

## switch_mode_back

Same as [switch_mode](#switch_mode), but when switching back from alterantive mode to the primary mode. If some settings are omitted, the ones from `"switch_mode"` are used.

## switch_mode_common

Common properties related to the fire mode switching.

### toggle_laser_spot

A boolean defining whether pressing the secondary attack should toggle the laser spot (the `"secondary_attack"` must be `"switch_mode"`).

## tool

An object defining tool-related properties used in [trigger_usetool]({{< ref trigger_usetool >}}).

### icon

The sprite icon to show when player is in the tool zone. This must be defined in **sprites/hud.txt**.

### trigger_delay

When tool is used in the designated area, fire the tool target with this delay. If the weapon gets holstered, the target won't be fired.

## viewmodel_body

The view model body. Should be an integer.

## viewmodel_body_alt

The view model body to use in the alternative mode.

## zoom

Zoom (scope) related properties.

Note: it expects the `"secondary_attack"` to be set to `"switch_mode"`.

### fov

The Field Of View used when zooming. The less the value the stronger the zoom. E.g. [weapon_crossbow]({{< ref weapon_crossbow >}}) uses values 20 and [weapon_357]({{< ref weapon_357 >}}) uses value 40 (in multiplayer).

### fov2

The Field Of View for the second zoom level. E.g. AWP and Scout sniper rifles in Counter Strike use it. If it's defined, pressing the secondary attack when weapon is already zoomed in, will switch weapon to the second zoom level. Pressing the button again will zoom out.

### sound

[Weapon soundscript](#weapon-soundscript) to play when zooming in.

### sound2

[Weapon soundscript](#weapon-soundscript) to play when switching to the second zoom level. If not defined it will use `"sound"`.

### unzoom_sound

[Weapon soundscript](#weapon-soundscript) to play when zooming out.

### fade

Screen fade to show on zooming in and out. Properties:

* `"color"` - the fade [color]({{< ref "JSON/#color" >}}).
* `"fade_time"` - the time to fade in (in seconds).
* `"hold_time"` - hold the screen fade for this time (in seconds).
* `"alpha"` - the fade [alpha]({{< ref "JSON/#alpha" >}}) value.

### reset_on_fire

A boolean defining whether the weapon should unscope on firing a shot.

### resume_after_reset

A boolean defining whether the weapon should zoom in again after resetting zoom via [reset_on_fire](#reset_on_fire).

### hide_viewmodel

A boolean defining whether the weapon viewmodel should be hidden when zoomed.

# Weapon soundscript

Similar to [Soundscripts]({{< ref soundscripts >}}) the weapon soundscripts allow to define properties like channel, volume, attenuation, pitch and the set of audio files to play on certain weapon events.

* Unlike regular soundscripts, weapon soundscripts don't have names and can be defined only in the context of weapon configuration.
* Some sounds are played via engine event system, thus the special treatment.
* The channel used by weapon soundscript is usually `"weapon"` or `"item"`. The others are allowed too, but not recommended.
* Unlike regular soundscripts, weapon soundscripts can define only up to 4 waves.
* It's possible to define a weapon soundscript using the array of sound paths like `["weapons/pl_gun1.wav", "weapons/pl_gun2.wav"]` instead of providing the object with `"waves"` parameter.
* Weapon soundscripts have an additional `"looped"` parameter which is a boolean defining whether the sound is looped. Currently this properly works only with [sound](#sound) and [sound_additional](#sound_additional) of [fire](#fire) and [alt_fire](#alt_fire). This is used for [weapon_minigun]({{< ref weapon_minigun >}}) spin sound loop.

# Player movement conditions

The conditions to match the current player movement state against for selecting an appropriate [spread](#spread) or [kickback](#kickback).

* `"in_air"` - whether player doesn't touch the ground (this includes being on a ladder).
* `"ducking"` - whether player is ducking.
* `"moving"` - whether player is moving, i.e. has non-zero speed. This also can be a number instead of boolean - in this case the condition is met if player's speed is higher than this value.
