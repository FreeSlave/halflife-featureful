---
bookHidden: true
bookToC: false
entityCategory: solid
---

# trigger_hurt

[TWHL](https://twhl.info/wiki/page/trigger_hurt)

### New parameters

* `Gibbing policy` - the gibbing behavior.
* `Untouchable entity name` - don't hurt targets with this classname or targetname.
* `Minimum health threshold` - don't decrease the toucher health beyond this value.

### New spawnflags

* ![](/images/svencoop.png) `Affect non-moving NPC's` - use entity's bounding box to find and hurt non-moving monsters. This will produce wrong results on non-cuboid brush shapes.
* `Full dmg every 0.5 seconds`. By default `trigger_hurt` deals half of the damage every 0.5 seconds. Tick this flag to make entity deal the full damage every 0.5 seconds.
* `Ignore Armor` - ignore player's armor. Do damage directly to the health.
* `No Camera Punch` - don't punch player's view camera.
