---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# ![](/images/opfor.png) spore

A spore grenade used by [ammo_spore]({{< ref ammo_spore >}}), [monster_shocktrooper]({{< ref monster_shocktrooper >}}) and [weapon_sporelauncher]({{< ref weapon_sporelauncher >}}). The rocket variant is used by [weapon_sporelauncher]({{< ref weapon_sporelauncher >}}).

### Skill variables

* **sk_plr_spore** - explosion damage.
* **sk_plr_spore_direct** - direct hit damage. If not defined, **sk_plr_spore** is used.

{{% hint info %}}
Spore deals two times - first when (and if) it hits something that can take damage, and the explosion damage right after that.
{{% /hint %}}

### Soundscripts

* **Spore.Bounce** - when bouncing from walls.
* **Spore.Impact** - explosion sound.

### Visuals

* **Spore.Model** - spore itself.
* **Spore.Sprite** - attached sprite.
* **Spore.Blow** - sprite played on impact.
* **Spore.BlowAlt** - alternative sprite played on impact.
* **Spore.Spray** - spray played on impact.
* **Spore.Trail** - trailing spray during the flight.
* **Spore.Light** - dynamic light created on impact.
