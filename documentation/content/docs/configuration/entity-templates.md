---
title: "Entity templates"
---

# {{% param "title" %}}

Entity templates allow to define custom defaults applied to some entities (currently mostly monsters).

Consider that you want to use a second variation of a bullsquid throughout the mod, e.g. the one with a custom model, red blood, increased health and custom relationship classification. As a mapper you could set these parameters directly for each instance of the monster. But copying the chosen parameters each time you want to place such monster becomes tedious quickly. It's also prone to errors: consider the situation when you set the model, blood and health, but forgot to set classification. To make it even worse, if during the development you decide to change, for example, the health value for this kind of bullsquids, you would need to go through all your maps, find these bullsquids and set the new health value. Unacceptable!

This is where Entity templates come to the rescue. You define the template with custom parameters once and set the template name in the monster's properties (or you define the template for a monster class and it's applied to monsters of this classname automatically). Now instead of keeping an eye on the 4 parameters (in the example with bullsquid) you can stick to one (the template name). Moreover, the Entity templates allow to configure the [visuals]({{< ref visuals >}}) and [soundscripts]({{< ref soundscripts >}}) of the monster.

The Entity templates are configured via **templates/entities.json**. Each template has a name that is used by entities to refer to the template.

# Examples

Here's a list of some useful example to give you the idea of what templates are capable of.

{{% details title="Example 1: Custom bullsquids" %}}
\
Let's say you want to add two new bullsquid variations: the yellow bullsquid from Half-Life Alpha and the crocodile-like green bullsquid from earlier stages of Half-Life 2 development. You made fine models for both bullsquids and grabbed custom sounds for the crocodile-like one. You also want to use the old spit model instead of a sprite as a projectile visual for the yellow bullsquid.

Create a **templates/entities.json** with the following contents:

```json
{
    "yellowbullsquid": {
        "own_visual": {
            "model": "models/yellowbullsquid.mdl"
        },
        "visuals": {
            "Bullsquid.Spit": {
                "model": "models/spit.mdl",
                "rendermode": "normal",
                "framerate": 1.0,
                "scale": 1
            }
        }
    },
    "crocsquid": {
        "own_visual": {
            "model": "models/crocsquid.mdl"
        },
        "soundscripts": {
            "Bullsquid.Attack": {
                "waves": ["bullsquid_hl2/attack2.wav", "bullsquid_hl2/attack3.wav"]
            },
            "Bullsquid.Growl": {
                "waves": ["bullsquid_hl2/attackgrowl1.wav", "bullsquid_hl2/attackgrowl2.wav", "bullsquid_hl2/attackgrowl3.wav"]
            }
        }
    }
}
```

Here, we created two templates called `yellowbullsquid` and `crocsquid`. Each defines the custom model (the `own_model` property). The yellow bullsquid defines a custom visual for its spit projectile. The crocodile-like bullsquid defines the custom soundscripts for spit attack and bite attack sounds.

Now, in the level editor all you have to do is to put some `monster_bullchicken` and set the desired template name in the `Entity Template` parameter (either `yellowbullsquid` or `crocsquid`). In the game these bullsquids will have the custom properties read from the template. Note that you still have access to the standard Half-Life bullsquid on your maps - just leave the `Entity Template` blank.

{{% /details %}}

{{% details title="Example 2: Female NPC" %}}
\
Let's say you want to add a female civilian NPC. You could base it on [monster_civilian]({{< ref monster_civilian >}}) or [monster_scientist]({{< ref monster_scientist >}}), with custom model, sounds and speech prefix.

```json
{
    "female_civ": {
        "own_visual": {
            "model": "models/us_gal.mdl"
        },
        "soundscripts": {
            "Civilian.Pain": {
                "waves": ["colette/colette_pain0.wav", "colette/colette_pain1.wav", "colette/colette_pain2.wav", "colette/colette_pain3.wav"]
            },
            "Civilian.Die": {
                "waves": ["colette/colette_die0.wav", "colette/colette_die1.wav", "colette/colette_die2.wav"]
            }
        },
        "speech_prefix": "FEM"
    }
}
```

{{% /details %}}

{{% details title="Example 3: Friendly Vortigaunt" %}}
\
Let's say your want a friendly vortigaunt variation in the mod, with a different model and beam effects. The default classification for the template can be changed via `classify` parameter:

```json
{
    "vort_friendly": {
        "own_visual": {
            "model": "models/islavef.mdl"
        },
        "classify": "Player Ally",
        "visuals": {
            "Vortigaunt.ZapBeamColor": {
                "color": [255, 96, 180]
            },
            "Vortigaunt.ArmBeamColor": {
                "color": [128, 16, 96]
            }
        }
    }
}
```

{{% /details %}}

{{% details title="Example 4: Modified default for a classname" %}}
\
If you give the template the same name as a monster's classname (e.g. `monster_scientist`), this template will be applied automatically to all instances of this class (unless the instance specifically refers to another template).

Let's assume you want to change the scale of the gibs for [monster_babycrab]({{< ref monster_babycrab >}}) and its default appearance - e.g. remove the default translucency (if you never noticed before - babycrabs in Half-Life are not fully opaque). You need to define the `"monster_babycrab"` template:

```json
{
    "monster_babycrab": {
        "own_visual": {
            "rendermode": "normal"
        },
        "gib_visual": {
            "scale": 0.5
        }
    },
}
```

Now all babycrabs will use this template (unless the different template is set in the babycrab's properties).

{{% /details %}}

# Format of entities.json

The document is an object where each property presents a named entity template. Template names technically can be any string, but it's better to stick to some strict set of characters, like latin letters, digits and underscore symbol, to avoid potential problems with compilers, level editors and bsp.

{{% hint warning %}}
It's better to avoid names starting with `monster_` unless you create a default template for the existing monster class. Consider that you called a template `monster_bodyguard` and use it on some security guards in your mod. Then, at some point Featureful SDK gets updated and introduces a new monster called `monster_bodyguard`. The template you previously defined now automatically gets applied to the new monster class even though it wasn't your original intention.
{{% /hint %}}

## Properties

Each template entry in the document may have the following properties:

### own_visual

The [visual]({{< ref visuals >}}) definition or the name of the visual from the *templates/visuals.json* for the monster's own model. This allows to change the default model of the monsters belonging to the template, the model scale or, for example, make the monsters semi-transparent:

```json
{
    "template_name": {
        "own_visual": {
            "model": "models/custom_model.mdl",
            "rendermode": "additive",
            "alpha": 100,
            "scale": 1.5
        }
    }
}
```

### gib_visual

The [visual]({{< ref visuals >}}) definition or the name of the visual from the *templates/visuals.json* for the monster's gibs. This allows to change the default model or other render properties of the monster's gibs.

```json
{
    "template_name": {
        "gib_visual": {
            "scale": 0.5,
            "rendermode": "texture",
            "alpha": 192
        }
    },
}
```

### classify

Default relationship classification for the monsters belonging to this template. Possible values:

* `"None"` - ignored by everyone and ignore everyone.
* `"Machine"` - aggressive against players and aliens. Used by turrets and robots.
* `"Human Passive"` - player ally who's afraid of enemies. Scientists and other civilians.
* `"Human Military"` - enemy human grunts.
* `"Alien Military"` - alien slaves, alien grunts and alien controllers.
* `"Alien Passive"` - ignore everyone, but get attacked by humans.
* `"Alien Monster"` - houndeyes, ichtyosaurus, zombies.
* `"Alien Prey"` - headrabs.
* `"Alien Predator"` - bullsquids. Attack each other and hunt alien preys.
* `"Player Ally"` - ally security guards and monsters who are enemies by default but set to have a reversed relationship.
* `"Race X Predator"` - pitdrones. Attack alien predators and alien preys.
* `"Race X Shock"` - shock troopers and voltigores.
* `"Player Ally Military"` - ally opfor soldiers. Will attack human passive and player ally if `opfor_grunts_dislike_civilians` feature is set to true (the Opposing Force behavior).
* `"Blackops"` - used by blackops assassins (if `blackops_classify` feature is set to true). Act as enemies to everyone else.
* `"Snark"` - used by snarks. Will attack gargs, but ignore alien military.
* `"Gargantua"` - used by gargs. Will be attacked by snarks.

Example:

```json
{
    "template_name": {
        "classify": "Player Ally"
    }
}
```

### health

Default health amount for the monster belonging to this template. Currently only constant numbers are supported (i.e. it doesn't depend on the chosen game difficulty).

```json
{
    "template_name": {
        "health": 100
    }
}
```

### size

The object with properties `min` and `max` each is 3-element array defining the mins and maxs for the Bounding Box:

```json
{
    "template_name":
    {
        "size": {
            "mins": [-40, -40, 0],
            "maxs": [40, 40, 200]
        }
    }
}
```

Instead of the object you can also set one of the named presets:

* `"snark"` - same size as snarks: `[-4, -4, 0]` - `[4, 4, 8]`.
* `"headcrab"` - same size as headcrabs: `[-12, -12, 0]` - `[12, 12, 24]`.
* `"small"` - same size as houndeyes: `[-16, -16, 0]` - `[16, 16, 36]`. The resulting size (but not mins and maxs) also equals to the size of the crouching player.
* `"human"` - same size as humanoid characters: `[-16, -16, 0]` - `[16, 16, 72]`.
* `"large"` - same size as bullsquids and alien grunts: `[-32, -32, 0]` - `[32, 32, 64]`.

```json
{
    "template_name": {
        "size": "headcrab"
    }
}
```

### collision_box

Some monsters define the so called Object Collision Box which is different from their base size. This is required for the hitscan attacks and projectile of null size to properly hit the model. E.g. the alien grunt's Object Collision Box is higher than his defined size. Otherwise you wouldn't be able to hit his head.

Most entities has their collision box to match the size. However some monsters redefine their collision box to match the model dimensions. If you change the `size` for such monsters, you may want to change the `collision_box` as well. The list of monsters who redefine their collision box:

* monster_alien_grunt
* monster_babygarg
* monster_bigmomma
* monster_garganuta
* monster_geneworm
* monster_leech
* monster_nihilanth
* monster_robocop
* monster_tentacle

### blood

Default blood color of the monster belonging to this template. Possible values:

* `"red"` - red human blood.
* `"yellow"` - yellow alien blood.
* `"no"` - don't bleed.

```json
{
    "template_name": {
        "blood": "red"
    }
}
```

### field_of_view

Default field of view of the monster belonging to this template. A number in range `[-1, 1]` where -1 means the full view in all directions and values closer to 1 mean narrower angle. The angle at which the monster can detect enemies depends on the field of view. If the monster has a full view it can't be ambushed from the back.

```json
{
    "template_name": {
        "field_of_view": -0.3
    }
}
```

One of the predefined presets can be used instead:

* `"full"` - full view. Used by controllers and turrets.
* `"wide"` - wide view. Used by humans and vortigaunts.
* `"average"` - used by bullsquids and alien grunts.
* `"tunnel"` - used by headcrabs and houndeyes.
* `"narrow"` - narrow view. No monsters use it by default.

```json
{
    "template_name": {
        "field_of_view": "wide"
    }
}
```

### visuals

The object consisting of the entries of the [visual]({{< ref visuals >}}) replacements. The entry key must be the name of the existing visual, used by a monster (consult the monster's entity page, e.g. [monster_bullchicken]({{< ref "monster_bullchicken/#visuals" >}})). The entry value should be either the name of the replacement visual from *templates/visuals.json* or the object defining the replacement visual.

{{% details title="Example" %}}

```json
{
    "bullsquid_template": {
        "visuals": {
            "Bullsquid.Spit": {
                "model": "sprites/mommaspit.spr",
                "color": [100, 255, 0],
                "scale": 1.5
            },
            "Bullsquid.Fleck": "CustomBullsquid.Fleck"
        }
    }
}
```

In this example we redefine some properties of the `Bullsquid.Spit` visual for a monster belonging to the `bullsquid_template` template. The `Bullsquid.Fleck` is also replaced with `CustomBullsquid.Fleck` which is expected to be defined in the *templates/visuals.json*. The latter approach allows to define the visual once and reuse it in many templates. The former approach might be more convenient because it allows you to define the visual in the template file.

{{% /details %}}

### soundscripts

The object consisting of the entries of the [soundscript]({{< ref soundscripts >}}) replacements. The entry key must be the name of the existing soundscript, used by a monster (consult the monster's entity page, e.g. [monster_bullchicken]({{< ref "monster_bullchicken/#soundscripts" >}}). The entry value should be either the name of the replacement soundscript from *sound/soundscripts.json* or the object defining the replacement soundscript.

{{% details title="Example" %}}

```json
{
    "bullsquid_template": {
        "soundscripts": {
            "Bullsquid.Attack": {
                "waves": ["bullsquid_hl2/attack2.wav", "bullsquid_hl2/attack3.wav"],
                "pitch": 110
            },
            "Bullsquid.Growl": "Bullsquid_HL2.Growl"
        }
    }
}
```

In this example we redefine some properties of the `Bullsquid.Attack` soundscript for a monster belonging to the `bullsquid_template` template. The `Bullsquid.Growl` is also replaced with `Bullsquid_HL2.Growl` which is expected to be defined in the *sound/soundscripts.json*. The latter approach allows to define the soundscript once and reuse it in many templates. The former approach might be more convenient because it allows you to define the soundscript in the template file.

{{% /details %}}

### sound_replacement

The object consisting of the entries where the key is a path to the original sound and the value is a path to the custom sound which will be played instead. It's rare that you would need this. It's better to rely on soundscripts when possible.

{{% details title="Example" %}}

```json
{
    "fem_scientist": {
        "sound_replacement": {
            "scientist/sci_pain1.wav" : "colette/colette_pain0.wav",
            "scientist/sci_pain2.wav" : "colette/colette_pain1.wav",
            "scientist/sci_pain3.wav" : "colette/colette_pain2.wav",
            "scientist/sci_pain4.wav" : "colette/colette_pain3.wav",
            "scientist/sci_pain5.wav" : "colette/colette_pain0.wav",
        }
    }
}
```

{{% /details %}}

### precached_sounds

An array of additionally sounds to precache (so they can be played in game). This is useful when the model has some sound events and the entity implementation doesn't acknowledge these sounds. For example, the gonome model from Opposing Force refers to **gonome/gonome_step1.wav** and **gonome/gonome_step2.wav** in the running animations. These sounds do not exist in the game, but even if you put sounds with such names in the game resources it won't work, because they're not being precahed by the game-code. The `precached_sounds` allows to specifiy the extra sounds to precache for an entity.

```json
{
    "monster_gonome": {
        "precached_sounds": ["gonome/gonome_step1.wav", "gonome/gonome_step2.wav"],
    }
}
```

If you don't want to specify the sounds manually you can rely on `autoprecache_sounds` instead.

### autoprecache_sounds

A boolean property which, if set to `true`, makes the entity belonging this template automatially precache all sounds referred in the model by the sound events. The sound events are `1004`, `1008`, `1011`, `1012`, `1013`. See also: [Model animation events]({{< ref "model animation events" >}}).

```json
{
    "monster_gonome": {
        "autoprecache_sounds": true
    }
}
```

### precached_soundscripts

An array of additional soundscripts to precache. This is useful when the model has some soundscript events (`1014`) and the entity implementation doesn't acknowledge these soundscripts.

```json
{
    "monster_gonome": {
        "precached_soundscripts": ["Gonome.CustomSoundScript1", "Gonome.CustomSoundScript2"]
    }
}
```

If you don't want to specify the soundscripts manually you can rely on `autoprecache_soundscripts` instead.

### autoprecache_soundscripts

A boolean property which, if set to `true`, makes the entity belonging this template automatially precache all soundscripts referred in the model by the soundscript events (`1014`). See also: [Model animation events]({{< ref "model animation events" >}}).

```json
{
    "monster_gonome": {
        "autoprecache_soundscripts": true
    }
}
```

### size_for_grapple

Defines the monster interaction with barnacle grapple (`weapon_grapple`). Possible values:

* `"no"` - no interaction. Grapple can't latch on the target.
* `"small"` - grapple pulls the target to the player (e.g. headcrabs).
* `"medium"` - grapple pulls the player to the target.
* `"large"` - currently the same effect as `medium`.
* `"fixed"` - grapple pulls the player to the target and the barnacle's tongue tip stays at the point of latching. The target is expected to be unmovable.

### speech_prefix

Defines the speech prefix for the monster's sentences. E.g. if the template is applied to the scientist, and the speech prefix is `"FEM"` the scientist will replace `SC_` with `FEM_` when playing the sentences: `FEM_HELLO` instead of `SC_HELLO`, etc.

Currently this affects only limited number of monsters:

* [monster_barney]({{< ref monster_barney >}})
* [monster_barniel]({{< ref monster_barniel >}})
* [monster_civilian]({{< ref monster_civilian >}})
* [monster_cleansuit_scientist]({{< ref monster_cleansuit_scientist >}})
* [monster_drillsergeant]({{< ref monster_drillsergeant >}})
* [monster_human_grunt_ally]({{< ref monster_human_grunt_ally >}})
* [monster_human_grunt_medic]({{< ref monster_human_medic_ally >}})
* [monster_human_grunt_torch]({{< ref monster_human_torch_ally >}})
* [monster_kate]({{< ref monster_kate >}})
* [monster_otis]({{< ref monster_otis >}})
* [monster_recruit]({{< ref monster_recruit >}})
* [monster_scientist]({{< ref monster_scientist >}})

### squad_capability

Defines the squad capabilities of the monster. It has the following properties:

* `"can_recruit"` - whether the monster can recruit other monsters into squad, i.e. whether he can be a squad leader. Some monsters that are able to be part of the squad, can't form squads by themselves. Examples: vortigaunts and female assassins. Set this to `true` to allow monsters belonging to this entity template to form squads.
* `"deny_recruiting"` - whether the monster will refuse to be included in the implicit (i.e. unnamed) squads. If the monster is explicitly set to be part of squad in the level editor, he still will join the squad.
* `"allow_different_classification"` - whether the monster can form squads with monsters of different relationship classification (it still requires monsters to not be enemies). By default monsters can form squads only with monsters of the same relationship classification (e.g. human military can form squads only with human military despite having non-enemy relationship with machine classification).
* `"require_same_classname"` - whether the monster requires the other monster to be of the same classname in order to form a squad. E.g. `monster_houndeye` can form squads only with other houndeyes by default.
* `"require_same_ent_template"` - whether the monster require the other monster to be of the same entity template in order to form a squad.

Note: the default squad capabilities are different depending on the monster's class. If some property is omitted the default one for the monster class will be used.

Example:

```json
{
    "special_vort": {
        "squad_capability": {
            "can_recruit": true,
            "deny_recruiting": false,
            "allow_different_classification": false,
            "require_same_classname": true,
            "require_same_ent_template": true
        }
    }
}
```

Note: not all monsters affected by these properties. The list of affected monsters currently includes:

* All grunt-like monsters, like human grunts, male assassins, shock troopers and robogrunts.
* Houndeyes
* Alien slaves
* Alien grunts
* Gargantuas (note that they have a separate relationship classification by default, so they won't participate in squads of other aliens).
* Alien controllers
* Female assassins
* All talk monsters like security guards, scientists and opfor human grunts.
* Pitdrones
* Voltigores

### open_door_capability

Defines whether the monster can open doors. For example, alien slaves and zombies can open doors by default, while pitdrones and alien grunts can't. This property allows the change the monster's capability.

Example:

```json
{
    "special_pitdrone": {
        "open_door_capability": true
    }
}
```

### check_melee_attack1

Redefines the check parameters for the monster's primary melee attack. Monsters do some checks before deciding what type of attack (if any) they want to perform, depending on the conditions like distance to the enemy.

This object has following properties:

* `"distance"` - distance from my origin to enemy's origin, in units. Most monsters have this value equal to 64 by default.
* `"dot"` - dot product, a number in range `[0, 1]` representing the angle between monster's facing and the enemy placement relatively to the monster. The close it to 1 the narrower the possible field of attack. Most monsters have this value equal to 0.7 by default.

When setting this property you probably also want to change the distance parameter on the corresponding [trace hull attack](#trace_hull_attacks) event. In the model find the animation with `ACT_MELEE_ATTACK1` activity (you'll need a model viewer with support for activity viewing, e.g. HLAM). Then look at events - one of them with a small value should correspond to the trace hull attack. Usually the monster entity page includes the information about events, so you don't need to do the datamining.

### check_melee_attack2

Same as `check_melee_attack1` but for the monster's secondary melee attack.

### trace_hull_attacks

The mapping between animation event indices and trace hull attack redefined parameters.

Most of monster's melee attacks are done via the trace hull check - a small hull does a trace in front of the monster to hit the enemy. If something is found on the way the knockback and punchangle (for player's camera) is applied to the target.

The property keys must be stringified numbers equal to the animation event indices. Each entry can have following properties:

* `"distance"` - distance of the hull check. How much distance in front of the monster should be checked for the hit.
* `"height"` - custom height (relative to the monster's origin) to perform a hull check from. By default for most monsters it's calculated as half of the monster's height, which is good enough for a general case. This property can be a number - a height in units, or a string in the form `"*<number>"` where the `<number>` is a number multiplied by monster's height to get the attack height, e.g. `*0.6`.
* `"punchangle"` - punch angle applied to the player's camera. This is an object with following properties:
    - `"pitch"` - pitch value.
    - `"yaw"` - yaw value.
    - `"roll"`- roll value.
* `"knock"` - knockback velocity applied to the monster or player. This is an object with following properties:
    - `"forward"` - forward velocity. Use negative value to pull the hit target inwards.
    - `"right"` - right velocity. Use negative value to push to the left.
    - `"up"` - upwards velocity.
    - `"player_only"` - whether the knockback is applied to player only.
* `"damage_info"` - [damage info](#damage_info). This allows to change the damage type of the attack and other damage characteristics.
* `"spawn_blood"` - a boolean denoting whether melee attack should make the hit target bleed.
* `"hit_soundscript"` - soundscript to play if trace hull attack hit something. Must be either the name of the soundscript from **sound/soundscripts.json** or the object defining the soundscript. You should prefer replacing the monster's soundscript via [soundscripts](#soundscripts) when possible.
* `"miss_soundscript"` - soundscript to play if trace hull attack didn't hit anything. Must be either the name of the soundscript from **sound/soundscripts.json** or the object defining the soundscript. You should prefer replacing the monster's soundscript via [soundscripts](#soundscripts) when possible.

The provided parameters are getting merged with the predefined parameters in-game (depending on the monster). For example, if the attack has the forward knock by default and your definition doesn't mention it, the forward knock is still preserved. You'll need to manually set it to 0 if you don't want it.

{{% details title="Example" %}}
\
Let's say you have a zombie with long arms and you want his attacks to have larger effective distance.

Zombie has following animation events:

* `1` - right arm attack.
* `2` - left arm attack.
* `3` - attack with both arms.

All these events are tied to animations with `ACT_MELEE_ATTACK1` activity.

So, you need to increase the check distance for primary melee attack by configuring `check_melee_attack1` parameters. Then you set custom properties for the events in `trace_hull_attacks`.

```json
{
    "long_arms": {
        "check_melee_attack1": {
            "distance": 100,
            "dot": 0.6
        },
        "trace_hull_attacks": {
            "1": {
                "distance": 110,
                "punchangle": {
                    "roll": -36
                },
                "knock": {
                    "right": -150,
                    "forward": -200
                },
                "spawn_blood": true
            },
            "2": {
                "distance": 110,
                "punchangle": {
                    "roll": 36
                },
                "knock": {
                    "right": 150,
                    "forward": -200
                },
                "spawn_blood": true
            },
            "3": {
                "distance": 110,
                "knock": {
                    "forward": -200
                },
                "spawn_blood": true,
                "damage_info": {
                    "type": ["acid", "poison"],
                    "type_policy": "add"
                },
                "hit_soundscript": {
                    "waves": ["bullchicken/bc_spithit1.wav", "bullchicken/bc_spithit2.wav", "bullchicken/bc_spithit3.wav"]
                },
                "miss_soundscript": {
                    "waves": ["bullchicken/bc_acid1.wav", "bullchicken/bc_acid2.wav"]
                }
            }
        }
    }
}
```

In this example we also set the trace hull attack on event 3 to have acid and poison damage type (in addition to the default slash damage type). We also set the hit and miss soundscripts (the sound channel is automatically set to `"weapon"`).

{{% /details %}}

{{% hint info %}}
You can create your own trace hull attacks without relying on the ones that are supported in the monster's code. Events with numbers in the `[20-999]` range should be safe to use for any monster. In this case the attack won't have any defined knock punch or damage, so you'll need to provide them. You'll also probably want to provide hit and miss soundscripts to play depending on the hit result. You can add unconditionally played sounds to the animation via events (see [model animation events]({{< ref "model-animation-events" >}})) and precache them via [precached_sounds](#precached_sounds) or [autoprecache_sounds](#autoprecache_sounds).
{{% /hint %}}

## Types

### damage_info

This type describes the damage data. It consists of the following properties:

* `"damage"` - the amount of damage.
* `"type"` - [damage type](#damage_type).
* `"type_policy"` - how to apply the damage types defined in `"type"` property.
    - `"replace"` - set the types from the `"type"` property only. Remove the types defined by default or inherited from the base template.
    - `"add"` - add types to the default or inherited ones. This is default value.
* `"nonlethal"` - a boolean that marks damage as non lethal. It can do damage, but can't kill a target.
* `"ignore_armor"` - a boolean that makes damage to ignore armor (currently only players have armor).
* `"gib"` - whether damage is set to force gibbing. Possible values:
    - `"always"` - always gib.
    - `"never"` - never gib.
    - `"normal"` - gib if damage is significantly higher than the target's health at the moment of hit.

### damage_type

A single string or an array of strings describing the damage types. Supports following values:
- `"generic"`
- `"crush"`
- `"bullet"`
- `"slash"`
- `"burn"`
- `"freeze"`
- `"blast"`
- `"club"`
- `"shock"`
- `"sonic"`
- `"energybeam"`
- `"paralyze"`
- `"nervegas"`
- `"poison"`
- `"radiation"`
- `"acid"`
- `"slowburn"`
- `"slowfreeze"`
