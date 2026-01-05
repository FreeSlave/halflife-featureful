---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_human_grunt_ally

Opposing Force human grunt.

{{% hint info %}}
Despite the `ally` part in the entity name, the monster can be made enemy to the player via custom classification in the entity parameters or entity template.
{{% /hint %}}

### Changes

* When human grunt checks for friendly fire he takes into account all allies, not only those who are in the same squad as him.

### Skill variables

* **sk_hgrunt_ally_health** - monster's health. If not defined, **sk_hgrunt_health** is used.
* **sk_hgrunt_ally_kick** - kick damage. If not defined, **sk_hgrunt_kick** is used.
* **sk_hgrunt_ally_pellets** - the number of shotgun pellets. If not defined, **sk_hgrunt_pellets** is used.
* **sk_hgrunt_ally_gspeed** - the grenade speed when it's thrown. If not defined, **sk_hgrunt_gspeed** is used.
* **sk_9mmAR_bullet** - 9mmAR damage.
* **sk_buckshot** - shotgun pellet damage. If not defined, **sk_plr_buckshot** is used.
* **sk_556_bullet** - M249 damage. If not defined, **sk_plr_556_bullet** is used.
* **sk_hgrunt_ally_gren_launch_delay** - delay before the next check for grenade launch after firing a grenade. Default value is 6.
* **sk_hgrunt_ally_gren_throw_delay** - delay before the next check for grenade toss after throwing a grenade. Default value is 6.

### Default classification

`Player Ally Military`

### Soundscripts

* **HGruntAlly.Pain** - pain sounds.
* **HGruntAlly.Die** - death sounds.
* **HGruntAlly.CallMedic** - call medic when low on health.
* **HGruntAlly.Reload** - reload sound. Derived from **NPC.Reload**
* **HGruntAlly.9MM** - burst MP5 shot. Derived from **NPC.9MM**
* **HGruntAlly.GrenadeLaunch** - MP5 grenade launch. Derived from **NPC.GrenadeLaunch**
* **HGruntAlly.Shotgun** - shotgun fire. Derived from **NPC.Shotgun**
* **HGruntAlly.M249** - M249 fire. Derived from **NPC.M249**
* **HGruntAlly.ReloadM249** - M249 reload.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

{{% hint info %}}
Opposing Force human grunt model doesn't actually have hitgroup 11. If you want to enable the helmet protection behavior, you must change the model and add a filter for the body group (so only helmet variations would have an additional protection) similarly to how it's implemented in [monster_human_grunt]({{% ref monster_human_grunt %}}) trace attack rules.
{{% /hint %}}

```json
{
    "monster_human_grunt_ally": {
        "trace_attack": [
            {
                "conditions": {
                    "hitgroup": ["chest", "stomach"],
                    "dmg_type": ["bullet", "slash", "club"]
                },
                "modifier": {
                    "dmg": "*0.5"
                }
            },
            {
                "conditions": {
                    "dmg_type": ["bullet", "slash", "blast", "club"],
                    "hitgroup": 11
                },
                "modifier": {
                    "dmg": "-20",
                    "dmg_min_threshold": 0.01,
                    "hitgroup": "head"
                },
                "threshold_effects": {
                    "ricochet": {
                        "chance": 1.0,
                        "scale": 1
                    }
                }
            },
            {
                "conditions": {
                    "hitgroup": 11
                },
                "modifier": {
                    "hitgroup": "head"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
