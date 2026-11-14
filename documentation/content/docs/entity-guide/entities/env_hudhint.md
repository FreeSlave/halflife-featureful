---
bookHidden: true
bookToC: false
---

# env_hudhint

Display a HUD hint (related to the key binding) to the player who activated this entity, similar to [Source](https://developer.valvesoftware.com/wiki/Env_hudhint).

This allows to show messages like `F  Flashlight` where `F` is the key binding used to toggle flashlight.

The HUD hints are shown at the right part of the screen, like in Half-Life 2.

### Parameters

* `Command` - the command name, like `+attack`, `+use`, etc. The key binding will be automatically found for the command. Unless `Custom message` is set, it will try to find a corresponding command description in **gfx/shell/kb_act.lst**. Note that displaying the command description may not work properly in Xash3D as it doesn't automatically localize the drawn texts (the ones that start with `#`).
* `Message` - a custom message to show along with the command key (or on its own if `Command` is not set). This must be the message id from [messages.json]({{< ref "text-messages/#translatable-messages" >}}).

{{% hint info %}}
At least `Command` or `Message` (or both) is expected to be set.
{{% /hint %}}

{{% hint warning %}}
The `Message` must refer to the single-line message. To show multi-line texts use [game_messagebox]({{< ref game_messagebox >}}).
{{% /hint %}}
