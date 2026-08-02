---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# grenaderound

Grenades launched by [weapon_grenadelauncher]({{< ref weapon_grenadelauncher >}}). Such grenades bounce off walls but explode on contact with a monster. They automatically explode in 2.5 seconds after being launched.

This uses the model **models/pipebomb.mdl** from Team Fortress Classic with `skin` set to 1.

### Skill variables

* **sk_plr_grenade** - explosion damage. If not defined, **sk_plr_hand_grenade** is used.

### Soundscripts

* **Grenade.Bounce** - bounce sound. The default definition is the same as of **HandGrenade.Bounce**.

### Visuals

* **Grenade.Trail** - trail beam visual.
