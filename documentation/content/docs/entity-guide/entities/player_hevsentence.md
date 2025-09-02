---
bookHidden: true
bookToC: false
---

# player_hevsentence

Plays HEV sentence, queued or at the moment of triggering.

### Parameters

* `Sentence` - HEV sentence to play. Can be a group name (e.g. `HEV_MED`) or a specific sentenc name `!HEV_PISTOL`.
* `No repeat time` - the time in seconds to not play the sentence if it was played recently. Applied only if `Queue sentence` spawnflag is set.

### Spawnflags

* `Queue sentence` - whether to put HEV sentence into queue instead of playing right away. This might play the sentence later if there have been other previously queued sentences.
