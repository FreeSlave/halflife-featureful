---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# squidtoxicspit

A poisonous projectile fired by [monster_bullchicken]({{< ref monster_bullchicken >}})

### Skill variables

* **sk_bullsquid_dmg_toxic_poison** - the poison damage of the toxic spit. By default it's **sk_bullsquid_dmg_spit** multiplied by 0.25.
* **sk_bullsquid_dmg_toxic_impact** - the impact damage of the toxic spit. By default it's **sk_bullsquid_dmg_spit** multiplied by 1.5.

### Projectile properties

Speed: 600

#### Direct damage

* Damage: **sk_bullsquid_dmg_toxic_impact**
* Type: acid

#### Aura damage

* Damage: **sk_bullsquid_dmg_toxic_poison**
* Type: poison
* Radius: 32
* Traits: timed damage is non lethal; ignores armor
