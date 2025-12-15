---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_9mmAR

[TWHL](https://twhl.info/wiki/page/weapon_9mmAR)

### Skill variables

* **sk_plr_9mmAR_bullet** - bullet damage.
* **sk_plr_9mmAR_grenade** - grenade explosion damage.

### Weapon templates

{{% tabs %}}

{{% tab "MP5 from Counter Strike" %}}

See [MP5 Navy]({{< ref "weapon-templates-presets#mp5-navy" >}})

{{% /tab %}}

{{% tab "Danger sound on alt fire" %}}

In old versions of Half-Life (before predicting was introduced) firing an AR grenade made a danger sound (for AI) to emit from the player. This got broken in later versions and not working properly since then. Since it was like this for a long time it practically became a default behavior, so the danger sound is disabled in Featureful SDK by default. This behavior can be restored

```json
{
    "weapon_9mmAR": {
        "alt_fire": {
            "extra_ai_sound": {
                "type": ["danger"],
                "time": 0.2
            }
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}
