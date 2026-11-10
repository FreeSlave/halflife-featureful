---
bookHidden: true
bookToC: false
---

# monster_bigmomma (aka Gonarch)

[TWHL](https://twhl.info/wiki/page/monster_bigmomma)

### Skill variables

* **sk_bigmomma_health_factor** - health factor for health in `info_bigmomma`.
* **sk_bigmomma_dmg_slash** - melee damage.
* [bmortar]({{< ref bmortar >}}) skill variables.

### Default classification

`Alien Monster`

### Default display name

`Big Momma`

### Soundscripts

* **BigMomma.Alert** - played on the event 14 in the *angry* sequences.
* **BigMomma.Pain** - getting hit.
* **BigMomma.Die** - played on the event 6 in the death sequence.
* **BigMomma.Attack** - melee attack.
* **BigMomma.Birth** - spawning a child. Played on the event 17 in the `spawn` sequence.
* **BigMomma.LayHeadcrab** - creating a baby headcrab.
* **BigMomma.ChildDie** - played when child dies.
* **BigMomma.Sack** - plays occasionally when idle.
* **BigMomma.LaunchMortar** - launching a projectile.
* **BigMomma.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **BigMomma.Use** - start following the ally player. Empty by default.
* **BigMomma.UnUse** - stop following the ally player. Empty by default.
* [bmortar soundscripts]({{< ref "bmortar#soundscripts" >}})
* **BigMomma.FootstepLeft**
* **BigMomma.FootstepRight**

### Visuals

* [bmortar visuals]({{< ref "bmortar#visuals" >}})
* **BigMomma.MortarSpray** - sprite spray that comes from Big Momma when launching the spit projectile and when this projectile touches the wall.

### Attacks

* *Melee Attack 1* - leg attack.
* *Melee Attack 2* - spawn a child.
* *Range Attack 1* - spit a mortar projectile.

### Animation events

* `1` - footstep left. Plays **BigMomma.FootstepLeft** soundscript.
* `2` - footstep right. Plays **BigMomma.FootstepRight** soundscript.
* `3` - footstep back left. Plays **BigMomma.FootstepLeft** soundscript.
* `4` - footstep back right. Plays **BigMomma.FootstepRight** soundscript.
* `5` - play **BigMomma.Sack** soundscript.
* `6` - play **BigMomma.Die** soundscript.
* `8` - attack with left leg.
* `9` - attack with right leg.
* `10` - attack with both legs.
* `11` - launch [bmortar]({{< ref bmortar >}}) projectile. Play **BigMomma.LaunchMortar** soundscript.
* `12` - lay a child (babycrab). Play **BigMomma.LayHeadcrab** soundscript.
* `13` - jump forward (used in `jump` animation sequence).
* `14` - play **BigMomma.Alert** soundscript.
* `15` - play **BigMomma.Pain** soundscript.
* `16` - play **BigMomma.Attack** soundscript.
* `17` - play **BigMomma.Birth** soundscript.
* `50` - fire the `Fire on approach` for the current `info_bigmomma`. Used in `breakwall` animation sequence.

{{% hint info %}}
While events `8`, `9` and `10` technically don't do a normal trace hull attack, the attack properties are still can be changed via the [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}).

Instead of a normal trace hull attack Big Momma searches for the target in the parallelepipedal volume in front.
{{% /hint %}}

### Entity template examples

{{% tabs %}}

{{% tab "Spawning grown headcrabs"%}}

```json
{
    "monster_bigmomma": {
        "children": {
            "classname": "monster_headcrab",
        }
    }
}
```

{{% /tab %}}

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

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_bigmomma": {
        "check_melee_attack1": {
            "distance": 170,
            "dot": 0.7
        },
        "trace_hull_attacks": {
            "8": {
                "distance": 128,
                "punchangle": {
                    "pitch": 15
                },
                "knock": {
                    "forward": 150,
                    "right": -200,
                    "up": 250
                },
                "damage_info": {
                    "type": ["slash", "crush"]
                }
            },
            "9": {
                "distance": 128,
                "punchangle": {
                    "pitch": 15
                },
                "knock": {
                    "forward": 150,
                    "right": 200,
                    "up": 250
                },
                "damage_info": {
                    "type": ["slash", "crush"]
                }
            },
            "10": {
                "distance": 128,
                "punchangle": {
                    "pitch": 15
                },
                "knock": {
                    "forward": 220,
                    "up": 250
                },
                "damage_info": {
                    "type": ["slash", "crush"]
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% tab "Gib disposition" %}}

The [gib disposition]({{< ref "entity-templates/#gib_disposition" >}}) that utilizes the bigmomma-specific gibs instead of base alien gibs (in Half-Life campaign the bigmomma gibs are spawned by mapping, via [env_shooter]({{< ref env_shooter >}})).

```json
{
    "monster_bigmomma": {
        "gib_visual": {
            "model": "models/big_momgibs.mdl"
        },
        "gib_disposition": [
            {
                "count": 1,
                "body": 0
            },
            {
                "count": 1,
                "body": 1
            },
            {
                "count": 4,
                "body": 2
            }
        ]
    }
}
```

{{% /tab %}}

{{% /tabs %}}
