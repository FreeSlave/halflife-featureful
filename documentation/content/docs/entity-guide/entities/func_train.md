---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_train

[TWHL](https://twhl.info/wiki/page/func_train)

### New parameters

* ![](/images/spirit.png) `Custom move sound`.
* ![](/images/spirit.png) `Custom stop sound`.
* `Sound radius` - configure platform sounds attenuation.
* ![](/images/svencoop.png) `Obey Trigger Mode` - make train interpret the use-type when triggered (`On` is to start, `Off` is to stop).
* Move sound pitch can be configured.

### New spawnflags

* ![](/images/spirit.png) `Origin on paths` - make the train to position itself on the path entities using its origin instead of geometric center.
* ![](/images/spirit.png) `Initially On`.

### Reaction to use-type

Always percieved as `Toggle` (due to compatibility with Half-Life trains) unless `Obey Trigger Mode` parameter is enabled. Activates or stops the train.

### Bugfixes

* Fixed a bug when train did not stop playing its sound when stopped by trigger.
