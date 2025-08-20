---
bookHidden: true
bookToC: false
entityCategory: pickup
title: "ammo_* entities"
---

# Ammo entities

Ammo entities share the `ammo_` prefix in their name and same parameters.

### The list of ammo entities

* `ammo_9mmclip`
* `ammo_9mmAR`
* `ammo_9mmbox`
* `ammo_ARgrenades`
* `ammo_buckshot`
* `ammo_357`
* `ammo_rpgclip`
* `ammo_gaussclip`
* `ammo_crossbow`
* `ammo_556`
* `ammo_762`
* `ammo_45acp`
* `ammo_57mm`

There's also the [ammo_spore]({{< ref ammo_spore >}}) but despite sharing the classname prefix, it technically is not a pickup item.

`ammo_45acp` and `ammo_57mm` provide `45acp` and `57mm` ammo types which are not used by any weapon by default. The weapon can be made to use one of these ammo types via [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) property in the weapon template.

The entities use the same model as `ammo_9mmAR`. You can set custom model via [Entity templates]({{< ref entity-templates >}}).

Example:

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

### Parameters

* `Custom Model` - custom model for the entity instance.
* `Custom amount` - custom ammo amount provided for the player on picking up.
* `Gravity Setting`
    - Unmoveable
    - Fall to the ground (default)
    - Hover in the air
    - Hover in the air, ignore brush collision
* `Master` - block picking up until master is activated.

### Spawnflags

* `TOUCH Only` - can be picked up only by touching.
* `USE Only` - can be picked up only by pressing +use on it.

### Server cvars

* **mp_ammo_respawndelay** - delay before ammo entity respawns in multiplayer game. -2 means default (which is 20 seconds in Half-Life). -1 means never respawn.
