---
bookHidden: true
bookToC: false
---

# monster_barney

[TWHL](https://twhl.info/wiki/page/monster_barney)

### Changes

* Checks for friendly fire when shooting.
* Now uses alert sentences (BA_ATTACK) when encountering enemy.

### New parameters

* `Gun state` allows to spawn a security guard with a gun drawn.

### Skill variables

* **sk_barney_health** - monster's health.
* **sk_9mm_bullet** - handgun bullet damage.
* **sk_357_bullet** - Python bullet damage.

### Default classification

`Player Ally`

### Soundscripts

* **Barney.Pain** - pain sound.
* **Barney.Die** - death sound.
* **Barney.FirePistol** - firing a handgun.
* **Barney.FirePython** - firing a Python. Derived from **NPC.Python**.

### Animation events

* `2` - draw a weapon
* `3` - fire a weapon.
* `4` - holster a weapon. Plays **Barney.FirePistol** or **Barney.FirePython** soundscripts.

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

{{% /tabs %}}
