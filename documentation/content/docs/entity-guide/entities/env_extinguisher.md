---
bookHidden: true
bookToC: false
---

# env_extinguisher

When fired, entity creates three sprites and plays a looped sound, mimicking the Gearbox implementation of fire extinguisher effect on their maps.

The entity was made for Field Intensity to ease and unify the creation of extinguisher effect. Backported to Featureful SDK.

### Parameters

* `Target on start/end` - the entity fires this target two times: when the entity is activated and when it's drained. In former case it sens `On` input and in the latter it send `Off`. Usually you want the target to be the [trigger_hurt]({{< ref trigger_hurt >}}) to deal a frost damage in the area for the duration of the effect.

### Spawnflags

* **Repeatable** - whether the entity can be used multiple times. If not ticked, the entity gets automatically removed when drained.

### Soundscripts

* **Extinguisher.Start** - starting sound (explosion sound by default).
* **Extinguisher.Sizzle** - the sound of "gas" coming out of extinguisher. It automatically stops when the effect ends.

### Visuals

* **Extinguisher.Sprite** - extinguisher effect sprite.
