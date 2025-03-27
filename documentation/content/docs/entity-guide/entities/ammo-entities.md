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

There's also the [ammo_spore]({{< ref ammo_spore >}}) but despite sharing the classname prefix, it technically is not a pickup item.

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
