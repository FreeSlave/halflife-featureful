---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_357

[TWHL](https://twhl.info/wiki/page/weapon_357)

### Skill variables

* **sk_plr_357_bullet** - bullet damage.

### Weapon templates

{{% tabs %}}

{{% tab "Python with a scope" %}}

This makes Python use a different submodel and allows scope ability like in Half-Life multiplayer.

```json
{
    "weapon_357": "python_zoom"
}
```

{{% /tab %}}

{{% tab "Python from Delta Particles" %}}

See [Delta Particles]({{< ref "weapon-templates-presets#delta-particles" >}}).

{{% /tab %}}

{{% tab "Longer reload" %}}
This makes python reload duration on par with animation length, so player can't fire it sooner than the reload animation is finished.

```json
{
    "weapon_357": {
        "reload": {
            "duration": 3.08
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
