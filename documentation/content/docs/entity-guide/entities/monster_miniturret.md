---
bookHidden: true
bookToC: false
---

# monster_miniturret

[TWHL](https://twhl.info/wiki/page/monster_miniturret)

### Changes

* `Death` and `50% Health Remaining` trigger conditions now work for turrets.

### Skill variables

* **sk_miniturret_health** - monster's health.
* **sk_9mm_bullet** - bullet damage.

### Default classification

`Machine`

### Soundscripts

* **Turret.Alert** - when detected an enemy.
* **Turret.Deploy** - deploy sound.
* **Turret.Undeploy** - retire sound.
* **Turret.Ping** - pinging noise. Played every second while searching for enemy.
* **Turret.Spinup** - a looping spin sound.
* **Turret.Die** - death sound.
* **MiniTurret.Shoot** - single shot. Derived from **NPC.9MMSingle**
