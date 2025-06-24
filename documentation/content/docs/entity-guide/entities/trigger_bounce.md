---
bookHidden: true
bookToC: false
entityCategory: solid
---

# ![](/images/spirit.png) trigger_bounce

A brush trigger that bounces away the entity that moves towards it (by default it affects only players). The angle set in the level editor defines the direction of bounce. The entity can be locked by master.

### Parameters

* `Minimum speed` - entity must move with this speed towards the trigger to be affected by bounce. The `Target` will be fired only in this case.
* `Factor` - bounce factor, where 1 means a perfect bounce. The values lower than 1 mean the entity will be bounced off with some loss of energy. The values higher than 1 mean the entity will be bounced off with higher speed than it had before bounce.

### Spawnflags

* `Truncate speed` - truncate the bounce speed to the specified `Minimum speed`.

### Reaction to use-type

Currently can't be turned off and on. Use master instead.
