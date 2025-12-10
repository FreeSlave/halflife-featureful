---
bookHidden: true
bookToC: false
---

# ambient_generic

[TWHL](https://twhl.info/wiki/page/ambient_generic)

### New parameters

* ![](/images/spirit.png) New parameters `Entity to play from` and `Channel to use for that entity`. These allow to play a sound on the channel of another entity.

{{% hint warning %}}
The specified `Entity to play from` can override the played sound through its own behavior if they decide to play something else on the selected channel. On entities that can produce sounds (like monsters, doors, trains, player, etc.) prefer using [ambient_extraspeaker]({{< ref ambient_extraspeaker >}}) instead.
{{% /hint %}}

{{% hint warning %}}
If `ambient_generic` plays a looped sound on another entity, the channel *must* be Static.
{{% /hint %}}

{{% hint info %}}
If you want to play a sound on another entity (including the player) consider using [ambient_extraspeaker]({{< ref ambient_extraspeaker >}}) instead.
{{% /hint %}}

{{% hint info %}}
You can play the sentence through the `ambient_generic` if you specify the sentence name with a leading `!` character instead of the wav file.
The [subtitles]({{< ref subtitles >}}) for the sentence are also shown in this case (if the `Not Toggled` spawnflag is enabled).
{{% /hint %}}
