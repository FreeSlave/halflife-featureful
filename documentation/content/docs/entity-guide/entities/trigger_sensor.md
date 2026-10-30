---
bookHidden: true
bookToC: false
---

# trigger_sensor

An entity, which, when triggered, detects the entities in a specified proximity to its position and fire the target with the detected entity as an activator. In combination with `[LE]` parameters of other entities it allows to create some interesting entity setups. Use it with [trigger_timer]({{< ref trigger_timer >}}) to make sensors that do detection checks over periods of time.

### Parameters

* `Radius` - detect entities in this radius, relative to the source position (this entity's origin by default).
* `Target` - trigger this target for every detected entity, using the detected entity as an activator.
* `Source Position [LP]` - use the calculated position instead of the entity origin, when searching for entities.
* `Line is blocked by` - whether to trace a line between the source position.
* `Trace line to` - when tracing a line, trace it to the specified part of the entity.
* `Fire trigger for` - whether to trigger the target for all detected entities, for only the closest or the farthest one.
* `Entity is alive` - check if entity is alive or dead.
* `Entity is damageable` - check if entity can take damage.
* `Trigger Master` - test the sensed entity against the master. Works as a filter. Use it with [trigger_compare]({{< ref trigger_compare >}}) to test the monster's health.
* `Required classname` - detect entities only of this classname.
* `Act as Classify` - used for relationship checks (see below).
* `My relationship to entity` - check the relationship between classification from `Act as Classify` and the detected entity.

### Reaction to use-type

Currently doesn't interpret the input use-type.
