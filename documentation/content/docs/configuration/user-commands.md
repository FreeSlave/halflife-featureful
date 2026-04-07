---
title: "User commands"
bookToC: false
---

# {{% param "title" %}}

**Featureful SDK** introduces some new user commands useful in normal gameplay (for development-related commands see [Developer console commands]({{< ref "developing-the-mod/#developer-console-commands" >}})).

* `nightvision` - toggle the [NVG]({{< ref nightvision >}}). Useful if player has both flashlight and NVG.
* `recruit_followers` and `disband_followers` - see [Fast recruiting]({{< ref "followers/#fast-recruiting" >}}).
* `dropammo` - drop ammo entity for the current weapon. This might be useful in multiplayer, e.g. in co-op if you want to share your ammo with ally player. Whether this command is allowed depends on the `mp_allowdropammo` and `sp_allowdropammo` [cvars]({{< ref "cvars/#weapons" >}}). The drop itself depends on the [dropammo]({{< ref "weapon-templates/#dropammo" >}}) of the weapon template.
* `dropammo2` - same as `dropammo`, but for secondary ammo (e.g. AR grenades from MP5).

These commands can be bound to selected keys via the `bind` console command. To add these commands to the keyboard options inteface, insert the following lines somewhere in your **gfx/shell/kb_act.lst** file:

```
"nightvision" "#Nightvision"
"recruit_followers" "#Recruit_Followers"
"disband_followers" "#Disband_Followers"
"dropammo" "#Drop_Ammo"
"dropammo2" "#Drop_Ammo_Secondary"
```

The `#` denotes that the actual text shown in the options menu will be taken from the localization file. In your **resource/valve_english.txt** add lines:

```
"Nightvision" "Nightvision"
"Recruit_Followers" "Recruit Followers"
"Disband_Followers" "Disband Followers"
"Drop_Ammo": "Drop Ammo"
"Drop_Ammo_Secondary": "Drop Secondary Ammo"
```

Read also:

* [TWHL article on user commands](https://twhl.info/wiki/page/Command)
* [TWHL article on setting game string](https://twhl.info/wiki/page/Tutorial%3A_Setting_up_a_Mod%3A_Part_3_-_Game_Strings_%28Steam%29)
