---
bookHidden: true
bookToC: false
---

# monster_osprey

[TWHL](https://twhl.info/wiki/page/monster_osprey)

### Changes

* Can be correctly spawned via [monstermaker]({{< ref monstermaker >}}) (notifies the monstermaker about its death). Use a custom `#target` keyvalue parameter so the spawned Osprey has the path to follow. Set the `#maxlivechildren` keyvalue parameter to ensure the resupply won't depend on the number of grunts alive on the map at the moment when the Osprey spawns. See [Configuring the spawned monster]({{< ref "monstermaker/#configuring-the-spawned-monster" >}}) and the **flyers_demo** map in the sample mod.
* `Death` and `50% Health Remaining` trigger conditions now work for Osprey.
* Osprey can use `Fire On Pass` triggers on the path entities.

The maximum number of grunts the Osprey can supply depends on the number of alive grunts at the moment the Osprey is activated. This is the default behavior from Half-Life which can be overriden by the `Custom number of grunts to resupply` parameter.

In Featureful SDK it also has some other changes:

* As Featureful SDK allows to chane the monster's relationship classification the Osprey will count only those grunts who are not enemy to it.
* If Osprey is configured to spawn grunts of certain [entity template]({{< ref "entity-templates" >}}) it will count only grunts of this entity template.

### New parameters

* `Rotor volume` - configure rotor volume. This can also be configured via **Osprey.Rotor** soundscript.
* `Rotor sound attenuation` - configure rotor sound attenuation. The less the value the larger the sound radius. The default attenuation is 0.15. This can also be configured via **Osprey.Rotor** soundscript.
* ![](/images/svencoop.png) `Grunt Type` - deploy Opposing Force grunts instead of Half-Life ones. Note: by default the spawned grunts will act as enemies to the player. Change the Osprey's classification to Player Ally so grunts inherit the custom classification.
* ![](/images/svencoop.png) `Number of grunts per deploy` - override the maximum number of grunts deployed at once. If defined this must be a number from 1 to 4.
* `Custom number of grunts to resupply` - override the maximum number of alive grunts the Osprey can resupply. After spawning this number of grunts the Osprey will stop deploying new ones until someone has died or been removed from the map.
* `Trigger on deploy (activator is Osprey)` - fire when Osprey is deploying grunts. The activator for this trigger is Osprey itself.
* `Trigger on deploy (activator is grunt)` - fire per grunt deploy. The activator for this trigger is the deployed grunt (so the trigger will be fired the number of times equal to the number of spawned grunts).
* `Obey Trigger Mode` - allows to stop and start Osprey. When stopped, the Osprey doesn't play rotor sound. If stopped during the deploy sequence  the Osprey will stop only after finishing the deploying.

### New spawnflags

* `Grunts Don't Drop Gun` - monsters spawned by Osprey won't drop guns like if they had `Don't Drop Gun` spawnflag set.
* `No deploy, just fly` - make osprey not care about grunt deploying and just fly on specified path. Such Osprey does not skip path corners with low speed like the default one does to find the deploy point.

### Skill variables

* **sk_osprey** - monster's health.

### Reaction to use-type

If `Start Inactive` spawnflag is set triggering the Osprey will make it start following its path.
If `Obey Trigger Mode` parameter is set to Yes, the Osprey will stop or continue depending on the use-type and current state.

{{% hint warning %}}
In order to remove (despawn) the Osprey properly you should:
* Have `Obey Trigger Mode` on it set to Yes.
* Trigger the aircraft (preferebly with `Off` use-type, e.g. via `trigger_relay`).
* Kill the aircraft via any kill-target trigger (e.g. `KillTarget` of `trigger_relay`). Ideally this should happen a bit later than turning the aircraft off (e.g. in 0.1 seconds).

If you just remove the aircraft (without turning it off first) while it's moving the rotor sound will continue playing until player saves and reloads the game.
{{% /hint %}}

### Default classification

`Machine`

### Soundscripts

* **Osprey.Rotor** - rotor sound during the flight. Must be looped.
* **Osprey.Crash** - death sound. Derived from **NPC.Crash**

### Visuals

* **Osprey.FallingSmoke** - smokes produced while Osprey is falling.
* **Osprey.DamageSmoke** - smoke indication of being damaged.
* **Osprey.Fireball** - big fireball played when Osprey crashes.
* **Osprey.BlastCircle** - wave played when Osprey crashes.
* **NPC.Rope** - the rope sprite for the rappelling grunts.

### Children

The type and variations of grunts spawned by Osprey can be configured via [entity templates]({{< ref "entity-templates/#children" >}}).

The list of monsters that can be correctly supplied by Osprey:

* [monster_human_grunt]({{< ref monster_human_grunt >}})
* [monster_human_grunt_ally]({{< ref monster_human_medic_ally >}})
* [monster_human_medic_ally]({{< ref monster_human_medic_ally >}})
* [monster_human_torch_ally]({{< ref monster_human_torch_ally >}})
* [monster_male_assassin]({{< ref monster_male_assassin >}})
* [monster_hwgrunt]({{< ref monster_hwgrunt >}})
* [monster_robogrunt]({{< ref monster_robogrunt >}})

### Entity template examples

{{% tabs %}}

{{% tab "Spawning human grunts with different weapons" %}}

```json
{
    "monster_osprey": {
        "children": [
            {
                "parameters": {
                    "weapons": 3
                },
                "chance": 0.5
            },
            {
                "parameters": {
                    "weapons": 10
                },
                "chance": 0.25
            },
            {
                "parameters": {
                    "weapons": 5
                },
                "chance": 0.25
            }
        ]
    }
}
```

{{% /tab %}}

{{% tab "Spawning different opfor human grunts" %}}

This will make Osprey to spawn opfor grunts with different weapons, including a medic.

```json
{
    "monster_osprey": {
        "children": [
            {
                "classname": "monster_human_grunt_ally",
                "parameters": {
                    "weapons": 3
                }
            },
            {
                "classname": "monster_human_grunt_ally",
                "parameters": {
                    "weapons": 10
                }
            },
            {
                "classname": "monster_human_grunt_ally",
                "parameters": {
                    "head": 1,
                    "weapons": 5
                }
            },
            {
                "classname": "monster_human_grunt_ally",
                "parameters": {
                    "weapons": 16
                }
            },
            {
                "classname": "monster_human_medic_ally",
                "parameters": {
                    "weapons": 10
                }
            }
        ]
    }
}
```

{{% /tab %}}

{{% tab "Spawning robogrunts" %}}

```json
{
    "monster_osprey": {
        "children": {
            "classname": "monster_robogrunt"
        }
    }
}
```

{{% /tab %}}

{{% /tabs %}}
