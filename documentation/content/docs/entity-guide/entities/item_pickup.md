---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# item_pickup

Generic item that can be picked by a player. The main purpose is to give the [inventory item]({{< ref "player-inventory" >}}).

### Parameters

* `Inventory item` - inventory item to give to player.
* `Inventory item count` - count of inventory items to give (if item is defined).
* `Message` - message from titles.txt to show on pickup.
* `Pickup sound` - custom sound to play on pickup.

### Soundscripts

* **Inventory.Pickup** - the item is being picked up. Empty by default. Designed to be overriden by the pickup entity template.
