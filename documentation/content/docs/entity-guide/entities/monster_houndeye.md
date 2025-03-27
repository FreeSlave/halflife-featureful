---
bookHidden: true
bookToC: false
---

# monster_houndeye

[TWHL](https://twhl.info/wiki/page/monster_houndeye)

### Changes

* Squad leader can play `leaderlook` animation (a feature missing in Half-Life).
* Shut their eyes when sleeping.
* Like a bullsquid, seeks for something tasty to eat.
* Restores health when eating.

### New spawnflags

* `Start sleeping` - put houndeyes to sleep at the level start without a need for scripted_sequence.

### Skill variables

* **sk_houndeye_health** - monster's health.
* **sk_houndeye_dmg_blast** - base shockwave damage. Gets factored by the number of houndeyes in the squad.

### Default classification

`Alien Monster`

### Soundscripts

* **HoundEye.Idle** - idle sounds.
* **HoundEye.Alert** - alert sounds.
* **HoundEye.Pain** - pain sounds.
* **HoundEye.Die** - death sounds.
* **HoundEye.Warn** - hunt/warn sounds.
* **HoundEye.Warmup** - start attack.
* **HoundEye.Sonic** - sonic blast.
* **HoundEye.Anger1** - played on event 4 in the `madidle` sequence.
* **HoundEye.Anger2** - played on event 5 in the `madidle` sequence.

### Visuals

* **Houndeye.Wave1** - wave visual used when houndeye is not in squad.
* **Houndeye.Wave2** - wave visual used when houndeye is in squad of 2.
* **Houndeye.Wave3** - wave visual used when houndeye is in squad of 3.
* **Houndeye.Wave4** - wave visual used when houndeye is in squad of 4 or more.

Shared visuals:

* **Houndeye.WaveBase** - sprite, life, beam parameters and alpha used by **Houndeye.Wave1**, **Houndeye.Wave2**, **Houndeye.Wave3** and **Houndeye.Wave4**.

Note: when customizing the wave color you usually want to configure it for each wave, for the consistent looks.
