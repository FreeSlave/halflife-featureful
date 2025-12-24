---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_crossbow

[TWHL](https://twhl.info/wiki/page/weapon_crossbow)

### Skill variables

See [crossbow_bolt]({{< ref "crossbow_bolt/#skill-variables" >}})

### Weapon templates

{{% tabs %}}

{{% tab "Explosive bolts" %}}

Make crossbow fire explosive bolts (like in multiplayer). Note that it does additional explosion damage.

```json
{
    "weapon_crossbow": {
        "fire": {
            "projectile": {
                "name": "crossbow_bolt explosive"
            }
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}
