---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/svencoop.png) weapon_uzi

Uzi from Sven Co-op. Akimbo version is not implemented yet.

### Skill variables

* **sk_plr_uzi** - bullet damage.

### Weapon templates

{{% tabs %}}

{{% tab "Uzi with a newer model" %}}

Default Uzi implementation uses the animation timings from the Sven Co-op 3.0. Later on the Uzi model has been changed along with animation durations. It also removed playing the deploy sound by animation event.

This template fixes the timings and sets the deploy sound.

```json
{
    "weapon_uzi": {
        "idle": [
            {
                "anim": 0,
                "duration": 1,
                "chance": 0.8
            },
            {
                "anim": 1,
                "duration": 3.0,
                "chance": 0.1
            },
            {
                "anim": 2,
                "duration": 1.18,
                "chance": 0.1
            }
        ],
        "deploy": {
            "duration": 0.9,
            "sound": "weapons/uzi/deploy.wav"
        },
        "reload": {
            "duration": 2.6
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}
