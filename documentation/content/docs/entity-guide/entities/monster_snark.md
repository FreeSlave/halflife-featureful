---
bookHidden: true
bookToC: false
---

# monster_snark

[TWHL](https://twhl.info/wiki/page/monster_snark)

### Skill variables

* **sk_snark_health** - monster's health.
* **sk_snark_dmg_bite** - bite damage.
* **sk_snark_dmg_pop** - the base explosion damage.
* **sk_snark_add_dmg_pop** - each bite insreases the explosion damage by the this number. By default this is the same as **sk_snark_dmg_pop**.
* **sk_snark_max_dmg_pop** - maximum explosion damage snark can reach by biting the prey. 0 means no limit. This is 0 by default.
* **sk_snark_lifespan** - maximum monster's lifespan before it explodes. Negative values mean an infinite lifespan. Default value is 15.
* **sk_snark_jump_delay** - delay (in seconds) between snark's jumps when it chases the enemy. Default value is 2.
* **sk_snark_jump_speed** - the speed the snark gains when jumping at enemy. Default value is 300.

### Default classification

`Snark`

### Soundscripts

* **Snark.Die** - death sound.
* **Snark.Gibbed** - gibbing sound.
* **Snark.Squeak** - squeek before blowing up.
* **Snark.Deploy** - bite sound. The pitch gets higher as snark gets closer to the detonation time.
* **Snark.Bounce** - bouncing off the ground. The pitch gets higher as snark gets closer to the detonation time.
