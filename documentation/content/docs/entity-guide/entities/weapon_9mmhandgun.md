---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_9mmhandgun

[TWHL](https://twhl.info/wiki/page/weapon_9mmhandgun)

### Skill variables

* **sk_plr_9mm_bullet** - bullet damage.

### Weapon templates

{{% tabs %}}

{{% tab "Glock with a silencer" %}}
Add or remove the silencer when the secondary attack is pressed (the cut feature).

When silencer is on, the weapon produces less sound for AI. It also makes the weapon more accurate in exchange for slightly slower fire rate to justify existence of different fire modes. You can balance `"spread"` and `"cycle_time"` to your liking.

```json
{
    "weapon_9mmhandgun": "glock_silencer"
}
```

* For switching back to non-silenced mode we use two animations in `"switch_mode_back"` - first holster the weapon, then deploy it without silencer.

{{% hint warning %}}
This won't work correctly with default HD model as it lacks the body variant with a silencer. So make sure you distribute the proper model with the mod to ensure it won't use the default HD model.
{{% /hint %}}

{{% /tab %}}

{{% tab "Glock-18 from Counter Strike" %}}

See [Glock-18]({{< ref "weapon-templates-presets#glock-18" >}})

{{% /tab %}}

{{% tab "Glock from Delta Particles" %}}

See [Delta Particles]({{< ref "weapon-templates-presets#delta-particles" >}}).

{{% /tab %}}

{{% /tabs %}}
