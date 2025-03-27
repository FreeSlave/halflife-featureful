---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_geneworm

The final boss in Opposing Force.

### Skill variables

* **sk_geneworm_health** - monster's health (per cycle).
* **sk_geneworm_dmg_spit** - acid cloud damage.
* **sk_geneworm_dmg_hit** - melee damage. Actually dealt by [trigger_geneworm_hit]({{< ref trigger_geneworm_hit >}}).

### Default classification

`Race X Shock`

### Soundscripts

* **GeneWorm.Idle** - idle sounds.
* **GeneWorm.Die** - death sounds.
* **GeneWorm.Attack1** - attacking the left side.
* **GeneWorm.Attack2** - attacking the right side.
* **GeneWorm.Attack3** - attacking the forward.
* **GeneWorm.BeamAttack** - range attack.
* **GeneWorm.Entry** - entering through the portal.
* **GeneWorm.EyePain** - getting shot in the eye.
* **GeneWorm.BigPain1** - pain before opening the orifice.
* **GeneWorm.BigPain2** - when the orifice is open.
* **GeneWorm.BigPain3** - pain before closing spawning a trooper.
* **GeneWorm.BigPain4** - before spawning a soldier and closing the orifice.
* **GeneWorm.LaunchSpawn** - launching the portal.
* **GeneWorm.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**. Emitted from [trigger_geneworm_hit]({{< ref trigger_geneworm_hit >}}).

### Visuals

* **GeneWorm.EyeLight** - eye entity light.
* **GeneWorm.Cloud** - acid cloud projectile.
* **GeneWorm.SpawnGlow** - glow sprite spawned in in the opening orifice when GeneWorm is in pain.
* **GeneWorm.SpawnGlowLight** - entity light at the orifice.
* **GeneWorm.SpawnTeleSprite** - sprite that comes out of the orifice after enough damage has been dealt to the orifice. This visual is applied to the same entity that poses as a sprite in the orifice, so the effect inherits its properties.
* **GeneWorm.SpawnTeleLight** - entity light produced by the teleport sprite.
* **GeneWorm.SpawnTeleBeam** - beams appearing before the trooper spawns, not hitting anything.
* **GeneWorm.SpawnTeleBeamHit** - beams appearing before the trooper spawns, hitting something. Derived from **GeneWorm.SpawnTeleBeam**.

### Special targets

Depending on the animation event Geneworm fires the following targets:

* `3` - **GeneWormLeftSlash** with `On` use-type.
* `4` - **GeneWormLeftSlash** with `Off` use-type.
* `5` - **GeneWormRightSlash** with `On` use-type.
* `6` - **GeneWormRightSlash** with `Off` use-type.
* `7` - **GeneWormCenterSlash** with `On` use-type.
* `8` - **GeneWormCenterSlash** with `Off` use-type.
* `9` - **GeneWormWallHit** with `Toggle` use-type.

When Geneworm is defeated, **GeneWormDead** is fired. 15 seconds later **GeneWormTeleport** automatically touches the player.
