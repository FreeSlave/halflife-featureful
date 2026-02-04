---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_train

[TWHL](https://twhl.info/wiki/page/func_train)

See also: [env_spritetrain]({{< ref env_spritetrain >}}), [env_modeltrain]({{< ref env_modeltrain >}})

### New parameters

* ![](/images/spirit.png) `Custom move sound`.
* ![](/images/spirit.png) `Custom stop sound`.
* `Sound radius` - configure platform sounds attenuation.
* ![](/images/svencoop.png) `Obey Trigger Mode` - make train interpret the use-type when triggered (`On` is to start, `Off` is to stop).
* Move sound pitch can be configured.
* `Collision with corpses` - allows to ignore collision with dead monsters. Corpses won't block this train and won't get crushed.
* `Instantly gib corpses` - instantly gib corpses that block the train, even if train deals 0 damage and `No default damage` spawnflag is set.
* `How to handle tiny creatures` - override handling of [tiny creatures]({{< ref "monsters/#tiny-creatures" >}}).

### New spawnflags

* ![](/images/spirit.png) `Origin on paths` - make the train to position itself on the path entities using its origin instead of geometric center.
* ![](/images/spirit.png) `Initially On`.
* `No default damage` - by default in Half-Life if `Damage on crash` for the train is set to 0, it sets the damage to 2 on spawn. Use this spawnflag to prevent such behavior.

### Reaction to use-type

Always percieved as `Toggle` (due to compatibility with Half-Life trains) unless `Obey Trigger Mode` parameter is enabled. Activates or stops the train.

### Bugfixes

* Fixed a bug when train did not stop playing its sound when stopped by trigger.
