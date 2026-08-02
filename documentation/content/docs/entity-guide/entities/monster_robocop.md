---
bookHidden: true
bookToC: false
---

# monster_robocop

A boss robot ported from Poke646.

### Skill variables

* **sk_robocop_health** - monster's health.
* **sk_robocop_dmg_mortar** - beam targeted explosion damage.
* **sk_robocop_dmg_fist** - fist wave damage.
* **sk_robocop_sw_radius** fist shockwave radius.

### Default classification

`Machine`

### Default display name

`Robocop`

### Soundscripts

* **RoboCop.Die** - death sound.
* **RoboCop.Pain** - pain sound. Empty by default.
* **RoboCop.Fist** - fist attack.
* **RoboCop.Charge** - charging laser attack.
* **RoboCop.Laser** - laser sound.
* **RoboCop.Step** - step sounds.

### Visuals

* **RoboCop.Eye** - eye sprite.
* **RoboCop.BeamSpot** - the sprite in the end of the beam.
* **RoboCop.Beam** - the laser beam.
* **RoboCop.ShockWave1** - the beam settings for the outer shockwave.
* **RoboCop.ShockWave2** - the beam settings for the middle shockwave.
* **RoboCop.ShockWave3** - the beam settings for the inner shockwave.

Shared visuals:

* **RoboCop.ShockWaveBase** - the base visual for shockwave attack.

### Attacks

* *Melee Attack 1* - shock wave.
* *Range Attack 1* - beam.

### Animation events

* `1` and `2` - play **RoboCop.Step** soundscript and shake effect.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack and Take Damage rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) and [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_robocop": {
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

{{% /tabs %}}
