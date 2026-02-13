---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_otis

### Skill variables

* **sk_otis_health** - monster's health. If not defined, **sk_barney_health** is used.
* **sk_357_bullet** - Desert Eagle bullet damage. Default value is 34.

### Default classification

`Player Ally`

### Default display name

`Security Guard`

### Soundscripts

* **Otis.Pain** - pain sound. Derived from **Barney.Pain**
* **Otis.Die** - death sound. Derived from **Barney.Die**
* **Otis.FireDesertEagle** - firing Desert Eagle. Derived from **NPC.DesertEagle**

### Notes

{{% hint info %}}
The Otis model from Opposing Force doesn't have a submodel with gun gone, unlike Barney. It means that when Otis drops the gun on his death the 'gun holstered' submodel is set on the model, i.e. the gun is still visible holstered on the dead body.

To address this issue Featureful SDK implements a check for the presence of submodel 4 on the gun bodygroup. While submodel 1 is 'gun holstered', submodel 2 is 'gun drawn' and submodel 3 is 'donut', the submodel 4 is supposed to be 'no gun'. If the submodel 4 exists it's getting set on the Otis model when he drops the gun. If there're less than 4 submodels on the gun bodygroup it still uses the old behavior (setting 'gun holstered' submodel) to maintain backward compatibility with the default model.
{{% /hint %}}

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
