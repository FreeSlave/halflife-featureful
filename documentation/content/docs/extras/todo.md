---
title: "TODO"
---

# TODO

This is the list of features that would be nice to have in our codebase. You are welcome to do a PR!

## Monsters

### New monsters

* ![](/images/decay.png) monster_alienflyer
* ![](/images/svencoop.png) monster_bodyguard
* ![](/images/svencoop.png) monster_kingpin
* ![](/images/svencoop.png) monster_stukabat
* monster_archer (probably should shoot spikes like pitdrone or some kinds of electric bolts)
* ![](/images/tfc.png) Engineer's Sentry Gun (3 different levels)
* Drones from Sweet Half-Life
* ![](/images/svencoop.png) Chumtoad from Sven Co-op or Todesangst 2
* Melee weapon humans like in Condition Zero: Deleted Scenes or Point of View.

### Monster's abilities

* Support for security guards reload (in Half-Life they never need to reload).
* ![](/images/svencoop.png) Ability of scientists and medics to revive allies (including player).
* ![](/images/svencoop.png) Alien grunts throwing snark nests.
* The ability for blackops assassins to throw flash grenades.
* Leap ability for gonomes.
* Zombies that spawn headcrabs when they're killed not in the head.

### Monster configuration

* Configuration of monster's weapon (e.g. let barneys wield shotguns, let human grunts wield sniper rifles, etc.). This should allow to implement variety of human-like npcs via entity templates, e.g. HEV scientists from Sweet Half-Life.
* Configuration of monster's dropped items.
* The option to turn off certain attacks for monsters (e.g. to make melee-only alien grunts).

## Weapons

### New weapons

* ![](/images/svencoop.png) weapon_uziakimbo
* ![](/images/tfc.png) TFC Flamethrower
* ![](/images/tfc.png) TFC Engineer's blaster (railgun)
* DMC/Quake lightning gun
* ![](/images/decay.png) Vortigaunt hands
* ![](/images/cstrike.png) flash grenade - should also make monsters temporarily blind
* Plasma gun from Sweet Half-Life

### New attacks

* ![](/images/svencoop.png) Secondary attack for shock roach
* ![](/images/svencoop.png) Throwable crowbar/knife

### Throwable grenades from TFC

* ![](/images/tfc.png) nail grenade
* ![](/images/tfc.png) napalm grenade
* ![](/images/tfc.png) MIRV grenade

### Weapon configuration

* Support for holster animations.
* Configuration of some visual effects on view models (e.g. beams on shockrifle and displacer).

## Player upgrades/effects

A generic way to describe player upgrades/effects. It should support creation of something similar to DMC artifacts and Opposing Force CTF runes.

## Monster status effects

Status effects like burning, frozen, electrified, stunned, poisoned, "silenced" (can't use abilities), etc. Status effects should be applied by attacks, environments or triggers.

## New entities

* ![](/images/spirit.png) env_beamtrail
* ![](/images/spirit.png) env_elight
* ![](/images/tfc.png) Dispenser - entity that gives player ammo and armor on touch.

## Entity features

* ![](/images/spirit.png) Support for Movewith from Spirit of Half-Life.
* ![](/images/svencoop.png) Ability for player to repair ally turrets and breakables with a pipe wrench.
* ![](/images/svencoop.png) Breakable doors

## Client-related features

### Visual effects

* ![](/images/spirit.png) Particle system from Spirit.
* Hit particles for materials (like in Day of Defeat).
* ![](/images/cstrike.png) Smoke effect (like smoke grenades from Counter-Strike)
* ![](/images/cstrike.png) Grass
* ![](/images/svencoop.png) Splash effects when firing at water (like in Sven Co-op or Day Of Defeat)
* Decorative dangling ropes (similar to move_rope/keyframe_rope from Source).

### HUD

* Floating combat text - damage numbers appearing when monster is hit.
* Radar from Counter-Strike (configurable in regard to what entities should be displayed on the radar and how)

### Global stuff

* Support for bigger maps.
* 3D Skybox implementation.
* Ability to build game libraries with server-only weapons (e.g. for singleplayer-only mods)

## Multiplayer features and modes

Multiplayer (especially deathmatch) is not focus of Featureful SDK, so this is very low priority stuff.

* Join team interface should work in team deathmatch.
* Out of the box vote system for map change and player's kick/ban.
* ![](/images/opfor.png) ![](/images/tfc.png) Capture The Flag support (for Opposing Force, Adrenaline Gamer and Team Fortress Classic maps)
* Last man standing mode (Adrenaline Gamer)
* Duel arena mode (Adrenaline Gamer)
* Configuration to spawn players as spectators first when they join the game.
* A command to prevent any players from further joining except as spectators.
* Support for `.loc` files from Adrenaline Gamer. Or another way to mark locations in multiplayer (might be useful for singleplayer as well!).
