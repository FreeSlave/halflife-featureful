---
bookHidden: true
bookToC: false
---

# player_capabilities

Enables/disables certain player's capabilities.

### Parameters

* `Attack capability` - configure the attack capability. When this is disabled the player won't be able to fire any weapon, neither by primary attack nor secondary one. If the attack capability has been disabled while player is in scope, the scope will reset automatically, to prevent situations when player can't unscope because they can't press the secondary attack.
* `Jump capability` - configure the jump capability.
* `Duck capability` - configure the duck (crouch) capability. Note: disabling the duck capability also prevents duck-jumping technique and longjump usage.
* `Use capability` - configure the use capability. When this is disabled the player won't be able to interact with buttons.
* `Step sounds` - configure whether the player's steps make sounds (this doesn't affect the step sounds as perceived by monsters' AI).
* `Save game capability` - configure whether the player can save the game (manually or through the quicksave). This doesn't affect autosaves. While the saves are disabled the attempt to save the game will show the `SAVE_DISABLED` message from **titles.txt** (if such message exists).

### Reaction to use-type

Changes the player's capability depending of the entity parameters and input type (if some option is set to `Copy use input`).
