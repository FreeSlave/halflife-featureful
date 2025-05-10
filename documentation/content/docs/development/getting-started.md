---
weight: 2
title: "Getting started"
---

# {{% param "title" %}}

So, you want to create a new mod or port an existing one using Featureful SDK.

## Download the mod sample and prepare your mod directory

1. Grab the archive with the mod sample from the [Releases](https://github.com/FreeSlave/halflife-featureful/releases) (download the associated halflife_featureful zip asset). Install it like you install other mods, i.e. unpack the archive into your Half-Life directory, so the **featureful** directory ends up being a subdirectory of Half-Life directory.
2. Create a directory for your own mod (if you haven't done that yet).
3. Copy **cl_dlls/**, **dlls/**, **delta.lst** and optionally **features/** and **templates/** from the **featureful** directory to your mod directory.
4. Copy **liblist.gam** as well and edit it to give a different `game` name. If you have previously created liblist.gam in your mod directory, make sure `gamedll` refers to `dlls/hl.dll`.
5. Alternatively, if you're starting from scratch, instead of doing 2-3 you can just rename the **featureful** directory to something else and edit **liblist.gam** afterwards. Note that the sample mod includes some resources you may not need, so it's better to start with your own mod directory and then copy only necessary things.
6. In your mod directory go to the **features/** subdirectory and start adjusting features as described below.

{{% hint warning %}}
**DO NOT** develop your mod in the **featureful** directory. Choose a different name for your mod directory. This will save you from risks of replacing your files when you install a new version of the sample mod. You can do some experiments and tests in the sample mod but don't treat it as a playground for your own work.
{{% /hint %}}

## Feature configuration

The main point of Featureful SDK is being configurable via a set of configuration files. 

You can configure various aspects of the game by editing the configuration files in **features/** directory (e.g. change the default HUD color or change the light source provided by suit from flashlight to NVG). Read [Feature configuration]({{< ref "configuration" >}}) about the files format.

## Enabling custom weapons

Decide which of the available weapons you want to include in your mod. Based on that you should copy all required resources to your mod either from the mod sample or other sources (e.g. from gearbox/ folder of Opposing Force). The resources vary depending on the weapon and usually include:

* Models (`v_`, `p_` and `w_`)
* HUD sprites and .txt file describing the weapon hud (e.g. weapon_eagle.txt)
* Projectile sprites and models (for some weapons, e.g. shock rifle, spore launcher)
* Sounds
* `events/*.sc` file (the Grapple doesn't need `.sc` file)
* Additional skill cvars in skill.cfg.

Open **features/featureful_weapons.cfg** in your mod directory and configure the list of enabled custom weapons.

{{% details "The list of all available non-standard weapons" %}}
```
pipewrench
knife
medkit
grapple
eagle
m249
displacer
sniperrifle
sporelauncher
shockrifle
penguin
uzi
```
{{% /details %}}

## Enabling custom monsters

Decide which of available non-standard Half-Life monsters you want to be use in the mod. Monsters need new resources to be included in the mod directory.

* Models (the monster's model itself; sometimes gibs and projectiles)
* Sounds
* Sprites (visual effects related to the monster)
* Additional skill cvars in skill.cfg
* Some monsters need additional entries in the `sound/sentences.txt`. E.g. Otis needs sentences with `OT_`prefix.

Open **features/featureful_monsters.cfg** in your mod directory and configure the list of enabled monsters.

{{% details "The list of all available non-standard monsters" %}}
```
cleansuit_scientist
rosenberg
keller
otis
barniel
kate

babygarg

gonome
zombie_barney
zombie_soldier

human_grunt_ally
human_grunt_medic
human_grunt_torch

hwgrunt

male_assassin
blkop_apache
blkop_osprey

robogrunt

pitdrone
shocktrooper
shockroach
voltigore
babyvoltigore
pitworm
geneworm

drillsergeant
recruit

floater
flybee
panthereye
robocop
```
{{% /details %}}

## Configuring server-side features

You can edit some server-side features via **features/featureful_server.cfg**. The file provided in the mod sample directory contains comments describing the features, so we don't go into detail on each feature here.

There's also a **features/featureful_exec.cfg** file which is getting executed by the engine and needs to be in the regular `.cfg` format (like server.cfg or skill.cfg). In this file you can set some cvars to values specific for your mod for features that are currently implemented as cvars only (e.g. `pushablemode` cvar).

Read some [recommendations]({{< ref "configuration.md/#recommendations" >}}) on configuring the mod.

## Configuring client-side features

You can edit some client-side features via **features/featureful_client.cfg**. The file provided in the mod sample directory contains comments describing the features, so we don't go into detail on each feature here.

## Further reading

* [Developing the mod]({{< ref developing-the-mod >}})
* [Feature configuration]({{< ref configuration >}})
* [Updating your mod]({{< ref updating-your-mod >}})
* [Shipping your mod]({{< ref shipping-your-mod >}})
