---
bookHidden: true
bookToC: false
---

# game_messagebox

Show a [message box]({{< ref message-boxes >}}) to the player.

The player can close the message box by pressing the primary attack button. If multiple `game_messagebox` are active the player will be closing them all in reverse order.

### Parameters

* `Message ID` - the message ID from [translatable messages]({{< ref "text-messages/#translatable-messages" >}}).
* `Autoclose distance` - if player is farther than this distance from the message box origin position the messagebox will automatically close. If the player is too far at the moment of activation it won't show the message box at all.
* `Origin position` - use another entity to treat as an origin position when checking if the message box must be autoclosed.

### Spawnflags

* `All Players` - whether to show the message box to all players.
