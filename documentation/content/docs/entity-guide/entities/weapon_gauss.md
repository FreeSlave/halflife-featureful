---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_gauss

[TWHL](https://twhl.info/wiki/page/weapon_gauss)

### Changes

* Plays idle animations.
* Fixed charge sound not stopping after the overcharge.
* Firing at thick brushes with a secondary attack won't reflect at the player.

### Skill variables

* **sk_plr_gauss** - single shot damage.
* **sk_plr_gauss_maxspin** - maximum damage for the secondary attack (the longer the spin the higher the damage). The default value is 200.
* **sk_plr_gauss_radius_factor** - the factor to calculate radius of the tau-cannon blast (when it hits something behind the wall). The radius is evaluated as the damage multiplied by this factor. The default value is 2.5 (forced to 1.75 in deathmatch).
* **sk_plr_gauss_overcharge** - overcharge damage (when spinning for too long). The default value is 50.

### Weapon templates

{{% tabs %}}

{{% tab "Enable fidget animation" %}}
This enables fidget animation on gauss.

```json
{
    "weapon_gauss": {
        "idle": [
            {
                "anim": 0,
                "chance": 0.5,
                "duration": [10, 15]
            },
            {
                "anim": 1,
                "chance": 0.25,
                "duration": [10, 15]
            },
            {
                "anim": 2,
                "chance": 0.25,
                "duration": 3.0
            }
        ]
    }
}
```

{{% /tab %}}

{{% tab "Reloadable clip" %}}
This makes gauss require reloading.

```json
{
    "weapon_gauss": {
        "max_clip": 20,
        "reload": {
            "anim": 2,
            "sound": ["weapons/shock_recharge.wav"]
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}


