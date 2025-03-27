---
bookHidden: true
bookToC: false
---

# env_global

[TWHL](https://twhl.info/wiki/page/env_global)

See also: [Global variables]({{< ref global-variables >}})

### New parameters

* New `Trigger Mode` options: `Modify/Set value` and `Set State: Obey Use-type`.
* `Initial Value` is used to set the initial numeric value of the global variable.

### New spawnflags

* `Act as Master`. See [master entities]({{< relref "master-entities.md" >}})

### Reaction to use-type

* Depending on the `Trigger Mode` it might ignore the input use-type and change the associated global variable state according to the selected trigger mode. Selecting `Set State: Obey Use-type` makes it obey the use-type.
* If `Trigger Mode` is `Modify/Set value` it reacts as follows:
* * If use-type is `Set` (e.g. fired by `game_counter_set` or `calc_eval_number`) it assigns the input value to the numeric value of the associated global variable.
* * If use-type is `Toggle` or `On` it increments the numeric value of the associated global variable by 1.
* * If use-type is `Off` it decrements the numeric value of the associated global variable by 1.
* * In all cases if the global variable doesn't exist yet, it gets created first with the numeric value from `Initial Value`.

### Master capability

If `Act as Master` spawnflag is enabled the entity can be used as a master. If the global state is `On`, then the internal state is reported as On (unlocked). Otherwise it's Off (locked).

### Calc Ratio

When used as `[LR]` parameter it reports the numeric value of the global variable. See [Locus system]({{< ref locus-system >}}) and [Math]({{< ref math >}}).
