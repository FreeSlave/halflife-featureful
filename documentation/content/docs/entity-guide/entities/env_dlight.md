---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) env_dlight

Creates a dynamic light.

{{% hint warning %}}
Maximum 32 dynamic lights can exist at any given time. Note that player's flashlight produces one dynamic light and some monsters (e.g. alien slaves) can produce dynamic lights as well.
{{% /hint %}}

### Spawnflags

* `Track position updates` makes the dynamic light update the position when the calculated Position is changing.
* `Position is Locus Entity` makes treating the Position parameter as Locus Entity instead of Locus Position. The resulting light position is the entity origin. When Track position updates is ticked using the entity as a reference point may produce better results (smooth movement of the dynamic light), at the expense of not letting to set the arbitrary position.

### Reaction to use-type

* Handles use-types to turn the light off or on.
* It's not possible to prolong the dynamic light lifetime when it's already active.
