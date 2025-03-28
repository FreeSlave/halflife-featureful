---
bookHidden: true
bookToC: false
---

# scripted_schedule

Allows to request a monster to play a schedule from a limited set of schedules. The schedule is a list of tasks the monster performs as part of its AI.

{{% hint warning %}}
Currently monsters don't keep their schedules between save-loads. So the monster will forget the schedule set via `scripted_schedule` on the reload.
{{% /hint %}}

{{% hint info %}}
`scripted_schedule` is just a suggestion. The monster might delay playing the schedule and it can be interrupted by the monster's AI conditions.
{{% /hint %}}

### Parameters

* `Affected Monster` - name of the monster to suggest an AI schedule to.
* `Suggested Schedule` - the schedule type to suggest.
    - `Clear suggested schedule` - clear previously suggested schedule (if any).
    - `Move away from spot` - just move away for some distance.
    - `Cover or retreat from spot` - try to cover from entity (i.e. find a place where the entity can't "see" the monster).
    - `Investigate spot` - move to spot, then move back to the original position.
    - `Turn to spot`
    - `Move to spot`
* `Min distance` - minimum distance for schedule, when applicable. E.g. the minimum distance to go away from the spot. If not specified or zero, the schedule will use its default value.
* `Max distance` - maximum distance for schedule, when applicable. E.g. the maximum distance to go away from the spot. If not specified or zero, the schedule will use its default value.
* `Spot entity` - entity used as a spot for schedule. The effect depends on the schedule. If it's blank, then monster will use itself as a spot or this `scripted_schedule` if `Suggested Schedule` is `Investigate spot` or `Turn to spot`.
* `Move preference` - a preferrable move activity (Auto, Walk, Run) where Auto means to chose the default move activity depending on the schedule. E.g. for "Cover or retreat from spot" it's "Run", while for others it's "Walk".
    - `Auto` - chose the default move activity depending on the schedule. E.g. for `Cover or retreat from spot` it's Run, while for others it's Walk.
    - `Walk`
    - `Run`
* `Spot preference` - treatment of the spot entity.
    - `Auto` - act as `Entity` if spot entity still exists. Otherwise acts as `Position`.
    - `Position` - calculate position of spot entity once and save it as spot for the schedule. This might matter if the spot entity itself can move (e.g. if it's another monster).
    - `Entity` - requires the presense of the spot entity at the time the schedule is played. 
