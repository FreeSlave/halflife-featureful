---
bookHidden: true
bookToC: false
entityCategory: object
---

# ![](/images/opfor.png) env_electrified_wire

An electrified segmented wire with physics simulation.

See also: [env_rope]({{< ref env_rope >}})

### Reaction to use-type

Always percieved as `Toggle` (due to Opposing Force compatibility) unless `Obey Trigger Mode` spawnflag is ticked - in this case the entity handles `On` and `Off` use-types as well. Changing the state makes electricity to turn off or on.
