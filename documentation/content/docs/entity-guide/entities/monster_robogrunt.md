---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) monster_robogrunt

Robogrunts act mostly the same way as human grunts, but they don't bleed and have 80% resistance to every types of damage besides explosion, shock, frost, acid and energy beam.

After death robogrunts play spark effects and explode after 2 seconds.

### Skill variables

* **sk_hgrunt_health** - monster's health.
* **sk_hgrunt_kick** - kick damage.
* **sk_hgrunt_pellets** - the number of shotgun pellets.
* **sk_hgrunt_gspeed** - the grenade speed when it's thrown.
* **sk_9mmAR_bullet** - 9mmAR damage.
* **sk_plr_buckshot** - shotgun pellet damage.
* **sk_rgrunt_explode** - after death explosion damage. If not defined, **sk_plr_hand_grenade** is used.

### Default classification

`Machine`

### Soundscripts

* **RGrunt.Die** - death sound.
* **RGrunt.Spark** - spark sound (when low on health or about to explode)
* **RGrunt.Reload** - reload sound. Derived from **NPC.Reload**
* **RGrunt.9MM** - burst MP5 shot. Derived from **NPC.9MM**
* **RGrunt.GrenadeLaunch** - MP5 grenade launch. Derived from **NPC.GrenadeLaunch**
* **RGrunt.Shotgun** - shotgun fire. Derived from **NPC.Shotgun**
* **RGrunt.Use** - start following the ally player.
* **RGrunt.UnUse** - stop following the ally player.
