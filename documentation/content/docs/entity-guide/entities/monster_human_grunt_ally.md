---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_human_grunt_ally

### Changes

* When human grunt checks for friendly fire he takes into account all allies, not only those who are in the same squad as him.

### Skill variables

* **sk_hgrunt_ally_health** - monster's health. If not defined, **sk_hgrunt_health** is used.
* **sk_hgrunt_ally_kick** - kick damage. If not defined, **sk_hgrunt_kick** is used.
* **sk_hgrunt_ally_pellets** - the number of shotgun pellets. If not defined, **sk_hgrunt_pellets** is used.
* **sk_hgrunt_ally_gspeed** - the grenade speed when it's thrown. If not defined, **sk_hgrunt_gspeed** is used.
* **sk_9mmAR_bullet** - 9mmAR damage.
* **sk_buckshot** - shotgun pellet damage. If not defined, **sk_plr_buckshot** is used.
* **sk_556_bullet** - M249 damage. If not defined, **sk_plr_556_bullet** is used.

### Default classification

`Player Ally Military`

### Soundscripts

* **HGruntAlly.Pain** - pain sounds.
* **HGruntAlly.Die** - death sounds.
* **HGruntAlly.CallMedic** - call medic when low on health.
* **HGruntAlly.Reload** - reload sound. Derived from **NPC.Reload**
* **HGruntAlly.9MM** - burst MP5 shot. Derived from **NPC.9MM**
* **HGruntAlly.GrenadeLaunch** - MP5 grenade launch. Derived from **NPC.GrenadeLaunch**
* **HGruntAlly.Shotgun** - shotgun fire. Derived from **NPC.Shotgun**
* **HGruntAlly.M249** - M249 fire. Derived from **NPC.M249**
* **HGruntAlly.ReloadM249** - M249 reload.
