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

### New spawnflags

* ![](/images/spirit.png) `Direct use only` - require a direct non-occluded view of entity in order to use it (superceded by a new parameter `Player use policy`)
* ![](/images/spirit.png) New flag `Player Can't Use` to disallow player use (button still can be called via other entities)
