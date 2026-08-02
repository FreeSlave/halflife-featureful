---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_tool

Generic weapon meant to be configured via [Weapon templates]({{< ref weapon-templates >}}).

By default it uses satchel radio model and works as a [tool]({{< ref "weapon-templates/#tool" >}}) weapon, similar to [weapon_radio]({{< ref weapon_radio >}}).

{{% hint info %}}
Don't forget to include **sprites/weapon_tool.txt** in your mod files.

Also enable `tool` in **features/featureful_weapons.cfg**.

Configure the weapon HUD position via **features/hud_weapon_layout.cfg**.
{{% /hint %}}
