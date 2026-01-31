---
bookHidden: true
bookToC: true
entityCategory: pickup
title: "ammo_* entities"
---

# Ammo entities

Ammo entities share the `ammo_` prefix in their name and same parameters.

### The list of ammo entities

| Entity classname | Ammo type | Default amount |
| :--------         | :-------      | :------- |
| `ammo_9mmclip`    | `9mm`         | 17 |
| `ammo_9mmAR`      | `9mm`         | 50 |
| `ammo_9mmbox`     | `9mm`         | 200 |
| `ammo_ARgrenades` | `ARgrenades`  | 2 |
| `ammo_buckshot`   | `buckshot`    | 12 |
| `ammo_357`        | `357`         | 6 |
| `ammo_rpgclip`    | `rockets`     | 1 (2 in multiplayer) |
| `ammo_gaussclip`  | `uranium`     | 20 |
| `ammo_crossbow`   | `bolts`       | 5 |
| `ammo_556`        | `556`         | 50 |
| `ammo_762`        | `762`         | 5 |
| `ammo_45acp`      | `45acp`       | 30 |
| `ammo_57mm`       | `57mm`        | 30 |
| `ammo_nails`      | `nails`       | 30 |
| `ammo_grenadeclip` | `grenades`   | 6  |

{{% hint info %}}
There's also the [ammo_spore]({{< ref ammo_spore >}}) but despite sharing the classname prefix, it technically is not a pickup item.
{{% /hint %}}

{{% hint info %}}
`ammo_45acp` and `ammo_57mm` provide `45acp` and `57mm` ammo types which are not used by any weapon by default. The weapon can be made to use one of these ammo types via [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) property in the weapon template.
{{% /hint %}}

### Configuration

Some aspects of ammo entities can be configured via [Entity templates]({{< ref entity-templates >}}).

#### Model

E.g. `ammo_45acp`, `ammo_57mm` and `ammo_nails` entities share the model with `ammo_9mmAR` by default. To change this you may use entity templates:

```json
{
    "ammo_45acp": {
        "own_visual": {
            "model": "models/w_45acp.mdl"
        }
    },
    "ammo_57mm": {
        "own_visual": {
            "model": "models/w_57mm.mdl"
        }
    }
}
```

#### Pickup sound

The pickup sound for the specific ammo entity can be changed via entity templates with the custom **Ammo.Pickup** soundscript.

```json
{
    "ammo_buckshot": {
        "soundscripts": {
            "Ammo.Pickup": {
                "waves": ["weapons/lock4.wav"]
            }
        }
    }
}
```

#### Ammo amount

The ammo amount provided by the specific ammo entity by default can be changed via the **features/ammo_amounts.cfg** file.

Example:

```
ammo_gaussclip 25
ammo_556 100
```

### New parameters

* `Custom Model` - custom model for the entity instance.
* `Custom amount` - custom ammo amount provided for the player on picking up.
* ![](/images/svencoop.png) `Gravity Setting`
    - Unmoveable
    - Fall to the ground (default)
    - Hover in the air
    - Hover in the air, ignore brush collision
* `Master` - block picking up until master is activated.

### New spawnflags

* ![](/images/svencoop.png) `TOUCH Only` - can be picked up only by touching.
* ![](/images/svencoop.png) `USE Only` - can be picked up only by pressing +use on it.

### Server cvars

* ![](/images/svencoop.png) **mp_ammo_respawndelay** - delay before ammo entity respawns in multiplayer game. -2 means default (which is 20 seconds in Half-Life). -1 means never respawn.
