---
bookHidden: true
bookToC: false
---

# monster_civilian

Generic npc that acts like scientist but doesn't have an ability to heal the player. Requires configuration of model, body, skin and Speech. By default uses the scientist model and sentences. Suitable for making civilian npcs like construction workers, hostages from Counter Strike and unarmed npcs from Condition Zero Deleted Scenes.

### Skill variables

* **sk_civilian_health** - monster's health. If not defined, **sk_scientist_health** is used.

### Default classification

`Human Passive`

### Default display name

`Civilian`

### Soundscripts

* **Civilian.Pain** - pain sound. Derived from **Scientist.Pain**
* **Civilian.Die** - death sound. Derived from **Scientist.Die**

### Entity template examples

{{% tabs %}}

{{% tab "Female civilian" %}}
Female civilian NPC with model from Condition Zero: Deleted Scenes and pain and death voices of Colette from Half-Life: Decay. Uses sentences with prefix `FEM` instead of `SC`.

```json
{
    "female_civ": {
        "own_visual": {
            "model": "models/us_gal.mdl"
        },
        "soundscripts": {
            "Civilian.Pain": {
                "waves": ["colette/colette_pain0.wav", "colette/colette_pain1.wav", "colette/colette_pain2.wav", "colette/colette_pain3.wav"]
            },
            "Civilian.Die": {
                "waves": ["colette/colette_die0.wav", "colette/colette_die1.wav", "colette/colette_die2.wav"]
            }
        },
        "speech_prefix": "FEM"
    }
}
```
{{% /tab %}}

{{% /tabs %}}
