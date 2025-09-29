---
bookHidden: true
bookToC: false
---

# player_calc_ratio

The entity reports the selected player value as its Calc Ratio. Can report the current health, armor, ammo amount, the inventory item count and maximum speed. Use it together with [trigger_compare]({{< ref trigger_compare >}}) to implement the map logic that depends on these player parameters.

### Parameters

* `Calc Param Type`
    - `Health` - current health value. If fraction is requested than the *current health* / *max health* ratio is returned. This might exceed 1 if player got overhealed (e.g. via [game_player_settings]({{< ref game_player_settings >}})).
    - `Armor` - current armor value. If fraction is requested than the *current armor* / *max armor* ratio is returned. This might exceed 1 if player got overcharged (e.g. via [game_player_settings]({{< ref game_player_settings >}})).
    - `Ammo` - current ammo count. This requires `Ammo name` to be specified, e.g. `9mm`. The result doesn't include the ammo loaded in the weapon clips. If fraction is requested, the *current ammo count* / *maximum ammo count* ratio is returned.
    - `Ammo, with clip` - current ammo amount, including ammo in weapon clips. This requires `Ammo name` to be specified, e.g. `9mm`. If fraction is requested the clip capacities of weapons are added to the maximum ammo count when calculating the *current ammo count* / *maximum ammo count* ratio.
    - `Inventory item count` - current inventory item count. This requires `Inventory item name` to be specified. If fraction is requested, the *current item count* / *max item count* is returned. This might exceed 1 if item count has been overflown (e.g. via [player_inventory]({{< ref player_inventory >}})). If inventory item doesn't have a limit, the value 1 is always returned.
    - `Maximum speed` - current player's maximum speed. If fraction is requested the *current max speed* / *default max speed* is returned. The current maximum speed can be changed via [player_speed]({{< ref player_speed >}}). Note: the reported speed doesn't take into account the [maximum speed]({{< ref "weapon-templates#player_maxspeed" >}}) set by an active weapon.
* `Calc value notion` - whether to retrieve value as an absolute value or as a fraction of the maximum value.
* `Ammo name` - ammo name for `Ammo` and `Ammo, with clip` requests.
* `Inventory item name` - inventory item name for `Inventory item count` request.

### Calc Ratio

Reports the result of calculation.
