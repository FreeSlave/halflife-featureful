---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# rpg_rocket

The rocket entity fired by [weapon_rpg]({{< ref weapon_rpg >}}) or by `func_tankrocket`.

### Skill variables

* **sk_plr_rpg** - rocket explosion damage.

### Projectile properties

Initial speed: 250. Accelerate to 2000 as it travels

Effect flags: `rocketflare`

#### Radius damage

* Damage: **sk_plr_rpg**
* Type: blast
* Radius: depends on damage

### Soundscripts

* **RPG.RocketIgnite** - rocket travel sound. Must be looped.

### Visuals

* **RPG.Trail** - trailing beam.
