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

{{% tab "Ott" %}}
Otis'es evil twin from Sven Co-op.
```json
{
    "barnabus": {
        "own_visual": {
            "model": "models/otisf.mdl"
        },
        "classify": "Human Military"
    }
}
```
{{% /tab %}}

{{% /tabs %}}
