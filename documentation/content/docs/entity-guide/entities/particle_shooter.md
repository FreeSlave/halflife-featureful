---
bookHidden: true
bookToC: false
---

# particle_shooter

Emits particles drawn via TriAPI (so they don't occupy the temporary entity slots). This can be used to create a constant source of smoke/steam/etc. Designed after the Day Of Defeat entity of the same name. `Shoot particles at` or `Shooter direction` must be defined in order for this entity to work.

See also: [env_smoker]({{< ref env_smoker >}}).

### Parameters

* `Shoot particles at [LP]` - emit particles in the direction towards this position.
* `Shooter direction [LV]` - emit particles in the specified direction. This option has higher preference than `Shoot particles at`.
* `Position to shoot from (blank = here) [LP]` - use this position to emit particles from.
* `Number of Particles` - number of particles to emit before deactivation. Set to the negative value to make `particle_shooter` emit particles indefinitely (or before the manual deactivation).
* `Delay between shots` - delay between particle emissions.
* `Particle life` - the life duration of a particle.
* `Particle Speed` - the particle movement speed.
* `Velocity Variance` - the direction variance relative to the base direction, in degrees.
* `Particle size` - particle size, in units (as a side of a square).
* `Fade Speed` - how fast the render amount fades. This value multiplied by 30 is substructed by the render amount, per second.
* `Scale Speed` - how fast the sprite expands or shrinks. This value multiplied by 30 is added to the initial size (in units), per second. Use negative values for shrinking.

### Spawnflags

* `Repeatable` - whether the `particle_shooter` can be activated again after it runs out of particles
* `Spiral` - make the particle move in spiral.
* `Collide with world` - whether particles collide with world brushes, decreasing the velocity on collision.
* `Animate` - whether the particle sprites should be animated.
* `Start on` - whether the entity should start emitting particles at start of the map.
* `Kill on collide` - whether the particles should be removed on collision with something.

### Reaction to use-type

Starts or stops emitting particles depending on the use input. If `particle_shooter` has been previously deactivated before exhausting the `Number of Particles` value, it will emit the remaining particles.
