---
bookHidden: true
bookToC: false
---

# game_text

[TWHL](https://twhl.info/wiki/page/game_text)

### Parameters

* The `Text Effect` parameter got a new option - `Use console font`. This makes the message to be printed in the console font, rather than additive font.
* `Fire when done` - a target to fire when the message disappears. The delay is calculated as a sum of fade in time, fade out time and hold time.

### Bugfixes

* Fixed the game crash when message has a line longer than 79 characters.
* Calling this entity without activator doesn't crash the game anymore.
