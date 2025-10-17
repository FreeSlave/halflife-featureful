---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) monster_target

A fake monster that acts as a target for other monsters (same as [npc_bullseye](https://developer.valvesoftware.com/wiki/Npc_bullseye) in Source games). It doesn't have a visible model. The entity can be disabled and enabled by a trigger. When disabled, the monsters won't see this entity as an enemy.

{{% hint info %}}
Use this entity to make other monsters shoot at its direction. As the entity is non-solid the bullets and projectiles fired by monsters will hit anything behind it.
{{% /hint %}}

### Parameters

* `When active, count as` - a monster's classification. Only those monsters who see this class as enemy will 'attack' the entity.

### Spawnflags

* `Start inactive` - start disabled.

### Reaction to use-type

Toggles the monster target depending on the input use-type.
