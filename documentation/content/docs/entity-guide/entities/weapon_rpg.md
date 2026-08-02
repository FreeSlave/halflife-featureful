---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_rpg

[TWHL](https://twhl.info/wiki/page/weapon_rpg)

### Changes

* Hides laser spot when player is using tank.

### Skill variables

See [rpg_rocket]({{< ref "rpg_rocket/#skill-variables" >}})

### Weapon templates

{{% tabs %}}

{{% tab "RPG without laser spot" %}}

```json
{
    "weapon_rpg": {
        "idle": [
            {
                "anim": 0,
                "chance": 0.75,
                "duration": 6.0
            },
            {
                "anim": 1,
                "chance": 0.25,
                "duration": 6.1
            }
        ],
        "idle_empty": [
            {
                "anim": 8,
                "chance": 0.75,
                "duration": 6.0
            },
            {
                "anim": 9,
                "chance": 0.25,
                "duration": 6.1
            }
        ],
        "alt_idle": null,
        "alt_idle_empty": null,
        "switch_mode_common": {
            "toggle_laser_spot": false
        },
        "laser_spot": {
            "start_on": false
        },
        "secondary_attack": "disabled"
    }
}
```

{{% /tab %}}

{{% /tabs %}}
