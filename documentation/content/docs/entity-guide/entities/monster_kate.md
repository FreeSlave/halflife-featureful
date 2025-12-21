---
bookHidden: true
bookToC: false
---

# monster_kate

Kate - a companion from Azure Sheep. Acts like [monster_barney]({{< ref monster_barney >}}), but also can perform kick attack.

Note: you'll need to add sentences with `KA_` prefix to your `sentences.txt` and put corresponding sounds to your mod directory.

### Skill variables

* **sk_kate_health** - monster's health.
* **sk_9mm_bullet** - handgun bullet damage.
* **sk_hgrunt_kick** - kick/punch damage.

### Default classification

`Player Ally`

### Soundscripts

* **Kate.Pain** - pain sound.
* **Kate.Die** - death sound.
* **Kate.FirePistol** - firing a handgun.
* **Kate.Kick** - kick sound.
* **Kate.Punch** - punch sound.

### Attacks

* *Melee Attack 1* - melee (kick).
* *Range Attack 1* - fire a gun.

### Animation events

* `6` - kick/punch trace hull attack. Deals **sk_hgrunt_kick** damage.

### Entity template examples

{{% tabs %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_kate": {
        "check_melee_attack1": {
            "distance": 64,
            "dot": 0.7
        },
        "trace_hull_attacks": {
            "6": {
                "distance": 70,
                "punchangle": {
                    "pitch": 5
                },
                "knock": {
                    "forward": -100,
                    "up": 50
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
