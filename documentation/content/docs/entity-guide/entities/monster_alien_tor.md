---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) monster_alien_tor

Alien commander from Sven Co-op capable of spawning up to 3 alien grunts.

{{% hint warning %}}
The Tor's energy beam and slam attacks can launch enemies upwards, so make sure the player won't access any unintended area with the help of these.
{{% /hint %}}

### Skill variables

* **sk_tor_health** - monster's health.
* **sk_tor_punch** - melee damage.
* **sk_tor_energybeam** - damage per energy beam tick.
* **sk_tor_sonicblast** - maximum slam attack damage.
* **sk_tor_lift_speed_ground** - amount of vertical velocity to apply to the enemy on energy beam attack when enemy is on the ground. Default value is 200. This is usually higher than **sk_tor_lift_speed** to help lifting the target from the ground.
* **sk_tor_lift_speed** - amount of vertical velocity to apply to the enemy on energy beam attack when enemy is *not* on the ground. By default it's **sk_tor_lift_speed_ground** multiplied by 0.6.

### Default classification

`Alien Military`

### Soundscripts

* **Tor.Idle** - idle sounds.
* **Tor.Alert** - alert sounds.
* **Tor.Pain** - pain sounds.
* **Tor.Die** - death sounds.
* **Tor.Attack** - sound to play on melee attack.
* **Tor.LeftFoot** - left foot step sound (event 11).
* **Tor.RightFoot** - left foot step sound (event 10). This is the same as **Tor.LeftFoot** by default.
* **Tor.Slam** - slam attack sounds.
* **Tor.Shoot** - energy beam attack sound.
* **Tor.Summon** - sound to play when Tor starts summoning.
* **Tor.SummonPortal** - sound to play when the portal is opened. This is played at the summon point.
* **Tor.SummonSpawn** - sound to play when the summon is finished. This is played at the summon point.
* **Tor.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Tor.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **Tor.Use** - start following the ally player. Derived from **Tor.Idle**
* **Tor.UnUse** - stop following the ally player. Derived from **Tor.Alert**

### Visuals

* **Tor.Slam** - slam attack beam.
* **Tor.Beam** - energy beam.
* **Tor.Beam2** - a spiral energy beam played along with the main one.
* **Tor.SummonBeam** - the temporary beam linking the Tor and the portal he creates.
* **Tor.SummonSprite** - the portal sprite.

### Attacks

* *Melee Attack 1* - slam attack.
* *Melee Attack 2* - staff attack.
* *Range Attack 1* - energy beam.
* *Range Attack 2* - summon.

### Animation events

* `1` - create slam wave.
* `2` - trace hull attack using the staff swing. Deals **sk_tor_punch** damage.
* `3` - start energy beam attack.
* `4` - start summoning another monster. Plays **Tor.Summon** and **Tor.SummonPortal** soundscripts.
* `7` - trace hull attack using the staff stab. Deals **sk_tor_punch** damage.
* `10` - play **Tor.RightFoot** soundscript.
* `11` - play **Tor.LeftFoot** soundscript.

### Entity template examples

{{% tabs %}}

{{% tab "Friendly Tor" %}}
```json
{
    "tor_friendly": {
        "inherits": "monster_alien_tor",
        "own_visual": {
            "model": "models/Torf.mdl"
        },
        "classify": "Player Ally",
        "visuals": {
            "Tor.Beam": {
                "color": [16, 255, 255]
            },
            "Tor.Beam2": {
                "color": [128, 255, 255]
            }
        }
    }
}
```
{{% /tab %}}

{{% tab "Trace Attack rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_alien_tor": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": 10,
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "-20",
                    "dmg_min_threshold": 0.1,
                    "hitgroup": "generic",
                    "no_blood": true
                },
                "effects": {
                    "ricochet": {
                        "certain_on_new_frame": true,
                        "chance": 0.1,
                        "scale": [1.0, 2.0]
                    },
                    "tracer": {
                        "chance": 0.5,
                        "variance": 0.3
                    }
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
