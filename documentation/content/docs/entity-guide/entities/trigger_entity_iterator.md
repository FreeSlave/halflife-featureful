---
bookHidden: true
bookToC: false
---

# ![](/images/svencoop.png) trigger_entity_iterator

[svenmanor](https://www.svenmanor.com/entity-guide/trigger_entity_iterator)

This entity allows you to simulate one or multiple entities triggering a specific other entity. For example, you can have all entities of classname "monster_zombie" trigger a [trigger_configure_monster]({{< ref trigger_configure_monster >}}). The triggered entity will receive every iterated entity as activator.

{{% hint info %}}
The `Run Mode` parameter from Sven Co-op is not implemented yet.
{{% /hint %}}

### Parameters

* `Filter Entities by Name` - any entity/entities by the given targetname will be iterated, given they also are of the specific class set in `Filter Entities by Classname` (if set).
* `Filter Entities by Classname` - any entity/entities by the given classname will be iterated, given they also match the name set in `Filter Entities by Name` (if set).
* `Filter Entities by Status` - allows to exclude living or dead entities from iterating over.
* `Entity's Trigger Target` - trigger on behalf of the iterated entity.
* `Entity's Trigger State` - the use-type to send on trigger.
* `Trigger at end of each Run` - trigger something after the full run has been completed. The original activator is forwarded.
* `Maximum Runs (0 for unlimited)` - specify how many complete iteration-passes may be performed before this entity is removed from the game.

### Reaction to use-type

The use-type is ignored.
