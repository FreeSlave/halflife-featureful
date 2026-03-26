---
bookHidden: true
bookToC: false
entityCategory: solid
---

# ![](/images/spirit.png) env_mirror

A fake mirror that reflects only models. Ported from Spirit of Half-Life 1.4. To use it as a mirror one should make a reflected brush geometry behind the mirror.

Unlike most of other `env_` entities this is the *brush entity*.

It's recommended to use this entity in the confined space for better control and to avoid potential performance issues.

### Known bugs

The models with controllers may render incorrectly (e.g. the turning of the monster's head).

### Parameters

* `Radius` - the models in this proximity from the center of the `env_mirror` will be reflected (rendered on the other side).

### Spawnflags

* `Draw Player` - whether the player's model should be reflected as well.

### Reaction to use-type

The mirror can turned on and off.
