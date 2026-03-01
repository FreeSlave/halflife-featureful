---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# kingpin_plasma_ball

A homing projectile launched by [monster_kingpin]({{< ref monster_kingpin >}})

### Skill variables

* **sk_kingpin_plasma_blast** - blast damage.

### Soundscripts

* **Kingpin.PlasmbaBall.Ambience** - the sound played as the plasma ball travels. Must be looped.
* **Kingpin.PlasmbaBall.Impact** - explosion sound.

### Visuals

* **Kingpin.PlasmaBall** - the projectile itself.
* **Kingpin.PlasmaBall.Trail** - beam trail.
* **Kingpin.PlasmaBall.ShockInner** - the inner shockwave created on impact.
* **Kingpin.PlasmaBall.ShockOuter** - the outer shockwave created on impact.
* **Kingpin.PlasmaBall.EntLight** - entity light emitted from the projectile.

Shared visuals:

* **Kingpin.PlasmaBall.Shock** - shared by **Kingpin.PlasmaBall.ShockInner** and **Kingpin.PlasmaBall.ShockOuter**
