---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/opfor.png) weapon_knife

### Changes

* Has an alternative attack (akin to one of [weapon_pipewrench]({{< ref weapon_pipewrench >}})).

### Skill variables

* **sk_plr_knife** - slash damage.
* **sk_plr_knife_stab_base** - stab (secondary attack) base damage. Default value is 20.
* **sk_plr_knife_stab_factor** - stab additional damage factor. Used to calculate the additional damage based on the charge time (the higher the value the less time player needs to reach the maximum damage). If not defined, **sk_plr_knife** is used.
* **sk_plr_knife_stab_max** - stab maximum damage. Default value is 100.

The stab attack damage is calculated by formula:

```
MIN(sk_plr_knife_stab_base + sk_plr_knife_stab_factor * time_since_charge_start, sk_plr_knife_stab_max)
```
