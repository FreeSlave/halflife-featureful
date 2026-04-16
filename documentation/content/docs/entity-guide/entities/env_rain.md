---
bookHidden: true
bookToC: false
---

# ![](/images/cstrike.png) env_rain

Featureful SDK implements rain effect similar to one used in Counter Strike and Condition Zero (e.g. on the **de_aztec** map).

Use `env_rain` to put rain on the map. To turn off/on the rain, give `env_rain` a name and call it by some trigger.

Besides falling raindrops rain includes other particle effects:

* Splashes when raindrops hit the ground.
* Ripples when raindrops hit water.
* Windpuffs - little clouds flying near the ground.

These extra effects can be turned off in spawnflags.

The particle parameters like sprite, color, lifetime and speed can be configured in `env_rain` options. If you don't provide any value the default ones will be used. Default sprites used for particles (and must be presented in the mod resources) are:

* **sprites/effects/rain.spr** - for raindrops
* **sprites/effects/ripple.spr** - for ripples
* **sprites/gas_puff_01.spr** - for windpuffs
* **sprites/wsplash3.spr** - for splashes (doesn't need to be distributed in the mod, already present in base Half-Life).

The way how the particles are lit depends on the light mode parameter:

* `Light doesn't affect color` - surrounding light won't effect the color (this also makes particles visible even in the complete darkness).
* `Light color affects color` - the resulting particle color will be mixed with color of the surrounding light.
* `Light intensity affects color` - the resulting particle color will be scaled by the surrounding light intensity (sum of color components).

{{% hint info %}}
Rainsplashes and ripples use `Light intensity affects color` by default. Set the corresponding parameters to `Light doesn't affect color` to ensure the effects are clearly visible even in dim light.

Alternatively you can also set some high brightness value.
{{% /hint %}}

Raindrops and windpuffs are affected by a global wind. The wind changes its strength and direction randomly. There's currently no way to configure the wind (will be implemented later).

{{% hint info %}}
Use different color to create acid or blood rain.
{{% /hint %}}

See also: [func_rainvolume]({{< ref func_rainvolume >}})

### Parameters

* `Rain intensity` the maximum number of raindrops to spawn per rain update. Default is 150. The result also depends on the user's `cl_weather` value. If you customize the rain volume you should adjust this value accordingly to preserve the similar rain density.
* `Update period (seconds)` - the delay between spawns of bunches of raindrops. Default is 0.3. Decreasing this value will produce raindrops more frequently. It's not recommended putting low values here to avoid the possible lag on weaker machines.
* `Rain max distance` - maximum distance the raindrops appear in. Default is 400.
* `Precipitation min height` - minimum height above the rain origin where the raindrops spawn. Default is 100. If rain is global the origin is rougly at the player's head. Localized rain uses the entity origin.
* `Precipitation max height` - maximum height above the rain origin where the raindrops spawn. Default is 300. If rain is global the origin is rougly at the player's head. Localized rain uses the entity origin.

Various parameters available for the visual effects configuration.

### Spawnflags

* `Start Active` - whether the rain starts automatically at the start of the map. Unnamed rains are always active.
* `Allow indoor` - tick this flag to allow raindrops appear in the indoor areas tick (that will make it look like raindrops appear from the ceiling). By default raindrops appear only under the sky.
* `Raindrops not affected by wind` - raindrops movement won't be affected by the global wind.
* `Disable wind puff particles` - disable wind puff particles that float above the ground by default.
* `No rain splashes` - disable rain splashes on the ground.
* `No ripples` - disable ripples on the water.
* `Localized` - tick this flag to make rain localized in the certain area independent of player position. By default the raindrops will appear around the player giving the illusion of global rain.
* `Distance is radius` - by default `Rain max distance` means half of the square area side the raindrops will spawn in. This flag makes it treat distance as circle radius.
* `Can go through brush entities` - whether raindrops can go through the brush entities.

### Reaction to use-type

Toggles the rain depending on the input use-type.
