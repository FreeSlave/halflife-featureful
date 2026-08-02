---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# grenade

This represents either a hand grenade (timed) or an AR grenade (contact).

### Skill variables

* **sk_plr_hand_grenade** - explosion damage of the hand (timed) grenade.
* **sk_plr_hand_grenade_hit** - hand grenade direct hit damage. Default value is 1.
* **sk_plr_9mmAR_grenade** - explosion damage of the AR (contact) grenade.

### Soundscripts

* **BaseGrenade.Debris** - plays together with explosion. Used with other explosive projectiles as well like satchels, rockets and tripmines.
* **HandGrenade.Bounce** - the hand grenade hits the wall.

### Visuals

* **HandGrenade.Model** - timed grenade model.
* **ARGrenade.Model** - contact grenade model.

### Entity template examples

{{% tabs %}}

{{% tab "Detonate on touch" %}}
Detonate hand grenade when it's touching something that can take damage, without waiting for timer.

```json
{
    "grenade": {
        "projectile": {
            "detonate_on_touch": "damageable"
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
