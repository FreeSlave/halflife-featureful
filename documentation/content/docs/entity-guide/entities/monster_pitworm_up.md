---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_pitworm_up

The boss worm creature from Opposing Force.

### Skill variables

* **sk_pitworm_health** - monster's health.
* **sk_pitworm_dmg_swipe** - melee damage.
* **sk_pitworm_dmg_beam** - beam damage (per 0.1 seconds).

### Default classification

`Race X Shock`

### Default display name

`Pit Worm`

### Soundscripts

* **PitWorm.HitGround** - hitting the ground (any material).
* **PitWorm.Angry** - not used currently.
* **PitWorm.Blast** - eye laser beam attack.
* **PitWorm.Swipe** - melee attack.
* **PitWorm.Shoot** - beam sounds.
* **PitWorm.Idle** - idle sounds.
* **PitWorm.Alert** - alert sounds.
* **PitWorm.Pain** - pain sounds.
* **PitWorm.Die** - death sounds.
* **PitWorm.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**

### Visuals

* **PitWorm.Beam** - eye laser beam.
* **PitWorm.EyeGlow** - eye glow during the laser beam attack.
* **PitWorm.EyeLight** - entity light produce during the laser beam attack.

### Reaction to use-type

If called with `Off` or `Toggle`, PitWorm will start playing its dying sequence.
