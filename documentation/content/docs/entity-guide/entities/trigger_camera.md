---
bookHidden: true
bookToC: false
---

# trigger_camera

[TWHL](https://twhl.info/wiki/page/trigger_camera)

### New parameters

* `Stop by player input` - allow player to press '+use' in order to stop watching at the camera.
* ![](/images/svencoop.png) `Target when turned off`.

### New spawnflags

* ![](/images/svencoop.png) `Player Invulnerable` - make player invulnerable to attacks while they're using the camera.
* ![](/images/svencoop.png) `Ignore Hold Time` - ignore hold time, turn off only when re-triggered.
* `Alive player only` - prevent camera activating on the dead player (which is possible in Half-Life).
* `Don't fire look target` - prevent firing the entity the camera looks at (we keep this weird behavior from Half-Life by default in order to maintain compatibility).
* `Don't slow down if not Freeze Player` - in Half-Life `trigger_camera` slows down its movement if `Freeze Player` spawnflag is not set. It's unknown why it's coded this way. Tick this flag to prevent such behavior.
* `Don't skip the first path corner` - in Half-Life `trigger_camera` actually skips moving to the first path corner path. Tick this flag to fix this behavior.
* `Instant Turn` - instantly turn to the target on activation. By default camera starts at its angle (as it's set in the editor) and then gradually turns to the target. Set this spawnflag when such behavior is not desired.

### Bugfixes

* The camera view is getting restored when the saved game is loaded.
* When `trigger_camera` is removed from the map, the player gets their view reset and gains the control back (if `Freeze Player` was set).
