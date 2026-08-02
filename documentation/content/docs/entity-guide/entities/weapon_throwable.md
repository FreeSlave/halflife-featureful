---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# weapon_throwable

Generic weapon meant to be configured via [Weapon templates]({{< ref weapon-templates >}}).

By default it acts very similar to [weapon_handgrenade]({{< ref weapon_handgrenade >}}) and uses its world, view and player model.

{{% hint info %}}
Don't forget to include **sprites/weapon_throwable.txt** in your mod files.

Also enable `throwable` in **features/featureful_weapons.cfg**.

Configure the weapon HUD position via **features/hud_weapon_layout.cfg**.
{{% /hint %}}
