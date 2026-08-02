---
bookHidden: true
bookToC: false
entityCategory: solid
---

# trigger_teleport

[TWHL](https://twhl.info/wiki/page/trigger_teleport)
[svenmanor](https://www.svenmanor.com/entity-guide/trigger_teleport)

### New parameters

* ![](/images/svencoop.png) `Start Inactive` - make teleport inactive until it's triggered by name.
* ![](/images/spirit.png) `Landmark entity`. The name of a point entity (e.g. info_target or info_teleport_destination) that is usually put inside the `trigger_teleport` brush. The player will be teleported to the destination using the relative distance. If landmark and destination entities have different yaw angles, the player will be rotated appropriately as well. This allows for making 'seemless' teleports. This is alternative to `Relative Teleport` spawnflag. If landmark is defined the velocity is always kept.

### New spawnflags

* ![](/images/svencoop.png) `Keep angles` and `Keep velocity` - keep player's angles and velocity upong teleporting.
* ![](/images/svencoop.png) `Relative Teleport`. The player's destination will be the location of the destination plus his offset from the trigger_teleport's origin in the moment of the player entering the trigger. If you decide to use this, you need to add an origin brush to your `trigger_teleport`.

### Reaction to use-type

If `Start Inactive` is enabled, the input can switch the teleport state depending on the use-type.
