---
bookHidden: true
bookToC: false
---

# game_counter

[TWHL](https://twhl.info/wiki/page/game_counter)

### New spawnflags

* ![](/images/svencoop.png) `Fire if over limit` - fire the target even if the internal value happens to be over the limit after triggering.

### Calc Ratio

Reports the counter value.

### Reaction to use-type

Increases the counter by 1 for `Toggle` and `On`. Decreases the counter by 1 for `Off`. Sets the counter for `Set` (e.g. if triggered by `game_counter_set`). In any case this checks if the limit is hit after the operation.
