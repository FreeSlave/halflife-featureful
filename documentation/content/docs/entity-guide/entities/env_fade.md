---
bookHidden: true
bookToC: false
---

# env_fade

[TWHL](https://twhl.info/wiki/page/env_fade)

### Changes

* The fade effect is now getting saved and restored so it doesn't get lost upon save-reload.
* Fade now can last longer than 16 seconds (in original Half-Life it's limited to 16 seconds max).

### New spawnflags

* `Directional blind` - make fade alpha depend on the angle between viewer camera and `env_fade` origin. Can be used to simulate flashbang-like effect - so the player won't get blinded when not looking at the fade source.
