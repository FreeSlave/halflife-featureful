---
bookHidden: true
bookToC: false
---

# env_smoker

Emits sprites that fly in the specified direction (or just upwards by default) and disappear when they finish their animation, therefore the life duration of each smoke depends on the framerate and number of frames.

{{% hint info %}}
Technically the `env_smoker` existed in Half-Life, but it was an auxiliary entity used as [monster_gargantua]({{< ref monster_gargantua >}}) death effect, not intended for use by level-designers. Mappers might have seen this entity in some Counter Strike fgd files as well. In Featureful SDK `env_smoker` got proper implementation and additional parameters.
{{% /hint %}}

{{% hint info %}}
The sprites emitted by `env_smoker` occupy the temporary entity slots on the client (the maximum is 500). You can use [particle_shooter]({{< ref particle_shooter>}}) as an alternative to emit particles that don't use temporary entities.
{{% /hint %}}

### Parameters

* `Times to smoke` - the number of sprites to emit after activation. 0 means infinite number of sprites (i.e. `env_smoker` will continue making sprites until it's turned off). When runs out of smokes, the entity either removes itself or automatically deactivates (depending on `Repeatable` spawnflag).
* `Smoke Scale` - the base scale of smoke sprites. The actual is randomized between this value and this value multiplied by 1.1
* `Scale unit type` - the meaning of `Smoke Scale` value. If the `Scale unit type` is not set, the smoke scale should be specified in deciles (tenths), i.e. scale 10 means the sprite of scale 1. This is to keep compatibility with original Half-Life entity. If `Scale unit type` is set to `Normal` the scale value is percieved as usual, like in other entities.
* `Dispersion of smoke particles` - the maximum deviation from the origin position on X and Y axes to create the sprite at. This allows to make a square area where the smoke sprites appear.
* `Min delay between smokes` - the minimum delay between smokes. The delay after each smoke is randomized in bounds between this value and `Max delay between smokes`.
* `Max delay between smokes` - the maximum delay between smokes.
* `Framerate` - sprite animation base framerate. The actual framerate is a bit randomized. Note that the higher framerate means sprites will die faster. Default framerate is 11.
* `Speed` - the sprite movement speed.
* `Scale Speed` - how fast the sprite will expand or shrink (in case of negative scale speed) relative to the base scale, scale factor per second. 0 means that sprites won't change their sizes.
* `Position to smoke at [LP]` - the direction of the smoke movement is evaluated as vector between the calculated position and the initial position.
* `Smoke direction [LV]` - the direction of the smoke movement defined by a calculated vector. This option has a higher preference over `Position to smoke at`.
* `Smoke from [LP]` - the position to shoot smoke sprites from. By default the position of the entity is used.

### Spawnflags

* `Start On` - whether the `env_smoker` starts emitting smoke at the start of the map. Unnamed `env_smoker` are always active.
* `Repeatable` - whether the entity should be reusable again after it runs out of the specified amount of `Times to smoke`. When no ticked, the entity will remove itself from the map when out of smokes.
* `Directed` - whether entity should use its angles to determine the smoke emission direction. This spawnflag doesn't have any effect if `Position to smoke at` or `Smoke direction` are specified.
* `Fade sprite` - whether the render amount of sprites fades gradually along with animation. Use it with sprites which animations don't have the embedded natural fading in them.

### Reaction to use-type

Starts or stops emitting smokes depending on the input. If `env_smoker` has been previously deactivated before exhausting the `Times to smoke` value, it will emit the remaining smokes.
