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

You should also configure the **Player.NVGOn** and **Player.NVGOff** [soundscripts]({{< ref soundscripts >}}) for nightvision sounds. See [Player soundscripts]({{< ref "player/#soundscripts" >}}).

In **features/featureful_client.cfg** you can configure the NVG parameters (like color, brightness) and whether the NVG style can be chosen by player with the help of the `cl_nvgstyle` cvar. Opposing Force and Counter Strike NVG parameters are configured separately.

### Allowing both flashlight and NVG

You can allow player to hold both flashlight and NVG. To do so set `suit_light_allow_both` feature in **features/featureful_server.cfg** to `true`.
The player can obtain the flashlight by picking up `item_flashlight` and NVG by picking up [item_nvgs]({{< ref item_nvgs >}}). The default light item coming with a suit depends on the `suit_light` feature.

If player has both items, then the flashlight command (`impulse 100`) toggles the flashlight. To use the NVG in this case player should bind `nightvision` command. See [User commands]({{< ref user-commands >}}).

Note that flashlight and NVG share the same battery and currently there's no option to configure this behavior.

### NVG icon sprites

Your mod might feature the flashlight in one part of the campaign and the NVG in another part. In this case you want different sprites to show up depending on the current light item.

`nvg_empty_sprite` and `nvg_full_sprite` features in **features/featureful_client.cfg** allow to configure which sprite names from hud.txt should be used when player has NVG.

### NVG fade time

The smooth transition to NVG state can be enabled by `nvg_fade_time` feature in **features/featureful_client.cfg** and by `cl_nvgfadetime` cvar value by user. This is the amount of seconds to switch between no-NVG state to full NVG. 0 means instant switch (default). Maximum value is 2.

This option may be used for aesthetic reasons or to help preventing potential epileptic seizures.
