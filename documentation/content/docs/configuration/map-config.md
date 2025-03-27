---
weight: 3
title: "Map config"
---

# {{% param "title" %}}

Featureful SDK allows maps to have the config file listing the starting given to the player when the map is started via `map` console command or via the *New game* menu dialog. The equipment won't be given if player entered the map via the `trigger_changelevel`.

Create a file with a name like **mapname.cfg** where the `mapname` is a name of the map file (without **.bsp** extension).

{{% hint info %}}
The sole presence of map config file makes the game to give player a suit.
{{% /hint %}}

{{% hint info %}}
You can also configure the starting equipment on the map itself, via [game_player_settings]({{< ref game_player_settings >}}) entity. This however requires map rebuilding if you want to make changes.
{{% /hint %}}

## Format

Each piece of equipment is defined by one line in the file.

### Player parameters

* `startarmor` - specify the starting armor.
* `starthealth` - specify the starting health.
* `maxhealth` - maximum player's health.
* `maxarmor` - maximum player's armor.

Example:

```
startarmor 20
starthealth 50
```

### Weapons

Just put the weapon entity name on the line:

```
weapon_crowbar
weapon_9mmhandgun
weapon_357
weapon_9mmAR
weapon_shotgun
weapon_crossbow
weapon_rpg
weapon_gauss
weapon_egon
weapon_hornetgun
```

### Ammo

Put **ammo!** followed by the name of ammo type and the amount of ammo to give:

```
ammo!9mm 150
ammo!buckshot 16
ammo!357 20
ammo!bolts 10
ammo!rockets 3
ammo!uranium 20
ammo!ARgrenades 5
ammo!762 5
ammo!556 50
ammo!spores 8
```

### Items

* `nosuit` - don't give player a suit.
* `suitlogon` - configure the suit logon. Possible values:
    - `short`
    - `long`
    - `no` (default)
* `item_longjump` - give player a longjump module.
* `suitlight` - starting light item. Omitting this parameter will give the default light item for the suit as configured in **features/featureful_client.cfg**. Possible values:
    - `flashlight`
    - `nvg` or `nightvision`
    - `nothing`

Example:

```
suitlogon short
item_longjump
suitlight nothing
```
