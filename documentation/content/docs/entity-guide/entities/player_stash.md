---
bookHidden: true
bookToC: false
---

# player_stash

This entity "stashes" the player's weapons, inventory items, health and armor values (depending on the entity configuration). When fired again, it "unstashes" (returns) them to the player who activated the entity.

Use `player_stash` in order to temporary remove player's equipment and return it later.

A single instance of the entity can't store info again without being unstashed prior.

### Spawnflags

* `No HUD notifications on unstash` - don't show the notifications in pickup history on unstash.

### Reaction to use-type

Acts depending on the input type and whether the entity has the stashed equipment. E.g. if `player_stash` already holds some items, it won't do anything when called with `On` input type.
