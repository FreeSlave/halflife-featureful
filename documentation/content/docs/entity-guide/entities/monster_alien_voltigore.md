---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_alien_voltigore

### Changes

* Restores health when eating.
* Does not stop range attack when enemy hides behind a cover to prevent abusing the interruption of long animation.

### Skill variables

* **sk_voltigore_health** - monster's health.
* **sk_voltigore_dmg_punch** - melee damage.
* **sk_voltigore_dmg_beam** - damage dealth by charged bolt.
* **sk_voltigore_dmg_explode** - explosion damage. If not defined, **sk_voltigore_dmg_beam** is used.

### Default classification

`Race X Shock`

### Soundscripts

* **Voltigore.Idle** - idle sounds.
* **Voltigore.Alert** - alert sounds.
* **Voltigore.Pain** - pain sounds.
* **Voltigore.Die** - death sounds.
* **Voltigore.Footstep** - footstep sounds.
* **Voltigore.BeamAttack** - starting beam attack.
* **Voltigore.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Voltigore.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**

### Visuals

* **Voltigore.ChargeBeam** - beams played while preparing the range attack.
* **Voltigore.DeathBeam** - beams played on death.

Shared visuals:

* **Voltigore.Beam** - sprite, color and alpha properties shared by **Voltigore.ChargeBeam** and **Voltigore.DeathBeam**.

Bolt projectile visuals:

* **ChargedBolt.Sprite** - the projectile sprite.
* **ChargedBolt.Beam** - the beam accompanying the projectile. This should match **Voltigore.ChargeBeam** for consistent looks.
* **ChargedBolt.HitBeam** - the accompanying beam when it hits something capable of getting damage. This uses a slightly different color. Should be kept in match with **ChargedBolt.Beam** for consistent looks.

### Entity template examples

{{% tabs %}}

{{% tab "Smaller size" %}}
Opposing Force voltigore geometry is kinda big which makes it harder for voltigores to navigate on the levels. This template makes them less in size (this doesn't affect the model scale, just the size of the boundbox).
```json
{
    "monster_alien_voltigore": {
        "size": {
            "mins": [-40, -40, 0],
            "maxs": [40, 40, 85]
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
