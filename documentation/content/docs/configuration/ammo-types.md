---
title: "Ammo types"
---

# {{% param "title" %}}

Featureful SDK provides a set of predefined ammo types. Internally the ammo type is presented by a name and the max amount player can carry.

## The list of ammo types

| Ammo name     | Default max ammo | [Ammo entitiy]({{< ref ammo-entities >}})     | Weapons using this ammo by default |
| :--------     | :-------           | :-------         | :------- |
| `buckshot`    | 125                | `ammo_buckshot`  | [weapon_shotgun]({{< ref weapon_shotgun >}}) |
| `9mm`         | 250                | `ammo_9mmclip`, `ammo_9mmAR`, `ammo_9mmbox` | [weapon_9mmhandgun]({{< ref weapon_9mmhandgun >}}), [weapon_9mmAR]({{< ref weapon_9mmAR >}}) |
| `ARgrenades`  | 10                 | `ammo_ARgrenades` | [weapon_9mmAR]({{< ref weapon_9mmAR >}}) (secondary attack) |
| `357`         | 36                 | `ammo_357`       | [weapon_357]({{< ref weapon_357 >}}), [weapon_eagle]({{< ref weapon_eagle >}}) |
| `uranium`     | 100                | `ammo_gaussclip` | [weapon_gauss]({{< ref weapon_gauss >}}), [weapon_egon]({{< ref weapon_egon >}}), [weapon_displacer]({{< ref weapon_displacer >}}) |
| `rockets`     | 5                  | `ammo_rpgclip`   | [weapon_rpg]({{< ref weapon_rpg >}}) |
| `bolts`       | 50                 | `ammo_crossbow`  | [weapon_crossbow]({{< ref weapon_crossbow >}}) |
| `Trip Mine`   | 5                  | none             | [weapon_tripmine]({{< ref weapon_tripmine >}}) |
| `Satchel Charge` | 5               | none             | [weapon_satchel]({{< ref weapon_satchel >}}) |
| `Hand Grenade`| 10                 | none             | [weapon_handgrenade]({{< ref weapon_handgrenade >}}) |
| `Snarks`      | 15                 | none             | [weapon_snark]({{< ref weapon_snark >}}) |
| `Hornets`     | 8                  | none             | [weapon_hornetgun]({{< ref weapon_hornetgun >}}) |
| `Medicine`    | 100                | none             | [weapon_medkit]({{< ref weapon_medkit >}}) |
| `Penguins`    | 9                  | none             | [weapon_penguin]({{< ref weapon_penguin >}}) |
| `556`         | 200                | `ammo_556`       | [weapon_249]({{< ref weapon_m249 >}}), [weapon_minigun]({{< ref weapon_minigun >}}) |
| `762`         | 15                 | `ammo_762`       | [weapon_sniperrifle]({{< ref weapon_sniperrifle >}}) |
| `Shocks`      | 10                 | none             | [weapon_shockrifle]({{< ref weapon_shockrifle >}}) |
| `spores`      | 20                 | [ammo_spore]({{< ref ammo_spore >}}) | [weapon_sporelauncher]({{< ref weapon_sporelauncher >}}) |
| `45acp`       | 200                | `ammo_45acp`     | none |
| `57mm`        | 200                | `ammo_57mm`      | none |
| `nails`       | 200                | `ammo_nails`     | [weapon_nailgun]({{< ref weapon_nailgun >}}) |
| `grenades`    | 50                 | `ammo_grenadeclip` | [weapon_grenadelauncher]({{< ref weapon_grenadelauncher >}}) |
| `fuel`        | 100                | `ammo_fuel`      | none |
| `cells`       | 100                | `ammo_cells`     | none |
| `charges`     | 10                 | `ammo_charges`   | none |
| `rounds`      | 200                | `ammo_rounds`    | none |
| `slugs`       | 100                | `ammo_slugs`     | none |

Some ammo types are not used by any weapons by default. These are defined for developer's convenience if they want to configure some weapon to use a different [ammo type]({{< ref "weapon-templates/#ammo_name" >}}) via the [weapon templates]({{< ref weapon-templates >}}).

{{% hint info %}}
While technically it's perfectly possible to allow registering custom new ammo types by a mod developer, the GoldSource has some technical limitations that prevent dynamic creation of new classnames for ammo entities, and registering a new ammo type without registering a new entity wouldn't be so useful.
{{% /hint %}}

## Managing player's ammo

Besides giving the ammo by ammo pickup entities, Featureful SDK provides other means to control player's ammo:

* [game_player_settings]({{< ref game_player_settings >}}) - allows to give, set or remove player's ammo.
* [player_calc_ratio]({{< ref player_calc_ratio >}}) - allows to check the current amount of player's ammo.

## Configuring max ammo

The max ammo carried by the player can be configured via **features/maxammo.cfg**.

Example:

```
9mm 200
buckshot 60
bolts 20
```
