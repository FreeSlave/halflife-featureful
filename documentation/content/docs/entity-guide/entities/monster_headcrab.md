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
        },
        "classify": "Alien Monster"
    }
}
```
{{% /tab %}}

{{% /tabs %}}
