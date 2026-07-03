---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# item_antidote

[TWHL](https://twhl.info/wiki/page/item_antidote)

### Changes

* Stops poison faster.
* If `item_antidote` is defined in **hud.txt** it will be used to show the antidote in pickup history.
* If `item_antidote` is defined in **hud.txt** the antidote sprite along with the current count will be shown in HUD, among [player inventory items]({{< ref "player-inventory" >}}).

### New parameters

* `Pickup sound` - custom pickup sound (by default `item_antidote` will use the **Antidote.Pickup** soundscript).

### Soundscripts

* **Antidote.Pickup** - the antidote is getting picked up. Empty by default.
