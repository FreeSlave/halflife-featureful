---
bookHidden: true
bookToC: false
---

# env_render

[TWHL](https://twhl.info/wiki/page/env_render)

### New parameters

* ![](/images/spirit.png) `Scale` - change the object scale (applies to both sprites and models).
* ![](/images/spirit.png) `Fade Time` - the time in seconds during which the object render parameters gradually change to the specified ones. Also affects the `Scale` change if it's specified.
* ![](/images/spirit.png) `Fade Coarseness` - time in seconds between the fading updates.
* ![](/images/spirit.png) `Trigger after fading` - fire after the object render parameter have changed.

### New spawnflags

* ![](/images/spirit.png) Remove Target - remove the affected entity when fade is done.
* ![](/images/spirit.png) Remove Self - remove itself when fade is done.
