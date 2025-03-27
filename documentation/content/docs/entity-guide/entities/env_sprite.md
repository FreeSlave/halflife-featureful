---
bookHidden: true
bookToC: false
---

# env_sprite

[TWHL](https://twhl.info/wiki/page/env_sprite)

### New parameters

* ![](/images/spirit.png) env_sprite can be attached to entity (or an attachment point).
* New parameter `Start frame` to configure the starting frame. Useful if you want to desync animations of several `env_sprite` instances located in the same area and using the same sprite (e.g. waterfall splatters). Negative values mean the random starting frame.

### New spawnflags

* ![](/images/svencoop.png) `Once + Remove` - remove sprite after playing it once. Should be used with `Play Once`.
* `Don't mess Yaw`. In Half-Life the yaw angle set in the editor for `env_sprite` becomes the roll angle in the game. This affects the directional sprites. Tick this flag to prevent this archaic behavior.
