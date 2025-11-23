---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) trigger_playerfreeze

Disables any player movement (including falling to gravity or being pushed), weapon attacks, the use input and flashlight control. Same effect as when player is frozen by [trigger_camera]({{< ref trigger_camera >}}).

Should be triggered again to let player regain control.

See [player_capabilities]({{< ref player_capabilities >}}) for more granular control over what player can or can't do.

### New parameters

* `Use type` - how this entity interprets the input when getting trigged. By default it's always 'Toggle' (freeze or unfreeze player depending on the current state). 'Unfreeze' means the player will either be unfrozen or nothing happen if player is already unfrozen. 'Freeze' means the player will either be frozen or nothing happen if player is already frozen. The 'Copy input' makes the entity interpret the input type ('On' is to freeze the player, 'Off' is to unfreez, and 'Toggle' is to toggle the state).

### Reaction to use-type

Depends on the `Use type` parameter.
