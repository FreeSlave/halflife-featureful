---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) trigger_hurt_remote

[svenmanor](https://www.svenmanor.com/entity-guide/trigger_hurt_remote)

A point entity that can deal damage to the specified target.

When dealing damage, the activator of the `trigger_hurt_remote` acts as an attacker.

### Parameters

* `Target` - deal damage to targets of this name. Put `!activator` to deal damage back to the activator.
* `Target Classname`. If specified, all entities with the given classname will be affected. This is independent from `Target`, which means the entity might cause damage twice to the same entity if Target is class of Target Class.
* `Gibbing policy` - the gibbing behavior.
* `Delay (Only when Constant)` - delay between hurts in the `Constant` mode.
* `Minimum health threshold` - don't decrease the toucher health beyond this value.
* `Armor damage` - deal damage to armor. Works only if 'Do Armor' flag is selected. When target is out of armor, the damage is not dealt. Can be negative to give an armor instead of draining it.

### Spawnflags

* `Instant Kill` - deal amount of damage that is enough to kill the target.
* `Constant` - make use of `Delay` keyvalue and affect target entities repeatedly.
* `Start On` - `trigger_hurt_remote` is enabled at start, this only makes sense when "Constant" flag is selected.
* `Do Armor Damage` - allows to use `Armor Damage` keyvalue.
* `Ignore Armor` - ignore player's armor. Do damage directly to the health.
* `No Camera Punch` - don't punch player's view camera.
* `Do Damage to Dead` - whether to do damage to corpses and monsters who already died. Use it with `Instant Kill` to gib the monster's corpse.

### Reaction to use-type

In `Constant` mode toggles the its state according to the use-type.
In non-constant mode does damage on each fire, unless the input use-type is `Off` - then it does nothing (compatibility with Sven Co-op).
