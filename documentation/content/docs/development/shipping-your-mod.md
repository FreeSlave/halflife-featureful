---
weight: 10
title: "Shipping your mod"
---

# Shipping your mod

So, you're ready to publish the demo or the full version of you mod based in Featureful SDK. This article will help you to figure out what things you should consider checking before shipping.

### Configuring liblist.gam

If you mod is singleplayer only, it's better to add `type "singleplayer_only"` line to your liblist.gam. In this case the *Multiplayer* tab in options is hidden and *Advanced* settings takes its place.

See also: [liblist.gam on TWHL](https://twhl.info/wiki/page/liblist.gam)

### Advanced settings

Make sure you ship the appropriate **user.scr** file. This is where the Advanced settings are described.

You can take the user.scr from a sample mod. If you disabled some cvars (via **featureful_client.cfg**) you should remove the corresponding entries in the **user.scr**.

For example, you disabled the `cl_flashlight_radius` cvar by setting `flashlight.radius.configurable` feature to false. To avoid user confusion this option should be deleted from Advanced settings as well.

See also: [settings.scr and user.scr on TWHL](https://twhl.info/wiki/page/settings.scr_and_user.scr)

### Exclude redundant files

Make sure your **config.cfg** doesn't get packaged into the mod archive. The config.cfg contains user settings such as volume, keyboard, sensitivity, etc.

Players have their own config.cfg file in the valve/ folder - this configuration is used automatically if the mod folder lacks its own config.cfg. It's better to prefer user settings unless there's a good reason to include custom config. One of such reasons is providing some keybinding by default (e.g. to access weapon slot 6). Sadly, there's no easy and stable way to provide only missing bindings, so you have to ship the whole config (e.g. Counter Strike and Opposing Force have additional keybindings and they provide custom config.cfg by default).

There're several auto-generated files that you might consider to exclude just to keep the archive cleaner, but if they're kept, they're not harmful:

* logos/remapped.bmp
* models/player/remapped.bmp
* buffer.dat
* custom.hpk
* tempdecal.wad
* voice_ban.dt
