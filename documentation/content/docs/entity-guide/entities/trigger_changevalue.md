---
bookHidden: true
bookToC: true
---

# ![](/images/svencoop.png) trigger_changevalue

[svenmanor](https://www.svenmanor.com/entity-guide/trigger_changevalue)

Dynamically replaces the [entity keyvalue]({{< ref entity-keyvalues >}}). Also can perform arithmetic, logic and string operations on the limited set of keyvalues (currently only [entvars]({{< ref "entity-keyvalues/#entvars" >}})).

For vector keyvalues (e.g. if destination key is `origin` or `velocity`) the source value is expected to be in the form `x y z` where x, y and z are numbers.

{{% hint warning %}}
Normally the entity keyvalues are set and processed at the entity initialization phase, so changing them mid-game might bring some unexpected results, do nothing or, on other hand, break things. Therefore `trigger_changevalue` must be used with care.
{{% /hint %}}

## Parameters

* `Destination entity [LE]` - the targetname of entity(-ies) affected by a change.
* `Destination key` - the key name of keyvalue to change. Note: this should be the internal key name, not the descriptive name featured in FGD. Toggle Smart Edit in the level editor to see the actual key name.
* `Source value` - the value to apply in the operation. Depending on the `Treat source value as` it's either a literal value or the name of the entity to calculate Ratio/Position/Velocity from and use it as a source value
* `Treat source value as` - interpretation of the `Source value`.
* `Action` - this specifies how destination keyvalue and static source-value correlate. What exactly happens also depends on the types of the two. While `Replace` works on all parameters, other actions work only on the limited set of keyvalues (as they require reading of the original value by key which is not straightforward operation in Half-Life). E.g. in the case of `Add` action it goes as `c = c + s` where `c` is the current value and `s` is a source value. If both the current value and the source value are vectors, it does a component-wise operation, e.g. `c.x = c.x * s.x`, `c.y = c.y * s.y`, `c = c.z * s.z`. If the current value is a vector and the source value is a scalar, it applies operation to all components, e.g. `c.x = o.x + s`, `c.y = o.y + s`, `c = c.z + s`. Bitwise operators work only on integer fields like `spawnflags` and `effects`.
    - `Replace` - `c = s`
    - `Add` - `c = c + s`
    - `Mul` - `c = c * s`
    - `Sub` - `c = c - s`
    - `Div` - `c = c - s`. It's an invalid operation to divide by 0. The integer division is performed if the keyvalue is integer.
    - `Mod` - `c = c % s`. Calculates the [Modulo](https://en.wikipedia.org/wiki/Modulo), i.e. reminder of the division. Floating point numbers are converted to integers for this operation.
    - `Pow` - `c` to the power of `s`.
    - `Bitwise AND` - `c = c & s`. [Bitwise AND](https://en.wikipedia.org/wiki/Bitwise_operation#AND). Works on integers only.
    - `Bitwise OR` - `c = c | s`. [Bitwise OR](https://en.wikipedia.org/wiki/Bitwise_operation#OR). Works on integers only.
    - `Bitwise XOR` - `c = c ^ s`. [Bitwise XOR](https://en.wikipedia.org/wiki/Bitwise_operation#XOR). Works on integers only.
    - `Clear bits` - `c = c & ~s`. 1-bits from `s` will be removed in `c`. Works on integers only.
    - `Append (String concatenation)` - `c = c + s`. Works on strings only.
    - `Min` - choose the minimum value between `c` and `s`. Works on numbers and vectors (does a per-component comparison).
    - `Max` - choose the maximum value between `c` and `s`. Works on numbers and vectors (does a per-component comparison).
* `Trigger after operation` - trigger entities after the value was set. This can call another `trigger_changevalue` without any delay so it's useful for chained operations.

## Spawnflags

* `Don't use X` - when performing action on vector keyvalue, leave the **X** as is.
* `Don't use Y` - when performing action on vector keyvalue, leave the **Y** as is.
* `Don't use Z` - when performing action on vector keyvalue, leave the **Z** as is.

## Reaction to use-type

The use-type is ignored.

## Examples

### Make something non-solid

Let's say you want to make some brush entity, e.g. `func_door`, non-solid.

* Set `Destination key` to `solid`.
* Set `Source value` to `0`.
* Set `Action` to `Replace` (default).

To make the brush entity solid again use value `4` (note that it poses the risk of making another entity stuck in it).

### Set the body of the model

* Set `Destination key` to `body`.
* Set `Source value` to the desired body number`.
* Set `Action` to `Replace` (default).

The real case examples:
* Adding or removing the G-man's phone in his Opposing Force model.
* Setting the submodel of decorative `item_generic` (e.g. switch between unbroken and broken states of the car model).

### Adding health

Let's say you have a `func_breakable` and you want a way to increase its health dynamically.

* Set `Destination key` to `health`.
* Set `Source value` to the positive value you want.
* Set `Action` to `Add`.

### Appending text

Let's say you have a `game_text` that shows some objective and the objective can be different depending on the previous events.

In the the `game_text` Message Text put the part of the text, e.g. `Eliminate all ` (notice the space in the end).

Then, you have two `trigger_changevalue` entities. Depending on some condition only one of them gets activated before showing the text.

For both `trigger_changevalue` entities:

* Set `Destination entity` to the targetname of `game_text`.
* Set `Destination key` to `message`.
* Set `Action` to `Append (String concatenation)`.

Then:

* In the first `trigger_changevalue` set `Source value` to `aliens`.
* In the second `trigger_changevalue` set `Source value` to `soldiers`.

Depending on what instance got triggered, the `game_text` then will show either "Eliminate all aliens" or "Eliminate all soldiers".

### Effects

Various predefined effects can be applied to the entity by changing the `effects` keyvalue.

Set the action to `Bitwise OR` and set the value to one of the following:

* `1` - surround the entity with the cloud of yellow blobs (same effect is used to mark entity as stuck in the geometry).
* `4` - make entity emit the dynamic light of constant radius.
* `16` - make entity take light from the ceiling (this is set for barnacles and ceiling turrets by default).
* `128` - make entity invisible. It also makes the entity stop sending its position updates to the client, so attachments (e.g. beams) on such entities won't work correctly. If you want something to be invisible but keep attachments working just use render parameters or [env_render]({{<  ref env_render >}}).
* `1024` - make entity not be reflected in [env_mirror]({{< ref env_mirror >}}).
* `2048` - make entity model always bright even in dark environment.

To remove any of these use `Clear bits` action. To toggle the effect use `Bitwise XOR`.

{{% hint info %}}
Don't forget to include the custom [delta.lst]({{< ref delta-lst >}}) for proper support of some effects.
{{% /hint %}}

### Punchangle

To apply a punchangle to a player:

* Set `Destination entity` to `*player`.
* Set `Destination key` to `punchangle`.
* Set `Action` to `Add` or `Replace` (depending on whether you want to add to the existing punchangle or replace it completely).
* Set `Source value` to the vector value. Examples:
    * `3 0 0` - punch down.
    * `-3 0 0` - punch up.
    * `0 3 0` - punch left.
    * `0 -3 0` - punch right.

The punch decays automatically so no other input is required to cancel the punch afterwards.
