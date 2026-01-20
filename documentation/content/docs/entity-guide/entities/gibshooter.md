---
bookHidden: true
bookToC: false
---

# gibshooter

[TWHL](https://twhl.info/wiki/page/gibshooter)

A shooter entity for producing gibs. See also: [env_shooter]({{< ref env_shooter >}}).

{{% hint info %}}
Set the `Delay between shots` to negative value to make `gibshooter` produce the specified number of projectiles on the same frame.
{{% /hint %}}

### New parameters

* ![](/images/spirit.png) `Shoot from position [LP]` - custom position to shoot gibs from (instead of the `gibshooter` origin).
* ![](/images/spirit.png) `Shoot at velocity/direction [LV]` - custom direction to shoot gibs at (instead of relying on entity's angles). Note: the calculated vector is not normalized automatically; you need to ensure the calc entity normalizes the vector, otherwise it will act as an additional speed factor.
* ![](/images/spirit.png) `Gib Speed Factor` (originally `Gib Velocity` in Half-Life) - now can be [locus ratio]({{< ref locus-system >}}) for dynamic gib speed.
* ![](/images/spirit.png) `Fire on spawn` - fire this target on behalf of the created gib. This allows to play additional effects on the gibs, e.g. beam trails, via the [locus system]({{< ref locus-system >}}).

### New spawnflags

* `Start instantly` - start producing projectile on the same frame it was called. By default in Half-Life it starts producing gibs/shots only on the next frame which may cause problems in certain setups.
