---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_penguin

Similar to [snark]({{< ref monster_snark >}}) but detonates like a grenade.

### Skill variables

* **sk_penguin_health** - monster's health. If not defined, **sk_snark_health** is used.
* **sk_penguin_dmg_bite** - bite damage. If not defined, **sk_snark_dmg_bite** is used.
* **sk_penguin_dmg_pop** - the base explosion damage. If not defined, **sk_plr_hand_grenade** is used.
* **sk_penguin_add_dmg_pop** - each bite insreases the explosion damage by the this number. By default this is the same as **sk_penguin_dmg_pop**.
* **sk_penguin_max_dmg_pop** - maximum explosion damage penguin can reach by biting the prey. 0 means no limit. By default this is **sk_plr_hand_grenade** multiplied by 5.
* **sk_penguin_lifespan** - maximum monster's lifespan before it explodes. Negative values mean an infinite lifespan. If not defined, **sk_snark_lifespan** is used.

### Default classification

`Snark`

### Soundscripts

Uses same soundscripts as [monster_snark]({{< ref monster_snark >}}).
