---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/opfor.png) weapon_eagle

### Changes

* Hides laser spot when player is using tank.

### Skill variables

* **sk_plr_eagle** - bullet damage.

### Weapon templates

{{% tabs %}}

{{% tab "Starting with laser spot on" %}}
This makes Desert Eagle to start with laser spot on the first deploy.

```json
{
    "weapon_eagle": {
        "start_in_alt_mode": true
    }
}
```

{{% /tab %}}

{{% tab "Desert Eagle from Counter Strike" %}}

See [Desert Eagle]({{< ref "weapon-templates-presets#desert-eagle" >}})

{{% /tab %}}

{{% /tabs %}}
