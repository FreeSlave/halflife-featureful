---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_male_assassin

### Changes

* When male assassin checks for friendly fire he takes into account all allies, not only those who are in the same squad as him.

### Skill variables

* **sk_massassin_health** - monster's health. If not defined, **sk_hgrunt_health** is used.
* **sk_massassin_kick** - kick damage. If not defined, **sk_hgrunt_kick** is used.
* **sk_massassin_gspeed** - the grenade speed when it's thrown. If not defined, **sk_hgrunt_gspeed** is used.

### Default classification

`Human Military` or `Blackops` if `blackops_classify` server feature is enabled.

### Soundscripts

* **Massn.Pain** - pain sounds. Empty by default.
* **Massn.Die** - death sounds. Empty by default.
* **Massn.Reload** - reload sound. Derived from **NPC.Reload**
* **Massn.9MM** - burst MP5 shot. Derived from **NPC.9MM**
* **Massn.GrenadeLaunch** - MP5 grenade launch. Derived from **NPC.GrenadeLaunch**
* **Massn.Sniper** - sniperrifle fire. Derived from **NPC.Sniper**

### Attacks

Same as [monster_human_grunt]({{< ref "monster_human_grunt/#attacks" >}}).

### Animation events

Same as [monster_human_grunt]({{< ref "monster_human_grunt/#animation-events" >}}), but deals **sk_massassin_kick** damage on kick event.

### Bugfixes

* Opposing Force male assassin model didn't set proper hitgroups for hitboxes (which prevented taking the increased headshot damage). The sample mod has the hitgroups fixed. If you use custom models, make sure the proper hitgroups are set for hitboxes.
