---
bookHidden: true
bookToC: false
---

# ambient_generic

[TWHL](https://twhl.info/wiki/page/ambient_generic)

### New parameters

* ![](/images/spirit.png) New parameters `Entity to play from` and `Channel to use for that entity`. These allow to play a sound on the channel of another entity.

{{% hint warning %}}
The specified `Entity to play from` can override the played sound through its own behavior if they decide to play something else on the selected channel.
{{% /hint %}}

{{% hint warning %}}
If `ambient_generic` plays a looped sound on another entity, the channel *must* be Static.
{{% /hint %}}
