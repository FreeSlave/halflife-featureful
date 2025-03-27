# Compatibility

This page provides an overview on compatibility of Featureful SDK with existing mods and addons.

## ![](/images/opfor.png) Opposing Force

This SDK implements all singleplayer Opposing Force features:

* All Opposing Force monsters, weapons and singleplayer entities are implemented.
* Opfor nightvision is implemented.
* The Opposing Force campaign is fully playable (if SDK is configured right).
* Third-party mods may experience some quirks when running on this SDK due to small differences in entity implementations, but should work for the most part.
* CTF support is **not** implemented.
* Client and server are **not** interchangable with ones from Opposing Force.

## ![](/images/bshift.png) Blue Shift

Blue Shift is fully playable (if SDK is configured right). However if you want to run Blue Shift maps in your mod you need to convert them to regular bsp format with **BShiftBSPConverter**.

## ![](/images/spirit.png) Spirit of Half-Life

The full compatibility is not planned. We mainly aim at support of Spirit 1.2, but some minor features from later versions are included. So far the sdk includes following Spirit features:

* [Locus system]({{< ref "locus-system" >}}) is implemented and some entities support it (check fgd).
* Some Spirit entities, parameters and flags are implemented. Look for ![](/images/spirit.png) Spirit markers on [entities]({{< ref "entity-guide" >}}).

Things that are **NOT** yet implemented:

* Move-with is missing (it's being worked in the [separate branch](https://github.com/FreeSlave/hlsdk-xash3d/tree/movewith)). You still can use motion_manager.
* Aurora particle system is not implemented (you're free to contribute!).
* In Spirit almost any entity had a state and could be a master entity for something else. We still stick to only certain entities like `multisource`, `env_state` and some others having a master capability. See [Master-entities]({{< ref "master-entities" >}}).

## ![](/images/svencoop.png) Sven Co-op

This SDK implements a decent layer of compatibility with Sven Co-op 3.0, specifically:

* Most (but not all) of [entities](Other-entities) are implemented, including [monsters](Monsters) and [weapons](Weapons). Many flags and parameters are implemented too (including those from more recent versions of SC).
* Some [cvars](Cvars) are added.
* Reading map .cfg files is implemented, but some things must be adjusted for Sven Co-op, e.g. resetting the config between map changes.

Sven Co-op 3.0 features that are missing:

* Model replacements for weapons.
* Support for map-specific skill configs.
* `weapon_uzi_akimbo` and `weapon_minigun`.

## Conflicts

Some spawnflags from Spirit and Sven Co-op have the same value, but different meaning, thus can't be supported both at the same time.

|Entity|Flag value|![](/images/spirit.png) Flag in Spirit|![](/images/svencoop.png) Flag in Sven Co-op|Our implementation|
|------|----------|--------------|------------------|------------------|
|**func_pushable**|512|`Can't pull`|`Explosives only`|![](/images/svencoop.png) `Explosives only`|

There's a difference in default behavior of some entities.

|Entity|![](/images/spirit.png) Behavior in Spirit|![](/images/svencoop.png) Behavior in Sven Co-op|Our implementation|
|------|------------------|----------------------|------------------|
|**trigger_relay**|Forwards its activator to the target|Sends itself as activator, but has a flag to forward its activator|![](/images/svencoop.png) Same as in Sven Co-op|

## Duplication

Some features exist in Spirit of Half-Life and Sven Co-op, but they are implemented using different means. When possible, we support both ways and that leads to duplication.

|Entity|Feature|![](/images/spirit.png) Spirit implementation|![](/images/svencoop.png) Sven Co-op implemenetation|
|------|-------|--------------------------------------------|------------------------------------------------|
|**func_door**|Configuring door to behave like if it didn't have a name|`Force Touchable` flag|`Ignore Targetname` parameter|


Entity parameters that have the same meaning but different internal names (we support both):

|Entity|Parameter|![](/images/spirit.png) Name in Spirit|![](/images/svencoop.png) Name in Sven Co-op|
|------|---------|------------------|----------------------|
|**env_funnel**|Custom sprite|`netname`|`sprite`|
|**func_train**|Custom move sound|`custommovesnd`|`noise`|
|**func_train**|Custom stop sound|`customstopsnd`|`noise1`|
