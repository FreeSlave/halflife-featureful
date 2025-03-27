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
* ![](/images/svencoop.png) `Ignore Hold Time`.
* `Alive player only` - prevent camera activating on the dead player (which is possible in Half-Life).
* `Don't fire look target` - prevent firing the entity the camera looks at (we keep this weird behavior from Half-Life by default in order to maintain compatibility).

### Bugfixes

* The camera view is getting restored when the saved game is loaded.
