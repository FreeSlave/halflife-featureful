---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# kingpin_plasma_cluster

A special object created by [monster_kingpin]({{< ref monster_kingpin >}}). This starts as a sprite that later bursts into 12 particles. The particles first disperse around and then slowly fall to the ground, being connected to the leftover flare via beams. The beams deal damage. Gameplay-wise it acts as an area denial attack which forces the player to change their position to avoid the massive damage.

### Skill variables

* **sk_kingpin_lightning** - base damage for the beam. The damage decreases as beams fade away and the particles fall down.

### Soundscripts

* **Kingpin.PlasmaCluster.Start** - play on spawn.
* **Kingpin.PlasmaCluster.Burst** - burst into particles.
* **Kingpin.PlasmaCluster.Zap** - play as the plasma cluster is falling anb when its beams deal damage to something.

### Visuals

* **Kingpin.PlasmaCluster.Sprite** - sprite to set on spawn. This must be animated. After the animation ends the cluster bursts into particles (so the delay dependson the framerate and the number of frames). The alpha is an initial sprite brightness that increases while the animation is played.
* **Kingpin.PlasmaCluster.Leftover** - post-burst leftover sprite.
* **Kingpin.PlasmaCluster.EntLight** - entity light created on burst and after the particles stop dispering and start falling.
* **Kingpin.PlasmaCluster.Particle** - particle sprite.
* **Kingpin.PlasmaCluster.ParticleAlt** - alternative particle sprite with a different color. Derived from **Kingpin.PlasmaCluster.Particle**
* **Kingpin.PlasmaCluster.Beam** - the beam that connects the particle to the sprite leftover.
* **Kingpin.PlasmaCluster.BeamAlt** - alternative beam with a different color. Derived from **Kingpin.PlasmaCluster.Beam**
