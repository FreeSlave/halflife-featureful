---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_alien_babyvoltigore

Baby version of [monster_alien_voltigore]({{< ref monster_alien_voltigore >}}). Unlike adult, can't use charged bolt attack.

### Skill variables

* **sk_babyvoltigore_health** - monster's healh.
* **sk_babyvoltigore_dmg_punch** - melee damage.

### Default classification

`Race X Shock`

### Soundscripts

* **BabyVoltigore.Idle** - idle sounds. Derived from **Voltigore.Idle**
* **BabyVoltigore.Alert** - alert sounds. Derived from **Voltigore.Alert**
* **BabyVoltigore.Pain** - pain sounds. Derived from **Voltigore.Pain**
* **BabyVoltigore.Die** - death sounds. Derived from **Voltigore.Die**
* **BabyVoltigore.Footstep** - footstep sounds. Derived from **Voltigore.Footstep**
* **BabyVoltigore.Attack** - starting melee attack.
* **BabyVoltigore.AttackHit** - melee attack hit. Derived from **NPC.AttackHit** with increased pitch.
* **BabyVoltigore.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss** with increased pitch.

### Entity template examples

{{% tabs %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_alien_babyvoltigore": {
        "check_melee_attack1": {
            "distance": 64,
            "dot": 0.6
        },
        "trace_hull_attacks": {
            "12": {
                "distance": 64,
                "punchangle": {
                    "pitch": 15,
                },
                "knock": {
                    "forward": 100,
                    "up": 50
                },
                "spawn_blood": true,
                "damage_info": {
                    "type": ["club"],
                }
            },
            "13": {
                "distance": 64,
                "punchangle": {
                    "pitch": 10,
                    "roll": -10
                },
                "knock": {
                    "right": -100,
                    "up": 50
                },
                "spawn_blood": true,
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
