---
bookHidden: true
bookToC: false
---

# monster_sentry

[TWHL](https://twhl.info/wiki/page/monster_sentry)

### Changes

* `Death` and `50% Health Remaining` trigger conditions now work for turrets.
* Can be correctly spawned via [monstermaker]({{< ref monstermaker >}}) (notifies the monstermaker about its death). If monstermaker doesn't set the children's name, the turret is automatically set to the autostart mode. If the children's name is set the spawned turret can be activated by a trigger using this name. Just like with others monsters, if the monstermaker spawns more than 1 monster the turrets that have been killed fade away.

### Skill variables

* **sk_sentry_health** - monster's health.
* **sk_9mmAR_bullet** - bullet damage.

### Default classification

`Machine`

### Default display name

`Sentry Turret`

### Soundscripts

* **Turret.Alert** - when detected an enemy.
* **Turret.Deploy** - deploy sound.
* **Turret.Undeploy** - retire sound.
* **Turret.Ping** - pinging noise. Played every second while searching for enemy.
* **Turret.Spinup** - a looping spin sound.
* **Turret.Die** - death sound.
* **Sentry.Shoot** - single shot. Derived from **NPC.9MMSingle**
