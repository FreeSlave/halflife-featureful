---
bookHidden: true
bookToC: false
---

# env_fade

[TWHL](https://twhl.info/wiki/page/env_fade)

### Changes

* The fade effect is now getting saved and restored so it doesn't get lost upon save-reload.

### New spawnflags

* `Directional blind` - make fade alpha depend on the angle between viewer camera and `env_fade` origin. Can be used to simulate flashbang-like effect - so the player won't get blinded when not looking at the fade source.
