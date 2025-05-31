---
title: "Weapon templates"
---

# {{% param "title" %}}

Weapon templates allow to configure some aspects of weapon behavior via **templates/weapons.json** file.

# Format of templates/weapons.json

The document is an object where each property presents a weapon template. The keys must be weapon entity names, case sensitive.

## max_clip

Allows to change the default max clip of the weapon.

{{% hint warning %}}
Currently it's not possible to set a max clip for a weapon that doesn't uses clips by default (e.g. [weapon_egon]({{< ref weapon_egon >}})).
{{% /hint %}}

Set `max_clip` to 0 to disable weapon clip. This means the weapon won't need to be reloaded and will use ammo right from the player's ammo pool.

```json
{
    "weapon_9mmAR": {
        "max_clip": 0
    },
    "weapon_shotgun": {
        "max_clip": 6
    }
}
```

## ammo_name

Allows to change the primary ammo the weapon is using. This only works for weapons that use ammo.

```json
{
    "weapon_shotgun": {
        "ammo_name": "9mm"
    },
    "weapon_crossbow": {
        "ammo_name": "uranium"
    }
}
```

Possible ammo names:

* `"buckshot"`
* `"9mm"`
* `"ARgrenades"`
* `"357"`
* `"uranium"`
* `"rockets"`
* `"bolts"`
* `"Trip Mine"`
* `"Satchel Charge"`
* `"Hand Grenade"`
* `"Snarks"`
* `"Hornets"`
* `"Medicine"`
* `"Penguins"`
* `"556"`
* `"762"`
* `"Shocks"`
* `"spores"`

{{% hint warning %}}
You still need to change the ammo sprite in the weapon hud .txt file to match the actual ammo type.
{{% /hint %}}
