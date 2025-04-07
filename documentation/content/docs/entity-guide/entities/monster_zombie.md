---
bookHidden: true
bookToC: false
---

# monster_zombie

[TWHL](https://twhl.info/wiki/page/monster_zombie)

### Skill variables

* **sk_zombie_health** - monster's health.
* **sk_zombie_dmg_one_slash** - one arm damage.
* **sk_zombie_dmg_both_slash** - two arms damage.

### Default classification

`Alien Monster`

### Soundscripts

* **Zombie.Idle** - idle sounds.
* **Zombie.Alert** - alert sounds.
* **Zombie.Pain** - pain sounds.
* **Zombie.Die** - death sounds.
* **Zombie.Attack** - plays occasionally on the slash attack.
* **Zombie.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Zombie.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**

### Attacks

* *Melee Attack 1* - melee.

### Animation events

* `1` - trace hull attack with right arm. Deals **sk_zombie_dmg_one_slash** damage. Plays **Zombie.AttackHit** or **Zombie.AttackMiss**. Plays **Zombie.Attack** with 50% chance.
* `2` - trace hull attack with left arm. Deals **sk_zombie_dmg_one_slash** damage. Plays **Zombie.AttackHit** or **Zombie.AttackMiss**. Plays **Zombie.Attack** with 50% chance.
* `3` - trace hull attack with both arms. Deals **sk_zombie_dmg_both_slash** damage. Plays **Zombie.AttackHit** or **Zombie.AttackMiss**. Plays **Zombie.Attack** with 50% chance.
