---
title: "Time based damage"
bookToC: false
---

# {{% param "title" %}}

Time Based Damage is a damage that is dealt to the player over some time duration after the initial impact is handled (e.g. if the initial impact had the `"poison"` type it will continue dealing some small damage over time).

Half-Life has 7 damage types that apply the time-based effect when this type of damage inflicts the player:

* `"paralyze"`
* `"nervegas"`
* `"poison"`
* `"radiation"`
* `"acid"`
* `"slowburn"`
* `"slowfreeze"`

Out of these only the `"poison"` actually does time-based damage in Half-Life. Valve had plans for making other types do damage as well, but they refused this idea for the retail.

Featureful SDK allows to configure time based damages via the **features/time_based_damage.json** file in the mod directory.

For example this is the definition of time based damage values as left in code by Valve (but in the code only the poison damage is actually used):

```json
{
    "types": {
        "paralyze": {
            "tick_count": 3,
            "damage_per_tick": 1,
        },
        "nervegas": {
            "tick_count": 3,
            "damage_per_tick": 5,
        },
        "poison": {
            "tick_count": 6,
            "damage_per_tick": 2,
        },
        "radiation": {
            "tick_count": 3,
            "damage_per_tick": 1,
        },
        "acid": {
            "tick_count": 3,
            "damage_per_tick": 5,
        },
        "slowburn": {
            "tick_count": 3,
            "damage_per_tick": 1,
        },
        "slowfreeze": {
            "tick_count": 3,
            "damage_per_tick": 1,
        }
    }
}
```

The top-level object `"types"` have properties named after the damage types. Each of such objects can have the following properties:

* `"damage_per_tick"` - how much damage the one 'tick' of the time based damage deals. By default this is 2 for `"poison"` and 0 for everything else.
* `"tick_count"` - how many times to do damage. This is set to 3 for all types, except for `"poison"` which has it set to 6, so the full dose of the time based damage applied by poison is `2 * 6 = 12` by default.
* `"interval"` - delay (in seconds) between damage 'ticks'. This is 2 seconds by default for all damage types.
* `"first_delay"` - delay before the portion of `"damage_per_tick"` is dealt after the initial impact. This is 0 by default which means the first timed based damage is dealt right after the initial impact.

{{% hint info %}}
The time based damage can be stopped sooner by some entities.

* [item_antidote]({{< ref item_antidote >}}) stops time based damage caused by `"poison"` and `"nervegas"` damage types.
* [item_radiation]({{< ref item_radiation >}}) stops time based damage caused by `"radiation"` damage type.
{{% /hint %}}

{{% hint warning %}}
The `"paralyze"` damage type is not drawn in HUD.

According to comments in the code the `"paralyze"` damage was planned to apply the temporary nerf to the player's movement. This hasn't been implemented yet.
{{% /hint %}}

{{% details title="Old Time based damage values" %}}
\
The values that seemingly were used during the development.

```json
{
    "types": {
        "paralyze": {
            "tick_count": 30,
            "damage_per_tick": 0,
        },
        "nervegas": {
            "tick_count": 16,
            "damage_per_tick": 5,
        },
        "poison": {
            "tick_count": 25,
            "damage_per_tick": 2,
        },
        "radiation": {
            "tick_count": 50,
            "damage_per_tick": 1,
        },
        "acid": {
            "tick_count": 10,
            "damage_per_tick": 5,
        },
        "slowburn": {
            "tick_count": 2,
            "damage_per_tick": 1,
        },
        "slowfreeze": {
            "tick_count": 1,
            "damage_per_tick": 3,
        }
    }
}
```
{{% /details %}}
