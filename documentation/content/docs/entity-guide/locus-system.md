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

### Locus parameters

Some entity parameters allow to make their behavior depend on the `Position`, `Velocity` or `Ratio` of another entity. These parameters are usually marked with `[LP]`, `[LV]` and `[LR]` suffixes (for *Locus Position*, *Locus Velocity* and *Locus Ratio* respectively) and expect the targetname of another entity as their value.

Such parameters also support literal values, e.g. `0 0 16` for vectors (`[LP]` and `[LV]` parameters) or single number for ratio parameters (`[LR]`).

Examples:

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

### Calculations

Sometimes you want to calculate position, velocity or scalar in a more complex way, e.g. add some shift to the origin, multiply the velocity by scalar, or add up two scalars. That's where the `calc_` entities come in handy. They calculate their `Position`, `Velocity` and `Ratio` values depending on `Position`, `Velocity` and `Ratio` of other entities.

E.g. [calc_position]({{< ref calc_position >}}) can find another entity, calculate the chosen position from it (*Eyes*, *Origin*, etc.) and add another vector as `Add offset` parameter. Then some other entity can set the targetname of the `calc_position` entity as a source value for one of its `[LP]` parameters.

You can find some Spirit of Half-Life tutorials related to the locus system online. For the most part they will be applicable to Featureful SDK as well.
