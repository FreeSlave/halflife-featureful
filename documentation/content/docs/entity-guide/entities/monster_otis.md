---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_otis

### Skill variables

* **sk_otis_health** - monster's health. If not defined, **sk_barney_health** is used.
* **sk_357_bullet** - Desert Eagle bullet damage.

### Default classification

`Player Ally`

### Soundscripts

* **Otis.Pain** - pain sound. Derived from **Barney.Pain**
* **Otis.Die** - death sound. Derived from **Barney.Die**
* **Otis.FireDesertEagle** - firing Desert Eagle. Derived from **NPC.DesertEagle**

### Entity template examples

{{% tabs %}}

{{% tab "Otto" %}}
Otis's evil twin from Sven Co-op.
```json
{
    "otto": {
        "own_visual": {
            "model": "models/otisf.mdl"
        },
        "classify": "Human Military"
    }
}
```
{{% /tab %}}

{{% tab "Trace Attack rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_otis": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": ["chest", "stomach"],
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "*0.5"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
