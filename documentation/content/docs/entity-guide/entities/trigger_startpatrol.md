---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) trigger_startpatrol

Sends a monster to patrol on the specified path consisted of `path_corner` entities.

Unlike scripted sequences, patrolling doesn't change the monster's AI state: the monster will still see enemies, listen to sounds and act accordingly.

{{% hint info %}}
To make monster run instead of walking set the speed in `path_corner` to 200 or higher.
{{% /hint %}}

### Parameters

* `Target monster [LE]` - targetname of the monster to send to patrol. This will affect only one monster even if there're several ones with the same name.
* `Patrol path start` - name of the first `path_corner` in the patrol path.
