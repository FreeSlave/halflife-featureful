---
bookHidden: true
bookToC: false
---

# ![](/images/opfor.png) monster_shockroach

### Skill variables

* **sk_shockroach_health** - monster's health.
* **sk_shockroach_dmg_bite** - bite damage.
* **sk_shockroach_lifespan** - lifespan after spawn, in seconds. Negative values mean an infinite lifespan.

{{% hint info %}}
In Opposing Force shockroaches don't take damage during the first two seconds after the spawn. In Featureful they do take damage for this period, but only the non-lethal one (i.e. the health can't go under 1).

If **sk_shockroach_lifespan** is set to a negative value (which means an infinite lifespan) this rule doesn't apply.
{{% /hint %}}

### Default classification

`Alien Prey`

{{% hint warning %}}
This makes pitdrones attack shockroaches. The behavior retained from Opposing Force.
{{% /hint %}}

### Default display name

`Shock Roach`

### Soundscripts

* **Shockroach.Idle** - idle sounds.
* **Shockroach.Alert** - alert sounds.
* **Shockroach.Pain** - pain sounds.
* **Shockroach.Die** - death sounds.
* **Shockroach.Leap** - jump sound.
* **Shockroach.Attack** - attack sound on jump.
* **Shockroach.Bite** - hit something.

Note: the shockroach uses a lot of sounds in the sequence events. These sounds can't be changed via soundscripts.

### Animation events

* `2` - occasionally play **Shockroach.Attack** soundscript.

### Entity template examples

{{% tabs %}}

{{% tab "Race X relationship classification" %}}
In Half-Life: Opposing Force shockroaches have the same classification as headcrabs which makes pitdrones attack them. Set custom default classification to avoid this behavior.
```json
{
    "monster_shockroach": {
        "classify": "Race X Shock"
    }
}
```
{{% /tab %}}

{{% tab "Take Damage rules" %}}
Uses same rules as [monster_headcrab]({{< ref monster_headcrab >}}).
{{% /tab %}}

{{% /tabs %}}
