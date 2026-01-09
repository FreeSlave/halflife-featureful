---
weight: 9
title: "Updating your mod"
---

# Updating your mod

When new version of Featureful SDK gets released you usually want to update to the latest version as it might include important bugfixes and new useful features.

Starting with year 2026 we distribute new versions in two forms:

* The full sample mod, with all assets and examples.
* The dev-only package which includes updated libraries and FGD.

Prefer downloading the dev-only package. Download the sample mod if you need to see some examples of new features introduced in this version.

## Step 0: use your own mod directory

{{% hint danger %}}
Don't use the **featureful** directory for your mod development. There're several reasons not to do so.
{{% /hint %}}

You could have used the sample mod as a base for your own mod when you first decided to use this SDK, but since then you added your own resources, modified configuration files, etc. And you don't want the resources of your mod get mixed with the resources of the sample mod from the new release.

## Step 1: update important files

* If you downloaded the dev-only package, just unpack it to your mod directory.
* If you download the sample mod, the thing that you absolutely want to copy from the sample mod to your own mod directory is **dlls/** and **cl_dlls/** directories. They contain game libraries that implement all the new features.

## Step 2: using new fgd

For mapping: make sure you use the new fgd provided with the dev-only package or the sample mod (**halflife_featureful.fgd**).

## Step 3: applying new features

Read the release changelog and see if it introduces any changes to the configuration files or configuration options. The changelog is usually detailed enough to include all the information about the new configurable features and configuration files and deprecation or removal of the outdated ones.

If there's anything related to what you're using in your mod - adjust accordingly.

{{% hint info %}}
If new release includes new config examples - they're included only in the full sample mod, but not in the dev-only package.
{{% /hint %}}

The new release having new configurable features or new configuraion files doesn't require you to merge changes into your featureful files or copy new configs. The game library provides default values for all features, and all configs are optional. In fact, the mod technically can run without **features/** directory at all.

## Notes

{{% hint info %}}
The new version of Half-Life Featureful is generally not compatible with the save files created previously. It means that you shouldn't expect the old saves to work correctly. If your mod has been released already and you want to update it with the new release of Half-Life Featureful, you should warn users about possible breakage of the save files.
{{% /hint %}}
