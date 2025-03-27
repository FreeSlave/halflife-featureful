---
bookHidden: true
bookToC: true
---

# ![](/images/svencoop.png) trigger_condition

[svenmanor](https://www.svenmanor.com/entity-guide/trigger_condition)

A, by default, toggleable entity, which, while activated, will compare a value of an entity's key with either a specified value or the keyvalue of another entity, using a specified comparator and fire its target for the according result- true or false- whenever the result of the condition changes (by default) or at the instant the `trigger_condition` entity is activated manually. While activated, the `trigger_condition` checks its condition at certain intervals.

## Parameters

* `Monitored entity [LE]` - name of the monitored entity.
* `Monitored key` - the monitored entity key (currently only [entvars]({{< ref "entity-keyvalues/#entvars" >}}) are supported). The value of this key will be used in comparisons.
* `Compare-entity [LE]` - name of the entity of which a keyvalue shall serve as a compare-value. Only one entity by this name should exist.
* `Compare-key` - name of the key which value is to be compared to the monitored key.
* `Compare-value (alternative)` - compare to this literal value instead of retrieving the value from `Compare-entity` by `Compare-key`.
* `Comparator` - the comparator type between the monitored keyvalue and the compare value. Vector keyvalues support only "Equal" and "Not equal" checks.
    - `== (Equal)` - the keyvalues are equal.
    - `!= (Not equal)` - the keyvalues are not equal.
    - `< (Less)` - the keyvalue of the monitored entity is less than the provided value.
    - `> (Greater)` - the keyvalue of the monitored entity is greater than the provided value.
    - `<= (Less or equal)` - the keyvalue of the monitored entity is less than or equal to the provided value.
    - `>= (Greater or equal)`- the keyvalue of the monitored entity is greater than or equal to the provided value.
    - `& (Logical AND)` - the keyvalue of the monitored entity contains at least one bit from the provided value. Works with integers only.
* `Target for 'true'-case` - target to trigger when the comparison result is true. Sends `Toggle` use-type.
* `Target for 'false'-case` - target to trigger when the comparison result is false. Sends `Toggle` use-type.
* `In Constant Mode fire when` - if the `trigger_condition` uses constant mode (i.e. no `Cyclic` spawnflag is not set), this specifies how it will fire its true- and false- target.
    - `Result has changed` - fire when result has changed: from false to true or vice versa, or from unknown to false or true (the first evaluation).
    - `Result has changed or it keeps being true` - fire `Target for 'true'-case` at specified intervals when result is true or fire `Target for 'false'-case` once when result becomes false.
    - `Result has changed or it keeps being false` - fire `Target for 'false'-case` at specified intervals when result is false or fire `Target for 'true'-case` once when result becomes true.
    - `Always` - fire the appropriate target in the specified intervals.
* `Check-interval (seconds)` - the interval between condition checks. 0.0 means to check every frame. Upon activation the interval also acts as a delay before the first check. Set `Don't delay first check` to avoid this behavior.

## Spawnflags

* `Start Off` - the `trigger_condition` starts off and has to be triggered on manually in order to work. This makes sense only for contant mode (i.e. `Cyclic` spawnflag is not set).
* `Don't use X (R)` - treat the first component of both vectors as equal.
* `Don't use Y (G)` - treat the second component of both vectors as equal.
* `Don't use Z (B)` - treat the third component of both vectors as equal.
* `Cyclic; no toggle` - changes the `trigger_condition` behaviour to a single-fire-behaviour. With this flag set, the entity will fire a target according to its condition's result once, and then idle until triggered again.
* `Forward activator` - pass the original activator when triggering targets (otherwise `trigger_condition` sends itself as activator). The mapper should make sure the original activator exists as long as the `trigger_condition` is active.
* `Ignore initial result`- if this is set and `Cyclic` flag is not selected, every time you activate it, the very first result it produces will be ignored and nothing will be triggered, and it will proceed normal according to the specified trigger-behaviour.
* `Don't delay first check` - don't delay first check when gets activated in constant mode. Note: this spawnflag didn't exist in Sven Co-op and was introduced in Featureful SDK.

## Reaction to use-type

Toggles the periodic check depending on the use-type. If `Cyclic` spawnflag is enabled, the use-type is ignored - each call performs a comparison.

## Examples

### Check the health value

* Set `Monitored key` to `health`.
* Set `Compare-value` to the desired value, e.g. 50.
* Set `Comparator` to the desired comparator, e.g. "Less".
