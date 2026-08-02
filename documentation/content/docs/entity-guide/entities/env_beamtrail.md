---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) env_beamtrail

Creates a beam trail effect on the entity. The entity must be moving when the trail is activated.

Examples:

* Set `Fire on spawn` of [env_shooter]({{< ref env_shooter >}}) to the `trigger_relay` with `Trigger State` set to `On` which targets the `env_beamtrail` to draw beam trails on the spawned gibs.

{{% hint warning %}}
The beam trail currently doesn't survive the save-restore.
{{% /hint %}}

### Parameters

### Spawnflags

* `Start off` - mark to prevent beam trail creation on the map start.

### Reaction to use-type

If called with `On` use-type, this will always create a beam trail on the target. Otherwise depending on the current state (the initial state depends on the `Start off` spawnflag) it will create the beam or remove the beam.

{{% hint danger %}}
Currently turning off the beam trail will kill all the beams attached to the entity.
{{% /hint %}}
