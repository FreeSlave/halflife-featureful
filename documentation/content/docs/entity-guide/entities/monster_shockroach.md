---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_shockroach

### Skill variables

* **sk_shockroach_health** - monster's health.
* **sk_shockroach_dmg_bite** - bite damage.
* **sk_shockroach_lifespan** - lifespan after spawn, in seconds.

### Default classification

`Alien Prey`

Note: this makes pitdrones attack shockroaches. The behavior retained from Opposing Force.

### Soundscripts

* **Shockroach.Idle** - idle sounds.
* **Shockroach.Alert** - alert sounds.
* **Shockroach.Pain** - pain sounds.
* **Shockroach.Die** - death sounds.
* **Shockroach.Leap** - jump sound.
* **Shockroach.Attack** - attack sound on jump.
* **Shockroach.Bite** - hit something.

Note: the shockroach uses a lot of sounds in the sequence events. These sounds can't be changed via soundscripts.

### Animation events

* `2` - occasionally play **Shockroach.Attack** soundscript.
