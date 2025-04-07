# Troubleshooting

This page will describe the problems the modmaker can encounter while using GoldSource and the Featureful SDK specifically.

## I can't find func_detail, info_texlights, etc. in the entity list

You should add **zhlt.fgd** to your game profile configuration in the editor. `func_detail`, `info_texlights`, etc. are compiler-specific entities and the Featureful SDK doesn't interfere with what external tools you want to use, so these entities are not included in the **halflife_featureful.fgd**.

## I got some invisible walls on the map and some brush entities got wrong render properties

You probably recompiled the map with entities only (`-onlyents`). If the internal order of entities on the map has changed (e.g. you removed some entities) and you recompiled entities only, the brush entities might get properties of other brush entities. Try the standard compilation process and see if the issue is gone.

## Model 'modelname' failed to precache because the item count is over the 512 limit

The engine has a limit on the number of models and sprites it can handle on a map. The more *different* models your map uses the more likely it will hit the limit.

Note: each brush entity is counted to the limit as well since brush entities are also considered as models and they're precached by the engine just like .mdl models.

You can track the number of precached models by using the `dump_precached_models` command (in developer mode) which shows the number and lists all the models.

If your map hits this limit, there's a number of ways to decrease the quantity of models used.

* If some `func_wall` or `func_illusionary` doesn't have special properties (like custom render parameters or the targetname some other entities refer to them with) you can turn them info `func_detail`. It's not a real in-game entity, it exists only during the map compilation. Every `func_detail` becomes the part of the world geometry and thus they are not precached as separate models.
* Merge some `func_wall` and `func_illusionary` entities into one entity if they have the same properties. E.g. you might have some nearby railing done with several `func_illusionary` - merge them into one entity. Thus it will be counted as only one model.
* If your map uses a lot of prop models (i.e. static models made to fill the environment), you can combine these models into one model with many submodels (bodies). This requires changing your resources so it's far from ideal.
* Weapons are always getting precached (to allow them in the `impulse 101` command). And each weapon precaches at least 3 models (p_, v_ and w_). So if your mod doesn't need some custom weapons you better disable them (in **features/featureful_weapons.txt**).
* Set `weapon_p_models` to false in **features/featureful_server.cfg** - this will decrease the number of used models. This is suitable if your campaign doesn't show the player from a third person (e.g. through the camera).
* Split the map into two. Sometimes there's just no other way to overcome GoldSource limitations.

## Host_Error: PF_precache_sound_I: Sound 'soundname' failed to precache because the item count is over the 512 limit.

The engine has a limit on the number of sounds it can handle on a map. The more *different* sounds your map uses the more likely it will hit the limit.

You can track the number of precached models by using the `dump_precached_sounds` command (in developer mode) which shows the number and lists all the sounds.

If your map hits this limit, there's a number of ways to decrease the quantity of sounds used:

* Weapons are always getting precached and usually each weapon precaches its own unique sounds. So if your mod doesn't need some custom weapons you better disable them (in **features/featureful_weapons.txt**).
* Change the map so it uses less sounds. E.g. remove some ambient_generics or monster classes.
* Configure the [[soundscripts]] to use less sounds. For example `monster_rosenberg` uses 9 pain sounds by default. You can redefine it to include less sounds.

## The entities don't appear or the map crashes even if I just fly through it with the noclip.

If it happens only on a certain map and others work fine, look at the compilation log of the map in trouble.

If the log has something like `Warning: Number of world leaves(8325) exceeded MAX_MAP_LEAFS(8192)`, you need to fit the map into the GoldSource limits. There're two general approaches:

* Decrease the number of non-entity brush details on the map (remember that func_details are not real entities!).
* Turn some complex geometry or some func_details into `func_wall` or `func_illusionary`.

Recompile the map and see how the number changes and whether it fits the `MAX_MAP_LEAFS` limit (to speedup the process you can use fast compilation parameters, because the error happens during the `bsp` stage, so there's no need for the full `vis` or `rad`).

## The monsters of specific class don't appear in the game

You probably didn't enable the monster in the **features/featureful_monsters.cfg**.

## Some entities or some entity parameters are not listed in JACK level editor.

1. Make sure you set the right **fgd** in the mod profile configuration and open the map in the this configuration.
2. Make sure you're using the latest [JACK](https://jack.hlfx.ru/en/download.html)
