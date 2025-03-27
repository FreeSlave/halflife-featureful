---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) motion_manager

Allows to sync a motion of one entity with another.

See [trigger_motion]({{< ref trigger_motion >}}) for one-time syncing.

### Parameters

* `Target to affect [LE]` - target entity.
* `Position (blank = no change)` - sync with this position.
* `Facing (blank = no change)` - sync with this vector.

### Spawnflags

* `Start On` - activate on start of the level.

### Reaction to use-type

* `Off` stops the motion syncing.
* `Toggle` and `On` enable the motion syncing (TODO: this needs to be revisited).
