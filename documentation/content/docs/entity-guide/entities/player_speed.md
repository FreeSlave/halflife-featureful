---
bookHidden: true
bookToC: false
---

# player_speed

Override maximum player speed.

### Parameters

* `Maxspeed value` - new maximum speed value to set. By default it's a fraction, i.e. value between 0 and 1. Set 0 to restore the base maximum speed.
* `Value mode` - whether the `Maxspeed value` is treated as a fraction of player's base maximum speed or an an absolute number.

### Notes

{{% hint info %}}
If the current active weapon modifies the [player's maximum speed]({{< ref "weapon-templates#player_maxspeed" >}}), the resulting maximum speed is calculated as *maxspeed set by a weapon* * *player_speed factor*. If the entity uses an absolute value, the factor is this *absolute value* / *base maximum speed*. Therefore the entity contributes the speed change to the same relative degree.
{{% /hint %}}

See also: [player's maximum speed]({{< ref "player#maximum-speed" >}}).
