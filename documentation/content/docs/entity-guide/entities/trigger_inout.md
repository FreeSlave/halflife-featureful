---
bookHidden: true
bookToC: false
entityCategory: solid
---

# ![](/images/spirit.png) trigger_inout

Track the entities entering and leaving the designated area, and trigger targets accodingly.

{{% hint warning %}}
This entity is expected to have a cuboid shape. Otherwise the `Fire on leaving` won't trigger correctly.
{{% /hint %}}

{{% hint info %}}
The check for leaving entities is executed every 0.1 seconds, so the fact of leaving may be detected a bit later.
{{% /hint %}}

### Parameters

* `Fire on entering` - fire when entity enters the area.
* `Fire on leaving` - fire when entity, that previously entered the area, leaves the area.
* `Fire on/off (entering/leaving)` - fire when entity enter or leaves the area. It sends the `On` use-type when entity enters, and `Off` when entity leaves.
