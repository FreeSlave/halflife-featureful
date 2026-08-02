---
bookHidden: true
bookToC: false
---

# monster_flybee

A fast moving flying creature with range attacks. Ported from Half-Life Invasion.

### Skill variables

* **sk_flybee_health** - monster's health.
* **sk_flybee_dmg_kick** - melee damage.
* **sk_flybee_dmg_beam** - beam damage.
* **sk_flybee_dmg_flyball** - damage of every flyball projectile.
* **sk_flybee_maxspeed** - the maximum flight speed a flybee can reach.

### Default classification

`Alien Monster`

### Default display name

`Flybee`

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

### Entity template examples

{{% tabs %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_flybee": {
        "trace_hull_attacks": {
            "1": {
                "distance": 70,
                "punchangle": {
                    "roll": 25
                },
                "damage_info": {
                    "type": ["club"],
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
