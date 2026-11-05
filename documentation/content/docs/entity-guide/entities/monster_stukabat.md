---
bookHidden: true
bookToC: false
---

# ![](/images/alpha.png) monster_stukabat

A flying alien creature cut from Half-Life. Designed similar to how it's made in Sven Co-op, but with a range attack from Thrill/Reprocessing. It works with both original Half-Life model and the one from Thrill/Reprocessing.

The stukabat uses a poisonous claw (bite) attack at close range and shoots poisonous [bomb]({{< ref stukabomb >}}) projectiles. The timed poison effect on both attacks ignore armor but can't do lethal damage.

After being idle for some time the stukabats will try to find a resting place on the ceiling. Unlike Sven Co-op, the stukabats won't try to land on the sky "ceiling". To find a place for rest the stukabat uses nodes - put air nodes somewhere under the ceiling so stukabats were able to find them. If stukabat can't find any perching position he will fly between nodes until he finds a good spot.

While on the ceiling stukabats can look at all directions but the look distance is greatly reduced.

### Skill variables

* **sk_stuka_health** - monsters' health.
* **sk_stuka_dmg_slash** - melee damage.
* [stukabomb]({{< ref stukabomb >}}) skill variables.

### Default classification

`Alien Monster`

### Default display name

`Stukabat`

### Soundscripts

* **Stukabat.Idle** - idle sounds. Empty by default.
* **Stukabat.IdlePerched** - idle sounds when stukabat is on the ceiling. Empty by default.
* **Stukabat.Alert** - alert sounds. Empty by default.
* **Stukabat.Pain** - pain sounds. Empty by default.
* **Stukabat.Die** - death sounds. Empty by default.
* **Stukabat.Attack** - range attack sound.
* **Stukabat.AttackHit** - melee attack hit. By default it uses **headcrab/hc_headbite.wav** with a high pitch.
* **Stukabat.AttackMiss** - melee attack miss.
* **Stukabat.Flap** - wing flap sound.
* [stukabomb soundscripts]({{< ref "stukabomb/#soundscripts" >}})

### Visuals

* [stukabomb visuals]({{< ref "stukabomb/#visuals" >}})

The particle effect used when the stukabat spits a bomb is currently not configurable.

### Attacks

* *Range Attack 1* - bomb.
* *Raneg Attack 2* - melee.

### Animation events

* `1` - spawn a bomb projectile.
* `2` - do a trace hull attack. The original Half-Life model doesn't have this event, it's simulated via animation frames. With Thrill/Reprocessing model it's getting used by regular rules.
* `8` - doesn't do anything (in original Valve idea it was used to change the flying monster's speed).
* `9` - play **Stukabat.Flap**.
