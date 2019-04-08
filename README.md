# Half-Life SDK for Xash3D and GoldSource [![Build Status](https://travis-ci.org/FreeSlave/hlsdk-xash3d.svg)](https://travis-ci.org/FreeSlave/hlsdk-xash3d) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/FreeSlave/hlsdk-xash3d?svg=true)](https://ci.appveyor.com/project/FreeSlave/hlsdk-xash3d)

Half-Life SDK for Xash3D & GoldSource with some fixes and features that can be useful for mod makers.

## Features

Most of the new features can be enabled or disabled. Go to game_shared/mod_features.h to select features you need.
Note that some features may require additional files to be included in your mod distribution. These can be models, sounds, sprites, etc.

In case you enable new monsters or weapons don't forget to add their skill values to skill.cfg. Talking monsters will also require changes in sentences.txt.

### New monsters

#### Opposing Force monsters

* monster_cleansuit_scientist
* monster_otis
* monster_blkop_osprey
* monster_blkop_apache
* monster_male_assassin
* monster_alien_voltigore
* monster_alien_babyvoltigore
* monster_pitdrone
* monster_shocktrooper
* monster_shockroach
* monster_pitworm (works a bit different from original, might be buggy)
* monster_geneworm (buggy)
* monster_human_grunt_ally
* monster_human_medic_ally (healing works a bit different from original)
* monster_human_torch_ally
* monster_drillsergeant
* monster_recruit
* monster_gonome
* monster_zombie_barney
* monster_zombie_soldier
* monster_op4loader

#### Sven Co-op monsters

* monster_babygarg - smaller version of Gargantua monster
* monster_hwgrunt
* monster_robogrunt

#### Opposing Force dead monsters

* monster_cleansuit_scientist_dead
* monster_otis_dead
* monster_human_grunt_ally_dead
* monster_alien_slave_dead (also has original Half-Life dead poses)
* monster_houndeye_dead
* monster_gonome_dead
* monster_zombie_soldier_dead

#### Other dead monsters

* monster_male_assassin_dead (was not in Opposing Force)
* monster_human_medic_ally_dead (was not in Opposing Force)
* monster_human_torch_ally_dead (was not in Opposing Force)
* monster_alien_grunt_dead
* monster_pitdrone_dead
* monster_shocktrooper_dead (Opposing Force had monster_ShockTrooper_dead, but it was bugged)
* monster_zombie_dead
* monster_zombie_barney_dead
* monster_generic_dead - generic dead monster with custom model and settable pose

### Monster features

* Health, relationship class, blood color, gibs and model can be customized in map editor.
* monster_barnacle health can be configured via skill.cfg
* Houndeye squad leader can play leaderlook animation.
* Houndeye shut their eyes when sleeping.
* Alien grunts, bullsquids, houndeyes, gonomes, pitdrones and voltigores restore health when they eat meat or enemy corpses.
* Alien slave attack rate can be configured via skill.cfg (In Half-Life it's hardcoded as 1 for easy and normal, and 1.5 for hard difficulty)
* Bullsquids can shoot alternative spit projectile that is slow poisonous ball.
* Configurable sound volume for apache and osprey rotors.
* Human grunts take into account allies that are not in their squad when checking for friendly fire.
* Security guards now check for friendly fire too.
* Security guards can be spawned with gun drawn.
* Scientists can heal other allies beside the player (e.g. security guards and other scientists)
* The delay before scientist can heal again can be configured via skill.cfg (1 minute by default)
* Ally monsters can have a configurable tolerance level to player hits.
* Monsters can have an alternative trigger condition to fire their trigger target.

### Alien slave features

Alien slaves (also known as vortigaunts) got many new abilities. Now they gain energy to heal themselves when attack enemies with lightning. They can use free energy to heal or revive ally vortigaunts and boost their claws damage. They also can use a coil attack and summon familiars (snarks or headcrabs). Their hands are glowing when they use their abilities. The available abilities can be configured in dlls/islave.cpp.

### New weapons

#### Opposing Force weapons

All Opposing Force weapons and corresponding ammo entities are implemented, but they may work a bit different.

* weapon_pipewrench
* weapon_knife (also has an alternative attack)
* weapon_grapple (no prediction, not tested in multiplayer)
* weapon_eagle
* weapon_penguin
* weapon_m249
* weapon_sniperrifle
* weapon_displacer
* weapon_sporelauncher
* weapon_shockrifle (plays idle animations)

#### Other new weapons

* weapon_medkit - TFC-like medkit that allows to heal allies (not enabled by default)

### Other new entities

#### Opposing Force entities

* env_blowercannon
* env_electrified_wire
* env_rope
* env_spritetrain (also supports animated sprites)
* item_generic
* item_nuclearbomb
* monster_skeleton_dead
* op4mortar
* trigger_playerfreeze

#### Decay entities

* item_eyescanner (works a bit different, may be not compatible with Decay PC maps)
* item_healthcharger
* item_recharge

#### Spirit entities

* env_model
* env_state

#### Sven Co-op entities

* env_xenmaker
* squadmaker (Actually just an alias to monstermaker since it has most of squadmaker features already, not all of them though)
* trigger_random
* trigger_random_time
* trigger_random_unique
* trigger_respawn

#### Others

* env_modeltrain - like env_spritetrain, but with animated model and configurable movement sound.
* env_warpball - easy way to create a teleportation effect for monster spawns. Also can be set as a template for monstermaker.
* trigger_killmonster - kill monster (possibly gibbing), playing random death animation.
* trigger_timer - continuously fire the same target over random delays.
* game_player_settings - give starting weapons, configure starting armor and health in singleplayer.
* multi_trigger - alternative to multi_manager for which J.A.C.K. can show connections to its targets.
* item_flashlight can be enabled to give player flashlight without giving a suit.

### New flags and parameters compatible with existing addons and mods

#### Opposing Force

Besides opfor entities (mentioned above) some opfor-specific parameters were added:

* 'Suspicious' parameter for scientists and security guards that make them pre-provoked towards the player.

#### Spirit

Very limited set of spirit-compatible features is implemented. *Move-with* is **NOT** implemented.

* func_train has 'Origin on paths' flag.
* Custom move and stop sounds can be configured for func_train and func_plat
* Monsters and models scale can be configured.
* 'Direct use only' flag for buttons and 'Direct use only' parameter for doors.
* 'Don't Drop Gun' flag for some monsters and monstermaker.
* Ally monsters can be 'locked' by master, so they decline following even if they are not pre-disaster.
* 'Decline following' sentence can be configured.

Note that the full compatibility with Spirit maps is not a goal.

#### Sven Co-op

Besides some opfor and sven co-op entities mentioned above there're some features made for compatibility with Sven Co-op maps.

* info_player_deathmatch can be turned on and off.
* trigger_push has 'No clients' and 'No monsters' flags
* 'Explosive Only' flag for breakables.
* Some spawnobject values in breakables are compatible with Sven Co-op.
* Wall chargers sounds and capacity can be configured in level editor.
* 'Is Player Ally' parameter for monsters that inverts their relationship with player and sets a different model for some monsters.
* 'Grunt Type' parameter for osprey.
* 'In-game name' parameter and `mp_allowmonsterinfo` cvar to show monster's display names in multiplayer.
* `npc_dropweapons` cvar to enable/disable weapon dropping by monsters in multiplayer.
* `mp_weapon_respawndelay`, `mp_ammo_respawndelay`, `mp_item_respawndelay` cvars to control item respawn time.
* `mp_npckill` cvar to control whether ally npc can get damage in co-op game.
* `mp_respawndelay` cvar.

Note that the full compatibility with Sven Co-op maps is not a goal.

### Other features

* Wall health and armor chargers now play "no" sound when player use them having full health or armor.
* Wall chargers can be turned off and on by triggers.
* Nightvision can be enabled instead of flashlight. Both Opposing Force and Counter Strike nightvision versions are implemented.
* 'Op4Mortar only' flag for breakables. Breakables with this flag can be destroyed only with op4mortar shells.
* monstermaker can have env_warpball template to automatically play teleportation effects on monster spawn.
* monstermaker can set custom health, body, skin, blood color, relationship class, gibs and model for spawned monsters.
* New flag for monstermaker - No ground check. The hack to remove the check for space under the monstermaker. Useful for spawning flying monsters or monsters that should fall.
* func_plat, func_train and func_door sound attenuation can be configured in level editor.
* Added use_through_walls cvar to prevent using things through walls (experimental, will be made into compile-time feature).
* func_tank can be configured to have a limited number of ammo.
* func_breakable can contain new items (e.g. Opposing Force weapons and ammo).
* Amount of health provided by soda may be configured via skill.cfg
* item_security can show message and play sound on pickup. If item_security is featured in hud.txt the sprite will be shown upon item pickup.
* 'Fire on Animation start' property for scripted sequence - allows to sync animation start and firing a trigger (useful for retina animations).
* Dropped weaponboxes have a model of dropped weapon.
* `mp_healthcharger_rechargetime` and `mp_hevcharger_rechargetime` cvars to control wall chargers recharge time.
* `mp_dmgperscore` and `mp_allydmgpenalty` to control how many points the player gets for dealing the damage in Co-op.
* `mp_keepinventory` to keep player inventory across changelevels in co-op.
* Map .cfg files for co-op games featuring starting weapons and ammo, similar to Sven Co-op.

## How to build

### CMake as most universal way

    mkdir build && cd build
    cmake ../
    make

Crosscompiling using mingw:

    mkdir build-mingw && cd build-mingw
    TOOLCHAIN_PREFIX=i686-w64-mingw32 # check up the actual mingw prefix of your mingw installation
    cmake ../ -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER="$TOOLCHAIN_PREFIX-gcc" -DCMAKE_CXX_COMPILER="$TOOLCHAIN_PREFIX-g++"

You may enable or disable some build options by -Dkey=value. All available build options are defined in CMakeLists.txt at root directory.
See below if you want to build the GoldSource compatible libraries.

See below, if CMake is not suitable for you:

### Windows

#### Using msvc

We use compilers provided with Microsoft Visual Studio 6. There're `compile.bat` scripts in both `cl_dll` and `dlls` directories.
Before running any of those files you must define `MSVCDir` variable which is the path to your msvc installation.

    set MSVCDir=C:\Program Files\Microsoft Visual Studio
    compile.bat

These scripts also can be ran via wine:

    MSVCDir="z:\home\$USER\.wine\drive_c\Program Files\Microsoft Visual Studio" wine cmd /c compile.bat

The libraries built this way are always GoldSource compatible.

There're dsp projects for Visual Studio 6 in `cl_dll` and `dlls` directories, but we don't keep them up-to-date. You're free to adapt them for yourself and try to import into the newer Visual Studio versions.

#### Using mingw

TODO

### Linux

    (cd dlls && make)
    (cd cl_dll && make)

### OS X

Nothing here.

### FreeBSD

    (cd dlls && gmake CXX=clang++ CC=clang)
    (cd cl_dll && gmake CXX=clang++ CC=clang)

### Android

Just typical `ndk-build`.
TODO: describe what it is.

### Building GoldSource-compatible libraries

To enable building the goldsource compatible client library add GOLDSOURCE_SUPPORT flag when calling cmake:

    cmake .. -DGOLDSOURCE_SUPPORT=ON

or when using make without cmake:

    make GOLDSOURCE_SUPPORT=1

Unlike original client by Valve the resulting client library will not depend on vgui or SDL2 just like the one that's used in FWGS Xash3d.

Note for **Windows**: it's not possible to create GoldSource compatible libraries using mingw, only msvc builds will work.

Note for **Linux**: GoldSource requires libraries (both client and server) to be compiled with libstdc++ bundled with g++ of major version 4 (versions from 4.6 to 4.9 should work).
If your Linux distribution does not provide compatible g++ version you have several options.

#### Method 1: Statically build with c++ library

This one is the most simple but has a drawback.

    cmake ../ -DGOLDSOURCE_SUPPORT=ON -DCMAKE_C_FLAGS="-static-libstdc++ -static-libgcc"

The drawback is that the compiled libraries will be larger in size.

#### Method 2: Build in Steam Runtime chroot

This is the official way to build Steam compatible games for Linux.

Clone https://github.com/ValveSoftware/steam-runtime and follow instructions https://github.com/ValveSoftware/steam-runtime#building-in-the-runtime

    sudo ./setup_chroot.sh --i386

Then use cmake and make as usual, but prepend the commands with `schroot --chroot steamrt_scout_i386 --`:

    mkdir build-in-steamrt && cd build-in-steamrt
    schroot --chroot steamrt_scout_i386 -- cmake ../ -DGOLDSOURCE_SUPPORT=ON
    schroot --chroot steamrt_scout_i386 -- make

#### Method 3: Create your own chroot with older distro that includes g++ 4.

Use the most suitable way for you to create an old distro 32-bit chroot. E.g. on Debian (and similar) you can use debootstrap.

    sudo debootstrap --arch=i386 jessie /var/chroot/jessie-debian-i386 # On Ubuntu type trusty instead of jessie
    sudo chroot /var/chroot/jessie-debian-i386

Inside chroot install cmake, make, g++ and libsdl2-dev. Then exit the chroot.

On the host system install schroot. Then create and adapt the following config in /etc/schroot/chroot.d/jessie.conf (you can choose a different name):

```
[jessie]
type=directory
description=Debian jessie i386
directory=/var/chroot/debian-jessie-i386/
users=yourusername
groups=yourusername
root-groups=root
preserve-environment=true
personality=linux32
```

Insert your actual user name in place of `yourusername`. Then prepend any make or cmake call with `schroot -c jessie --`:

    mkdir build-in-chroot && cd build-in-chroot
    schroot --chroot jessie -- cmake ../ -DGOLDSOURCE_SUPPORT=ON
    schroot --chroot jessie -- make

#### Method 4:  Install the needed g++ version yourself

TODO: describe steps.

#### Configuring Qt Creator to use toolchain from chroot

Create a file with the following contents anywhere:

```sh
#!/bin/sh
schroot --chroot steamrt_scout_i386 -- cmake "$@"
```

Make it executable.
In Qt Creator go to `Tools` -> `Options` -> `Build & Run` -> `CMake`. Add a new cmake tool and specify the path of previously created file.
Go to `Kits` tab, clone your default configuration and choose your CMake tool there.
Choose the new kit when opening CMakeLists.txt.
