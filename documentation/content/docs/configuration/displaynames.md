---
title: "Display names"
bookToC: false
---

# {{% param "title" %}}

Entities can have display names. These are shown as part of text information when `sp_allowmonsterinfo` (in singleplayer) or `mp_allowmonsterinfo` (in multiplayer) is enabled.

Most monster classes define a default display name (these are usually documented on the monster's entity page). The default display name can be changed via [entity tempalates]({{< ref "entity-templates/#displayname" >}}). The display name also can be changed for the monster instance via the `In-game Name` entity parameter. The display names are supposed to be defined in English (as the default language) but can be translated (see below).

## Translatable display names

Independently of how the display name is defined (either by the monster class, or by the entity template or by the instance paramater) it can be localized.

**displaynames.json** (located in the addon directory for localization) defines the mapping between the original display names and the translated ones. Example of Russian localization:

```json
{
    "Scientist": "Ученый",
    "Security Guard": "Охранник"
}
```

Users who have the Russian localization installed will see `Ученый` instead of `Scientist` when looking at the scientist.

{{% hint info %}}
The search for translated display names is currently case-insensitive.
{{% /hint %}}

In addition to the display name the info also shows the amount of health with a descriptive string. The latter can be localized as well. See [predefined messages]({{< ref "text-messages/#predefined-messages" >}}).
