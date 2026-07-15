---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/alpha.png) item_radiation

Radiation canister (or antirad) is a pickable item that is used automatically upon the player getting the [time based damage]({{< ref "time-based-damage" >}}) effect from radiation and also gives temporary immunity against the subsequent time based damage (but doesn't protect from the impact damage).

The limit on the number of radiation canisters the player can carry is configured in [templates/inventory.json]({{< ref "player-inventory" >}}). By default there's no limit.

{{% hint warning %}}
The radiation damage doesn't apply the time based damage by default. To make this item useful you'll need to enable the [time based damage]({{< ref "time-based-damage">}}) for radiation.
{{% /hint %}}

{{% hint info %}}
When `item_radiation` is picked up the suit doesn't play any sentence by default. This can be configured via the [entity templates]({{< ref "entity-templates/#pickup" >}}).

When `item_radiation` is getting used the suit plays the `!HEV_HEAL5` sentence.
{{% /hint %}}

See also: [item_antidote]({{< ref item_antidote >}})

* If `item_radiation` is defined in **hud.txt** it will be used to show the radiation canister in pickup history.
* If `item_radiation` is defined in **hud.txt** the sprite and the current count will be shown in HUD, among [player inventory items]({{< ref "player-inventory" >}}).

### Skill variables

* **sk_antirad_time** - how long the player will be protected from getting the time based effect from the subsequent radiation attacks after the radiation canister has been applied. This is 10 seconds by default.

### Soundscripts

* **Antirad.Pickup** - the radiation canister is getting picked up.
* [Player.AntiradUse]({{< ref "player/#soundscripts" >}})
