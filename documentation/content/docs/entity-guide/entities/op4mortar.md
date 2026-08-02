---
bookHidden: true
bookToC: false
entityCategory: object
---

# ![](/images/opfor.png) op4mortar

The stationary mortar weapon from Opposing Force. Can act aggressive to the player or can be controlled by a player via [func_op4mortarcontroller]({{< ref func_op4mortarcontroller >}}).

### Parameters

* `Mortar Shell Velocity` - the speed of the mortar projectile. For AI mortars you should set a high value otherwise the mortar won't be able to find the right angle to fire. The Opposing Force map that uses `op4mortar` sets the velocity to 1400. Alternatively you can make the map with the skybox high enough for the mortar shell arch.
* `Fire Delay` - the delay between firing, in seconds.
* `Classification` - a classification to use in relationship checks when `op4mortar` is active and seeking for enemies. By default it perceives only players as enemies (similar to [func_tank]({{< ref func_tank >}})).

### Skill variables

See [mortar_shell]({{< ref mortar_shell >}}).

### Soundscripts

* **Op4Mortar.Rotate** - rotation sound (both vertical and horizontal).
* **Op4Mortar.Launch** - when mortar projectile is launched.
* See [mortar_shell]({{< ref mortar_shell >}}).

### Visuals

See [mortar_shell]({{< ref mortar_shell >}}).

### Notes

The `op4mortar` in Opposing Force had the `Enemy Type` parameter which was confusing and not clear how to use. It was removed in Featureful.
