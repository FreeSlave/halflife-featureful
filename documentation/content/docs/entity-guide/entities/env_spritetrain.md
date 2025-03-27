---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) env_spritetrain

Similar to [func_train]({{< ref func_train >}}), but always non-solid and represented by a sprite instead of brushes.

See also: [env_modeltrain]({{< ref env_modeltrain >}})

### Notes

* If render amount is not specified or 0 it's set automatically to 255.
* If render mode is not specified or 0 it's set automatically to Additive (Opposing Force compatibility).

### Reaction to use-type

Always percieved as `Toggle` (due to compatibility with Half-Life trains) unless `Obey Trigger Mode` parameter is enabled. Activates or stops the train.
