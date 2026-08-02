---
bookHidden: true
bookToC: false
---

# env_energy_ball_trap

The energy ball that plays a looped sound and gets alerted when player is nearby. If player stays close to it for a certain amount of time, the ball fires itself at the player, acting as a [controller's head ball projectile]({{< ref controller_head_ball >}}). After a while the ball respawns at its original location.

The entity was made for Field Intensity and backported to Featureful SDK.

### Parameters

* `Custom detection radius` - custom detection radius, in units.
* `Custom respawn time` - custom time before respawn, in seconds.

### Skill variables

* **sk_controller_dmgzap** - the head energy ball projectile damage (same as for [monster_alien_controller]({{< ref monster_alien_controller >}})).
* **sk_zaptrap_sense_radius** - detection radius, in units.
* **sk_zaptrap_respawn_time** - time before respawn, in seconds.

### Soundscripts

* **ZapTrap.Detect** - a looped sound that starts playing when the player is close. It stops playing when the player is at safe distance.
* **ZapTrap.Launch** - a projectile launch sound.

### Visuals

* **ZapTrap.EnergyBall** - the ball visual. Based on **Controller.HeadBall** from [monster_alien_controller]({{< ref monster_alien_controller >}}).
