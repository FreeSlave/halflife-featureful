---
bookHidden: true
bookToC: false
---

# monster_human_grunt

[TWHL](https://twhl.info/wiki/page/monster_human_grunt)

### Changes

* When human grunt checks for friendly fire he takes into account all allies, not only those who are in the same squad as him.

### Skill variables

* **sk_hgrunt_health** - monster's health.
* **sk_hgrunt_kick** - kick damage.
* **sk_hgrunt_pellets** - the number of shotgun pellets.
* **sk_hgrunt_gspeed** - the grenade speed when it's thrown.
* **sk_9mmAR_bullet** - 9mmAR damage.
* **sk_plr_buckshot** - shotgun pellet damage.

### Default classification

`Human Military`

### Soundscripts

* **HGrunt.Pain** - pain sounds.
* **HGrunt.Die** - death sounds.
* **HGrunt.Reload** - reload sound. Derived from **NPC.Reload**
* **HGrunt.9MM** - burst MP5 shot. Derived from **NPC.9MM**
* **HGrunt.GrenadeLaunch** - MP5 grenade launch. Derived from **NPC.GrenadeLaunch**
* **HGrunt.Shotgun** - shotgun fire. Derived from **NPC.Shotgun**
* **HGrunt.Use** - start following the ally player.
* **HGrunt.UnUse** - stop following the ally player.

### Attacks

* *Melee Attack 1* - melee (kick).
* *Range Attack 1* - primary weapon (MP5 or shotgun).
* *Range Attack 2* - throw grenade or launch AR grenade.

### Animation events

* `2` - reload the weapon clip.
* `3` - kick trace hull attack. Deals **sk_hgrunt_kick** damage.
* `4` - first fire from the primary weapon. Play a weapon fire soundscript (depends on the equipped weapon).
* `5` - second fire from the primary weapon (only MP5).
* `6` - third fire from the primary weapon (only MP5).
* `7` - throw a timed grenade.
* `8` - launch a contact grenade.
* `9` - drop a timed grenade.
* `10` - talk about found enemy that previously has eluded the squad.
* `11` - drop a weapon (on death).
