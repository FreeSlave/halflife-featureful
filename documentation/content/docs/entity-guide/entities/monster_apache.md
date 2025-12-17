---
bookHidden: true
bookToC: false
---

# monster_apache

[TWHL](https://twhl.info/wiki/page/monster_apache)

### Changes

* Can be correctly spawned via [monstermaker]({{< ref monstermaker >}}) (notifies the monstermaker about its death). Use a custom `#target` keyvalue parameter so the spawned Apache has the path to follow. See [Configuring the spawned monster]({{< ref "monstermaker/#configuring-the-spawned-monster" >}}) and the **flyers_demo** map in the sample mod.
* `Death` and `50% Health Remaining` trigger conditions now work for Apache.
* Apache can use `Fire On Pass` triggers on the path entities.

### New parameters

* `Rotor volume` - configure rotor volume. This can also be configured via **Apache.Rotor** soundscript.
* `Obey Trigger Mode` - allows to stop and start Apache. When stopped, the Apache doesn't play rotor sound and doesn't attack.

### Skill variables

* **sk_apache_health** - monster's health.

### Reaction to use-type

If `Start Inactive` spawnflag is set triggering the Apache will make it start following its path.
If `Obey Trigger Mode` parameter is set to Yes, the Apache will stop or continue depending on the use-type and current state.

{{% hint warning %}}
In order to remove (despawn) the Apache properly you should:
* Have `Obey Trigger Mode` on it set to Yes.
* Trigger the aircraft (preferebly with `Off` use-type, e.g. via `trigger_relay`).
* Kill the aircraft via any kill-target trigger (e.g. `KillTarget` of `trigger_relay`). Ideally this should happen a bit later than turning the aircraft off (e.g. in 0.1 seconds).

If you just remove the aircraft (without turning it off first) while it's moving the rotor sound will continue playing until player saves and reloads the game.
{{% /hint %}}

### Default classification

`Human Military`

### Soundscripts

* **Apache.Rotor** - rotor sound during the flight. Must be looped.
* **Apache.FireGun** - firing a bullet.
* **Apache.Crash** - death sound. Derived from **NPC.Crash**
* **Apache.RPG** - rocket launch sound. Emitted from the rocket projectile.

### Visuals

* **Apache.RocketModel** - Apache rocket model.
* **Apache.RocketTrail** - Apache rocket trailing beam.
* **Apache.FallingSmoke** - smokes produced while Apache is falling.
* **Apache.CrashSmoke** - big smoke on crash.
* **Apache.RocketSmoke** - smoke when launching rocket.
* **Apache.DamageSmoke** - smoke indication of being damaged.
* **Apache.Fireball** - big fireball played when Apache crashes.
* **Apache.BlastCircle** - wave played when Apache crashes.
* **Wreckage.Smoke** - smokes played after the Apache crashed.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack and Take Damage rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) and [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

{{% hint info %}}
The smoke behavior can't be currently expressed via entity templates, so the standard behavior always stays as is.
{{% /hint %}}

```json
{
    "monster_apache": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": 6,
                    "dmg_type": ["energybeam", "bullet", "club"]
                },
                "modifier": {
                    "skip_damage": true
                }
            },
            {
                "conditions": {
                    "dmg": "<=50",
                    "hitgroup": [1, 2],
                    "invert_hitgroup_check": true
                },
                "modifier": {
                    "skip_damage": true
                },
                "effects": {
                    "ricochet": {
                        "scale": 1.0,
                        "chance": 1.0
                    }
                }
            }
        ],
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["blast"]
                },
                "modifier": {
                    "dmg": "*2"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
