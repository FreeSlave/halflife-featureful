---
title: "Default keys"
bookToC: false
---

# {{% param "title" %}}

## Preface

When it comes to mods with non-standard commands that the player wants to have bound to the key, there's no easy and trusty way to provide the default bindings for these commands. Opposing Force and Counter Strike both introduce custom user commands and they provide their own **config.cfg** files. This means that your Half-Life settings (like keybindings, user name, sensitivity, etc.) are not carried over to these games making you to manually reapply the changes if you want to have the same custom controls and other settings in all games.

Featureful SDK introduces some custom commands, like `nightvision`, `recruit_followers` and `disband_followers`, so the mods based on Featureful SDK face the same problem if the developer wants these commands to be bound by default.

Another problem with Half-Life's config distributed by default is that it doesn't specify any keys bound to `slot6`, `slot7`, etc. commands. Which means that if you want to use weapon slots past 5 (which is possible in Featureful SDK) you also face the problem of players not being able to directly use these slots unless they change their settings.

Featureful SDK comes with a solution that allows to avoid distributing the custom **config.cfg** while still providing the default bindings, without the fear of rebinding the player's keys that have already been bound to something else.

## default_keys.cfg

The **default_keys.cfg** can be used to define the commands and the default keys bound to them.

The format is simple as `"<command>" "desired key"` on each line.

Example:

```c
"slot6" "6"
"slot7" "7"
"nightvision" "n"
"nightvision" "h"
"recruit_followers" "z"
"disband_followers" "x"
"impulse 1" "m"
```

Upon loading the game it will do the following checks:

1. Check if the command is already bound on some key/button. If it's true nothing will be done for this command. E.g. the user might have bound some keys to `slot6` and `slot7` in their Half-Life config. Or it's not the first time the user launches the mod and all commands have already been bound.
2. Check if the desired key/button is unbound. If it's already bound to something, another desired key will be used (if provided). In the example above we set desired keys for `nightvision` command to `n` and `h`, so if the player already has something bound to the `n` key (e.g. the `noclip` command), the `nightvision` will be bound to `h`. If the latter is already bound to something as well it won't do anything leaving the command not bound to any key.
3. The first desired unbound key will be bound to the specified command.

{{% hint info %}}
The default keybindings are applied during the client initialization (in the game menu) in GoldSource. On Xash3D due to the design differences the default keybindings will be applied only once the player is in the actual game (i.e. on some map).
{{% /hint %}}

{{% hint info %}}
The custom `impulse` commands are meant for map-specific actions that can be set up via the [trigger_impulse]({{< ref trigger_impulse >}}).
{{% /hint %}}

## Adding new commands to the options menu

You probably want to add new commands to **gfx/shell/kb_act.lst** and **gfx/shell/kb_def.lst**

Read also: [TWHL article on setting game strings](https://twhl.info/wiki/page/Tutorial%3A_Setting_up_a_Mod%3A_Part_3_-_Game_Strings_%28Steam%29)
