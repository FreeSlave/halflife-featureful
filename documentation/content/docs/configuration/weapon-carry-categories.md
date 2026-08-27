---
title: "Weapon carry categories"
---

# {{% param "title" %}}

Featureful SDK allows to enable one-per-category weapon inventory restriction. This allows to prohibit player from carrying more than one primary weapon and one secondary weapon at once (like it's implemented in Counter Strike), so player must choose what weapon to carry at a time. This is configured via the **features/weapon_carry_categories.cfg** file.

The format is simple: `weapon_name <category>`. Category must be a non-zero value.

Example:

```
weapon_crowbar 1
weapon_pipewrench 1
weapon_9mmhandgun 2
weapon_357 2
weapon_eagle 2
weapon_9mmAR 3
weapon_shotgun 3
weapon_crossbow 3
weapon_gauss 4
weapon_egon 4
```

Here the crowbar and pipewrench belong to category 1 and therefore are mutually exclusive (player can't carry both). Same with handgun/python/desert eagle, etc.

If player already carries a weapon from a certain category, he won't be able to pick up other weapons that belong to the same category by touch - he'll need to press a `+use` key in order to pick up a new weapon and drop the previous one.

{{% hint info %}}
If weapon belongs to some carry category it can be picked up by pressing `+use` even if `pickup_policy` cvar is set to 0.
{{% /hint %}}

{{% hint info %}}
Carry categories and the weapon layout in HUD are completely unrelated things.
{{% /hint %}}
