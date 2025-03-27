---
weight: 9
title: "Updating your mod"
---

# Updating your mod

When new version of Featureful SDK gets released you usually want to update to the latest version as it might include important bugfixes and new useful features.

So, you downloaded the new release and it might be tempting to just update the contents of your mod with the contents of **featureful** directory of the sample mod, but do not hurry!

## Step 0: use your own mod directory

{{% hint danger %}}
Don't use the **featureful** directory for your mod development. There're several reasons not to do so.
{{% /hint %}}

You could have used the sample mod as a base for your own mod when you first decided to use this SDK, but since then you added your own resources, modified configuration files, etc. And you don't want the resources of your mod get mixed with the resources of the sample mod from the new release.

## Step 1: copy dlls

The thing that you absolutely want to copy from the sample mod to your own mod directory is **dlls** and **cl_dlls** directories. They contain game libraries that implement all the new features.

## Step 2: using new fgd

For mapping: make sure you use the new fgd provided with the sample mod (**halflife_featureful.fgd**).

## Step 3: applying new features

Next step is to examine the **features** directory of the new release and see if there're any new files or new configurable features in these files. The changelog of the release is usually detailed enough to include all the information about the new configurable features and deprecation of the old ones.

You probably adjusted the featureful configs for your mod during the development, so replacing them with the ones from the new release is not a good idea (you risk losing your modifications).

The new release having new configurable features doesn't require you to merge changes into your featureful files. The game library provides default values for all features. In fact, the mod technically can run without **features** directory at all, every featureful file is optional.

So, just add and configure those features that you find useful.

## Step 4: new stuff outside of "features" directory

Sometimes the new release of the sample mod includes new configuration files serving as example on configurataion of some new aspect introduced in Half-Life Featureful. For example, the sample mod contains **sound/captions.txt** related to the captions system. These files serve just as an example. You don't need to copy them to your mod, but it's good idea to examine these files to learn their format in case you find the feature useful.

Again, new optional files are usually described in the changelog.

## Notes

{{% hint info %}}
The new version of Half-Life Featureful is generally not compatible with the save files created previously. It means that you shouldn't expect the old saves to work correctly. If your mod has been released already and you want to update it with the new release of Half-Life Featureful, you should warn users about possible breakage of the save files.
{{% /hint %}}
