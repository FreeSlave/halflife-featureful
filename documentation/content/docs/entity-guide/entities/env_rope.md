---
bookHidden: true
bookToC: false
entityCategory: object
---

# ![](/images/opfor.png) env_rope

A segmented climbable rope with physics simulation.

{{% hint info %}}
Opposing Force ropes use many entities and this may cause bugs with attachments on the certain maps. If you want to avoid this you can increase the number of bits for `skin` field of `entity_state_t` in the delta.lst (just copy it from Half-Life to your mod directory and modify), from 9 to 12. The sample mod of Featureful SDK already provides a custom **delta.lst**, so you can take it from here.
{{% /hint %}}

See also: [env_electrified_wire]({{< ref env_electrified_wire >}})

### Spawnflags

* `No map transition` - disable `env_rope` transitioning to the next map when it's close to the `trigger_changelevel`.

### Soundscripts

* **Rope.Grab** - player grabbing a rope
* **Rope.Creak** - creak sound.

### Bugfixes

* In Opposing Force player could stuck in brush geometry while on rope. Featureful SDK has additional checks to prevent stucking in the walls.
