---
bookHidden: true
bookToC: false
---

# ![](/images/alpha.png) monster_archer

An archer fish - an alien creature cut from Half-Life. Designed similar to how it's made in the mods Thrill and Reprocessing, but also supports the original model available in Half-Life.

The monster utilizes the melee (bite) attack and the short range lightning attack.

### Skill variables

* **sk_archer_health** - monster's health.
* **sk_archer_dmg_bite** - bite damage.
* **sk_archer_dmg_zap** - zap damage (unlike Thrill where the damage is applied twice, this will be applied once).
* **sk_archer_delay_zap** - the minimum delay before the next attack after using a zap attack. This is `[2.5, 7.0]` range by default.

### Default classification

`Alien Monster`

### Default display name

`Archer`

### Soundscripts

* **Archer.Idle** - idle sounds. Empty by default.
* **Archer.Alert** - alert sounds. Also played when Archer is charging at its enemy. Uses unused ichthyosaur alert sound by default (with slightly higher pitch).
* **Archer.Pain** - pain sounds. Uses unused ichthyosaur pain sound by default (with slightly higher pitch).
* **Archer.Die** - death sounds. Uses unused ichthyosaur death sounds by default (with slightly higher pitch).
* **Archer.Bite** - trying to bite an enemy. Uses unused bullsquid bite sound by default.
* **Archer.ZapPowerup** - starting zap attack.
* **Archer.Electro** - played at the end of ligtning.

### Visuals

* **Archer.ZapBeam** - zap beam.

### Attacks

* *Melee Attack 1* - bite.
* *Range Attack 1* - beam attack (if the sequence with such activity is missing it uses `shoot` animation).

### Animation events

* `1` - bite attack.
* `2` - zap attack.
* `3` - same as `2`, to support the model from Thrill and Reprocessing.
