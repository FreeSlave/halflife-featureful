---
bookHidden: true
bookToC: false
---

# game_player_settings

This entity allows to give/remove weapons and ammo to the player, configure player's health, armor and some other settings.

The `game_player_settings` without a targetname is triggered automatically when player spawns unless the [map .cfg]({{< ref map-config >}}) file exists (then it takes the priority and contents of `game_player_settings` are ignored). Use this entity to configure the starting equipment and other settings for the player, in the new game or when the map has been loaded via console command.
