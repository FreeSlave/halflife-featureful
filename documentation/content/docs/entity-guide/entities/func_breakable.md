---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_breakable

[TWHL](https://twhl.info/wiki/page/func_breakable)

### New parameters

* ![](/images/svencoop.png) Most of spawnobject values are compatible with those in Sven Co-op.
* `Relative to attack` option is now working as intended (was buggy in Half-Life).

### New spawnflags

* ![](/images/svencoop.png) `Explosive Only` - make the breakable immune to any non-explosive damage.
* `Op4Mortar only` - make the breakable resistant to anything besides `op4mortar` shells.
* `Not solid` - make breakable non-solid (can be destroyed only with trigger).
* `Smoke trails on gibs` - add gray particle trails on gibs (using the default effect from the engine).
* `Transparent gibs` - make gibs semi-transparent.

### Bugfixes

* Fixed `func_breakable` taking damage from some attacks even if `Only Trigger` spawnflag is set (original Half-Life bug).
