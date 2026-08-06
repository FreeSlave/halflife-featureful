---
bookHidden: true
bookToC: false
entityCategory: object
---

# ![](/images/decay.png) item_recharge

Model-based suit charger from PS2 version of Half-Life.

{{% hint info %}}
This entity can play sound [animation events]({{< ref model-animation-events >}}). Use entity template with [autoprecache_sounds]({{< ref "entity-templates/#autoprecache_sounds" >}}) enabled to precache such sounds.
{{% /hint %}}

### Parameters

* `Collision type`
    - `Approximate. Arm interpolation` - use approximate collision to avoid blocking doors.
    - `Accurate. No arm interpolation` - use more accurate collision, but lose the bone controller interpolation (the arm following the player won't be smooth). Good option when the custom model without an arm is used.

### Skill variables

Same as [func_recharge]({{< ref "func_recharge/#skill-variables" >}})

### Soundscripts

* Same as [func_recharge]({{< ref "func_recharge/#soundscripts" >}})
* **SuitRecharge.Deploy** - player when the player is nearby, along with the deploy animation. Derived from **SuitRecharge.Start**.

### Visuals

* **SuitRecharge.Glass** - the glass model covering the electric coils.
* **SuitRecharge.Beam** - the beam shown at the end of the charger's "arm".

### Model assumptions

The default model of the suit charger is **models/hev.mdl**. It's expected to have the following properties:

* The following bone controllers:
    - index 0 - for the camera that tracks the player's movement. The value 0 means the camera is looking forward.
    - index 1 - the left coil that's changing while the suit charger is being used.
    - index 2 - the right coil that's changing while the suit charger is being used.
    - index 3 - the arm part that rotates to the player. The value 0 means the arm is at middle position when deployed.
* The following sequences:
    - **rest** - waiting to be used or depleted.
    - **deploy** - deploy an arm when the player is near.
    - **retract_arm** - retract the arm when player went out of reach.
    - **give_charge** - start giving a needle shot.
    - **retract_charge** - retract the needle before retracting an arm when player stops using the suit charger.
    - **prep_charge** - the arm is deployed but is not being used yet.
    - **charge_idle** - continuous use.

{{% hint info %}}
All bone controllers and all the sequences are optional. If some sequence is missing it resets to the first sequence (index 0) in the model.
{{% /hint %}}

### Entity template examples

{{% tabs %}}

{{% tab "Simplified" %}}
Simplified version without a glass model, without beams and with an empty deploy soundscript. Can be useful with a custom model that the glass and beams don't match and when the deploy sound is undesirable.

```json
{
    "item_recharge": {
        "visuals": {
            "SuitRecharge.Glass": {
                "model": ""
            },
            "SuitRecharge.Beam": {
                "model": ""
            }
        },
        "soundscripts": {
            "SuitRecharge.Deploy": {
                "waves": []
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}
