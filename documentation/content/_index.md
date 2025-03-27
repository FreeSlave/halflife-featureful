---
title: "Feauteful SDK Overview"
---

# Half-Life Featureful SDK

**Half-Life Featureful** aims to serve as a base for a classic Half-Life mod. This SDK provides entities from various mods and official addons, as well as additional entity parameters, bugfixes and new monsters' abilities and behavioral changes. Half-Life Featureful is meant to be used by modmakers without experience in HLSDK programming, who seek for easy ways to bring more features to their mods.

Read how to [base the mod on Featureful SDK]({{< ref "getting-started" >}}).

## Feature overview

* All Opposing Force monsters and weapons are implemented, as well as other opfor-specific entities (excluding CTF-related). Some Sven Co-op monsters are included as well.
* Some of Spirit of Half-Life features merged into the codebase, e.g. the [locus system]({{< ref locus-system >}}).
* Modeled health and HEV chargers and eye scanner from PS2 version of Half-Life are implemented.
* Introduced many new entities and additional parameters for standard Half-Life entities. Timers, trigger randomizers. Alternatives to `multisource` that can be used as [master entities]({{< ref master-entities >}}). Useful properties for [monstermaker]({{< ref monstermaker >}}), [scripted_sequence]({{< ref scripted_sequence >}}) and [scripted_sentence]({{< ref scripted_sentence >}}).
* Precise configuration of some [behavior aspects and properties of monsters]({{< ref "monsters#new-common-features" >}}).
* Improvements to [monsters]({{< ref monsters >}}) AI and new abilities for standard monsters.
* The concept of [Soundscripts]({{< ref soundscripts >}}) similar to one from Source, which allows to configure sounds used by monsters and some other entities without changing the source code.
* The concept of [Visuals]({{< ref visuals >}}) that allows to configure models/sprites and render properties of some visual effects without changing the source code.
* A lot of features and behavior aspects can be adjusted specifically for your mod via the [Feature configuration]({{< ref configuration >}}) files without rebuilding the game libraries. E.g. the unused custom weapons can be disabled and their assets won't need to be distributed with the mod.
* Configurable [warpball effects]({{< ref warpball-templates >}}) for monstermakers.
* [Subtitles]({{< ref subtitles >}}) system.
* Simple [player inventory]({{< ref player-inventory >}}) system.
* [HUD scaling]({{< ref "hud#hud-scaling" >}})
* Weather effects like [rain]({{< ref env_rain >}}) and [snow]({{< ref env_snow >}}).
* Crossplatform. Support for GoldSource (including old pre-SDL2 versions) and Xash3D-FWGS engines. VGUI and VGUI-less builds. SDK potentially may work on mobile platforms, but it needs to be tested.

## SDK usage in real projects

Half-Life Featureful is not just SDK in vacuum. It's a result of working on the real mods:

* [Half-Life: Field Intensity](https://www.moddb.com/mods/field-intensity) uses the same codebase but versions before 1.6 were released before the feature configuration concept was introduced. Practically Field Intensity is the reason why this SDK exists.
* [Half-Life: Induction](https://www.moddb.com/mods/half-life-induction) uses this codebase since version 1.3. The Induction source code was released as a [branch](https://github.com/FreeSlave/halflife-featureful/tree/induction) of this repository along with the release of version 1.4.
* [Half-Life: Hard Duty](https://www.moddb.com/mods/hard-duty-second-edition) - in development, demo available.
* [The Big Boned and The Curious](https://www.moddb.com/mods/the-big-boned-and-the-curious) - in development, demo available.
* [Half-Life: Recovery](https://www.moddb.com/mods/half-life-recovery) - in development.
* [Half-Life: Military Duty - Operation Firestorm](https://www.moddb.com/mods/military-duty) - in development.
* [X - Conundrum](https://www.moddb.com/mods/x-conundrum) - in development, demo available (the mod, however, branched off to utilize its own gamecode).
* [Half-Life: Deep Cover](https://www.moddb.com/mods/half-life-deep-cover) - in development, demo available.
* [Retrograde](https://www.moddb.com/mods/retrograde) - in development.

## Compatibility with other mods and addons

Throughout this wiki we use the following icons to mark if the entity, flag, parameter, cvar or some other feature is designed after some other mod or addon. The presence of the marker may not guarantee the full compatibility in terms of entity or cvar behavior, yet the modmaker can expect it to have a similar design and work almost in the same way.

* ![](/images/opfor.png) - *Opposing Force* entity, parameter or cvar.
* ![](/images/bshift.png) - *Blue Shift* compatible entity or flag.
* ![](/images/decay.png) - Entities from *Decay* and PS2 version of Half-Life.
* ![](/images/svencoop.png) - *Sven Co-op* compatible (or implemented in similar fashion) entity, parameter, cvar or feature.
* ![](/images/spirit.png) - *Spirit of Half-Life* compatible flag, parameter or entity.
* ![](/images/cstrike.png) - entity, cvar or feature from *Counter Strike*.

Read [compatibility]({{< ref "compatibility" >}}) for more detailed overwiew.

## FAQ

**Q:** Why is it called Featureful?  
**A:** In the context of this SDK features are adjustable gameplay aspects. Most of the features are configured via text files and don't require rebuilding of the game libraries. In that sense "customizable" or "featureable" would fit more, but "featureful" just sounds better.

**Q:** You call it featureFUL, but the *SDK X* provides more features to me!  
**A:** There're different Half-Life SDKs available, and each of them has their own focus. **Featureful** may lack some of the features that exist in other projects, but it also introduces a bunch of its own additions and improvements. More features will be incorporated in future.

**Q:** I want to report an issue/request a feature.  
**A:** You can do it either on github [Issues](https://github.com/FreeSlave/halflife-featureful/issues) or in the Hazard Team's [Discord server](https://discord.gg/A7ZErQAjfa)

**Q:** Is there a roadmap for future updates of this SDK?  
**A:** There's no strict roadmap, but you can look at the [[TODO]] list.

**Q:** Is it well-tested?  
**A:** The mods [Field Intensity](https://www.moddb.com/mods/field-intensity) and [Half-Life: Induction](https://www.moddb.com/mods/half-life-induction) are based on the same code. There're also several mods in development using this SDK. So one can say it's pretty well tested and continuously being tested in the new projects.

**Q:** If Field Intensity uses the same codebase, why Featureful SDK lacks some of FI additions like objectives interface and improved squad mechanic?  
**A:** Some of these features were quite experimental and not ready to be merged back into the main codebase. Some of them are made for Field Intensity only and probably won't be merged back at all.

**Q:** Can I play the original Half-Life campaign using Featureful SDK?  
**A:** Very likely yes. It's been a long time since I tested it though, so you're welcome to try and report any discovered issues.

**Q:** Can Featureful SDK be used to create a multiplayer mod?  
**A:** In theory yes. We didn't break anything multiplayer-related and the multiplayer environment is usually taken into account when making changes and designing new entities. Some of the changes were even introduced purely for multiplayer. But the main focus of this SDK is being a base for singleplayer mods.

**Q:** Why this SDK is built without VGUI by default?  
**A:** It's to increase the portability of produced binaries among custom Xash3D versions that don't include support for VGUI. Most singleplayer mods don't use VGUI anyway, but it still can be enabled in the build configuration.

**Q:** What are `[LP]`, `[LE]`, etc. in entity parameter names?  
**A:** See [Locus system]({{< ref locus-system >}}).
