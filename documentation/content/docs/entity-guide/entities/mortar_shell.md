---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# mortar_shell

A projectile fired by [op4mortar]({{< ref op4mortar >}})

### Changes

* Mortar shells in Opposing Force don't produce AI danger sounds, so monsters don't react to them. Featureful SDK adds the danger sounds during the mortar shell flight.

### Skill variables

* **sk_op4mortar** - the mortar shell explosion damage. If not set, the value of **sk_plr_rpg** multiplied by 2 is used.

### Projectile properties

Speed: 600

#### Radius damage

* Damage: **sk_op4mortar**
* Type: blast
* Radius: depends on damage

### Soundscripts

* **Op4Mortar.Fly** - the projectile flight sound.

### Visuals

* **Op4Mortar.Trail** - the mortar projectile trail sprite.
