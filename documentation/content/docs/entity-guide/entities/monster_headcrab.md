---
bookHidden: true
bookToC: false
---

# monster_headcrab

[TWHL](https://twhl.info/wiki/page/monster_headcrab)

### Skill variables

* **sk_headcrab_health** - monster's health.
* **sk_headcrab_dmg_bite** - bite damage.

### Default classification

`Alien Prey`

### Default display name

`Headcrab`

### Soundscripts

* **Headcrab.Idle** - idle sounds.
* **Headcrab.Alert** - alert sounds.
* **Headcrab.Pain** - pain sounds.
* **Headcrab.Die** - death sounds.
* **Headcrab.Leap** - jump sound.
* **Headcrab.Attack** - attack sound on jump.
* **Headcrab.Bite** - hit something.

### Animation events

* `2` - occasionally play **Headcrab.Attack** soundscript.

### Entity template examples

{{% tabs %}}

{{% tab "Chicken from They Hunger" %}}
```json
{
    "chicken": {
        "own_visual": {
            "model": "models/chicken.mdl"
        },
        "blood": "red",
        "classify": "Alien Monster",
        "soundscripts": {
            "Headcrab.Idle": {
                "waves": ["chicken/ch_idle1.wav", "chicken/ch_idle2.wav"]
            },
            "Headcrab.Alert": {
                "waves": ["chicken/ch_alert1.wav", "chicken/ch_alert2.wav"]
            },
            "Headcrab.Pain": {
                "waves": ["chicken/ch_pain1.wav", "chicken/ch_pain2.wav"]
            },
            "Headcrab.Die": {
                "waves": ["chicken/ch_die1.wav", "chicken/ch_die2.wav"]
            },
            "Headcrab.Attack": {
                "waves": ["chicken/ch_attack1.wav", "chicken/ch_attack2.wav"]
            }
        }
    }
}
```
{{% /tab %}}

{{% tab "The Hand from They Hunger" %}}
```json
{
    "thehand": {
        "own_visual": {
            "model": "models/thehand.mdl"
        },
        "blood": "red",
        "classify": "Alien Monster",
        "skill": {
            "headcrab_health": "*2",
            "headcrab_dmg_bite": "*2"
        },
        "soundscripts": {
            "Headcrab.Attack": {
                "waves": ["thehand/hc_attack1.wav"]
            }
        }
    }
}
```
{{% /tab %}}

{{% tab "Poisonous headcrab" %}}
Mimick the HL2 headcrab by providing the custom model and adding the poison damage type to the headcrab's attack.
```json
{
    "headcrab_black": {
        "own_visual": {
            "model": "models/headcrab_black.mdl"
        },
        "touch_attack": {
            "damage_info": {
                "type": "poison"
            }
        }
    }
}
```

{{% hint info %}}
If you go with HL2 poisonous headcrab idea you would also want to redefine the soundscripts.
{{% /hint %}}

{{% hint warning %}}
The toxin mechanic from HL2 is not implemented currently, so it won't work the same.
{{% /hint %}}

{{% /tab %}}

{{% tab "Take Damage rules" %}}
The [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_headcrab": {
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["acid"],
                    "attack_affinity": ["friendly", "neutral"]
                },
                "modifier": {
                    "skip_damage": true
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
