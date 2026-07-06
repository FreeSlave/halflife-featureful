---
bookHidden: true
bookToC: false
entityCategory: pickup
---

# ![](/images/alpha.png) item_antidote

[TWHL](https://twhl.info/wiki/page/item_antidote)

Antidote (also known as antitoxin) is a pickable item that is used automatically upon the player getting the [time based damage]({{< ref "time-based-damage" >}}) effect from poison and nervegas and also gives temporary immunity against the subsequent time based damage (but doesn't protect from the impact damage).
The player can carry many antidotes (there's currently no limit on maximum antidotes carried).

{{% hint info %}}
When `item_antidote` is picked up the suit plays the `!HEV_DET4` sentence. This can be configured via the [entity templates]({{< ref "entity-templates/#pickup" >}}).

When `item_antidote` is getting used the suit plays the `!HEV_HEAL4` sentence.
{{% /hint %}}

See also: [item_radiation]({{< ref item_radiation >}})

### Changes

* Stops poison/nervegas time based effect faster.
* If `item_antidote` is defined in **hud.txt** it will be used to show the antidote in pickup history.
* If `item_antidote` is defined in **hud.txt** the antidote sprite along with the current count will be shown in HUD, among [player inventory items]({{< ref "player-inventory" >}}).

### New parameters

* `Pickup sound` - custom pickup sound (by default `item_antidote` will use the **Antidote.Pickup** soundscript).

### Skill variables

* **sk_antidote_time** - how long the player will be protected from getting the time based effect from the subsequent toxic attacks after the antidote has been applied. This is 10 seconds by default.

### Soundscripts

* **Antidote.Pickup** - the antidote is getting picked up.
* [Player.AntidoteUse]({{< ref "player/#soundscripts" >}})

### Entity template examples

{{% tabs %}}

{{% tab "Custom pickup sentence" %}}
```json
{
    "item_antidote": {
        "pickup": {
            "suit_sentence": "!HEV_ANTIDOTE"
        }
    }
}
```

Possible **sentences.txt** entry:

```
HEV_ANTIDOTE fvox/blip blip blip, antitoxin_shot(e50), acquired
```
{{% /tab %}}

{{% /tabs %}}
