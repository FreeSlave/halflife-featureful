---
title: "FAQ"
---

# Frequently Asked Questions

### Why is it called Featureful?

In the context of this SDK features are adjustable gameplay aspects. Most of the features are configured via text files and don't require rebuilding of the game libraries. In that sense "customizable" or "featureable" would fit more, but "featureful" just sounds better.

### You call it featureFUL, but the *SDK X* provides more features to me!

There're different Half-Life SDKs available, and each of them has their own focus. **Featureful** may lack some of the features that exist in other projects, but it also introduces a bunch of its own additions and improvements. More features will be incorporated in future.

### I want to report an issue/request a feature.

You can do it either on github [Issues](https://github.com/FreeSlave/halflife-featureful/issues) or in the Hazard Team's [Discord server](https://discord.gg/A7ZErQAjfa)

### Is there a roadmap for future updates of this SDK?

There's no strict roadmap, but you can look at the [TODO]({{< ref TODO >}}) list.

### Is it well-tested?

The mods [Field Intensity](https://www.moddb.com/mods/field-intensity) and [Half-Life: Induction](https://www.moddb.com/mods/half-life-induction) are based on the same code. There're also several mods in development using this SDK. So one can say it's pretty well tested and continuously being tested in the new projects.

### Can I play the original Half-Life campaign using Featureful SDK?

Very likely yes. It's been a long time since I tested it though, so you're welcome to try and report any discovered issues.

### Can I play other mods using Featureful SDK?

The primary goal of Featureful SDK is to provide a base for *new* mods in development, not to run old mods.

You should be able to play mappacks and any mods that don't have code changes (i.e. they use vanilla Half-Life game libraries) and also mods for Opposing Force (with the proper configuration).

Everything else will require porting by hand as code modifications usually mean the mod has custom logic that can't be handled/interpreted by Featureful SDK. The porting process may include manual map changes (through the `ripent`, `bspguy` or similar software) and resource changes. You may have some success with porting certain mods but ultimately some mods are deemed to be impossible to port properly without introducing the modifications to the Featureful SDK codebase.

### Can I fork the repository and make my own changes to the codebase?

Yes. But in this case you become fully responsible for the code maintenance. Sometimes I make pretty big changes to the codebase that could become hard to merge if you made your own modifications and now want to merge with the upstream. Also most online tutorials won't work because they're written for vanilla HLSDK and the Featureful codebase is quite different.

The main goal of this SDK is to aid non-programmer developers. I may answer your questions regarding the code, but I'm not really interested in providing the assistance unless you want to contribute to the project (e.g. do a pull request).

### Can I borrow code from this SDK for my own project?

Yes. But it's up to you to figure out how to properly port the features you're interested in to your own codebase.

### Can Featureful SDK be used to create a multiplayer mod?

In theory yes. We didn't break anything multiplayer-related and the multiplayer environment is usually taken into account when making changes and designing new entities. Some of the changes were even introduced purely for multiplayer. But the main focus of this SDK is being a base for singleplayer mods.

### Why this SDK is built without VGUI by default?

It's to increase the portability of produced binaries among custom Xash3D versions that don't include support for VGUI. Most singleplayer mods don't use VGUI anyway, but it still can be enabled in the build configuration.

### Does it work on Android?

The releases include the **apk** for Android launcher. The application allows to set a game directory so in theory it can be used to play the clean (i.e. without code changes) featureful-based mod that didn't have an official release on Android.

### What are `[LP]`, `[LE]`, etc. in entity parameter names?

See [Locus system]({{< ref locus-system >}}).
