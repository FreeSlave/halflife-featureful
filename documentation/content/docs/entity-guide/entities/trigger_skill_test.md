---
bookHidden: true
bookToC: false
---

# trigger_skill_test

Compares the current game difficulty to the specified one and fires a target accordingly to the result.

### Parameters

* `Comparison to pass` - the comparison type
    - `Equal`
    - `Greater or equal`
    - `Greater`
    - `Less or equal`
    - `Less`
    - `Not equal`
* `Compare skill to` - the value the current difficulty is getting compared to.
    - `Easy`
    - `Medium`
    - `Hard`
* `Fire on comparison pass` - fire when comparison is evaluated to true.
* `Fire on comparison fail` - fire when comparison is evaluated to false.
* `Fire on Easy` - fire when the current game difficulty is Easy.
* `Fire on Medium` - fire when the current game difficulty is Medium.
* `Fire on Hard` - fire when the current game difficulty is Hard.
