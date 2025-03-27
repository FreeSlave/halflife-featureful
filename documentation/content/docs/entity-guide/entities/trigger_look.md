---
bookHidden: true
bookToC: false
entityCategory: solid
---

# trigger_look

Triggers a target when the player looks at something while standing in the trigger area.

Designed after [trigger_look](https://developer.valvesoftware.com/wiki/Trigger_look) entity from Source.

### Parameters

* `Look Target` - the entity the player should look at in order to trigger a target.
* `Look Time` - the time, in seconds, that the player must look the target before firing the target. Resets if player leaves trigger, or looks outside the `Field of View` threshold.
* `Field Of View` - how close the player has to be looking at the target. 1.0 = straight ahead\n 0.0 = +/- 90 degrees\n -1.0 = all directions).	
* `Trigger on look` - the name of entity to trigger after player looked at the look target to the specified amount of time.
* `Master` - when master is locked, entity is not active.
