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
* **Zombie.Die** - death sounds. Empty by default.
* **Zombie.Attack** - plays occasionally on the slash attack.
* **Zombie.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Zombie.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**

### Attacks

* *Melee Attack 1* - melee.

### Animation events

* `1` - trace hull attack with right arm. Deals **sk_zombie_dmg_one_slash** damage. Plays **Zombie.AttackHit** or **Zombie.AttackMiss**. Plays **Zombie.Attack** with 50% chance.
* `2` - trace hull attack with left arm. Deals **sk_zombie_dmg_one_slash** damage. Plays **Zombie.AttackHit** or **Zombie.AttackMiss**. Plays **Zombie.Attack** with 50% chance.
* `3` - trace hull attack with both arms. Deals **sk_zombie_dmg_both_slash** damage. Plays **Zombie.AttackHit** or **Zombie.AttackMiss**. Plays **Zombie.Attack** with 50% chance.

### Entity template examples

{{% tabs %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_zombie": {
        "check_melee_attack1": {
            "distance": 64,
            "dot": 0.7
        },
        "trace_hull_attacks": {
            "1": {
                "distance": 70,
                "punchangle": {
                    "pitch": 5,
                    "roll": -18
                },
                "knock": {
                    "right": -100
                }
            },
            "2": {
                "distance": 70,
                "punchangle": {
                    "pitch": 5,
                    "roll": 18
                },
                "knock": {
                    "right": 100
                }
            },
            "3": {
                "distance": 70,
                "punchangle": {
                    "pitch": 5
                },
                "knock": {
                    "forward": -100
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
