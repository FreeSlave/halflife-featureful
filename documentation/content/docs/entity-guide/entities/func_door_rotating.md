---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_door_rotating

[TWHL](https://twhl.info/wiki/page/func_door_rotating)

Has same changes as [func_door]({{< ref func_door >}})

### Features

* `doors_open_in_move_direction`. Server feature. Allows to make `func_door_rotating` open in the direction the player is moving instead of opening the way the player is facing.
* `door_rotating_starts_open_fix`. Server feature. Original Half-Life has a bug that makes the named `func_door_rotating` with `Starts open` spawnflag unresponsive after the first triggering. Set this to true to fix this issue. This is left disabled by default just in case some existing map relies on this behavior.
