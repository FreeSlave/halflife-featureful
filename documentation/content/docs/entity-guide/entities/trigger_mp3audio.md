---
bookHidden: true
bookToC: false
---

# trigger_mp3audio

Plays an arbitrary mp3 audio file from the mod directory using the Music volume set by client.

### Parameters

* `Track name` - the path to the mp3 file relative to the mod directory. If empty, it stops currently playing track, if any.

### Spawnflags

* `Remove on fire` - remove this entity after it gets triggered.
* `Looped` - whether the track should be looped. If client loads the save where this entity has been previously activated, the track will start playing again.

### Reaction to use-type

If the input is `Off` or if the input is `Toggle` and the entity has been previously activated, it stops playing the current track. Otherwise it restarts the track.
