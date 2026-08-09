---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/czeror.png) weapon_blowtorch

A tool "weapon" designed to be used in [trigger_usetool]({{< ref trigger_usetool >}}) area.

This tool expects `icon_blowtorch` entry in the **hud.txt**.

Player can't move while using the blowtorch. This can be configured via [prevent_movement]({{< ref "weapon-templates#prevent_movement" >}}).

{{% hint info %}}
The view model is mirrored by default to match Condition Zero Deleted Scenes. This can be configured via [mirror_viewmodel]({{< ref "weapon-templates#mirror_viewmodel" >}}).
{{% /hint %}}

When idle blowtorch plays glow, flame and dynamic light effects. When fired the visual effects get stronger.

{{% hint warning %}}
The visual effects are currently not configurable.
{{% /hint %}}

Blowtorch deals 1 point of `burn` damage per 0.1 seconds.

Set the `Acceptable Damage Type` parameter on [func_breakable]({{< ref func_breakable >}}) or use the following [entity template]({{< ref entity-templates >}}) to allow breaking it only with `burn` damage type:

```json
{
    "only_burn": {
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": "burn",
                    "dmg_type_match": "none"
                },
                "modifier": {
                    "skip_damage": true
                }
            }
        ]
    }
}
```
