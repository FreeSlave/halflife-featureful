---
title: "Entity templates"
toc_start_level: 1
---

# {{% param "title" %}}

Entity templates allow to define custom defaults applied to some entities (currently mostly monsters).

Consider that you want to use a second variation of a bullsquid throughout the mod, e.g. the one with a custom model, red blood, increased health and custom relationship classification. As a mapper you could set these parameters directly for each instance of the monster. But copying the chosen parameters each time you want to place such monster becomes tedious quickly. It's also prone to errors: consider the situation when you set the model, blood and health, but forgot to set classification. To make it even worse, if during the development you decide to change, for example, the health value for this kind of bullsquids, you would need to go through all your maps, find these bullsquids and set the new health value. Unacceptable!

This is where Entity templates come to the rescue. You define the template with custom parameters once and set the template name in the monster's properties (or you define the template for a monster class and it's applied to monsters of this classname automatically). Now instead of keeping an eye on the 4 parameters (in the example with bullsquid) you can stick to one (the template name). Moreover, the Entity templates allow to configure the [visuals]({{< ref visuals >}}) and [soundscripts]({{< ref soundscripts >}}) of the monster.

The Entity templates are configured via **templates/entities.json**. Each template has a name that is used by entities to refer to the template.

{{% hint info %}}
Entity templates can become hard to maintain when they grow in size and count. Ensure you studied the [JSON]({{< ref json >}}) format and use the proper software to view and edit **.json** files.
{{% /hint %}}

{{% hint warning %}}
When editing the entity templates ensure to run the mod in the developer mod to see the errors on the screen and allow re-parsing the configuration on the save-reload or the map restart. See [Recommendations]({{< ref "developing-the-mod/#recommendations" >}}).
{{% /hint %}}

## Examples

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
        },
        "skill": {
            "islave_health": 50,
            "islave_dmg_claw": 10,
            "islave_dmg_zap": 12,
            "islave_zap_rate": 1.25,
            "islave_revival": 1
        }
    }
}
```

We also set skill values the way so friendly vortigaunts have the same stats across all difficulties.

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

### inherits

See [inheriting templates](#inheriting-templates).

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
* monster_alien_tor
* monster_apache
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

* `"red"` - red (human) blood.
* `"yellow"` - yellow (alien) blood.
* `"no"` - don't bleed.
* `"white"`
* `"grey"` or `"gray"`
* `"brown"`
* `"olive"` - dirty/tinted green
* `"greygreen"` or `"graygreen"`
* `"orange"`
* `"goldish"` - bright orange
* `"peach"`
* `"magenta"`
* `"tan"`
* `"blue"`
* `"darkblue"`
* `"bluish"` - bluish/navy

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

[Known issue](#model-sound-precaching-issue)

### autoprecache_sounds

A boolean property which, if set to `true`, makes the entity belonging this template automatially precache all sounds referred in the model by the sound events. The sound events are `1004`, `1008`, `1011`, `1012`, `1013`. See also: [Model animation events]({{< ref "model animation events" >}}).

```json
{
    "monster_gonome": {
        "autoprecache_sounds": true
    }
}
```

[Known issue](#model-sound-precaching-issue)

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

[Known issue](#model-sound-precaching-issue)

### autoprecache_soundscripts

A boolean property which, if set to `true`, makes the entity belonging this template automatially precache all soundscripts referred in the model by the soundscript events (`1014`). See also: [Model animation events]({{< ref "model animation events" >}}).

```json
{
    "monster_gonome": {
        "autoprecache_soundscripts": true
    }
}
```

[Known issue](#model-sound-precaching-issue)

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

{{% hint info %}}
The default squad capabilities are different depending on the monster's class. If some property is omitted the default one for the monster class will be used.
{{% /hint %}}

{{% details title="Example" %}}
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
{{% /details %}}

{{% hint warning %}}
Not all monsters affected by these properties. The list of affected monsters currently includes:

* All grunt-like monsters, like human grunts, heavy weapons grunts, male assassins, shock troopers and robogrunts.
* Houndeyes
* Alien slaves
* Alien grunts
* Gargantuas (note that they have a separate relationship classification by default, so they won't participate in squads of other aliens).
* Alien controllers
* Female assassins
* All talk monsters like security guards, scientists and opfor human grunts.
* Pitdrones
* Voltigores
{{% /hint %}}

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
* `"dot"` - dot product, a number in range `[0, 1]` representing the angle between monster's facing and the enemy placement relatively to the monster. The closer this value to 1 the narrower the possible field of attack. Most monsters have this value equal to 0.7 by default.

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
* `"spawn_blood"` - a boolean denoting whether melee attack should make the hit target bleed (if it can) if damage has been dealt.
* `"hit_soundscript"` - soundscript to play if trace hull attack hit something. Must be either the name of the soundscript from **sound/soundscripts.json** or the object defining the soundscript. You should prefer replacing the monster's soundscript via [soundscripts](#soundscripts) when possible.
* `"miss_soundscript"` - soundscript to play if trace hull attack didn't hit anything. Must be either the name of the soundscript from **sound/soundscripts.json** or the object defining the soundscript. You should prefer replacing the monster's soundscript via [soundscripts](#soundscripts) when possible.

{{% hint warning %}}
The distance must be large enough for the checking hull to leave the monster's geometry.

The distance value must also be in agreement with the `check_melee_attack1` and `check_melee_attack2` conditions to avoid situations when monster thinks they can hit their target but the trace hull attack doesn't propagate far enough to actually hit the target.
{{% /hint %}}

{{% hint warning %}}
The resulting height value (whether it's defined as a constant or as a multiplier for the monster's height) must be at least 18 units for the land monsters. Otherwise the hull check will stop at the ground level (as the monster's origin of the land monsters is at their feet).
{{% /hint %}}

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

### touch_attack

The object that redefines the parameters for the monster's touch attack. Currently works only for [headcrabs]({{< ref monster_headcrab >}}), [shockroaches]({{< ref monster_shockroach >}}) and [monster_panthereye]({{< ref monster_panthereye >}}).

{{% details title="Example" %}}

```json
{
    "monster_headcrab": {
        "touch_attack": {
            "damage_info": {
                "type": "poison"
            },
            "spawn_blood": true
        }
    }
}
```
{{% /details %}}

Properties:

* `"damage_info"` - [damage info](#damage_info) type. This allows to change the damage type of the attack and other damage characteristics.
* `"spawn_blood"` - a boolean denoting whether the touch attack should make the hit target bleed (if it can) if damage has been dealt.

### take_damage

The rules for how incoming damage is dealt to the entity, depending on the amount of damage, type of damage or even the type of entity who dealt the damage. This allows to make monsters immune, resistant or vulnerable to certain attack types (e.g. think of something like Gargantua being resistant to most types of damage).

`"take_damage"` is an **array** of rules. Each rule is an object that includes *conditions* and *modifier*. Conditions consist of checks against incoming damage type, attacker type, etc. If conditions are met, the modifier is applied to the incoming damage. Otherwise the next rule in the array is examined (if there're any). If none criteria are met, the damage is applied without modifications.

As the rules are checked in order of definition, the more specialized checks must go first, and more general checks can go later.

{{% details title="Example" %}}

```json
{
    "weak_against_bullets_and_player": {
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["bullet"]
                },
                "modifier": {
                    "dmg": "*2"
                }
            },
            {
                "conditions": {
                    "attacker": {
                        "classname": ["player"]
                    }
                },
                "modifier": {
                    "dmg": "+20"
                }
            }
        ]
    }
}
```

The entity of `weak_against_bullets_and_player` template will take double damage from bullets. If the damage is not a bullet type, but the attacker is player, the entity will take damage increased by 20. Otherwise, the damage will be applied as is.

{{% /details %}}

{{% hint info %}}
The take damage rules provided in the entity template completely replace the "native" rules the entity might have. So the presence of empty `"take_damage"` array cancels the native resistance the entity might have (e.g. Gargantua's resistance to certain types of damage).
{{% /hint %}}

#### conditions

An object consisting of the following properties:

* `"dmg_type"` - array or single entry of [damage type](#damage_type). The incoming damage type is matched against this set using the `"dmg_type_match"` property.
* `"dmg_type_match"` - how the incoming damage type must be matched against the `"dmg_type"`. Possible values:
    - `"one"` - at least one of the types must match. This is the default (so you can omit defining `"dmg_type_match"` entirely).
    - `"all"` - the incoming damage must include all of the types defined in `"dmg_type"`.
    - `"none"` - the incoming damage must include none of the types defined in `"dmg_type"`.
    - `"exact"` - the set of incoming damage types must be equal to set defined by `"dmg_type"`.
* `"dmg"` - a string defining the comparison to the incoming damage amount. This must start with `<=`, `>=`, `<` and `>` and followed by a number. E.g. `"<=10"`. Possible comparators:
    - `<=` - less or equal.
    - `>=` - greater or equal.
    - `<` - strictly less.
    - `>` - strictly greater.
* `"attacker"` - an attacker [entity filter](#entity_filter). If attacker passes the filter, the criteria is met. The attacker is an entity who initiated the attack. E.g. player or monster.
* `"inflictor"` - an inflictor [entity filter](#entity_filter). If inflictor passes the filter, the criteria is met. The inflictor is an entity that dealt the damage. E.g. a projectile. For hitscan and melee attacks the inflictor is the same as attacker.
* `"self"` - [entity filter](#entity_filter) for the entity itself. This allows to check for own life state, body group value, etc.
* `"attack_affinity"` - the relationship between entity and attacker. This allows to check for friendly fire or self-inflicted harm. Can be a single entry or array of entries. Entries can have following values:
    - `"enemy"` - whether either this entity or attacker see the other one as enemy. If at least one of two entities is hostile to another, the rest of checks are ignored.
    - `"friendly"` - whether this entity sees attacker as ally, or attacker sees this entity as ally.
    - `"self"` - whether this entity inflicted damage on itself (e.g. getting damage from previously thrown grenade).
    - `"neutral"` - whether both entities see each other as neutrals.
* `"gib"` - check the incoming damage for the gibbing rule (whether the monster should turn into gibs on death).
    - `"normal"` - the incoming damage follows normal gibbing rule (gib if the incoming damage in significantly higher than the current health).
    - `"always"` - the incoming damage forces gibbing.
    - `"never"` - the incoming damage is set to never gib the monster.

All properties are optional.

#### modifier

An object consisting of the following properties:

* `"dmg"` - a damage amount modifier. This must be a string starting with `=`, `*`, `+` or `-` and followed by the number or `health` string. If it's set to `health`, the current entity's health value will be used in place of damage value.
    - `=` - set the damage to the provided value. Setting `"=health"` will make an entity to take damage equal to its current health.
    - `*` - multiply the damage by the provided value. This allows to scale the incoming damage. Use values between 0 and 1 to decrease the amount of damage. E.g. setting `"*2"` will double the incoming damage and setting `"*0.5"` will halve it.
    - `+` - increase the damage by the provided value.
    - `-` - decrease the damage by the provided value. Damage can't go lower than 0. If the original damage value is less than provided one, it will be set to 0.
* `"dmg_min_threshold"` - optional numeric value which denotes that the incoming damage can't be decreased lower than this value (e.g. due to `*` or `-` damage modifiers).
* `"skip_damage"` - when set to true, makes entity completely skip taking the damage.
* `"no_blood"` - when set to true, prevents entity from spawning blood.
* `"gib"` - change the gibbing rule.
    - `"normal"` - set normal gibbing rule (gib if the incoming damage is much higher than the current health).
    - `"always"` - force gibbing.
    - `"never"` - don't gib the monster even on high damage.

All properties are optional.

### trace_attack

The rules for how incoming trace attacks affect the entity. Trace attacks are directional attacks that usually lead to displaying effects like blood or ricochet (e.g. when hitting the armor) at the place of hit. Trace attack handling happens **before** the entity takes damage.

Just like [take_damage](#take_damage), `"trace_attack"` is an array of rules. Each rule includes conditions and modifier similar to ones of `"take_damage"`. The difference is that it allows to take into account the monster's hitgroup and modify the damage accordingly and add some visual effects (e.g. ricochet).

As the rules are checked in order of definition, the more specialized checks must go first, and more general checks can go later.

{{% details title="Example" %}}

```json
{
    "armored": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": 10,
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "-20",
                    "dmg_min_threshold": 0.01,
                    "hitgroup": "head"
                },
                "threshold_effects": {
                    "ricochet": {}
                },
            },
            {
                "conditions": {
                    "hitgroup": ["chest", "stomach"],
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "*0.5"
                }
            }
        ]
    }
}
```

In this example if hitgroup 10 is hit by bullet or melee attack, the incoming damage is decreased by 20 and the perceived hitgroup is changed to head hitgroup. If damage has crossed the minimum threshold, the threshold effects are played (ricochet in this case).

If stomach or chest are hit with bullet or melee attack the incoming damage is halved.

{{% /details %}}

{{% hint info %}}
Generally there's no point in defining `"trace_attack"` rules if you're not going to check for hitgroups or add visual effects. Just use `"take_damage"` instead.
{{% /hint %}}

{{% hint info %}}
The trace attack rules provided in the entity template completely replace the "native" rules the entity might have. So the presence of empty `"trace_attack"` array cancels the native resistance and effects the entity might have (e.g. Gargantua's resistance to certain types of damage and ricochet effect).
{{% /hint %}}

#### conditions

Has same properties as conditions of [take_damage](#take_damage), but also has hitgroup related checks:

* `"hitgroup"` - a single entry or array of hitgroups to test against. Entries can be be numbers representing the hitgroup value in the model (e.g. stomach hitgroup has a number 3) or they can be strings corresponding to the standard hitgroups:
    - `"generic"` (0)
    - `"head"` (1)
    - `"chest"` (2)
    - `"stomach"` (3)
    - `"left arm"` (4)
    - `"right arm"` (5)
    - `"left leg"` (6)
    - `"right leg"` (7)
    - custom hitgroups must be referred by a number (e.g. armor hitgroup is usually implemented via hitgroup 10).
* `"invert_hitgroup_check"` - a boolean. Whether to invert the hitgroup check, i.e. instead of testing whether the hitgroup is from specified the set, test that the hitgroup is not in the set.

#### modifier

Has same properties as modifier of [take_damage](#take_damage), but also provides a way to modify the perceived hitgroup:

* `"hitgroup"` - a single entry to change the perceived hitgroup to. This is useful with custom hitgroups in order to change them into some standard one, so the hitgroup damage multiplier (e.g. for headshots) is properly applied and monster knows their last body part that took damage to play a proper death animation in case of death.

{{% hint info %}}
You can omit the modifier completely if you want just to play `effects`.
{{% /hint %}}

#### effects

An object that allows to add some visual effects at the hit location when conditions are met. It has following properties:

* `"ricochet"` - an object decribing the ricochet effect (ricochet sprite and streaks). Example: shooting at the Barney's helmet.
    - `"certain_on_new_frame"` - whether the ricochet has 100% chance of playing if entity was hit for the first time on the current frame. This is `true` by default.
    - `"chance"` - chance of playing ricochet effect, in range `[0.0, 1.0]` where 0.0 means zero chance of playing, 1.0 means 100% chance (and 0.5 means 50% chance). If `certain_on_new_frame` set to `true` and entity wasn't yet hit on the current frame the chance is ignored (i.e. the ricochet always plays). If it was already hit on the current frame or `certain_on_new_frame` set to `false`, the ricochet is playing randomdly depending on the chance. Default chance is `0.0`.
    - `"scale"` - scale of ricochet sprite. Can be [range]({{< ref "JSON/#range" >}}). The default scale is `1.0`.
    - All properties are optional. You can define an empty object to get the effect.
* `"tracer"` - an object describing the tracer effect (single tracer line) that is displayed at the direction opposite to the direction of attack. Example: shooting at the alien grunt's armor.
    - `"certain_on_new_frame"` - similar to one for `ricochet` effect, but set to `false` by default.
    - `"chance"` - same as for `ricochet` effect. The default chance is `1.0`.
    - `"variance"` - the random variance of tracer direction. The default variance is `0.3`.
    - All properties are optional. You can define an empty object to get the effect.

#### threshold_effects

Same as `effects`, but plays only if `dmg_min_threshold` has been applied. `effects` are still played independently.

### children

This property configures the monster's children. E.g. grunts supplied by the [monster_osprey]({{< ref monster_osprey >}}) or babies delivered by [monster_bigmomma]({{< ref monster_bigmomma >}}). This allows to change the children's classname in order to spawn a different monster and provide custom properties for the spawned entity.

The children can be configured in two ways: via the object and via the array (if you want to have multiple children variants).

The following example will make `monster_bigmomma` spawn grown headcrabs instead of babycrabs. This will also apply a `"headcrab_black"` template to the spawned entity.

```json
{
    "monster_bigmomma": {
        "children": {
            "classname": "monster_headcrab",
            "parameters": {
                "ent_template": "headcrab_black"
            }
        }
    }
}
```

* `"classname"` - the classname of the monster to spawn. It can be anything really, but the parent usually expects the children to be monster entities. This property is optional - if it's not set the default child classname will be used, e.g. `monster_babycrab` in case of `monster_bigmomma`.
* `"parameters"` - the object of key-value parameters like they're set in the level editor (the internal names should be used for keys and values, like if the *SmartEdit* is toggled). Examples of keys: `"ent_template"`, `"health"`, `"weapons"`. The values can be strings or numbers.

The following example demonstrates how the children array can be used to allow spawning monsters with different parameters:

```json
{
    "monster_osprey": {
        "children": [
            {
                "parameters": {
                    "weapons": 3
                },
                "chance": 0.5
            },
            {
                "parameters": {
                    "weapons": 10
                },
                "chance": 0.25
            },
            {
                "parameters": {
                    "weapons": 5
                },
                "chance": 0.25
            }
        ]
    }
}
```

This uses the array form of `"children"` property, introducing another sub-property:

* `"chance"` - the chance of spawning this particular variant of the child relatively to chances of other children. This can be any positive number. This is an optional property - the default chance is 1.

In this example:

* The classname is not explicitly set, so the Osprey uses the default one (`monster_human_grunt` or `monster_human_grunt_ally` depending on its properties).
* It sets 50% chance of spawning the human grunt with 9mmAR and handgrenade. (`weapons` value 3).
* It sets 25% chance of spawning the human grunt with shotgun and handgrendes (`weapons` value 10).
* It sets 25% chance of spawning the human grunt with 9mmAR and AR grenades (`weapons` value 5).

The following example demonstrates how the monster can spawn mixed children, i.e. children of different classnames:

```json
{
    "monster_osprey": {
        "children": [
            {
                "classname": "monster_human_grunt"
            },
            {
                "classname": "monster_hwgrunt"
            },
            {
                "classname": "monster_male_assassin"
            },
            {
                "classname": "monster_robogrunt"
            }
        ]
    }
}
```

Here it's using the array form again, with equal chances for each monster type to spawn.

The list of monsters who is capable of spawning children:

* [monster_alien_slave]({{< ref monster_alien_slave >}}) - spawns [monster_snark]({{< ref monster_snark >}}) or [monster_headcrab]({{< ref monster_headcrab >}}) (depends on the chosen weapon).
* [monster_alien_tor]({{< ref monster_alien_tor >}}) - spawns [monster_alien_grunt]({{< ref monster_alien_grunt >}}).
* [monster_bigmomma]({{< ref monster_bigmomma >}}) - spawns [monster_babycrab]({{< ref monster_babycrab >}}).
* [monster_osprey]({{< ref monster_osprey >}}) - spawns [monster_human_grunt]({{< ref monster_human_grunt >}}) or [monster_human_grunt_ally]({{<  ref monster_human_grunt_ally>}}) depending on the Grunt Type parameter.
* [monster_blkop_osprey]({{< ref monster_blkop_osprey >}}) - spawns [monster_male_assassin]({{< ref monster_male_assassin >}}).
* [monster_shocktrooper]({{< ref monster_shocktrooper >}}) - drops [monster_shockroach]({{< ref monster_shockroach >}}) on death.
* [monster_geneworm]({{< ref monster_geneworm >}}) - spawns [monster_shocktrooper]({{< ref monster_shocktrooper >}}) after taking enough damage.

### equipment_drop

An array that defines the item drop from the monster depending on monster's `weapons` value. This allows, for example, to make `monster_human_grunt` drop something else instead of [weapon_9mmAR]({{< ref weapon_9mmAR >}}) or [weapon_shotgun]({{< ref weapon_shotgun >}}).

Example:

```json
{
    "monster_human_grunt": {
        "equipment_drop": [
            {
                "weapons": 1,
                "classname": "weapon_smg"
            },
            {
                "weapons": 8,
                "classname": "weapon_shotgun"
            },
            {
                "weapons": 4,
                "classname": "ammo_ARgrenades",
                "at_position": "body"
            }
        ]
    },
    "monster_male_assassin": {
        "equipment_drop": [
            {
                "weapons": 1,
                "classname": "weapon_rifle"
            },
            {
                "weapons": 8,
                "classname": "weapon_sniperrifle"
            },
            {
                "weapons": 4,
                "classname": "ammo_ARgrenades",
                "at_position": "body"
            }
        ]
    }
}
```

This makes human grunts and male assassins drop [weapon_smg]({{< ref weapon_smg >}}) and [weapon_rifle]({{< ref weapon_rifle >}}) instead of `weapon_9mmAR`.

Currently this property affects only the following monsters:

* [monster_barney]({{< ref monster_barney >}})
* [monster_barniel]({{< ref monster_barniel >}})
* [monster_human_grunt]({{< ref monster_human_grunt >}})
* [monster_human_grunt_ally]({{< ref monster_human_grunt_ally >}})
* [monster_human_grunt_medic]({{< ref monster_human_medic_ally >}})
* [monster_human_grunt_torch]({{< ref monster_human_torch_ally >}})
* [monster_kate]({{< ref monster_kate >}})
* [monster_male_assassin]({{< ref monster_male_assassin >}})
* [monster_otis]({{< ref monster_otis >}})
* [monster_robogrunt]({{< ref monster_robogrunt >}})

Behavior details:

* Each check is independent of others.
* Defining the `equipment_drop` completely replaces the item drop rules for the monster. I.e. you can't just replace one weapon drop with another - you must define the full list.
* Even if the list doesn't contain a weapon, the monster will still change the model to non-weapon body on death.
* Setting an *empty* array (`[]`) removes the item drop, but the change to non-weapon body is still applied on monster's death.

Each array item can have the following properties:

* `"weapons"` - the value of `weapons` parameter to check against. This is usually a power of 2 number (1, 2, 4, 8, etc.), but in general you can think of it as of a bit flag. This property is optional - if it's not defined, the drop is unconditional (you can also use [loot_drop](#loot_drop) for the drop that doesn't depend on the `weapons` parameter).
* `"weapons_match"` - the type of `weapons` match. Optional. Possible values:
    - `"one"` - at least one bit must match. This is the default option.
    - `"all"` - the monster's `weapons` parameter must contain all the bits from the `weapons` property of the array item.
    - `"none"` - none of the bits must match.
    - `"exact"` - the exact equality is expected. Use this if you want to check for the `0` value.
* `"classname"` - the classname of the dropped item. This is a required property.
* `"ent_template"` - the entity template for the dropped item. Optional.
* `"at_position"` - the position to spawn the dropped item at. Optional. Possible values:
    - `"gun"` - at gun position (depends on the monster's usually an attachment on the hand). This is the default value.
    - `"body"` - at *body* position, usually somewhere between the monster's center and the head. This suits for non-weapon drops (otherwise it would look weird to drop an item from the same place as a weapon).

### loot_drop

Defines additional items dropped from monster when it dies or [func_breakable]({{< ref func_breakable >}}) when it gets destroyed.

{{% hint info %}}
Loot drop doesn't interfere with weapons and items the monster drops by default (e.g. `monster_human_grunt` dropping his weapon). Loot drops are extra items. To modify the 'native' drop use [equipment_drop](#equipment_drop).
{{% /hint %}}

Loot can be defined in 2 forms: as an array and as an object.

Array example:

```json
{
    "monster_zombie": {
        "loot_drop": [
            {
                "classname": "ammo_9mmAR",
                "chance": 0.3
            },
            {
                "classname": "item_healthkit",
                "ent_template": "custom_healthkit",
            },
            {
                "classname": "item_pickup",
                "pickup_name": "battery_blue",
                "chance": 0.5
            }
        ]
    }
}
```

This defines 3 items which may drop from zombie upon its death. Each entry can have following properties:

* `"classname"` - the entity classname to spawn.
* `"ent_template"` - the entity template name to apply to the spawned item (e.g. if you want items with custom models to be dropped).
* `"pickup_name"` - the [Player Inventory]({{< ref player-inventory >}}) item name. This applied only if the classname is [item_pickup]({{< ref item_pickup >}}).
* `"chance"` - the chance of drop, the number between 0 and 1, where 1 equals 100% chance of drop (this is a default value).
* `"weight"` - if maximum weight is defined (see below), whether the item will drop depends on the weight of other items. Default weight is 1.

Object example:

```json
{
    "monster_zombie": {
        "loot_drop": {
            "items": [
                {
                    "classname": "ammo_9mmAR",
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
                }
            ],
            "max_weight": 2
        } 
    }
}
```

This is similar to array definition, but the item list goes into the `"items"` property. When using the object definition, it becomes possible to define the `"max_weight"` value. The weight sum of dropped items can't exceed the `"max_weight"` value. The items to spawn are chosen in random order from the list until adding the next item would exceed the limit. In this example:

* Monster can drop `ammo_9mmAR` and `item_healthkit` at the same time because the sum of their weights doesn't exceed the maximum.
* Monster can drop `item_pickup` and `item_healthkit` at the same time because the sum of their weights doesn't exceed the maximum.
* Monster can't drop `ammo_9mmAR` and `item_pickup` at the same time because the sum of their weights exceeds the maximum.
* Monster can't drop all 3 items at the same time.

Thus the weights allow to balance how much "goodies" can be dropped from the monster. The empty string (`""`) can be provided as a classname if you want the "empty drop" to have a weight.

The `"chance"` property still can be applied for more randomization.

{{% hint info %}}
If `"max_weight"` is 0 (default) or the list consists of 1 item only, the weight limit is not applied.
{{% /hint %}}

### pain

Monsters have different rules for playing the pain sounds. Some set a delay before playing the pain sound again (e.g. [alien grunts]({{< ref monster_alien_grunt >}})). Some play it by random chance (e.g. [zombies]({{< ref monster_zombie >}})). This object allows to configure the pain sound behavior via the following properties:

* `"delay"` - the minimum interval (in seconds) between pain sounds. Useful to prevent sound spamming when monster is being hit frequently. Can be [range]({{< ref "JSON/#range" >}}).
* `"chance"` - the chance of playing the pain sound when getting damage. Can be used as an alternative to `"delay"` in order to prevent sound spamming. `1.0` means 100% chance. Must be a number larger than `0.0`.
* `"lower_bound_dmg"` - the monster will play pain sound only if damage is strictly higher than this value. This is `0` for most monsters.
* `"allow_when_dying"` - monster is allowed to play pain sound when it's in dying animation.

{{% hint info %}}
If some property is omitted the default one for the monster class will be used.
{{% /hint %}}

{{% details title="Example" %}}
```json
{
    "monster_bullchicken": {
        "pain": {
            "delay": 0.2,
            "chance": 0.5,
            "lower_bound_dmg": 8,
            "allow_when_dying": true
        }
    }
}
```
{{% /details %}}

### skill

An object that allows to override the skill values for the entity template. See also: [Skill variables]({{< ref "skill-variables" >}}).

```json
{
    "custom_vort": {
        "skill": {
            "islave_health": "barney_health",
            "islave_zap_rate": "*1.5",
            "islave_dmg_zap": 40,
            "islave_dmg_claw": [10, 15, 20]
        }
    }
}
```

Each object property represents an override. The keys are the difficulty-independent names of skill variables you want to override (with or without *sk_* prefix). The values can come in various forms:

* As a string - the name of another skill variable (with or without *sk_* prefix). If the replacement variable doesn't exist, the warning is printed in the console and the original is used.
* As a string starting with `*` followed by a number - the number works as a multiplier for the original skill value.
* As a number - the same value will be used on all difficulties.
* As an array of three numbers - values for easy, medium and hard difficulties.

{{% hint info %}}
Skill values are replaced as whole - it's not possible to replace skill value for a specific difficulty only.
{{% /hint %}}

{{% hint warning %}}
Using multiplier replacements (like `"*1.5"`) don't work with skill variables that fallback to other skill variables values.
{{% /hint %}}

### displayname

Allows to change a default [display name]({{< ref displaynames >}}) for the entity.

```json
{
    "monster_alien_slave": {
        "displayname": "Vortigaunt"
    }
}
```

### pickup

An object that defines pickup-related properties.

#### hud_sprite

The name of the custom HUD sprite to show when the item is picked up. By default the item classname (e.g. `item_battery`) is used as a HUD sprite name.

This property affects only the following items:

* [item_battery]({{< ref item_battery >}})
* [item_healthkit]({{< ref item_healthkit >}})
* [item_longjump]({{< ref item_longjump >}})
* [item_antidote]({{< ref item_antidote >}})
* [item_security]({{< ref item_security >}})
* [item_flashlight]({{< ref item_flashlight >}})
* [item_nvgs]({{< ref item_nvgs >}})
* [item_helmet]({{< ref item_helmet >}})
* [item_armorvest]({{< ref item_armorvest >}})

```json
{
    "custom_healthkit": {
        "pickup": {
            "hud_sprite": "my_healthkit"
        }
    },
    "custom_battery": {
        "pickup": {
            "hud_sprite": "my_battery"
        }
    },
}
```

### projectile

An object that defines projectile-related properties.

#### effects_flags

An array of pre-defined effect-flags recognized by the engine. Possible item values:

* `"rocketflare"` - a rocket flare effect (used by [rpg_rocket]({{< ref rpg_rocket >}}) and [hvr_rocket]({{< ref hvr_rocket >}}) by default). This produces a white sprite of randomized scale attached to the projectile as well as the small dynamic light.
* `"brightlight"` - a bright light effect - a large dynamic light attached to the projectile.

Set the empty array to override the default effect flags (e.g. to remove the rocket flare from the `rpg_rocket`).

```json
{
    "rpg_rocket": {
        "projectile": {
            "effect_flags": []
        }
    },
    "crossbow_bolt": {
        "projectile": {
            "effect_flags": ["rocketflare"]
        }
    }
}
```

## Inheriting templates

Entity templates can be derived from another entity template. Let's say you defined a custom template for a vortigaunt (`monster_alien_slave`), with different visuals, for example. And now you want to define more templates for vortigaunts with the same custom visuals and some additional changes (e.g. a different model or even more custom visuals). Without inheritance you would need to copy the defined properties into the new template and then extend the template with new properties. This is far from ideal, as in case you wanted to change some property value, you would have to go through all the templates and change the value in each instance. This is where the template inheritance comes in handy.

The inheritance is done via `"inherits"` property with value set to the template name which is used as a base. The derived entity template inherits everything from the base allowing to extend upon it. The derived template can be used as a base for another template (e.g. A -> B -> C), but loops (A -> B -> A) are prohibited.

Example:

```json
{
    "monster_alien_slave": {
        "visuals": {
            "Vortigaunt.ZapBeamColor": {
                "color": [255, 96, 180]
            }
        }
    },
    "custom_vort": {
        "inherits": "monster_alien_slave",
        "own_visual": {
            "model": "models/custom_vort.mdl"
        }
    },
    "custom_vort2": {
        "inherits": "custom_vort",
        "visuals": {
            "Vortigaunt.ArmBeamColor": {
                "color": [128, 16, 96]
            }
        }
    }
}
```

In this example the template `custom_vort2` inherits both `own_visual` from `custom_vort` and `visuals` from the `monster_alien_slave`, extending the latter with its own visuals.

{{% hint info %}}
Some properties can't be extended via the entity template inheritance. E.g. [take_damage](#take_damage) and [trace_attack](#trace_attack) are either inherited as is or they are getting replaced completely if they are defined in the derived template.
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
* `"no_blood"` - if set to `true`, this damage can't make the target bleed.
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

### entity_filter

Set of conditions to filter entities by. When matching against the filter, some entity acts as an *initiator*, which allows to compare some parameters of the entity that is being filtered to the parameters of the initiator. E.g. `"attacker"` and `"inflictor"` filters in [take_damage](#take_damage) and [trace_attack](#trace_attack) have the entity that is taking damage as an initiator.

* `"classname"` - a single string or array of strings to match the entity's classname against. Special value `"same"` means "same classname as initiator". E.g. `["same", "monster_zombie"]` means 'the classname of the filtered entity must be either the same as initiator's classname or equals to `monster_zombie`'.
* `"ent_template"` - a single string or array of strings to match the entity's template against. Special value `"same"` means "same entity template as initiator". Use `""` (empty string) if you want to check for entities without an entity template.
* `"classify"` - a single string or array of strings to match the entity's relationship classification against. Special value `"same"` means "same classification as initiator". See [classify](#classify) for the possible values.
* `"is_combat_character"` - a boolean. Whether the entity is capable of participating in classification relationships, i.e. a monster or a player.
* `"life_state"` - a life state of the filtered entity. Can be a single entry or an array of entries.
    - `"alive"` - entity is alive.
    - `"dying"` - entity is dying (e.g. monster in the death animation).
    - `"dead"` - entity is dead (e.g. monster's corpse lying on ground after the death animation is over).
    - Combination `["alive", "dying"]` means still alive or currently dying.
    - Combination `["dying", "dead"]` means 'not alive'.
* `"body_filter"` - an array of entity body values to test for. Each item can be either integer or an object. If it's an integer the filter checks if the current entity's absolute body value equals to this value. If it's the object it can have the following properties:
    - `"bodygroup"` - the body group number to check. E.g. human grunts use bodygroup 1 as a head group.
    - `"submodel"` - the current value of the specified body group.
    - Both properties are required.
* `"negate"` - a boolean. Whether to change the filter result to opposite (a logical *Not* applied to the filter result).

All properties are optional. All checks are joined by logical *And* to evaluate the filter result.

## Known issues

### Model sound precaching issue

{{% hint warning %}}
There might be problems with precaching sounds from models for entities (mainly monsters) coming from the [monstermaker]({{< ref monstermaker >}}). Until the issue is resolved you should ensure the map has at least one spawned monster of the same entity template and model as long as such monster can spawn from the monstermaker.
{{% /hint %}}
