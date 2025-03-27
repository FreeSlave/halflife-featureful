---
weight: 3
title: "Client CVars"
bookToC: false
---

# {{% param "title" %}}

Featureful SDK introduces new client cvars that can be set by player to adjust their game experience.

The cvars also can be included as options in the [Advanced settings menu]({{< ref "shipping-your-mod/#advanced-settings" >}}) to make accessing them even more convenient for the player.

## Cvar list

* `cl_viewbob` - disable (0) or enable (1) weapon bobbing.
* `cl_rollangle` - angle of view rolling when player goes sideways. Set to 0 to disable viewroll.
* `cl_weapon_sparks` - whether to create sparks from the bullet impact on the walls.
* `cl_weapon_wallpuff` - whether to create a puff from the bullet impact on the walls.
* `cl_muzzlelight` - whether some player weapons produce a dynamic light upon firing.
* `cl_flashlight_custom` - whether to use a custom flashlight code.
* `cl_flashlight_radius` - radius of light spot for a custom flashlight.
* `cl_flashlight_fade_distance` - the distance the falloff of custom flashlight begins.
* `cl_nvgstyle` - **0** means Opposing Force nightvision style. **1** means Counter-Strike nightvision style.
* `cl_nvgradius_cs` - radius of dynamic light of Counter-Strike like nightvision.
* `cl_nvgradius_of` - radius of dynamic light of Opposing Force like nightvision.
* `cl_subtitles` - whether to show captions. For mods that use `sound/captions.txt` system.
* `hud_min_alpha` - the minimum alpha values for HUD parameters like health, armor and ammo.
* `hud_scale` - see [HUD scaling]({{< ref "HUD/#hud-scaling" >}}).
* `_satctrl` - the Satchel Charge control scheme. **0** is classic (secondary attack throws more satchels, the detonation is always on the primary attack), **1** is anniversary (throw by primary attack, detonate by secondary attack).
* `_grenphys` - the Hand Grenade physics. **0** is classic, **1** is anniversary (higher velocity).
* `cl_motd_vgui` - whether the VGUI interface is preferred for MOTD (Message Of The Day) over the non-VGUI one (used in multiplayer). The client library must be built with VGUI support.
* `cl_scoreboard_vgui` - whether the VGUI interface is preferred for Scoreboard in multiplayer. The client library must be built with VGUI support.
