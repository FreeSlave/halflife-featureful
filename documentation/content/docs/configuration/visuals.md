---
title: "Visuals"
---

# {{% param "title" %}}

Originally in Half-Life the sprites, models and the parameters they're drawn with (like color, alpha value, etc.) are explicitly defined in the game code. This means that without changing the code the mod author can't, for example, change the color of alien controller's projectiles or parameters of vortigaunt's beams.

Featureful SDK introduces a concept of *visuals* which allows to modify some parameters without digging into the game code.
Entities refer to *visuals* that can be defined in the outer file - **templates/visuals.json**. Each visual has a name that entities use to refer to them. Visuals define a set of properties related to rendering like the sprite/model in use, the render mode, the color and others.

Example:

```json
{
    "Vortigaunt.ZapBeam": {
        "color": [242, 0, 213],
        "alpha": 200,
        "sprite": "sprites/xsmoke3.spr",
        "noise": 40,
        "width": 60
    },
    "Bullsquid.Spit": {
        "sprite": "sprites/e-tele1.spr",
        "scale": 0.25,
        "rendermode": "Additive",
        "alpha": 180,
        "renderfx": "Constant Glow",
        "framerate": 15.0
    }
}
```

This redefines the look of vortigaunts's zap beams and bullsquid's spit sprite. If some parameters are omitted, the default ones are used. This allows to configure only certain aspects of the visuals without a need for providing all parameters.

## Format of visuals.json

The document is an object where each property presents a visual.
Each entry in the document may have the following properties:

* `"model"` - the model or sprite used by a visual.
* `"sprite"` - a synonym for `model`.
* `"rendermode"` - a render mode. Possible values: `"Normal"`, `"Color"`, `"Texture"`, `"Glow"`, `"Solid"`, `"Additive"`. As a mapper you should be familiar with render modes.
* `"color"` - a render [color]({{< ref "JSON/#color" >}}).
* `"alpha"` - render amount (opacity/brightness).
* `"renderfx"` - a render special effect. Possible values: `"Normal"`, `"Constant Glow"` (can be used with `"Glow"` render mode), `"Distort"`, `"Hologram"`, `"Glow Shell"`.
* `"scale"` - a scale of model/sprite. 1.0 is normal scale. Can be [range]({{< ref "JSON/#range" >}}).
* `"framerate"` - the animation framerate. 0 means no animation. Animated sprites usually use 10.0 as framerate. Can be [range]({{< ref "JSON/#range" >}}).
* `"width"` - how wide the beam is. The integer number between 1 and 255. Used by beams only.
* `"noise"` - the amplitude of the beam. The integer number between 0 and 255. Used by beams only.
* `"scrollrate"` - the scroll rate of the beam. The integer number between 0 and 255. Used by beams only.
* `"life"` - the life (in seconds) of beams, dynamic/entity lights and some temporary sprites. Can be [range]({{< ref "JSON/#range" >}}).
* `"radius"` - the dynamic/entity light radius. Used by some dynamic or entity lights. Can be [integer range]({{< ref "JSON/#range_int" >}}).
* `"beamflags"` - array of beam flags. Possible values for array items:
  - `"sine"` - make the beam curve (like Gluon Gun beam).
  - `"solid"` - draw the beam texture as solid (used by ropes in *_repel* entities).
  - `"shadein"` - shade at the start.
  - `"shadeout"` - shade at the end.
* `"wave"` - type of the beam wave (circle). This is applicable only to beam visuals that are played as waves/rings, e.g. the houndeye's shockwave or Osprey's blast circle. Possible values:
  - `"cylinder"`
  - `"torus"`
  - `"disk"`

Some parameters are applied only to certain types of visuals (e.g. beam specific parameters are used by beams only). Defining these parameters has no effect if the visual doesn't use them.

## Debugging

If you're unsure about applied visual properties, you can check them by the `dump_visuals` command in the console (the `developer` mode must be enabled and some map should be loaded).

* Type `dump_visuals` to get all currently registered visuals and their properties (so e.g. if there were no Garg on a map, his visuals won't be shown).
* Provide arguments to get only the visuals you're interested in, e.g. `dump_visuals Vortigaunt.ZapBeam Vortigaunt.PowerupBeam`.
* If the provided argument ends with `.` the command will show all visuals starting with this string. E.g. `dump_visuals Vortigaunt. Bullsquid.` prints all registered visuals which names start with `Vortigaunt.` and `Bullsquid.`.

# Shared visuals

Some visuals act as a base for others. It means that through customization of the base visual you provide customized parameters for those visuals that depend on the base one. E.g **Houndeye.WaveBase** defines beam parameters (except the color) for **Houndeye.Wave1**, **Houndeye.Wave2**, **Houndeye.Wave3** and **Houndeye.Wave4** which define their own color on top of the base parameters. This is done to ease the configuration of some visuals. The developer still has the right to decide to configure parameters of each derived visual separately, e.g. provide a different sprite for each kind of houndeye's wave.

## List of entity visuals

{{% hint info %}}
All names are case-insensitive. For example **Houndeye.WaveBase** and **houndeye.wavebase** refer to the same visual.
{{% /hint %}}

Visuals used by a certain entity are listed on the page for the entity. See [entities]({{< ref "entity-guide" >}}).

### Common NPC visuals

* **NPC.Rope** - the rope sprite used by *repel* entities and [Osprey]({{< ref monster_osprey >}}) when deploying new troopers.
