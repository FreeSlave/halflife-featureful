---
title: "Warpball templates"
---

# # {{% param "title" %}}

**Warpball** is a set of visual and sound effects usually played when the alien monster spawns (gets teleported plot-wise) via the [monstermaker]({{< ref monstermaker >}}) entity. In original Half-Life such effect usually was implemented as a combination of `env_sprite`, `env_beam` and a couple of `ambient_generic` entities. Of course such approach is tedious and is prone to errors as a level designer needs to create bunch of entities for every spawn.

Featureful SDK aims at making the level designer life easier and provides ways of defining the warpball effect once. Then monstermakers can refer to the template and reproduce the effect defined by the template when they spawn a monster.

## Defining warpball templates

Featureful SDK provides two ways to define warpball templates:

1. Via the **templates/warpball.json** file (global for the mod, preferred).
2. Via the [env_warpball]({{< ref env_warpball >}}) entity placed on the map (local for the map, deprecated).

The sample mod provides quite elaborate example of the **templates/warpball.json** file which you can use in your mod without modifications or adjust for your own needs. Using this method is preferred over the `env_warpball` as it doesn't require the modifications to the map if the level designer decides to change the template parameters. Also, the definitions are global for the mod, so the level designer doesn't need to put `env_warpball` on every map (and thus is free of burden of ensuring that every `env_warpball` on all maps is up to date with a current vision) which ensures that the played effect is universal for the mod.

## Using warpball templates

To use a warpball template with a monstermaker the level designer should set a template name in the `Warpball template` parameter of the `monstermaker` entity. The template name is either from **templates/warpball.json** or the targetname of the `env_warpball` entity on the map. The templates from the file have a priority over the `env_warpball` entities with the same name.

For a quick start:

1. Copy the **templates** subdirectory from the sample mod to your mod directory.
2. Set `Warpball template` parameter of the `monstermaker` to `auto`.
3. Upon triggering the `monstermaker`, the warpball effect will be played depending on the type of the monster it spawns.

## Format of templates/warpball.json

The root document has two properties: `entity_mappings` and `templates`.

### Entity mappings

The `entity_mappings` object defines named mappings between the monster classnames and the preferred warpball templates. The mapping name acts as a template name itself.

Example:

```json
{
//...
  "entity_mappings": {
    "auto": {
      "monster_alien_slave": "xen",
      "monster_alien_grunt": "xen_big",
      "monster_headcrab": "xen_small",
      "default": "xen"
    }
  }
//...
}
```

The `monstermaker` which has `auto` as a value for the `Warpball template` parameter will choose the template depending on the monster's classname it's going to spawn. In this particular example the monstermakers that spawn alien grunts will use a `xen_big` warpball template and the monstermakers that spawn headcrabs will use a `xen_small` template. The `default` property allows to define which template should be used if the monster's classname is not in the list.

Entity mappings allow a level designer to care less about the template they want to choose for the particular monster as they can just rely on one mapping, e.g. `auto`.

The `auto` is not some hardcoded property. The user can give it a different name or define other entity mappings.

### Templates

The `templates` object defines a list of named warpball templates with all of their parameters such as sprites, sounds, dynamic light, shake, etc.

Example of the named template:

```json
{
//...
    "xen": {
      "sprite1": {
        "sprite": "sprites/fexplo1.spr",
        "alpha": 255,
        "color": "77 210 130",
        "framerate": 12,
        "scale": 1.0
      },
      "sprite2": {
        "sprite": "sprites/xflare1.spr",
        "alpha": 255,
        "color": "77 210 130",
        "framerate": 12,
        "scale": 1.0
      },
      "sound1": {
        "sound": "debris/beamstart2.wav",
        "volume": 1,
        "pitch": 100,
        "attenuation": 0.8
      },
      "sound2": {
        "sound2": "debris/beamstart7.wav",
        "volume": 1.0,
        "pitch": 100,
        "attenuation": 0.8
      },
      "beam": {
        "sprite": "sprites/lgtning.spr",
        "color": "20 243 20",
        "alpha": 220,
        "width": 30,
        "noise": 65,
        "life": "0.5,1.6"
      },
      "beam_radius": 192,
      "beam_count": "10,20",
      "light": {
        "color": "77 210 130",
        "radius": 200
      },
      "shake": null,
      "spawn_delay": 0.4
    }
//...
}
```

`sprite1` and `sprite2` define sprites to be played. Each can define:

* `sprite` - path to the sprite.
* `color` - sprite [color]({{< ref "JSON/#color" >}}).
* `alpha` - sprite [opacity]({{< ref "JSON/#alpha" >}}).
* `scale` - the sprite scale.
* `framerate` - the animation speed (frames per second).

`sound1` and `sound2` define sounds to be played. Each can define:

* `sound` - path to the sound in *sound* directory.
* `volume` - the max is 1.0.
* `pitch` - the norm is 100. [Integer range]({{< ref "JSON/#range_int" >}}).
* `attenuation` (i.e. how the sound's volume decreases depending on the distance. 0.8 means large radius, 1.25 means medium radius)

`beam` defines the parameters of beams to be played.

* `sprite` - path to the sprite used as a beam texture.
* `color` - beam [color]({{< ref "JSON/#color" >}}).
* `alpha` - beam [opacity]({{< ref "JSON/#alpha" >}}).
* `width` - how wide the beam is. The max is 255.
* `noise` - the amplitude of the beam. The max is 255.
* `life` - for how long (in seconds) the beams will render. [Range]({{< ref "JSON/#range" >}}).

`beam_radius` - the maximum length of the beams. When the effect is played beams will try to reach the surrounding walls.
`beam_count` - the number of the beams to spawn. [Integer range]({{< ref "JSON/#range_int" >}}).

`light` defines parameters for the dynamic light.

* `color` - light [color]({{< ref "JSON/#color" >}}).
* `radius` - light radius in units (default provided or inherited if not explicitly set)
* `life` - duration in seconds (default provided or inherited if not explicitly set)

Set "light" to `null` if you don't want a dynamic light effect.

`shake` defines the shake effect.

* `radius`
* `duration` (must be higher than 0 to enable the shake).
* `frequency`
* `amplitude`

Refer to [env_shake](https://twhl.info/wiki/page/env_shake) documentation to understand the meaning of these parameters.

By default the warpball effect doesn't have shake.

`ai_sound` defines the optional AI sound produced when the effect starts (e.g. to attract other monsters' attention).

* `type` - the type of AI sound. Possible values are `combat` and `danger`.
* `duration` - duration of sound.
* `radius` - the effective radius.

By default the warpball effect doesn't make AI sounds.

`sound1`, `sound2`, `sprite1`, `sprite2`, `light`, `shake` and `ai_sound` can be set to `null` to disable the particular aspect of the warpball effect inherited from the base template.

`spawn_delay` - the delay (in seconds) between the start of the warpball effect and the spawn of the monster. This is not a part of the visual or sound effects, but it's convenient to define it in the template, so all monstermakers that use the warpball template will have this delay before monster spawn.
The `monstermaker` instance can redefine the delay by providing the `Delay before spawn` parameter.

The spawn delay is an important for the gameplay: the player will have some time to react to the incoming portal before the actual monster appears. The numbers between 0.3 and 0.5 are good. While the delay is in effect the invisible hull is created to prevent other monsters and players from blocking the monster's spawn (note that other entities like doors and trains still can get in the way).

`position` - the object that defines the custom position of the warpball effect relative to the monster's origin. Currently has only one parameter:

* `vertical_shift` - the distance (in units) between the monster's origin (usually at the monster's feet) and the center of the warpball effect.

If `position` is not defined the center of the monster's body is used.

### Template inheritance

The warpball template can inherit values of another template via `inherits` property. This can save time when defining new templates.

Example:

```json
{
//...
    "xen_big": {
      "inherits": "xen",
      "sprite1": {
        "scale": 1.25
      },
      "sprite2": {
        "scale": 1.25
      },
      "beam_radius": 256,
      "shake": {
        "radius": 192,
        "duration": 1.0,
        "frequency": 160,
        "amplitude": 6
      },
      "sound1": {
        "attenuation": 0.6
      },
      "sound2": {
        "attenuation": 0.6
      },
      "position": {
        "vertical_shift": 42
      }
    }
//...
}
```

The template "xen_big" copies all parameters from "xen" tempalte and overrides the scale of the sprites, the attenuation of the sounds, the beam radius and also adds a shake effect.

Use `inherits` property to specify another template as a base.
