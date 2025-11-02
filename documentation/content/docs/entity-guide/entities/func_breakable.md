---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_breakable

[TWHL](https://twhl.info/wiki/page/func_breakable)

### New parameters

* ![](/images/svencoop.png) Most of spawnobject values are compatible with those in Sven Co-op.
* `Relative to attack` option is now working as intended (was buggy in Half-Life).
* ![](/images/svencoop.png) `HUD Info name` - name to display when player is looking at this breakable. Requires `Show HUD Info` spawnflag.
* `Switch to alternate texture` - switch the textures (from `+0` to `+A` variant or vice-versa) on the brush model when half or less health has left. This allows to simulate the half-broken materials (e.g. cracked glass).

### New spawnflags

* ![](/images/svencoop.png) `Show HUD Info` - whether to show HUD info for players looking at this breakable. This is not affected by `mp_allowmonsterinfo` and `sp_allowmonsterinfo` cvars.
* ![](/images/svencoop.png) `Explosive Only` - make the breakable immune to any non-explosive damage.
* `Op4Mortar only` - make the breakable resistant to anything besides `op4mortar` shells.
* `Not solid` - make breakable non-solid (can be destroyed only with trigger).
* `Smoke trails on gibs` - add gray particle trails on gibs (using the default effect from the engine).
* `Transparent gibs` - make gibs semi-transparent.

### Calc Ratio

Reports the *health* / *max health* fraction.

### Soundscripts

* **Breakable.BustCeiling** - the breakable of ceiling tile material gets destroyed.
* **Breakable.BustConcrete** - the breakable of concrete (cinder block) material gets destroyed.
* **Breakable.BustComputer** - the breakable of computer material gets destroyed. By default it's the same as **Breakable.BustMetal**.
* **Breakable.BustFlesh** - the breakable of flesh material gets destroyed.
* **Breakable.BustGlass** - the breakable of glass material gets destroyed.
* **Breakable.BustMetal** - the breakable of metal material gets destroyed.
* **Breakable.BustRocks** - the breakable of rocks material gets destroyed. By default it's the same as **Breakable.BustConcrete**.
* **Breakable.BustWood** - the breakable of wood material gets destroyed.

### Bugfixes

* Fixed `func_breakable` taking damage from some attacks even if `Only Trigger` spawnflag is set (original Half-Life bug).

### Entity template examples

{{% tabs %}}

{{% tab "Take Damage rules" %}}
The [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate entity's native ones. Could be used as a starting point for further changes.

```json
{
    "func_breakable": {
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": "club"
                },
                "modifier": {
                    "dmg": "*2"
                }
            },
            {
                "conditions": {
                    "dmg_type": "poison"
                },
                "modifier": {
                    "dmg": "*0.1"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% tab "Vulnerable only to certain projectile type" %}}
The `func_breakable` belonging to the following entity template will take damage from [shockroach projectiles]({{< ref shock_beam >}}) only.

```json
{
    "shockable_breakable": {
        "take_damage": [
            {
                "conditions": {
                    "inflictor": {
                        "classname": "shock_beam",
                        "negate": true
                    }
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
