---
weight: 3
title: "CVars"
---

## Client CVars

Featureful SDK introduces new client cvars that can be set by player to adjust their game experience.

The cvars also can be included as options in the [Advanced settings menu]({{< ref "shipping-your-mod/#advanced-settings" >}}) to make accessing them even more convenient for the player.

### View

* `cl_viewbob` - disable (0) or enable (1) weapon bobbing. The latter is the same as in HL Anniversary version.
* `cl_rollangle` - angle of view rolling when player goes sideways. Set to 0 to disable viewroll.
* `cl_viewmodel_lag` - disable (0) or enable (1) the view model lag (also known as weapon lag). When enabled, it makes the weapon 'lag' behind the player's camera rotation simulating the fact that the head and body turn faster than the hands. You could have noticed this effect in Half-Life 2.

### HUD

* [HUD]({{< ref "HUD" >}}) related cvars.
* `cl_subtitles` - whether to show captions. For mods that use [sound/captions.txt]({{< ref subtitles >}}) system.

### Weapon effects

* `cl_weapon_sparks` - whether to create sparks from the bullet impact on the walls. Similar to Counter-Strike.
* `cl_weapon_wallpuff` - whether to create a puff from the bullet impact on the walls. Similar to Counter-Strike.
* `cl_muzzlelight` - whether some player weapons produce a dynamic light upon firing.

### Blood effects

* `cl_bloodsplatter_style` - select a blood splatter style. Possible values:
    - `0` - default sprite-based splatter and spray.
    - `1` - legacy particle-based splatter used in Day One.
    - `2` - particle cloud (similar to blood effect from Quake).
    - `3` - sprite particle cloud with fade effect (similar to blood effect from Quake 2). This expects the **sprites/dot_index.spr** sprite of index-alpha texture format to be present in the game files (distributed with the sample mod). If it's missing the **sprites/dot.spr** will be used in the additive mode.
* `cl_bloodstream_threshold` - the minimum amount of damage per hit that can produce the blood streams (the effect used in Half-Life Alpha and some mods). `0` means no bloodstreams. A good start is `40` (the default python damage in Half-Life).

### Flashlight

* `cl_flashlight_custom` - whether to use a custom flashlight code.
* `cl_flashlight_radius` - radius of light spot for a custom flashlight.
* `cl_flashlight_fade_distance` - the distance at which the falloff of custom flashlight begins.

### Nightvision

* `cl_nvgstyle` - configurable nightvision style. This is available only if `nvgstyle.configurable` feature is enabled. See [Nightvision]({{< ref nightvision >}}).
    - 0 - Opposing Force nightvision style.
    - 1 - Counter-Strike nightvision style.
* `cl_nvgradius_cs` - radius of dynamic light of Counter-Strike like nightvision.
* `cl_nvgradius_of` - radius of dynamic light of Opposing Force like nightvision.
* `cl_nvgfadetime` - fade time in seconds, for the smooth transition.

### Debugging

* `cl_pmanstats` - show ParticleMan statistics (particles drawn via TriAPI).
* `cl_showpos` - whether to show current position, angles and velocity info, similar to Source games.
    - 0 - don't show.
    - 1 - show **eye position** and angles. E.g. if [trigger_camera]({{< ref trigger_camera >}}) is active the position and angles of the camera will be displayed.
    - 2 - show **player's origin** and angles.
    - If cvar exists in the engine (e.g. on Xash3D FWGS), the engine implementation is used.
* `hud_soundlevelmeter` - show the radius of the sound produced by the player actions. Monsters may react to player sounds. The server must run in developer mode to send the sound level updates.

### Weapon behavior (HL Anniversary compatibility)

* `_satctrl` - the Satchel Charge control scheme. **0** is classic (secondary attack throws more satchels, the detonation is always on the primary attack), **1** is anniversary (throw by primary attack, detonate by secondary attack).
* `_grenphys` - the Hand Grenade physics. **0** is classic, **1** is anniversary (higher velocity).

### Speedrunning

* `cl_autojump` - jump automatically as soon as player touches the ground while +jump is held.
* `hud_speedometer` - show [speedometer]({{< ref "HUD/#speedometer" >}}).
* See also: [server speedrun-related cvars](#speedrunning-and-movement).

### Multiplayer

* `cl_motd_vgui` - whether the VGUI interface is preferred for MOTD (Message Of The Day) over the non-VGUI one (used in multiplayer). The client library must be built with VGUI support.
* `cl_scoreboard_vgui` - whether the VGUI interface is preferred for Scoreboard in multiplayer. The client library must be built with VGUI support.

## Server CVars

Featureful SDK introduces new server cvars that affect some aspects of gameplay. Server cvars values must be set in **features/featureful_exec.cfg** to apply them on game launch.

{{% hint info %}}
Technically user can change server cvars anytime via console and this may affect the intended gameplay. But the choice to do so depends on the user's conscience.

Changes to the server cvars values are not saved between game launches, unlike for client ones.
{{% /hint %}}

### Item interaction

* `pushablemode` controls how [func_pushables]({{< ref func_pushable >}}) are pushed by a player.
    - -1 is for default Half-Life behavior before the anniversary update (the excessive acceleration is present).
    - 0 is for default Half-Life behavior in anniversary update.
    - 1 is for g-cont fix (used in some Xash3D mods). Allow player to push pushables by going back, left or right. Prevent excessive acceleration by ignoring the effect of `+use` when the regular forward push is applied.
* `pickup_policy` controls how player can pick up items.
    - 0 is by touch only (like in Half-Life).
    - 1 is by pressing +use only.
    - 2 is for both by touch and pressing +use.
* `items_physics_fix` - various attempts to fix some bugs with items that drop from breakables.
    - 0 - default Half-Life behavior (items fall through in a tight space).
    - 1 - items dropped from [func_breakable]({{< ref func_breakable >}}) boxes placed in a tight space don't fall through. Items still fall through if box is in the air.
    - 2 - same effect as 1 plus items placed in a tight space don't fall through at the level start. But prespawned items start to fall through clip brushes.
    - 3 - same effect as 1 plus fix for boxes in the air. Items placed in a tight space don't fall through, but they fall through clip brushes no matter whether they're prespawned or come from func_breakable.

### NPC

* `npc_tridepth`. Triangulation depth to use when building paths for monsters. Possible values: 1, 2, 3. Note that higher values may affect the performance.
* `npc_tridepth_all`. If set to 0, only player's followers and monsters in scripts will use the value specified by `npc_tridepth` as the triangulation depth, and other monsters will use the value 1. If set to 1, all monsters will use the value specified by `npc_tridepth`. Note that this may affect the performance.
* `npc_forget_enemy_time`. If monster didn't observe its enemy for this amount of time (in seconds), the monster forgets about their enemy. Setting 0 means that monsters never forget their enemies (the standard behavior in Half-Life).
* `npc_active_after_combat`. If set to 1, monsters will wander around a bit after the combat ends.
* `npc_get_to_enemy_nearest`. If set to 1, melee-oriented monsters will try to get closer to the enemy even if they can't build the proper route to the enemy. This is especially helpful to fix the exploit that allows player to "hide" from enemy path finding by standing on small elevations (e.g. tables) or on lower ground. It's better to use with `npc_trace_hull_attack_retry` enabled.
* `npc_lateral_retreat`. If set to 1, monsters can use lateral retreat when running away from enemy (without relying on the node graph). Default value is 1.
* `npc_trace_hull_attack_retry`. If set to 1, monsters performing the hull trace for melee attacks will retry the failed trace starting from a different point a bit higher and a bit lower than before. This helps to deal damage to the enemy standing on small elevations compared to the attacker.
* `npc_vanilla_kick_behavior`. In original Half-Life some monsters (e.g. human grunts) could apply the velocity to any entity when they kick it (if the entity's physics allow it), either accidentally or via scripted sequence. Generally this is not desirable behavior so it got reworked in Featureful, but it's still configurable via cvar.
    - -1 - don't allow kicking anything besides monsters, players and pushables.
    - 0 - allow kicking func_door_rotating in scripted sequences.
    - 1 - allow kicking anything (vanilla behavior).
    - 2 - allow kicking anything but only in scripted sequences.
* `sp_allowmonsterinfo` - show monsters' display names and current health in singleplayer when player looks at the monster. Default value is 0.
    - 0 - don't show monster info.
    - 1 - show info for all monsters.
    - 2 - show info only for ally monsters.
    - 3 - show info only for ally monsters when player is currently equipped with portable medkit.
* `mp_allowmonsterinfo` - same as `sp_allowmonsterinfo`, but for multiplayer. Default value is 0.

### Weapons

* `mp_allowdropammo` - whether the ammo drop via the `dropammo` and `dropammo2` commands are allowed in multiplayer. The default value is 1.
* `sp_allowdropammo` - whether the ammo drop via the `dropammo` and `dropammo2` commands are allowed in singleplayer. The default value is 0.

### Speedrunning and movement

* `sv_bunnyhop` - whether the bunny hop is enabled.
* `grenade_jump` - the factor for the vertical velocity added when the player is taking damage. Use the value `0` for completely removing the grenade/rocket jumps.
