---
bookHidden: true
bookToC: false
---

# trigger_changemaxclip

Dynamically change the max clip of the weapon.

If player doesn't have the specified weapon, nothing happens.

If weapon doesn't have a clip (e.g. `weapon_gauss`), nothing happens.

### Parameters

* `Weapon name` - weapon entity name, e.g. `weapon_9mmAR`. The name must be exact, with proper case.
* `Value to use` - value to use in operation. Must be positive number.
* `Change Max Clip Mode` - the operation type.
    - Set - set exact max clip.
    - Add - increase the max clip.
    - Substruct - decrease the max clip.
* `Handle ammo excess` - how to handle the case when the current ammo amount in the clip is bigger than the new limit.
    - Truncate - truncate to new maximum.
    - Move to ammo - move the excessive ammo to the ammo reserve.
    - Keep as is - leave the current clip as is.
