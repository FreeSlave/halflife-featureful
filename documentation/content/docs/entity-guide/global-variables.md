---
title: "Global variables"
bookToC: false
---

# {{% param "title" %}}

Global variables are named variables accessible across multiple levels. This feature existed in original Half-Life and has been extended in Featureful SDK.

In addition to **State** (which can be `On`, `Off` or `Dead`) each global variable has an associated numeric *integer* **Value**.

Just like in original HL, global variables can be set and read via [env_global]({{< relref "env_global.md" >}}) entity. It got some new parameters in Featureful SDK.

## Console commands

To see the output of the following commands you must set `developer` cvar to 1 or higher.

* `impulse 104` is used to output all existing global variables.
* `set_global_state` - set a global variable state (`off`, `on`, `dead`). This is for debugging purposes and it's allowed only when `sv_cheats` is enabled.
* `set_global_value` - set a number value associated with a global variable. This is for debugging purposes and it's allowed only when `sv_cheats` is enabled.
