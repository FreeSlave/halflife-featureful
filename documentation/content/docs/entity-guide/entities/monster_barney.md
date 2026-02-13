---
bookHidden: true
bookToC: false
---

# monster_barney

[TWHL](https://twhl.info/wiki/page/monster_barney)

### Changes

* Checks for friendly fire when shooting.
* Now uses alert sentences (`BA_ATTACK`) when encountering enemy.
* Can set a custom head if the model has multiple heads (the bodygroup index 2 is supposed to be the head bodygroup).

### New parameters

* `Gun state` allows to spawn a security guard with a gun drawn.

### Skill variables

* **sk_barney_health** - monster's health.
* **sk_9mm_bullet** - handgun bullet damage.
* **sk_357_bullet** - Python bullet damage.

### Default classification

`Player Ally`

### Default display name

`Security Guard`

### Soundscripts

* **Barney.Pain** - pain sound.
* **Barney.Die** - death sound.
* **Barney.FirePistol** - firing a handgun.
* **Barney.FirePython** - firing a Python. Derived from **NPC.Python**.

### Animation events

* `2` - draw a weapon
* `3` - fire a weapon. Play **Barney.FirePistol** or **Barney.FirePython** soundscripts.
* `4` - holster a weapon.

### Entity template examples

{{% tabs %}}

{{% tab "Barnabus" %}}
Barney's evil twin from Sven Co-op.
```json
{
    "barnabus": {
        "own_visual": {
            "model": "models/barnabus.mdl"
        },
        "classify": "Human Military"
    }
}
```
{{% /tab %}}

{{% tab "Trace Attack rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_barney": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": ["chest", "stomach"],
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "*0.5"
                }
            },
            {
                "conditions": {
                    "hitgroup": 10,
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "-20",
                    "dmg_min_threshold": 0.01,
                    "hitgroup": "head"
                },
                "threshold_effects": {
                    "ricochet": {
                        "chance": 1.0,
                        "scale": 1
                    }
                }
            },
            {
                "conditions": {
                    "hitgroup": 10
                },
                "modifier": {
                    "hitgroup": "head"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
