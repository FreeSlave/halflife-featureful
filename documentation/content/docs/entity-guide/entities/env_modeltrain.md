---
bookHidden: true
bookToC: false
---

# env_modeltrain

Similar to [func_train]({{< ref func_train >}}), but always non-solid and represented by a .mdl model instead of brushes. The model will automatically change its angles to face the next target in path. By default it plays the first model sequence (index 0) with framerate 1.0. Even if animation doesn't have a loop flag it's forced to loop.

See also: [env_spritetrain]({{< ref env_spritetrain >}})

### Parameters

* `Idle sequence` - the name of the model sequence to play while the train is inactive (idle).
* `Custom idle framerate` - animation framerate when idle sequence is played.
* `Move sequence` - the name of the model sequence to play while the train is active (moving).
* `Custom move framerate` - animation framerate when move sequence is played.

### Reaction to use-type

Always percieved as `Toggle` (due to compatibility with Half-Life trains) unless `Obey Trigger Mode` parameter is enabled. Activates or stops the train.
