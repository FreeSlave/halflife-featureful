---
bookHidden: true
bookToC: false
---

# monster_generic_dead

Generic dead monster with custom model and configurable pose. Unlike other `monster_*_dead` entities this doesn't have a predefined list of death animations, so you need to specify the sequence name or activity.

### Parameters

* `Pose` - name of the sequence. If this is defined the `Death Type` is not used.
* `Death Type` - selection of the death activity. If there's no sequence with such activity in the model, the `ACT_DIESIMPLE` is used.
