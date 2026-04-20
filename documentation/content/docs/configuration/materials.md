---
title: "Materials"
---

# {{% param "title" %}}

Featureful SDK uses the same materials system as in Half-Life: the mapping between the material type and textures is defined in the `sound/materials.txt`. But there're few changes on top of that:

* The maximum number of materials has been increased from 512 to 1024.
* The mod developer can define their own material types and change the parameters of the predefined material types.
* The snow material type is included as one of the predefined materials.
* The console command `dump_materials` prints all material types and its parameters (`developer` mode must be enabled).

See also: [TWHL article on materials.txt](https://twhl.info/wiki/page/materials.txt)

## Snow material

Snow material is denoted either by **N** (Counter Strike compatible ![](/images/cstrike.png)) or **O** (Opposing Force compatible ![](/images/opfor.png)) character.

The following step sounds must be provided if your mod uses snow material:

* **player/pl_snow1.wav**
* **player/pl_snow2.wav**
* **player/pl_snow3.wav**
* **player/pl_snow4.wav**

You can take step sounds either from Counter Strike or from Opposing Force.

Note: technically **N** and **O** are distinct materials with the equal predefined properties. They can be configured separately.

## Configuring material types

The mod developer can add new material types or change the parameters of predefined materials by editing the **features/materials.json**. This file is completely optional - the predefined material types existing in Half-Life (plus snow material) are included regardless.

Example:

```json
{
    "materials": {
        "L": {
            "step": {
                "right": ["player/pl_gravel1.wav", "player/pl_gravel3.wav"],
                "left": ["player/pl_gravel2.wav", "player/pl_gravel4.wav"],
                "walking": {
                    "time": 700,
                    "volume": 0.5
                },
                "running": {
                    "time": 350,
                    "volume": 0.7
                }
            },
            "hit": {
                "waves": ["debris/concrete1.wav", "debris/concrete2.wav", "debris/concrete3.wav"],
                "volume_bar": 0.1,
                "volume": 0.7,
                "attenuation": 1.25,
                "wallpuff_color": [90, 80, 70],
                "impact_particle_color": [90, 80, 70]
            }
        }
    },
    "wallpuff_color": [100, 100, 100],
    "wallpuff_alpha": [200, 250],
    "impact_particle_color": [90, 90, 90]
}
```

In this example the new material **L** is defined. To use it in the mod the developer should add entries that refer to this material type in **sound/materials.txt**.

## Format of materials.json

The root object can have the following properties:

* `materials` - the object that defines the named entries describing the material types, either new ones or providing custom parameters for predefined ones.
* `ladder_step` - parameters to use when the player is on the ladder. It's not really a material type so it's configured separately.
* `wade_step` - parameters to use when the player is at least knee-deep in the water. It's not really a material type so it's configured separately.
* `default_material` - the name of the default material type which is used when the material type for the texture is not defined. By default it's `C` (concrete) and indeed you won't find any material of this type in the Half-Life's **sound/materials.txt** because it's already implied. This option allows to change which material should be used as default one.
* `default_step_material` - the name of the material type to use for footstep sounds if the other material doesn't define its own footstep sounds (e.g. wood and glass materials don't define the special footstep sounds in Half-Life). If not defined, the `default_material` will be used.
* `slosh_material` - the name of the material type to use when player is feet-deep in the water. By default it's **S**.
* `flesh_material` - the name of the material type to use as a *flesh* material (melee weapons depend on that). By default it's **F**.
* `wallpuff_color` - the default wallpuff [color]({{< ref "json/#color" >}}).
    - Default value is `[40, 40, 40]`.
    - This color is applied to all materials unless the material defines its own wallpuff color.
    - The only materials that set their own color by default are concrete (`C`) and wood (`W`). If custom `wallpuff_color` is set, these materials will adjust their default colors accordingly, but you may still want to set a custom color specifically for a material, especially for wood, since it may get weird color when adjusted.
    - The sprites with `SPR_INDEXALPHA` texture format usually require setting a custom wallpuff color, as the default one would be too dark for them.
* `wallpuff_alpha` - the [range]({{< ref "json/#range_int" >}}) of wallpuff alpha value. The default value is `[120, 180]`. The higher the values the brighter the wallpuffs.
* `impact_particle_color` - the default color of impact particles (usually from bullet impact) for all materials that don't define its own color. Note that the resulting color may not match exactly the one you provided as native engine particles support colors only from the [internal palette]({{< ref color-palette >}}). Default value is almost pitch-black.

### materials

The property names are material type names that must be 1 character long. This can be the name of the predefined material type (if you want to change its parameters) or the name of the new material type.

Each entry in `materials` can have two properties: `step` and `hit`. The `step` defines footstep related parameters used when player walks on the texture of this material type. The `hit` defines how the material reacts to the bullet or melee weapon hits.

#### step

The `step` entry has the following properties:

* `right` and `left` - the arrays of paths to sounds to play when the player takes a step with the right or left foot. The maximum number of sounds in each array is 5. Both `left` and `right` should have at least one sound defined for the material to have its own footstep sounds.
* `walking` and `running` define various parameters to use depending on whether the player is walking or running.

Each `walking` and `running` entries can have the following parameters:

* `volume` - the number between 0.0 and 1.0 to define a footstep sound volume. By default it's 0.2 for walking and 0.5 for running.
* `time` - the delay in milliseconds between foostep sounds. By default it's 400 for walking and 300 for running.

If you define a new material type, you can omit both `walking` and `running` entries to rely on default values.

#### hit

The `hit` entry has the following properties:

* `waves` - the array of paths to sounds to play as a hit sound. The maximum count is 5.
* `volume` - the number between 0.0 and 1.0 that defines a hit sound volume. By default it's 0.9.
* `volume_bar` - the number between 0.0 and 1.0 that defines a the sound volume of the melee weapon (e.g. a crowbar) when it's hitting this material. The melee weapons play its own sounds along with the texture sound. By default it's 0.5.
* `attenuation` - attenuation of the hit sound. By default it's 0.8 (normal attenuation).
* `allow_wallpuff` - a boolean value defining whether the wallpuffs (`cl_weapon_wallpuff`) are allowed to spawn when hitting this material. `true` by default.
* `allow_weapon_sparks` - a boolean value defining whether the bullet impact streaks/sparks are allowed to spawn when hitting this material. `true` by default. Out of the predefined material the wood (**W**) has set it to `false`.
* `play_sparks` - a boolean value defining whether the spark effect and sound should sometimes play when the material is hit. `false` by default. Out of the predefined materials the computer (**P**) has set it to `true`, so when you hit the computer screen the spark is produced sometimes.
* `wallpuff_color` - the custom color of the wallpuff produced.
* `impact_particle_color` - the color of impact particles (usually from bullet impact). Same as top-level property, but for a certain material.

### ladder_step and wade_step

Both `ladder_step` and `wade_step` have the same format as `step` entry of the material.

## Wallpuffs

Wallpuffs are temporary sprites created on bullet impact on the world geometry, similar to Counter Strike.

This effect is disabled by default, but the user can enable it via the `cl_weapon_wallpuff` cvar or the developer can force it via the `weapon_wallpuff` feature in **features/featureful_client.cfg**.

The sprites are configued in **features/featureful_client.cfg**.

Wallpuffs can be different colors depending on the type of material that was hit.
