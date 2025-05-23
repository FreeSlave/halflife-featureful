---
bookHidden: true
bookToC: false
---

# monster_babycrab

[TWHL](https://twhl.info/wiki/page/monster_babycrab)

### Skill variables

* **sk_headcrab_health** - monster's health. This is multiplied by 0.25 for babycrab.
* **sk_headcrab_dmg_bite** - melee damage. This is multiplied by 0.3 for babycrab.

### Default classification

`Alien Prey`

### Soundscripts

* **Babycrab.Idle** - idle sounds. Derived from **Headcrab.Idle** with increased pitch and decreased volume.
* **Babycrab.Alert** - alert sounds. Derived from **Headcrab.Alert** with increased pitch and decreased volume.
* **Babycrab.Pain** - pain sounds. Derived from **Headcrab.Pain** with increased pitch and decreased volume.
* **Babycrab.Die** - death sounds. Derived from **Headcrab.Die** with increased pitch and decreased volume.
* **Babycrab.Leap** - jump sound. Derived from **Headcrab.Leap** with increased pitch and decreased volume.
* **Babycrab.Attack** - attack sound on jump. Derived from **Headcrab.Attack** with increased pitch and decreased volume.
* **Babycrab.Bite** - hit something. Derived from **Headcrab.Bite** with increased pitch and decreased volume.

### Animation events

* `2` - occasionally play **Babycrab.Attack** soundscript.

### Entity template examples

{{% tabs %}}

{{% tab "Opaque babycrab" %}}
By default babycrabs have a bit of transparency in them. This template makes them opaque and also sets a smaller gib scale, because it's weird they produce gibs as big as other monsters.
```json
{
    "monster_babycrab": {
        "own_visual": {
            "rendermode": "normal"
        },
        "gib_visual": {
            "scale": 0.5
        }
    },
}
```
{{% /tab %}}

{{% tab "Take Damage rules" %}}
Uses same rules as [monster_headcrab]({{< ref monster_headcrab >}}).
{{% /tab %}}

{{% /tabs %}}
