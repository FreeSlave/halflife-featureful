---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_breakable

[TWHL](https://twhl.info/wiki/page/func_breakable)

### New parameters

* ![](/images/svencoop.png) Most of spawnobject values are compatible with those in Sven Co-op.
* `Spawn On Break (classname)` - specify the classname of the spawn object manually instead choosing from the predefined list. Technically this can be any entity. This allows easily spawn monsters from the breakables.
* `Entity template for Spawn On Break` - set [entity template]({{< ref entity-templates >}}) for the spawn object.
* `info_item_random entity` - name of the [info_item_random]({{< ref info_item_random >}}) entity for the item drop randomization. If this is defined the `func_breakable` will prefer using this instead of `Spawn On Break`.
* `Acceptable Damage Type` - take damage only if it's of this type. This is checked after the [take_damage]({{< ref "entity-templates/#take_damage" >}}) rules.
* `Relative to attack` option is now working as intended (was buggy in Half-Life).
* `Custom gib scale` - custom gib model scale.
* `Custom number of gibs` - custom number of gibs (by default it depends on the breakable's size).
* ![](/images/svencoop.png) `HUD Info name` - name to display when player is looking at this breakable. Requires `Show HUD Info` spawnflag.
* `Switch Texture When Damaged` - switch the textures (from `+0` to `+A` variant or vice-versa) on the brush model when half or less health has left. This allows to simulate the half-broken materials (e.g. cracked glass).
* `Spark When Hit` - allow spark effect when the breakable is hit, even if material is not a `Computer`.

{{% hint info %}}
The [loot drop]({{< ref "entity-templates/#loot_drop" >}}) can be configured for the breakable's entity template. This will make `func_breakable` drop additional items.
{{% /hint %}}

### New spawnflags

* ![](/images/svencoop.png) `Show HUD Info` - whether to show HUD info for players looking at this breakable. This is not affected by `mp_allowmonsterinfo` and `sp_allowmonsterinfo` cvars.
* ![](/images/svencoop.png) `Explosive Only` - make the breakable immune to any non-explosive damage.
* `Op4Mortar only` - make the breakable resistant to anything besides `op4mortar` shells.
* `Not solid` - make breakable non-solid (can be destroyed only with trigger).
* `Smoke trails on gibs` - add gray particle trails on gibs (using the default effect from the engine).
* `Transparent gibs` - make gibs semi-transparent.

### Calc Ratio

Reports the *health* / *max health* fraction.

### Default display name

`Breakable`

### Soundscripts

When the breakable gets hit (depending on the material):

* **Breakable.Computer** - computer.
* **Breakable.Concrete** - concrete or rocks.
* **Breakable.Flesh** - flesh.
* **Breakable.Glass** - glass.
* **Breakable.Metal** - metal.
* **Breakable.Wood** - wood.

{{% hint info %}}
In addition to the breakable sound the material sound of the texture is also playing (with decreased volume).
{{% /hint %}}

{{% hint warning %}}
Changing the breakable hit soundscripts globally also affects the sounds produced by gibs spawned via [env_shooter]({{< ref "env_shooter#soundscripts" >}}).
{{% /hint %}}

When the breakable gets destroyed (depending on the material):

* **Breakable.BustCeiling** - ceiling tile.
* **Breakable.BustConcrete** - concrete (cinder block).
* **Breakable.BustComputer** - computer. By default it's the same as **Breakable.BustMetal**.
* **Breakable.BustFlesh** - flesh.
* **Breakable.BustGlass** - glass.
* **Breakable.BustMetal** - metal.
* **Breakable.BustRocks** - rocks. By default it's the same as **Breakable.BustConcrete**.
* **Breakable.BustWood** - wood.

Other soundscripts:

* **Breakable.Spark** - the spark sound to play along with the spark sprite effect. Derived from [Material.Spark]({{< ref "soundscripts/#effect-soundscripts" >}}).

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
