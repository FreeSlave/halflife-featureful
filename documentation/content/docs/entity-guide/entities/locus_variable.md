---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) locus_variable

When triggered it calculates and stores the specified Position, Velocity and Ratio. Other entities then can use the recorded values via `LP`, `LV` and `LR` parameters.

If `Child's name` is provided, then a temporary entity is created and used to record the calculated values. The child exists for the specified time duration. The provided name can be used by other entities to use the recorded values.

`Fire on spawn` sends the `locus_variable` itself or the created child as an activator.
