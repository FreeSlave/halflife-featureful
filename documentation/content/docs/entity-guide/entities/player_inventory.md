---
bookHidden: true
bookToC: false
---

# player_inventory

Add, remove or set the specified count of the specific [player inventory item]({{< ref player-inventory >}}).

### Parameters

* `Item name` - the kind of inventory item to operate on. Special case: empty item name in combination with `Decrease by count` or `Set count` operation and count set to 0 means removing all items. 'Fire on' triggers don't fire in this case.
* `Inventory operation` - what to do with an item.
* `Item Count` - decrease/increase the count of items or set this count of items (depending on the operation).
* `Fire on count change` - fire if the operation changed the count of item.
* `Fire on max count limit` - fire when can't give an item duplicate due to the max count limit for this item (if it has one). This never fires when `Allow overflow` spawnflag is set.
* `Fire on item limit` - fire when can't give a new item due to the max inventory item limit (the player has maximum allowed number of different inventory items and can't carry more).

### Spawnflags

* `Remove on fire` - remove this entity after operation.
* `Allow overflow` - whether setting or increasing the item count can overflow the maximum limit for this kind of item.

### Reaction to use-type

If the `Inventory operation` is set to `Depends on input`, then the *Off* use-type will decrease the item count, and *Set* use-type (e.g. via the `game_counter_set`) will set the exact item count (the `Item Count` is ignored in this case).
