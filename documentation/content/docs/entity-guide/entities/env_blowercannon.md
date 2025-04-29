---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) env_blowercannon

Shoots various projectiles, e.g. spores or shockbeams.

The fire direction is evaluated as following:

1. If `Direction [LV]` is specified, calculate it from locus.
2. If `Target to shoot at [LE]` is specified, calculate the direction between target and `env_blowercannon` itself.
3. Otherwise, the angles of the entity are used to calculate the direction.

### Parameters

* `Projectile owner [LE]` - don't collide with this entity.
* `Weapon Type` - the projectile type.

### Spawnflags

* `Don't delay first fire`. In Opposing Force the first shot in `Toggle On/Off` mode and every shot in `Fire When Triggered` is delayed by the specified `Delay`. Tick this flag to avoid delay on the first shot.

### Reaction to use-type

The use-type is ignored. The entity is always getting toggled.

### See also

* [displacer_ball]({{< ref displacer_ball >}})
* [shock_beam]({{< ref shock_beam >}})
* [spore]({{< ref spore >}})
* [squidspit]({{< ref squidspit >}})
