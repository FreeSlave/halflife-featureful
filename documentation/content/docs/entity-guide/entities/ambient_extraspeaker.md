---
bookHidden: true
bookToC: false
---

# ambient_extraspeaker

Similar to [ambient_generic]({{< ref ambient_generic >}}) but designed to play a sound on another entity. It doesn't occupy the sound channels of another entity, so it's safe to use without fearing that it interrupts the entity's sound or that the entity interrupts the sound played by `ambient_extraspeaker`. Instead it creates a special entity that follows another entity around basically acting as an extra speaker with its own set of sound channels.

This entity can be used for radio transmissions or other sounds that are supposed to play in the player's head, i.e. without the attenuation affecting the sound.

### Parameters

* `Attach to entity` - the name of the entity to play the sound on. This can be `*player` to play the sound that follows the player.
* `Channel to use` - the channel of the extra speaker to use.
