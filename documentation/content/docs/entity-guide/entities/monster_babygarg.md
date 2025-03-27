---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) monster_babygarg

### Skill variables

* **sk_babygargantua_health** - monster's health.
* **sk_babygargantua_dmg_slash** - melee damage.
* **sk_babygargantua_dmg_fire** - fire damage (per 0.1 seconds).
* **sk_babygargantua_dmg_stomp** - stomp damage (per 0.1 seconds).

### Default classification

`Gargantua`

### Soundscripts

* **BabyGarg.Stomp** - stomp projectile sound. Must be looped. Emitted from stomp projectile. Derived from **Garg.Stomp** with increased pitch.
* **BabyGarg.AttackHit** - melee attack hit. Derived from **NPC.AttackHit** with decreased pitch.
* **BabyGarg.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss** with decreased pitch.
* **BabyGarg.BeamAttackOn** - flame on sound. Derived from **Garg.BeamAttackOn** with decreased volume.
* **BabyGarg.BeamAttackRun** - flame run sound. Derived from **Garg.BeamAttackRun** with decreased volume.
* **BabyGarg.BeamAttackOff** - flame off sound. Derived from **Garg.BeamAttackOff** with decreased volume.
* **BabyGarg.Footstep** - footstep sound.
* **BabyGarg.Idle** - idle sound.
* **BabyGarg.Alert** - alert sound.
* **BabyGarg.Pain** - pain sound.
* **BabyGarg.Attack** - occasional roar played on the flame attack.
* **BabyGarg.StompSound** - stomp launch sound.
* **BabyGarg.Breath** - played on the event 6 in some idle sequences.

### Visuals

* **BabyGarg.Eye** - eye sprite. Note: as brightness of the eye dynamically changes, the alpha of the visual means the maximum brightness.
* **BabyGarg.FlameBig** - wider part of the flame (reddish by default). Derived from **Garg.FlameBig** with the smaller beam width.
* **BabyGarg.FlameSmall** - narrower part of the flame (bluish by default). Derived from **Garg.FlameSmall** with the smaller beam width.
* **BabyGarg.FlameLight** - entity light emitted on each flame update. Derived from **Garg.FlameLight** without changes.
* **BabyGarg.Stomp** - sprites emitted by the stomp attack.
