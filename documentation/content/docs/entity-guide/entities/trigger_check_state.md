---
bookHidden: true
bookToC: false
---

# trigger_check_state

Tests the state of another entity and fires the appropriate targets. Can be used to manually check the state of entities which are usually used as masters.

### Parameters

* `Entity to check` - the name of the entity to check its state. If omitted, nothing happens. The first found entity of such name will be used.
* `Target (on & off)` - the target to fire. If state turns out to be On it sends the `On` use-type. Otherwise it sends the `Off` use-type.
* `Fire if off` - trigger if state is Off. Sends the `Toggle` use-type.
* `Fire if on` - trigger if state is On. Sends the `Toggle` use-type.
* `Fire if absent` - trigger if the specified entity wasn't found (e.g. was deleted or hasn't spawn yet).
