---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) monster_robogrunt

Robogrunts act mostly the same way as human grunts, but they don't bleed and have 80% resistance to every types of damage besides explosion, shock, frost, acid and energy beam.

When die robogrunts play spark effects and explode after 2 seconds.

### Skill variables

* **sk_hgrunt_health** - monster's health.
* **sk_hgrunt_kick** - kick damage.
* **sk_hgrunt_pellets** - the number of shotgun pellets.
* **sk_hgrunt_gspeed** - the grenade speed when it's thrown.
* **sk_9mmAR_bullet** - 9mmAR damage.
* **sk_buckshot** - shotgun pellet damage. If not defined, **sk_plr_buckshot** is used.
* **sk_rgrunt_explode** - after death explosion damage. If not defined, **sk_plr_hand_grenade** is used.

### Default classification

`Machine`

### Default display name

`Robo Grunt`

### Soundscripts

* **RGrunt.Die** - death sound.
* **RGrunt.Pain** - pain sound. Empty by default.
* **RGrunt.Spark** - spark sound (when low on health or about to explode)
* **RGrunt.Reload** - reload sound. Derived from **NPC.Reload**
* **RGrunt.9MM** - burst MP5 shot. Derived from **NPC.9MM**
* **RGrunt.GrenadeLaunch** - MP5 grenade launch. Derived from **NPC.GrenadeLaunch**
* **RGrunt.Shotgun** - shotgun fire. Derived from **NPC.Shotgun**
* **RGrunt.Use** - start following the ally player.
* **RGrunt.UnUse** - stop following the ally player.

### Entity template examples

{{% tabs %}}

{{% tab "Trace Attack and Take Damage rules" %}}
The [trace attack]({{< ref "entity-templates/#trace_attack" >}}) and [take damage]({{< ref "entity-templates/#take_damage" >}}) rules that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_robogrunt": {
        "trace_attack": [
            {
                "conditions": {
                    "dmg_type": ["energybeam", "crush", "blast", "shock", "freeze", "acid"],
                    "dmg_type_match": "none"
                },
                "effects": {
                    "ricochet": {
                        "chance": 0.1,
                        "scale": [1, 2],
                        "certain_on_new_frame": true
                    }
                }
            }
        ],
        "take_damage": [
            {
                "conditions": {
                    "dmg_type": ["energybeam", "crush", "blast"],
                    "dmg_type_match": "none"
                },
                "modifier": {
                    "dmg": "*0.2"
                }
            }
        ]
    }
}
```
{{% /tab %}}

{{% /tabs %}}
