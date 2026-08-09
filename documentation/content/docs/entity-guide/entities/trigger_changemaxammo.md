---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) trigger_changemaxammo

[svenmanor](https://www.svenmanor.com/entity-guide/trigger_changemaxammo)

This entity, when triggered, changes maximum ammo amount of selected ammo type for a player.

### Parameters

* `Ammo name` - ammo name. See [Ammo types]({{< ref ammo-types >}}).
* `Value to use` - value to use along with `Change max ammo mode`.
* `Change Max Ammo Mode` - set up the way the new ammo limit is modified.
    - `Set` - `Value to use` becomes maximum ammo amount.
    - `Reset to default` - maximum ammo amount is reverted to its default value (`Value to use` is ignored).
    - `Add` - `Value to use` is added to the current maximum ammo amount.
    - `Subtract` - `Value to use` is subtracted from the current maximum ammo amount.

### Spawnflags

* `All player` - apply new ammo limit rules to all players (normally, in multiplayer the entity affects only the activator).
* `Remove excess ammo` - truncate the ammo amount if the current amount is higher than the new limit. By default the current ammo amount is left as is even if it exceeds the limit.

### Reaction to use-type

Indifferent to the use-type.
