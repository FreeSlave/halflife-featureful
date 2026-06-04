---
bookHidden: true
bookToC: false
entityCategory: solid
---

# momentary_door

[TWHL](https://twhl.info/wiki/page/momentary_door)

A door that should be opened by [momentary_rot_button]({{< ref momentary_rot_button >}}).

### Bugfixes

* Fixed the move sound not stopping sometimes.

### New parameters

* `Sound Radius` - sound radius for move and stop sounds.
* `Collision with corpses` - allows to ignore collision with dead monsters. Corpses won't block this door and won't get crushed.
* `Instantly gib corpses` - instantly gib corpses that block the door, even if door deals 0 damage.
* `How to handle tiny creatures` - override handling of [tiny creatures]({{< ref "monsters/#tiny-creatures" >}}).
* `Fire On Start` - fire when the door starts moving (either direction).
* `Fire On Start Trigger State` - use-type for `Fire On Start` target.
* `Fire On Stop` - fire when door stops moving (even if it doesn't end up in the terminal position).
* `Fire On Stop Trigger State` - use-type for `Fire On Stop` target.
* `Fire On Open Start` - fire when the door starts moving towards opened state.
* `Fire On Open Start Trigger State` - use-type for `Fire On Open Start` target.
* `Fire On Close Start` - fire when the door starts moving towards closed state.
* `Fire On Close Start Trigger State` - use-type for `Fire On Close Start` target.
* `Fire On Open End` - fire when the door reached the opened state.
* `Fire On Open End Trigger State` - use-type for `Fire On Open End` target.
* `Fire On Close End` - fire when the door reached the closed state.
* `Fire On Close End Trigger State` - use-type for `Fire On Close End` target.

### Known bugs

* If `Collision with corpses` is set to `Don't collide with corpses` the `momentary_door` will still get stuck if there're multiple corpses on its way.

### Reaction to use-type

Only `Set` is allowed - sets the progression to the opened state. The value is between 0 and 1.

### Features

* Affected by `doors_blocked_fade_corpses` server feature the same way as the [func_door]({{< ref func_door >}}).

### Calc Ratio

Reports its current progression to the opened state, the value between 0 and 1 where 0 is fully closed and 1 if fully opened.
