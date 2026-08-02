---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_tracktrain

[TWHL](https://twhl.info/wiki/page/func_tracktrain)

### Changes

The train controls can be toggled via [trigger_vehiclecontrols]({{< ref trigger_vehiclecontrols >}}).

### New parameters

* ![](/images/svencoop.png) Custom move, start and brake sounds.
* `Use Sound Override` - custom use sound to play instead of **plats/train_use1.wav**.
* `Sound radius` - configure platform sounds attenuation.
* `Collision with corpses` - allows to ignore collision with dead monsters. Corpses won't block this train and won't get crushed.
* `Instantly gib corpses` - instantly gib corpses that block the train, even if train deals 0 damage.
* `How to handle tiny creatures` - override handling of [tiny creatures]({{< ref "monsters/#tiny-creatures" >}}).
* `Touch proxy targetname` - create an invisible proxy entity that can touch triggers and give it this name. The entity will move with a train at its origin. The [trigger_multiple]({{< ref trigger_multiple >}}) or [trigger_once]({{< ref trigger_once >}}) can check against this targetname or the `func_tracktrain_proxy` classname. This hack is needed because the trains and vehicles have such physics that don't touch triggers.
