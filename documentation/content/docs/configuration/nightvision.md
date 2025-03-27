---
title: "Nightvision"
---

# {{% param "title" %}}

### Two styles of NVG

Featureful SDK implements two styles of Night vision goggles:

* ![](/images/opfor.png) Classic Opposing Force night vision (requires **sprites/of_nv_b.spr** resource in the mod directory).
* ![](/images/cstrike.png) Counter Strike like night vision.

The Counter Strike style is usually better accepted by players, so it's the default option in Featureful SDK.

### Enabling and configuring NVG

In order to let player have NVG instead of the flashlight set `suit_light` feature in **features/featureful_server.cfg** to `nvg`.

You should also configure the **Player.NVGOn** and **Player.NVGOff** [soundscripts]({{< ref soundscripts >}}) for nightvision sounds.

**Note:** the old way of configuring the NVG sound was through the `nvg_sound_on` and `nvg_sound_off` features. These are deprecated and eventually will be removed.

In **features/featureful_client.cfg** you can configure the NVG parameters (like color, brightness) and whether the NVG style can be chosen by player with the help of the `cl_nvgstyle` cvar. Opposing Force and Counter Strike NVG parameters are configured separately.

### Allowing both flashlight and NVG

You can allow player to hold both flashlight and NVG. To do so set `suit_light_allow_both` feature in **features/featureful_server.cfg** to `true`.
The player can obtain the flashlight by picking up `item_flashlight` and NVG by picking up [item_nvgs]({{< ref item_nvgs >}}). The default light item coming with a suit depends on the `suit_light` feature.

If player has both items, then the flashlight command (`impulse 100`) toggles the flashlight. To use the NVG in this case player should bind `nightvision` command. To let player configure this keybind via menu, the mod developer needs to add a new line somewhere in **gfx/shell/kb_act.lst**:

```"nightvision"	"Nightvision"```

Note that flashlight and NVG share the same battery and currently there's no option to configure this behavior.

### NVG icon sprites

Your mod might feature the flashlight in one part of the campaign and the NVG in another part. In this case you want different sprites to show up depending on the current light item.

`nvg_empty_sprite` and `nvg_full_sprite` features in **features/featureful_client.cfg** allow to configure which sprite names from hud.txt should be used when player has NVG.
