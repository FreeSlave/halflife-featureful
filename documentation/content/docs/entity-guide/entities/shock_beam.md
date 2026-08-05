---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# ![](/images/opfor.png) shock_beam

### Skill variables

* **sk_plr_shockroachs** - impact damage in singleplayer.
* **sk_plr_shockroachm** - impact damage in multiplayer.
* **sk_shockroach** - impact damage for Monster entites. If not defined, **sk_plr_shockroachs** is used.

### Projectile properties

Speed: 2000

#### Direct damage

* Damage: **sk_shockroach** or **sk_plr_shockroachs** and **sk_plr_shockroachm** (if fired by player)
* Type: shock

### Soundscripts

* **ShockBeam.Impact** - on impact with something.

### Visuals

* **ShockBeam.Sprite** - the sprite accompanying projectile.
* **ShockBeam.Beam1** - one of the beams attached to projectile.
* **ShockBeam.Beam2** - one of the beams attached to projectile.
* **ShockBeam.Light** - dynamic light created on impact.
* **ShockBeam.Shell** - render properties temporarily applied on the target.
