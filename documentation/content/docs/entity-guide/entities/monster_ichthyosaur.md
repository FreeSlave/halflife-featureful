---
bookHidden: true
bookToC: false
---

# monster_ichthyosaur

[TWHL](https://twhl.info/wiki/page/monster_ichthyosaur)

### Skill variables

* **sk_ichthyosaur_health** - monster's health.
* **sk_ichthyosaur_shake** - bite damage.

### Default classification

`Alien Monster`

### Soundscripts

* **Ichthyosaur.Idle** - idle sounds.
* **Ichthyosaur.Alert** - alert sounds.
* **Ichthyosaur.Pain** - pain sounds.
* **Ichthyosaur.Die** - death sounds.
* **Ichthyosaur.Attack** - starting chasing the enemy.
* **Ichthyosaur.Bite** - trying to bite an enemy.

### Attacks

* *Melee Attack 1* - bite.

### Animation events

* `1` and `2` - bite enemy. While these are not technically trace hull attacks, parameters like knock, punch and damage info still can be overriden via `"trace_hull_attacks"` property in the entity template. Plays **Ichthyosaur.Bite** soundscript.
