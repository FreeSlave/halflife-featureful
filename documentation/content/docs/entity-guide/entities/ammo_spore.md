---
bookHidden: true
bookToC: false
entityCategory: object
---

# ammo_spore

Spore ammo. Can be placed on walls and ceiling (but the angle must be adjusted for proper placement).

### Soundscripts

* **Spore.Ammo** - when player takes a spore.
* **Spore.AmmoPuke** - plays when the `ammo_spore` takes damage and pukes the spore. Empty by default.

Enable puke sound like in Sven Co-op via [soundscripts]({{< ref soundscripts >}}):

```json
{
    "Spore.AmmoPuke": {
        "waves": ["weapons/splauncher_altfire.wav"],
        "pitch": [85, 116]
    }
}
```

### Visuals

* **Spore.AmmoSprite** - temporary sprite to show in front of `ammo_spore` then it takes damage and pukes the spore. This uses **sprites/spore_exp_c_01.spr** by default. This is the Featureful SDK addition - no effect was playing in Opposing Force.
