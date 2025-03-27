---
title: "Followers"
---

# {{% param "title" %}}

Some ally monsters can follow the player when they're called by a player. In original Half-Life only scientists and security guards (barneys) could do that. Opposing Force expanded the list with cleansuit scientist, Otis and ally soldiers. Featureful SDK adds more monsters to the list, like vortigaunts, alien grunts, HL human grunts, blackops assassins (if they're set to be player ally).

## Fast recruiting

When the mod utilizes a lot of allies on one map it becomes difficult to control them for a player. Featureful SDK implements some user commands to make it easier.

* `recruit_followers` - make nearby allies follow the player.
* `disband_followers` - call off all following allies.

By default these commands don't affect any monsters. You need to configure the list of affected monsters in **features/followers.json**. The sample mod provides the example:

```json
{
    "fast_recruit_monsters": [
        "monster_human_grunt_ally",
        "monster_human_torch_ally",
        "monster_human_medic_ally"
    ],
    "fast_recruit_range": 500
}
```

The **fast_recruit_monsters** is the list of monster names (should be full entity names) affected by the commands. **fast_recruit_range** is a radius (in units) around the player to search allies in.

{{% hint warning %}}
As fast recruiting allows to call ally monsters from distance, it may potentially affect the map flow intended by the mapper if they don't keep this fact in mind.
{{% /hint %}}

`recruit_followers` and `disband_followers` commands can be bound to selected keys via the `bind` console command. If you want to add these commands to the keyboard options, add the following lines somewhere in your **gfx/shell/kb_act.lst** file:

```
"recruit_followers" "#Recruit_Followers"
"disband_followers" "#Disband_Followers"
```

The `#` denotes that the actual text shown in the options menu will be taken from the localization file. In your **resource/valve_english.txt** add lines:

```
"Recruit_Followers" "Recruit Followers"
"Disband_Followers" "Disband Followers"
```

Read also:

* [TWHL article on user commands](https://twhl.info/wiki/page/Command)
* [TWHL article on setting game string](https://twhl.info/wiki/page/Tutorial%3A_Setting_up_a_Mod%3A_Part_3_-_Game_Strings_%28Steam%29)
