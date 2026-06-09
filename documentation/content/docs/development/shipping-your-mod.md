---
weight: 10
title: "Shipping your mod"
---

# Shipping your mod

So, you're ready to publish the demo or the full version of your mod based on Featureful SDK. This article will help you to figure out what things you should consider checking before shipping.

### Configuring liblist.gam

If your mod is singleplayer only, it's better to add `type "singleplayer_only"` line to your liblist.gam. In this case the *Multiplayer* tab in options becomes hidden and *Advanced* settings tab takes its place.

See also: [liblist.gam on TWHL](https://twhl.info/wiki/page/liblist.gam)

### Advanced settings

Make sure you ship the appropriate **user.scr** file. This is where the Advanced settings are described.

You can take the user.scr from a sample mod. If you disabled some cvars (via **featureful_client.cfg**) you should remove the corresponding entries in the **user.scr**.

For example, you disabled the `cl_flashlight_radius` cvar by setting `flashlight.radius.configurable` feature to `false`. To avoid user confusion this option should be deleted from Advanced settings as well.

Make sure the default setting in **user.scr** matches the default value that is set by features in **featureful_client.cfg**. For example, if `weapon_wallpuff.enabled_by_default` is set to `true`, set the `cl_weapon_wallpuff` value in **user.scr** to 1.

{{% hint warning %}}
The entries in **user.scr** are getting updated with the user settings. So if you set anything to non-default values during development it will affect this file contents. We recommend to keep the separate "clean" instance of **user.scr** file with default settings somewhere else or under a different name. Before shipping the mod copy this "clean" version to the **<modfolder>/user.scr**.
{{% /hint %}}

See also: [settings.scr and user.scr on TWHL](https://twhl.info/wiki/page/settings.scr_and_user.scr)

### userconfig.cfg

**DO NOT** ship the custom **userconfig.cfg** with your mod unless you're absolutely have to. This file is supposed to be edited by the user, not shipped with a mod.

Use **features/featureful_client.cfg** to set the default values for some custom client cvars. Use **features/featureful_exec.cfg** to set custom default values for server cvars. Use [player templates]({{< ref "player-templates" >}}) to change the default player traits like maximum speed.

### Exclude redundant files

Make sure your **config.cfg** doesn't get packaged into the mod archive. The config.cfg contains user settings such as volume, keyboard, sensitivity, etc.

Players have their own config.cfg file in the **valve/** folder - this configuration is used automatically if the mod folder lacks its own config.cfg. If you want to provide the default keybindings for commands that didn't exist in base Half-Life (and thus don't appear in uers's valve/config.cfg), refer to [Default keys]({{< ref "default-keys" >}}).

There're also several auto-generated files that you might consider to exclude just to keep the archive cleaner, but if they're kept, they're not harmful:

* logos/remapped.bmp
* models/player/remapped.bmp
* buffer.dat
* custom.hpk
* tempdecal.wad
* voice_ban.dt
