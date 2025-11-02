---
bookHidden: true
bookToC: false
---

# env_shooter

[TWHL](https://twhl.info/wiki/page/env_shooter)

`env_shooter` is capable of shooting two types of objects - gibs and shots. The latter is an addition from *Spirit of Half-Life* and usually is used with sprites (although not necessary), while the former is designed to work with models.

{{% hint info %}}
Set the `Delay between shots` to negative value to make `env_shooter` produce the specified number of projectiles on the same frame.
{{% /hint %}}

{{% hint info %}}
To set a sprite you may need to specify the path manually instead of relying on the open file dialog in the level editor.
{{% /hint %}}

### New parameters

* ![](/images/spirit.png) `Gib/Shot physics` - define the physics of the projectile. Also this determines whether `env_shooter` shots *gibs* or *shots*.
* ![](/images/spirit.png) `Start frame (shot)` - start sprite frame. Only for *shots*.
* ![](/images/spirit.png) `Framerate (shot)` - sprite framerate. Only for *shots*. Set to 10 or other value for animated sprites.
* ![](/images/spirit.png) `Bounce friction (shot)` - friction when the projectile touches the ground. Only for *shots*.
* ![](/images/spirit.png) `Fire on collision (shot) (locus = shot)` - a target to fire on collision, with *shot* acting as an activator.
* ![](/images/spirit.png) `Fire on collision (shot) (locus = wall)` - a targer to fire on collision, with the collided entity acting as an activator.
* ![](/images/spirit.png) `Shot size (X Y Z)` - the *shot* size. Must be 3 positive numbers.

### New spawnflags

* `Scale models` - whether scaling is applied to the models. The reason why this behavior is not enabled by default is that some env_shooters in Half-Life set big scales, but model scaling is not implemented in Half-Life.
* `Don't wait till land`. By default gibs won't start their lifetime countdown until they land on the ground. Tick this flag to start countdown right away, without waiting for landing.
* `Start instantly` - start producing projectile on the same frame it was called. By default in Half-Life it starts producing gibs/shots only on the next frame which may cause some problems.

### Soundscripts

When the spawned gib touches something (depending on the material):

* **Gib.Concrete** - concrete. Derived from **Breakable.Concrete**
* **Gib.Flesh** - flesh. Derived from **Breakable.Flesh**
* **Gib.Glass** - glass. Derived from **Breakable.Glass**
* **Gib.Metal** - metal. Unlike others this is not derived from the **Breakable** soundscript because in Half-Life gibs and breakables use slightly different sets of sounds for the metal material.
* **Gib.Wood** - wood. Derived from **Breakable.Wood**

{{% hint info %}}
**Breakable** soundscripts are described on [func_breakable]({{< ref func_breakable >}}).
{{% /hint %}}
