---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_alien_voltigore

### Changes

* Restores health when eating.
* Does not stop range attack when enemy hides behind a cover to prevent abusing the interruption of long animation.

### Skill variables

* **sk_voltigore_health** - monster's health.
* **sk_voltigore_dmg_punch** - melee damage.
* [charged_bolt]({{< ref charged_bolt >}}) skill variables.
* **sk_voltigore_dmg_explode** - explosion damage. If not defined, **sk_voltigore_dmg_beam** is used.

### Default classification

`Race X Shock`

### Default display name

`Voltigore`

### Soundscripts

* **Voltigore.Idle** - idle sounds.
* **Voltigore.Alert** - alert sounds.
* **Voltigore.Pain** - pain sounds.
* **Voltigore.Die** - death sounds.
* **Voltigore.Footstep** - footstep sounds.
* **Voltigore.BeamAttack** - starting beam attack.
* **Voltigore.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Voltigore.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**

### Visuals

* **Voltigore.ChargeBeam** - beams played while preparing the range attack.
* **Voltigore.DeathBeam** - beams played on death.
* [charged_bolt visuals]({{< ref "charged_bolt/#visuals" >}})

Shared visuals:

* **Voltigore.Beam** - sprite, color and alpha properties shared by **Voltigore.ChargeBeam** and **Voltigore.DeathBeam**.

### Attacks

* *Melee Attack 1* - melee.
* *Range Attack 1* - charged bolt attack.

### Animation events

* `2` - launch a charged bolt.
* `12` - trace hull attack with both claws. Deals **sk_voltigore_dmg_punch** damage. Plays **Voltigore.AttackHit** or **Voltigore.AttackMiss** soundscripts.
* `13` - trace hull attack with one claw. Deals **sk_voltigore_dmg_punch** damage. Plays **Voltigore.AttackHit** or **Voltigore.AttackMiss** soundscripts.

### Entity template examples

{{% tabs %}}

{{% tab "Smaller size" %}}
Opposing Force voltigore geometry is kinda big which makes it harder for voltigores to navigate on the levels. This template makes them less in size (this doesn't affect the model scale, just the size of the boundbox).
```json
{
    "monster_alien_voltigore": {
        "size": {
            "mins": [-40, -40, 0],
            "maxs": [40, 40, 85]
        }
    }
}
```
{{% /tab %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_alien_voltigore": {
        "check_melee_attack1": {
            "distance": 128,
            "dot": 0.6
        },
        "trace_hull_attacks": {
            "12": {
                "distance": 128,
                "punchangle": {
                    "pitch": 20,
                },
                "knock": {
                    "forward": 150,
                    "up": 100
                },
                "spawn_blood": true,
                "damage_info": {
                    "type": ["club"],
                }
            },
            "13": {
                "distance": 128,
                "punchangle": {
                    "pitch": 15,
                    "roll": -15
                },
                "knock": {
                    "right": -150,
                    "up": 100
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
