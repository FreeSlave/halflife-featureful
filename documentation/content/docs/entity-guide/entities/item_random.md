---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# item_random

Spawns a random item from the list and removes itself. If entity has a targetname, it needs to be triggered in order to spawn a random item.

Configuring probablities for each `item_random` is a tedious work, so it's better to use [info_item_random]({{< ref info_item_random >}}) in combination with [item_random_proxy]({{< ref item_random_proxy >}}) to avoid repeating yourself.

### Parameters

Parameters are probabilities of items spawning. The probabilities don't need to sum up to 100, the sum can be arbitrary.

* `No item chance` - chance that no item will be spawned at all.

{{% hint warning %}}
Supports up to 16 different items maximum.
{{% /hint %}}

### Spawnflags

* `Predetermined` - whether the random is predetermined when the map starts, opposed to randomizing when the named `item_random` is called.
