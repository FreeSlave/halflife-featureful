---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_zombie_soldier

### Skill variables

* **sk_zombie_soldier_health** - monster's health.
* **sk_zombie_soldier_dmg_one_slash** - one arm damage.
* **sk_zombie_soldier_dmg_both_slash** - two arms damage.

### Default classification

`Alien Monster`

### Soundscripts

Uses same soundscripts as [monster_zombie]({{< ref monster_zombie >}}).

### Attacks

Same as [monster_zombie]({{< ref monster_zombie >}}).

### Animation events

Same as [monster_zombie]({{< ref monster_zombie >}}).

### Bugfixes

* Opposing Force zombie soldier model didn't set proper hitgroups for hitboxes (which prevented taking the increased headshot damage). The sample mod has the hitgroups fixed. If you use custom models, make sure the proper hitgroups are set for hitboxes.
