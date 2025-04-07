---
bookHidden: true
bookToC: false
---

# monster_alien_grunt

[TWHL](https://twhl.info/wiki/page/monster_alien_grunt)

### Changes

* Restores health when eating.
* Hitting the alien grunt's head now counts as a headshot (in original Half-Life it counts as regular shot). The damage multiplier is still lower than the one from skill.cfg in order to not break a balance much.

### Skill variables

* **sk_agrunt_health** - monster's health.
* **sk_agrunt_dmg_punch** - melee damage.
* [hornet skill variables]({{< ref "hornet/#skill-variables" >}}).

### Default classification

`Alien Military`

### Soundscripts

* **AlienGrunt.Idle** - idle speaking sounds. Alien grunts speak phrases consisting of short words.
* **AlienGrunt.Alert** - alert sounds.
* **AlienGrunt.Pain** - pain sounds.
* **AlienGrunt.Die** - death sounds.
* **AlienGrunt.Attack** - played on melee attack.
* **AlienGrunt.LeftFoot** - left footstep.
* **AlienGrunt.RightFoot** - right footstep.
* **AlienGrunt.Fire** - firing a hornet.
* **AlienGrunt.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **AlienGrunt.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **AlienGrunt.Use** - start following the ally player. Derived from **AlienGrunt.Idle**
* **AlienGrunt.UnUse** - stop following the ally player. Derived from **AlienGrunt.Alert**
* [hornet soundscripts]({{< ref "hornet/#soundscripts" >}}).

### Visuals

* **AlienGrunt.MuzzleFlash** - temporary flash sprite played when firing a hornet.
* [hornet visuals]({{< ref "hornet/#visuals" >}}).

### Attacks

* *Melee Attack 1* - melee.
* *Range Attack 1* - fire hornets.

### Animation events

* `1` - `5` - spawn a [hornet]({{< ref hornet >}}) projectile, with **AlienGrunt.MuzzleFlash** visual and **AlienGrunt.Fire** soundscript.
* `10` - play **AlienGrunt.LeftFoot** soundscript.
* `11` - play **AlienGrunt.RightFoot** soundscript.
* `12` - trace hull attack with left arm. Deals **sk_agrunt_dmg_punch** damage. Plays **AlienGrunt.AttackHit** or **AlienGrunt.AttackMiss** soundscripts.
* `13` - trace hull attack with right arm. Deals **sk_agrunt_dmg_punch** damage. Plays **AlienGrunt.AttackHit** or **AlienGrunt.AttackMiss** soundscripts.
