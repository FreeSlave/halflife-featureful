---
title: "Weapon templates"
---

# {{% param "title" %}}

Weapon templates allow to configure some aspects of weapon behavior via **templates/weapons.json** file.

# Format of templates/weapons.json

The document is an object where each property presents a weapon template. The keys must be weapon entity names, case sensitive.

## max_clip

Allows to change the default max clip of the weapon.

Currently it's not possible to set a max clip for a weapon that doesn't uses clips by default (e.g. [weapon_egon]({{< ref weapon_egon >}})).

Set `max_clip` to disable weapon clip. This means the weapon won't need to be reloaded and will use ammo right from the player's ammo pool.

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
