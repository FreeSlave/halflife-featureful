---
bookHidden: true
bookToC: false
---

# monster_bullchicken (aka Bullsquid)

[TWHL](https://twhl.info/wiki/page/monster_bullchicken)

### Changes

* Spit projectile precision depends on difficulty level.
* Can spit an alternative projectile - the slow poisonous ball. Depends on **sk_bullsquid_toxicity** cvar.
* Restores health when eating.

### Skill variables

* **sk_bullsquid_health** - monster's health.
* **sk_bullsquid_dmg_bite** - bite damage.
* **sk_bullsquid_dmg_whip** - tail whip damage.
* **sk_bullsquid_dmg_spit** - acid spit damage.
* **sk_bullsquid_toxicity** - whether bullsquids can use toxic spit attack. Default values: 0 on easy, 1 on medium and hard.
* **sk_bullsquid_dmg_toxic_poison** - the poison damage of the toxic spit.
* **sk_bullsquid_dmg_toxic_impact** - the impact damage of the toxic spit.

### Default classification

`Alien Predator`

### Soundscripts

* **Bullsquid.Idle** - idle sounds.
* **Bullsquid.Alert** - alert sounds.
* **Bullsquid.Pain** - pain sounds.
* **Bullsquid.Die** - death sounds.
* **Bullsquid.Growl** - a growling sound on the bite attack.
* **Bullsquid.Attack** - spit attack sound.
* **Bullsquid.AttackToxic** - toxic spit attack sound.
* **Bullsquid.Bite** - bit someone with a bite attack.
* **Bullsquid.SpitTouch** - one of the sounds played when the spit projectile hits something. Emitted from the spit projectile. Derived from **NPC.SpitTouch**
* **Bullsquid.SpitHit** - one of the sounds played when the spit projectile hits something. Emitted from the spit projectile. Derived from **NPC.SpitHit**
* **Bullsquid.ToxicSpitTouch** - one of the sounds played when the toxic spit projectile hits something. Emitted from the spit projectile.
* **Bullsquid.ToxicSpitHit** - one of the sounds played when the toxic spit projectile hits something. Emitted from the spit projectile.

### Visuals

* **Bullsquid.TinySpit** - sprites sprayed out of bullsquid mouth on spit attack.
* **Bullsquid.Spit** - the spit projectile sprite.
* **Bullsquid.Fleck** - sprites sprayed on the spit projectile impact with the wall.
* **Bullsquid.ToxicTinySpit** - sprites sprayed out of bullsquid mouth on toxic spit attack.
* **Bullsquid.ToxicSpit** - the toxic spit projectile sprite.
* **Bullsquid.ToxicFleck** - sprites sprayed on the toxic spit projectile impact with the wall.
* **Bullsquid.ToxicParticle** - sprites emitted from the toxic spit projectile as it flies. Note: currently can't be animated.

Shared visuals:

* **Bullsquid.TinySpitBase** - defines a sprite used by **Bullsquid.TinySpit**, **Bullsquid.ToxicTinySpit**, **Bullsquid.Fleck** and **Bullsquid.ToxicFleck**.

Notes:

* Spit customizations are also applied to [env_blowercannon]({{< ref env_blowercannon >}}).

### Entity template examples

{{% tabs %}}

{{% tab "Yellow bullsquid from HL Alpha" %}}
```json
{
    "yellowbullsquid": {
        "own_visual": {
            "model": "models/yellowbullsquid.mdl"
        },
        "visuals": {
            "Bullsquid.Spit": {
                "model": "models/spit.mdl",
                "rendermode": "normal",
                "framerate": 1.0,
                "scale": 1
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
