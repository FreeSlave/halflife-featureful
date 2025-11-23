---
bookHidden: true
bookToC: false
entityCategory: solid
---

# func_vehicle

[TWHL](https://twhl.info/wiki/page/func_vehicle)

The controllable vehicle entity added in HL 25th Anniversary update (under the Featureful SDK it works in *steam_legacy* as well).

### Changes

The vehicle controls can be toggled via [trigger_vehiclecontrols]({{< ref trigger_vehiclecontrols >}}).

### New parameters

* `Custom reverse speed` - custom reverse speed. By default for reverse movement the value of `Speed` multiplied by 0.35 is used. This parameter allows to override the this behavior.
* `Custom deceleration per 0.1 seconds` - custom deceleration. By default when the vehicle doesn't get input from its controls it decelerates by 20 units per 0.1 seconds. This parameter allows to change this behavior.
* `Stop sound at halt` - stop the engine sound when vehicle has stopped and isn't controlled by a player. By default the sound keeps running after player leaves the vehicle.
* `Custom brake sound` - custom brake sound to play instead of **plats/vehicle_brake1.wav**.
* `Custom ignition sound` - custom ignition sound to play instead of **plats/vehicle_ignition.wav**.
* `Touch proxy targetname` - create an invisible proxy entity that can touch triggers and give it this name. The entity will move with a vehicle at its origin. The [trigger_multiple]({{< ref trigger_multiple >}}) or [trigger_once]({{< ref trigger_once >}}) can check against this targetname or the `func_vehicle_proxy` classname. This hack is needed because the trains and vehicles have such physics that don't touch triggers.

### Bugfixes

* Fixed the movement sound not always playing when it should (the HL 25th Anniversary update bug).

### Notes

The *steam_legacy* version of Half-Life doesn't provide the **events/vehicle.sc** file which is required for proper vehicle sound handling. If you want to use the `func_vehicle` in your mod and support the *steam_legacy* branch ensure that the file exists in your mod directory.
