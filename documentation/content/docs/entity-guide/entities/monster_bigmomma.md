---
bookHidden: true
bookToC: false
---

# monster_bigmomma (aka Gonarch)

[TWHL](https://twhl.info/wiki/page/monster_bigmomma)

### Skill variables

* **sk_bigmomma_health_factor** - health factor for health in `info_bigmomma`.
* **sk_bigmomma_dmg_slash** - melee damage.
* **sk_bigmomma_dmg_blast** - acid mortar damage.
* **sk_bigmomma_radius_blast** - acid mortar blast radius.

### Default classification

`Alien Monster`

### Soundscripts

* **BigMomma.Alert** - played on the event 14 in the *angry* sequences.
* **BigMomma.Pain** - getting hit.
* **BigMomma.Die** - played on the event 6 in the death sequence.
* **BigMomma.Attack** - melee attack.
* **BigMomma.Birth** - spawning a child. Played on the event 17 in the *spawn* sequence.
* **BigMomma.LayHeadcrab** - creating a baby headcrab.
* **BigMomma.ChildDie** - played when child dies.
* **BigMomma.Sack** - plays occasionally when idle.
* **BigMomma.LaunchMortar** - launching a projectile.
* **BigMomma.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* [bmortar soundscripts]({{< ref "bmortar#soundscripts" >}})

### Visuals

* [bmortar visuals]({{< ref "bmortar#visuals" >}})
* **BigMomma.MortarSpray** - sprite spray that comes from Big Momma when launching the spit projectile and when this projectile touches the wall.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack and Take Damage rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) and [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_bigmomma": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": [1],
                    "invert_hitgroup_check": true
                },
                "modifier": {
                    "dmg": "=0.1"
                },
                "effects": {
                    "ricochet": {
                        "certain_on_new_frame": true,
                        "chance": 0.1,
                        "scale": [1.0, 2.0]
                    }
                }
            }
        ],
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["acid"],
                    "attacker": {
                        "attack_affinity": ["friendly", "self", "neutral"]
                    }
                },
                "modifier": {
                    "dmg": "=0"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
