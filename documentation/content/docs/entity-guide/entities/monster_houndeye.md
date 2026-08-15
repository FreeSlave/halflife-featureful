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
* **sk_houndeye_squad_bonus_factor** - bonus factor for the additional shockwave damage when houndeye is in squad. The additional damage is calculated as `sk_houndeye_dmg_blast * sk_houndeye_squad_bonus_factor * (squad_size - 1)`. Default value is 1.1.

### Default classification

`Alien Monster`

### Default display name

`Houndeye`

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
* **HoundEye.Use** - start following the ally player. Derived from **HoundEye.Idle**
* **HoundEye.UnUse** - stop following the ally player. Derived from **HoundEye.Alert**

### Visuals

* **Houndeye.Wave1** - wave visual used when houndeye is not in squad.
* **Houndeye.Wave2** - wave visual used when houndeye is in squad of 2.
* **Houndeye.Wave3** - wave visual used when houndeye is in squad of 3.
* **Houndeye.Wave4** - wave visual used when houndeye is in squad of 4 or more.

Shared visuals:

* **Houndeye.WaveBase** - sprite, life, beam parameters and alpha used by **Houndeye.Wave1**, **Houndeye.Wave2**, **Houndeye.Wave3** and **Houndeye.Wave4**.

Note: when customizing the wave color you usually want to configure it for each wave, for the consistent looks.

### Attacks

* *Range Attack 1* - sonic wave.

### Animation events

* `1` - play **HoundEye.Warn** soundscript.
* `2` - play **HoundEye.Warmup** soundscript.
* `3` - sonic wave attack. Plays **HoundEye.Sonic** soundscript.
* `4` - play **HoundEye.Anger1** soundscript.
* `5` - play **HoundEye.Anger2** soundscript.
* `6` - jump back.
* `7` - set model skin to value 2 if blinking is allowed at the moment.

### Entity template examples

{{% tabs %}}

{{% tab "Jumping" %}}

Let houndeyes jump freely using the [jumping]({{< ref "entity-templates/#jumping" >}}) property.

```json
{
    "monster_houndeye": {
        "jumping": {
            "ability": true,
            "start_sequence": "madidle2",
            "start_frame_fraction": 0.1
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}

