---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# charged_bolt

A projectile fired by [monster_alien_voltigore]({{< ref monster_alien_voltigore >}})

### Skill variables

* **sk_voltigore_dmg_beam** - damage dealt by charged bolt.
* **sk_voltigore_dmg_beam_aura** - damage dealt in proximity to charged bolt as it travels.

### Projectile properties

Speed: 1000

#### Direct damage

* Damage: **sk_voltigore_dmg_beam**
* Type: shock

#### Aura damage

* Damage: **sk_voltigore_dmg_beam_aura**
* Type: shock
* Radius: 32

### Visuals

* **ChargedBolt.Sprite** - the projectile sprite.
* **ChargedBolt.Beam** - the beam accompanying the projectile. This should match **Voltigore.ChargeBeam** for consistent looks.
* **ChargedBolt.HitBeam** - the accompanying beam when it hits something capable of getting damage. This uses a slightly different color. Should be kept in match with **ChargedBolt.Beam** for consistent looks.
