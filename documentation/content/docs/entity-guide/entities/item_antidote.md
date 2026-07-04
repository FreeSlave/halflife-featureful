---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# item_antidote

[TWHL](https://twhl.info/wiki/page/item_antidote)

Antidote is a pickable item that is used automatically upon the player getting the [time based damage]({{< ref "time-based-damage" >}}) effect from poison and nervegas and also gives temporary immunity against the subsequent time based damage (but doesn't protect from the impact damage).
The player can carry many antidotes (there's currently no limit on maximum antidotes carried).

### Changes

* Stops poison/nervegas time based effect faster.
* If `item_antidote` is defined in **hud.txt** it will be used to show the antidote in pickup history.
* If `item_antidote` is defined in **hud.txt** the antidote sprite along with the current count will be shown in HUD, among [player inventory items]({{< ref "player-inventory" >}}).

### New parameters

* `Pickup sound` - custom pickup sound (by default `item_antidote` will use the **Antidote.Pickup** soundscript).

### Skill variables

* `antidote_time` - how long the player will be protected from getting the time based effect from the subsequent toxic attacks after the antidote has been applied. This is 10 seconds by default.

### Soundscripts

* **Antidote.Pickup** - the antidote is getting picked up. Empty by default.
* [Player.AntidoteUse]({{< ref "player/#soundscripts" >}})
