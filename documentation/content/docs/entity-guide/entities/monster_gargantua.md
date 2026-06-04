---
bookHidden: true
bookToC: false
---

# monster_gargantua

[TWHL](https://twhl.info/wiki/page/monster_gargantua)

### Bugfixes

* Fixed Gargantua not using the stomp attack if he loses the sight on enemy too often.
* Fixed Gargantua not being able to attack its enemy with stomp attack when enemy is hiding in some niche in the wall so Garg can't see it.
* Fixed Gargantua's glowing eye not transitioning across the changelevel.
* Fixed death effects re-playing again on save and reload.

### Skill variables

* **sk_gargantua_health** - monster's health.
* **sk_gargantua_dmg_slash** - melee damage.
* **sk_gargantua_dmg_fire** - fire damage (per 0.1 seconds).
* **sk_gargantua_dmg_stomp** - stomp damage (per 0.1 seconds).
* **sk_gargantua_stomp_initial_speed** - initial stomp speed. Default value is 0.

### Default classification

`Gargantua`

### Default display name

`Gargantua`

### Soundscripts

* **Garg.Stomp** - stomp projectile sound. Must be looped. Emitted from stomp projectile.
* **Garg.AttackHit** - melee attack hit. Derived from **NPC.AttackHit** with decreased pitch.
* **Garg.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss** with decreased pitch.
* **Garg.BeamAttackOn** - flame on sound.
* **Garg.BeamAttackRun** - flame run sound. Expected to be looped.
* **Garg.BeamAttackOff** - flame off sound.
* **Garg.Footstep** - footstep sound.
* **Garg.Idle** - idle sound.
* **Garg.Alert** - alert sound.
* **Garg.Pain** - pain sound.
* **Garg.Attack** - flame attack sound.
* **Garg.StompSound** - stomp launch sound.
* **Garg.Breath** - played on the event 6 in some idle sequences.

### Visuals

* **Garg.Eye** - eye sprite. Note: as brightness of the eye dynamically changes, the alpha of the visual means the maximum brightness.
* **Garg.FlameWide** - wider part of the flame (reddish by default).
* **Garg.FlameNarrow** - narrower part of the flame (bluish by default).
* **Garg.FlameLight** - entity light emitted on each flame update.
* **Garg.Stomp** - sprites emitted by the stomp attack.

Shared visuals:

* **Garg.FlameBase** - alpha and beam scrollrate used by **Garg.FlameBig** and **Garg.FlameSmall**.

### Attacks

* *Melee Attack 1* - melee.
* *Melee Attack 2* - flames.
* *Range Attack 1* - stomp.

### Animation events

* `1` - trace hull attack with right arm. Deals **sk_gargantua_dmg_slash** damage. Plays **Garg.AttackHit** or **Garg.AttackMiss** soundscripts.
* `3` and `4` - play **Garg.Footstep** soundscript and shake effect.
* `5` - create a stomp projectile. Play **Garg.StompSound** soundscript.
* `6` - play **Garg.Breath** soundscript.

### Entity template examples

{{% tabs %}}

{{% tab "Opposing Force Gargantua" %}}
Gargantua in Opposing Force has an increased size to handle the impact from zero-sized projectiles (e.g. [shock_beam]({{< ref shock_beam >}})) properly. This however doesn't let Gargantua to go through some passages because of the increased height.

```json
{
    "monster_gargantua": {
        "size": {
            "mins": [-40, -40, 0],
            "maxs": [40, 40, 214]
        }
    }
}
```
{{% /tab %}}

{{% tab "Trace Attack and Take Damage rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) and [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_gargantua": {
        "trace_attack": [
            {
                "conditions": {
                    "dmg_type": ["energybeam", "crush", "blast"],
                    "dmg_type_match": "none"
                },
                "modifier": {
                    "dmg": "=0"
                },
                "effects": {
                    "ricochet": {
                        "chance": 0.2,
                        "scale": [0.5, 1.5],
                        "certain_on_new_frame": true
                    }
                }
            }
        ],
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["energybeam", "crush", "blast"],
                    "dmg_type_match": "none"
                },
                "modifier": {
                    "dmg": "*0.01"
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
    "monster_gargantua": {
        "check_melee_attack1": {
            "distance": 80,
            "dot": 0.7
        },
        "check_melee_attack2": {
            "distance": 330,
            "dot": 0.8
        },
        "trace_hull_attacks": {
            "1": {
                "distance": 90,
                "height": 64,
                "punchangle": {
                    "pitch": -30,
                    "yaw": -30,
                    "roll": 30
                },
                "knock": {
                    "right": -100
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
