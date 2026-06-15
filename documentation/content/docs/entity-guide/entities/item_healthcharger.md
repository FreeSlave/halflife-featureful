---
bookHidden: true
bookToC: false
entityCategory: object
---

# ![](/images/decay.png) item_healthcharger

Model-based health charger from PS2 version of Half-Life.

### Skill variables

Same as [func_healthcharger]({{< ref "func_healthcharger/#skill-variables" >}})

### Soundscripts

* Same as [func_healthcharger]({{< ref "func_healthcharger/#soundscripts" >}})
* **WallHealth.Deploy** - player when the player is nearby, along with the deploy animation. Derived from **WallHealth.Start**.

### Visuals

* **WallHealth.Tank** - the tank/jar model (with a green liquid).

### Model assumptions

The default model of the health charger is **models/health_charger_body.mdl**. It's expected to have the following properties:

* The following bone controllers:
    - index 0 - the arm part that rotates to the player. The value 0 means the arm is at middle position when deployed.
    - index 1 - the camera part that tracks the player's movement. The value 0 means the camera is looking forward.
* The following sequences:
    - **still** - waiting to be used.
    - **deploy** - deploy an arm when the player is near.
    - **retract_arm** - retract the arm when player went out of reach.
    - **give_shot** - start giving a needle shot.
    - **retract_shot** - retract the needle before retracting an arm when player stops using the health charger.
    - **prep_shot** - the arm is deployed but is not being used yet.
    - **shot_idle** - continuous use.
    - **inactive** - depleted.

{{% hint detail %}}
All bone controllers and all the sequences are optional. If some sequence is missing it resets to the first sequence (index 0) in the model.
{{% /hint %}}

The default tank/jar model spawned along with the health charger is **models/health_charger_both.mdl**. It's expected to have the following properties:

* The first bone controller is used to show the amount of healing juice left.
* The sequence 0 is a still state (before the health charger is getting used).
* The sequence 1 is a sloshing state (the health charger is being used. This is forced to be a looping animation).
* The sequence 2 is a transition to rest (from sloshing to still).

### Entity template examples

{{% tabs %}}

{{% tab "Simplified" %}}
Simplified version without a tank/jar model and with an empty deploy soundscript. May be useful with custom model that the tank/jar doesn't match and when the deploy sound is undesirable.

```json
{
    "item_healthcharger": {
        "visuals": {
            "WallHealth.Tank": {
                "model": ""
            }
        },
        "soundscripts": {
            "WallHealth.Deploy": {
                "waves": []
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
