---
bookHidden: true
bookToC: false
---

# env_streak

Generates streaks - temporary effects similar to sparks or traces from bullets.

{{% hint warning %}}
Featureful SDK 2025-03-29 was the last version where `env_streak` was directional by default (it used entity angles for direction). In latter versions `env_streak` generates streaks in random direction by default.
{{% /hint %}}

### Parameters

* `Count` - the count of streaks per activation. Each streak gets its own randomized life duration and randomized direction.
* `Speed (directional)` - base speed of streaks for directional mode.
* `Length` - the scale of streak tracer.
* `Random velocity variance` - random velocity spread in all directions. For both directional and non-directional mode. Usually you want to provide smaller values for directional streaks, otherwise it will look like random direction.
* `Tracer color index` - tracer color, from the predefined set of colors.
* `Min life` - minimum life duration of the streak.
* `Max life` - maximum life duration of the streak.
* `Particle type` - particle physics type, from the predefined set of types in the engine.
* `Position (blank = here) [LP]` - position to generate streaks from.
* `Direction (blank = random) [LV]` - the base direction to generate streaks in. This automatically implies the directional mode. If direction is defined, the spawnflag `Use angles for direction` is ignored. The calculated `LV` can be normalized or not - if it's not normalized the vector length will affect the streak velocity.

### Spawnflags

* `Remove on fire` - whether the entity should remove itself after activation.
* `Use angles for direction` - use entity angles set in the level editor as a streak base direction. This implies the directional mode.
