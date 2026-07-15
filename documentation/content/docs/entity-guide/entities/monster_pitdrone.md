---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_pitdrone

### Changes

* Restores health when eating.
* When spawning via monstermaker, pitdrone always spawns with spikes loaded. In Opposing Force pitdrones spawned without spikes and unable to reload.

### Skill variables

* **sk_pitdrone_health** - monster's health.
* **sk_pitdrone_dmg_bite** - both claw attack damage. This is dealt two times during the attack due to animation events.
* **sk_pitdrone_dmg_whip** - single claw attack damage.
* **sk_pitdrone_dmg_spit** - spike damage.

{{% hint info %}}
The skill variable names might be confusing as pitdrones don't bite and don't have a tail to whip, but these are the names that are used in Opposing Force.
{{% /hint %}}

### Default classification

`Race X Predator`

### Default display name

`Pit Drone`

### Soundscripts

* **PitDrone.Idle** - idle sounds.
* **PitDrone.Alert** - alert sounds.
* **PitDrone.Pain** - pain sounds.
* **PitDrone.Die** - death sounds.
* **PitDrone.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **PitDrone.AttackHit** - hitting something when attacking with both claws.
* **PitDrone.Use** - start following the ally player. Derived from **PitDrone.Idle**
* **PitDrone.UnUse** - stop following the ally player. Derived from **PitDrone.Alert**, but with a normal pitch.
* **PitDrone.SpikeHitWorld** - when spike hits the world. Emitted from the spike projectile.
* **PitDrone.SpikeHitBody** - when spike hits someone. Emitted from the spike projectile.

Note: the pitdrone uses a lot of sounds in the sequence events. These sounds can't be changed via soundscripts.

### Visuals

* **Pitdrone.Spike** - spike projectile.
* **Pitdrone.SpikeTrail** - a trail beam following the projectile.
* **Pitdrone.TinySpit** - sprites sprayed out of pitdrone head on spike attack.

### Attacks

* *Melee Attack 1* - both claw attack.
* *Melee Attack 2* - two single claw attacks.
* *Range Attack 1* - fire a spike.

### Animation events

* `1` - spawn spike projectile.
* `2` - both claw trace hull attack. Deals **sk_pitdrone_dmg_bite** damage. This event is played twice during the "bite" animation, with the event `6` between them, so the second trace hull attack might not reach the target due to the knockback.
* `4` - single claw trace hull attack. Deals **sk_bullsquid_dmg_whip** damage. This event is played twice during the "whip" animation. Unlike the Opposing Force implementation, it doesn't force the target to gib on death.
* `5` - hop (when pitdrone is surprised by attack).
* `6` - knock away trace hull attack (during both claw attack). Doesn't do damage by default, just a knockback.

### Entity template examples

{{% tabs %}}

{{% tab "Let pitdrones open doors" %}}
*In Half-Life: Opposing Force pitdrones can't open doors.*
```json
{
    "monster_pitdrone": {
        "open_door_capability": true
    }
}
```
{{% /tab %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_pitdrone": {
        "check_melee_attack1": {
            "distance": 70,
            "dot": 0.7
        },
        "check_melee_attack2": {
            "distance": 70,
            "dot": 0.7
        },
        "trace_hull_attacks": {
            "2": {
                "distance": 70,
                "knock": {
                    "forward": 100,
                    "up": 100
                }
            },
            "4": {
                "distance": 70,
                "punchangle": {
                    "pitch": 20,
                    "roll": -20
                },
                "knock": {
                    "right": -100,
                    "up": 100
                }
            },
            "6": {
                "distance": 70,
                "knock": {
                    "forward": 200,
                    "up": 200,
                    "player_only": true
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
