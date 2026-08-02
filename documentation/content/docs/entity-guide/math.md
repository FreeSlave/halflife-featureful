---
title: "Math"
bookToC: false
---

# {{% param "title" %}}

Half-Life has a very limited number of entities to do math operations. They're `game_counter` and `game_counter_set`. These entities are very limited by themselves, and **Featureful SDK** provides more powerful ways to do math operations.

### Calc Ratio

All entities have a calculable value associated with them - the concept borrowed from *Spirit of Half-Life*, where it's called Calc Ratio (see also: [Locus system]({{< ref locus-system >}})). While most entities have a constant zero value as Calc Ratio, there're few entities that implement this concept in Featureful SDK and might have a dynamic value:

* [game_counter]({{< ref game_counter >}}) - the Calc Ratio is the current counter value. Note that `game_counter` holds integer values only.
* [env_global]({{< ref env_global >}}) - since each global has an integer number associated with them in Featureful SDK, the `env_global` can be used to access this number.
* Monsters. For them Calc Ratio is the *health* / *max health* fraction (the the number in [0..1] range). Killed monsters report 0.
* [func_breakable]({{< ref func_breakable >}}). Same as for monsters, the Calc Ratio is the *health* / *max health* fraction.
* Wall rechargers (health and armor). For them Calc Ratio is the *juice left* / *max juice capacity* fraction (the the number in [0..1] range).
* [calc_ratio]({{< ref calc_ratio >}}) - Calc Ratio is calculated based on Calc Ratio of another entity.
* [calc_eval_number]({{< ref calc_eval_number >}}) - calculate a number from two operands.
* [calc_numfromvec]({{< ref calc_numfromvec >}}) - get a component from a vector.
* [game_number]({{< ref game_number >}}) - Calc Ratio is a stored floating point value.
* [player_calc_ratio]({{< ref player_calc_ratio >}}) - report some player related values like current health, armor and ammo.

### Locus Ratio

Locus Ratio (or `LR` for short) is a type of entity parameter. In FGD file they're marked with `[LR]` suffix. `LR` parameter can be either a constant number or a name of another entity. In the latter case the Calc Ratio result of another entity will be used as a number.
The entity that have `LR` parameters can behave differently depending on their values.

Some examples of entities having `LR` parameters:

* [trigger_push]({{< ref trigger_push >}}) - can have a speed factor calculable from another entity.
* [env_shooter]({{< ref env_shooter >}}) - can have a speed factor calculable from another entity.
* [calc_ratio]({{< ref calc_ratio >}}) - calculates its own Calc Ratio depending on `LR` parameter that could refer to another entity.
* Vector related calc entities (e.g. `calc_subvelocity`) have `LR` parameter for factor. 
* [calc_eval_number]({{< ref calc_eval_number >}}) - calculate its own Calc Ratio depending on two `LR` parameters.
* [trigger_compare]({{< ref trigger_compare >}}) - when triggered, compares calculated values of two `LR` parameters and fire the targets accordingly to result.

### Storing a value

There're few entities that can act as value storage for the result of `calc_eval_number`.

* [game_counter]({{< ref game_counter >}}) - the stored value will be cast to integer. The counter may fire its target and reset if it hit the limit.
* [game_number]({{< ref game_number >}}) - the stored value will be preserved as is. This entity just stores the value and it's expected to be accessed by other entities.
* [env_global]({{< ref env_global >}}) with `Modify/Set value` trigger mode. The value will be cast to integer.
