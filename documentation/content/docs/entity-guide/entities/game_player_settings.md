---
bookHidden: true
bookToC: false
---

# game_player_settings

This entity allows to give/remove weapons and ammo to the player, configure player's health, armor and some other settings.

The `game_player_settings` without a targetname is triggered automatically when player spawns in the new game unless the [map .cfg]({{< ref map-config >}}) file exists (then it takes the priority and contents of `game_player_settings` are ignored).

### Parameters

* Parameters to give or remove ammo of certain type. Use negative values to remove ammo.
* Parameters to give or remove weapons.
* `Suit` - give or remove HEV suit. Note that removing the suit doesn't reset the armor. You may want to set `Set armor` to `=0`.
* `Suit light` - give or remove light device - flashlight or NVG.
* `Longjump` - give or remove longjump.
* `Set health` - set current player's health.
* `Allow OverHeal` - whether the new health value can exceed the maximum player's health value.
* `Set max health` - set player's maximum health. If the new maximum health is less than previous one, the current health will be clamped to the new maximum (unless `Allow OverHeal` is set).
* `Set armor` - set current player's armor.
* `Allow armor OverCharge` - whether the new armor value can exceed the maximum player's armor value.
* `Set max armor` - set player's maximum armor. If the new armor health is less than previous one, the current armor will be clamped to the new maximum (unless `Allow armor OverCharge` is set).

{{% hint info %}}
With `Set health`, `Set max health`, `Set armor`, `Set max armor` you can use the following value prefixes:
* `+` - add to the current value, e.g. `+20`.
* `-` - subtract from the current value, e.g. `-20`. The max health can't go below 1. The armor and max armor can't go below 0. Using `Set health` with subtracting value may kill the player if they don't have enough health.
* `=` - set the current value, e.g. `=120`. This is the same as value without prefix, but allows to set armor or max armor to 0 (`=0`). Health and max health can't be set to 0.
* `<` - set the specified value or do nothing if the current player's value is already lower than this value. E.g. setting `Set health` to `<80` will set the current health to 80 or leave it without a change if it's already lower.
* `>` - set the specified value or do nothing if the current player's value is already higher than this value. E.g. setting `Set health` to `>80` will set the current health to 80 or leave it without a change if it's already higher.
{{% /hint %}}
