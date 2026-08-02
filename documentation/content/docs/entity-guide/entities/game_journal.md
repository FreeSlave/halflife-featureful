---
bookHidden: true
bookToC: false
---

# game_journal

Entity to control the player's [journal]({{< ref journal >}}).

### Parameters

* `Journal Section` - the ID of the journal section to update the message in.
* `Journal Record` - the message ID for the [text]({{< ref "text-messages/#translatable-messages" >}}) to set in the selected journal section. Leaving it blank will erase the current message.

### Spawnflags

* `Repeatable` - don't remove entity after firing.
* `Don't notify` - don't notify about journal update (in case the section has notification settings).
