---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_conveyor

[TWHL](https://twhl.info/wiki/page/func_conveyor)

### New parameters

* `Use Set Policy` - configure how the conveyor reacts to the *Set* use-type. Allows to control the conveyor speed via the `game_counter_set` entity.

### Reaction to use-type

If the use-type is *Set* the reacton depends on `Use Set Policy` parameter. By default it's the same as in Half-Life (ignore the value and reverse the speed like with other use-types). This can be changed to set the passed value as a new speed or add the passed value to the current speed (use negative numbers to substract).
