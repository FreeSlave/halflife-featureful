---
bookHidden: true
bookToC: false
---

# monster_apache

[TWHL](https://twhl.info/wiki/page/monster_apache)

### New parameters

* `Rotor volume` - configure rotor volume.

### Skill variables

* **sk_apache_health** - monster's health.

### Default classification

`Human Military`

### Soundscripts

* **Apache.Rotor** - rotor sound during the flight. Must be looped.
* **Apache.FireGun** - firing a bullet.
* **Apache.Crash** - death sound. Derived from **NPC.Crash**
* **Apache.RPG** - rocket launch sound. Emitted from the rocket projectile.

### Visuals

* **Apache.RocketModel** - apacke rocket model.
* **Apache.RocketTrail** - apache rocket trailing beam.
* **Apache.FallingSmoke** - smokes produced while apache is falling.
* **Apache.CrashSmoke** - big smoke on crash.
* **Apache.RocketSmoke** - smoke when launching rocket.
* **Apache.DamageSmoke** - smoke indication of being damaged.
* **Apache.Fireball** - big fireball played when apache crashes.
* **Apache.BlastCircle** - wave played when apache crashes.
* **Wreckage.Smoke** - smokes played after the apache crashed.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack and Take Damage rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) and [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

{{% hint info %}}
The smoke behavior can't be currently expressed via entity templates, so the standard behavior always stays as is.
{{% /hint %}}

```json
{
    "monster_apache": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": 6,
                    "dmg_type": ["energybeam", "bullet", "club"]
                },
                "modifier": {
                    "skip_damage": true
                }
            },
            {
                "conditions": {
                    "dmg": "<=50",
                    "hitgroup": [1, 2],
                    "invert_hitgroup_check": true
                },
                "modifier": {
                    "skip_damage": true
                },
                "effects": {
                    "ricochet": {
                        "scale": 1.0,
                        "chance": 1.0
                    }
                }
            }
        ],
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["blast"]
                },
                "modifier": {
                    "dmg": "*2"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
