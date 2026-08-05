---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# nail

### Skill variables

* **sk_nail** - nail damage. Default value is 8.
* **sk_plr_nail** - nail damage dealt by player's nailgun. Default value is 8.

### Projectile properties

Speed: 1000

#### Direct damage

* Damage: **sk_nail** or **sk_plr_nail** (if fired by player)
* Type: generic
* Gib: never

### Soundscripts

* **Nail.HitBody** - plays with 50% chance when hitting something that can take damage.
* **Nail.HitWall** - plays with 50% chance when hitting something that can't take damage.

### Visuals

* **Nail.Model** - nail itsef.
