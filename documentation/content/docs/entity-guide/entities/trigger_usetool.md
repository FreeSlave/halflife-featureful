---
bookHidden: true
bookToC: false
entityCategory: solid
---

# ![](/images/czeror.png) trigger_usetool

This brush entity defines where and what tools should be used.

### Parameters

* `Target` - have this entity target something when a tool is used in the zone.
* `Tool Name` - a tool to be used in this area.
    - [Camera]({{< ref weapon_camera >}})
    - [Radio]({{< ref weapon_radio >}})
* `Tool Target` - where the player needs to be looking to activate the tool. The best way to get this to work is to target an `info_target`.

### Spawnflags

* `Remove on use` -  remove this entity from the game after activation.
