---
bookHidden: true
bookToC: false
---

# player_speed

Override maximum player speed.

### Parameters

* `Maxspeed value` - new maximum speed value to set. By default it's a fraction, i.e. value between 0 and 1. Set 0 to restore the default value.
* `Value mode` - whether the `Maxspeed value` is treated as a fraction of player's default maximum speed or an an absolute number.

{{% hint info %}}
The actual maximum speed can't exceed the one set by `sv_maxspeed` cvar (320 by default). So setting the factor higher than 1 or the absolute value higher than the `sv_maxspeed` value won't do the difference.
{{% /hint %}}
