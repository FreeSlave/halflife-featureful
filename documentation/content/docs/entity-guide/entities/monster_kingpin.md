---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) monster_kingpin

Kingpin's behavior is based on one from Sven Co-op (the compatible model is expected), but with its own twists, designed for Field Intensity.

* Like in Sven Co-op the Kingpin can launch [homing plasma projectiles]({{< ref kingpin_plasma_ball >}}) at its enemies.
* Instead of the lightning attack (which looked pretty unfair in Sven Co-op as it didn't require any prior charge and could deal damage on great distances) the Kingpin got a [plasma cluster]({{< ref kingpin_plasma_cluster >}}) attack. The original lightning attack from Sven Co-op may be added later.
* The Kingpin can teleport, but only to designated spots. This gives the mapper better control on where the Kingpin can teleport to. Free teleport will be implemented later.
* The Kingpin can't deflect projectiles yet. This will be implemented later.

### Parameters

* `Teleport target` - the name of the teleport spot. Usually an `info_target` is used as a teleport spot. If there're multiple targets with such name the Kingpin chooses the closest one but only if it's not too close to the current Kingpin's position (it would look silly if Kingpin used teleport just to travel distance of a few steps). After teleportation the Kingpin will face the same direction as the spot entity.
* `Can use second chance` - whether Kingpin can use a special teleport when he's threatened (the health is lower than half and the shield is lower than one quarter) to restore its shield to the fullest.

### Spawnflags

* `Escape on death` - escape away via teleport instead of gibbing. Still counts as death for trigger condition. This can be used to denote that it's not done with a Kingpin yet and it will reappear later in the campaign. Escape uses the same animation as death, but accompanied by different visual effects and doesn't spawn gibs.

### Skill variables

* **sk_kingpin_health** - monster's health.
* **sk_kingpin_shield** - maximum shiled health. This is also the amount of extra resource that's converted to the shield health when it regenerates.
* **sk_kingpin_melee** - melee damage.
* See [kingpin_plasma_ball]({{< ref kingpin_plasma_ball >}}) and [kingpin_plasma_cluster]({{< ref kingpin_plasma_cluster >}}) for projectile skill variables.

### Default classification

`Gargantua`

### Default display name

`Kingpin`

### Soundscripts

* **Kingpin.Idle** - idle sounds.
* **Kingpin.Alert** - alert sounds.
* **Kingpin.Pain** - pain sounds.
* **Kingpin.Die** - death sounds.
* **Kingpin.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Kingpin.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **Kingpin.PlasmaClusterAttack** - Kingpin creates a plasma cluster.
* **Kingpin.TeleportEnter** - Kingpin starts teleporting. The sound is played on the teleport sprite.
* **Kingpin.TeleportExit** - Kingpin finished teleporting. The sound is played on the exit teleport sprite, at the new spot.
* **Kingpin.Escape** - Kingpin starts escaping (when `Escape on death` spawnflag is set). This is played on the sprite.
* **Kingpin.EscapeEnd** - Kingpin finishes its escape. This is played on the sprite.
* See [kingpin_plasma_ball]({{< ref kingpin_plasma_ball >}}) and [kingpin_plasma_cluster]({{< ref kingpin_plasma_cluster >}}) for projectile soundscripts.

### Visuals

* **Kingpin.Shield** - the glow shell to display when the Kingpin takes damage while still having a shield.
* **Kingpin.ShieldDebris** - the sprite that is getting shot out when the Kingpin takes directional damage while still having a shield. The actual size scales depending on the damage. The maximum is 3 times larger than the scale defined in the visual.
* **Kingpin.Glow** - the eye glow sprite. Kingpin has 4 eyes attached to different attachment points.
* **Kingpin.PlasmaClusterBeam** - beam from one of the eyes to the point where the plasma cluster is created.
* **Kingpin.TeleportEnter** - enter teleport sprite. Derived from **Kingpin.Teleport**
* **Kingpin.TeleportExit** - exit teleport sprite. This is the same as **Kingpin.TeleportEnter** by default. Derived from **Kingpin.Teleport**
* **Kingpin.TeleportTrailBeam** - the beam trail from the old position to the new one. This tells the player where the Kingpin has teleported to.
* **Kingpin.DeathBeam** - chaotic beams created on Kingpin's death.
* **Kingpin.DeathEndBeam** - chaotic beams created in the end of Kingpin's death.
* **Kingpin.Escape** - escape start sprite (when `Escape on death` spawnflag is set).
* **Kingpin.EscapeEnd** - escape end sprite.
* **Kingpin.EscapeBeam** - chaotic beams created at the start of Kingpin's escape.
* **Kingpin.EscapeEndBeam** - chaotic beams created in the end of Kingpin's escape.
* **Kingpin.EscapeWave** - beam wave played when Kingpin escapes.
* See [kingpin_plasma_ball]({{< ref kingpin_plasma_ball >}}) and [kingpin_plasma_cluster]({{< ref kingpin_plasma_cluster >}}) for projectile visuals.

Shared visuals:

* **Kingpin.Teleport** - common base for **Kingpin.TeleportEnter** and **Kingpin.TeleportExit**.

### Animation events

* `1` - trace hull attack with left arm. Deals **sk_kingpin_melee** damage. Plays **Kingpin.AttackHit** or **Kingpin.AttackMiss**.
* `2` - trace hull attack with right arm. Deals **sk_kingpin_melee** damage. Plays **Kingpin.AttackHit** or **Kingpin.AttackMiss**.
* `3` - create a plasma ball projectile.
* `4` - launch a plasma ball projectile created at event `3`.
