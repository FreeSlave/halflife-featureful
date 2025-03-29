---
title: "Player"
bookToC: false
---

# {{% param "title" %}}

Player is represented by `player` entity in-game.

### Special entity names

Certain player related events automatically trigger entities with special targetnames, providing the player as activator (in most cases).

See also: [Special Entity Names on TWHL wiki](https://twhl.info/wiki/page/Special_Entity_Names)

#### game_playerspawn

Triggered when player spawns. This can be used to configure some player parameters. It also can serve as an indicator that the map was loaded via a *Start game* menu or a console command and not through the changelevel.

In original Half-Life entities named `game_playerspawn` are triggered only in multiplayer. Featureful SDK fires them in singleplayer as well.

#### game_playerdie

Triggered when player dies.

In original Half-Life entities named `game_playerdie` are triggered only in multiplayer. Featureful SDK fires them in singleplayer as well.

### Skill variables

* **sk_flashlight_drain_time** - time in seconds for flashlight to fully drain its battery. By default it's 120 seconds (like in Half-Life).
* **sk_flashlight_charge_time** - time in seconds for flashlight to fully recharge. By default it's 20 (like in Half-Life).
* **sk_plr_armor_strength** - the armor strength relative to health strength. By default in Half-Life 1 armor point has the same strength as 2 health points.

### Soundscripts

* **Player.FlashlightOn** - turn on the flashlight.
* **Player.FlashlightOff** - turn off the flashlight.
* **Player.NVGOn** - turn on the NVG.
* **Player.NVGOff** - turn off the NVG.
* **Player.FallBodySplat** - played when player dies from fall damage.
* **Player.TrainUse** - start using a train.
* **Player.VehicleUse** - start using a vehicle.
* **Player.UnderwaterExhale** - player is underwater and losing air. In Half-Life it's the sound of exhaling bubbles.
* **Player.Undrown** - player gets out of the water while having no air left in lungs.
* **Player.EmergeInhale** - player get out of the water after spending some amount of air.
* **SprayCan.Paint** - player paints a spray on the wall. It's actually played on the entity created temporarily, not on the player.
