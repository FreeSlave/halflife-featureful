---
title: "AI sounds"
---

# {{% param "title" %}}

Half-Life has a concept of abstract sounds (or *scents*) that can be sensed by monsters. It's somewhat important part of the Half-Life AI, so we want to describe it in this article along with any changes the Featureful SDK has to offer in relation to this concept.

Despite the name these are not audible sounds but rather temporary marks/hints on the map (thus they're also used for scents). Usually the AI sounds are created by some in-game events (e.g. firing a weapon or a grenade being about to explode). Monsters may react to these sounds if they're close enough.

See also:

* [ai_sound](https://developer.valvesoftware.com/wiki/Ai_sound) - the same concept described for Source on Valve Developer Community.
* [Sound, smells and the system behind them](https://twhl.info/wiki/page/Monsters_Programming_-_Sounds%2C_smells_and_the_system_behind_them) - the TWHL article on the AI sound system. This is however written as an overview from a programming point of view.

## Properties

Each sound or scent has the following properties:

* Origin - location in space where the sound/scent was created.
* Type - the type of sound (more on that later). The type can be composite i.e. one sound can have a union of several types.
* Volume - radius of the sound (so nearby monsters can hear it).
* Duration - for how long the sound lingers.

## Types

There're several types of sounds/scents in Half-Life. Monsters can sense certain types of sounds/scents and react to them appropriately.

### Combat

Sounds that are usually produced by firing a weapon (both by monsters and players). Typical monster's reaction to this type of sound is to turn to the sound origin. Under certain conditions monsters might go closer to the sound origin in order to "investigate" what happened there:

* The monster is in active alert after combat (if `Active alert after combat` parameter is set or `npc_active_after_combat` is enabled).
* The monster is in 'hunt' state after their squad member was killed by a yet unknown enemy.
* [monster_human_assassin]({{< ref monster_human_assassin >}}) always investigates the combat sounds.

### Danger

Sounds of pending danger (e.g. something is about to explode). Produced by grenades and spores. Humans and humanoid monsters (except zombies) can detect this sound and flee from its origin in attempt to escape the danger.

The danger sound is produced by the following entities:

* [grenade]({{< ref grenade >}}) - as hand grenade hits something and 1 second before it detonates, or as AR grenade is getting launched.
* [spore]({{< ref spore >}}) - as spore grenade is bouncing off something.
* [func_mortar_field]({{< ref func_mortar_field >}}) - before the first mortar strike.
* [mortar_shell]({{< ref mortar_shell >}}) - as it flies.

### Player

Sounds produced by a player character and his weapons.

* Step sounds as the player moves. The sound volume depends on the movement speed.
* Player falling to the ground after jump.
* Firing a weapon. Produced in union with [Combat](#combat) sound.

### World

This was supposed to be produced by doors and breakables but left almost unused in-game.

* If something triggers the [monster_tentacle]({{< ref monster_tentacle >}}) with an `On` use-type the world sound is created at the activator's origin which may draw the tentacle's attention. Same applies to [monster_pitworm_up]({{< ref monster_pitworm_up >}}).
* Squishing a cockroach produces a world sound.

### Carcass

A scent produced by a monster after they finish the dying animation. This attracts some monsters:

* [monster_bullchicken]({{< ref monster_bullchicken >}}).
* [monster_cockroach]({{< ref monster_cockroach >}}).
* [monster_houndeye]({{< ref monster_houndeye >}}).
* [monster_pitdrone]({{< ref monster_pitdrone >}}).

### Meat

A scent produced by gibs that can bleed (created after the monster's body has been gibbed or by `gibshooter`).

In terms of monsters' reaction this is basically the same as [Carcass](#carcass).

### Garbage

This scent was supposed to be produced by trash cans but it was never implemented in the game.

## Mapping

Original Half-Life didn't allow level-designer to do much with AI sounds. The `Trigger Condition` parameter on monsters allowed to fire a certain target when monster hears a combat, player or world sound and that was it.

Featureful SDK provides some new features in this regard.

### env_soundmark

[env_soundmark]({{< ref env_soundmark >}}) allows to produce AI sounds by a trigger. This can be used, for example, to scare away some monsters who normally would flee from danger sound. Or attract a monster who would normally be attracted to the combat sound or some scent.

### Warpballs

[env_warpball]({{< ref env_warpball >}}) and [monstermaker]({{< ref monstermaker >}}) with a warpball template set can produce combat and danger sounds if configured to do so. This can be used to draw attention of nearby monsters to the teleportation effect (so e.g. security guard would recognize that some monster spawned behind his back and turn to it).

### Custom soundmask

Soundmask is a set of sounds/scents the monster can hear/smell. The default soundmask depends on the monster type. E.g. alien slave do listen to danger, combat and player sounds but don't care for scents, while bullsquids smell meat and carcass but can't recognize the danger sound.

The soundmask can be changed via the monster's `Custom Sound Mask` parameter. For example it's possible to make the monster deaf to player's footsteps to prevent the monster turning when the player is approaching from behind.

{{% hint info %}}
Monsters who see player as a friend usually don't react to his footsteps. This is done so ally monsters won't constantly turn to the player when he moves around like if they were watching him with suspicion.
{{% /hint %}}

## Debugging

The `displaysoundlist` allows to report changes in the AI sound list. This cvar existed in vanilla Half-Life but it required the level restart (or reload) after being set to 1 in order for the reporting to start.

In Featureful SDK its behavior has changed:

* Changing the `displaysoundlist` value doesn't require the level restart: the reporting starts or stops right away.
* When `displaysoundlist` is set to 1 it reports the number of active sounds in the console every 0.3 seconds but only if the number of sounds has been changed inbetween (in Half-Life it has been reported unconditionally). This also reports insertions and expirations of sounds.
* When `displaysoundlist` is set to 2 it does the same as above and additionally visualizes the sound locations with temporary beams. The beam color denotes the sound type: red means danger, orange means combat and pink means carcass/meat.

## Notes

{{% hint warning %}}
Currently the AI sounds don't get saved, therefore they're getting lost on save-restore.
{{% /hint %}}
