---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/alpha.png) item_adrenaline

Adrenaline canister/syringe is a pickable item that is used automatically to revive the player in 3 seconds after his death.

The limit on the number of adrenalines the player can carry is configured in [templates/inventory.json]({{< ref "player-inventory" >}}). By default there's no limit.

{{% hint info %}}
Adrenaline can't revive the player if he drowned, died from the fall damage or was gibbed.
{{% /hint %}}

{{% hint info %}}
When `item_adrenaline` is picked up the suit doesn't play any sentence by default. This can be configured via the [entity templates]({{< ref "entity-templates/#pickup" >}}).

When `item_adrenaline` is getting used the suit plays the `!HEV_HEAL9` sentence. This sentence is not defined in Half-Life **sentences.txt**. We recommend adding this line after the `HEV_HEAL8`:

```
HEV_HEAL9 fvox/(p140) boop, boop, boop, (p100) hiss, adrenaline_shot
```
{{% /hint %}}

* If `item_adrenaline` is defined in **hud.txt** it will be used to show in pickup history.
* If `item_adrenaline` is defined in **hud.txt** the sprite and the current count will be shown in HUD, among [player inventory items]({{< ref "player-inventory" >}}).

### Skill variables

* **sk_adrenaline_health** - how much health the player upon adrenaline revival. Default value is 25. This can be higher than the player's maximum health.

### Soundscripts

* **Adrenaline.Pickup** - the adrenaline is getting picked up.
* [Player.AdrenalineUse]({{< ref "player/#soundscripts" >}})

### Entity template examples

{{% tabs %}}

{{% tab "Custom pickup sentence" %}}
```json
{
    "item_adrenaline": {
        "pickup": {
            "suit_sentence": "!HEV_ADRENALINE"
        }
    }
}
```

Possible **sentences.txt** entry:

```
HEV_ADRENALINE fvox/blip blip blip, (p103) adrenaline_shot(e50)(v80) (p100) acquired
```
{{% /tab %}}

{{% /tabs %}}
