---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# crossbow_bolt

### Skill variables

* **sk_plr_xbow_bolt_monster** - damage dealt to the monster on bolt hit. Explosive bolts additionally deal **sk_plr_xbow_bolt_explo** damage.
* **sk_plr_xbow_bolt_client** - direct hit damage to the player. Explosive bolts additionally deal **sk_plr_xbow_bolt_explo** damage.
* **sk_plr_xbow_bolt_explo** - bolt explosion damage (used by `"crossbow_bolt explosive"` projectile variant, e.g. in multiplayer). This is 40 by default.
* **sk_plr_xbow_bolt_hitscan** - hitscan attack damage in multiplayer (when zoomed). This is 120 by default.

### Projectile properties

Speed: 2000 (1000 if fired underwater)

#### Direct damage

* Damage: **sk_plr_xbow_bolt_monster** (**sk_plr_xbow_bolt_client** when hits player)
* Type: bullet
* Gib: never

#### Radius damage

{{% hint %}}
Used only with `crossbow_bolt explosive` variant.
{{% /hint %}}

* Damage: **sk_plr_xbow_bolt_explo**
* Type: blast
* Radius: 128
* Gib: always

### Soundscripts

* **Crossbow.BoltHitBody** - hitting something that can take damage.
* **Crossbow.BoltHitWorld** - hitting something that can't take damage.
