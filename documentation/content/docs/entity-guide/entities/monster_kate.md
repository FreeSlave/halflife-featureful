---
bookHidden: true
bookToC: false
---

# monster_kate

Kate - a companion from Azure Sheep. Acts like [monster_barney]({{< ref monster_barney >}}), but also can perform kick attack.

Note: you'll need to add sentences with `KA_` prefix to your `sentences.txt` and put corresponding sounds to your mod directory.

### Skill variables

* **sk_kate_health** - monster's health.
* **sk_9mm_bullet** - handgun bullet damage.
* **sk_hgrunt_kick** - kick damage.

### Default classification

`Player Ally`

### Soundscripts

* **Kate.Pain** - pain sound.
* **Kate.Die** - death sound.
* **Kate.FirePistol** - firing a handgun.

### Attacks

* *Melee Attack 1* - melee (kick).
* *Range Attack 1* - fire a gun.

### Animation events

* `6` - kick trace hull attack. Deals **sk_hgrunt_kick** damage.
