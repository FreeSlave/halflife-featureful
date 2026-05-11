---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_button

[TWHL](https://twhl.info/wiki/page/func_button)

### New parameters

* `Player use policy` - choose an algorithm of detecting if player can use a button.
* ![](/images/svencoop.png) Custom sounds (including locked and unlocked sounds and sentences).
* New parameter `Use Type` to set the passed use type to a button's target (On or Off).
* `Trigger on return` - target to trigger when button is back. This is for moveable buttons.
* `Trigger before move/rotate` - target to trigger when button has been activated but hasn't moved (or rotated for [func_rot_button]({{< ref func_rot_button >}})) yet. This is for moveable buttons.
* `Delay before toggling again` - delay before the button with `Toggle` spawnflag can be pressed again (by default there's no delay).
* `Target on locked` - target to trigger if button is used when locked by master. Delay between such usages is 2 seconds.

### New spawnflags

* ![](/images/spirit.png) `Direct use only` - require a direct non-occluded view of entity in order to use it (superceded by a new parameter `Player use policy`)
* ![](/images/spirit.png) New flag `Player Can't Use` to disallow player use (button still can be called via other entities). Note: if you just want a brush with a switchable texture use [func_wall]({{< ref func_wall >}}).
