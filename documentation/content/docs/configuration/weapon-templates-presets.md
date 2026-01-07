---
title: "Weapon templates (presets)"
---

# {{% param "title" %}}

The sample mod includes some premade [weapon templates]({{< ref "weapon-templates" >}}) to simulate weapons from other mods.

{{% hint info %}}
If not stated otherwise, the weapon template can be applied to any fully configurable weapon, replacing its behavior (via [from_scratch]({{< ref "weapon-templates#from_scratch" >}}) property).
{{% /hint %}}

## Counter Strike

The templates for CS weapons are stored in the **templates/weapons/cs/** directory. They include almost every CS firearm (except for dual berretas).

{{% details "Example" open %}}

```json
{
    "weapon_9mmhandgun": "cs/glock18",
    "weapon_eagle": "cs/deagle",
    "weapon_9mmAR": "cs/mp5navy",
    "weapon_shotgun": "cs/m3",
    "weapon_sniperrifle": "cs/awp",
    "weapon_pistol": "cs/usp",
    "weapon_pistol2": "cs/fiveseven",
    "weapon_smg": "cs/ump45",
    "weapon_smg2": "cs/p90",
    "weapon_rifle": "cs/m4a1",
    "weapon_rifle2": "cs/famas",
    "weapon_shotgun2": "cs/xm1014",
    "weapon_sniperrifle2": "cs/g3sg1"
}
```

{{% /details %}}

As a mod developer you need to take into account that CS handles weapons different (comparing to HL) in several aspects and this behavior is replicated in the weapon templates:

* Most CS weapons use different [spread]({{< ref "weapon-templates#spread" >}}) and [kickback]({{< ref "weapon-templates#kickback" >}}) rules.
    - The fire spread gets wider with each fire, making the player to use short burst fire to keep the shots accurate (unlike HL where the spread is randomized in a constant range independent of previous shots).
    - Kickback (recoil) is server-side, i.e. it actually affects the direction of the next shot (unlike HL where the recoil is client-side and purely visual).
    - The kickback (recoil) inscreases with each fire making the player control their pointing devices to compensate for the recoil.
    - Both spread and kickback depend on whether the player is in on the ground, moving or ducking. Usually the weapons shoot more accurate when player is ducking and less accurate when player is moving or not on the ground (e.g. in jump or on the ladder).
* Weapons have [range modifier]({{< ref "weapon-templates#range_modifier" >}}) that decreases the damage on longer distances. In HL the bullet damage is constant independent of the distance.
* Most CS weapons can shoot underwater, except for FAMAS, Galil and shotguns. While this does little impact in CS as it usually doesn't feature maps with water deep enough for players to swim, this is something to remember when you're making a mod.
* Most CS weapons eject shells to the left side (when playing in the right-handed setup which is default in modern CS). There're some exceptions like P90, AUG, FAMAS and M249. If you don't want shells to eject to the left side, you may remove [left_side]({{< ref "weapon-templates#left_side" >}}) property or set it to `false`.
* Shell ejection positions for most weapons depend on the attachment 2 on the weapon view model. The exceptions are M3, Scout and AWP.
* CS view weapon models were made as left-handed. We set [mirror_viewmodel]({{< ref "weapon-templates#mirror_viewmodel" >}}) for each CS weapon to make them appear right-handed.

{{% hint info %}}
In Counter Strike each weapon has an associated max player speed. That's why player moves slower when, for example, a sniper rifle is deployed. The distributed weapon presets don't replicate the CS weapons speed penalties, but it can be configured via [player_maxspeed]({{< ref "weapon-templates#player_maxspeed" >}}).
{{% /hint %}}

CS weapons demonstrate some other traits that are not possible (yet) to express via weapon templates:

* CS firearms can penetrate walls.

We advise not mixing HL and CS weapon fire styles in the same mod as it might be confusing to the player, so don't mix HL and CS weapons without making appropriate changes. Follow these recommendations:

* Decide whether you want a dynamic [spread]({{< ref "weapon-templates#spread" >}}) and [kickback]({{< ref "weapon-templates#kickback" >}}) from CS. Set it appropriately for all related firearms.
* Decide whether you want bullet damage depend on the range and set or remove [range modifier]({{< ref "weapon-templates#range_modifier" >}}) appropriately.
* Configure [damage]({{< ref "weapon-templates#damage" >}}) values to your liking. Since replicating the CS damage values wouldn't fit the HL singleplayer balance (the damage values in CS are balanced around players having 100 hp and low time-to-kill) as a rule of thumb we took original damage values and divided them by 2.5 (except for Desert Eagle, AWP and Scout).
* Set [allow_underwater]({{< ref "weapon-templates#allow_underwater" >}}) for each weapon in accordance to your intentions.

CS also uses some shell models that are not present in HL, so you must provide them in the mod (or just copy from **cstrike/models**) or set [shell_model]({{< ref "weapon-templates#shell_model" >}}) to **models/shell.mdl** which exists in base Half-Life. CS uses following models for shells:

* **pshell.mdl** - used by pistols and some SMGs.
* **rshell.mdl** - used by rifles and some SMGs.
* **rshell_big.mdl** - used by Scout and AWP.
* **shotgunshell.mdl** - used by shotguns, but reuses the model from base Half-Life, so you don't need to worry about it.

{{% hint info %}}
Shell models and world weapon models (*w_* prefixed) in CS on average are smaller than ones in Half-Life. You might want to scale models accordingly.
{{% /hint %}}

{{% hint info %}}
Currently you can't have all CS weapons at once in your mod - there're simply not enough weapon slots. You can select the limited set of preferred weapons and stick to them. E.g. there's not much sense in utilizing both AUG and SG 552 as they're very similar. And you probably don't need all kinds of SMGs.
{{% /hint %}}

### Pistols

{{% hint info %}}
All pistols in CS are semi-auto which means player needs to release and press the attack button again in order to make another shot. This is different from HL where weapons are automatic, i.e. the weapon continues firing as long as player has the attack button pressed. You can remove `"semiauto"` property from the weapon template to allow full auto behavior.
{{% /hint %}}

{{% details_header title="Glock-18" %}}
```json
{
    "weapon_9mmhandgun": "cs/glock18"
}
```

{{% hint info %}}
Burst fire is performed by pressing the secondary attack button, unlike CS where the secondary attack switches the mode to burst and firing still needs to be performed with a primary attack button.

This also implements burst fire as actual series of consecutive shots opposite to CS where it seems to fire three bullets at the same time.
{{% /hint %}}

{{% hint warning %}}
The end of the reload animation is not consistent with the idle animation - this is the original CS model problem.
{{% /hint %}}

Required models:

* **w_glock18.mdl**
* **v_glock18.mdl**
* **p_glock18.mdl**

Required sounds:

* **weapons/glock18-2.wav**
* **weapons/clipout1.wav**
* **weapons/clipin1.wav**
* **weapons/sliderelease1.wav**
* **weapons/slideback1.wav**
* **weapons/de_clipin.wav**
* **weapons/de_clipout.wav**
{{% /details_header %}}

{{% details_header title="USP" %}}
```json
{
    "weapon_pistol": "cs/usp"
}
```

{{% hint info %}}
In CS it doesn't make sense to use the silenced USP: it deals less damage and has worse accuracy. Having quiet gunshots is a very weak bonus.

USP produces less noise for NPCs in a silenced mode, but it's still not enough for a trade off (at least until proper stealth mechanic is implemented in Featureful SDK).

So, to justify the silencer we took a liberty of completely removing the kickback when the silencer is attached (the `"kickback"` is set to the empty array for the `"alt_fire"`). You may also play with `"spread"` and `"cycle_time"` to balance the two attack modes between each other.
{{% /hint %}}

{{% hint info %}}
USP uses `45acp` ammo type, so you should place some [ammo_45acp]({{< ref ammo-entities >}}) on your maps or change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to some other ammo type.
{{% /hint %}}

Required models:

* **w_usp.mdl**
* **v_usp.mdl**
* **p_usp.mdl**

Required sounds:

* **weapons/usp_unsil-1.wav**
* **weapons/usp1.wav**
* **weapons/usp2.wav**
* **weapons/usp_clipout.wav**
* **weapons/usp_clipin.wav**
* **weapons/usp_silencer_on.wav**
* **weapons/usp_silencer_off.wav**
* **weapons/usp_sliderelease.wav**
* **weapons/usp_slideback.wav**
{{% /details_header %}}

{{% details_header title="Desert Eagle" %}}
```json
{
    "weapon_eagle": "cs/deagle"
}
```

{{% hint info %}}
Desert Eagle in Counter Strike uses ammo type `50AE` which is not defined in Featureful SDK. This template still uses `357` ammo like in Opposing Force.
{{% /hint %}}

Required models:

* **w_deagle.mdl**
* **v_deagle.mdl**
* **p_deagle.mdl**

Required sounds:

* **weapons/deagle-1.wav**
* **weapons/deagle-2.wav**
* **weapons/de_clipin.wav**
* **weapons/de_clipout.wav**
* **weapons/de_deploy.wav**

{{% /details_header %}}

{{% details_header title="Five-Seven" %}}
```json
{
    "weapon_pistol": "cs/fiveseven"
}
```

{{% hint info %}}
Five-Seven uses `57mm` ammo type, so you should place some [ammo_57mm]({{< ref ammo-entities >}}) on your maps or change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to some other ammo type.
{{% /hint %}}

Required models:

* **w_fiveseven.mdl**
* **v_fiveseven.mdl**
* **p_fiveseven.mdl**

Required sounds:

* **weapons/fiveseven-1.wav**
* **weapons/fiveseven_clipout.wav**
* **weapons/fiveseven_clipin.wav**
* **weapons/fiveseven_sliderelease.wav**
* **weapons/fiveseven_slidepull.wav**

{{% /details_header %}}

{{% details_header title="P228" %}}
```json
{
    "weapon_pistol": "cs/p228"
}
```

{{% hint info %}}
P228 in Counter Strike uses ammo type `357SIG` which is not defined in Featureful SDK. This template uses `357` ammo type. You can change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to your liking.
{{% /hint %}}

Required models:

* **w_p228.mdl**
* **v_p228.mdl**
* **p_p228.mdl**

Required sounds:

* **weapons/p228-1.wav**
* **weapons/p228_clipout.wav**
* **weapons/p228_clipin.wav**
* **weapons/p228_sliderelease.wav**
* **weapons/p228_slidepull.wav**
{{% /details_header %}}

### Submachine guns

{{% details_header title="MAC-10" %}}
```json
{
    "weapon_smg": "cs/mac10"
}
```

{{% hint info %}}
MAC-10 uses `45acp` ammo type, so you should place some [ammo_45acp]({{< ref ammo-entities >}}) on your maps or change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to some other ammo type.
{{% /hint %}}

Required models:

* **w_mac10.mdl**
* **v_mac10.mdl**
* **p_mac10.mdl**

Required sounds:

* **weapons/mac10-1.wav**
* **weapons/mac10_clipout.wav**
* **weapons/mac10_clipin.wav**
* **weapons/mac10_boltpull.wav**
{{% /details_header %}}

{{% details_header title="TMP" %}}

```json
{
    "weapon_smg": "cs/tmp"
}
```

{{% hint info %}}
TMP has a silencer attached so it's configuired to make less noise for AI.
{{% /hint %}}

{{% hint warning %}}
The end of the reload animation is not consistent with the idle animation - this is the original CS model problem.
{{% /hint %}}

Required models:

* **w_tmp.mdl**
* **v_tmp.mdl**
* **p_tmp.mdl**

Required sounds:

* **weapons/tmp-1.wav**
* **weapons/tmp-2.wav**
{{% /details_header %}}

{{% details_header title="MP5 Navy" %}}
```json
{
    "weapon_9mmAR": "cs/mp5navy"
}
```

Required models:

* **w_mp5.mdl**
* **v_mp5.mdl**
* **p_mp5.mdl**

Required sounds:

* **weapons/mp5-1.wav**
* **weapons/mp5-2.wav**
* **weapons/mp5_clipout.wav**
* **weapons/mp5_clipin.wav**
* **weapons/mp5_slideback.wav**
{{% /details_header %}}

{{% details_header title="UMP-45" %}}
```json
{
    "weapon_smg": "cs/ump45"
}
```

{{% hint info %}}
UMP-45 uses `45acp` ammo type, so you should place some [ammo_45acp]({{< ref ammo-entities >}}) on your maps or change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to some other ammo type.
{{% /hint %}}

Required models:

* **w_ump45.mdl**
* **v_ump45.mdl**
* **p_ump45.mdl**

Required sounds:

* **weapons/ump45-1.wav**
* **weapons/ump45_clipout.wav**
* **weapons/ump45_clipin.wav**
* **weapons/ump45_boltslap.wav**
{{% /details_header %}}

{{% details_header title="P90" %}}
```json
{
    "weapon_smg": "cs/p90"
}
```

{{% hint info %}}
P90 uses `57mm` ammo type, so you should place some [ammo_57mm]({{< ref ammo-entities >}}) on your maps or change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to some other ammo type.
{{% /hint %}}

{{% hint warning %}}
The end of the reload animation is not consistent with the idle animation - this is the original CS model problem.
{{% /hint %}}

Required models:

* **w_p90.mdl**
* **v_p90.mdl**
* **p_p90.mdl**

Required sounds:

* **weapons/p90-1.wav**
* **weapons/p90_clipout.wav**
* **weapons/p90_clipin.wav**
* **weapons/p90_boltpull.wav**
* **weapons/p90_cliprelease.wav**
{{% /details_header %}}

### Assault rifles

{{% details_header title="FAMAS" %}}
```json
{
    "weapon_rifle": "cs/famas"
}
```

{{% hint info %}}
Burst fire is performed by pressing the secondary attack button, unlike CS where the secondary attack switches the mode to burst and firing still needs to be performed with a primary attack button.
{{% /hint %}}

Required models:

* **w_famas.mdl**
* **v_famas.mdl**
* **p_famas.mdl**

Required sounds:

* **weapons/famas-1.wav**
* **weapons/famas-2.wav**
* **weapons/famas_clipout.wav**
* **weapons/famas_clipin.wav**
* **weapons/famas_boltpull.wav**
* **weapons/famas_boltslap.wav**
* **weapons/famas_forearm.wav**
{{% /details_header %}}

{{% details_header title="M4A1" %}}
```json
{
    "weapon_rifle": "cs/m4a1"
}
```

Required models:

* **w_m4a1.mdl**
* **v_m4a1.mdl**
* **p_m4a1.mdl**

Required sounds:

* **weapons/m4a1_unsil-1.wav**
* **weapons/m4a1_unsil-2.wav**
* **weapons/m4a1-1.wav**
* **weapons/m4a1_clipin.wav**
* **weapons/m4a1_clipout.wav**
* **weapons/m4a1_boltpull.wav**
* **weapons/m4a1_deploy.wav**
* **weapons/m4a1_silencer_on.wav**
* **weapons/m4a1_silencer_off.wav**
{{% /details_header %}}

{{% details_header title="AUG" %}}
```json
{
    "weapon_rifle": "cs/aug"
}
```

Required models:

* **w_aug.mdl**
* **v_aug.mdl**
* **p_aug.mdl**

Required sounds:

* **weapons/aug-1.wav**
* **weapons/aug_clipout.wav**
* **weapons/aug_clipin.wav**
* **weapons/aug_boltpull.wav**
* **weapons/aug_boltslap.wav**
* **weapons/aug_forearm.wav**
{{% /details_header %}}

{{% details_header title="Galil" %}}
```json
{
    "weapon_rifle": "cs/galil"
}
```

Required models:

* **w_galil.mdl**
* **v_galil.mdl**
* **p_galil.mdl**

Required sounds:

* **weapons/galil-1.wav**
* **weapons/galil-2.wav**
* **weapons/galil_clipout.wav**
* **weapons/galil_clipin.wav**
* **weapons/galil_boltpull.wav**
{{% /details_header %}}

{{% details_header title="AK-47" %}}
```json
{
    "weapon_rifle": "cs/ak47"
}
```

{{% hint info %}}
AK-47 uses 762 ammunition (in both CS and real life). In our configuration it uses `556` ammo type - same as other rifles.
{{% /hint %}}

Required models:

* **w_ak47.mdl**
* **v_ak47.mdl**
* **p_ak47.mdl**

Required sounds:

* **weapons/ak47-1.wav**
* **weapons/ak47-2.wav**
* **weapons/ak47_clipout.wav**
* **weapons/ak47_clipin.wav**
* **weapons/ak47_boltpull.wav**
{{% /details_header %}}

{{% details_header title="SG 552" %}}
```json
{
    "weapon_rifle": "cs/sg552"
}
```

Required models:

* **w_sg552.mdl**
* **v_sg552.mdl**
* **p_sg552.mdl**

Required sounds:

* **weapons/sg552-1.wav**
* **weapons/sg552-2.wav**
* **weapons/sg552_clipout.wav**
* **weapons/sg552_clipin.wav**
* **weapons/sg552_boltpull.wav**
{{% /details_header %}}

### Shotguns

{{% details_header title="M3" %}}
```json
{
    "weapon_shotgun": "cs/m3"
}
```

Required models:

* **w_m3.mdl**
* **v_m3.mdl**
* **p_m3.mdl**

Required sounds:

* **weapons/m3-1.wav**
* **weapons/m3_insertshell.wav**
* **weapons/m3_pump.wav**
{{% /details_header %}}

{{% details_header title="XM1014" %}}
```json
{
    "weapon_shotgun2": "cs/xm1014"
}
```

Required models:

* **w_xm1014.mdl**
* **v_xm1014.mdl**
* **p_xm1014.mdl**

Required sounds:

* **weapons/xm1014-1.wav**
{{% /details_header %}}

### Sniper rifles

{{% hint warning %}}
Support for black frames around the crosshair when scoped is not implemented yet.
{{% /hint %}}

{{% details_header title="Scout" %}}
```json
{
    "weapon_sniperrifle": "cs/scout"
}
```

Required models:

* **w_scout.mdl**
* **v_scout.mdl**
* **p_scout.mdl**

Required sounds:

* **weapons/scout_fire-1.wav**
* **weapons/scout_bolt.wav**
* **weapons/scout_clipin.wav**
* **weapons/scout_clipout.wav**
* **weapons/zoom.wav**
{{% /details_header %}}

{{% details_header title="AWP" %}}
```json
{
    "weapon_sniperrifle": "cs/awp"
}
```

{{% hint info %}}
AWP in Counter Strike uses ammo type `338Magnum` which is not defined in Featureful SDK. This template uses `762` ammo type. You can change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to your liking.
{{% /hint %}}

Required models:

* **w_awp.mdl**
* **v_awp.mdl**
* **p_awp.mdl**

Required sounds:

* **weapons/awp1.wav**
* **weapons/boltpull1.wav**
* **weapons/boltup.wav**
* **weapons/boltdown.wav**
* **weapons/awp_deploy.wav**
* **weapons/awp_clipin.wav**
* **weapons/awp_clipout.wav**
* **weapons/zoom.wav**
{{% /details_header %}}

{{% details_header title="SG 550" %}}
```json
{
    "weapon_sniperrifle2": "cs/sg550"
}
```

Required models:

* **w_sg550.mdl**
* **v_sg550.mdl**
* **p_sg550.mdl**

Required sounds:

* **weapons/sg550-1.wav**
* **weapons/sg550_boltpull.wav**
* **weapons/sg550_clipin.wav**
* **weapons/sg550_clipout.wav**
* **weapons/zoom.wav**
{{% /details_header %}}

{{% details_header title="G3SG1" %}}
```json
{
    "weapon_sniperrifle2": "cs/g3sg1"
}
```

Required models:

* **w_g3sg1.mdl**
* **v_g3sg1.mdl**
* **p_g3sg1.mdl**

Required sounds:

* **weapons/g3sg1-1.wav**
* **weapons/g3sg1_slide.wav**
* **weapons/g3sg1_clipin.wav**
* **weapons/g3sg1_clipout.wav**
* **weapons/zoom.wav**
{{% /details_header %}}

### Machine guns

{{% details_header title="M249" %}}
```json
{
    "weapon_m249": "cs/m249"
}
```

Required models:

* **w_m249.mdl**
* **v_m249.mdl**
* **p_m249.mdl**

Required sounds:

* **weapons/m249-1.wav**
* **weapons/m249-2.wav**
* **weapons/m249_boxout.wav**
* **weapons/m249_boxin.wav**
* **weapons/m249_chain.wav**
* **weapons/m249_coverup.wav**
* **weapons/m249_coverdown.wav**
{{% /details_header %}}

## Deathmatch Classic

The templates for Deathmatch Classic weapons are stored in the **templates/weapons/dmc/** directory. Not all weapons are implemented yet.

{{% details "Example" open %}}
```json
{
    "weapon_shotgun": "dmc/shotgun",
    "weapon_shotgun2": "dmc/supershotgun",
    "weapon_nailgun": "dmc/nailgun",
    "weapon_rifle": "dmc/supernailgun"
}
```
{{% /details %}}

{{% details_header title="Shotgun" %}}
```json
{
    "weapon_shotgun": "dmc/shotgun"
}
```

A simple shotgun.

Required models:

* **v_shot.mdl**
* **p_shot.mdl**

{{% hint warning %}}
The **w_** model is missing in Deathmatch Classic because shotgun is provided to the player by default (so no need for the world model). This template uses **w_weaponbox.mdl** instead. Change it if you have a custom model.
{{% /hint %}}

Required sounds:

* **weapons/guncock.wav**
{{% /details_header %}}

{{% details_header title="Super Shotgun" %}}
```json
{
    "weapon_shotgun2": "dmc/supershotgun"
}
```

A simple double barrel shotgun.

Required models:

* **g_shot2.mdl**
* **v_shot2.mdl**
* **p_shot2.mdl**

Required sounds:

* **weapons/shotgn2.wav**
{{% /details_header %}}

{{% details_header title="Nailgun" %}}
```json
{
    "weapon_nailgun": "dmc/nailgun"
}
```

Required models:

* **g_nail.mdl**
* **v_nail.mdl**
* **p_nail.mdl**

Required sounds:

* **weapons/rocket1i.wav**
{{% /details_header %}}

{{% details_header title="Super Nailgun" %}}
```json
{
    "weapon_rifle": "dmc/supernailgun"
}
```

Required models:

* **g_nail2.mdl**
* **v_nail2.mdl**
* **p_nail2.mdl**

Required sounds:

* **weapons/spike2_dmc.wav**
{{% /details_header %}}

{{% hint info %}}
The DMC nailguns use a different model for the nails. You can copy **spike.mdl** from DMC under the **nail.mdl** name or use the following [entity template]({{< ref entity-templates >}}):

```json
{
    "dmc_nail": {
        "own_visual": {
            "model": "models/spike.mdl"
        }
    }
}
```
{{% /hint %}}

## Delta Particles

The templates for Delta Particles weapons are stored in the **templates/weapons/dp/** directory. Not all weapons are implemented yet.

{{% details "Example" open %}}
```json
{
    "weapon_9mmhandgun": "dp/glock",
    "weapon_357": "dp/python",
    "weapon_eagle": "dp/44desert_eagle",
    "weapon_9mmAR": "dp/m4a1",
    "weapon_smg": "dp/smg",
    "weapon_shotgun": "dp/shotgun",
    "weapon_sniperrifle": "dp/barrett_m82a1"
}
```
{{% /details %}}

Delta Particles changes the way some HL weapons behave as well as adds its own weapons. The common distinctive traits:

* Weapon models are more rich on sounds (played via animation events).
* Firearms produce screen shake effect on firing.
* Weapons may come with randomized amount of starting ammo.
* In Delta Particles weapons use the holster animation. This is not implemented yet.

{{% details_header title="Glock" %}}
```json
{
    "weapon_9mmhandgun": "dp/glock"
}
```

Glock from Delta Particles with burst fire and shells ejecting to the left side.

{{% hint warning %}}
This template is designed specifically for `weapon_9mmhandgun`, it won't work properly for other weapons without modifications.
{{% /hint %}}

Required model:

* **v_9mmhandgun.mdl** (custom model from Delta Particles)

Required sounds:

* **items/9mmclip_insert.wav**
* **items/9mmclip_release.wav**
* **items/9mmclip_slide.wav**
* **items/deploy_default1.wav**
* **weapons/common_hand1.wav**
* **weapons/common_hand2.wav**
* **weapons/glock_slideback1.wav**
{{% /details_header %}}

{{% details_header title="Python" %}}
```json
{
    "weapon_357": "dp/python"
}
```

Python with a scope ability and a laser spot when unscoped.

{{% hint warning %}}
This template is designed specifically for `weapon_357`, it won't work properly for other weapons without modifications.
{{% /hint %}}

Required model:

* **v_357.mdl** (custom model from Delta Particles)

Required sounds:

* **items/deploy_357.wav**
* **items/deploy_default2.wav**
* **weapons/357_bulletsout.wav**
* **weapons/magnum_ammo_in.wav**
* **weapons/magnum_ammo_out.wav**
{{% /details_header %}}

{{% details_header title=".44 Desert Eagle" %}}
```json
{
    "weapon_eagle": "dp/44desert_eagle"
}
```

{{% hint info %}}
Desert Eagle in Delta Particles uses ammo type `44` which is not defined in Featureful SDK. This template uses `357` ammo type. You can change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to your liking.
{{% /hint %}}

Required models:

* **w_44_desert_eagle.mdl**
* **v_44_desert_eagle.mdl**
* **p_44_desert_eagle.mdl**
* **44shell.mdl**

Required sounds:

* **items/44_slide.wav**
* **items/44insert1.wav**
* **items/44release1.wav**
* **items/44slide1.wav**
* **weapons/44_gun_fire.wav**
{{% /details_header %}}

{{% details_header title="M4A1" %}}
```json
{
    "weapon_9mmAR": "dp/m4a1"
}
```

M4A1 with a grenade launcher. Uses `556` ammo.

Required models:

* **w_m4a1.mdl**
* **v_m4a1.mdl**
* **p_m4a1.mdl**
* **rifleshell.mdl**

Required sounds:

* **weapons/556mm_fire.wav**
* **weapons/glauncher.wav** (custom sound from Delta Particles)
* **weapons/glauncher2.wav** (custom sound from Delta Particles)
* **items/gl_insertshell.wav**
* **items/glauncher_in.wav**
* **items/glauncher_out.wav**
* **items/clipinsert1.wav** (custom sound from Delta Particles)
* **items/cliprelease1.wav** (custom sound from Delta Particles)
* **items/AR_slide.wav**
* **items/deploy_default3.wav**
* **items/deploy_m4a1.wav**

{{% /details_header %}}

{{% details_header title="SMG" %}}
```json
{
    "weapon_smg": "dp/smg"
}
```

Required models:

* **w_smg.mdl**
* **v_smg.mdl**
* **p_smg.mdl**
* **45acp_shell.mdl**

Required sounds:

* **weapons/smg_fire1.wav**
* **weapons/smg_fire2.wav**
* **weapons/smg_deploy.wav**
* **weapons/common_hand2.wav**
* **items/deploy_default1.wav**
* **items/deploy_default2.wav**
* **items/smg_clipinsert1.wav**
* **items/smg_cliprelease1.wav**
* **items/smg_slide.wav**
* **items/smg_silencer_add.wav**
* **items/smg_silencer_remove.wav**

{{% /details_header %}}

{{% details_header title="Shotgun" %}}
```json
{
    "weapon_shotgun": "dp/shotgun"
}
```

{{% hint warning %}}
This template is designed specifically for `weapon_shotgun`, it won't work properly for other weapons without modifications.
{{% /hint %}}

{{% hint warning %}}
Delta Particles uses custom animation events and attachment point for shell ejection. This is not implemented in Featureful SDK. Instead, the ejection delay and shell offset are provided as constant value in the weapon template file.
{{% /hint %}}

Required models:

* **v_shotgun.mdl** (custom model from Delta Particles)

Required sounds:

* **weapons/shotgun_end.wav**
* **weapons/shotgun_start.wav**
* **weapons/12gauge_in.wav**
* **weapons/grip_start.wav**
* **weapons/grip_end.wav**
* **weapons/common_hand2.wav**
* **weapons/reload1.wav** (custom sound from Delta Particles)
* **weapons/reload3.wav** (custom sound from Delta Particles)
* **weapons/scock1.wav** (custom sound from Delta Particles)
{{% /details_header %}}

{{% details_header title="Barrett M82A1" %}}
```json
{
    "weapon_sniperrifle": "dp/barrett_m82a1"
}
```

Required models:

* **w_barrett_m82.mdl**
* **v_barrett_m82.mdl**
* **p_barrett_m82.mdl**
* **snipershell.mdl**

Required sounds:

* **items/deploy_barrett.wav**
* **weapons/sniper_fire.wav**
* **weapons/sniper_optic1.wav**
* **weapons/sniper_mag_out.wav**
* **weapons/sniper_mag_in.wav**
* **weapons/sniper_bolt2.wav**
* **weapons/sniper_bolt3.wav**
{{% /details_header %}}

## Half-Life

Full written configurations for some standard Half-Life weapons are stored in the **templates/weapons/hl** directory. These can be used as a reference. Or you can copy the configuration for your custom weapon and change it a bit if you want a weapon similar to the standard Half-Life one.

{{% details_header title="9mm Handgun (Glock)" %}}
```json
{
    "weapon_pistol": "hl/9mmhandgun"
}
```
{{% /details_header %}}

{{% details_header title="357 Python" %}}
```json
{
    "weapon_pistol2": "hl/357"
}
```
{{% /details_header %}}

{{% details_header title="9mm AR (MP5)" %}}
```json
{
    "weapon_smg": "hl/9mmAR"
}
```
{{% /details_header %}}

{{% details_header title="SPAS Shotgun" %}}
```json
{
    "weapon_shotgun2": "hl/shotgun"
}
```
{{% /details_header %}}

{{% details_header title="Crossbow" %}}
```json
{
    "weapon_sniperrifle2": "hl/crossbow"
}
```
{{% /details_header %}}

{{% details_header title="Hornetgun" %}}
```json
{
    "weapon_rifle": "hl/hornetgun"
}
```
{{% /details_header %}}

## Opposing Force

Full written configurations for some standard Opposing Force weapons are stored in the **templates/weapons/op4** directory. These can be used as a reference. Or you can copy the configuration for your custom weapon and change it a bit if you want a weapon similar to the standard Opposing Force one.

{{% details_header title="Desert Eagle" %}}
```json
{
    "weapon_pistol": "op4/eagle"
}
```
{{% /details_header %}}

{{% details_header title="M40A1" %}}
```json
{
    "weapon_sniperrifle2": "hl/sniperrifle"
}
```
{{% /details_header %}}

## Poke646

The templates for Poke646 weapons are stored in the **templates/weapons/poke646/** directory. Not all weapons are implemented yet.

{{% details "Example" open %}}
```json
{
    "weapon_pistol": "poke646/bradnailer",
    "weapon_nailgun": "poke646/nailgun"
}
```
{{% /details %}}

{{% details_header title="Bradnailer" %}}
```json
{
    "weapon_pistol": "poke646/bradnailer"
}
```

Required models:

* **w_bradnailer.mdl**
* **v_bradnailer.mdl**
* **p_bradnailer.mdl**

Required sounds:

* **weapons/bradnailer.wav**
{{% /details_header %}}

{{% details_header title="Nailgun" %}}
```json
{
    "weapon_nailgun": "poke646/nailgun"
}
```

Required models:

* **w_nailgun.mdl**
* **v_nailgun.mdl**
* **p_nailgun.mdl**

Required sounds:

* **weapons/nailgun.wav**
{{% /details_header %}}

## Sven Co-op

Starting with Sven Co-op 4.0 the mod uses custom behaviors and models for some Half-Life weapons. These templates are written for Sven Co-op 5.x weapons.

The common distinctive traits:

* Sven Co-op models use the `5005` animation event to play muzzleflashes. The mod has its own muzzleflash definition system which isn't implemented in Featureful SDK yet. Instead it has a simple stub of playing the same muzzleflash as default HL shotgun or python on the first attachment.

{{% details_header title="MP5" %}}
MP5 with a zoom for secondary attack instead of grenade launcher.

```json
{
    "weapon_9mmAR": "sc/mp5"
}
```

{{% hint info %}}
This weapon template uses the same model and sound paths as a Half-Life 9mmAR but the resources themselves are different in Sven Co-op.
{{% /hint %}}

{{% /details_header %}}

{{% details_header title="Shotgun" %}}
Shotgun with a faster secondary attack instead of doubleshot.

```json
{
    "weapon_shotung": "sc/shotgun"
}
```

{{% hint info %}}
This weapon template uses the same model and sound paths as a Half-Life shotgun but the resources themselves are different in Sven Co-op.
{{% /hint %}}

{{% /details_header %}}

## Team Fortress Classic

{{% hint warning %}}
Team Fortress Classic doesn't have world models for weapons. You may need to provide custom world models yourself.
{{% /hint %}}

{{% details_header title="Assault Cannon" %}}
```json
{
    "weapon_minigun": "tfc/assaultcannon"
}
```

A weapon used by the Heavy Weapon Guy class. This doesn't prevent jumping and uses `buckshot` ammo.

{{% hint warning %}}
This template is designed specifically for [weapon_minigun]({{< ref weapon_minigun >}}), it won't work properly for other weapons without modifications.
{{% /hint %}}

Required models:

* **v_tfac.mdl**
* **p_mini.mdl**

Required sounds:

* **weapons/asscan1.wav**
* **weapons/asscan2.wav**
* **weapons/asscan3.wav**

{{% /details_header %}}

## They Hunger

{{% details_header title="AP9" %}}
```json
{
    "weapon_smg": "hunger/ap9"
}
```

{{% hint info %}}
AP9 in They Hunger uses its own ammo type which is not defined in Featureful SDK. This template uses `45acp` ammo type. You can change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to your liking.
{{% /hint %}}

Required models:

* **w_ap9.mdl**
* **v_ap9.mdl**
* **p_ap9.mdl**

Required sounds:

* **weapons/ap9_fire.wav**
* **weapons/ap9_clipout.wav**
* **weapons/ap9_clipin.wav**
* **weapons/ap9_bolt.wav**

{{% /details_header %}}

{{% details_header title="Chaingun" %}}
```json
{
    "weapon_minigun": "hunger/chaingun"
}
```

Very similar to Team Fortress Classic assault cannon, but needs reloading. This doesn't impose the speed or jump penalty on the player and uses `9mm` ammo.

{{% hint warning %}}
This template is designed specifically for [weapon_minigun]({{< ref weapon_minigun >}}), it won't work properly for other weapons without modifications.
{{% /hint %}}

Required models:

* **v_tfac.mdl**
* **p_tfac.mdl**

Required sounds:

* **weapons/asscan1.wav**
* **weapons/asscan2.wav**
* **weapons/asscan3.wav**

{{% /details_header %}}

{{% details_header title="Taurus" %}}
```json
{
    "weapon_pistol": "hunger/taurus"
}
```

{{% hint info %}}
Taurus in They Hunger uses its own ammo type which is not defined in Featureful SDK. This template uses `45acp` ammo type. You can change [ammo_name]({{< ref "weapon-templates#ammo_name" >}}) to your liking.
{{% /hint %}}

Required models:

* **w_taurus.mdl**
* **v_taurus.mdl**
* **p_taurus.mdl**

Required sounds:

* **weapons/tau_fire.wav**
* **weapons/tau_back.wav**
* **weapons/tau_clipout.wav**
* **weapons/tau_clipin.wav**
* **weapons/tau_release.wav**

{{% /details_header %}}
