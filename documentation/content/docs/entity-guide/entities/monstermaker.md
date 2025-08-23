---
bookHidden: true
bookToC: false
---

# monstermaker

[TWHL](https://twhl.info/wiki/page/monstermaker)

### New parameters

* `Delay after blocked` - delay before the next spawn attempt if previous was blocked by another monster. If not specified, the 'Delay between spawns' parameter is used as a delay.
* `Target's Activator` - activator for `Target On Release`. E.g. can be set to `Spawned monster` to make a monster act as an activator for the trigger.
* `Spawn position [LP]` - a position to spawn a monster at.
* `Face to [LP]` - the spawned monster will face to this position. If set to `*player` it faces to the closest alive player.
* `Warpball` - the name of the warpball template or `env_warpball` entity to play along with the monster spawn. The warpball template search is prioritized over the entity search.

### New spawnflags

* ![](/images/svencoop.png) `Auto size BBox` enables a more precise calculation of free area when checking if monster fits the area. This flag is ticked by default in fgd. Without this flag the monstermaker will use a rough calculation which often exaggerates the real size of the monster thus not allowing a monster to spawn even if they would have enough space to fit.
* ![](/images/svencoop.png) `Cyclic backlog` - use together with `Cyclic`. Keep track of the number of monsters who could not spawn at the time of monstermaker's firing (e.g. if the spawn place was blocked). The monsters from backlog will spawn later.
* `No ground check` removes the check for unoccupied space under the monstermaker. Useful for spawning flying monsters or monsters that should fall. Without this flag the monstermaker will avoid spawning a monster if another monster is currently underneath.

### Choosing a spawn point

By default monstermaker creates a monster using its own origin as a spawn point. The `Spawn position [LP]` parameter allows to set a different entity as a spawn point.

You can also randomize the spawn position:

1. Create multiple `info_target` entities
2. Give them the identical targetname to all of them.
3. Set this name as `Spawn position [LP]` value prepending the name with `@` symbol (e.g. if you name info_targets *position*, set the parameter value to *@position*).

This is useful if monstermaker can spawn multiple monsters and you want them to appear in different places on each spawn.

### Configuring the spawning monster

Unlike original Half-Life, the `monstermaker` now can define custom health, body, skin, blood color, relationship class, gibs, model and other properties for the spawned monsters.

Even if `monstermaker` lacks some property specific for the monster, one can define it via custom keyvalues. In the editor toggle SmartEdit (like you do with `multi_manager`) and add keyvalues you want to forward to the monster prepending the key names with # symbol. This allows to set some parameters that can't be set via monstermaker otherwise (e.g. bodystate of `monster_barney`). Example:

```
#bodystate 1
```

### Bugfixes

* Flag `Start ON` is working now (was bugged in Half-Life).
* Dead monsters no longer block the spawn point.

### Notes

* The `monstermaker` deletes itself if it doesn't have any more entities to spawn. In original Half-Life the monstermaker entity stayed on the map in this case.
* The alias [squadmaker]({{< ref squadmaker >}}) exists for this entity, purely for compatibility with Sven Co-op.
