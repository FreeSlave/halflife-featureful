---
bookHidden: true
bookToC: false
---

# monster_panthereye

An alien creature cut from Half-Life.

The panthereye uses 3 animation events to perform their attacks:

* Id `1`. Left claw attack. The player's screen is punched to the left when he looks at the panther. Monsters and jumping players are pushed to the right side relatively to the panther.
* Id `2`. Right claw low attack. The player's screen is punched to the right a bit when he looks at the panther. Monsters and jumping players are pushed a bit to the left side relatively to the panther.
* Id `3`. Right claw high attack. Similar to `2`, but the punch and the push are stronger (like in left claw attack, but the opposite side).

Note: each event deals the same amount of damage defined by the `sk_panthereye_dmg_claw` skill value. But one animation may include several events. E.g. `attack_primary` animation in the panthereye model distributed in the demo mod plays events `1` and `2` on different frames.

### Skill variables

* **sk_panthereye_health** - monster's health.
* **sk_panthereye_dmg_claw** - melee damage.

### Default classification

`Alien Monster`

### Soundscripts

* **PantherEye.Idle** - idle sounds.
* **PantherEye.Alert** - alert sounds. Empty by default.
* **PantherEye.Pain** - pain sounds. Empty by default.
* **PantherEye.Die** - death sounds.
* **PantherEye.Attack** - roar on melee attack.
* **PantherEye.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **PantherEye.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
