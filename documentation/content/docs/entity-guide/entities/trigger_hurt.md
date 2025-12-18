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
* `Obey Trigger Mode` - make `trigger_hurt` properly respond to `Off` and `On` use-types. By default it always interprets them as `Toggle` (original Half-Life behavior).

### New spawnflags

* ![](/images/svencoop.png) `Affect non-moving NPC's` - use entity's bounding box to find and hurt non-moving monsters. This will produce wrong results on non-cuboid brush shapes.
* `Full dmg every 0.5 seconds`. By default `trigger_hurt` deals half of the damage every 0.5 seconds. Tick this flag to make entity deal the full damage every 0.5 seconds.
* `Ignore Armor` - ignore player's armor. Do damage directly to the health.
* `No Camera Punch` - don't punch player's view camera.

### Reaction to use-type

Always percieved as `Toggle` (for Half-Life compatibility) unless `Obey Trigger Mode` parameter is set to 'Yes' - in this case the entity handles `On` and `Off` use-types as it should. Changing the state makes the trigger to turn off or on.
