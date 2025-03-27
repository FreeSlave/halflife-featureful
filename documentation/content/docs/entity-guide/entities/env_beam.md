---
bookHidden: true
bookToC: false
---

# env_beam

[TWHL](https://twhl.info/wiki/page/env_beam)

### New spawnflags

* `Draw Solid` - draw a solid non-additive beam. This will make the beam look kinda like a ribbon.
* `Draw Sine` - draw a wavy beam, similar to the one projected from Gluon Gun.
* `No Camera Punch` - don't punch the player's view camera when dealing damage.

### Bugfixes

* Fixed a bug that made beams have an incorrect direction after save-restore in some cases (e.g. when `env_beam` is set to be its own `Start Entity`).
* Ring beams with infinite duration continue playing after save-restore.
