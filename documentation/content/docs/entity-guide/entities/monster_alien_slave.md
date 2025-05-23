---
bookHidden: true
bookToC: false
---

# monster_alien_slave (aka Vortigaunt)

[TWHL](https://twhl.info/wiki/page/monster_alien_slave)

### Changes

* Restores some bits of its health upon damaging targets with electro-attack (if `vortigaunt_selfheal` feature is enabled).
* When full on health vortigaunt gains an additional energy upon electrocuting enemies. The energy is used to execute new abilities.
* ![](/images/svencoop.png) Can revive fallen ally alien slaves using an excess energy (if `vortigaunt_revive` feature is enabled). Unlike Sven Co-op implementation it does not halve the maximum vortigaunt health. Instead vortigaunt can be revived only once.
* Can heal injured ally alien slaves spending an excess energy (if `vortigaunt_heal` feature is enabled).
* New ability: a coil attack (if `vortigaunt_coil_attack` feature is enabled).
* Vortigaunt's arms are glowing upon charging an electro attack.
* When a vortigaunt has an excess energy, his claws deal more damage (claws are glowing at the time of melee attack) (if `vortigaunt_arm_boost` feature is enabled).
* When idle, plays some additional effects like random lightning (a feature cut from Half-Life) (if `vortigaunt_idle_effects` feature is enabled).
* If player is ally, vortigaunt can charge player's suit armor (requires energy) kinda like in Half-Life 2 (if `vortigaunt_armor_charge` feature is enabled).

### Skill variables

* **sk_islave_health** - monster's health.
* **sk_islave_dmg_claw** - melee damage.
* **sk_islave_dmg_clawrake** - melee damage (event 2) (not used?).
* **sk_islave_dmg_zap** - zap beam damage. Note: alien slave creates two zap beams, so he can deal double damage if both beams hit the target. This also controls how much alien slave heals another. The maximum coil attack damage is this variable multiplied by 2.5.
* **sk_islave_zap_rate** - framerate of zap attack animation. In Half-Life it wasn't configurable. Default values: 1 on easy and medium, 1.5 on hard (replicating the behavior from Half-Life).
* **sk_islave_revival** - whether alien slaves can revive each other. `0` means revival is not allowed. `1` means revival is allowed. Default values: 0 on easy and medium, 1 on hard. 

### Default classification

`Alien Military`

### Soundscripts

* **Vortigaunt.Pain** - pain sounds.
* **Vortigaunt.Die** - death sounds.
* **Vortigaunt.ZapPowerup** - played several times during the zap attack powerup.
* **Vortigaunt.ZapShoot** - played on lightning attack.
* **Vortigaunt.Electro** - played at the end of ligtning.
* **Vortigaunt.GlowArm** - melee attack with charged arms.
* **Vortigaunt.IdleZap** - played along with idle effects.
* **Vortigaunt.SummonStart** - starting spawn a familiar.
* **Vortigaunt.SummonEnd** - finishing spawn a familiar.
* **Vortigaunt.AttackHit** - melee attack hit. Derived from **NPC.AttackHit**
* **Vortigaunt.AttackMiss** - melee attack miss. Derived from **NPC.AttackMiss**
* **Vortigaunt.SuitOn** - charge token of friendly vortigaunt touches the player.

### Visuals

* **Vortigaunt.PowerupBeam** - beams produced on attack powerup.
* **Vortigaunt.PowerupLight** - dynamic light emitted on attack powerup.
* **Vortigaunt.ZapBeam** - attack zap beam.
* **Vortigaunt.RevivalBeam** - revival beam.
* **Vortigaunt.IdleBeam** - temporary beams occasionally played when vortigaunt is idle.
* **Vortigaunt.IdleLight** - dynamic light occasionally emitted when vortigaunt is idle.
* **Vortigaunt.CoilBeam** - ring beam produced on the coil attack.
* **Vortigaunt.HandGlow** - sprites appearing on vortigaunt's hands when he uses range attacks or boosted melee attack.
* **Vortigaunt.MeleeTrailBeam** - beam trail emitted from arms when vortigaunt uses a melee attack and has some spare energy.
* **Vortigaunt.SummonBeam** - beams produced on summon animation.
* **Vortigaunt.SummonSprite** - sprite of the summoning effect.
* **Vortigaunt.SummonLight** - dynamic light emitted on summoning effect.
* **Vortigaunt.ChargeToken** - charge token sprite.
* **Vortigaunt.ChargeTokenLight** - entity light emitted from the charge token sprite.

Shared visuals:

* **Vortigaunt.ZapBeamColor** - color shared by **Vortigaunt.ZapBeam**, **Vortigaunt.RevivalBeam**, **Vortigaunt.SummonBeam**, **Vortigaunt.CoilBeam** and **Vortigaunt.HandGlow**.
* **Vortigaunt.ArmBeamColor** - color shared by **Vortigaunt.PowerupBeam**, **Vortigaunt.IdleBeam** and **Vortigaunt.MeleeTrailBeam**.
* **Vortigaunt.BeamLightColor** - color shared by **Vortigaunt.PowerupLight**, **Vortigaunt.IdleLight** and **Vortigaunt.SummonLight**.

### Attacks

* *Melee Attack 1* - melee.
* *Range Attack 1* - zap beam or coil beam attack.
* *Range Attack 2* - heal or revive other vortigaunt.

### Animation events

* `1` - trace hull attack with an arm. Deals **sk_islave_dmg_claw** damage. Plays **Vortigaunt.AttackHit** or **Vortigaunt.AttackMiss** soundscripts.
* `2` - trace hull attack. Deals **sk_islave_dmg_clawrake** damage. Plays **Vortigaunt.AttackHit** or **Vortigaunt.AttackMiss** soundscripts. Not used in the standard model.
* `3` - create arm beam with **Vortigaunt.PowerupBeam** or **Vortigaunt.RevivalBeam** visual on both arms and dynamic light with **Vortigaunt.PowerupLight** visual in front of the monster. Plays **Vortigaunt.ZapPowerup** soundscript.
* `4` - perform the beam attack hit from two arms (deals **sk_islave_dmg_zap** damage per beam) with **Vortigaunt.ZapBeam** visuals or peform a coil attack with **Vortigaunt.CoilBeam** visual. Plays **Vortigaunt.Electro** soundscript.
* `5` - clear zap beams.

### Entity template examples

{{% tabs %}}

{{% tab "Allow squad leaders" %}}
Allow vortigaunts to be squad leaders (in original Half-Life they can't).
```json
{
    "monster_alien_slave": {
        "squad_capability": {
            "can_recruit": true
        }
    }
}
```
{{% /tab %}}

{{% tab "Friendly vortigaunt" %}}
Player friendly vortigaunt from Sven Co-op.
```json
{
    "vort_friendly": {
        "inherits": "monster_alien_slave",
        "own_visual": {
            "model": "models/islavef.mdl"
        },
        "classify": "Player Ally",
        "visuals": {
            "Vortigaunt.ZapBeamColor": {
                "color": [96, 180, 255]
            },
            "Vortigaunt.ArmBeamColor": {
                "color": [96, 180, 255]
            }
        }
    }
}
```
{{% /tab %}}

{{% tab "Azure Sheep experimental vortigaunt" %}}
Experimental fusion of human and chumtoad from Azure Sheep.
```json
{
    "vort_exp": {
        "inherits": "monster_alien_slave",
        "own_visual": {
            "model": "models/sslave.mdl"
        },
        "blood": "red",
        "visuals": {
            "Vortigaunt.ZapBeamColor": {
                "color": [255, 128, 255]
            },
            "Vortigaunt.ArmBeamColor": {
                "color": [255, 128, 255]
            },
            "Vortigaunt.BeamLightColor": {
                "color": [255, 128, 255]
            }
        }
    }
}
```
{{% /tab %}}

{{% tab "Trace Attack and Take Damage rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) and [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_alien_slave": {
        "trace_attack": [
            {
                "conditions": {
                    "dmg_type": ["shock"],
                    "attack_affinity": ["friendly", "self"]
                },
                "modifier": {
                    "skip_damage": true
                }
            }
        ],
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["slash"],
                    "attack_affinity": ["friendly"]
                },
                "modifier": {
                    "skip_damage": true
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
