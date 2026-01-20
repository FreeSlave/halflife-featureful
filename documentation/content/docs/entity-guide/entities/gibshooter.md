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

* `At position` - custom position to shoot gibs from (instead of the `gibshooter` origin).
* `At velocity` - custom direction to shoot gibs at (instead of relying on entity angle).
* `Gib Speed Factor` (originally `Gib Velocity` in Half-Life) - now can be [locus ratio]({{< ref locus-system >}}) for dynamic gib speed.
* `Fire on spawn` - fire this target on behalf of the created gib.

### New spawnflags

* `Start instantly` - start producing projectile on the same frame it was called. By default in Half-Life it starts producing gibs/shots only on the next frame which may cause some problems.
