---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/czeror.png) weapon_camera

A tool "weapon" designed to be used in [trigger_usetool]({{< ref trigger_usetool >}}) area. This has 1.8 seconds [trigge delay]({{< ref "weapon-templates#trigger_delay" >}}) before the target activation.

This tool expects `icon_camera` entry in the **hud.txt**.

Player can't move while using the camera. This can be configured via [prevent_movement]({{< ref "weapon-templates#prevent_movement" >}}).

{{% hint info %}}
The view model is mirrored by default to match Condition Zero Deleted Scenes. This can be configured via [mirror_viewmodel]({{< ref "weapon-templates#mirror_viewmodel" >}}).
{{% /hint %}}

{{% hint info %}}
Currently it's not possible to lock the player view while using the camera. This might be implemented later.
{{% /hint %}}
