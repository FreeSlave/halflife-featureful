---
bookHidden: true
bookToC: false
---

# monster_alien_slave (aka Vortigaunt)

[TWHL](https://twhl.info/wiki/page/monster_alien_slave)

### Changes

* Restores some bits of its health upon damaging targets with zap attack (if **sk_islave_selfheal** is higher than 0).
* When full on health a vortigaunt gains an additional energy upon zapping enemies. The energy is used to perform the following abilities:
    - Revival of another ally vortigaunt (spends energy equal to the maximum health of the revived vortigaunt).
    - Healing an ally vortigaunt (energy converts to health).
    - Sending an armor charge to the ally player (spends energy equal to the amount of armor the charge token provides).
* Performing any of these abilities requires a vortigaunt to have higherr than 0 energy. The energy can go negative if performing an ability spends more energy than the vortigaunt currently has. In order to use the abilities again the vortigaunt must gain the positive value.
* The amount of energy the vortigaunt gets from zap attack depends on the amount of damage it dealt. So, if enemy is protected from shock attacks, damaging it will give less energy.
* The vortigaunt with `Squad Leader` spawnflag (even if he's not in the squad) has an initial pool of free energy on spawn. He also spends less energy comparing to other vortigaunts.
* ![](/images/svencoop.png) Can revive fallen ally alien slaves using an excess energy (if **sk_islave_revival** is higher than 0). Unlike Sven Co-op implementation it does not halve the maximum vortigaunt health. Instead each vortigaunt can be revived only once.
* Can heal injured ally alien slaves spending an excess energy (if **sk_islave_heal** is higher than 0).
* New ability: a coil attack (enabled if **sk_islave_coil_attack** is not 0) - makes a wave around the vortigaunt damaging all the nearby enemies and healing nearby ally vortigaunts. The coil attack neither gives nor requires energy, but healing nearby ally vortigaunts will take energy.
* Vortigaunt's arms are glowing upon charging an electro attack.
* When a vortigaunt has an excess energy, his claws deal more damage (if **sk_islave_arm_boost** is not 0). Claws are glowing at the time of melee attack.
* When idle, plays some additional effects like random lightning (if **sk_islave_idle_effects** is not 0). This is a feature cut from Half-Life.
* If player is ally, vortigaunt can charge player's suit armor kinda like in Half-Life 2 (if **sk_vortigaunt_armor_charge** is higher than 0). This action requires energy.

{{% hint warning %}}
If you don't want any new vortigaunt's abilities put the following entries in your **skill.cfg**:

```
sk_islave_revival 0
sk_islave_coil_attack 0
sk_islave_selfheal 0
sk_islave_heal 0
```
{{% /hint %}}

### New parameters

* `Initial energy` - custom amount of initial energy. If 0 or not set, the value of **sk_islave_initial_energy** is used.
* `Weapons` - allows vortigaunt to summon a familiar creature (snark or headcrab). Spawning other monsters is possible via [children]({{< ref "entity-templates/#children" >}}) configuration. Note that the same properties will be applied independently whether the vortigaunt is set to spawn snarks or headcrabs).

### Skill variables

* **sk_islave_health** - monster's health.
* **sk_islave_dmg_claw** - melee damage.
* **sk_islave_dmg_clawrake** - melee damage (event 2) (not used?).
* **sk_islave_dmg_zap** - zap beam damage. Note: alien slave creates two zap beams, so he can deal double damage if both beams hit the target. This also controls how much alien slave heals another.
* **sk_islave_zap_rate** - framerate of zap attack animation. In Half-Life it wasn't configurable. Default values: 1 on Easy and Medium, 1.5 on Hard (replicating the behavior from Half-Life).
* **sk_islave_revival** - whether alien slaves can revive each other. `0` means revival is not allowed. `1` means revival is allowed. Default values: 0 on easy and medium, 1 on hard.
* **sk_islave_coil_attack** - whether alien slaves can perform a coil attack. Default value is 1.
* **sk_islave_dmg_coil** - the maximum coil attack damage. By default it's **sk_islave_dmg_zap** multiplied by 2.5.
* **sk_islave_selfheal** - amount of self healing from zap attacks. By default it fallbacks to **sk_islave_dmg_zap**. Set it to 0 to disable self healing. The actual amount of self-healing depends on the damage the vortigaunt dealt with a zap attack, so it can't exceed the **sk_islave_dmg_zap** (unless the target has a special vulnerability to the shock attack or it was a crit hit, e.g. a headshot, so it dealt more damage).
* **sk_islave_heal** - the maximum amount of healing the vortigaunt can provide to another vortigaunt. By default it fallbacks to **sk_islave_dmg_zap**.
* **sk_islave_arm_boost** - whether the vortigaunt can use a boosted melee attack (when he has some free energy).
* **sk_islave_boosted_dmg_claw** - the boosted melee attack damage. By default it's **sk_islave_dmg_claw** multiplied by 1.5.
* **sk_islave_idle_effects** - whether the vortigaunt can play additional idle effects. Default value is 0 (disabled).
* **sk_islave_initial_energy** - amount of initial energy. Default value is 0.
* **sk_islave_max_energy** - maximum amount of energy the vortigaunt can get. By default it fallbacks to **sk_islave_health**.
* **sk_islave_delay_zap** - the minimum delay before the next attack after using a zap attack. This is `[0.5, 4.0]` range by default.
* **sk_islave_delay_coil** - the minimum delay before the next attack after using a coil attack. This is `[0.9, 4.0]` range by default.
* **sk_vortigaunt_armor_charge** - how much armor the charge token provides to an ally player. By default it fallbacks to **sk_battery**.

### Default classification

`Alien Military`

### Soundscripts

* **Vortigaunt.Idle** - idle sounds.
* **Vortigaunt.Alert** - alert sounds.
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
* **Vortigaunt.Use** - start following the ally player. Derived from **Vortigaunt.Idle**
* **Vortigaunt.UnUse** - stop following the ally player. Derived from **Vortigaunt.Alert**

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

{{% tab "Azure Sheep Pantheyeye" %}}
The Panthereye in Azure Sheep is based on vortigaunt, so we can implement it via entity template. Do not confuse it with [monster_panthereye]({{< ref monster_panthereye >}}). We took liberty of increasing the monster's bounding box size to allow projectiles properly hit the panther.

```json
{
    "asheep_panther": {
        "health": 800,
        "own_visual": {
            "model": "models/panther.mdl"
        },
        "size": {
            "mins": [-32, -32, 0],
            "maxs": [32, 32, 112]
        },
        "skill": {
            "islave_health": [800, 800, 1000],
            "islave_dmg_claw": [16, 20, 20],
            "islave_dmg_clawrake": [25, 25, 25],
            "islave_dmg_zap": [40, 40, 45],
            "islave_zap_rate": [1, 1, 1.5],
            "islave_revival": 0
        },
        "soundscripts": {
            "Vortigaunt.Idle": {
                "waves": ["panther/p_idle1.wav", "panther/p_idle2.wav"]
            },
            "Vortigaunt.Alert": {
                "waves": ["panther/p_alert1.wav", "panther/p_alert2.wav"]
            },
            "Vortigaunt.Pain": {
                "waves": ["panther/p_pain1.wav", "panther/p_pain2.wav"]
            },
            "Vortigaunt.Die": {
                "waves": ["panther/p_die1.wav", "panther/p_die2.wav"]
            },
            "Vortigaunt.AttackHit": {
                "waves": ["panther/pclaw_strike1.wav", "panther/pclaw_strike2.wav", "panther/pclaw_strike3.wav"]
            },
            "Vortigaunt.AttackMiss": {
                "waves": ["panther/pclaw_miss1.wav", "panther/pclaw_miss2.wav"]
            },
            "Vortigaunt.ZapPowerup": {
                "waves": ["panther/p_zap2.wav"]
            },
            "Vortigaunt.ZapShoot": {
                "waves": ["panther/p_shoot1.wav"]
            },
            "Vortigaunt.Electro": {
                "waves": ["panther/p_electro1.wav"]
            }
        },
        "visuals": {
            "Vortigaunt.ZapBeamColor": {
                "color": [192, 64, 8]
            },
            "Vortigaunt.ArmBeamColor": {
                "color": [192, 64, 8]
            },
            "Vortigaunt.BeamLightColor": {
                "color": [192, 64, 8]
            }
        },
        "trace_attack": [
            {
                "conditions": {
                    "dmg_type": ["shock"],
                    "attack_affinity": ["friendly", "self"]
                },
                "modifier": {
                    "skip_damage": true
                }
            },
            {
                "conditions": {
                    "hitgroup": "head",
                    "invert_hitgroup_check": true,
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "=0.01"
                },
                "effects": {
                    "ricochet": {}
                }
            }
        ]
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

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_alien_slave": {
        "check_melee_attack1": {
            "distance": 64,
            "dot": 0.7
        },
        "trace_hull_attacks": {
            "1": {
                "distance": 70,
                "punchangle": {
                    "pitch": 5,
                    "roll": 18
                }
            },
            "2": {
                "distance": 70,
                "punchangle": {
                    "pitch": 5,
                    "roll": 18
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
