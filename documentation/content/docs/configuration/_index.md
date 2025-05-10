---
weight: 2
bookFlatSection: true
title: "Feature configuration"
---

# {{% param "title" %}}

The main purpose of Featureful SDK is being configurable without programming. This can be achieved via adding and editing several files in the mod directory.

The list of featureful configuration files:

* [features/featureful_client.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/featureful_client.cfg) allows configuring client-side features, like HUD color, allowed client-side cvars and their default values.
* [features/featureful_server.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/featureful_server.cfg) allows configuring server-side features, related to different aspects of the game, like monsters and items.
* [features/featureful_monsters.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/featureful_monsters.cfg) is the list of enabled custom monsters (i.e. non-standard for the original Half-Life).
* [features/featureful_weapons.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/featureful_weapons.cfg) is the list of enabled custom weapons (i.e. non-standard for the original Half-Life).
* [features/featureful_exec.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/featureful_exec.cfg) is the config file that gets executed on game dll initialization.
* [features/ammo_amounts.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/ammo_amounts.cfg) - configuring default ammo amounts per weapon and ammo entities.
* [features/hud_weapon_layout.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/hud_weapon_layout.cfg) - configuring layout of weapons in HUD (slots and positions).
* [features/maxammo.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/maxammo.cfg) - configuring max ammo amount for each ammo type.
* **features/followers.json** - configuring player [followers]({{< ref followers >}}) properties.
* **features/materials.json** - configuring texture [materials]({{< ref materials >}}) properties.
* **sound/captions.txt** and **sound/captions_profiles.txt** - parts of [subtitles system]({{< ref subtitles >}}).
* **sound/soundscripts.json** - configuring [soundscripts]({{< ref soundscripts >}}).
* **templates/visuals.json** - configuring [visuals]({{< ref visuals >}}).
* **templates/entities.json** - configuring [entity templates]({{< ref entity-templates >}}).
* **templates/warpball.json** - configuring [warpball templates]({{< ref warpball-templates >}}).
* **templates/objecthint.json** - configuring [object hints]({{< ref object-hints >}}).
* **templates/inventory.json** - configuring [player inventory]({{< ref player-inventory >}}) related properties.
* **save_titles.txt** - configuring [save titles]({{< ref save-titles >}}).

{{% hint info %}}
All configuration files are optional. If something is missing the defaults are provided.
{{% /hint %}}

{{% hint info %}}
If you modify *featureful_* files you must restart the game in order for these changes to take effect.
{{% /hint %}}

## File format

The format for **featureful_client.cfg** and **featureful_server.cfg** is basically the same.

The comments start with `//`. Features are defined as key-value pairs separated by spacebars. There're boolean, integer, color and floating-point parameters.

Boolean parameters can take values `true`, `yes`, `1` and `false`, `no`, `0`.

Color parameters can be either the hexadecimal number (e.g. `0xFFA000`) or RGB values separated by spacebars (e.g. `255 160 0`)

The format for **featureful_monsters.cfg** and **featureful_weapons.cfg** is a bit different. The comments start with `//` and non-empty lines specify which weapons and monsters should be enabled in the mod.

**featureful_exec.cfg**, unlike others, is a real configuration file, i.e. it will be executed as other .cfg files by the engine server-side. Thus it has a different format compared to other featureful file. Usually you want to put some server cvars here. But remember that that these cvars can be overriden by a user's **game.cfg** or changed in console during the game. Our current philosophy regarding that is simple: if the user changes something in the console, he knows what he's doing. Still in future we might provide an option to remove undesired cvars and set the constant values instead.

## JSON files

Some files are in [JSON]({{< ref JSON >}}) format used for more complex structured data.

## Distributing

The files should be put in the **features/** subdirectory of your mod directory.

If a feature config file doesn't exist the default parameters will be used in game.
If some feature in the config file is omitted or commented out, the default behavior will be used.

You must disable weapons unused in your mod. Otherwise if your mod is lacking the resources for these weapons there will be a precache error.

You also should disable unused monsters. Even if you don't use some non-standard monster on the mod maps, there can be some errors in console about missing cvars if skill cvars related to this monster are not present in skill.cfg.

## Recommendations

Some features are set by default to conform to the vanilla Half-Life behavior, so there's less chance of breaking the existing maps. However, if your mod utilizes its own maps (I hope so!), it's better to change some options to avoid undesired behaviors.

Here's the list of recommendations for **features/featureful_server.cfg**:

* Set `corpse_player_collision_fix` to `true` to fix the consequences of the engine bug that makes brush entities to receive player as a blocker while he stands on the corpse nearby.
* Set `monsters_spawned_named_wait_trigger` to `false` to fix problems with unresponsiveness of the monsters who come from monstermaker and have a targetname.
* Set `doors_blocked_recheck` to `true` so doors continue moving in case they crushed the monster (instead of returning to "open" position).
* Set `door_rotating_starts_open_fix` to `true` to fix the bug with func_door_rotating becoming unresponsive after the first use if it has `Starts Open` spawnflag.
* Set `bigmomma_wait_fix` to `true` so wait values of `info_bigmomma` are actually used.
* Set `bigmomma_lastnode_fix` fix to `true` so Big Momma won't reuse the last node more than once (the original Half-Life bug).
* Set `items_instant_drop` to `false` to make healthkits and batteries fall from the center of the `func_breakable` instead of appearing on the floor right away.

Some Half-Life behavior aspects were changed from vanilla to better alternatives by default, as these changes weren't observed to cause troubles. Still, if you're interested, here's the list of server features that are set to be non-vanilla by default (consult [featureful_server.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/featureful_server.cfg) for descriptions):

* `satchels_pickable`
* `monsters_delegate_squad_leadership`
* `monsters_eat_for_health`
* `sentry_retract`
* Features related to new vortigaunts and bullsquid abilities.

There're also some cvars that you might be interested in changing in [featureful_exec.cfg](https://github.com/FreeSlave/halflife-featureful/blob/featureful/features/featureful_exec.cfg):

* `npc_forget_enemy_time`
* `npc_tridepth`
* `npc_active_after_combat`

Be mindful when deciding on what custom weapons and monsters should be enabled in your mod. Featureful SDK provides many new weapons and monsters, but it's not the reason to throw all of them into your mod! First think of what actually belongs to the campaign you're making. It's better to utilize less to its full extent instead of turning your mod into a zoo.

