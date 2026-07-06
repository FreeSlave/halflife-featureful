---
title: "Player"
bookToC: false
---

# {{% param "title" %}}

Player is represented by `player` entity in-game.

### Changes

* Player can `+use` and toggle flashlight during the weapon reload (in original Half-Life player couldn't do so).

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

* **Player.Death** - death sound (unless player is gibbed). This is empty by default.
* **Player.DeathUnderwater** - death sound to use if player died underwater (unless player is gibbed). This is empty by default.
* **Player.FlashlightOn** - turn on the flashlight.
* **Player.FlashlightOff** - turn off the flashlight.
* **Player.NVGOn** - turn on the NVG. This is empty by default.
* **Player.NVGOff** - turn off the NVG. This is empty by default.
* **Player.FallBodySplat** - played when player dies from fall damage.
* **Player.FallPain** - played when player lands from the height enough to get a fall damage.
* **Player.Jump** - played when player jumps (like in Team Fortress Classic or Deathmatch Classic). This is empty by default. If it's empty the material step sound is played on jump (Half-Life behavior).
* **Player.TrainUse** - start using a train.
* **Player.VehicleUse** - start using a vehicle.
* **Player.Wade** - player enters or leaves the water. Also played sometimes while player is in water.
* **Player.UnderwaterExhale** - player is underwater and losing air. In Half-Life it's the sound of exhaling bubbles.
* **Player.Undrown** - player gets out of the water while having no air left in lungs.
* **Player.EmergeInhale** - player get out of the water after spending some amount of air.
* **Player.Geiger** - geiger counter sounds played when player is close to the radioactive [trigger_hurt]({{< ref trigger_hurt >}}).
* **Player.LongJump** - long jump sound. This is empty by default.
* **Player.AntidoteUse** - played when [item_antidote]({{< ref item_antidote >}}) is used.
* **Player.AntiradUse** - played when [item_radiation]({{< ref item_radiation >}}) is used.
* **SprayCan.Paint** - player paints a spray on the wall. It's actually played on the entity created temporarily, not on the player.

Example:

```json
{
    "Player.DeathUnderwater": {
        "waves": ["player/h2odeath.wav"]
    },
    "Player.NVGOn": {
        "waves": ["items/nvg_on.wav"]
    },
    "Player.NVGOff": {
        "waves": ["items/nvg_off.wav"]
    },
    "Player.LongJump": {
        "waves": ["ctf/pow_big_jump.wav"]
    }
}
```

### Maximum speed

The player's base maximum speed depends on the value of the `sv_maxspeed` cvar value. This is 320 by default. Multiplayer configs (listenserver.cfg and server.cfg) usually set it to 270.

There're several factors that may contribute to the change in the player's maximum speed:

* The [player template]({{< ref player-templates >}}) can change the base maximum speed.
* The maximum speed set by a [currently equipped weapon]({{< ref "weapon-templates#player_maxspeed" >}}) or when [weapon is fired]({{< ref "weapon-templates#fire-player_maxspeed" >}}).
* The effect of the [player_speed]({{< ref player_speed >}}) entity.

{{% hint warning %}}
The actual maximum speed can't exceed the one set by the `sv_maxspeed` cvar. None of the above factors can change that even if the resulting speed is higher. You need to set a custom value `sv_maxspeed` in **features/featureful_exec.cfg** to allow higher speeds, e.g. `sv_maxspeed 400`.
{{% /hint %}}
