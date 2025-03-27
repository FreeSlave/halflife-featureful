---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_door

[TWHL](https://twhl.info/wiki/page/func_door)

### New parameters

* ![](/images/spirit.png) `Direct use only` - require a non-occluded view of entity in order to use it.
* ![](/images/svencoop.png) `Ignore Targetname` allows to open named doors by touching (same as the flag above, but in the parameter form).
* ![](/images/svencoop.png) Custom Move and Stop sounds.
* ![](/images/svencoop.png) Custom `Locked` and `Unlocked` sounds and sentences.
* `Sound radius` - configure door sounds attenuation.
* ![](/images/spirit.png) `Fire On Open` - fire when door has opened. This parameter is added for compatibility with Spirit of Half-Life. You can use `Fire On Open End` instead.
* ![](/images/svencoop.png) `Fire On Start`, `Fire On Stop` and corresponding parameters to denote trigger states. Fire when door starts or stops moving (either direction).
* ![](/images/svencoop.png) `Fire On Open Start`, `Fire On Close Start`, `Fire On Open End` and `Fire On Close End` and corresponding parameters to denote trigger states.
* ![](/images/svencoop.png) `Obey Trigger Mode` - make doors interpret the use-type when triggered (`On` is to open, `Off` is to close). By default in Half-Life doors ignore the input use-type.
* `Collision with corpses` - allows to ignore collision with dead monsters. Corpses won't block this door and won't get crushed.
* `Continue moving if blocker is crushed`. By default this behavior is controlled globally by the `doors_blocked_recheck` feature. This option allows to configure the behavior specifically for the door instance.
* `If locked, play locked sound on use` - allows to enable playing the locked sounds for usable doors when the door is locked by a master. In Half-Life usable doors don't play the locked sounds.

### New spawnflags

* ![](/images/spirit.png) `Force Touchable` - allows to open named doors by touching.

### Reaction to use-type

Depends on the `Obey Trigger Mode` parameter.

### Features

* `doors_blocked_recheck`. Server feature. Re-check if blocking entity is destroyed by door's damage and the door can continue moving. In Half-Life the door returns to its previous position when blocked by something even if the obstacle is destroyed by the damage inflicted by door.
* `doors_blocked_fade_corpses`. Server feature. Whether killed monsters are getting non-solid and fade if they block the door movement. Set it to true if you want to ensure that corpses won't block the doors.
