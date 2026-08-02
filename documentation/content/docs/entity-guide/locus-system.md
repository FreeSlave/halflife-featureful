---
title: "Locus system"
bookToC: false
---

# {{% param "title" %}}

Featureful SDK borrows the locus concept from Spirit of Half-Life.

Each entity has three values associated with them: `Position`, `Velocity` (direction) and `Ratio`, the first two being vectors, and the third being a scalar.

* The `Position` is the origin for the most entities.
* The `Velocity` is the velocity for the most entities.
* The `Ratio` is 0 for the most entities, but there're [some exceptions]({{< ref "math#calc-ratio" >}}).

## Locus parameters

Some entity parameters allow to make their behavior depend on the `Position`, `Velocity` or `Ratio` of another entity. These parameters are usually marked with `[LP]`, `[LV]` and `[LR]` suffixes (for *Locus Position*, *Locus Velocity* and *Locus Ratio* respectively) and expect the targetname of another entity as their value.

Such parameters also support literal values, e.g. `0 0 16` for vectors (`[LP]` and `[LV]` parameters) or single number for ratio parameters (`[LR]`). Random ranges for `[LR]` parameters are supported as well, e.g. `1..2` will yield a random number on each request.

Examples of locus parameters:

* [env_explosion]({{< ref env_explosion >}}) has the `Initial position (blank = here) [LP]` parameter. Set the targetname of another entity to change where the explosion will take place. E.g. you can set the name of the monster and explosion will appear at their origin when `env_explosion` is triggered.
* [env_shooter]({{< ref env_shooter >}}) has a `At velocity [LV]` parameter to set the velocity for gibs from another source instead of using the `env_shooter` angles.
* [trigger_push]({{< ref trigger_push >}}) has a `Push speed multiplier [LR]` parameter which allows to dynamically change the push speed depending on the state of another entity.

Special name `!activator` allows to refer to the activator of the trigger chain and use its respected associated value as an entity parameter. This allows, for example, to use the activator as a position to play some effect. 

Example:

* Set the `Position (blank = here) [LP]` of [env_shockwave]({{< ref env_shockwave >}}) to the `!activator`, so the effect will be played at the origin of the activator.

Some entity parameters have `[LE]` suffix in their names, for *Locus Entity*. It means that this parameter supports the `!activator` syntax as a way to refer to another entity.

Example:

* [trigger_hurt_remote]({{< ref trigger_hurt_remote >}}) has `Target [LE]` parameter. If you put `!activator` there, it will do damage to its activator upon activation.

{{% hint info %}}
* The special name `!activator` comes from Sven Co-op and Source games.
* The special name `!caller` is not supported yet.
* The special name `*locus` from Spirit of Half-Life is supported and synonymous to `!activator`.
* The special name `*player` allows to refer to the player. In singleplayer it always refers to the player. In multiplayer it works the same way as `!activator`.
{{% /hint %}}

## Console commands

In the developer mode the following commands can be used to get the entity's `Position`, `Velocity` and `Ratio` by its targetname.

* `calc_position`
* `calc_velocity`
* `calc_ratio`

Example: `calc_position barney` (considering there's an entity named `barney` on the map).

## Calculations

Sometimes you want to calculate position, velocity or scalar in a more complex way, e.g. add some shift to the origin, multiply the velocity by scalar, or add up two scalars. That's where the `calc_` entities come in handy. They calculate their `Position`, `Velocity` and `Ratio` values depending on `Position`, `Velocity` and `Ratio` of other entities.

E.g. [calc_position]({{< ref calc_position >}}) can find another entity, take the selected position from it (*Eyes*, *Origin*, etc.) and add another vector via the `Add offset` parameter. The calculated position is reported as `Position` value of this `calc_position` (instead of its origin). Some other entity can set the targetname of the `calc_position` entity as a source value for one of its `[LP]` parameters to use the calculated position.

* [calc_eval_number]({{< ref calc_eval_number >}}) (specific to Featureful, didn't exist in SoHL)
* [calc_numfromvec]({{< ref calc_numfromvec >}}) (introduced in SoHL 1.8, has limited implementation in Featureful)
* [calc_position]({{< ref calc_position >}}) (renamed to `calc_posfroment` in SoHL 1.8)
* [calc_ratio]({{< ref calc_ratio >}}) (renamed to `calc_numfromnum` in SoHL 1.8)
* [calc_subvelocity]({{< ref calc_subvelocity >}}) (renamed to `calc_vecfroment` in SoHL 1.8)
* [calc_vecfromnums]({{< ref calc_vecfromnums >}}) (specific to Featureful, didn't exist in SoHL)
* [calc_velocity_path]({{< ref calc_velocity_path >}}) (renamed to `calc_vecfrompos` in SoHL 1.8)
* [calc_velocity_polar]({{< ref calc_velocity_polar >}}) (renamed to `calc_vecfromvec` in SoHL 1.8)

{{% hint info %}}
The `calc_` entities re-calculate the resulting value on each demand. The [locus_variable]({{< ref locus_variable >}}) can be used to calculate and save the result.
{{% /hint %}}

You can find some Spirit of Half-Life tutorials related to the locus system online. For the most part they will be applicable to Featureful SDK as well.
