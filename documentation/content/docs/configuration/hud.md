---
title: "HUD"
---

# {{% param "title" %}}

This page describes various HUD features added in Half-Life Featureful SDK.

### HUD scaling

HUD scales depending on the `hud_scale` cvar. The value 1 means default rendering (no rescaling). The value 0 means autoscaling - the effective scaling will be chosen depending on the screen resolution. 

Notes:

* Rescaling is not available in Software Mode.
* Xash3D has its own engine-side `hud_scale` cvar. If the engine provides its own cvar, the game library doesn't create one and relies on the default sprite rendering.
* HUD scaling is not the same as high resolution HUD added in Half-Life anniversary update. The former scales sprites dynamically, the latter uses large sprites and depends on new entries in **sprites/hud.txt**.

### Drawing armor icon next to health

The cvar `hud_armor_near_health` lets user to choose whether the armor should be drawn near the health like in HL 25 anniversary.

### Move mode

An icon that shows the current movement state of player like in PS2 version of Half-Life.
In order to enable this icon:
* Set `movemode.enabled_by_default` to true in **features/featureful_client.cfg** or set `movemode.configurable` to true. In the latter case whether the icon will be shown will depend on the value of the `hud_draw_movemode` client cvar.
* Add `mode_stand`, `mode_run`, `mode_crouch` and `mode_jump` sprite definitions in **sprites/hud.txt**. These should refer to the sprite file in your mod and describe the dimensions.

Example (considering the sprite is at **sprites/hud_mode.spr**)
```
mode_stand      640 hud_mode    0       0       32      32
mode_run        640 hud_mode    32      0       32      32
mode_crouch     640 hud_mode    0       32      32      32
mode_jump       640 hud_mode    32      32      32      32
```
Don't forget to increase the value at the beginning of the file by 4! (because we added 4 lines)

### Configuring HUD color

Default HUD color is set by `hud_color` feature in **features/featureful_client.cfg**. Default HUD color when the player is low on health is set via `hud_color_critical` feature.

There's also a client command `hud_color` available for a user to customize their HUD color (but only when `hud_color.configurable` is set to `true`).
E.g. `hud_color 0 160 0` sets the HUD color to green.
`hud_color default` resets the color to the default value.

### HUD minimum alpha

Upon changing, health, armor and ammo numbers in HUD temporarily brighten up and then gradually decrease their alpha value (opaqueness). This value can be configured by the `hud_min_alpha` cvar. The value is clamped between 100 and 200.

### Weapons HUD

Like in base Half-Life the HUD elements for weapons are defined in **sprites/weapon_name.txt** files (where the `weapon_name` is replaced with the actual weapon classname, e.g. **sprites/weapon_9mmAR.txt**, etc.). Featureful SDK introduces some new entries:

* `crosshair2` - a crosshair to use when the weapon is in alt mode (for weapons which [secondary attack]({{< ref "weapon-templates/#secondary_attack" >}}) is `switch_mode`). Note: `zoom` entry is still preferred for weapons with a scope.
* `autoaim2` - an autoaim crosshair to use when the weapon is in alt mode. Note: `zoom_autoaim` entry is still preferred for weapons with a scope.
* `mode` - an icon to draw in the bottom right corner (near the ammo if the weapon uses ammo) when the weapon is in primary mode. Unlike other sprites that are rendered with a HUD color in additive mode, this one is rendered in its original color and with respect to the sprite type (i.e.  sprites with transparency are supported). To make it depend on the current HUD color, enable `weaponmode_uses_hud_color` in **features/featureful_client.cfg**.
* `mode2` - same as `mode` but for alt mode.

{{% hint info %}}
When adding new entries to the weapon's hud txt file, don't forget to increase the number in the beginning to update the number of entries to read.
{{% /hint %}}

### Speedometer

The speedometer can be useful for debugging and for speedrunning.

* `hud_speedometer` - shows the player speed (this doesn't include the vertical velocity). Value 2 also shows the [player's maximum speed]({{< ref "player/#maximum-speed" >}}).
* `hud_speedometer_below_cross` - shows the speedometer under the crosshair.

### Combat text

Combat text is damage numbers that appear when the player deals damage to something, similar to how it's implemented in some other games. This can be used for debugging.

* `hud_combattext` - whether to show a combat text.
* `hud_combattext_time` - how long the combat text is shown.
* `hud_combattext_speed` - floating speed.
