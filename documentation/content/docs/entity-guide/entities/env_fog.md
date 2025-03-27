---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) ![](/images/cstrike.png) env_fog

Use `env_fog` to put a global fog on the map. Set the entity name to make it toggleable by triggers.

Fog covers the whole map (including the indoor areas). There's currently no alternative to it.

You can use several `env_fog` entities on one map, but it's better to make sure that only one is active at a time.

{{% hint warning %}}
Beams may look weird in the fog (the transparent part won't render correctly). There's no fix for this. Avoid using beams in the foggy areas.
{{% /hint %}}

{{% hint info %}}

The `GL Fog type` option used to allow selecting between *Linear* and *Exponential* fog (OpenGL terms), but after the Half-Life anniversary update it doesn't work as expected anymore. Linear fog uses `Start distance` and `End distance` values and it will look for a player like a wall of color starting with a certain distance. Exponential fog relies only on `density`, which should be a number smaller than 0.01, and it's considered to be more realistic.

* By default in GoldSource the fog is exponential (exponential-squared to be precise).
* On `steam_legacy` branch the custom fog type will be applied as expected.
* On the current (anniversary) version the fog will always be exponential if the shaders are on. Otherwise the behavior is the same as on `steam_legacy`.
* To get the linear fog on the current version of GoldSource with shaders on you must copy `platform/gl_shaders` subdirectory to your mod and edit the `fs_world.frag` to always use the linear fog. Note that it will force the fog to the certain type disregarding the configured type in the `env_fog`.

{{% /hint %}}

### Spawnflags

* `Start active` - whether the fog is active at the start of the map. Unnamed fogs are always active.
* `Affect skybox` - whether the skybox is covered by fog as well.

### Reaction to use-type

Toggles the fog depending on the input use-type.
