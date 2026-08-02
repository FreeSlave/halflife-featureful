---
bookHidden: true
bookToC: false
---

# monster_floater

An alien creature cut from Half-Life with no clearly defined canonical behavior.
The current implementation is taken from Field Intensity - floaters are suicidal flyers who come closer to enemies and explode. Take the model from Field Intensity to enable glowing sprites attached to the floater (with the standard model from Half-Life glowing sprites won't spawn as they require the attachment points in the model).

{{% hint info %}}
Original Half-Life also has the entity `monster_bloater` which uses the floater model, but doesn't have an AI. This entity is left as is in Featureful SDK.
{{% /hint %}}

### Skill variables

* **sk_floater_health** - monster's health.
* **sk_floater_explode** - the explosion damage.
* **sk_floater_basespeed** - base speed. Default value is 100.
* **sk_floater_extraspeed** - maximum extra speed bonus the float gains when it's bloating (moving to the enemy in order to explode nearby). Default value is 400.
* **sk_floater_bloat_time** - the time (in seconds) before floater explodes. Default value is 2.1.
* **sk_floater_bloat_distance** - the distance at which the floater starts bloating. Default value is 128.

### Default classification

`Alien Monster`

### Default display name

`Floater`

### Soundscripts

* **Floater.Idle** - idle sounds. Empty by default.
* **Floater.Alert** - alert sounds. Empty by default.
* **Floater.Pain** - pain sounds.
* **Floater.Howl** - howling sound (played when floaters move).
* **Floater.Bloat** - start bloating.
* **Floater.Explode** - explosion sound.

{{% hint info %}}
Sound files are not included in the sample mod. Ideally you should provide your own sounds.
{{% /hint %}}

### Visuals

* **Floater.Blow** - the sprite effect when the floater explodes.
* **Floater.BlowAlt** - alternative sprite effect when the floater explodes. The effect is randomized between this and  **Floater.Blow**.
* **Floater.BlowSpray** - sprites sprayed out when the floater explodes.
* **Floater.BlowLight** - the dynamic light produced when the floater explodes.
* **Floater.Glow** - glowing sprite to use as a pair of sprites attached to the floater (when using a model with attachments).
* **Floater.GlowUnprovoked** - glowing sprite when floater is set to Ignore Player and hasn't been provoked yet.
* **Floater.GlowBloating** - glowing sprite when floater starts bloating.
