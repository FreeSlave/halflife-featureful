---
bookHidden: true
bookToC: false
---

# ![](/images/alpha.png) monster_panthereye

An alien creature cut from Half-Life.

### Skill variables

* **sk_panthereye_health** - monster's health.
* **sk_panthereye_dmg_claw** - melee damage.

### Default classification

`Alien Monster`

### Default display name

`Panthereye`

### Soundscripts

* **PantherEye.Idle** - idle sounds.
* **PantherEye.Alert** - alert sounds. Empty by default.
* **PantherEye.Pain** - pain sounds. Empty by default.
* **PantherEye.Die** - death sounds.
* **PantherEye.Attack** - roar on melee attack.
* **PantherEye.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **PantherEye.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**

### Attacks

* *Melee Attack 1* - melee.
* *Range Attack 1* - leap attack.

### Animation events

* `1` - Left claw attack. The player's screen is punched to the left when he looks at the panther. Monsters and jumping players are pushed to the right side relatively to the panther.
* `2` - Right claw low attack. The player's screen is punched to the right a bit when he looks at the panther. Monsters and jumping players are pushed a bit to the left side relatively to the panther.
* `3` - Right claw high attack. Similar to `2`, but the punch and the push are stronger (like in left claw attack, but the opposite side).

Note: each event deals the same amount of damage defined by the **sk_panthereye_dmg_claw** skill value. But one animation may include several events. E.g. `attack_primary` animation in the panthereye model distributed in the demo mod plays events `1` and `2` on different frames.

### Entity template examples

{{% tabs %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_panthereye": {
        "check_melee_attack1": {
            "distance": 84,
            "dot": 0.7
        },
        "trace_hull_attacks": {
            "1": {
                "distance": 84,
                "punchangle": {
                    "pitch": 5,
                    "roll": 18
                },
                "knock": {
                    "right": 100,
                    "forward": -50,
                    "up": 50
                }
            },
            "2": {
                "distance": 84,
                "punchangle": {
                    "pitch": 5,
                    "roll": -9
                },
                "knock": {
                    "right": -25,
                    "forward": -25,
                    "up": 25
                }
            },
            "3": {
                "distance": 84,
                "punchangle": {
                    "pitch": 5,
                    "roll": -18
                },
                "knock": {
                    "right": -100,
                    "forward": -50,
                    "up": 50
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
