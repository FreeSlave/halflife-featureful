---
weight: 3
title: "Developing the mod"
bookToC: false
---

# {{% param "title" %}}

Apart from [Feature configuration]({{< ref configuration >}}) the development process for the mod based on Featureful SDK is basically the same as for original Half-Life and other mods.

You can use any GoldSource tools you would use otherwise, with the exception of level editor: currently the FGD distributed with the sample mod is made specificially for JACK and may not work with other level editors. If there's a demand for Trenchbroom compatible FGD we will start distributing the one as well.

Most mapping or resource-handling tutorials you can find for Half-Life are probably suitable for Featureful SDK as well. So please don't bother the author with generic Half-Life questions that can be answered in tutorials or in community chats. We expect you have some (at least basic) experience with Half-Life modding and mapping prior to switching or trying out the Featureful SDK, so this documentation doesn't go into details of configuring the level editor, compiling the maps, etc. - there're plenty of other resources dedicated to these matters.

## Recommendations

During development it's recommended to launch your mod in the developer mode to see all diagnostics messages shown on the game or level start up and during the mod testing. This might help you to trace the issues with the mod configuration or with the levels.

It's also recommended to launch the mod with cheats enabled, so all the developer commands work.

To launch the game in the developer mode with cheats enabled add `-dev +developer 1 +sv_cheats 1` to your mod launch options. Set `+developer 2` to see more diagnostic messages (mostly related to monsters AI). Levels 3 and 4 will give even more diagnostic messages.

## Developer console commands

Featureful SDK introduces some new console commands designed to aid the developer with debugging. Some require `sv_cheats` to be enabled.

* `buddha` - Alternative to `god`. Player can take damage, but their health never gets lower than 1hp.
* `fire` - trigger entity(-ies) by name (the first argument) or the entity in front of the player (if the argument is not provided). The special name `!cross` means the entity in front of the player. The second (optional) argument is either `on`, `off` or `set` allows to change the input trigger type. If the second argument is `set`, the third argument is expected which is a number to set (can be used to set the value for `game_counter`, etc.). Note: the player is passed as an *activator*.
* [global variables related commands]({{< ref "global-variables/#console-commands" >}})
* [Player inventory related commands]({{< ref "player-inventory/#developer-commands" >}})
* `report_ai_state` - report AI state of all monsters with the provided classname.
* `entities_count` - print the current number of entities on the map at console. Use it to see how close the map is to the entity limit.
* `calc_ratio` - report a Calc Ratio of specified entity. See [Math]({{< ref math >}}) and [Locus system]({{< ref locus-system >}}).
* `calc_state` - report state of specified entity (On or Off). See [Master entities]({{< ref master-entities >}}).
* `dump_ammo_types` - report registered ammo types.
* `dump_ammo_types_client` - report ammo types as known to the client (should be the same as `dump_ammo_types`).
* `dump_materials` - report registered texture materials. See [Materials]({{< ref materials >}}).
* `dump_precached_models` and `dump_precached_sounds` - show the count and the list of precached models (including sprites and brush models) or sounds. Useful to keep track of the number of precached resources and to see what's exactly is being precached.
* `dump_soundscripts` - report soundscripts. See [soundscript debugging]({{< ref "soundscripts/#debugging" >}}).
* `dump_visuals` - report visuals. See [visuals debugging]({{< ref "visuals/#debugging" >}}).
* `test_particles` - produce some particles. This is for ParticleMan testing.
