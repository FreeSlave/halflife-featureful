---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# hornet

### Skill variables

* **sk_hornet_dmg** - hornet damage.
* **sk_plr_hornet_dmg** - hornet damage dealt by player's hornetgun. Default value is 7.

### Soundscripts

* **Hornet.Buzz** - plays occasionally when hornet is tracking the enemy.
* **Hornet.Die** - when hornet is hitting the enemy.

### Visuals

* **Hornet.Model** - hornet itself.
* **Hornet.Trail** - trail beam of the projectile (red by default).
* **Hornet.TrailAlt** - trail beam of the alternative a bit faster projectile (orange by default).
* **Hornet.Puff** - sprite that appears sometimes when hornet is close to the enemy.

Shared visuals:

* **Hornet.TrailBase** - sprite, alpha, beam parameters and life used by **Hornet.Trail** and **Hornet.TrailAlt**.

### Entity template examples

{{% tabs %}}

{{% tab "HL Alpha trail colors" %}}
```json
{
    "hornet": {
        "visuals": {
            "Hornet.Trail": {
                "color": [255, 128, 0]
            },
            "Hornet.TrailAlt": {
                "color": [0, 100, 255]
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
