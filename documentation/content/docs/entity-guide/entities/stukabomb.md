---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# stukabomb

A poisonous projectile fired by [monster_stukabat]({{< ref monster_stukabat >}}). It obeys the gravity, so it will travel in arch. When it hits something it does damage in a small radius.

By default the projectile uses the **models/spit.mdl** as its model - this model was supposed to be used for the bullsquid's projectile (but was changed to the sprite in retail) and was left in the game files. The stukabat's animations suppose the stukabat were to use some kinda of bombs, but no bomb model was preserved. Thrill and Reprocessing mods use the **models/spit.mdl** instead, and so does the Featureful SDK.

### Skill variables

* **sk_stuka_dmg_bomb** - spit damage.

### Projectile properties

Speed: 900

#### Radius damage

* Damage: **sk_stuka_dmg_bomb**
* Type: poison
* Radius: 64

### Soundscripts

* **Stukabat.BombTouch** - one of the sounds played when the spit projectile hits something. Derived from **NPC.SpitTouch**
* **Stukabat.BombHit** - one of the sounds played when the spit projectile hits something. Derived from **NPC.SpitHit**

### Visuals

* **Stukabat.Bomb** - the projectile itself.
