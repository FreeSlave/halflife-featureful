---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/opfor.png) weapon_grapple

Grappling hook. Allows to latch on textures which names start with `XENO_GRAPPLE`, [ammo_spore]({{< ref ammo_spore >}}) and monsters. Small monsters will be pulled to the player.

{{% hint warning %}}
Latching on monsters may let player get to the unintended locations. Make sure to configure `Size For Grapple` for monster instance or set `size_for_grapple` in the [entity template]({{< ref entity-templates >}}) in order to avoid this.
{{% /hint %}}

### Skill variables

* **sk_plr_grapple** - damage per 0.5 seconds.
