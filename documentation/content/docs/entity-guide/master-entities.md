---
title: "Master entities"
bookToC: false
---

# {{% param "title" %}}

In original Half-Life there's a concept of **Master** entity that allows to "lock" other entities (e.g. prevent doors from opening) until the Master entity state is On. 

The only entities capable of being Master in Half-Life are `multisource` and `game_team_master`. The `multisource`, however, might be quite confusing and hard to use as master. That's why Half-Life Featureful SDK introduces more ways to implement Masters.

### Master capable entities

* [env_state]({{< ref env_state >}})
* [calc_state]({{< ref calc_state >}})
* [player_hasitem]({{< ref player_hasitem >}})
* [player_hasweapon]({{< ref player_hasweapon >}})
* [player_hasinventory]({{< ref player_hasinventory >}})
* [trigger_compare]({{< ref trigger_compare >}})

### Globals as masters

In Half-Life Featureful SDK global variables can be used as Master entities as well. Use `env_global` with `Act as Master` spawnflag set. Then set the `env_global` targetname (the entity's name, not the underlying global variable name!) as Master for other entity. If the underlying global variable state is On, then the Master will be considered enabled. Otherwise (Off and Dead states, or if the global variable was not initialized) it's considered disabled. This allows to implement cross-level masters easily without resorting to using `multisource` or `trigger_auto`.

### Inverting master check

![](/images/spirit.png) Reaction of the entity to its master's state can be reverted by adding `~` symbol at the start of the referred name.

### Other entities as masters

Most entities can't be used as masters. It's different in *Spirit of Half-Life* where every entity has the associated state and can be used as a master. In Featureful SDK we still stick to more classic approach where only few entities have Master capability. This might change in future (preferably it should be a configurable feature though).
