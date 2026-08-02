---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) calc_subvelocity

Calculates a vector value based on the velocity or angles of another entity. Can be used in `LV` parameters.

### Parameters 

* `Entity to use [LE]`
* `Value to calculate from`
* * `Movement Velocity` - get entity velocity.
* * `Angle` - get direction from entity angles.
* * `View Angle` - get direction from entity (e.g. player) view angles.
* * `Calc Velocity [LV]` - get Calc Velocity of another entity. This allows to use the result of another `calc_subvelocity` as a base. You must ensure that the evaluation chain won't lead to recursion!
* `Scale factor [LR]` - scale by this factor after getting the base vector and after possible normalization. If not set the value 1 is used.
* `Add offset [LV]` - add offset after (possible) normalization and scale factor have been applied. If not set the vector 0 0 0 is used (i.e. not offset).

### Spawnflags

* `Normalize before scaling` - normalize vector before applying scale and offset.
* `Flip Vertical` - invert Z component in the result vector.
* `Discard X` - set X component to 0 in the result vector.
* `Discard Y` - set Y component to 0 in the result vector.
* `Discard Z` - set Z component to 0 in the result vector.
