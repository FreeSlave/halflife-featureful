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
    - [Tool]({{< ref weapon_tool >}})
* `Tool Target` - where the player needs to be looking to be able to activate the tool. The best way to get this to work is to target an `info_target`. If no tool target defined, player can look in any direction.

### Spawnflags

* `Remove on use` -  remove this entity from the game after activation.
