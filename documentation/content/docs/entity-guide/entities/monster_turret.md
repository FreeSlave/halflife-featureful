---
bookHidden: true
bookToC: false
---

# monster_turret

[TWHL](https://twhl.info/wiki/page/monster_turret)

### Changes

* `Death` and `50% Health Remaining` trigger conditions now work for turrets.
* Can be correctly spawned via [monstermaker]({{< ref monstermaker >}}) (notifies the monstermaker about its death). If monstermaker doesn't set the children's name, the turret is automatically set to the autostart mode. If the children's name is set the spawned turret can be activated by a trigger using this name. Just like with others monsters, if the monstermaker spawns more than 1 monster the turrets that have been killed fade away. Use a custom `#orientation` keyvalue parameter to spawn a ceiling turret.

### Skill variables

* **sk_turret_health** - monster's health.
* **sk_12mm_bullet** - bullet damage.

### Default classification

`Machine`

### Soundscripts

* **Turret.Alert** - when detected an enemy.
* **Turret.Deploy** - deploy sound.
* **Turret.Undeploy** - retire sound.
* **Turret.Ping** - pinging noise. Played every second while searching for enemy.
* **Turret.Spinup** - a looping spin sound.
* **Turret.Die** - death sound.
* **Turret.Shoot** - single shot.
* **Turret.SpinUpCall** - starting spin up.
* **Turret.SpinDownCall** - spin down.

### Visuals

* **Turret.Glow** - turret sprite. Active while searching for enemy.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_turret": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": 10
                },
                "modifier": {
                    "dmg": "=0.1"
                },
                "effects": {
                    "ricochet": {
                        "chance": 0.1,
                        "scale": [1, 2],
                        "certain_on_new_frame": true
                    }
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
