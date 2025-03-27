---
bookHidden: true
bookToC: false
entityCategory: solid
---

# ![](/images/opfor.png) trigger_xen_return

When touched by a player, `trigger_xen_return` displaces player to the [info_displacer_earth_target]({{< ref info_displacer_earth_target >}}) point.

If there're several instance of `info_displacer_earth_target` on the map, it selects the one that is closest to the location of the player at the moment before they got teleported to Xen by displacer.
