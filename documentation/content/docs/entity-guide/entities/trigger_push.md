---
bookHidden: true
bookToC: false
entityCategory: solid
---

# trigger_push

[TWHL](https://twhl.info/wiki/page/trigger_push)

### New parameters

* ![](/images/spirit.png) `Push speed multiplier [LR]` - factor the push speed by this (possibly dynamic) value.
* ![](/images/spirit.png) `Push direction (blank = angle) [LV]` - push direction to use instead of entity's angles.
* `Obey Trigger Mode` - make `trigger_push` properly respond to `Off` and `On` use-types. By default it always interprets them as `Toggle` (original Half-Life behavior).

### New spawnflags

* ![](/images/svencoop.png) `No clients` - don't push player.
* ![](/images/svencoop.png) `No monsters` - don't push monsters.

### Reaction to use-type

Always percieved as `Toggle` (for Half-Life compatibility) unless `Obey Trigger Mode` parameter is set to 'Yes' - in this case the entity handles `On` and `Off` use-types as it should. Changing the state makes the trigger to turn off or on.
