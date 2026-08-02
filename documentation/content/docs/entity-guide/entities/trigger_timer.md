---
bookHidden: true
bookToC: false
---

# trigger_timer

Continuously fires its target over random delays.

### Parameters

* `Minimum delay` - minimum delay, in seconds, before firing another target. The actual delay is randomized between this and `Maximum delay`.
* `Maximum delay` - maximum delay, in seconds, before firing another target. If it's less than `Minimum delay` the delay won't be randomized and will always be the same as `Minimum delay`.
* `Trigger number limit` - `trigger_timer` will automatically stop after triggering the target this number of times. After that `trigger_timer` can be reactivated again via external trigger. 0 means no limit.
* `Trigger on limit` - trigger when the trigger limit is reached.

### Spawnflags

* `Start on` - automatically activate `trigger_timer` on the map start.
* `Don't delay first fire` - don't use delay on the first target fire upon the activation.
* `Forward activator` - forward my activator when triggering my target. Without this flag `trigger_timer` passes itself as an activator.
* `Scheduled delays` - makes `trigger_timer` infinitely schedule the values of timed delays beforehand. If player loads the saved game the `trigger_timer` will use the same delays as it scheduled originally.
* `Don't reset trigger counter on deactivation` - if `Trigger number limit` is defined and `trigger_timer` is turned off due to the external trigger, it won't reset the trigger counter to 0. This allows to activate `trigger_timer` later with the preserved trigger counter. If trigger counter reaches the trigger number limit it will still naturally reset to 0 and deactivate.

### Reaction to use-type

Starts or stops the periodic target firing depending on the use-type. On deactivation the trigger counter resets automatically unless `Don't reset trigger counter on deactivation` spawnflag is set.
