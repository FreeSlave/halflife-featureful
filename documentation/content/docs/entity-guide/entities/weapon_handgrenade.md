---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_handgrenade

[TWHL](https://twhl.info/wiki/page/weapon_handgrenade)

### Changes

* Plays draw animation after finishing a throw.

### Skill variables

See [grenade]({{< ref "grenade/#skill-variables" >}})

### Weapon templates

{{% tabs %}}

{{% tab "Throwable spore" %}}
Throw a [spore]({{< ref spore >}}) instead of the hand grenade.

```json
{
    "weapon_handgrenade": {
        "fire": {
            "projectile": {
                "name": "spore"
            }
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}
