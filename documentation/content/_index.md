---
title: "Featureful SDK Overview"
---

# Half-Life Featureful SDK

**Half-Life Featureful** aims to serve as a base for a classic Half-Life mod. This SDK provides entities from various mods and official addons, as well as additional entity parameters, bugfixes and new monsters' abilities and behavioral changes. Half-Life Featureful is meant to be used by modmakers without experience in HLSDK programming, who seek for easy ways to bring more features to their mods.

Read how to [base the mod on Featureful SDK]({{< ref "getting-started" >}}).

## Feature overview

* All Opposing Force monsters and weapons are implemented, as well as other opfor-specific entities (excluding CTF-related). Some Sven Co-op monsters are included as well.
* Some of Spirit of Half-Life features merged into the codebase, e.g. the [locus system]({{< ref locus-system >}}).
* Model-based health and HEV chargers and eye scanner from PS2 version of Half-Life are implemented.
* Introduced many new entities and additional parameters for standard Half-Life entities. Timers, trigger randomizers. Alternatives to `multisource` that can be used as [master entities]({{< ref master-entities >}}). Useful properties for [monstermaker]({{< ref monstermaker >}}), [scripted_sequence]({{< ref scripted_sequence >}}) and [scripted_sentence]({{< ref scripted_sentence >}}).
* Precise configuration of some [behavior aspects and properties of monsters]({{< ref "monsters#new-common-features" >}}).
* Improvements to [monsters]({{< ref monsters >}}) AI and new abilities for standard monsters.
* A lot of features and behavior aspects can be adjusted specifically for your mod via the [Feature configuration]({{< ref configuration >}}) files without rebuilding the game libraries.
* The concept of [Soundscripts]({{< ref soundscripts >}}) similar to one from Source, which allows to configure sounds used by monsters and some other entities without changing the source code.
* The concept of [Visuals]({{< ref visuals >}}) that allows to configure models/sprites and render properties of some visual effects without changing the source code.
* The concept of [Entity templates]({{< ref entity-templates >}}).
* The concept of [Player templates]({{< ref player-templates >}}).
* The concept of [Weapon templates]({{< ref weapon-templates >}}).
* Configurable [warpball effects]({{< ref warpball-templates >}}) for monstermakers.
* [Subtitles]({{< ref subtitles >}}) system.
* [Journal]({{< ref journal >}}) system (for objectives information, etc.)
* [Player inventory]({{< ref player-inventory >}}) system (for quest items).
* [HUD scaling]({{< ref "hud#hud-scaling" >}})
* Weather effects like [rain]({{< ref env_rain >}}) and [snow]({{< ref env_snow >}}).
* Support for GoldSource (including old pre-SDL2 versions) and Xash3D-FWGS engines. VGUI and VGUI-less builds. Crossplatform: Windows, Linux, and virtually any platform supported by Xash3D-FWGS (including Android).

## SDK usage in real projects

Half-Life Featureful is not just SDK in vacuum. It's a result of working on the real mods.

* [Half-Life: Field Intensity](https://www.moddb.com/mods/field-intensity) uses the same codebase but versions before 1.6 were released before the feature configuration concept was introduced. Practically Field Intensity is the reason why this SDK exists. The Field Intensity 1.7 source code was released as a [branch](https://github.com/FreeSlave/halflife-featureful/tree/field_intensity_1.7) of this repository.
* [Half-Life: Induction](https://www.moddb.com/mods/half-life-induction) uses this codebase since version 1.3. The Induction source code was released as a [branch](https://github.com/FreeSlave/halflife-featureful/tree/induction) of this repository along with the release of version 1.4.
* [Half-Life: Hard Duty](https://www.moddb.com/mods/hard-duty-second-edition) - in development, demo available.
* [The Big Boned and The Curious](https://www.moddb.com/mods/the-big-boned-and-the-curious) - in development, demo available.
* [Half-Life: Recovery](https://www.moddb.com/mods/half-life-recovery) - in development.
* [Half-Life: Military Duty - Operation Firestorm](https://www.moddb.com/mods/military-duty) - in development.
* [X - Conundrum](https://www.moddb.com/mods/x-conundrum) - in development, demo available (the mod, however, branched off to utilize its own gamecode).
* [Half-Life: Deep Cover](https://www.moddb.com/mods/half-life-deep-cover) - in development, demo available.
* [Retrograde](https://www.moddb.com/mods/retrograde) - in development.
* [Specific impulse](https://www.moddb.com/mods/specific-impulse) - in development, demo available.

## Compatibility with other mods and addons

Throughout this wiki we use the following icons to mark if the entity, flag, parameter, cvar or some other feature is designed after some other mod or addon. The presence of the marker may not guarantee the full compatibility in terms of entity or cvar behavior, yet the modmaker can expect it to have a similar design and work almost in the same way.

* ![](/images/opfor.png) - *Opposing Force* entity, parameter or cvar.
* ![](/images/bshift.png) - *Blue Shift* compatible entity or flag.
* ![](/images/decay.png) - Entities from *Decay* and PS2 version of Half-Life.
* ![](/images/svencoop.png) - *Sven Co-op* compatible (or implemented in similar fashion) entity, parameter, cvar or feature.
* ![](/images/spirit.png) - *Spirit of Half-Life* compatible flag, parameter or entity.
* ![](/images/cstrike.png) - entity, cvar or feature from *Counter Strike*.
* ![](/images/czeror.png) - entity or feature from *Condition Zero Deleted Scenes*.
* ![](/images/tfc.png) - weapon or feature from *Team Fortress Classic*.

Read [compatibility]({{< ref "compatibility" >}}) for more detailed overwiew.

## FAQ

See [FAQ]({{< ref faq >}}).
