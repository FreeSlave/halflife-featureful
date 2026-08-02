---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) trigger_random

[svenmanor](https://www.svenmanor.com/entity-guide/trigger_random)

Triggers randomly picked target from the target set.

{{% hint info %}}
The Timed mode exists for compatibility with Sven Co-op. You can use [trigger_timer]({{< ref trigger_timer >}}) in combination with non-timed `trigger_random` instead.

`trigger_random_time` and `trigger_random_unique` aliases exist purely for compatibility with Sven Co-op. Use `trigger_random` with appropriate spawnflags instead.
{{% /hint %}}

### Parameters

* `Target 1-16` - the list of targets to randomly choose from and fire. In `Timed` mode the `trigger_random` sends itself as an activator. Otherwise the activator is forwarded.
* `Target Count` - amount of random targets you have specified. This is now optional and left for compatibility with Sven Co-op's `trigger_random`. When omitted, the count will be calculated automatically depending on the non-empty targets. The list of targets is allowed to have empty entries: the result depends on the position of the last non-empty target.
* `Minimum delay (Timed only)` - minimum delay, in seconds, before firing another target. The actual delay is randomized between this and `Maximum delay`.
* `Maximum delay (Timed only)` - maximum delay, in seconds, before firing another target. If it's less than `Minimum delay` the delay won't be randomized and will always be the same as `Minimum delay`.
* `Trigger number limit (Timed only)` - automatically stop after triggering this number of targets (including empty ones, if present). This was not in Sven Co-op and a special addition. A flexible replacement for `Trigger Once` spawnflag.
* `Trigger on limit (Timed only)` - trigger when the `Trigger number limit` is reached.
* `Trigger on exhaust (Unique only)` - fire when last unique target is called. Can be fired again if `trigger_random` is `Reusable` and it drains the unique targets another time.
* `Index of first target (1-16)` - first target to fire on the first entity call. 0 means no preference.

### Spawnflags

* `Start on (Timed only)` - whether the timed `trigger_random` is active on the level start.
* `Trigger Once (Timed only)` - trigger a random target once, instead of continuously triggering random targets until deactivation.
* `Reusable (Unique only)` - 
* `Timed` - enables `Minimum delay` and `Maximum delay` keyvalues so you can specify to wait a random amount of time before triggering random targets.
* `Unique` - trigger will pick target (each time it's triggered or in intervals for timed `trigger_random`), that haven't been triggered yet, randomly.
* `Not repetitive` - don't repeat the same target twice in a row. Technically targets still can be repeated if there're several with the same name.
* `Scheduled order` - makes trigger_random infinitely schedule the order in which it fires its targets beforehand. If player loads the saved game the `trigger_random` will fire its targets in the same scheduled order. This prevents player from getting 'the best outcome' just by loading the save again and again until they're satisfied with the result. The schedule is getting defined at the map load, so technically player still can circumvent the restriction by loading the save from a previous map and going through the changelevel to the new map again which will yield a different schedule for `trigger_random`. This should be fixed in future."
* `Scheduled delays (Timed only)` - makes `trigger_random` infinitely schedule the values of timed delays beforehand. If player loads the saved game the `trigger_random` will use the same delays as it scheduled originally.

### Reaction to use-type

If set to `Timed`, it continues or stops the execution independently of the use-type. In non-timed mode use-type is ignored as well.
