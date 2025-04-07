---
bookHidden: true
bookToC: false
---

# monster_flybee

The fast moving flying creature with range attacks. Ported from Half-Life Invasion.

### Skill variables

* **sk_flybee_health** - monster's health.
* **sk_flybee_dmg_kick** - melee damage.
* **sk_flybee_dmg_beam** - beam damage.
* **sk_flybee_dmg_flyball** - damage of every flyball projectile.

### Default classification

`Alien Monster`

### Soundscripts

* **Flybee.Idle** - idle sounds.
* **Flybee.Alert** - alert sounds.
* **Flybee.Pain** - pain sounds.
* **Flybee.Die** - death sounds.
* **Flybee.Attack** - starting chasing the enemy.
* **Flybee.Bite** - melee attack.
* **Flybee.Beam** - beam attack.
* **Flybee.Electro** - ball projectile impact. Emitted from the projectile.

{{% hint info %}}
The soundscripts use ichthyosaur sounds by default.
{{% /hint %}}

### Visuals

* **Flybee.Ball** - the flyball sprite.
* **Flybee.BallTrail** - the trailing sprite behind the flyball.
* **Flybee.ZapBeam** - zap attack beam.
* **Flybee.ZapBeamAlt** - zap attack alternative beam. Same as **Flybee.ZapBeam** but different default color.
* **Flybee.Zap** - zap attack sprite.
* **Flybee.ZapWave** - beam settings for the zap attack shockwave.

### Attacks

* *Melee Attack 1* - bite.
* *Range Attack 1* - projectiles.
* *Range Attack 2* - beam.

### Animation events

* `1` - bite trace hull attack. Plays **Flybee.Bite** soundscript.
* `2` - fire 4 electric balls.
* `3` - beam attack.
