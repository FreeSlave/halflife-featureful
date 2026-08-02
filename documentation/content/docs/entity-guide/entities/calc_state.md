---
bookHidden: true
bookToC: false
---

# calc_state

This entity takes states of two other entities (e.g. [env_state]({{< ref env_state >}}) or [env_global]({{< ref env_global >}})) and evaluate the resulting state using the specified logical operator (AND, OR, etc.). The `On` state is interpreted as True and the `Off` state is interpreted as False from boolean algebra.

`calc_state` will trigger other entities depending on the result of the operation when it's triggered by something else.

### Parameters

* `Operand 1 [LE]` - entity to use as a first operand.
* `Operand 2 [LE]` - entity to use as a second operand.
* `Operation` - logic boolean operation to perform on states of operands to get the result.
* `Target (on & off)` - trigger entities sending On if the calculated state is On, and Off if the calculated state is Off.
* `Fire when eval to False` - trigger entities when the calculated state is Off.
* `Fire when eval to True` - trigger entities when the calculated state is On.
* `Operand 1 fallback` - how to treat the missing first operand.
* `Operand 2 fallback` - how to treat the missing second operand.

If `calc_state` is set to target the [env_state]({{< ref env_state >}}) then the latter is basically used as a variable to store the result of the operation allowing for the arbitrary boolean expressions. The [env_global]({{< ref env_global >}}) can be used in a similar fashion if it has `Set State: Obey Use-type` as a trigger mode.

### Master capability

The calculated state is reported. Checking the state doesn't fire the targets. 

If there's an error (e.g. missing operand) the reported state is Off.
